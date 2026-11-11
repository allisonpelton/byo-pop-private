#ifndef __SEXYAPPBASE_H__
#define __SEXYAPPBASE_H__

// Portable SexyAppBase (SDL2 / Emscripten).
//
// Keeps the SexyAppFramework 1.3 public API and virtual hooks that game code overrides or calls, with
// the Win32/DirectX members replaced by SDL ones. Behavioural contracts preserved from 1.3:
//   * fixed 100 Hz Update() ticks (mFrameTime = 10 ms) with an accumulator capped at 200 ms,
//   * at most ~10 consecutive updates before a forced draw (slow down instead of frame skipping),
//   * Rand()/SRand() MT19937 seeded with the tick count in Init(),
//   * properties/default.xml string table, registry-style settings (persisted to a file),
//   * dialogs (DoDialog/KillDialog/DialogButtonPress -> ButtonPress(2000/3000 + id)).
// On Emscripten the loop runs from emscripten_set_main_loop; each browser frame runs every due update
// and then one draw.

#include "Common.h"
#include "Rect.h"
#include "Color.h"
#include "ButtonListener.h"
#include "DialogListener.h"
#include "Buffer.h"
#include "SharedImage.h"
#include "Ratio.h"

struct SDL_Window;
union SDL_Event;

namespace Sexy
{

class WidgetManager;
class DDInterface;
class Image;
class DDImage;
class Widget;
class SoundManager;
class MusicInterface;
class MemoryImage;
class Dialog;
class ResourceManager;

class WidgetSafeDeleteInfo
{
public:
	int mUpdateAppDepth;
	Widget* mWidget;
};

typedef std::list<WidgetSafeDeleteInfo> WidgetSafeDeleteList;
typedef std::set<MemoryImage*> MemoryImageSet;
typedef std::map<int, Dialog*> DialogMap;
typedef std::list<Dialog*> DialogList;
typedef std::vector<std::string> StringVector;
typedef std::map<std::string, SexyString> StringSexyStringMap;
typedef std::map<std::string, std::string> StringStringMap;
typedef std::map<std::string, std::wstring> StringWStringMap;
typedef std::map<std::string, bool> StringBoolMap;
typedef std::map<std::string, int> StringIntMap;
typedef std::map<std::string, double> StringDoubleMap;
typedef std::map<std::string, StringVector> StringStringVectorMap;
typedef std::map<std::string, std::string, StringLessNoCase> RegistryValueMap;

// port: one line of a -script file (see SexyAppBase::RunScript)
struct ScriptCommand
{
	int mUpdateCount;
	std::string mCommand;
	std::string mArgs;
	int mLine;
};

// port (web): a file the app writes at shutdown, as it would be written now (SexyAppBase::GetShutdownSaves)
struct ShutdownSave
{
	std::string mFileName;  // the app's name for it, e.g. "userdata\savegame.dat"
	std::string mData;
};

// port: a game's message about a mod, shown in the mod menu (SexyAppBase::GetModMessages), e.g. a rule the
// game could not use
struct ModMessage
{
	std::string mModId;  // "" for a message about all mods
	std::string mText;   // UTF-8
	bool mIsError;
};

// port: what SexyAppBase::RequestRestart did
enum
{
	RESTART_STARTED,   // the app shuts down and starts again
	RESTART_CONFIRM,   // a game is in progress: ask first (theMessage), then call again confirmed
	RESTART_BLOCKED    // not now (theMessage says why)
};

class ModMenu;

enum
{
	CURSOR_POINTER,
	CURSOR_HAND,
	CURSOR_DRAGGING,
	CURSOR_TEXT,
	CURSOR_CIRCLE_SLASH,
	CURSOR_SIZEALL,
	CURSOR_SIZENESW,
	CURSOR_SIZENS,
	CURSOR_SIZENWSE,
	CURSOR_SIZEWE,
	CURSOR_WAIT,
	CURSOR_NONE,
	CURSOR_CUSTOM,
	NUM_CURSORS
};

enum
{
	UPDATESTATE_MESSAGES,
	UPDATESTATE_PROCESS_1,
	UPDATESTATE_PROCESS_2,
	UPDATESTATE_PROCESS_DONE
};

// MessageBox-style flags/results used by MsgBox() (Win32 values).
enum
{
	MB_OK = 0,
	MB_OKCANCEL = 1,
	MB_YESNO = 4,
	MB_ICONERROR = 0x10,
	MB_ICONQUESTION = 0x20,
	MB_ICONWARNING = 0x30,
	MB_ICONINFORMATION = 0x40,
	IDOK = 1,
	IDCANCEL = 2,
	IDYES = 6,
	IDNO = 7
};

class SexyAppBase : public ButtonListener, public DialogListener
{
public:
	ulong mRandSeed;

	std::string mCompanyName;
	std::string mFullCompanyName;
	std::string mProdName;
	SexyString mTitle;
	std::string mRegKey;
	std::string mChangeDirTo;

