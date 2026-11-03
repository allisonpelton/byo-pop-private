// Disassembles and creates a function at each address of a list (one hex address a line): the virtual functions
// of MahjongEscapeAC.exe that are only called through their vtables, which Ghidra's analysis never reached
// (games/mahjongchina/tools/vtables.py writes the list to re/mahjongchina/vfuncs.txt).
// Usage (headless): -postScript MjCreateFunctions.java <list file>
//@category Mahjongchina
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.nio.file.*;
import java.util.*;

public class MjCreateFunctions extends GhidraScript {
	@Override
	protected void run() throws Exception {
		String[] args = getScriptArgs();
		int made = 0, failed = 0;
		for (String line : Files.readAllLines(Paths.get(args[0]))) {
			line = line.trim();
			if (line.isEmpty()) continue;
			Address a = toAddr(Long.parseLong(line, 16));
			if (getFunctionAt(a) != null) continue;
			if (getInstructionAt(a) == null) disassemble(a);
			Function f = createFunction(a, null);
			if (f == null) { printerr("MjCreateFunctions: no function at " + a); failed++; }
			else made++;
		}
		println("MjCreateFunctions: created " + made + ", failed " + failed);
	}
}
