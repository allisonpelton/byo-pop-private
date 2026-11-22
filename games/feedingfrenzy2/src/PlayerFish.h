#ifndef __FEEDINGFRENZY2_PLAYERFISH_H__
#define __FEEDINGFRENZY2_PLAYERFISH_H__

// The fish the player steers (PlayerFish, 0x348 bytes, vtables 0x552ca4 and, for its eater part, 0x552ddc
// (feedingfrenzy2); its constructor 0x403f30 (feedingfrenzy2)). The player spawn makes it once and brings it in as the
// stage starts: it drops in from the top of the water (SpawnState), then swims where the mouse pushes it (PlayState).
// The mouse's motion is a force each step, the water's drag slows it, and its speed is held in the behaviour's
// ellipse; the play area's sides and bottom hold it in, and at the surface it leaves the water and falls back.
//
// Translated so far: the drop in, the steering, the move, the water's surface and the edges. The eating, the dash,
// the suck, the tilt, the effects and the HUD are not yet, and each step that needs them says so where it would run.

#include "Actor.h"
#include "FishAI.h"
#include "sigslot.h"

namespace engine
{
class Mouse;
}

class PlayerSpawn;

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
	// 0x4379f0 (feedingfrenzy2): the timers of the play (not yet translated; each set by code not yet translated)
	void updatePlayTimers(engine::UpdateContext& theContext) {}

	// The water's surface (0x540651 (feedingfrenzy2)): the share of the body under it, the visual's tint by it, a
	// rise under it slowed, and the fish leaving and entering the water
	void keepAtSurface(Shape* theSurface);

	// The stage's camera follows it (slot 0x54, 0x43a8b0 (feedingfrenzy2), the actors'): the body's centre where it is
	engine::Vector2f getCameraTarget();

	void setSpawn(PlayerSpawn* theSpawn) { mSpawn = theSpawn; }
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
	float mSpeedScale;                   // +0x260: the speed's and the acceleration's scale, 1 as it comes in (a
	                                     // bonus's speed sets it, not yet translated)
	float mJumpHeight;                   // +0x2a0: the highest above the surface of the jump
	bool mJumpPeakShown;                 // +0x2a4: the jump's height told, once a jump
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
	// 0x54 (0x417e10 (feedingfrenzy2)): eating (PlayerEatingState): not yet translated, as the player's eating is not

private:
	PlayerFish* mPlayer;  // +0x20
};

#endif
