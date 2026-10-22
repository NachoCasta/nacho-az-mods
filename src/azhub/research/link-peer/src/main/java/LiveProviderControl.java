/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.*;
import java.nio.file.*;
import java.util.*;
import org.deepsymmetry.beatlink.*;

/** Independent discovery and complete menu control of the emulator export. */
public final class LiveProviderControl {
    public static int sourceIndex(String flag) {
        if(flag.equals("--emulator-link")) return 0;
        String prefix="--emulator-link-";
        if(flag.startsWith(prefix) && flag.length()==prefix.length()+1) {
            char index=flag.charAt(prefix.length());
            if(index>='1' && index<='8') return index-'1';
        }
        throw new IllegalArgumentException("Expected isolated emulator source1..8");
    }
    public static int sourceNumber(int index) {
        Integer override=Integer.getInteger("azhub.testSourceNumber");
        if(override!=null) {
            if(index!=0 || override<41 || override>48) throw new IllegalArgumentException("Bounded emulator source number required");
            return override;
        }
        return 41+index;
    }
    public static boolean fixturePeerAddress(String address) {
        return address.equals("192.0.2.2") || Boolean.getBoolean("azhub.testLinkLocalPeer") && address.equals("169.254.2.2");
    }
    private static boolean linkLocalCandidate(String address) {
        if(!address.matches("169\\.254\\.[0-9]{1,3}\\.[0-9]{1,3}")) return false;
        String[] parts=address.split("\\.");int third=Integer.parseInt(parts[2]),last=Integer.parseInt(parts[3]);
        return third>=1 && third<=254 && last<=255 && parts[2].equals(Integer.toString(third)) && parts[3].equals(Integer.toString(last));
    }
    public static String sourceAddress(int index) {
        if(index<0 || index>=8) throw new IllegalArgumentException("Source index");
        String selected=System.getProperty("azhub.testSourceAddress"+index);
        if(selected!=null) {
            if(!Boolean.getBoolean("azhub.testLinkLocalPeer") || !linkLocalCandidate(selected)) throw new IllegalArgumentException("Bounded isolated link-local source required");
            return selected;
        }
        String override=System.getProperty("azhub.testSourceAddress");
        if(override!=null) {
            if(index!=0 || !override.matches("192\\.0\\.2\\.(?:[3-9]|10)"))
                throw new IllegalArgumentException("Bounded single-source emulator address required");
            return override;
        }
        return "192.0.2."+(3+index);
    }
    public static void validateNetwork() throws Exception {
        if(Files.getAttribute(Path.of("/proc/self/ns/net"),"unix:ino").equals(
                Files.getAttribute(Path.of("/proc/1/ns/net"),"unix:ino")))
            throw new IllegalStateException("Initial namespace refused");
        int interfaces=0;
        for(var nic:Collections.list(NetworkInterface.getNetworkInterfaces())) {
            if(!nic.isUp() || nic.isLoopback()) continue;
            if(!nic.getName().equals("azlink0")) throw new IllegalStateException("Unexpected interface");
            ++interfaces;
            for(var address:Collections.list(nic.getInetAddresses()))
                if(address instanceof Inet4Address && !fixturePeerAddress(address.getHostAddress()))
                    throw new IllegalStateException("Unexpected address");
        }
        if(interfaces!=1) throw new IllegalStateException("Missing isolated TAP");
    }
    public static void main(String[] args) throws Exception {
        if(args.length!=2 && args.length!=3) throw new IllegalArgumentException("PDB ENTITY_ORACLE_TSV [--emulator-link-N]");
        int index=args.length==3 ? sourceIndex(args[2]) : 0;
        String address=sourceAddress(index);
        validateNetwork();
        DeviceFinder finder=DeviceFinder.getInstance();
        try {
            finder.start();
            long deadline=System.nanoTime()+15_000_000_000L;
            boolean found=false;
            while(!found && System.nanoTime()<deadline) {
                for(var device:finder.getCurrentDevices())
                    if(device.getDeviceNumber()==sourceNumber(index) && device.getAddress().getHostAddress().equals(address)) found=true;
                if(!found) Thread.sleep(50);
            }
            if(!found) throw new AssertionError("Source announcement not discovered");
            int port;
            try(Socket query=new Socket(address,12523)) {
                query.setSoTimeout(3000);
                query.getOutputStream().write(new byte[]{0,0,0,15,'R','e','m','o','t','e','D','B','S','e','r','v','e','r',0});
                byte[] response=query.getInputStream().readNBytes(2);
                if(response.length!=2) throw new AssertionError("Incomplete port reply");
                port=((response[0]&255)<<8)|(response[1]&255);
            }
            System.out.println("PASS: unchanged Beat Link discovers provider"+sourceNumber(index)+" at "+address+"; DB port="+port);
            DbLibraryControl.main(new String[]{Integer.toString(port),args[0],args[1],"--emulator-link-"+(index+1)});
        } finally { finder.stop(); }
    }
}
