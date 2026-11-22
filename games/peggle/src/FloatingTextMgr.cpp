// FloatingTextMgr.cpp: see FloatingTextMgr.h.

#include "NotTranslated.h"
#include "FloatingTextMgr.h"

using namespace Sexy;

// 0x46e6e0
FloatingTextMgr::FloatingTextMgr()
{
	mLastTextId = 0;
}

// 0x469d10 (0x469820)
void FloatingTextMgr::Clear()
{
	mTexts.clear();
}

// 0x469de0: the text of that number goes. A text has no fields yet, so a list that holds one stops here.
void FloatingTextMgr::EraseText(int theTextId)
{
	if (!mTexts.empty())
		NOT_TRANSLATED("FloatingTextMgr::EraseText with a text (FloatingText has no fields yet)");
}

// 0x469e40
void FloatingTextMgr::Update()
{
	NOT_TRANSLATED("FloatingTextMgr::Update");
}
