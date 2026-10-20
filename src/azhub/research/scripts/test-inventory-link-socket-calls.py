#!/usr/bin/env python3
"""Conservative argument-inventory checks; no firmware fixture required."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('inventory', Path(__file__).with_name('inventory-link-socket-calls.py'))
inventory = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inventory)

class Arguments(unittest.TestCase):
    def test_constant_copy_and_zero(self):
        self.assertEqual(inventory.argument([('0','mov','w5, #1'),('4','mov','w1, w5')],1),1)
        self.assertEqual(inventory.argument([('0','mov','w3, wzr')],3),0)
    def test_call_and_branch_stop_resolution(self):
        for instruction in ['bl','blr','b','b.ne','cbz','tbz','ret']:
            self.assertIsNone(inventory.argument([('0','mov','w1, #1'),('4',instruction,'x0')],1))
    def test_unknown_and_second_destination(self):
        self.assertIsNone(inventory.argument([('0','mov','w1, #1'),('4','ldp','x0, x1, [sp]')],1))
        self.assertIsNone(inventory.argument([('0','ldr','w2, [x0]')],2))
    def test_register_width(self):
        self.assertEqual(inventory.argument([('0','mov','x5, #0x100000001'),('4','mov','w1, w5')],1),1)
    def test_no_invented_value(self):
        self.assertIsNone(inventory.argument([('0','mov','w1, w20')],1))
        self.assertIsNone(inventory.argument([],1))
    def test_tail_receive_and_separate_read(self):
        calls, imports = inventory.scan([
            '00000200 <recvfrom@plt>:\n',
            ' 100: 52800003 mov w3, #0\n',
            ' 104: 1400003f b 200 <recvfrom@plt>\n',
            ' 108: 52802002 mov w2, #0x100\n',
            ' 10c: 9400007d bl 300 <read@plt>\n'])
        self.assertEqual(imports, {'recvfrom':'0x200'})
        self.assertEqual(calls[0]['transfer'], 'b')
        self.assertEqual(calls[0]['flags'], 0)
        self.assertEqual(calls[1]['function'], 'read')
        self.assertEqual(calls[1]['buffer_size'], 256)
    def test_indirect_transfer_is_not_claimed_as_direct(self):
        calls, _ = inventory.scan([' 100: d63f0000 blr x0\n'])
        self.assertEqual(calls, [])

if __name__ == '__main__':
    unittest.main()
