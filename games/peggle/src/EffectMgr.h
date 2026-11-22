// EffectMgr.h: the board's effects (the original's Sexy::EffectMgr), in five lists, one for each draw layer. Only the making
// and the emptying are translated so far.

#ifndef __EFFECTMGR_H__
#define __EFFECTMGR_H__

#include "Effect.h"

#include <list>

namespace Sexy
{

static const int NUM_EFFECT_LISTS = 5;

class EffectMgr
{
public:
	EffectMgr();		   // 0x4471e0
	virtual ~EffectMgr() {} // 0x44b550

	void Clear(); // 0x443720: every effect of every list is deleted
	int EraseAllOfType(int theType);	// 0x443780: how many it erased
	void Update(bool theSlowMo);		// 0x45bf10
	void UpdateSlowMo(float theSubFrame);		// 0x45bfb0

	std::list<Effect> mLists[NUM_EFFECT_LISTS]; // +4, 12 bytes each in the original
};

} // namespace Sexy

#endif
