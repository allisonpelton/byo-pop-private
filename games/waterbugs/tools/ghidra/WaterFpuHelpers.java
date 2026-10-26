// Gives the CRT's x87 helpers their FPU-stack arguments, so that the decompiler shows what the game converts and
// computes: __ftol2_sse (0x443e40) reads ST0 and returns the truncated int in EAX; _CIcos (0x444270), _CIsin
// (0x4443a0) and _CIsqrt (0x444490) read ST0 and leave the result in ST0; _CIatan2 (0x44421a) reads ST1 = y and
// ST0 = x. Without this every "(int)x" in the game reads as FUN_00443a00() with no argument.
//@category byo-pop
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.Float10DataType;
import ghidra.program.model.data.IntegerDataType;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Parameter;
import ghidra.program.model.listing.ParameterImpl;
import ghidra.program.model.listing.ReturnParameterImpl;
import ghidra.program.model.listing.VariableStorage;
import ghidra.program.model.symbol.SourceType;

public class WaterFpuHelpers extends GhidraScript {
	private VariableStorage reg(String name) throws Exception {
		Register r = currentProgram.getRegister(name);
		return new VariableStorage(currentProgram, r);
	}

	private void set(long addr, String name, DataType ret, String retReg, String... params) throws Exception {
		Function f = getFunctionAt(toAddr(addr));
		if (f == null) f = createFunction(toAddr(addr), name);
		Parameter[] ps = new Parameter[params.length / 2];
		for (int i = 0; i < ps.length; i++)
			ps[i] = new ParameterImpl(params[2 * i], Float10DataType.dataType, reg(params[2 * i + 1]), currentProgram);
		ReturnParameterImpl r = new ReturnParameterImpl(ret, reg(retReg), currentProgram);
		f.updateFunction("__cdecl", r, Function.FunctionUpdateType.CUSTOM_STORAGE, true, SourceType.USER_DEFINED, ps);
		f.setName(name, SourceType.USER_DEFINED);
		println("WaterFpuHelpers: " + name + " at " + f.getEntryPoint());
	}

	@Override
	public void run() throws Exception {
		set(0x443e40, "__ftol2_sse", IntegerDataType.dataType, "EAX", "x", "ST0");
		set(0x444270, "_CIcos", Float10DataType.dataType, "ST0", "x", "ST0");
		set(0x4443a0, "_CIsin", Float10DataType.dataType, "ST0", "x", "ST0");
		set(0x444490, "_CIsqrt", Float10DataType.dataType, "ST0", "x", "ST0");
		set(0x44421a, "_CIatan2", Float10DataType.dataType, "ST0", "y", "ST1", "x", "ST0");
	}
}
