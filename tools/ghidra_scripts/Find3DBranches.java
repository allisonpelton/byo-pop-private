// Lists the code that branches on the framework's 3D flag in an analysed PopCap program, so a reader can tell what a
// game does differently with 3D acceleration on, beyond drawing faster.
//
// How it finds the flag: SexyAppFramework reads the registry value "Is3D" into its display's 3D flag
// (mDDInterface->mIs3D), and SexyAppBase::Is3DAccelerated() is a small getter of that flag. The script finds every
// function that uses an "Is3D" string, takes the two-level byte stores right after that use ([[x+A]+B], the app's
// display pointer at A and the flag at B), and picks the small getters that read the same [[this+A]+B]. It then lists:
//   - every caller of each getter: the calling function, its class (from Ghidra's namespace, or from the RTTI of the
//     vtable that holds it, with the slot), its own callers when it is not virtual, the strings it uses, and the
//     decompiled lines around each call;
//   - every inline read or write of [[x+A]+B] outside the getters (link-time code generation inlines the getter);
//   - the other small accessors of the display pointer (Is3DAccelerationSupported/Recommended and the like), with
//     their callers;
//   - every two-level dword read through the display pointer, by field offset (the D3D interface pointer is one);
//   - the strings that name 3D or hardware acceleration and the functions that use them.
// Each function is tagged [fw] when its class is one of the framework's, else [game?]: the reader decides.
//
// Usage (headless): -postScript Find3DBranches.java <out.txt> [getter:0xADDR]... [flag:0xADDR]... [strings:<word+word>]
//   [context:<lines>] [maxinline:<n>]
//   getter:  use this function as the 3D getter (the automatic choice is still printed)
//   flag:    also list every instruction that reads or writes this global (an engine's own 3D flag)
//   show:    only print this function in full (several may be given; with getter:/flag:/strings: the scan runs too)
//   strings: also decompile every function that uses a string containing one of these words, joined by '+'
//            (any case). analyzeHeadless.bat splits arguments at spaces, '=', ',' and ';', so none of those.
//@category Analysis
import ghidra.app.decompiler.*;
import ghidra.app.decompiler.component.DecompilerUtils;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.regex.*;

public class Find3DBranches extends GhidraScript {

	static final Pattern MEM = Pattern.compile(
		"(byte|word|dword|qword) ptr (?:[A-Z]S:)?\\[([A-Z]{2,3})(?: \\+ 0x([0-9a-fA-F]+))?\\]");

	// SexyAppFramework's own classes (every generation we have seen); a function of one of these is framework code
	static final Set<String> FW = new HashSet<>(Arrays.asList(
		"SexyAppBase", "SexyApp", "DDInterface", "D3DInterface", "D3DTester", "DDImage", "MemoryImage", "Image",
		"Graphics", "GraphicsState", "WidgetManager", "Widget", "WidgetContainer", "Dialog", "DialogButton",
		"ButtonWidget", "Checkbox", "Slider", "ScrollbarWidget", "ScrollbuttonWidget", "EditWidget", "ListWidget",
		"HyperlinkWidget", "TextWidget", "ResourceManager", "SoundManager", "DSoundManager", "DSoundInstance",
		"BassSoundManager", "BassSoundInstance", "MusicInterface", "BassMusicInterface", "FModMusicInterface",
		"Font", "ImageFont", "SysFont", "NativeDisplay", "Buffer", "MTRand", "PakInterface", "XMLParser",
		"HTTPTransfer", "InternetManager", "SharedImage", "SharedImageRef", "Ratio", "PropertiesParser",
		"DescParser", "ResourceManagerException", "NativeDisplay", "D3DTextureData", "TextureData", "MsgBox",
		"Checkbox", "CursorWidget", "FlashWidget", "HTMLWidget", "DialogListener", "ButtonListener",
		"PopAnim", "PIEffect", "RenderDevice3D", "D3D8Interface", "D3D9Interface", "DXRenderDevice",
		"GLInterface", "GraphicsDriver", "DXDriver", "IGraphicsDriver", "RenderDevice", "Graphics3D",
		"DeviceImage", "PopLoc", "Mixer", "SoundInstance", "LuaApp", "PopLua", "XWidget", "XAnim"));

	PrintWriter out;
	DecompInterface decomp;
	Listing listing;
	ReferenceManager refs;
	FunctionManager fm;
	Memory mem;
	int context = 6;
	int maxInline = 40;

