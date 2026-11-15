#ifndef __ASTROPOP_BRICK_H__
#define __ASTROPOP_BRICK_H__

// Brick: the objects of the playfield. The base class is the plain coloured brick (original constructor 0x4067b0,
// sizeof 0x234, vtables 0x55d5a8 for the object and 0x55d500 for its listener part); the four classes made from it are
//   KillAllBrick  0x4073f0, 0x55d618   the coloured brick that clears its colour (IMAGE_BRICK_KILLALL_*)
//   SpecialBrick  0x407480, 0x55d730   radial bomb, stone, steel, row nuke, mutator (type set by SetType)
//   FuelCell      0x407bd0, 0x55d860   the fuel cell
//   PowerBrick    0x407d80, 0x55d988   freeze, column nuke and booster (type set by SetType)
// An object is also a listener (EventBus.h): its events change its state, its flags and which events it hears.
// The members keep the original's offsets in comments (the object's own, not the listener part's, which is +0xc);
// the names that say little (mFlagNNN, mNumNNN) are those whose meaning is not settled yet.

#include "EventBus.h"
#include "FrameAnim.h"
#include "Particle.h"
#include "Color.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include <vector>

namespace Sexy
{

class Board;

// Events (list ids of the bus). Those marked "arg" carry a number.
enum
{
	EVENT_SPAWN = 0,            // 0x402fe0: the object starts afresh, state 0
	EVENT_LAND = 1,             // 0x401920
	EVENT_SETTLE = 2,           // 0x402f20 (and its variants)
	EVENT_DIE = 3,              // 0x403fd0 (and its variants): the object has been hit
	EVENT_STOP = 4,             // 0x4019d0
	EVENT_5 = 5,
	EVENT_6 = 6,
	EVENT_7 = 7,
	EVENT_8 = 8,
	EVENT_BEGIN_FALL = 9,       // 0x401a20
	EVENT_A = 0xa,
	EVENT_B = 0xb,
	EVENT_C = 0xc,
	EVENT_D = 0xd,
	EVENT_E = 0xe,
	EVENT_F = 0xf,
	EVENT_10 = 0x10,
	EVENT_11 = 0x11,
	EVENT_12 = 0x12,
	EVENT_13 = 0x13,
	EVENT_14 = 0x14,
	EVENT_15 = 0x15,
	EVENT_16 = 0x16,
	EVENT_17 = 0x17,
	EVENT_18 = 0x18,
	EVENT_19 = 0x19,
	EVENT_1A = 0x1a,
	EVENT_1B = 0x1b,
	EVENT_1C = 0x1c,
	EVENT_1D = 0x1d,
	EVENT_1E = 0x1e,
	EVENT_1F = 0x1f,            // arg
	EVENT_20 = 0x20,            // arg: 0x401a60
	EVENT_21 = 0x21,            // arg
	EVENT_22 = 0x22,            // arg
	EVENT_23 = 0x23,            // arg
	EVENT_24 = 0x24,            // arg
	EVENT_25 = 0x25,            // arg
	EVENT_26 = 0x26,            // arg
	EVENT_27 = 0x27,            // arg
	EVENT_28 = 0x28,            // arg
	EVENT_29 = 0x29,            // arg
	EVENT_2A = 0x2a             // arg
};

class Brick : public EventListener
{
public:
	// original +0x14, +0xa8, +0xe8, +0x128: four frame counters of 0x40 bytes
	FrameAnim				mGlowAnim;          // +0x14 IMAGE_BRICK_BONUS_GLOW, 13 updates a frame
	std::vector<Particle*>	mParticles;         // +0x58 (mNumParticles + 35) fall particles
	std::vector<Particle*>	mSparks;            // +0x68
	std::vector<Image*>		mRainbowImages;     // +0x78 the five brick images a rainbow brick shows in turn
	Color					mColor;             // +0x84
	Color					mColor2;            // +0x94
	Board*					mBoard;             // +0xa4
	FrameAnim				mAnim;              // +0xa8
	FrameAnim				mDeathAnim;         // +0xe8
	FrameAnim				mFxAnim;            // +0x128
	Image*					mImage;             // +0x168
	MemoryImage*			mGlowImage;         // +0x170 the glow image tinted with mColor
	Brick*					mSupport;           // +0x174 the brick this one rests on while it rises (mSupported)
	bool					mVisible;           // +0x178
	bool					mBouncing;          // +0x179 the bounce (UpdateIdle) runs
	bool					mBounceFire;        // +0x17a fire event 0xb when the bounce is over
	bool					mRetargeted;        // +0x17c SetTargetY was called since the brick last arrived: UpdateState1 takes its step even when the brick is already above the target
	bool					mWobble;            // +0x17d
	bool					mSpecial;           // +0x17e the killall brick
	bool					mLightRising;       // +0x17f the flash light goes up (StartFlash) or down (StopFlash); read by UpdateLight
	bool					mLightChanging;     // +0x180 the light (mLight) is moving towards its end value; UpdateLight clears it on arrival
	bool					mShake;             // +0x182
	bool					mFadeRising;        // +0x184 the dimming fade (mFade) goes up, else down; UpdateFade turns it at 0 and 255
	bool					mFading;            // +0x185 the dimming fade runs (UpdateFade) and the brick is drawn with it
	bool					mPulseRising;       // +0x186 the pulse (mPulse) goes up, else down; the mutation pulse turns it at 0 and 255
	bool					mBlastsOnPop;       // +0x187 the brick blasts its neighbours when it is popped (a long streak or a crossbar blast sets it); OnBrickPopped acts on it
	bool					mSupported;         // +0x188 the brick rises on a support brick (SetTargetY fires event 0x10, FireLanded event 0x11); mSupport is that brick
	bool					mHidden;            // +0x189
	bool					mGoesOffOnArrival;  // +0x18a a power brick of a launched group that goes off as soon as the group arrives (OnBrickArrived)
	bool					mMutationHeld;      // +0x18b event 0x28 with 1 holds the mutation timer and the pulse; the Board sets it while the field waits to be cleared
	bool					mLanded;            // +0x18c event 7 (landed) has been fired; false while a launched brick is on its way
	bool					mRainbow;           // +0x18d
	bool					mGone;              // +0x18e the object is out of play
	bool					mSprocketOn;        // +0x18f
	bool					mSprocketFading;    // +0x190 the sprocket light goes off: mLightValue counts down to 0 (OnSprocketOff)
	bool					mTrail;             // +0x191
	bool					mTurbotOn;          // +0x192
	bool					mTurbotFading;      // +0x193 the turbot light goes off: mLightValue counts down to 0 (OnTurbotOff)
	bool					mGlowTranslucent;   // +0x194 the bonus glow is drawn with the alpha mGlowAlpha (event 0x25 with a value below 255)
	int						mShakeX;            // +0x198
	int						mShakeY;            // +0x19c
	int						mState;             // +0x1a4
	int						mType;              // +0x1a8 colour 0..4, 5 rainbow ... 6 radial, 7 stone, 9 steel ...
	int						mTick;              // +0x1ac
	int						mFallLightStep;     // +0x1b4 the light the brick gains each update while it falls (0; 6 once OnBeginFall starts the fall)
	int						mLight;             // +0x1b8
	int						mColumn;            // +0x1bc
	int						mHighlightState;    // +0x1c0 -1: not highlighted; 0: the highlight of a hit colour runs (OnEvent20)
	int						mFade;              // +0x1c4
	int						mMutateTimer;       // +0x1c8
	int						mPulse;             // +0x1cc
	int						mMutated;           // +0x1d0 1: the brick has become a mutator
	int						mNumParticles;      // +0x1d4
	int						mNumFrames;         // +0x1dc
	int						mBounceState;       // +0x1e0
	int						mBounceDelay;       // +0x1e4
	int						mRainbowIndex;      // +0x1e8
	int						mRainbowTimer;      // +0x1ec
	int						mSprocketLight;     // +0x1f0
	int						mSprocketAlpha;     // +0x1f4
	int						mTurbotLight;       // +0x1f8
	int						mTurbotAlpha;       // +0x1fc
	int						mLightValue;        // +0x200
	int						mSprocketCel;       // +0x204
	int						mTurbotCel;         // +0x208
	int						mGlowAlpha;         // +0x20c
	float					mX;                 // +0x210
	float					mY;                 // +0x214
	float					mOffsetY;           // +0x218
	float					mWobbleSpeed;       // +0x21c
	float					mSpeedX;            // +0x220
	float					mSpeedY;            // +0x224
	float					mTargetY;           // +0x228
	float					mFloorY;            // +0x22c
	float					mBounceY;           // +0x230

