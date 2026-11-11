#!/usr/bin/env python3
"""Find unnamed and unknown identifiers in a port's source (docs/CODE_STANDARDS.md, "Names").

    python tools/namecheck.py games/<game>               checks games/<game>/src, with games/<game>/namecheck.txt
                                                         (and the folders its "folder" and "engine" lines add)
    python tools/namecheck.py games/<game> --list        also prints every allowed and exempted finding
    python tools/namecheck.py <file or folder>... --config <file>
    python tools/namecheck.py --selftest                 the checker on its own small cases

Prints one line per finding, "path:line: [check] name: why", then "namecheck: N findings (...)", and exits 1 when
there is any (2 on a bad config), 0 otherwise. Comments and string literals are taken out of the code before the
identifier checks, so a log text such as "unknown key" or a save field's name in quotes is never a finding; the
comment check reads only the comments.

The checks (each one a rule of docs/CODE_STANDARDS.md):
  ghidra    a decompiler's stand-in name: local_1c, param_2, iVar3, uStack_8, DAT_00506560, FUN_004cb182,
            field_0x1c, PTR_..., LAB_..., in_EAX, extraout_..., unaff_..., undefined4 (a type)
  unknown   a name that says it is unknown: mUnknown, mUnk, unk_10, aUnk2, mUnnamed
  standin   a generic word with a number or an offset in place of a name: mUnused218, mPad04, mField1c, mValue2,
            aInt3, gData5064f0, mTemp, mDummy, mUnusedImagesA (a number that means something, as in mSwapPiece1 and 2, is not one)
  offset    a name that ends in its offset or address in place of the rest of its name: mScale288, mFlags1E4,
            mCamera144C, Rand85B970, Record1C (three or more upper-case hex characters with a digit among them, or two
            with a hex letter; 2D and 3D, lower-case tails and short numbers such as mSwapPiece1 or aCos36 are not)
  comment   a comment that puts "unknown" in place of a name: "purpose unknown", "unknown field", "??", "no idea",
            "TODO: name"
  stage     a comment that names a development stage, milestone or phase by its number ("stage 7b", "phase 2",
            "milestone 3"): a reader of the code cannot follow it, so the comment names the feature instead. A game
            whose own word it is (a game's stage 3, a boss's phase 2), or a file or folder name that carries it
            (mods/stage4), takes an allow rule on the matched text, with "." for its space: allow stage stage.?4
  short     a one-letter variable or parameter still used more than 10 lines after it is declared (short-span), outside
            the letters the config allows (by default i, j, k for indices, x, y, z for coordinates and g for a
            Graphics*, the framework's own convention): a one-letter name is fine where its type is on the same
            screen, as in a lambda's parameters or a three-line helper
  tag       an address in a comment of a shared engine (engines/<name>, docs/ENGINES.md) that does not name its game:
            the engine's comments give each game's address as "0x40c2e0 (cosmicbugs), 0x40c4d0 (waterbugs)" (a range
            as "0x40c1a0..0x40c890 (cosmicbugs)"), with the game's folder name, so that each game's checks read its
            own. Only the addresses of an exe (0x400000 to 0x7fffff) count; a word in parentheses that is not a game's
            name ("0x41ac20 (launched)") is not a tag
  addr      a function definition with neither an address (0x4cb182) nor a port marker ("port:", "port(...)") in
            its comment, so that the coverage check can pair every function with the export; a file whose first
            comment carries the port marker holds only the port's own functions. An address or marker on the
            function's declaration in a header that is checked too (its class's declaration, in the comment on its
            lines or the comment block just above) counts as well, as it does for the coverage check

The config (games/<game>/namecheck.txt) holds one rule a line, each ending in " -- <reason>"; a rule without a
reason is an error, so every exception says why it is there:
  allow <check> <name regex>            the name (whole identifier) is not a finding of that check
  exempt <check> <path glob>            the check does not run on those files (relative to the game folder)
  short-ok <path glob> <letters>        more one-letter names allowed in those files, e.g. "r g b a" in drawing code
  port-file <path glob>                 every function in those files is the port's own (as a marker would say)
  short-span <lines>                    how far a one-letter name may reach before it is a finding (default 10)
  addr-skip <name regex>                function names the addr check skips (helpers local to one function)
  folder <name>                         another source folder of the game, checked with src/ when the game folder is
                                        given (a port with an engine of its own beside src/); its files are listed
                                        and matched as <name>/<file>
  engine <name>                         a shared engine, engines/<name> at the top of the repository, checked with the
                                        game's src/ and run through the tag check as well; its files are listed and
                                        matched as engines/<name>/<file>
"""
import argparse, fnmatch, os, re, sys

