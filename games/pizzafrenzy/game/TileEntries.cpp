#include "TileEntries.h"

using namespace pizzafrenzy;

namespace
{
const int DEFAULT_TILE_LAYER = 3;
const float TOPPING_SCALE = 0.55f;
const float TOPPING_SCALE_AS_PREVIEW = 0.65f;
const int PIVOT_CENTRE = 1;
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
                 engine::Image* theCouponImage) :
	mCouponImage(theCouponImage), mName(theName), mDisplayName(theDisplayName), mUsesImageAsPreview(0)
{
	setImage(theImage);
}

// 0x41dc60: the preview is the topping drawn on a copy of the cheese pizza's image (res\pizza\cheesePizza.jpg), which
// the port's image has no copy and no drawing on an image for, so the preview is left out
void Topping::setImage(engine::Image* theImage)
{
	mImage = theImage;
	if (theImage != nullptr)
		theImage->setPivot(PIVOT_CENTRE);
	if (mUsesImageAsPreview == 0)
	{
		if (theImage != nullptr)
			mScaledImage = theImage->createScaled(TOPPING_SCALE, TOPPING_SCALE);
	}
	else
	{
		if (theImage != nullptr)
			mScaledImage = theImage->createScaled(TOPPING_SCALE_AS_PREVIEW, TOPPING_SCALE_AS_PREVIEW);
		mPreviewImage = theImage;
	}
	if (mScaledImage != nullptr)
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
