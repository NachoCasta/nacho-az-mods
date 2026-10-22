#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Real socket controls for concurrent metadata sessions and bounded shutdown."""
import importlib.util
import json
from pathlib import Path
import socket
import tempfile
import threading
import unittest
from link_session_pool import SessionPool
from link_snapshot_backend import SnapshotBackend
import link_db_wire as wire


def module(name):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(name+'.py'))
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result

provider=module('link-public-provider-probe');guard=module('link-library-peer')


class ConcurrentSessions(unittest.TestCase):
    def setUp(self):
        temp=tempfile.TemporaryDirectory();self.addCleanup(temp.cleanup)
        path=Path(temp.name)/'snapshot.json'
        path.write_text(json.dumps({'format':1,'tracks':[{'id':1,'title':'Alpha','bpm100':12000}],
            'playlists':[{'id':2,'parent':0,'name':'Set','folder':False,'tracks':[1]}]}))
        self.backend=SnapshotBackend(path);self.stop=threading.Event();self.pool=SessionPool(2)
        self.addCleanup(self.pool.close)

    def connect(self):
        server,client=socket.socketpair();client.settimeout(1);self.addCleanup(client.close)
        accepted=self.pool.submit(server,lambda owned:provider.client_session(self.stop,owned,1051,17,guard,self.backend))
        return client,accepted

    def setup(self,client):
        client.sendall(guard.number(1));self.assertEqual(guard.field(client),1)
        client.sendall(wire.packet(1,0,[3]));self.assertEqual(wire.message(client)['arguments'],[0,17])

    def test_idle_client_does_not_block_second_and_menus_remain_independent(self):
        first,ok=self.connect();self.assertTrue(ok);self.setup(first)
        first.sendall(wire.packet(2,0x1004,[0x03010401,0]));wire.message(first)
        second,ok=self.connect();self.assertTrue(ok);self.setup(second)
        second.sendall(wire.packet(2,0x1105,[0x03010401,0,0,1]));wire.message(second)
        for client,title in ((second,'Set'),(first,'Alpha')):
            client.sendall(wire.packet(3,0x3000,[0x03010401,0,1]))
            self.assertEqual(wire.message(client)['type'],0x4001)
            self.assertEqual(wire.message(client)['arguments'][3],title)
            self.assertEqual(wire.message(client)['type'],0x4201)
        self.pool.close();self.assertFalse(self.pool.sessions)

    def test_capacity_refuses_extra_socket_and_shutdown_interrupts_greeting(self):
        first,ok=self.connect();self.assertTrue(ok)
        second,ok=self.connect();self.assertTrue(ok)
        third,ok=self.connect();self.assertFalse(ok);self.assertEqual(third.recv(1),b'')
        self.pool.close();self.assertFalse(self.pool.sessions)
        last,ok=self.connect();self.assertFalse(ok);self.assertEqual(last.recv(1),b'')


if __name__=='__main__':unittest.main()
