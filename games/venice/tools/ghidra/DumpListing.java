// Writes what Ghidra has at each address of a range, instruction or data or undefined, for finding code that
// auto-analysis left out of its functions.
//   args: <out file> <lo hex> <hi hex>
//@category Venice
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import java.io.*;

public class DumpListing extends GhidraScript {
	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		long lo = Long.parseLong(args[1], 16);
		long hi = Long.parseLong(args[2], 16);
		AddressSpace space = currentProgram.getAddressFactory().getDefaultAddressSpace();
		Listing listing = currentProgram.getListing();
		try (PrintWriter out = new PrintWriter(new BufferedWriter(new FileWriter(args[0])))) {
			Address address = space.getAddress(lo);
			Address end = space.getAddress(hi);
			while (address.compareTo(end) < 0) {
				CodeUnit unit = listing.getCodeUnitContaining(address);
				if (unit == null) {
					out.println(address + "  (nothing)");
					address = address.add(1);
					continue;
				}
				Function function = listing.getFunctionContaining(unit.getMinAddress());
				out.println(unit.getMinAddress() + "  " + unit + (function == null ? "" : "   ; in " + function.getName()));
				address = unit.getMaxAddress().add(1);
			}
		}
	}
}
