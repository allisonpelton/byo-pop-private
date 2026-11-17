#ifndef __FEEDINGFRENZY_STAGEMGR_H__
#define __FEEDINGFRENZY_STAGEMGR_H__

// A stage (StageMgr, 0x364 bytes, constructor 0x43bcc0, vtable 0x4a7060, key handler at +0x11c): the container that
// holds a stage's backgrounds, its fish and the player, and plays it. It loads the stage's file (the spawners, the
// player, the music), keeps the camera on the player, tests every fish against every other each frame, and runs the
// stage from its start screen to its end.

#include "FishSpawner.h"
#include "Geometry.h"
#include "Node.h"
#include "Particles.h"
#include "ScreenLayout.h"
#include "Sound.h"
#include "SplatFactory.h"
#include "StageInfo.h"
#include "TextItem.h"
#include "FadeContainer.h"
#include "Music.h"

#include <string>
#include <vector>

namespace feedingfrenzy
{

class FishBase;
class FishSchool;
class PlayerFish;
class BubbleString;
class StageHud;
class StageInfoScreen;
class StageEndScreen;
class WaveFx;

// What kind of stage it is (+0x1c4; the stage's file says it)
enum StageType
{
	STAGE_NORMAL = 0,       // a stage of the game
	STAGE_TIMED_NORMAL = 1, // a normal stage played in the timed game, which ends when its time is up
	STAGE_TIMED_BONUS = 2,  // a bonus stage: the player eats as much as possible in a time
	STAGE_BONUS = 3,
	STAGE_SURVIVAL = 4,     // the same, in the timed game
	STAGE_MENU = 5          // the stage behind the main menu: its fish swim and no one plays
};

// What the stage is doing (+0x154)
enum StageState
{
	STATE_START_SCREEN = 0,  // the start screen shows (and the stage is waiting to play)
	STATE_LOADING = 1,       // the stage's files are being read, a step each frame
	STATE_PAUSED = 2,
	STATE_PLAYING = 3,
	STATE_MERMAID = 4,       // the stage is cleared and the mermaid swims in
	STATE_ENDING = 5,        // the end of the stage: the banner shows, then the end screen
	STATE_END_SCREEN = 6     // the end screen (or game over) shows
};

// How a stage ends (+0x160)
enum StageEnd
{
	END_CLEARED = 0,  // the player has eaten enough
	END_TIME_UP = 1,  // the time of a bonus stage is up
	END_LOST = 2      // the player has lost the stage
};

// What the fade of the music is doing (+0x170)
enum MusicFade
{
	MUSIC_FADE_NONE = 0,
	MUSIC_FADE_IN = 1,
	MUSIC_FADE_OUT = 2
};

class StageMgr : public engine::Container, public engine::KeyHandler
{
public:
	StageMgr();            // 0x43bcc0
	~StageMgr() override;  // 0x437d80

	// 0x436fa0: the stage's files are to be read, from the manifest's entry for it
	void startLoad(StageInfo* theInfo);
	// 0x4371e0 and 0x4371f0: the load is done, and how far it is (a tenth for each step)
	bool isLoaded() const { return mLoadStep == LOAD_STEP_DONE; }
	float getLoadProgress() const { return (float)mLoadStep * 0.1f; }

	// 0x43bfd0: the groups of the stage, the player, the layouts and the screens
	void init();
	// 0x439170: the splats, the stage-clear bubbles and the banners
	void loadEffects();
	// 0x4396f0: the stage is ready: the player starts, the spawners' first fish come in and the start screen shows
	void beginPlay();
	// 0x4374c0: the player has accepted the start screen: the stage plays
	void startPlaying();
	// 0x437020 and 0x436ff0: the stage is paused, or plays again
	void pause();
	void resume();
	// 0x437110: the stage is not being played: the start screen, the loading, the pause or the end screen shows
	bool isWaiting() const;
	// 0x437200 and 0x437220: the music fades out, and in
	void fadeMusicOut();
	void fadeMusicIn();
	// 0x4375c0 (the escape and P keys): a stage that plays is paused
	void pauseIfPlaying();
	// 0x438000: the stage's objects are let go
	void cleanup();

