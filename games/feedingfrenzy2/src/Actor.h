#ifndef __FEEDINGFRENZY2_ACTOR_H__
#define __FEEDINGFRENZY2_ACTOR_H__

// The things that move about a stage. An Actor has a visual, the shapes of its visual data (its outline, body and
// mouth) moved with it, a position moved by its velocity, and the velocity steered towards the one it wants; it sits
// on one of the stage's layers, by the detail level. A Fish is an actor with a fish's visual, speeds and rank, that
// can eat fish of lower ranks; an AIFish is one the computer moves through the states of FishAI.h.
//
// An actor is made once for its spawn's pool and reused: the spawn sets it up (behaviour, visual data, rank), puts it
// at its entry point and adds it to the stage; when it dies (a fish, when its state machine has no state left) its
// spawn takes it out of the stage and puts it back in the pool.

#include "ActorInfo.h"
#include "Container.h"
#include "FishAI.h"
#include "Geometry.h"
#include "Object.h"
#include "Shape.h"
#include "State.h"
#include "VisualData.h"
#include "sigslot.h"

class FishVisual;
class PlayerFish;
class Stage;
class Visual;

// A shape of the actor's (0x0c bytes each, the actor's +0x24): the visual data's, where the actor's transform is,
// and the copy the transform was last laid on
struct ActorShape
{
	ActorShape() : mTransform(nullptr) {}

	engine::Ref<Shape> mShape;        // +0x00
	ShapeTransform* mTransform;       // +0x04: the actor's own, for every shape
	engine::Ref<Shape> mTransformed;  // +0x08
};

// The roles of the actor's shapes (setupShapes's order)
enum ActorShapeIndex
{
	ACTOR_SHAPE_VISUAL = 0,  // "visual": the frame's box
	ACTOR_SHAPE_BODY = 1,    // "body"
	ACTOR_SHAPE_MOUTH = 2,   // "mouth"
	ACTOR_SHAPE_COUNT = 5
};

// What every actor is (Actor, 0xf0 bytes; its constructor 0x4226f0 (feedingfrenzy2))
class Actor : public engine::Object
{
public:
	// 0x4226f0 (feedingfrenzy2): in the water, at the origin, still, on layer 0
	Actor();

