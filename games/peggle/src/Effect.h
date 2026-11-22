// Effect.h: one animation of the board that is not a piece of the level (a burst of stars, a ring of light, the master's
// cheer, a ball's fade): the original's Sexy::Effect, kept by value in the lists of the EffectMgr. The fields and the update
// are here; the kinds' own updates and the drawing come with the effects' own translation (the fields they use keep the
// interim names mUnknownNN, the offset in the original, until then).

#ifndef __EFFECT_H__
#define __EFFECT_H__

namespace Sexy
{

class Effect
{
public:
	Effect();			   // 0x43e540 (with 0x432e00): every field cleared
	virtual ~Effect() {} // 0x439bd0: gives back what the effect holds (two owned objects, three shared ones)

	bool Update(bool theSlowMo); // 0x45b650: false when the effect is over and goes
	bool UpdateSlowMo(float theSubFrame); // 0x45b870

	int mLayer;		  // +0x04: which of the manager's lists it is in (the priority, -1 to 3)
	int mType;		  // +0x08: which kind of effect (6, 0x28 and 0x29 are the cannon's burst)
	int mAge;		  // +0x0c: the updates it has had (a negative age is a delay)
	int mLife;		  // +0x10: the updates it lasts, 0 for ever
	float mX;		  // +0x14
	float mY;		  // +0x18
	float mVelocityX; // +0x1c
	float mVelocityY; // +0x20
	float mGravity;	  // +0x24: when above 0 the vertical speed grows by it each update, up to 10
	float mUnknown28; // +0x28: an angle, in radians
	int mUnknown2C;	  // +0x2c
	int mUnknown30;	  // +0x30
	int mColor;		  // +0x34
	int mUnknown38;	  // +0x38
	int mUnknown3C;	  // +0x3c
	bool mUnknown40;  // +0x40: set, the effect does not move in the slow-motion updates
	void* mUnknown44; // +0x44: an owned object (none yet)
	void* mUnknown48; // +0x48: an owned object (none yet)
	void* mUnknown4C; // +0x4c: a shared object (none yet)
	void* mUnknown50; // +0x50
	void* mUnknown54; // +0x54
};

} // namespace Sexy

#endif
