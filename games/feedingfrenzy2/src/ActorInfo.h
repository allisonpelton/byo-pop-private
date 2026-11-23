#ifndef __FEEDINGFRENZY2_ACTORINFO_H__
#define __FEEDINGFRENZY2_ACTORINFO_H__

// What the actors' file (resource/actors/fishes.xml) says of each kind of actor, kept by name in the fish factory:
// - a model (ModelInfo, <model>) is a picture, a single image or an animation's folder, with its shapes and anchor
//   points; a fish model (FishModelInfo, <fishModel>) adds the frames and directions a fish's animations need;
// - a visual (VisualInfo, <visual>) is a model drawn with a colour effect; a fish visual (FishVisualInfo,
//   <fishVisual>) is one of a fish model;
// - a behaviour (BehaviorInfo, <behavior>) keeps every attribute of its element for the actor that reads it; a fish
//   behaviour (FishBehaviorInfo, <fishBehavior>) is how a fish the computer moves swims, sees, schools and reacts to
//   others.
// Each element has its handler (ModelInfo::DocHandler, VisualInfo::DocHandler, FishBehaviorInfo::DocHandler, each
// 0x5c bytes); a model and a visual are put in the factory when their element ends, a fish behaviour when it begins,
// a behaviour at once by the factory's handler.

#include "Geometry.h"
#include "MasterDocHandler.h"
#include "Params.h"
#include "Shape.h"

#include <map>
#include <string>
#include <vector>

// 0x46e340 (feedingfrenzy2): the string in lower case (CharLowerA)
std::string toLower(const std::string& theString);

// A <model> (0x6c bytes, vtable 0x5521b0 (feedingfrenzy2); its constructor 0x477b70 (feedingfrenzy2))
class ModelInfo : public engine::Object
{
public:
	ModelInfo() : mAnimated(false), mSize(0) {}

	// 0x04 (0x45a460 (feedingfrenzy2)): the element's attributes read
	virtual void load(engine::Params& theAttributes);
	// 0x08 (0x4200b0 (feedingfrenzy2)): put in the factory's models under its name
	virtual void addToFactory();

	std::string mName;                                           // +0x0c
	std::string mSource;                                         // +0x28: the image's name, else the animation's folder
	bool mAnimated;                                              // +0x44: no image given
	int mSize;                                                   // +0x48: the model's natural size
	std::map<std::string, engine::Ref<Shape>> mShapes;           // +0x4c: by role
	std::map<std::string, engine::Vector2f> mAnchors;            // +0x58: points by role
};

// A <fishModel> (0xb0 bytes, vtable 0x5506e4 (feedingfrenzy2); its constructor 0x4777c0 (feedingfrenzy2))
class FishModelInfo : public ModelInfo
{
public:
	FishModelInfo()
		: mEatFrame(0), mSuckFrame(0), mSwimShuttle(0), mIdleShuttle(0), mForward(0, 0), mOrientation(0),
		  mStunScale(0)
	{
	}

	// 0x04 (0x45a6b0 (feedingfrenzy2))
	void load(engine::Params& theAttributes) override;
	// 0x08 (0x423a00 (feedingfrenzy2)): put in the factory's fish models
	void addToFactory() override;

	std::string mMenu;              // +0x64: the fish's picture on the menus
	int mEatFrame;                  // +0x80: the chomp's frame of the eat animation
	int mSuckFrame;                 // +0x84: the eat animation's frame held while sucking
	engine::IntRange mTurnSafe;     // +0x88: the turn animation's frames eating does not interrupt
	int mSwimShuttle;               // +0x90
	int mIdleShuttle;               // +0x94
	engine::Vector2f mForward;      // +0x98: the direction in the world taken as forward
	float mOrientation;             // +0xa0: the image's direction against forward, 1 or -1
	float mStunScale;               // +0xa4
};

// A visual's colour effect (VisualInfo's +0x2c), by the colorEffect attribute in any case
enum ColorEffect
{
	COLOR_EFFECT_NONE = 0,         // "none", and any name not below
	COLOR_EFFECT_HSV_ADD = 1,      // "hsvadd"
	COLOR_EFFECT_RGB_ADD = 2,      // "rgbadd"
	COLOR_EFFECT_RGB_MULTIPLY = 3, // "rgbmultiply"
	COLOR_EFFECT_RGB_BLEND = 4,    // "rgbblend"
	COLOR_EFFECT_RAINBOW = 5       // "rainbow"
};

