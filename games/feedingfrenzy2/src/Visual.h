#ifndef __FEEDINGFRENZY2_VISUAL_H__
#define __FEEDINGFRENZY2_VISUAL_H__

// An actor's look in the stage: a container of three groups (behind, mirrored and in front) with the actor's frames
// in the mirrored one, played as an AnimatedImage (Visual, 0xec bytes and its virtual base, vtable 0x553d24
// (feedingfrenzy2); its constructor 0x420b50 (feedingfrenzy2)). A fish's (FishVisual, 0x4a8 bytes, vtable 0x550ad4
// (feedingfrenzy2); its constructor 0x420c50 (feedingfrenzy2)) runs a machine of states that pick the run to play from
// the fish's movement: idle, swim, a turn (the mirrored group flipped at its end), eat (with the chomp at the model's
// eatFrame), puke, the suck's intro and outro, and stunned.

#include "Container.h"
#include "AnimatedImage.h"
#include "State.h"
#include "TimeDriver.h"
#include "KeyInterpolator.h"
#include "VisualData.h"
#include "sigslot.h"

#include <string>

namespace engine
{
class Theme;
}
class Fish;

class Visual : public engine::Container, public sigslot::HasSlots
{
public:
	Visual();

	// 0x469130 (feedingfrenzy2): the groups made and added, behind, mirrored (with the image in it) and in front, and
	// the image's end passed on
	void setup();
	// 0x4314d0 (feedingfrenzy2): the data's frames shown by the image, and the visual named after the model
	void setData(VisualData* theData);
	// 0x461360 (feedingfrenzy2): the run of the name played at the rate (backwards below 0) in the mode; a run
	// already playing that way goes on unless restart
	void play(const std::string& theRun, float theRate, int theMode, bool restart);
	// 0x118 (0x41f700 (feedingfrenzy2)), connected to the image's end (0x4c2a40 (feedingfrenzy2)): mFinished sent
	virtual void onImageFinished(unsigned int theRun);

	VisualData* getData() const { return mData; }
	engine::AnimatedImage* getImage() const { return mImage; }
	engine::Container* getBehindGroup() const { return mBehindGroup; }
	engine::Container* getMirroredGroup() const { return mMirroredGroup; }
	engine::Container* getFrontGroup() const { return mFrontGroup; }

	// +0xc8: a run played once came to its end
	sigslot::signal0 mFinished;

protected:
	engine::Ref<VisualData> mData;                 // +0xd8
	engine::Ref<engine::AnimatedImage> mImage;     // +0xdc
	engine::Ref<engine::Container> mMirroredGroup; // +0xe0: "VisualMirroredGroup"
	engine::Ref<engine::Container> mBehindGroup;   // +0xe4: "VisualBehindGroup"
	engine::Ref<engine::Container> mFrontGroup;    // +0xe8: "VisualFrontGroup"
};

class FishVisual;

// A state of a fish's visual (FishVisual::FishVisualState, 0x10 bytes, vtable 0x550c14 (feedingfrenzy2); its
// constructor 0x40f870 (feedingfrenzy2))
class FishVisualState : public engine::State
{
public:
	explicit FishVisualState(FishVisual* theVisual) : mVisual(theVisual) {}

	// 0x1c: the image's run came to its end (the machine's 0x477390 (feedingfrenzy2), connected to the image's
	// finished signal); nothing by default
	virtual void onRunFinished() {}
	// 0x20 (0x47d630 (feedingfrenzy2)): the seconds from the eat run's start to its chomp, the model's eatFrame at the
	// run's rate
	virtual float getTimeToChomp();
	// 0x24 (0x502684 (feedingfrenzy2)): whether an eat may start now; always by default
	virtual bool canEat() { return true; }
	// 0x28 (0x502684 (feedingfrenzy2)): whether a turn may start now; always by default
	virtual bool canTurn() { return true; }
	// 0x2c (0x418c80 (feedingfrenzy2)): eat, the visual put in its eat state
	virtual void eat();

protected:
	FishVisual* mVisual;  // +0x0c
};

// 0x40f900 (feedingfrenzy2): "idle" played, back and forth where the model's idleShuttle is 1 or more
class FishVisualIdleState : public FishVisualState
{
public:
	explicit FishVisualIdleState(FishVisual* theVisual) : FishVisualState(theVisual) {}
	// 0x04 (0x40b3a0 (feedingfrenzy2))
	void enter() override;
	// 0x0c (0x41bff0 (feedingfrenzy2)): unless it turns, swimming once the speed's square passes the swim threshold
	void update(engine::UpdateContext* theContext) override;
};

