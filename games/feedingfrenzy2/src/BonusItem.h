#ifndef __FEEDINGFRENZY2_BONUSITEM_H__
#define __FEEDINGFRENZY2_BONUSITEM_H__

// The bubbles the player eats for a bonus. A bonus comes in fading up over 0.4 seconds, then floats on with the speed
// it was given; it pops (its bubble bursting) when it meets the water's surface, and goes once it has been in view and
// leaves it. Eaten, it is drawn into the eater's mouth and fades, and once swallowed it gives its bonus: three more
// seconds for a time bubble (BonusTime), a life for a fish bubble (Bonus1Up). Each kind has its effect of the stage's
// theme, played behind its bubble.

#include "Actor.h"
#include "Geometry.h"
#include "ParticleSystem.h"
#include "State.h"
#include "Timetable.h"
#include "sigslot.h"

#include <string>

class BonusItem;

// What every state of a bonus is (BonusItem::BonusItemState, vtable 0x554920 (feedingfrenzy2); its constructor
// 0x488720 (feedingfrenzy2))
class BonusItemState : public engine::State
{
public:
	explicit BonusItemState(BonusItem* theBonus) : mBonus(theBonus), mEdible(false) {}

	// 0x1c (0x5021f5 (feedingfrenzy2)): the bonus is to leave as the stage ends; nothing by default
	virtual void evacuate() {}
	bool isEdible() const { return mEdible; }

protected:
	BonusItem* mBonus;  // +0x0c
	bool mEdible;       // +0x10: a player may eat it
	// +0x11: set while it floats, read only by the alien fish's slot (the bonus's 0xb0, 0x425a10 (feedingfrenzy2)),
	// not yet translated
};

// A bonus's state machine (BonusItem::BonusItemStateMgr, its constructor 0x407000 (feedingfrenzy2))
typedef engine::StateMgr<BonusItemState> BonusItemStateMgr;

// A bubble that gives a bonus (BonusItem, vtable 0x5547fc (feedingfrenzy2); its constructor 0x483a90
// (feedingfrenzy2))
class BonusItem : public Actor
{
public:
	// 0x483a90 (feedingfrenzy2): no effect, poppable, and the speeds' ranges (-10, 0) across and (-60, 10) up
	BonusItem();

	// 0x04 (0x41d9a0 (feedingfrenzy2)): a plain visual made and set up; with an effect's name, the stage's theme's
	// effect of it played in the visual's behind group
	void create(Stage* theStage) override;
	// 0x0c (0x40dce0 (feedingfrenzy2)): the actor's, the pop's frames added to the data, and the effect put at the
	// centre of the visual's shape
	void setVisualData(VisualData* theData) override;
	// 0x18 (0x4822b0 (feedingfrenzy2)): the actor's, in the stage's bonuses, its visual opaque, shown and at its first
	// frame; with a behaviour the speeds' ranges read from it; the effect started; still at a point, or at the
	// bottom given a speed drawn in each range; then spawning
	void addToStage(Stage* theStage, int theLocation) override;
	// 0x1c (0x407aa0 (feedingfrenzy2)): no state, out of the stage's bonuses, the effect stopped, and the actor's
	void remove() override;
	// 0x8c (0x497100 (feedingfrenzy2)): swallowed, its state ended (so it is dead to its spawn)
	void beSwallowed(Fish* theEater) override { mStateMgr.changeState(nullptr); }
	// 0xa8 (0x438d20 (feedingfrenzy2)): its state lets a player eat it
	bool isEdibleBy(Fish* theEater) override;
	// 0xcc (0x4051c0 (feedingfrenzy2)): the effect stopped, and drawn into the eater's mouth over the seconds
	void beginBeingEaten(Fish* theEater, float theSeconds) override;
	// 0xe0 (0x472b80 (feedingfrenzy2)): a "bonus" to every viewer
	bool isOfClass(const std::string& theClass, Actor* theViewer) override { return theClass == "bonus"; }
	// 0xe4 (0x437030 (feedingfrenzy2))
	bool isDead() override { return mStateMgr.getState() == nullptr; }
	// 0xe8 (0x4a2350 (feedingfrenzy2)): the effect stopped, and the state told
	void evacuate() override;
	// 0xf0 (0x452570 (feedingfrenzy2)): the state's update
	void update(engine::UpdateContext& theContext) override;
	// 0x108 (0x453910 (feedingfrenzy2)): a floating step's checks: poppable and meeting the water's surface (when
	// the stage has one away from the top), it pops; once seen and now out of view, it goes; false for either
	bool checkFloat();
	// 0x10c (0x4a3d70 (feedingfrenzy2)): its bubble bursts
	void pop();

	BonusItemStateMgr& getStateMgr() { return mStateMgr; }

protected:
	std::string mEffectName;               // +0xf0: set by each kind
	engine::Ref<engine::ParticleSystem> mEffect;  // +0x10c

private:
	// 0x40e120 (feedingfrenzy2): with the data's one run, the frames resource/bonus/BubblePop0.jpg and BubblePop1.jpg
	// and an empty one added after its frames, and the run "pop" of those three at 15 frames a second
	void addPopFrames();

