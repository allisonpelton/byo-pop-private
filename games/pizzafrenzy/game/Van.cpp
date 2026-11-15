#include "Van.h"
#include "CityMap.h"
#include "CustomerPopup.h"
#include "GameMode.h"
#include "ImageNode.h"
#include "App.h"
#include "Order.h"
#include "PizzaFrenzy.h"
#include "Tiles.h"

#include <algorithm>
#include <cmath>

using namespace pizzafrenzy;

namespace
{
const char* const SOUND_LEAVE_CAR = "vehicle_leave_car";
const char* const SOUND_LEAVE_VAN = "vehicle_leave_van";
const char* const SOUND_ARRIVE = "vehicle_arrive";
const char* const SOUND_POLICE = "special_police";
const char* const SOUND_POLICE_ARREST = "special_police_arrest";
const char* const VEHICLE_CLASS = "vehicle";
const char* const POLICE_CAR_TYPE = "PoliceCar";
const char* const RED_LIGHT_IMAGE = "res\\fx\\policeLightRed.jpg";
const char* const BLUE_LIGHT_IMAGE = "res\\fx\\policeLightBlue.jpg";
// a false arrest: the mode's action, and the splat that comes up where the car is
const char* const ACTION_FALSE_ARREST = "falseArrest";
const char* const FALSE_ARREST_SPLAT = "falseArrest";

// a vehicle with a speed of at most this is a car, and sounds like one when it leaves
const float CAR_SPEED = 200.0f;
const float ARRIVED_SECONDS = 1.0f;
const float LIGHTS_RATE = 4.0f;  // the police car's lights change this many times a second
enum { LAYER_GROUND = 3 };

// the pictures of the vehicle: east and west, north, south, and a parked one
enum
{
	IMAGE_EAST_WEST = 0,
	IMAGE_NORTH = 1,
	IMAGE_SOUTH = 2,
	IMAGE_ARRIVED = 3,
	IMAGE_PARKED = 4
};

// 0x42a600's table (0x532938): left, down, right, up
const engine::IntPair SEARCH_DIRECTIONS[4] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};

bool sameCell(const engine::IntPair& theFirst, const engine::IntPair& theSecond)
{
	return theFirst.mFirst == theSecond.mFirst && theFirst.mSecond == theSecond.mSecond;
}

void playSound(const char* theName)
{
	PizzaFrenzy::instance()->mSounds->play(theName, 1.0f, 1.0f);
}

// 0x429cf0's grid: a node for each cell
struct PathNode
{
	PathNode* mParent;
	int mF;
	int mG;
	int mH;
	int mCost;
	bool mInHeap;
	bool mClosed;
	engine::IntPair mCell;
};

class PathSearch
{
public:
	PathSearch(CityMap* theMap, const engine::IntPair& theStart, const engine::IntPair& theGoal) :
		mMap(theMap), mGoal(theGoal), mStart(theStart)
	{
	}

