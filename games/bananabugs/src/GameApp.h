// GameApp: Banana Bugs' application (0x42c2b0..0x42dd90 in the original). It reads the distributor's file, sets the
// window to 800x600 over the game's 1920x1200 design space, loads the sounds, the two fonts and the music on the loading
// thread, then makes the one widget (GameWidget) that runs every screen through the scene graph (ObjBase).
#pragma once

#include "SexyApp.h"

namespace Sexy
{

class BBFont;
class GameWidget;
class SoundQueue;

// The music, as LoadingThreadCompleted loads it
enum MusicId
{
	MUSIC_LEVEL_1 = 0,
	MUSIC_LEVEL_2 = 1,
	MUSIC_LEVEL_3 = 2,
	MUSIC_LEVEL_4 = 3,
	MUSIC_META = 4,
	MUSIC_MAIN_MENU = 5,
	MUSIC_STATS = 6,
	MUSIC_VICTORY = 7,
	MUSIC_INTRO = 8,
	MUSIC_STATS_2 = 9,
	MUSIC_LEVEL_5 = 10,
};

class GameApp : public SexyApp
{
public:
	int mForcedWidth;         // the window's size whatever the mode, or -1 for the desktop's (see SwitchScreenMode)
	int mForcedHeight;
	BBFont* mRedFont;
	BBFont* mFont;
	SoundQueue* mSoundQueue;
	GameWidget* mGameWidget;
	float mDesignScaleX;      // design pixels to a window pixel, for the mouse
	float mDesignScaleY;
	float mWindowToDesignX;   // 1920 / the window's width
	float mWindowToDesignY;
	float mRootScaleX;        // the root object's scale (1 at the forced size)
	float mRootScaleY;
	bool mWidescreen;         // wider than 4:3
	int mCurrentMusic;
	bool mFocused;            // the cursor may be moved (WarpCursor)
	int mLastWidth;           // the size the pictures were last made for
	int mLastHeight;
	int mScreenModeKind;      // 0 at the forced size, 1 at the desktop's
	bool mFocusLost;          // set as the focus goes; the widget's next update clears it as it pauses the game
	char mRegistryKeyText[256]; // partner.xml's RegistryKey, as read
	char mPartnerImage[256];  // partner.xml's PartnerImage: a distributor's logo for the opening screen, or ""

	GameApp();
	virtual ~GameApp();

	void UpdateFrames() override;
	void LoadingThreadCompleted() override;
	void LoadingThreadProc() override;
	void Init() override;
	void SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false) override;
	using SexyApp::SwitchScreenMode;
	void GotFocus() override;
	void LostFocus() override;
	bool HandleScriptCommand(const ScriptCommand& theCommand) override; // port: the standard test commands (TestCommands.cpp)

	bool ReadPartnerFile();
	static bool DesktopIsWidescreen();
	void SetForcedSize(int theWidth, int theHeight);
	void ChangeMusic(int theSong);
	void WarpCursor(int x, int y);

	static void HideCursor();
	static void ShowCursor();
};

// 0x60e6bc is the engine's copy (ObjBase::sApp); this is the app itself
extern GameApp* gApp;

} // namespace Sexy
