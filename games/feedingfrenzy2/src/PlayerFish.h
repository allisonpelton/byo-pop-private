#ifndef __FEEDINGFRENZY2_PLAYERFISH_H__
#define __FEEDINGFRENZY2_PLAYERFISH_H__

// The fish the player steers (PlayerFish, 0x348 bytes, vtables 0x552ca4 and, for its eater part, 0x552ddc
// (feedingfrenzy2); its constructor 0x403f30 (feedingfrenzy2)). The player spawn makes it once and brings it in as the
// stage starts: it drops in from the top of the water (SpawnState), then swims where the mouse pushes it (PlayState).
// The mouse's motion is a force each step, the water's drag slows it, and its speed is held in the behaviour's
// ellipse; the play area's sides and bottom hold it in, and at the surface it leaves the water and falls back.
//
// Translated so far: the drop in, the steering, the move, the water's surface and the edges, and the eating: the
// food and points of each fish swallowed, the frenzy that multiplies them and the score. The dash, the suck, the
// tilt, the effects, the texts, the sounds and the HUD are not yet, and each step that needs them says so where it
// would run.

#include "Actor.h"
#include "FishAI.h"
#include "sigslot.h"

#include <vector>

namespace engine
{
class Mouse;
}

class PlayerSpawn;

// Points the player scored, kept until its next update adds them to the score (8 bytes, the player's +0x1fc)
struct ScoredPoints
{
	ScoredPoints(int thePoints, int theMultiplier) : mPoints(thePoints), mMultiplier(theMultiplier) {}

	int mPoints;      // +0x00
	int mMultiplier;  // +0x04: the frenzy's multiplier they were scored at
};

class PlayerFish : public Fish, public sigslot::HasSlots
{
public:
	PlayerFish();

	// 0x04 (0x409e60 (feedingfrenzy2)): the fish's; then the stunned effect and the theme's playerDashFx,
	// playerFallFx, playerEatFx and playerEatBonusFx particles, and the angler's glow: not yet translated, as the
	// effects are not
	void create(Stage* theStage) override;
	// 0x18 (0x406de0 (feedingfrenzy2)): the fish's; no push, among the stage's players, at the centre of the water,
	// at its normal speed, kept between the sides, and dropping in (SpawnState)
	void addToStage(Stage* theStage, int theLocation) override;
	// 0x1c (0x404e10 (feedingfrenzy2)): its prey let go, its state ended, out of the stage's players, then the fish's
	void remove() override;
	// 0x5c (0x43b2e0 (feedingfrenzy2)): the velocity pushed by the force and, out of the water, by its fall; the drag;
	// held in the speed's ellipse unless dashing; the position moved by it and by the push, kept in the water and the
	// visual moved
	void move(engine::UpdateContext& theContext) override;
	// 0xf0 (0x44c3b0 (feedingfrenzy2)): its layer by the detail level, then its state's update
	void update(engine::UpdateContext& theContext) override;
	// 0x104 (0x4468b0 (feedingfrenzy2)): above the bottom, between the sides while it keeps to them, and at the
	// water's surface (or, with no water, below the sky)
	void keepInWater() override;
	// 0x108 (0x495700 (feedingfrenzy2)): the fish's; in the stage, its rank's name shown over it (0x405630
	// (feedingfrenzy2)), not yet translated
	void setPlayerVisualData(float theSize, FishVisualData* theData);
	// 0x110 (0x419900 (feedingfrenzy2)): swimming (PlayState)
	void startPlay();
	// 0x11c (0x445970 (feedingfrenzy2)): the body pushed back inside the edge it crosses, and the velocity across the
	// edge taken away
	void pushOut(Shape* theEdge);

	// 0x497040 (feedingfrenzy2): the engine's mouse heard (its motion, and its buttons pressed and let go), the motion
	// so far forgotten; nothing while it is heard already
	void connectMouse();
	// 0x49d4a0 (feedingfrenzy2): its motion and its presses no longer heard (its releases still are), and not sucking
	void disconnectMouse();

	// 0x43aa40 (feedingfrenzy2): the mouse's motion since the last step a force, at the mouse's sensitivity, the
	// acceleration and the share of the fish in the water
	void steer(engine::UpdateContext& theContext);
	// 0x437790 (feedingfrenzy2): the timers counted down by the step: the flash after coming in, and the dash
	void updateTimers(engine::UpdateContext& theContext);
	// 0x4379f0 (feedingfrenzy2): the timers of the play: the frenzy's, whose end takes one from the frenzy's count
	void updatePlayTimers(engine::UpdateContext& theContext);

