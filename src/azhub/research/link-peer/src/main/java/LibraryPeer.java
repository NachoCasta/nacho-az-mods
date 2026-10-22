/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.*;
import java.util.*;
import org.deepsymmetry.beatlink.*;
import org.deepsymmetry.beatlink.data.*;

/** Independent unmodified Beat Link menu client; isolated emulator only. */
public final class LibraryPeer {
    private static void network() throws Exception {
        boolean found=false;
        for (NetworkInterface nic: Collections.list(NetworkInterface.getNetworkInterfaces())) {
            if (nic.isLoopback()) continue;
            if (!nic.isUp() || !nic.getName().equals("azlink0"))
                throw new IllegalStateException("Unexpected network interface");
            for (InetAddress a: Collections.list(nic.getInetAddresses())) {
                if (a.getHostAddress().equals("192.0.2.2")) found=true;
                else if (!a.isLinkLocalAddress()) throw new IllegalStateException("Unexpected address");
            }
        }
        if (!found) throw new IllegalStateException("Missing isolated peer address");
    }
    public static void main(String[] args) throws Exception {
        if (args.length>1 || (args.length==1 && !args[0].equals("--fetch-native-database") && !args[0].equals("--fetch-native-audio") && !args[0].equals("--probe-shared-address")))
            throw new IllegalArgumentException("Expected an isolated native-library probe mode");
        network();
        DeviceFinder finder=DeviceFinder.getInstance();
        VirtualCdj cdj=VirtualCdj.getInstance();
        MetadataFinder metadata=MetadataFinder.getInstance();
        try {
            finder.start();
            cdj.setDeviceName("Hub Library Research");
            cdj.setDeviceNumber((byte)3); cdj.setUseStandardPlayerNumber(true);
            if (!cdj.start() || cdj.getDeviceNumber()!=3 ||
                !cdj.getLocalAddress().getHostAddress().equals("192.0.2.2"))
                throw new IllegalStateException("Unexpected player assignment");
            cdj.setSendingStatus(true);
            metadata.start();
            SlotReference source=SlotReference.getSlotReference(1,CdjStatus.TrackSourceSlot.USB_SLOT);
            long until=System.nanoTime()+20_000_000_000L;
            while (metadata.getMediaDetailsFor(source)==null && System.nanoTime()<until) Thread.sleep(100);
            if (metadata.getMediaDetailsFor(source)==null)
                throw new IllegalStateException("Native USB1 media details not discovered");
            System.out.println("{\"event\":\"native_media_discovered\",\"player\":1}");
            MenuLoader menus=MenuLoader.getInstance();
            int roots=menus.requestRootMenuFrom(source,0).size();
            int playlists=menus.requestPlaylistMenuFrom(source,0).size();
            int tracks=menus.requestTrackMenuFrom(source,0).size();
            System.out.println("{\"event\":\"menus\",\"root_items\":"+roots+
                ",\"playlist_items\":"+playlists+",\"track_items\":"+tracks+"}");
            if (roots<1 || playlists<1 || tracks<1) throw new IllegalStateException("Empty native menu");
            if (args.length==1 && args[0].equals("--probe-shared-address")) {
                SlotReference second=SlotReference.getSlotReference(2,CdjStatus.TrackSourceSlot.USB_SLOT);
                try {
                    int count=menus.requestRootMenuFrom(second,0).size();
                    System.out.println("{\"event\":\"second_device_root\",\"items\":"+count+"}");
                } catch (Exception problem) {
                    System.out.println("{\"event\":\"second_device_error\",\"class\":\""+
                        problem.getClass().getSimpleName()+"\"}");
                    System.err.println("Second device query: "+problem.getMessage());
                }
            }
            if (args.length==1 && args[0].startsWith("--fetch-")) {
                java.io.File destination=java.io.File.createTempFile("native-link-control-", ".pdb");
                try {
                    org.deepsymmetry.cratedigger.FileFetcher fetcher=
                        org.deepsymmetry.cratedigger.FileFetcher.getInstance();
                    fetcher.setRetransmitTimeout(1000);
                    fetcher.fetch(InetAddress.getByName("192.0.2.15"),"/C/",
                        args[0].equals("--fetch-native-audio") ?
                            "Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3" :
                            "PIONEER/rekordbox/export.pdb",destination);
                    byte[] content=java.nio.file.Files.readAllBytes(destination.toPath());
                    byte[] hash=java.security.MessageDigest.getInstance("SHA-256").digest(content);
                    StringBuilder hex=new StringBuilder();
                    for (byte value:hash) hex.append(String.format("%02x",value&255));
                    System.out.println("{\"event\":\"native_file_fetch\",\"bytes\":"+
                        content.length+",\"sha256\":\""+hex+"\"}");
                } finally { java.nio.file.Files.deleteIfExists(destination.toPath()); }
            }
        } finally { metadata.stop(); cdj.stop(); finder.stop(); }
    }
}
