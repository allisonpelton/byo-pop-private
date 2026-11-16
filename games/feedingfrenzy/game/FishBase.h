#ifndef __FEEDINGFRENZY_FISHBASE_H__
#define __FEEDINGFRENZY_FISHBASE_H__

// A fish (FishBase, 0x2e8 bytes, constructor 0x408520, vtable 0x49f710, destructor 0x407b10): a container that holds
// the fish's animations (a selector over its swimming, turning, eating and extra pictures) and moves with a velocity
// that follows a desired one. It has the rectangles the stage's collisions test (its body, its mouth and its hit
// rectangle, each as the picture is drawn and as it stands in the stage), the list of the fish it is eating, its size,
// the level that says who eats whom, and its points. The computer's fish (AIFish.h) and the player's (PlayerFish.h)
// derive from it.

#include "CollisionRect.h"
#include "FishAnimImage.h"
#include "ImageFilter.h"
#include "Node.h"
#include "Selector.h"
#include <string>
#include <vector>

namespace feedingfrenzy
{

class FishSchool;

// How a fish's animation is playing (+0x134)
enum FishAnimState
{
	ANIM_IDLE = 0,        // the swimming picture waits on its first frame, or the extra animation plays
	ANIM_SWIM = 1,
	ANIM_TURN_LEFT = 2,   // turning to face left
	ANIM_TURN_RIGHT = 3,
	ANIM_EAT = 4
};

// What a fish is, for the spawners' statistics (+0x2e4)
enum FishKind
{
	FISH_KIND_FISH = 0,
	FISH_KIND_BONUS = 1,   // a bonus bubble
	FISH_KIND_HAZARD = 2   // a jellyfish
};

// What the player's eating of a victim counts as (+0x2d4)
enum FishEdibleKind
{
	EDIBLE_FISH = 0,
	EDIBLE_OBJECT = 1   // a mine or a bonus bubble
};

// A fish's thought bubble: the picture it shows above a fish that has noticed another, and where (+0x2a4 to +0x2cc)
struct ThoughtBubbleInfo
{
	float mScale;
	std::string mImage;
	engine::Point mOffset;
};

class FishBase : public engine::Container
{
public:
	FishBase();            // 0x408520
	~FishBase() override;  // 0x407b10

	// slot 0x14: 0x407020 and slot 0x18: 0x407050: the fish moves, and its rectangles follow
	void setPosition(const engine::Point& thePosition) override;
	void setPosition(float theX, float theY) override;
	// slot 0x7c: 0x406420: a frame of a fish that is in the stage
	void update(engine::UpdateInfo& theInfo) override;

