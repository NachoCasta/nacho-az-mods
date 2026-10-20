#!/usr/bin/env python3
"""Prepare an offline, source-frozen emulator request-delay variant; never deploy.

The supported candidate remains untouched. One USB-resident marker pauses a
private common-stage request after provenance capture and before the existing
stale check. The normal cancellation/admission implementation is unchanged.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--freeze", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--marker", required=True)
    args = parser.parse_args()
    source = args.source.resolve(strict=True)
    output = args.output.resolve()
    workspace = Path(__file__).resolve().parents[4]
    if workspace not in output.parents or output.exists():
        parser.error("Output must be a new directory inside this worktree")
    if not re.fullmatch(r"/media/usb/sd[a-z]+1/MODS/v5-request-delay-common", args.marker):
        parser.error("Use an explicit mods-USB partition and the fixed marker basename")
    freeze = json.loads(args.freeze.read_text(encoding="utf-8"))
    inputs = freeze["production_source_files"]
    for row in inputs:
        path = source / row["path"]
        if source not in path.resolve().parents or digest(path) != row["sha256"]:
            parser.error("Source differs from the frozen candidate: " + row["path"])
    # Preserve the same build/package inputs without generated candidate binaries.
    shutil.copytree(source, output, ignore=shutil.ignore_patterns("out", "__pycache__"))
    target = output / "link-request-origin.h"
    original = target.read_text(encoding="utf-8")
    anchor = "static uint64_t probe_queue_common_body(uint64_t a,uint64_t b,uint64_t c,uint64_t d,\n    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {\n"
    assert original.count(anchor) == 1
    injection = '''    /* Emulator-only fault fixture: no production cancellation changes. */
    if(lf_runtime_ready() && probe_request_depth && probe_request_depth<=8) {
        struct probe_request_scope *captured=&probe_request_scopes[probe_request_depth-1];
        struct utsname kernel;
        if(captured->valid && !uname(&kernel) && !strncmp(kernel.release,"6.6.",4) &&
            !unlink("MARKER")) {
            lf_log("request-delay begin controller=%#lx media=%#lx content=%u type=%u generation=%llu devnum=%u",
                (unsigned long)captured->controller,(unsigned long)captured->media,
                captured->content_id,captured->load_type,captured->origin.generation,
                captured->origin.devnum);
            struct timespec remaining={8,0};
            while(nanosleep(&remaining,&remaining) && errno==EINTR) {}
            lf_log("request-delay end controller=%#lx",(unsigned long)captured->controller);
        }
    }
'''.replace("MARKER", args.marker)
    changed = original.replace(anchor, anchor + injection, 1)
    assert changed.replace(injection, "", 1) == original
    target.write_text(changed, encoding="utf-8", newline="\n")
    for row in inputs:
        if row["path"] != "link-request-origin.h":
            assert digest(output / row["path"]) == row["sha256"]
    record = {
        "scope": "Offline emulator-only diagnostic variant; not built or installed",
        "supported_source_freeze": str(args.freeze.resolve()),
        "source_freeze_sha256": digest(args.freeze),
        "verified_production_files": len(inputs),
        "changed_file": "link-request-origin.h",
        "original_sha256": digest(source / "link-request-origin.h"),
        "diagnostic_sha256": digest(target),
        "marker": args.marker,
        "delay_seconds": 8,
        "kernel_guard": "6.6.* only",
        "effects": "Consumes USB marker; pauses captured private request before existing stale check; no new hook or cancellation path",
        "supported_candidate_modified": False,
    }
    (output / "request-delay-provenance.json").write_text(
        json.dumps(record, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(record))


if __name__ == "__main__":
    main()
