#!/usr/bin/env python3
"""Prints a first C++ draft of functions of the Ghidra export: the virtual calls become named method calls, float literals are
decoded, and the base object's fields get their names. The draft is meant to be edited, not compiled as it stands.

    python games/venice/tools/cpp.py 41d600 [41de50 ...]
    python games/venice/tools/cpp.py --fields 0x148=mKind,0x14c=mFilled 41c5a0

Everything the draft cannot name stays as the export wrote it (FUN_, DAT_, local_ names), so what is left to do is visible.
"""
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import decread

SLOTS = {
    0x04: "AddStage", 0x0c: "GoToStage", 0x10: "GoToStageByName", 0x14: "Load", 0x24: "SetAttribute", 0x2c: "AddFilter",
    0x34: "AddEvent", 0x40: "NewChild", 0x44: "AddChild", 0x48: "MakeSparkles", 0x4c: "MakeParticles", 0x58: "OnObjectClosed",
    0x60: "Move", 0x6c: "Update", 0x74: "Draw", 0x78: "DrawSelf", 0x8c: "Kill", 0x90: "PopEffect", 0x94: "OnJewelHit",
    0x98: "SetUp", 0x9c: "SpinBoost", 0xa0: "PositionSlot",
}
ATTRIBUTES = ["AttributeXScale", "AttributeYScale", "AttributeX", "AttributeY", "AttributeZ", "AttributeRotation", "AttributeRed",
              "AttributeGreen", "AttributeBlue", "AttributeAlpha", "AttributeFrameNumber", "AttributeAll"]
FILTERS = ["FilterSuperCos", "FilterSquared", "FilterCubed", "FilterSquareRoot", "FilterSined", "FilterCosined", "FilterDecaySined",
           "FilterDecayCosined", "FilterLoopAnim", "FilterPingPongAnim", "FilterHotSpot", "FilterVelocity", "FilterVelocityLooped"]
FIELDS = {
    0x04: "mNext", 0x08: "mPrev", 0x0c: "mParent", 0x10: "mType", 0x14: "mIsDead", 0x2c: "mName", 0x44: "mNoCulling",
    0x4c: "mBlitStyle", 0x68: "mImage", 0xa0: "mWorld[AttributeXScale]", 0xa4: "mWorld[AttributeYScale]",
    0xa8: "mWorld[AttributeX]", 0xac: "mWorld[AttributeY]", 0xb4: "mWorld[AttributeRotation]", 0xd0: "mLocal[AttributeXScale]",
    0xd4: "mLocal[AttributeYScale]", 0xd8: "mLocal[AttributeX]", 0xdc: "mLocal[AttributeY]", 0xe4: "mLocal[AttributeRotation]",
    0xe8: "mLocal[AttributeRed]", 0xec: "mLocal[AttributeGreen]", 0xf0: "mLocal[AttributeBlue]", 0xf4: "mLocal[AttributeAlpha]",
    0x100: "mTickCount", 0x104: "mZOrder", 0x108: "mVisible", 0x12c: "mBilinear", 0x13c: "mFlipX",
}
LEVEL_FIELDS = {0x27c: "mMouseX", 0x280: "mMouseY", 0x28c: "mLeftClick", 0x290: "mRightClick", 0x29c: "mPaused",
                0x3a0: "mComboCount", 0x3a4: "mComboTicksLeft", 0x3ac: "mShotsFired", 0x3b0: "mHits", 0x3b4: "mCombos",
                0x3d0: "mSparkleImage", 0x3d4: "mLevelParticleImage", 0x3d8: "mWhiteParticleImage"}
STAGE_FIELDS = {0x94: "mAttributeMask", 0xb0: "mVisible", 0xb4: "mLoopStart", 0xb8: "mLoopEnd", 0xbc: "mFrame", 0xc0: "mLength",
                0xc8: "mCarriesFilters", 0xd8: "mName"}
