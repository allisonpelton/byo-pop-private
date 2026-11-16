// ThunderballApp.h: Peggle's application class (the original's Sexy::ThunderballApp, a global object whose
// constructor runs before WinMain; WinMain calls Init, Start and Shutdown on it).

#ifndef __THUNDERBALLAPP_H__
#define __THUNDERBALLAPP_H__

#include "CharacterMgr.h"
#include "GameMode.h"
#include "HighScoreMgr.h"
#include "ImageMgr.h"
#include "ProfileMgr.h"
#include "SexyApp.h"
#include "StageMgr.h"
#include "StatsMgr.h"
#include "TrophyMgr.h"
#include "WidgetMover.h"

#include <string>

namespace Sexy
{

class BlendedImage;
class Image;
class LoadingScreen;
class Widget;

// the rows of mFeverStars: the star pictures of the fever effects, each in three sizes
enum FeverStarColor
{
	FEVER_STAR_GLOW = 0,
	FEVER_STAR_RED = 1,
	FEVER_STAR_GREEN = 2,
	FEVER_STAR_BLUE = 3,
	NUM_FEVER_STAR_COLORS = 4
};
static const int NUM_FEVER_STAR_SIZES = 3;
static const int NUM_BRICK_IMAGES = 8;
static const int NUM_PEG_IMAGES = 8;

class ThunderballApp : public SexyApp
{
public:
	ThunderballApp(); // 0x426f10
	~ThunderballApp() override;

	void Init() override; // 0x42ca30
	void LoadingThreadProc() override; // 0x421a70: loads the rest of the resources while the loading screen is up

	// The pictures the game makes from the ones it loads: the bricks and pegs (a second set for the colorblind), and the
	// stars of the fever.
	void CopyBrickTextures(); // 0x4081d0
	void CopyPegTextures();	  // 0x41c610
	void MakeFeverStars();	  // 0x408280
	bool SetColorblind(bool theColorblind); // 0x41c810: true when it changed

	// Music. The level music is every piece of music other than the two of the menus (the original lists the music folder; the
	// game ships one), loaded when the loading screen is up.
	void LoadLevelMusic(); // 0x4a6f90 (the original's thread procedure), 0x405740

	// The screens: the main menu and the others are made by these (each takes the previous screen off the way it was
	// told to scroll). One of each kind is up at most.
	void ShowLoadingScreen();					  // 0x4296c0
	void ShowMainMenu();						  // 0x42d480
	void ShowHelpScreen(bool theFromMenu, bool theScrollOn); // 0x423540
	void ShowBoard(bool theNewGame, bool theScrollOn);		 // 0x42f860
	void ShowReplay(bool theKeepBoard);			  // 0x42fb00
	void DoCreateUserDialog();					  // 0x40c3b0
	void CleanupScreen(Widget* theScreen);		  // 0x4234f0: takes the screen off the manager and deletes it
	void CleanupScreens(bool theIncludeBoard);	  // 0x429390: all of them (the board stays when told)
	void CheckScrollOff(Widget* theScreen, WidgetScroll theTo, bool theRemoveWhenDone); // 0x423480
	void ScrollOn(Widget* theScreen);			  // 0x4234c0: brings it in from the edge the last screen left to

	// the managers; the original makes them with new in its constructor and keeps them at these offsets of the app
	CharacterMgr mCharacterMgr;		 // +0x844
	HighScoreMgr mHighScoreMgr;		 // +0x848
	ImageMgr mImageMgr;				 // +0x84c (the level pictures)
	ProfileMgr mProfileMgr;			 // +0x850
	StageMgr mStageMgr;				 // +0x858
	StatsMgr mStatsMgr;				 // +0x85c
	StatsMgr mCheatStatsMgr;		 // +0x860 (what the level editor's test games record)
	TrophyMgr mTrophyMgr;			 // +0x864

	Image* mBrickImages[NUM_BRICK_IMAGES] = {}; // +0x7dc: one for each of the brick picture's cels (a copy each; owned)
	Image* mBrickGlowImage = nullptr;	   // +0x7fc (the loaded picture; not owned)
	Image* mFeverStars[NUM_FEVER_STAR_COLORS][NUM_FEVER_STAR_SIZES] = {}; // +0x800: by FeverStarColor, then the size (20, 30, 40)

	WidgetMover mWidgetMover;		 // +0x738
	WidgetScroll mScrollOnFrom = SCROLL_NONE; // +0x750: the edge the next screen comes in from (the one the last left to)

	int mGameMode = GAME_MODE_NONE;	 // +0x760
	int mStartStage = 0;			 // +0x764: the stage a game starts at
	int mStartLevel = 0;			 // +0x768
	bool mLevelMusicPending = false; // +0x779: the level music is still to be loaded
	int mNextLevelMusicId = 1;		 // +0x780: the music slot the next level music gets
	int mLevelMusicParam = 9;		 // +0x784 (set with the two above; read by the music code)
	std::string mReplayFileName;	 // +0x79c: the replay to show (empty: none)

	// the screens (null when one is not up). The board's, the help screen's and the others' types come with their classes.
	Widget* mBoard = nullptr;		 // +0x7b8
	Widget* mHelpScreen = nullptr;	 // +0x7c0
	Widget* mLevelScreen = nullptr;	 // +0x7c4
	LoadingScreen* mLoadingScreen = nullptr; // +0x7c8
	Widget* mMainMenu = nullptr;	 // +0x7cc
	Widget* mTrophyScreen = nullptr; // +0x7d0
	Widget* mStoryScreen = nullptr;	 // +0x7d4
	Widget* mAdventureScreen = nullptr; // +0x7d8

	PlayerInfo* mCurrentPlayer = nullptr; // +0x878 (the profile in use; owned by mProfileMgr)
};

extern ThunderballApp* gThunderballApp;

// Globals of the original (0x650a55, 0x65057c, 0x65059c): the setting of the colorblind pictures, and the peg pictures that
// can be drawn between pixels (one for each of the eight pegs of the picture, and the shadow's).
extern bool gColorblind;
extern BlendedImage* gPegImages[NUM_PEG_IMAGES];
extern BlendedImage* gPegShadowImage;

} // namespace Sexy

#endif
