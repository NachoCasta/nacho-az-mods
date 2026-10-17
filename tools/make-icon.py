#!/usr/bin/env python3
"""Render our original hub icon using only the Python standard library."""
import struct,zlib
from pathlib import Path
W=256;pixels=bytearray(bytes((22,25,30))*W*W)
def rect(x,y,w,h,c):
 for j in range(y,y+h):
  for i in range(x,x+w):pixels[(j*W+i)*3:(j*W+i)*3+3]=bytes(c)
blue=(75,172,239);orange=(255,154,30);dark=(22,25,30);body=(49,57,69)
rect(119,45,18,44,orange);rect(105,29,46,38,orange);rect(115,35,8,15,dark);rect(133,35,8,15,dark)
rect(45,86,166,70,body)
for x in (63,115,167):rect(x,110,26,22,orange)
rect(119,156,18,28,blue);rect(59,175,138,10,blue)
for x in (56,115,174):
 rect(x,175,26,30,blue);rect(x-7,201,40,27,blue);rect(x+1,209,8,10,dark);rect(x+17,209,8,10,dark)
def chunk(t,data):return struct.pack('>I',len(data))+t+data+struct.pack('>I',zlib.crc32(t+data)&0xffffffff)
raw=b''.join(b'\0'+pixels[y*W*3:(y+1)*W*3] for y in range(W))
png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',W,W,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(raw,9))+chunk(b'IEND',b'')
p=Path(__file__).resolve().parents[1]/'src/azhub/linkfake/icon.png';p.write_bytes(png);print(p)