// A <visual> (0x48 bytes, vtable 0x5506c0 (feedingfrenzy2); its constructor 0x478b30 (feedingfrenzy2))
class VisualInfo : public engine::Object
{
public:
	VisualInfo() : mColorEffect(COLOR_EFFECT_NONE)
	{
		for (int i = 0; i < 4; i++)
			mColorParams[i] = 0.0f;
	}

	// 0x04 (0x45b3b0 (feedingfrenzy2); a fish visual's 0x47aa30 (feedingfrenzy2) jumps here)
	virtual void load(engine::Params& theAttributes);
	// 0x08 (0x410110 (feedingfrenzy2)): the model of the name, looked for among the models and then the fish models
	virtual void setModel(const std::string& theName);
	// 0x0c (0x40f270 (feedingfrenzy2)): put in the factory's visuals under its name
	virtual void addToFactory();

	std::string mName;                       // +0x0c
	engine::Ref<ModelInfo> mModel;           // +0x28
	int mColorEffect;                        // +0x2c: a ColorEffect
	float mColorParams[4];                   // +0x30: the effect's four numbers
};

// A <fishVisual> (0x4c bytes, vtable 0x550704 (feedingfrenzy2); its constructor 0x478bc0 (feedingfrenzy2))
class FishVisualInfo : public VisualInfo
{
public:
	// 0x08 (0x423a80 (feedingfrenzy2)): the fish model of the name, and the visual's model the same
	void setModel(const std::string& theName) override;
	// 0x0c (0x423a60 (feedingfrenzy2)): put in the factory's fish visuals
	void addToFactory() override;

	engine::Ref<FishModelInfo> mFishModel;  // +0x40
};

// A <behavior> (0x48 bytes, vtable 0x54f4b4 (feedingfrenzy2); its constructor 0x426a90 (feedingfrenzy2)): a set of
// parameters, every attribute of its element, with what all behaviours share read out
class BehaviorInfo : public engine::Params
{
public:
	BehaviorInfo() : mInAir(false) {}

	// 0x04 (0x472e60 (feedingfrenzy2)): the attributes added to its own, then read
	void load(engine::Params& theAttributes)
	{
		merge(theAttributes);
		read();
	}
	// 0x08 (0x45ec60 (feedingfrenzy2))
	virtual void read();

	std::string mName;        // +0x18: "NONAME" when the element has none
	bool mInAir;              // +0x34
	engine::IntRange mDepth;  // +0x38: from the depth attribute, or the altitude's negated; with neither, from
	                          // INT_MAX down to INT_MIN
};

// A <react> in a <fishBehavior> (FishBehaviorInfo::FishReaction, 0x50 bytes; its constructor 0x46e080
// (feedingfrenzy2)): what the fish does on seeing an actor of a class, and how often
class FishReaction : public engine::Object
{
public:
	FishReaction(const std::string& theClass, const std::string& theReaction, float theFrequency)
		: mClass(theClass), mReaction(theReaction), mFrequency(theFrequency)
	{
	}

	std::string mClass;     // +0x0c
	std::string mReaction;  // +0x28
	float mFrequency;       // +0x44
};

// A <fishBehavior> (0x12c bytes, vtable 0x550528 (feedingfrenzy2); its constructor 0x46d250 (feedingfrenzy2)).
// Speeds are in pixels a second and rates in a second; a frequency is the seconds between two changes, drawn
// between the range's ends.
class FishBehaviorInfo : public BehaviorInfo
{
public:
	FishBehaviorInfo()
		: mSightDistance(0), mSightCos(0), mAccelRate(0), mRotationRate(0), mDragRate(0), mFluidDrag(0),
		  mSightCheckFrequency(0), mSchoolSeparationDistance(0), mSchoolNeighborDistance(0),
		  mSchoolSeparationFactor(0), mSchoolAlignmentFactor(0), mSchoolCohesionFactor(0), mSchoolGoalFactor(0),
		  mSchoolFleeFactor(0), mSchoolChaseMultiplier(0)
	{
	}

	// 0x08 (0x4526e0 (feedingfrenzy2))
	void read() override;

