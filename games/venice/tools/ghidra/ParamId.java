// Runs Ghidra's "Decompiler Parameter ID" over the game's code so the decompilation shows the `this` pointer (ecx) and the
// right number of arguments of the engine's helper functions; without it a call such as FindChildByType(type, after)
// loses the object it is called on.
//   analyzeHeadless <projects> venice_game -process VeniceGame.exe -noanalysis -postScript ParamId.java
//@category Venice
import ghidra.app.cmd.function.DecompilerParameterIdCmd;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.symbol.SourceType;

public class ParamId extends GhidraScript {
	@Override
	public void run() throws Exception {
		AddressSet code = new AddressSet();
		code.add(toAddr(0x401000), toAddr(0x43a900));
		code.add(toAddr(0x43fa60), toAddr(0x442300));
		DecompilerParameterIdCmd command =
			new DecompilerParameterIdCmd("Decompiler Parameter ID", code, SourceType.ANALYSIS, true, true, 60);
		boolean done = command.applyTo(currentProgram, monitor);
		println("ParamId done: " + done);
	}
}