	// slot 0xe4: 0x4076b0: the animations of the fish, a file name prefix each (the frames <prefix><n> with the
	// fish's extension, counting from 1): swimming, turning, eating and one extra, each but the first optional (null
	// for none, and a file with no frame counts as none)
	virtual void loadAnimations(const char* theSwimPath, const char* theTurnPath, const char* theEatPath,
	                            const char* theExtraPath);
	// slot 0xe8: 0x4083c0: the animations in the folder of a fish ("swim\swim_cycle.", "turn\turn_cycle." and
	// "eat\eat_cycle.")
	virtual void loadAnimationsOf(const std::string& theFolder);
	// slot 0xec: the effects a kind of fish makes, which the fish file names
	virtual void loadEffects() = 0;
	// slot 0xf0: 0x406180: the speeds: the greatest and the least in each direction, how fast the velocity follows a
	// desired one that is not zero and one that is
	virtual void setSpeeds(float theMaxSpeedX, float theMaxSpeedY, float theMinSpeedX, float theMinSpeedY,
	                       float theSpeedUp, float theSlowDown);
	virtual void setSize(float theSize) { mSize = theSize; }          // slot 0xf4: 0x4061c0
	virtual void setLevel(float theLevel) { mLevel = theLevel; }      // slot 0xf8: 0x4061d0
	virtual void setPoints(float thePoints) { mPoints = thePoints; }  // slot 0xfc: 0x4061e0
	// slot 0x100: 0x4061f0: how far the fish sees and the angle (degrees) of its sight
	virtual void setVision(float theRange, float theAngle);
	// slot 0x104: 0x408220: the sound that plays when the fish is eaten, and its pitch
	virtual void setEatenSound(const std::string& theName, float thePitch);
	// slot 0x108: 0x406e20: the body rectangle, as the picture is drawn
	virtual void setBodyRect(float theLeft, float theTop, float theRight, float theBottom);
	// slot 0x10c: 0x406e50: the mouth rectangle, as the picture is drawn
	virtual void setMouthRect(const CollisionRect& theRect);
	virtual CollisionRect* getMouthRect() { return &mPlacedMouthRect; }  // slot 0x110: 0x406e80
	// slots 0x114, 0x118, 0x11c: 0x407d30 and slot 0x120: 0x416820: the computer's fish take the ranges of their
	// decisions (AIFish.h); any other fish does not
	virtual void setIdleRanges(float, float, float, float) {}
	virtual void setChaseRanges(float, float, float, float) {}
	virtual void setFleeRanges(float, float, float, float) {}
	virtual void setSightCheckRange(float) {}
	virtual engine::Rect* getBodyRect() { return &mPlacedBodyRect; }  // slot 0x124: 0x406e90
	// slot 0x128: 0x406290: the hit rectangle, as the picture is drawn
	virtual void setHitRect(const CollisionRect& theRect);
	virtual CollisionRect* getPlacedHitRect() { return &mPlacedHitRect; }  // slots 0x12c and 0x134: 0x406410
	virtual CollisionRect* getScaledHitRect() { return &mScaledHitRect; }  // slot 0x130: 0x406400
	// slot 0x138: 0x4067b0: the fish can eat (the argument, the fish that it would eat, is not read): it is not turning,
	// shows and is not being eaten itself
	virtual bool canEat(FishBase* theVictim);
	// slot 0x13c: whether the fish can be eaten by a fish of the level
	virtual bool canBeEatenByLevel(float theEaterLevel) = 0;
	// slot 0x140: whether the fish can be eaten by the fish
	virtual bool canBeEatenBy(FishBase* theEater) = 0;
	// slot 0x144: 0x406db0: the eater starts to eat the fish, over a time, which the fish spends moving into the
	// eater's mouth
	virtual void startBeingEaten(FishBase* theEater, float theSeconds);
	// slot 0x148: 0x419740: the eater has eaten the fish; false when the eater cannot
	virtual bool finishBeingEaten(FishBase*) { return false; }
	// slot 0x14c: 0x416820
	virtual void onTouched(FishBase*) {}
	virtual bool isStunnable() { return mStunnable; }       // slot 0x150: 0x406ff0
	virtual bool isPoisonProof() { return mPoisonProof; }   // slot 0x154: 0x407000
	virtual bool isTargetable() { return mTargetable; }     // slot 0x158: 0x407010
	// slot 0x15c: 0x407e60: the eating animation, unless the fish is turning at the wrong moment
	virtual void playEatAnimation();
	// slot 0x160: the fish takes a victim in its mouth
	virtual void eat(FishBase* theVictim) = 0;
	// slot 0x164: the fish has finished eating what is in its mouth
	virtual void finishEating() {}
	virtual bool isHuge() { return mHuge; }                           // slot 0x168: 0x402060
	virtual engine::Point getNosePosition() { return mNose; }         // slot 0x16c: 0x402070
	virtual engine::Point getTailPosition() { return mTail; }         // slot 0x170: 0x4020a0
	virtual engine::Point getCentre();                                // slot 0x174: 0x407100
	virtual float getLevel() { return mLevel; }                       // slot 0x178: 0x4020d0
	// slot 0x17c: 0x406230: the fish is put in the stage, swimming, with the first picture
	virtual void putInPlay();
	// slot 0x180: 0x40dcf0: the stage has added the fish (nothing does anything at it)
	virtual void onEnterStage() {}
	virtual bool isInPlay() { return mInPlay; }                       // slot 0x184: 0x406280
	// slot 0x188: 0x407d70: the fish leaves the stage
	virtual void takeOutOfPlay();
	virtual void setSwallowed(bool theSwallowed) { mSwallowed = theSwallowed; }  // slot 0x18c: 0x407080
	virtual bool isSwallowed() { return mSwallowed; }                            // slot 0x190: 0x407090
	virtual void setMarked(bool theMarked) { mMarked = theMarked; }              // slot 0x194: 0x4020e0
	virtual bool isMarked() { return mMarked; }                                  // slot 0x198: 0x4020f0
	// slot 0x19c: 0x406d60: the fish shows on the screen; slot 0x1a0: 0x406d80: it is inside the stage
	virtual bool isOnScreen();
	virtual bool isInStage();
	virtual float getPoints() { return mPoints; }                                // slot 0x1a4: 0x402100
	virtual engine::Point getVelocity() { return mVelocity; }                    // slot 0x1a8: 0x402110
	virtual void setVelocity(float theX, float theY) { mVelocity.set(theX, theY); }  // slot 0x1ac: 0x402140
	virtual float getSize() { return mSize; }                                    // slot 0x1b0: 0x402160
	// slots 0x1b4: 0x404850 and 0x1b8: 0x404a70: the name of the kind of fish
	virtual std::string getTypeName() { return mTypeName; }
	virtual void setTypeName(const std::string& theName) { mTypeName = theName; }
	// slot 0x1bc: the fish is stunned for the seconds by a fish, and shows its effect when asked
	virtual void stun(float theSeconds, FishBase* theSource, bool theShowEffect) = 0;
	virtual int getEatCount() { return mEatCount; }                    // slot 0x1c0: 0x402170
	virtual void addPush(float theX, float theY);                      // slot 0x1c4: 0x406ef0
	virtual void clearPush() { mPush.set(0.0f, 0.0f); }                // slot 0x1c8: 0x406f20
	virtual void setPushAgainst(float theX, float theY) { mPushAgainst.set(theX, theY); }  // slot 0x1cc: 0x406f30
	virtual void addPushAgainst(const engine::Point& thePush);                              // slot 0x1d0: 0x406f50
	virtual void clearPushAgainst() { mPushAgainst.set(0.0f, 0.0f); }  // slot 0x1d4: 0x406fa0
	virtual bool isEaten() { return mEaten; }                          // slot 0x1d8: 0x406fb0
	virtual void setEaten(bool theEaten) { mEaten = theEaten; }        // slot 0x1dc: 0x406fc0
	virtual bool stingsOnContact() { return mStings; }                 // slot 0x1e0: 0x406fe0
	virtual void onContact(FishBase*) {}                               // slot 0x1e4: 0x416820
	virtual bool isPickup() { return mPickup; }                        // slot 0x1e8: 0x4071e0
	virtual void setSchool(FishSchool* theSchool) { mSchool = theSchool; }  // slot 0x1ec: 0x406fd0
	virtual void setColorShift(engine::HsvShiftFilter* theShift) { mColorShift = theShift; }  // slot 0x1f0: 0x4070a0
	// slot 0x1f4: 0x40dcf0: the fish is told to swim away (the computer's fish do)
	virtual void leave() {}
	// slot 0x1f8: 0x4070b0: the frame of the eating animation that eats ("eatFinishFrame")
	virtual void setEatFrame(int theFrame) { mEatFrame = theFrame; }
	// slot 0x1fc: 0x407d40: another fish has shown it a reaction, which the computer's fish take
	virtual void alert(FishBase*, float, float) {}
	virtual void endAlert() {}  // slot 0x200: 0x40dcf0
	// slot 0x204: 0x4608c0
	virtual bool isAlerted() { return false; }
	// slot 0x208: 0x408290: the thought bubble's scale, picture and place
	virtual void setThoughtBubble(const ThoughtBubbleInfo& theInfo);
	// slot 0x20c: 0x4070e0: the fish's eating counts as food to the player
	virtual bool countsAsFood() { return mCountsAsFood; }
	virtual int getKind() { return mKind; }                  // slot 0x210: 0x402180
	virtual int getEdibleKind() { return mEdibleKind; }      // slot 0x214: 0x4070f0
	// slot 0x218: 0x4071b0: the frames of the eating animation that a fish that is eating may not turn in
	virtual void setUnsafeEatFrames(int theFirst, int theLast);
	virtual bool isHidden() { return mHidden; }              // slot 0x21c: 0x4071d0
	// slot 0x220: 0x407260: the rectangles, from the picture that shows
	virtual void updateRects();
	// slot 0x224: 0x4062c0: the hit rectangle as the fish stands in the stage
	virtual void updatePlacedHitRect();
	// slot 0x228: 0x4067f0: the nose, the tail and the body and mouth rectangles as the fish stands in the stage
	virtual void updatePlacedRects();
	virtual void move(float theSeconds);       // slot 0x22c: 0x4074d0: the velocity follows the desired one
	virtual void updateAnimation();            // slot 0x230: 0x406470
	virtual void updateTurn();                 // slot 0x234: 0x4066a0
	virtual void updateSwim();                 // slot 0x238: 0x407380
	virtual void showExtraAnimation();         // slot 0x23c: 0x406ea0
	virtual void updateEat();                  // slot 0x240: 0x406720
	// slot 0x244: 0x406b00: the fish shrinks into the mouth of its eater until the time is up
	virtual void updateBeingEaten(engine::UpdateInfo& theInfo);