	int mRelaxUpdateBacklogCount;
	int mPreferredX;
	int mPreferredY;
	int mWidth;
	int mHeight;
	int mFullscreenBits;
	double mMusicVolume;
	double mSfxVolume;
	double mDemoMusicVolume;
	double mDemoSfxVolume;
	bool mNoSoundNeeded;
	bool mWantFMod;
	bool mCmdLineParsed;
	bool mSkipSignatureChecks;
	bool mStandardWordWrap;
	bool mbAllowExtendedChars;

	bool mOnlyAllowOneCopyToRun;
	uchar mAdd8BitMaxTable[512];
	WidgetManager* mWidgetManager;
	DialogMap mDialogMap;
	DialogList mDialogList;
	bool mSEHOccured;
	bool mShutdown;
	bool mExitToTop;
	bool mIsWindowed;
	bool mIsPhysWindowed;
	bool mFullScreenWindow;
	bool mForceFullscreen;
	bool mForceWindowed;
	bool mInitialized;
	bool mProcessInTimer;
	DWORD mTimeLoaded;
	SDL_Window* mHWnd;  // the SDL window (named after the original HWND member)
	bool mIsScreenSaver;
	bool mAllowMonitorPowersave;
	bool mNoDefer;
	bool mFullScreenPageFlip;
	bool mTabletPC;
	DDInterface* mDDInterface;
	bool mAlphaDisabled;
	bool mWant3D;  // port: the 3D choice Set3DAcclerated recorded (mUserChanged3DSetting), applied again whenever the display is made
	MusicInterface* mMusicInterface;
	bool mReadFromRegistry;
	std::string mRegisterLink;
	std::string mProductVersion;
	Image* mCursorImages[NUM_CURSORS];
	bool mIsOpeningURL;
	bool mShutdownOnURLOpen;
	std::string mOpeningURL;
	DWORD mOpeningURLTime;
	DWORD mLastTimerTime;
	DWORD mLastBigDelayTime;
	double mUnmutedMusicVolume;
	double mUnmutedSfxVolume;
	int mMuteCount;
	int mAutoMuteCount;
	bool mDemoMute;
	bool mMuteOnLostFocus;
	MemoryImageSet mMemoryImageSet;
	SharedImageMap mSharedImageMap;
	bool mCleanupSharedImages;

	int mNonDrawCount;
	int mFrameTime;

	bool mIsDrawing;
	bool mLastDrawWasEmpty;
	bool mHasPendingDraw;
	double mPendingUpdatesAcc;
	double mUpdateFTimeAcc;
	DWORD mLastTimeCheck;
	DWORD mLastTime;
	DWORD mLastUserInputTick;

	int mSleepCount;
	int mDrawCount;
	int mUpdateCount;
	int mUpdateAppState;
	int mUpdateAppDepth;
	double mUpdateMultiplier;
	bool mPaused;
	int mFastForwardToUpdateNum;
	bool mFastForwardToMarker;
	bool mFastForwardStep;
	DWORD mLastDrawTick;
	DWORD mNextDrawTick;
	int mStepMode;

	int mCursorNum;
	SoundManager* mSoundManager;
	WidgetSafeDeleteList mSafeDeleteList;
	bool mMouseIn;
	bool mRunning;
	bool mActive;
	bool mMinimized;
	bool mPhysMinimized;
	bool mIsDisabled;
	bool mHasFocus;
	int mDrawTime;
	ulong mFPSStartTick;
	int mFPSFlipCount;
	int mFPSDirtyCount;
	int mFPSTime;
	int mFPSCount;
	bool mShowFPS;
	int mShowFPSMode;
	int mScreenBltTime;
	bool mAutoStartLoadingThread;
	bool mLoadingThreadStarted;
	bool mLoadingThreadCompleted;
	bool mLoaded;
	bool mYieldMainThread;
	bool mLoadingFailed;
	bool mSysCursor;
	bool mCustomCursorsEnabled;
	bool mCustomCursorDirty;
	bool mLastShutdownWasGraceful;
	bool mIsWideWindow;
	float mMouseSensitivity;

	int mNumLoadingThreadTasks;
	int mCompletedLoadingThreadTasks;

	// Demo recording/playback is not supported by the port; kept so game code can test the flags.
	bool mRecordingDemoBuffer;
	bool mPlayingDemoBuffer;
	bool mManualShutdown;

	bool mDebugKeysEnabled;
	bool mEnableMaximizeButton;
	bool mCtrlDown;
	bool mAltDown;

	int mSyncRefreshRate;
	bool mVSyncUpdates;
	bool mWaitForVSync;
	bool mUserChanged3DSetting;
	bool mAutoEnable3D;
	bool mTest3D;
	bool mWidescreenAware;
	Rect mScreenBounds;
	bool mEnableWindowAspect;
	Ratio mWindowAspect;

	StringWStringMap mStringProperties;
	StringBoolMap mBoolProperties;
	StringIntMap mIntProperties;
	StringDoubleMap mDoubleProperties;
	StringStringVectorMap mStringVectorProperties;
	ResourceManager* mResourceManager;

