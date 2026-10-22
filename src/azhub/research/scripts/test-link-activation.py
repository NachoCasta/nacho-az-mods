#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import unittest
from link_activation import Activation


class Handshake(unittest.TestCase):
    def test_first_ack_delayed_repeat_then_activation(self):
        state=Activation(18)
        first=state.request(10)
        self.assertEqual((len(first),first[10],first[33]),(48,0x16,18))
        self.assertIsNone(state.poll(12.99))
        self.assertEqual(state.poll(13),first)
        self.assertIsNone(state.poll(14))
        active=state.request(15)
        self.assertEqual((len(active),active[10],active[33],active[36],active[37]),(72,0x47,18,18,4))
        self.assertEqual(active[40:44],bytes.fromhex('12345678'))
        self.assertEqual(state.request(20),active)
    def test_provider_state_is_independent(self):
        a,b=Activation(17),Activation(18)
        a.request(0);a.request(1)
        self.assertEqual(b.request(2)[10],0x16)
        self.assertEqual((a.count,b.count),(2,1))


if __name__=='__main__':unittest.main()
