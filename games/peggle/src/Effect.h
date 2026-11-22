// Effect.h: one animation of the board that is not a piece of the level (a burst of stars, a ring of light, the master's
// cheer, a ball's fade): the original's Sexy::Effect, kept by value in the lists of the EffectMgr. Only the shell is here
// (the destructor); the fields, the updating and the drawing come with the effects' own translation.

#ifndef __EFFECT_H__
#define __EFFECT_H__

namespace Sexy
{

class Effect
{
public:
	Effect() {}
	virtual ~Effect() {} // 0x439bd0: gives back what the effect holds (two owned objects, three shared ones)
};

} // namespace Sexy

#endif