	Brick(Board* theBoard, EventBus* theBus, int theColumn);    // 0x4067b0
	virtual ~Brick();                                           // 0x4060f0

	// ---- the listener part
	virtual void	OnEvent(int theEvent, EventListener* theSource, int theArg) override;
	virtual void	OnSpawn();                          // 0x402fe0
	virtual void	OnLand();                           // 0x401920
	virtual void	OnSettle();                         // 0x402f20 event 2
	virtual void	OnDie();                            // 0x403fd0 event 3
	virtual void	OnStop();                           // 0x4019d0 event 4
	virtual void	OnBeginFall();                      // 0x401a20 event 9
	void			OnTurbotReset();                    // 0x4026d0 event 0x1e: the turbot light is reset and the brick listens for the next turbot start
	void			OnEvent14();                        // 0x401780
	void			OnSprocketOn();                        // 0x4025d0
	void			OnTurbotOn();                        // 0x402670
	void			OnSprocketOff();                        // 0x402630
	void			OnTurbotOff();                        // 0x402710
	void			OnEvent20(int theArg);              // 0x401a60
	virtual void	OnBoosterPause(int theArg) { (void) theArg; }    // 0x4010b0 event 0x2a: nothing (the power brick listens)

	// ---- the object part (original vtable 0x55d5a8)
	virtual void	SetState(int theState);             // slot 1 0x402ee0
	virtual void	UpdateState0();                     // slot 2 0x401310
	virtual void	UpdateState2();                     // slot 3 0x401570
	virtual void	UpdateState1();                     // slot 4 0x4015d0 (falling)
	virtual void	UpdateState3();                     // slot 5 0x401700
	virtual bool	UpdateState6();                     // slot 6 0x401750
	virtual void	UpdateState8();                     // slot 7 0x4017f0
	virtual bool	UpdateFallOff();                    // slot 8 0x4017a0
	virtual bool	CheckParticlesDone();               // slot 9 0x403f50
	virtual void	UpdateFade();                       // slot 10 0x401820
	virtual void	UpdateRainbow();                    // slot 11 0x4062d0
	virtual void	UpdateSparks();                     // slot 12 0x406e20
	virtual void	DrawGlow(Graphics* g);              // slot 13 0x402450
	virtual void	Draw(Graphics* g, bool theGlow);    // slot 14 0x404360
	virtual bool	Update();                           // slot 15 0x402e10: true when the brick is done (a state-6 step or the end of the death)
	virtual void	SetType(int theType);               // slot 16 0x403c60
	virtual void	DrawAt(Graphics* g, bool theGlow, float theX, float theY);  // slot 17 0x4030c0
	virtual void	StartFlash();                       // slot 18 0x401c60
	virtual void	Finish();                           // slot 19 0x401900
	virtual bool	IsIdle();                           // slot 20 0x406270
	virtual void	SetParticleX(float theX);           // slot 21 0x404180
	virtual void	SetParticleY(float theY);           // slot 22 0x4041f0
	virtual void	SetVisible(bool theVisible);        // slot 25 0x4018f0
	virtual void	StartMutatorCountdown();            // slot 26 0x401230

