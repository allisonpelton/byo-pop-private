"""Rewrite the accesses to virtual-base sub-objects in Ghidra text (used by decread.py).

An entity class (Shark, Piranha, Torpedo...) has the virtual bases GameObject and Enemy. The compiler reaches a field of
a base through the vbptr: Ghidra writes

    *(int *)(this + *(int *)(*(int *)(this + 8) + 4) + 0xe4)             constructor style, this = the complete object
    *(int *)(*(int *)(*(int *)(param_1 + -0x28) + 4) + 0xa8 + param_1)   method style, param_1 = a sub-object

`*(int *)(P + A)` is the vbptr (A is its offset from P), entry 4*K of its table is the distance from the vbptr to the
K-th virtual base, so the address is `P + A + vbtable[K] + (X - A)` = the start of the base part plus `X - A`: the
field at offset `X - A` inside the base part (K = 1: GameObject, written `G`; K = 2: Enemy, `E`; the order of Shark and
Piranha, ENTITIES.md says how the other classes differ). This rewrites those to `G.e4{c}` / `E.58`, and a sub-object
pointer to `&G.0`-style addresses. The same is done, statement by statement in order, for a local that holds the vbptr
(`iVar6 = *(int *)(param_1 + -0x28)`), a vbtable entry (`iVar2 = *(int *)(*(int *)(this + 8) + 4)`) or a sub-object
address (`iVar8 = param_1 + *(int *)(iVar6 + 4)`); a branch that reassigns the local can still fool it.
Fields of the class's own part stay as they are (`this.10`, or `param_1 + -0xc` in method style: add the distance from
the part to the sub-object `param_1` points to).
"""
import re

NAMES = {1: "G", 2: "E"}
SUFFIX = {"char": "c", "uchar": "c", "byte": "c", "bool": "c", "undefined1": "c", "short": "s", "ushort": "s",
          "undefined2": "s", "float": "f", "double": "d", "float10": "d"}
TYPE = r"(?:[A-Za-z_][\w:]*(?: [\w:]+)*)"
NUM = r"-?(?:0x[0-9a-f]+|\d+)"
TOK = r"@@T\d+@@"
TERM = r"(?:\(int\))?(?:" + TOK + r"|-?\w+)"
CHAIN = re.compile(r"(?<![\w.)\]@])" + TERM + r"(?: \+ " + TERM + r")+(?![\w.(\[@])")


def num(s):
    s = s.strip()
    neg = s.startswith("-")
    if neg:
        s = s[1:]
    v = int(s, 16) if s.startswith("0x") else int(s)
    return -v if neg else v


def hexs(v):
    return ("-" if v < 0 else "") + "%x" % abs(v)


def ref_text(k, off):
    return "%s.%s" % (NAMES.get(k, "V%d" % k), hexs(off))


def join_statements(lines):
    out, cur = [], None
    for ln in lines:
        cur = ln if cur is None else cur + " " + ln.strip()
        if cur.count("(") > cur.count(")"):
            continue
        out.append(cur)
        cur = None
    if cur is not None:
        out.append(cur)
    return out