	std::vector<engine::IntPair> run()
	{
		std::vector<engine::IntPair> aPath;
		int aColumns = mMap->getColumns();
		int aRows = mMap->getRows();
		if (!isOnGrid(mStart) || !isOnGrid(mGoal))
			return aPath;
		mRows = aRows;
		mNodes.assign(aColumns * aRows, PathNode());
		for (int aColumn = 0; aColumn < aColumns; aColumn++)
		{
			for (int aRow = 0; aRow < aRows; aRow++)
			{
				PathNode& aNode = nodeAt(engine::IntPair{aColumn, aRow});
				aNode.mParent = nullptr;
				aNode.mF = aNode.mG = aNode.mH = 0;
				aNode.mInHeap = aNode.mClosed = false;
				aNode.mCell = engine::IntPair{aColumn, aRow};
				Tile* aTile = mMap->getTileAt(aNode.mCell);
				aNode.mCost = aTile != nullptr ? aTile->getWeight(aNode.mCell) : 0;
			}
		}
		PathNode& aStart = nodeAt(mStart);
		aStart.mG = aStart.mCost;
		aStart.mH = distance(mStart, mGoal);
		aStart.mF = aStart.mG + aStart.mH;
		pushHeap(&aStart);
		while (!mHeap.empty())
		{
			PathNode* aNode = popHeap();
			aNode->mClosed = true;
			if (sameCell(aNode->mCell, mGoal))
			{
				for (PathNode* aStep = aNode; aStep != nullptr; aStep = aStep->mParent)
					aPath.push_back(aStep->mCell);
				std::reverse(aPath.begin(), aPath.end());
				return aPath;
			}
			expand(aNode);
		}
		return aPath;
	}

private:
	bool isOnGrid(const engine::IntPair& theCell) const
	{
		return -1 < theCell.mFirst && theCell.mFirst < mMap->getColumns() && -1 < theCell.mSecond &&
		       theCell.mSecond < mMap->getRows();
	}
	PathNode& nodeAt(const engine::IntPair& theCell) { return mNodes[theCell.mFirst * mRows + theCell.mSecond]; }
	static int distance(const engine::IntPair& theFrom, const engine::IntPair& theTo)
	{
		return std::abs(theFrom.mFirst - theTo.mFirst) + std::abs(theFrom.mSecond - theTo.mSecond);
	}
	// 0x429770's cousin: a road is at the cell
	bool isPassable(const engine::IntPair& theCell)
	{
		if (!isOnGrid(theCell))
			return false;
		Tile* aTile = mMap->getTileAt(theCell);
		return aTile != nullptr && aTile->isRoadAt(theCell);
	}
	// 0x429770: more than two roads lead from the cell
	bool isJunction(const engine::IntPair& theCell)
	{
		int aRoads = 0;
		for (int i = 0; i < 4; i++)
		{
			engine::IntPair aNeighbour = {theCell.mFirst + SEARCH_DIRECTIONS[i].mFirst,
			                              theCell.mSecond + SEARCH_DIRECTIONS[i].mSecond};
			if (isPassable(aNeighbour))
				aRoads++;
		}
		return 2 < aRoads;
	}

	// The heap is the MSVC's, a minimum on the nodes' f (0x429a90, 0x429ae0, 0x429c40, 0x429e50)
	void adjustHeap(size_t theHole, size_t theBottom, PathNode* theValue)
	{
		size_t aTop = theHole;
		size_t anIndex = theHole;
		while (anIndex < (theBottom - 1) / 2)
		{
			anIndex = 2 * (anIndex + 1);
			if (mHeap[anIndex - 1]->mF < mHeap[anIndex]->mF)
				anIndex--;
			mHeap[theHole] = mHeap[anIndex];
			theHole = anIndex;
		}
		if (theBottom % 2 == 0 && theHole == (theBottom - 2) / 2)
		{
			mHeap[theHole] = mHeap[theBottom - 1];
			theHole = theBottom - 1;
		}
		while (aTop < theHole && theValue->mF < mHeap[(theHole - 1) / 2]->mF)
		{
			mHeap[theHole] = mHeap[(theHole - 1) / 2];
			theHole = (theHole - 1) / 2;
		}
		mHeap[theHole] = theValue;
	}
	void pushHeap(PathNode* theNode)
	{
		theNode->mInHeap = true;
		mHeap.push_back(theNode);
		size_t aHole = mHeap.size() - 1;
		while (0 < aHole && theNode->mF < mHeap[(aHole - 1) / 2]->mF)
		{
			mHeap[aHole] = mHeap[(aHole - 1) / 2];
			aHole = (aHole - 1) / 2;
		}
		mHeap[aHole] = theNode;
	}
	PathNode* popHeap()
	{
		PathNode* aTop = mHeap[0];
		PathNode* aLast = mHeap.back();
		if (1 < mHeap.size())
		{
			mHeap[mHeap.size() - 1] = mHeap[0];
			adjustHeap(0, mHeap.size() - 1, aLast);
		}
		mHeap.pop_back();
		aTop->mInHeap = false;
		return aTop;
	}
	void makeHeap()
	{
		size_t aCount = mHeap.size();
		for (size_t aHole = aCount / 2; 0 < aHole;)
		{
			aHole--;
			adjustHeap(aHole, aCount, mHeap[aHole]);
		}
	}

