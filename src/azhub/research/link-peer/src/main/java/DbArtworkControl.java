/* SPDX-License-Identifier: GPL-3.0-only */
import java.awt.image.BufferedImage;
import java.io.*;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.channels.Channels;
import java.nio.file.*;
import java.util.Arrays;
import javax.imageio.ImageIO;
import org.deepsymmetry.beatlink.dbserver.*;
import org.deepsymmetry.beatlink.data.AlbumArt;
import org.deepsymmetry.beatlink.Util;
/** Independent artwork codec/decoder control with an explicitly synthetic JPEG. */
public final class DbArtworkControl {
    public static void main(String[] args) throws Exception {
        if(args.length==2 && args[0].equals("--generate")) {
            for(int dimension:new int[]{80,240}) {
                BufferedImage image=new BufferedImage(dimension,dimension,BufferedImage.TYPE_INT_RGB);
                for(int y=0;y<dimension;++y) for(int x=0;x<dimension;++x) image.setRGB(x,y,((x*3)<<16)|((y*3)<<8)|((x^y)*3));
                String path=dimension==80 ? args[1] : Util.highResolutionPath(args[1]);
                if(!ImageIO.write(image,"jpg",Path.of(path).toFile())) throw new AssertionError("No JPEG encoder");
            }
            return;
        }
        if(args.length!=3) throw new IllegalArgumentException("LOCAL_PORT ART_ID JPEG_FILE");
        try(Socket socket=new Socket("127.0.0.1",Integer.parseInt(args[0]))) {
            socket.setSoTimeout(5000);
            var constructor=Client.class.getDeclaredConstructor(Socket.class,int.class,int.class); constructor.setAccessible(true);
            Client client=constructor.newInstance(socket,41,3);
            var output=Channels.newChannel(socket.getOutputStream()); var input=new DataInputStream(socket.getInputStream());
            new Message(10,0x2003,new NumberField(0x03080401),new NumberField(Integer.parseInt(args[1]))).write(output);
            Message reply=Message.read(input);
            if(reply.messageType.getValue()!=0x4002 || reply.transaction.getValue()!=10) throw new AssertionError("Artwork envelope");
            byte[] bytes=((BinaryField)reply.arguments.get(3)).getValueAsArray();
            if(!Arrays.equals(bytes,Files.readAllBytes(Path.of(args[2])))) throw new AssertionError("Artwork bytes differ");
            BufferedImage decoded=new AlbumArt(null,ByteBuffer.wrap(bytes)).getImage();
            if(decoded==null || decoded.getWidth()!=80 || decoded.getHeight()!=80) throw new AssertionError("Artwork decode");
            new Message(12,0x2003,new NumberField(0x03080401),new NumberField(Integer.parseInt(args[1])),NumberField.WORD_1).write(output);
            Message highReply=Message.read(input);
            if(highReply.messageType.getValue()!=0x4002 || highReply.transaction.getValue()!=12) throw new AssertionError("High artwork envelope");
            byte[] high=((BinaryField)highReply.arguments.get(3)).getValueAsArray();
            if(!Arrays.equals(high,Files.readAllBytes(Path.of(Util.highResolutionPath(args[2]))))) throw new AssertionError("High artwork bytes differ");
            BufferedImage highImage=new AlbumArt(null,ByteBuffer.wrap(high)).getImage();
            if(highImage==null || highImage.getWidth()!=240 || highImage.getHeight()!=240) throw new AssertionError("High artwork decode");
            new Message(11,0x2003,new NumberField(0x03080401),new NumberField(0)).write(output);
            Message missing=Message.read(input);
            if(missing.messageType.getValue()!=0x4002 || ((BinaryField)missing.arguments.get(3)).getValueAsArray().length!=0)
                throw new AssertionError("Missing artwork response");
            System.out.println("PASS: unchanged Beat Link codec and AlbumArt decode exact synthetic80x80 JPEG, bytes="+bytes.length+", missing ID empty");
            System.out.println("PASS: high-resolution path matches unchanged Util helper and exact synthetic240x240 JPEG, bytes="+high.length);
            var close=Client.class.getDeclaredMethod("close"); close.setAccessible(true); close.invoke(client);
        }
    }
}
