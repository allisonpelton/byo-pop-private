#include "Stage.h"

#include "Actor.h"
#include "DrawContext.h"
#include "Engine.h"
#include "GameData.h"
#include "Params.h"
#include "Spawn.h"
#include "VectorMath.h"
#include "Visual.h"

#include <algorithm>
#include <cstdint>

using namespace engine;

namespace
{

const float HUD_BAND_HEIGHT = 75.0f;        // 0x42960000 in 0x4146b0 (feedingfrenzy2): the play bounds' top in a game
const float LAYER_MARGIN_X = 200.0f;        // 0x4858d0's arguments in 0x4146b0 (feedingfrenzy2)
const float LAYER_MARGIN_Y = 160.0f;
const float DEFAULT_SIZE_UNIT = 16.0f;      // 0x5667b8 (feedingfrenzy2)
const float DEFAULT_MIN_LIGHT = 0.3f;       // 0x54ddac (feedingfrenzy2)
const float DEFAULT_MAX_LIGHT = 0.9f;       // 0x566934 (feedingfrenzy2)
const float STEP_SECONDS = 0.01f;           // 0x54dd9c (feedingfrenzy2): a step of the run, as a float
const double STEP_SECONDS_DOUBLE = 0.01;    // 0x566d58 (feedingfrenzy2): the same as a double, taken off the time left
const double CAMERA_CENTRE = 0.5;           // 0x566760 (feedingfrenzy2): the start's camera, half across the range
const double HALF = 0.5;                    // 0x566760 (feedingfrenzy2)
const double ZERO = 0.0;                    // 0x566828 (feedingfrenzy2)

// An edge set to its normal and distance
void setEdge(shape::Edge& theEdge, float theNormalX, float theNormalY, int theDistance)
{
	theEdge.mDistance = (float)theDistance;
	theEdge.mNormal = Vector2f(theNormalX, theNormalY);
}

// Every match taken out of the list (std::remove and erase)
template <class T>
void removeFrom(std::vector<T*>& theList, T* theItem)
{
	theList.erase(std::remove(theList.begin(), theList.end(), theItem), theList.end());
}

} // namespace

void MaskContainer::enable()
{
	if (mMaskOn)
		return;
	// The mask's key interpolator (+0xcc) cleared and given its two keys, the mask's fade (+0x164) started over 0.4
	// to 1 second, and the mask on (0x4283f0 and 0x480250 (feedingfrenzy2)): not yet translated, as the night's
	// stages are not
	getEngine()->log("MaskContainer: the night's mask not yet translated\n");
	mMaskOn = true;
}

void MaskContainer::paint(DrawContext* theContext)
{
	// With the mask on, the children are drawn under the darkness mask's image, cut around the player (the rest of
	// 0x44f1a0 (feedingfrenzy2)): not yet translated, so they are drawn as with it off
	Container::paint(theContext);
}

Stage::Stage(GameData* theGameData, Container* theContainer)
	: mScrollRate(0.0f), mSizeUnit(DEFAULT_SIZE_UNIT), mBonus(false), mStepTime(0.0f), mWaterColor(0.0f, 0.0f, 0.0f, 0.0f),
	  mCameraFollows(false), mMaskFadeTime(0.0f), mNight(false), mLight(0.0f), mLightTarget(0.0f),
	  mMinLight(DEFAULT_MIN_LIGHT), mMaxLight(DEFAULT_MAX_LIGHT), mBoss(false), mFinalBoss(false)
{
	mGameData = theGameData;
	mContainer = theContainer;
	// The init (0x4038d0 (feedingfrenzy2)): the stage's own theme
	mTheme = new Theme();
	mTheme->load("resource/themes/baseTheme.xml");
	// The game's HUD (GameHud, 0x260 bytes, at +0x2e0, over the theme), the stun bubbles' effect (StunBubbleFx, 0xf8
	// bytes, at +0x308) and the theme's splashFxBig, splashFxMedium and splashFxSmall (+0x30c to +0x314): not yet
	// translated
	getEngine()->log("Stage: the HUD, the stun bubbles and the splashes not yet translated\n");
}