	// the cells the node leads to: along each road to the next crossing or to the goal
	void expand(PathNode* theNode)
	{
		for (int i = 0; i < 4; i++)
		{
			const engine::IntPair& aDirection = SEARCH_DIRECTIONS[i];
			engine::IntPair anEnd = theNode->mCell;
			int aRun = 0;
			engine::IntPair aPosition = {anEnd.mFirst + aDirection.mFirst, anEnd.mSecond + aDirection.mSecond};
			while (isPassable(aPosition))
			{
				if (isJunction(aPosition) || sameCell(aPosition, mGoal))
				{
					anEnd = aPosition;
					break;
				}
				anEnd = aPosition;
				aRun += nodeAt(aPosition).mCost;
				aPosition = engine::IntPair{anEnd.mFirst + aDirection.mFirst, anEnd.mSecond + aDirection.mSecond};
			}
			if (sameCell(anEnd, theNode->mCell))
				continue;
			PathNode* anEndNode = &nodeAt(anEnd);
			if (anEndNode == theNode->mParent || !isPassable(anEnd))
				continue;
			int aNewCost = theNode->mG + anEndNode->mCost + aRun;
			if (anEndNode->mInHeap)
			{
				if (aNewCost < anEndNode->mG)
				{
					anEndNode->mG = aNewCost;
					anEndNode->mParent = theNode;
					anEndNode->mF = anEndNode->mH + aNewCost;
					if (1 < mHeap.size())
						makeHeap();
				}
			}
			else if (anEndNode->mClosed)
			{
				// 0x429820 (the propagation to the nodes it came to) never goes further
				if (aNewCost < anEndNode->mG)
				{
					anEndNode->mParent = theNode;
					anEndNode->mG = aNewCost;
					anEndNode->mH = distance(anEnd, mGoal);
					anEndNode->mF = anEndNode->mG + anEndNode->mH;
					pushHeap(anEndNode);
				}
			}
			else
			{
				anEndNode->mParent = theNode;
				anEndNode->mG = aNewCost;
				anEndNode->mH = distance(anEnd, mGoal);
				anEndNode->mF = anEndNode->mG + anEndNode->mH;
				pushHeap(anEndNode);
			}
		}
	}

	CityMap* mMap;
	engine::IntPair mGoal;
	engine::IntPair mStart;
	int mRows = 0;
	std::vector<PathNode> mNodes;
	std::vector<PathNode*> mHeap;
};
}

std::vector<engine::IntPair> pizzafrenzy::findPath(CityMap* theMap, const engine::IntPair& theStart,
                                                   const engine::IntPair& theGoal)
{
	return PathSearch(theMap, theStart, theGoal).run();
}

// 0x430860
Van::Van(const std::string& theTypeName) :
	mMap(nullptr),
	mCell{0, 0},
	mHomeCell{0, 0},
	mNext(0),
	mProgress(0),
	mSpeed(0),
	mDirty(false),
	mSegmentStart{0, 0},
	mLayer(LAYER_GROUND),
	mHome(nullptr),
	mPhase(PHASE_PARKED),
	mArrivedTime(0)
{
	setName(theTypeName);
}

// 0x42f920
Van::~Van()
{
}

// 0x42f690
void Van::setup(CityMap* theMap, const engine::IntPair& theCell, BuildingTile* theTile)
{
	mMap = theMap;
	mHome = theTile;
	mHomeCell = theCell;
	placeAt(theCell);
	layout();
	theMap->addVehicle(this);
	faceRoad();
	mDirty = false;
}

// 0x42fb80
void Van::placeAt(const engine::IntPair& theCell)
{
	if (mMap == nullptr)
		return;
	engine::IntPair aPoint = mMap->cellToPoint(theCell);
	setPosition((float)(aPoint.mFirst - mMap->getCellSize() / 2), (float)(aPoint.mSecond - mMap->getCellSize() / 2));
	mCell = theCell;
	mImages->select(IMAGE_EAST_WEST, 0.0f, true);
	setOpacity(1.0f);
}

// 0x42f770
void Van::leave()
{
	addOwnFlags(engine::NODE_REMOVE);
}

// 0x430070
void Van::loadType()
{
	Vehicle* aType = static_cast<Vehicle*>(PizzaFrenzy::instance()->mTileManifest->getTile(getName(), VEHICLE_CLASS));
	makeImages(aType);
	mLayer = LAYER_GROUND;
	mSpeed = (float)aType->mSpeed;
	park();
}