	// --- port additions ---
	// Emulated registry: "SOFTWARE\<mRegKey>\[subkey\]value" -> "sz:text" / "dword:123" / "hex:0a1b..",
	// case-insensitive like the real one, persisted to <user dir>/registry.ini.
	RegistryValueMap mRegistry;
	bool mRegistryLoaded;
	bool mRegistryDirty;  // changed since the last successful SaveRegistry()
	std::vector<std::string> mCmdLineArgs;
	int mLastMouseX;  // last mouse position in logical coordinates
	int mLastMouseY;
	bool mLoadingThreadRunning;  // LoadingThreadProc() is running (synchronously, see StartLoadingThread)
	bool mNothingDue;  // Emscripten: the last Process() found nothing to do before the next browser frame
	bool mBackupDirty;  // Emscripten: a user file changed since the last UpdateUserDataBackup()
	DWORD mLastBackupCheck;  // Emscripten: when UpdateUserDataBackup() last compared, and last backed up
	DWORD mLastBackupTime;
	bool mBackupHasSave;  // Emscripten: what the last backup held as the shutdown saves and registry
	std::string mBackupSave;  // the saves' names and bytes, one after the other
	std::string mBackupRegistry;
	bool mRegistryCapture;  // RegistryWrite() only changes mRegistry (GetShutdownRegistryText)

