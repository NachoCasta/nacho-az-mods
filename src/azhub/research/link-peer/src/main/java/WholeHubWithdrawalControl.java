/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.*;
import java.nio.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import org.deepsymmetry.beatlink.*;
import org.deepsymmetry.beatlink.dbserver.Client;

/** Independent discovery/session lifetime, plus explicit repository RPC checks. */
public final class WholeHubWithdrawalControl {
    static Set<Integer> visible(DeviceFinder finder) {
        Set<Integer> result=new HashSet<>();
        for(var device:finder.getCurrentDevices()) {
            int index=device.getDeviceNumber()-41;
            if(index>=0 && index<8 && device.getAddress().getHostAddress().equals(LiveProviderControl.sourceAddress(index))) result.add(index);
        }
        return result;
    }
    static int port(int source) throws Exception {
        try(Socket query=new Socket(LiveProviderControl.sourceAddress(source),12523)) {
            query.setSoTimeout(3000);
            query.getOutputStream().write(new byte[]{0,0,0,15,'R','e','m','o','t','e','D','B','S','e','r','v','e','r',0});
            byte[] reply=query.getInputStream().readNBytes(2);
            if(reply.length!=2) throw new AssertionError("Port lookup failed");
            return ((reply[0]&255)<<8)|(reply[1]&255);
        }
    }
    public static void main(String[] args) throws Exception {
        if(args.length!=0) throw new IllegalArgumentException("No arguments");
        LiveProviderControl.validateNetwork();
        DeviceFinder finder=DeviceFinder.getInstance();
        List<Socket> sessions=new ArrayList<>();List<Client> clients=new ArrayList<>();
        byte[][] roots=new byte[8][];int[] ports=new int[8];
        LoadedDeckStatusControl.LifecycleStatus loaded=null;
        try(DatagramSocket rpc=new DatagramSocket()) {
            rpc.setSoTimeout(2000);finder.start();long deadline=System.nanoTime()+15_000_000_000L;
            while(visible(finder).size()!=8 && System.nanoTime()<deadline) Thread.sleep(25);
            if(visible(finder).size()!=8) throw new AssertionError("Eight sources required");
            var constructor=Client.class.getDeclaredConstructor(Socket.class,int.class,int.class);constructor.setAccessible(true);
            Set<String> handles=new HashSet<>();
            for(int source=0;source<8;++source) {
                String address=LiveProviderControl.sourceAddress(source);ports[source]=port(source);
                Socket metadata=new Socket(address,ports[source]);sessions.add(metadata);metadata.setSoTimeout(2000);
                clients.add(constructor.newInstance(metadata,41+source,3));
                byte[] mount=ProviderLifecycleControl.rpc(rpc,50111,100005,1,1,
                    ProviderLifecycleControl.opaque("/C/".getBytes(StandardCharsets.UTF_16LE)),address);
                if(mount.length!=36 || ByteBuffer.wrap(mount).getInt()!=0) throw new AssertionError("MOUNT failed");
                roots[source]=Arrays.copyOfRange(mount,4,36);
                if(!handles.add(Base64.getEncoder().encodeToString(roots[source]))) throw new AssertionError("Cross-source root identity reused");
            }
            for(int source=0;source<8;++source) for(int other=0;other<8;++other) {
                byte[] reply=ProviderLifecycleControl.rpc(rpc,50111,100003,2,1,roots[other],LiveProviderControl.sourceAddress(source));
                if(ByteBuffer.wrap(reply).getInt()!=(source==other?0:70)) throw new AssertionError("Cross-source root admission");
            }
            ProviderLifecycleControl.loadedFile(rpc,roots[0]);
            loaded=LoadedDeckStatusControl.startForLifecycle();loaded.check();
            System.out.println("READY: eight independent discoveries/sessions, unique roots,56 foreign-handle STALE checks and real loaded-file/status control");System.out.flush();
            ProviderLifecycleControl.signal('W');loaded.check();rpc.setSoTimeout(200);
            for(int source=0;source<8;++source) {
                String address=LiveProviderControl.sourceAddress(source);
                for(int alias:new int[]{111,50111}) {
                    try { ProviderLifecycleControl.rpc(rpc,alias,100003,2,1,roots[source],address);throw new AssertionError("RPC endpoint retained"); }
                    catch(AssertionError timeout) { if(!"RPC timeout".equals(timeout.getMessage())) throw timeout; }
                }
                try { if(sessions.get(source).getInputStream().read()!=-1) throw new AssertionError("Metadata retained"); }
                catch(SocketException closed) { /* Reset also closes the session. */ }
                for(int endpoint:new int[]{12523,ports[source]}) {
                    try(Socket probe=new Socket()) {
                        probe.connect(new InetSocketAddress(address,endpoint),300);
                        throw new AssertionError("TCP endpoint retained");
                    } catch(ConnectException | SocketTimeoutException closed) { /* Endpoint withdrawn. */ }
                }
            }
            deadline=System.nanoTime()+20_000_000_000L;
            while(!visible(finder).isEmpty() && System.nanoTime()<deadline) Thread.sleep(50);
            if(!visible(finder).isEmpty()) throw new AssertionError("Withdrawn sources still discovered");
            loaded.check();
            System.out.println("PASS: all eight RPC aliases, metadata sessions, query endpoints and independent discoveries withdrawn");
        } finally {
            for(Socket session:sessions) try { session.close(); } catch(Exception ignored) { }
            try { if(loaded!=null) loaded.close(); } finally { finder.stop(); }
        }
    }
}
