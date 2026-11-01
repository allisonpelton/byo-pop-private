// Commits the decompiler's parameters (and return) of every function of the engine and the game (0x401000-0x447000)
// to the database, so that the callers' decompilation passes them all: Ghidra's analysis leaves each function as
// "undefined FUN_x()", and a caller then shows only the stack arguments it can see pushed, without the ECX of a
// __thiscall or the ECX/EDX of a __fastcall. Two rounds, since a callee's committed parameters change what its caller
// is inferred to take. Functions named by the port's symbols keep what ApplySymbols gave them.
// Usage (headless): -postScript MjCommitParams.java
//@category Mahjongchina
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.pcode.HighFunction;
import ghidra.program.model.pcode.HighFunctionDBUtil;
import ghidra.program.model.symbol.SourceType;

public class MjCommitParams extends GhidraScript {
	@Override
	protected void run() throws Exception {
		DecompInterface di = new DecompInterface();
		di.toggleCCode(false);
		di.openProgram(currentProgram);
		AddressSet set = new AddressSet(toAddr(0x401000L), toAddr(0x446fffL));
		for (int round = 0; round < 2; round++) {
			int done = 0, failed = 0;
			FunctionIterator it = currentProgram.getFunctionManager().getFunctions(set, true);
			while (it.hasNext()) {
				Function f = it.next();
				if (f.isThunk() || f.getSignatureSource() == SourceType.USER_DEFINED || f.getSignatureSource() == SourceType.IMPORTED) continue;
				DecompileResults r = di.decompileFunction(f, 60, monitor);
				HighFunction hf = r.getHighFunction();
				if (hf == null) {
					if (failed < 4 && round == 0) println("MjCommitParams: " + f.getEntryPoint() + ": " + r.getErrorMessage());
					failed++;
					continue;
				}
				// undefined types first; a char argument (one byte in a four-byte slot) needs the decompiler's types
				try {
					try {
						HighFunctionDBUtil.commitParamsToDatabase(hf, false, HighFunctionDBUtil.ReturnCommitOption.COMMIT, SourceType.ANALYSIS);
					} catch (Exception e1) {
						HighFunctionDBUtil.commitParamsToDatabase(hf, true, HighFunctionDBUtil.ReturnCommitOption.COMMIT, SourceType.ANALYSIS);
					}
					done++;
				} catch (Exception e) {
					if (failed < 4 && round == 0) println("MjCommitParams: " + f.getEntryPoint() + ": " + e);
					failed++;
				}
			}
			println("MjCommitParams: round " + round + ": " + done + " committed, " + failed + " failed");
		}
		di.dispose();
	}
}
