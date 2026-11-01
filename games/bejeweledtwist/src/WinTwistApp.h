// Sexy::WinTwistApp (vtable 0x7f433c, 111 slots; 0x75e900-0x768d00): Bejeweled Twist's app. Its fields are named
// here by their offsets in the original's object; the framework's own fields are SexyAppBase's.
#pragma once

#include "Rect.h"
#include "SexyApp.h"

#include <vector>

namespace Sexy {

class ButtonWidget;
class CustomResourceManager;
class EffectsManager;
class Gates;
class Hyperspace;
class Image;
class LoaderScreen;
class Menu;
class MenuButtonWidget;
class PlayButton;
class Profile;
class SoundInstance;
class Widget;

class WinTwistApp : public SexyApp {
public:
	// The three sizes of the game's art (images\1200\, images\768\, images\600\): the height of the screen it was
	// drawn for. 2D mode always uses 600 at 800x600.
	enum { ART_RES_1200 = 1200, ART_RES_768 = 768, ART_RES_600 = 600 };

	int mArtRes = 0;                    // +0xf2c
	float mScale = 0.0f;                // +0xf30: mArtRes / 1200, from the art's 1600x1200 to the screen
	bool mAllowArtRes1200 = true;       // +0xf28: the largest art only when the machine can take it
	bool mUseSexyCache = true;          // +0xf14: PopCap's image cache (SexyCache) had the pictures: no loader screen
	bool mInitialLoadDone = false;      // +0xf8d: Init got past loading the loader screen's resources
	bool mTubeLoaded = false;           // +0xf8c: the Tube group (0x763092)
	bool mLoadingThreadDone = false;    // +0x7f1: LoadingThreadProc loaded everything
	int mLoadMode = 0;                  // +0xf18: 1 skips the Sounds group, 2 loads only it (never set: always 0)
	bool mFlag764 = false;              // +0x764: set by the loader while mFlag784 is (meaning not yet known)
	bool mFlag784 = false;              // +0x784: (meaning not yet known; the loader's other buttons depend on it)
	Widget* mBoard = nullptr;           // +0x7e4: the board while a game is on (comes with the board)
	LoaderScreen* mLoaderScreen = nullptr;  // +0x860
	Menu* mMainMenu = nullptr;          // +0x864
	Gates* mGates = nullptr;            // +0xc70: the main menu's gates
	Profile* mProfile = nullptr;        // +0x7ec: the players
	ButtonWidget* mWindowedButton = nullptr;  // +0x91c: top right, switches to a window (hidden in one)
	Image* mFruitImage = nullptr;       // +0x904: the fruit pictures the menu chose (comes with the board)
	Rect mFruitRect;                    // +0x908: its first cel
	bool mResumeGame = false;           // +0x930: a saved game is resumed at once (-resume; comes with the board)
	bool mPlayPressed = false;          // +0xf60: the menu's play button was pressed: resume the last game
	int mLastGameMode = 0;              // +0xf64: the high scores to show after a game
	bool mUnknownF85 = false;           // +0xf85: StartMainMenu starts a classic game at once
	int mAutoPlayMode = -1;             // +0x1044: -autoplay's game (1 classic, 3 blitz, 4 the other), -1 none
	float mAutoPlayDelay = 0;           // +0x1048: -autoplay starts its game when this falls to 0
	bool mPlaySamples = true;           // +0xf74: PlaySample plays (always on so far)
	Hyperspace* mHyperspace = nullptr;  // +0xf20: the space behind every screen
	EffectsManager* mFXManager = nullptr;  // +0xf78: the effects over the whole screen
	bool mSoundsOff = false;            // +0x928: no sound effects (set elsewhere, not yet read)
	SoundInstance* mUnmanagedSound = nullptr;  // +0x938: a tracked sound whose volume UpdateFrames leaves alone
	double mSfxVolumeScale = 1.0;       // +0x948: applied to the tracked sounds
	struct TrackedSound {
		SoundInstance* mInstance;
		double mVolume;
	};
	std::vector<TrackedSound> mTrackedSounds;  // +0x960: sounds UpdateFrames keeps at mSfxVolumeScale until they end

	WinTwistApp();                      // 0x766ce7
	~WinTwistApp() override;            // 0x768c34

	void Init() override;               // 0x7652cc
	void LoadingThreadProc() override;  // 0x76497d
	void UpdateFrames() override;       // 0x765f8d
	void PlaySample(int theSoundNum) override;  // 0x7675d8
	using SexyApp::PlaySample;

	// 0x76657b (slot 110): the loading is done, the main menu comes up
	virtual void StartMainMenu();
	// 0x765e04: back to the main menu, with the stats of the game just played, or with the challenges
	void ShowMainMenu(bool theEndStats, bool theChallenges, float theDelay);
	void ShowHyperspace();      // 0x75f588: the space behind every screen, made if need be
	void CreateGates();         // 0x75ec15
	void CreateMainMenu();      // 0x75ecb8
	void PlaceWindowedButton(); // 0x75e966
	// 0x763b19: a main menu button of theImage's cels, theCover over it while it is shut, listening to the app
	MenuButtonWidget* MakeMenuButton(Image* theImage, Image* theCover, int theId, int theX, int theY,
		const SexyString& theLabel, const SexyString& theToolTip);
	// The main menu's spiral button (main menu +0x2e4)
	PlayButton* GetMainMenuPlayButton();

	// 0x760659: 2D mode, the art at 600 and the screen at 800x600 in 16 bits
	void Use2DMode();
	// port: the game's own question "is Direct3D on" (the base's answers whether SDL's renderer is accelerated). The
	// port draws as the original does without Direct3D, so the game always hears no.
	bool Is3DAccelerated() { return false; }
	// 0x766974: plays a sound unless sounds are off, the app has no focus or the board is paused; theTrack keeps it
	// in mTrackedSounds, theScaleVolume multiplies its volume by mSfxVolumeScale
	SoundInstance* PlaySoundEx(int theSoundId, int thePan, float thePitch, float theVolume, bool theTrack,
		bool theScaleVolume);
	// 0x763092: loads the Tube group (the sidebar's tube) and extracts it
	bool LoadTubeResources();
};

extern WinTwistApp* gApp;  // 0xe0b698

}  // namespace Sexy
