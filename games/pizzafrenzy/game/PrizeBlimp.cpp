#include "PrizeBlimp.h"
#include "App.h"
#include "CityMap.h"
#include "Color4.h"
#include "Font.h"
#include "Host.h"
#include "ImageNode.h"
#include "Modifiers.h"
#include "PizzaFrenzy.h"
#include "Players.h"
#include "Random.h"
#include "ScreenNodes.h"
#include "Tip.h"

#include <algorithm>

using namespace pizzafrenzy;

namespace
{
const char* const BLIMP_TYPE = "PrizeBlimp";
const char* const VEHICLE_CLASS = "vehicle";
const char* const SHADOW_IMAGE = "res\\vehicles\\blimp_dropshadow.jpg";
const char* const ENGINE_SOUND = "bonus_blimpEngine";
const char* const DROP_SOUND = "bonus_blimpDropTip";

// the shadow hangs below and to the left of the blimp, and both rock over six seconds
const float SHADOW_X = -67.0f;
const float SHADOW_Y = 113.0f;
const float ROCK_SECONDS = 6.0f;
const float BLIMP_ROCK_SIZE = 15.0f;
const float SHADOW_ROCK_X = -15.0f;
const float SHADOW_ROCK_Y = -30.0f;
const int SMOOTH_TYPE = 1;

// the blimp is let go when it is this far past the screen's edge
const float OFF_SCREEN_X = 100.0f;
const float OFF_SCREEN_Y = 50.0f;

// the tips come up in a column under the blimp's middle, and the blimp drops them over the middle of the screen
const float DROP_LEFT = 50.0f;
const float DROP_RIGHT = 650.0f;
const float DROP_OFFSET_X = -15.0f;
const float DROP_OFFSET_Y = 45.0f;
const int DROP_COUNT = 3;

// the dot matrix sign under the blimp: string 0x69 with the player's name, in capitals, white on black in the font, seen
// through a window from 82 left of the blimp's position to 61 right of it, 17 above; the text crosses it from the right
// edge every eight seconds
const int STRING_SIGN = 0x69;
const char* const SIGN_FONT = "res\\fonts\\dotMatrix.xml";
const int SIGN_LEFT = -82;
const int SIGN_RIGHT = 61;
const float SIGN_Y = -17.0f;
const float SIGN_SECONDS = 8.0f;
const uint8_t SIGN_ALPHA_THRESHOLD = 0xff;

// the seconds between drops, a range (0x53290c, set by the static initializer 0x4f87c0)
const engine::Point DROP_DELAY(1.0f, 1.75f);
}

// 0x431330: the pictures and the engine are made by the slots
PrizeBlimp::PrizeBlimp() : Van(BLIMP_TYPE), mDropTimer(0), mEngine(nullptr), mFlownOff(false), mSign(nullptr),
	mSignTimer(0)
{
}

namespace
{
// 0x46dc60: the text in capitals, as CharUpperA makes them in the Western code page
std::string toCapitals(std::string theText)
{
	for (char& c : theText)
	{
		unsigned char aChar = (unsigned char)c;
		if ((aChar >= 'a' && aChar <= 'z') || (aChar >= 0xe0 && aChar <= 0xfe && aChar != 0xf7))
			aChar = (unsigned char)(aChar - 0x20);
		else if (aChar == 0x9a || aChar == 0x9c || aChar == 0x9e)
			aChar = (unsigned char)(aChar - 0x10);
		else if (aChar == 0xff)
			aChar = 0x9f;
		c = (char)aChar;
	}
	return theText;
}
}

// 0x430d50
void PrizeBlimp::loadType()
{
	Vehicle* aType = static_cast<Vehicle*>(PizzaFrenzy::instance()->mTileManifest->getTile(getName(), VEHICLE_CLASS));
	if (aType == nullptr)
		return;
	engine::Image* aShadow = engine::App::instance()->createImage(SHADOW_IMAGE, false);
	aShadow->setPivot(1);
	engine::ImageNode* aShadowNode = new engine::ImageNode(aShadow);
	aShadowNode->setPosition(SHADOW_X, SHADOW_Y);
	addChild(aShadowNode);
	makeImages(aType);
	mLayer = LAYER_AIR;  // above the vehicles on the ground (GameMode::addVehicleNode)
	mSpeed = (float)aType->mSpeed;
	setSmoothType(SMOOTH_TYPE);
	addListener(new engine::WaveModifier(0.0f, ROCK_SECONDS, 0.0f, BLIMP_ROCK_SIZE, 0.0f));
	aShadowNode->addListener(new engine::WaveModifier(ROCK_SECONDS, ROCK_SECONDS, SHADOW_ROCK_X, SHADOW_ROCK_Y, 0.0f));

	// the sign: the text drawn once on a black picture, which the node shows through its clip (scrollSign)
	engine::UserInfo* aUser = engine::Players::instance()->current();
	std::string aText = toCapitals(formatMessage(engine::App::instance()->loadString(STRING_SIGN),
		aUser != nullptr ? aUser->getName() : std::string()));
	engine::Font* aFont = engine::App::instance()->getFont(SIGN_FONT);
	if (aFont == nullptr)
		return;
	engine::Host* aHost = engine::getHost();
	engine::Image* aSignImage = aHost->createBlankImage(aFont->getStringWidth(aText.c_str()), aFont->getHeight());
	if (aSignImage == nullptr)
		return;
	engine::Graphics* aGraphics = aHost->createImageGraphics(aSignImage);
	aSignImage->fill(engine::Color4(0, 0, 0, 1.0f));
	aSignImage->setAlphaMode(engine::ALPHA_BLEND, SIGN_ALPHA_THRESHOLD);
	if (aGraphics != nullptr)
	{
		aGraphics->setFont(aFont);
		aGraphics->drawString(aText.c_str(), 0.0f, 0.0f);
		delete aGraphics;
	}
	mSign = new engine::ImageNode(aSignImage);
	mSign->setClipped(true);
	addChild(mSign);
	mSignTimer = 0;
	// (the original also makes a balloon tip here and lets it go at once)
}