// 0x42fda0
void Van::park()
{
	mPhase = PHASE_PARKED;
	mPath.clear();
	if (mEmitter != nullptr)
		mEmitter->stop();
	if (mImages != nullptr && IMAGE_PARKED < mImages->getCount())
		mImages->select(IMAGE_PARKED, 0.0f, true);
	placeAt(mHomeCell);
	faceRoad();
	mDirty = true;
}

// 0x42f780
void Van::updateVehicle(engine::UpdateInfo& theInfo)
{
	if (mPhase == PHASE_DRIVING)
		move(theInfo);
	else if (mPhase == PHASE_ARRIVED)
	{
		mArrivedTime -= theInfo.mSeconds;
		if (mArrivedTime <= 0.0f)
		{
			finish();
			park();
		}
	}
	Container::update(theInfo);
}

// 0x430480
void Van::arriveSound()
{
	playSound(SOUND_ARRIVE);
}

// 0x42fe60
void Van::arrive()
{
	mPhase = PHASE_ARRIVED;
	mArrivedTime = ARRIVED_SECONDS;
	if (IMAGE_ARRIVED < mImages->getCount())
		mImages->select(IMAGE_ARRIVED, 0.0f, true);
	arriveSound();
}

// 0x42fd20
void Van::finish()
{
	mTarget->finish();
	mPath.clear();
	mHome->removeVehicle(this);
}

// 0x430130
void Van::startPath(const engine::IntPair& theCell)
{
	mPath = findWay(theCell);
	if (!mPath.empty())
	{
		mNext = 1;
		mSegmentStart = mCell;
		mProgress = 0;
		if (mNext < mPath.size())
			face(mPath[mNext]);
	}
}

// 0x4302f0
void Van::goTo(CustomerTile* theTile)
{
	mTarget = theTile;
	engine::IntPair aCell = theTile->getRoadCell();
	if (mPath.empty() && !sameCell(mCell, aCell))
	{
		playSound(mSpeed <= CAR_SPEED ? SOUND_LEAVE_CAR : SOUND_LEAVE_VAN);
		startPath(aCell);
		if (!mPath.empty())
		{
			mPhase = PHASE_DRIVING;
			// the driving state's entry
			if (mEmitter != nullptr)
				mEmitter->start();
		}
	}
}

// 0x42f900
std::vector<engine::IntPair> Van::findWay(const engine::IntPair& theCell)
{
	return findPath(mMap, mCell, theCell);
}

// 0x42f700
void Van::faceRoad()
{
	if (mMap == nullptr)
		return;
	for (int i = 0; i < 4; i++)
	{
		engine::IntPair aCell = {mCell.mFirst + SEARCH_DIRECTIONS[i].mFirst,
		                         mCell.mSecond + SEARCH_DIRECTIONS[i].mSecond};
		if (mMap->isRoadAt(aCell))
		{
			face(aCell);
			return;
		}
	}
}

// 0x42f940: the effect of the type comes first, so the pictures draw over it
void Van::makeImages(Vehicle* theType)
{
	ParticleEffect* anEffect = PizzaFrenzy::instance()->mTileManifest->getParticleEffect(theType->mEffect);
	if (anEffect != nullptr)
	{
		mEmitter = new engine::SignalParticleSystem();
		mEmitter->setUp(anEffect->mDef);
		addChild(mEmitter);
	}
	mImages = new engine::Selector();
	addChild(mImages);
	engine::Image* anImages[3] = {theType->mImage, theType->mNorthImage, theType->mSouthImage};
	for (int i = 0; i < 3; i++)
	{
		anImages[i]->setPivot(1);
		mImages->addChild(new engine::ImageNode(anImages[i]));
	}
	mImages->select(IMAGE_EAST_WEST, 0.0f, true);
}

// 0x42fc30: the picture is the east and west one for a way across, the north's or south's for one along
void Van::face(const engine::IntPair& theCell)
{
	engine::IntPair aDifference = {theCell.mFirst - mCell.mFirst, theCell.mSecond - mCell.mSecond};
	if (aDifference.mFirst == 0)
		mImages->select(aDifference.mSecond < 0 ? IMAGE_NORTH : IMAGE_SOUTH, 0.0f, true);
	else
		mImages->select(IMAGE_EAST_WEST, 0.0f, true);
	setScale(aDifference.mFirst < 0 ? -1.0f : 1.0f, 1.0f);
}

