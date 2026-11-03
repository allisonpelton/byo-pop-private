// Gives the C runtime's _ftol2 (0x77dfe0 in BejeweledTwist.exe) its real signature: it takes its float on the x87
// stack (ST0) and returns an int in EAX. Without it the decompiler drops every value passed to it, and most of the
// game's coordinates with them. Usage (headless): -postScript FixFtol.java [hex address]
//@category Analysis
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.Float10DataType;
import ghidra.program.model.data.IntegerDataType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.ParameterImpl;
import ghidra.program.model.listing.ReturnParameterImpl;
import ghidra.program.model.listing.VariableStorage;
import ghidra.program.model.listing.Function.FunctionUpdateType;
import ghidra.program.model.symbol.SourceType;

public class FixFtol extends GhidraScript {
	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		long addr = args.length > 0 ? Long.parseLong(args[0], 16) : 0x77dfe0L;
		Function f = getFunctionAt(toAddr(addr));
		if (f == null) {
			println("FixFtol: no function at " + Long.toHexString(addr));
			return;
		}
		VariableStorage st0 = new VariableStorage(currentProgram, currentProgram.getRegister("ST0"));
		VariableStorage eax = new VariableStorage(currentProgram, currentProgram.getRegister("EAX"));
		ParameterImpl p = new ParameterImpl("x", Float10DataType.dataType, st0, currentProgram);
		ReturnParameterImpl r = new ReturnParameterImpl(IntegerDataType.dataType, eax, currentProgram);
		f.updateFunction("__cdecl", r, FunctionUpdateType.CUSTOM_STORAGE, true, SourceType.USER_DEFINED, p);
		f.setName("_ftol2", SourceType.USER_DEFINED);
		println("FixFtol: " + f.getSignature().getPrototypeString());
	}
}
