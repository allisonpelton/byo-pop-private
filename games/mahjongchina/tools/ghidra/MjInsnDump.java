// Writes every instruction of the functions in the code range (0x401000-0x447000) to <out>, one line each:
//   <function address> <instruction address> <mnemonic> <operands>
// with each operand written as its kind: a register by name, an immediate as #<hex>, a memory operand as
// [<registers>+m] (its displacement hidden, so a field that moved still compares equal), and a branch or call
// target as @<hex>. games/mahjongchina/tools/compare_exes.py compares the two games' dumps.
// Usage (headless, read-only): -postScript MjInsnDump.java <out>
//@category Mahjongchina
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.lang.OperandType;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import java.io.*;

public class MjInsnDump extends GhidraScript {
	@Override
	protected void run() throws Exception {
		String out = getScriptArgs()[0];
		Listing listing = currentProgram.getListing();
		AddressSet range = new AddressSet(toAddr(0x401000L), toAddr(0x446fffL));
		int n = 0;
		try (PrintWriter w = new PrintWriter(new BufferedWriter(new FileWriter(out)))) {
			FunctionIterator fit = listing.getFunctions(range, true);
			while (fit.hasNext()) {
				Function f = fit.next();
				String fa = Long.toHexString(f.getEntryPoint().getOffset());
				InstructionIterator it = listing.getInstructions(f.getBody(), true);
				while (it.hasNext()) {
					Instruction ins = it.next();
					StringBuilder sb = new StringBuilder();
					sb.append(fa).append(' ').append(Long.toHexString(ins.getAddress().getOffset())).append(' ');
					sb.append(ins.getMnemonicString());
					for (int i = 0; i < ins.getNumOperands(); i++) {
						int type = ins.getOperandType(i);
						Object[] objs = ins.getOpObjects(i);
						sb.append(i == 0 ? " " : ",");
						if (OperandType.isDynamic(type)) {
							sb.append('[');
							boolean any = false;
							for (Object o : objs) {
								if (o instanceof Register) {
									if (any) sb.append('+');
									sb.append(((Register) o).getName());
									any = true;
								} else if (o instanceof Address) {
									sb.append(any ? "+" : "").append('@').append(Long.toHexString(((Address) o).getOffset()));
									any = true;
								} else if (o instanceof Scalar) {
									long v = ((Scalar) o).getUnsignedValue();
									if (v >= 0x400000L && v < 0x700000L)
										sb.append(any ? "+" : "").append('@').append(Long.toHexString(v));
									else
										sb.append(any ? "+" : "").append('m');
									any = true;
								}
							}
							sb.append(']');
						} else {
							for (Object o : objs) {
								if (o instanceof Register) {
									sb.append(((Register) o).getName());
								} else if (o instanceof Address) {
									sb.append('@').append(Long.toHexString(((Address) o).getOffset()));
								} else if (o instanceof Scalar) {
									long v = ((Scalar) o).getUnsignedValue();
									if (v >= 0x400000L && v < 0x700000L)
										sb.append('@').append(Long.toHexString(v));
									else
										sb.append('#').append(Long.toHexString(v));
								}
							}
						}
					}
					w.println(sb);
					n++;
				}
			}
		}
		println("MjInsnDump: " + n + " instructions -> " + out);
	}
}