EXTENSIONS = (".cpp", ".h", ".c", ".hpp", ".inl", ".cc")

GHIDRA = re.compile(r"^(?:local_[0-9a-fA-F_]+|param_\d+|[a-z]{1,2}Var\d+|[a-z]{0,3}[Ss]tack_?[0-9a-fA-F_]+|"
                    r"(?:DAT|FUN|PTR|LAB|SUB|thunk_FUN|switchD|caseD|BYTE|WORD|DWORD|QWORD)_[0-9a-fA-F]{4,}\w*|"
                    r"field_?0x[0-9a-fA-F]+|field\d+_0x[0-9a-fA-F]+|in_[A-Z]{2,3}\w*|extraout_\w+|unaff_\w+|"
                    r"undefined\d*|_?offset_0x[0-9a-fA-F]+|offset_\d+)$")
UNKNOWN = re.compile(r"(?:(?:^|_)[Uu]|(?<=[a-z0-9])U)n(?:k(?:nown)?|named)(?=[A-Z0-9_]|$)")
STANDIN_WORDS = ("Unused|Pad|Padding|Field|Data|Value|Val|Var|Int|Float|Double|Flag|Bool|Temp|Tmp|Dummy|Spare|"
                 "Reserved|Byte|Word|Dword|Ptr|Pointer|Offset|Member|Thing|Stuff|Unk|Misc|Blah|Foo|Bar")
# a prefix (m, g, s, k, a, an, the, or nothing) and a generic word with a number, a hex offset or nothing after it
STANDIN = re.compile(r"^(?:m|g|s|k|a|an|the)?_?(?:%s)_?(?:0x)?[0-9A-Fa-f]*[0-9][0-9A-Fa-f]*$" % STANDIN_WORDS)
STANDIN_BARE = re.compile(r"^(?:m|g|s|a|an|the)(?:Temp|Tmp|Dummy|Foo|Blah|Stuff|Thing|Misc|Unused(?:[A-Z]\w*)?)$")
# a word, then an offset or an address of upper-case hex in place of the rest of the name (not "2D" or "3D")
OFFSET = re.compile(r"^(?:(?:m|g|s|k|a|an|the)[A-Z]|[A-Z])\w*?[a-z_]((?:0x)?[0-9A-F]{2,8})$")


STAGE = re.compile(r"\b(?:stage|phase|milestone)[ -]?\d+[a-z]?\b", re.I)


def is_offset_name(name):
    m = OFFSET.match(name)
    if not m or name.isupper():
        return False
    tail = m.group(1).replace("0x", "")
    # 2D and 3D; a year, as in the framework's Dialog2004; a float suffix, as in Atan2F
    if not re.search(r"[0-9]", tail) or re.search(r"[0-9]D$", tail) or re.fullmatch(r"(?:19|20)[0-9]{2}", tail) or \
            re.fullmatch(r"[0-9]F", tail):
        return False
    return len(tail) >= 3 or re.search(r"[A-CEF]", tail) is not None


COMMENT = re.compile(r"(?i)(?:\b(?:purpose|meaning|use|role|function|type)\s+(?:is\s+)?(?:unknown|unclear|not known)\b|"
                     r"\bunknown\s+(?:purpose|meaning|use|role|field|member|value|flag|variable|param(?:eter)?|"
                     r"global|static|data|offset|struct|thing)\b|"
                     r"^\s*(?:\+0x[0-9a-f]+\s*)?\(?unknown\)?\s*\.?$|\?\?|\bno idea\b|\bTODO:?\s*name\b|\bunnamed\b|"
                     r"\bwhat (?:this|it) (?:is|does) is unknown\b)")
