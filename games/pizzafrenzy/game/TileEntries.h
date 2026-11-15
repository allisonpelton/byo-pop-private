#ifndef __PIZZAFRENZY_TILEENTRIES_H__
#define __PIZZAFRENZY_TILEENTRIES_H__

// The entries of the tile manifest (TileManifest.h): the tiles of the map and the vehicles, the customers, the
// toppings with their upgrades, the particle effects, the special events and the score titles. Each is made from an
// element of res\manifests\tileManifest.xml by the manifest's handler (TileManifest.cpp).

#include "Font.h"
#include "Geometry.h"
#include "Graphics.h"
#include "Object.h"

#include <string>
#include <vector>

namespace pizzafrenzy
{

// A tile of the map (0x34 bytes, constructor 0x41d7b0): its image, centred on its pivot, and the layer it is drawn in
class TileType : public engine::Object
{
public:
	TileType(engine::Image* theImage, int theLayer);  // 0x41d7b0

	engine::Point mBottomRightOffset;  // +0x0c
	engine::Point mBase;               // +0x14
	engine::Ref<engine::Image> mImage;  // +0x1c
	int mLayer;                        // +0x20
	engine::Point mDimensions;         // +0x24: the tiles it covers
};

// The kitchen's tile (0x44 bytes, constructor 0x41d8c0)
class KitchenTileType : public TileType
{
public:
	KitchenTileType(engine::Image* theImage, engine::Point theParkingSpot);  // 0x41d8c0

	engine::Point mParkingSpot;  // +0x2c
};

// A customer's house (0x58 bytes, constructor 0x41ea20)
class CustomerTileType : public TileType
{
public:
	// 0x41ea20
	CustomerTileType(engine::Image* theImage, engine::Point theParkingSpot, const std::string& theCharacterName);

	engine::Point mParkingSpot;  // +0x2c
	std::string mCharacterName;  // +0x34
};

// A bridge (0x40 bytes, constructor 0x41d990): its road, and both with the way unblocked
class BridgeTileType : public TileType
{
public:
	BridgeTileType(engine::Image* theImage, engine::Image* theRoadImage, engine::Image* theUnblockedImage,
	               engine::Image* theUnblockedRoadImage);  // 0x41d990

	engine::Ref<engine::Image> mRoadImage;            // +0x2c
	engine::Ref<engine::Image> mUnblockedImage;       // +0x30
	engine::Ref<engine::Image> mUnblockedRoadImage;   // +0x34
};

// A vehicle (0x9c bytes, constructor 0x41eb20): drawn by its east-west image, with the others for the other ways
class Vehicle : public TileType
{
public:
	Vehicle(engine::Image* theNorthImage, engine::Image* theSouthImage, engine::Image* theEastWestImage,
	        const std::string& theDescription, const std::string& theClass, const std::string& theEffect);  // 0x41eb20
	// 0x41db90
	void setParkedImages(engine::Image* theParkedImage, engine::Image* theHoverImage);

	std::string mDescription;  // +0x2c
	std::string mEffect;       // +0x48
	std::string mClass;        // +0x64
	int mSpeed;                // +0x80
	engine::Ref<engine::Image> mNorthImage;   // +0x84
	engine::Ref<engine::Image> mSouthImage;   // +0x88
	engine::Ref<engine::Image> mParkedImage;  // +0x8c
	engine::Ref<engine::Image> mHoverImage;   // +0x90
};

// A customer (0x60 bytes, constructor 0x41ee20)
class Character : public engine::Object
{
public:
	enum
	{
		SPECIAL_NONE = 0,
		SPECIAL_CHAMELEON = 1,
		SPECIAL_POLICE = 2
	};

	Character(const std::string& theName, engine::Image* theImage, int theNumPizzas, float thePatience,
	          float theGenerosity, const std::string& theGreeting, const std::string& theSpecial);  // 0x41ee20

	engine::Ref<engine::Image> mImage;  // +0x0c
	int mNumPizzas;                     // +0x10
	float mPatience;                    // +0x14
	float mGenerosity;                  // +0x18
	std::string mName;                  // +0x1c
	std::string mGreeting;              // +0x38
	int mSpecial;                       // +0x54
};

// An upgrade of a topping (0x54 bytes, constructor 0x41f470)
class Upgrade : public engine::Object
{
public:
	// 0x41f470
	Upgrade(const std::string& theName, const std::string& theDescription, int theBonusMultiplier, int theCombo);

	std::string mName;         // +0x0c
	std::string mDescription;  // +0x28
	int mBonusMultiplier;      // +0x44
	int mCombo;                // +0x48
};

// A topping (0x70 bytes, constructor 0x420a90)
class Topping : public engine::Object
{
public:
	Topping(const std::string& theName, const std::string& theDisplayName, engine::Image* theImage,
	        engine::Image* theCouponImage);  // 0x420a90
	// 0x41dc60: the image centred, and the smaller ones made from it
	void setImage(engine::Image* theImage);

	engine::Ref<engine::Image> mImage;         // +0x0c
	engine::Ref<engine::Image> mCouponImage;   // +0x10
	engine::Ref<engine::Image> mScaledImage;   // +0x14
	engine::Ref<engine::Image> mPreviewImage;  // +0x18
	std::vector<engine::Ref<Upgrade>> mUpgrades;  // +0x1c
	std::string mName;                         // +0x2c
	std::string mDisplayName;                  // +0x48
	int mUsesImageAsPreview;                   // +0x64 (0)
};

// A special event (0xa8 bytes, constructor 0x41f5e0)
class SpecialEvent : public engine::Object
{
public:
	SpecialEvent(const std::string& theCharacter, const std::string& theName, const std::string& theType,
	             engine::Image* thePortrait, const std::string& theSound, const std::string& theDescription,
	             bool theDeliver);  // 0x41f5e0

	engine::Ref<engine::Image> mPortrait;  // +0x0c
	std::string mType;                     // +0x10
	std::string mName;                     // +0x2c
	std::string mCharacter;                // +0x48
	std::string mSound;                    // +0x64
	std::string mDescription;              // +0x80
	bool mDeliver;                         // +0x9c
};

// A particle effect (0x1c bytes, constructor 0x41de80): the layer it is drawn in
class ParticleEffect : public engine::Object
{
public:
	explicit ParticleEffect(int theLayer) : mLayer(theLayer) {}  // 0x41de80

	int mLayer;  // +0x10
};

// A title of the scores (0x34 bytes, constructor 0x41f000): the text from its points up
class ScoreTitle : public engine::Object
{
public:
	ScoreTitle(const std::string& theText, int thePoints) : mText(theText), mPoints(thePoints) {}  // 0x41f000

	std::string mText;  // +0x0c
	int mPoints;        // +0x28
};

} // namespace pizzafrenzy

#endif
