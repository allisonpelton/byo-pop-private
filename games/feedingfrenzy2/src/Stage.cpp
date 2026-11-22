#include "Stage.h"

#include "DrawContext.h"
#include "Engine.h"
#include "GameData.h"
#include "Params.h"

using namespace engine;

namespace
{

const float HUD_BAND_HEIGHT = 75.0f;        // 0x42960000 in 0x4146b0 (feedingfrenzy2): the play bounds' top in a game
const float LAYER_MARGIN_X = 200.0f;        // 0x4858d0's arguments in 0x4146b0 (feedingfrenzy2)
const float LAYER_MARGIN_Y = 160.0f;
const float DEFAULT_SIZE_UNIT = 16.0f;      // 0x5667b8 (feedingfrenzy2)
const float DEFAULT_MIN_LIGHT = 0.3f;       // 0x54ddac (feedingfrenzy2)
const float DEFAULT_MAX_LIGHT = 0.9f;       // 0x566934 (feedingfrenzy2)

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
	: mScrollRate(0.0f), mSizeUnit(DEFAULT_SIZE_UNIT), mBonus(false), mWaterColor(0.0f, 0.0f, 0.0f, 0.0f), mNight(false),
	  mLight(0.0f), mLightTarget(0.0f), mMinLight(DEFAULT_MIN_LIGHT), mMaxLight(DEFAULT_MAX_LIGHT), mBoss(false),
	  mFinalBoss(false)
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
	// The spawns' counts (each spawn list's slot 0x14 over +0x280, the players' four more when +0x290 has any) and
	// their setup (slot 0x10 over +0x280): not yet translated, as the spawns are not
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
	// The camera's start from the base layer's (0x49ab50, 0x49ab30 and 0x4882a0 (feedingfrenzy2), with the set's
	// night light), "PlayGroup" found on the first layer's actors, the camera (0x48cdf0, 0x475dd0, 0x49ab10 and
	// 0x49aae0 (feedingfrenzy2)) and the six edges of the play area (+0x7c, 0x48f620 (feedingfrenzy2), from the
	// water level and the sky's ceiling): not yet translated
	getEngine()->log("Stage: the camera and the play area's edges not yet translated\n");
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
	// <spawn> (PlayerSpawn and NonPlayerSpawn, 0x408710 and 0x469960 (feedingfrenzy2)) and its <phase>s: not yet
	// translated
}
