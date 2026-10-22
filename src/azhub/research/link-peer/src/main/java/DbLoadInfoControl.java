/* SPDX-License-Identifier: GPL-3.0-only */
import java.io.*;
import java.net.*;
import java.nio.channels.Channels;
import java.nio.file.Path;
import java.util.*;
import org.deepsymmetry.beatlink.dbserver.*;
import org.deepsymmetry.cratedigger.Database;

/** Local0x2102 control using unchanged Beat Link message codecs and handshake. */
public final class DbLoadInfoControl {
    static long number(Message row,int index) { return ((NumberField)row.arguments.get(index)).getValue(); }
    public static void main(String[] args) throws Exception {
        if(args.length!=3) throw new IllegalArgumentException("LOCAL_PORT PDB TRACK_ID");
        long id=Long.parseLong(args[2]);
        try(Socket socket=new Socket("127.0.0.1",Integer.parseInt(args[0])); Database db=new Database(Path.of(args[1]).toFile())) {
            socket.setSoTimeout(5000);
            var constructor=Client.class.getDeclaredConstructor(Socket.class,int.class,int.class); constructor.setAccessible(true);
            Client client=constructor.newInstance(socket,41,3);
            var track=db.trackIndex.get(id); if(track==null) throw new AssertionError("Missing input track");
            var output=Channels.newChannel(socket.getOutputStream()); var input=new DataInputStream(socket.getInputStream());
            new Message(10,0x2102,new NumberField(0x03010401),new NumberField(id)).write(output);
            Message offer=Message.read(input);
            if(offer.messageType.getValue()!=0x4000 || number(offer,0)!=0x2102 || number(offer,1)!=7) throw new AssertionError("Load-info offer");
            new Message(11,0x3000,new NumberField(0x03010401),new NumberField(0),new NumberField(7)).write(output);
            if(Message.read(input).messageType.getValue()!=0x4001) throw new AssertionError("Missing header");
            List<Message> rows=new ArrayList<>();
            for(int i=0;i<7;++i) {
                Message row=Message.read(input);
                if(row.messageType.getValue()!=0x4101 || row.transaction.getValue()!=11) throw new AssertionError("Item envelope");
                rows.add(row);
            }
            if(Message.read(input).messageType.getValue()!=0x4201) throw new AssertionError("Missing footer");
            if(number(rows.get(0),1)!=1 || number(rows.get(0),6)!=4 ||
               number(rows.get(1),1)!=track.duration() || number(rows.get(2),1)!=track.tempo() ||
               !((StringField)rows.get(3).arguments.get(3)).getValue().equals(Database.getText(track.comment())) ||
               number(rows.get(4),0)!=track.fileSize() || number(rows.get(4),1)!=id ||
               !((StringField)rows.get(4).arguments.get(3)).getValue().equals(Database.getText(track.filePath())) ||
               number(rows.get(5),1)!=1 || number(rows.get(5),6)!=0x2f || number(rows.get(6),1)!=track.keyId())
                throw new AssertionError("Load-info values differ from independent PDB");
            System.out.println("PASS: Beat Link decodes seven MP3 load-info rows with exact PDB path/size/duration/tempo/comment/key ID");
            var close=Client.class.getDeclaredMethod("close"); close.setAccessible(true); close.invoke(client);
        }
    }
}
