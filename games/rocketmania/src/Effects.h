// The effects of Rocket Mania Deluxe: the fireworks, sparkles, flames and falling leaves. In the original they are class
// "Rocket" (0x4172f0, 0x128 bytes) with a manager (0x41a830) and a pool of particles (0x41ac40). See docs/GAME_NOTES.md.

#ifndef __ROCKETMANIA_EFFECTS_H__
#define __ROCKETMANIA_EFFECTS_H__

#include "Color.h"
#include <list>
#include <vector>

namespace Sexy
{

class RocketApp;
class Graphics;
class Image;
struct Effect;
class EffectManager;

// 0x50 bytes: one spark, flame or leaf of an effect.
struct Particle
{
	int			mSerial;        // +0x00 how many particles the effect had when this one was made (odd ones are drawn
	                            //       differently by kind 6)
	double		mX, mY, mZ;     // +0x08
	double		mVX, mVY, mVZ;  // +0x20
	int			mAge;           // +0x38 updates left; below zero it goes
	int			mLife;          // +0x3c the updates it started with
	int			mColor;         // +0x40 which pair of the effect's colours it blends between
	Particle*	mNext;          // +0x44 (in the pool: the next free one)
	Particle*	mPrev;          // +0x48
	Particle() : mSerial(0), mX(0), mY(0), mZ(0), mVX(0), mVY(0), mVZ(0), mAge(0), mLife(0), mColor(0), mNext(NULL),
		mPrev(NULL) {}
};

// 0x41ac40: a fixed number of particles that all the effects of a manager draw from (1000); an effect makes no more when
// the pool is empty.
class ParticlePool
{
public:
	std::vector<Particle>	mParticles;
	Particle*				mFree;      // +0x08

	ParticlePool(int theCount);

	bool					HasFree() const { return mFree != NULL; }   // 0x41ad40
	Particle*				Take();                                     // 0x41ad50
	void					Give(Particle* theParticle);                // 0x41ad70
};

// The effect of one kind; kinds 0 to 0x16 are the ones the game makes (the table in EffectKinds.inc is what the original
// constructor stores for each). The board changes the fields it needs after Add: where (mX, mY), how long it makes
// particles (mLife), the delay before it starts (mc0) and a few of the particles' settings.
struct Effect
{
	RocketApp*		mApp;               // +0x00
	EffectManager*	mManager;
	int				mKind;              // +0x04
	bool			mNotifyBoard;       // +0x08 the board is told (0x428d10) when the effect goes
	bool			mSounds;            // +0x09 plays a firework sound when the delay is over
	Color			mColors[12];        // +0x0c six colours the particles start with, six they end with (each +6)
	int				mColorChoice;       // +0x3c the pair of colours that the particles take (3 to 5), random
	int				m40;                // +0x40 the sound that plays when the delay is over (-1 none)
	bool			mFlag;              // +0x44 the manager deletes it when it is finished
	double			mX, mY, mZ;         // +0x48, +0x50, +0x58 where the particles start
	double			mSpreadX, mSpreadY, mSpreadZ;   // +0x60, +0x68, +0x70
	double			m78, m80, m88;      // +0x78 the effect moves by this each update
	bool			m90;                // +0x90 the layer it is drawn in
	Particle*		mParticles;         // +0x94 the list, the newest first
	ParticlePool*	mPool;              // +0x98
	double			ma0, ma8, mb0;      // +0xa0 added to the particles' speed every update
	int				mMaxParticles;      // +0xb8
	int				mParticleCount;     // +0xbc
	int				mc0;                // +0xc0 updates to wait before it starts
	int				mLife;              // +0xc4 updates it makes particles for (0 stops, below zero never)
	double			mDirX, md0, mDirZ;  // +0xc8 the direction the particles fly (made a unit vector)
	double			mDirSpreadX, mDirSpreadY, mDirSpreadZ;      // +0xe0, +0xe8, +0xf0
	double			mf8;                // +0xf8 their speed
	double			mSpeedSpread;       // +0x100
	double			m108;               // +0x108 how much speed is kept each update
	int				m110, m114;         // +0x110 how long a particle lives, and the spread of that
	int				m118, m11c, m120;   // +0x118 how many are made each time, the spread, and (+0x120) the updates between

	Effect(RocketApp* theApp, EffectManager* theManager, int theKind, bool theFlag);   // 0x4172f0
	~Effect();                                                                          // 0x418fc0

	bool			Update();                                       // 0x419340: false when it has nothing left to do
	void			Draw(Graphics* g, bool theLayer);               // 0x4194d0

private:
	void			Spawn(int theCount);                            // 0x419020
	void			UpdateParticle(Particle* theParticle);          // 0x4192d0
	void			RemoveParticle(Particle* theParticle);          // 0x419280
	void			SetBlendColor(Graphics* g, Particle* theParticle, double theFraction);   // 0x419450
	void			DrawFrames(Graphics* g, Image* theImage, int theFrames, bool theBlend);
};

// 0x41a830: the effects of the board (or of the menu) and the particles they share.
class EffectManager
{
public:
	RocketApp*			mApp;
	std::list<Effect*>	mEffects;       // +0x08
	ParticlePool*		mPool;          // +0x10
	bool				mClosing;       // the manager is being deleted (the effects do not tell the board then)

	EffectManager(RocketApp* theApp);
	~EffectManager();                                           // 0x41a8c0

	Effect*				Add(int theKind, bool theFlag);         // 0x41a9c0
	void				Remove(Effect* theEffect);              // 0x41aa80
	bool				Has(Effect* theEffect);                 // 0x41ab40
	void				Update();                               // 0x41ab70
	void				Draw(Graphics* g, bool theLayer);       // 0x41abb0
	void				Clear();                                // 0x41aae0: all of them go
};

}

#endif