bool Stage::load(const StageManifest::Stage& theEntry)
{
	// 0x48f5c0 (feedingfrenzy2)
	mEntry = theEntry;
	MasterDocHandler aMaster;
	StageDocHandler* aHandler = new StageDocHandler(this);
	aHandler->attach(&aMaster, "stage");
	if (!getEngine()->parseXml(mEntry.mResource, &aMaster))
		return false;
	// The spawns' steps (each one's slot 0x14 over +0x280, four more when the stage has players) are the loader's
	// progress bar's; the port loads at once and shows none
	int aProgress = 0;
	for (size_t i = 0; i < mSpawners.size(); i++)
		mSpawners[i]->setup(aProgress);
	setupLayers();
	// The rest (0x405590, 0x45ef40, the slots 0x38 and 0x3c, 0x416330, the Orphanage at +0x2c0, the splashes'
	// containers on the first layer's actors, the slots 0x40 and 0x44 for a stage with players, and 0x4719f0
	// (feedingfrenzy2)): not yet translated
	getEngine()->log("Stage: the stage's actors not yet translated\n");
	return true;
}

void Stage::setupLayers()
{
	Engine* anEngine = getEngine();
	float aHeight = (float)anEngine->getHeight();
	float aWidth = (float)anEngine->getWidth();
	mBounds = RectF(0.0f, 0.0f, aWidth, aHeight);
	// The play bounds start below the HUD's band in a stage with players (0x491730 (feedingfrenzy2) over +0x290):
	// the spawns are not yet translated, so the stage has none, as a menu stage has none
	bool hasPlayers = false;
	if (hasPlayers)
		mBounds.top = HUD_BAND_HEIGHT;
	mMaskContainer = new MaskContainer();
	mMaskContainer->setClipRect(mBounds.left, mBounds.top, mBounds.right, mBounds.bottom);
	// The mask's image (resource/hud/darknessMask.png, through the engine's slot 0x6c), kept at +0xb8, and its
	// centre at (400, 300) (+0xbc): not yet translated
	mContainer->addChild(mMaskContainer.get());
	if (!mNight)
	{
		mMaskContainer->setMaskOn(false);
		mLight = 0.0f;
		mLightTarget = 0.0f;
	}
	else
	{
		mMaskContainer->setMaskOn(true);
		mLight = mMinLight;
		mLightTarget = mMinLight;
		// +0x304, 0: the light's timer (not yet translated)
	}
	// +0x1cc: the engine's slot 0x24 (not yet translated)
	mLayerSet = new LayerSet();
	mLayerSet->load(mSetPath);
	mLayerSet->fit(LAYER_MARGIN_X, LAYER_MARGIN_Y, mBounds);
	mLayerSet->setPosition(mBounds.left, mBounds.top);
	mMaskContainer->addChild(mLayerSet.get());
	// The mask's light from the base layer's position and size (0x49ab50, 0x49ab30 and 0x4882a0 (feedingfrenzy2),
	// with the set's night light): not yet translated, as the night's mask is not
	getLayerContainer(0)->setName("PlayGroup");
	// 0x48cdf0 (feedingfrenzy2): the base layer's pan size from the origin, through whole numbers (0x475dd0 and
	// 0x49ab10 (feedingfrenzy2)), scaled by 1 (0x49aae0 (feedingfrenzy2))
	const Vector2f& aPanSize = mLayerSet->getInfo(mLayerSet->getBaseLayer())->getPanSize();
	int aLeft = 0;
	int aTop = 0;
	int aRight = (int)aPanSize.x;
	int aBottom = (int)aPanSize.y;
	mPlayArea = RectF((float)aLeft, (float)aTop, (float)aRight, (float)aBottom);
	// 0x48f620 (feedingfrenzy2): the box of the area
	float aHalfWidth = (float)(((double)mPlayArea.right - mPlayArea.left) * HALF);
	mViewBox.mDiagonal.x = aHalfWidth;
	float aHalfHeight = (float)(((double)mPlayArea.bottom - mPlayArea.top) * HALF);
	mViewBox.mDiagonal.y = aHalfHeight;
	mViewBox.mCenter.x = (float)((double)aHalfWidth + mPlayArea.left);
	mViewBox.mCenter.y = (float)((double)mPlayArea.top + aHalfHeight);
	int aWater = mLayerSet->getWaterLevel();
	int aSky = mLayerSet->getSkyCeiling();
	setEdge(mEdges[STAGE_EDGE_BELOW_WATER], 0.0f, 1.0f, -aWater);
	setEdge(mEdges[STAGE_EDGE_SKY], 0.0f, 1.0f, -aSky);
	setEdge(mEdges[STAGE_EDGE_ABOVE_WATER], 0.0f, -1.0f, aWater);
	setEdge(mEdges[STAGE_EDGE_BOTTOM], 0.0f, -1.0f, aBottom);
	setEdge(mEdges[STAGE_EDGE_LEFT], 1.0f, 0.0f, -aLeft);
	setEdge(mEdges[STAGE_EDGE_RIGHT], -1.0f, 0.0f, aRight);
}