	// 0x43bb60 (slot 0x94): a frame
	void update(engine::UpdateInfo& theInfo) override;
	// 0x437680 and 0x419740: the escape and P keys pause the stage (the second is never used)
	bool keyDown(int theKey) override;
	bool keyUp(int theKey) override { return false; }

	// The state the stage is in (0x4373f0), and the kind it is (0x4370b0)
	int getState() const { return mState; }
	int getType() const { return mType; }
	// 0x4373e0: a flag the end of a bonus stage clears: the stage has no more time
	bool isTimed() const { return mTimed; }
	StageInfo* getStageInfo() { return mStageInfo; }  // 0x437430
	const std::string& getStageName() const { return mStageName; }  // the stage's friendly name
	const std::string& getNextStageName() const { return mNextStageName; }
	int getNextLevelAt() const { return mNextLevelAt; }

	// The time of the stage: what is left (0x4370a0) and the stage's whole (0x437090)
	float getSecondsLeft() const { return mSecondsLeft; }
	void setSecondsLeft(float theSeconds) { mSecondsLeft = theSeconds; }  // 0x437070
	void setTimeLimit(float theSeconds) { mTimeLimit = theSeconds; }      // 0x437090
	float getTimeLimit() const { return mTimeLimit; }
	void resetSecondsLeft() { mSecondsLeft = mTimeLimit; }                // 0x437080
	float getSecondsPlayed() const { return mSecondsPlayed; }             // +0x2bc

	// How the stage ends
	void endLost();       // 0x437160: the player has lost
	void endTimeUp();     // 0x437130: the time of a bonus stage is up
	void beginMermaidEnd();    // 0x43a640: the stage is cleared; the mermaid swims in (when nothing was lost)
	void clearStage();         // 0x43a590: every fish leaves and the bonus bubbles are no more
	void showEndScreen();      // 0x43b3b0: the end screen, with the stage's results
	void showGameOver();       // 0x4398f0: the game over screen, or the one that offers to continue
	void useContinue();        // 0x4375e0: the player goes on with a continue
	void closeScreen();        // 0x437640: the screen that shows is let go
	void goToNextStage();      // 0x4375b0: the stage after this
	void acceptScreen();       // 0x437400: the start or end screen is told it can be accepted
	void onPlayerEaten() { mDeaths++; }  // 0x438610: the player was eaten (a stage with none of them is perfect)
	void hideEndBanner();      // 0x4377c0
	void showStageClearBanner();  // 0x43a1e0
	void showLostBanner();     // 0x4376e0
	void showPerfectBanner();  // 0x43a360
	void showTimeUpBanner();   // 0x43a470

	// 0x436700 and 0x4367a0: where a place of the stage shows on the screen, and the reverse; 0x436840: where a place
	// of a layer shows (layer 0 the one in front, 1 the middle, 2 the one behind, scrolling slower)
	engine::Point worldToScreen(const engine::Point& thePoint) const;
	engine::Point screenToWorld(const engine::Point& thePoint) const;
	engine::Point layerToScreen(const engine::Point& thePoint, int theLayer) const;
	// 0x436900: the fish's rectangle overlaps the stage; 0x4369c0: it overlaps the view
	bool isFishInStage(FishBase* theFish);
	bool isFishOnScreen(FishBase* theFish);
	// 0x437040: the stage's size; its view stays
	void setWorldSize(int theWidth, int theHeight);
	int getWorldWidth() const { return mWorldWidth; }       // 0x42bc30
	int getWorldHeight() const { return mWorldHeight; }     // 0x437fb0
	int getScreenWidth() const { return mViewWidth; }       // 0x42be10
	int getScreenHeight() const { return mViewHeight; }     // 0x437fc0
	float getVerticalOffset() const { return mVerticalOffset; }  // +0x2c0: the height the HUD takes
	float getCameraX() const { return mCameraX; }
	float getCameraY() const { return mCameraY; }
	// 0x436a90: the camera shakes, for the seconds, up to the amount
	void shakeCamera(float theSeconds, float theAmount);

