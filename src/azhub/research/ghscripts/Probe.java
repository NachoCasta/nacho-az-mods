// Headless helper for EP147 research.
// args: a list of commands, each one of
//   dec:<hexaddr>      decompile the function containing addr
//   xref:<hexaddr>     list callers / references to addr (with containing function)
//   str:<text>         find defined strings containing text, then their referencing functions
// Output goes to stdout (println).
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;

public class Probe extends GhidraScript {
	private DecompInterface dec;
	private java.io.PrintWriter out;

	@Override
	public void println(String s) {
		out.println(s);
		out.flush();
	}

	private String fname(Address a) {
		Function f = getFunctionContaining(a);
		return f == null ? "?" : f.getName() + "@" + f.getEntryPoint();
	}

	private void decompile(Address a) {
		Function f = getFunctionContaining(a);
		if (f == null) {
			println("== no function at " + a);
			return;
		}
		DecompileResults r = dec.decompileFunction(f, 120, monitor);
		println("== decompile " + f.getName() + " @ " + f.getEntryPoint());
		println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "FAILED: " + r.getErrorMessage());
	}

	private void xref(Address a) {
		println("== xrefs to " + a + " (" + fname(a) + ")");
		for (Reference r : getReferencesTo(a))
			println("  " + r.getFromAddress() + " " + r.getReferenceType() + " in " + fname(r.getFromAddress()));
	}

	private void str(String text) {
		println("== strings containing '" + text + "'");
		DataIterator it = currentProgram.getListing().getDefinedData(true);
		while (it.hasNext() && !monitor.isCancelled()) {
			Data d = it.next();
			if (!d.hasStringValue())
				continue;
			String v = String.valueOf(d.getValue());
			if (!v.contains(text))
				continue;
			println(" " + d.getAddress() + " \"" + v + "\"");
			for (Reference r : getReferencesTo(d.getAddress()))
				println("    <- " + r.getFromAddress() + " in " + fname(r.getFromAddress()));
		}
	}

	@Override
	public void run() throws Exception {
		out = new java.io.PrintWriter(new java.io.FileWriter(System.getProperty("user.home") + "/probe.out"));
		dec = new DecompInterface();
		dec.openProgram(currentProgram);
		for (String cmd : getScriptArgs()) {
			int i = cmd.indexOf(':');
			String k = cmd.substring(0, i), v = cmd.substring(i + 1);
			if (k.equals("dec"))
				decompile(toAddr(Long.parseLong(v, 16)));
			else if (k.equals("xref"))
				xref(toAddr(Long.parseLong(v, 16)));
			else if (k.equals("str"))
				str(v);
			else if (k.equals("vt")) {
				// vt:<hexaddr>:<count>  list (and decompile) vtable slots
				String[] p = v.split(":");
				Address base = toAddr(Long.parseLong(p[0], 16));
				int n = Integer.parseInt(p[1]);
				for (int j = 0; j < n; j++) {
					long t = getLong(base.add(8L * j));
					println("== vt slot " + j + " -> " + Long.toHexString(t));
					if (t != 0)
						decompile(toAddr(t));
				}
			}
		}
		dec.dispose();
	}
}