RectF Stage::getArea(bool underwater) const
{
	RectF anArea = mPlayArea;
	if (underwater)
		anArea.top = (float)mLayerSet->getWaterLevel();
	else
	{
		anArea.top = (float)mLayerSet->getSkyCeiling();
		anArea.bottom = (float)mLayerSet->getWaterLevel();
	}
	return anArea;
}

Container* Stage::getLayerContainer(int theLayer)
{
	return mLayerSet->getLayer(mLayerSet->getBaseLayer() + theLayer)->getActors();
}

void Stage::removeActor(Actor* theActor)
{
	removeFrom(mActors, theActor);
}

void Stage::removeFish(Fish* theFish)
{
	removeFrom(mFish, theFish);
}

void Stage::removeAIFish(AIFish* theFish)
{
	removeFrom(mAIFish, theFish);
}

const std::vector<Actor*>& Stage::findSeenActors(AIFish* theViewer)
{
	mSeen.clear();
	FishBehaviorInfo* aBehavior = theViewer->getFishBehavior();
	for (Actor* anActor : mActors)
	{
		if (!(0.0f < aBehavior->mSightDistance))
			continue;
		if (theViewer->isUnderwater() != anActor->isUnderwater())
			continue;
		Vector2f aMouth = theViewer->getMouthPosition();
		Vector2f aWay = difference(anActor->getPosition(), aMouth);
		double aSightDistance = aBehavior->mSightDistance;
		if (!(aSightDistance * aSightDistance >= lengthSquared(aWay)))
			continue;
		Vector2f aDirection = aWay;
		normalize(aDirection);
		// The way the viewer faces is straight across
		float aFacing = theViewer->getFishVisual()->getFacing();
		float aCos = (float)(aDirection.x * aFacing + aDirection.y * ZERO);
		if (!(aBehavior->mSightCos < aCos))
			continue;
		float aDistance = length(aWay);
		if (!(aDistance >= 0.0f))
			continue;
		std::vector<Actor*>::iterator aPlace = mSeen.begin();
		for (; aPlace != mSeen.end(); ++aPlace)
		{
			if (aDistance < length(difference((*aPlace)->getPosition(), aMouth)))
				break;
		}
		mSeen.insert(aPlace, anActor);
	}
	return mSeen;
}

void Stage::updateActors(UpdateContext* theContext)
{
	// The mask's lights (+0x190 of the mask container) cleared: not yet translated, as the night's mask is not
	std::vector<Actor*> anActors = mActors;
	for (Actor* anActor : anActors)
	{
		if (std::find(mActors.begin(), mActors.end(), anActor) == mActors.end())
		{
			getEngine()->log("Actor %08x has vanished from the list.\n", (unsigned int)(uintptr_t)anActor);
			continue;
		}
		anActor->update(*theContext);
	}
}

