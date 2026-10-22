/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.*;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.TimeUnit;
import org.deepsymmetry.beatlink.CdjStatus;
import org.deepsymmetry.beatlink.dbserver.*;
import org.deepsymmetry.cratedigger.Database;

/** Local independent browsing control. The entity TSV comes from the corrected
 * Crate Digger oracle, not from the C provider under test. */
public final class DbLibraryControl {
    static final Message.MenuIdentifier MENU=Message.MenuIdentifier.MAIN_MENU;
    static final CdjStatus.TrackSourceSlot SLOT=CdjStatus.TrackSourceSlot.COLLECTION;
    static long number(Message row,int index) { return ((NumberField)row.arguments.get(index)).getValue(); }
    static List<Message> browse(Client client,Message.KnownType type,Field... args) throws Exception {
        Message offer=client.menuRequest(type,MENU,SLOT,args);
        return client.renderMenuItems(MENU,SLOT,CdjStatus.TrackType.REKORDBOX,offer);
    }
    public static void main(String[] args) throws Exception {
        boolean emulator=args.length==4;
        int source=0;
        if(emulator && !args[3].equals("--emulator-link")) {
            if(!args[3].matches("--emulator-link-[1-8]")) throw new IllegalArgumentException("Source flag");
            source=args[3].charAt(args[3].length()-1)-'1';
        }
        if(args.length!=3 && !emulator) throw new IllegalArgumentException("LOCAL_PORT PDB ORACLE_TSV [--emulator-link]");
        if(emulator) {
            if(Files.getAttribute(Path.of("/proc/self/ns/net"),"unix:ino").equals(
                    Files.getAttribute(Path.of("/proc/1/ns/net"),"unix:ino")))
                throw new IllegalStateException("Initial namespace refused");
            boolean found=false;
            for(var nic:Collections.list(NetworkInterface.getNetworkInterfaces())) {
                if(!nic.isUp() || nic.isLoopback()) continue;
                if(!nic.getName().equals("azlink0")) throw new IllegalStateException("Unexpected interface");
                for(var address:Collections.list(nic.getInetAddresses())) if(address instanceof Inet4Address) {
                    if(!LiveProviderControl.fixturePeerAddress(address.getHostAddress())) throw new IllegalStateException("Unexpected IPv4 address");
                    found=true;
                }
            }
            if(!found) throw new IllegalStateException("Missing isolated TAP");
        }
        Map<Long,List<long[]>> memberships=new TreeMap<>();
        Map<Long,List<long[]>> folders=new TreeMap<>();
        Set<Long> playlistIds=new TreeSet<>();
        for(String line:Files.readAllLines(Path.of(args[2]))) {
            String[] fields=line.split("\t",-1); int table=Integer.parseInt(fields[0]);
            long id=Long.parseLong(fields[1]),parent=Long.parseLong(fields[2]),order=Long.parseLong(fields[3]);
            if(table==8) memberships.computeIfAbsent(id,k->new ArrayList<>()).add(new long[]{order,Long.parseLong(fields[4])});
            if(table==7) {
                long folder=Long.parseLong(fields[5]);
                folders.computeIfAbsent(parent,k->new ArrayList<>()).add(new long[]{order,id});
                if(folder==0) playlistIds.add(id);
            }
        }
        for(var rows:memberships.values()) rows.sort(Comparator.comparingLong(r->r[0]));
        for(var rows:folders.values()) rows.sort(Comparator.<long[]>comparingLong(r->r[0]).thenComparingLong(r->r[1]));
        // Index the independent local oracle before connecting. Concurrent PDB
        // decoding must not consume the provider's bounded handshake deadline.
        try(Database db=new Database(Path.of(args[1]).toFile()); Socket socket=new Socket(emulator ? LiveProviderControl.sourceAddress(source) : "127.0.0.1",Integer.parseInt(args[0]))) {
            socket.setSoTimeout(5000);
            var constructor=Client.class.getDeclaredConstructor(Socket.class,int.class,int.class); constructor.setAccessible(true);
            Client client=constructor.newInstance(socket,LiveProviderControl.sourceNumber(source),3);
            if(!client.tryLockingForMenuOperations(5,TimeUnit.SECONDS)) throw new AssertionError("Menu lock");
            try {
                if(browse(client,Message.KnownType.ROOT_MENU_REQ,new NumberField(0),new NumberField(0xffffff)).size()!=2)
                    throw new AssertionError("Root menu");
                var tracks=browse(client,Message.KnownType.TRACK_MENU_REQ,new NumberField(0));
                Set<Long> ids=new HashSet<>();
                for(Message row:tracks) {
                    long id=number(row,1); var track=db.trackIndex.get(id);
                    if(track==null || !ids.add(id) || !((StringField)row.arguments.get(3)).getValue().equals(Database.getText(track.title())))
                        throw new AssertionError("Track content mismatch");
                }
                if(ids.size()!=db.trackIndex.size()) throw new AssertionError("Track count mismatch");
                int nodes=0,entries=0;
                for(var folder:folders.entrySet()) {
                    var rows=browse(client,Message.KnownType.PLAYLIST_REQ,new NumberField(0),new NumberField(folder.getKey()),new NumberField(1));
                    if(rows.size()!=folder.getValue().size()) throw new AssertionError("Folder count mismatch");
                    for(int i=0;i<rows.size();++i) if(number(rows.get(i),1)!=folder.getValue().get(i)[1]) throw new AssertionError("Folder order mismatch");
                    nodes+=rows.size();
                }
                for(long id:playlistIds) {
                    var expected=memberships.getOrDefault(id,Collections.emptyList());
                    var rows=browse(client,Message.KnownType.PLAYLIST_REQ,new NumberField(0),new NumberField(id),new NumberField(0));
                    if(rows.size()!=expected.size()) throw new AssertionError("Playlist count mismatch");
                    for(int i=0;i<rows.size();++i) if(number(rows.get(i),1)!=expected.get(i)[1] || number(rows.get(i),9)!=expected.get(i)[0])
                        throw new AssertionError("Playlist order mismatch");
                    entries+=rows.size();
                }
                System.out.println("PASS: unchanged Beat Link browses tracks="+ids.size()+", ordered nodes="+nodes+", ordered memberships="+entries);
            } finally { client.unlockForMenuOperations(); }
            var close=Client.class.getDeclaredMethod("close");close.setAccessible(true);close.invoke(client);
        }
    }
}
