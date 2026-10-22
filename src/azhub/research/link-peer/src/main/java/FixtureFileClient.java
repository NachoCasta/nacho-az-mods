/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.*;
import java.nio.file.*;
import java.security.MessageDigest;
import java.util.*;
import org.deepsymmetry.cratedigger.FileFetcher;

/** Independent library file-transfer control on the isolated fixture endpoint only. */
public final class FixtureFileClient {
    public static void main(String[] args) throws Exception {
        if(args.length!=0) throw new IllegalArgumentException("No arguments supported");
        if(Files.getAttribute(Path.of("/proc/self/ns/net"),"unix:ino").equals(
                Files.getAttribute(Path.of("/proc/1/ns/net"),"unix:ino")))
            throw new IllegalStateException("Host namespace refused");
        int taps=0;
        for(var interfaces=NetworkInterface.getNetworkInterfaces();interfaces.hasMoreElements();) {
            var nic=interfaces.nextElement();
            if(!nic.isUp() || nic.isLoopback()) continue;
            if(!nic.getName().equals("azlink0")) throw new IllegalStateException("Unexpected interface");
            taps++;
            for(var addresses=nic.getInetAddresses();addresses.hasMoreElements();) {
                var address=addresses.nextElement();
                if(address instanceof Inet4Address && !address.getHostAddress().equals("192.0.2.2"))
                    throw new IllegalStateException("Unexpected address");
            }
        }
        if(taps!=1) throw new IllegalStateException("Expected isolated TAP");
        var client=FileFetcher.getInstance();client.setRetransmitTimeout(1000);
        for(String path:List.of("PIONEER/rekordbox/export.pdb",
                "Contents/Aaliyah/Are You That Somebody/Aaliyah - Are You That Somebody-1.mp3")) {
            Path output=Files.createTempFile("azhub-fixture-fetch-", ".bin");
            try {
                client.fetch(InetAddress.getByName("192.0.2.2"),"/C/",path,output.toFile());
                byte[] content=Files.readAllBytes(output);
                StringBuilder hex=new StringBuilder();
                for(byte b:MessageDigest.getInstance("SHA-256").digest(content))hex.append(String.format("%02x",b&255));
                System.out.println("{\"event\":\"fixture_file_fetch\",\"kind\":\""+
                    (path.endsWith(".pdb")?"database":"audio")+"\",\"bytes\":"+content.length+
                    ",\"sha256\":\""+hex+"\"}");
            } finally {Files.deleteIfExists(output);}
        }
    }
}
