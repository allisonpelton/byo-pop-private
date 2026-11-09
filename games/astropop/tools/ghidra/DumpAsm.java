// Writes the disassembly of every function in an address range, one file per function: <outDir>/<addr>.asm.
// Each line is "<addr> <instruction>", with the name of a call target or of the data an operand refers to after a ';'.
// Usage (headless, -noanalysis): -postScript DumpAsm.java <outDir> <loHex> <hiHex> [addr,addr,...]
//@category Export
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public class DumpAsm extends GhidraScript {
	@Override
	public void run() throws Exception {
		String[] a = getScriptArgs();
		Path out = Paths.get(a[0]);
		Files.createDirectories(out);
		long lo = Long.parseLong(a[1], 16), hi = Long.parseLong(a[2], 16);
		// optional 4th argument: comma-separated addresses (hex) where code is not yet a function: made one first
		if (a.length > 3) {
			for (String x : a[3].split(",")) {
				Address ad = toAddr(Long.parseLong(x.trim(), 16));
				if (currentProgram.getFunctionManager().getFunctionAt(ad) == null) {
					disassemble(ad);
					createFunction(ad, null);
				}
			}
		}
		Listing listing = currentProgram.getListing();
		int n = 0;
		for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
			long ea = f.getEntryPoint().getOffset();
			if (ea < lo || ea >= hi) continue;
			StringBuilder sb = new StringBuilder();
			InstructionIterator it = listing.getInstructions(f.getBody(), true);
			while (it.hasNext()) {
				Instruction ins = it.next();
				sb.append(String.format("%08x %s", ins.getAddress().getOffset(), ins.toString()));
				StringBuilder notes = new StringBuilder();
				for (Reference r : ins.getReferencesFrom()) {
					Address to = r.getToAddress();
					if (to == null || !to.isMemoryAddress()) continue;
					Function g = currentProgram.getFunctionManager().getFunctionAt(to);
					if (g != null) {
						notes.append(' ').append(g.getName(true));
						continue;
					}
					Data d = listing.getDataAt(to);
					if (d != null && d.hasStringValue()) {
						notes.append(" \"").append(String.valueOf(d.getValue()).replace("\n", "\\n")).append('"');
						continue;
					}
					Symbol s = currentProgram.getSymbolTable().getPrimarySymbol(to);
					if (s != null && s.getSource() != SourceType.DEFAULT) notes.append(' ').append(s.getName());
					else if (r.getReferenceType().isData()) notes.append(String.format(" @%08x", to.getOffset()));
				}
				if (notes.length() > 0) sb.append("  ;").append(notes);
				sb.append('\n');
			}
			Files.write(out.resolve(String.format("%08x.asm", ea)), sb.toString().getBytes(StandardCharsets.UTF_8));
			n++;
		}
		println("DumpAsm: " + n + " functions");
	}
}
