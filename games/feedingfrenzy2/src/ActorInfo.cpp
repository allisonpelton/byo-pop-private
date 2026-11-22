#include "ActorInfo.h"

#include "Engine.h"
#include "FishFactory.h"

#include <climits>
#include <cmath>
#include <cstdio>

using namespace engine;

namespace
{

// The degrees in a radian, a float (0x5ac6a8 (feedingfrenzy2), set at the start from the double pi at 0x5a72f8
// (feedingfrenzy2) over 180)
const float DEGREES_TO_RADIANS = (float)(3.141592653589793 / 180.0);

} // namespace

std::string toLower(const std::string& theString)
{
	std::string aLower = theString;
	for (size_t i = 0; i < aLower.size(); i++)
	{
		if (aLower[i] >= 'A' && aLower[i] <= 'Z')
			aLower[i] = (char)(aLower[i] - 'A' + 'a');
	}
	return aLower;
}

void ModelInfo::load(Params& theAttributes)
{
	mName = theAttributes.getString("name", "");
	std::string anImage = theAttributes.getString("image", "");
	std::string anAnimation = theAttributes.getString("anim", "");
	mAnimated = anImage.empty();
	mSource = anImage.empty() ? anAnimation : anImage;
	mSize = theAttributes.getInt("size", 0);
}

void ModelInfo::addToFactory()
{
	FishFactory::get()->mModels[mName] = this;
}

void FishModelInfo::load(Params& theAttributes)
{
	ModelInfo::load(theAttributes);
	mMenu = theAttributes.getString("menu", "");
	mEatFrame = theAttributes.getInt("eatFrame", 0);
	mSuckFrame = theAttributes.getInt("suckFrame", 0);
	mTurnSafe = theAttributes.getIntRange("turnSafe", IntRange(2, 3));
	mSwimShuttle = theAttributes.getInt("swimShuttle", 0);
	mIdleShuttle = theAttributes.getInt("idleShuttle", 0);
	mForward = theAttributes.getFloatPair("forward", Vector2f(1.0f, 0.0f));
	mOrientation = theAttributes.getFloat("orientation", -1.0f);
	mStunScale = theAttributes.getFloat("stunScale", 1.0f);
	// The older way of giving the mouth, kept as the anchor of that name: whole numbers, as floats
	if (theAttributes.has("mouth"))
	{
		Vector2<int> aMouth = theAttributes.getIntPair("mouth", Vector2<int>(0, 0));
		mAnchors["mouth"] = Vector2f((float)aMouth.x, (float)aMouth.y);
	}
}

void FishModelInfo::addToFactory()
{
	FishFactory::get()->mFishModels[mName] = this;
}

void VisualInfo::load(Params& theAttributes)
{
	mName = theAttributes.getString("name", "");
	// The model's name, the visual's own when the element gives none
	setModel(theAttributes.getString("model", mName));
	std::string anEffect = toLower(theAttributes.getString("colorEffect", "none"));
	std::string aParams = theAttributes.getString("colorParams", "0,0,0,0");
	if (anEffect == "hsvadd")
		mColorEffect = COLOR_EFFECT_HSV_ADD;
	else if (anEffect == "rgbadd")
		mColorEffect = COLOR_EFFECT_RGB_ADD;
	else if (anEffect == "rgbmultiply")
		mColorEffect = COLOR_EFFECT_RGB_MULTIPLY;
	else if (anEffect == "rgbblend")
		mColorEffect = COLOR_EFFECT_RGB_BLEND;
	else if (anEffect == "rainbow")
		mColorEffect = COLOR_EFFECT_RAINBOW;
	else
		mColorEffect = COLOR_EFFECT_NONE;
	sscanf(aParams.c_str(), " %f , %f , %f, %f ", &mColorParams[0], &mColorParams[1], &mColorParams[2],
		&mColorParams[3]);
}

void VisualInfo::setModel(const std::string& theName)
{
	mModel = FishFactory::get()->findAnyModel(theName);
}

void VisualInfo::addToFactory()
{
	FishFactory::get()->mVisuals[mName] = this;
}

void FishVisualInfo::setModel(const std::string& theName)
{
	FishModelInfo* aModel = FishFactory::get()->findFishModel(theName);
	if (aModel == nullptr)
	{
		// 0x517f01 (feedingfrenzy2): a message box in the original, as above
		getEngine()->log("Bad Name: There is no such fishModel as %s.\n", theName.c_str());
	}
	mFishModel = aModel;
	mModel = mFishModel.get();
}

void FishVisualInfo::addToFactory()
{
	FishFactory::get()->mFishVisuals[mName] = this;
}

void BehaviorInfo::read()
{
	mName = getString("name", "NONAME");
	mInAir = getBool("inAir", false);
	mDepth = IntRange(INT_MAX, INT_MIN);
	if (has("altitude"))
	{
		// Heights above the surface, as depths
		IntRange anAltitude = getIntRange("altitude", mDepth);
		mDepth = IntRange(-anAltitude.mHigh, -anAltitude.mLow);
	}
	else if (has("depth"))
		mDepth = getIntRange("depth", mDepth);
}

