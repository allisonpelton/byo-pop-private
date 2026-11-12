#ifndef __TIPTOP_TIPTOPAPP_H__
#define __TIPTOP_TIPTOPAPP_H__

// The port's host for TipTop (no original counterpart: the original was a Win32 program on its LOWOS engine).
// The framework runs the window, the clock and the input; each of its 100 Hz updates runs one pass of the
// original's main loop (Game.h), which draws into LOWOS's 640x480 screen, and the widget presents that screen.

#include "SexyAppBase.h"
#include "Widget.h"

#include <vector>

namespace Sexy
{

class MemoryImage;
struct AutoplayState;

class TipTopWidget : public Widget
{
public:
	MemoryImage* mScreenImage;
	uint32_t mShownWrites;  // lowos::HostScreenWrites() when mScreenImage last held the screen (Draw), or when the screen was found the same
	bool mHasShown;         // Draw has filled mScreenImage once
	// The rectangle of the image that Draw changed in its last call, to the block of 16 pixels in x (empty, width 0, when the
	// picture was as before; the whole image at the first call), for a presenter that sends only what changed
	int mChangedX, mChangedY, mChangedWidth, mChangedHeight;
	// and the same pixels more closely: the runs of 32x32 tiles, one row of tiles at a time, that hold a changed pixel (in play
	// the changed pixels fill 5 to 6% of the image, the tiles 14 to 17% and the one rectangle 39 to 43%)
	struct ChangedRect
	{
		int x, y, width, height;
	};
	std::vector<ChangedRect> mChangedRects;
	bool ScreenDiffersFromShown();

	TipTopWidget();
	virtual ~TipTopWidget();

	virtual void Update() override;
	virtual void Draw(Graphics* g) override;
	virtual void KeyChar(SexyChar theChar) override;
	virtual void KeyDown(KeyCode theKey) override;
	virtual void KeyUp(KeyCode theKey) override;
	virtual void MouseMove(int x, int y) override;
	virtual void MouseDrag(int x, int y) override;
	virtual void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;
	virtual void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;
};

class TipTopApp : public SexyAppBase
{
public:
	TipTopWidget* mWidget;
	bool mGameStarted;
	bool mGameEnded;
	bool mApplyingGameMode;  // SwitchScreenMode is the game's own request (LOWOS_ScreenScope)

	// Test hooks (testhooks/Autoplay.cpp): -autoplay=<seed> plays games by itself with a bot that has its own
	// random numbers
	bool mAutoplay;
	int mAutoplayMode;       // 0 regular, 1 strategy, 2 speed
	int mAutoplayGames;
	int mAutoplayCap;
	std::string mAutoplayLog;
	std::string mAutoplayShots;
	int mAutoplaySave;        // -autoplaysave=N
	bool mAutoplayUi;         // -autoplayui: clicks the button Enter would press, for the original, which reads its keys
	                          // with DirectInput (games/tiptop/tools/identical/)
	std::string mAutoplayScript;  // -autoplayscript=<file>: the bot's input as a -script file
	AutoplayState* mAutoplayState;
	// -statelog=<file>: a line "<update> <name> <value>" at each change of the values that decide play (testhooks/Autoplay.cpp)
	std::string mStateLogName;
	FILE* mStateLog;
	void UpdateStateLog();
	// -buttonlog=<file>: the controls of the top layer (procedure, rectangle, command) whenever they change, for the
	// phase-two tours (testhooks/Autoplay.cpp)
	std::string mButtonLogName;
	// -soundlog=<file>: each sound's creation and start (engine/Lowos.cpp)
	FILE* mSoundLog;
	FILE* mButtonLog;
	void UpdateButtonLog();
	void UpdateMusicLog();

	TipTopApp();
	virtual ~TipTopApp();

	virtual void Init() override;
	virtual void Shutdown() override;
	virtual void CloseRequestAsync() override;
	virtual void GotFocus() override;
	virtual void LostFocus() override;
	virtual void SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false) override;
	virtual void SetMusicVolume(double theVolume) override;
	virtual void HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;
	// Test hooks (testhooks/Script.cpp): TipTop's -script commands, canvasshot and musicwav
	virtual bool HandleScriptCommand(const ScriptCommand& theCommand) override;
	// testhooks/Commands.cpp: the standard test commands (docs/TEST_COMMANDS.md): state, goto, level, set, win, lose, seed, move
	bool HandleTestCommand(const ScriptCommand& theCommand);
	// testhooks/Canvas.cpp: canvasshot <file> saves the whole screen of a -canvas run
	void StartCanvasShot(const std::string& theFile);
	void SaveCanvasShot();
	std::string mCanvasShot;  // the file of a canvasshot, saved after the next frame
	// testhooks/Canvas.cpp: -scramble fills the screen with noise before each pass and -screenhash=<file> logs a hash of the
	// screen after each pass: a pass that paints the whole screen gives the same hashes with and without the noise
	void ScrambleScreen();
	void LogScreenHash();
	bool mScrambleScreen = false;
	FILE* mScreenHashLog = nullptr;
	// The music's SoundFont bank (MusicSynth.h): -soundfont=<file>, else tiptop.sf2 beside the program
	std::string mSoundFont;
	using SexyAppBase::SwitchScreenMode;

	// One pass of the original's main loop, then the screen mode it asked for
	void RunFrame();
	// One step of the autoplay bot, before the game's pass
	void Autoplay();
};

extern TipTopApp* gApp;

} // namespace Sexy

#endif
