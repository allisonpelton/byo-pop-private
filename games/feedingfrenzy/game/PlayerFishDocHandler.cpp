#include "Font.h"
#include "PlayerFishDocHandler.h"
#include "Exception.h"
#include "ImageFilter.h"
#include "JohnDoryPlayer.h"
#include "Layout.h"
#include "PlayerFish.h"
#include "StageMgr.h"

#include <cstdio>

using namespace engine;

namespace feedingfrenzy
{

// What a player's file does not say
const float DEFAULT_MAX_SPEED_X = 145.0f;
const float DEFAULT_MAX_SPEED_Y = 110.0f;
const float DEFAULT_ACCEL_RATE = 550.0f;
const float DEFAULT_DRAG = 150.0f;
const float DEFAULT_FLUID_DRAG = 0.5f;
const float DEFAULT_FRENZY_TIME = 1.1f;
const int DEFAULT_EAT_FINISH_FRAME = 5;
const float DEFAULT_UNSAFE_EAT_LAST_FRAME = 100.0f;
// The kinds of player fish the files name, and the name each is known by to the stage's other fish
const char* const PLAYER_KIND = "PlayerFish";
const char* const JOHN_DORY_KIND = "JohnDoryPlayer";
const char* const PLAYER_ELEMENT = "playerFish";
const char* const MISSING_ANIM_PATH = "BADPATH";
const char* const BAD_CLASS_MESSAGE = "COULDN'T CONSTRUCT PLAYER CLASS";

// The four numbers of a rectangle, as the player's file writes them ("left,top,right,bottom")
static Rect rectFromString(const std::string& theText)
{
	float aLeft = 0.0f, aTop = 0.0f, aRight = 0.0f, aBottom = 0.0f;
	sscanf(theText.c_str(), "%f,%f,%f,%f", &aLeft, &aTop, &aRight, &aBottom);
	return Rect(aLeft, aTop, aRight, aBottom);
}

// 0x416750: a new player fish of the class, with its type name set; null for a name no class has (the classes of the
// registry at 0x416580 that the stages' files never name are left out)
static PlayerFish* createPlayerOfClass(const std::string& theClassName)
{
	if (theClassName == PLAYER_KIND)
	{
		PlayerFish* aPlayer = new PlayerFish();
		aPlayer->setTypeName(PLAYER_KIND);
		return aPlayer;
	}
	if (theClassName == JOHN_DORY_KIND)
	{
		PlayerFish* aPlayer = new JohnDoryPlayer();
		aPlayer->setTypeName(JOHN_DORY_KIND);
		return aPlayer;
	}
	return nullptr;
}

// 0x4182a0
void PlayerFishDocHandler::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (theName == PLAYER_ELEMENT)
		readPlayer(theAttributes);
}

// 0x417530
void PlayerFishDocHandler::readPlayer(const XmlAttributes& theAttributes)
{
	std::string aClass = getStringAttribute(theAttributes, "class", PLAYER_KIND);
	PlayerFish* aPlayer = createPlayerOfClass(aClass);
	if (aPlayer == nullptr)
		throw Exception(BAD_CLASS_MESSAGE);

	float aMaxSpeedX = getFloatAttribute(theAttributes, "maxSpeedX", DEFAULT_MAX_SPEED_X);
	float aMaxSpeedY = getFloatAttribute(theAttributes, "maxSpeedY", DEFAULT_MAX_SPEED_Y);
	float anAccelRate = getFloatAttribute(theAttributes, "accelRate", DEFAULT_ACCEL_RATE);
	float aDrag = getFloatAttribute(theAttributes, "drag", DEFAULT_DRAG);
	float aFluidDrag = getFloatAttribute(theAttributes, "fluidDrag", DEFAULT_FLUID_DRAG);
	float aFrenzyTime = getFloatAttribute(theAttributes, "frenzyTime", DEFAULT_FRENZY_TIME);
	aPlayer->setFrenzyTime(aFrenzyTime);
	aPlayer->setSpeeds(aMaxSpeedX, aMaxSpeedY, 0.0f, 0.0f, anAccelRate, aDrag);
	aPlayer->setFluidDrag(aFluidDrag);

	// The colour shift: hue in degrees, then the saturation and the value
	float aHue = 0.0f, aSaturation = 0.0f, aValue = 0.0f;
	sscanf(getStringAttribute(theAttributes, "hsvOffset", "0,0,0").c_str(), "%f,%f,%f", &aHue, &aSaturation, &aValue);
	if (aHue != 0.0f || aSaturation != 0.0f || aValue != 0.0f)
	{
		HsvShiftFilter* aShift = new HsvShiftFilter();
		aShift->setShift(aHue, aSaturation, aValue);
		aPlayer->setColorShift(aShift);
	}

	aPlayer->loadAnimationsOf(getStringAttribute(theAttributes, "animPath", MISSING_ANIM_PATH));

	Rect aBody = rectFromString(getStringAttribute(theAttributes, "hotSpot", "-24,-24,24,24"));
	aPlayer->setBodyRect(aBody.left, aBody.top, aBody.right, aBody.bottom);
	std::string aCollision = getStringAttribute(theAttributes, "collision", "");
	if (!aCollision.empty())
	{
		CollisionRect aHitRect;
		aHitRect.set(rectFromString(aCollision));
		aPlayer->setHitRect(aHitRect);
	}

	aPlayer->setEatFrame((int)getIntAttribute(theAttributes, "eatFinishFrame", DEFAULT_EAT_FINISH_FRAME));
	FloatPair anUnsafeFrames =
	    getFloatRangeAttribute(theAttributes, "unsafeEatFrames", FloatPair{0.0f, DEFAULT_UNSAFE_EAT_LAST_FRAME});
	aPlayer->setUnsafeEatFrames((int)anUnsafeFrames.mFirst, (int)anUnsafeFrames.mSecond);
	mStage->setPlayer(aPlayer);
}

} // namespace feedingfrenzy
