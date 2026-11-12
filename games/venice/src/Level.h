// The level: the scene object that every screen, menu and level of the game is made from. It holds the screen's music, the mouse
// as the screen sees it (only the top scene sees it), the combo timer and the counts the summary screens show.

#ifndef VENICE_LEVEL_H
#define VENICE_LEVEL_H

#include "ImageRecord.h"
#include "SceneObject.h"
#include "VeniceSound.h"

// One of a level's pieces of music (0x48 bytes in the original): the sound, its name in the data file and the channel this level
// started it on (a sound shared with another level is told apart by the channel)
struct LevelMusic
{
	Sound* mSound = nullptr;
	char mName[64] = {};
	int mChannel = 0;
};

// 0x3dc bytes in the original
class Jewel;

class Level : public SceneObject
{
public:
	LevelMusic mIntroMusic; // 0x190
	LevelMusic mMainMusic; // 0x1d8
	LevelMusic mWinMusic; // 0x220
	float mIntroTicksLeft = 0; // 0x268: counts down from the intro's length (in 4 ms ticks); the main music fades in at its end
	float mIntroTime = 0; // 0x26c: the intro's length in milliseconds
	float mWinFadeTicks = 0; // 0x270: counts down while the main music fades into the win music
	int mNumber = 0; // 0x278: the level's number in the scene file list it was pushed from
	// the pointer and the left and right buttons as the screen sees them: only the top scene reads them, the others see zero
	float mMouseX = 0; // 0x27c
	float mMouseY = 0; // 0x280
	float mLeftPressed = 0; // 0x284: 1 on the update the left button went down
	float mRightPressed = 0; // 0x288
	float mLeftClick = 0; // 0x28c: a press that was not already counted
	float mRightClick = 0; // 0x290
	float mPreviousLeftPressed = 0; // 0x294
	float mPreviousRightPressed = 0; // 0x298
	int mPaused = 0; // 0x29c: the scene is not updated (a screen above it asked)
	char mTitle[64] = {}; // 0x2a0
	char mThumbnail[64] = {}; // 0x2e0
	char mWinMessage[64] = {}; // 0x320
	char mLoseMessage[64] = {}; // 0x360
	int mComboCount = 0; // 0x3a0: the plates that tumbled in a row
	int mComboTicksLeft = 0; // 0x3a4: the combo ends when this reaches zero
	int mShotsFired = 0; // 0x3ac
	int mHits = 0; // 0x3b0
	int mCombos = 0; // 0x3b4
	int mScrollTicks = 0; // 0x3c4: the updates the level spent scrolling, which is the time the summary shows
	int mDimmed = 0; // 0x3c8: the scene is drawn over black (the logos at the start)
	int mEndlessSinkTicks = 0; // 0x3cc: the Survival mode's sinking sound plays every 750 updates once the water rises
	ImageRecord* mSparkleImage = nullptr; // 0x3d0
	ImageRecord* mLevelParticleImage = nullptr; // 0x3d4
	ImageRecord* mWhiteParticleImage = nullptr; // 0x3d8

	Level() { mType = ObjectTypeLevel; } // 0x40e820
	~Level() override; // 0x40ea20
	void Load(const char* theText, SceneTag* theTag) override; // 0x40ec10
	bool Update() override; // 0x40f090
	void Draw() override; // 0x40f8e0
	void OnFirstUpdate() override; // 0x40ef50

	void SetMusicVolume(float thePercent);
	void StopMusic();

	// 0x40f5e0: the intro fades into the main music, the main music into the win music
	void FadeMusic();
	// 0x40f690: one update of the crossfade between two pieces of music over a number of ticks
	bool CrossFade(LevelMusic& theFrom, LevelMusic& theTo, float* theTicksLeft, float thePeriod);

	// 0x40f960: a plate sank, or a jewel was lost: the level is lost, or (in the Journey mode) a try is
	void LoseTry();
	// 0x40fae0: the message that goes with a lost try, and what comes after it (the next try, the summary or the click to play again)
	void ShowLossMessage();
	// 0x4100c0: a missed trick shot ends the trick shot level (a level of the Trick Shot mode has one shot)
	void MissTrickShot();
	// 0x410260: a jewel fell in the water without ever filling a slot while the Journey mode's player can still lose
	void CheckLostJewel(Jewel* theJewel);
	// 0x410770: the messages that float on the screen go
	void KillPopupTexts();
};

// 0x410200: whether any plate in the plate layer has a slot to fill
bool AnyPlateIncomplete();

#endif