	// Test hooks, set from the command line (also the base for a "set seed" option and replays):
	//   -seed=N         seed Rand() with N instead of the tick count
	//   -script=<file>  scripted input; the real mouse, keyboard and focus changes are ignored. One
	//                   command per line, "<update count> <command> [args]", run when mUpdateCount
	//                   reaches the count (before that update; '#' starts a comment, except inside "..."):
	//                     click <x> <y>        mouse down + up (left button) at logical coordinates
	//                     rclick <x> <y>       mouse down + up with the right button
	//                     down|up|move <x> <y> one mouse event (a move with the button down drags)
	//                     key <keycode>        key down + up (number or KeyCodes.cpp name, e.g. RETURN);
	//                                          Enter, Backspace, Tab and Esc also type their character
	//                                          (no other key does: "key SPACE" types no space)
	//                     keydown|keyup <keycode>  only the key down (typing nothing) or only the key up,
	//                                          e.g. to hold SHIFT or MENU (Alt) around another key
	//                     char <c>             one typed character (KeyChar): the first character of
	//                                          the argument, which is trimmed; "char SPACE" (exactly)
	//                                          types a space
	//                     screenshot <png>     draw the screen now and save it at the logical size (the
	//                                          hovered gem's crossfade from the 1x pixels, as in 1x
	//                                          rendering, see CreateCrossfadeImage)
	//                     screenshotfull <png> the same at the render target's size (shows HD art)
	//                     focus lose|regain    the window loses or regains the focus, as its focus
	//                                          event does (mute, LostFocus/GotFocus), like origrun's
	//                     quit                 Shutdown()
	//                     pointer              answers POINTER {"x","y","in"}: the mouse as the widgets
	//                                          last saw it, in game pixels (tools/web/pagecheck.js)
	//                     modmenu <what> ...   the mod menu (framework/ModMenu.h; a mod id with spaces in
	//                                          "..."): open, close, page <mod> | list, enable <mod> 0|1,
	//                                          move <mod> -1|1, set <mod> <setting> <JSON value>, reset <mod>,
	//                                          restart [confirm], dump <file> (the state JSON). F2 is real
	//                                          input only: "key F2" goes to the focused widget.
	//                   Other commands go to the game's HandleScriptCommand (WinDMApp: save <file>).
	//   -fast           updates run back to back on a virtual clock and the screen is drawn every
	//                   FAST_DRAW_INTERVAL updates, so a script runs as fast as the machine allows
	//   -trace          the game prints its state to stdout (WinDMApp: the board when it settles)
	//   -drawevery      every update is drawn before the next one runs (with -fast too), instead of
	//                   catching up with updates that are not drawn when the machine falls behind, so
	//                   game logic that waits for a draw (a transition done once drawn) ends at the same
	//                   update on any machine; the game runs slower than real time if drawing is slow
	//   -renderscale=N  render at N times the logical size (1..4) whatever the window size, e.g. to
	//                   check HD art with screenshotfull
	//   -realinput=<file>  real input: timed SDL events pushed into SDL's own queue (SDL_PushEvent), so they take
	//                   the path a hand's events take (SDL_PollEvent, HandleEvent, the window-to-game mapping, the
	//                   widget manager, F2 and Alt+Enter); the system's own mouse, keyboard, text and focus events
	//                   are dropped, so the run is the same whatever happens to the real ones (docs/REAL_INPUT.md).
	//                   The file is a -script file: the events of a line are pushed when update <count> starts and
	//                   handled after it, before the next update, as a hand's events between two updates are. Not
	//                   with -script or a game's -autoplay, which drop real input. A point is "<x> <y>" in window
	//                   pixels, or "game <x> <y>" in the game's, mapped as the window shows them at that moment;
	//                   [button] is left (the default), right, middle, x1 or x2; <key> is an SDL key name (A, 7,
	//                   Return, Escape, Space, Backspace, Tab, F2, Up, Left Shift, Left Alt, Left Ctrl, Keypad 1).
	//                     move <point>          SDL_MOUSEMOTION with the buttons held (none for no change of
	//                                           point, except the first after a leave); the first move, and the
	//                                           first after a leave, comes after the window's SDL_WINDOWEVENT_ENTER
	//                     down <point> [button] [clicks]  SDL_MOUSEBUTTONDOWN, after a move to the point (clicks 1;
	//                                           an even count is a double click)
	//                     up <point> [button]   SDL_MOUSEBUTTONUP, after a move to the point
	//                     click <point> [button]     down and up
	//                     dblclick <point> [button]  down and up, then down and up with clicks 2
	//                     wheel <dy> [dx]       SDL_MOUSEWHEEL at the mouse (dy > 0: away from the player)
	//                     enter, leave          the window's SDL_WINDOWEVENT_ENTER or LEAVE
	//                     keydown|keyup <key>   SDL_KEYDOWN or SDL_KEYUP with its scancode and the modifiers held,
	//                                           updated first as SDL does (Left Alt's own press carries KMOD_LALT);
	//                                           a press that types (letters, digits, Space, symbols, on a US
	//                                           layout, with Shift and Caps Lock, without Ctrl or Alt) is followed
	//                                           by its SDL_TEXTINPUT, as Windows sends WM_CHAR after WM_KEYDOWN
	//                     key <key>             keydown and keyup
	//                     keyrepeat <key>       SDL_KEYDOWN with repeat set (and its text), as a held key repeats
	//                     type <text>           each character's key pressed and released, Shift held for capitals
	//                                           and shifted symbols (printable ASCII; "..." keeps outer spaces)
	//                     text <text>           SDL_TEXTINPUT alone, UTF-8 (as an input method sends it)
	//                     focus lost|gained     SDL_WINDOWEVENT_FOCUS_LOST or _GAINED (also lose|regain); a loss
	//                                           releases the held keys first, as SDL_ResetKeyboard does
	//                     minimize, restore, maximize  SDL_MinimizeWindow and so on: the window really changes and
	//                                           SDL sends its own events (a hand's minimize also loses the focus:
	//                                           write "focus lost" before it)
	//                     resize <w> <h>        SDL_SetWindowSize: the window's client area, in screen pixels
	//                     close                 the window's close button: SDL_WINDOWEVENT_CLOSE, then SDL_QUIT
	//                     screenshot, screenshotfull, quit, pointer  as in -script (done at once, not pushed)
	//                   Full screen is Alt+Enter: "keydown Left Alt", "key Return", "keyup Left Alt". Other
	//                   commands go to the game's HandleScriptCommand, run at once; so does a bare "move" (no
	//                   point), which asks the game's bot for its next move (docs/TEST_COMMANDS.md).
	//   -commands=<file>  live commands, for a driver that decides as it goes (docs/TEST_COMMANDS.md, "Sending
	//                   them"): it appends lines "<command> [args]" to the file (no update count; '#' comments and
	//                   blank lines as in -script), and the framework reads the new whole lines between updates (each
	//                   update, every FAST_DRAW_INTERVAL with -fast) and runs them at the next one, in order, through
	//                   -script's dispatch: the framework's commands above, then the game's HandleScriptCommand. With
	//                   -realinput (its file may be empty) they take -realinput's instead, so a driver's moves are
	//                   real SDL events. A line still being written waits for its end of line; a file not there yet
	//                   is waited for; one that gets shorter is read again from its start. Each command is answered
	//                   on the log, which is then flushed line by line: "OK <command>" when the framework did it,
	//                   "ERROR <command>: <reason>" when it could not or nobody knows the command, and a game's own
	//                   command answers itself (STATE {...}, OK, ERROR). The window's own input still counts, as in
	//                   a plain run (with -realinput it is dropped). The run is a test run (IsTestRun). On the web,
	//                   byopop_test_command(line) queues one line the same way (the player-only test page's
	//                   byopop.test.command, docs/WEB_PLAYER.md). The channel's own command:
	//                     wait <n>              the lines after it wait n updates; answered "OK wait <n>" when
	//                                           they are up (n 0: at once)
	//   -hold           with -commands, natively: once the game has loaded, an update runs only when the driver
	//                   lets it, so that a game under -fast does not run on while its driver decides, and the same
	//                   lines play the same way. New lines let one update run (the one that runs them), "wait <n>"
	//                   lets n run; held, nothing moves (-fast's clock neither) and the loop sleeps a millisecond at
	//                   a time. tools/playbymove plays a game this way by its "move" answers.
	enum { FAST_DRAW_INTERVAL = 10 };
	bool mHasSeedParam;
	bool mFastMode;
	bool mTraceMode;
	bool mDrawEvery;  // -drawevery
	int mForcedRenderScale;  // -renderscale=N
	bool mDrawingLogicalShot;  // SaveScreenshot at the logical size is drawing
	DWORD mFastClock;          // -fast: virtual milliseconds, advanced instead of sleeping
	int mLastFastDrawUpdate;   // -fast: mUpdateCount at the last draw
	std::string mScriptFileName;
	std::vector<ScriptCommand> mScript;
	size_t mScriptPos;         // next command to run
	bool mScriptedInput;       // a test bot (the game's -autoplay) feeds mScript: real input is ignored as in a -script run
	// -realinput: timed SDL events (RunRealInput), and the hand that plays them, as SDL would keep it for real input
	std::string mRealInputFileName;
	std::vector<ScriptCommand> mRealInput;
	size_t mRealInputPos;      // next command to run
	int mRealMouseX;           // the mouse in window pixels (-1 before its first move)
	int mRealMouseY;
	unsigned int mRealButtons; // SDL_BUTTON() mask of the buttons held
	int mRealClicks;           // the click count of the last press (its release carries it too)
	bool mRealMouseIn;         // the window has had SDL_WINDOWEVENT_ENTER and no LEAVE since
	unsigned short mRealMods;  // KMOD_ state
	std::vector<int> mRealKeysDown;  // the SDL keycodes held
	// live commands: -commands=<file> natively, byopop_test_command on the web (RunLiveCommands)
	std::string mCommandsFileName;
	long long mCommandsFilePos;    // bytes of the file read so far
	int mCommandsFileLine;         // its lines read so far
	bool mCommandsWaitLogged;      // "waiting for" the file was logged
	int mLiveCommandCount;         // the web's commands so far (their line numbers)
	std::vector<ScriptCommand> mLiveCommands;  // waiting for the next update
	bool mCommandsHold;            // -hold: once loaded, an update runs only when the driver's lines let it
	int mLiveRunUntil;             // -hold: the updates run while mUpdateCount is below it
	int mLiveWaitUntil;            // "wait <n>": the update its answer and the lines after it wait for, -1 for none
	std::string mLiveWaitLine;     // that wait's line, for its answer

