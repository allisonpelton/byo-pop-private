//Marks TipTop.exe's _EH_prolog (0x44d1b0) before analysis, so that Ghidra decompiles the functions that call it
//(MSVC 7.0 C++ functions with destructors to run) with their own frames and parameters instead of unaff_EBP.
//@category TipTop
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;

public class TipTopEhProlog extends GhidraScript {
	@Override
	public void run() throws Exception {
		Address a = toAddr(0x44d1b0);
		disassemble(a);
		Function f = getFunctionAt(a);
		if (f == null) {
			f = createFunction(a, "_EH_prolog");
		}
		else {
			f.setName("_EH_prolog", SourceType.USER_DEFINED);
		}
		f.setCallFixup("EH_prolog");
		println("TipTopEhProlog: EH_prolog fixup on " + a);
	}
}