	// 0x04: what the actor needs before its first spawn, for the stage (a fish's visual)
	virtual void create(Stage* theStage) {}
	// 0x08 (0x4611c0 (feedingfrenzy2)): each shape's transform the actor's own; the "visual", "body" and "mouth"
	// shapes of the visual data
	virtual void setupShapes();
	// 0x0c (0x422620 (feedingfrenzy2)): the visual given the data, kept, and the shapes set up from it
	virtual void setVisualData(VisualData* theData);
	// 0x10 (0x41db90 (feedingfrenzy2))
	virtual void setBehavior(BehaviorInfo* theBehavior) { mBehavior = theBehavior; }
	// 0x18 (0x425fe0 (feedingfrenzy2)): still, not yet seen, in the stage's actors, and its visual on its layer
	virtual void addToStage(Stage* theStage, int theLocation);
	// 0x1c (0x407a80 (feedingfrenzy2)): its visual taken off, out of the stage's actors and the stage (a fish's,
	// 0x475ff0 (feedingfrenzy2), does the same itself)
	virtual void remove();
	// 0x20 (0x49d800 (feedingfrenzy2))
	bool inStage() const { return mStage != nullptr; }
	// 0x24 (0x474ae0 (feedingfrenzy2)): where the actor comes in for the location (a side drawn for "side", which the
	// location then says), just outside the bounds at a drawn height or across; the point for any other
	engine::Vector2f getEntryPoint(Stage* theStage, int& theLocation, const engine::Vector2f& thePoint);
	// 0x28 (0x45aca0 (feedingfrenzy2)): the position, the shapes' transform and the visual moved with it
	virtual void setPosition(float x, float y);
	// 0x2c (0x43d040 (feedingfrenzy2)) to 0x3c
	const engine::Vector2f& getPosition() const { return mPosition; }
	void setVelocity(const engine::Vector2f& theVelocity) { mVelocity = theVelocity; }
	const engine::Vector2f& getVelocity() const { return mVelocity; }
	void setDesiredVelocity(const engine::Vector2f& theVelocity) { mDesiredVelocity = theVelocity; }
	const engine::Vector2f& getDesiredVelocity() const { return mDesiredVelocity; }
	// 0x40 (0x44c810 (feedingfrenzy2))
	// The step's force set outright (the player's states)
	void setForce(const engine::Vector2f& theForce) { mForce = theForce; }
	void addForce(float x, float y)
	{
		mForce.x = (float)((double)x + mForce.x);
		mForce.y = (float)((double)mForce.y + y);
	}
	// 0x44 to 0x50 (0x502c04, 0x502bfd, 0x480dc0 and 0x502bf6 (feedingfrenzy2))
	void setAccelRate(float theRate) { mAccelRate = theRate; }
	float getAccelRate() const { return mAccelRate; }
	void setDragRate(float theRate) { mDragRate = theRate; }
	float getDragRate() const { return mDragRate; }
	// 0x58 (0x43d410 (feedingfrenzy2)): the outline meets the stage's view
	bool isInView();
	// 0x5c (0x43d480 (feedingfrenzy2)): the velocity steered towards the wanted one at the acceleration (or slowed by
	// the drag when none is wanted), the position moved by it, kept in the water, and the visual moved
	virtual void move(engine::UpdateContext& theContext);
	// 0x60 (0x43c1f0 (feedingfrenzy2))
	Visual* getVisual() const { return mVisual.get(); }
	// 0x64 (0x474ad0 (feedingfrenzy2))
	Shape* getShape(int theIndex) const { return mShapes[theIndex].mShape.get(); }
	// 0x68 (0x43dd30 (feedingfrenzy2)): the shape copied into its transformed one and the transform laid on it
	Shape* getTransformedShape(int theIndex);
	// 0x6c (0x502bd4 (feedingfrenzy2)): the actor eats the other (a fish's, 0x474c30 (feedingfrenzy2))
	virtual bool canEat(Actor* theOther) { return false; }
	// 0x70 and 0x74 (0x41d740 and 0x440900 (feedingfrenzy2))
	void setRank(int theRank) { mRank = theRank; }
	int getRank() const { return mRank; }
	// 0x9c (0x422520 (feedingfrenzy2)): the bounds the actor keeps to, from the stage's area: under the water, or in
	// the air above it, or within the behaviour's depths below the water when it has them
	void setBounds(Stage* theStage, bool inAir);
	// 0xa0 (0x4915e0 (feedingfrenzy2))
	const engine::RectF& getBounds() const { return mBounds; }
	// 0xa4 (0x502bd9 (feedingfrenzy2))
	bool isUnderwater() const { return mUnderwater; }
	// 0x80 and 0x84 (0x411210 and 0x411200 (feedingfrenzy2)): the food it is to its eater, and the points it is now
	int getFoodValue() const { return mFoodValue; }
	int getFoodValueNow() const { return mFoodValueNow; }
	// 0x78 (0x43d8b0 (feedingfrenzy2)): the body's shape where it is
	Shape* getBodyShape() { return getTransformedShape(ACTOR_SHAPE_BODY); }
	// 0x8c (0x5014fa (feedingfrenzy2)): swallowed by the eater; nothing by default
	virtual void beSwallowed(Fish* theEater) {}
	// 0x90 (0x53c96f (feedingfrenzy2)): swallowed by the player: its spawn told it died
	virtual void eatenByPlayer(PlayerFish* thePlayer) { mDied.emit(); }
	// 0xa8 (0x502bd4 (feedingfrenzy2)): the actor may be eaten (the eater not looked at); not by default
	virtual bool isEdibleBy(Fish* theEater) { return false; }
	// 0xc0 (0x502bee (feedingfrenzy2)): the eater may have the actor now; always by default
	virtual bool canBeEatenBy(Fish* theEater) { return true; }
	// 0xcc (0x502be0 (feedingfrenzy2)): drawn into the eater's mouth over the seconds; nothing by default
	virtual void beginBeingEaten(Fish* theEater, float theSeconds) {}
	// 0xe0 (0x502c50 (feedingfrenzy2)): the actor is of the class a <react> names, to the viewer (a fish's,
	// 0x43b230 (feedingfrenzy2))
	virtual bool isOfClass(const std::string& theClass, Actor* theViewer) { return false; }
	// 0xe4: the actor is dead, for its spawn to take back (a fish's, 0x4447c0 (feedingfrenzy2))
	virtual bool isDead() = 0;
	// 0xe8: the actor leaves as the stage ends (the actor's pure; a fish's, 0x5021f5 (feedingfrenzy2), does nothing;
	// an AI fish's 0x4a0260 (feedingfrenzy2))
	virtual void evacuate() {}
	// 0xf0: each step of the stage's run
	virtual void update(engine::UpdateContext& theContext) = 0;
	// 0xf8 (0x425fb0 (feedingfrenzy2)): the visual put in its layer's actors
	void addVisualToLayer();
	// 0xfc (0x4222b0 (feedingfrenzy2))
	engine::Container* getLayerContainer(Stage* theStage);
	// 0x100 (0x472e30 (feedingfrenzy2)): the visual out of what holds it
	void removeVisual();
	// 0x104: the actor kept in the water (an AI fish's, 0x43d070 (feedingfrenzy2))
	virtual void keepInWater() {}