void FishBehaviorInfo::read()
{
	BehaviorInfo::read();
	mClass = getString("class", "NOCLASS");
	mBaseBehavior = getString("baseBehavior", "wander");
	mMaxSpeedX = getRange("maxSpeedX", FloatRange(40.0f, 80.0f));
	mMaxSpeedY = getRange("maxSpeedY", FloatRange(40.0f, 80.0f));
	mMinSpeedX = getRange("minSpeedX", FloatRange(0.0f, 0.0f));
	mMinSpeedY = getRange("minSpeedY", FloatRange(0.0f, 0.0f));
	mSightDistance = getFloat("sightDist", 80.0f);
	// The angle in degrees, its radians rounded to a float, and their cosine
	float aSightAngle = (float)((double)DEGREES_TO_RADIANS * (double)getFloat("sightAngle", 30.0f));
	mSightCos = (float)cos((double)aSightAngle);
	mAccelRate = getFloat("accelRate", 400.0f);
	mRotationRate = (float)((double)DEGREES_TO_RADIANS * (double)getFloat("rotRate", 50.0f));
	mDragRate = getFloat("dragRate", 70.0f);
	mFluidDrag = getFloat("fluidDrag", 0.0001f);
	mWanderMoveChangeFrequency = getRange("wanderMoveChangeFreq", FloatRange(2.0f, 6.0f));
	mWanderStateChangeFrequency = getRange("wanderStateChangeFreq", FloatRange(5.0f, 10.0f));
	mChaseMoveChangeFrequency = getRange("chaseMoveChangeFreq", FloatRange(0.05f, 0.5f));
	mChaseStateChangeFrequency = getRange("chaseStateChangeFreq", FloatRange(5.0f, 15.0f));
	mRunMoveChangeFrequency = getRange("runMoveChangeFreq", FloatRange(0.5f, 3.0f));
	mRunStateChangeFrequency = getRange("runStateChangeFreq", FloatRange(1.0f, 5.0f));
	mSightCheckFrequency = getFloat("sightCheckFreq", 0.1f);
	mSchoolSize = getIntRange("schoolSize", IntRange(1, 1));
	mSchoolSeparationDistance = getFloat("schoolSeparationDist", 20.0f);
	mSchoolNeighborDistance = getFloat("schoolNeighborDist", 80.0f);
	mSchoolSeparationFactor = getFloat("schoolSeparationFactor", 1.2f);
	mSchoolCohesionFactor = getFloat("schoolCohesionFactor", 0.5f);
	mSchoolAlignmentFactor = getFloat("schoolAlignmentFactor", 0.95f);
	mSchoolGoalFactor = getFloat("schoolGoalFactor", 0.5f);
	mSchoolFleeFactor = getFloat("schoolFleeFactor", 2.0f);
	mSchoolChaseMultiplier = getFloat("schoolChaseMult", 1.0f);
	mSchoolMoveChangeFrequency = getRange("schoolMoveChangeFreq", FloatRange(3.0f, 6.0f));
}

void ModelInfoDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName == "shape")
	{
		std::string aClass = theAttributes.getString("class", "");
		std::string aRole = theAttributes.getString("role", "body");
		Ref<Shape> aShape;
		if (aClass == "box")
		{
			// (The original's default is the new shape's unset value; every box in the file has both)
			shape::Box* aBox = new shape::Box();
			aShape = aBox;
			aBox->mCenter = theAttributes.getFloatPair("center", aBox->mCenter);
			aBox->mDiagonal = theAttributes.getFloatPair("diagonal", aBox->mDiagonal);
		}
		else if (aClass == "circle")
		{
			shape::Circle* aCircle = new shape::Circle();
			aShape = aCircle;
			aCircle->mCenter = theAttributes.getFloatPair("center", aCircle->mCenter);
			aCircle->mRadius = theAttributes.getFloat("radius", aCircle->mRadius);
		}
		if (aShape != nullptr && !aRole.empty())
			mInfo->mShapes[aRole] = aShape;
	}
	else if (theName == "anchor")
	{
		std::string aRole = theAttributes.getString("role", "");
		Vector2f aLocation = theAttributes.getFloatPair("location", Vector2f(0.0f, 0.0f));
		if (!aRole.empty())
			mInfo->mAnchors[aRole] = aLocation;
	}
}

void FishBehaviorInfoDocHandler::begin(Params& theAttributes)
{
	mInfo = new FishBehaviorInfo();
	mInfo->load(theAttributes);
	FishFactory::get()->mFishBehaviors[mInfo->mName] = mInfo;
}

void FishBehaviorInfoDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName != "react")
		return;
	std::string aClass = theAttributes.getString("class", "");
	std::string aReaction = theAttributes.getString("reaction", "");
	float aFrequency = theAttributes.getFloat("reactFreq", 0.0f);
	mInfo->mReactions.push_back(new FishReaction(aClass, aReaction, aFrequency));
}