	std::string mClass;                                     // +0x40: "NOCLASS" when the element has none
	std::string mBaseBehavior;                              // +0x5c: "wander" by default
	engine::FloatRange mMaxSpeedX;                          // +0x78
	engine::FloatRange mMaxSpeedY;                          // +0x80
	engine::FloatRange mMinSpeedX;                          // +0x88
	engine::FloatRange mMinSpeedY;                          // +0x90
	float mSightDistance;                                   // +0x98
	float mSightCos;                                        // +0x9c: the cosine of the sight's angle
	float mAccelRate;                                       // +0xa0
	float mRotationRate;                                    // +0xa4: in radians a second (the file's in degrees)
	float mDragRate;                                        // +0xa8
	float mFluidDrag;                                       // +0xac
	engine::FloatRange mWanderMoveChangeFrequency;          // +0xb0
	engine::FloatRange mWanderStateChangeFrequency;         // +0xb8
	engine::FloatRange mChaseMoveChangeFrequency;           // +0xc0
	engine::FloatRange mChaseStateChangeFrequency;          // +0xc8
	engine::FloatRange mRunMoveChangeFrequency;             // +0xd0
	engine::FloatRange mRunStateChangeFrequency;            // +0xd8
	float mSightCheckFrequency;                             // +0xe0
	engine::IntRange mSchoolSize;                           // +0xe4
	float mSchoolSeparationDistance;                        // +0xec
	float mSchoolNeighborDistance;                          // +0xf0
	float mSchoolSeparationFactor;                          // +0xf4
	float mSchoolAlignmentFactor;                           // +0xf8
	float mSchoolCohesionFactor;                            // +0xfc
	float mSchoolGoalFactor;                                // +0x100
	float mSchoolFleeFactor;                                // +0x104
	float mSchoolChaseMultiplier;                           // +0x108
	engine::FloatRange mSchoolMoveChangeFrequency;          // +0x10c
	std::vector<engine::Ref<FishReaction>> mReactions;      // +0x114: in the file's order
};

// ModelInfo::DocHandler: a <model> or a <fishModel>, its <shape>s and <anchor>s
class ModelInfoDocHandler : public engine::SubDocHandler
{
public:
	explicit ModelInfoDocHandler(ModelInfo* theInfo) : mInfo(theInfo) {}

	// 0x04 of its own table (0x473d20 (feedingfrenzy2)): the model's attributes read
	void begin(engine::Params& theAttributes) override { mInfo->load(theAttributes); }
	// 0x00 (0x451eb0 (feedingfrenzy2))
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	// 0x04 (0x461df0 (feedingfrenzy2)): at its own element's end, the model put in the factory
	void endElement(const std::string& theName) override
	{
		if (theName == getRootName())
			mInfo->addToFactory();
	}

private:
	engine::Ref<ModelInfo> mInfo;  // +0x50
};

// VisualInfo::DocHandler: a <visual> or a <fishVisual>
class VisualInfoDocHandler : public engine::SubDocHandler
{
public:
	explicit VisualInfoDocHandler(VisualInfo* theInfo) : mInfo(theInfo) {}

	// 0x04 of its own table (0x473d20 (feedingfrenzy2))
	void begin(engine::Params& theAttributes) override { mInfo->load(theAttributes); }
	// 0x00 (0x502be0 (feedingfrenzy2)): nothing
	void startElement(const std::string& theName, engine::Params& theAttributes) override {}
	// 0x04 (0x424dc0 (feedingfrenzy2)): at its own element's end, the visual put in the factory
	void endElement(const std::string& theName) override
	{
		if (theName == getRootName())
			mInfo->addToFactory();
	}

private:
	engine::Ref<VisualInfo> mInfo;  // +0x50
};

// FishBehaviorInfo::DocHandler: a <fishBehavior> and its <react>s
class FishBehaviorInfoDocHandler : public engine::SubDocHandler
{
public:
	// 0x04 of its own table (0x4741a0 (feedingfrenzy2)): a new fish behaviour read and put in the factory
	void begin(engine::Params& theAttributes) override;
	// 0x00 (0x45bec0 (feedingfrenzy2))
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	// 0x04 (0x5014fa (feedingfrenzy2)): nothing
	void endElement(const std::string& theName) override {}

private:
	engine::Ref<FishBehaviorInfo> mInfo;  // +0x50
};

#endif
