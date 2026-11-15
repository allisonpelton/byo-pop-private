#include "Tiles.h"
#include "App.h"
#include "CityMap.h"
#include "PizzaFrenzy.h"

using namespace pizzafrenzy;

namespace
{
const char* const KIND_ROAD = "roads";
const char* const KIND_CUSTOMER = "customer";
const char* const KIND_KITCHEN = "kitchen";
const char* const KIND_BRIDGE = "bridge";

// which of the four cells around a road are roads, as the bits of the number of its picture in the sheet
const int ROAD_NEXT_ROW = 1;
const int ROAD_PREVIOUS_COLUMN = 2;
const int ROAD_PREVIOUS_ROW = 4;
const int ROAD_NEXT_COLUMN = 8;
const float WATER_OPACITY = 0.5f;

// the board of a kitchen, which has five pictures of numbers
const int SIGN_DIGIT_COUNT = 5;
const float SIGN_X = 4.0f;
const float SIGN_Y = -38.0f;
const char* const SIGN_BORDER_IMAGE = "res\\fx\\signBorder.jpg";

engine::IntPair toIntPair(const engine::Point& thePoint)
{
	engine::IntPair aPair;
	aPair.mFirst = (int)thePoint.x;
	aPair.mSecond = (int)thePoint.y;
	return aPair;
}
}

// 0x41b570
Tile::Tile() : mMap(nullptr), mImageWidth(0), mImageHeight(0)
{
	mCell.mFirst = -1;
	mCell.mSecond = -1;
	mBottomRight.mFirst = 0;
	mBottomRight.mSecond = 0;
	mBase.mFirst = 0;
	mBase.mSecond = 0;
	mDimensions.mFirst = 1;
	mDimensions.mSecond = 1;
}

// 0x41b050
engine::Node* Tile::getLayerNode(int theLayer)
{
	if (theLayer == LAYER_GROUND)
		return this;
	if (theLayer == LAYER_SFX)
		return mSfxNode;
	return nullptr;
}

// 0x41b260: the picture's size, the cells, the offsets and the picture are the entry's; the name is taken even when
// there is no entry
bool Tile::setTileType(TileType* theEntry, const std::string& theInstanceName)
{
	mInstanceName = theInstanceName;
	if (theEntry == nullptr)
		return false;
	mImageWidth = theEntry->mImage->getWidth();
	mImageHeight = theEntry->mImage->getHeight();
	mDimensions = toIntPair(theEntry->mDimensions);
	mBottomRight = toIntPair(theEntry->mBottomRightOffset);
	mBase = toIntPair(theEntry->mBase);
	mImage = theEntry->mImage;
	return true;
}

// 0x41b070
void Tile::locate(CityMap* theMap, const engine::IntPair& theCell)
{
	mMap = theMap;
	mCell = theCell;
	engine::IntPair aPoint = theMap->cellToPoint(theCell);
	layout();
	setPosition((float)(aPoint.mFirst - mBottomRight.mFirst) - (float)mImageWidth * 0.5f,
	            (float)(aPoint.mSecond - mBottomRight.mSecond) - (float)mImageHeight * 0.5f);
	if (mSfxNode != nullptr)
		mSfxNode->setPosition(*getPosition());
}

// 0x41b4c0
void Tile::place(CityMap* theMap, const engine::IntPair& theCell)
{
	if (mInstanceName.empty())
		mInstanceName = theMap->nextName();
	locate(theMap, theCell);
	mMap->place(theCell, this);
}

// 0x426f30
RoadTile::RoadTile(const std::string& theTypeName) : mWeight(1)
{
	setName(theTypeName);
	setKind(KIND_ROAD);
}

// 0x426c90
bool RoadTile::accept(CityMap* theMap, const std::string& theInstanceName)
{
	TileType* anEntry = PizzaFrenzy::instance()->mTileManifest->getRoadTile(getName(), 0);
	if (anEntry != nullptr && anEntry->mImage != nullptr)
	{
		mRoadNode = new engine::ImageNode(anEntry->mImage);
		addChild(mRoadNode);
	}
	setTileType(anEntry, theInstanceName);
	return true;
}

// 0x426c10
engine::Node* RoadTile::getLayerNode(int theLayer)
{
	return theLayer == LAYER_GROUND_ABOVE ? this : nullptr;
}

// 0x426d30: the picture of the sheet by which of the four cells around are roads; water is only seen while a game
// is on the screen
void RoadTile::prepare()
{
	engine::IntPair aCell = getCell();
	engine::IntPair aPreviousRow = {aCell.mFirst, aCell.mSecond - 1};
	engine::IntPair aNextRow = {aCell.mFirst, aCell.mSecond + 1};
	engine::IntPair aNextColumn = {aCell.mFirst + 1, aCell.mSecond};
	engine::IntPair aPreviousColumn = {aCell.mFirst - 1, aCell.mSecond};
	bool aPreviousRowIsRoad = mMap->isRoadAt(aPreviousRow);
	bool aNextRowIsRoad = mMap->isRoadAt(aNextRow);
	bool aNextColumnIsRoad = mMap->isRoadAt(aNextColumn);
	bool aPreviousColumnIsRoad = mMap->isRoadAt(aPreviousColumn);
	int aMask = 0;
	if (aPreviousColumnIsRoad)
		aMask |= ROAD_PREVIOUS_COLUMN;
	if (aNextColumnIsRoad)
		aMask |= ROAD_NEXT_COLUMN;
	if (aNextRowIsRoad)
		aMask |= ROAD_NEXT_ROW;
	if (aPreviousRowIsRoad)
		aMask |= ROAD_PREVIOUS_ROW;
	TileType* anEntry = PizzaFrenzy::instance()->mTileManifest->getRoadTile(getName(), aMask);
	if (anEntry == nullptr || mRoadNode == nullptr)
		return;
	mImage = anEntry->mImage;
	mRoadNode->setImage(anEntry->mImage);
	bool aWater = mName == "Water";
	if (aWater && !PizzaFrenzy::instance()->mGameScreenActive)
	{
		mRoadNode->setVisible(false);
		mImage = nullptr;
		return;
	}
	mRoadNode->setVisible(true);
	mRoadNode->setOpacity(aWater ? WATER_OPACITY : 1.0f);
}

