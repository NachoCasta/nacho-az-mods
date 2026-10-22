#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import unittest
from link_fixture_network import fixture_addresses,verify_namespace,source_mac


class Network(unittest.TestCase):
    def test_declared_link_local_peer_only(self):
        rows=self.rows(1)
        rows[1]['addr_info'].append({'family':'inet','local':'169.254.2.2','prefixlen':16,'broadcast':'169.254.255.255'})
        verify_namespace(rows,link_local=True)
        with self.assertRaises(ValueError):verify_namespace(rows)
        rows[1]['addr_info'][-1]['local']='169.254.2.3'
        with self.assertRaises(ValueError):verify_namespace(rows,link_local=True)
    def test_derived_source_mac_vectors(self):
        parent='52:54:00:12:34:56'
        self.assertEqual(source_mac(parent,0).hex(),'42d2bad9aa10')
        self.assertEqual(source_mac(parent,4).hex(),'42d2bad9aa14')
        self.assertEqual(len({source_mac(parent,slot) for slot in range(8)}),8)
        for bad,slot in [('00:00:00:00:00:00',0),('01:00:00:00:00:01',0),(parent,8)]:
            with self.assertRaises(ValueError):source_mac(bad,slot)
    def rows(self,count):
        return [{'ifname':'lo'},{'ifname':'azlink0','flags':['UP'],'addr_info':[
            {'family':'inet','local':a,'prefixlen':24,'broadcast':'192.0.2.255'}
            for a in fixture_addresses(count)]}]
    def test_exact_one_two_eight_fixtures(self):
        for count in (1,2,8):verify_namespace(self.rows(count),count)
    def test_virtual_mac_fixture_requires_exact_parent_kind_mac_and_addresses(self):
        import copy
        rows=[{'ifname':'lo'},{'ifname':'azlink0','ifindex':2,'flags':['UP'],'addr_info':[],
            'linkinfo':{'info_kind':'tun','info_data':{'type':'tap'}}}]
        for i,address in enumerate(fixture_addresses(2),1):
            rows.append({'ifname':'azpub'+str(i),'link':'azlink0','flags':['UP'],
                'address':f'02:00:00:00:11:{40+i:02x}','linkinfo':{'info_kind':'macvlan'},
                'addr_info':[{'family':'inet','local':address,'prefixlen':24,'broadcast':'192.0.2.255'}]})
        verify_namespace(rows,2,True)
        for field,value in [('link','eth0'),('address','02:00:00:00:00:99')]:
            wrong=copy.deepcopy(rows);wrong[2][field]=value
            with self.assertRaises(ValueError):verify_namespace(wrong,2,True)
        wrong=copy.deepcopy(rows);wrong[1]['linkinfo']['info_kind']='ether'
        with self.assertRaises(ValueError):verify_namespace(wrong,2,True)
        with self.assertRaises(ValueError):verify_namespace(rows+[{'ifname':'eth0'}],2,True)

    def test_nat_suppression_covers_only_declared_fixture_addresses(self):
        import importlib.util
        spec=importlib.util.spec_from_file_location('transport',__import__('pathlib').Path(__file__).with_name('link-primary-tap-transport.py'))
        transport=importlib.util.module_from_spec(spec);spec.loader.exec_module(transport)
        import socket
        owned=tuple(socket.inet_aton(a) for a in fixture_addresses(2))
        for address,expected in (('192.0.2.2',True),('192.0.2.3',True),('192.0.2.15',False)):
            frame=bytearray(34);frame[12:14]=b'\x08\x00';frame[14]=0x45;frame[16:18]=(20).to_bytes(2,'big');frame[26:30]=socket.inet_aton(address)
            self.assertEqual(transport.owned_peer(frame,owned),expected)
            arp=bytearray(42);arp[12:14]=b'\x08\x06';arp[14:20]=b'\x00\x01\x08\x00\x06\x04';arp[28:32]=socket.inet_aton(address)
            self.assertEqual(transport.owned_peer(arp,owned),expected)

    def test_extra_addresses_interfaces_and_wrong_scope_refused(self):
        for rows,count in ((self.rows(2),1),(self.rows(1),2),
                (self.rows(1)+[{'ifname':'eth0'}],1)):
            with self.assertRaises(ValueError):verify_namespace(rows,count)
        rows=self.rows(2);rows[1]['addr_info'][1]['local']='10.0.0.3'
        with self.assertRaises(ValueError):verify_namespace(rows,2)
        for count in (0,9):
            with self.assertRaises(ValueError):fixture_addresses(count)


if __name__=='__main__':unittest.main()
