#include "GameScreen.h"
#include "App.h"
#include "Host.h"
#include "ImageFilter.h"
#include "PizzaFrenzy.h"
#include "PizzaPopup.h"

#include <algorithm>
#include <cstdio>

using namespace pizzafrenzy;

namespace
{
const char* const GAME_LAYOUT_FILE = "res/screenLayouts/gameScreen.xml";
const char* const TITLES_NODE = "titles";
const char* const DECORATION_NODE_NAME = "decoration";
const char* const GROUP_NAME_ROAD = "RoadGroup";
const char* const GROUP_NAME_GROUND = "GroundGroup";
const char* const GROUP_NAME_AIR = "AirGroup";
const char* const GROUP_NAME_SFX = "SfxGroup";
const char* const GROUP_NAME_TIPS = "TipsGroup";
const char* const GROUP_NAME_ORDER_CURSOR = "OrderCursor";
const float FLASH_SECONDS = 0.3f;  // how long the order cursor's flash shows

// 0x470a50 (pizzafrenzy): two rectangles that overlap; an empty rectangle overlaps nothing
bool rectsIntersect(const engine::Rect& theFirst, const engine::Rect& theSecond)
{
	if (theSecond.isEmpty() || theFirst.isEmpty())
		return false;
	return theFirst.left < theSecond.right && theFirst.top < theSecond.bottom && theSecond.left < theFirst.right
	       && theSecond.top < theFirst.bottom;
}

// the popup of a house or kitchen, when it takes a click (the tile's slots 0x17c and 0x18c), else none
PizzaPopup* getClickablePopup(Tile* theTile)
{
	BuildingTile* aTile = static_cast<BuildingTile*>(theTile);
	return aTile->isClickable() ? aTile->getOccupant() : nullptr;
}

// the popup's bounds on the screen: its own, moved by the tile's place
engine::Rect getPopupRect(Tile* theTile, PizzaPopup* thePopup)
{
	engine::Rect aRect = *thePopup->getBounds();
	engine::Point* aPlace = theTile->getPosition();
	aRect.offset(aPlace->x, aPlace->y);
	return aRect;
}

// the node is the hover when the order cursor's box overlaps its rectangle more than any before it (0x470a50, 0x470ba0
// and 0x470910): the overlap's area is worked in doubles, as the original's x87 did, and kept as a float
void keepBest(const engine::Rect& theRect, engine::Rect* theCursor, engine::Node* theNode, engine::Node*& theFound,
              float& theBestArea)
{
	if (!rectsIntersect(*theCursor, theRect))
		return;
	engine::Rect anOverlap;
	anOverlap.left = std::max(theCursor->left, theRect.left);
	anOverlap.right = std::min(theCursor->right, theRect.right);
	anOverlap.top = std::max(theCursor->top, theRect.top);
	anOverlap.bottom = std::min(theCursor->bottom, theRect.bottom);
	double anHeight = (double)anOverlap.bottom - (double)anOverlap.top;
	double anArea = anHeight * ((double)anOverlap.right - (double)anOverlap.left);
	if (anArea > theBestArea)
	{
		theFound = theNode;
		theBestArea = (float)anArea;
	}
}

engine::MouseEvent makeMouseEvent(engine::Node* theTarget)
{
	engine::MouseEvent anEvent;
	anEvent.mTarget = theTarget;
	anEvent.mPosition = engine::App::instance()->getMousePosition();
	anEvent.mButtons = 0;
	return anEvent;
}

// one node of the city drawn into the ground's picture
void drawInto(engine::Graphics* theGraphics, engine::Node* theNode)
{
	if (theNode == nullptr)
		return;
	theGraphics->pushState();
	theNode->applyTransform(theGraphics);
	theNode->draw(theGraphics);
	theGraphics->popState();
}

// 0x41ab00: the place of a node whose bounds end at the right and bottom: before the first child that is lower, or as
// low and not as far to the right
std::vector<engine::Node*>::iterator findPlace(std::vector<engine::Node*>& theChildren, float theRight, float theBottom)
{
	std::vector<engine::Node*>::iterator aPlace = theChildren.begin();
	while (aPlace != theChildren.end())
	{
		engine::Rect* aBounds = (*aPlace)->getBounds();
		if (aBounds->bottom == theBottom)
		{
			if (aBounds->right < theRight)
				break;
		}
		else if (theBottom < aBounds->bottom)
		{
			break;
		}
		++aPlace;
	}
	return aPlace;
}

engine::Container* makeGroup(const char* theName)
{
	engine::Container* aGroup = new engine::Container();
	aGroup->setName(theName);
	return aGroup;
}
}