	// The shapes' transform's scales, from the visual's (the stage's 0x439c40 (feedingfrenzy2), 0x439bf0
	// (feedingfrenzy2) the axes')
	void setShapeScale(const engine::Vector2f& theAxisScale, float theScale)
	{
		mTransform.mAxisScale = theAxisScale;
		mTransform.mScale = theScale;
	}
	bool hasTurned() const { return mTurned; }
	void clearTurned() { mTurned = false; }
	void setMaxForce(const engine::Vector2f& theForce) { mMaxForce = theForce; }
	void setMaxVelocity(const engine::Vector2f& theVelocity) { mMaxVelocity = theVelocity; }
	Stage* getStage() const { return mStage; }
	bool wasSeen() const { return mSeen; }
	void setSeen(bool theSeen) { mSeen = theSeen; }
	bool isLeaving() const { return mLeaving; }
	void setLeaving(bool isLeaving) { mLeaving = isLeaving; }
	BehaviorInfo* getBehavior() const { return mBehavior.get(); }
	int* getDetailLayers() { return mDetailLayers; }
	void setLayer(int theLayer) { mLayer = theLayer; }

	// +0x0c: the actor died, for its spawn
	sigslot::signal0 mDied;

protected:
	// 0x518393 (feedingfrenzy2): the visual taken off its layer, the position moved by the difference of the two
	// layers' offsets, and the visual put on the new one
	void changeLayer(int theLayer);
	// 0x4690f0 (feedingfrenzy2): the shapes' transform at the position, moved by the layer's offset off the base
	// layer
	void updateTranslation();
	// The layer the detail level puts the actor on, and the change to it, as each update starts (inlined in
	// 0x4447e0 (feedingfrenzy2))
	void checkLayer(engine::UpdateContext& theContext);