	static class Pair {
		final long a, b;
		Pair(long a, long b) { this.a = a; this.b = b; }
		@Override public boolean equals(Object o) { return o instanceof Pair && ((Pair) o).a == a && ((Pair) o).b == b; }
		@Override public int hashCode() { return Long.hashCode(a) * 31 + Long.hashCode(b); }
		@Override public String toString() { return String.format("[[x+0x%x]+0x%x]", a, b); }
	}

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		if (args.length < 1) throw new IllegalArgumentException("usage: Find3DBranches.java <out.txt> [getter:0x..] ...");
		List<Address> forcedGetters = new ArrayList<>();
		List<Address> flags = new ArrayList<>();
		String stringsRegex = null;
		List<Address> shows = new ArrayList<>();
		for (int i = 1; i < args.length; i++) {
			// key:value (analyzeHeadless.bat splits arguments at '=', ',' and ';', so those cannot be used)
			String[] kv = args[i].split("[:=]", 2);
			if (kv.length < 2) { println("ignored argument " + args[i]); continue; }
			String k = kv[0], v = kv[1];
			if (k.equals("getter")) forcedGetters.add(toAddr(Long.decode(v)));
			else if (k.equals("flag")) flags.add(toAddr(Long.decode(v)));
			else if (k.equals("strings")) stringsRegex = v;
			else if (k.equals("context")) context = Integer.parseInt(v);
			else if (k.equals("maxinline")) maxInline = Integer.parseInt(v);
			else if (k.equals("show")) shows.add(toAddr(Long.decode(v)));
			else println("ignored argument " + args[i]);
		}
		listing = currentProgram.getListing();
		refs = currentProgram.getReferenceManager();
		fm = currentProgram.getFunctionManager();
		mem = currentProgram.getMemory();
		decomp = new DecompInterface();
		decomp.setOptions(new DecompileOptions());
		decomp.toggleCCode(true);
		decomp.toggleSyntaxTree(true);
		decomp.setSimplificationStyle("decompile");
		decomp.openProgram(currentProgram);