BUILTIN = (r"(?:(?:const|unsigned|signed|static|volatile|register|long|short)\s+)*"
           r"(?:int|char|bool|double|float|auto|size_t|long|short|unsigned|uchar|uint|ulong|ushort|wchar_t|"
           r"u?int(?:8|16|32|64)_t|SexyChar)")
TYPE = r"(?:%s|(?:const\s+)?(?:\w+::)*[A-Z]\w*(?:<[^;(){}]*>)?)" % BUILTIN
SHORT = re.compile(r"(?<![\w.>])%s(?:\s*(?:\*|&)+\s*|\s+)(?:const\s+)?([A-Za-z])\b\s*(?=[=;,)\[:{(])" % TYPE)
FUNC_DEF = re.compile(r"^(?!\s)(?!(?:if|for|while|switch|return|else|do|case|default|namespace|class|struct|enum|"
                      r"union|typedef|using|template|public|private|protected|#)\b)"
                      r"[\w:<>,*&~ \[\]]*?\b((?:\w+::)*~?\w+|operator\s*\S+)\s*\(")
DEFAULT_SHORT = set("ijkxyzg")
PORT_MARK = re.compile(r"\bport\s*(?::|\()")
ADDR = re.compile(r"\b0x[0-9a-fA-F]{5,8}\b")
# an address of an exe, a range's end, and the tag after it
TAGGED = re.compile(r"\b0x([0-9a-fA-F]{5,8})\b(?:\.\.0x([0-9a-fA-F]{5,8})\b)?(?: \(([a-z0-9]+)\))?")
EXE_LOW, EXE_HIGH = 0x400000, 0x7fffff


def strip(text):
    """The code with comments and literals blanked (lines kept), and the comments as (line, text)."""
    out, comments = [], []
    i, n, line = 0, len(text), 1
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            j = text.find("\n", i)
            j = n if j < 0 else j
            comments.append((line, text[i + 2:j]))
            out.append(" " * (j - i))
            i = j
        elif c == "/" and i + 1 < n and text[i + 1] == "*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            body = text[i + 2:j - 2]
            for k, part in enumerate(body.split("\n")):
                comments.append((line + k, part))
            chunk = text[i:j]
            out.append("".join("\n" if ch == "\n" else " " for ch in chunk))
            line += chunk.count("\n")
            i = j
        elif c == "R" and text.startswith('R"', i) and (i == 0 or not (text[i - 1].isalnum() or text[i - 1] == "_")):
            k = text.find("(", i)
            delim = text[i + 2:k]
            j = text.find(")" + delim + '"', k)
            j = n if j < 0 else j + len(delim) + 2
            chunk = text[i:j]
            out.append("".join("\n" if ch == "\n" else " " for ch in chunk))
            line += chunk.count("\n")
            i = j
        elif c in "\"'":
            if c == "'" and i > 0 and text[i - 1].isalnum():   # a digit separator, 1'000
                out.append(c)
                i += 1
                continue
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            if j < n and text[j] == c:
                j += 1
            out.append(c + " " * (j - i - 2) + c if j - i >= 2 else " ")
            i = j
        else:
            if c == "\n":
                line += 1
            out.append(c)
            i += 1
    return "".join(out), comments


