# Sharing transport kernel build research

The emulator prototype uses MACVLAN, while the prepared target4.4.194 config
has MACVLAN/IPVLAN disabled. A separate module can be built locally from the
unmodified vendor GPL driver. This is build feasibility, not runtime acceptance.
No module was installed or loaded and the public mod remains0.3.2.

Run from this checkout inside Linux/WSL with your matching prepared vendor GPL
tree and its Linaro6.3.1 prefix. Use a new ignored output directory:

```sh
python3 src/azhub/research/scripts/build-sharing-macvlan.py \
  --kernel /path/to/prepared/XDJ-AZ/kernel \
  --compiler-prefix /path/to/pinned/bin/aarch64-linux-gnu- \
  --out local/integration/sharing-macvlan-rebuild
python3 src/azhub/research/scripts/audit-kernel-module-exports.py \
  --kernel /path/to/prepared/XDJ-AZ/kernel \
  --undefined local/integration/sharing-macvlan-rebuild/undefined-symbols.txt \
  --out local/integration/sharing-macvlan-rebuild/source-export-audit.json
```

The builder rejects different config/source hashes, kernel release or compiler
version. It copies the GPL source only into ignored local output and names the
module `usb_hub_macvlan`. The GPL license/author metadata and driver source are
unchanged. It does not modify the prepared config or install/deploy anything.
Generated binaries and copied vendor source must remain local until a separate
reviewed release decision, including source/license distribution requirements.

`CONFIG_MACVLAN_MODULE=1` is a module-only compiler flag. Prepared headers contain
one MACVLAN conditional: the inline `macvlan_dev_real_dev` helper switches between
pointer lookup and a BUG fallback. This review finds no MACVLAN-gated shared
structure fields under include/. It is not a complete runtime ABI proof.

The existing vendor build wrapper expects an unavailable `python` executable.
The explicit pinned gcc override matches the established kernel build procedure;
it avoids modifying system Python or vendor code. Build logs retain that earlier
failure separately. The build record contains exact source/config/module hashes.

Module.symvers is absent. All79 undefined symbols have GPL source export
declarations, but conditional compilation and the running target's actual export
table have not been verified. Matching vermagic and a successful static scan do
not prove that the module loads or works. Do not load this4.4 module in the6.6
emulator or infer physical support from its built-in MACVLAN.

Next review runtime symbol/ABI compatibility, integrate exact firmware/kernel
guards and owned module/device cleanup, and implement address conflict handling
and the live per-stick backend. Physical actions require separate authorization.