	// The fish and the schools in the stage
	PlayerFish* getPlayer() { return mPlayer; }                          // 0x436fe0
	const std::vector<FishBase*>& getFishList() const { return mFish; }  // 0x436fd0
	void addFish(FishBase* theFish, int theLayer);   // 0x43ccc0: it joins the list and the group of its layer
	void addSchool(FishSchool* theSchool);           // 0x43ce10
	void removeSchool(FishSchool* theSchool);        // 0x4385b0
	void addSpawner(FishSpawner* theSpawner);        // 0x43cd60: the spawners are kept with the highest level first
	std::vector<FishSpawner*>& getSpawners() { return mSpawners; }  // 0x4371d0
	int countFishToEat() const;                   // 0x437be0: the fish of the spawners that count the fish to eat
	int countEaten() const;                       // 0x437c20: and how many of them were eaten
	int countEatenOfAll() const;                  // 0x437c60: every fish and bonus bubble eaten, the mermaid's too
	int countBonusEaten() const;                  // 0x437cb0
	int countEatenOfLevel(float theLevel) const;  // 0x437cf0
	void recountSpawners();                       // 0x4377f0: every spawner counts its fish again

	// The groups of the stage a fish or an effect is added to
	void addToPlayGroup(engine::Node* theNode) { mPlayGroup->addChild(theNode); }          // 0x4370c0
	engine::Container* getPlayGroup() { return mPlayGroup; }                              // 0x46c490 (+0x124)
	void addToBehind(engine::Node* theNode) { mBehindGroup->addChild(theNode); }           // 0x437310
	void removeFromBehind(engine::Node* theNode) { mBehindGroup->removeChild(theNode); }   // 0x437320
	void addToMidGround(engine::Node* theNode) { mMidGroundGroup->addChild(theNode); }     // 0x437330
	void removeFromMidGround(engine::Node* theNode) { mMidGroundGroup->removeChild(theNode); }  // 0x437340
	void addToInFront(engine::Node* theNode) { mInFrontGroup->addChild(theNode); }         // 0x437350
	void removeFromInFront(engine::Node* theNode) { mInFrontGroup->removeChild(theNode); }  // 0x437360
	void addToOverlay(engine::Node* theNode) { mOverlayGroup->addChild(theNode); }         // 0x4371b0
	void removeFromOverlay(engine::Node* theNode) { mOverlayGroup->removeChild(theNode); }  // 0x4371c0
	void setStartDelay(float theSeconds) { mStartDelay = theSeconds; }                     // 0x437370

	// The splats: the score of a fish eaten (0x439d00), the time a bonus adds (0x439f40), a chomp (0x439c00: the
	// size 0 to 2 picks the mini, small or large ones), a message (0x43a0c0), the frenzy and the double frenzy
	// (0x43a850 and 0x43a900) and the burst when the player grows (0x43a9b0)
	void showEatScore(float thePoints);
	void showTimeBonus(float theSeconds);
	void showChomp(int theSize, float theX, float theY);
	void showMessage(const std::string& theMessage);
	void showFrenzy();
	void showDoubleFrenzy();
	void showLevelUp();

	// The stage's file says them: the name and what follows (0x43b5a0 and 0x43b600), the layouts (0x43b7f0), the
	// sounds and music (0x43b8a0)
	void setStageName(const std::string& theName) { mStageName = theName; }
	void setNext(const std::string& theName, int theLevelAt);
	void setLayoutNames(const std::string& theBackground, const std::string& theForeground, const std::string& theMiddle);
	void setSounds(const std::string& theIntro, const std::string& theOutro, const std::string& theAmbient,
	               const std::string& theMusic);
	void setType(int theType) { mType = theType; }  // 0x437060
	void setPlayer(PlayerFish* thePlayer);          // 0x438dc0

