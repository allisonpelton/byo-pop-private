// FloatingTextMgr.h: the texts that float up from the board (the points of a peg, "Extreme Fever", ...): the original's
// Sexy::FloatingTextMgr, with its texts by value in one list. Only the making and the emptying are translated so far.

#ifndef __FLOATINGTEXTMGR_H__
#define __FLOATINGTEXTMGR_H__

#include <list>

namespace Sexy
{

// One floating text. Only the shell is here: the fields (the text, its place, its colour and its time, and the number the
// manager gave it, +0x60) come with FloatingText's translation.
class FloatingText
{
public:
	FloatingText() {}
	virtual ~FloatingText() {}
};

class FloatingTextMgr
{
public:
	FloatingTextMgr();			  // 0x46e6e0
	virtual ~FloatingTextMgr() {} // 0x471e10

	void Clear(); // 0x469d10: every text is gone
	void Update();		// 0x469e40

	int mLastTextId;				 // +4: the number of the text made last (AddText counts up from 1, skipping 0 when it wraps)
	std::list<FloatingText> mTexts; // +8
};

} // namespace Sexy

#endif
