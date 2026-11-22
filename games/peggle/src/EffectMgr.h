// EffectMgr.h: the board's effects (the original's Sexy::EffectMgr), in five lists, one for each draw layer. The making, the
// emptying and the update are translated; the update of the kinds that have their own is not.

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

	Effect& AddEffect(int theType, float theX, float theY, int thePriority, bool theAtFront = false); // 0x44de40: a new effect in the list of that priority
	Effect* GetEffectByType(int theType); // 0x441650: the first one of that type, in the lists' order, or none
	void Clear(); // 0x443720: every effect of every list is deleted
	int EraseAllOfType(int theType);	// 0x443780: how many it erased
	void Update(bool theSlowMo);		// 0x45bf10
	void UpdateSlowMo(float theSubFrame);		// 0x45bfb0

	std::list<Effect> mLists[NUM_EFFECT_LISTS]; // +4, 12 bytes each in the original
};

// 0x44efe0: the burst of sparks at the cannon's mouth when a ball is shot, thrown out along theAngle (does nothing with no board up)
void AddShotBurst(float theX, float theY, float theAngle);

} // namespace Sexy

#endif