	// 0x436ab0: the camera follows the player; 0x436e90: the player's fish swims to the cursor
	void updateCamera(float theSeconds);
	void followCursor();

	// The mouse catcher's callbacks for a button going down (0x437440) and coming up (0x437480); the argument is the
	// stage's player
	static void onMouseDown(int theX, int theY, int theButton, void* thePlayer);
	static void onMouseUp(int theX, int theY, int theButton, void* thePlayer);

	StageInfoScreen* getStartScreen() { return mStartScreen; }
	// port (test commands): the screen over the stage that shows, as a word ("" when none does)
	const char* getScreenName() const;
	StageHud* getHud() { return mHud; }
	FishSpawner* getBonusSpawner() { return mBonusSpawner; }  // +0x2ac: the spawner of the mermaid

private:
	// The steps of the load (+0x14c)
	enum
	{
		LOAD_STEP_SOUNDS = 0,
		LOAD_STEP_FILE = 1,
		LOAD_STEP_LAYOUTS = 2,
		LOAD_STEP_MUSIC = 3,
		LOAD_STEP_AMBIENT = 4,
		LOAD_STEP_POOLS = 5,
		LOAD_STEP_MERMAID = 6,
		LOAD_STEP_START_SCREEN = 7,
		LOAD_STEP_SCREENS = 8,
		LOAD_STEP_DONE = 9
	};

	void updateLoad();                                   // 0x43ad30
	void updatePlaying(engine::UpdateInfo& theInfo);     // 0x4382c0
	void updateEnding(engine::UpdateInfo& theInfo);      // 0x438660
	void updateMusicFade(engine::UpdateInfo& theInfo);   // 0x437240
	void testCollisions();                               // 0x437830
	void removeFishOutOfPlay();                          // 0x438210
	void loadLayouts();                                  // 0x438e50
	void loadMusic();                                    // 0x43b670
	void loadAmbient();                                  // 0x43aba0
	void setUpScreens();                                 // 0x438870: the objective the start screens show
	void addHudSpawners();                               // 0x439a50
	void showBanner(BubbleString* theBanner, float theX, float theY);
	static void onEatSplatArrived(int thePoints);        // 0x4370d0

