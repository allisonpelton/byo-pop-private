#ifndef __MAHJONGJAPAN_WIDGETS_H__
#define __MAHJONGJAPAN_WIDGETS_H__

// The engine's widgets (0x426910..0x428dc0): the edit field holder (0x426910), the list box (0x426ac0) and the text
// edit (0x427920, vtable 0x51e350). See re/mahjongjapan/NOTES.md for the functions read so far.
//
// Not translated yet: only the layer's focus call is declared here, so the engine builds (see Widgets.cpp).

#include "Object.h"

namespace jb
{

// 0x4287a0: the text edit's focus on or off (the layers' focus lists hold text edits only)
void SetObjectFocus(Object* theObject, bool theFocus);

} // namespace jb

#endif
