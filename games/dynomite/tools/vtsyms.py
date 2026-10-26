"""Name the slots of the game's vtables (the game classes have no RTTI) for Ghidra: each game vtable is matched
with the framework vtable whose slots it shares most (Widget, ButtonWidget, Dialog, SexyApp, ButtonListener),
and every slot it overrides gets `<Class>::<slot name>`. Classes not named yet in CLASSES are `C<vtable>`.

    python games/dynomite/tools/vtsyms.py > games/dynomite/docs/symbols/10_vtables.jsonl
"""
import json, sys
sys.argv = [sys.argv[0]]
exec(open('games/dynomite/tools/vtscan.py').read().split("if len(sys.argv) > 1:")[0])

# The 2003 framework's slots (games/dynomite/docs/FRAMEWORK_2003.md)
WIDGET = ['~Widget', 'AddedToManager', 'RemovedFromManager', 'OrderInManagerChanged', 'SetVisible', 'SetColors4',
          'SetColors3', 'SetColor', 'GetColor', 'SetTransient', 'SetDisabled', 'ShowFinger', 'ResizeRect', 'Resize',
          'Move', 'WantsFocus', 'Draw', 'Update', 'GotFocus', 'LostFocus', 'KeyChar', 'KeyDown', 'KeyUp', 'MouseEnter',
          'MouseLeave', 'MouseMove', 'MouseDown', 'MouseUp3', 'MouseUp', 'MouseDrag', 'WriteCenteredLineShadow',
          'WriteCenteredLine', 'WriteString', 'WriteWordWrapped', 'GetWordWrappedHeight', 'GetNumDigits',
          'WriteNumberFromStrip', 'Contains', 'GetInsetRect', 'GetRect', 'Intersects', 'MarkDirtyFull', 'MarkDirty']
BUTTON = WIDGET + ['SetFont', 'IsButtonDown']
DIALOG = WIDGET + ['SetButtonFont', 'SetHeaderFont', 'SetLinesFont', 'GetPreferredHeight', 'IsModal']
LISTENER = ['ButtonPressCC', 'ButtonPress', 'ButtonDepress', 'ButtonDownTick']
APP = LISTENER + ['vf04', 'Shutdown', 'vf06', 'vf07', 'UpdateCursor', 'vf09', 'vf10', 'ShutdownHook', '~SexyApp',
                  'vf13', 'Init', 'InitHook', 'ReadFromRegistry', 'WriteToRegistry', 'vf18', 'vf19', 'vf20', 'vf21',
                  'LostFocus', 'GotFocus', 'OpenURL'] + ['vf%02d' % i for i in range(25, 61)]
BASES = {0x502f44: ('Widget', WIDGET), 0x5031c4: ('ButtonWidget', BUTTON), 0x503294: ('Dialog', DIALOG),
         0x5045f4: ('SexyApp', APP), 0x502f20: ('ButtonListener', LISTENER)}

# Named by the texts each uses and by the link order, which follows the source files' names (the comments of
# properties/default.xml list them: Dynomite.cpp, DynomiteFossil.cpp, ... UpsellDialog.cpp)
CLASSES = {0x4fa410: 'FossilizedApp', 0x4f7470: 'Dynomite', 0x4f9a10: 'DynomiteFossil', 0x4f9ce0: 'DynomitePanic',
           0x4f9e68: 'DynomiteStomped', 0x4fa100: 'DynomiteTimeTrial', 0x4fa298: 'EnterNameDialog',
           0x4fb4d8: 'GetSaveNameDialog', 0x4fb688: 'HighScoreDialog', 0x4fb970: 'IntroScreen',
           0x4fc4c0: 'OptionsDialog', 0x4fc744: 'PuzzleListWidget', 0x4fc890: 'RegisterDialog',
           0x4fca40: 'SaveGameDialog', 0x4fcd88: 'TitleScreen', 0x4ff5e0: 'UpsellDialog'}


def slots(vt, n):
    out = []
    for i in range(n):
        v = read32(vt + 4 * i)
        if not is_code(v) or (i and vt + 4 * i in known):
            break
        out.append(v)
    return out


base_slots = {b: slots(b, len(t[1])) for b, t in BASES.items()}
seen = set()
for start, n in scan():
    if start in known:
        continue
    st = storers(start)
    if not st:
        continue
    mine = slots(start, n)
    best, score = None, 0
    for b, bs in base_slots.items():
        k = sum(1 for i in range(min(len(bs), len(mine))) if bs[i] == mine[i])
        if k > score and len(mine) >= len(bs) - 1:
            best, score = b, k
    cls = CLASSES.get(start, 'C%06x' % (start & 0xffffff))
    slotnames = BASES[best][1] if best else []
    bs = base_slots.get(best, [])
    for i, f in enumerate(mine):
        if f < 0x401000 or f >= 0x442000 or f in seen:
            continue
        if i < len(bs) and bs[i] == f:
            continue
        cur = names.get(f) or funcs.get(f, {}).get('name', '')
        if cur and not cur.startswith(('FUN_', 'thunk_FUN_')) and f in funcs and f not in names:
            pass
        nm = slotnames[i] if i < len(slotnames) else 'vf%02d' % i
        seen.add(f)
        print(json.dumps({'addr': '%08x' % f, 'name': '%s::%s' % (cls, nm),
                          'comment': 'vtable %08x slot %d' % (start, i)}))
