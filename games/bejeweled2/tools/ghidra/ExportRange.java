// Re-decompiles the functions in an address range after giving every member function the __thiscall convention
// (and the exe's _EH_prolog helper a thiscall-like one, so that ECX survives its call), and writes them like
// tools/ghidra_scripts/ExportAll.java does, into <outDir>/decomp/<ns>/<addr>_<name>.c.
//   analyzeHeadless <projects> <project> -process <program> -noanalysis -scriptPath <this folder>
//       -postScript ExportRange.java <outDir> <lo hex> <hi hex> [<eh prolog address hex>]
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.app.decompiler.parallel.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.util.task.TaskMonitor;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class ExportRange extends GhidraScript {
	static String safe(String s) {
		String r = s.replaceAll("[^A-Za-z0-9_.-]", "_");
		return r.length() > 80 ? r.substring(0, 80) : r;
	}

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		Path outDir = Paths.get(args[0]);
		long lo = Long.parseLong(args[1], 16), hi = Long.parseLong(args[2], 16);
		FunctionManager fm = currentProgram.getFunctionManager();
		int cc = 0;
		for (Function f : fm.getFunctions(true)) {
			if (f.isExternal() || f.isThunk()) continue;
			if (f.getParentNamespace() instanceof ghidra.program.model.listing.GhidraClass) {
				String cur = f.getCallingConventionName();
				if (cur == null || !cur.equals("__thiscall")) {
					try { f.setCallingConvention("__thiscall"); cc++; } catch (Exception e) { printerr("cc " + f.getEntryPoint() + ": " + e.getMessage()); }
				}
			}
		}
		if (args.length > 3) {
			Function eh = fm.getFunctionAt(toAddr(Long.parseLong(args[3], 16)));
			if (eh != null) {
				try { eh.setCallingConvention("__thiscall"); eh.setName("EH_prolog", SourceType.USER_DEFINED); } catch (Exception e) { printerr("eh: " + e.getMessage()); }
			}
		}
		println("thiscall set on " + cc + " functions");
		List<Function> funcs = new ArrayList<>();
		for (Function f : fm.getFunctions(true)) {
			long a = f.getEntryPoint().getOffset();
			if (!f.isExternal() && !f.isThunk() && a >= lo && a < hi) funcs.add(f);
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
		int n = 0;
		for (String[] r : results) {
			String ns = r[1].equals("Global") ? "_global" : r[1].replace("::", "__");
			Path dir = outDir.resolve("decomp").resolve(safe(ns));
			Files.createDirectories(dir);
			Files.write(dir.resolve(r[0] + "_" + safe(r[2]) + ".c"),
				("// " + r[0] + "  " + r[1] + "::" + r[2] + "\n\n" + r[3]).getBytes(StandardCharsets.UTF_8));
			n++;
		}
		println("ExportRange: " + n + " functions -> " + outDir.toAbsolutePath());
	}
}
