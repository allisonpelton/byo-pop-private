// Sexy::WinTwistApp (vtable 0x7f433c, 111 slots; 0x75e900-0x768d00): Bejeweled Twist's app. Its fields are named
// here by their offsets in the original's object; the framework's own fields are SexyAppBase's.
#pragma once

#include "Buffer.h"
#include "Rect.h"
#include "SexyApp.h"
#include "TwistDialogListener.h"

#include <vector>

namespace Sexy {

class Board;
class D3DObject;
class Credits;
class ButtonWidget;
class CustomBassMusicInterface;
class CustomResourceManager;
class EffectsManager;
class ChallengeInfo;
class Gates;
class Hyperspace;
class Image;
class MemoryImage;
class LoaderScreen;
class Menu;
class MenuButtonWidget;
class PlayButton;
class Profile;
class Ranks;
class SoundInstance;
class UnderDialogWidget;
class VizManager;
class Widget;

class WinTwistApp : public SexyApp, public TwistDialogListener {
public:
	// The three sizes of the game's art (images\1200\, images\768\, images\600\): the height of the screen it was
	// drawn for. 2D mode always uses 600 at 800x600.
	enum { ART_RES_1200 = 1200, ART_RES_768 = 768, ART_RES_600 = 600 };

	int mArtRes = 0;                    // +0xf2c
	float mScale = 0.0f;                // +0xf30: mArtRes / 1200, from the art's 1600x1200 to the screen
	bool mArtResChanged = false;        // +0xf35: the options changed the art's size: "ArtRes" is written (0x762b40)
	bool mAllowArtRes1200 = true;       // +0xf28: the largest art only when the machine can take it
	bool mUseSexyCache = true;          // +0xf14: PopCap's image cache (SexyCache) had the pictures: no loader screen
	bool mInitialLoadDone = false;      // +0xf8d: Init got past loading the loader screen's resources
	bool mTubeLoaded = false;           // +0xf8c: the Tube group (0x763092)
	bool mLoadingThreadDone = false;    // +0x7f1: LoadingThreadProc loaded everything
	int mLoadMode = 0;                  // +0xf18: 1 skips the Sounds group, 2 loads only it (never set: always 0)
	bool mFlag764 = false;              // +0x764: set by the loader while mFlag784 is (meaning not yet known)
	bool mFlag784 = false;              // +0x784: (meaning not yet known; the loader's other buttons depend on it)
	Board* mBoard = nullptr;            // +0x7e4: the board while a game is on
	bool mUnknown8D8 = false;           // +0x8d8: cleared with the dialog 40
	// +0x7f4: each colour's gem drawn ahead at 7 sizes, from 1.375 times a cell (size 0) down to 0.625 in eighths
	// (size 3 is the cell), centred in 1.5 cells; a gem breaking up shrinks through them. +0x7f8: each colour's bomb
	// at half size. Made by MakeGemImages, null until then.
	MemoryImage* mGemImages[7][7] = {};
	MemoryImage* mBombImages[7] = {};
	bool mGemImagesMade = false;        // port: the original tests +0x7f4 for null
	int mPendingMusic = -1;             // +0x8e4: the track to play once the music has faded out, -1 none
	int mPendingMusicDelay = 0;         // +0x8e8: frames to wait after that (never set: always 0)
	int mNumIngameTracks = 0;           // +0x92c: the game's tunes, tracks 100 on (properties\music.xml)
	// port: mMusicInterface as the game's music (the original casts it, whatever CreateMusicInterface made); null
	// without sound, when the music calls do nothing
	CustomBassMusicInterface* mTwistMusic = nullptr;
	LoaderScreen* mLoaderScreen = nullptr;  // +0x860
	Menu* mMainMenu = nullptr;          // +0x864
	Gates* mGates = nullptr;            // +0xc70: the main menu's gates
	Profile* mProfile = nullptr;        // +0x7ec: the players
	Ranks* mRanks = nullptr;            // +0xf58: the ranks' names
	ChallengeInfo* mChallengeInfo = nullptr;  // +0xf5c: the challenges (properties\challenges.xml) and the goal check
	// The two top tens (classic and the other timeless mode: ScoreList), users\scores.dat
	struct HighScore {
		SexyString mName;
		int mLevel = 0;
		int mScore = 0;
	};
	HighScore mHighScores[20];          // +0x970: 0x24 each, ten a list
	bool mScoresLoaded = false;         // +0x50c: off while LoadScores reads them
	ButtonWidget* mWindowedButton = nullptr;  // +0x91c: top right, switches to a window (hidden in one)
	Image* mFruitImage = nullptr;       // +0x904: the fruit pictures the menu chose (comes with the board)
	Rect mFruitRect;                    // +0x908: its first cel
	bool mResumeGame = false;           // +0x930: a saved game is resumed at once (-resume; comes with the board)
	// +0xf60: the menu's play button was pressed (with no mode: resume the last game); every new game from the menu
	// sets it too, and a board started with it set starts afresh instead of from its save (0x6c9d0b)
	bool mPlayPressed = false;
	int mLastGameMode = 0;              // +0xf64: the high scores to show after a game
	int mUnknownF68 = 0;                // +0xf68: the board sets -1 (but not for a replay)
	int mUnknownF6C = 0;                // +0xf6c
	int mUnknownF70 = 0;                // +0xf70
	int mUnknown75C = 0;                // +0x75c: counts the games ended
	int mUnknown780 = 0;                // +0x780: counts the board's updates
	int mUnknown7D4 = 0;                // +0x7d4: seconds played (UpdateStats), and the next three by kind of game
	int mUnknown7D8 = 0;                // +0x7d8
	int mUnknown7DC = 0;                // +0x7dc: in puzzles
	int mUnknown7E0 = 0;                // +0x7e0
	int mUnknownC74[84] = {};           // +0xc74: cleared by a new board (0x75fc8a) unless the old one has +0x37c4
	// +0x758 (the framework's): the copy is registered (Init 0x7653a1, from the wrapper or MarketingMode); the
	// user's copies are full versions
	bool mRegistered = true;
	bool mUnknown759 = false;           // +0x759 (the framework's): set unless partner.xml's MarketingMode is LevelLock (Init)
	// +0xfa0: three of them (0x2c bytes each), cleared by every new game (0x760492), saved with the game (0x6a3192)
	struct UnknownFA0 {
		Buffer mBuffer;
		int mUnknown20 = -1;  // +0x20 (saved as a short)
		int mUnknown24 = -1;  // +0x24 (saved as a short)
		int mUnknown28 = 0;   // +0x28
	};
	UnknownFA0 mUnknownFA0[3];
	bool mUnknownF85 = false;           // +0xf85: StartMainMenu starts a classic game at once
	int mAutoPlayMode = -1;             // +0x1044: -autoplay's game (1 classic, 3 blitz, 4 zen), -1 none
	// port: this run draws through the GPU (GPUDisplay), as the original does with Direct3D: unless "Is3D" off, -nod3d9
	// or -noacceleration ask for 2D, or the display cannot do 3D (DisplayInitFailed)
	bool mUse3D = true;
	int mUse3DSwitch = -1;  // port: -3d (1), -nod3d9 or -noacceleration (0), which win over "Is3D"
	float mAutoPlayDelay = 0;           // +0x1048: -autoplay starts its game when this falls to 0
	bool mPlaySamples = true;           // +0xf74: PlaySample plays (always on so far)
	Hyperspace* mHyperspace = nullptr;  // +0xf20: the space behind every screen
	VizManager* mVizManager = nullptr;  // +0xc6c: the background behind a game
	// The 3D models (0x508650; empty in 2D mode): Init loads space's, the loading thread the rest
	D3DObject* mSidebarObject = nullptr;       // +0x81c
	D3DObject* mSpaceObject = nullptr;         // +0x820
	D3DObject* mOuterSpaceObject = nullptr;    // +0x824
	D3DObject* mStarsObject = nullptr;         // +0x828
	D3DObject* mSaucerObject = nullptr;        // +0x82c
	D3DObject* mSaucerLightsObject = nullptr;  // +0x830
	D3DObject* mCrystalObjects[7] = {};        // +0x834
	D3DObject* mPlanetObjects[2] = {};         // +0x850
	D3DObject* mPlanetRingsObject = nullptr;   // +0x858
	D3DObject* mWarpConeObject = nullptr;      // +0x85c
	bool mUnknownF1C = false;           // +0xf1c: the game was paused by losing the focus (0x75f51e)
	Widget* mUnknownF24 = nullptr;      // +0xf24: a 0x420-byte widget over everything (0x7243b1)
	EffectsManager* mFXManager = nullptr;  // +0xf78: the effects over the whole screen
	UnderDialogWidget* mUnderDialog = nullptr;  // +0xf7c: darkens the screen behind the dialogs
	float mDialogDim = 0;               // +0xf90: how dark mUnderDialog is, 0 to 1
	Credits* mCredits = nullptr;        // +0xf94: the credits while they are up (ShowCredits)
	Widget* mOptionsDialog = nullptr;   // +0xf40: the options while they are up (comes with the options)
	Widget* mUnknownF44 = nullptr;      // +0xf44: another widget the screen darkens behind (not yet known)
	bool mShowHints = true;             // +0xf8e: the options' hints box (registry ShowHints)
	bool mDoViz = true;                 // +0xf86: the options' visualizers box (registry DoViz)
	bool mSoundsOff = false;            // +0x928: no sound effects (set elsewhere, not yet read)
	SoundInstance* mUnmanagedSound = nullptr;  // +0x938: the voice (PlayVoice), released by UpdateVoice once it ends
	double mSfxVolumeScale = 1.0;       // +0x948: applied to the tracked sounds; lowered while a voice speaks
	double mVolumeRampMin = 0.6;        // +0x950: VOLUME_RAMP_MIN, the lowest mSfxVolumeScale under a voice
	double mVolumeRampTime = 0.3;       // +0x958: VOLUME_RAMP_TIME, seconds to ramp from 1 to mVolumeRampMin
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
	void GotFocus() override;           // 0x75f4f7: the board's focus pause ends
	void LostFocus() override;          // 0x75f51e: the board pauses (shown in a timed game) while the window is away
	MusicInterface* CreateMusicInterface() override;  // 0x75f98a
	void Shutdown() override;           // 0x762a0d
	void WriteToRegistry() override;    // 0x762b40
	void ReadFromRegistry() override;   // 0x762da8
	void MakeWindow() override;         // 0x75ef2a
	// 0x75fa42: every dialog is a TwistDialog that pops up, heard by the app
	Dialog* NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
		const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) override;
	// 0x75f2db: as wide as its buttons need (half the screen at least), a little above the middle
	Dialog* DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
		const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) override;
	// 0x75fab7: the dialog shrinks away first; it leaves the app's dialogs once it is gone (TwistDialog::Update)
	bool KillDialog(int theDialogId) override;
	using SexyApp::KillDialog;
	void ButtonDepress(int theId) override;  // 0x75f54e
	void DialogButtonDepress(int theDialogId, int theButtonId) override;  // 0x7633ea (also 0x767579)
	void DialogThirdButtonDepress(int theDialogId) override;  // 0x75f94f
	// port (tests): newgame <mode>, a new game of that mode at once, as the mode menu would start it
	bool HandleScriptCommand(const ScriptCommand& theCommand) override;
	// port: the standard test commands (TestCommands.cpp, docs/TEST_COMMANDS.md): state, goto, level, unlock, set, win,
	// lose, seed
	bool HandleTestCommand(const ScriptCommand& theCommand);
	std::string TestScreen();
	void UpdateTestGoto();
	int mTestGotoMode = -1;  // port (tests): "goto <mode>" pressed Play; the mode's button is pressed once the select opens
	// 0x76350c: -autoplay[=classic|zen|blitz], and the original's other test switches
	void HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;
	void PlaySample(int theSoundNum) override;  // 0x7675d8
	// 0x75f631: mutes, and pauses a fruit dance's music (Board::UpdateGame)
	void Mute(bool autoMute = false) override;
	// 0x75f672: unmutes, and a fruit dance's music goes on at the louder of the music's and the sounds' volumes
	void Unmute(bool autoMute = false) override;
	void PlayVoice(int theSoundId, bool theInterrupt);  // 0x767584 (slot 108): one at a time in mUnmanagedSound
	void UpdateVoice();                 // 0x75f1df: the ended voice released, the other sounds ramped down under one
	using SexyApp::PlaySample;

	// 0x75fada: the score list of a game mode (0 for mode 1, 1 for mode 3), -1 for a mode without one
	static int ScoreList(int theGameMode);
	// 0x7619f1: theScore's place in theGameMode's top ten (-1: none, or a mode without one), into +0xf68; the menu
	// (made if need be) keeps it for the scores it shows next
	int CheckForHighScore(int theGameMode, SexyString theName, int theLevel, int theScore);
	void LoadScores();          // 0x764ea9
	// 0x764f9c: the high score just made (the place in +0xf68, its score and level in +0xf6c and +0xf70, the
	// player's name) goes into its list, the rows below it move down and the lists are saved
	void AddHighScore();
	void SaveScores();          // 0x76387e
	void SetDefaultScores();    // 0x7637ee: the lists a new install starts with, from the exe's tables
	// 0x763964: "<player folder>\savegame_<mode>.dat", a challenge's with its name
	std::string GetSaveName(int theMode, int theChallenge);
	// 0x760492: a new board of theMode over the menu, afresh with thePlayPressed (see mPlayPressed)
	void NewGame(int theMode, bool thePlayPressed, int theChallenge);
	bool SaveExists(int theMode, int theChallenge);  // 0x763ace: never in blitz
	void DeleteSave(int theMode, int theChallenge);  // 0x763a85: never zen's or a puzzle's
	// 0x75eacd: the mode select (Mandala) over the screen, in app +0xf24; theMode 0 from the menu, 1 or 2 the zen jump
	void ShowModeSelect(int theMode);
	void OpenModeSelect();      // 0x76046d: the menu's play button
	// 0x763ca4: properties\music.xml and both songs, with their tracks
	void LoadMusic();
	void MakeGemImages();               // 0x7620a1
	void FreeGemImages();               // 0x762342 (the part that frees them)
	// 0x75fc23: plays a track, after the music fades out with theFade (unless none is playing); theExtra1 and 2 are
	// BASS's two extra play arguments, which the port leaves out
	void PlayMusic(int theTrackId, bool noLoop, bool theFade, int theExtra1, int theExtra2);
	int GetCurrentMusic();      // 0x75fc12: the track playing, -1 none
	void FadeOutMusic();        // 0x75fbf9
	// 0x76657b (slot 110): the loading is done, the main menu comes up
	virtual void StartMainMenu();
	// 0x765e04: back to the main menu, with the stats of the game just played, or with the challenges
	void ShowMainMenu(bool theEndStats, bool theChallenges, float theDelay);
	void ShowHyperspace();      // 0x75f588: the space behind every screen, made if need be
	void CreateGates();         // 0x75ec15
	void CreateMainMenu();      // 0x75ecb8
	void DeleteBoard();         // 0x75ebe3
	void PlaceWindowedButton(); // 0x75e966
	// 0x763b19: a main menu button of theImage's cels, theCover over it while it is shut, listening to the app
	MenuButtonWidget* MakeMenuButton(Image* theImage, Image* theCover, int theId, int theX, int theY,
		const SexyString& theLabel, const SexyString& theToolTip);
	// The main menu's spiral button (main menu +0x2e4)
	PlayButton* GetMainMenuPlayButton();

	// 0x760659: 2D mode, the art at 600 and the screen at 800x600 in 16 bits
	void Use2DMode();
	// 0x75fcd7: gGemWidth and gGemHeight for the art's size (64, 82 or 128)
	void SetGemSize();
	// port: the game's own question "is Direct3D on": yes when this run draws through GPUDisplay (2D runs clear the
	// framework's flag, which says whether SDL's renderer is accelerated, in PostDDInterfaceInitHook)
	bool Is3DAccelerated() { return SexyAppBase::Is3DAccelerated(); }
	// Whether it could be on (the options' "Not Supported" and "not recommended" answers; the original asks the
	// Direct3D device's caps). port: whether 3D mode's context can be had, WebGL2 in a browser; natively the port's
	// OpenGL is always there
	bool Is3DAccelerationSupported();
	bool Is3DAccelerationRecommended() { return Is3DAccelerationSupported(); }
	// port: a 3D run's display is GPUDisplay, in an OpenGL window
	DDInterface* CreateDDInterface() override;
	bool DisplayInitFailed(int theResult) override;  // 0x7606de
	uint32_t GetExtraWindowFlags() override;
	// 0x76063e (slot 69): with 3D as the framework has it
	void SwitchScreenMode(bool wantWindowed) override;
	// 0x75ed1e (slot 107): the windowed button and the options' full screen boxes follow
	void SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false) override;
	void PostDDInterfaceInitHook() override;
	using SexyApp::SwitchScreenMode;
	// 0x7632f7: the options close: their settings are applied (the resolution with theResolution); over the menu
	// the panel stays, over a game the dialog zooms out
	void FinishOptionsDialog(bool theResolution);
	// 0x75f6fc: the options come up over the game (if they are not up), as a base modal
	void ShowOptionsDialog();
	// 0x75f82d: the help comes up over the game (if it is not up), as a base modal
	void ShowHelpDialog();
	// 0x75fd40: the credits come up over everything (z order 1000) as a base modal; the Mandala's hidden sequence
	// is their only way in
	void ShowCredits();
	using SexyAppBase::RehupFocus;
	// 0x766974: plays a sound unless sounds are off, the app has no focus or the board is paused; theTrack keeps it
	// in mTrackedSounds, theScaleVolume multiplies its volume by mSfxVolumeScale
	SoundInstance* PlaySoundEx(int theSoundId, int thePan, float thePitch, float theVolume, bool theTrack,
		bool theScaleVolume);
	// 0x767601 (slot 109): PlaySoundEx panned by theX across the window
	void PlaySoundAt(int theSoundId, int theX, float thePitch, float theVolume);
	// 0x763092: loads the Tube group (the sidebar's tube) and extracts it
	bool LoadTubeResources();
};

extern WinTwistApp* gApp;  // 0xe0b698
// 0x84e884, 0x84e888: a gem's cell on the board, set by the art's size (WinTwistApp::SetGemSize)
extern int gGemWidth;
extern int gGemHeight;

}  // namespace Sexy