GLOBALS = {
    "DAT_005ba2a8": "gShooter", "DAT_005ba2a4": "gActiveLevel", "DAT_005ba2b0": "gMiddleLayer", "DAT_005ba2b4": "gPlateLayer",
    "DAT_005ba29c": "gSparkleLayer", "DAT_005ba2a0": "gTutorial", "DAT_005ba2ac": "gOuterLayer", "gScenes_tail": "gScenes.mTail",
    "DAT_005ba0a4": "gTuning.mEarthquakePoint", "DAT_005ba084": "gTuning.mScrollingSlowPoint",
    "DAT_005ba088": "gTuning.mPushOnShotFired", "DAT_005ba08c": "gTuning.mPushOnFilledSlot", "DAT_005ba090": "gTuning.mPushOnCombo",
    "DAT_005ba0a0": "gTuning.mDifficultyChangeOnLoss", "DAT_005ba0a8": "gTuning.mPushFilledPlate",
    "DAT_005ba0ac": "gTuning.mBlockingSlots", "DAT_005ba0b0": "gTuning.mBounceOffTop", "DAT_005ba0b4": "gTuning.mCascadingCombos",
    "DAT_005ba0b8": "gTuning.mNumberOfUniqueSlots", "DAT_005ba0c4": "gTuning.mPlateMoveWhenFullSlotHit",
    "DAT_005ba0c8": "gTuning.mTossOnFilledSlot", "DAT_005ba0cc": "gTuning.mTossOnFilledPlate", "DAT_005ba0d0": "gTuning.mBumpsToExplode",
    "DAT_005b5148": "gTuning.mReloadTime", "DAT_005b514c": "gTuning.mCupidWaitMs",
}
PROFILE = re.compile(r"\*\(int \*\)\(&DAT_00a1c9a0 \+ DAT_00a29578 \* 0x1278\)")


def field_name(offset, extra):
    if offset in extra:
        return extra[offset]
    return FIELDS.get(offset)