	// port: the mod menu (framework/ModMenu.h, docs/MOD_MENU_DESIGN.md). F2 opens and closes it; nothing of it
	// exists until it first opens.
	ModMenu* mModMenu;          // while it is open
	bool mModMenuKeyDown;       // the F2 that toggled it is down: its key-up is not passed on
	int mModMenuClosingKey;     // the key that closed it (Esc, Enter, Space) while it is held: its repeats and the
	                            // text they type are not passed on, so they cannot reach the game (its release is)
	int mKeyInHandling;         // the key whose real press (or the text it typed) the widgets are getting, else 0
	int mLastKeyPressed;        // the last real key pressed: the key behind the text input that follows it
	bool mRestartRequested;     // RequestRestart: the app shuts down to start again (main(): TakeRelaunch)

	// port (web): the click that brings the page back from the background reaches neither the game nor its
	// widgets (HandleEvent)
	bool mActivationClickPending; // the app just became active: a press until mActivatedTime + 150 ms activated it
	DWORD mActivatedTime;       // SDL's timestamp of that event
	DWORD mActivationButtons;   // SDL_BUTTON masks of the presses dropped that way: their releases are dropped too

protected:
	void RehupFocus();
	void ClearKeysDown();
	void UpdateFTimeAcc();
	virtual bool Process(bool allowSleep = true);
	virtual void UpdateFrames();
	virtual bool DoUpdateFrames();
	virtual void DoUpdateFramesF(float theFrac);
	virtual void MakeWindow();
	// port: the display MakeWindow makes (a DDInterface unless a game gives another, such as a GPUDisplay), and SDL
	// window flags it adds to the window's own (0 unless a game's display needs one, such as SDL_WINDOW_OPENGL)
	virtual DDInterface* CreateDDInterface();
	virtual uint32_t GetExtraWindowFlags();
	// port: called by MakeWindow when its display's Init fails with theResult, before it reports the failure and exits;
	// a game that can go on without what failed (Bejeweled Twist's 2D mode for a display without 3D) makes its window
	// again and returns true. False by default.
	virtual bool DisplayInitFailed(int theResult);
	virtual void EnforceCursor();
	virtual void ReInitImages();
	virtual void DeleteNativeImageData();
	virtual void DeleteExtraImageData();

	virtual void LoadingThreadCompleted();
	static void LoadingThreadProcStub(void* theArg);

	void ProcessSafeDeleteList();
	void DoExit(int theCode);
	void TakeScreenshot();

	// Event processing (SDL replacement for WindowProc / ProcessDeferredMessages). ProcessEvents()
	// handles one pending event and returns false when there was none.
	virtual bool ProcessEvents();
	virtual void HandleEvent(const SDL_Event& theEvent);
	void TranslateMouse(int theWindowX, int theWindowY, int* theX, int* theY);
	void MouseMoveTo(int theX, int theY);  // the part of a mouse event shared by real and scripted input