	// The eater part's 0x10 (0x47a250 (feedingfrenzy2)): the fish's, and the highest rank of the bite's prey kept
	// (the theme's sound fishEaten not yet translated)
	void eat(Actor* thePrey, float theSeconds) override;
	// The eater part's 0x14 (0x42a6d0 (feedingfrenzy2)): with prey in the bite, its draws, the frenzy added to and the
	// chomp's splat; then each prey's food and points (the frenzy's multiplier, the bonus's scale, and in the air the
	// chain of fish eaten there), the chain ended in the water, and the fish's
	void swallow() override;
	// 0x411220 (feedingfrenzy2): points above none kept for the next update, at the frenzy's multiplier
	void addScore(int thePoints);
	// 0x130 (0x47c550 (feedingfrenzy2)): fish added to the frenzy's count (at most 35), its time set to the seconds
	// given or held at least for the hold (half again as long at each seventh), and its multiplier one more for each
	// seven
	void addFrenzy(int theCount, float theSeconds);
	// 0x4605d0 (feedingfrenzy2) with the stage's HUD: the frenzy shown
	void showFrenzy();
	// 0x124 (0x49de90 (feedingfrenzy2))
	void setFrenzyHoldTime(float theSeconds) { mFrenzyHoldTime = theSeconds; }

	// The water's surface (0x540651 (feedingfrenzy2)): the share of the body under it, the visual's tint by it, a
	// rise under it slowed, and the fish leaving and entering the water
	void keepAtSurface(Shape* theSurface);

	// The stage's camera follows it (slot 0x54, 0x43a8b0 (feedingfrenzy2), the actors'): the body's centre where it is
	engine::Vector2f getCameraTarget();

	void setSpawn(PlayerSpawn* theSpawn) { mSpawn = theSpawn; }
	PlayerSpawn* getSpawn() const { return mSpawn; }
	// 0x128 and 0x12c (feedingfrenzy2)
	int getFrenzyCount() const { return mFrenzyCount; }
	int getFrenzyMultiplier() const { return mFrenzyMultiplier; }
	float getSuckCharge() const { return mSuckCharge; }
	bool isInWater() const { return mInWater; }
	void setInWater(bool inWater) { mInWater = inWater; }
	void setJumpPeakShown(bool isShown) { mJumpPeakShown = isShown; }
	Timetable& getSpawnFlash() { return mSpawnFlash; }
	void clearMotion() { mMotion = engine::Vector2<int>(0, 0); }

private:
	// 0x427cb0 (feedingfrenzy2): the mouse's motion added up until the next steer
	void onMouseMotion(const engine::Vector2<int>& theMotion)
	{
		mMotion.x += theMotion.x;
		mMotion.y += theMotion.y;
	}
	// 0x414d00 (feedingfrenzy2): the left button dashes and the right one sucks: not yet translated
	void onMouseDown(int theButton) {}
	// 0x40c620 (feedingfrenzy2): the right button let go ends the suck
	void onMouseUp(int theButton)
	{
		if (theButton == 1)
			mSucking = false;
	}
	// 0x53fc00 (feedingfrenzy2): back in the water
	void enterWater();
	// 0x51db6f (feedingfrenzy2): out of the water, the jump begun
	void leaveWater();