// 0x40f6b0 (feedingfrenzy2): "swim" played, its rate from the speed
class FishVisualSwimState : public FishVisualState
{
public:
	explicit FishVisualSwimState(FishVisual* theVisual) : FishVisualState(theVisual) {}
	// 0x04 (0x42fe00 (feedingfrenzy2))
	void enter() override;
	// 0x0c (0x44c780 (feedingfrenzy2)): unless it turns, idle below the idle threshold where the model has an idle
	// run, else the run's rate the speed times the swim's rate and its base
	void update(engine::UpdateContext* theContext) override;
};

// 0x40f6f0 (feedingfrenzy2), 0x1c bytes: "turn" played once, the mirrored group flipped at its end
class FishVisualTurnState : public FishVisualState
{
public:
	explicit FishVisualTurnState(FishVisual* theVisual) : FishVisualState(theVisual), mEatSuppressed(false) {}
	// 0x04 (0x4243c0 (feedingfrenzy2)): twice the rate for a fast turn
	void enter() override;
	// 0x08 (0x474820 (feedingfrenzy2)): the facing flipped once the turn passed its first safe frame, and a chomp
	// held back sent
	void exit() override;
	// 0x0c (0x471660 (feedingfrenzy2)): the mirror's value through the turn
	void update(engine::UpdateContext* theContext) override;
	// 0x1c (0x4773b0 (feedingfrenzy2)): back to idle or swim
	void onRunFinished() override;
	// 0x24 (0x447100 (feedingfrenzy2)): not in the model's safe frames of the turn, unless the turn is fast
	bool canEat() override;
	// 0x28 (0x502bf3 (feedingfrenzy2))
	bool canTurn() override { return false; }
	// 0x2c (0x40e7d0 (feedingfrenzy2)): outside the safe frames the eat starts; inside them it waits ("EAT
	// SUPPRESSED.") and the chomp is sent as the turn ends
	void eat() override;

private:
	bool mEatSuppressed;  // +0x10
};

// 0x40f730 (feedingfrenzy2), 0x1c bytes: "eat" played once from its start, the chomp sent at the model's eatFrame
class FishVisualEatState : public FishVisualState
{
public:
	explicit FishVisualEatState(FishVisual* theVisual) : FishVisualState(theVisual), mChomped(false) {}
	// 0x04 (0x4826a0 (feedingfrenzy2))
	void enter() override;
	// 0x0c (0x471590 (feedingfrenzy2)): the chomp at its frame; after it the fish may turn
	void update(engine::UpdateContext* theContext) override;
	// 0x1c (0x417d70 (feedingfrenzy2)): a chomp not yet sent sent, then back to idle or swim
	void onRunFinished() override;
	// 0x20 (0x40e4f0 (feedingfrenzy2)): before the chomp, the seconds until it; after it, the base's
	float getTimeToChomp() override;

private:
	bool mChomped;  // +0x10
};

// 0x40f770 (feedingfrenzy2), 0x1c bytes: "eat" played backwards, the chomp as it passes the model's eatFrame
class FishVisualPukeState : public FishVisualState
{
public:
	explicit FishVisualPukeState(FishVisual* theVisual) : FishVisualState(theVisual), mChomped(false) {}
	// 0x04 (0x531fec (feedingfrenzy2))
	void enter() override;
	// 0x0c (0x53dd06 (feedingfrenzy2))
	void update(engine::UpdateContext* theContext) override;
	// 0x1c (0x53d30b (feedingfrenzy2))
	void onRunFinished() override;
	// 0x20 (0x531f4e (feedingfrenzy2)): the seconds until the eatFrame, 0 past it
	float getTimeToChomp() override;

private:
	bool mChomped;  // +0x10
};

// 0x40f7b0 (feedingfrenzy2): "swim" set and stopped at once, the fish held still
class FishVisualStunnedState : public FishVisualState
{
public:
	explicit FishVisualStunnedState(FishVisual* theVisual) : FishVisualState(theVisual) {}
	// 0x04 (0x414550 (feedingfrenzy2))
	void enter() override;
};

// 0x40f7f0 (feedingfrenzy2), 0x18 bytes: "SuckIntro" played once
class FishVisualSuckIntroState : public FishVisualState
{
public:
	explicit FishVisualSuckIntroState(FishVisual* theVisual) : FishVisualState(theVisual) {}
	// 0x04 (0x489410 (feedingfrenzy2)): with the suck's effect (+0x114), the effect started at the mouth
	void enter() override;
	// 0x08 (0x48f860 (feedingfrenzy2)): the suck's effect stopped
	void exit() override;
	// 0x20 (0x490e30 (feedingfrenzy2)): the eat run's frames from the suck's to the chomp, at its rate
	float getTimeToChomp() override;
};

