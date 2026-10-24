#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Run the production recovery helper against disposable sysfs fixtures only."""
import os
from pathlib import Path
import subprocess
import tempfile

SOURCE = Path(__file__).with_name('assets') / 'recover-hubs.sh'


def check(kernel='4.4.194', module=True, interface='2-1:1.0', device_class='09',
          driver=None, controller='fe3c0000.usb', expected=False, status=0):
    with tempfile.TemporaryDirectory(prefix='azhub-startup-') as temporary:
        root = Path(temporary)
        devices = root / 'sys/bus/usb/devices'
        devices.mkdir(parents=True)
        bind = root / 'sys/bus/usb/drivers/hub/bind'
        bind.parent.mkdir(parents=True)
        bind.write_text('')
        if module:
            (root / 'sys/module/azhub').mkdir(parents=True)
        bus = interface.split('-', 1)[0]
        target = root / 'sys/devices/platform' / controller / ('usb' + bus) / interface
        target.mkdir(parents=True)
        if device_class is not None:
            (target / 'bInterfaceClass').write_text(device_class + '\n')
        if driver:
            destination = bind.parent if driver == 'bound' else root / 'missing-driver'
            (target / 'driver').symlink_to(destination)
        (devices / interface).symlink_to(target)
        commands = root / 'commands'
        commands.mkdir()
        uname = commands / 'uname'
        uname.write_text('#!/bin/sh\nprintf "%s\\n" "' + kernel + '"\n')
        uname.chmod(0o755)
        script = root / 'recover.sh'
        # Substitute literal fixture roots in a private copy, never add a
        # production environment override that could target arbitrary paths.
        script.write_text(SOURCE.read_text().replace('/sys/', str(root / 'sys') + '/'))
        result = subprocess.run(['sh', str(script), 'startup'],
                                env={**os.environ, 'PATH': str(commands) + ':' + os.environ['PATH']},
                                capture_output=True, text=True, timeout=5)
        assert result.returncode == status, (result.returncode, result.stderr)
        assert bind.read_text() == (interface + '\n' if expected else '')
        assert ('hub startup retry ' in result.stdout) == expected, result.stdout


check(expected=True)
check(interface='1-1:1.0', controller='fe380000.usb', expected=True)
check(interface='2-1.2:1.0', expected=True)
check(controller='fe380000.usb')
check(driver='bound')
check(driver='dangling')
check(interface='2-0:1.0')
check(interface='usb2')
check(interface='2-1')
check(interface='2-1:1.0;false')
check(device_class='08')
check(device_class='03')
check(device_class=None)
check(controller='internal-controller')
check(module=False, status=3)
check(kernel='4.4.195', status=3)
check(kernel='6.6.138-dirty', controller='emulator-pci', module=False, expected=True)
print('PASS: startup helper retries only unbound external hubs after kernel readiness; native devices, bound/root/foreign hubs and unsupported kernels untouched')
