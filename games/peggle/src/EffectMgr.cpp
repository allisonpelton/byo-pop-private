// EffectMgr.cpp: see EffectMgr.h.

#include "NotTranslated.h"
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

// 0x45bf10
void EffectMgr::Update(bool theSlowMo)
{
	NOT_TRANSLATED("EffectMgr::Update");
}

// 0x45bfb0
void EffectMgr::UpdateSlowMo(float theSubFrame)
{
	NOT_TRANSLATED("EffectMgr::UpdateSlowMo");
}

// 0x443780
int EffectMgr::EraseAllOfType(int theType)
{
	for (int aList = 0; aList < NUM_EFFECT_LISTS; aList++)
		if (!mLists[aList].empty())
			NOT_TRANSLATED("EffectMgr::EraseAllOfType with effects (Effect has no fields yet)");
	return 0;
}
