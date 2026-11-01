#include "Widgets.h"

namespace jb
{

// 0x4287a0: TextEdit::SetFocus. The text edit is not translated yet; until it is, nothing has the focus.
void SetObjectFocus(Object* theObject, bool theFocus)
{
	(void)theObject;
	(void)theFocus;
}

} // namespace jb