class Config:
    def __init__(self):
        self.allow = {}       # check -> [regex]
        self.exempt = {}      # check -> [glob]
        self.short_ok = []    # (glob, set of letters)
        self.port_files = []
        self.addr_skip = []
        self.folders = []     # more folders of the game folder to check
        self.engines = []     # shared engines (engines/<name>) to check with the game's src/
        self.short_span = 10

    @staticmethod
    def load(path):
        cfg = Config()
        if not path or not os.path.exists(path):
            return cfg
        for number, raw in enumerate(open(path, encoding="utf-8"), 1):
            text = raw.strip()
            if not text or text.startswith("#"):
                continue
            if " -- " not in text:
                raise ValueError("%s:%d: a rule needs ' -- <reason>'" % (path, number))
            rule = text.split(" -- ", 1)[0].split()
            kind = rule[0]
            if kind == "allow" and len(rule) == 3:
                cfg.allow.setdefault(rule[1], []).append(re.compile("^(?:%s)$" % rule[2]))
            elif kind == "exempt" and len(rule) == 3:
                cfg.exempt.setdefault(rule[1], []).append(rule[2])
            elif kind == "short-ok" and len(rule) >= 3:
                cfg.short_ok.append((rule[1], set("".join(rule[2:]))))
            elif kind == "port-file" and len(rule) == 2:
                cfg.port_files.append(rule[1])
            elif kind == "short-span" and len(rule) == 2:
                cfg.short_span = int(rule[1])
            elif kind == "addr-skip" and len(rule) == 2:
                cfg.addr_skip.append(re.compile("^(?:%s)$" % rule[1]))
            elif kind == "folder" and len(rule) == 2:
                cfg.folders.append(rule[1])
            elif kind == "engine" and len(rule) == 2:
                cfg.engines.append(rule[1])
            else:
                raise ValueError("%s:%d: cannot read rule %r" % (path, number, text))
        return cfg

    def allowed(self, check, name):
        return any(r.match(name) for r in self.allow.get(check, ()))

    def exempted(self, check, rel):
        return any(fnmatch.fnmatch(rel, g) for g in self.exempt.get(check, ()))

    def short_letters(self, rel):
        letters = set(DEFAULT_SHORT)
        for glob, extra in self.short_ok:
            if fnmatch.fnmatch(rel, glob):
                letters |= extra
        return letters


def identifiers(code):
    for number, text in enumerate(code.split("\n"), 1):
        if text.lstrip().startswith("#include"):
            continue
        for m in re.finditer(r"\b[A-Za-z_]\w*\b", text):
            yield number, m.group(0)


def check_file(path, rel, cfg, findings, skipped):
    text = open(path, encoding="utf-8", errors="replace").read()
    code, comments = strip(text)
    lines = code.split("\n")

    def report(check, number, name, why):
        if cfg.exempted(check, rel) or cfg.allowed(check, name):
            skipped.append((rel, number, check, name, why))
        else:
            findings.append((rel, number, check, name, why))

    seen = set()
    for number, name in identifiers(code):
        key = (number, name)
        if key in seen:
            continue
        seen.add(key)
        if GHIDRA.match(name):
            report("ghidra", number, name, "a decompiler's stand-in name")
        elif UNKNOWN.search(name):
            report("unknown", number, name, "a name that says it is unknown")
        elif STANDIN.match(name) or STANDIN_BARE.match(name):
            report("standin", number, name, "a generic word or a number in place of a name")
        elif is_offset_name(name):
            report("offset", number, name, "an offset or address in place of the rest of a name")

    for number, body in comments:
        m = COMMENT.search(body)
        if m:
            report("comment", number, m.group(0).strip(), "a comment that puts 'unknown' in place of a name")
        for m in STAGE.finditer(body):
            report("stage", number, m.group(0), "a comment that names a development stage, not the feature")

    if rel.startswith("engines/"):
        games = getattr(cfg, "games", None)
        for number, body in comments:
            for m in TAGGED.finditer(body):
                ends = [m.group(1)] + ([m.group(2)] if m.group(2) else [])
                if not any(EXE_LOW <= int(a, 16) <= EXE_HIGH for a in ends):
                    continue
                if m.group(3) is None or (games is not None and m.group(3) not in games):
                    report("tag", number, "0x" + m.group(1), "an address in a shared engine without its game's tag")

    letters = cfg.short_letters(rel)
    for number, codeline in enumerate(lines, 1):
        if codeline.lstrip().startswith("#"):
            continue
        for m in SHORT.finditer(codeline):
            letter = m.group(1)
            if letter in letters:
                continue
            span = scope_span(lines, number - 1, m.end(), letter)
            if span > cfg.short_span:
                report("short", number, letter, "a one-letter name used over %d lines" % span)

    # addr: the comment block right above a definition (or on its line) names an address or the port marker
    is_port_file = any(fnmatch.fnmatch(rel, g) for g in cfg.port_files)
    if not is_port_file and comments:
        first = comments[0][1]
        is_port_file = bool(PORT_MARK.search(first)) and comments[0][0] <= 3
    if is_port_file or not rel.endswith((".cpp", ".c", ".cc")):
        return
    by_line = {}
    for number, body in comments:
        by_line.setdefault(number, []).append(body)
    raw = text.split("\n")
    depth = 0
    for index, codeline in enumerate(lines):
        number = index + 1
        if depth == 0:
            m = FUNC_DEF.match(codeline)
            if m and not codeline.rstrip().endswith(";") and not re.match(r"^\s*\w+\s*\(", codeline.split("//")[0]) \
                    and is_definition(lines, index):
                name = m.group(1)
                short_name = name.split("::")[-1]
                if not any(r.match(short_name) or r.match(name) for r in cfg.addr_skip):
                    block = list(by_line.get(number, []))
                    k = index - 1
                    while k >= 0 and (raw[k].strip().startswith(("//", "/*", "*", "template")) or
                                      (raw[k].strip() == "" and False)):
                        block += by_line.get(k + 1, [])
                        k -= 1
                    joined = " ".join(block)
                    declared = getattr(cfg, "declared", set())
                    # the header's key is the innermost class and the name (Inner::Name for Outer::Inner::Name); a
                    # name under a namespace alone (Sexy::Name) matches a free function's declaration
                    parts = name.split("::")
                    in_header = name in declared or (len(parts) > 2 and "::".join(parts[-2:]) in declared) or \
                        (len(parts) == 2 and parts[0] in getattr(cfg, "namespaces", set()) and parts[1] in declared)
                    if not ADDR.search(joined) and not PORT_MARK.search(joined) and not in_header:
                        report("addr", number, name, "a function without an address or a port marker above it")
        depth += codeline.count("{") - codeline.count("}")
        if depth < 0:
            depth = 0
    return