		out = new PrintWriter(new OutputStreamWriter(new FileOutputStream(args[0]), StandardCharsets.UTF_8));
		try {
			out.println("# Find3DBranches: " + currentProgram.getName() + "  (image base " + currentProgram.getImageBase()
				+ ", " + fm.getFunctionCount() + " functions)");
			out.println();
			for (Address a : shows) {
				Function f = fm.getFunctionContaining(a);
				if (f == null) { out.println("## show " + a + ": no function"); continue; }
				out.println("## show " + describe(f));
				printCallerInfo(f);
				printWhole(f, 400);
				out.println();
			}
			if (shows.isEmpty() || !forcedGetters.isEmpty() || !flags.isEmpty() || stringsRegex != null)
				analyse(forcedGetters, flags, stringsRegex);
		} finally {
			out.close();
			decomp.dispose();
		}
		println("Find3DBranches done -> " + args[0]);
	}

	// ---------------------------------------------------------------------------------------------------------------

	private void analyse(List<Address> forcedGetters, List<Address> flags, String stringsRegex) throws Exception {
		// 1. the "Is3D" strings and the functions that use them
		Map<Address, String> regStrings = new LinkedHashMap<>();
		for (String s : new String[] { "Is3D", "Is3DSupported", "Is3DRecommended", "Is3DAccelerated" }) {
			for (Address a : findString(s, false)) regStrings.put(a, s);
			for (Address a : findString(s, true)) regStrings.put(a, s + " (UTF-16)");
		}
		Map<Address, Set<Address>> useSites = stringUses(regStrings.keySet());
		out.println("## 1. Registry strings and the functions that use them");
		Map<Function, Set<Address>> regFuncs = new LinkedHashMap<>();
		for (Address s : regStrings.keySet()) {
			Set<Address> sites = useSites.getOrDefault(s, Collections.emptySet());
			out.println("  \"" + regStrings.get(s) + "\" at " + s + ": used at " + sites);
			if (!regStrings.get(s).startsWith("Is3D ") && !regStrings.get(s).equals("Is3D")) continue;
			for (Address site : sites) {
				Function f = fm.getFunctionContaining(site);
				if (f != null) regFuncs.computeIfAbsent(f, k -> new TreeSet<>()).add(site);
			}
		}
		out.println();

		// 2. the two-level byte stores right after each "Is3D" use
		Map<Pair, Integer> storePairs = new LinkedHashMap<>(); // pair -> distance in instructions from the use
		for (Map.Entry<Function, Set<Address>> e : regFuncs.entrySet()) {
			for (Address site : e.getValue()) {
				int dist = 0;
				Map<String, long[]> regA = new HashMap<>();
				InstructionIterator it = listing.getInstructions(site, true);
				while (it.hasNext() && dist < 120) {
					Instruction ins = it.next();
					if (!e.getKey().getBody().contains(ins.getAddress())) break;
					dist++;
					String mn = ins.getMnemonicString();
					if (mn.equals("MOV") && ins.getNumOperands() == 2) {
						Matcher m0 = MEM.matcher(ins.getDefaultOperandRepresentation(0));
						if (m0.matches() && m0.group(1).equals("byte")) {
							long[] ra = regA.get(m0.group(2));
							if (ra != null) {
								Pair p = new Pair(ra[0], disp(m0));
								storePairs.merge(p, dist, Math::min);
							}
						}
					}
					track(ins, regA, null, dist);
				}
			}
		}
		out.println("## 2. Two-level byte stores after an \"Is3D\" use (pair: instructions after the use)");
		for (Map.Entry<Pair, Integer> e : storePairs.entrySet()) out.println("  " + e.getKey() + ": " + e.getValue());
		out.println();

		// 3. small getters [[reg+A]+B] -> byte
		Map<Function, Pair> candidates = new LinkedHashMap<>();
		for (Function f : fm.getFunctions(true)) {
			if (f.isThunk() || f.getBody().getNumAddresses() > 0x30) continue;
			Pair p = byteGetter(f);
			if (p != null) candidates.put(f, p);
		}
		Map<Function, Pair> getters = new LinkedHashMap<>();
		String how;
		for (Map.Entry<Function, Pair> e : candidates.entrySet())
			if (storePairs.containsKey(e.getValue())) getters.put(e.getKey(), e.getValue());
		how = "a small getter of a pair stored after an \"Is3D\" use";
		Set<Pair> pairs = new LinkedHashSet<>(getters.values());
		if (getters.isEmpty() && !storePairs.isEmpty()) {
			Pair best = null;
			int bd = Integer.MAX_VALUE;
			for (Map.Entry<Pair, Integer> e : storePairs.entrySet())
				if (e.getValue() < bd) { bd = e.getValue(); best = e.getKey(); }
			pairs.add(best);
			how = "UNCERTAIN: no getter function matched; the nearest store after an \"Is3D\" use (inline reads only)";
		}
		for (Address a : forcedGetters) {
			Function f = fm.getFunctionAt(a);
			if (f == null) { out.println("  getter:" + a + ": no function there"); continue; }
			Pair p = candidates.containsKey(f) ? candidates.get(f) : byteGetter(f);
			getters.put(f, p);
			if (p != null) pairs.add(p);
			how = "given on the command line (getter:)";
		}
		out.println("## 3. The 3D getter");
		out.println("  chosen by: " + how);
		for (Map.Entry<Function, Pair> e : getters.entrySet())
			out.println("  getter " + e.getKey().getEntryPoint() + " " + e.getKey().getName(true) + " reads " + e.getValue()
				+ ", " + callSites(e.getKey()).size() + " call sites");
		out.println("  pairs used for the inline scan: " + pairs);
		out.println("  other small two-level byte getters (" + candidates.size() + " in all), by call sites:");
		List<Map.Entry<Function, Pair>> cl = new ArrayList<>(candidates.entrySet());
		Map<Function, Integer> nCalls = new HashMap<>();
		for (Map.Entry<Function, Pair> e : cl) nCalls.put(e.getKey(), callSites(e.getKey()).size());
		cl.sort((x, y) -> nCalls.get(y.getKey()) - nCalls.get(x.getKey()));
		for (int i = 0; i < Math.min(12, cl.size()); i++)
			out.println("    " + cl.get(i).getKey().getEntryPoint() + " " + cl.get(i).getValue() + " " + nCalls.get(cl.get(i).getKey()));
		out.println();
		for (Map.Entry<Function, Set<Address>> e : regFuncs.entrySet()) {
			out.println("  -- " + describe(e.getKey()) + " uses \"Is3D\"; around each use:");
			printContext(e.getKey(), e.getValue(), 10, 14);
		}
		out.println();

		// 4. callers of each getter
		Set<Function> getterSet = new HashSet<>(getters.keySet());
		out.println("## 4. Callers of the getter");
		for (Function g : getters.keySet()) {
			Map<Function, Set<Address>> callers = callersOf(g);
			out.println("  getter " + g.getEntryPoint() + ": " + callers.size() + " calling functions");
			summary(callers);
			out.println();
			for (Map.Entry<Function, Set<Address>> e : callers.entrySet()) {
				out.println("### " + describe(e.getKey()));
				printCallerInfo(e.getKey());
				printContext(e.getKey(), e.getValue(), context, context + 2);
				out.println();
			}
		}

		// 5. inline reads and writes of the flag, and two-level dword reads through the display pointer
		Set<Long> as = new HashSet<>();
		for (Pair p : pairs) as.add(p.a);
		Map<Function, Set<Address>> inline = new LinkedHashMap<>();
		Map<Long, Map<Function, Integer>> dwordUses = new TreeMap<>();
		if (!as.isEmpty()) {
			for (Function f : fm.getFunctions(true)) {
				if (getterSet.contains(f) || f.isThunk()) continue;
				Map<String, long[]> regA = new HashMap<>();
				int idx = 0;
				for (Instruction ins : listing.getInstructions(f.getBody(), true)) {
					idx++;
					for (int i = 0; i < ins.getNumOperands(); i++) {
						Matcher m = MEM.matcher(ins.getDefaultOperandRepresentation(i));
						if (!m.matches()) continue;
						long[] ra = regA.get(m.group(2));
						if (ra == null || idx - ra[1] > 10) continue;
						if (m.group(1).equals("byte") && pairs.contains(new Pair(ra[0], disp(m))))
							inline.computeIfAbsent(f, k -> new TreeSet<>()).add(ins.getAddress());
						else if (m.group(1).equals("dword"))
							dwordUses.computeIfAbsent(disp(m), k -> new LinkedHashMap<>()).merge(f, 1, Integer::sum);
					}
					track(ins, regA, as, idx);
				}
			}
		}
		out.println("## 5. Inline reads or writes of the 3D flag outside the getter: " + inline.size() + " functions");
		summary(inline);
		out.println();
		int n = 0;
		for (Map.Entry<Function, Set<Address>> e : inline.entrySet()) {
			if (n++ >= maxInline) { out.println("  (" + (inline.size() - maxInline) + " more not decompiled)"); break; }
			out.println("### " + describe(e.getKey()));
			printCallerInfo(e.getKey());
			printContext(e.getKey(), e.getValue(), context, context + 2);
			out.println();
		}

		// 6. other small accessors of the display pointer (Is3DAccelerationSupported / Recommended and the like)
		out.println("## 6. Other small functions that start from the display pointer and read through it ([[this+A]+T], A = "
			+ hexes(as) + "): Is3DAccelerationSupported/Recommended and the like");
		int accessors = 0;
		for (Function f : fm.getFunctions(true)) {
			if (as.isEmpty()) break;
			if (getterSet.contains(f) || f.isThunk() || f.getBody().getNumAddresses() > 0x40) continue;
			Long first = firstLoad(f);
			if (first == null || !as.contains(first) || !twoLevel(f, first)) continue;
			Map<Function, Set<Address>> callers = callersOf(f);
			if (accessors++ >= 20) {
				out.println("### accessor (not decompiled) " + describe(f) + ", " + callers.size() + " calling functions");
				continue;
			}
			out.println("### accessor " + describe(f) + ", " + callers.size() + " calling functions");
			printWhole(f, 30);
			summary(callers);
			int k = 0;
			for (Map.Entry<Function, Set<Address>> e : callers.entrySet()) {
				if (isFramework(e.getKey())) continue;
				if (k++ >= 10) { out.println("  -- (more callers not decompiled)"); break; }
				out.println("  -- caller " + describe(e.getKey()));
				printCallerInfo(e.getKey());
				printContext(e.getKey(), e.getValue(), 4, 5);
			}
			out.println();
		}

		out.println("## 7. Two-level dword reads through the display pointer, by field offset (functions, up to 12 each)");
		for (Map.Entry<Long, Map<Function, Integer>> e : dwordUses.entrySet()) {
			StringBuilder b = new StringBuilder();
			int k = 0;
			for (Function f : e.getValue().keySet()) {
				if (k++ >= 12) { b.append(" ..."); break; }
				b.append(' ').append(f.getEntryPoint()).append(isFramework(f) ? "[fw]" : "").append(cls(f) == null ? "" : "(" + cls(f) + ")");
			}
			out.println(String.format("  +0x%x: %d functions:%s", e.getKey(), e.getValue().size(), b));
		}
		out.println();

		// 8. strings about 3D and hardware acceleration
		out.println("## 8. Strings about 3D or hardware acceleration, and the functions that use them");
		Pattern sp = Pattern.compile("(?i)(is3d|3d ?accel|hardware ?accel|hw ?accel|direct ?3d|(?<![%\\d.-])\\b3d\\b|nobits3d|nobits2d|2dbig|safe ?mode|software ?mode|renderer)");
		int shown = 0;
		Map<Function, Set<Address>> strFuncs = new LinkedHashMap<>();
		String[] wanted = stringsRegex == null ? new String[0] : stringsRegex.toLowerCase().split("[+,]");
		for (Data d : listing.getDefinedData(true)) {
			if (!d.hasStringValue()) continue;
			String v = String.valueOf(d.getValue());
			boolean wantFull = false;
			for (String w : wanted) if (!w.isEmpty() && v.toLowerCase().contains(w)) wantFull = true;
			if (!wantFull && !sp.matcher(v).find()) continue;
			if (v.startsWith("D3DERR_") || v.startsWith("DDERR_")) continue;
			List<String> users = new ArrayList<>();
			for (Reference r : refs.getReferencesTo(d.getAddress())) {
				Function f = fm.getFunctionContaining(r.getFromAddress());
				users.add(f == null ? r.getFromAddress().toString() : f.getEntryPoint() + (isFramework(f) ? "[fw]" : ""));
				if (wantFull && f != null) strFuncs.computeIfAbsent(f, k -> new TreeSet<>()).add(r.getFromAddress());
			}
			if (shown++ < 80) out.println("  " + d.getAddress() + " \"" + clip(v, 100) + "\" <- " + users);
		}
		out.println();

		// 9. requested strings, decompiled in full
		if (!strFuncs.isEmpty()) {
			out.println("## 9. Functions that use a string containing one of: " + stringsRegex);
			for (Map.Entry<Function, Set<Address>> e : strFuncs.entrySet()) {
				out.println("### " + describe(e.getKey()));
				printCallerInfo(e.getKey());
				printContext(e.getKey(), e.getValue(), 12, 20);
				out.println();
			}
		}

		// 10. readers and writers of given globals
		for (Address flag : flags) {
			Map<Function, Set<Address>> users = new LinkedHashMap<>();
			for (Reference r : refs.getReferencesTo(flag)) {
				Function f = fm.getFunctionContaining(r.getFromAddress());
				if (f != null) users.computeIfAbsent(f, k -> new TreeSet<>()).add(r.getFromAddress());
			}
			out.println("## 10. Users of the global " + flag + ": " + users.size() + " functions");
			summary(users);
			out.println();
			for (Map.Entry<Function, Set<Address>> e : users.entrySet()) {
				out.println("### " + describe(e.getKey()));
				printCallerInfo(e.getKey());
				printContext(e.getKey(), e.getValue(), context, context + 2);
				out.println();
			}
		}
	}

	// ---------------------------------------------------------------------------------------------------------------
	// instruction patterns

	static long disp(Matcher m) {
		return m.group(3) == null ? 0 : Long.parseLong(m.group(3), 16);
	}

	/** Track "MOV reg, dword ptr [x + A]" loads (A in want, or any A when want is null); forget a register when written. */
	private void track(Instruction ins, Map<String, long[]> regA, Set<Long> want, int idx) {
		String mn = ins.getMnemonicString();
		if (mn.equals("CALL")) { regA.clear(); return; }
		if (ins.getNumOperands() == 0) return;
		Object[] o0 = ins.getOpObjects(0);
		if (o0.length != 1 || !(o0[0] instanceof Register)) return;
		if (mn.equals("CMP") || mn.equals("TEST") || mn.equals("PUSH")) return;
		Register r = (Register) o0[0];
		Register base = r.getBaseRegister() != null ? r.getBaseRegister() : r;
		String name = base.getName();
		if (mn.equals("MOV") && ins.getNumOperands() == 2 && r.getBitLength() == 32) {
			Matcher m = MEM.matcher(ins.getDefaultOperandRepresentation(1));
			if (m.matches() && m.group(1).equals("dword") && (want == null || want.contains(disp(m)))) {
				regA.put(name, new long[] { disp(m), idx });
				return;
			}
		}
		regA.remove(name);
	}

	/** A small function with no call that loads [r1 + A] (dword) into r2 and then reads the byte [r2 + B]. */
	private Pair byteGetter(Function f) {
		List<Instruction> ins = new ArrayList<>();
		for (Instruction i : listing.getInstructions(f.getBody(), true)) {
			if (i.getMnemonicString().equals("CALL")) return null;
			ins.add(i);
			if (ins.size() > 12) return null;
		}
		boolean ret = false;
		for (Instruction i : ins) if (i.getMnemonicString().startsWith("RET")) ret = true;
		if (!ret) return null;
		Map<String, long[]> regA = new HashMap<>();
		int idx = 0;
		for (Instruction i : ins) {
			idx++;
			for (int k = 0; k < i.getNumOperands(); k++) {
				Matcher m = MEM.matcher(i.getDefaultOperandRepresentation(k));
				if (!m.matches() || !m.group(1).equals("byte")) continue;
				long[] ra = regA.get(m.group(2));
				if (ra != null && !(k == 0 && i.getMnemonicString().equals("MOV"))) return new Pair(ra[0], disp(m));
			}
			track(i, regA, null, idx);
		}
		return null;
	}

	/** The displacement of the first memory load in a function when it is "MOV reg, dword ptr [ECX + A]" (or any base). */
	private Long firstLoad(Function f) {
		int n = 0;
		for (Instruction i : listing.getInstructions(f.getBody(), true)) {
			if (n++ > 6) return null;
			for (int k = 0; k < i.getNumOperands(); k++) {
				Matcher m = MEM.matcher(i.getDefaultOperandRepresentation(k));
				if (!m.matches()) continue;
				if (m.group(2).equals("ESP") || m.group(2).equals("EBP")) continue;
				if (i.getMnemonicString().equals("MOV") && k == 1 && m.group(1).equals("dword")) return disp(m);
				return null;
			}
		}
		return null;
	}

	/** Whether the function reads a dword through the register it loaded from [x + a]. */
	private boolean twoLevel(Function f, long a) {
		Map<String, long[]> regA = new HashMap<>();
		Set<Long> want = Collections.singleton(a);
		int idx = 0;
		for (Instruction i : listing.getInstructions(f.getBody(), true)) {
			idx++;
			for (int k = 0; k < i.getNumOperands(); k++) {
				Matcher m = MEM.matcher(i.getDefaultOperandRepresentation(k));
				if (m.matches() && m.group(1).equals("dword") && regA.containsKey(m.group(2))) return true;
			}
			track(i, regA, want, idx);
		}
		return false;
	}

	// ---------------------------------------------------------------------------------------------------------------
	// strings

	private List<Address> findString(String s, boolean wide) throws Exception {
		byte[] raw = s.getBytes(StandardCharsets.US_ASCII);
		byte[] pat;
		if (wide) {
			pat = new byte[raw.length * 2 + 2];
			for (int i = 0; i < raw.length; i++) pat[i * 2] = raw[i];
		} else {
			pat = Arrays.copyOf(raw, raw.length + 1);
		}
		List<Address> found = new ArrayList<>();
		for (MemoryBlock b : mem.getBlocks()) {
			if (!b.isInitialized() || b.isExecute()) continue;
			Address a = b.getStart();
			while (a != null && a.compareTo(b.getEnd()) <= 0) {
				Address hit = mem.findBytes(a, b.getEnd(), pat, null, true, monitor);
				if (hit == null) break;
				boolean start = true;
				try {
					int step = wide ? 2 : 1;
					byte prev = mem.getByte(hit.subtract(step));
					if (Character.isLetterOrDigit((char) (prev & 0xff)) || prev == '_') start = false;
				} catch (Exception e) { /* block start */ }
				if (start) found.add(hit);
				a = hit.add(1);
			}
		}
		return found;
	}

	/** Every instruction that refers to one of these addresses, by reference or by an immediate operand. */
	private Map<Address, Set<Address>> stringUses(Collection<Address> targets) {
		Map<Address, Set<Address>> uses = new HashMap<>();
		Map<Long, Address> byOffset = new HashMap<>();
		for (Address t : targets) {
			byOffset.put(t.getOffset(), t);
			for (Reference r : refs.getReferencesTo(t))
				if (listing.getInstructionAt(r.getFromAddress()) != null)
					uses.computeIfAbsent(t, k -> new TreeSet<>()).add(r.getFromAddress());
		}
		if (byOffset.isEmpty()) return uses;
		for (Instruction ins : listing.getInstructions(true)) {
			for (int i = 0; i < ins.getNumOperands(); i++) {
				for (Object o : ins.getOpObjects(i)) {
					if (!(o instanceof Scalar)) continue;
					Address t = byOffset.get(((Scalar) o).getUnsignedValue());
					if (t != null) uses.computeIfAbsent(t, k -> new TreeSet<>()).add(ins.getAddress());
				}
			}
		}
		return uses;
	}

	// ---------------------------------------------------------------------------------------------------------------
	// functions, classes and callers

	private List<Address> callSites(Function g) {
		List<Address> sites = new ArrayList<>();
		List<Address> targets = new ArrayList<>();
		targets.add(g.getEntryPoint());
		Address[] thunks = g.getFunctionThunkAddresses(true);
		if (thunks != null) targets.addAll(Arrays.asList(thunks));
		for (Address t : targets)
			for (Reference r : refs.getReferencesTo(t))
				if (r.getReferenceType().isCall() || r.getReferenceType().isJump()) sites.add(r.getFromAddress());
		return sites;
	}

	private Map<Function, Set<Address>> callersOf(Function g) {
		Map<Function, Set<Address>> callers = new TreeMap<>(Comparator.comparing(Function::getEntryPoint));
		for (Address s : callSites(g)) {
			Function f = fm.getFunctionContaining(s);
			if (f != null && !f.equals(g)) callers.computeIfAbsent(f, k -> new TreeSet<>()).add(s);
		}
		return callers;
	}

	private final Map<Function, String> clsCache = new HashMap<>();

	/** The function's class: its Ghidra namespace, else the RTTI name of a vtable that holds it (with the slot). */
	private String cls(Function f) {
		if (clsCache.containsKey(f)) return clsCache.get(f);
		String c = null;
		Namespace ns = f.getParentNamespace();
		if (ns != null && !ns.isGlobal() && !ns.getName().startsWith("switchD")) c = ns.getName(true);
		if (c == null) {
			for (Reference r : refs.getReferencesTo(f.getEntryPoint())) {
				if (r.getReferenceType().isCall() || r.getReferenceType().isJump()) continue;
				if (listing.getInstructionAt(r.getFromAddress()) != null) continue;
				String v = vtableClass(r.getFromAddress());
				if (v != null) { c = v; break; }
			}
		}
		clsCache.put(f, c);
		return c;
	}

	private String vtableClass(Address slot) {
		try {
			Address a = slot;
			for (int i = 0; i < 600; i++) {
				Address prev = a.subtract(4);
				Address p = toAddr(mem.getInt(prev) & 0xffffffffL);
				MemoryBlock b = mem.getBlock(p);
				if (b == null || !b.isExecute()) break;
				a = prev;
			}
			Address col = toAddr(mem.getInt(a.subtract(4)) & 0xffffffffL);
			if (mem.getBlock(col) == null || mem.getInt(col) != 0) return null;
			Address td = toAddr(mem.getInt(col.add(12)) & 0xffffffffL);
			if (mem.getBlock(td) == null) return null;
			StringBuilder name = new StringBuilder();
			for (int i = 0; i < 200; i++) {
				byte c = mem.getByte(td.add(8 + i));
				if (c == 0) break;
				name.append((char) (c & 0xff));
			}
			String n = name.toString();
			if (!n.startsWith(".?A")) return null;
			long idx = slot.subtract(a) / 4;
			return demangle(n) + " vt" + a + "[" + idx + "]";
		} catch (Exception e) {
			return null;
		}
	}

	static String demangle(String n) {
		String s = n.substring(4);
		if (s.endsWith("@@")) s = s.substring(0, s.length() - 2);
		if (s.startsWith("?$")) return s;
		List<String> parts = new ArrayList<>(Arrays.asList(s.split("@")));
		Collections.reverse(parts);
		return String.join("::", parts);
	}

	private boolean isFramework(Function f) {
		String c = cls(f);
		if (c == null) return false;
		String head = c.split(" ")[0];
		String[] parts = head.split("::");
		return FW.contains(parts[parts.length - 1]);
	}

	private String describe(Function f) {
		String c = cls(f);
		return f.getEntryPoint() + " " + f.getName() + (c == null ? "" : " {" + c + "}") + " size " + f.getBody().getNumAddresses()
			+ (isFramework(f) ? " [fw]" : " [game?]");
	}

	private void summary(Map<Function, Set<Address>> m) {
		for (Map.Entry<Function, Set<Address>> e : m.entrySet())
			out.println("    " + describe(e.getKey()) + "  sites " + e.getValue());
	}

	private void printCallerInfo(Function f) {
		List<String> s = new ArrayList<>();
		for (Instruction i : listing.getInstructions(f.getBody(), true)) {
			for (Reference r : i.getReferencesFrom()) {
				Data d = listing.getDataAt(r.getToAddress());
				if (d != null && d.hasStringValue()) {
					String v = "\"" + clip(String.valueOf(d.getValue()), 60) + "\"";
					if (!s.contains(v)) s.add(v);
				}
			}
			if (s.size() >= 15) break;
		}
		if (!s.isEmpty()) out.println("  strings: " + String.join(", ", s));
		if (cls(f) != null && cls(f).contains(" vt")) return;
		Map<Function, Set<Address>> up = callersOf(f);
		if (up.isEmpty()) { out.println("  called from: (no direct calls; a function pointer or vtable without RTTI?)"); return; }
		List<String> c = new ArrayList<>();
		int k = 0;
		for (Function u : up.keySet()) {
			if (k++ >= 6) { c.add("... " + (up.size() - 6) + " more"); break; }
			c.add(u.getEntryPoint() + (cls(u) == null ? "" : " {" + cls(u) + "}") + (isFramework(u) ? "[fw]" : ""));
		}
		out.println("  called from: " + String.join("; ", c));
	}

	// ---------------------------------------------------------------------------------------------------------------
	// decompiled text

	private List<ClangLine> lines(Function f) {
		DecompileResults res = decomp.decompileFunction(f, 90, monitor);
		if (res == null || !res.decompileCompleted() || res.getCCodeMarkup() == null) {
			out.println("    (decompile failed: " + (res == null ? "?" : res.getErrorMessage()) + ")");
			return null;
		}
		return DecompilerUtils.toLines(res.getCCodeMarkup());
	}

	static String text(ClangLine l) {
		StringBuilder b = new StringBuilder(l.getIndentString());
		for (ClangToken t : l.getAllTokens()) b.append(t.getText());
		return b.toString();
	}

	private void printWhole(Function f, int max) {
		List<ClangLine> ls = lines(f);
		if (ls == null) return;
		for (int i = 0; i < Math.min(max, ls.size()); i++) out.println("    " + text(ls.get(i)));
		if (ls.size() > max) out.println("    ...");
	}

	private void printContext(Function f, Set<Address> sites, int before, int after) {
		List<ClangLine> ls = lines(f);
		if (ls == null) return;
		// each line's address ranges; a site whose instruction maps to no line (a PUSH folded into a call) takes the
		// first of the next 12 instructions that does
		List<Address[]> ranges = new ArrayList<>();
		List<Integer> owner = new ArrayList<>();
		for (int i = 0; i < ls.size(); i++) {
			for (ClangToken t : ls.get(i).getAllTokens()) {
				Address mn = t.getMinAddress(), mx = t.getMaxAddress();
				if (mn == null || mx == null) continue;
				ranges.add(new Address[] { mn, mx });
				owner.add(i);
			}
		}
		Set<Integer> hit = new TreeSet<>();
		for (Address s : sites) {
			Instruction ins = listing.getInstructionAt(s);
			for (int step = 0; step < 13; step++) {
				Address lo = ins == null ? s : ins.getMinAddress();
				Address hi = ins == null ? s : ins.getMaxAddress();
				boolean found = false;
				for (int r = 0; r < ranges.size(); r++) {
					Address[] rg = ranges.get(r);
					if (rg[0].compareTo(hi) <= 0 && rg[1].compareTo(lo) >= 0) { hit.add(owner.get(r)); found = true; }
				}
				if (found || ins == null || !f.getBody().contains(ins.getAddress())) break;
				ins = ins.getNext();
			}
		}
		if (ls.size() <= 70) {
			for (int i = 0; i < ls.size(); i++) out.println((hit.contains(i) ? "  >>" : "    ") + text(ls.get(i)));
			if (hit.isEmpty()) out.println("  (the sites " + sites + " map to no line)");
			return;
		}
		if (hit.isEmpty()) {
			out.println("  (the sites " + sites + " map to no line; the first 40 lines)");
			for (int i = 0; i < 40; i++) out.println("    " + text(ls.get(i)));
			return;
		}
		boolean[] show = new boolean[ls.size()];
		for (int h : hit)
			for (int i = Math.max(0, h - before); i <= Math.min(ls.size() - 1, h + after); i++) show[i] = true;
		show[0] = true;
		int last = -2;
		for (int i = 0; i < ls.size(); i++) {
			if (!show[i]) continue;
			if (i != last + 1) out.println("    ...");
			out.println((hit.contains(i) ? "  >>" : "    ") + text(ls.get(i)));
			last = i;
		}
		if (last != ls.size() - 1) out.println("    ...");
	}

	static String clip(String s, int n) {
		s = s.replace("\r", "\\r").replace("\n", "\\n");
		return s.length() > n ? s.substring(0, n) + "..." : s;
	}

	static String hexes(Set<Long> xs) {
		List<String> s = new ArrayList<>();
		for (long x : xs) s.add(String.format("0x%x", x));
		return String.join(", ", s);
	}
}
