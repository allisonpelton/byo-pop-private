// Exports everything the porting effort needs from an analyzed program:
//   functions.jsonl  - one record per function (name, namespace, signature, callers, callees, referenced strings)
//   strings.jsonl    - defined strings (ascii + utf16) with referencing functions
//   vtables.jsonl    - every RTTI "vftable" symbol with its slots resolved to functions
//   decomp/<ns>/<addr>_<name>.c - decompiled C for every function
//   all.c            - every decompiled function concatenated in address order
// Usage (headless): -postScript ExportAll.java <outDir>
//@category Export
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.app.decompiler.parallel.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.*;
import ghidra.util.task.TaskMonitor;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class ExportAll extends GhidraScript {

	private Path outDir;
	private final Map<Address, List<String>> funcStrings = new HashMap<>();

	static String esc(String s) {
		if (s == null) return "null";
		StringBuilder b = new StringBuilder("\"");
		for (char c : s.toCharArray()) {
			switch (c) {
				case '"': b.append("\\\""); break;
				case '\\': b.append("\\\\"); break;
				case '\n': b.append("\\n"); break;
				case '\r': b.append("\\r"); break;
				case '\t': b.append("\\t"); break;
				default:
					if (c < 0x20 || c > 0x7e) b.append(String.format("\\u%04x", (int) c));
					else b.append(c);
			}
		}
		return b.append('"').toString();
	}

	static String arr(Collection<String> xs) {
		StringBuilder b = new StringBuilder("[");
		boolean first = true;
		for (String x : xs) {
			if (!first) b.append(',');
			b.append(esc(x));
			first = false;
		}
		return b.append(']').toString();
	}

	static String safe(String s) {
		String r = s.replaceAll("[^A-Za-z0-9_.-]", "_");
		return r.length() > 80 ? r.substring(0, 80) : r;
	}

	private BufferedWriter open(String name) throws IOException {
		return Files.newBufferedWriter(outDir.resolve(name), StandardCharsets.UTF_8);
	}

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		outDir = Paths.get(args.length > 0 ? args[0] : "re");
		Files.createDirectories(outDir.resolve("decomp"));
		exportStrings();
		exportVtables();
		exportFunctions();
		exportDecompiled();
		println("ExportAll done -> " + outDir.toAbsolutePath());
	}

	private void exportStrings() throws Exception {
		Listing listing = currentProgram.getListing();
		ReferenceManager rm = currentProgram.getReferenceManager();
		FunctionManager fm = currentProgram.getFunctionManager();
		int n = 0;
		try (BufferedWriter w = open("strings.jsonl")) {
			DataIterator it = listing.getDefinedData(true);
			while (it.hasNext() && !monitor.isCancelled()) {
				Data d = it.next();
				if (!d.hasStringValue()) continue;
				Object v = d.getValue();
				if (v == null) continue;
				String s = v.toString();
				Set<String> refs = new TreeSet<>();
				for (Reference r : rm.getReferencesTo(d.getAddress())) {
					Function f = fm.getFunctionContaining(r.getFromAddress());
					if (f != null) {
						refs.add(f.getEntryPoint().toString());
						funcStrings.computeIfAbsent(f.getEntryPoint(), k -> new ArrayList<>()).add(s);
					}
					else {
						refs.add("data:" + r.getFromAddress());
					}
				}
				w.write("{\"addr\":" + esc(d.getAddress().toString()) + ",\"type\":" +
					esc(d.getDataType().getName()) + ",\"value\":" + esc(s) + ",\"xrefs\":" +
					arr(refs) + "}\n");
				n++;
			}
		}
		println("strings: " + n);
	}

	private void exportVtables() throws Exception {
		SymbolTable st = currentProgram.getSymbolTable();
		Memory mem = currentProgram.getMemory();
		FunctionManager fm = currentProgram.getFunctionManager();
		AddressSetView text = null;
		MemoryBlock tb = mem.getBlock(".text");
		if (tb != null) text = new AddressSet(tb.getStart(), tb.getEnd());
		int n = 0;
		try (BufferedWriter w = open("vtables.jsonl")) {
			SymbolIterator it = st.getSymbolIterator(true);
			while (it.hasNext()) {
				Symbol s = it.next();
				String name = s.getName();
				if (!name.equals("vftable") && !name.startsWith("vftable_") && !name.endsWith("_vftable")) continue;
				Address a = s.getAddress();
				StringBuilder slots = new StringBuilder("[");
				for (int i = 0; i < 400; i++) {
					Address slot = a.add(4L * i);
					if (i > 0) {
						// stop at the next labeled symbol (next vtable / RTTI locator)
						Symbol[] syms = st.getSymbols(slot);
						boolean stop = false;
						for (Symbol x : syms) if (x.getSource() != SourceType.DEFAULT) stop = true;
						if (stop) break;
					}
					int p;
					try { p = mem.getInt(slot); } catch (Exception e) { break; }
					Address target = a.getNewAddress(p & 0xffffffffL);
					if (text == null || !text.contains(target)) break;
					Function f = fm.getFunctionAt(target);
					if (i > 0) slots.append(',');
					slots.append("{\"i\":" + i + ",\"addr\":" + esc(target.toString()) + ",\"name\":" +
						esc(f == null ? null : f.getSymbol().getName(true)) + "}");
				}
				slots.append(']');
				w.write("{\"class\":" + esc(s.getParentNamespace().getName(true)) + ",\"symbol\":" +
					esc(s.getName(true)) + ",\"addr\":" + esc(a.toString()) + ",\"slots\":" + slots + "}\n");
				n++;
			}
		}
		println("vtables: " + n);
	}

	private void exportFunctions() throws Exception {
		FunctionManager fm = currentProgram.getFunctionManager();
		int n = 0;
		try (BufferedWriter w = open("functions.jsonl")) {
			for (Function f : fm.getFunctions(true)) {
				if (monitor.isCancelled()) break;
				Set<String> callers = new TreeSet<>();
				for (Function c : f.getCallingFunctions(monitor)) callers.add(c.getEntryPoint().toString());
				Set<String> callees = new TreeSet<>();
				for (Function c : f.getCalledFunctions(monitor)) callees.add(c.getEntryPoint().toString());
				List<String> strs = funcStrings.getOrDefault(f.getEntryPoint(), Collections.emptyList());
				w.write("{\"addr\":" + esc(f.getEntryPoint().toString()) +
					",\"name\":" + esc(f.getName()) +
					",\"ns\":" + esc(f.getParentNamespace().getName(true)) +
					",\"full\":" + esc(f.getSymbol().getName(true)) +
					",\"sig\":" + esc(f.getPrototypeString(true, false)) +
					",\"size\":" + f.getBody().getNumAddresses() +
					",\"thunk\":" + f.isThunk() +
					",\"external\":" + f.isExternal() +
					",\"callers\":" + arr(callers) +
					",\"callees\":" + arr(callees) +
					",\"strings\":" + arr(strs) + "}\n");
				n++;
			}
		}
		println("functions: " + n);
	}

	private void exportDecompiled() throws Exception {
		FunctionManager fm = currentProgram.getFunctionManager();
		List<Function> funcs = new ArrayList<>();
		for (Function f : fm.getFunctions(true)) {
			if (!f.isExternal() && !f.isThunk()) funcs.add(f);
		}
		DecompileConfigurer configurer = ifc -> {
			DecompileOptions o = new DecompileOptions();
			o.grabFromProgram(currentProgram);
			ifc.setOptions(o);
			ifc.toggleCCode(true);
			ifc.toggleSyntaxTree(false);
			ifc.setSimplificationStyle("decompile");
		};
		DecompilerCallback<String[]> cb = new DecompilerCallback<>(currentProgram, configurer) {
			@Override
			public String[] process(DecompileResults r, TaskMonitor m) throws Exception {
				Function f = r.getFunction();
				String c;
				if (r.decompileCompleted() && r.getDecompiledFunction() != null) c = r.getDecompiledFunction().getC();
				else c = "/* decompile failed: " + r.getErrorMessage() + " */\n";
				return new String[] { f.getEntryPoint().toString(), f.getParentNamespace().getName(true), f.getName(), c };
			}
		};
		cb.setTimeout(120);
		List<String[]> results = ParallelDecompiler.decompileFunctions(cb, funcs, monitor);
		cb.dispose();
		results.sort(Comparator.comparing(x -> x[0]));
		try (BufferedWriter all = open("all.c")) {
			for (String[] r : results) {
				String ns = r[1].equals("Global") ? "_global" : safe(r[1].replace("::", "__"));
				Path dir = outDir.resolve("decomp").resolve(ns);
				Files.createDirectories(dir);
				String header = "// " + r[0] + "  " + r[1] + "::" + r[2] + "\n";
				Files.writeString(dir.resolve(r[0] + "_" + safe(r[2]) + ".c"), header + r[3], StandardCharsets.UTF_8);
				all.write(header);
				all.write(r[3]);
				all.write("\n");
			}
		}
		println("decompiled: " + results.size());
	}
}