CLASS_HEAD = re.compile(r"^\s*(?:template\s*<[^>]*>\s*)?(?:class|struct)\s+(?:\w+\s+)*?(\w+)\s*(?:final\s*)?(?::[^;{]*)?$|"
                        r"^\s*(?:template\s*<[^>]*>\s*)?(?:class|struct)\s+(?:\w+\s+)*?(\w+)\s*(?:final\s*)?(?::[^;{]*)?\{")
DECL_NAME = re.compile(r"(?:^|[\s*&:])(~?\w+|operator\s*[^\s(]+)\s*\(")


def declared_addresses(path):
    """Qualified names (Class::Name, or Name outside a class) whose declaration in this header carries an address or the
    port marker: in the comment on the declaration's lines or in the comment block just above it."""
    text = open(path, encoding="utf-8", errors="replace").read()
    code, comments = strip(text)
    lines = code.split("\n")
    raw = text.split("\n")
    by_line = {}
    for number, body in comments:
        by_line.setdefault(number, []).append(body)
    found = set()
    stack = []          # (class name, depth inside its braces)
    depth = 0
    pending = None
    index = 0
    while index < len(lines):
        codeline = lines[index]
        m = CLASS_HEAD.match(codeline)
        if m and not codeline.rstrip().endswith(";"):
            pending = m.group(1) or m.group(2)
        in_scope = (stack and depth == stack[-1][1]) or (not stack)
        start_depth = depth
        d = DECL_NAME.search(codeline) if in_scope and pending is None else None
        end = index
        if d and not re.match(r"^\s*(?:return|if|for|while|switch|#|typedef|using|static_assert)\b", codeline):
            # the statement runs to its ';' or '{'
            while end < len(lines) - 1 and ";" not in lines[end] and "{" not in lines[end]:
                end += 1
            block = []
            for k in range(index, end + 1):
                block += by_line.get(k + 1, [])
            k = index - 1
            while k >= 0 and raw[k].strip().startswith("//"):
                block += by_line.get(k + 1, [])
                k -= 1
            joined = " ".join(block)
            if ADDR.search(joined) or PORT_MARK.search(joined):
                name = d.group(1).replace(" ", "")
                found.add("%s::%s" % (stack[-1][0], name) if stack else name)
        for k in range(index, end + 1):
            for ch in lines[k]:
                if ch == "{":
                    depth += 1
                    if pending:
                        stack.append((pending, depth))
                        pending = None
                elif ch == "}":
                    if stack and stack[-1][1] == depth:
                        stack.pop()
                    depth = max(depth - 1, 0)
        index = end + 1
    return found