// 0x4266e0
BridgeTile::BridgeTile(const std::string& theTypeName) : mRaisedTime(0), mRaised(false)
{
	setName(theTypeName);
	setKind(KIND_BRIDGE);
}

// 0x4267a0: the road's pictures and the ground's, the way blocked and then open, each pair a selector
bool BridgeTile::accept(CityMap* theMap, const std::string& theInstanceName)
{
	mRoadImages = new engine::Selector();
	mRoadImages->setName("RoadImages");
	mGroundImages = new engine::Selector();
	mGroundImages->setName("GroundImages");
	addChild(mGroundImages);
	TileType* anEntry = PizzaFrenzy::instance()->mTileManifest->getTile(getName(), getKind());
	if (anEntry != nullptr)
	{
		BridgeTileType* aBridge = static_cast<BridgeTileType*>(anEntry);
		mRoadImages->addChild(new engine::ImageNode(aBridge->mRoadImage));
		mGroundImages->addChild(new engine::ImageNode(aBridge->mImage));
		mRoadImages->addChild(new engine::ImageNode(aBridge->mUnblockedRoadImage));
		mGroundImages->addChild(new engine::ImageNode(aBridge->mUnblockedImage));
		mRoadImages->select(1, 0, true);
		mGroundImages->select(1, 0, true);
	}
	setTileType(anEntry, theInstanceName);
	return true;
}

// 0x4265c0
engine::Node* BridgeTile::getLayerNode(int theLayer)
{
	if (theLayer == LAYER_GROUND)
		return this;
	if (theLayer == LAYER_ROAD)
		return mRoadImages;
	return nullptr;
}

// 0x426580: the road's pictures are in the road group, so they are put where the bridge is
void BridgeTile::place(CityMap* theMap, const engine::IntPair& theCell)
{
	Tile::place(theMap, theCell);
	mRoadImages->setPosition(*getPosition());
}

// 0x41ba60
CustomerTile::CustomerTile(const std::string& theTypeName)
{
	setName(theTypeName);
	setKind(KIND_CUSTOMER);
	mParkingSpot.mFirst = 0;
	mParkingSpot.mSecond = 0;
}

// 0x41bbe0
bool CustomerTile::accept(CityMap* theMap, const std::string& theInstanceName)
{
	TileType* anEntry = PizzaFrenzy::instance()->mTileManifest->getTile(getName(), getKind());
	if (anEntry == nullptr)
		return false;
	CustomerTileType* aCustomer = static_cast<CustomerTileType*>(anEntry);
	addChild(new engine::ImageNode(anEntry->mImage));
	setTileType(anEntry, theInstanceName);
	mParkingSpot = toIntPair(aCustomer->mParkingSpot);
	mSfxNode = new engine::Container();
	mCharacterName = aCustomer->mCharacterName;
	return true;
}

// 0x41cf90
KitchenTile::KitchenTile(const std::string& theTypeName) : mVanType("PizzaVan"), mSignFrame(-1)
{
	setName(theTypeName);
	setKind(KIND_KITCHEN);
	mParkingSpot.mFirst = 0;
	mParkingSpot.mSecond = 0;
}

// 0x41d190: the house's picture, and the board with the numbers over it, which has no pictures yet (the game's play
// gives them)
bool KitchenTile::accept(CityMap* theMap, const std::string& theInstanceName)
{
	TileType* anEntry = PizzaFrenzy::instance()->mTileManifest->getTile(getName(), getKind());
	if (anEntry != nullptr)
	{
		KitchenTileType* aKitchen = static_cast<KitchenTileType*>(anEntry);
		addChild(new engine::ImageNode(anEntry->mImage));
		mParkingSpot = toIntPair(aKitchen->mParkingSpot);
		setTileType(anEntry, theInstanceName);
		mSignBoard = new engine::ImageNode();
		mSignBoard->setPosition(SIGN_X, SIGN_Y);
		addChild(mSignBoard);
		mSignFrame = -1;
		mSignDigits = new engine::Selector();
		mSignDigits->addChild(new engine::ImageNode());
		mSignDigits->setPosition(*mSignBoard->getPosition());
		addChild(mSignDigits);
		for (int i = 0; i < SIGN_DIGIT_COUNT; i++)
		{
			mSignDigitNodes[i] = new engine::ImageNode();
			mSignDigits->addChild(mSignDigitNodes[i]);
		}
		engine::Image* aBorder = engine::App::instance()->createImage(SIGN_BORDER_IMAGE, false);
		aBorder->setPivot(1);
		engine::ImageNode* aBorderNode = new engine::ImageNode(aBorder);
		aBorderNode->setPosition(*mSignBoard->getPosition());
		addChild(aBorderNode);
	}
	mSfxNode = new engine::Container();
	return true;
}

// 0x41c720: the base's, then the kitchen lets go of the object at +0x1b0, which the play makes
void KitchenTile::place(CityMap* theMap, const engine::IntPair& theCell)
{
	Tile::place(theMap, theCell);
}
