# SPDX-License-Identifier: GPL-3.0-only
"""Explicit contiguous documentation-address fixtures, never physical LAN aliases."""
import ipaddress

def source_mac(parent,slot):
    """Mirror the versioned candidate identity for isolated DHCP transport only."""
    native=bytes.fromhex(parent.replace(':',''))
    if len(native)!=6 or native[0]&1 or not any(native) or not 0<=slot<8:
        raise ValueError('Expected unicast controller MAC and slot0..7')
    value=14695981039346656037
    for byte in b'usb-hub-support-public-mac-v1'+native:
        value=((value^byte)*1099511628211)&((1<<64)-1)
    result=bytearray(value.to_bytes(8,'little')[:6]);result[0]=(result[0]&252)|2;result[5]=(result[5]&248)|slot
    if result[:5]==native[:5] and result[5]&248==native[5]&248:result[0]^=4
    return bytes(result)


def fixture_addresses(count):
    if not 1 <= count <= 8:
        raise ValueError('Expected one to eight fixture addresses')
    return ['192.0.2.'+str(n) for n in range(2,2+count)]


def verify_namespace(rows,count=1,macvlan=False,link_local=False):
    if macvlan:
        if link_local:raise ValueError('Mixed MACVLAN/link-local fixture is not declared')
        return verify_macvlan(rows,count)
    peers=[row for row in rows if row.get('ifname')!='lo']
    if len(peers)!=1 or peers[0].get('ifname')!='azlink0':
        raise ValueError('Expected only lo and isolated azlink0')
    peer=peers[0]
    if 'UP' not in peer.get('flags',[]):
        raise ValueError('TAP must already be up')
    actual=sorted((a.get('local'),a.get('prefixlen'),a.get('broadcast'))
        for a in peer.get('addr_info',[]) if a.get('family')=='inet')
    expected=[(a,24,'192.0.2.255') for a in fixture_addresses(count)]
    if link_local:expected=sorted(expected+[('169.254.2.2',16,'169.254.255.255')])
    if actual!=expected:
        raise ValueError('Unexpected isolated fixture addresses')
    for a in peer.get('addr_info',[]):
        if a.get('family')=='inet6' and not ipaddress.ip_address(a['local']).is_link_local:
            raise ValueError('Unexpected global IPv6 address')


def verify_macvlan(rows,count):
    addresses=fixture_addresses(count)
    expected={'lo','azlink0'}|{'azpub'+str(n) for n in range(1,count+1)}
    if {r.get('ifname') for r in rows}!=expected:
        raise ValueError('Unexpected interface in virtual-MAC fixture')
    by_name={r['ifname']:r for r in rows}
    parent=by_name['azlink0']
    if ('UP' not in parent.get('flags',[]) or parent.get('addr_info') or
        parent.get('linkinfo',{}).get('info_kind')!='tun' or
        parent.get('linkinfo',{}).get('info_data',{}).get('type')!='tap'):
        raise ValueError('Parent TAP must be up and unnumbered')
    for index,address in enumerate(addresses,1):
        row=by_name['azpub'+str(index)]
        if ('UP' not in row.get('flags',[]) or
            not (row.get('link')=='azlink0' or
                 (parent.get('ifindex') is not None and row.get('link_index')==parent['ifindex'])) or
            row.get('linkinfo',{}).get('info_kind')!='macvlan' or
            row.get('address')!=f'02:00:00:00:11:{40+index:02x}'):
            raise ValueError('Unexpected virtual MAC parent/type/address')
        inet=[(a.get('local'),a.get('prefixlen'),a.get('broadcast'))
              for a in row.get('addr_info',[]) if a.get('family')=='inet']
        if inet!=[(address,24,'192.0.2.255')]:
            raise ValueError('Unexpected virtual MAC IPv4 address')
        if any(a.get('family')=='inet6' for a in row.get('addr_info',[])):
            raise ValueError('IPv6 not expected in virtual-MAC fixture')
