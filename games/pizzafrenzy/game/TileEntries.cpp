#include "TileEntries.h"
#include "sprout/App.h"
#include "sprout/Host.h"

using namespace pizzafrenzy;

namespace
{
const int DEFAULT_TILE_LAYER = 3;
const float TOPPING_SCALE = 0.55f;
const float TOPPING_SCALE_AS_PREVIEW = 0.65f;
const int PIVOT_CENTRE = 1;
const float PREVIEW_OFFSET_Y = 3.0f;
const char* const CHEESE_PIZZA_IMAGE = "res\\pizza\\cheesePizza.jpg";
}

// 0x41d7b0
TileType::TileType(engine::Image* theImage, int theLayer) :
	mBottomRightOffset(0, 0), mBase(0, 0), mImage(theImage), mLayer(theLayer), mDimensions(1, 1)
{
	if (theImage != nullptr)
		theImage->setPivot(PIVOT_CENTRE);
}

// 0x41d8c0
KitchenTileType::KitchenTileType(engine::Image* theImage, engine::Point theParkingSpot) :
	TileType(theImage, DEFAULT_TILE_LAYER), mParkingSpot(theParkingSpot)
{
}

// 0x41ea20
CustomerTileType::CustomerTileType(engine::Image* theImage, engine::Point theParkingSpot,
                                   const std::string& theCharacterName) :
	TileType(theImage, DEFAULT_TILE_LAYER), mParkingSpot(theParkingSpot), mCharacterName(theCharacterName)
{
}

// 0x41d990
BridgeTileType::BridgeTileType(engine::Image* theImage, engine::Image* theRoadImage, engine::Image* theUnblockedImage,
                               engine::Image* theUnblockedRoadImage) :
	TileType(theImage, DEFAULT_TILE_LAYER),
	mRoadImage(theRoadImage), mUnblockedImage(theUnblockedImage), mUnblockedRoadImage(theUnblockedRoadImage)
{
	if (theRoadImage != nullptr)
		theRoadImage->setPivot(PIVOT_CENTRE);
	if (theUnblockedImage != nullptr)
		theUnblockedImage->setPivot(PIVOT_CENTRE);
	if (theUnblockedRoadImage != nullptr)
		theUnblockedRoadImage->setPivot(PIVOT_CENTRE);
}

// 0x41eb20
Vehicle::Vehicle(engine::Image* theNorthImage, engine::Image* theSouthImage, engine::Image* theEastWestImage,
                 const std::string& theDescription, const std::string& theClass, const std::string& theEffect) :
	TileType(theEastWestImage, DEFAULT_TILE_LAYER),
	mDescription(theDescription), mEffect(theEffect), mClass(theClass), mSpeed(0),
	mNorthImage(theNorthImage), mSouthImage(theSouthImage)
{
}

// 0x41db90
void Vehicle::setParkedImages(engine::Image* theParkedImage, engine::Image* theHoverImage)
{
	mParkedImage = theParkedImage;
	mHoverImage = theHoverImage;
	if (theParkedImage != nullptr)
		theParkedImage->setPivot(PIVOT_CENTRE);
	if (theHoverImage != nullptr)
		theHoverImage->setPivot(PIVOT_CENTRE);
}

// 0x41ee20
Character::Character(const std::string& theName, engine::Image* theImage, int theNumPizzas, float thePatience,
                     float theGenerosity, const std::string& theGreeting, const std::string& theSpecial) :
	mImage(theImage), mNumPizzas(theNumPizzas), mPatience(thePatience), mGenerosity(theGenerosity), mName(theName),
	mGreeting(theGreeting), mSpecial(SPECIAL_NONE)
{
	if (theImage != nullptr)
		theImage->setPivot(PIVOT_CENTRE);
	if (theSpecial == "chameleon")
		mSpecial = SPECIAL_CHAMELEON;
	else if (theSpecial == "police")
		mSpecial = SPECIAL_POLICE;
}

// 0x41f470
Upgrade::Upgrade(const std::string& theName, const std::string& theDescription, int theBonusMultiplier, int theCombo) :
	mName(theName), mDescription(theDescription), mBonusMultiplier(theBonusMultiplier), mCombo(theCombo)
{
}

// 0x420a90
Topping::Topping(const std::string& theName, const std::string& theDisplayName, engine::Image* theImage,
                 engine::Image* theCouponImage, int theUsesImageAsPreview) :
	mCouponImage(theCouponImage), mName(theName), mDisplayName(theDisplayName),
	mUsesImageAsPreview(theUsesImageAsPreview)
{
	// port: the toppings made so far number this one (ToppingOrder)
	static int sToppingsMade = 0;
	mMadeOrder = sToppingsMade++;
	theCouponImage->setPivot(PIVOT_CENTRE);
	setImage(theImage);
}

// 0x41dc60: the preview is the scaled topping drawn on a copy of the cheese pizza's image, in its middle and three
// pixels up. The scaled image's pivot is the full-size image's centre scaled with it, which is where it is drawn
// from.
void Topping::setImage(engine::Image* theImage)
{
	mImage = theImage;
	theImage->setPivot(PIVOT_CENTRE);
	if (mUsesImageAsPreview == 0)
	{
		mScaledImage = theImage->createScaled(TOPPING_SCALE, TOPPING_SCALE);
		engine::Image* aCheese = engine::App::instance()->getImage(CHEESE_PIZZA_IMAGE);
		mPreviewImage = aCheese->clone();
		engine::Graphics* aGraphics = engine::getHost()->createImageGraphics(mPreviewImage);
		if (aGraphics != nullptr)
		{
			aGraphics->drawImage(mScaledImage,
				engine::IntRect(0, 0, mScaledImage->getWidth(), mScaledImage->getHeight()),
				(float)aCheese->getWidth() * 0.5f, (float)aCheese->getHeight() * 0.5f - PREVIEW_OFFSET_Y);
			delete aGraphics;
		}
		mPreviewImage->setPivot(PIVOT_CENTRE);
	}
	else
	{
		mScaledImage = theImage->createScaled(TOPPING_SCALE_AS_PREVIEW, TOPPING_SCALE_AS_PREVIEW);
		mPreviewImage = theImage;
	}
	mScaledImage->setPivot(PIVOT_CENTRE);
}

// 0x41f5e0
SpecialEvent::SpecialEvent(const std::string& theCharacter, const std::string& theName, const std::string& theType,
                           engine::Image* thePortrait, const std::string& theSound, const std::string& theDescription,
                           bool theDeliver) :
	mPortrait(thePortrait), mType(theType), mName(theName), mCharacter(theCharacter), mSound(theSound),
	mDescription(theDescription), mDeliver(theDeliver)
{
	if (thePortrait != nullptr)
		thePortrait->setPivot(PIVOT_CENTRE);
}
