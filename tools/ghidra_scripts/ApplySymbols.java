// Applies recovered symbol names, prototypes, and struct field names to the program.
// Input: a JSON-lines file, one object per line:
//   {"addr":"0041a2b0","name":"Sexy::Board::Update","sig":"void __thiscall Update(void)","comment":"..."}
//   {"addr":"00536f10","label":"gBoardInstance","type":"int"}                 (data label)
//   {"struct":"Board_data","offset":"0x1bc","field":"mGameOverCount","type":"int"}  (struct field)
// "name" may contain "::" to place the function into a class/namespace (created if missing).
// "sig" is optional. Unknown type names in a prototype are retried as "void *" (pointers) or
// "undefined4"; failures are reported, never fatal. Later lines win over earlier ones.
// Usage (headless): -postScript ApplySymbols.java <symbols.jsonl> [more.jsonl ...]
//@category Symbol
import ghidra.app.script.GhidraScript;
import ghidra.app.util.parser.FunctionSignatureParser;
import ghidra.app.cmd.function.ApplyFunctionSignatureCmd;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.util.data.DataTypeParser;

import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

public class ApplySymbols extends GhidraScript {

	private static final Pattern FIELD = Pattern.compile("\"(\\w+)\"\\s*:\\s*\"((?:[^\"\\\\]|\\\\.)*)\"");
	private static final Set<String> BUILTIN = new HashSet<>(Arrays.asList(
		"void", "bool", "char", "uchar", "short", "ushort", "int", "uint", "long", "ulong", "float", "double",
		"wchar_t", "unsigned", "signed", "const", "longlong", "ulonglong", "byte", "word", "dword",
		"undefined", "undefined1", "undefined2", "undefined4", "undefined8", "__thiscall", "__cdecl",
		"__stdcall", "__fastcall"));

	private DataTypeManager dtm;
	private Map<String, Structure> structs;

	private Map<String, String> parse(String line) {
		Map<String, String> m = new HashMap<>();
		Matcher mt = FIELD.matcher(line);
		while (mt.find()) m.put(mt.group(1), mt.group(2).replace("\\\"", "\"").replace("\\\\", "\\"));
		return m;
	}

	private Namespace namespaceFor(String qualified) throws Exception {
		String[] parts = qualified.split("::");
		Namespace ns = currentProgram.getGlobalNamespace();
		SymbolTable st = currentProgram.getSymbolTable();
		for (int i = 0; i < parts.length - 1; i++) {
			Namespace child = st.getNamespace(parts[i], ns);
			if (child == null) child = st.createClass(ns, parts[i], SourceType.USER_DEFINED);
			ns = child;
		}
		return ns;
	}

	private boolean knownType(String name) {
		if (BUILTIN.contains(name)) return true;
		List<DataType> out = new ArrayList<>();
		dtm.findDataTypes(name, out);
		return !out.isEmpty();
	}

	/** Replace identifiers that are not known data types with void (pointers) / undefined4. */
	private String sanitizeSig(String sig) {
		int paren = sig.indexOf('(');
		if (paren < 0) return sig;
		String head = sig.substring(0, paren);
		String params = sig.substring(paren);
		// return type: everything before the last identifier of head (the function name)
		Matcher nm = Pattern.compile("(\\w+)\\s*$").matcher(head);
		if (!nm.find()) return sig;
		String ret = head.substring(0, nm.start());
		String fname = nm.group(1);
		return fixTypes(ret, false) + fname + fixParams(params);
	}

	private String fixParams(String params) {
		String inner = params.substring(1, params.lastIndexOf(')'));
		if (inner.trim().isEmpty() || inner.trim().equals("void")) return "(" + inner + ")";
		List<String> out = new ArrayList<>();
		for (String p : inner.split(",")) out.add(fixTypes(p, true));
		return "(" + String.join(",", out) + ")";
	}

	private String fixTypes(String decl, boolean isParam) {
		String d = decl.replace("&", "*").replaceAll("\\b(struct|class|enum)\\s+", "").replaceAll("\\w+::", "");
		Matcher m = Pattern.compile("[A-Za-z_]\\w*").matcher(d);
		StringBuffer sb = new StringBuffer();
		List<String> idents = new ArrayList<>();
		while (m.find()) idents.add(m.group());
		// for parameters, the last identifier is the parameter name when there are >= 2 identifiers
		String paramName = (isParam && idents.size() >= 2) ? idents.get(idents.size() - 1) : null;
		m.reset();
		int seen = 0;
		while (m.find()) {
			String id = m.group();
			seen++;
			boolean isName = paramName != null && seen == idents.size();
			if (!isName && !knownType(id)) m.appendReplacement(sb, d.substring(m.end()).trim().startsWith("*") ? "void" : "undefined4");
			else m.appendReplacement(sb, id);
		}
		m.appendTail(sb);
		return sb.toString();
	}

	private static final Pattern CC = Pattern.compile("\\b(__thiscall|__cdecl|__stdcall|__fastcall)\\b");

