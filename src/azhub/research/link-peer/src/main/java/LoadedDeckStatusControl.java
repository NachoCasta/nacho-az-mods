/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.*;
import java.nio.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.concurrent.atomic.AtomicReference;
import org.deepsymmetry.beatlink.*;

/** Synthetic loaded-deck status control, not a CDJ or an audio renderer.
 * Uses unchanged Beat Link discovery, packet framing/template and status parser.
 * Repository code fills public source/track fields missing from VirtualCdj's API.
 * All live traffic is restricted to the existing isolated emulator TAP. */
public final class LoadedDeckStatusControl {
    static final int TRACK=1884;
    static final String FILE="Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3";
    static void verifyFixture(String path) throws Exception {
        try(var db=new org.deepsymmetry.cratedigger.Database(new java.io.File(path))) {
            var row=db.trackIndex.get((long)TRACK);
            if(row==null || !org.deepsymmetry.cratedigger.Database.getText(row.filePath()).equals("/"+FILE))
                throw new AssertionError("Loaded status ID does not identify the retained MP3");
            if(DbLibraryControl.SLOT!=CdjStatus.TrackSourceSlot.COLLECTION)
                throw new AssertionError("Public provider/client collection representation differs");
            System.out.println("PASS: independent PDB parser binds loaded status track"+TRACK+" to retained MP3 path; public COLLECTION4/native USB3");
        }
    }
    // Model string matches the legacy status template; this is still synthetic.
    static final String MODEL="CDJ-2000nexus";
    static byte[] template() throws Exception {
        var field=VirtualCdj.class.getDeclaredField("STATUS_PAYLOAD");
        field.setAccessible(true);
        return ((byte[])field.get(null)).clone();
    }
    static DatagramPacket packet(int source,int serial) throws Exception {
        if((source!=1 && source!=2 && (source<41 || source>48)) || serial<1)
            throw new IllegalArgumentException("Bounded fixture source and positive packet sequence required");
        byte[] payload=template();
        payload[0x02]=3;payload[0x05]=3;
        payload[0x08]=1;payload[0x09]=(byte)source;
        payload[0x0a]=(byte)(source<=2 ? 3:4);payload[0x0b]=1;
        Util.numberToBytes(TRACK,payload,0x0d,4);
        payload[0x5c]=3;payload[0x6a]=(byte)0xc4;payload[0x6c]=0x7a;
        Util.numberToBytes(12000,payload,0x73,2);
        payload[0x7e]=9;payload[0x7f]=0;payload[0x80]=(byte)0xff;
        Util.numberToBytes(serial,payload,0x81,4);payload[0x87]=(byte)(1+(serial-1)%4);
        Util.numberToBytes(serial,payload,0xa9,4);
        byte[] name=Arrays.copyOf(MODEL.getBytes(StandardCharsets.US_ASCII),20);
        DatagramPacket result=Util.buildPacket(Util.PacketType.CDJ_STATUS,ByteBuffer.wrap(name),ByteBuffer.wrap(payload));
        // Decode the emitted public fields with the unmodified independent parser.
        DatagramPacket observed=new DatagramPacket(result.getData(),result.getLength(),InetAddress.getByName("192.0.2.2"),50002);
        CdjStatus decoded=new CdjStatus(observed);
        if(decoded.getDeviceNumber()!=3 || decoded.getTrackSourcePlayer()!=source ||
                decoded.getTrackSourceSlot()!=(source<=2 ? CdjStatus.TrackSourceSlot.USB_SLOT:CdjStatus.TrackSourceSlot.COLLECTION) ||
                decoded.getTrackType()!=CdjStatus.TrackType.REKORDBOX ||
                decoded.getRekordboxId()!=TRACK || !decoded.isTrackLoaded() || !decoded.isPlaying())
            throw new AssertionError("Independent parser did not confirm loaded/playing source identity");
        result.setPort(50002);return result;
    }
    /** Shares the lifecycle client's already-running DeviceFinder listener. */
    static final class LifecycleStatus implements AutoCloseable {
        volatile boolean stopping;
        volatile Throwable failure;
        volatile int sent;
        final java.util.concurrent.CountDownLatch ready=new java.util.concurrent.CountDownLatch(1);
        final Thread worker;
        LifecycleStatus() {
            worker=new Thread(()-> {
                VirtualCdj cdj=VirtualCdj.getInstance();
                try {
                    LiveProviderControl.validateNetwork();
                    for(var device:DeviceFinder.getInstance().getCurrentDevices())
                        if(device.getDeviceNumber()==3) throw new IllegalStateException("Fixture player3 occupied");
                    cdj.setDeviceName(MODEL);cdj.setDeviceNumber((byte)3);cdj.setUseStandardPlayerNumber(true);
                    if(!cdj.start() || cdj.getDeviceNumber()!=3 || !cdj.getLocalAddress().getHostAddress().equals("192.0.2.2") || cdj.isSendingStatus())
                        throw new IllegalStateException("Unexpected lifecycle status assignment");
                    var field=VirtualCdj.class.getDeclaredField("socket");field.setAccessible(true);
                    @SuppressWarnings("unchecked")
                    DatagramSocket socket=((AtomicReference<DatagramSocket>)field.get(cdj)).get();
                    if(socket==null || socket.getLocalPort()!=50002) throw new IllegalStateException("Status socket missing");
                    long deadline=System.nanoTime()+180_000_000_000L;
                    while(!stopping && System.nanoTime()<deadline) {
                        int serial=sent+1;
                        for(String target:new String[]{LiveProviderControl.sourceAddress(0),"192.0.2.15"}) {
                            DatagramPacket frame=packet(41,serial);frame.setAddress(InetAddress.getByName(target));socket.send(frame);
                        }
                        sent=serial;ready.countDown();Thread.sleep(200);
                    }
                    if(!stopping) throw new IllegalStateException("Loaded lifecycle status deadline exceeded");
                } catch(Throwable error) { failure=error;ready.countDown(); }
                finally { cdj.stop(); }
            },"loaded-lifecycle-status");
            worker.setDaemon(true);
        }
        void check() { if(failure!=null) throw new IllegalStateException("Loaded lifecycle status failed",failure); }
        public void close() throws Exception {
            stopping=true;worker.join(10000);check();
            if(worker.isAlive()) throw new IllegalStateException("Loaded status worker did not stop");
            if(sent<10) throw new IllegalStateException("Insufficient loaded status samples");
            System.out.println("LOADED_STATUS_COMPLETE: packets_per_destination="+sent+"; no manual external unload");System.out.flush();
        }
    }
    static LifecycleStatus startForLifecycle() throws Exception {
        LifecycleStatus control=new LifecycleStatus();control.worker.start();
        if(!control.ready.await(20,java.util.concurrent.TimeUnit.SECONDS)) {
            control.stopping=true;throw new IllegalStateException("Loaded status startup timed out");
        }
        control.check();
        System.out.println("LOADED_STATUS_READY: shared DeviceFinder, source41/collection4/track"+TRACK+" to provider and native AZ");System.out.flush();
        return control;
    }
    static void selfTest() throws Exception {
        byte[] original=template();
        for(int source:new int[]{1,2,41,42,43,44,45,46,47,48}) {
            packet(source,1);packet(source,2);
        }
        if(!Arrays.equals(original,template())) throw new AssertionError("Library template mutated");
        for(int source:new int[]{0,3,4,40,49,255}) {
            try { packet(source,1);throw new AssertionError("Unbounded source accepted"); }
            catch(IllegalArgumentException expected) { }
        }
        System.out.println("PASS: unchanged Beat Link parser confirms native USB3/public COLLECTION4 loaded track identity; library template unchanged");
    }
    @SuppressWarnings("unchecked")
    public static void main(String[] args) throws Exception {
        if(args.length==1 && args[0].equals("--self-test")) { selfTest();return; }
        if(args.length==2 && args[0].equals("--verify-fixture")) { verifyFixture(args[1]);return; }
        boolean controlled=args.length==3 && args[2].equals("--also-native-controlled");
        if(args.length!=2 && !controlled) throw new IllegalArgumentException("--native-source1 | --native-source2 | --emulator-link-N SECONDS");
        int source;String target;
        if(args[0].equals("--native-source1") || args[0].equals("--native-source2")) {
            source=args[0].endsWith("1") ? 1:2;target="192.0.2.15";
        } else {
            int index=LiveProviderControl.sourceIndex(args[0]);
            source=LiveProviderControl.sourceNumber(index);target=LiveProviderControl.sourceAddress(index);
            if(!target.matches("192\\.0\\.2\\.(?:[3-9]|10)")) throw new IllegalArgumentException("Documentation-address fixture only");
        }
        int seconds=Integer.parseInt(args[1]);
        if(seconds<5 || seconds>180) throw new IllegalArgumentException("Duration5..180 seconds");
        LiveProviderControl.validateNetwork();
        if(!NetworkInterface.getByName("azlink0").getInetAddresses().asIterator().hasNext())
            throw new IllegalStateException("Isolated TAP missing");
        DeviceFinder finder=DeviceFinder.getInstance();VirtualCdj cdj=VirtualCdj.getInstance();
        int sent=0;
        try {
            finder.start();
            for(var device:finder.getCurrentDevices()) if(device.getDeviceNumber()==3)
                throw new IllegalStateException("Fixture player3 is already occupied");
            cdj.setDeviceName(MODEL);cdj.setDeviceNumber((byte)3);cdj.setUseStandardPlayerNumber(true);
            if(!cdj.start() || cdj.getDeviceNumber()!=3 || !cdj.getLocalAddress().getHostAddress().equals("192.0.2.2"))
                throw new IllegalStateException("Unexpected fixture player assignment");
            // Keep native status generation disabled: it reports its own source.
            if(cdj.isSendingStatus()) throw new IllegalStateException("Conflicting automatic status sender");
            var field=VirtualCdj.class.getDeclaredField("socket");field.setAccessible(true);
            DatagramSocket socket=((AtomicReference<DatagramSocket>)field.get(cdj)).get();
            if(socket==null || socket.getLocalPort()!=50002 || !socket.getLocalAddress().getHostAddress().equals("192.0.2.2"))
                throw new IllegalStateException("Unexpected status socket");
            long until=System.nanoTime()+seconds*1_000_000_000L;
            while(System.nanoTime()<until && (!controlled || System.in.available()==0)) {
                DatagramPacket status=packet(source,++sent);status.setAddress(InetAddress.getByName(target));socket.send(status);
                if(controlled && !target.equals("192.0.2.15")) {
                    DatagramPacket nativeStatus=packet(source,sent);
                    nativeStatus.setAddress(InetAddress.getByName("192.0.2.15"));socket.send(nativeStatus);
                }
                if(sent==1) {
                    System.out.println("READY: synthetic player3 loaded/playing track"+TRACK+" from source"+source+"/slot"+(source<=2?3:4)+"; no audio renderer");System.out.flush();
                }
                Thread.sleep(200);
            }
            if(controlled && (System.in.available()==0 || System.in.read()!='S'))
                throw new AssertionError("Missing explicit end-of-control signal after withdrawal/reconnect");
            System.out.println("PASS: synthetic loaded/playing status remained advertised; packets="+sent+" source="+source);
        } finally { cdj.stop();finder.stop(); }
    }
}
