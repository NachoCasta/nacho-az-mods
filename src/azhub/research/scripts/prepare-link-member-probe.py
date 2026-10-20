#!/usr/bin/env python3
"""Generate an emulator-only member 7/8 research overlay, without deployment.

The default retains the physical Link filter. Explicit experiment flags enable
isolated-network coexistence probes. Every variant rejects the hardware kernel;
none is a release candidate or permission to test on a physical network.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil


def main():
    root = Path(__file__).resolve().parents[4]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=root / 'local/diagnostics/link-member-probe')
    parser.add_argument('--private-loopback', action='store_true',
                        help='Probe scoped native admission for 127.77.0.7/8; still blocks physical Link')
    parser.add_argument('--trace-library-ownership', action='store_true',
                        help='Probe private dispatch admission using matching client socket tuples')
    parser.add_argument('--external-link-probe', action='store_true',
                        help='Isolated-emulator protocol experiment; NOT safe for physical networks')
    parser.add_argument('--mask-private-status', action='store_true',
                        help='Probe hiding known source identity fields on external AZ status')
    parser.add_argument('--udp-ingress-probe', action='store_true',
                        help='Collect kernel ingress metadata and reject known private UDP claims')
    parser.add_argument('--mode-recovery-probe', action='store_true',
                        help='Reannounce sources after observed native deferred mode completion')
    parser.add_argument('--udp-read-audit', action='store_true',
                        help='Log bounded receive-call metadata on isolated Link UDP ports')
    parser.add_argument('--mode-load-audit', action='store_true',
                        help='Log native unload identity/state during mode transitions')
    parser.add_argument('--mode-load-retention-probe', action='store_true',
                        help='Retain mode2 load provenance with a captured physical USB identity')
    parser.add_argument('--load-queue-probe', action='store_true',
                        help='Trace both native load queue stages; allow one-shot emulator delay markers')
    args = parser.parse_args()
    if args.external_link_probe and not (args.private_loopback and args.trace_library_ownership):
        parser.error('External probe requires private loopback and library ownership experiments')
    if args.mask_private_status and not args.external_link_probe:
        parser.error('Status mask requires the isolated external probe')
    if args.udp_ingress_probe and not args.external_link_probe:
        parser.error('UDP ingress probe requires the isolated external probe')
    if args.mode_recovery_probe and not args.external_link_probe:
        parser.error('Mode recovery probe requires the isolated external probe')
    if args.udp_read_audit and not args.udp_ingress_probe:
        parser.error('UDP read audit requires the isolated UDP ingress probe')
    if args.mode_load_audit and not args.mode_recovery_probe:
        parser.error('Mode load audit requires the mode recovery experiment')
    if args.mode_load_retention_probe and not args.mode_load_audit:
        parser.error('Mode load retention requires mode load diagnostics')
    if args.load_queue_probe and not args.mode_load_retention_probe:
        parser.error('Load queue probe requires the load retention experiment')
    destination = args.output.resolve()
    if not destination.is_relative_to(root / 'local') or destination.exists():
        parser.error('Output must be a new directory under this repository local/')
    source = root / 'src/azhub/linkfake'
    destination.mkdir(parents=True)
    original = {}
    for file in source.iterdir():
        if file.is_file() and (file.suffix in ('.c', '.h') or file.name == 'Makefile'):
            shutil.copyfile(file, destination / file.name)
            original[file.name] = hashlib.sha256(file.read_bytes()).hexdigest()

    replacements = {
        'linkfake.c': [
            ('static const int players[]={3,3,4,4,3,4,3,4};',
             'static const int players[]={7,7,8,8,7,8,7,8};'),
            ('hub_peer_ips[f->player-3]', 'hub_peer_ips[f->player-7]'),
            ('f->player<3 || f->player>4', 'f->player<7 || f->player>8'),
            ('player<3 || player>4', 'player<7 || player>8'),
        ],
        'discovery.h': [('static const unsigned players[]={3,3,4,4,3,4,3,4};',
                         'static const unsigned players[]={7,7,8,8,7,8,7,8};')],
        'alias-mount.h': [('/media/player03/', '/media/player07/'),
                          ('/media/player04/', '/media/player08/')],
        'community.c': [
            ('    lf_core=core;\n', '''    struct utsname probe_kernel;
    if (uname(&probe_kernel) || strcmp(probe_kernel.release,"6.6.138-dirty")) return -35;
    core->log("usb-hub-support","MEMBER 7/8 FEASIBILITY PROBE: emulator only, physical Link still filtered");
    lf_core=core;
''')],
    }
    if args.private_loopback:
        header = Path(__file__).with_name('link-private-admission-probe.h')
        shutil.copyfile(header, destination / header.name)
        replacements['linkfake.c'].append((
            'struct in_addr peer={.s_addr=hub_peer_ips[f->player-7]};',
            'struct in_addr peer={.s_addr=htonl(0x7f4d0000u | (unsigned)f->player)};'))
        replacements['linkfake.c'].append((
            'if (f) a->sin_addr = f->ip;',
            'if (f) a->sin_addr.s_addr = hub_peer_ips[f->player-7];'))
        replacements['community.c'].extend([
            ('static int community_init(const az_core_v1 *core) {',
             '#include "link-private-admission-probe.h"\n\nstatic int community_init(const az_core_v1 *core) {'),
            ('    __atomic_store_n(&community_ready,1,__ATOMIC_RELEASE);',
             '    probe_private_admission_install();\n'
             '    if (lf_hook_failures) return community_abort(-36);\n'
             '    __atomic_store_n(&community_ready,1,__ATOMIC_RELEASE);'),
        ])
    if args.trace_library_ownership:
        header = Path(__file__).with_name('link-library-ownership-probe.h')
        shutil.copyfile(header, destination / header.name)
        registry = Path(__file__).with_name('link-socket-registry.h')
        shutil.copyfile(registry, destination / registry.name)
        requester = Path(__file__).with_name('link-private-requester.h')
        shutil.copyfile(requester, destination / requester.name)
        replacements['linkfake.c'].extend([
            ('static unsigned char traced[4096];',
             '#include "link-socket-registry.h"\n'
             '#include "link-private-requester.h"\n'
             'static struct link_socket_registry probe_sockets=LINK_SOCKET_REGISTRY_INIT;\n'
             'static unsigned char traced[4096];'),
            ('    int requester=rq ? atoi(rq) : 0;\n'
             '    if (requester && getenv("LINKFAKE_PEER_REQUESTERS") && fd>=0 && fd<4096 && target_of_fd[fd]>=3)\n'
             '        requester+=target_of_fd[fd]-3;',
             '    int requester=fd>=0 && fd<4096 ? probe_requester_for_player(target_of_fd[fd]):0;'),
            ('    if (fd >= 0 && fd < 4096) traced[fd] = 0; /* a reused fd number is a new connection */',
             '    link_socket_forget(&probe_sockets,fd);\n'
             '    if (fd >= 0 && fd < 4096) traced[fd] = 0; /* a reused fd number is a new connection */'),
            ('        int r = real_connect(fd, (void *)&lo, sizeof lo);',
             '        struct link_socket_token ownership=link_socket_begin(&probe_sockets,fd,f->player);\n'
             '        int r = real_connect(fd, (void *)&lo, sizeof lo);\n'
             '        int connect_errno=errno;\n'
             '        link_socket_finish(&probe_sockets,ownership,r==0 || connect_errno==EINPROGRESS);\n'
             '        errno=connect_errno;'),
            ('    if (fd >= 0 && fd < 4096) srv_fd[fd] = 0;',
             '    link_socket_forget(&probe_sockets,fd);\n'
             '    if (fd >= 0 && fd < 4096) srv_fd[fd] = 0;'),
            ('static int hub_slot_code(int player, int slot);',
             'static int hub_slot_code(int player, int slot);\n'
             'static int probe_message_allowed(uint64_t message,int player);\n'
             'static uint64_t probe_dispatch(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);'),
            ('return o_dispense(a, b, c, d, e, f, g, h);',
             'return probe_dispatch(a, b, c, d, e, f, g, h);'),
        ])
        replacements['hubservers.h'] = [
            ('if (fakes[i].present\n',
             'if (fakes[i].present && probe_message_allowed(message,fakes[i].player)\n'),
        ]
        replacements['community.c'].extend([
            ('static int community_init(const az_core_v1 *core) {',
             '#include "link-library-ownership-probe.h"\n\nstatic int community_init(const az_core_v1 *core) {'),
            ('    __atomic_store_n(&community_ready,1,__ATOMIC_RELEASE);',
             '    probe_library_ownership_install();\n'
             '    if (lf_hook_failures) return community_abort(-37);\n'
             '    __atomic_store_n(&community_ready,1,__ATOMIC_RELEASE);'),
        ])
    if args.external_link_probe:
        replacements['community.c'].extend([
            ('    setenv("LINKFAKE_LOCAL_ONLY","1",1);',
             '    unsetenv("LINKFAKE_LOCAL_ONLY");'),
            ('MEMBER 7/8 FEASIBILITY PROBE: emulator only, physical Link still filtered',
             'ISOLATED EXTERNAL LINK PROBE: emulator only; network provenance and status privacy incomplete'),
        ])
    if args.mask_private_status:
        replacements.setdefault('safe-eject.h', []).append((
            'static struct { uintptr_t controller; uint64_t epoch; int valid,ticket; } safe_cached_decks[4];',
            'static struct { uintptr_t controller; uint64_t epoch; uint32_t content_id; int valid,ticket; } safe_cached_decks[4];'))
        replacements.setdefault('native-detach.h', []).append((
            'safe_cached_decks[deck].epoch=load->epoch;',
            'safe_cached_decks[deck].epoch=load->epoch;\n            safe_cached_decks[deck].content_id=load->content_id;'))
        header = Path(__file__).with_name('link-cached-status-probe.h')
        shutil.copyfile(header, destination / header.name)
        replacements['community.c'].append((
            '#include "safe-eject.h"',
            '#include "safe-eject.h"\n#include "link-cached-status-probe.h"'))
        replacements['linkfake.c'].append((
            'static int hub_slot_code(int player, int slot);',
            'static int hub_slot_code(int player, int slot);\n'
            'static int probe_cached_status_private(const uint8_t *,size_t);'))
        replacements['linkfake.c'].append((
            '    if (!f) return real_sendto(fd, buf, n, flags, to, tl);',
            '''    if (!f && a && (ntohl(a->sin_addr.s_addr)>>24)!=127 &&
        ntohs(a->sin_port)==50002 && n==1152) {
        uint8_t external_status[1152];
        memcpy(external_status,buf,n);
        int private_locations=hub_status_hide_private_locations(external_status,n);
        if (probe_cached_status_private(external_status,n) &&
            hub_status_hide_source(external_status,n,external_status[0x28],external_status[0x29]))
            return real_sendto(fd,external_status,n,flags,to,tl);
        for (int source=0;source<nfakes;source++)
            if (hub_status_hide_source(external_status,n,fakes[source].player,fakes[source].slot)) {
                return real_sendto(fd,external_status,n,flags,to,tl);
            }
        if(private_locations) return real_sendto(fd,external_status,n,flags,to,tl);
    }
    if (!f) return real_sendto(fd, buf, n, flags, to, tl);'''))
    if args.udp_ingress_probe:
        for name in ('link-udp-ingress-probe.h', 'link-udp-receive.h', 'link-udp-policy.h'):
            shutil.copyfile(Path(__file__).with_name(name), destination / name)
        original_link = (source / 'linkfake.c').read_text(encoding='utf-8')
        start = original_link.index('ssize_t recvfrom(')
        end = original_link.index('int connect(', start)
        replacements['linkfake.c'].extend([
            (original_link[start:end], '#include "link-udp-ingress-probe.h"\n\n'),
            ('        f->port=address.sin_port;',
             '        f->port=address.sin_port;\n        probe_udp_sender_add(f->sock,f->player);'),
            ('    if (fd >= 0 && fd < 4096) srv_fd[fd] = 0;',
             '    probe_udp_forget(fd);\n    if (fd >= 0 && fd < 4096) srv_fd[fd] = 0;'),
        ])
    if args.udp_read_audit:
        name = 'link-udp-read-audit.h'
        shutil.copyfile(Path(__file__).with_name(name), destination / name)
        replacements['linkfake.c'].extend([
            ('#include "link-udp-ingress-probe.h"',
             '#include "link-udp-read-audit.h"\n#include "link-udp-ingress-probe.h"'),
            ('    ssize_t r = real_read(fd, b, n);',
             '    probe_udp_read_audit("read",fd,n,0,(uintptr_t)__builtin_return_address(0));\n'
             '    ssize_t r = real_read(fd, b, n);'),
            ('    ssize_t r = real_recv(fd, b, n, fl);',
             '    probe_udp_read_audit("recv",fd,n,fl,(uintptr_t)__builtin_return_address(0));\n'
             '    ssize_t r = real_recv(fd, b, n, fl);'),
        ])
        replacements['link-udp-ingress-probe.h'] = [(
            '    if(!lf_runtime_ready()) return real_recvfrom(fd,buffer,size,flags,source,length);',
            '    probe_udp_read_audit("recvfrom",fd,size,flags,(uintptr_t)__builtin_return_address(0));\n'
            '    if(!lf_runtime_ready()) return real_recvfrom(fd,buffer,size,flags,source,length);')]
    if args.mode_recovery_probe:
        header = Path(__file__).with_name('link-mode-recovery-probe.h')
        shutil.copyfile(header, destination / header.name)
        replacements['linkfake.c'].append((
            'static void hub_mount_poll(void);',
            'static void hub_mount_poll(void);\nstatic void probe_mode_reset_sources(time_t *,int *);'))
        replacements['hubservers.h'].append((
            '    if (!started) started=now;',
            '    if (!started) started=now;\n    probe_mode_reset_sources(&ready_at,&waiting);'))
        replacements['community.c'].extend([
            ('static void community_enable_internal_link(void) {',
             'static void probe_mode_maintain_cache(void);\n'
             'static void community_enable_internal_link(void) { probe_mode_maintain_cache(); }\n'
             'static void community_enable_internal_link_unlocked(void) {'),
            ('static int community_init(const az_core_v1 *core) {',
             '#include "link-mode-recovery-probe.h"\n\nstatic int community_init(const az_core_v1 *core) {'),
            ('    __atomic_store_n(&community_ready,1,__ATOMIC_RELEASE);',
             '    probe_mode_recovery_install();\n'
             '    if (lf_hook_failures) return community_abort(-38);\n'
             '    __atomic_store_n(&community_ready,1,__ATOMIC_RELEASE);'),
        ])
    if args.mode_load_audit:
        replacements.setdefault('safe-eject.h', []).append((
            'lf_core->log(EJECT_ID,"safe-eject unload-completed controller=%#lx",root);',
            'lf_core->log(EJECT_ID,"safe-eject unload-completed controller=%#lx state=%d mode=%u media=%#lx epoch=%llu",'
            'root,native_state,native_mode,media,(unsigned long long)epoch);'))
    if args.mode_load_retention_probe:
        shutil.copyfile(Path(__file__).with_name('link-load-origin-probe.h'),
                        destination / 'link-load-origin-probe.h')
        replacements['safe-eject.h'].extend([
            ('#include "eject-detach.h"',
             '#include "eject-detach.h"\n#include "link-load-origin-probe.h"'),
            ('    uint64_t epoch,media;',
             '    uint64_t epoch,media;\n    struct eject_item origin;\n    int origin_valid;\n'
             '    int committed_valid,committed_source;\n'
             '    uint32_t committed_content_id;\n'
             '    uint64_t committed_media,committed_epoch;\n'
             '    struct eject_item committed_origin;'),
            ('static struct safe_load safe_loads[16];',
             'static struct safe_load safe_loads[16];\n'
             '#define PROBE_COMMITTED_STATUS_ORIGIN 1\n'
             'static int safe_usb_identity(const char *,struct eject_item *);\n'
             'static int safe_same(const struct eject_item *,const struct eject_item *);'),
            ('    int source=safe_source_identity(*(uint64_t *)(record+0x38)),denied=0;',
             '''    int source=safe_source_identity(*(uint64_t *)(record+0x38)),denied=0;
    struct eject_item origin={0};char origin_path[256]={0};
    pthread_mutex_lock(&safe_source_mutex);
    if(source>=0 && source<nfakes && fakes[source].present)
        snprintf(origin_path,sizeof origin_path,"%s",fakes[source].dir);
    pthread_mutex_unlock(&safe_source_mutex);
    int origin_valid=*origin_path && safe_usb_identity(origin_path,&origin);
'''),
            ('        load->source=source; load->loading=1;',
             '        load->source=source; load->loading=1;\n'
             '        load->origin=origin;load->origin_valid=origin_valid;'),
            ('if (hub_detach_unload_matches(native_state,load->loading,load->media,media)) load->source=-1;',
             'if (hub_detach_unload_matches(native_state,load->loading,load->media,media) &&\n'
             '            !(native_mode==2 && load->origin_valid)) load->source=-1;'),
            ('        else safe_cached_forget_locked(load->deck);',
             '''        else {
            safe_cached_forget_locked(load->deck);
            /* Pending and failed replacements never lend their identity to the
             * still-loaded track. Publish only matched successful completion. */
            load->committed_valid=load->source>=0 && load->origin_valid;
            load->committed_source=load->source;
            load->committed_content_id=load->content_id;
            load->committed_media=load->media;
            load->committed_epoch=load->epoch;
            load->committed_origin=load->origin;
        }'''),
            ('        int deck=load->deck;\n        if (native_state==0',
             '''        if (native_state==0 && !load->loading && native_mode!=2 &&
            load->committed_valid && load->committed_media==hub_detach_media(media))
            load->committed_valid=0;
        int deck=load->deck;
        if (native_state==0'''),
        ])
        replacements.setdefault('native-detach.h', []).append((
            '        uint64_t connection=safe_native_connection_locked(source);',
            '''        int origin_matches=0;
        if(load) for(int i=0;i<SAFE_RECORDS;i++)
            if(probe_load_origin_matches(load->origin_valid,&load->origin,
                &safe_records[i].item,safe_records[i].sources,source)) origin_matches=1;
        if(!origin_matches) load=NULL;
        uint64_t connection=safe_native_connection_locked(source);'''))
    if args.load_queue_probe:
        replacements['community.c'].append(('    ,.tick=eject_tick', '    ,.tick=probe_queue_tick'))
        shutil.copyfile(Path(__file__).with_name('link-request-tasks.h'),
                        destination / 'link-request-tasks.h')
        shutil.copyfile(Path(__file__).with_name('link-load-queue-probe.h'),
                        destination / 'link-load-queue-probe.h')
        replacements['safe-eject.h'].extend([
            ('static int safe_same(const struct eject_item *,const struct eject_item *);',
             'static int safe_same(const struct eject_item *,const struct eject_item *);\n'
             'static int probe_request_admission_origin(uintptr_t,uintptr_t,struct eject_item *);'),
            ('    int origin_valid=*origin_path && safe_usb_identity(origin_path,&origin);',
             '    int origin_valid=source<0 || probe_request_admission_origin(controller,record,&origin);'),
            ('        denied=!load || fakes[source].eject_reserved;',
             '        denied=!load || fakes[source].eject_reserved || !origin_valid;'),
            ('    safe_message("Cannot load from a USB that is being ejected.");\n    /* Native DownloadResult',
             '    safe_message(origin_valid ? "Cannot load from a USB that is being ejected.":\n'
             '        "USB changed while loading. Select the track again.");\n    /* Native DownloadResult'),
            ('static void (*safe_original_load_common)(uintptr_t,uintptr_t,uintptr_t);',
             'static void (*safe_original_load_common)(uintptr_t,uintptr_t,uintptr_t);\n'
             'static void probe_request_begin(uintptr_t,uintptr_t,uintptr_t);\n'
             'static void probe_request_end(void);'),
            ('    safe_original_load_common(controller,track,options);\n}',
             '    probe_request_begin(controller,track,options);\n'
             '    safe_original_load_common(controller,track,options);\n'
             '    probe_request_end();\n}'),
        ])
        replacements['community.c'].extend([
            ('static int community_init(const az_core_v1 *core) {',
             '#include "link-load-queue-probe.h"\n\nstatic int community_init(const az_core_v1 *core) {'),
            ('    __atomic_store_n(&community_ready,1,__ATOMIC_RELEASE);',
             '    probe_queue_install();\n'
             '    if (lf_hook_failures) return community_abort(-39);\n'
             '    __atomic_store_n(&community_ready,1,__ATOMIC_RELEASE);'),
        ])
    for name, pairs in replacements.items():
        path = destination / name
        text = path.read_text(encoding='utf-8')
        for before, after in pairs:
            if text.count(before) != 1:
                raise RuntimeError(f'{name}: expected exactly one replacement for {before!r}')
            text = text.replace(before, after)
        path.write_bytes(text.encode('utf-8'))
    evidence = {
        'purpose': 'Emulator-only member 7/8 research; not a release candidate',
        'physical_link_filter_retained': not args.external_link_probe,
        'isolated_external_link_experiment': args.external_link_probe,
        'private_status_prefix_mask': args.mask_private_status,
        'partial_udp_ingress_guard': args.udp_ingress_probe,
        'native_mode_completion_recovery': args.mode_recovery_probe,
        'udp_read_audit': args.udp_read_audit,
        'mode_load_audit': args.mode_load_audit,
        'mode_load_retention_probe': args.mode_load_retention_probe,
        'load_queue_probe': args.load_queue_probe,
        'real_kernel_rejected_before_initialization': True,
        'production_source_changed': False,
        'private_loopback_admission_probe': args.private_loopback,
        'library_socket_ownership_observation': args.trace_library_ownership,
        'experimental_private_dispatch_guard': args.trace_library_ownership,
        'private_requesters': {'7': 5, '8': 6} if args.trace_library_ownership else None,
        'base_source_sha256': original,
        'overlay_source_sha256': {
            p.name: hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(destination.iterdir()) if p.is_file()
        },
    }
    (destination / 'overlay.json').write_text(json.dumps(evidence, indent=2) + '\n', encoding='utf-8')
    print(destination)


if __name__ == '__main__':
    main()