def scope_span(lines, index, column, letter):
    """How many lines after its declaration a name is last used, within the block (or the function, for a parameter
    or a for-loop's variable) that declares it."""
    word = re.compile(r"(?<![\w.])%s\b(?!\s*::)" % re.escape(letter))
    head = lines[index][:column]
    in_parens = head.count("(") > head.count(")")
    depth, risen, last = 0, False, index
    rest = lines[index][column:]
    for k in range(index, len(lines)):
        text = rest if k == index else lines[k]
        for ch in text:
            if ch == "{":
                depth += 1
                risen = True
            elif ch == "}":
                depth -= 1
        if word.search(text) and k != index:
            last = k
        if depth < 0 or (in_parens and risen and depth == 0):
            break
        if k - index > 2000:
            break
    return last - index


def is_definition(lines, index):
    """A definition: its parameter list closes and a '{' (or ': init') follows before any ';'."""
    text = ""
    for k in range(index, min(index + 12, len(lines))):
        text += lines[k] + "\n"
        brace, semi = text.find("{"), text.find(";")
        if brace >= 0 and (semi < 0 or brace < semi):
            head = text[:brace]
            return head.count("(") > 0 and "=" not in head.split(")")[-1].replace("==", "")
        if semi >= 0:
            return False
    return False


def walk(paths):
    for p in paths:
        if os.path.isfile(p):
            yield p
            continue
        for root, dirs, files in os.walk(p):
            dirs.sort()
            for f in sorted(files):
                if f.endswith(EXTENSIONS):
                    yield os.path.join(root, f)


SELFTEST = [
    # (source, expected findings as "check name")
    ("int local_1c = param_2 + iVar3;\n", ["ghidra local_1c", "ghidra param_2", "ghidra iVar3"]),
    ("int x = DAT_00506560; FUN_004cb182(field_0x1c);\n", ["ghidra DAT_00506560", "ghidra FUN_004cb182", "ghidra field_0x1c"]),
    ("int mUnknown; int mUnk3; int unk_10; int aChunkHeader; int mUnkeyed;\n", ["unknown mUnknown", "unknown mUnk3", "unknown unk_10"]),
    ("int mUnused218; char mPad04[3]; int mValue2; int aDummy; Image* mUnusedImagesA;\n",
     ["standin mUnused218", "standin mPad04", "standin mValue2", "standin aDummy", "standin mUnusedImagesA"]),
    ("int mSwapPiece1; int mX0; int mPadBeforeX; int mClearedAtMoveStart;\n", []),
    ("float mScale288; int mFlags1E4; Camera mCamera144C; int Rand85B970(); struct Record1C;\n",
     ["offset mScale288", "offset mFlags1E4", "offset mCamera144C", "offset Rand85B970", "offset Record1C"]),
    ("bool mIs3D; Viz2D* mViz; float aCos36; int Clamp01(); int mPiece2; TwistD3D* mD3D;\n", []),
    ('const char* aText = "unknown field id mUnused218 local_1c";\n', []),
    ("int mFlags; // +0x218 purpose unknown\nint mCount; // the number of unknown keys\n", ["comment purpose unknown"]),
    ("int mDirt; // stage 7b: the dirt layers\n// Phase-2 of the Milestone 3 work\nint mGold; // the gold board\n",
     ["stage stage 7b", "stage Phase-2", "stage Milestone 3"]),
    ("// the stages of a rocket, the next phase, stage one\nconst char* aText = \"stage 2\";\n", []),
    ("// 0x401000\nvoid F()\n{\n\tint q = 0;\n" + "\tq++;\n" * 12 + "}\n", ["short q"]),
    ("// 0x401000\nvoid F()\n{\n\tint q = 0;\n\tq++;\n}\n", []),
    ("// 0x401000\nvoid F(Graphics* g)\n{\n" + "\tg->Draw();\n" * 20 + "\tfor (int i = 0; i < 3; i++) x(i);\n}\n", []),
    ("// 0x4cd287\nbool Board::FindSets()\n{\n\treturn false;\n}\n", []),
    ("// port: the port's own\nstatic bool Helper()\n{\n\treturn false;\n}\n", []),
    ("// Reads one record\nstatic bool Helper()\n{\n\treturn false;\n}\n", ["addr Helper"]),
    ("int Helper();\nint aValue = Helper();\n", []),
]

