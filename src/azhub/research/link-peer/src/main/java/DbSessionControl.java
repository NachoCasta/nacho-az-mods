/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.*;
import org.deepsymmetry.beatlink.dbserver.Client;

/** Invoke the unchanged Beat Link handshake on an explicitly local socket. */
public final class DbSessionControl {
    public static void main(String[] args) throws Exception {
        if(args.length!=1) throw new IllegalArgumentException("LOCAL_PORT");
        try(Socket socket=new Socket()) {
            socket.connect(new InetSocketAddress(InetAddress.getLoopbackAddress(),Integer.parseInt(args[0])),2000);
            socket.setSoTimeout(2000);
            var constructor=Client.class.getDeclaredConstructor(Socket.class,int.class,int.class);
            constructor.setAccessible(true);
            Client client=constructor.newInstance(socket,41,3);
            if(!client.isConnected()) throw new AssertionError("Handshake failed");
            var close=Client.class.getDeclaredMethod("close"); close.setAccessible(true); close.invoke(client);
        }
        System.out.println("PASS: unchanged Beat Link Client handshake with C provider41/requester3");
    }
}
