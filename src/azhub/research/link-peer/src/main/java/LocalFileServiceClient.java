/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.*;
import java.nio.file.*;
import java.security.MessageDigest;
import org.deepsymmetry.cratedigger.FileFetcher;

/** Unmodified Crate Digger file client on an isolated test network. */
public final class LocalFileServiceClient {
    public static void main(String[] args) throws Exception {
        boolean emulatorLink = args.length == 4;
        boolean nativeUsb = emulatorLink && args[3].matches("--emulator-native-usb[12]");
        int source=0;
        if(emulatorLink && !nativeUsb && !args[3].equals("--emulator-link")) {
            if(!args[3].matches("--emulator-link-[1-8]")) throw new IllegalArgumentException("Source flag");
            source=args[3].charAt(args[3].length()-1)-'1';
        }
        if (args.length != 0 && args.length != 3 && !emulatorLink)
            throw new IllegalArgumentException("Optional arguments: relative path, expected bytes, expected SHA256");
        String relative = args.length == 0 ? "Music/Álbum 🎵/track.bin" : args[0];
        long expectedBytes = args.length == 0 ? 30001 : Long.parseLong(args[1]);
        if (Path.of(relative).isAbsolute() || expectedBytes < 0 || expectedBytes > 32 * 1024 * 1024)
            throw new IllegalArgumentException("Bounded relative fixture required");
        if (Files.getAttribute(Path.of("/proc/self/ns/net"), "unix:ino").equals(
                Files.getAttribute(Path.of("/proc/1/ns/net"), "unix:ino")))
            throw new IllegalStateException("Initial network namespace refused");
        int taps = 0;
        for (var interfaces = NetworkInterface.getNetworkInterfaces(); interfaces.hasMoreElements();) {
            var nic = interfaces.nextElement();
            if (nic.isUp() && !nic.isLoopback()) {
                if (!emulatorLink || !nic.getName().equals("azlink0"))
                    throw new IllegalStateException("Unexpected test interface");
                ++taps;
                for (var addresses = nic.getInetAddresses(); addresses.hasMoreElements();) {
                    var address = addresses.nextElement();
                    if (address instanceof Inet4Address && !LiveProviderControl.fixturePeerAddress(address.getHostAddress()))
                        throw new IllegalStateException("Unexpected test address");
                }
            }
        }
        if (emulatorLink && taps != 1) throw new IllegalStateException("Expected isolated TAP");
        var client = FileFetcher.getInstance();
        client.setRetransmitTimeout(500);
        Path output = Files.createTempFile("azhub-c-service-fetch-", ".bin");
        try {
            String address = nativeUsb ? "192.0.2.15" : emulatorLink ? LiveProviderControl.sourceAddress(source) : "127.0.0.1";
            String export = nativeUsb && args[3].endsWith("2") ? "/B/" : "/C/";
            client.fetch(InetAddress.getByName(address), export,
                         relative, output.toFile());
            byte[] content = Files.readAllBytes(output);
            if (content.length != expectedBytes) throw new AssertionError("Wrong byte count: " + content.length);
            if (args.length == 0) for (int i = 0; i < content.length; ++i)
                if ((content[i] & 255) != (i * 37 + i / 256) % 256)
                    throw new AssertionError("Wrong payload byte at " + i);
            var hex = new StringBuilder();
            for (byte b : MessageDigest.getInstance("SHA-256").digest(content))
                hex.append(String.format("%02x", b & 255));
            if (args.length != 0 && !hex.toString().equals(args[2]))
                throw new AssertionError("Wrong fixture hash");
            System.out.println("{\"event\":\"c_service_crate_digger_fetch\",\"bytes\":" + content.length
                               + ",\"sha256\":\"" + hex + "\",\"address\":\"" + address
                               + "\",\"export\":\"" + export + "\"}");
        } finally {
            Files.deleteIfExists(output);
        }
    }
}
