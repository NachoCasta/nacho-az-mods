/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.*;
import org.deepsymmetry.beatlink.DeviceFinder;

/** Explicit passive discovery on an assigned physical link-local Ethernet network. */
public final class PhysicalEthernetDiscovery {
    public static void main(String[] args) throws Exception {
        if(args.length!=3 || !args[0].equals("--physical-ethernet"))
            throw new IllegalArgumentException("--physical-ethernet LOCAL_IPV4 SECONDS");
        InetAddress local=InetAddress.getByName(args[1]);
        NetworkInterface nic=NetworkInterface.getByInetAddress(local);
        int seconds=Integer.parseInt(args[2]);
        if(!(local instanceof Inet4Address) || !local.isLinkLocalAddress() || nic==null || !nic.isUp() || nic.isLoopback() || seconds<1 || seconds>30)
            throw new IllegalArgumentException("Assigned link-local interface and1..30 seconds required");
        var finder=DeviceFinder.getInstance();
        java.util.Set<String> seen=new java.util.TreeSet<>();
        try {
            finder.start();long deadline=System.nanoTime()+seconds*1000000000L;
            while(System.nanoTime()<deadline) {
                for(var device:finder.getCurrentDevices()) {
                    if(!device.getAddress().isLinkLocalAddress()) continue;
                    String row=device.getDeviceNumber()+" "+device.getAddress().getHostAddress()+" "+device.getDeviceName();
                    if(seen.add(row)) System.out.println("DISCOVERED "+row);
                }
                Thread.sleep(100);
            }
            System.out.println("PASSIVE_DISCOVERY_COMPLETE devices="+seen.size());
        } finally {finder.stop();}
    }
}