// 0x40f830 (feedingfrenzy2), 0x1c bytes: "SuckOutro" played once, the chomp at the eatFrame's place in it
class FishVisualSuckOutroState : public FishVisualState
{
public:
	explicit FishVisualSuckOutroState(FishVisual* theVisual) : FishVisualState(theVisual), mChomped(false) {}
	// 0x04 (0x494840 (feedingfrenzy2))
	void enter() override;
	// 0x0c (0x4033c0 (feedingfrenzy2))
	void update(engine::UpdateContext* theContext) override;
	// 0x1c (0x490b40 (feedingfrenzy2))
	void onRunFinished() override;
	// 0x20 (0x495580 (feedingfrenzy2))
	float getTimeToChomp() override;

private:
	bool mChomped;  // +0x10
};

// The visual's machine (FishVisual::FishVisualStateMgr, vtable 0x550c88 (feedingfrenzy2); its constructor 0x41a510
// (feedingfrenzy2))
class FishVisualStateMgr : public engine::StateMgr<FishVisualState>, public sigslot::HasSlots
{
public:
	// 0x477390 (feedingfrenzy2), connected to the image's finished signal: passed to the current state
	void onImageFinished(unsigned int theRun)
	{
		FishVisualState* aState = getState();
		if (aState != nullptr)
			aState->onRunFinished();
	}
};

class FishVisual : public Visual
{
public:
	// 0x420c50 (feedingfrenzy2): the stage's theme and the fish; the states made, the visual updated every frame
	FishVisual(engine::Theme* theTheme, Fish* theFish);

	// 0xa4 (0x43b470 (feedingfrenzy2)): the machine, the children, the mirror's scale and the rotation, the size's
	// pulse, then the effects
	void update(engine::UpdateContext& theContext) override;
	// 0xdc (0x421b60 (feedingfrenzy2)): out of its parent, the machine emptied
	void clearParent() override;

	// 0x421030 (feedingfrenzy2): the groups, the image's end passed to the machine, and the inked effect in front
	void setupFish();
	// 0x46d7f0 (feedingfrenzy2): the data, the model's forward and orientation, whether it has idle and turn runs, and
	// the effects' points
	void setFishData(FishVisualData* theData);
	// 0x4251c0 (feedingfrenzy2): the size's pulse from the old size over the new, over the seconds
	void startSizePulse(float theFrom, float theTo, float theSeconds);

	// 0x451d80 (feedingfrenzy2): a turn started when the movement along forward passes the threshold against the
	// facing; true when it did
	bool turnIfNeeded();
	// 0x424400 (feedingfrenzy2): the turn state, or with no turn run the facing flipped at once
	void turn();
	// 0x422aa0 (feedingfrenzy2): idle below the idle threshold where there is an idle run, else swim
	void resume();
	// 0x4262e0 (feedingfrenzy2)
	void changeState(FishVisualState* theState) { mStateMgr.changeState(theState); }
	FishVisualState* getState() { return mStateMgr.getState(); }

	FishVisualData* getFishData() const { return mFishData; }
	const engine::Vector2f& getVelocity() const { return mVelocity; }
	void setVelocity(const engine::Vector2f& theVelocity) { mVelocity = theVelocity; }
	float getFacing() const { return mFacing; }
	// 0x41b550 (feedingfrenzy2): the facing, the mirror (the model's orientation times it) and the scale of the
	// attached components, the mirror's on the axis the model faces along
	void setFacing(float theFacing);
	// The size's pulse's scale, 1 as the fish comes in (the fish's 0x421ae0 (feedingfrenzy2))
	void setPulseScale(float theScale) { mScale = theScale; }
	float getPulseScale() const { return mScale; }
	// The swim run's rate per unit of speed and its base (the fish's 0x42aa30 (feedingfrenzy2))
	void setSwimRate(float theScale, float theBase)
	{
		mSwimRateScale = theScale;
		mSwimRateBase = theBase;
	}
	// The stage's scroll rate, each step (the stage's 0x439c40 (feedingfrenzy2))
	void setTurnBias(float theBias) { mTurnBias = theBias; }
	const engine::Vector2f& getMirrorScale() const { return mMirrorScale; }
	// 0x475f50 (feedingfrenzy2): the effects still playing stopped (not yet translated, as the effects are not) and
	// the image's colour put back to white, its mode the parent's
	void resetEffects();

