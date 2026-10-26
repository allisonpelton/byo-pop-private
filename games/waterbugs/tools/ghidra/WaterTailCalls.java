// The blit wrappers 0x429380..0x42a210 pick a blitter by the destination's bits per pixel and jump to it with the
// caller's own stack (a tail call), so the decompiler sees only the arguments they read themselves (the third, the
// format) and every caller loses the rest. This gives each wrapper the parameter count of the blitter it jumps to
// (run after WaterCommitParams, which commits the blitters' own parameters).
//@category byo-pop
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.Undefined4DataType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Parameter;
import ghidra.program.model.listing.ParameterImpl;
import ghidra.program.model.symbol.SourceType;

public class WaterTailCalls extends GhidraScript {
	@Override
	public void run() throws Exception {
		int done = 0;
		for (Function f : currentProgram.getFunctionManager().getFunctions(toAddr(0x4295f0), true)) {
			if (f.getEntryPoint().getOffset() >= 0x42a210)
				break;
			Function target = null;
			for (Instruction ins : currentProgram.getListing().getInstructions(f.getBody(), true)) {
				if (!ins.getFlowType().isJump() && !ins.getFlowType().isCall())
					continue;
				for (Address a : ins.getFlows()) {
					if (a.getOffset() >= 0x42fe50 && a.getOffset() < 0x441c50) {
						target = getFunctionAt(a);
						break;
					}
				}
				if (target != null)
					break;
			}
			if (target == null || target.getParameterCount() <= f.getParameterCount())
				continue;
			Parameter[] ps = new Parameter[target.getParameterCount()];
			for (int i = 0; i < ps.length; i++) {
				DataType t = i < f.getParameterCount() ? f.getParameter(i).getDataType() : target.getParameter(i).getDataType();
				if (t == null)
					t = Undefined4DataType.dataType;
				ps[i] = new ParameterImpl("param_" + (i + 1), t, currentProgram);
			}
			f.updateFunction("__cdecl", null, Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true,
				SourceType.USER_DEFINED, ps);
			done++;
		}
		println("WaterTailCalls: " + done + " wrappers");
	}
}
