#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Actual selector-open body with SDK surface/focus stand-ins; no firmware hooks."""
import os,re,shlex,subprocess,tempfile
from pathlib import Path
root=Path(__file__).resolve().parent
source=(root/'eject-ui.h').read_text()
match=re.search(r'static void eject_open\(int port\)\s*\{',source)
assert match
depth=1
for end in range(match.end(),len(source)):
    depth+=(source[end]=='{')-(source[end]=='}')
    if not depth:break
else:raise AssertionError('eject_open body not found')
body=source[match.end():end]
harness=r'''
#define main view_input_control_main
#include "test-eject-view-input.c"
#undef main
#include "eject-input-health.h"
static struct hub_input_health eject_input_health;
static az_surface *eject_encoder_surface,*eject_touch_surface;
static unsigned eject_seen_raw,eject_seen_edit;
static uint64_t eject_raw_seen_ms,eject_edit_seen_ms;
static int eject_touch_view,panel_present,view_now,allow_focus;
static unsigned focus_requests,shown;
static int eject_panel_present(void) {return panel_present;}
static int view(void) {return view_now;}
static int request_focus(az_surface *s,int on) {
    assert(s==eject_encoder_surface);
    if(on) {focus_requests++;return allow_focus;}
    focus_releases++;return 1;
}
static void set_show(az_surface *s,int on) {
    assert(s==eject_encoder_surface || s==eject_touch_surface);
    if(on)shown++;else hide_calls++;
}
static void set_place(az_surface *s,az_rect r) {
    assert(s==eject_encoder_surface || s==eject_touch_surface);
    assert((r.w==720 && r.h==500) || (r.w==1 && r.h==1));
}
'''
harness+='static void eject_open(int port) {'+body+'}\n'
harness+=r'''
static void fresh(void) {
    reset();eject_active=0;eject_capture=0;eject_focus_owned=0;
    eject_encoder_surface=(az_surface *)(uintptr_t)1;
    eject_touch_surface=(az_surface *)(uintptr_t)2;
    eject_surface=eject_encoder_surface;
    eject_input_health=(struct hub_input_health){.armed=1};
    panel_present=allow_focus=1;view_now=AZ_VIEW_SOURCE;focus_requests=shown=0;
}
int main(void) {
    assert(!view_input_control_main());
    static az_core_v1 api={.surface_show=set_show,.surface_place=set_place,
        .surface_redraw=redraw,.surface_focus=request_focus,.view=view,
        .log=log_message,.banner=banner};lf_core=&api;
    fresh();eject_open(1);
    assert(eject_active && !eject_touch_only && eject_capture && eject_focus_owned);
    assert(focus_requests==1 && shown==1 && eject_surface==eject_encoder_surface);
    fresh();eject_input_health.fault=HUB_INPUT_FAULT_MASK;eject_keys_down=0x81;
    eject_open(1);
    assert(eject_active && eject_touch_only && !eject_capture && !eject_focus_owned);
    assert(!focus_requests && shown==1 && eject_surface==eject_touch_surface);
    eject_close();assert(!eject_active && !eject_closing && !focus_releases);
    /* A quarantined filter never needs re-registration or a restart. */
    for(unsigned n=0;n<10000;n++) {
        eject_open(1);assert(eject_touch_only && eject_active && !eject_capture);
        eject_close();assert(!focus_requests && !focus_releases);
    }
    fresh();eject_input_health.fault=HUB_INPUT_RAW_MISSING;eject_touch_surface=NULL;
    eject_open(1);assert(!eject_active && !eject_capture && !shown && !focus_requests);
    fresh();panel_present=0;eject_open(1);assert(!shown && !focus_requests && !eject_active);
    fresh();view_now=AZ_VIEW_MENU;eject_open(1);assert(!shown && !focus_requests && !eject_active);
    fresh();eject_closing=1;eject_open(1);assert(!shown && !focus_requests && !eject_active);
    fresh();allow_focus=0;eject_open(1);
    assert(!eject_active && !eject_capture && !eject_focus_owned && focus_requests==1 && hide_calls==1);
    fresh();eject_input_health.fault=HUB_INPUT_EDIT_MISSING;eject_open(1);
    assert(eject_touch_only && !focus_requests);
    eject_input_health.fault=0;eject_open(1);
    assert(!eject_touch_only && eject_capture && eject_focus_owned && focus_requests==1 && hide_calls==1);
    puts("selector open: normal focus, touch after faults,10000 retries, recovered mode, missing panel/surface, competing focus/menu and held-close refusals passed");
}
'''
compiler=shlex.split(os.environ.get('TEST_CC','cc'))
sdk=os.environ.get('AZ_SDK_INCLUDE','../../azmod')
flags={'plain':['-O2'],'asan':['-g','-O1','-fsanitize=address,undefined','-fno-sanitize-recover=undefined'],
       'tsan':['-g','-O1','-fsanitize=thread']}
with tempfile.TemporaryDirectory(prefix='azhub-selector-') as temp:
    src=Path(temp)/'test.c';src.write_text(harness)
    for mode in os.environ.get('SAN','plain').split():
        output=Path(temp)/mode
        subprocess.run(compiler+flags[mode]+['-Wall','-Wextra','-Wno-unused-function','-I'+str(root),
            '-I'+sdk,'-pthread',str(src),'-o',str(output)],check=True)
        subprocess.run([str(output)],check=True)
