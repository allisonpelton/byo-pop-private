// Gives the compiler's floating point helpers and a few small game functions the signatures the decompiler needs to
// show their arguments and results (a value in ST0 is invisible otherwise: "__ftol()" shows no argument).
//   run once on the project, before ExportRange.java; the changes stay in the project
//@category Bejeweled2
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.*;
import ghidra.program.model.listing.Function.FunctionUpdateType;
import ghidra.program.model.symbol.SourceType;
import java.util.*;

public class FixSigs extends GhidraScript {
	private Function fn(String addr) {
		return getFunctionAt(toAddr(addr));
	}

	// a helper that takes numArgs doubles in ST0, ST1 and returns in ST0 (retType) or EAX (int)
	private void fpu(String addr, String name, int numArgs, boolean returnsInt) throws Exception {
		Function f = fn(addr);
		if (f == null) {
			println("no function at " + addr);
			return;
		}
		f.setName(name, SourceType.USER_DEFINED);
		List<Variable> params = new ArrayList<>();
		for (int i = 0; i < numArgs; i++)
			params.add(new ParameterImpl("x" + i, DoubleDataType.dataType, currentProgram.getRegister("ST" + i), currentProgram));
		Variable ret = returnsInt
			? new ReturnParameterImpl(IntegerDataType.dataType, currentProgram.getRegister("EAX"), currentProgram)
			: new ReturnParameterImpl(DoubleDataType.dataType, currentProgram.getRegister("ST0"), currentProgram);
		f.updateFunction("__cdecl", ret, params, FunctionUpdateType.CUSTOM_STORAGE, true, SourceType.USER_DEFINED);
	}

	private void retType(String addr, DataType t) throws Exception {
		Function f = fn(addr);
		if (f != null)
			f.setReturnType(t, SourceType.USER_DEFINED);
	}

	private void rename(String addr, String name) throws Exception {
		Function f = fn(addr);
		if (f != null)
			f.setName(name, SourceType.USER_DEFINED);
	}

	@Override
	public void run() throws Exception {
		fpu("005e8080", "ftol", 1, true);       // __ftol2_sse: ST0 -> EAX
		fpu("005e8390", "CIcos", 1, false);
		fpu("005e84c0", "CIsin", 1, false);
		fpu("005e8604", "CIsqrt", 1, false);
		retType("005aefa9", DoubleDataType.dataType);   // CurvedVal::GetOutVal
		retType("005af12c", DoubleDataType.dataType);   // CurvedVal::GetInVal
		retType("005946f6", FloatDataType.dataType);    // GetRandFloat
		rename("00447cb0", "Color_rgb");
		rename("00447cd0", "Color_rgba");
		rename("0048ef90", "Rect4");
		println("FixSigs done");
	}
}