	engine::Ref<engine::Container> mPlayGroup;       // +0x124: everything that moves with the camera
	engine::Ref<PlayerFish> mPlayer;                 // +0x128
	engine::Ref<engine::Container> mBehindGroup;     // +0x12c: "behind", under the player
	engine::Ref<engine::Container> mInFrontGroup;    // +0x130: "inFrontOfPlayer"
	engine::Ref<engine::Container> mFishGroupFront;  // +0x134: "fishGroupFg"
	engine::Ref<engine::Container> mFishGroupMiddle; // +0x138: "fishGroupMg", in the middle layout
	engine::Ref<engine::Container> mFishGroupBack;   // +0x13c: "fishGroupBg", in the background layout
	engine::Ref<engine::Container> mMidGroundGroup;  // +0x140: "midGroundGroup", in the middle layout
	engine::Ref<engine::Container> mHudOffsetGroup;  // +0x144: "hudOffsetGroup", below the HUD
	engine::Ref<engine::Container> mOverlayGroup;    // +0x148
	int mLoadStep;                                   // +0x14c
	int mPoolIndex;                                  // +0x150: the spawner whose pool the load makes next
	int mState;                                      // +0x154
	int mStateBeforePause;                           // +0x158
	engine::Ref<engine::ParticleSystem> mClearBubbles;  // +0x15c: the bubbles of a stage cleared
	int mEndKind;                                    // +0x160
	bool mEndHandled;                                // +0x164
	float mEndSeconds;                               // +0x168: how long the end shows before the screen
	int mMusicFade;                                  // +0x170
	float mMusicFadeSeconds;                         // +0x174
	float mMusicFadeLeft;                            // +0x178
	StageHud* mHud;                                  // +0x17c
	engine::Ref<engine::ScreenLayout> mBgLayout;     // +0x180
	engine::Ref<WaveFx> mWaveFx;                     // +0x184: the water's waves on the background
	engine::Ref<engine::ScreenLayout> mMgLayout;     // +0x188
	engine::Ref<engine::ScreenLayout> mFgLayout;     // +0x18c
	std::vector<FishBase*> mFish;                    // +0x190
	std::vector<FishSchool*> mSchools;               // +0x1a0
	engine::Ref<StageInfoScreen> mStartScreen;           // +0x1b0
	engine::Ref<StageEndScreen> mCompleteScreen;         // +0x1b4
	engine::Ref<StageInfoScreen> mContinueScreen;        // +0x1b8
	engine::Ref<StageInfoScreen> mGameOverScreen;        // +0x1bc
	StageInfoScreen* mScreenShown;                   // +0x1c0
	int mType;                                       // +0x1c4
	bool mTimed;                                     // +0x1c8
	int mWorldWidth;                                 // +0x1cc
	int mWorldHeight;                                // +0x1d0
	int mMarginX;                                    // +0x1d4: how near the player gets to the edge before the camera moves
	int mMarginY;                                    // +0x1d8
	float mCameraX;                                  // +0x1dc
	float mCameraY;                                  // +0x1e0
	int mViewWidth;                                  // +0x1e4
	int mViewHeight;                                 // +0x1e8
	std::string mStageName;                          // +0x1ec
	std::string mNextStageName;                      // +0x208
	int mNextLevelAt;                                // +0x224: the score at which the next stage opens
	std::string mIntroSound;                         // +0x228
	std::string mOutroSound;                         // +0x244
	std::string mAmbientFile;                        // +0x260
	std::string mMusicFile;                          // +0x27c
	engine::Sound* mMusic;                           // +0x298
	std::vector<FishSpawner*> mSpawners;             // +0x29c
	FishSpawner* mBonusSpawner;                      // +0x2ac
	float mSecondsLeft;                              // +0x2b0
	float mTimeLimit;                                // +0x2b4
	int mDeaths;                                     // +0x2b8
	float mSecondsPlayed;                            // +0x2bc
	float mVerticalOffset;                           // +0x2c0
	engine::Ref<engine::SplatFactory> mSplats;       // +0x2c4
	engine::Ref<engine::SplatFactory> mEatSplats;    // +0x2c8
	StageInfo* mStageInfo;                           // +0x2d0
	std::string mBackgroundLayoutName;               // +0x2d4
	std::string mMiddleLayoutName;                   // +0x2f0
	std::string mForegroundLayoutName;               // +0x30c
	engine::Ref<engine::MusicSystem> mAmbient;       // +0x328
	BubbleString* mStageBanner;                       // +0x32c ("levelEndStage")
	BubbleString* mClearBanner;                       // +0x330 ("levelEndClear")
	BubbleString* mLostBanner;                        // +0x334 ("levelEndSorry")
	BubbleString* mPerfectBanner;                     // +0x338 ("levelEndPerfect")
	BubbleString* mTimeUpBanner;                      // +0x33c ("levelEndTimeUp")
	float mBannerSeconds;                            // +0x344: how long until the banners come off
	float mShakeSeconds;                             // +0x348
	float mShakeAmount;                              // +0x34c
	float mStartDelay;                               // +0x350
	engine::Ref<engine::FadeContainer> mPauseCover;  // +0x354: the pause message
	float mCoverSeconds;                             // +0x358: how long the pause message stays
};

} // namespace feedingfrenzy

#endif
