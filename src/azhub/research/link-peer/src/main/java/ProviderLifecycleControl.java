/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.*;
import java.nio.*;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import org.deepsymmetry.beatlink.*;
import org.deepsymmetry.beatlink.dbserver.Client;

/** Isolated withdrawal control: independent DeviceFinder plus explicit RPC
 * probes. RPC checks are repository-authored, not an independent NFS client. */
public final class ProviderLifecycleControl {
    static int transaction=100;
    static byte[] words(int... values) {
        ByteBuffer bytes=ByteBuffer.allocate(values.length*4);
        for(int value:values) bytes.putInt(value);
        return bytes.array();
    }
    static byte[] rpc(DatagramSocket socket,int port,int program,int version,int procedure,byte[] arguments) throws Exception {
        return rpc(socket,port,program,version,procedure,arguments,LiveProviderControl.sourceAddress(0));
    }
    static byte[] rpc(DatagramSocket socket,int port,int program,int version,int procedure,byte[] arguments,String address) throws Exception {
        int xid=++transaction;
        byte[] header=words(xid,0,2,program,version,procedure,0,0,0,0);
        byte[] request=Arrays.copyOf(header,header.length+arguments.length);
        System.arraycopy(arguments,0,request,header.length,arguments.length);
        for(int retry=0;retry<3;++retry) {
            socket.send(new DatagramPacket(request,request.length,InetAddress.getByName(address),port));
            byte[] buffer=new byte[16384]; DatagramPacket reply=new DatagramPacket(buffer,buffer.length);
            try { socket.receive(reply); } catch(SocketTimeoutException timeout) { continue; }
            if(!reply.getAddress().getHostAddress().equals(address) || reply.getPort()!=port)
                throw new AssertionError("Unexpected RPC endpoint");
            if(reply.getLength()<24 || !Arrays.equals(Arrays.copyOf(buffer,24),words(xid,1,0,0,0,0)))
                throw new AssertionError("RPC reply header");
            return Arrays.copyOfRange(buffer,24,reply.getLength());
        }
        throw new AssertionError("RPC timeout");
    }
    static byte[] opaque(byte[] value) {
        ByteBuffer bytes=ByteBuffer.allocate(4+((value.length+3)&~3));
        bytes.putInt(value.length);bytes.put(value);return bytes.array();
    }
    static byte[] join(byte[] first,byte[] second) {
        byte[] result=Arrays.copyOf(first,first.length+second.length);
        System.arraycopy(second,0,result,first.length,second.length);return result;
    }
    static byte[] loadedFile(DatagramSocket socket,byte[] root) throws Exception {
        byte[] handle=root;
        for(String component:LoadedDeckStatusControl.FILE.split("/")) {
            byte[] reply=rpc(socket,50111,100003,2,4,join(handle,opaque(component.getBytes(StandardCharsets.UTF_16LE))));
            if(reply.length<36 || ByteBuffer.wrap(reply).getInt()!=0) throw new AssertionError("Loaded-file LOOKUP failed");
            handle=Arrays.copyOfRange(reply,4,36);
        }
        byte[] reply=rpc(socket,50111,100003,2,6,join(handle,words(0,1024,0)));
        if(reply.length<1100 || ByteBuffer.wrap(reply).getInt()!=0 || ByteBuffer.wrap(reply,72,4).getInt()!=1024)
            throw new AssertionError("Loaded-file first READ failed");
        byte[] hash=java.security.MessageDigest.getInstance("SHA-256").digest(Arrays.copyOfRange(reply,76,1100));
        StringBuilder hex=new StringBuilder();for(byte value:hash) hex.append(String.format("%02x",value&255));
        if(!hex.toString().equals("ef1f4332b398858cfd7c7cf46cba9a8325d08b1127bc55c251d2390b4ff3f58e"))
            throw new AssertionError("Loaded-file first block differs from fixture");
        System.out.println("FILE_READY: retained MP3 handle and exact1024-byte first block; track"+LoadedDeckStatusControl.TRACK+" fixture");System.out.flush();
        return handle;
    }
    static boolean discovered(DeviceFinder finder) {
        for(var device:finder.getCurrentDevices())
            if(device.getDeviceNumber()==41 && device.getAddress().getHostAddress().equals(LiveProviderControl.sourceAddress(0))) return true;
        return false;
    }
    static int neighborIndex() {
        int index=Integer.getInteger("azhub.neighborIndex",1);
        if(index<1 || index>7) throw new IllegalArgumentException("Neighbor index1..7");
        return index;
    }
    static boolean neighborIsVisible(DeviceFinder finder) {
        for(var device:finder.getCurrentDevices())
            if(device.getDeviceNumber()==41+neighborIndex() && device.getAddress().getHostAddress().equals(LiveProviderControl.sourceAddress(neighborIndex()))) return true;
        return false;
    }
    static void neighborVisible(DeviceFinder finder,boolean required) {
        if(required && !neighborIsVisible(finder)) throw new AssertionError("Neighbor discovery lost");
    }
    static void signal(int expected) throws Exception {
        int seconds=Integer.getInteger("azhub.lifecycleWaitSeconds",45);
        if(seconds<45 || seconds>100) throw new IllegalArgumentException("Lifecycle wait45..100 seconds");
        long deadline=System.nanoTime()+seconds*1_000_000_000L;
        while(System.in.available()==0 && System.nanoTime()<deadline) Thread.sleep(10);
        if(System.in.available()==0 || System.in.read()!=expected) throw new AssertionError("Missing lifecycle signal");
    }
    public static void main(String[] args) throws Exception {
        boolean neighbor=args.length==3 && args[2].equals("--neighbor");
        boolean drainOnly=args.length==2 && args[1].equals("--drain-only");
        boolean reconnect=drainOnly || (args.length==2 || neighbor) && args[1].equals("--retire-and-reconnect");
        if(args.length!=1 && !reconnect) throw new IllegalArgumentException("METADATA_PORT [--retire-and-reconnect | --drain-only]");
        LiveProviderControl.validateNetwork();
        DeviceFinder finder=DeviceFinder.getInstance();
        LoadedDeckStatusControl.LifecycleStatus loadedStatus=null;
        try(DatagramSocket datagram=new DatagramSocket(); Socket metadata=new Socket(LiveProviderControl.sourceAddress(0),Integer.parseInt(args[0]))) {
            datagram.setSoTimeout(2000); metadata.setSoTimeout(3000);
            finder.start();
            long deadline=System.nanoTime()+5_000_000_000L;
            while((!discovered(finder) || (neighbor && !neighborIsVisible(finder))) && System.nanoTime()<deadline) Thread.sleep(25);
            if(!discovered(finder)) throw new AssertionError("Source not initially visible");
            neighborVisible(finder,neighbor);
            for(int port:new int[]{111,50111}) {
                byte[] reply=rpc(datagram,port,100000,2,3,words(100003,2,17,0));
                if(!Arrays.equals(reply,words(111))) throw new AssertionError("Portmapper alias mismatch");
            }
            byte[] mount=rpc(datagram,50111,100005,1,1,new byte[]{0,0,0,2,'/','C',0,0});
            if(mount.length!=36 || ByteBuffer.wrap(mount).getInt()!=0) throw new AssertionError("Mount failed");
            byte[] handle=Arrays.copyOfRange(mount,4,36);
            byte[] fileHandle=Boolean.getBoolean("azhub.loadedFileControl")?loadedFile(datagram,handle):null;
            if(Boolean.getBoolean("azhub.loadedStatusControl")) loadedStatus=LoadedDeckStatusControl.startForLifecycle();
            var constructor=Client.class.getDeclaredConstructor(Socket.class,int.class,int.class);
            constructor.setAccessible(true); constructor.newInstance(metadata,41,3);
            System.out.println("READY: both RPC ports agree; source discovered and root handle retained"); System.out.flush();
            signal('W');
            if(loadedStatus!=null) loadedStatus.check();
            for(int port:new int[]{111,50111}) {
                if(reconnect) {
                    datagram.setSoTimeout(200);
                    try { rpc(datagram,port,100003,2,1,handle); throw new AssertionError("RPC endpoint retained after drain"); }
                    catch(AssertionError timeout) { if(!"RPC timeout".equals(timeout.getMessage())) throw timeout; }
                    if(fileHandle!=null) {
                        try { rpc(datagram,port,100003,2,6,join(fileHandle,words(0,1024,0)));throw new AssertionError("Loaded-file endpoint retained after drain"); }
                        catch(AssertionError timeout) { if(!"RPC timeout".equals(timeout.getMessage())) throw timeout; }
                    }
                } else {
                    if(!Arrays.equals(rpc(datagram,port,100003,2,1,handle),words(70))) throw new AssertionError("Old handle not STALE");
                    if(!Arrays.equals(rpc(datagram,port,100005,1,5,new byte[0]),words(0))) throw new AssertionError("Export still listed");
                }
            }
            try { if(metadata.getInputStream().read()!=-1) throw new AssertionError("Metadata connection retained"); }
            catch(SocketException closed) { /* Native reset also closes the owned session. */ }
            try(Socket query=new Socket()) {
                query.connect(new InetSocketAddress(LiveProviderControl.sourceAddress(0),12523),1000);
                query.setSoTimeout(3000);
                try {
                    query.getOutputStream().write("\0\0\0\17RemoteDBServer\0".getBytes(StandardCharsets.ISO_8859_1));
                    if(query.getInputStream().read()!=-1) throw new AssertionError("DB port still published");
                } catch(SocketException closed) { /* Rejected after withdrawal. */ }
            } catch(ConnectException | SocketTimeoutException closed) { if(!reconnect) throw closed; }
            deadline=System.nanoTime()+20_000_000_000L;
            while(discovered(finder) && System.nanoTime()<deadline) Thread.sleep(50);
            if(discovered(finder)) throw new AssertionError("DeviceFinder retained withdrawn source");
            neighborVisible(finder,neighbor);
            if(reconnect) {
                System.out.println("WITHDRAWN: RPC endpoints drained, metadata/query closed and DeviceFinder expired"); System.out.flush();
                if(drainOnly) {
                    System.out.println("PASS: native withdrawal drains retained RPC handle and metadata session; independent DeviceFinder expires source");
                    return;
                }
                signal('R'); datagram.setSoTimeout(2000);
                if(loadedStatus!=null) loadedStatus.check();
                deadline=System.nanoTime()+15_000_000_000L;
                while(!discovered(finder) && System.nanoTime()<deadline) Thread.sleep(25);
                if(!discovered(finder)) throw new AssertionError("Reconnected source not discovered");
                neighborVisible(finder,neighbor);
                for(int port:new int[]{111,50111})
                    if(!Arrays.equals(rpc(datagram,port,100003,2,1,handle),words(70))) throw new AssertionError("Old handle resolved after reconnect");
                if(fileHandle!=null) {
                    for(int port:new int[]{111,50111})
                        if(!Arrays.equals(rpc(datagram,port,100003,2,6,join(fileHandle,words(0,1024,0))),words(70)))
                            throw new AssertionError("Old loaded-file handle resolved after reconnect");
                    System.out.println("FILE_STALE: old loaded-file READ returns70 on both RPC aliases after reconnect");System.out.flush();
                }
                byte[] replacement=rpc(datagram,50111,100005,1,1,new byte[]{0,0,0,2,'/','C',0,0});
                if(replacement.length!=36 || ByteBuffer.wrap(replacement).getInt()!=0 || Arrays.equals(handle,Arrays.copyOfRange(replacement,4,36)))
                    throw new AssertionError("Replacement root handle not fresh");
                System.out.println("PASS: integrated retirement, rediscovery and old-handle STALE on both RPC aliases after reconnect");
            } else System.out.println("PASS: both RPC aliases return STALE/empty exports, TCP/port lookup close, unchanged DeviceFinder expires source");
        } finally { try { if(loadedStatus!=null) loadedStatus.close(); } finally { finder.stop(); } }
    }
}