	engine::Ref<VisualData> mVisualData;    // +0x1c
	engine::Ref<Visual> mVisual;            // +0x20
	ActorShape mShapes[ACTOR_SHAPE_COUNT];  // +0x24
	ShapeTransform mTransform;              // +0x60
	Stage* mStage;                          // +0x78: while it is in the stage
	int mDetailLayers[3];                   // +0x7c: the layer at each detail level
	int mLayer;                             // +0x88: the layer it is on
	bool mSeen;                             // +0x8c: it has been in view
	bool mLeaving;                          // +0x8d: it leaves the stage, no new state taken (set by EvacuateAI)
	int mFoodValue;                         // +0x90
	int mFoodValueNow;                      // +0x94
	engine::Vector2f mPosition;             // +0x98
	engine::Vector2f mForce;                // +0xa0: the acceleration of the step
	engine::Vector2f mMaxForce;             // +0xa8: the ellipse the force is kept in, none when (0, 0)
	engine::Vector2f mVelocity;             // +0xb0
	engine::Vector2f mMaxVelocity;          // +0xb8: the same for the velocity
	engine::Vector2f mDesiredVelocity;      // +0xc0
	bool mTurned;                           // +0xc8: the force pushed against the wanted way across
	float mAccelRate;                       // +0xcc
	float mDragRate;                        // +0xd0
	bool mUnderwater;                       // +0xd4
	engine::RectF mBounds;                  // +0xd8
	engine::Ref<BehaviorInfo> mBehavior;    // +0xe8
	int mRank;                              // +0xec
};

// A fish's state machine (FishStateMgr, vtable 0x550208 (feedingfrenzy2)): the engine's, of fish states
typedef engine::StateMgr<FishState> FishStateMgr;

// A fish (Fish, 0x170 bytes, vtables 0x54fc84 and, for its eater part, 0x54fdac (feedingfrenzy2); its constructor
// 0x481370 (feedingfrenzy2)). Its eater part (Eater, at +0xf0) is folded in: the fish eats fish ranked below its
// eater rank.
class Fish : public Actor
{
public:
	Fish();

	// 0x04 (0x4814a0 (feedingfrenzy2)): the fish's visual made in the stage's theme and set up
	void create(Stage* theStage) override;
	// 0x18 (0x421ae0 (feedingfrenzy2)): the actor's, in the stage's fish, its visual at its own scale and shown,
	// facing the way it comes in from
	void addToStage(Stage* theStage, int theLocation) override;
	// 0x1c (0x475ff0 (feedingfrenzy2)): out of the stage's fish, its visual's effects stopped and its colour put
	// back, its visual taken off, out of the stage's actors and the stage
	void remove() override;
	// 0x6c (0x474c30 (feedingfrenzy2)): the other is ranked below the eater rank
	bool canEat(Actor* theOther) override { return getEaterRank() > theOther->getRank(); }
	// 0x7c (0x41d750 (feedingfrenzy2)): the food value, and the value now (0x88, 0x41d770 (feedingfrenzy2))
	void setFoodValue(int theValue)
	{
		mFoodValue = theValue;
		mFoodValueNow = theValue;
	}
	// 0xe0 (0x43b230 (feedingfrenzy2)): a "predator" to a viewer it may eat, "prey" to a viewer that may eat it, of
	// no other class
	bool isOfClass(const std::string& theClass, Actor* theViewer) override;
	// 0xe4 (0x4447c0 (feedingfrenzy2)): no state left
	bool isDead() override { return mStateMgr.getState() == nullptr; }
	// 0x108 (0x422660 (feedingfrenzy2)): the fish's visual given the data (its size pulsed when it had one before),
	// the size and data kept, and the shapes set up
	void setFishVisualData(float theSize, FishVisualData* theData);
	// 0x118 (0x42aa30 (feedingfrenzy2)): the behaviour kept and the speeds drawn from it, the swim's rate set, the
	// acceleration and drag taken
	virtual void setFishBehavior(FishBehaviorInfo* theBehavior);