	// ---- helpers called directly
	int		GetEffectiveType();                         // 0x4010f0 (12 for a mutated brick)
	bool	IsJammed();                                 // 0x401110
	void	Retire();                                   // 0x401130
	void	RebuildGlow();                              // 0x401140
	void	SetTargetY(int theY);                       // 0x401270
	void	StartBounce();                              // 0x401bb0
	void	PlayBoardSound(int theSound);               // 0x401c40
	void	StopFlash();                                // 0x401c80
	void	UpdateLight();                              // 0x401c90
	void	StartShake();                               // 0x401d00
	void	StopShake();                                // 0x401d20
	void	StopFade();                                 // 0x401d30
	void	UpdateMutation();                           // 0x401d60
	void	StartMutation();                            // 0x401fa0
	void	DrawBiohazard(Graphics* g, float theX, float theY);     // 0x402090
	void	DrawSprocket(Graphics* g);                  // 0x402190
	void	DrawTurbot(Graphics* g);                    // 0x4022f0
	void	DrawDeathStrip(Graphics* g);                // 0x4032e0
	void	DrawParticles(Graphics* g);                 // 0x404260
	void	DrawSparks(Graphics* g);                    // 0x4042f0
	void	FireLanded();                               // 0x4025a0
	void	UpdateWobble();                             // 0x401880
	void	StartWobble();                              // 0x401c10
	void	AdvanceParticleCount();                     // 0x4033b0
	void	MakeRainbow();                              // 0x406d70
	void	SetParticleImages(Image* theImage);         // 0x404750 (and 0x404790, 0x4047d0, 0x404810)

protected:
	static int		CelOf(const FrameAnim& theAnim) { return (theAnim.mNumFrames == 1) ? 0 : theAnim.mFrame; }
	int				ShakeOffsetX() const { return mShake ? mShakeX : 0; }
	int				ShakeOffsetY() const { return mShake ? mShakeY : 0; }
	static Image*	TypeImage(int theType, bool theSpecial);    // GetImageById((colour-blind ? 0x21a : 0x214) + (special ? 0xd : 0) + type)
	void			DrawLightPass(Graphics* g, Image* theImage, int theCel, int theShakeX, int theShakeY);  // the standard brick's light
	void			DrawOverlays(Graphics* g, int theShakeX, int theShakeY);    // biohazard, sprocket and turbot
	void			DrawBody(Graphics* g, Image* theImage, int theShakeX, int theShakeY);  // image, light, overlays
	static Image*	SafeImage(int theId);                       // GetImageById, NULL for an id outside the table
};

}

#endif
