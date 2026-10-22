/* SPDX-License-Identifier: GPL-3.0-only */
import java.net.*;
import java.nio.*;
import java.nio.file.*;
import java.util.*;
import java.io.DataInputStream;
import java.nio.channels.Channels;
import org.deepsymmetry.beatlink.CdjStatus;
import org.deepsymmetry.beatlink.dbserver.*;
import org.deepsymmetry.beatlink.data.*;
import org.deepsymmetry.cratedigger.pdb.RekordboxAnlz;

/** Invoke unchanged Beat Link analysis queries/decoding over local TCP. */
public final class DbAnalysisControl {
    public static void main(String[] args) throws Exception {
        if(args.length!=4) throw new IllegalArgumentException("LOCAL_PORT TRACK_ID ORACLE_DIRECTORY FIXTURE_DIRECTORY");
        try(Socket socket=new Socket("127.0.0.1",Integer.parseInt(args[0]))) {
            socket.setSoTimeout(5000);
            var constructor=Client.class.getDeclaredConstructor(Socket.class,int.class,int.class); constructor.setAccessible(true);
            Client client=constructor.newInstance(socket,41,3);
            var method=AnalysisTagFinder.class.getDeclaredMethod("getTagViaDbServer",int.class,SlotReference.class,String.class,String.class,Client.class);
            method.setAccessible(true); AnalysisTagFinder finder=AnalysisTagFinder.getInstance();
            SlotReference slot=SlotReference.getSlotReference(41,CdjStatus.TrackSourceSlot.COLLECTION);
            String[][] selections={{"DAT","PQTZ"},{"EXT","PWV4"},{"EXT","PWV5"},{"EXT","PCO2"},{"2EX","PWV6"},{"2EX","PWV7"}};
            int bytes=0;
            for(String[] selection:selections) {
                RekordboxAnlz.TaggedSection section=(RekordboxAnlz.TaggedSection)method.invoke(finder,Integer.parseInt(args[1]),slot,"."+selection[0],selection[1],client);
                if(section==null) throw new AssertionError("No parsed section for "+selection[1]);
                byte[] expected=Files.readAllBytes(Path.of(args[2],selection[0]+"-"+selection[1]+"-0-java.bin"));
                ByteBuffer header=ByteBuffer.wrap(expected);
                long fourcc=Integer.toUnsignedLong(header.getInt()),headerSize=Integer.toUnsignedLong(header.getInt()),length=Integer.toUnsignedLong(header.getInt());
                if(section.fourcc()==null || section.fourcc().id()!=fourcc || section.lenHeader()!=headerSize || section.lenTag()!=length ||
                   !Arrays.equals(section._raw_body(),Arrays.copyOfRange(expected,12,expected.length))) throw new AssertionError("Section mismatch");
                bytes+=expected.length;
            }
            if(method.invoke(finder,Integer.parseInt(args[1]),slot,".EXT","ZZZZ",client)!=null) throw new AssertionError("Missing tag was not empty");
            RekordboxAnlz.TaggedSection rawGrid=(RekordboxAnlz.TaggedSection)method.invoke(finder,Integer.parseInt(args[1]),slot,".DAT","PQTZ",client);
            RekordboxAnlz.BeatGridTag expectedGrid=(RekordboxAnlz.BeatGridTag)rawGrid.body();
            new Message(100,0x2204,new NumberField(0x03080401),new NumberField(Integer.parseInt(args[1])))
                .write(Channels.newChannel(socket.getOutputStream()));
            Message gridReply=Message.read(new DataInputStream(socket.getInputStream()));
            if(gridReply.messageType.getValue()!=0x4602 || gridReply.transaction.getValue()!=100) throw new AssertionError("Beatgrid envelope");
            BeatGrid grid=new BeatGrid(null,gridReply);
            if(grid.beatCount!=expectedGrid.numBeats()) throw new AssertionError("Beat count mismatch");
            for(int i=1;i<=grid.beatCount;++i) {
                var beat=expectedGrid.beats().get(i-1);
                if(grid.getBeatWithinBar(i)!=beat.beatNumber() || grid.getBpm(i)!=beat.tempo() || grid.getTimeWithinTrack(i)!=beat.time())
                    throw new AssertionError("Beatgrid conversion mismatch");
            }
            System.out.println("PASS: unchanged Beat Link legacy grid matches all "+grid.beatCount+" exported beat positions/tempos/times");
            Message detailReply=client.simpleRequest(Message.KnownType.WAVE_DETAIL_REQ,Message.KnownType.WAVE_DETAIL,
                client.buildRMST(Message.MenuIdentifier.MAIN_MENU,slot.slot),new NumberField(Integer.parseInt(args[1])),NumberField.WORD_0);
            WaveformDetail detail=new WaveformDetail(null,detailReply,WaveformFinder.WaveformStyle.BLUE);
            RekordboxAnlz sourceAnalysis=RekordboxAnlz.fromFile(Path.of(args[3],"fixture.EXT").toString());
            WaveformDetail expectedDetail=new WaveformDetail(null,sourceAnalysis,WaveformFinder.WaveformStyle.BLUE);
            if(!detail.getData().equals(expectedDetail.getData()) || detail.getFrameCount()!=expectedDetail.getFrameCount())
                throw new AssertionError("Legacy waveform detail differs from exported samples");
            System.out.println("PASS: unchanged Beat Link legacy waveform matches "+detail.getFrameCount()+" original samples");
            Message previewReply=client.simpleRequest(Message.KnownType.WAVE_PREVIEW_REQ,Message.KnownType.WAVE_PREVIEW,
                client.buildRMST(Message.MenuIdentifier.DATA,slot.slot),NumberField.WORD_1,
                new NumberField(Integer.parseInt(args[1])),NumberField.WORD_0);
            var previewConstructor=WaveformPreview.class.getDeclaredConstructor(DataReference.class,Message.class,WaveformFinder.WaveformStyle.class);
            previewConstructor.setAccessible(true);
            WaveformPreview preview=previewConstructor.newInstance(null,previewReply,WaveformFinder.WaveformStyle.BLUE);
            RekordboxAnlz datAnalysis=RekordboxAnlz.fromFile(Path.of(args[3],"fixture.DAT").toString());
            WaveformPreview expectedPreview=new WaveformPreview(null,datAnalysis,WaveformFinder.WaveformStyle.BLUE);
            // Beat Link8 counts the100-byte tiny suffix as50 additional blue
            // columns. Compare the primary400 columns and tiny bytes separately.
            if(preview.segmentCount!=450 || expectedPreview.segmentCount!=400) throw new AssertionError("Preview column count");
            for(int i=0;i<expectedPreview.segmentCount;++i) {
                if(preview.segmentHeight(i,false)!=expectedPreview.segmentHeight(i,false) ||
                   !preview.segmentColor(i,false).equals(expectedPreview.segmentColor(i,false))) throw new AssertionError("Preview rendering values");
            }
            ByteBuffer previewBytes=((BinaryField)previewReply.arguments.get(3)).getValue();
            if(previewBytes.remaining()!=900) throw new AssertionError("Legacy preview length");
            boolean tinyMatched=false;
            for(var section:datAnalysis.sections()) {
                if(section.fourcc()!=null && section.fourcc().id()==0x50575632) {
                    byte[] tiny=((RekordboxAnlz.WavePreviewTag)section.body()).data();
                    byte[] actual=new byte[100]; previewBytes.position(800); previewBytes.get(actual);
                    if(!Arrays.equals(tiny,actual)) throw new AssertionError("Tiny preview samples");
                    tinyMatched=true;
                }
            }
            if(!tinyMatched) throw new AssertionError("Missing independent tiny preview");
            System.out.println("PASS: legacy preview matches400 rendered columns and100 original tiny samples");
            var close=Client.class.getDeclaredMethod("close"); close.setAccessible(true); close.invoke(client);
            System.out.println("PASS: unchanged Beat Link analysis decoder matches6 real sections, bytes="+bytes+", and missing-tag response");
        }
    }
}
