#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise actual preprocessed installers, without claiming firmware hooks."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
CC = shlex.split(os.environ.get('TEST_CC', 'cc'))
HISTORICAL_CC = shlex.split(os.environ.get('TEST_HISTORICAL_CC', 'gcc'))
SDK = os.environ.get('AZ_SDK_INCLUDE', '../../azmod')
INSTALLERS = {
    'probe_install': {0xef05c8: 'probe_parse'},
    'probe_private_admission_install': {0x13df908: 'probe_network_snapshot', 0x13f22b8: 'probe_operate'},
    'probe_library_ownership_install': {0xf00de0: 'probe_message_destroy', 0xeee540: 'probe_handshake'},
    'probe_mode_recovery_install': {0x140a088: 'probe_mode_complete', 0x1408ef0: 'probe_mode_request', 0x13fa7a8: 'probe_mode_network'},
    'probe_member_continuity_install': {0x13e0448: 'probe_member_disconnect', 0x13e18d8: 'probe_member_clear'},
    'probe_queue_install': dict(zip(
        (0xaae298, 0x237f900, 0xaab598, 0xaab818, 0xa692e0, 0xa693c0,
         0xa7c848, 0xa7c9d8, 0x6d1838, 0xa77390, 0xa69db0),
        ('probe_id_start', 'probe_queue_pop', 'probe_id_failure_destroy',
         'probe_id_failure_delete', 'probe_allow_destroy', 'probe_allow_delete',
         'probe_queue_destroy', 'probe_queue_delete', 'probe_queue_post',
         'probe_queue_common', 'probe_queue_allow'))),
}


def body(source, name):
    match = re.search(r'\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', source)
    assert match, 'Missing compiled function: ' + name
    start = match.end()
    depth = 1
    for index in range(start, len(source)):
        depth += (source[index] == '{') - (source[index] == '}')
        if not depth:
            return source[start:index]
    raise AssertionError('Unclosed compiled function: ' + name)


def preprocess(file, flags=()):
    return subprocess.check_output(CC + ['-E', '-P', '-I' + SDK, *flags, file],
                                   cwd=ROOT, text=True)


def check(source, directory):
    # Check the entire translation unit, including nested installers, for zero
    # or duplicate literal parser registrations surviving the real build flags.
    calls = re.findall(r'make_hook\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(\w+)\s*\)', source)
    assert [(a, h) for a, h in calls if int(a, 16) == 0xef05c8] == [('0xef05c8', 'probe_parse')], 'Parser must be claimed exactly once'
    init = body(source, 'community_init')
    ordered = ['setenv("LINKFAKE_PROBE","1",1)', 'probe_install()',
               'probe_private_admission_install()', 'probe_library_ownership_install()',
               'probe_mode_recovery_install()', 'probe_queue_install()',
               'source_badge_install()',
               '__atomic_store_n(&community_ready,1,3)']
    compact = re.sub(r'\s+', '', init)
    positions = [compact.index(token) for token in ordered]
    assert positions == sorted(positions), 'Installers must precede worker readiness'
    assert 'o_dbparse(a, b, c, d, e, f, g, h)' in body(source, 'h_dbparse')
    assert 'h_dbparse(a,b,c,d,e,f,g,h)' in re.sub(r'\s+', '', body(source, 'probe_parse'))
    assert '__atomic_load_n(&community_ready,2)==1' in re.sub(r'\s+', '', body(source, 'lf_runtime_ready'))
    assert 'community_ready' in body(source, 'injector')
    assert 'community_ready' in body(source, 'community_mount_worker')
    # Compile and execute the surviving installer bodies with a recording core.
    # This detects runtime omissions (e.g. a new false guard), callback mistakes,
    # missing trampoline assignments and duplicate claims across installers.
    bodies = {name: body(source, name) for name in INSTALLERS}
    assignments = re.findall(r'(\w+)\s*=\s*make_hook\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(\w+)\s*\)', '\n'.join(bodies.values()))
    variables = sorted({a for a, _, _ in assignments} | {'o_dbparse'})
    callbacks = sorted({c for _, _, c in assignments})
    harness = '#include <stdint.h>\n#include <assert.h>\n#include <stdlib.h>\n'
    harness += 'typedef void (*fn)(void); static void native(void) {}\n'
    harness += 'static uintptr_t addresses[256]; static fn handlers[256]; static int count;\n'
    harness += 'static fn make_hook(uintptr_t a,fn h){assert(count<256); addresses[count]=a; handlers[count++]=h; return native;}\n'
    harness += '#define getenv(x) ((char *)"1")\n#define lf_log(...) ((void)0)\n'
    harness += 'static void hub_install(void){} static void hub_tag_cache_install(void){}\n'
    harness += ''.join('static fn ' + name + ';\n' for name in variables)
    harness += ''.join('static void ' + name + '(void){}\n' for name in callbacks)
    harness += ''.join('static void ' + name + '(void);\n' for name in bodies)
    harness += ''.join('static void ' + name + '(void){' + content + '}\n' for name, content in bodies.items())
    # The member installer must be reached through the mode installer; calling
    # it separately would hide a missing nested installation or double claims.
    harness += 'int main(void){' + ''.join(name + '();' for name in INSTALLERS if name != 'probe_member_continuity_install')
    harness += 'assert(o_dbparse==native);'
    for expected in INSTALLERS.values():
        for address, callback in expected.items():
            harness += '{int n=0;for(int i=0;i<count;i++)if(addresses[i]==' + hex(address) + '){n++;assert(handlers[i]==' + callback + ');}assert(n==1);}'
    harness += 'return 0;}\n'
    test = directory / 'registration.c'
    binary = directory / 'registration'
    test.write_text(harness)
    subprocess.run(CC + ['-Wall', '-Wextra', str(test), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)


def main():
    supported = preprocess('community.c', ('-DLF_EJECT_UI', '-DLF_SAFE_EJECT', '-DLF_EJECT_AZ'))
    with tempfile.TemporaryDirectory(prefix='azhub-hooks-') as temporary:
        directory = Path(temporary)
        check(supported, directory)
        for replacement in ('', 'o_dbparse=make_hook(0xef05c8,probe_parse);' * 2):
            broken, count = re.subn(r'o_dbparse\s*=\s*make_hook\(0xef05c8,\s*probe_parse\);', replacement, supported)
            assert count == 1
            try:
                check(broken, directory)
            except AssertionError:
                continue
            raise AssertionError('Regression checker accepted zero/double parser registration')
    historical = preprocess('linkfake.c')
    assert re.findall(r'make_hook\s*\(0xef05c8,\s*(\w+)\)', historical) == ['h_dbparse']
    # The retained standalone research branch uses GCC atomic builtins on C11
    # atomic objects. Compile it with the GCC family used by the ARM SDK;
    # Clang still preprocesses and executes the supported registration controls.
    subprocess.run(HISTORICAL_CC + ['-fsyntax-only', '-I' + SDK, 'linkfake.c'], cwd=ROOT, check=True)
    print('PASS: supported installers, parser trampoline/readiness, zero/double negative controls; historical branch compiles')


if __name__ == '__main__':
    main()
