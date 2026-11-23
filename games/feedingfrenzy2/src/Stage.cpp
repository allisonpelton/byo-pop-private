#include "Stage.h"

#include "Actor.h"
#include "BonusItem.h"
#include "DrawContext.h"
#include "Engine.h"
#include "FeedingFrenzyTwo.h"
#include "GameData.h"
#include "GameHud.h"
#include "Params.h"
#include "PlayerFish.h"
#include "PlayerSpawn.h"
#include "Spawn.h"
#include "StringTable.h"
#include "UserData.h"
#include "VectorMath.h"
#include "Visual.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

using namespace engine;

namespace
{

const float HUD_BAND_HEIGHT = 75.0f;        // 0x42960000 in 0x4146b0 (feedingfrenzy2): the play bounds' top in a game
const float LAYER_MARGIN_X = 200.0f;        // 0x4858d0's arguments in 0x4146b0 (feedingfrenzy2)
const float LAYER_MARGIN_Y = 160.0f;
const float DEFAULT_SIZE_UNIT = 16.0f;      // 0x5667b8 (feedingfrenzy2)
const float AMBIENT_VOLUME = 0.7f;          // 0x3f333333 in 0x48f900 (feedingfrenzy2): the ambient sounds' layers
const float DEFAULT_MIN_LIGHT = 0.3f;       // 0x54ddac (feedingfrenzy2)
const float DEFAULT_MAX_LIGHT = 0.9f;       // 0x566934 (feedingfrenzy2)
const float STEP_SECONDS = 0.01f;           // 0x54dd9c (feedingfrenzy2): a step of the run, as a float
const double STEP_SECONDS_DOUBLE = 0.01;    // 0x566d58 (feedingfrenzy2): the same as a double, taken off the time left
const double CAMERA_CENTRE = 0.5;           // 0x566760 (feedingfrenzy2): the start's camera, half across the range
const double HALF = 0.5;                    // 0x566760 (feedingfrenzy2)
// The word at the stage's end (0x416330 (feedingfrenzy2): 0x566b4c, 0x566a18 and 0x54dd6c)
const float MESSAGE_X = 400.0f;
const float MESSAGE_Y = 200.0f;
const float MESSAGE_SPACING = 8.0f;
// The completion's marks and the word's bubbling in (0x4994b0 and 0x4334b0 (feedingfrenzy2): 0x54de28, 0x566c44
// and 0x54a334)
const int COMPLETION_MARK_COUNT = 3;
const float COMPLETION_BURST_TIME = 2.5f;
const float COMPLETION_END_TIME = 3.5f;
const float COMPLETION_APPEAR_TIME = 2.0f;
// The camera's path after the player's death (0x4034b0 (feedingfrenzy2): 0x56696c, 0x54dda0 and 0x54ddb0)
const float DIED_CAMERA_HOLD_TIME = 1.5f;
const float DIED_CAMERA_END_TIME = 3.0f;
const float DIED_CAMERA_STRAIGHT_SHARE = 0.5f;
const double ZERO = 0.0;                    // 0x566828 (feedingfrenzy2)
const float PLAYER_FLASH_SECONDS = 3.0f;    // 0x40400000 in 0x4f37cf (feedingfrenzy2)

// 0x4050d0 (feedingfrenzy2): the stage's number and the title of its screen data ("%d. %s"), or its id in brackets
// when it has none; empty before the first stage
std::string stageNameOf(FeedingFrenzyTwo* theApp)
{
	GameData* aData = theApp->mGameData.get();
	if (aData == nullptr || aData->getStage() < 0)
		return "";
	int aStage = aData->getStage();
	const std::string& anId = aData->getStages().getStages()[aStage].mId;
	ScreenDataEntry* aScreenData = theApp->getScreenData(anId);
	char aName[512];
	if (aScreenData == nullptr)
		snprintf(aName, sizeof(aName), "(%s)", anId.c_str());
	else
		snprintf(aName, sizeof(aName), "%d. %s", aStage + 1, aScreenData->mTitle.c_str());
	return aName;
}

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
	: mScrollRate(0.0f), mSizeUnit(DEFAULT_SIZE_UNIT), mBonus(false), mStageTime(0.0f), mTimeLeft(0.0f), mStepTime(0.0f),
	  mWaterColor(0.0f, 0.0f, 0.0f, 0.0f), mCanSuck(false), mCameraFollows(false), mMaskFadeTime(0.0f), mNight(false), mLight(0.0f), mLightTarget(0.0f),
	  mMinLight(DEFAULT_MIN_LIGHT), mMaxLight(DEFAULT_MAX_LIGHT), mCompleted(false), mBoss(false), mFinalBoss(false)
{
	mGameData = theGameData;
	mContainer = theContainer;
	// The init (0x4038d0 (feedingfrenzy2)): the stage's own theme
	mTheme = new Theme();
	mTheme->load("resource/themes/baseTheme.xml");
	mHud = new GameHud(this);
	mHud->setup(mTheme.get());
	// The stun bubbles' effect (StunBubbleFx, 0xf8 bytes, at +0x308) and the theme's splashFxBig, splashFxMedium and
	// splashFxSmall (+0x30c to +0x314): not yet translated
	getEngine()->log("Stage: the stun bubbles and the splashes not yet translated\n");
}

Stage::~Stage()
{
	unload();
}

void Stage::unload()
{
	// port: a player still in the stage taken out first. The original keeps it (the stage's players, +0x340, hold it,
	// and the next start takes it as the player already in); how the play's ends take it out is not yet translated,
	// so until it is, the unload does, so that no other stage runs it
	for (size_t i = 0; i < mPlayerSpawns.size(); i++)
		mPlayerSpawns[i]->takeOut();
	mPlayerSpawns.clear();
	mFishSpawns.clear();
	mSpawners.clear();
	mNonPlayerSpawns.clear();
	mSpawnsByName.clear();
	// The Orphanage (+0x2c0) let go of: not yet translated
	mContainer->removeAllChildren();
	mMaskContainer = nullptr;
	mLayerSet = nullptr;
	// The script's waves (+0x210, their times at +0x218, the next at +0x224 and the time at +0x228) and the stage's
	// timer (+0x238, likewise) emptied (0x406fb0 (feedingfrenzy2)): not yet translated, as the script is not
	// The game's music (+0x31c), its slot 0xec and its release (0x4a3c50 (feedingfrenzy2)): not yet translated
	if (mAmbientSounds)
		mAmbientSounds->stop();
	mAmbientSounds = nullptr;
	// The stun bubbles' effect (+0x308, 0x4049c0 (feedingfrenzy2)) and the three splashes (+0x30c to +0x314, 0x4322f0
	// and 0x42a650 (feedingfrenzy2)): not yet translated
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
	// progress bar's; the port loads at once and shows none. The layers come first (slot 0x30), as the spawns' setup
	// keeps their fish to the water the layer set gives
	setupLayers();
	int aProgress = 0;
	for (size_t i = 0; i < mSpawners.size(); i++)
		mSpawners[i]->setup(aProgress);
	// The fish spawns' ranks from the first player spawn's (0x405590 (feedingfrenzy2)) and the bonus stage's counts
	// reset (0x45ef40 (feedingfrenzy2)): not yet translated
	setupOverlays();
	setupMessage();
	// The slot 0x38 (the HUD's fish ranks' icons, 0x405c60 (feedingfrenzy2)): not yet translated
	loadAmbientSounds();
	// The rest (the Orphanage at +0x2c0, the splashes' containers on the first layer's actors, the slots 0x40 and
	// 0x44 for a stage with players, and 0x4719f0 (feedingfrenzy2)): not yet translated
	getEngine()->log("Stage: the stage's actors not yet translated\n");
	return true;
}

void Stage::updatePlay(UpdateContext* theContext)
{
	if (mPlayerSpawns.empty())
		return;
	// The jump's marks on the HUD (its +0x208 to +0x220, each passed shows the next with the jump's height effect):
	// not yet translated, as the jump's height is not
	PlayerSpawn* aSpawn = mPlayerSpawns.front().get();
	PlayerFish* aFish = aSpawn->getFish();
	if (!aSpawn->isLive())
	{
		if (mBonus)
		{
			// On a bonus stage the word playerDied, the player's bonuses dropped and the stage's end with the sound
			// playerDie: not yet translated, as the bonus stages are not
			return;
		}
		// The player's bonuses dropped (0x4a2120 (feedingfrenzy2), the 2X's and the speed's): not yet translated, as
		// the bonuses are not
		if (mGameData->getMode() != GAME_MODE_TIMED)
			mGameData->setLives(mGameData->getLives() - 1);
		mHud->showLives(mGameData->getLives());
		mStateMgr.changeState(new PlayerDiedState(this));
		return;
	}
	updateHud(theContext);
	if (mGameData->getSpawnPhase() < aSpawn->getPhaseIndex())
		mGameData->setSpawnPhase(mGameData->getSpawnPhase() + 1);
	// The game data's +0x38 and +0x3c added the frame's time, and the stage's slot 0x64 when +0x2cc is set: not yet
	// translated
	if (!mBonus)
	{
		// 0x50a507 (feedingfrenzy2): the last phase's goal reached
		int aPhase = aSpawn->getPhaseIndex();
		bool isDone = aSpawn->getPhase(aPhase).mFoodGoal <= mGameData->getFood() &&
			aPhase + 1 == aSpawn->getPhaseCount();
		// Not with a boss in play (+0x270, not yet translated) or on a boss's stage
		if (isDone && !mBoss)
		{
			mCompleted = true;
			stopSpawns();
			if (aSpawn->getDeaths() == 0)
			{
				setMessage("stagePerfect");
				// The spawn "mermaid" given the spawn type mermaidStars and started, and the mermaid's state
				// (MermaidState, 0x51cc00 (feedingfrenzy2)), which ends in the completion with stageClear once she has
				// swum through: not yet translated. port: until it is, the completion follows at once
				mStateMgr.changeState(new CompletionState(this, "stageClear"));
				return;
			}
			setMessage("stageClear");
			mStateMgr.changeState(new CompletionState(this, "stageClear"));
			return;
		}
	}
	else
	{
		// The bonus stage's counts (0x45ef40 (feedingfrenzy2)) on the HUD, and its end once its fish are gone: not
		// yet translated
	}
	if (0.0f < mStageTime)
	{
		// The stage's timer (+0x238, 0x45f040 (feedingfrenzy2)) stepped, which holds the clock while its marks run:
		// not yet translated, as the script that sets it is not, so the clock always runs
		if (mTimeLeft <= 0.0f)
		{
			// 0x4a49a0 (feedingfrenzy2): the time's end waits while the fish is busy
			FishState* aState = aFish->getStateMgr().getState();
			if (aState != nullptr && aState->isBusy())
				return;
			// On a bonus stage its counts shown as "%d/%d" first: not yet translated
			setMessage("stageTimeUp");
			mStateMgr.changeState(new CompletionState(this, "gameOver"));
			return;
		}
		mTimeLeft = mTimeLeft - theContext->mElapsed;
		mHud->showTime((int)ceil((double)mTimeLeft));
	}
}

void Stage::updateHud(UpdateContext* theContext)
{
	PlayerSpawn* aSpawn = mPlayerSpawns.front().get();
	if (!aSpawn->isLive())
		return;
	if (!mBonus)
		mHud->getGrowthMeter()->setValue((float)mGameData->getFood());
	mHud->showTime((int)ceil((double)mTimeLeft));
	mHud->showLives(mGameData->getLives());
	mHud->showSuckCharge(aSpawn->getFish()->getSuckCharge());
	// The bonuses' bubbles (0x455920 and 0x455850 (feedingfrenzy2), the 2X's and the speed's by their timers):
	// not yet translated, as the bonuses are not
}

void Stage::setupOverlays()
{
	if (!mPlayerSpawns.empty())
	{
		mContainer->addChild(mHud.get());
		mHud->reset();
		if (!mBonus)
			mHud->setGrowthGoals(mPlayerSpawns.front().get());
		// On a bonus stage the HUD's count of its fish (0x4a3f70 (feedingfrenzy2) with +0x2c4): not yet translated
		mHud->showCounter(mBonus);
		mHud->showSuck(true);
	}
	mOverlays = new Container();
	mOverlays->setName("StageOverlays");
	mContainer->addChild(mOverlays.get());
	// The info font's text drawn by a signal (0x433940 (feedingfrenzy2)) into a component over the screen, and the
	// stun bubbles' effect (+0x308) in the overlays: not yet translated
}

void Stage::setupMessage()
{
	Font* aFont = mTheme->getFont("bubbleTextFont");
	Surface* aBubble = mTheme->getImage("bubbleTextBubble");
	ParticleSystemDef* anEffect = mTheme->getParticleDef("bubbleTextFx");
	mMessage = new BubbleText();
	mMessage->setPosition(MESSAGE_X, MESSAGE_Y);
	mMessage->setFont(aFont);
	mMessage->setBubble(aBubble);
	mMessage->setEffect(anEffect);
	mMessage->setSpacing(MESSAGE_SPACING);
	StringTable* aStrings = getApp()->mStrings.get();
	size_t aLongest = 0;
	const char* const MESSAGE_IDS[] = {"stageTimeUp", "stageClear", "stagePerfect", "playerDied"};
	for (const char* anId : MESSAGE_IDS)
		aLongest = maxOf(aLongest, aStrings->getString(anId).size());
	mMessage->addFreeGlyphs((int)aLongest);
}

void Stage::setMessage(const std::string& theStringId)
{
	mMessage->setText(getApp()->mStrings->getString(theStringId));
}

void Stage::showMessage(float theStart, float theDuration)
{
	mOverlays->addChild(mMessage.get());
	mMessage->appear(theStart, theDuration);
}

void Stage::burstMessage()
{
	mMessage->burst();
	// The theme's sound bubbleTextBurst at full volume: not yet translated, as the sounds are not
}

void Stage::hideMessage()
{
	mOverlays->removeChild(mMessage.get());
}

void Stage::stopSpawns()
{
	for (size_t i = 0; i < mNonPlayerSpawns.size(); i++)
		mNonPlayerSpawns[i]->setStart(SPAWN_START_LEAVE);
	// The orphanage's actors sent away too (+0x2c0, its slot 0x2c, 0x4a2420 (feedingfrenzy2)): not yet translated,
	// as the orphanage is not
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

void Stage::removeBonus(BonusItem* theBonus)
{
	removeFrom(mBonuses, theBonus);
}

Vector2f Stage::toScreen(int theLayer, const Vector2f& thePoint)
{
	Vector2f aLayer = getLayerContainer(theLayer)->getAbsolutePosition();
	return Vector2f((float)((double)thePoint.x + aLayer.x), (float)((double)aLayer.y + thePoint.y));
}

void Stage::addPlayerSpawn(PlayerSpawn* theSpawn)
{
	mPlayerSpawns.push_back(theSpawn);
}

void Stage::addPlayer(PlayerFish* thePlayer)
{
	mPlayers.push_back(thePlayer);
}

void Stage::addPlayerScore(int thePoints, PlayerFish* thePlayer)
{
	if (mGameData->addScore(thePoints))
	{
		// The HUD's text extraLife at the player, and the theme's sound bonus1Up: not yet translated
	}
}

void Stage::removePlayer(PlayerFish* thePlayer)
{
	for (size_t i = mPlayers.size(); i > 0; i--)
		if (mPlayers[i - 1].get() == thePlayer)
			mPlayers.erase(mPlayers.begin() + (i - 1));
}

void Stage::connectFirstPlayer()
{
	// port: the players checked too; the original's start has always brought one in by then
	if (!mPlayerSpawns.empty() && !mPlayers.empty())
		mPlayers.front()->connectMouse();
}

void Stage::updateCamera(UpdateContext* theContext)
{
	if (mPlayers.empty())
		return;
	PlayerFish* aPlayer = mPlayers.front().get();
	if (mCameraFollows)
		mLayerSet->followCamera(aPlayer->getCameraTarget(), theContext->mElapsed);
	// The night's mask's light at the visual's place (the visual's slot 0x10, kept at the mask container's +0xbc):
	// not yet translated, as the mask is not
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

void Stage::updateMeetings(UpdateContext* theContext)
{
	for (size_t i = 0; i < mFish.size(); i++)
	{
		Fish* aFish = mFish[i];
		int anEaterRank = aFish->getEaterRank();
		int aRank = aFish->getRank();
		for (size_t j = i + 1; j < mFish.size(); j++)
		{
			Fish* anOther = mFish[j];
			int anOtherEaterRank = anOther->getEaterRank();
			int anOtherRank = anOther->getRank();
			if (aRank < anOtherEaterRank)
				anOther->consider(aFish);
			else if (anOtherRank < anEaterRank)
				aFish->consider(anOther);
		}
	}
}

void Stage::updateBonusMeetings(UpdateContext* theContext)
{
	for (const Ref<PlayerFish>& aPlayer : mPlayers)
	{
		for (size_t i = 0; i < mBonuses.size(); i++)
			aPlayer->consider(mBonuses[i]);
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

void Stage::loadAmbientSounds()
{
	mAmbientSounds = new MusicSystem();
	mAmbientSounds->load(mAmbientSound);
	mAmbientSounds->setVolume(AMBIENT_VOLUME);
}

void Stage::updateAmbientSounds(UpdateContext* theContext)
{
	if (mAmbientSounds)
		mAmbientSounds->update(theContext);
}

void Stage::start()
{
	pause(false);
	// The game data's +0x30, +0x34 and +0x38 set from its +0x10 and +0x14 and to 0, for the HUD: not yet translated
	mGameData->setSpawnPhase(0);
	mGameData->setFood(0);
	mMaskFadeTime = 0.0f;
	mCameraFollows = true;
	mCompleted = false;
	// +0x270 (the boss, null) reset, and the stage's timer at +0x238 cleared (0x406fb0 (feedingfrenzy2)): read by
	// nothing translated yet
	// The stage's time: in a bonus stage kept, else in the timed mode the time carried in added, else none; shown on
	// the HUD, rounded up, with the clock shown when there is a time, and the lives and the stage's name
	if (!mBonus)
	{
		if (mGameData->getMode() == GAME_MODE_TIMED)
			mStageTime = (float)mGameData->getTimeRollover() + mStageTime;
		else
			mStageTime = 0.0f;
	}
	mTimeLeft = mStageTime;
	mHud->showTime((int)ceil((double)mTimeLeft));
	mHud->showTimer(0.0f < mStageTime);
	mHud->showLives(mGameData->getLives());
	mHud->showStageName(stageNameOf(getApp()));
	// The ambient sounds each given their first delay (a draw of the random numbers each), and played at once when
	// it is none
	if (mAmbientSounds)
		mAmbientSounds->start();
	// +0x3b0, 0: read by nothing translated yet
	if (mPlayers.empty())
	{
		startPlayers();
		const RectF& aRange = mLayerSet->getPanRange();
		float aTop = aRange.top;
		// 0x459e00 (feedingfrenzy2): the range's width
		float aWidth = (float)((double)aRange.right - aRange.left);
		float aCentre = (float)(aWidth * CAMERA_CENTRE);
		mLayerSet->setCamera(Vector2f(aCentre, aTop));
		return;
	}
	// A player in already (0x4f37cf (feedingfrenzy2)): the food eaten so far as startPlayers sets it, the first
	// player's flash put on again for 3 seconds (0x52e081 (feedingfrenzy2), the visual's flash not yet translated),
	// and the run
	PlayerSpawn* aSpawn = mPlayerSpawns.front().get();
	setFoodFromPhase(aSpawn);
	mPlayers.front()->getSpawnFlash().start(PLAYER_FLASH_SECONDS);
	startRun();
}

void Stage::setFoodFromPhase(PlayerSpawn* theSpawn)
{
	// The boss's stages start at the food of 0x5ac620 (feedingfrenzy2), set by code not yet translated: 0 until it is
	int aFood = 0;
	if (!mBoss && theSpawn->getPhaseIndex() > 0)
		aFood = theSpawn->getPhase(theSpawn->getPhaseIndex() - 1).mFoodGoal;
	mGameData->setFood(aFood);
}

void Stage::startPlayers()
{
	// 0x491730 (feedingfrenzy2): the list is empty
	if (mPlayerSpawns.empty())
	{
		startRun();
		return;
	}
	if (mGameData->getLives() <= 0)
	{
		// The cold path 0x4f2f7d (feedingfrenzy2): the machine emptied
		mStateMgr.changeState(nullptr);
		return;
	}
	PlayerSpawn* aSpawn = mPlayerSpawns.front().get();
	setFoodFromPhase(aSpawn);
	aSpawn->start();
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
	// With player spawns and a player in, the first one stops hearing the mouse, or hears it again
	if (!mPlayerSpawns.empty() && !mPlayers.empty())
	{
		if (thePaused)
			mPlayers.front()->disconnectMouse();
		else
			mPlayers.front()->connectMouse();
	}
	// The script (+0x31c, 0x502648 (feedingfrenzy2)) paused or resumed: not yet translated
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
	// 0x5c (0x447ef0 (feedingfrenzy2)). Translated so far: the actors' updates, the fish's meetings, the spawns,
	// the bonuses' meetings with the players, the camera, the fish's visuals and the ambient sounds; the hazards'
	// step with the players (0x4c) and the stage's ending (0x5c) are not yet
	mStage->updateActors(theContext);
	mStage->updateMeetings(theContext);
	mStage->updateBonusMeetings(theContext);
	mStage->updateSpawns(theContext);
	mStage->updateCamera(theContext);
	mStage->updateFishVisuals(theContext);
	mStage->updateAmbientSounds(theContext);
	mStage->updatePlay(theContext);
}

CompletionState::CompletionState(Stage* theStage, const std::string& theSound)
    : StageState(theStage), mMarks(COMPLETION_MARK_COUNT), mSound(theSound)
{
}

void CompletionState::enter()
{
	mMarks.mTimes[0] = 0.0f;
	mMarks.mTimes[1] = COMPLETION_BURST_TIME;
	mMarks.mTimes[2] = COMPLETION_END_TIME;
	mMarks.mOn = true;
	GameData* aGameData = getApp()->mGameData.get();
	int aSecondsLeft = (int)ceil((double)mStage->getTimeLeft());
	if (aSecondsLeft < 1)
		aSecondsLeft = 0;
	if (!mStage->isBonus())
		aGameData->setTimeRollover(aSecondsLeft);
	else if (mStage->isCompleted())
		aGameData->setTimeRollover(aGameData->getTimeRollover() + aSecondsLeft);
	// The script (+0x31c), when the stage has one, ended (its slot 0xe8) and let go (0x4a3c50 (feedingfrenzy2)), and
	// the theme's sound of mSound played on the application's sound (0x489670 and 0x48d420 (feedingfrenzy2)): not
	// yet translated, as the scripts and the sounds are not
	getApp()->mUserData->recordGame(*aGameData);
	getApp()->mUserData->save();
}

void CompletionState::exit()
{
	mStage->hideMessage();
	// The application's sound stopped (0x490370 (feedingfrenzy2)): not yet translated, as the sounds are not
}

void CompletionState::update(UpdateContext* theContext)
{
	mStage->updateActors(theContext);
	mStage->updateSpawns(theContext);
	mStage->updateCamera(theContext);
	mStage->updateFishVisuals(theContext);
	mStage->updateHud(theContext);
	mStage->updateAmbientSounds(theContext);
	if (!mMarks.step(theContext->mElapsed))
		return;
	if (mMarks.mNext == 1)
		mStage->showMessage(0.0f, COMPLETION_APPEAR_TIME);
	else if (mMarks.mNext == 2)
	{
		mStage->burstMessage();
		mStage->getAmbientSounds()->stop();
	}
	else if (mMarks.mNext == 3)
	{
		// After a stage whose goal was reached, with the spawn "mystery" in it, the epilog (EpilogState, 0x98 bytes):
		// not yet translated, as the mystery stage's spawn is not
		mStage->getStateMgr().changeState(nullptr);
	}
}

void PlayerDiedState::enter()
{
	// The theme's sound playerDie played on the application's sound (0x489670 and 0x48d420 (feedingfrenzy2)): not
	// yet translated, as the sounds are not
	mStage->setMessage("playerDied");
	mMarks.mTimes[0] = 0.0f;
	mMarks.mTimes[1] = COMPLETION_BURST_TIME;
	mMarks.mTimes[2] = COMPLETION_END_TIME;
	mMarks.mOn = true;
	LayerSet* aLayerSet = mStage->getLayerSet();
	const RectF& aPanRange = aLayerSet->getPanRange();
	// 0x459e00 (feedingfrenzy2): the width, kept at the x87's precision
	Vector2f aTop((float)(((double)aPanRange.right - aPanRange.left) * HALF), aPanRange.top);
	mCamera.clear();
	Vector2Key aKey;
	aKey.mTime = 0.0f;
	aKey.mValue = aLayerSet->getPanPosition();
	mCamera.addKey(aKey);
	aKey.mTime = DIED_CAMERA_HOLD_TIME;
	aKey.mValue = aLayerSet->getPanPosition();
	mCamera.addKey(aKey);
	aKey.mTime = DIED_CAMERA_END_TIME;
	aKey.mValue = aTop;
	mCamera.addKey(aKey);
	mCamera.easeStretch(1, true, true, DIED_CAMERA_STRAIGHT_SHARE);
}

void PlayerDiedState::exit()
{
	mStage->hideMessage();
}

void PlayerDiedState::update(UpdateContext* theContext)
{
	mStage->updateActors(theContext);
	mStage->updateSpawns(theContext);
	mStage->updateHud(theContext);
	mStage->getLayerSet()->setCamera(mCamera.getValue(mMarks.mTime));
	mStage->updateFishVisuals(theContext);
	mStage->updateAmbientSounds(theContext);
	if (!mMarks.step(theContext->mElapsed))
		return;
	if (mMarks.mNext == 1)
		mStage->showMessage(0.0f, COMPLETION_APPEAR_TIME);
	else if (mMarks.mNext == 2)
		mStage->burstMessage();
	else if (mMarks.mNext == 3)
		mStage->startPlayers();
}

void RunState::enter()
{
	// The theme's music of the stage's gameMusic (0x489670 and 0x48d420 (feedingfrenzy2)) on the application's music:
	// not yet translated, as the music is not
	mStage->connectFirstPlayer();
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
		// 0x493fc0 (feedingfrenzy2)
		PlayerSpawn* aSpawn = new PlayerSpawn(mStage);
		SubDocHandler* aHandler = aSpawn->createDocHandler();
		aHandler->attach(mMaster, theName);
		aHandler->begin(theAttributes);
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
