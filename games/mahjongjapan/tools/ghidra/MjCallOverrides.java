// Gives every indirect call of the game and framework code (0x401000-0x447000) a call-site
// signature with as many arguments as were pushed for it, so the decompilation shows the arguments of the virtual
// calls. Without a prototype Ghidra drops them: MSVC's __thiscall callee pops its own arguments, so nothing after
// the call says how many there were. The count walks back from the call over the pushes (and "sub esp, n") to the
// previous call, a return or the function's start; a call through [reg+offset] loaded from ECX's object is
// __thiscall, any other indirect call __stdcall.
// Usage (headless): -postScript MjCallOverrides.java
//@category Mahjongjapan
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.pcode.HighFunctionDBUtil;
import ghidra.program.model.scalar.Scalar;

public class MjCallOverrides extends GhidraScript {
	private static final long[][] RANGES = {{0x401000L, 0x447000L}};

	@Override
	protected void run() throws Exception {
		int done = 0, failed = 0;
		Listing listing = currentProgram.getListing();
		for (long[] r : RANGES) {
			AddressSet set = new AddressSet(toAddr(r[0]), toAddr(r[1] - 1));
			InstructionIterator it = listing.getInstructions(set, true);
			while (it.hasNext()) {
				Instruction ins = it.next();
				if (!ins.getMnemonicString().equals("CALL")) continue;
				Object[] ops = ins.getOpObjects(0);
				boolean indirect = !(ops.length == 1 && ops[0] instanceof Address);
				if (!indirect) continue;
				// a call through a register alone (call eax) or memory ([eax+8], [ecx]); not through an import's
				// slot ([0x51cxxx], no register), whose prototype Ghidra knows
				boolean hasReg = false;
				for (Object o : ops) if (o instanceof ghidra.program.model.lang.Register) hasReg = true;
				if (!hasReg) continue;
				Function f = getFunctionContaining(ins.getAddress());
				if (f == null) continue;
				int bytes = 0;
				boolean thiscall = ops.length >= 1 && ops[0] instanceof ghidra.program.model.lang.Register
					? false : true;
				Address prologueEnd = prologueEnd(f);
				Instruction p = ins.getPrevious();
				int steps = 0;
				while (p != null && steps++ < 64 && f.getBody().contains(p.getAddress())
					&& p.getAddress().compareTo(prologueEnd) >= 0) {
					String m = p.getMnemonicString();
					if (m.equals("CALL") || m.equals("RET")) break;
					if (m.equals("PUSH") && isSave(f, p)) break;
					if (m.equals("PUSH")) bytes += 4;
					else if (m.equals("POP")) bytes -= 4;
					else if (m.equals("SUB") && p.getDefaultOperandRepresentation(0).equals("ESP")
						&& p.getScalar(1) != null) bytes += (int) p.getScalar(1).getValue();
					else if (m.equals("ADD") && p.getDefaultOperandRepresentation(0).equals("ESP")
						&& p.getScalar(1) != null) bytes -= (int) p.getScalar(1).getValue();
					p = p.getPrevious();
				}
				if (bytes < 0 || bytes > 64) continue;
				// [reg+off] on an object: thiscall when ECX holds it (MSVC loads ECX with the object for a virtual call)
				if (ins.getDefaultOperandRepresentation(0).startsWith("dword ptr [")) thiscall = true;
				// the caller pops the arguments: __cdecl, no this
				Instruction n = ins.getNext();
				boolean cdecl = n != null && n.getMnemonicString().equals("ADD")
					&& n.getDefaultOperandRepresentation(0).equals("ESP") && n.getScalar(1) != null
					&& n.getScalar(1).getValue() == bytes;
				if (cdecl) thiscall = false;
				FunctionDefinitionDataType sig = new FunctionDefinitionDataType("icall_" + ins.getAddress());
				ParameterDefinition[] params = new ParameterDefinition[bytes / 4 + (thiscall ? 1 : 0)];
				int k = 0;
				if (thiscall) params[k++] = new ParameterDefinitionImpl("this", new PointerDataType(), null);
				for (int i = 0; i < bytes / 4; i++)
					params[k++] = new ParameterDefinitionImpl("a" + (i + 1), new Undefined4DataType(), null);
				sig.setArguments(params);
				sig.setReturnType(new Undefined4DataType());
				try {
					sig.setCallingConvention(cdecl ? "__cdecl" : thiscall ? "__thiscall" : "__stdcall");
					HighFunctionDBUtil.writeOverride(f, ins.getAddress(), sig);
					done++;
				}
				catch (Exception e) {
					failed++;
				}
			}
		}
		println("MjCallOverrides: " + done + " call sites, " + failed + " failed");
	}

	// A push that saves a callee-saved register: EBX, ESI, EDI or EBP, not written by the function before it (MSVC
	// may save them after an early test, not only in the prologue)
	private boolean isSave(Function f, Instruction push) {
		String r = push.getDefaultOperandRepresentation(0);
		if (!(r.equals("EBX") || r.equals("ESI") || r.equals("EDI") || r.equals("EBP"))) return false;
		Instruction p = getInstructionAt(f.getEntryPoint());
		while (p != null && p.getAddress().compareTo(push.getAddress()) < 0) {
			for (Object o : p.getResultObjects())
				if (o instanceof ghidra.program.model.lang.Register
					&& ((ghidra.program.model.lang.Register) o).getBaseRegister().getName().equals(r)) return false;
			p = p.getNext();
		}
		return true;
	}

	// The first instruction after the prologue: the saves of registers (and "push ecx" for a local), the frame
	// ("mov ebp, esp", "sub esp, n") and MSVC's SEH frame (push -1; push handler; mov eax, fs:[0]; push eax;
	// mov fs:[0], esp), in any order
	private Address prologueEnd(Function f) {
		Instruction p = getInstructionAt(f.getEntryPoint());
		int k = 0;
		while (p != null) {
			String m = p.getMnemonicString();
			String op0 = p.getNumOperands() > 0 ? p.getDefaultOperandRepresentation(0) : "";
			String op1 = p.getNumOperands() > 1 ? p.getDefaultOperandRepresentation(1) : "";
			boolean save = m.equals("PUSH") && (op0.equals("EBP") || op0.equals("EBX") || op0.equals("ESI")
				|| op0.equals("EDI") || op0.equals("ECX"));
			boolean seh = (m.equals("PUSH") && k < 2 && p.getScalar(0) != null)
				|| (m.equals("MOV") && (op0.contains("FS:") || op1.contains("FS:")))
				|| (m.equals("PUSH") && op0.equals("EAX") && p.getPrevious() != null
					&& p.getPrevious().toString().contains("FS:"));
			boolean frame = (m.equals("MOV") && op0.equals("EBP") && op1.equals("ESP"))
				|| (m.equals("SUB") && op0.equals("ESP"));
			if (!save && !seh && !frame) return p.getAddress();
			p = p.getNext();
			k++;
		}
		return f.getEntryPoint();
	}
}