	// Test hooks (see mScript)
	bool LoadScript(const std::string& theFileName);
	bool LoadScriptFile(const std::string& theFileName, const char* theWhat, std::vector<ScriptCommand>* theCommands);
	void RunScript();
	void RunRealInput();
	// one command of -script, -realinput or the live channel: what became of it, and the reason of a failure
	enum { SCRIPT_RAN, SCRIPT_GAME, SCRIPT_FAILED, SCRIPT_UNKNOWN };
	int RunScriptCommand(const ScriptCommand& theCommand, std::string* theError);
	int RunRealInputCommand(const ScriptCommand& theCommand, std::string* theError);
	void PollLiveCommands();
	void RunLiveCommands();
	bool LiveUpdatesHeld();
	void PushRealEvent(SDL_Event* theEvent);
	void PushRealWindowEvent(int theWindowEvent);
	void PushRealMouseMove(int theX, int theY);
	void PushRealButton(int theButton, bool isDown, int theX, int theY, int theClicks);
	void PushRealKey(int theKey, bool isDown, bool isRepeat);
	void PushRealText(const std::string& theText);
	// A script command the framework does not know; return true if handled (the default handles none)
	virtual bool HandleScriptCommand(const ScriptCommand& theCommand);
	bool SaveScreenshot(const std::string& theFileName, bool fullResolution = false);
	// SaveScreenshot for the framework's shots taken mid-update (after the count moved, before the widgets updated),
	// drawn with the count one lower
	bool SaveScreenshotMidUpdate(const std::string& theFileName, bool fullResolution = false);

	// Registry emulation helpers
	void LoadRegistry();
	bool SaveRegistry();
	std::string GetRegistryText();
	std::string RegistryPath();
	std::string RegistryValuePath(const std::string& theValueName);
	bool RegistryWrite(const std::string& theValueName, const std::string& theValue);
	bool RegistryRead(const std::string& theValueName, std::string* theValue);

	// The tail of 1.3's Start() (after the main loop), shared with the Emscripten loop callback.
	void MainLoopFinished();

#ifdef __EMSCRIPTEN__
	static void EmscriptenMainLoop(void* theArg);
	// the page went out of sight (another tab, the browser minimized): the focus is lost at once (SexyAppBase.cpp, DoMainLoop)
	static void EmscriptenPageHidden(SexyAppBase* theApp);
#endif

public:
	SexyAppBase();
	virtual ~SexyAppBase();

	// port: the app clock: timeGetTime(), or the virtual clock with -fast (public for WidgetManager)
	DWORD GetClockTime();

	// Common overrides:
	virtual MusicInterface* CreateMusicInterface();
	virtual void InitHook();
	virtual void ShutdownHook();
	virtual void PreTerminate();
	virtual void LoadingThreadProc();
	virtual void WriteToRegistry();
	virtual void ReadFromRegistry();
	virtual Dialog* NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode);
	virtual void PreDisplayHook();

	// Public methods
	virtual void BeginPopup();
	virtual void EndPopup();
	virtual int MsgBox(const std::string& theText, const std::string& theTitle = "Message", int theFlags = MB_OK);
	virtual void Popup(const std::string& theString);
	virtual void SafeDeleteWidget(Widget* theWidget);

	virtual void URLOpenFailed(const std::string& theURL);
	virtual void URLOpenSucceeded(const std::string& theURL);
	virtual bool OpenURL(const std::string& theURL, bool shutdownOnOpen = false);
	virtual std::string GetProductVersion(const std::string& thePath);

	virtual void SEHOccured();
	virtual std::string GetGameSEHInfo();
	virtual void Shutdown();

