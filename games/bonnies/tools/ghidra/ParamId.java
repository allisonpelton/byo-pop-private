// Run Ghidra's Decompiler Parameter ID analysis over the program once. Bonnie's Bookstore's renderer takes float
// coordinates on the stack, which the default analysis leaves as unnamed stack reads (games/bonnies/docs/README.md).
//@category byo-pop

import ghidra.app.script.GhidraScript;

public class ParamId extends GhidraScript {
	@Override
	public void run() throws Exception {
		setAnalysisOption(currentProgram, "Decompiler Parameter ID", "true");
		analyzeAll(currentProgram);
		println("ParamId: done");
	}
}
