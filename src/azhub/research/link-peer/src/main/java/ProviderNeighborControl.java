/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.*;
import java.nio.*;
import java.util.*;
import java.util.concurrent.TimeUnit;
import org.deepsymmetry.beatlink.*;
import org.deepsymmetry.beatlink.dbserver.*;

/** Two live exports: independent Beat Link menus/discovery plus explicit
 * repository RPC probes. Keep the neighbor active while source0 withdraws/recovers. */
public final class ProviderNeighborControl {
    static String neighborAddress() { return LiveProviderControl.sourceAddress(ProviderLifecycleControl.neighborIndex()); }
    static byte[] rpc(DatagramSocket socket,int port,int program,int version,int procedure,byte[] arguments,String target) throws Exception {
        return ProviderLifecycleControl.rpc(socket,port,program,version,procedure,arguments,target);
    }
    static byte[] root(DatagramSocket socket,String target) throws Exception {
        byte[] mounted=rpc(socket,50111,100005,1,1,new byte[]{0,0,0,2,'/','C',0,0},target);
        if(mounted.length!=36 || ByteBuffer.wrap(mounted).getInt()!=0) throw new AssertionError("Root mount");
        return Arrays.copyOfRange(mounted,4,36);
    }
    static void alive(Client client,DatagramSocket socket,byte[] handle) throws Exception {
        for(int port:new int[]{111,50111}) {
            byte[] reply=rpc(socket,port,100003,2,1,handle,neighborAddress());
            if(reply.length<4 || ByteBuffer.wrap(reply).getInt()!=0) throw new AssertionError("Neighbor handle retired");
        }
        if(!client.tryLockingForMenuOperations(5,TimeUnit.SECONDS)) throw new AssertionError("Menu lock");
        try {
            if(DbLibraryControl.browse(client,Message.KnownType.ROOT_MENU_REQ,new NumberField(0),new NumberField(0xffffff)).size()!=2)
                throw new AssertionError("Neighbor menus unavailable");
        } finally { client.unlockForMenuOperations(); }
    }
    static void signal(int expected,Client client,DatagramSocket socket,byte[] handle) throws Exception {
        int seconds=Integer.getInteger("azhub.lifecycleWaitSeconds",45);
        if(seconds<45 || seconds>100) throw new IllegalArgumentException("Lifecycle wait45..100 seconds");
        long deadline=System.nanoTime()+seconds*1_000_000_000L,next=0;
        while(System.in.available()==0 && System.nanoTime()<deadline) {
            if(System.nanoTime()>=next) { alive(client,socket,handle); next=System.nanoTime()+500_000_000L; }
            Thread.sleep(10);
        }
        if(System.in.available()==0 || System.in.read()!=expected) throw new AssertionError("Missing neighbor lifecycle signal");
        alive(client,socket,handle);
    }
    public static void main(String[] args) throws Exception {
        if(args.length!=1) throw new IllegalArgumentException("SECOND_METADATA_PORT");
        LiveProviderControl.validateNetwork();
        try(DatagramSocket datagram=new DatagramSocket(); Socket metadata=new Socket(neighborAddress(),Integer.parseInt(args[0]))) {
            datagram.setSoTimeout(2000); metadata.setSoTimeout(5000);
            byte[] first=root(datagram,LiveProviderControl.sourceAddress(0)),second=root(datagram,neighborAddress());
            if(Arrays.equals(first,second)) throw new AssertionError("Shared root identity");
            for(int port:new int[]{111,50111}) {
                if(!Arrays.equals(rpc(datagram,port,100003,2,1,first,neighborAddress()),ProviderLifecycleControl.words(70)) ||
                   !Arrays.equals(rpc(datagram,port,100003,2,1,second,LiveProviderControl.sourceAddress(0)),ProviderLifecycleControl.words(70)))
                    throw new AssertionError("Cross-source handle accepted");
            }
            var constructor=Client.class.getDeclaredConstructor(Socket.class,int.class,int.class); constructor.setAccessible(true);
            Client client=constructor.newInstance(metadata,41+ProviderLifecycleControl.neighborIndex(),3); alive(client,datagram,second);
            System.out.println("READY: two independent roots reject cross-source handles; neighbor session active"); System.out.flush();
            signal('W',client,datagram,second);
            System.out.println("WITHDRAWN: neighbor root and original TCP menu session survive source0 withdrawal"); System.out.flush();
            signal('R',client,datagram,second);
            if(!Arrays.equals(second,root(datagram,neighborAddress()))) throw new AssertionError("Neighbor root replaced");
            System.out.println("PASS: two-source root isolation and continuous neighbor RPC/TCP menus across withdrawal/recovery");
        }
    }
}
