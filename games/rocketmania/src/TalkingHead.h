// The talking head of Rocket Mania Deluxe (class TalkingHead 0x4161a0, the announcer that sways beside the board and speaks
// at the big moments) and the hover tips of the board (0x413aa0). See
// docs/GAME_NOTES.md.

#ifndef __ROCKETMANIA_TALKINGHEAD_H__
#define __ROCKETMANIA_TALKINGHEAD_H__

#include "ButtonWidget.h"

namespace Sexy
{

class RocketApp;
class Graphics;
struct Effect;
class EffectManager;
class SoundInstance;

// 0x4161a0, 0x110 bytes: a button widget that the app adds to its widget manager at start-up (0x42d360) and keeps for the
// whole run. It is a head on a body with a shadow, held by two springs: the place it should be at (Place) pulls the real
// place (mPosX, mPosY) along, and a pendulum (mTilt) turns the head a little while it moves. A mood (SetMood) is a list of
// mouths that it shows one after the other while a voice line plays (Say). Climbing out of a hole in the board's corner
// (StartSlide) is a second way for it to appear. The head is not a mouse target.
class TalkingHead : public ButtonWidget
{
public:
	RocketApp*		mApp;               // +0xc4
	Effect*			mTrail;             // +0xa0 the flames of the rocket it rides in on, in the board's effects
	bool			mFlying;            // +0xa4 on the way in: it drops from the top on a swinging path
	int				mTargetX;           // +0xa8 where Place told it to go
	int				mTargetY;           // +0xac
	bool			mShadow;            // +0xb0 the shadow under the body is drawn
	int				mSlideSpeed;        // +0xb4 the climb out of the hole: 12 up, -12 down, 0 not moving
	int				mSlidePos;          // +0xb8 how far it has climbed (0 to 599)
	int				mSlideDelay;        // +0xbc updates to wait before the climb starts
	bool			mHidden;            // +0xc0 after a climb down: nothing is drawn
	bool			mSkipDraw;          // +0xc1 the next draw is left out (once)
	double			mTilt;              // +0xc8 the head's turn in radians
	double			mTiltSpeed;         // +0xd0
	double			mPosX, mPosY;       // +0xd8, +0xe0 where it really is (the widget is put at these numbers)
	double			mVelX, mVelY;       // +0xe8, +0xf0
	double			mSway;              // +0x98 how far the head is from the body (5 to 25), swung by the speed
	int				mMood;              // +0xf8 the mood it is in (-1 none)
	int				mMoodFrame;         // +0xfc updates in this step
	int				mMoodStep;          // +0x100
	int				mMouth;             // +0x104 the picture of the head: 0 to 6 the mouths, 7 and 8 the turned heads
	bool			mIdle;              // +0x108 the player has not touched anything for two minutes
	SoundInstance*	mVoice;             // +0x10c the voice line that plays

public:
	TalkingHead(RocketApp* theApp);
	virtual ~TalkingHead();

	virtual void	Update() override;                                  // 0x4166b0
	virtual void	Draw(Graphics* g) override;                         // 0x416b10
	virtual void	Move(int theNewX, int theNewY) override;            // 0x416660: the head is put 20 above the place

	void			SetMood(int theMood);                               // 0x416360
	void			Place(int theX, int theY);                          // 0x416630: where it should go (and the widget's flag)
	void			Say(int theSound);                                  // 0x416aa0: plays a voice line of the sound manager
	void			SetShadow(bool theShadow) { mShadow = theShadow; }  // 0x416b00
	bool			IsSliding() const;                                  // 0x416e00
	bool			IsInHole() const;                                   // 0x416e20
	void			StartSlide(bool theUp, int theDelay);               // 0x416e50
	void			SetSlide(bool theUp);                               // 0x416e80
	void			SkipDraw() { mSkipDraw = true; }                    // 0x416eb0
	void			Surface();                                          // 0x416de0: out of the hole, in front of everything
	void			DrawHole(Graphics* g);                              // 0x416d50: the board draws the hole it climbs from
	void			FlyIn(EffectManager* theEffects);                   // 0x4163a0: rides in on a rocket for the level's end
	void			Follow(EffectManager* theEffects);                  // 0x416470: the flames follow it, and the board's top rows go
	void			FlyOut(EffectManager* theEffects);                  // 0x4164f0: the flames are over and it goes to the side
	void			GoToRest(bool theClimb);                            // 0x416520: out of the way (and, if asked, up from the hole)
	void			Arrive();                                           // 0x416590: from the left of the board to its place
	void			StepAside();                                        // 0x4165f0: out of the way of a dialog
};

// 0x413aa0, 0x18 bytes: the pointer resting on one of seven places of the board for 81 updates makes a tip appear that
// explains what is there; it fades in over 16 updates and goes when the pointer leaves the place.
class HoverTip
{
public:
	RocketApp*		mApp;               // +0x00
	int				mRestCount;         // +0x04 updates the pointer has been still
	int				mLastX, mLastY;     // +0x08, +0x0c where it was
	int				mRegion;            // +0x10 the place a tip is shown for (-1 none)
	int				mShown;             // +0x14 updates the tip is up

	HoverTip(RocketApp* theApp);

	int				Hit(int theX, int theY);                            // 0x413ad0: the place under the pointer, -1 for none
	void			Update(int theX, int theY);                         // 0x413b40
	void			Draw(Graphics* g);                                  // 0x414000
private:
	void			DrawTip(Graphics* g, int theRegion);                // 0x413c00
};

}

#endif
