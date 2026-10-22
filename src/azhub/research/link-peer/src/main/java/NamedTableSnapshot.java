/* SPDX-License-Identifier: GPL-3.0-only */
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.io.PrintWriter;
import java.lang.reflect.Field;
import java.util.*;
import io.kaitai.struct.ByteBufferKaitaiStream;
import org.deepsymmetry.cratedigger.Database;
import org.deepsymmetry.cratedigger.pdb.RekordboxPdb;

/** Research oracle using Crate Digger0.2.1 row decoding. The old generated
 * page-count interpretation is overridden using the current upstream schema;
 * the dependency jar and all row/string decoders remain unchanged. */
public final class NamedTableSnapshot {
    private static long number(Object row,String method) throws Exception {
        return ((Number)row.getClass().getMethod(method).invoke(row)).longValue();
    }
    private static String name(Object row,String method) throws Exception {
        return Database.getText((RekordboxPdb.DeviceSqlString)
            row.getClass().getMethod(method).invoke(row));
    }
    public static void main(String[] args) throws Exception {
        if(args.length!=2) throw new IllegalArgumentException("INPUT_PDB NEW_LOCAL_TSV");
        byte[] input=Files.readAllBytes(Path.of(args[0]));
        RekordboxPdb db=new RekordboxPdb(new ByteBufferKaitaiStream(input),false);
        Field countField=RekordboxPdb.Page.class.getDeclaredField("numRows");
        countField.setAccessible(true);
        long total=0;
        try(PrintWriter out=new PrintWriter(Files.newBufferedWriter(Path.of(args[1]),
                StandardCharsets.UTF_8,StandardOpenOption.CREATE_NEW))) {
            for(var table:db.tables()) {
                int type=(int)table.type().id();
                if(!Set.of(1,2,3,4,5,7,8,13).contains(type)) continue;
                var ref=table.firstPage(); Set<Long> seen=new HashSet<>();
                while(true) {
                    if(!seen.add(ref.index())) throw new IllegalStateException("Page cycle");
                    var page=ref.body();
                    int at=Math.toIntExact(ref.index()*db.lenPage()+24);
                    int allocated=((input[at]&255)|((input[at+1]&255)<<8))&8191;
                    countField.set(page,allocated);
                    if(page.isDataPage() && allocated>0) for(var group:page.rowGroups())
                        for(var entry:group.rows()) if(entry.present()) {
                            if(group.groupIndex()*16+entry.rowIndex()>=allocated)
                                throw new IllegalStateException("Presence beyond allocated offsets");
                            Object row=entry.body(); long id,parent=0,order=0,track=0,folder=0;
                            String text="";
                            if(type==8) {
                                id=number(row,"playlistId"); order=number(row,"entryIndex"); track=number(row,"trackId");
                            } else {
                                id=number(row,"id"); text=name(row,type==13?"path":"name");
                                if(type==3) parent=number(row,"artistId");
                                if(type==7) {
                                    parent=number(row,"parentId"); order=number(row,"sortOrder");
                                    folder=number(row,"rawIsFolder")!=0?1:0;
                                }
                            }
                            out.printf("%d\t%d\t%d\t%d\t%d\t%d\t",type,id,parent,order,track,folder);
                            for(byte b:text.getBytes(StandardCharsets.UTF_8)) out.printf("%02x",b&255);
                            out.println(); ++total;
                        }
                    if(ref.index()==table.lastPage().index()) break;
                    ref=page.nextPage();
                }
            }
            if(out.checkError()) throw new IllegalStateException("Output failure");
        }
        System.out.println("Named-table rows: "+total);
    }
}
