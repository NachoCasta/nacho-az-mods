/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Collections;
import java.util.concurrent.atomic.AtomicLong;
import org.deepsymmetry.beatlink.*;

/** Bounded Beat Link protocol peer, restricted to the isolated emulator network. */
public final class LinkPeer {
    private static synchronized void log(String event, String fields) {
        System.out.println("{\"event\":\"" + event + "\",\"monotonic_ns\":"
                + System.nanoTime() + (fields.isEmpty() ? "" : "," + fields) + "}");
    }

    private static String quote(String text) {
        return "\"" + text.replace("\\", "\\\\").replace("\"", "\\\"")
                .replace("\n", "\\n").replace("\r", "\\r") + "\"";
    }

    private static void checkNetwork() throws Exception {
        boolean found = false;
        for (NetworkInterface nic : Collections.list(NetworkInterface.getNetworkInterfaces())) {
            if (nic.isLoopback()) continue;
            if (!nic.getName().equals("azlink0") || !nic.isUp()) {
                throw new IllegalStateException("Unexpected interface: " + nic.getName());
            }
            for (InetAddress address : Collections.list(nic.getInetAddresses())) {
                if (address.getHostAddress().equals("192.0.2.2")) found = true;
                else if (!address.isLinkLocalAddress())
                    throw new IllegalStateException("Unexpected address: " + address);
            }
        }
        if (!found) throw new IllegalStateException("Missing isolated azlink0 address");
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 4 && args.length != 7) throw new IllegalArgumentException(
                "Usage: LinkPeer ROLE PLAYER SECONDS BPM [--physical-lan LOCAL_IPV4 AZ_IPV4]");
        String role = args[0];
        int player = Integer.parseInt(args[1]);
        int seconds = Integer.parseInt(args[2]);
        double tempo = Double.parseDouble(args[3]);
        if (!java.util.Set.of("observe", "status", "master", "handoff", "follower").contains(role)
                || player < 1 || player > 4 || seconds < 1 || seconds > 3600
                || !Double.isFinite(tempo) || tempo < 40 || tempo > 250)
            throw new IllegalArgumentException("Invalid bounded peer configuration");
        String localAddress = "192.0.2.2";
        InetAddress physicalTarget = null;
        if (args.length == 7) {
            if (!args[4].equals("--physical-lan")
                    || !(role.equals("status") || role.equals("handoff")) || seconds > 60)
                throw new IllegalArgumentException("Physical trial permits bounded status or negotiated handoff only");
            InetAddress local = InetAddress.getByName(args[5]);
            physicalTarget = InetAddress.getByName(args[6]);
            NetworkInterface nic = NetworkInterface.getByInetAddress(local);
            if (!(local instanceof java.net.Inet4Address)
                    || !(physicalTarget instanceof java.net.Inet4Address)
                    || !local.isSiteLocalAddress() || !physicalTarget.isSiteLocalAddress()
                    || local.equals(physicalTarget) || nic == null || !nic.isUp() || nic.isLoopback())
                throw new IllegalArgumentException("Expected assigned private IPv4 and explicit AZ address");
            boolean sameSubnet = false;
            for (java.net.InterfaceAddress address : nic.getInterfaceAddresses()) {
                if (!address.getAddress().equals(local)) continue;
                int prefix = address.getNetworkPrefixLength();
                if (prefix < 8 || prefix > 30) continue;
                byte[] a = local.getAddress(), b = physicalTarget.getAddress();
                boolean match = true;
                for (int bit = 0; bit < prefix; bit++)
                    if ((a[bit / 8] & (1 << (7 - bit % 8))) != (b[bit / 8] & (1 << (7 - bit % 8)))) match = false;
                sameSubnet |= match;
            }
            if (!sameSubnet) throw new IllegalArgumentException("AZ is not on selected local subnet");
            localAddress = local.getHostAddress();
            log("physical_trial", "\"local\":" + quote(localAddress)
                    + ",\"target\":" + quote(physicalTarget.getHostAddress()));
        } else {
            checkNetwork();
        }
        VirtualCdj cdj = VirtualCdj.getInstance();
        DeviceFinder finder = DeviceFinder.getInstance();
        BeatFinder beats = BeatFinder.getInstance();
        AtomicLong statuses = new AtomicLong(), beatCount = new AtomicLong();
        finder.addDeviceAnnouncementListener(new DeviceAnnouncementListener() {
            private void announce(String event, DeviceAnnouncement device) {
                log(event, "\"player\":" + device.getDeviceNumber() + ",\"address\":"
                        + quote(device.getAddress().getHostAddress()) + ",\"name\":"
                        + quote(device.getDeviceName()));
            }
            public void deviceFound(DeviceAnnouncement device) { announce("found", device); }
            public void deviceLost(DeviceAnnouncement device) { announce("lost", device); }
        });
        cdj.addUpdateListener(update -> {
            statuses.incrementAndGet();
            log("status", "\"player\":" + update.getDeviceNumber()
                    + ",\"bpm100\":" + update.getBpm() + ",\"master\":" + update.isTempoMaster()
                    + ",\"synced\":" + update.isSynced()
                    + ",\"effective_bpm\":" + (update.getBpm() == 65535 ? "null" : Double.toString(update.getEffectiveTempo()))
                    + ",\"pitch\":" + update.getPitch()
                    + ",\"packet_bytes\":" + update.getPacketBytes().length);
        });
        beats.addBeatListener(beat -> {
            beatCount.incrementAndGet();
            log("beat", "\"player\":" + beat.getDeviceNumber() + ",\"bpm100\":" + beat.getBpm()
                    + ",\"beat_within_bar\":" + beat.getBeatWithinBar());
        });
        try {
            finder.start();
            if (physicalTarget != null) {
                Thread.sleep(4000);
                boolean found = false;
                for (DeviceAnnouncement device : finder.getCurrentDevices()) {
                    if (!device.getAddress().equals(physicalTarget))
                        throw new IllegalStateException("Other physical DJ device discovered; refusing trial");
                    if (device.getDeviceNumber() == player)
                        throw new IllegalStateException("Requested peer player is occupied");
                    found = true;
                }
                if (!found) throw new IllegalStateException("Explicit AZ target not discovered");
                int matching = 0;
                for (NetworkInterface nic : Collections.list(NetworkInterface.getNetworkInterfaces())) {
                    if (!nic.isUp() || nic.isLoopback()) continue;
                    for (java.net.InterfaceAddress address : nic.getInterfaceAddresses()) {
                        if (!(address.getAddress() instanceof java.net.Inet4Address)) continue;
                        int prefix = address.getNetworkPrefixLength();
                        if (prefix < 8 || prefix > 30) continue;
                        byte[] a = address.getAddress().getAddress(), b = physicalTarget.getAddress();
                        boolean match = true;
                        for (int bit = 0; bit < prefix; bit++)
                            if ((a[bit / 8] & (1 << (7 - bit % 8))) != (b[bit / 8] & (1 << (7 - bit % 8)))) match = false;
                        if (match) matching++;
                    }
                }
                if (matching != 1)
                    throw new IllegalStateException("Ambiguous peer interface; refusing trial");
            }
            beats.start();
            log("listening", "\"role\":" + quote(role) + ",\"requested_player\":" + player);
            if (!role.equals("observe")) {
                cdj.setDeviceName("AZ Hub Test Peer");
                cdj.setDeviceNumber((byte) player);
                cdj.setUseStandardPlayerNumber(true);
                if (!cdj.start()) throw new IllegalStateException("VirtualCdj failed to start");
                if (!cdj.getLocalAddress().getHostAddress().equals(localAddress)
                        || cdj.getDeviceNumber() != player)
                    throw new IllegalStateException("Unexpected peer address or player assignment");
                cdj.setTempo(tempo);
                cdj.setSendingStatus(true);
                if (role.equals("master")) {
                    cdj.setPlaying(true);
                    cdj.becomeTempoMaster();
                } else if (role.equals("handoff")) {
                    // Discovery alone does not populate VirtualCdj's master.
                    // Without a received master status, becomeTempoMaster()
                    // asserts ownership instead of sending a yield request.
                    long deadline = System.nanoTime() + 10_000_000_000L;
                    DeviceUpdate previous;
                    while ((previous = cdj.getTempoMaster()) == null && System.nanoTime() < deadline)
                        Thread.sleep(20);
                    if (previous == null)
                        throw new IllegalStateException("No observed native master; handoff not attempted");
                    log("master_request", "\"previous_player\":" + previous.getDeviceNumber());
                    cdj.setPlaying(true);
                    cdj.becomeTempoMaster();
                    deadline = System.nanoTime() + 5_000_000_000L;
                    while (!cdj.isTempoMaster() && System.nanoTime() < deadline)
                        Thread.sleep(20);
                    log("master_result", "\"peer_master\":" + cdj.isTempoMaster());
                    if (!cdj.isTempoMaster())
                        throw new IllegalStateException("Native master handoff did not complete within five seconds");
                    cdj.setTempo(tempo);
                } else if (role.equals("follower")) {
                    cdj.setPlaying(true);
                    cdj.setSynced(true);
                }
                log("started", "\"player\":" + cdj.getDeviceNumber() + ",\"role\":" + quote(role));
            }
            Thread.sleep(seconds * 1000L);
        } finally {
            log("stopping", "\"peer_master\":" + cdj.isTempoMaster()
                    + ",\"peer_synced\":" + cdj.isSynced());
            cdj.stop();
            beats.stop();
            finder.stop();
            log("stopped", "\"statuses\":" + statuses.get() + ",\"beats\":" + beatCount.get());
        }
    }
}
