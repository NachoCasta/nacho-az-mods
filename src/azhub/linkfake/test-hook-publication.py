#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Enter a hook before claim_hook returns, using the production wrapper."""
import subprocess
import tempfile
from pathlib import Path

from importlib.machinery import SourceFileLoader

registration = SourceFileLoader('registration', str(Path(__file__).with_name('test-hook-registration.py'))).load_module()
source = registration.preprocess('community.c', ('-DLF_EJECT_UI', '-DLF_SAFE_EJECT', '-DLF_EJECT_AZ'))
wrapper = registration.body(source, 'make_hook')
prefix = '''
#include <stdint.h>
#include <assert.h>
typedef uint64_t (*callback_t)(void);
static callback_t original;
static int lf_hook_failures, entered, saw_null;
static uint64_t native(void) { return 42; }
static uint64_t callback(void) {
    entered++;
    if (!original) { saw_null++; return 0; }
    return original();
}
static int claim(const char *id, uintptr_t address, void *hook, void **out) {
    (void)id; (void)address;
    *out = (void *)native;
    ((callback_t)hook)(); /* Native entry after publication, before return. */
    return 0;
}
static struct { int (*claim_hook)(const char *, uintptr_t, void *, void **); } core = {claim}, *lf_core = &core;
'''
old = '''static void *old_hook(uintptr_t target, void *hook) {
    void *result = 0;
    lf_core->claim_hook("usb-hub-support", target, hook, &result);
    return result;
}
'''
main = '''
int main(void) {
    original = old_hook(0x237f900, (void *)callback);
    assert(entered == 1 && saw_null == 1); /* Unfixed negative control. */
    entered = saw_null = 0;
    original = 0;
    make_hook(0x237f900, (void *)callback, (void **)&original);
    assert(entered == 1 && saw_null == 0 && original() == 42);
    assert(lf_hook_failures == 0);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='hook-publication-') as temporary:
    path = Path(temporary)
    test = path / 'test.c'
    test.write_text(prefix + 'static void make_hook(uintptr_t target, void *hook, void **original) {' + wrapper + '}\n' + old + main)
    for flags in (['-O2'], ['-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all']):
        subprocess.run(registration.CC + flags + [str(test), '-o', str(path / 'test')], check=True)
        subprocess.run([str(path / 'test')], check=True)
print('PASS: callback during claim return window; old wrapper observes null, production wrapper calls native (plain/ASan+UBSan)')