// 0x41ada0
void SortedContainer::addChild(engine::Node* theChild)
{
	if (std::find(mChildren.begin(), mChildren.end(), theChild) != mChildren.end())
		return;
	theChild->layout();
	float aRight = theChild->getBounds()->right;
	float aBottom = theChild->getBounds()->bottom;
	mChildren.insert(findPlace(mChildren, aRight, aBottom), theChild);
	setFlags(theChild->getAllFlags());
	setFlags(engine::NODE_LAYOUT);
	theChild->setParent(this);
}

// 0x41ae70
void SortedContainer::reorderChild(engine::Node* theChild)
{
	std::vector<engine::Node*>::iterator aCurrent = std::find(mChildren.begin(), mChildren.end(), theChild);
	float aRight = theChild->getBounds()->right;
	float aBottom = theChild->getBounds()->bottom;
	std::vector<engine::Node*>::iterator aPlace = findPlace(mChildren, aRight, aBottom);
	if (aCurrent != mChildren.end() && aCurrent + 1 != aPlace)
	{
		// the child's own count is not changed: it leaves its place and comes back to another
		mChildren.erase(aCurrent);
		mChildren.insert(findPlace(mChildren, aRight, aBottom), theChild);
	}
}

// 0x434be0
GameScreen::GameScreen() : mFlashTime(0)
{
}

// 0x436440: the speed bonus's banner (0x44fb70) and the message banner (0x42bb30), which the original makes here
// and adds, are translated with the game (the placing of the message banner is (400, 500))
void GameScreen::link(Hud* theHud, CityMap* theMap)
{
	mMap = theMap;
	mGroundNode = new engine::ImageNode();
	addChild(mGroundNode);
	mRoadGroup = makeGroup(GROUP_NAME_ROAD);
	addChild(mRoadGroup);
	mGroundGroup = new SortedContainer();
	mGroundGroup->setName(GROUP_NAME_GROUND);
	addChild(mGroundGroup);
	mAirGroup = makeGroup(GROUP_NAME_AIR);
	addChild(mAirGroup);
	mTipsGroup = makeGroup(GROUP_NAME_TIPS);
	addChild(mTipsGroup);
	mSfxGroup = makeGroup(GROUP_NAME_SFX);
	addChild(mSfxGroup);
	mHud = theHud;
	if (theHud != nullptr)
		addChild(theHud);
	mOrderCursor = makeGroup(GROUP_NAME_ORDER_CURSOR);
	addChild(mOrderCursor);
	mGameLayout = new engine::ScreenLayout();
	mGameLayout->loadLayout(GAME_LAYOUT_FILE);
	mTitles = mGameLayout->getNode(TITLES_NODE);
	addChild(mGameLayout);
	addOwnFlags(engine::NODE_MOUSE);
}

// 0x4350c0
void GameScreen::clearGroups()
{
	mRoadGroup->removeAllChildren();
	mGroundGroup->removeAllChildren();
	mAirGroup->removeAllChildren();
	mSfxGroup->removeAllChildren();
	mTipsGroup->removeAllChildren();
	mOrderCursor->removeAllChildren();
}