	virtual void DoParseCmdLine();
	virtual void ParseCmdLine(const std::string& theCmdLine);
	virtual void HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue);
	virtual void HandleNotifyGameMessage(int theType, int theParam);
	virtual void HandleGameAlreadyRunning();

	virtual void Start();
	virtual void Init();
	virtual void PreDDInterfaceInitHook();
	virtual void PostDDInterfaceInitHook();
	virtual bool ChangeDirHook(const char* theIntendedPath);
	virtual void PlaySample(int theSoundNum);
	virtual void PlaySample(int theSoundNum, int thePan);

	virtual double GetMasterVolume();
	virtual double GetMusicVolume();
	virtual double GetSfxVolume();
	virtual bool IsMuted();

	virtual void SetMasterVolume(double theVolume);
	virtual void SetMusicVolume(double theVolume);
	virtual void SetSfxVolume(double theVolume);
	virtual void Mute(bool autoMute = false);
	virtual void Unmute(bool autoMute = false);

	void StartLoadingThread();
	virtual double GetLoadingThreadProgress();

	void CopyToClipboard(const std::string& theString);
	std::string GetClipboard();

	void SetCursor(int theCursorNum);
	int GetCursor();
	void EnableCustomCursors(bool enabled);
	// GetImage gives NULL for a missing file, or with -placeholders (platform/Placeholders.h) a checkerboard of the
	// size GetPlaceholderSize gives (default 64x64; a game overrides it with its images' sizes, so cels and layout
	// stay put). GetOptionalImage never gives a placeholder: for a file a game may lack (a co-brand image).
	// GrowPlaceholder enlarges a placeholder to at least theWidth x theHeight (a font's glyphs need the room).
	virtual DDImage* GetImage(const std::string& theFileName, bool commitBits = true);
	DDImage* GetOptionalImage(const std::string& theFileName, bool commitBits = true);
	virtual void GetPlaceholderSize(const std::string& theFileName, int* theWidth, int* theHeight);
	static void GrowPlaceholder(Image* theImage, int theWidth, int theHeight);
	virtual SharedImageRef GetSharedImage(const std::string& theFileName, const std::string& theVariant = "", bool* isNew = NULL);

	void CleanSharedImages();
	void PrecacheAdditive(MemoryImage* theImage);
	void PrecacheAlpha(MemoryImage* theImage);
	void PrecacheNative(MemoryImage* theImage);
	void SetCursorImage(int theCursorNum, Image* theImage);

	DDImage* CreateCrossfadeImage(Image* theImage1, const Rect& theRect1, Image* theImage2, const Rect& theRect2, double theFadeFactor);
	void ColorizeImage(Image* theImage, const Color& theColor);
	DDImage* CreateColorizedImage(Image* theImage, const Color& theColor);
	DDImage* CopyImage(Image* theImage, const Rect& theRect);
	DDImage* CopyImage(Image* theImage);
	void MirrorImage(Image* theImage);
	void FlipImage(Image* theImage);
	void RotateImageHue(Sexy::MemoryImage* theImage, int theDelta);
	ulong HSLToRGB(int h, int s, int l);
	ulong RGBToHSL(int r, int g, int b);
	void HSLToRGB(const ulong* theSource, ulong* theDest, int theSize);
	void RGBToHSL(const ulong* theSource, ulong* theDest, int theSize);

	void AddMemoryImage(MemoryImage* theMemoryImage);
	void RemoveMemoryImage(MemoryImage* theMemoryImage);
	void Remove3DData(MemoryImage* theMemoryImage);
	virtual void SwitchScreenMode();
	virtual void SwitchScreenMode(bool wantWindowed);
	virtual void SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false);
	virtual void SetAlphaDisabled(bool isDisabled);

	virtual Dialog* DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode);
	virtual Dialog* GetDialog(int theDialogId);
	virtual void AddDialog(int theDialogId, Dialog* theDialog);
	virtual void AddDialog(Dialog* theDialog);
	virtual bool KillDialog(int theDialogId, bool removeWidget, bool deleteWidget);
	virtual bool KillDialog(int theDialogId);
	virtual bool KillDialog(Dialog* theDialog);
	virtual int GetDialogCount();
	virtual void ModalOpen();
	virtual void ModalClose();
	virtual void DialogButtonPress(int theDialogId, int theButtonId);
	virtual void DialogButtonDepress(int theDialogId, int theButtonId);

	virtual void GotFocus();
	virtual void LostFocus();
	virtual bool DebugKeyDown(int theKey);
	virtual void CloseRequestAsync();
	bool Is3DAccelerated();
	bool Is3DAccelerationSupported();
	bool Is3DAccelerationRecommended();
	void Set3DAcclerated(bool is3D, bool reinit = true);
	virtual void Done3dTesting();
	virtual std::string NotifyCrashHook();
	virtual bool CheckSignature(const Buffer& theBuffer, const std::string& theFileName);
	virtual bool DrawDirtyStuff();
	virtual void Redraw(Rect* theClipRect);

	// Properties access methods
	bool LoadProperties(const std::string& theFileName, bool required, bool checkSig);
	bool LoadProperties();
	virtual void InitPropertiesHook();

	// Resource access methods
	void LoadResourceManifest();
	void ShowResourceError(bool doExit = false);

	bool GetBoolean(const std::string& theId);
	bool GetBoolean(const std::string& theId, bool theDefault);
	int GetInteger(const std::string& theId);
	int GetInteger(const std::string& theId, int theDefault);
	double GetDouble(const std::string& theId);
	double GetDouble(const std::string& theId, double theDefault);
	SexyString GetString(const std::string& theId);
	SexyString GetString(const std::string& theId, const SexyString& theDefault);
	StringVector GetStringVector(const std::string& theId);

	void SetBoolean(const std::string& theId, bool theValue);
	void SetInteger(const std::string& theId, int theValue);
	void SetDouble(const std::string& theId, double theValue);
	void SetString(const std::string& theId, const std::wstring& theValue);

	// Registry access methods (emulated, see mRegistry)
	bool RegistryGetSubKeys(const std::string& theKeyName, StringVector* theSubKeys);
	bool RegistryReadString(const std::string& theValueName, std::string* theString);
	bool RegistryReadInteger(const std::string& theValueName, int* theValue);
	bool RegistryReadBoolean(const std::string& theValueName, bool* theValue);
	bool RegistryReadData(const std::string& theValueName, uchar* theValue, ulong* theLength);
	bool RegistryWriteString(const std::string& theValueName, const std::string& theString);
	bool RegistryWriteInteger(const std::string& theValueName, int theValue);
	bool RegistryWriteBoolean(const std::string& theValueName, bool theValue);
	bool RegistryWriteData(const std::string& theValueName, const uchar* theValue, ulong theLength);
	bool RegistryEraseKey(const SexyString& theKeyName);
	void RegistryEraseValue(const SexyString& theValueName);

	// File access methods (through the VFS). Writes always land in the writable user dir ("userdata\x"
	// -> <user dir>/x, other relative paths -> <user dir>/<path>); reads look there first, then in the
	// game data (mods + base dir).
	bool WriteBufferToFile(const std::string& theFileName, const Buffer* theBuffer);
	bool ReadBufferFromFile(const std::string& theFileName, Buffer* theBuffer, bool dontWriteToDemo = false);
	bool WriteBytesToFile(const std::string& theFileName, const void* theData, unsigned long theDataLen);
	bool FileExists(const std::string& theFileName);
	bool EraseFile(const std::string& theFileName);

	// Misc methods
	virtual void DoMainLoop();
	virtual bool UpdateAppStep(bool* updated);
	virtual bool UpdateApp();
	int InitDDInterface();
	void ClearUpdateBacklog(bool relaxForASecond = false);
	bool IsScreenSaver();
	virtual bool AppCanRestore();
	virtual void ShowFPS(bool show);

	// --- port additions ---
	// Command-line style args ("-key=value"), filled by main() before Init().
	void SetCmdLine(int argc, char** argv);
	// Web: persists the IDBFS-mounted user dir to IndexedDB (no-op natively, files are written
	// directly). Called after every save, registry write and erase.
	virtual void SyncUserData();
	// Web: a page can be closed, reloaded, discarded or crash without the app shutting down, and the
	// browser may not even report it. So localStorage keeps a backup of the user dir as it would be
	// after a shutdown at this moment (Platform::BackupUserData), restored at the next start: the user
	// files, with the registry as WriteToRegistry() would leave it, plus GetShutdownSaves(), the files the
	// app writes at shutdown (none: no file; Bejeweled: the save and its sidecar, taken at the same tick).
	// Saves and settings still reach the user dir only as in 1.3.
	virtual void GetShutdownSaves(std::vector<ShutdownSave>* theSaves);
	// The registry writes a shutdown makes, last one last (only mRegistry changes; see mRegistryCapture)
	virtual void WriteShutdownRegistry();
	std::string GetShutdownRegistryText();
	void UpdateUserDataBackup(bool force);
	void PageGoingAway();
	// Test hook: queues a command as if it were a -script line, run by RunScript after the commands
	// already queued for that update. A test bot uses it, so its input takes exactly the script's path.
	void AddScriptCommand(int theUpdateCount, const std::string& theCommand, const std::string& theArgs);
	// A live command line (-commands, the web's byopop_test_command), queued for the next update: false for a blank
	// line, a comment or a full queue
	bool QueueLiveCommand(const std::string& theLine, int theLineNum);

	// --- port: the mod menu and restarts (docs/MOD_MENU_DESIGN.md 5, 6) ---
	// The menu is a base modal widget above everything (not a dialog: it is not in mDialogMap). It pauses the
	// game through ModalOpen()/ModalClose() exactly as a modal dialog does. F2 (no modifiers, no repeat) toggles
	// it with real input, never in a -script run; scripts use "modmenu ..." (RunScript).
	void OpenModMenu();
	void CloseModMenu();
	bool IsModMenuOpen() const { return mModMenu != NULL; }

	// Restarts the game: the app shuts down as on Quit (the game's own shutdown save) and starts again. Natively
	// main() starts the exe again (TakeRelaunch, Platform::Relaunch); on the web the page reloads. Returns
	// RESTART_BLOCKED when CanRestartNow says no, RESTART_CONFIRM when a game is in progress and isConfirmed is
	// false (theMessage: the question), else RESTART_STARTED.
	int RequestRestart(bool isConfirmed, SexyString* theMessage);
	// port: quits as the game's own Quit does (the page's Close on a game's window: byopop_quit). The default shuts
	// down at once; a game whose Quit says goodbye first overrides it to do the same and shut down after. It must
	// not block: on the web the sound plays only once this returns, so such a game defers its Shutdown() to an update.
	virtual void QuitAsGame();
	// main(), after Shutdown() and before deleting the app: true when the app should start again with theArgs
	// (logs "restart: <args>"). A test run (rundesk's BYOPOP_TEST_RUN, the oracle's BEJ_TEST_DESKTOP, -script or a
	// test bot) does not start again unless BYOPOP_RELAUNCH_TEST_ARGS is set: then its arguments are added.
	bool TakeRelaunch(std::vector<std::string>* theArgs);
	// Hooks, each with a default: whether a restart is possible now (default yes), whether a game is in progress
	// (the restart then asks first; default no), the arguments a restart keeps (default: -gamedir, -userdir,
	// -modsdir, -changedir, -renderscale as given; a game adds its own), and the game's messages per mod.
	virtual bool CanRestartNow(SexyString* theReason);
	virtual bool HasGameInProgress();
	virtual void GetRestartArgs(std::vector<std::string>* theArgs);
	virtual void GetModMessages(std::vector<ModMessage>* theMessages);
	// True in a test run (rundesk, the oracle, a -script run or a test bot): a restart does not start the game
	// again (TakeRelaunch), and MsgBox and Popup only log their message, since nobody could answer them
	bool IsTestRun();
	// port: moves the mouse pointer to a point of the game (the originals' SetCursorPos). It does nothing in a test run
	// (IsTestRun: a hidden desktop's warp could move the user's real pointer) and on the web (a page cannot move it).
	void WarpMouse(int theX, int theY);
};

extern SexyAppBase* gSexyAppBase;

}

#endif