	// 0x4070c0: the fish faces right (its picture is turned over)
	bool isMirrored() const { return mScale.x < 0.0f; }

	std::vector<FishBase*> mVictims;  // +0x1e0: the fish it is eating, until it has finished

	engine::Point mVelocity;        // +0x11c: pixels a second
	engine::Selector* mAnimations;  // +0x124: the pictures, one shows at a time (counted by the container)
	bool mHasTurnAnimation;         // +0x128
	bool mHasEatAnimation;          // +0x129
	bool mHasExtraAnimation;        // +0x12a: an animation that plays while the fish waits
	int mExtraAnimationIndex;       // +0x12c: its place in the selector
	bool mHuge;                     // +0x130
	int mAnimState;                 // +0x134: a FishAnimState
	bool mEatFinished;              // +0x138: the eating animation has reached its frame that eats
	int mEatCount;                  // +0x13c: how many times the player has eaten a fish like it
	float mPreviousVelocityX;       // +0x140: the horizontal velocity of the frame before
	float mMaxSpeedX;               // +0x144
	float mMaxSpeedY;               // +0x148
	float mMinSpeedX;               // +0x14c
	float mMinSpeedY;               // +0x150
	float mSpeedUp;                 // +0x154: how fast the velocity follows a desired one that is not zero
	float mSlowDown;                // +0x158: how fast it follows a desired one of zero
	float mSize;                    // +0x15c: the picture's scale
	float mPoints;                  // +0x160
	engine::Point mNose;            // +0x164: where the fish's front is, in the stage
	engine::Point mTail;            // +0x16c
	float mLevel;                   // +0x174: a fish is eaten by a fish of a higher level
	bool mInPlay;                   // +0x178
	float mVisionRange;             // +0x17c
	float mVisionRangeSquared;      // +0x180
	float mVisionCosine;            // +0x184: the cosine of the sight's angle
	engine::Point mDesiredVelocity; // +0x188: the velocity it moves towards
	engine::Point mPush;            // +0x190: added to the velocity each frame
	engine::Point mPushAgainst;     // +0x198: added to the velocity each frame
	std::string mTypeName;          // +0x1a0
	std::string mEatenSoundName;    // +0x1bc
	float mEatenSoundPitch;        // +0x1d8
	FishBase* mEater;               // +0x1ec: the fish that is eating it
	bool mBeingEaten;               // +0x1f0
	float mBeingEatenSeconds;       // +0x1f4: how long it takes
	float mBeingEatenLeft;          // +0x1f8
	engine::Point mEatenFrom;       // +0x1fc: where the fish stood when the eating began
	engine::Point mEatenCentreOffset;  // +0x204: where its centre was from there
	engine::Rect mPlacedBodyRect;   // +0x20c: the body rectangle as it stands in the stage
	engine::Rect mBodyRect;         // +0x21c: as the picture is drawn
	CollisionRect mPlacedMouthRect; // +0x22c
	CollisionRect mMouthRect;       // +0x240
	CollisionRect mHitRect;         // +0x254: as the picture is drawn
	CollisionRect mScaledHitRect;   // +0x268: scaled by the size
	CollisionRect mPlacedHitRect;   // +0x27c: as it stands in the stage
	bool mStings;                   // +0x290
	bool mPickup;                   // +0x291: eaten by touching it
	bool mEaten;                    // +0x292
	FishSchool* mSchool;          // +0x294: the school the fish swims in, or null
	bool mPoisonProof;              // +0x298
	bool mStunnable;                // +0x299
	bool mTargetable;               // +0x29a
	bool mReactable;                // +0x29b: it may react to another fish
	bool mSwallowed;                // +0x29c
	bool mMarked;                   // +0x29d
	engine::HsvShiftFilter* mColorShift;  // +0x2a0: the colour offset of the pictures, or null
	ThoughtBubbleInfo mThought;     // +0x2a4
	int mEatFrame;                  // +0x2cc
	bool mPngFrames;                // +0x2d0: its pictures are png files, not jpeg
	bool mCountsAsFood;             // +0x2d1
	int mEdibleKind;                // +0x2d4: a FishEdibleKind
	int mUnsafeEatFirstFrame;       // +0x2d8
	int mUnsafeEatLastFrame;        // +0x2dc
	bool mHidden;                   // +0x2e0
	int mKind;                      // +0x2e4: a FishKind
};

} // namespace feedingfrenzy

#endif
