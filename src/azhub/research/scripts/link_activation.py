# SPDX-License-Identifier: GPL-3.0-only
"""Bounded public computer activation control, no persistent settings writes."""
class Activation:
    def __init__(self,number):
        self.number=number;self.count=0;self.repeat_at=None

    def packet(self,kind):
        data=bytearray(48 if kind==0x16 else 72)
        data[:10]=b'Qspt1WmJOL';data[10]=kind;data[11:20]=b'rekordbox'
        data[31:34]=bytes((1,1,self.number))
        if kind==0x47:
            data[35:38]=bytes((36,self.number,4))
            data[40:44]=bytes.fromhex('12345678');data[47]=1
            data[48:54]=bytes((1,2,3,1,2,1))
        return bytes(data)

    def request(self,now):
        self.count+=1
        if self.count==1:
            self.repeat_at=now+3
            return self.packet(0x16)
        return self.packet(0x47)

    def poll(self,now):
        if self.repeat_at is not None and now>=self.repeat_at:
            self.repeat_at=None
            return self.packet(0x16)
        return None
