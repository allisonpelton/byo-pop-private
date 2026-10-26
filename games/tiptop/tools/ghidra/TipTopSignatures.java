//Gives TipTop.exe's functions the port's names and prototypes (games/tiptop/tools/ghsigs.py writes the list), so
//that the decompilation of the game code passes every argument at its calls.
//Lines: address|name|return|parameter kinds (ptr, byte, short, int, long, double; ... for varargs)
//@category TipTop
import java.io.BufferedReader;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.List;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.ByteDataType;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DoubleDataType;
import ghidra.program.model.data.IntegerDataType;
import ghidra.program.model.data.LongLongDataType;
import ghidra.program.model.data.PointerDataType;
import ghidra.program.model.data.ShortDataType;
import ghidra.program.model.data.VoidDataType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Function.FunctionUpdateType;
import ghidra.program.model.listing.ParameterImpl;
import ghidra.program.model.listing.ReturnParameterImpl;
import ghidra.program.model.listing.Variable;
import ghidra.program.model.symbol.SourceType;

public class TipTopSignatures extends GhidraScript {
	private DataType kind(String k) {
		switch (k) {
		case "ptr": return new PointerDataType();
		case "byte": return ByteDataType.dataType;
		case "short": return ShortDataType.dataType;
		case "long": return LongLongDataType.dataType;
		case "double": return DoubleDataType.dataType;
		case "void": return VoidDataType.dataType;
		default: return IntegerDataType.dataType;
		}
	}

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		int done = 0;
		try (BufferedReader r = new BufferedReader(new FileReader(args[0]))) {
			String line;
			while ((line = r.readLine()) != null) {
				String[] f = line.trim().split("\\|", -1);
				if (f.length < 4) {
					continue;
				}
				Function fn = getFunctionAt(toAddr(Long.parseLong(f[0], 16)));
				if (fn == null) {
					println("TipTopSignatures: no function at " + f[0]);
					continue;
				}
				fn.setName(f[1], SourceType.USER_DEFINED);
				fn.setCallingConvention("__cdecl");
				List<Variable> params = new ArrayList<>();
				boolean varargs = false;
				int i = 0;
				for (String p : f[3].isEmpty() ? new String[0] : f[3].split(",")) {
					if (p.equals("...")) {
						varargs = true;
						continue;
					}
					params.add(new ParameterImpl("p" + (++i), kind(p), currentProgram));
				}
				fn.updateFunction("__cdecl", new ReturnParameterImpl(kind(f[2]), currentProgram), params,
					FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true, SourceType.USER_DEFINED);
				fn.setVarArgs(varargs);
				done++;
			}
		}
		println("TipTopSignatures: " + done + " prototypes");
	}
}