def vbase(text):
    if "+ -0x" not in text and "+ 8) + 4)" not in text and "+ 4) + 4)" not in text:
        pass
    toks = []
    state0 = {}   # local -> (P, A): holds the vbptr
    statev = {}   # local -> token: holds a vbtable entry
    state2 = {}   # local -> (K, off): holds the address of part K plus off

    def tok(p, a, k, orig):
        toks.append((p, a, k, orig))
        return "@@T%d@@" % (len(toks) - 1)

    def group_ref(inner):
        """Parse a flat sum holding exactly one token; return (K, offset in the part) or None."""
        terms = [x.strip() for x in inner.split(" + ")]
        tk = [x for x in terms if re.fullmatch(TOK, x)]
        if len(tk) != 1:
            return None
        p, a, k, _ = toks[int(tk[0][3:-2])]
        off = 0
        seen_base = False
        for x in terms:
            if x == tk[0]:
                continue
            if x in (p, "(int)" + p):
                if seen_base:
                    return None
                seen_base = True
            elif re.fullmatch(NUM, x):
                off += num(x)
            else:
                return None
        if not seen_base:
            return None
        return (k, off - a)

    def process(ln):
        if "(" not in ln and "[" not in ln and not (statev or state2 or state0):
            return ln
        ln = re.sub(r"\(int\)(\w+)(?= \+ )", r"\1", ln)   # `(int)this + x` is the same sum without the cast
        ln = re.sub(r"(?<= \+ )\(int\)(\w+)(?=[ )])", r"\1", ln)
        # 1. the vbptr read and the vbtable entry
        def vb_direct(m):
            return tok(m.group(1), num(m.group(2)), num(m.group(3)) // 4, m.group(0))
        ln = re.sub(r"\*\(int \*\)\(\*\(int \*\)\((?:\(int\))?(\w+) \+ (" + NUM + r")\) \+ (" + NUM + r")\)", vb_direct, ln)

        def vb_alias(m):
            if m.group(1) in state0:
                p, a = state0[m.group(1)]
                return tok(p, a, num(m.group(2)) // 4, m.group(0))
            return m.group(0)
        if state0:
            ln = re.sub(r"\*\(int \*\)\((\w+) \+ (" + NUM + r")\)", vb_alias, ln)
        # 2. locals that hold a vbtable entry, as a term of a sum
        if statev:
            def with_v(m):
                terms = [x.strip() for x in m.group(1).split(" + ")]
                if any(x in statev for x in terms) and "@@T" not in m.group(1):
                    return "(" + " + ".join(statev.get(x, x) for x in terms) + ")"
                return m.group(0)
            ln = re.sub(r"\(([^()]* \+ [^()]*)\)", with_v, ln)
            m = re.match(r"^(\s*\w+ = )([^;(]* \+ [^;(]*)(;.*)$", ln)
            if m and "@@T" not in m.group(2):
                terms = [x.strip() for x in m.group(2).split(" + ")]
                if any(x in statev for x in terms):
                    ln = m.group(1) + " + ".join(statev.get(x, x) for x in terms) + m.group(3)
        if "@@T" in ln:
            # 3. this[expr] is a byte at this + expr
            ln = re.sub(r"\b(\w+)\[([^\[\]]*" + TOK + r"[^\[\]]*)\]", lambda m: "*(byte *)(%s + %s)" % (m.group(1), m.group(2)), ln)
            # 4. parenthesised sums, then bare sums (call arguments, assignments)
            changed = True
            while changed:
                changed = False
                for m in re.finditer(r"(?<!\w)\(([^()]*" + TOK + r"[^()]*)\)", ln):
                    r = group_ref(m.group(1))
                    if r is not None:
                        ln = ln[:m.start()] + "@@R%d,%d@@" % r + ln[m.end():]
                        changed = True
                        break

            def bare(m):
                r = group_ref(m.group(0))
                return "@@R%d,%d@@" % r if r is not None else m.group(0)
            ln = CHAIN.sub(bare, ln)
        # 5. locals that hold the address of a part
        if state2:
            def deref_alias(m):
                v = m.group(2)
                if v not in state2:
                    return m.group(0)
                k, base = state2[v]
                off = base + (num(m.group(3)) if m.group(3) else 0)
                suf = SUFFIX.get(m.group(1).strip(), "")
                return ref_text(k, off) + ("{" + suf + "}" if suf else "")
            ln = re.sub(r"\*\((" + TYPE + r") \*+\)\((\w+) \+ (" + NUM + r")\)", deref_alias, ln)
            ln = re.sub(r"\*\((" + TYPE + r") \*+\)\((\w+)\)()", deref_alias, ln)
            ln = re.sub(r"\*\((" + TYPE + r") \*+\)(\w+)\b()", deref_alias, ln)

            def addr_alias(m):
                v = m.group(1)
                if v not in state2:
                    return m.group(0)
                k, base = state2[v]
                return "&" + ref_text(k, base + num(m.group(2)))
            ln = re.sub(r"\((?:" + TYPE + r") \*+\)\((\w+) \+ (" + NUM + r")\)", addr_alias, ln)

        # 6. the converted references
        def deref(m):
            suf = SUFFIX.get(m.group(1).strip(), "")
            return ref_text(int(m.group(2)), int(m.group(3))) + ("{" + suf + "}" if suf else "")
        ln = re.sub(r"\*\((" + TYPE + r") \*+\)@@R(-?\d+),(-?\d+)@@", deref, ln)
        ln = re.sub(r"@@R(-?\d+),(-?\d+)@@", lambda m: "&" + ref_text(int(m.group(1)), int(m.group(2))), ln)

        # 7. what this statement does to the locals
        m = re.match(r"^\s*(\w+) = (.*);$", ln)
        if m:
            v, rhs = m.group(1), m.group(2)
            state0.pop(v, None)
            statev.pop(v, None)
            state2.pop(v, None)
            mm = re.fullmatch(r"\*\(int \*\)\((?:\(int\))?(\w+) \+ (" + NUM + r")\)", rhs)
            if mm:
                state0[v] = (mm.group(1), num(mm.group(2)))
            elif re.fullmatch(TOK, rhs):
                statev[v] = rhs
            else:
                mm = re.fullmatch(r"&(G|E|V\d+)\.(-?[0-9a-f]+)", rhs)
                if mm:
                    k = {"G": 1, "E": 2}.get(mm.group(1)) or int(mm.group(1)[1:])
                    state2[v] = (k, num(("-0x" + mm.group(2)[1:]) if mm.group(2).startswith("-") else "0x" + mm.group(2)))
        return ln

    out = []
    stack = []
    for ln in join_statements(text.split("\n")):
        st = ln.strip()
        if st.startswith("}") and stack:
            for d, saved in zip((state0, statev, state2), stack.pop()):
                d.clear()
                d.update(saved)
        out.append(process(ln))
        if st.endswith("{"):
            stack.append((dict(state0), dict(statev), dict(state2)))
    t = "\n".join(out)
    # tokens left over (a sum this could not read) go back to the original text
    return re.sub(r"@@T(\d+)@@", lambda m: toks[int(m.group(1))][3], t)
