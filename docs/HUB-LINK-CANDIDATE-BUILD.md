# Local sharing candidate reproduction

This procedure creates an ignored local candidate and source snapshot. It never
installs, reloads, publishes, edits a catalog or moves a tag. Published0.3.2 stays
unchanged. Draft0.4.0 metadata is applied only inside the staging directory.

Use the pinned Linaro6.3.1 AArch64 compiler and SDK1.2.3
(`62a156abf3a4789e9e3cdd610fe106227a3a8967`). Set `TASK_CC_PREFIX` to the absolute
toolchain prefix ending in `aarch64-linux-gnu-`; set `TASK_SDK_INCLUDE` to the
absolute directory containing `azmod-mod.h` and `TASK_PACKER` to the matching
preserved SDK `tools/azmod-pack`. These are local input paths, not controller
paths. From this checkout in Linux/WSL:

```sh
make -C src/azhub/linkfake sharing-runtime-candidate sharing-broker-candidate \
  CC="${TASK_CC_PREFIX}gcc" AZ_SDK_INCLUDE="$TASK_SDK_INCLUDE" \
  OUT=../../../local/integration/sharing-candidate
```

Reproduce the target MACVLAN build using
[the transport procedure](HUB-LINK-TRANSPORT-BUILD.md). Its prepared kernel,
driver and compiler hashes must match; compilation/static exports are not target
runtime acceptance. The existing deliberate GPL hub-depth module and its source
remain separate from this MACVLAN module.

```sh
python3 src/azhub/research/scripts/prepare-sharing-candidate.py \
  --out local/publishing/sharing-candidate-reproduction \
  --runtime local/integration/sharing-candidate/usb-hub-support-sharing-runtime-candidate.so \
  --helper local/integration/sharing-candidate/usb-hub-support-network-helper-candidate \
  --macvlan-build local/integration/sharing-macvlan-rebuild \
  --packer "$TASK_PACKER" --version 0.4.0
```

The output directory must be new. The script stages an explicit allowlist,
invokes the unchanged packer and package validator, verifies0755 archive modes
for both executable assets, and records package/member/input hashes. The helper
is an executable ELF despite its `.so` asset suffix; the DHCP hook is a shell
script with the same suffix. The suffix follows the pinned packer's executable
mode rule; neither requires a core modification or production chmod.

`source.tar` includes project C/headers, build files, research control scripts,
Java clients, candidate documentation, required original assets and a candidate
manifest overlay. The receipt hashes each included file. MACVLAN source/license/
build material is included under `candidate/macvlan-source`; obtain the complete
matching vendor GPL kernel/toolchain independently as documented. Firmware,
music, extracted libraries, Ghidra projects, screenshots and local diagnostics
are never part of this allowlist. This is a source snapshot, not a source-commit
claim for the dirty worktree.

The package must still pass relevant final host/emulator regressions and receive
separately authorized physical AZ/CDJ trials. Target kernel4.4.194 module loading,
actual model-specific discovery/loading and audio handling remain unproven by
packaging. Read RELEASING.md again before any publication decision.