	bool mPoppable;                // +0x110: true from the constructor, which nothing translated changes
	engine::FloatRange mSpeedRangeX;  // +0x114
	engine::FloatRange mSpeedRangeY;  // +0x11c
	BonusItemStateMgr mStateMgr;   // +0x124
};

// Coming in (BonusItem::SpawnState, 0x48 bytes, vtable 0x554984 (feedingfrenzy2); its constructor 0x488760
// (feedingfrenzy2)): faded up over 0.4 seconds as it floats, then floating
class BonusSpawnState : public BonusItemState
{
public:
	explicit BonusSpawnState(BonusItem* theBonus) : BonusItemState(theBonus) { mEdible = true; }

	// 0x04 (0x407a30 (feedingfrenzy2)): the fade's time started, the visual clear
	void enter() override;
	// 0x0c (0x434a40 (feedingfrenzy2)): moved and checked; still there, the visual's opacity by the share of the
	// fade, and at its end opaque and floating
	void update(engine::UpdateContext* theContext) override;
	// 0x1c (0x50207f (feedingfrenzy2)): it pops
	void evacuate() override;

private:
	Timetable mFade;  // +0x14
};

// Floating (BonusItem::FloatState, 0x1c bytes, vtable 0x5549b8 (feedingfrenzy2); its constructor 0x4044a0
// (feedingfrenzy2))
class BonusFloatState : public BonusItemState
{
public:
	explicit BonusFloatState(BonusItem* theBonus) : BonusItemState(theBonus) { mEdible = true; }

	// 0x0c (0x4538f0 (feedingfrenzy2)): moved and checked
	void update(engine::UpdateContext* theContext) override;
	// 0x1c (0x4a3dc0 (feedingfrenzy2)): it pops
	void evacuate() override;
};

// Its bubble bursting (BonusItem::PopState, 0x2c bytes, vtable 0x5549ec (feedingfrenzy2); its constructor 0x4a3d00
// (feedingfrenzy2)): the run "pop" played once, and at its end no state
class BonusPopState : public BonusItemState, public sigslot::HasSlots
{
public:
	explicit BonusPopState(BonusItem* theBonus) : BonusItemState(theBonus) {}

	// 0x04 (0x4a0590 (feedingfrenzy2)): the run played at the normal rate once, the visual's end heard
	void enter() override;
	// 0x08 (0x4a4520 (feedingfrenzy2)): no longer heard
	void exit() override;

private:
	// 0x4a4a40 (feedingfrenzy2): the bonus's state ended
	void onPopped();
};

// Being eaten (BonusItem::BonusEatenState, 0x58 bytes, vtable 0x554a30 (feedingfrenzy2); its constructor 0x48b060
// (feedingfrenzy2)): drawn from where it was into the eater's mouth over the seconds, fading as it goes, then hidden.
// It never ends by itself: the eater's swallow ends it.
class BonusEatenState : public BonusItemState
{
public:
	BonusEatenState(BonusItem* theBonus, Fish* theEater, float theSeconds)
	    : BonusItemState(theBonus), mEater(theEater), mSeconds(theSeconds)
	{
	}

	// 0x04 (0x48b460 (feedingfrenzy2)): where it starts kept and the time started; at once hidden when the seconds
	// are none
	void enter() override;
	// 0x08 (0x405240 (feedingfrenzy2)): hidden and opaque again
	void exit() override;
	// 0x0c (0x433330 (feedingfrenzy2)): on the way to the mouth by the share of the time, its opacity what is left of
	// it; hidden at its end
	void update(engine::UpdateContext* theContext) override;

private:
	Fish* mEater;              // +0x0c of its own part: its eater part
	engine::Vector2f mStart;   // +0x10
	Timetable mTime;           // +0x18
	float mSeconds;            // +0x40
};

// A time bubble (BonusTime, 0x148 bytes, vtable 0x555384 (feedingfrenzy2); its constructor 0x4a4130
// (feedingfrenzy2)), its effect bonusTimeFx
class BonusTime : public BonusItem
{
public:
	BonusTime();

	// 0x90 (0x494590 (feedingfrenzy2)): the actor's; the theme's sound bonusTimePickup (not yet translated), the
	// HUD's time bonus text where it was, and three seconds more on the stage's clock
	void eatenByPlayer(PlayerFish* thePlayer) override;
};

// A fish bubble (Bonus1Up, vtable 0x5543f4 (feedingfrenzy2); its constructor 0x5341a4 (feedingfrenzy2)), its effect
// bonus1UpFx
class Bonus1Up : public BonusItem
{
public:
	Bonus1Up();

	// 0x90 (0x53e8e6 (feedingfrenzy2)): the actor's; a life more, the theme's sound bonus1Up (not yet translated),
	// and the strings' extraLife rising where it was
	void eatenByPlayer(PlayerFish* thePlayer) override;
};

#endif
