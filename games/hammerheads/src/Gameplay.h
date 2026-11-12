#ifndef __HAMMERHEADS_GAMEPLAY_H__
#define __HAMMERHEADS_GAMEPLAY_H__

// The rules of a level that run by themselves (0x434fc0-0x435b70): which cubbies rise and when, how many creatures may be
// up at once, the timers of the power-ups, and the phase the level is in. One object of this class exists while a level
// or the shop is on screen; GetGameplay() makes it on first use and gives it the backdrop and cubby images of the level.

#include <vector>

#include "Object.h"

namespace Sexy
{

class Cubby;
class Creature;
class Image;

class Gameplay : public Object
{
public:
	enum
	{
		PHASE_PLAYING = 0,			// creatures rise until the level's timer ends
		PHASE_DRAINING = 1,			// no new ones: waits for every cubby to empty (and for a challenge sign to be dealt with)
		PHASE_PAUSING = 2,			// a second after the last cubby, then the level ends
		PHASE_ENDED = 3,
		PHASE_OVER = 4,				// the player ran out of health
		CHALLENGE_LEVEL_INTERVAL = 5	// every fifth level of Classic and Tough Cookie offers a challenge (0x591950)
	};

	std::vector<ObjectRef> mCubbies;		// +0x84: every cubby of the board (the board loader adds them)
	int				mPhase;					// +0x94
	double			mPhaseTimer;			// +0x98: the time left of PHASE_PAUSING
	double			mNextPopTimer;			// +0xa0: time until the next cubby rises; 0 when none is due
	// The power-ups: each has the time it has left and a delay that runs down first (both in the level's time units)
	double			mCloverTimer;			// +0xa8: while above 0, a creature that carries nothing may drop a coin or a heart
	double			mTimerB0;				// +0xb0
	double			mDoubleHitTimer;		// +0xb8: while above 0, the hammer is drawn with a "+2"
	double			mTimerC0;				// +0xc0
	bool			mPopNow;				// +0xc8: a cubby is to rise at the next pass, without waiting
	bool			mFirstCubbyStarted;		// +0xc9: level 1's first gnome has been started
	bool			mChallengeShown;		// +0xca: this level's challenge sign has been sent up
	bool			mChallengeDone;			// +0xcb: and has been dealt with
	bool			mFlagCC;				// +0xcc
	double			mDelayD0;				// +0xd0: before mCloverTimer starts to run
	double			mDelayD8;				// +0xd8: before mTimerC0
	double			mDelayE0;				// +0xe0: before mDoubleHitTimer
	double			mDelayE8;				// +0xe8: before mTimerB0
	Image*			mBackdrop;				// +0xf0
	Image*			mCubbyBack;				// +0xf4
	Image*			mPlatform;				// +0xf8
	Image*			mCrosshair;				// +0xfc

public:
	Gameplay();								// 0x435460
	OBJECT_TYPE(Gameplay)

	virtual void	FixedUpdate() override;	// 0x435350
	virtual void	Update(float theFrac) override;	// 0x435770

	Image*			GetBackdrop() const;				// 0x434fc0
	Image*			GetCubbyBackImage() const;			// 0x434fd0
	Image*			GetPlatformImage() const;			// 0x434fe0
	Image*			GetCrosshair() const;				// 0x434ff0: the plain one while the shop or a challenge is open
	void			SetTheme();							// 0x434de0: the images of the level's season
	bool			IsTimerB0Active() const;			// 0x42d660 (among the Avatar's functions): mTimerB0 is above 0; a click that misses costs nothing then
	bool			IsPlaying() const;					// 0x434c80: playing or draining
	void			StartEnding();						// 0x435030: the level's time is up
	void			SetPlaying();						// 0x435090
	bool			IsChallengeDue() const;				// 0x4350a0: this level offers a challenge that has not been won
	bool			IsChallengeWaiting() const;			// 0x435120
	void			SetChallengeDone();					// 0x435140
	void			GameOver();							// 0x435680

	Cubby*			GetCubby(int theIndex) const;		// 0x4347c0
	int				CountCubbiesWithKind(int theKind) const;	// 0x4347f0: cubbies whose creature is of the kind
	int				CountReady() const;					// 0x434950: empty, or sinking with nothing on
	int				CountInState(int theState) const;	// 0x434a10
	int				CountEmpty() const;					// 0x434a80
	int				PickEmptyCubby() const;				// 0x434a90: the index of a random empty cubby, or -1
	void			PopRandomCubby();					// 0x434ca0
	void			PopCubbies(int theCount);			// 0x434d60
	bool			NoBouncingOrFlying() const;			// 0x434d10: no bouncing or flying gnome is about
	int				CountLimited() const;				// 0x4351b0: cubbies up with a creature that counts toward the limit
	Creature*		FindCreatureOfKind(int theKind) const;	// 0x435210: the first one up, or null
	bool			AllReady() const;					// 0x435270
};

Gameplay* GetGameplay();									// 0x435650

}

#endif