	PlayerSpawn* mSpawn;                 // +0x180
	engine::Vector2<int> mMotion;        // +0x184: the mouse's motion since the last steer
	engine::Vector2<int> mLastMotion;    // +0x18c: the motion the last steer took
	engine::Mouse* mMouse;               // +0x194: the mouse while it is heard
	float mWaterShare;                   // +0x198: the share of the body in the water, 1 at first
	bool mInWater;                       // +0x19c
	// +0x19d: set by code not yet translated (as the fish comes back into the water), cleared as it drops in; the
	// play's update reads it (cold, not yet translated): left out until it is
	bool mSucking;                       // +0x1a8: the right button held
	bool mCanSuck;                       // +0x1a9: the stage's canSuck, as the fish comes in
	Timetable mSpawnFlash;               // +0x1b0: the flash after the fish comes in, ended at 3 seconds (0x4941e0
	                                     // (feedingfrenzy2): the visual's flash taken off, not yet translated)
	float mDashTime;                     // +0x1d8: while above 0, the speed is not held
	// +0x1dc, +0x1e0 and +0x1e4: timers counted down by the step, set by code not yet translated: left out
	engine::Vector2f mPush;              // +0x1f4: a push the fish moves by, (0, 0) as it comes in
	std::vector<ScoredPoints> mScored;   // +0x1fc: points scored since the last update
	float mSuckCharge;                   // +0x20c: the suck's charge, 0 to 1 (1 as the fish comes in; the suck's
	                                     // use and recharge not yet translated)
	int mBiggestRankEaten;               // +0x22c: the highest rank in the bite, 0 after each swallow
	bool mBonusEat;                      // +0x230: the bite gets the bonus's effect (set by code not yet translated)
	int mPointsScale;                    // +0x234: 1 as it comes in (a bonus sets it, not yet translated)
	float mSpeedScale;                   // +0x260: the speed's and the acceleration's scale, 1 as it comes in (a
	                                     // bonus's speed sets it, not yet translated)
	int mFrenzyCount;                    // +0x290: fish eaten in the frenzy, 0 to 35
	float mFrenzyTime;                   // +0x294: until the count drops by one
	float mFrenzyHoldTime;               // +0x298: the time the frenzy holds after a bite
	int mFrenzyMultiplier;               // +0x29c: one more for each seven in the count
	float mJumpHeight;                   // +0x2a0: the highest above the surface of the jump
	bool mJumpPeakShown;                 // +0x2a4: the jump's height told, once a jump
	int mAirChain;                       // +0x2a8: fish eaten in the air since the fish was last in the water
	bool mKeptBetweenSides;              // +0x308: true as it comes in
};

// The player fish dropping in from the top of the water (SpawnState, 0x2c bytes, vtable 0x552e3c (feedingfrenzy2);
// its constructor 0x497b30 (feedingfrenzy2))
class SpawnState : public FishState
{
public:
	explicit SpawnState(PlayerFish* thePlayer) : FishState(thePlayer), mPlayer(thePlayer) {}

	// 0x04 (0x487b00 (feedingfrenzy2)): still, at the middle of the water's width at the top, in the water unless the
	// water's top is below the stage's top, the flash's time set
	void enter() override;
	// 0x08 (0x49e020 (feedingfrenzy2)): the flash's time started over, the mouse's motion forgotten
	void exit() override;
	// 0x0c (0x432890 (feedingfrenzy2)): pushed down until its mouth passes a depth, then swimming
	void update(engine::UpdateContext* theContext) override;

private:
	PlayerFish* mPlayer;  // +0x20
};

// The player fish swimming where the mouse pushes it (PlayState, 0x2c bytes, vtable 0x552ec4 (feedingfrenzy2); its
// constructor 0x4198a0 (feedingfrenzy2))
class PlayState : public FishState
{
public:
	// 0x4198a0 (feedingfrenzy2): in play and edible (and the flags +0x12 to +0x18, read by code not yet translated)
	explicit PlayState(PlayerFish* thePlayer) : FishState(thePlayer), mPlayer(thePlayer)
	{
		mInPlay = true;
		mEdible = true;
	}

	// 0x0c (0x438fe0 (feedingfrenzy2)): steered, moved and its timers counted down
	void update(engine::UpdateContext* theContext) override;
	// 0x54 (0x417e10 (feedingfrenzy2)): eating (PlayerEatingState)
	void eat(Actor* thePrey) override;

private:
	PlayerFish* mPlayer;  // +0x20
};

// The player fish eating (PlayerEatingState, 0x40 bytes, vtable 0x552f4c (feedingfrenzy2); its constructor 0x417db0
// (feedingfrenzy2)): it swims on while the visual plays its eat, and goes back to swimming after the chomp
class PlayerEatingState : public EatingState
{
public:
	// 0x417db0 (feedingfrenzy2): the eating state's (the flags +0x12, +0x13, +0x15 and +0x1a, read by code not yet
	// translated)
	// 0x417db0 (feedingfrenzy2): busy
	explicit PlayerEatingState(PlayerFish* thePlayer) : EatingState(thePlayer), mPlayer(thePlayer) { mBusy = true; }

	// 0x0c (0x437690 (feedingfrenzy2)): steered, moved and its timers counted down; after the chomp, swimming again
	void update(engine::UpdateContext* theContext) override;

private:
	PlayerFish* mPlayer;  // +0x34
};

#endif