	private boolean applySig(Function f, String sig) {
		String cc = null;
		Matcher ccm = CC.matcher(sig);
		if (ccm.find()) cc = ccm.group(1);
		sig = CC.matcher(sig).replaceAll("").replaceAll("\\s+", " ").trim();
		// qualified function names are not accepted by the parser: keep only the simple name
		sig = sig.replaceAll("[\\w:]+::(\\w+)\\s*\\(", "$1(");
		// members default to __thiscall; static members must say __cdecl explicitly
		if (cc == null && f.getParentNamespace() instanceof GhidraClass) cc = "__thiscall";
		for (int attempt = 0; attempt < 2; attempt++) {
			String s = attempt == 0 ? sig : sanitizeSig(sig);
			try {
				FunctionSignatureParser p = new FunctionSignatureParser(dtm, null);
				FunctionDefinitionDataType def = p.parse(f.getSignature(), s);
				if (cc != null) def.setCallingConvention(cc);
				ApplyFunctionSignatureCmd cmd = new ApplyFunctionSignatureCmd(f.getEntryPoint(), def, SourceType.USER_DEFINED);
				if (cmd.applyTo(currentProgram)) return true;
			}
			catch (Exception e) {
				if (attempt == 1) printerr("sig failed " + f.getEntryPoint() + " '" + sig + "' / '" + s + "': " + e.getMessage());
			}
		}
		return false;
	}

	private Structure findStruct(String name) {
		if (structs == null) {
			structs = new HashMap<>();
			Iterator<Structure> it = dtm.getAllStructures();
			while (it.hasNext()) {
				Structure s = it.next();
				structs.putIfAbsent(s.getName(), s);
			}
		}
		return structs.get(name);
	}

	private DataType parseType(String t) {
		try {
			DataTypeParser dp = new DataTypeParser(dtm, dtm, null, DataTypeParser.AllowedDataTypes.ALL);
			return dp.parse(t.replaceAll("\\w+::", ""));
		}
		catch (Exception e) {
			return null;
		}
	}

	private boolean applyField(Map<String, String> m) {
		Structure s = findStruct(m.get("struct"));
		if (s == null) { printerr("no struct " + m.get("struct")); return false; }
		int off = Integer.decode(m.get("offset"));
		String name = m.get("field");
		DataType dt = m.containsKey("type") ? parseType(m.get("type")) : null;
		if (dt != null && dt.getLength() > 0 && off + dt.getLength() <= s.getLength()) {
			try {
				s.replaceAtOffset(off, dt, dt.getLength(), name, m.get("comment"));
				return true;
			}
			catch (Exception e) {
				// fall through to rename-only
			}
		}
		DataTypeComponent c = s.getComponentContaining(off);
		if (c == null || c.getOffset() != off) {
			// undefined bytes: insert a 1-byte placeholder so the name exists
			try {
				s.replaceAtOffset(off, Undefined1DataType.dataType, 1, name, m.get("comment"));
				return true;
			}
			catch (Exception e) {
				printerr("field failed " + m + ": " + e.getMessage());
				return false;
			}
		}
		try {
			c.setFieldName(name);
			if (m.containsKey("comment")) c.setComment(m.get("comment"));
			return true;
		}
		catch (Exception e) {
			printerr("field rename failed " + m + ": " + e.getMessage());
			return false;
		}
	}

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		if (args.length < 1) { printerr("usage: ApplySymbols.java <symbols.jsonl> ..."); return; }
		FunctionManager fm = currentProgram.getFunctionManager();
		dtm = currentProgram.getDataTypeManager();
		int renamed = 0, sigs = 0, labels = 0, fields = 0, failed = 0;
		List<String> lines = new ArrayList<>();
		for (String a : args) lines.addAll(Files.readAllLines(Paths.get(a)));
		// structs first so prototypes can reference renamed types
		for (String line : lines) {
			Map<String, String> m = parse(line.trim());
			if (m.containsKey("struct")) { if (applyField(m)) fields++; else failed++; }
		}
		for (String line : lines) {
			line = line.trim();
			if (line.isEmpty() || line.startsWith("//")) continue;
			Map<String, String> m = parse(line);
			if (!m.containsKey("addr")) continue;
			Address a = toAddr(Long.parseLong(m.get("addr").replaceFirst("^0x", ""), 16));
			try {
				if (m.containsKey("name")) {
					Function f = fm.getFunctionAt(a);
					if (f == null) f = createFunction(a, null);
					if (f == null) { printerr("no function at " + a); failed++; continue; }
					String q = m.get("name");
					Namespace ns = namespaceFor(q);
					String simple = q.contains("::") ? q.substring(q.lastIndexOf("::") + 2) : q;
					f.setParentNamespace(ns);
					f.setName(simple, SourceType.USER_DEFINED);
					renamed++;
					if (m.containsKey("sig") && applySig(f, m.get("sig"))) sigs++;
					if (m.containsKey("comment")) f.setComment(m.get("comment"));
				}
				else if (m.containsKey("label")) {
					String q = m.get("label");
					Namespace ns = namespaceFor(q);
					String simple = q.contains("::") ? q.substring(q.lastIndexOf("::") + 2) : q;
					Symbol s = currentProgram.getSymbolTable().createLabel(a, simple, ns, SourceType.USER_DEFINED);
					s.setPrimary();
					labels++;
					if (m.containsKey("type")) {
						DataType dt = parseType(m.get("type"));
						if (dt != null) {
							clearListing(a, a.add(Math.max(dt.getLength(), 1) - 1));
							createData(a, dt);
						}
					}
				}
			}
			catch (Exception e) {
				printerr("failed " + line + ": " + e.getMessage());
				failed++;
			}
		}
		println("ApplySymbols: renamed=" + renamed + " sigs=" + sigs + " labels=" + labels + " fields=" + fields +
			" failed=" + failed);
	}
}