	// 0x8c (0x416a10 (feedingfrenzy2)): swallowed, the fish's state ended (so it is dead to its spawn)
	void beSwallowed(Fish* theEater) override { mStateMgr.changeState(nullptr); }
	// 0xa8 (0x43dbe0 (feedingfrenzy2)): the fish's state lets another eat it
	bool isEdibleBy(Fish* theEater) override;
	// 0xc0 (0x481210 (feedingfrenzy2)): the fish's state lets this eater have it now
	bool canBeEatenBy(Fish* theEater) override;
	// 0xcc (0x4800f0 (feedingfrenzy2)): drawn into the eater's mouth over the seconds (EatenState)
	void beginBeingEaten(Fish* theEater, float theSeconds) override;
	// 0x4251a0 (feedingfrenzy2): the state changed to unless the fish is leaving
	void changeState(FishState* theState);

	// The eater part (Eater, its slots by number):
	// 0x00 (0x43df80 (feedingfrenzy2)): the fish may start an eat: it has a state, in play, its visual's state lets it
	// eat, and it is not held
	bool canEatNow();
	// 0x04 and 0x08
	void setEaterRank(int theRank) { mEaterRank = theRank; }
	int getEaterRank() const { return mEaterRank; }
	// 0x0c (0x43dee0 (feedingfrenzy2)): the prey eaten when the fish may eat, the prey may be eaten by it and its body
	// meets the mouth; the prey drawn in over the seconds to the chomp, at most 0.2
	void consider(Actor* thePrey);
	// 0x10 (0x47a2b0 (feedingfrenzy2)): the prey taken, and the fish's state told of the eat
	virtual void eat(Actor* thePrey, float theSeconds);
	// 0x14 (0x477660 (feedingfrenzy2)): each prey taken swallowed, and the list emptied
	virtual void swallow();
	// 0x24 (0x43ded0 (feedingfrenzy2)): the mouth's shape where it is
	Shape* getMouthShape() { return getTransformedShape(2); }
	// 0x28 (0x43dc00 (feedingfrenzy2)): the visual data's mouth laid on the shapes' transform
	engine::Vector2f getMouthPosition() const;

	FishVisual* getFishVisual() const { return mFishVisual.get(); }
	FishBehaviorInfo* getFishBehavior() const { return mFishBehavior.get(); }
	const engine::Vector2f& getMinSpeed() const { return mMinSpeed; }
	const engine::Vector2f& getMaxSpeed() const { return mMaxSpeed; }
	FishStateMgr& getStateMgr() { return mStateMgr; }

protected:
	int mEaterRank;                              // +0xf8
	std::vector<engine::Ref<Actor>> mPrey;       // +0xfc: the prey being drawn in until the chomp
	// +0x10c: the meals, the player's to puke: not yet translated
	float mSize;                                 // +0x11c
	engine::Ref<FishVisualData> mFishVisualData; // +0x120
	engine::Ref<FishVisual> mFishVisual;         // +0x124: the same as the actor's visual
	engine::Ref<FishBehaviorInfo> mFishBehavior; // +0x138
	engine::Vector2f mMinSpeed;                  // +0x13c
	engine::Vector2f mMaxSpeed;                  // +0x144
	float mHoldTime;                             // +0x14c: counted down to 0 each update; set by code not yet
	                                             // translated
	FishStateMgr mStateMgr;                      // +0x154
};

// A fish the computer moves (AIFish, 0x1a4 bytes, vtable 0x54e6cc and, for its eater part, 0x54e810
// (feedingfrenzy2); its constructor 0x41d550 (feedingfrenzy2))
class AIFish : public Fish, public sigslot::HasSlots
{
public:
	AIFish();