// 0x435c60: the ground's picture has the background and what the tiles and decorations draw into it for good; the
// rest are nodes in the groups. A decoration that no tile is under is drawn into the picture too.
void GameScreen::drawCity()
{
	engine::Host* aHost = engine::getHost();
	int aColumns = mMap->getColumns();
	int aCellSize = mMap->getCellSize();
	int aRows = mMap->getRows();
	if (mGroundImage == nullptr)
		mGroundImage = aHost->createBlankImage(aCellSize * aColumns, aCellSize * aRows);
	engine::Graphics* aGraphics = aHost->createImageGraphics(mGroundImage);
	if (!mMap->getBackground().empty())
	{
		engine::Ref<engine::Image> aBackground = engine::App::instance()->createImage(mMap->getBackground().c_str(),
		                                                                              false);
		std::string aHsvShift = mMap->getHsvShift();
		if (aBackground != nullptr)
		{
			aGraphics->drawImage(aBackground, engine::IntRect(0, 0, aBackground->getWidth(), aBackground->getHeight()),
			                     0, 0);
			float aHue = 0;
			float aSaturation = 0;
			float aValue = 0;
			sscanf(aHsvShift.c_str(), "%f,%f,%f", &aHue, &aSaturation, &aValue);
			if (aHue != 0.0f || aSaturation != 0.0f || aValue != 0.0f)
			{
				engine::HsvShiftFilter aFilter;
				aFilter.setShift(aHue, aSaturation, aValue);
				mGroundImage->applyFilter(aFilter);
			}
		}
	}
	mRoadGroup->removeAllChildren();
	mGroundGroup->removeAllChildren();
	mAirGroup->removeAllChildren();
	mSfxGroup->removeAllChildren();

	std::map<std::string, engine::Ref<Tile>>& aTiles = mMap->mTiles;
	std::map<std::string, engine::Ref<Tile>>::iterator aTileIt;
	for (aTileIt = aTiles.begin(); aTileIt != aTiles.end(); ++aTileIt)
	{
		Tile* aTile = aTileIt->second;
		aTile->prepare();
		if (engine::Node* aNode = aTile->getLayerNode(LAYER_ROAD))
			mRoadGroup->addChild(aNode);
		if (engine::Node* aNode = aTile->getLayerNode(LAYER_GROUND))
			mGroundGroup->addChild(aNode);
		if (engine::Node* aNode = aTile->getLayerNode(LAYER_AIR))
			mAirGroup->addChild(aNode);
		if (engine::Node* aNode = aTile->getLayerNode(LAYER_SFX))
			mSfxGroup->addChild(aNode);
	}
	std::vector<engine::Ref<Decoration>>& aDecorations = mMap->mDecorations;
	for (size_t i = 0; i < aDecorations.size(); i++)
	{
		if (engine::Node* aNode = aDecorations[i]->getNode(LAYER_GROUND))
			mGroundGroup->addChild(aNode);
		if (engine::Node* aNode = aDecorations[i]->getNode(LAYER_AIR))
			mAirGroup->addChild(aNode);
	}

	for (int aLayer = LAYER_GROUND_BELOW; aLayer <= LAYER_GROUND_ABOVE; aLayer++)
	{
		for (aTileIt = aTiles.begin(); aTileIt != aTiles.end(); ++aTileIt)
			drawInto(aGraphics, aTileIt->second->getLayerNode(aLayer));
		for (size_t i = 0; i < aDecorations.size(); i++)
			drawInto(aGraphics, aDecorations[i]->getNode(aLayer));
	}

	// a node of the ground group that is a decoration and has no tile near it never changes, so it is drawn into the
	// picture and leaves the group
	std::vector<engine::Node*>& aGroundChildren = mGroundGroup->mChildren;
	size_t aChildIndex = 0;
	while (aChildIndex < aGroundChildren.size())
	{
		engine::Node* aChild = aGroundChildren[aChildIndex];
		bool aNearTile = false;
		for (aTileIt = aTiles.begin(); aTileIt != aTiles.end(); ++aTileIt)
		{
			Tile* aTile = aTileIt->second;
			aTile->layout();
			aChild->layout();
			if (rectsIntersect(*aTile->getBounds(), *aChild->getBounds()))
			{
				aNearTile = true;
				break;
			}
		}
		engine::ImageNode* anImageNode = dynamic_cast<engine::ImageNode*>(aChild);
		if (!aNearTile && aChild->getName() == DECORATION_NODE_NAME && anImageNode != nullptr
		    && anImageNode->mImage != nullptr)
		{
			engine::Image* anImage = anImageNode->mImage;
			aGraphics->drawImage(anImage, engine::IntRect(0, 0, anImage->getWidth(), anImage->getHeight()),
			                     aChild->getPosition()->x, aChild->getPosition()->y);
			engine::Ref<engine::Node> aRemoved = aChild;
			mGroundGroup->removeChild(aChild);
		}
		else
		{
			aChildIndex++;
		}
	}

	// the map's vehicles (+0xc0) are put in the air group when they are in layer 4 and in the ground group else: the
	// game's play makes them

	mGroundNode->setImage(mGroundImage);
	delete aGraphics;
}

// 0x435ba0
void GameScreen::updateScreen(engine::UpdateInfo& theInfo)
{
	// the speed bonus's banner (+0x204, 0x44fcd0) takes its frame here: left out with the banner
	std::vector<engine::Ref<Van>>& aVehicles = mMap->mVehicles;
	for (size_t i = 0; i < aVehicles.size(); i++)
	{
		Van* aVan = aVehicles[i];
		if (aVan->isDirty() && aVan->getLayer() == LAYER_GROUND)
		{
			mGroundGroup->reorderChild(aVan);
			aVan->clearDirty();
		}
	}
	updateHover(theInfo);
}

