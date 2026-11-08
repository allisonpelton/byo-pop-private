// Writes the disassembly of every function in an address range, one instruction a line, for reading what the
// decompiler left out (the arguments of calls it could not type, the register an ECX comes from).
//   args: <out file> <lo hex> <hi hex>
// Output: "== <addr> <function name> (<size>)" and then "<addr>  <instruction>" lines; a call to a function shows
// the callee's name after a semicolon.
//@category Bejeweled2
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;

public class DumpAsm extends GhidraScript {
	@Override
	public void run() throws Exception {
		String[] a = getScriptArgs();
		String outFile = a[0];
		long lo = Long.parseLong(a[1], 16);
		long hi = Long.parseLong(a[2], 16);
		AddressSpace sp = currentProgram.getAddressFactory().getDefaultAddressSpace();
		Listing listing = currentProgram.getListing();
		int count = 0;
		try (PrintWriter w = new PrintWriter(new BufferedWriter(new FileWriter(outFile)))) {
			FunctionIterator it = currentProgram.getFunctionManager().getFunctions(sp.getAddress(lo), true);
			while (it.hasNext()) {
				Function f = it.next();
				if (f.getEntryPoint().getOffset() >= hi)
					break;
				w.println("== " + f.getEntryPoint() + " " + f.getName(true) + " (" + f.getBody().getNumAddresses() + ")");
				InstructionIterator ii = listing.getInstructions(f.getBody(), true);
				while (ii.hasNext()) {
					Instruction in = ii.next();
					String extra = "";
					if (in.getFlowType().isCall()) {
						Reference[] refs = in.getReferencesFrom();
						for (Reference r : refs) {
							if (r.getReferenceType().isCall()) {
								Function c = currentProgram.getFunctionManager().getFunctionAt(r.getToAddress());
								if (c != null)
									extra = " ; " + c.getName(true);
							}
						}
					} else {
						for (Reference r : in.getReferencesFrom()) {
							if (r.getReferenceType().isData()) {
								Symbol s = currentProgram.getSymbolTable().getPrimarySymbol(r.getToAddress());
								if (s != null && !s.getName().startsWith("DAT_"))
									extra = " ; " + s.getName(true);
							}
						}
					}
					w.println(in.getAddress() + "  " + in.toString() + extra);
				}
				count++;
			}
		}
		println("DumpAsm: " + count + " functions -> " + outFile);
	}
}
