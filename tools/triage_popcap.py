#!/usr/bin/env python3
"""Triage installed PopCap games for decompilation: find each game's executables, unwrap the PopCap DRM
wrapper, and report what the Bejeweled pipeline needs to know (framework, RTTI, packing, data files).

    python tools/triage_popcap.py [--json out.json] <game folder> [<game folder> ...]

Read-only: nothing in the install folders is modified.
"""
import argparse
import datetime
import json
import os
import re
import struct
import sys

MARKER = b"!popcapdrmprotect!"
PAK_MAGIC = bytes(b ^ 0xF7 for b in b"\xc0\x4a\xc0\xba")  # PopCap pak header 0xBAC04AC0 XOR 0xF7


def pe_info(data, base=0):
    """Basic PE header facts for the image starting at `base`, or None."""
    try:
        pe = base + struct.unpack_from("<I", data, base + 0x3C)[0]
        if data[pe:pe + 4] != b"PE\0\0":
            return None
        machine, nsec, ts = struct.unpack_from("<HHI", data, pe + 4)
        opt_size = struct.unpack_from("<H", data, pe + 20)[0]
        sections = []
        end = 0
        for i in range(nsec):
            sh = pe + 24 + opt_size + i * 40
            name = data[sh:sh + 8].rstrip(b"\0").decode("latin-1")
            vsize, va, rsize, rptr = struct.unpack_from("<IIII", data, sh + 8)
            sections.append((name, vsize, rsize))
            end = max(end, rptr + rsize)
        rich = data.find(b"Rich", base, pe)
        return {
            "machine": hex(machine),
            "timestamp": datetime.datetime.fromtimestamp(ts, datetime.timezone.utc).strftime("%Y-%m-%d"),
            "sections": [s[0] for s in sections],
            "text_size": next((s[1] for s in sections if s[0] in (".text", "CODE")), None),
            "image_end": end,
            "rich_header": rich >= 0,
        }
    except struct.error:
        return None


def rtti_classes(blob):
    names = set(m.decode("latin-1") for m in re.findall(rb"\.\?A[VU]([A-Za-z_][\w@?$<>]*?)@@", blob))
    sexy = sorted(n for n in names if n.endswith("@Sexy") or n.startswith("Sexy@"))
    other = sorted(n for n in names if not (n.endswith("@Sexy") or "std@" in n or n.startswith("?$")) and "@std" not in n)
    return sexy, other


def triage_exe(path):
    data = open(path, "rb").read()
    info = {"file": os.path.basename(path), "size": len(data)}
    outer = pe_info(data)
    if not outer:
        info["note"] = "not a PE"
        return info
    info["outer"] = outer
    blob = data
    m = data.find(MARKER)
    if m >= 0:
        inner_start = data.find(b"MZ", m)
        inner = pe_info(data, inner_start) if inner_start >= 0 else None
        info["drm_wrapper"] = {"marker_offset": hex(m), "inner_offset": hex(inner_start) if inner else None}
        if inner:
            info["inner"] = inner
            blob = data[inner_start:inner_start + inner["image_end"]]
    # protections / packers worth flagging
    secs = set((info.get("inner") or outer)["sections"])
    flags = []
    if ".bind" in secs:
        flags.append("SteamStub .bind section")
    if any(s.startswith("UPX") for s in secs):
        flags.append("UPX")
    if b"Armadillo" in blob or b"ArmAccess" in blob:
        flags.append("Armadillo?")
    if b"SteamStartup" in blob or b"steam_api" in blob:
        flags.append("Steam API")
    info["flags"] = flags
    sexy, other = rtti_classes(blob)
    info["sexy_classes"] = len(sexy)
    info["game_classes"] = [c for c in other if not c.startswith(("type_info", "bad_", "_com_error", "exception"))][:80]
    info["has_lua"] = b"LuaApp" in blob or b"lua_" in blob
    info["has_todlib"] = b"TodParticle" in blob or b"TodFoley" in blob or b"Reanimat" in blob
    info["has_flash"] = b"FlashWidget" in blob
    info["wide_strings"] = b"PB_W" in blob
    return info


def triage_folder(folder):
    res = {"folder": folder, "exes": [], "data": {}}
    if not os.path.isdir(folder):
        res["error"] = "missing"
        return res
    names = os.listdir(folder)
    for n in sorted(names):
        p = os.path.join(folder, n)
        if n.lower().endswith(".exe") and os.path.isfile(p) and not re.match(r"(?i)(unins|popuninst|uninstall|.*setup|dxsetup|vcredist|.*updater|crashreport)", n):
            res["exes"].append(triage_exe(p))
    for n in names:
        p = os.path.join(folder, n)
        if n.lower().endswith(".pak") and os.path.isfile(p):
            with open(p, "rb") as f:
                head = f.read(4)
            res["data"].setdefault("paks", []).append({"file": n, "size": os.path.getsize(p),
                                                      "popcap_xor_f7": head == PAK_MAGIC, "head": head.hex()})
    counts = {}
    for root, _, files in os.walk(folder):
        if root.count(os.sep) - folder.count(os.sep) > 3:
            continue
        for f in files:
            ext = os.path.splitext(f)[1].lower()
            counts[ext] = counts.get(ext, 0) + 1
    res["data"]["extensions"] = dict(sorted(counts.items(), key=lambda kv: -kv[1])[:20])
    for marker in ("properties/resources.xml", "properties/default.xml", "resources.xml"):
        if os.path.isfile(os.path.join(folder, *marker.split("/"))):
            res["data"].setdefault("manifests", []).append(marker)
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("folders", nargs="+")
    ap.add_argument("--json")
    args = ap.parse_args()
    results = [triage_folder(f) for f in args.folders]
    for r in results:
        print("==", r["folder"], r.get("error", ""))
        print("   data:", r["data"])
        for e in r["exes"]:
            o, i = e.get("outer", {}), e.get("inner")
            print(f"   {e['file']:28s} {e['size']:>9} outer {o.get('timestamp')} text={o.get('text_size')}"
                  + (f" | DRM wrapper -> inner {i['timestamp']} text={i['text_size']}" if i else "")
                  + f" | sexy={e.get('sexy_classes')} game={len(e.get('game_classes', []))} flags={e.get('flags')}"
                  + (" lua" if e.get("has_lua") else "") + (" todlib" if e.get("has_todlib") else ""))
            if e.get("game_classes"):
                print("      classes:", ", ".join(e["game_classes"][:40]))
    if args.json:
        json.dump(results, open(args.json, "w"), indent=1)


if __name__ == "__main__":
    sys.exit(main())