// 0x435380
void GameScreen::updateHover(engine::UpdateInfo& theInfo)
{
	engine::Point aMouse = engine::App::instance()->getMousePosition();
	mOrderCursor->setPosition(aMouse);
	if (0.0f < mFlashTime)
	{
		mFlashTime = mFlashTime - theInfo.mSeconds;
		if (mFlashTime < 0.0f && mFlashIndicators != nullptr)
		{
			mFlashTime = 0;
			mFlashIndicators->setVisible(false);
		}
	}

	engine::Node* aFound = nullptr;
	std::vector<engine::Ref<Tile>>& aKitchens = mMap->mKitchens;
	std::vector<engine::Ref<Tile>>& aCustomers = mMap->mCustomers;
	std::list<engine::Ref<CouponButton>>& aCoupons = mHud->mCoupons;
	std::vector<engine::Ref<TipPopup>>& aTips = mMap->mTips;
	if (mOrderCursor->mChildren.empty())
	{
		// the last of them that has the cursor's point in it, one house or kitchen at a time
		for (size_t i = 0; i < aKitchens.size(); i++)
		{
			PizzaPopup* aPopup = getClickablePopup(aKitchens[i]);
			if (aPopup != nullptr && getPopupRect(aKitchens[i], aPopup).contains(*mOrderCursor->getBounds()))
			{
				aFound = aPopup;
				break;
			}
		}
		for (size_t i = 0; i < aCustomers.size(); i++)
		{
			PizzaPopup* aPopup = getClickablePopup(aCustomers[i]);
			if (aPopup != nullptr && getPopupRect(aCustomers[i], aPopup).contains(*mOrderCursor->getBounds()))
			{
				aFound = aPopup;
				break;
			}
		}
		std::list<engine::Ref<CouponButton>>::iterator aCouponIt;
		for (aCouponIt = aCoupons.begin(); aCouponIt != aCoupons.end(); ++aCouponIt)
		{
			if ((*aCouponIt)->getBounds()->contains(*mOrderCursor->getBounds()))
			{
				aFound = *aCouponIt;
				break;
			}
		}
		for (size_t i = 0; i < aTips.size(); i++)
		{
			if (aTips[i]->getBounds()->contains(*mOrderCursor->getBounds()) && aTips[i]->isWaiting())
			{
				aFound = aTips[i];
				break;
			}
		}
	}
	else
	{
		// the one whose box the cursor's overlaps most, the first of them when two overlap as much; the area is
		// worked in the x87's wider numbers, and kept as a float
		float aBestArea = 0;
		for (size_t i = 0; i < aKitchens.size(); i++)
		{
			PizzaPopup* aPopup = getClickablePopup(aKitchens[i]);
			if (aPopup != nullptr)
				keepBest(getPopupRect(aKitchens[i], aPopup), mOrderCursor->getBounds(), aPopup, aFound, aBestArea);
		}
		for (size_t i = 0; i < aCustomers.size(); i++)
		{
			PizzaPopup* aPopup = getClickablePopup(aCustomers[i]);
			if (aPopup != nullptr)
				keepBest(getPopupRect(aCustomers[i], aPopup), mOrderCursor->getBounds(), aPopup, aFound, aBestArea);
		}
		std::list<engine::Ref<CouponButton>>::iterator aCouponIt;
		for (aCouponIt = aCoupons.begin(); aCouponIt != aCoupons.end(); ++aCouponIt)
			keepBest(*(*aCouponIt)->getBounds(), mOrderCursor->getBounds(), *aCouponIt, aFound, aBestArea);
		for (size_t i = 0; i < aTips.size(); i++)
		{
			if (aTips[i]->isWaiting())
				keepBest(*aTips[i]->getBounds(), mOrderCursor->getBounds(), aTips[i], aFound, aBestArea);
		}
	}
	setHover(aFound);
}

// 0x435380's last part: the hover is left and the node entered
void GameScreen::setHover(engine::Node* theNode)
{
	if (mHover != nullptr && mHover != theNode)
	{
		engine::MouseEvent anEvent = makeMouseEvent(mHover);
		mHover->handleMouseExitEvent(anEvent);
		mHover = nullptr;
	}
	if (theNode != nullptr && theNode != mHover)
	{
		mHover = theNode;
		engine::MouseEvent anEvent = makeMouseEvent(theNode);
		theNode->handleMouseEnterEvent(anEvent);
	}
}

// 0x435320
void GameScreen::onMouseDownEvent(engine::MouseEvent& theEvent)
{
	if (mHover != nullptr)
		mHover->handleMouseDownEvent(theEvent);
}

// 0x435340
void GameScreen::onRightDownEvent(engine::MouseEvent& theEvent)
{
	if (mHover != nullptr)
		mHover->handleRightDownEvent(theEvent);
}

// 0x435210
void GameScreen::flashCursor()
{
	if (mFlashIndicators != nullptr)
	{
		mFlashIndicators->setVisible(true);
		mFlashTime = FLASH_SECONDS;
	}
}

// 0x4351c0
void GameScreen::setFlashIndicators(engine::Node* theIndicators)
{
	mFlashIndicators = theIndicators;
	theIndicators->setVisible(false);
}

// 0x4352a0
void GameScreen::setCouponCursor(engine::Node* theNode)
{
	mOrderCursor->removeAllChildren();
	if (theNode != nullptr)
	{
		theNode->setPosition(0.0f, 0.0f);
		mOrderCursor->addChild(theNode);
	}
}

// 0x435180: what the mouse is over is let go there too, without leaving it
void GameScreen::hideOrderCursor()
{
	mOrderCursor->removeAllChildren();
	mHover = nullptr;
}
