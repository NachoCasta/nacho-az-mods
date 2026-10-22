/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.Socket;
import java.util.Objects;
import org.deepsymmetry.beatlink.CdjStatus;
import org.deepsymmetry.beatlink.dbserver.*;
import org.deepsymmetry.beatlink.data.CueList;
import org.deepsymmetry.cratedigger.pdb.RekordboxAnlz;
/** Local legacy cue query compared to independent direct analysis decoding. */
public final class DbCueControl {
    public static void main(String[] args) throws Exception {
        if(args.length!=3) throw new IllegalArgumentException("LOCAL_PORT TRACK_ID DAT_FILE");
        try(Socket socket=new Socket("127.0.0.1",Integer.parseInt(args[0]))) {
            socket.setSoTimeout(5000);
            var constructor=Client.class.getDeclaredConstructor(Socket.class,int.class,int.class); constructor.setAccessible(true);
            Client client=constructor.newInstance(socket,41,3);
            Message reply=client.simpleRequest(Message.KnownType.CUE_LIST_REQ,Message.KnownType.CUE_LIST,
                client.buildRMST(Message.MenuIdentifier.DATA,CdjStatus.TrackSourceSlot.COLLECTION),new NumberField(Integer.parseInt(args[1])));
            CueList actual=new CueList(reply),expected=new CueList(RekordboxAnlz.fromFile(args[2]));
            if(!actual.equals(expected) || actual.getHotCueCount()!=expected.getHotCueCount() ||
               actual.getMemoryPointCount()!=expected.getMemoryPointCount()) throw new AssertionError("Legacy cue mismatch");
            System.out.println("PASS: unchanged Beat Link legacy cue decoder matches "+actual.entries.size()+" real entries");
            Message extended=client.simpleRequest(Message.KnownType.CUE_LIST_EXT_REQ,Message.KnownType.CUE_LIST_EXT,
                client.buildRMST(Message.MenuIdentifier.DATA,CdjStatus.TrackSourceSlot.COLLECTION),new NumberField(Integer.parseInt(args[1])),NumberField.WORD_0);
            CueList extendedActual=new CueList(extended);
            CueList extendedExpected=new CueList(RekordboxAnlz.fromFile(args[2].substring(0,args[2].length()-3)+"EXT"));
            if(!extendedActual.equals(extendedExpected)) throw new AssertionError("Extended cue positions/types mismatch");
            int comments=0;
            for(int i=0;i<extendedActual.entries.size();++i) {
                var a=extendedActual.entries.get(i); var b=extendedExpected.entries.get(i);
                if(!a.comment.equals(b.comment) || a.colorId!=b.colorId || !Objects.equals(a.embeddedColor,b.embeddedColor) ||
                   !Objects.equals(a.rekordboxColor,b.rekordboxColor)) throw new AssertionError("Extended cue comments/colors mismatch");
                if(!a.comment.isEmpty()) ++comments;
            }
            System.out.println("PASS: unchanged Beat Link extended cues match "+extendedActual.entries.size()+" entries, including "+comments+" comments and all colors");
            var close=Client.class.getDeclaredMethod("close"); close.setAccessible(true); close.invoke(client);
        }
    }
}
