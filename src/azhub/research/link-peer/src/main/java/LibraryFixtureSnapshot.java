/* SPDX-License-Identifier: GPL-3.0-only */
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import org.deepsymmetry.cratedigger.Database;

/** Local research input for an independent public backend; never package its output. */
public final class LibraryFixtureSnapshot {
    private static String quote(String value) {
        StringBuilder out=new StringBuilder("\"");
        for(char c:value.toCharArray()) {
            if(c=='"' || c=='\\') out.append('\\').append(c);
            else if(c<32) out.append(String.format("\\u%04x",(int)c));
            else out.append(c);
        }
        return out.append('"').toString();
    }
    public static void main(String[] args) throws Exception {
        if(args.length!=2) throw new IllegalArgumentException("INPUT_PDB OUTPUT_LOCAL_JSON");
        Path source=Path.of(args[0]);Path destination=Path.of(args[1]);
        if(Files.exists(destination)) throw new IllegalArgumentException("Output already exists");
        try(Database db=new Database(source.toFile())) {
            StringJoiner tracks=new StringJoiner(",");
            for(long id:new TreeSet<Long>(db.trackIndex.keySet())) {
                var row=db.trackIndex.get(id);
                var artist=db.artistIndex.get(row.artistId());
                var album=db.albumIndex.get(row.albumId());
                var key=db.musicalKeyIndex.get(row.keyId());
                var genre=db.genreIndex.get(row.genreId());
                var label=db.labelIndex.get(row.labelId());
                tracks.add("{\"id\":"+id+",\"title\":"+quote(Database.getText(row.title()))+
                    ",\"path\":"+quote(Database.getText(row.filePath()))+
                    ",\"bpm100\":"+row.tempo()+",\"duration\":"+row.duration()+
                    ",\"artist_id\":"+row.artistId()+",\"album_id\":"+row.albumId()+
                    ",\"artist\":"+quote(artist==null?"":Database.getText(artist.name()))+
                    ",\"album\":"+quote(album==null?"":Database.getText(album.name()))+
                    ",\"key\":"+quote(key==null?"":Database.getText(key.name()))+
                    ",\"genre\":"+quote(genre==null?"":Database.getText(genre.name()))+
                    ",\"label\":"+quote(label==null?"":Database.getText(label.name()))+
                    ",\"key_id\":"+row.keyId()+",\"genre_id\":"+row.genreId()+
                    ",\"label_id\":"+row.labelId()+",\"bitrate\":"+row.bitrate()+
                    ",\"file_size\":"+row.fileSize()+",\"rating\":"+row.rating()+",\"artwork_id\":"+row.artworkId()+
                    ",\"comment\":"+quote(Database.getText(row.comment()))+
                    ",\"analysis_path\":"+quote(Database.getText(row.analyzePath()))+"}");
            }
            StringJoiner folders=new StringJoiner(",");
            for(var group:db.playlistFolderIndex.entrySet()) for(var entry:group.getValue()) {
                if(entry==null) continue; // Sparse playlist positions are not database rows.
                var ids=db.playlistIndex.getOrDefault(entry.id,Collections.emptyList());
                folders.add("{\"id\":"+entry.id+",\"parent\":"+group.getKey()+
                    ",\"name\":"+quote(entry.name)+",\"folder\":"+entry.isFolder+
                    ",\"tracks\":"+ids.toString()+"}");
            }
            Files.writeString(destination,"{\"format\":1,\"tracks\":["+tracks+
                "],\"playlists\":["+folders+"]}\n",StandardCharsets.UTF_8);
            System.out.println("Snapshot: tracks="+db.trackIndex.size()+", playlist folders="+db.playlistFolderIndex.size());
        }
    }
}