def convert(text, extra):
    # a call whose arguments the export wrapped onto the next line
    text = re.sub(r"\)\)\n\s+\(", "))(", text)
    # the virtual calls
    def call(match):
        this = match.group(1)
        slot = int(match.group(2), 16)
        name = SLOTS.get(slot, "Slot%02x" % (slot // 4))
        return "%s->%s(" % (this, name)
    text = re.sub(r"\(\*\*\(fn\*\*\)\(\*\*\(int \*\*\)\(([^()]*(?:\([^()]*\))?[^()]*)\) \+ (0x[0-9a-f]+|\d+)\)\)\(",
                  lambda m: "%s->%s(" % (m.group(1).replace("(int)", ""), SLOTS.get(int(m.group(2), 0), "Slot%02x" % (int(m.group(2), 0) // 4))), text)
    text = re.sub(r"\(\*\*\(fn\*\*\)\(\*\*\(int \*\*\)([A-Za-z_]\w*) \+ (0x[0-9a-f]+|\d+)\)\)\(",
                  lambda m: "%s->%s(" % (m.group(1), SLOTS.get(int(m.group(2), 0), "Slot%02x" % (int(m.group(2), 0) // 4))), text)
    text = re.sub(r"\(\*\*\(fn\*\*\)\(\*\*\(int \*\*\)\(([^()]+)\) \+ (0x[0-9a-f]+|\d+)\)\)\(",
                  lambda m: "%s->%s(" % (m.group(1), SLOTS.get(int(m.group(2), 0), "Slot%02x" % (int(m.group(2), 0) // 4))), text)
    text = re.sub(r"\(\*\*\(fn\*\*\)\(\*(\(int\))?([A-Za-z_]\w*) \+ (0x[0-9a-f]+|\d+)\)\)\(",
                  lambda m: "%s->%s(" % (m.group(2), SLOTS.get(int(m.group(3), 0), "Slot%02x" % (int(m.group(3), 0) // 4))), text)
    text = re.sub(r"\(\*\*\(fn\*\*\)\(\*\*(\w+) \+ (0x[0-9a-f]+|\d+)\)\)\(",
                  lambda m: "%s->%s(" % (m.group(1), SLOTS.get(int(m.group(2), 0), "Slot%02x" % (int(m.group(2), 0) // 4))), text)

    # attribute and filter numbers in the calls that take them
    def attr_call(match):
        number = int(match.group(2))
        return "%s(%s" % (match.group(1), ATTRIBUTES[number] if number < len(ATTRIBUTES) else match.group(2))
    text = re.sub(r"(->SetAttribute)\((\d+)", attr_call, text)

    def filter_call(match):
        ftype = int(match.group(2), 0)
        attr = int(match.group(3), 0)
        return "%s(%s, %s" % (match.group(1), FILTERS[ftype] if ftype < len(FILTERS) else match.group(2),
                               ATTRIBUTES[attr] if attr < len(ATTRIBUTES) else match.group(3))
    text = re.sub(r"(->AddFilter)\((0x[0-9a-f]+|\d+),(0x[0-9a-f]+|\d+)", filter_call, text)

    # fields: *(float *)((int)X + 0xNN), *(int *)(X + 0xNN), X[0xNN]
    def field(match):
        base = match.group(2).replace("(int)", "").strip()
        offset = int(match.group(3), 16)
        name = field_name(offset, extra)
        if name is None:
            return match.group(0)
        if base in ("gActiveLevel", "(int)gActiveLevel"):
            name = LEVEL_FIELDS.get(offset, name)
        return "%s->%s" % (base, name)
    text = re.sub(r"\*\((float|int|u32|uint|void \*|char|u8) ?\*\)\(((?:\(int\))?[\w.>\[\]-]+) \+ 0x([0-9a-f]+)\)", field, text)
    text = re.sub(r"\*\((float|int|u32|uint) ?\*\)\(\*\(int \*\)\(((?:\(int\))?[\w.>\[\]-]+) \+ 0x([0-9a-f]+)\) \+ 0x([0-9a-f]+)\)",
                  lambda m: "%s->%s->%s" % (m.group(2).replace("(int)", ""), field_name(int(m.group(3), 16), extra) or "?",
                                           STAGE_FIELDS.get(int(m.group(4), 16), "+0x" + m.group(4))) if int(m.group(3), 16) in (0x50, 0x54, 0x58) else m.group(0), text)
    # an index into an object held in an int array: X[0x2a] is the field at 0xa8
    def indexed(match):
        name = field_name(int(match.group(2), 16) * 4, extra)
        return "%s->%s" % (match.group(1), name) if name else match.group(0)
    text = re.sub(r"\(float\)(gActiveLevel|\w+)\[(0x[0-9a-f]+)\]", lambda m: indexed(m), text)
    text = re.sub(r"(\w+)\[(0x[0-9a-f]+)\]", lambda m: indexed(m) if m.group(1) not in ("local_28",) else m.group(0), text)
    text = re.sub(r"0x[0-9a-f]{8}/\*(-?[0-9.e+-]+f)\*/", lambda m: m.group(1), text)
    text = re.sub(r"(?<![\w.])(-?\d+)f\b", lambda m: m.group(1) + ".0f", text)
    for old, new in GLOBALS.items():
        text = text.replace(old, new)
    text = PROFILE.sub("CurrentProfile().mMode", text)
    text = text.replace("SceneObject_FindChildByType(", "FindChildByType(").replace("SceneObject_CountChildrenOfType(", "CountChildrenOfType(")
    text = text.replace("SceneStage_AddFilter(", "AddStageFilter(").replace("SceneStage_FindFilter(", "FindStageFilter(")
    text = text.replace("SceneObject_AdoptNewestChild(", "AdoptNewestChild(").replace("SceneObject_SetImageRecord(", "SetImageRecord(")
    text = text.replace("SceneObject_SetImage(", "SetImage(").replace("SceneObject_Update(", "SceneObject::Update(")
    text = text.replace("FUN_00403440(", "VeniceAbs(").replace("FUN_00403400(", "VeniceCos(").replace("FUN_00403480(", "VeniceSin(")
    text = text.replace("FUN_004034c0(", "VeniceSqrt(").replace("FUN_00567550(", "strcpy(")
    text = text.replace("_strstr(", "strstr(").replace("_strcmp(", "strcmp(").replace("_sprintf(", "sprintf(")
    return text


def main():
    arguments = sys.argv[1:]
    extra = {}
    if arguments and arguments[0] == "--fields":
        for pair in arguments[1].split(","):
            offset, name = pair.split("=")
            extra[int(offset, 16)] = name
        arguments = arguments[2:]
    names = decread.load_symbols()
    functions = decread.load_functions()
    for argument in arguments:
        if "-" in argument:
            low, high = (int(x, 16) for x in argument.split("-"))
            addresses = [a for a in sorted(functions) if low <= a < high]
        else:
            addresses = [int(argument, 16)]
        for address in addresses:
            text = functions.get(address)
            if text is None:
                print("// %08x not in the export" % address)
                continue
            out = decread.compact(text, names)
            lines = [l for l in out.split("\n") if not re.match(r"^\s*(puStack_c = |local_10 = ExceptionList;|ExceptionList = (local_10|&local_10);)", l)]
            print(convert("\n".join(lines), extra))
            print()


if __name__ == "__main__":
    main()
