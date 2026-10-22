/* SPDX-License-Identifier: GPL-3.0-only */
import java.nio.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import io.kaitai.struct.ByteBufferKaitaiStream;
import org.deepsymmetry.cratedigger.pdb.RekordboxAnlz;

/** Independent Crate Digger ANLZ section extraction; output stays local. */
public final class AnalysisSectionControl {
    public static void main(String[] args) throws Exception {
        if(args.length!=4) throw new IllegalArgumentException("INPUT TAG OCCURRENCE NEW_OUTPUT");
        byte[] tag=args[1].getBytes(StandardCharsets.US_ASCII);
        if(tag.length!=4) throw new IllegalArgumentException("Four-character tag required");
        long code=Integer.toUnsignedLong(ByteBuffer.wrap(tag).getInt()); int occurrence=Integer.parseInt(args[2]);
        if(occurrence<0) throw new IllegalArgumentException("Negative occurrence");
        byte[] bytes=Files.readAllBytes(Path.of(args[0]));
        RekordboxAnlz file=new RekordboxAnlz(new ByteBufferKaitaiStream(bytes));
        long position=file.lenHeader();
        for(var section:file.sections()) {
            long sectionPosition=position; position+=section.lenTag();
            // Older enum tables omit some valid tags. The unchanged generated
            // parser still provides their section boundaries and raw body.
            long actualCode=Integer.toUnsignedLong(ByteBuffer.wrap(bytes,Math.toIntExact(sectionPosition),4).getInt());
            if(actualCode!=code) continue;
            if(occurrence--!=0) continue;
            byte[] body=section._raw_body();
            ByteBuffer output=ByteBuffer.allocate(Math.toIntExact(section.lenTag()));
            output.putInt((int)code).putInt(Math.toIntExact(section.lenHeader())).putInt(Math.toIntExact(section.lenTag())).put(body);
            if(output.hasRemaining()) throw new AssertionError("Section size mismatch");
            Files.write(Path.of(args[3]),output.array(),StandardOpenOption.CREATE_NEW);
            System.out.println("Extracted analysis section bytes="+output.capacity()); return;
        }
        throw new IllegalArgumentException("Section not found");
    }
}
