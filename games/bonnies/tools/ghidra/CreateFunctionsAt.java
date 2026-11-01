// Disassemble and create a function at each address of a list (one hex address per line). Bonnie's Bookstore's C++
// static initializers and the destructors they register with atexit are reached only through the CRT's __initterm
// table and push immediates, so Ghidra's analysis does not find them (games/bonnies/docs/README.md).
//@category byo-pop

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

import java.nio.file.Files;
import java.nio.file.Paths;

public class CreateFunctionsAt extends GhidraScript {
	@Override
	public void run() throws Exception {
		int created = 0, failed = 0;
		for (String line : Files.readAllLines(Paths.get(getScriptArgs()[0]))) {
			line = line.trim();
			if (line.isEmpty()) continue;
			Address a = toAddr(Long.parseLong(line.replaceFirst("^0x", ""), 16));
			if (getFunctionAt(a) != null) continue;
			disassemble(a);
			Function f = createFunction(a, null);
			if (f == null) { printerr("no function at " + a); failed++; }
			else created++;
		}
		println("CreateFunctionsAt: created " + created + ", failed " + failed);
	}
}