// 0x42f650
void Van::clearParent(engine::Node* theParent)
{
	Container::clearParent(theParent);
	mMap->removeVehicle(this);
}

// 0x430510: along the straight lines from one cell of the way to the next
void Van::move(engine::UpdateInfo& theInfo)
{
	int aRowBefore = (int)getY();
	float aRemaining = mSpeed * theInfo.mSeconds;
	int aCellSize = mMap->getCellSize();
	while (0.0f < aRemaining)
	{
		float aVectorX = (float)(mPath[mNext].mFirst - mSegmentStart.mFirst);
		float aVectorY = (float)(mPath[mNext].mSecond - mSegmentStart.mSecond);
		float aLength = std::sqrt(aVectorX * aVectorX + aVectorY * aVectorY) * (float)aCellSize;
		float aSegment = (1.0f - mProgress) * aLength;
		if (aRemaining < aSegment)
		{
			float aUnit = std::sqrt(aVectorX * aVectorX + aVectorY * aVectorY);
			float aPositionX = getX() + aVectorX / aUnit * aRemaining;
			float aPositionY = getY() + aVectorY / aUnit * aRemaining;
			float aHalfCell = (float)aCellSize * 0.5f;
			engine::IntPair aPoint = {(int)(aPositionX + aHalfCell), (int)(aPositionY + aHalfCell)};
			engine::IntPair aTestCell = mMap->pointToCell(aPoint);
			if (!mMap->isBlockedAt(aTestCell))
			{
				setPosition(aPositionX, aPositionY);
				mCell = aTestCell;
				mProgress += aRemaining / aLength;
			}
			break;
		}
		aRemaining -= aSegment;
		mSegmentStart = mPath[mNext];
		mCell = mPath[mNext];
		engine::IntPair aPoint = mMap->cellToPoint(mCell);
		setPosition((float)(aPoint.mFirst - aCellSize / 2), (float)(aPoint.mSecond - aCellSize / 2));
		mProgress = 0;
		mNext++;
		if (mNext == mPath.size())
		{
			arrive();
			break;
		}
		face(mPath[mNext]);
	}
	if ((int)getY() != aRowBefore)
		mDirty = true;
}

// 0x432d60
PoliceCar::PoliceCar() : Van(POLICE_CAR_TYPE)
{
}

// 0x432a60
void PoliceCar::makeImages(Vehicle* theType)
{
	Van::makeImages(theType);
	engine::App* anApp = engine::App::instance();
	engine::Image* aRedImage = anApp->getImage(RED_LIGHT_IMAGE);
	aRedImage->setPivot(1);
	mRedLight = new engine::ImageNode(aRedImage);
	engine::Image* aBlueImage = anApp->getImage(BLUE_LIGHT_IMAGE);
	aBlueImage->setPivot(1);
	mBlueLight = new engine::ImageNode(aBlueImage);
	mLights = new engine::AnimImage();
	mLights->addChild(mRedLight);
	mLights->addChild(mBlueLight);
	mLights->finishLoading();
	mLights->setRate(LIGHTS_RATE);
	mLights->play();
	addChild(mLights);
}

// 0x432e30
void PoliceCar::finish()
{
	Order* anOrder = mTarget->getOrder();
	if (anOrder == nullptr || anOrder->asksForPizza())
	{
		mTarget->hangUp();
		PizzaFrenzy* aGame = PizzaFrenzy::instance();
		aGame->mMode->action(ACTION_FALSE_ARREST);
		engine::Point aPosition = *getPosition();
		aGame->mSplatFactory->create(FALSE_ARREST_SPLAT, aPosition.x, aPosition.y);
	}
	else
	{
		mTarget->finish();
		playSound(SOUND_POLICE_ARREST);
	}
	mPath.clear();
	mHome->removeVehicle(this);
}

// 0x433010
void PoliceCar::goTo(CustomerTile* theTile)
{
	Van::goTo(theTile);
	playSound(SOUND_POLICE);
}