# (source, expected findings) of a file of a shared engine, where an address names its game
TAG_SELFTEST = [
    ("// 0x401000 (cosmicbugs), 0x401100 (waterbugs)\nvoid F()\n{\n}\n", []),
    ("// 0x401000\nvoid F()\n{\n}\n", ["tag 0x401000"]),
    ("// 0x401000..0x402000 (cosmicbugs), 0x401200..0x402200 (waterbugs)\nvoid F()\n{\n}\n", []),
    ("// 0x41ac20 (launched): slows by its drag\nvoid F()\n{\n}\n", ["tag 0x41ac20"]),
    ("// 0x401000 (cosmicbugs): the mask 0xff0000 and the size 0x100000 are not addresses\nvoid F()\n{\n}\n", []),
    ("// 0x401000 (nosuchgame)\nvoid F()\n{\n}\n", ["tag 0x401000"]),
    ("// port: the port's own, with 0x401000 (cosmicbugs) in a mention\nvoid F()\n{\n}\n", []),
]


def selftest():
    import tempfile
    failures = 0
    with tempfile.TemporaryDirectory() as folder:
        for index, (source, expected) in enumerate(SELFTEST):
            path = os.path.join(folder, "case%d.cpp" % index)
            with open(path, "w", encoding="utf-8", newline="\n") as f:
                f.write(source)
            findings = []
            check_file(path, "case.cpp", Config(), findings, [])
            got = sorted("%s %s" % (f[2], f[3]) for f in findings)
            if got != sorted(expected):
                failures += 1
                print("selftest case %d: expected %s, got %s" % (index, sorted(expected), got))
        tag_config = Config()
        tag_config.games = {"cosmicbugs", "waterbugs"}
        for index, (source, expected) in enumerate(TAG_SELFTEST):
            path = os.path.join(folder, "tag%d.cpp" % index)
            with open(path, "w", encoding="utf-8", newline="\n") as f:
                f.write(source)
            findings = []
            check_file(path, "engines/shared/tag.cpp", tag_config, findings, [])
            got = sorted("%s %s" % (f[2], f[3]) for f in findings if f[2] == "tag")
            if got != sorted(expected):
                failures += 1
                print("selftest tag case %d: expected %s, got %s" % (index, sorted(expected), got))
        # a game folder: its src/ alone, then with a "folder" line that adds its engine/, then one that names no
        # folder, then with a shared engine (engines/shared beside games/) added by an "engine" line
        game = os.path.join(folder, "games", "game")
        for name, source in ((os.path.join("games", "game", "src", "a.cpp"), "// 0x401000\nvoid F()\n{\n}\n"),
                             (os.path.join("games", "game", "engine", "b.cpp"), "int mUnknown;\n"),
                             (os.path.join("engines", "shared", "c.cpp"), "int mUnknown;\n")):
            os.makedirs(os.path.dirname(os.path.join(folder, name)), exist_ok=True)
            with open(os.path.join(folder, name), "w", encoding="utf-8", newline="\n") as f:
                f.write(source)
        folder_cases = [("", [], 1), ("folder engine -- the engine is the game's too\n", ["engine/b.cpp unknown mUnknown"], 2),
                        ("folder nowhere -- no such folder\n", "error", 0),
                        ("engine shared -- the engine both games share\n", ["engines/shared/c.cpp unknown mUnknown"], 2),
                        ("engine nowhere -- no such engine\n", "error", 0)]
        for rules, expected, files in folder_cases:
            with open(os.path.join(game, "namecheck.txt"), "w", encoding="utf-8", newline="\n") as f:
                f.write(rules)
            try:
                findings, skipped, count, config = run([game])
                got = ["%s %s %s" % (f[0], f[2], f[3]) for f in findings]
            except ValueError:
                got, count = "error", 0
            if got != expected or count != files:
                failures += 1
                print("selftest folder case %r: expected %s in %d files, got %s in %d" % (rules, expected, files, got, count))
    total = len(SELFTEST) + len(TAG_SELFTEST) + len(folder_cases)
    print("namecheck selftest: %d of %d cases %s" % (total - failures, total,
                                                       "pass" if not failures else "pass, %d fail" % failures))
    return 1 if failures else 0


