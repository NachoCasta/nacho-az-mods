# SPDX-License-Identifier: GPL-3.0-only
"""Host-only metadata prototype over one hub fixture snapshot, not live export."""
import json
from pathlib import Path


class SnapshotBackend:
    def __init__(self,path,number=17):
        if number not in (17,18,*range(41,49)):raise ValueError('Unsupported isolated provider identity')
        self.number=number
        path=Path(path)
        if path.stat().st_size>16*1024*1024:raise ValueError('Oversized snapshot')
        data=json.loads(path.read_text(encoding='utf-8'))
        if data.get('format')!=1 or len(data['tracks'])>100000:raise ValueError('Unsupported snapshot')
        self.tracks={row['id']:row for row in data['tracks']}
        if len(self.tracks)!=len(data['tracks']):raise ValueError('Duplicate track ID')
        self.playlists=data['playlists'];self.track_rows=sorted(self.tracks.values(),key=lambda x:(x['title'].casefold(),x['id']))
        self.track_count=len(self.tracks)
    @staticmethod
    def item(title,identity,kind,parent=0,position=0):
        return [parent,identity,len(title.encode('utf-16be'))+2,title,2,'',kind,0,0,position,0,0]
    def request(self,request,menus):
        tx,kind,args=request['transaction'],request['type'],request['arguments']
        if kind==0:return [(0x4000,[0,self.number])]
        if kind==0x3007:return [(0x4000,[kind,0])]
        if kind==0x100:return []
        if kind==1:return []
        # Empty analysis controls use the response layouts from pinned Beat Link.
        # Actual cue/analysis data is not yet served by this snapshot prototype.
        if kind==0x2104:return [(0x4702,[kind,0,0,b'',0x24,0,0,0,b''])]
        if kind==0x2b04:return [(0x4e02,[kind,0,0,b'',0])]
        if kind==0x2c04:return [(0x4f02,[kind,0,0,b'',1])]
        if kind==0x2004:return [(0x4402,[kind,0,0,b''])]
        if not args or not isinstance(args[0],int):return [(0x4003,[kind,0])]
        key=(args[0]>>8)&0xffff
        rows=None
        if kind==0x1000:rows=[self.item('TRACK',0,0x83),self.item('PLAYLIST',0,0x84)]
        elif kind==0x1004:rows=[self.item(x['title'],x['id'],4) for x in self.track_rows]
        elif kind==0x2102 and len(args)>=2:
            track=self.tracks.get(args[1])
            # Initial load control covers only the explicitly selected MP3 fixture.
            # Other decoder identities require separate native evidence.
            if track is not None and track.get('path','').lower().endswith('.mp3'):
                rows=[self.item('',1,4),
                      self.item('',track.get('duration',0),11),
                      self.item('',track.get('bpm100',0),13),
                      self.item(track.get('comment',''),track['id'],0x23),
                      self.item(track['path'],track['id'],0,track.get('file_size',0)),
                      self.item('',1,0x2f),
                      self.item(track.get('key',''),track.get('key_id',0),15)]
        elif kind==0x2002 and len(args)>=2:
            track=self.tracks.get(args[1])
            if track is not None:
                title=self.item(track['title'],track['id'],4)
                title[8]=track.get('artwork_id',0)
                rows=[title]
                for field,identity,item_type in (
                    ('artist','artist_id',7),('album','album_id',2),
                    ('genre','genre_id',6),('key','key_id',15),
                    ('label','label_id',14)):
                    rows.append(self.item(track.get(field,''),track.get(identity,0),item_type))
                for field,item_type in (('duration',11),('bpm100',13),('rating',10),('bitrate',16)):
                    rows.append(self.item('',track.get(field,0),item_type))
                rows.append(self.item(track.get('comment',''),0,0x23))
        elif kind==0x1105 and len(args)>=4:
            identity,folder=args[2:4]
            if folder:rows=[self.item(x['name'],x['id'],1 if x['folder'] else 8) for x in self.playlists if x['parent']==identity]
            else:
                playlist=next((x for x in self.playlists if x['id']==identity and not x['folder']),None)
                if playlist is not None:rows=[self.item(self.tracks[i]['title'],i,4,identity,p+1) for p,i in enumerate(playlist['tracks']) if i in self.tracks]
        elif kind==0x3000 and len(args)>=3:
            rows=menus.get(key)
            if rows is None:return [(0x4003,[kind,0])]
            offset,limit=args[1:3]
            if not 0<=offset<=len(rows) or not 1<=limit<=64:return [(0x4003,[kind,0])]
            return [(0x4001,[1,offset])]+[(0x4101,row) for row in rows[offset:offset+limit]]+[(0x4201,[1,0])]
        if rows is None:return [(0x4003,[kind,0])]
        if len(menus)>=16 and key not in menus:return [(0x4003,[kind,0])]
        menus[key]=rows
        return [(0x4000,[kind,len(rows)])]