// 0x430b90: the sign's text moves on; once a pass's eight seconds are up the next starts at the window's right edge
void PrizeBlimp::scrollSign(engine::UpdateInfo& theInfo)
{
	float aTimer = mSignTimer - theInfo.mSeconds;
	mSignTimer = aTimer;
	if (aTimer < 0.0f)
		mSignTimer = SIGN_SECONDS;
	if (mSign == nullptr)
		return;
	float aProgress = (float)(1.0 - (double)mSignTimer * (1.0 / SIGN_SECONDS));
	int aLeft = SIGN_RIGHT - (int)((double)(mSign->getImageWidth() + SIGN_RIGHT - SIGN_LEFT) * aProgress);
	mSign->setPosition((float)std::max(aLeft, SIGN_LEFT), SIGN_Y);
	int aClipLeft = std::max(0, SIGN_LEFT - aLeft);
	int aClipRight = std::min(mSign->getImageWidth(), SIGN_RIGHT - aLeft);
	mSign->setClipRect(engine::Rect((float)aClipLeft, 0.0f, (float)aClipRight, (float)mSign->getImageHeight()));
}

// 0x430cc0
void PrizeBlimp::updateVehicle(engine::UpdateInfo& theInfo)
{
	fly(theInfo);
	scrollSign(theInfo);
	Container::update(theInfo);
	float aTimer = mDropTimer;
	mDropTimer = aTimer - theInfo.mSeconds;
	if (aTimer - theInfo.mSeconds < 0.0f && !mFlownOff)
	{
		float aX = getPosition()->x;
		if (DROP_LEFT < aX && aX < DROP_RIGHT)
		{
			dropTips();
			mDropTimer = engine::randomFloat(DROP_DELAY);
		}
	}
}

// 0x430970
void PrizeBlimp::fly(engine::UpdateInfo& theInfo)
{
	int aRowBefore = (int)getY();
	engine::Point aPosition = *getPosition();
	aPosition.x = mVelocity.x * theInfo.mSeconds + aPosition.x;
	aPosition.y = mVelocity.y * theInfo.mSeconds + aPosition.y;
	setPosition(aPosition);
	if ((int)getY() != aRowBefore)
		mDirty = true;
	if (((float)engine::SCREEN_WIDTH + OFF_SCREEN_X < aPosition.x && 0.0f < mVelocity.x)
	    || (aPosition.x < -OFF_SCREEN_X && mVelocity.x < 0.0f)
	    || ((float)engine::SCREEN_HEIGHT + OFF_SCREEN_Y < aPosition.y && 0.0f < mVelocity.y)
	    || (aPosition.y < -OFF_SCREEN_Y && mVelocity.y < 0.0f))
	{
		leave();
		if (mEngine != nullptr)
			mEngine->stop();
		mFlownOff = true;
	}
}

// 0x430b30: the picture is the first, turned round when the blimp flies to the right
void PrizeBlimp::face(const engine::IntPair& theCell)
{
	mImages->select(0, 0.0f, true);
	float aScaleX = 1.0f;
	if (0.0f < mVelocity.x)
		aScaleX = -1.0f;
	setScale(aScaleX, 1.0f);
}

// 0x431110: a home tile of none
void PrizeBlimp::start(CityMap* theMap, const engine::Point& thePosition)
{
	mHome = nullptr;
	mMap = theMap;
	layout();
	setPosition(thePosition.x, thePosition.y);
	theMap->addVehicle(this);
	mDirty = false;
	mDropTimer = 0;
	mEngine = PizzaFrenzy::instance()->mSounds->getVoice(ENGINE_SOUND);
	if (mEngine != nullptr)
	{
		mEngine->setLoop(true);
		mEngine->play();
	}
	mFlownOff = false;
}

// 0x430ac0
void PrizeBlimp::setDirection(const engine::Point& theDirection)
{
	mVelocity.x = theDirection.x * mSpeed;
	mVelocity.y = theDirection.y * mSpeed;
	face(engine::IntPair{0, 0});
}

// 0x431220: each tip is dropped at the same place, 15 left of the blimp's position and 45 under it
void PrizeBlimp::dropTips()
{
	engine::Point aPoint = *getPosition();
	aPoint.x = aPoint.x + DROP_OFFSET_X;
	aPoint.y = aPoint.y + DROP_OFFSET_Y;
	for (int i = 0; i < DROP_COUNT; i++)
		PizzaFrenzy::instance()->mMode->addTip(nullptr, aPoint, new BalloonTip());
	PizzaFrenzy::instance()->mSounds->play(DROP_SOUND, 1.0f, 1.0f);
}