	// 0x04 (0x481320 (feedingfrenzy2)): the fish's, and its stunned effect (not yet translated)
	void create(Stage* theStage) override;
	// 0x18 (0x421a90 (feedingfrenzy2)): the fish's, its home where it comes in, in the stage's AI fish, and swimming
	// in (0x134, 0x421980 (feedingfrenzy2))
	void addToStage(Stage* theStage, int theLocation) override;
	// 0x1c (0x476130 (feedingfrenzy2)): its eater's prey let go, its state ended, out of the stage's AI fish, then
	// the fish's
	void remove() override;
	// The eater part's 0x10 (0x41cb20 (feedingfrenzy2)): the stage's theme's sound fishEaten first (not yet
	// translated, as the sounds are not), then the fish's
	void eat(Actor* thePrey, float theSeconds) override;
	// 0xf0 (0x4447e0 (feedingfrenzy2)): its layer by the detail level, its hold counted down; a state that has gone
	// ended, else the state's update (a fish with meals in it that may puke pukes first, not yet translated)
	void update(engine::UpdateContext& theContext) override;
	// 0x104 (0x43d070 (feedingfrenzy2)): under the water kept under it, in the air kept above it
	void keepInWater() override;
	// 0xe8 (0x4a0260 (feedingfrenzy2)): with a state, leaving (EvacuateAI); not when its +0x150, false from the
	// constructor, is set (0x502941 (feedingfrenzy2), not yet translated)
	void evacuate() override;
	// 0x110 (0x4288b0 (feedingfrenzy2)): the next state by a draw: schooling in a school, chasing the player, flying
	// by or wandering
	void pickState();
	// 0x118 (0x46c800 (feedingfrenzy2)): the fish's, and how often it chases and flies by
	void setFishBehavior(FishBehaviorInfo* theBehavior) override;
	// 0x12c (0x47c000 (feedingfrenzy2)): the reaction's state taken towards the other: fleeing it ("run"), avoiding
	// it or chasing it; false for any other reaction
	bool react(FishReaction* theReaction, Actor* theOther);
	// 0x130 (0x473bd0 (feedingfrenzy2)): the fish may react so: run from or avoid an actor that eats it, chase one it
	// eats, and any other reaction at all
	bool canReact(FishReaction* theReaction, Actor* theOther);
	// 0x90 (0x419580 (feedingfrenzy2)): swallowed by the player: the last of its school to go gives the player the
	// school's bonus, then the actor's
	void eatenByPlayer(PlayerFish* thePlayer) override;
	// 0x13c (0x48f710 (feedingfrenzy2)): the fish turns aside from the other
	void avoid(Actor* theOther);
	// 0x138 (0x41ab30 (feedingfrenzy2)): the fish swims with its school (SchoolingAI)
	void joinSchool();
	// 0x434930 (feedingfrenzy2): the distance from the mouth to the other when the fish sees it (as the stage's
	// 0x43d0c0 (feedingfrenzy2) sees), else -1
	float distanceInSight(Actor* theOther);
	// 0x437ac0 (feedingfrenzy2): a look about: the actors in sight, nearest first, and the first with a reaction drawn
	// reacted to
	void lookAround();

	const engine::Vector2f& getHome() const { return mHome; }
	SchoolOfFish* getSchool() const { return mSchool.get(); }
	void setSchool(SchoolOfFish* theSchool) { mSchool = theSchool; }

private:
	// 0x435180 (feedingfrenzy2): the behaviour's first reaction to the other, by its class, that the fish may have
	// and whose frequency the draw passes; none when no reaction does
	FishReaction* findReaction(Actor* theOther);
	// 0x51e371 (feedingfrenzy2): the body pushed back inside the edge it crosses
	void keepInside(Shape* theEdge);

	// +0x180 to +0x18c: four floats at 0, read by nothing translated yet
	engine::Vector2f mHome;     // +0x190: where it came in
	engine::Ref<SchoolOfFish> mSchool;  // +0x198: the school it came in with, kept after it leaves the stage
	float mChaseFrequency;      // +0x19c: "chaseFreq", the share of picks that chase the player
	float mFlybyFrequency;      // +0x1a0: "flybyFreq"
};

#endif