def main():
    if sys.argv[1:] == ["--selftest"]:
        return selftest()
    ap = argparse.ArgumentParser(description="Find unnamed and unknown identifiers (docs/CODE_STANDARDS.md).")
    ap.add_argument("paths", nargs="+", help="a game folder (games/<game>: its src/, the folders its namecheck.txt adds, and that file) or files and folders")
    ap.add_argument("--config", help="the rules file (default: <game folder>/namecheck.txt)")
    ap.add_argument("--list", action="store_true", help="also print the findings the config allows or exempts")
    ap.add_argument("--checks", help="comma-separated checks to run (default: all)")
    args = ap.parse_args()

    wanted = set(args.checks.split(",")) if args.checks else None
    try:
        findings, skipped, count, config = run(args.paths, args.config, wanted)
    except ValueError as e:
        print("namecheck: " + str(e))
        return 2
    for rel, number, check, name, why in findings:
        print("%s:%d: [%s] %s: %s" % (rel, number, check, name, why))
    if args.list:
        for rel, number, check, name, why in skipped:
            print("%s:%d: [%s] %s: allowed by the config" % (rel, number, check, name))
    tally = {}
    for f in findings:
        tally[f[2]] = tally.get(f[2], 0) + 1
    summary = ", ".join("%s %d" % kv for kv in sorted(tally.items())) or "none"
    print("namecheck: %d findings in %d files (%s); %d allowed by %s" %
          (len(findings), count, summary, len(skipped), config if config and os.path.exists(config) else "no config"))
    return 1 if findings else 0


def run(paths, config=None, wanted=None):
    """Check the paths: the findings, the findings the config allows, the number of files and the config's path.
    A game folder (one with a src/ in it) stands for its src/ and for the folders its config's "folder" lines name."""
    base = None
    targets = []
    for p in paths:
        if os.path.isdir(os.path.join(p, "src")):
            base = base or p
            targets.append(os.path.join(p, "src"))
        else:
            targets.append(p)
    config = config or (os.path.join(base, "namecheck.txt") if base else None)
    cfg = Config.load(config)
    if base:
        for name in cfg.folders:
            folder = os.path.join(base, name)
            if not os.path.isdir(folder):
                raise ValueError("%s: folder %s is not in %s" % (config, name, base))
            if folder not in targets:
                targets.append(folder)
    shared = {}
    if base:
        repo = os.path.dirname(os.path.dirname(os.path.abspath(base)))
        games_folder = os.path.join(repo, "games")
        cfg.games = set(os.listdir(games_folder)) if os.path.isdir(games_folder) else None
        for name in cfg.engines:
            folder = os.path.join(repo, "engines", name)
            if not os.path.isdir(folder):
                raise ValueError("%s: engine %s is not in %s" % (config, name, os.path.join(repo, "engines")))
            shared[os.path.abspath(folder)] = "engines/" + name
            targets.append(folder)
    root = base or os.path.commonpath([os.path.abspath(t) for t in targets])

    findings, skipped = [], []
    count = 0
    cfg.declared = set()
    cfg.namespaces = set()
    for path in walk(targets):
        if path.endswith((".h", ".hpp")):
            cfg.declared |= declared_addresses(path)
        try:
            cfg.namespaces |= set(re.findall(r"\bnamespace\s+(\w+)", open(path, encoding="utf-8", errors="replace").read()))
        except OSError:
            pass
    for path in walk(targets):
        rel = os.path.relpath(path, root).replace("\\", "/")
        if base and rel.startswith("src/"):
            rel = rel[4:]
        for folder, name in shared.items():
            if os.path.abspath(path).startswith(folder + os.sep):
                rel = name + "/" + os.path.relpath(os.path.abspath(path), folder).replace("\\", "/")
        check_file(path, rel, cfg, findings, skipped)
        count += 1
    if wanted:
        findings = [f for f in findings if f[2] in wanted]
        skipped = [f for f in skipped if f[2] in wanted]
    return findings, skipped, count, config


if __name__ == "__main__":
    sys.exit(main())
