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

// 0x469e40
void FloatingTextMgr::Update()
{
	NOT_TRANSLATED("FloatingTextMgr::Update");
}
