#include "StageMgr.h"
#include "App.h"
#include "FishBase.h"
#include "FishFactory.h"
#include "Mine.h"
#include "FishFood.h"
#include "FishSchool.h"
#include "FishSpawner.h"
#include "Background.h"
#include "PlayerFish.h"
#include "Random.h"
#include "BubbleString.h"
#include "StageDocHandler.h"
#include "StageHud.h"
#include "GameSession.h"
#include "StageEndScreen.h"
#include "StageInfoScreen.h"
#include "WaveFx.h"

#include <algorithm>
#include <cstdio>

using namespace engine;

namespace feedingfrenzy
{

namespace
{
// The stage's size and what it starts with (the constructor 0x43bcc0)
const int START_WORLD_WIDTH = 640;
const int START_WORLD_HEIGHT = 480;
const int START_MARGIN_X = 200;
const int START_MARGIN_Y = 150;
const int START_VIEW_WIDTH = 640;
const int START_VIEW_HEIGHT = 480;
const float START_FADE_SECONDS = 0.2f;

// How far the layers of the stage move with the camera: the foreground and the middle layout (0x436ab0), and the
// fish layers (0x436840: the middle and the back)
const float FOREGROUND_PARALLAX = 0.7f;
const float MIDDLE_PARALLAX = 0.4f;

// The camera of a stage with the player in state 4 (the stage is cleared) goes to the stage's middle at this speed
// and stops when it is this near
const float CENTRE_SPEED = 10.0f;
const float CENTRE_STOP_DISTANCE = 20.0f;

// How far the cursor's target stays from the view's edge (0x436e90)
const int CURSOR_EDGE_MARGIN = 10;

// The groups and layouts the stage makes (the names are what the files and the game's tools see)
const char* const HUD_FILE = "config\\gameHud.xml";
const char* const SPLATS_FILE = "config\\splats.xml";
const char* const EAT_SPLATS_FILE = "config\\eatSplats.xml";
const char* const CLEAR_BUBBLES_FILE = "config\\fx\\stageClearBubbles.xml";
const char* const GAMEPLAY_SOUNDS_FILE = "config\\gameplaySounds.xml";
const char* const STAGE_FILE_NAME = "stage.xml";
const char* const BONUS_FISH_FILE = "config\\fish\\Mermaid.xml";
const char* const CONTINUE_SCREEN_FILE = "config\\gameOverContinue.xml";
const char* const GAME_OVER_SCREEN_FILE = "config\\gameOver.xml";
const char* const PAUSE_FONT_FILE = "resources\\hud\\textFont.xml";
const float CLEAR_BUBBLES_X = 320.0f;
const float CLEAR_BUBBLES_Y = 240.0f;
// The banners that end a stage: the seconds each letter waits after the one before, how far apart the letters are (the
// stage, clear and sorry banners, and the perfect and time up banners), and how long the clear banner waits to begin
const float BANNER_LETTER_DELAY = 0.15f;
const float BANNER_WIDE_SPACING = 80.0f;
const float BANNER_NARROW_SPACING = 71.0f;
const float BANNER_CLEAR_START_DELAY = 0.8f;
const float PAUSE_MESSAGE_X = 10.0f;
const float PAUSE_MESSAGE_Y = 450.0f;

// How long the pause message shows when a stage starts (0x4396f0), and how long the banners of the end of a stage
// stay (0x43a1e0 and the others)
const float PAUSE_MESSAGE_SECONDS = 5.0f;
const float BANNER_SECONDS = 2.5f;
const float END_SECONDS = 4.0f;

// The banners are placed with their middle at the stage's middle, at these heights
const float BANNER_X = 320.0f;
const float BANNER_Y = 200.0f;
const float CLEAR_BANNER_Y = 310.0f;

// The fade of the screens
const float START_SCREEN_FADE_SECONDS = 0.5f;
const float END_SCREEN_FADE_SECONDS = 0.2f;
const float SCREEN_HIDE_SECONDS = 0.1f;

// The time at which the sound of the music fades (the music's own setting, 0x43b670)
const float MUSIC_FADE_IN_SECONDS = 1.0f;
const float MUSIC_FADE_OUT_SECONDS = 2.0f;
const float AMBIENT_VOLUME = 0.7f;
// The levels addHudSpawners starts from: below every kind, and above every kind
const float NO_LEVEL_YET = -9999.0f;
const float NO_NEXT_LEVEL = 9999.0f;
// What the stage sets the background's waves to each frame
const float WAVE_SIZE = 1.25f;
const float WAVE_AMOUNT = 0.03f;

// How a collision pass of two fish ends: what the rectangle tests find out
enum
{
	BONUS_LEVEL_NONE = 0
};

// The state the player is in that the stage reads (PlayerFish.h gives them all)
}

// 0x43bcc0
StageMgr::StageMgr() :
	mLoadStep(LOAD_STEP_SOUNDS), mPoolIndex(0), mState(STATE_START_SCREEN), mStateBeforePause(STATE_PLAYING),
	mEndKind(END_CLEARED), mEndHandled(false), mEndSeconds(0.0f), mMusicFade(MUSIC_FADE_NONE),
	mMusicFadeSeconds(START_FADE_SECONDS), mMusicFadeLeft(0.0f), mHud(nullptr), mScreenShown(nullptr),
	mType(STAGE_NORMAL), mTimed(false), mWorldWidth(START_WORLD_WIDTH), mWorldHeight(START_WORLD_HEIGHT),
	mMarginX(START_MARGIN_X), mMarginY(START_MARGIN_Y), mCameraX(0.0f), mCameraY(0.0f), mViewWidth(START_VIEW_WIDTH),
	mViewHeight(START_VIEW_HEIGHT), mNextLevelAt(0), mMusic(nullptr), mBonusSpawner(nullptr), mSecondsLeft(0.0f),
	mTimeLimit(0.0f), mDeaths(0), mSecondsPlayed(0.0f), mVerticalOffset(0.0f), mStageInfo(nullptr),
	mStageBanner(nullptr), mClearBanner(nullptr), mLostBanner(nullptr), mPerfectBanner(nullptr),
	mTimeUpBanner(nullptr), mBannerSeconds(0.0f), mShakeSeconds(0.0f), mShakeAmount(0.0f), mStartDelay(0.0f),
	mCoverSeconds(0.0f)
{
	setFlags(NODE_LISTENERS);
}

// 0x437d80
StageMgr::~StageMgr()
{
}

// 0x436fa0
void StageMgr::startLoad(StageInfo* theInfo)
{
	mState = STATE_LOADING;
	mLoadStep = LOAD_STEP_SOUNDS;
	mStageInfo = theInfo;
	mPoolIndex = 0;
}

// 0x43bfd0: the groups of the stage are made and put together: the layouts hold the fish groups of their layers, the
// player stands between the middle and the front ones, with the splats of what it eats, then the HUD over them all
void StageMgr::init()
{
	setName("StageMgr");
	FishFood* aFood = FishFood::instance();
	mHudOffsetGroup = new Container();
	mPlayGroup = new Container();
	mPlayGroup->setName("PlayGroup");
	mFishGroupFront = new Container();
	mFishGroupFront->setName("fishGroupFg");
	mFishGroupMiddle = new Container();
	mFishGroupMiddle->setName("fishGroupMg");
	mFishGroupBack = new Container();
	mFishGroupBack->setName("fishGroupBg");

	mPlayer->setPosition((float)(mWorldWidth / 2), (float)(mWorldHeight / 2));
	mPlayer->putInPlay();
	mFish.push_back(mPlayer);

	// The original also gives the catcher an empty callback for the mouse moving with a button down (0x40dcf0)
	Background* aCatcher = new Background(mWorldWidth, mWorldHeight);
	if (mType != STAGE_MENU)
	{
		aCatcher->setDownCallback(onMouseDown, mPlayer);
		aCatcher->setUpCallback(onMouseUp, mPlayer);
	}

	mHudOffsetGroup->setPosition(10.0f, 10.0f);
	mHudOffsetGroup->setName("hudOffsetGroup");
	addChild(mHudOffsetGroup);
	mHudOffsetGroup->addChild(mPlayGroup);

	if (mBgLayout == nullptr)
	{
		mBgLayout = new ScreenLayout();
		mBgLayout->setName("BgLayout(empty)");
	}
	mPlayGroup->addChild(mBgLayout);
	mBgLayout->addChild(mFishGroupBack);

	if (mMgLayout == nullptr)
	{
		mMgLayout = new ScreenLayout();
		mMgLayout->setName("MgLayout(empty)");
	}
	mPlayGroup->addChild(mMgLayout);
	mMgLayout->addChild(mFishGroupMiddle);

	mMidGroundGroup = new Container();
	mMidGroundGroup->setName("midGroundGroup");
	mMgLayout->addChild(mMidGroundGroup);

	mBehindGroup = new Container();
	mBehindGroup->setName("behind");
	mPlayGroup->addChild(mBehindGroup);
	mPlayGroup->addChild(mPlayer);

	mEatSplats = new SplatFactory();
	mPlayGroup->addChild(mEatSplats);
	mEatSplats->load(EAT_SPLATS_FILE);
	mPlayGroup->addChild(mFishGroupFront);

	mInFrontGroup = new Container();
	mInFrontGroup->setName("inFrontOfPlayer");
	mPlayGroup->addChild(mInFrontGroup);
	if (mFgLayout != nullptr)
		mPlayGroup->addChild(mFgLayout);

	mOverlayGroup = new Container();
	mPlayGroup->addChild(mOverlayGroup);

	mHud = new StageHud();
	mHud->load(HUD_FILE, this);
	mHud->getRoot()->layout();
	mHud->getRoot()->setName("HudLayout");
	Node* aBackground = mHud->getRoot()->getNode("bg");
	mVerticalOffset = aBackground->getBounds()->height();
	if (mType == STAGE_MENU)
	{
		mHud->getRoot()->setVisible(false);
		mVerticalOffset = 0.0f;
		mPlayer->setVisible(false);
		mPlayer->setCursorLocked(false);
	}
	mHudOffsetGroup->setPosition(0.0f, mVerticalOffset);
	mViewHeight -= (int)mVerticalOffset;

	aCatcher->setName("BackgroundBackground");
	addChild(aCatcher);
	addChild(mHud->getRoot());

	if (mType != STAGE_MENU)
		aFood->addKeyHandler(static_cast<KeyHandler*>(mPlayer.get()));
	if (mType != STAGE_MENU)
		aFood->addKeyHandler(this);
	addHudSpawners();
	if (mType != STAGE_MENU)
		loadEffects();
	Mine::resetLeaveCount();

	mPauseCover = new FadeContainer();
	addChild(mPauseCover);
	TextItem* aPauseMessage = new TextItem();
	mPauseCover->addChild(aPauseMessage);
	aPauseMessage->setFont(PAUSE_FONT_FILE);
	aPauseMessage->setText(FishFood::instance()->getStrings().get("pauseMsg"));
	aPauseMessage->setPosition(PAUSE_MESSAGE_X, PAUSE_MESSAGE_Y);
	aPauseMessage->setHAlign(0);
	mPauseCover->setOpacity(0.0f);
	mPauseCover->setVisible(true);
}

// 0x439170: the splats and bubbles that show what the player does, and the banners that end a stage
void StageMgr::loadEffects()
{
	mSplats = new SplatFactory();
	addChild(mSplats);
	mSplats->load(SPLATS_FILE);

	mClearBubbles = new ParticleSystem();
	mClearBubbles->setPosition(CLEAR_BUBBLES_X, CLEAR_BUBBLES_Y);
	addChild(mClearBubbles);
	mClearBubbles->load(CLEAR_BUBBLES_FILE);

	const GameStrings& aStrings = FishFood::instance()->getStrings();
	mStageBanner = new BubbleString();
	addChild(mStageBanner);
	mStageBanner->setText(aStrings.get("levelEndStage"), BANNER_LETTER_DELAY, BANNER_WIDE_SPACING, 0.0f);
	mClearBanner = new BubbleString();
	addChild(mClearBanner);
	mClearBanner->setText(aStrings.get("levelEndClear"), BANNER_LETTER_DELAY, BANNER_WIDE_SPACING,
	                      BANNER_CLEAR_START_DELAY);
	mLostBanner = new BubbleString();
	addChild(mLostBanner);
	mLostBanner->setText(aStrings.get("levelEndSorry"), BANNER_LETTER_DELAY, BANNER_WIDE_SPACING, 0.0f);
	mPerfectBanner = new BubbleString();
	addChild(mPerfectBanner);
	mPerfectBanner->setText(aStrings.get("levelEndPerfect"), BANNER_LETTER_DELAY, BANNER_NARROW_SPACING, 0.0f);
	mTimeUpBanner = new BubbleString();
	addChild(mTimeUpBanner);
	mTimeUpBanner->setText(aStrings.get("levelEndTimeUp"), BANNER_LETTER_DELAY, BANNER_NARROW_SPACING, 0.0f);
}

// 0x4396f0: the stage is built: the player begins its life, the camera starts in the middle of the stage, and the
// spawners' first fish come in; the menu's stage plays at once, a game's shows its start screen
void StageMgr::beginPlay()
{
	mPlayer->startLife();
	mCameraX = (float)(mWorldWidth - mViewWidth) * 0.5f;
	mCameraY = (float)(mWorldHeight - mViewHeight) * 0.5f;
	if (mCameraY < 1.0f)
		mCameraY = 0.0f;
	mPlayGroup->setPosition(-mCameraX, -mCameraY);

	for (size_t i = 0; i < mSpawners.size(); i++)
		mSpawners[i]->spawnInitial();

	if (mType == STAGE_MENU)
	{
		mState = STATE_PLAYING;
		startPlaying();
		return;
	}
	if (FishFood::instance()->getSettings().areTipsEnabled())
	{
		if (mPauseCover != nullptr && mType != STAGE_MENU)
			mPauseCover->fade(true, 1.0f, 1.0f, false);
		mCoverSeconds = PAUSE_MESSAGE_SECONDS;
	}
	FishFood::instance()->playSound(mIntroSound, 1.0f, 1.0f);
	mStartScreen->connect();
	FishFood::instance()->showScreen(mStartScreen, START_SCREEN_FADE_SECONDS);
	mState = STATE_START_SCREEN;
	mScreenShown = mStartScreen;
}

// 0x4374c0: the stage plays: it joins the controller's screens, the music and the ambient sounds start, the start
// screen goes, and the window's mouse is put where the player's fish is
void StageMgr::startPlaying()
{
	mState = STATE_PLAYING;
	FishFood* aFood = FishFood::instance();
	aFood->addStage(this);
	mSecondsPlayed = 0.0f;
	// port: the original puts the window's cursor at (325, 265) here (SetCursorPos); the port's cursor is the
	// player's own, and stays where it is
	if (mType != STAGE_MENU)
	{
		if (mMusic != nullptr)
			mMusic->play();
		if (mAmbient != nullptr)
			mAmbient->start();
		if (mScreenShown != nullptr)
		{
			aFood->hideScreen(mScreenShown, SCREEN_HIDE_SECONDS);
			mScreenShown = nullptr;
		}
		aFood->hideScreen(mStartScreen, SCREEN_HIDE_SECONDS);
	}
	resume();
}

// 0x437020
void StageMgr::pause()
{
	int aState = mState;
	mState = STATE_PAUSED;
	if (aState != STATE_PAUSED)
		mStateBeforePause = aState;
}

// 0x436ff0: the stage goes on, and the sounds with it
void StageMgr::resume()
{
	mState = mStateBeforePause;
	SoundSystem* aSounds = App::getSoundSystem();
	if (aSounds != nullptr)
		aSounds->setVolume(1.0f);
}

// 0x437110
bool StageMgr::isWaiting() const
{
	return mState >= 0 && (mState < STATE_PLAYING || mState == STATE_END_SCREEN);
}

// 0x4375c0 (slot 0x130): the pause key
void StageMgr::pauseIfPlaying()
{
	if (mState == STATE_PLAYING)
		FishFood::instance()->pauseGame();
}

// 0x437680: only the escape and P keys are taken
bool StageMgr::keyDown(int theKey)
{
	const int KEY_ESCAPE = 0x1b;
	const int KEY_P = 0x50;
	if (theKey != KEY_ESCAPE && theKey != KEY_P)
		return false;
	pauseIfPlaying();
	return true;
}

// 0x438000: the stage lets go of what it holds, for the controller to delete it
void StageMgr::cleanup()
{
	FishFood* aFood = FishFood::instance();
	aFood->removeKeyHandler(this);
	if (mPlayer != nullptr)
		aFood->removeKeyHandler(static_cast<KeyHandler*>(mPlayer.get()));
	if (mScreenShown != nullptr)
	{
		FishFood::instance()->hideScreen(mScreenShown, 0.0f);
		mScreenShown->setAcceptable(false);
	}
	if (mType != STAGE_MENU)
	{
		mStartScreen = nullptr;
		mCompleteScreen = nullptr;
		mContinueScreen = nullptr;
		mGameOverScreen = nullptr;
	}
	mFishGroupFront->removeAllChildren();
	mFishGroupMiddle->removeAllChildren();
	mFishGroupBack->removeAllChildren();
	while (!mSpawners.empty())
	{
		delete mSpawners.front();
		mSpawners.erase(mSpawners.begin());
	}
	delete mBonusSpawner;
	mBonusSpawner = nullptr;
	if (mMusic != nullptr)
		mMusic->close();
	mAmbient = nullptr;
	mSplats = nullptr;
	mEatSplats = nullptr;
	delete mHud;
	mHud = nullptr;
	mPlayGroup->removeAllChildren();
}

// 0x43bb60 (slot 0x94): the ambient sound, the music's fade, the player's fish to the cursor, and the state's frame
void StageMgr::update(UpdateInfo& theInfo)
{
	if (mAmbient != nullptr)
		mAmbient->update(theInfo);
	updateMusicFade(theInfo);
	if (mState != STATE_LOADING)
		followCursor();
	switch (mState)
	{
	case STATE_START_SCREEN:
	case STATE_END_SCREEN:
		Container::update(theInfo);
		break;
	case STATE_LOADING:
		updateLoad();
		break;
	case STATE_PLAYING:
		updatePlaying(theInfo);
		break;
	case STATE_MERMAID:
	case STATE_ENDING:
		updateEnding(theInfo);
		break;
	}
}

// 0x437240: the music's volume follows the fade: a fade in lifts it and a fade out lowers it, a step at a time
void StageMgr::updateMusicFade(UpdateInfo& theInfo)
{
	if (mMusic == nullptr)
		return;
	if (0.0f <= mMusicFadeLeft)
	{
		mMusicFadeLeft -= theInfo.mSeconds;
		if (mMusicFadeLeft < 0.0f)
			mMusicFadeLeft = 0.0f;
	}
	float aShare = mMusicFadeLeft / mMusicFadeSeconds;
	if (mMusicFade == MUSIC_FADE_IN)
	{
		if (mMusic->getVolume() < aShare)
			mMusic->setVolume(aShare);
	}
	else if (mMusicFade == MUSIC_FADE_OUT)
	{
		float aTarget = 1.0f - aShare;
		if (aTarget < mMusic->getVolume())
			mMusic->setVolume(aTarget);
	}
}

// 0x437200 and 0x437220
void StageMgr::fadeMusicOut()
{
	mMusicFade = MUSIC_FADE_OUT;
	mMusicFadeSeconds = START_FADE_SECONDS;
	mMusicFadeLeft = START_FADE_SECONDS;
}

void StageMgr::fadeMusicIn()
{
	mMusicFade = MUSIC_FADE_IN;
	mMusicFadeSeconds = START_FADE_SECONDS;
	mMusicFadeLeft = START_FADE_SECONDS;
}

// 0x4382c0: a frame of a stage that plays: the waves, the time, every pair of fish tested, the schools, the camera,
// and the spawners' frame (a stage whose spawners are all done ends)
void StageMgr::updatePlaying(UpdateInfo& theInfo)
{
	if (mWaveFx != nullptr)
	{
		mWaveFx->setSize(WAVE_SIZE);
		mWaveFx->setTime(mWaveFx->getTime() + theInfo.mSeconds);
		mWaveFx->setAmount(WAVE_AMOUNT);
	}
	mSecondsLeft -= theInfo.mSeconds;
	mSecondsPlayed += theInfo.mSeconds;
	if (mSecondsLeft <= 0.0f)
	{
		mSecondsLeft = 0.0f;
		if (mType == STAGE_TIMED_NORMAL && !mPlayer->isDying())
			endLost();
		else if (mType == STAGE_TIMED_BONUS || mType == STAGE_SURVIVAL)
		{
			endTimeUp();
			mTimed = false;
		}
	}
	if (0.0f < mCoverSeconds)
	{
		mCoverSeconds -= theInfo.mSeconds;
		if (mCoverSeconds < 0.0f && mPauseCover != nullptr && mType != STAGE_MENU)
			mPauseCover->fade(false, 1.0f, 1.0f, false);
	}
	testCollisions();
	mHud->update(theInfo);
	for (size_t i = 0; i < mSchools.size(); i++)
		mSchools[i]->update(theInfo);
	Container::update(theInfo);
	if (mType != STAGE_MENU)
		updateCamera(theInfo.mSeconds);
	removeFishOutOfPlay();

	bool anAllDone = true;
	if (0.0f < mStartDelay)
	{
		mStartDelay -= theInfo.mSeconds;
		return;
	}
	if (mPlayer != nullptr && mPlayer->getState() != PLAYER_DYING)
	{
		for (size_t i = 0; i < mSpawners.size(); i++)
		{
			mSpawners[i]->update(theInfo, this);
			if (anAllDone)
				anAllDone = mSpawners[i]->isDone();
		}
		if (anAllDone)
			beginMermaidEnd();
	}
}

// 0x438660: a frame of the end: the banner of how it ended shows, the time runs out and the screen of the results
// comes (the game over screen of a timed game that has lost)
void StageMgr::updateEnding(UpdateInfo& theInfo)
{
	if (mState == STATE_ENDING || (mState == STATE_MERMAID && mBonusSpawner->isDone()))
	{
		if (!mEndHandled)
		{
			if (mEndKind == END_CLEARED)
			{
				if (mState == STATE_MERMAID && mBonusSpawner->isDone())
					showPerfectBanner();
				else
					showStageClearBanner();
			}
			else if (mEndKind == END_TIME_UP)
				showTimeUpBanner();
			else if (mEndKind == END_LOST)
				showLostBanner();
			mEndHandled = true;
		}
		mState = STATE_ENDING;
		mEndSeconds -= theInfo.mSeconds;
		if (0.0f < mBannerSeconds)
		{
			mBannerSeconds -= theInfo.mSeconds;
			if (mStageBanner != nullptr && mBannerSeconds < 0.0f)
			{
				mStageBanner->hide();
				mClearBanner->hide();
				mPerfectBanner->hide();
				mTimeUpBanner->hide();
				mLostBanner->hide();
				FishFood::instance()->playSound("playerSpawn", 1.0f, 1.0f);
			}
		}
		if (mEndSeconds < 0.0f)
		{
			if (FishFood::instance()->getSession()->getMode() == GAME_MODE_TIMED && mType != STAGE_TIMED_BONUS &&
			    mSecondsLeft <= 0.0f)
				showGameOver();
			else
				showEndScreen();
		}
	}
	else
	{
		testCollisions();
		mHud->update(theInfo);
	}
	updateCamera(theInfo.mSeconds);
	mHud->update(theInfo);
	Container::update(theInfo);
	removeFishOutOfPlay();
	if (mPauseCover != nullptr && mType != STAGE_MENU)
		mPauseCover->fade(false, 1.0f, 1.0f, false);
}

// 0x437160 and 0x437130
void StageMgr::endLost()
{
	Sound* aMusic = mMusic;
	mState = STATE_ENDING;
	mEndKind = END_LOST;
	mEndHandled = false;
	mEndSeconds = END_SECONDS;
	if (aMusic != nullptr)
		aMusic->stop();
	if (mAmbient != nullptr)
		mAmbient->stop();
}

void StageMgr::endTimeUp()
{
	mState = STATE_ENDING;
	mEndKind = END_TIME_UP;
	mEndHandled = false;
	mEndSeconds = END_SECONDS;
}

// 0x43a640: every spawner is done: a stage the player has not lost a fish in lets the mermaid swim in, another ends
void StageMgr::beginMermaidEnd()
{
	if (mState == STATE_ENDING)
		return;
	if (mType == STAGE_NORMAL && mDeaths == 0)
	{
		Point aPlace;
		Point aVelocity;
		Point aNose = mPlayer->getNosePosition();
		if (aNose.x >= (float)(mWorldWidth / 2))
		{
			aPlace.x = (float)mWorldWidth;
			aVelocity.x = -100.0f;
		}
		else
		{
			aPlace.x = -300.0f;
			aVelocity.x = 100.0f;
		}
		aPlace.y = mPlayer->getNosePosition().y - 100.0f;
		if (aPlace.y <= 0.0f)
			aPlace.y = 0.0f;
		float aLowest = (float)mWorldHeight - 300.0f;
		if (aLowest < aPlace.y)
			aPlace.y = aLowest;
		mBonusSpawner->spawnOffscreen(this, false, aPlace.x, aPlace.y, false);
		fadeMusicOut();
		FishFood::instance()->playSound("mermaid", 1.0f, 1.0f);
		mState = STATE_MERMAID;
		mEndSeconds = END_SECONDS;
	}
	else
		mState = STATE_ENDING;
	mEndKind = END_CLEARED;
	mEndHandled = false;
	mEndSeconds = END_SECONDS;
	if (mType == STAGE_TIMED_BONUS)
		mTimed = true;
}

// 0x43a590: the bonus bubbles go and every fish leaves
void StageMgr::clearStage()
{
	for (size_t i = 0; i < mSpawners.size(); i++)
		mSpawners[i]->setMode(SPAWN_PAUSED);
	for (size_t i = 0; i < mFish.size(); i++)
		mFish[i]->leave();
	FishFood::instance()->playSound("starPickup", 1.0f, 1.0f);
}

// 0x43b3b0: the stage's results show, with the end's sound
void StageMgr::showEndScreen()
{
	FishFood* aFood = FishFood::instance();
	if (mScreenShown != nullptr)
	{
		aFood->hideScreen(mScreenShown, SCREEN_HIDE_SECONDS);
		mScreenShown = nullptr;
	}
	if (mMusic != nullptr)
		mMusic->stop();
	if (mAmbient != nullptr)
		mAmbient->stop();
	mState = STATE_END_SCREEN;
	aFood->playSound(mOutroSound, 1.0f, 1.0f);
	mCompleteScreen->setUpResults(mType, aFood->getSession()->getMode(), mEatSplats);
	if (mEatSplats != nullptr)
		mPlayGroup->removeChild(mEatSplats);
	mScreenShown = mCompleteScreen;
	aFood->slideLogoIn();
	aFood->showScreen(mScreenShown, END_SCREEN_FADE_SECONDS);
}

// 0x4398f0: the game is over: the screen that offers a continue when there is one, else the last
void StageMgr::showGameOver()
{
	FishFood* aFood = FishFood::instance();
	pause();
	aFood->slideLogoIn();
	aFood->playSound("gameOver", 1.0f, 1.0f);
	StageInfoScreen* aScreen;
	if (0 < aFood->getSession()->getContinues())
	{
		mContinueScreen->setText("continueCountText",
		                         aFood->getStrings().format("contLeftFormat", {aFood->getSession()->getContinues()}));
		aScreen = mContinueScreen;
	}
	else
		aScreen = mGameOverScreen;
	mScreenShown = aScreen;
	aFood->showScreen(aScreen, END_SCREEN_FADE_SECONDS);
}

// 0x4375e0: the game goes on with a continue
void StageMgr::useContinue()
{
	if (mScreenShown == nullptr)
		return;
	FishFood* aFood = FishFood::instance();
	aFood->hideScreen(mScreenShown, SCREEN_HIDE_SECONDS);
	aFood->slideLogoOut();
	mScreenShown = nullptr;
	aFood->getSession()->useContinue();
	mPlayer->startLife();
	resume();
}

// 0x437640: the screen that shows goes
void StageMgr::closeScreen()
{
	if (mScreenShown != nullptr)
	{
		FishFood::instance()->hideScreen(mScreenShown, SCREEN_HIDE_SECONDS);
		mScreenShown->setAcceptable(false);
		mScreenShown = nullptr;
	}
	FishFood::instance()->finishStage();
}

// 0x4375b0: the stage after this
void StageMgr::goToNextStage()
{
	FishFood::instance()->goToStage(mStageInfo->getIndex() + 1);
}

// 0x437400
void StageMgr::acceptScreen()
{
	StageInfoScreen* aScreen;
	if (mState == STATE_START_SCREEN)
		aScreen = mStartScreen;
	else
	{
		if (mState != STATE_END_SCREEN)
			return;
		aScreen = mCompleteScreen;
	}
	aScreen->setAcceptable(true);
}

// 0x4377c0
void StageMgr::hideEndBanner()
{
	if (mPerfectBanner != nullptr)
		mPerfectBanner->hide();
	fadeMusicIn();
}

// 0x43a1e0: the stage is cleared: the two lines of the banner
void StageMgr::showStageClearBanner()
{
	FishFood::instance()->playSound("stageClear", 1.0f, 1.0f);
	fadeMusicOut();
	mClearBubbles->startEmitting();
	showBanner(mStageBanner, BANNER_X, BANNER_Y);
	showBanner(mClearBanner, BANNER_X, CLEAR_BANNER_Y);
	mBannerSeconds = BANNER_SECONDS;
}

// 0x4376e0: the stage is lost
void StageMgr::showLostBanner()
{
	mClearBubbles->startEmitting();
	fadeMusicOut();
	showBanner(mLostBanner, BANNER_X, BANNER_Y);
	mBannerSeconds = BANNER_SECONDS;
}

// 0x43a360
void StageMgr::showPerfectBanner()
{
	FishFood::instance()->playSound("stageClear", 1.0f, 1.0f);
	fadeMusicOut();
	mClearBubbles->startEmitting();
	showBanner(mPerfectBanner, BANNER_X, BANNER_Y);
	mBannerSeconds = BANNER_SECONDS;
}

// 0x43a470
void StageMgr::showTimeUpBanner()
{
	FishFood::instance()->playSound("stageClear", 1.0f, 1.0f);
	fadeMusicOut();
	mClearBubbles->startEmitting();
	mBannerSeconds = BANNER_SECONDS;
	showBanner(mTimeUpBanner, BANNER_X, BANNER_Y);
}

void StageMgr::showBanner(BubbleString* theBanner, float theX, float theY)
{
	theBanner->show(Point(theX - theBanner->getWidth() * 0.5f, theY));
}

// 0x436700
Point StageMgr::worldToScreen(const Point& thePoint) const
{
	return Point(thePoint.x - mCameraX, (thePoint.y - mCameraY) + mVerticalOffset);
}

// 0x4367a0
Point StageMgr::screenToWorld(const Point& thePoint) const
{
	return Point(thePoint.x + mCameraX, thePoint.y + mCameraY);
}

// 0x436840: the first layer scrolls with the camera, the second slower and the third a little faster
Point StageMgr::layerToScreen(const Point& thePoint, int theLayer) const
{
	float aShare = 0.0f;
	if (theLayer == 1)
		aShare = MIDDLE_PARALLAX;
	else if (theLayer == 2)
		aShare = FOREGROUND_PARALLAX;
	return Point(thePoint.x - aShare * mCameraX, thePoint.y);
}

// 0x436900
bool StageMgr::isFishInStage(FishBase* theFish)
{
	Rect aBounds = *theFish->getBounds();
	return aBounds.intersects(Rect(0.0f, 0.0f, (float)mWorldWidth, (float)mWorldHeight));
}

// 0x4369c0
bool StageMgr::isFishOnScreen(FishBase* theFish)
{
	Rect aBounds = *theFish->getBounds();
	return aBounds.intersects(
		Rect(mCameraX, mCameraY, (float)mViewWidth + mCameraX, (float)mViewHeight + mCameraY));
}

// 0x437040
void StageMgr::setWorldSize(int theWidth, int theHeight)
{
	mWorldWidth = theWidth;
	mWorldHeight = theHeight;
}

// 0x436a90
void StageMgr::shakeCamera(float theSeconds, float theAmount)
{
	mShakeSeconds = theSeconds;
	mShakeAmount = theAmount;
}

// 0x436ab0: the camera follows the player: the stage's two layouts scroll slower than the play group, and the camera
// moves when the player's fish nears the view's edge (while the fish comes into the stage it settles to the stage's
// middle, and in the feeding fury it stays)
void StageMgr::updateCamera(float theSeconds)
{
	Point aNose = mPlayer->getNosePosition();
	if (mPlayer->getState() == PLAYER_FURY)
		return;
	Point aScreen = worldToScreen(aNose);
	aScreen.y -= mVerticalOffset;
	if (mFgLayout != nullptr)
		mFgLayout->setPosition(mCameraX * -FOREGROUND_PARALLAX, 0.0f);
	if (mMgLayout != nullptr)
		mMgLayout->setPosition(mCameraX * -MIDDLE_PARALLAX, 0.0f);
	if (mPlayer->getState() == PLAYER_SPAWN && mType != STAGE_MENU)
	{
		Point aCentre((float)(mWorldWidth - mViewWidth) * 0.5f, (float)(mWorldHeight - mViewHeight) * 0.5f);
		aCentre.subtract(Point(mCameraX, mCameraY));
		if (CENTRE_STOP_DISTANCE < aCentre.length())
		{
			aCentre.normalize();
			aCentre.scale(CENTRE_SPEED);
			mCameraX = aCentre.x + mCameraX;
			mCameraY = aCentre.y + mCameraY;
		}
	}
	else
	{
		float aMarginX = (float)mMarginX;
		if (aScreen.x < aMarginX)
		{
			float aCamera = aNose.x - aMarginX;
			if (aCamera < 0.0f)
				aCamera = 0.0f;
			mCameraX = aCamera;
		}
		if ((float)(mViewWidth - mMarginX) < aScreen.x)
		{
			float aWanted = (aNose.x - (float)mViewWidth) + aMarginX;
			float aCamera = (float)(mWorldWidth - mViewWidth);
			if (aWanted <= aCamera)
				aCamera = aWanted;
			mCameraX = aCamera;
		}
		float aMarginY = (float)mMarginY;
		if (aScreen.y < aMarginY)
		{
			float aCamera = aNose.y - aMarginY;
			if (aCamera < 0.0f)
				aCamera = 0.0f;
			mCameraY = aCamera;
		}
		if ((float)(int)(float)(mViewHeight - mMarginY) < aScreen.y)
		{
			float aWanted = (aNose.y - (float)mViewHeight) + aMarginY;
			float aCamera = (float)(int)(float)(mWorldHeight - mViewHeight);
			if (aWanted <= aCamera)
				aCamera = aWanted;
			mCameraY = aCamera;
		}
	}
	if (0.0f < mShakeSeconds)
	{
		mShakeSeconds -= theSeconds;
		mCameraX = randomFloat(Point(-mShakeAmount, mShakeAmount)) + mCameraX;
		mCameraY = randomFloat(Point(-mShakeAmount, mShakeAmount)) + mCameraY;
	}
	mPlayGroup->setPosition(-mCameraX, -mCameraY);
}

// 0x436e90: the player's fish swims to where the cursor is, kept from the view's lower and right edges
void StageMgr::followCursor()
{
	Point aMouse = App::instance()->getMousePosition();
	int aX = (int)aMouse.x;
	int aY = (int)aMouse.y;
	if ((float)aY < mVerticalOffset)
		aY = (int)mVerticalOffset;
	int aRight = mViewWidth - CURSOR_EDGE_MARGIN;
	if (aRight < aX)
		aX = aRight;
	float aBottom = ((float)mViewHeight + mVerticalOffset) - (float)CURSOR_EDGE_MARGIN;
	if (aBottom < (float)aY)
		aY = (int)aBottom;
	mPlayer->setTarget((float)aX, (float)aY - mVerticalOffset, true);
}

// 0x437440 and 0x437480: a button goes to the player's fish, after the fish has looked where the cursor is
void StageMgr::onMouseDown(int theX, int theY, int theButton, void* thePlayer)
{
	FishFood::instance()->getStageMgr()->followCursor();
	PlayerFish* aPlayer = static_cast<PlayerFish*>(thePlayer);
	if (aPlayer == nullptr)
		return;
	if (theButton != Background::BUTTON_LEFT)
		aPlayer->rightButtonDown();
	else
		aPlayer->leftButtonDown();
}

void StageMgr::onMouseUp(int theX, int theY, int theButton, void* thePlayer)
{
	FishFood::instance()->getStageMgr()->followCursor();
	PlayerFish* aPlayer = static_cast<PlayerFish*>(thePlayer);
	if (aPlayer == nullptr)
		return;
	if (theButton != Background::BUTTON_LEFT)
		aPlayer->rightButtonUp();
	else
		aPlayer->leftButtonUp();
}

// 0x437830: every pair of fish is tested, the earlier one against each later one, until one eats another; a fish
// that stings touches what it overlaps
void StageMgr::testCollisions()
{
	for (size_t i = 0; i < mFish.size(); i++)
	{
		FishBase* aFirst = mFish[i];
		CollisionRect aFirstHit = *aFirst->getPlacedHitRect();
		for (size_t j = i + 1; j < mFish.size(); j++)
		{
			FishBase* aSecond = mFish[j];
			CollisionRect aSecondHit = *aSecond->getPlacedHitRect();
			if (aFirst->canEat(aSecond) && aSecondHit.intersects(*aFirst->getBodyRect()))
			{
				if (aSecond->canBeEatenBy(aFirst))
				{
					aFirst->eat(aSecond);
					break;
				}
			}
			else if (mPlayer != nullptr && mType != STAGE_MENU && aFirst == mPlayer && aSecond->isPickup() &&
			         aFirst->getBodyRect()->intersects(aSecond->getMouthRect()->getRect()) &&
			         aFirst->canEat(aSecond) && !mPlayer->isInvincible())
			{
				aFirst->eat(aSecond);
				break;
			}

			if (aSecond->canEat(aFirst) && aFirstHit.intersects(*aSecond->getBodyRect()))
			{
				if (aFirst->canBeEatenBy(aSecond))
				{
					aSecond->eat(aFirst);
					break;
				}
			}
			else if (mPlayer != nullptr && mType != STAGE_MENU && aSecond == mPlayer && aFirst->isPickup() &&
			         aSecond->getBodyRect()->intersects(aFirst->getMouthRect()->getRect()) &&
			         aSecond->canEat(aFirst))
			{
				aSecond->eat(aFirst);
				break;
			}

			if (aSecond->stingsOnContact())
			{
				CollisionRect aSting = *aSecond->getPlacedHitRect();
				if (aSting.intersects(aFirstHit))
				{
					aSecond->onContact(aFirst);
					if (mPlayer->getState() != PLAYER_DYING)
						break;
				}
			}
			if (aFirst->stingsOnContact())
			{
				CollisionRect aSting = *aFirst->getPlacedHitRect();
				if (aSting.intersects(aSecondHit))
				{
					aFirst->onContact(aSecond);
					if (mPlayer->getState() != PLAYER_DYING)
						break;
				}
			}
		}
	}
}

// 0x438210: a fish that is out of play leaves the list and the groups; the spawners then pick their rules again
void StageMgr::removeFishOutOfPlay()
{
	bool aRemoved = false;
	for (size_t i = 0; i < mFish.size();)
	{
		FishBase* aFish = mFish[i];
		if (!aFish->isInPlay())
		{
			mFish.erase(mFish.begin() + i);
			mFishGroupFront->removeChild(aFish);
			mFishGroupMiddle->removeChild(aFish);
			mFishGroupBack->removeChild(aFish);
			aRemoved = true;
		}
		else
			i++;
	}
	if (aRemoved)
		recountSpawners();
}

// 0x4377f0
void StageMgr::recountSpawners()
{
	for (size_t i = 0; i < mSpawners.size(); i++)
		mSpawners[i]->recount();
}

// 0x43ccc0
void StageMgr::addFish(FishBase* theFish, int theLayer)
{
	mFish.push_back(theFish);
	if (theLayer == 0)
		mFishGroupFront->addChild(theFish);
	else if (theLayer == 1)
		mFishGroupMiddle->addChild(theFish);
	else if (theLayer == 2)
		mFishGroupBack->addChild(theFish);
}

// 0x43cd60: a spawner goes before the first with a lower level
void StageMgr::addSpawner(FishSpawner* theSpawner)
{
	for (std::vector<FishSpawner*>::iterator anIt = mSpawners.begin(); anIt != mSpawners.end(); ++anIt)
	{
		if ((*anIt)->getLevel() < theSpawner->getLevel())
		{
			mSpawners.insert(anIt, theSpawner);
			return;
		}
	}
	mSpawners.push_back(theSpawner);
}

// 0x43ce10
void StageMgr::addSchool(FishSchool* theSchool)
{
	mSchools.push_back(theSchool);
}

// 0x4385b0
void StageMgr::removeSchool(FishSchool* theSchool)
{
	for (std::vector<FishSchool*>::iterator anIt = mSchools.begin(); anIt != mSchools.end();)
	{
		if (*anIt == theSchool)
			anIt = mSchools.erase(anIt);
		else
			++anIt;
	}
}

// 0x437be0: how many fish the spawners that are counted have in all
int StageMgr::countFishToEat() const
{
	int aCount = 0;
	for (size_t i = 0; i < mSpawners.size(); i++)
		if (mSpawners[i]->getMode() == SPAWN_ONLY_UNEATEN)
			aCount += mSpawners[i]->getPoolSize();
	return aCount;
}

// 0x437c20
int StageMgr::countEaten() const
{
	int aCount = 0;
	for (size_t i = 0; i < mSpawners.size(); i++)
		if (mSpawners[i]->getMode() == SPAWN_ONLY_UNEATEN)
			aCount += mSpawners[i]->getEatenCount();
	return aCount;
}

// 0x437c60
int StageMgr::countEatenOfAll() const
{
	int aCount = 0;
	for (size_t i = 0; i < mSpawners.size(); i++)
		aCount += mSpawners[i]->getBonusEatenCount() + mSpawners[i]->getFishEatenCount();
	return aCount + mBonusSpawner->getBonusEatenCount();
}

// 0x437cb0
int StageMgr::countBonusEaten() const
{
	int aCount = 0;
	for (size_t i = 0; i < mSpawners.size(); i++)
		aCount += mSpawners[i]->getBonusEatenCount();
	return aCount + mBonusSpawner->getBonusEatenCount();
}

// 0x437cf0
int StageMgr::countEatenOfLevel(float theLevel) const
{
	int aCount = 0;
	for (size_t i = 0; i < mSpawners.size(); i++)
		if (mSpawners[i]->getLevel() == theLevel)
			aCount += mSpawners[i]->getFishEatenCount();
	return aCount;
}

// 0x4370d0: a score that was shown reaches the player's fish and grows it
void StageMgr::onEatSplatArrived(int thePoints)
{
	FishFood::instance()->getStageMgr()->getPlayer()->addPoints((float)(short)thePoints);
}

// 0x439d00: what the player has eaten shows where its fish is, and grows the fish when it arrives
void StageMgr::showEatScore(float thePoints)
{
	if (mType == STAGE_MENU)
		return;
	Point aPlace = worldToScreen(mPlayer->getNosePosition());
	char aText[16];
	Splat* aSplat;
	if (0.0f <= thePoints)
	{
		snprintf(aText, sizeof(aText), "+%d", (int)thePoints);
		aSplat = mSplats->create("playerEat", aPlace.x, aPlace.y, onEatSplatArrived, (int)thePoints);
	}
	else
	{
		snprintf(aText, sizeof(aText), "%d", (int)thePoints);
		aSplat = mSplats->create("playerEatNegative", aPlace.x, aPlace.y, onEatSplatArrived, (int)thePoints);
	}
	aSplat->setText(aText);
}

// 0x439f40: seconds of time a fish gives
void StageMgr::showTimeBonus(float theSeconds)
{
	mSecondsLeft += theSeconds;
	Point aPlace = worldToScreen(mPlayer->getNosePosition());
	Splat* aSplat = mSplats->create("bonusSplat", aPlace.x, aPlace.y);
	aSplat->setText(FishFood::instance()->getStrings().format("timeSplat", {(int)theSeconds}));
}

// 0x439c00: a chomp at a place of the stage, of the size of the fish that was eaten (a picture of four)
void StageMgr::showChomp(int theSize, float theX, float theY)
{
	int aVariation = randomInt(0, 4);
	if (3 < aVariation)
		aVariation = 3;
	if (theSize < 0)
		theSize = 0;
	else if (2 < theSize)
		theSize = 3;
	char aName[32];
	if (theSize == 0)
		snprintf(aName, sizeof(aName), "chompSplatMini%d", aVariation);
	else if (theSize == 1)
		snprintf(aName, sizeof(aName), "chompSplatSmall%d", aVariation);
	else if (theSize == 2)
		snprintf(aName, sizeof(aName), "chompSplatLarge%d", aVariation);
	mEatSplats->create(aName, theX, theY);
}

// 0x43a0c0
void StageMgr::showMessage(const std::string& theMessage)
{
	Point aPlace = worldToScreen(mPlayer->getNosePosition());
	Splat* aSplat = mSplats->create("bonusSplat", aPlace.x, aPlace.y);
	aSplat->setText(theMessage);
}

// 0x43a850 and 0x43a900: the banner of a frenzy
void StageMgr::showFrenzy()
{
	mSplats->create("frenzySplat", 320.0f, 240.0f);
}

void StageMgr::showDoubleFrenzy()
{
	mSplats->create("doubleFrenzySplat", 320.0f, 240.0f);
}

// 0x43a9b0: the burst at the player's fish when it has grown a size
void StageMgr::showLevelUp()
{
	Point aNose = mPlayer->getNosePosition();
	float aX;
	if (mPlayer->getNosePosition().x <= 0.0f)
		aX = mPlayer->getMouthRect()->getRect().width() * 0.5f + aNose.x;
	else
		aX = aNose.x - mPlayer->getMouthRect()->getRect().width() * 0.5f;
	Point aPlace = worldToScreen(Point(aX, aNose.y));
	mSplats->create("levelUpBurst", aPlace.x, aPlace.y);
	mSplats->create("levelUp", aPlace.x, aPlace.y);
}

// 0x43b5a0 and 0x43b600
void StageMgr::setNext(const std::string& theName, int theLevelAt)
{
	mNextStageName = theName;
	mNextLevelAt = theLevelAt;
}

// 0x43b7f0
void StageMgr::setLayoutNames(const std::string& theBackground, const std::string& theForeground,
                              const std::string& theMiddle)
{
	mBackgroundLayoutName = theBackground;
	mForegroundLayoutName = theForeground;
	mMiddleLayoutName = theMiddle;
}

// 0x43b8a0
void StageMgr::setSounds(const std::string& theIntro, const std::string& theOutro, const std::string& theAmbient,
                         const std::string& theMusic)
{
	mIntroSound = theIntro;
	mOutroSound = theOutro;
	mAmbientFile = theAmbient;
	mMusicFile = theMusic;
}

// 0x438dc0: the player the stage's file names
void StageMgr::setPlayer(PlayerFish* thePlayer)
{
	mPlayer = thePlayer;
	mPlayer->setName("PlayerFish");
}

// 0x438e50: the three layouts of the stage; the background always, the others when the stage's file names them
void StageMgr::loadLayouts()
{
	mBgLayout = new ScreenLayout();
	mBgLayout->loadLayout(mBackgroundLayoutName);
	mBgLayout->setName("BgLayout");
	mWaveFx = new WaveFx();
	mBgLayout->setBltFx(mWaveFx);
	if (!mForegroundLayoutName.empty())
	{
		mFgLayout = new ScreenLayout();
		mFgLayout->loadLayout(mForegroundLayoutName);
		mFgLayout->setName("FgLayout");
	}
	if (!mMiddleLayoutName.empty())
	{
		mMgLayout = new ScreenLayout();
		mMgLayout->loadLayout(mMiddleLayoutName);
		mMgLayout->setName("MgLayout");
	}
}

// 0x43b670: the music of the stage is loaded, to play at its start
void StageMgr::loadMusic()
{
	SoundSystem* aSounds = App::getSoundSystem();
	if (aSounds != nullptr && !mMusicFile.empty())
	{
		aSounds->load(mMusicFile.c_str(), mMusicFile.c_str());
		mMusic = aSounds->createSound(mMusicFile.c_str(), false, 1);
		if (mMusic != nullptr)
		{
			mMusic->setLoop(true);
			mMusic->setFadeIn(MUSIC_FADE_IN_SECONDS);
			mMusic->setFadeOut(MUSIC_FADE_OUT_SECONDS);
		}
	}
}

// 0x43aba0: the ambient sounds the stage's file names
void StageMgr::loadAmbient()
{
	if (!mAmbientFile.empty())
	{
		mAmbient = new MusicSystem();
		mAmbient->load(mAmbientFile);
		mAmbient->setVolume(AMBIENT_VOLUME);
	}
}

// 0x43ad30: a step of the load a frame: the sounds, the stage's file, the layouts, the music, the ambient sounds, a
// pool of fish for each spawner (a spawner a frame), the bonus fish, and the screens
void StageMgr::updateLoad()
{
	std::string aFolder = mStageInfo->getPath();
	aFolder += "\\";
	switch (mLoadStep)
	{
	case LOAD_STEP_SOUNDS:
		FishFood::instance()->getSoundBank().load(GAMEPLAY_SOUNDS_FILE);
		mLoadStep = LOAD_STEP_FILE;
		break;
	case LOAD_STEP_FILE:
		loadStageFile(aFolder + STAGE_FILE_NAME, this);
		mLoadStep = LOAD_STEP_LAYOUTS;
		break;
	case LOAD_STEP_LAYOUTS:
		loadLayouts();
		mLoadStep = LOAD_STEP_MUSIC;
		break;
	case LOAD_STEP_MUSIC:
		loadMusic();
		mLoadStep = LOAD_STEP_AMBIENT;
		break;
	case LOAD_STEP_AMBIENT:
		loadAmbient();
		mLoadStep = LOAD_STEP_POOLS;
		break;
	case LOAD_STEP_POOLS:
		if (mPoolIndex < (int)mSpawners.size())
		{
			mSpawners[mPoolIndex]->createFish();
			mPoolIndex++;
		}
		else
			mLoadStep = (mType != STAGE_MENU) ? LOAD_STEP_MERMAID : LOAD_STEP_DONE;
		break;
	case LOAD_STEP_MERMAID:
		if (mType != STAGE_MENU)
		{
			mBonusSpawner = new FishSpawner();
			mBonusSpawner->load(BONUS_FISH_FILE, 1);
			mBonusSpawner->createFish();
			mBonusSpawner->setDisplayMenuImage(false);
			mBonusSpawner->setMode(SPAWN_STOPPED);
		}
		mLoadStep = LOAD_STEP_START_SCREEN;
		break;
	case LOAD_STEP_START_SCREEN:
		mStartScreen = new StageInfoScreen(this);
		mStartScreen->loadLayout(aFolder + "startScreen.xml");
		mLoadStep = LOAD_STEP_SCREENS;
		break;
	case LOAD_STEP_SCREENS:
		mCompleteScreen = new StageEndScreen(this);
		mCompleteScreen->loadLayout(aFolder + "completeScreen.xml");
		mContinueScreen = new StageInfoScreen(this);
		mContinueScreen->loadLayout(CONTINUE_SCREEN_FILE);
		mGameOverScreen = new StageInfoScreen(this);
		mGameOverScreen->loadLayout(GAME_OVER_SCREEN_FILE);
		setUpScreens();
		mLoadStep = LOAD_STEP_DONE;
		break;
	}
}

// A node of a screen is shown or hidden
static void setNodeVisible(ScreenLayout* theScreen, const char* theName, bool theVisible)
{
	theScreen->getNode(theName)->setVisible(theVisible);
}

// A time (a number of seconds) is put in a text node of a screen
static void setNodeTime(ScreenLayout* theScreen, const char* theName, int theSeconds, bool theWithHours)
{
	static_cast<TextItem*>(theScreen->getNode(theName))->setTime(theSeconds, theWithHours);
}

// 0x438870: the objective the start screen shows and the groups of the end screen that go with the game's mode; in the
// timed game a normal stage is timed, and the time left from the stage before is added to this stage's
void StageMgr::setUpScreens()
{
	GameSession* aSession = FishFood::instance()->getSession();
	bool aTimedGame = aSession->getMode() == GAME_MODE_TIMED;
	if (aTimedGame && mType == STAGE_NORMAL)
		mType = STAGE_TIMED_NORMAL;
	if (aTimedGame)
	{
		setNodeVisible(mStartScreen, "NoTimeObjective", false);
		setNodeVisible(mStartScreen, "TimeObjective", true);
		setNodeVisible(mCompleteScreen, "FunFactGroup", true);
		setNodeVisible(mCompleteScreen, "TimeModeGroup", true);
		setNodeVisible(mCompleteScreen, "TimeBonusGroup", false);
		if (mType != STAGE_TIMED_BONUS)
		{
			setNodeVisible(mStartScreen, "timeObjectiveLine1Time", true);
			setNodeTime(mStartScreen, "timeObjectiveLine1Time", (int)mTimeLimit, true);
			setNodeVisible(mStartScreen, "timeObjectiveLine2Header", true);
			mTimeLimit = aSession->getSecondsPlayed() + mTimeLimit;
			aSession->resetSecondsPlayed();
			setNodeVisible(mStartScreen, "timeObjectiveLine2Time", true);
			setNodeTime(mStartScreen, "timeObjectiveLine2Time", (int)mTimeLimit, true);
		}
		else
		{
			setNodeTime(mStartScreen, "timeObjectiveLine1Time", (int)mTimeLimit, false);
			setNodeVisible(mStartScreen, "timeObjectiveLine2Header", false);
			setNodeVisible(mStartScreen, "timeObjectiveLine2Time", false);
		}
	}
	else
	{
		setNodeVisible(mStartScreen, "TimeObjective", false);
		setNodeVisible(mStartScreen, "NoTimeObjective", true);
		bool aBonus = mType == STAGE_TIMED_BONUS;
		setNodeVisible(mCompleteScreen, "FunFactGroup", !aBonus);
		setNodeVisible(mCompleteScreen, "TimeModeGroup", false);
		setNodeVisible(mCompleteScreen, "TimeBonusGroup", aBonus);
	}
	TextItem* aTitle = static_cast<TextItem*>(mStartScreen->getNode("title"));
	if (aTitle != nullptr)
	{
		aTitle->setText(FishFood::instance()->getStrings().format(
			"levelStartCapFormat", {mStageInfo->getIndex(), mStageInfo->getName()}));
	}
	resetSecondsLeft();
}

// 0x439a50: the HUD and the end screen get the pictures of the (up to four) kinds of fish that show a menu picture,
// the weakest first
void StageMgr::addHudSpawners()
{
	float aLastLevel = NO_LEVEL_YET;
	for (int i = 0; i < HUD_MENU_BUBBLES; i++)
	{
		FishSpawner* aNext = nullptr;
		float aNextLevel = NO_NEXT_LEVEL;
		for (size_t j = 0; j < mSpawners.size(); j++)
		{
			FishSpawner* aSpawner = mSpawners[j];
			if (aSpawner->getDisplayMenuImage() && aLastLevel < aSpawner->getLevel() && !aSpawner->isShownInHud() &&
			    aSpawner->getLevel() < aNextLevel)
			{
				aNext = aSpawner;
				aNextLevel = aSpawner->getLevel();
			}
		}
		if (aNext == nullptr)
			return;
		mHud->setMenuBubble(i, aNext->getMenuImage(), aNext->getLevel(), aNext->getColorShift());
		if (mCompleteScreen != nullptr)
			mCompleteScreen->setMenuBubble(i, aNext->getMenuImage(), aNext->getLevel(), aNext->getColorShift());
		aNext->setShownInHud(true);
		aLastLevel = aNextLevel;
	}
}

// port (test commands)
const char* StageMgr::getScreenName() const
{
	if (mScreenShown == nullptr)
		return "";
	if (mScreenShown == mStartScreen)
		return "stagestart";
	if (mScreenShown == mCompleteScreen)
		return "stageend";
	if (mScreenShown == mContinueScreen)
		return "continue";
	if (mScreenShown == mGameOverScreen)
		return "gameover";
	return "";
}

} // namespace feedingfrenzy