void Stage::updateFishVisuals(UpdateContext* theContext)
{
	for (Fish* aFish : mFish)
	{
		FishVisual* aVisual = aFish->getFishVisual();
		aVisual->setVelocity(aFish->hasTurned() ? aFish->getDesiredVelocity() : aFish->getVelocity());
		aVisual->setTurnBias(mScrollRate);
		aFish->setShapeScale(aVisual->getMirrorScale(), aVisual->getPulseScale());
	}
	// At night, the light's step (the stage's 0x70): not yet translated, as the night's stages are not
}

void Stage::start()
{
	pause(false);
	// The game data's +0x30, +0x34, +0x38 and +0x40 set from its +0x10 and +0x14 and to 0, for the HUD: not yet
	// translated
	mGameData->setSpawnPhase(0);
	mMaskFadeTime = 0.0f;
	mCameraFollows = true;
	// +0x270 (the boss, null) and +0x391 (false) reset, and the stage's timer at +0x238 cleared (0x406fb0
	// (feedingfrenzy2)): read by nothing translated yet
	// The stage's time (+0x190 and +0x198: in a bonus stage kept, else in the timed mode, the game data's +0x0c at 1,
	// added to by its +0x2c, else 0) shown on the HUD with the lives and the player's name (0x44c910, 0x494620,
	// 0x402550, 0x4050d0 and 0x44b4f0 (feedingfrenzy2)): not yet translated, as the HUD is not
	// The ambient sounds (+0x318) each given its first delay (0x486810 and 0x486860 (feedingfrenzy2), a draw of the
	// random numbers each): not yet translated, as the sounds are not, so the draws after them are off by theirs
	// +0x3b0, 0: read by nothing translated yet
	// A stage with players (+0x340) starts them and puts the camera on them: not yet translated, as the players are
	// not, so every stage starts as one without
	startPlayers();
	const RectF& aRange = mLayerSet->getPanRange();
	float aTop = aRange.top;
	// 0x459e00 (feedingfrenzy2): the range's width
	float aWidth = (float)((double)aRange.right - aRange.left);
	float aCentre = (float)(aWidth * CAMERA_CENTRE);
	mLayerSet->setCamera(Vector2f(aCentre, aTop));
}

void Stage::startPlayers()
{
	// With players (+0x290), the game data's +0x10 at more than 0: the game data's +0x40 from the first player's
	// (or the boss's, 0x5ac620), the player's slot 0x1c, and the run; at 0 or less, the machine emptied: not yet
	// translated, as the players are not
	startRun();
}

void Stage::startRun()
{
	mStateMgr.changeState(new RunState(this));
}

void Stage::pause(bool thePaused)
{
	if (thePaused)
		mContainer->addOffState(STATE_ANIMATED);
	else
		mContainer->removeOffState(STATE_ANIMATED);
	// The players (+0x340) paused or resumed with the stage when it has some (+0x290; 0x49d4a0 and 0x497040
	// (feedingfrenzy2)), and the script (+0x31c, 0x502648 (feedingfrenzy2)): not yet translated
}

void Stage::updateSpawns(UpdateContext* theContext)
{
	// A stage's script with waves (+0x210, the waves' times at +0x218, the next at +0x224 and the time at +0x228)
	// holds the spawns until they are through: not yet translated, so the spawns are never held
	bool canSpawn = true;
	for (size_t i = 0; i < mSpawners.size(); i++)
		mSpawners[i]->update(theContext, canSpawn);
}

void Stage::update(UpdateContext* theContext)
{
	if (0.0f < mMaskFadeTime)
	{
		// A night's mask fading (its time down by the frame's, the mask's slot 0x18 given two draws from +0x268, or
		// (0, 0) once it is out): not yet translated, as the night's stages are not
	}
	float aStepTime = theContext->mElapsed + mStepTime;
	mStepTime = aStepTime;
	float anElapsed = theContext->mElapsed;
	int aSteps = 0;
	while (STEP_SECONDS <= mStepTime)
	{
		aSteps++;
		mStepTime = (float)(mStepTime - STEP_SECONDS_DOUBLE);
		theContext->mElapsed = STEP_SECONDS;
		StageState* aState = mStateMgr.getState();
		if (aState != nullptr)
			aState->update(theContext);
	}
	if (aSteps == 0)
	{
		theContext->mElapsed = mStepTime;
		StageState* aState = mStateMgr.getState();
		if (aState != nullptr)
			aState->update(theContext);
		mStepTime = 0.0f;
	}
	theContext->mElapsed = anElapsed;
}

