#!/usr/bin/env python3
"""Adversarial stream framing controls for the isolated public backend prototype."""
import struct
import unittest
import link_db_wire as wire


class Stream:
    def __init__(self,data,chunk=1):self.data=data;self.chunk=chunk
    def recv(self,size):
        size=min(size,self.chunk);out=self.data[:size];self.data=self.data[size:];return out


class Framing(unittest.TestCase):
    def test_fragmented_coalesced_and_native_short_type_array(self):
        first=wire.packet(1,0x4101,[7,'Fixture \U0001f3b5',b'\x00\x01'])
        second=wire.packet(2,0x4000,[0,17])
        short=second[:16]+struct.pack('!I',2)+second[20:22]+second[32:]
        for chunk in (1,2,7,65536):
            stream=Stream(first+short,chunk)
            self.assertEqual(wire.message(stream)['arguments'],[7,'Fixture \U0001f3b5',b'\x00\x01'])
            self.assertEqual(wire.message(stream),{'transaction':2,'type':0x4000,'arguments':[0,17]})
            self.assertEqual(stream.data,b'')
    def test_extended_rows_and_short_numeric_tag(self):
        frame=wire.packet(1,0x4101,list(range(16)))
        self.assertEqual(wire.message(Stream(frame))['arguments'],list(range(16)))
        frame=bytearray(wire.packet(1,0x4000,[0,17]));frame[20]=4;frame[21]=5
        self.assertEqual(wire.message(Stream(bytes(frame)))['arguments'],[0,17])
    def test_elided_empty_blob_preserves_following_fields_and_message(self):
        first=wire.packet(1,0x3000,[0x01010401,0,20,0,20,0,0,b''])
        second=wire.packet(2,0x3000,[0,b'',7])
        stream=Stream(first+second)
        self.assertEqual(wire.message(stream)['arguments'],[0x01010401,0,20,0,20,0,0,b''])
        self.assertEqual(wire.message(stream)['arguments'],[0,b'',7])
        self.assertEqual(stream.data,b'')
        with self.assertRaises(ValueError):wire.packet(3,0,[b''])
    def test_payload_bounds_and_truncation(self):
        with self.assertRaises(ValueError):wire.field(Stream(b'\x14'+struct.pack('!I',65537)))
        with self.assertRaises(EOFError):wire.message(Stream(wire.packet(1,0x4000,[0,17])[:-1]))
        with self.assertRaises(ValueError):wire.field(Stream(b'\x26'+struct.pack('!I',1)+b'AB'))
    def test_type_and_header_rejection(self):
        frame=bytearray(wire.packet(1,0x4000,[0,17]));frame[20]=2
        with self.assertRaises(ValueError):wire.message(Stream(bytes(frame)))
        frame=wire.packet(1,0x4000,[0,17]);bad=frame[:16]+struct.pack('!I',3)+frame[20:23]+frame[32:]
        with self.assertRaises(ValueError):wire.message(Stream(bad))
        with self.assertRaises(ValueError):wire.packet(1,0,[0]*17)


if __name__=='__main__':unittest.main()
