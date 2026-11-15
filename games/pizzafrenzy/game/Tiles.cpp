#include "Tiles.h"
#include "App.h"
#include "CityMap.h"
#include "CustomerPopup.h"
#include "PizzaFrenzy.h"
#include "GameMode.h"
#include "PizzaPopup.h"
#include "Van.h"

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
// how far from a building the nearest road is looked for, in cells across and down
const int ROAD_SEARCH_RANGE = 4;
const float RAISED_SECONDS = 2.0f;
// where the popup of a kitchen stands on its tile (0x41c640)
const float POPUP_TOP = 16.0f;

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
KitchenTile::KitchenTile(const std::string& theTypeName) : mVanType("PizzaVan"), mSignFrame(-1), mSignTime(0)
{
	setName(theTypeName);
	setKind(KIND_KITCHEN);
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

// 0x41c720: the base's, then the kitchen lets go of its popup
void KitchenTile::place(CityMap* theMap, const engine::IntPair& theCell)
{
	Tile::place(theMap, theCell);
	mOccupant = nullptr;
}

// 0x4263f0: the time the bridge is raised counts down, and at the end of it the bridge is let down
void BridgeTile::tick(engine::UpdateInfo& theInfo)
{
	if (0.0f < mRaisedTime)
	{
		mRaisedTime -= theInfo.mSeconds;
		if (mRaisedTime < 0.0f)
			setRaised(false);
	}
}

// 0x4265e0: both pairs of pictures show the one the bridge is in; a raised bridge stays so for two seconds
void BridgeTile::setRaised(bool theRaised)
{
	int aFrame = theRaised ? 0 : 1;
	mRoadImages->select(aFrame, 0, true);
	mGroundImages->select(aFrame, 0, true);
	mRaised = theRaised;
	mRaisedTime = theRaised ? RAISED_SECONDS : 0.0f;
}

// 0x41c490
BuildingTile::BuildingTile()
{
	mParkingSpot.mFirst = 0;
	mParkingSpot.mSecond = 0;
}

// 0x41c180
void BuildingTile::tick(engine::UpdateInfo& theInfo)
{
	if (mOccupant != nullptr)
	{
		mOccupant->tick(theInfo);
		return;
	}
	if (mWaiting != nullptr)
	{
		setOccupant(mWaiting);
		mWaiting = nullptr;
	}
}

// 0x41bfa0
void BuildingTile::setOccupant(PizzaPopup* thePopup)
{
	thePopup->setScale(0.0f);
	if (mOccupant == nullptr)
	{
		mOccupant = thePopup;
		static_cast<engine::Container*>(mSfxNode.get())->addChild(thePopup);
		float aCellSize = (float)PizzaFrenzy::instance()->mCityMap->getCellSize();
		engine::Point aPoint = *thePopup->getPosition();
		aPoint.x += getPosition()->x;
		aPoint.y += getPosition()->y;
		float aBottomX = (float)((int)(aPoint.x / aCellSize) + 1) * aCellSize;
		float aBottomY = (float)((int)(aPoint.y / aCellSize) + 1) * aCellSize;
		mOccupant->setPosition(aBottomX - getPosition()->x, aBottomY - getPosition()->y);
		return;
	}
	mWaiting = thePopup;
}

// 0x41c120
void BuildingTile::removeOccupant()
{
	if (mOccupant != nullptr)
		mOccupant->leave();
}

// 0x41bd20: the click is the mode's
void BuildingTile::onMouseDownEvent(engine::MouseEvent& theEvent)
{
	PizzaFrenzy::instance()->mMode->click(this);
}

// 0x41c0f0
void BuildingTile::clearOccupant()
{
	mOccupant = nullptr;
}

// 0x41bf10
bool BuildingTile::isClickable()
{
	return mOccupant != nullptr && mOccupant->isClickable();
}

// 0x41c140
void BuildingTile::hangUp()
{
	if (mOccupant != nullptr)
		mOccupant->hangUp();
}

// 0x41bf50
void BuildingTile::pick()
{
	if (mOccupant != nullptr)
		mOccupant->pick();
}

// 0x41bd40: the road cell is the parking spot's from the tile's cell when a road is there, else the nearest road
engine::IntPair BuildingTile::getRoadCell()
{
	engine::IntPair aCell = {mCell.mFirst + mParkingSpot.mFirst, mCell.mSecond + mParkingSpot.mSecond};
	Tile* aTile = mMap->getTileAt(aCell);
	if (aTile != nullptr && aTile->isRoadAt(aCell))
		return aCell;
	Tile* aRoad = mMap->findNearestRoad(mCell, engine::IntPair{ROAD_SEARCH_RANGE, ROAD_SEARCH_RANGE});
	if (aRoad != nullptr)
		return aRoad->getCell();
	return engine::IntPair{-1, -1};
}

// 0x41bf70
void BuildingTile::unpick()
{
	if (mOccupant != nullptr && mOccupant->isPicked())
		mOccupant->unpick();
}

// 0x41c160
bool BuildingTile::isOccupantLeaving()
{
	return mOccupant != nullptr && mOccupant->isLeaving();
}

// 0x41c3d0: each van is told to let go (0x13c, which does nothing), and they are released
void BuildingTile::clearVehicles()
{
	mVehicles.clear();
}

// 0x41b7b0: the customer called by the tile's character name, of the city that is played
Character* CustomerTile::getCharacter()
{
	return PizzaFrenzy::instance()->mTileManifest->getCharacterByNameTheme(mCharacterName);
}

// 0x41b940
Order* CustomerTile::getOrder()
{
	if (mOccupant != nullptr && mOccupant->getKind() == 0)
		return mOccupant->getOrder();
	return nullptr;
}

// 0x41b8f0
Order* CustomerTile::getActiveOrder()
{
	if (mOccupant != nullptr && isClickable() && mOccupant->getKind() == 0)
		return mOccupant->getOrder();
	return nullptr;
}

// 0x41b6a0
void CustomerTile::setOrder(Order* theOrder, bool theFlag)
{
	CustomerPopup* aPopup = new CustomerPopup();
	aPopup->setOwner(this);
	aPopup->setOrder(theOrder, theFlag);
	setOccupant(aPopup);
}

// 0x41b730
void CustomerTile::callPolice(int theSpecial)
{
	if (theSpecial == Character::SPECIAL_POLICE)
	{
		PolicePopup* aPopup = new PolicePopup();
		aPopup->setOwner(this);
		setOccupant(aPopup);
	}
}

// 0x41b820
void CustomerTile::serve()
{
	if (mOccupant != nullptr)
		mOccupant->serve();
}

// 0x41b840
void CustomerTile::finish()
{
	if (mOccupant != nullptr && mOccupant->getKind() == POPUP_CUSTOMER)
	{
		static_cast<CustomerPopup*>(mOccupant.get())->finish();
		if (getCharacter()->mSpecial == Character::SPECIAL_POLICE &&
		    PizzaFrenzy::instance()->mMode->hasPendingSpecialOrder())
			callPolice(Character::SPECIAL_POLICE);
	}
}

// 0x41b8b0
void CustomerTile::setImpatient(bool theImpatient)
{
	if (mOccupant != nullptr && mOccupant->getKind() == POPUP_CUSTOMER)
		static_cast<CustomerPopup*>(mOccupant.get())->setImpatient(theImpatient);
}

// 0x41b980
void CustomerTile::flash()
{
	if (mOccupant != nullptr && mOccupant->getKind() == POPUP_CUSTOMER)
		static_cast<CustomerPopup*>(mOccupant.get())->flash();
}

// 0x41c6f0
Topping* KitchenTile::getTopping()
{
	if (mOccupant == nullptr)
		return nullptr;
	return getOccupant()->getTopping();
}

// 0x41c640: the tile is shown, its popup is made for the topping and put at the top of the tile
void KitchenTile::setUpPopup(Topping* theTopping)
{
	setVisible(true);
	removeOccupant();
	PizzaKitchenPopup* aPopup = new PizzaKitchenPopup();
	aPopup->setOwner(this);
	aPopup->setTopping(theTopping);
	aPopup->setPosition(0.0f, POPUP_TOP);
	setOccupant(aPopup);
}

// 0x41c7b0: the building's frame, then the time the sign shows runs out
void KitchenTile::tick(engine::UpdateInfo& theInfo)
{
	BuildingTile::tick(theInfo);
	if (0.0f < mSignTime)
	{
		mSignTime -= theInfo.mSeconds;
		if (mSignTime < 0.0f)
		{
			mSignTime = 0.0f;
			// the sign's board is cleared and its digits shown (0x4682e0): translated next
		}
	}
}

// 0x41cb80: the sign shows the length of the combo in the hue its row of five gives; translated next
void KitchenTile::showCombo(int theLength, int theFives)
{
	// the sign's pictures are shifted in hue from the images "kitchenMarquis%d" and "kitchenMarquis4": translated next
}

// 0x41c560
void BuildingTile::addVehicle(const engine::IntPair& theCell, Van* theVan)
{
	mVehicles.push_back(theVan);
	theVan->setup(mMap, theCell, this);
	PizzaFrenzy::instance()->mMode->addVehicleNode(theVan);
}

// 0x41c460 (by 0x41c360)
void BuildingTile::removeVehicle(Van* theVan)
{
	theVan->leave();
	for (size_t i = 0; i < mVehicles.size(); i++)
	{
		if (mVehicles[i].get() == theVan)
		{
			mVehicles.erase(mVehicles.begin() + i);
			return;
		}
	}
}

// 0x41c9e0: the vans of the type "copter" and "canoe" are not translated, and are vans
Van* KitchenTile::getVehicle()
{
	for (size_t i = 0; i < mVehicles.size(); i++)
	{
		Van* aVan = static_cast<Van*>(mVehicles[i].get());
		if (aVan->isIdle())
			return aVan;
	}
	Van* aVan = new Van(mVanType);
	aVan->loadType();
	addVehicle(getRoadCell(), aVan);
	return aVan;
}

// 0x41cf50
void KitchenTile::sendVan(CustomerTile* theTile)
{
	Van* aVan = getVehicle();
	if (aVan != nullptr)
	{
		aVan->goTo(theTile);
		theTile->serve();
	}
	if (mOccupant != nullptr)
		mOccupant->serve();
}