	// The states, for the fish to set
	FishVisualIdleState* getIdleState() const { return mIdleState; }
	FishVisualSwimState* getSwimState() const { return mSwimState; }
	FishVisualTurnState* getTurnState() const { return mTurnState; }
	FishVisualEatState* getEatState() const { return mEatState; }
	FishVisualPukeState* getPukeState() const { return mPukeState; }
	FishVisualStunnedState* getStunnedState() const { return mStunnedState; }
	FishVisualSuckIntroState* getSuckIntroState() const { return mSuckIntroState; }
	FishVisualSuckOutroState* getSuckOutroState() const { return mSuckOutroState; }

	// +0xec: the chomp, at the eat run's eatFrame
	sigslot::signal0 mChomp;

private:
	friend class FishVisualState;
	friend class FishVisualIdleState;
	friend class FishVisualSwimState;
	friend class FishVisualTurnState;
	friend class FishVisualEatState;
	friend class FishVisualPukeState;
	friend class FishVisualStunnedState;
	friend class FishVisualSuckIntroState;
	friend class FishVisualSuckOutroState;

	// 0x471550 (feedingfrenzy2): the image's frame from its run's start, 0 without frames or a run
	int getRunFrame() const;
	// 0x4715f0 (feedingfrenzy2): how far through its run the image is, 0 to 1, 0 without frames or a run
	double getRunProgress() const;
	// 0x4054f0 (feedingfrenzy2): the seconds the image's frame has shown
	double getFrameTimeShown() const;
	// The mirror's scale from its value (+0x168 and +0x16c)
	void setMirrorScale();

	uint64_t mLastUpdateFrame;                     // +0x100: the frame of the last update
	engine::Ref<engine::Theme> mTheme;             // +0x108
	engine::Ref<FishVisualData> mFishData;         // +0x10c
	Fish* mFish;                                   // +0x110
	// +0x114 and +0x118: the suck's effect and its kin, set only for the player (not yet translated)
	float mIdleThreshold;                          // +0x128: the speed's square below which it idles (20)
	float mSwimThreshold;                          // +0x12c: the speed's square above which an idle fish swims (60)
	float mTurnThreshold;                          // +0x130: the movement along forward that turns it (15)
	bool mFastTurn;                                // +0x134: the turn run at twice the rate
	bool mHasIdle;                                 // +0x136: the data has an "idle" run
	bool mHasTurn;                                 // +0x137: the data has a "turn" run
	engine::Vector2f mVelocity;                    // +0x138
	engine::Vector2f mForward;                     // +0x140: the model's forward
	float mSwimRateScale;                          // +0x148: the swim run's rate per unit of speed
	float mSwimRateBase;                           // +0x14c
	float mTurnBias;                               // +0x150: added to the movement along forward
	float mScale;                                  // +0x154: the visual's scale, the size's pulse
	float mMirrorRotation;                         // +0x158: the mirrored group's rotation
	float mFacing;                                 // +0x15c: 1 or -1
	float mOrientation;                            // +0x160: the model's
	float mMirror;                                 // +0x164: the facing times the orientation, through a turn
	engine::Vector2f mMirrorScale;                 // +0x168: the attached components' scale (not yet translated)
	// +0x170: set to 1, read by nothing translated yet
	engine::FloatKeyInterpolator mPulseCurve;      // +0x174: the size's pulse, by its time
	engine::TimeDriver mPulseTime;                 // +0x1c0
	engine::TimeDriver mFadeTime;                  // +0x1e8: the fade of +0x210 (not yet translated)
	// +0x210 (a component fading out) and the four effects (+0x218, 0x88 bytes each: the stunned stars, the glow,
	// the inked effect and the halo, run by their timed events) and the attached components (+0x448): not yet
	// translated but for the inked effect's particle system and the points; none of them is set on a menu
	engine::Ref<engine::Component> mInkedEffect;   // +0x328: the effects' third
	engine::Vector2f mEffectPoints[4];             // +0x290 and every 0x88 after: the effects' points in the data
	FishVisualStateMgr mStateMgr;                  // +0x458
	engine::Ref<FishVisualIdleState> mIdleState;           // +0x484
	engine::Ref<FishVisualSwimState> mSwimState;           // +0x488
	engine::Ref<FishVisualTurnState> mTurnState;           // +0x48c
	engine::Ref<FishVisualEatState> mEatState;             // +0x490
	engine::Ref<FishVisualPukeState> mPukeState;           // +0x494
	engine::Ref<FishVisualStunnedState> mStunnedState;     // +0x498
	engine::Ref<FishVisualSuckIntroState> mSuckIntroState; // +0x49c
	engine::Ref<FishVisualSuckOutroState> mSuckOutroState; // +0x4a0
};

#endif