void RunState::update(UpdateContext* theContext)
{
	// The script (the stage's +0x31c) checked first (its slot 0xe4): not yet translated
	// Without a boss in play (+0x270), the stage's slots 0x48, 0x4c, 0x50 and 0x54 (the actors' updates and their
	// meetings), 0x58 (the spawns), 0x60 (the camera), 0x68 and 0x6c; with one, 0x68, 0x6c and the boss's 0xf0; then
	// 0x5c (0x447ef0 (feedingfrenzy2)). Translated so far: the actors' updates, the spawns and the fish's visuals;
	// the players' steps (0x4c, 0x54 and 0x60) have nothing to do without players; the fish's meetings (0x50,
	// 0x440660 (feedingfrenzy2)), the ambient sounds (0x6c) and the clean-up (0x5c) are not yet
	mStage->updateActors(theContext);
	mStage->updateSpawns(theContext);
	mStage->updateFishVisuals(theContext);
}

void StageDocHandler::begin(Params& theAttributes)
{
	mStage->mSizeUnit = theAttributes.getFloat("sizeUnit", DEFAULT_SIZE_UNIT);
	mStage->mScrollRate = theAttributes.getFloat("scrollRate", 0.0f);
	mStage->mSetPath = theAttributes.getString("set", "");
	mStage->mNight = theAttributes.getBool("night", false);
	mStage->mBoss = theAttributes.getBool("boss", false);
	mStage->mFinalBoss = theAttributes.getBool("finalBoss", false);
	mStage->mMinLight = theAttributes.getFloat("minLight", DEFAULT_MIN_LIGHT);
	mStage->mMaxLight = theAttributes.getFloat("maxLight", DEFAULT_MAX_LIGHT);
	mStage->mBonus = theAttributes.getBool("bonus", false);
	mStage->mGameMusic = theAttributes.getString("gameMusic", "");
	mStage->mAmbientSound = theAttributes.getString("ambientSound", "resource/sounds/ambience1.xml");
	mStage->mWaterColor = theAttributes.getColor("waterColor", Color(0.0f, 0.0f, 0.0f, 0.0f));
	mStage->mBossLayout = theAttributes.getString("bossLayout", "");
	// The handler's +0x50: the engine's slot 0xe4, a 64-bit value (not yet translated)
}

void StageDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	// The engine's clock read twice (its slots 0xe4 and 0xe8), for nothing kept
	if (theName == "player")
	{
		// A player's spawn (PlayerSpawn, 0xa4 bytes, its constructor 0x493fc0 (feedingfrenzy2)) read by its own
		// handler: not yet translated
		getEngine()->log("Stage: the player's spawn not yet translated\n");
		return;
	}
	if (theName == "spawn")
	{
		// 0x482e30 (feedingfrenzy2)
		std::string aClassName = theAttributes.getString("class", "");
		// 0x40a670 (feedingfrenzy2): in the timed mode (the game data's +0x0c at 1) the star and the extra life come
		// as the bonus of time; the mode is not yet translated
		ActorClassInfo* aClass = ActorClassRegistry::get()->find(aClassName);
		if (aClass == nullptr)
		{
			getEngine()->log("Dude, I don't know how to spawn '%s'.  Capitalization counts.\n", aClassName.c_str());
			return;
		}
		NonPlayerSpawn* aSpawn = aClass->createSpawn(mStage);
		SubDocHandler* aHandler = aSpawn->createDocHandler();
		aHandler->attach(mMaster, theName);
		aHandler->begin(theAttributes);
		return;
	}
	if (theName == "script" || theName == "epilog")
		// The stage's script (Script, 0x44 bytes, at +0x2cc) and its epilog (Epilog, 0x24 bytes, at +0x2d0), each
		// read by its own handler: not yet translated
		getEngine()->log("Stage: the %s not yet translated\n", theName.c_str());
}
