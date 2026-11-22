// EffectMgr.cpp: see EffectMgr.h.

#include "EffectMgr.h"

using namespace Sexy;

// 0x4471e0
EffectMgr::EffectMgr()
{
}

// 0x443720
void EffectMgr::Clear()
{
	for (int i = 0; i < NUM_EFFECT_LISTS; i++)
		mLists[i].clear();
}
