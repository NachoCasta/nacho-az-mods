#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise the reporting-only DHCP hook without a client or network traffic."""
import os
from pathlib import Path
import subprocess

hook=Path(__file__).with_name('public-dhcp-event-hook.sh').resolve()
base={'PATH':'/nonexistent','AZHUB_DHCP_EVENT_FD':'3','AZHUB_DHCP_INTERFACE':'azpub0',
      'interface':'azpub0','ip':'192.0.2.3','subnet':'255.255.255.0','lease':'3600','serverid':'192.0.2.1'}
def run(event,**changes):
    env=dict(base);env.update(changes)
    return subprocess.run(['/bin/sh','-c','exec 3>&1; exec /bin/sh "$1" "$2"','hook-test',str(hook),event],
                          env=env,capture_output=True,text=True,timeout=5)
for event in ('bound','renew'):
    result=run(event);assert result.returncode==0
    assert result.stdout==f'1\t{event}\tazpub0\t192.0.2.3\t255.255.255.0\t3600\t192.0.2.1\n'
for event in ('deconfig','nak','leasefail'):
    result=run(event);assert result.returncode==0
    assert result.stdout==f'1\t{event}\tazpub0\t-\t-\t-\t-\n'
for changes in ({'ip':'$(touch /tmp/azhub-dhcp-unwanted)'},{'lease':'1\nINJECT'},
                {'subnet':''},{'serverid':'1.2.3.4\tINJECT'},{'lease':'9'*10000}):
    result=run('bound',**changes);assert result.returncode==0
    assert result.stdout=='1\tinvalid\tazpub0\t-\t-\t-\t-\n'
for changes in ({'interface':'eth0'},{'AZHUB_DHCP_EVENT_FD':''},
                {'interface':'azpub0\n','AZHUB_DHCP_INTERFACE':'azpub0\n'}):
    result=run('bound',**changes);assert result.returncode!=0 and not result.stdout
result=run('unknown');assert result.returncode!=0 and not result.stdout
print('PASS: reporting-only DHCP events, explicit interface/fd guard and hostile environment fields; no external commands available')
