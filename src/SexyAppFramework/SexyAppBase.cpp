// Portable SexyAppBase (SDL2 / Emscripten), ported from SexyAppFramework 1.3's SexyAppBase.cpp.
//
// The platform-neutral logic (update timing, dialogs, properties, image helpers, volumes, registry
// semantics) follows 1.3. The Win32/DirectX parts are replaced by SDL: WindowProc and the deferred
// message queue become ProcessEvents()/HandleEvent(), the registry is emulated in a text file in the
// user dir, files go through Platform::VFS. Demo recording/playback, the screensaver mode, SEH crash
// reporting, the cursor thread and the debug overlays (FPS, memory dump, perf) are not ported.

#include "SexyAppBase.h"
#include "WidgetManager.h"
#include "Widget.h"
#include "KeyCodes.h"
#include "DDInterface.h"
#include "DDImage.h"
#include "MemoryImage.h"
#include "Dialog.h"
#include "Graphics.h"
#include "PropertiesParser.h"
#include "ResourceManager.h"
#include "MTRand.h"
#include "SDLSoundManager.h"
#include "SDLMusicInterface.h"
#include "SoundInstance.h"
#include "../platform/VFS.h"
#include "../platform/ImageLoader.h"
#include "../platform/ModManager.h"
#include "../platform/WebPersist.h"

#include <SDL.h>
#include <stb_image_write.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

using namespace Sexy;

SexyAppBase* Sexy::gSexyAppBase = NULL;

static SDL_Cursor* gSystemCursors[SDL_NUM_SYSTEM_CURSORS];

//////////////////////////////////////////////////////////////////////////
// Helpers

// Game strings are narrow Windows-1252/Latin-1 text; SDL and the browser use UTF-8.
static std::string Latin1ToUTF8(const std::string& theString)
{
	std::string aResult;
	for (size_t i = 0; i < theString.length(); i++)
	{
		uchar c = (uchar) theString[i];
		if (c < 0x80)
			aResult += (char) c;
		else
		{
			aResult += (char) (0xC0 | (c >> 6));
			aResult += (char) (0x80 | (c & 0x3F));
		}
	}
	return aResult;
}

// Keeps the code points that fit in a narrow SexyChar (Latin-1) and drops the rest.
static std::string UTF8ToLatin1(const char* theString)
{
	std::string aResult;
	const uchar* p = (const uchar*) theString;
	while (*p != 0)
	{
		uint aCodePoint;
		int aLength;
		if (*p < 0x80)
		{
			aCodePoint = *p;
			aLength = 1;
		}
		else if ((*p & 0xE0) == 0xC0)
		{
			aCodePoint = *p & 0x1F;
			aLength = 2;
		}
		else if ((*p & 0xF0) == 0xE0)
		{
			aCodePoint = *p & 0x0F;
			aLength = 3;
		}
		else if ((*p & 0xF8) == 0xF0)
		{
			aCodePoint = *p & 0x07;
			aLength = 4;
		}
		else
		{
			p++;
			continue;
		}

		int i = 1;
		for (; (i < aLength) && ((p[i] & 0xC0) == 0x80); i++)
			aCodePoint = (aCodePoint << 6) | (p[i] & 0x3F);
		p += i;

		if ((i == aLength) && (aCodePoint < 0x100))
			aResult += (char) aCodePoint;
	}
	return aResult;
}

// KeyCodes.h uses Windows virtual key codes.
static int SDLKeyToKeyCode(SDL_Keycode theKey)
{
	if ((theKey >= SDLK_a) && (theKey <= SDLK_z))
		return 'A' + (theKey - SDLK_a);
	if ((theKey >= SDLK_0) && (theKey <= SDLK_9))
		return '0' + (theKey - SDLK_0);
	if ((theKey >= SDLK_F1) && (theKey <= SDLK_F12))
		return KEYCODE_F1 + (theKey - SDLK_F1);
	if ((theKey >= SDLK_KP_1) && (theKey <= SDLK_KP_9))
		return KEYCODE_NUMPAD1 + (theKey - SDLK_KP_1);

	switch (theKey)
	{
	case SDLK_BACKSPACE:	return KEYCODE_BACK;
	case SDLK_TAB:			return KEYCODE_TAB;
	case SDLK_RETURN:
	case SDLK_KP_ENTER:		return KEYCODE_RETURN;
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:		return KEYCODE_SHIFT;
	case SDLK_LCTRL:
	case SDLK_RCTRL:		return KEYCODE_CONTROL;
	case SDLK_LALT:
	case SDLK_RALT:			return KEYCODE_MENU;
	case SDLK_PAUSE:		return KEYCODE_PAUSE;
	case SDLK_CAPSLOCK:		return KEYCODE_CAPITAL;
	case SDLK_ESCAPE:		return KEYCODE_ESCAPE;
	case SDLK_SPACE:		return KEYCODE_SPACE;
	case SDLK_PAGEUP:		return KEYCODE_PRIOR;
	case SDLK_PAGEDOWN:		return KEYCODE_NEXT;
	case SDLK_END:			return KEYCODE_END;
	case SDLK_HOME:			return KEYCODE_HOME;
	case SDLK_LEFT:			return KEYCODE_LEFT;
	case SDLK_UP:			return KEYCODE_UP;
	case SDLK_RIGHT:		return KEYCODE_RIGHT;
	case SDLK_DOWN:			return KEYCODE_DOWN;
	case SDLK_PRINTSCREEN:	return KEYCODE_SNAPSHOT;
	case SDLK_INSERT:		return KEYCODE_INSERT;
	case SDLK_DELETE:		return KEYCODE_DELETE;
	case SDLK_LGUI:			return KEYCODE_LWIN;
	case SDLK_RGUI:			return KEYCODE_RWIN;
	case SDLK_APPLICATION:	return KEYCODE_APPS;
	case SDLK_KP_0:			return KEYCODE_NUMPAD0;
	case SDLK_KP_MULTIPLY:	return KEYCODE_MULTIPLY;
	case SDLK_KP_PLUS:		return KEYCODE_ADD;
	case SDLK_KP_MINUS:		return KEYCODE_SUBTRACT;
	case SDLK_KP_PERIOD:	return KEYCODE_DECIMAL;
	case SDLK_KP_DIVIDE:	return KEYCODE_DIVIDE;
	case SDLK_NUMLOCKCLEAR:	return KEYCODE_NUMLOCK;
	case SDLK_SCROLLLOCK:	return KEYCODE_SCROLL;
	// VK_OEM_* (US layout positions)
	case SDLK_SEMICOLON:	return 0xBA;
	case SDLK_EQUALS:		return 0xBB;
	case SDLK_COMMA:		return 0xBC;
	case SDLK_MINUS:		return 0xBD;
	case SDLK_PERIOD:		return 0xBE;
	case SDLK_SLASH:		return 0xBF;
	case SDLK_BACKQUOTE:	return 0xC0;
	case SDLK_LEFTBRACKET:	return 0xDB;
	case SDLK_BACKSLASH:	return 0xDC;
	case SDLK_RIGHTBRACKET:	return 0xDD;
	case SDLK_QUOTE:		return 0xDE;
	}

	return KEYCODE_UNKNOWN;
}

// The control characters Windows' TranslateMessage delivers as WM_CHAR (EditWidget uses Ctrl+C/V/X/Z,
// dialogs may look for '\r'). SDL only reports printable text.
static int GetControlChar(const SDL_Keysym& theKeysym)
{
	if (theKeysym.mod & KMOD_ALT)
		return 0; // WM_SYSCHAR, never seen by the game

	bool ctrlDown = (theKeysym.mod & KMOD_CTRL) != 0;
	switch (theKeysym.sym)
	{
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		return ctrlDown ? '\n' : '\r';
	case SDLK_BACKSPACE:
		return ctrlDown ? 0x7F : '\b';
	case SDLK_TAB:
		return '\t';
	case SDLK_ESCAPE:
		return 0x1B;
	}

	if ((ctrlDown) && (theKeysym.sym >= SDLK_a) && (theKeysym.sym <= SDLK_z))
		return 1 + (theKeysym.sym - SDLK_a);

	return 0;
}

// WM_xBUTTONDOWN / WM_xBUTTONDBLCLK click codes: 1 left, -1 right, 3 middle, 2 / -2 double-click. Like
// Windows, every second click of a rapid series is the double-click.
static int GetClickCode(const SDL_MouseButtonEvent& theEvent, bool isDown)
{
	bool isDoubleClick = (isDown) && (theEvent.clicks >= 2) && ((theEvent.clicks % 2) == 0);

	switch (theEvent.button)
	{
	case SDL_BUTTON_LEFT:	return isDoubleClick ? 2 : 1;
	case SDL_BUTTON_RIGHT:	return isDoubleClick ? -2 : -1;
	case SDL_BUTTON_MIDDLE:	return 3;
	}

	return 0;
}

static void SetSystemCursor(int theCursorNum)
{
	SDL_SystemCursor anId;
	switch (theCursorNum)
	{
	case CURSOR_NONE:
	case CURSOR_CUSTOM:
		SDL_ShowCursor(SDL_DISABLE);
		return;
	case CURSOR_HAND:
	case CURSOR_DRAGGING:		anId = SDL_SYSTEM_CURSOR_HAND; break;
	case CURSOR_TEXT:			anId = SDL_SYSTEM_CURSOR_IBEAM; break;
	case CURSOR_CIRCLE_SLASH:	anId = SDL_SYSTEM_CURSOR_NO; break;
	case CURSOR_SIZEALL:		anId = SDL_SYSTEM_CURSOR_SIZEALL; break;
	case CURSOR_SIZENESW:		anId = SDL_SYSTEM_CURSOR_SIZENESW; break;
	case CURSOR_SIZENS:			anId = SDL_SYSTEM_CURSOR_SIZENS; break;
	case CURSOR_SIZENWSE:		anId = SDL_SYSTEM_CURSOR_SIZENWSE; break;
	case CURSOR_SIZEWE:			anId = SDL_SYSTEM_CURSOR_SIZEWE; break;
	case CURSOR_WAIT:			anId = SDL_SYSTEM_CURSOR_WAIT; break;
	default:					anId = SDL_SYSTEM_CURSOR_ARROW; break;
	}

	if (gSystemCursors[anId] == NULL)
		gSystemCursors[anId] = SDL_CreateSystemCursor(anId);
	if ((gSystemCursors[anId] != NULL) && (SDL_GetCursor() != gSystemCursors[anId]))
		SDL_SetCursor(gSystemCursors[anId]);
	SDL_ShowCursor(SDL_ENABLE);
}

static bool IsAbsolutePath(const std::string& thePath)
{
	return ((!thePath.empty()) && ((thePath[0] == '/') || (thePath[0] == '\\'))) ||
		((thePath.length() > 1) && (thePath[1] == ':'));
}

// Where a game file lives in the writable user dir, as a VFS path: "userdata\x" stays as it is (the VFS
// maps it to <user dir>/x), any other relative path becomes "userdata/<path>". This is also where
// VFS::Open() puts writes of non-userdata paths, so files the game writes can be read back.
static std::string GetUserDataPath(const std::string& theFileName)
{
	if (IsAbsolutePath(theFileName))
		return theFileName;

	std::string aPath = Platform::VFS::Normalize(theFileName);
	if (sexy_strnicmp(aPath.c_str(), "userdata/", 9) == 0)
		return aPath;
	return "userdata/" + aPath;
}

static std::string EscapeRegistryString(const std::string& theString)
{
	std::string aResult;
	for (size_t i = 0; i < theString.length(); i++)
	{
		char c = theString[i];
		if (c == '\\')
			aResult += "\\\\";
		else if (c == '\n')
			aResult += "\\n";
		else if (c == '\r')
			aResult += "\\r";
		else
			aResult += c;
	}
	return aResult;
}

static std::string UnescapeRegistryString(const std::string& theString)
{
	std::string aResult;
	for (size_t i = 0; i < theString.length(); i++)
	{
		char c = theString[i];
		if ((c == '\\') && (i + 1 < theString.length()))
		{
			c = theString[++i];
			if (c == 'n')
				c = '\n';
			else if (c == 'r')
				c = '\r';
		}
		aResult += c;
	}
	return aResult;
}

#ifdef __EMSCRIPTEN__

EM_JS_DEPS(sexyappbase, "$UTF8ToString");

// A browser tab can't close itself, so after the game shuts down the page decides what to show
// (Module.onGameExit, set by web/shell.html). Saves are written by then; the page waits for
// Module.sexySyncBusy/sexySyncPending to clear before offering a reload.
EM_JS(void, sexyapp_notify_exit, (), {
	if (typeof Module['onGameExit'] === 'function')
		Module['onGameExit']();
});

EM_JS(void, sexyapp_alert, (const char* theText), {
	alert(UTF8ToString(theText));
});

EM_JS(int, sexyapp_confirm, (const char* theText), {
	return confirm(UTF8ToString(theText)) ? 1 : 0;
});

// The browser may close, reload or discard the page without giving control back, while 1.3 only
// saves on shutdown. When the page is hidden or about to unload (SDL reports beforeunload as
// SDL_APP_TERMINATING), PageGoingAway() writes the settings and brings the backup up to date. Event
// watches run synchronously inside the browser's event handler, between frames. Shutdown() hides the
// window itself; by then mShutdown is set and the shutdown saves are written.
static int SDLCALL EmscriptenEventWatch(void* theArg, SDL_Event* theEvent)
{
	SexyAppBase* anApp = (SexyAppBase*) theArg;

	bool goingAway = (theEvent->type == SDL_APP_TERMINATING) ||
		((theEvent->type == SDL_WINDOWEVENT) && (theEvent->window.event == SDL_WINDOWEVENT_HIDDEN));
	if ((goingAway) && (!anApp->mShutdown))
		anApp->PageGoingAway();

	return 0;
}

// Browser window focus doesn't pair up reliably: an embedded page (an iframe, an app's web view) can
// be blurred and then clicked without getting a focus event. SDL also passes on a blur only when its
// own focus flag changes, so once a focus event is missed it reports no further blurs. 1.3 treats an
// unfocused app as in the background and ignores clicks, so the game stopped responding. This
// replaces SDL's handlers and passes every focus and blur on as it comes; HandleEvent also takes a
// press or key in the page as focus.
static EM_BOOL EmscriptenFocusCallback(int theEventType, const EmscriptenFocusEvent* theFocusEvent, void* theArg)
{
	SexyAppBase* anApp = (SexyAppBase*) theArg;

	SDL_Event anEvent;
	SDL_zero(anEvent);
	anEvent.type = SDL_WINDOWEVENT;
	anEvent.window.windowID = SDL_GetWindowID(anApp->mHWnd);
	anEvent.window.event = (theEventType == EMSCRIPTEN_EVENT_FOCUS) ? SDL_WINDOWEVENT_FOCUS_GAINED : SDL_WINDOWEVENT_FOCUS_LOST;
	SDL_PushEvent(&anEvent);
	return EM_FALSE;
}

#endif

//////////////////////////////////////////////////////////////////////////

SexyAppBase::SexyAppBase()
{
	gSexyAppBase = this;

	mOnlyAllowOneCopyToRun = true;
	mNoDefer = false;
	mFullScreenPageFlip = true;
	mTimeLoaded = GetTickCount();
	mSEHOccured = false;
	mCompanyName = "PopCap";
	mProdName = "Product";
	mTitle = _S("SexyApp");
	mShutdown = false;
	mExitToTop = false;
	mWidth = 640;
	mHeight = 480;
	mFullscreenBits = 16;
	mIsWindowed = true;
	mIsPhysWindowed = true;
	mFullScreenWindow = false;
	mPreferredX = -1;
	mPreferredY = -1;
	mIsScreenSaver = false;
	mAllowMonitorPowersave = true;
	mHWnd = NULL;
	mDDInterface = NULL;
	mMusicInterface = NULL;
	mFrameTime = 10;
	mNonDrawCount = 0;
	mDrawCount = 0;
	mSleepCount = 0;
	mUpdateCount = 0;
	mUpdateAppState = 0;
	mUpdateAppDepth = 0;
	mPendingUpdatesAcc = 0.0;
	mUpdateFTimeAcc = 0.0;
	mHasPendingDraw = true;
	mIsDrawing = false;
	mLastDrawWasEmpty = false;
	mLastTimeCheck = 0;
	mLastTime = 0;
	mLastUserInputTick = 0;
	mUpdateMultiplier = 1;
	mPaused = false;
	mFastForwardToUpdateNum = 0;
	mFastForwardToMarker = false;
	mFastForwardStep = false;
	mSoundManager = NULL;
	mCursorNum = CURSOR_POINTER;
	mMouseIn = false;
	mRunning = false;
	mActive = true;
	mProcessInTimer = false;
	mMinimized = false;
	mPhysMinimized = false;
	mIsDisabled = false;
	mLoaded = false;
	mYieldMainThread = false;
	mLoadingFailed = false;
	mLoadingThreadStarted = false;
	mAutoStartLoadingThread = true;
	mLoadingThreadCompleted = false;
	mNumLoadingThreadTasks = 0;
	mCompletedLoadingThreadTasks = 0;
	mLastDrawTick = timeGetTime();
	mNextDrawTick = timeGetTime();
	mSysCursor = true;
	mForceFullscreen = false;
	mForceWindowed = false;
	mHasFocus = true;
	mCustomCursorsEnabled = false;
	mCustomCursorDirty = false;
	mIsOpeningURL = false;
	mShutdownOnURLOpen = false;
	mOpeningURLTime = 0;
	mLastTimerTime = 0;
	mLastBigDelayTime = 0;
	mInitialized = false;
	mLastShutdownWasGraceful = true;
	mReadFromRegistry = false;
	mCmdLineParsed = false;
	mSkipSignatureChecks = false;
	mCtrlDown = false;
	mAltDown = false;
	mStepMode = 0;
	mCleanupSharedImages = false;
	mStandardWordWrap = true;
	mbAllowExtendedChars = true;
	mEnableMaximizeButton = false;
	mMouseSensitivity = 1.0f;
	mTabletPC = false;
	mIsWideWindow = false;

	mMusicVolume = 0.85;
	mSfxVolume = 0.85;
	mDemoMusicVolume = mDemoSfxVolume = 0.0;
	mUnmutedMusicVolume = mMusicVolume;
	mUnmutedSfxVolume = mSfxVolume;
	mMuteCount = 0;
	mAutoMuteCount = 0;
	mDemoMute = false;
	mMuteOnLostFocus = true;
	mFPSTime = 0;
	mFPSStartTick = GetTickCount();
	mFPSFlipCount = 0;
	mFPSCount = 0;
	mFPSDirtyCount = 0;
	mShowFPS = false;
	mShowFPSMode = 0;
	mDrawTime = 0;
	mScreenBltTime = 0;
	mAlphaDisabled = false;
	mDebugKeysEnabled = false;
	mNoSoundNeeded = false;
	mWantFMod = false;

	mSyncRefreshRate = 100;
	mVSyncUpdates = false;
	mWaitForVSync = false;
	mUserChanged3DSetting = false;
	mAutoEnable3D = false;
	mTest3D = false;
	mRelaxUpdateBacklogCount = 0;
	mWidescreenAware = false;
	mScreenBounds = Rect(0, 0, mWidth, mHeight);
	mEnableWindowAspect = false;
	mWindowAspect.Set(4, 3);

	mPlayingDemoBuffer = false;
	mManualShutdown = false;
	mRecordingDemoBuffer = false;

	mRandSeed = 0;
	mRegistryLoaded = false;
	mRegistryDirty = false;
	mLastMouseX = 0;
	mLastMouseY = 0;
	mLoadingThreadRunning = false;
	mNothingDue = false;
	mBackupDirty = true;
	mLastBackupCheck = 0;
	mLastBackupTime = 0;
	mBackupHasSave = false;
	mRegistryCapture = false;
	mHasSeedParam = false;
	mFastMode = false;
	mForcedRenderScale = 0;
	mTraceMode = false;
	mFastClock = 0;
	mLastFastDrawUpdate = 0;
	mScriptPos = 0;

	int i;

	for (i = 0; i < NUM_CURSORS; i++)
		mCursorImages[i] = NULL;

	for (i = 0; i < 256; i++)
		mAdd8BitMaxTable[i] = i;

	for (i = 256; i < 512; i++)
		mAdd8BitMaxTable[i] = 255;

	// Set default strings.  Init could read in overrides from partner.xml
	SetString("DIALOG_BUTTON_OK",		L"OK");
	SetString("DIALOG_BUTTON_CANCEL",	L"CANCEL");

	SetString("UPDATE_CHECK_TITLE",		L"Update Check");
	SetString("UPDATE_CHECK_BODY",		L"Checking if there are any updates available for this product ...");

	SetString("UP_TO_DATE_TITLE",		L"Up to Date");
	SetString("UP_TO_DATE_BODY",		L"There are no updates available for this product at this time.");
	SetString("NEW_VERSION_TITLE",		L"New Version");
	SetString("NEW_VERSION_BODY",		L"There is an update available for this product.  Would you like to visit the web site to download it?");

	mWidgetManager = new WidgetManager(this);
	mResourceManager = new ResourceManager(this);
}

SexyAppBase::~SexyAppBase()
{
	Shutdown();

	DialogMap::iterator aDialogItr = mDialogMap.begin();
	while (aDialogItr != mDialogMap.end())
	{
		mWidgetManager->RemoveWidget(aDialogItr->second);
		delete aDialogItr->second;
		++aDialogItr;
	}
	mDialogMap.clear();
	mDialogList.clear();

	delete mWidgetManager;
	mWidgetManager = NULL;
	delete mResourceManager;
	mResourceManager = NULL;

	SharedImageMap::iterator aSharedImageItr = mSharedImageMap.begin();
	while (aSharedImageItr != mSharedImageMap.end())
	{
		SharedImage* aSharedImage = &aSharedImageItr->second;
		delete aSharedImage->mImage;
		mSharedImageMap.erase(aSharedImageItr++);
	}

	delete mDDInterface; // also deletes the screen image
	mDDInterface = NULL;
	delete mMusicInterface;
	mMusicInterface = NULL;
	delete mSoundManager;
	mSoundManager = NULL;

	for (int i = 0; i < SDL_NUM_SYSTEM_CURSORS; i++)
	{
		if (gSystemCursors[i] != NULL)
		{
			SDL_FreeCursor(gSystemCursors[i]);
			gSystemCursors[i] = NULL;
		}
	}

	if (mHWnd != NULL)
	{
		SDL_Window* aWindow = mHWnd;
		mHWnd = NULL;
		SDL_DestroyWindow(aWindow);
	}

	gSexyAppBase = NULL;
}

void SexyAppBase::ClearUpdateBacklog(bool relaxForASecond)
{
	mLastTimeCheck = GetClockTime();
	mUpdateFTimeAcc = 0.0;

	if (relaxForASecond)
		mRelaxUpdateBacklogCount = 1000;
}

bool SexyAppBase::IsScreenSaver()
{
	return mIsScreenSaver;
}

bool SexyAppBase::AppCanRestore()
{
	return !mIsDisabled;
}

//////////////////////////////////////////////////////////////////////////
// Dialogs

Dialog* SexyAppBase::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = new Dialog(NULL, NULL, theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	return aDialog;
}

Dialog* SexyAppBase::DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	KillDialog(theDialogId);

	Dialog* aDialog = NewDialog(theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);

	AddDialog(theDialogId, aDialog);

	return aDialog;
}

Dialog* SexyAppBase::GetDialog(int theDialogId)
{
	DialogMap::iterator anItr = mDialogMap.find(theDialogId);

	if (anItr != mDialogMap.end())
		return anItr->second;

	return NULL;
}

bool SexyAppBase::KillDialog(int theDialogId, bool removeWidget, bool deleteWidget)
{
	DialogMap::iterator anItr = mDialogMap.find(theDialogId);

	if (anItr != mDialogMap.end())
	{
		Dialog* aDialog = anItr->second;

		// set the result to something else so DoMainLoop knows that the dialog is gone
		// in case nobody else sets mResult
		if (aDialog->mResult == -1)
			aDialog->mResult = 0;

		DialogList::iterator aListItr = std::find(mDialogList.begin(), mDialogList.end(), aDialog);
		if (aListItr != mDialogList.end())
			mDialogList.erase(aListItr);

		mDialogMap.erase(anItr);

		if (removeWidget || deleteWidget)
			mWidgetManager->RemoveWidget(aDialog);

		if (aDialog->IsModal())
		{
			ModalClose();
			mWidgetManager->RemoveBaseModal(aDialog);
		}

		if (deleteWidget)
			SafeDeleteWidget(aDialog);

		return true;
	}

	return false;
}

bool SexyAppBase::KillDialog(int theDialogId)
{
	return KillDialog(theDialogId, true, true);
}

bool SexyAppBase::KillDialog(Dialog* theDialog)
{
	return KillDialog(theDialog->mId);
}

int SexyAppBase::GetDialogCount()
{
	return (int) mDialogMap.size();
}

void SexyAppBase::AddDialog(int theDialogId, Dialog* theDialog)
{
	KillDialog(theDialogId);

	if (theDialog->mWidth == 0)
	{
		// Set the dialog position ourselves
		int aWidth = mWidth/2;
		theDialog->Resize((mWidth - aWidth)/2, mHeight / 5, aWidth, theDialog->GetPreferredHeight(aWidth));
	}

	mDialogMap.insert(DialogMap::value_type(theDialogId, theDialog));
	mDialogList.push_back(theDialog);

	mWidgetManager->AddWidget(theDialog);
	if (theDialog->IsModal())
	{
		mWidgetManager->AddBaseModal(theDialog);
		ModalOpen();
	}
}

void SexyAppBase::AddDialog(Dialog* theDialog)
{
	AddDialog(theDialog->mId, theDialog);
}

void SexyAppBase::ModalOpen()
{
}

void SexyAppBase::ModalClose()
{
}

void SexyAppBase::DialogButtonPress(int theDialogId, int theButtonId)
{
	if (theButtonId == Dialog::ID_YES)
		ButtonPress(2000 + theDialogId);
	else if (theButtonId == Dialog::ID_NO)
		ButtonPress(3000 + theDialogId);
}

void SexyAppBase::DialogButtonDepress(int theDialogId, int theButtonId)
{
	if (theButtonId == Dialog::ID_YES)
		ButtonDepress(2000 + theDialogId);
	else if (theButtonId == Dialog::ID_NO)
		ButtonDepress(3000 + theDialogId);
}

void SexyAppBase::GotFocus()
{
}

void SexyAppBase::LostFocus()
{
}

//////////////////////////////////////////////////////////////////////////
// URLs

void SexyAppBase::URLOpenFailed(const std::string& theURL)
{
	mIsOpeningURL = false;
}

void SexyAppBase::URLOpenSucceeded(const std::string& theURL)
{
	mIsOpeningURL = false;

	if (mShutdownOnURLOpen)
		Shutdown();
}

bool SexyAppBase::OpenURL(const std::string& theURL, bool shutdownOnOpen)
{
	if ((!mIsOpeningURL) || (theURL != mOpeningURL))
	{
		mShutdownOnURLOpen = shutdownOnOpen;
		mIsOpeningURL = true;
		mOpeningURL = theURL;
		mOpeningURLTime = GetTickCount();

		// port: SDL_OpenURL hands the URL to the system browser (window.open on the web). 1.3 waited for
		// the browser to take the focus before reporting success; SDL can't see that, so success is
		// reported as soon as the URL was handed over.
		if (SDL_OpenURL(theURL.c_str()) == 0)
		{
			URLOpenSucceeded(theURL);
			return true;
		}
		else
		{
			URLOpenFailed(theURL);
			return false;
		}
	}

	return true;
}

std::string SexyAppBase::GetProductVersion(const std::string& thePath)
{
	// port: there are no version resources to read
	return "";
}

void SexyAppBase::SetCursorImage(int theCursorNum, Image* theImage)
{
	if ((theCursorNum >= 0) && (theCursorNum < NUM_CURSORS))
	{
		mCursorImages[theCursorNum] = theImage;
		EnforceCursor();
	}
}

void SexyAppBase::TakeScreenshot()
{
	// port: not supported
}

double SexyAppBase::GetLoadingThreadProgress()
{
	if (mLoaded)
		return 1.0;
	if (!mLoadingThreadStarted)
		return 0.0;
	if (mNumLoadingThreadTasks == 0)
		return 0.0;
	return std::min(mCompletedLoadingThreadTasks / (double) mNumLoadingThreadTasks, 1.0);
}

//////////////////////////////////////////////////////////////////////////
// Registry emulation
//
// Values keep their registry type: REG_SZ as "sz:text", REG_DWORD (also used for booleans) as
// "dword:<decimal>", REG_BINARY as "hex:<bytes>". Typed reads fail on a type mismatch like 1.3's.
// The file is an INI with one section per key:
//
//   [SOFTWARE\PopCap\Bejeweled]
//   MusicVolume=dword:85
//   LastScoreName=sz:Player

std::string SexyAppBase::RegistryPath()
{
	return "userdata/registry.ini";
}

std::string SexyAppBase::RegistryValuePath(const std::string& theValueName)
{
	return RemoveTrailingSlash("SOFTWARE\\" + mRegKey) + "\\" + theValueName;
}

void SexyAppBase::LoadRegistry()
{
	if (mRegistryLoaded)
		return;
	mRegistryLoaded = true;

	std::vector<uint8_t> aData;
	if (!Platform::VFS::Get().ReadAll(RegistryPath(), aData))
		return;

	std::istringstream aStream(std::string(aData.begin(), aData.end()));
	std::string aKey;
	std::string aLine;
	while (std::getline(aStream, aLine))
	{
		if ((!aLine.empty()) && (aLine[aLine.length() - 1] == '\r'))
			aLine.resize(aLine.length() - 1);
		if ((aLine.empty()) || (aLine[0] == ';'))
			continue;

		if (aLine[0] == '[')
		{
			size_t aClosePos = aLine.rfind(']');
			aKey = aLine.substr(1, (aClosePos == std::string::npos) ? std::string::npos : aClosePos - 1);
			continue;
		}

		size_t anEqualsPos = aLine.find('=');
		if ((anEqualsPos != std::string::npos) && (!aKey.empty()))
			mRegistry[aKey + "\\" + aLine.substr(0, anEqualsPos)] = aLine.substr(anEqualsPos + 1);
	}
}

std::string SexyAppBase::GetRegistryText()
{
	// Group the values by key
	typedef std::map<std::string, std::string, StringLessNoCase> KeyMap;
	KeyMap aKeys;
	for (RegistryValueMap::const_iterator anItr = mRegistry.begin(); anItr != mRegistry.end(); ++anItr)
	{
		size_t aSlashPos = anItr->first.rfind('\\');
		if (aSlashPos == std::string::npos)
			continue;
		aKeys[anItr->first.substr(0, aSlashPos)] += anItr->first.substr(aSlashPos + 1) + "=" + anItr->second + "\n";
	}

	std::string aText = "; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n";
	for (KeyMap::const_iterator anItr = aKeys.begin(); anItr != aKeys.end(); ++anItr)
		aText += "\n[" + anItr->first + "]\n" + anItr->second;
	return aText;
}

bool SexyAppBase::SaveRegistry()
{
	std::string aText = GetRegistryText();
	FILE* aFP = Platform::VFS::Get().Open(RegistryPath(), "wb");
	if (aFP == NULL)
		return false;
	bool success = fwrite(aText.data(), 1, aText.length(), aFP) == aText.length();
	success = (fclose(aFP) == 0) && (success);

	if (success)
		mRegistryDirty = false;
	return success;
}

bool SexyAppBase::RegistryWrite(const std::string& theValueName, const std::string& theValue)
{
	if (mRegKey.length() == 0)
		return false;

	LoadRegistry();
	mRegistry[RegistryValuePath(theValueName)] = theValue;
	if (mRegistryCapture)
		return true;
	mRegistryDirty = true;

	bool success = SaveRegistry();
	SyncUserData();
	return success;
}

bool SexyAppBase::RegistryRead(const std::string& theValueName, std::string* theValue)
{
	if (mRegKey.length() == 0)
		return false;

	LoadRegistry();
	RegistryValueMap::const_iterator anItr = mRegistry.find(RegistryValuePath(theValueName));
	if (anItr == mRegistry.end())
		return false;

	*theValue = anItr->second;
	return true;
}

bool SexyAppBase::RegistryWriteString(const std::string& theValueName, const std::string& theString)
{
	return RegistryWrite(theValueName, "sz:" + EscapeRegistryString(theString));
}

bool SexyAppBase::RegistryWriteInteger(const std::string& theValueName, int theValue)
{
	return RegistryWrite(theValueName, StrFormat("dword:%d", theValue));
}

bool SexyAppBase::RegistryWriteBoolean(const std::string& theValueName, bool theValue)
{
	int aValue = theValue ? 1 : 0;
	return RegistryWriteInteger(theValueName, aValue);
}

bool SexyAppBase::RegistryWriteData(const std::string& theValueName, const uchar* theValue, ulong theLength)
{
	static const char aHexChars[] = "0123456789abcdef";

	std::string aValue = "hex:";
	for (ulong i = 0; i < theLength; i++)
	{
		aValue += aHexChars[theValue[i] >> 4];
		aValue += aHexChars[theValue[i] & 0x0F];
	}
	return RegistryWrite(theValueName, aValue);
}

void SexyAppBase::WriteToRegistry()
{
	RegistryWriteInteger("MusicVolume", (int) (mMusicVolume * 100));
	RegistryWriteInteger("SfxVolume", (int) (mSfxVolume * 100));
	RegistryWriteInteger("Muted", (mMuteCount - mAutoMuteCount > 0) ? 1 : 0);
	RegistryWriteInteger("ScreenMode", mIsWindowed ? 0 : 1);
	RegistryWriteInteger("PreferredX", mPreferredX);
	RegistryWriteInteger("PreferredY", mPreferredY);
	RegistryWriteInteger("CustomCursors", mCustomCursorsEnabled ? 1 : 0);
	RegistryWriteInteger("InProgress", 0);
	RegistryWriteBoolean("WaitForVSync", mWaitForVSync);
}

bool SexyAppBase::RegistryEraseKey(const SexyString& _theKeyName)
{
	std::string theKeyName = SexyStringToStringFast(_theKeyName);
	if (mRegKey.length() == 0)
		return false;

	LoadRegistry();

	std::string aPrefix = RegistryValuePath(theKeyName) + "\\";
	bool found = false;
	RegistryValueMap::iterator anItr = mRegistry.begin();
	while (anItr != mRegistry.end())
	{
		if (sexy_strnicmp(anItr->first.c_str(), aPrefix.c_str(), aPrefix.length()) == 0)
		{
			mRegistry.erase(anItr++);
			found = true;
		}
		else
			++anItr;
	}

	if (found)
	{
		mRegistryDirty = true;
		SaveRegistry();
		SyncUserData();
	}
	return found;
}

void SexyAppBase::RegistryEraseValue(const SexyString& _theValueName)
{
	std::string theValueName = SexyStringToStringFast(_theValueName);
	if (mRegKey.length() == 0)
		return;

	LoadRegistry();
	if (mRegistry.erase(RegistryValuePath(theValueName)) > 0)
	{
		mRegistryDirty = true;
		SaveRegistry();
		SyncUserData();
	}
}

bool SexyAppBase::RegistryGetSubKeys(const std::string& theKeyName, StringVector* theSubKeys)
{
	theSubKeys->clear();

	if (mRegKey.length() == 0)
		return false;

	LoadRegistry();

	// A key exists while something is stored below it
	std::string aPrefix = RemoveTrailingSlash(RegistryValuePath(theKeyName)) + "\\";
	bool found = false;
	for (RegistryValueMap::const_iterator anItr = mRegistry.begin(); anItr != mRegistry.end(); ++anItr)
	{
		if (sexy_strnicmp(anItr->first.c_str(), aPrefix.c_str(), aPrefix.length()) != 0)
			continue;
		found = true;

		std::string aRest = anItr->first.substr(aPrefix.length());
		size_t aSlashPos = aRest.find('\\');
		if (aSlashPos == std::string::npos)
			continue;

		std::string aSubKey = aRest.substr(0, aSlashPos);
		bool isNew = true;
		for (int i = 0; i < (int) theSubKeys->size(); i++)
			if (sexy_stricmp((*theSubKeys)[i].c_str(), aSubKey.c_str()) == 0)
				isNew = false;
		if (isNew)
			theSubKeys->push_back(aSubKey);
	}

	return found;
}

bool SexyAppBase::RegistryReadString(const std::string& theKey, std::string* theString)
{
	std::string aValue;
	if ((!RegistryRead(theKey, &aValue)) || (aValue.compare(0, 3, "sz:") != 0))
		return false;

	*theString = UnescapeRegistryString(aValue.substr(3));
	return true;
}

bool SexyAppBase::RegistryReadInteger(const std::string& theKey, int* theValue)
{
	std::string aValue;
	if ((!RegistryRead(theKey, &aValue)) || (aValue.compare(0, 6, "dword:") != 0))
		return false;

	*theValue = (int) strtoll(aValue.c_str() + 6, NULL, 10);
	return true;
}

bool SexyAppBase::RegistryReadBoolean(const std::string& theKey, bool* theValue)
{
	int aValue;
	if (!RegistryReadInteger(theKey, &aValue))
		return false;

	*theValue = aValue != 0;
	return true;
}

bool SexyAppBase::RegistryReadData(const std::string& theKey, uchar* theValue, ulong* theLength)
{
	std::string aValue;
	if ((!RegistryRead(theKey, &aValue)) || (aValue.compare(0, 4, "hex:") != 0))
		return false;

	ulong aLength = (ulong) (aValue.length() - 4) / 2;
	if (aLength > *theLength)
	{
		// like RegQueryValueEx's ERROR_MORE_DATA
		*theLength = aLength;
		return false;
	}

	for (ulong i = 0; i < aLength; i++)
		theValue[i] = (uchar) strtoul(aValue.substr(4 + i * 2, 2).c_str(), NULL, 16);
	*theLength = aLength;
	return true;
}

void SexyAppBase::ReadFromRegistry()
{
	mReadFromRegistry = true;
	mRegKey = SexyStringToString(GetString("RegistryKey", StringToSexyString(mRegKey)));

	if (mRegKey.length() == 0)
		return;

	int anInt;
	if (RegistryReadInteger("MusicVolume", &anInt))
		mMusicVolume = anInt / 100.0;

	if (RegistryReadInteger("SfxVolume", &anInt))
		mSfxVolume = anInt / 100.0;

	if (RegistryReadInteger("Muted", &anInt))
		mMuteCount = anInt;

	if (RegistryReadInteger("ScreenMode", &anInt))
		mIsWindowed = anInt == 0;

	RegistryReadInteger("PreferredX", &mPreferredX);
	RegistryReadInteger("PreferredY", &mPreferredY);

	if (RegistryReadInteger("CustomCursors", &anInt))
		EnableCustomCursors(anInt != 0);

	RegistryReadBoolean("WaitForVSync", &mWaitForVSync);

	if (RegistryReadInteger("InProgress", &anInt))
		mLastShutdownWasGraceful = anInt == 0;

	if (!IsScreenSaver())
		RegistryWriteInteger("InProgress", 1);
}

//////////////////////////////////////////////////////////////////////////
// Files

bool SexyAppBase::WriteBytesToFile(const std::string& theFileName, const void* theData, unsigned long theDataLen)
{
	// The VFS creates the folders and puts relative paths into the user dir
	FILE* aFP = Platform::VFS::Get().Open(theFileName, "w+b");
	if (aFP == NULL)
		return false;

	bool success = fwrite(theData, 1, theDataLen, aFP) == theDataLen;
	success = (fclose(aFP) == 0) && (success);

	SyncUserData();
	return success;
}

bool SexyAppBase::WriteBufferToFile(const std::string& theFileName, const Buffer* theBuffer)
{
	return WriteBytesToFile(theFileName, theBuffer->GetDataPtr(), theBuffer->GetDataLen());
}

bool SexyAppBase::ReadBufferFromFile(const std::string& theFileName, Buffer* theBuffer, bool dontWriteToDemo)
{
	Platform::VFS& aVFS = Platform::VFS::Get();

	std::vector<uint8_t> aData;
	if ((!aVFS.ReadAll(GetUserDataPath(theFileName), aData)) && (!aVFS.ReadAll(theFileName, aData)))
		return false;

	theBuffer->Clear();
	if (!aData.empty())
		theBuffer->SetData(&aData[0], (int) aData.size());
	return true;
}

bool SexyAppBase::FileExists(const std::string& theFileName)
{
	Platform::VFS& aVFS = Platform::VFS::Get();
	return (!aVFS.Resolve(GetUserDataPath(theFileName)).empty()) || (!aVFS.Resolve(theFileName).empty());
}

bool SexyAppBase::EraseFile(const std::string& theFileName)
{
	// Only files in the user dir can be erased; the game data is read-only
	std::string aPath = Platform::VFS::Get().Resolve(GetUserDataPath(theFileName));
	if ((aPath.empty()) || (remove(aPath.c_str()) != 0))
		return false;

	SyncUserData();
	return true;
}

void SexyAppBase::SyncUserData()
{
	Platform::SyncUserData();
	mBackupDirty = true;
}

bool SexyAppBase::GetShutdownSave(std::string* /*theFileName*/, Buffer* /*theBuffer*/)
{
	return false;
}

void SexyAppBase::WriteShutdownRegistry()
{
	WriteToRegistry();
}

// The registry as a shutdown now would leave it ("" if it wouldn't be written yet), leaving the
// registry and its file as they are
std::string SexyAppBase::GetShutdownRegistryText()
{
	if ((!mReadFromRegistry) || (mRegKey.length() == 0))
		return "";

	LoadRegistry();
	RegistryValueMap aRegistry = mRegistry;
	mRegistryCapture = true;
	WriteShutdownRegistry();
	mRegistryCapture = false;
	std::string aText = GetRegistryText();
	mRegistry.swap(aRegistry);
	return aText;
}

// The path inside the user dir that VFS::Open() writes a game file to ("" if outside it)
static std::string UserDirRelativePath(const std::string& theFileName)
{
	std::string aPath = GetUserDataPath(theFileName);
	if (sexy_strnicmp(aPath.c_str(), "userdata/", 9) == 0)
		return aPath.substr(9);
	return "";
}

// Called at the end of every browser frame (force: when the page goes away, and after shutdown).
// Four times a second it compares what a shutdown would write with the last backup. A save appearing
// or going, a settings change and any user file written are backed up at once; a changing save (a
// game in progress changes every tick) at most every 2 seconds.
void SexyAppBase::UpdateUserDataBackup(bool force)
{
#ifdef __EMSCRIPTEN__
	DWORD aTime = timeGetTime();
	if ((!force) && (!mBackupDirty) && (aTime - mLastBackupCheck < 250))
		return;
	mLastBackupCheck = aTime;

	std::string aFileName;
	Buffer aBuffer;
	bool hasSave = GetShutdownSave(&aFileName, &aBuffer);
	std::string aSave;
	if (hasSave)
		aSave.assign((const char*) aBuffer.GetDataPtr(), aBuffer.GetDataLen());
	std::string aRegistry = GetShutdownRegistryText();

	bool saveChanged = (hasSave != mBackupHasSave) || (aSave != mBackupSave);
	bool saveDue = (hasSave != mBackupHasSave) || (aTime - mLastBackupTime >= 2000);
	if ((!force) && (!mBackupDirty) && (aRegistry == mBackupRegistry) && ((!saveChanged) || (!saveDue)))
		return;

	std::vector<Platform::BackupFile> aFiles;
	if (!aRegistry.empty())
		aFiles.push_back(Platform::BackupFile{ UserDirRelativePath(RegistryPath()), aRegistry });
	if ((hasSave) && (!UserDirRelativePath(aFileName).empty()))
		aFiles.push_back(Platform::BackupFile{ UserDirRelativePath(aFileName), aSave });
	Platform::BackupUserData(aFiles);

	mBackupDirty = false;
	mLastBackupTime = aTime;
	mBackupHasSave = hasSave;
	mBackupSave.swap(aSave);
	mBackupRegistry.swap(aRegistry);
#else
	(void) force;
#endif
}

void SexyAppBase::PageGoingAway()
{
	// 1.3 writes the settings only on shutdown, which a closed tab never reaches
	if (mReadFromRegistry)
		WriteToRegistry();
	UpdateUserDataBackup(true);
}

//////////////////////////////////////////////////////////////////////////

void SexyAppBase::SEHOccured()
{
	SetMusicVolume(0);
	mSEHOccured = true;
	EnforceCursor();
}

std::string SexyAppBase::GetGameSEHInfo()
{
	int aSecLoaded = (GetTickCount() - mTimeLoaded) / 1000;

	char aTimeStr[16];
	snprintf(aTimeStr, sizeof(aTimeStr), "%02d:%02d:%02d", (aSecLoaded/60/60), (aSecLoaded/60)%60, aSecLoaded%60);

	std::string anInfoString =
		"Product: " + mProdName + "\r\n" +
		"Version: " + mProductVersion + "\r\n";

	anInfoString +=
		"Time Loaded: " + std::string(aTimeStr) + "\r\n"
		"Fullscreen: " + (mIsWindowed ? std::string("No") : std::string("Yes")) + "\r\n";

	return anInfoString;
}

void SexyAppBase::ShutdownHook()
{
}

void SexyAppBase::Shutdown()
{
	if (!mShutdown)
	{
		mExitToTop = true;
		mShutdown = true;
		ShutdownHook();

		if (mMusicInterface != NULL)
			mMusicInterface->StopAllMusic();

		if (mHWnd != NULL)
			SDL_HideWindow(mHWnd);

		if (mReadFromRegistry)
			WriteToRegistry();
	}
}

void SexyAppBase::DoExit(int theCode)
{
	exit(theCode);
}

//////////////////////////////////////////////////////////////////////////
// Updating and drawing

void SexyAppBase::UpdateFrames()
{
	mUpdateCount++;

	if (mScriptPos < mScript.size())
		RunScript();

	if (!mMinimized)
	{
		if (mWidgetManager->UpdateFrame())
			++mFPSDirtyCount;
	}

	mMusicInterface->Update();
	CleanSharedImages();
}

void SexyAppBase::DoUpdateFramesF(float theFrac)
{
	if ((mVSyncUpdates) && (!mMinimized))
		mWidgetManager->UpdateFrameF(theFrac);
}

bool SexyAppBase::DoUpdateFrames()
{
	if ((mLoadingThreadCompleted) && (!mLoaded))
	{
		mLoaded = true;
		mYieldMainThread = false;
		LoadingThreadCompleted();
	}

	UpdateFrames();
	return true;
}

void SexyAppBase::Redraw(Rect* theClipRect)
{
	// Do mIsDrawing check because we could enter here at a bad time if any windows messages
	//  are processed during WidgetManager->Draw
	if ((mIsDrawing) || (mShutdown))
		return;

	mDDInterface->Present();

	mFPSFlipCount++;
}

bool SexyAppBase::DrawDirtyStuff()
{
	MTAutoDisallowRand aDisallowRand;

	DWORD aStartTime = timeGetTime();

	// port: the GPU back buffer isn't persistent, so a frame is either skipped or redrawn completely.
	// Whether to draw follows 1.3: something is dirty, the custom cursor changed or a second passed
	// (and the draw throttle used while loading allows it). The first frame is always drawn.
	bool hasDirtyWidgets = false;
	for (WidgetList::iterator anItr = mWidgetManager->mWidgets.begin(); anItr != mWidgetManager->mWidgets.end(); ++anItr)
	{
		if (((*anItr)->mDirty) && ((*anItr)->mVisible))
			hasDirtyWidgets = true;
	}

	if (((!hasDirtyWidgets) && (aStartTime - mLastDrawTick < 1000) && (!mCustomCursorDirty) && (mDrawCount > 0)) ||
		((int) (aStartTime - mNextDrawTick) < 0))
	{
		mHasPendingDraw = false;
		mLastDrawWasEmpty = true;
		return false;
	}

	mIsDrawing = true;
	mDDInterface->BeginFrame();
	mWidgetManager->MarkAllDirty();
	mWidgetManager->DrawScreen();
	mIsDrawing = false;

	mLastDrawWasEmpty = false;

	mDrawCount++;

	DWORD aMidTime = timeGetTime();

	mFPSCount++;
	mFPSTime += aMidTime - aStartTime;

	mDrawTime += aMidTime - aStartTime;

	DWORD aPreScreenBltTime = timeGetTime();
	mLastDrawTick = aPreScreenBltTime;

	// Presents the frame; DDInterface draws the custom cursor on top
	mDDInterface->SetCursorPos(mLastMouseX, mLastMouseY);
	Redraw(NULL);

	// This is our one UpdateFTimeAcc if we are vsynched
	UpdateFTimeAcc();

	DWORD aEndTime = timeGetTime();

	mScreenBltTime = aEndTime - aPreScreenBltTime;

	if ((mLoadingThreadStarted) && (!mLoadingThreadCompleted))
	{
		int aTotalTime = aEndTime - aStartTime;

		mNextDrawTick += 35 + std::max(aTotalTime, 15);

		if ((int) (aEndTime - mNextDrawTick) >= 0)
			mNextDrawTick = aEndTime;
	}
	else
		mNextDrawTick = aEndTime;

	mHasPendingDraw = false;
	mCustomCursorDirty = false;

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Message boxes

void SexyAppBase::BeginPopup()
{
}

void SexyAppBase::EndPopup()
{
	ClearUpdateBacklog();
	ClearKeysDown();

	if (mWidgetManager->mDownButtons)
		mWidgetManager->DoMouseUps();
}

int SexyAppBase::MsgBox(const std::string& theText, const std::string& theTitle, int theFlags)
{
	int aButtons = theFlags & 0x0F;
	int aResult = IDOK;

	BeginPopup();

#ifdef __EMSCRIPTEN__
	SDL_Log("%s: %s", theTitle.c_str(), theText.c_str());

	std::string aMessage = Latin1ToUTF8(theTitle + "\n\n" + theText);
	if ((aButtons == MB_YESNO) || (aButtons == MB_OKCANCEL))
	{
		bool accepted = sexyapp_confirm(aMessage.c_str()) != 0;
		if (aButtons == MB_YESNO)
			aResult = accepted ? IDYES : IDNO;
		else
			aResult = accepted ? IDOK : IDCANCEL;
	}
	else
		sexyapp_alert(aMessage.c_str());
#else
	SDL_MessageBoxButtonData aButtonData[2];
	int aNumButtons = 1;
	int aCancelResult = IDOK;
	if (aButtons == MB_YESNO)
	{
		aButtonData[0] = { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDYES, "Yes" };
		aButtonData[1] = { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDNO, "No" };
		aNumButtons = 2;
		aCancelResult = IDNO;
	}
	else if (aButtons == MB_OKCANCEL)
	{
		aButtonData[0] = { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDOK, "OK" };
		aButtonData[1] = { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDCANCEL, "Cancel" };
		aNumButtons = 2;
		aCancelResult = IDCANCEL;
	}
	else
		aButtonData[0] = { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDOK, "OK" };

	Uint32 aBoxFlags = SDL_MESSAGEBOX_INFORMATION;
	if ((theFlags & 0xF0) == MB_ICONERROR)
		aBoxFlags = SDL_MESSAGEBOX_ERROR;
	else if ((theFlags & 0xF0) == MB_ICONWARNING)
		aBoxFlags = SDL_MESSAGEBOX_WARNING;

	std::string aTitle = Latin1ToUTF8(theTitle);
	std::string aText = Latin1ToUTF8(theText);
	SDL_Window* aParent = ((mHWnd != NULL) && (SDL_GetWindowFlags(mHWnd) & SDL_WINDOW_SHOWN)) ? mHWnd : NULL;

	SDL_MessageBoxData aData = { aBoxFlags | SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT, aParent, aTitle.c_str(), aText.c_str(), aNumButtons, aButtonData, NULL };
	int aButtonId = -1;
	if ((SDL_ShowMessageBox(&aData, &aButtonId) == 0) && (aButtonId >= 0))
		aResult = aButtonId;
	else
		aResult = aCancelResult;
#endif

	EndPopup();

	return aResult;
}

void SexyAppBase::Popup(const std::string& theString)
{
	BeginPopup();
	if (!mShutdown)
	{
		std::string aTitle = SexyStringToString(GetString("FATAL_ERROR", _S("FATAL ERROR")));
		SDL_Log("%s: %s", aTitle.c_str(), theString.c_str());

#ifdef __EMSCRIPTEN__
		sexyapp_alert(Latin1ToUTF8(aTitle + "\n\n" + theString).c_str());
#else
		SDL_Window* aParent = ((mHWnd != NULL) && (SDL_GetWindowFlags(mHWnd) & SDL_WINDOW_SHOWN)) ? mHWnd : NULL;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, Latin1ToUTF8(aTitle).c_str(), Latin1ToUTF8(theString).c_str(), aParent);
#endif
	}
	EndPopup();
}

void SexyAppBase::SafeDeleteWidget(Widget* theWidget)
{
	WidgetSafeDeleteInfo aWidgetSafeDeleteInfo;
	aWidgetSafeDeleteInfo.mUpdateAppDepth = mUpdateAppDepth;
	aWidgetSafeDeleteInfo.mWidget = theWidget;
	mSafeDeleteList.push_back(aWidgetSafeDeleteInfo);
}

//////////////////////////////////////////////////////////////////////////
// Input and window events

void SexyAppBase::HandleNotifyGameMessage(int theType, int theParam)
{
	// port: no single-instance messaging
}

void SexyAppBase::RehupFocus()
{
	bool wantHasFocus = mActive && !mMinimized;

	if (wantHasFocus != mHasFocus)
	{
		mHasFocus = wantHasFocus;

		if (mHasFocus)
		{
			if (mMuteOnLostFocus)
				Unmute(true);

			mWidgetManager->GotFocus();
			GotFocus();
		}
		else
		{
			if (mMuteOnLostFocus)
				Mute(true);

			mWidgetManager->LostFocus();
			LostFocus();

			mWidgetManager->DoMouseUps();
		}
	}
}

void SexyAppBase::ClearKeysDown()
{
	if (mWidgetManager != NULL) // fix stuck alt-key problem
	{
		for (int aKeyNum = 0; aKeyNum < 0xFF; aKeyNum++)
			mWidgetManager->mKeyDown[aKeyNum] = false;
	}
	mCtrlDown = false;
	mAltDown = false;
}

void SexyAppBase::ShowFPS(bool show)
{
	// port: the FPS overlay is not ported
	mShowFPS = show;
	mWidgetManager->MarkAllDirty();
}

bool SexyAppBase::DebugKeyDown(int theKey)
{
	// port: the framework's debug keys (FPS, screenshots, memory dumps, 3D toggle) are not ported
	return false;
}

void SexyAppBase::CloseRequestAsync()
{
}

bool SexyAppBase::ProcessEvents()
{
	SDL_Event anEvent;
	if (!SDL_PollEvent(&anEvent))
		return false;

	HandleEvent(anEvent);
	return true;
}

void SexyAppBase::TranslateMouse(int theWindowX, int theWindowY, int* theX, int* theY)
{
	if (mDDInterface != NULL)
		mDDInterface->WindowToLogical(theWindowX, theWindowY, theX, theY);
	else
	{
		*theX = theWindowX;
		*theY = theWindowY;
	}
}

void SexyAppBase::MouseMoveTo(int theX, int theY)
{
	mLastMouseX = theX;
	mLastMouseY = theY;

	mWidgetManager->MouseMove(theX, theY);

	if (!mMouseIn)
	{
		mMouseIn = true;
		EnforceCursor();
	}

	if ((mDDInterface != NULL) && (mDDInterface->mCursorImage != NULL))
	{
		// the custom cursor moves with the next frame
		mDDInterface->SetCursorPos(theX, theY);
		mCustomCursorDirty = true;
	}
}

void SexyAppBase::HandleEvent(const SDL_Event& theEvent)
{
	// port: a -script run takes its input only from the script, and focus changes (which pause the
	// game) are ignored too, so the run is the same whatever happens to the window
	if (!mScript.empty())
	{
		switch (theEvent.type)
		{
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEWHEEL:
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		case SDL_TEXTINPUT:
			return;
		case SDL_WINDOWEVENT:
			if ((theEvent.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) || (theEvent.window.event == SDL_WINDOWEVENT_FOCUS_LOST) ||
				(theEvent.window.event == SDL_WINDOWEVENT_MINIMIZED) || (theEvent.window.event == SDL_WINDOWEVENT_MAXIMIZED) ||
				(theEvent.window.event == SDL_WINDOWEVENT_RESTORED) || (theEvent.window.event == SDL_WINDOWEVENT_LEAVE))
				return;
			break;
		}
	}

#ifdef __EMSCRIPTEN__
	// A press or key in the page means the page has focus, even when the browser sent no focus
	// event (see EmscriptenFocusCallback). Windows activates a window before passing on the click.
	if ((!mActive) && (!mSEHOccured) && (!mShutdown) &&
		((theEvent.type == SDL_MOUSEBUTTONDOWN) || (theEvent.type == SDL_KEYDOWN) || (theEvent.type == SDL_FINGERDOWN)))
	{
		mActive = true;
		RehupFocus();
	}
#endif

	switch (theEvent.type)
	{
	case SDL_QUIT:
		// WM_CLOSE
		CloseRequestAsync();
		mManualShutdown = true;
		Shutdown();
		break;

	case SDL_WINDOWEVENT:
		switch (theEvent.window.event)
		{
		case SDL_WINDOWEVENT_FOCUS_GAINED:
		case SDL_WINDOWEVENT_FOCUS_LOST:
			// WM_ACTIVATEAPP
			if ((!mSEHOccured) && (!mShutdown))
			{
				mActive = theEvent.window.event == SDL_WINDOWEVENT_FOCUS_GAINED;
				RehupFocus();

				if ((mActive) && (!mIsWindowed))
					mWidgetManager->MarkAllDirty();

				if ((mIsOpeningURL) && (!mActive))
					URLOpenSucceeded(mOpeningURL);
			}
			break;

		case SDL_WINDOWEVENT_MINIMIZED:
		case SDL_WINDOWEVENT_MAXIMIZED:
		case SDL_WINDOWEVENT_RESTORED:
			// WM_SIZE
			{
				bool isMinimized = theEvent.window.event == SDL_WINDOWEVENT_MINIMIZED;
				mPhysMinimized = isMinimized;

				if ((!mShutdown) && (isMinimized != mMinimized))
				{
					mMinimized = isMinimized;

					// We don't want any sounds (or music) playing while its minimized
					if (mMinimized)
					{
						if (mMuteOnLostFocus)
							Mute(true);
					}
					else
					{
						if (mMuteOnLostFocus)
							Unmute(true);

						mWidgetManager->MarkAllDirty();
					}
				}

				RehupFocus();
			}
			break;

		case SDL_WINDOWEVENT_SIZE_CHANGED:
			if (mDDInterface != NULL)
				mDDInterface->UpdateDisplaySize();
			// fullscreen can also be left without the game (Esc in the browser, the window manager)
			mIsWindowed = (SDL_GetWindowFlags(mHWnd) & SDL_WINDOW_FULLSCREEN) == 0;
			mIsPhysWindowed = mIsWindowed;
			mWidgetManager->MarkAllDirty();
			break;

		case SDL_WINDOWEVENT_EXPOSED:
			mWidgetManager->MarkAllDirty();
			break;

		case SDL_WINDOWEVENT_MOVED:
			// WM_MOVE: remember the normal window position
			if ((mIsWindowed) && ((SDL_GetWindowFlags(mHWnd) & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_MINIMIZED | SDL_WINDOW_FULLSCREEN)) == 0))
			{
				mPreferredX = theEvent.window.data1;
				mPreferredY = theEvent.window.data2;
			}
			break;

		case SDL_WINDOWEVENT_LEAVE:
			// 1.3 detected this by polling the cursor in WM_TIMER
			if (mMouseIn)
			{
				mMouseIn = false;
				mWidgetManager->MouseExit(mLastMouseX, mLastMouseY);
				EnforceCursor();
			}
			break;
		}
		break;

	case SDL_MOUSEMOTION:
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		if (!mSEHOccured)
		{
			bool isMotion = theEvent.type == SDL_MOUSEMOTION;
			int x;
			int y;
			TranslateMouse(isMotion ? theEvent.motion.x : theEvent.button.x, isMotion ? theEvent.motion.y : theEvent.button.y, &x, &y);
			mLastUserInputTick = timeGetTime();
			MouseMoveTo(x, y);

			if (theEvent.type == SDL_MOUSEBUTTONDOWN)
			{
				int aClickCode = GetClickCode(theEvent.button, true);
				if (aClickCode != 0)
					mWidgetManager->MouseDown(x, y, aClickCode);
			}
			else if (theEvent.type == SDL_MOUSEBUTTONUP)
			{
				int aClickCode = GetClickCode(theEvent.button, false);
				if (aClickCode != 0)
					mWidgetManager->MouseUp(x, y, aClickCode);
			}
		}
		break;

	case SDL_MOUSEWHEEL:
		{
			int aDelta = theEvent.wheel.y;
			if (theEvent.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
				aDelta = -aDelta;
			if (aDelta != 0)
				mWidgetManager->MouseWheel(aDelta);
		}
		break;

	case SDL_KEYDOWN:
	case SDL_KEYUP:
		{
			bool keyDown = theEvent.type == SDL_KEYDOWN;
			int aKeyCode = SDLKeyToKeyCode(theEvent.key.keysym.sym);

			mLastUserInputTick = timeGetTime();

			if (aKeyCode == KEYCODE_CONTROL)
				mCtrlDown = keyDown;
			if (aKeyCode == KEYCODE_MENU)
				mAltDown = keyDown;

			if (!keyDown)
			{
				if (aKeyCode != KEYCODE_UNKNOWN)
					mWidgetManager->KeyUp((KeyCode) aKeyCode);
				break;
			}

#ifndef __EMSCRIPTEN__
			if ((aKeyCode == KEYCODE_RETURN) && (theEvent.key.keysym.mod & KMOD_ALT) && (!mForceFullscreen) && (!mForceWindowed))
			{
				SwitchScreenMode(!mIsWindowed);
				ClearKeysDown();
				break;
			}
#endif

			if (aKeyCode != KEYCODE_UNKNOWN)
			{
				if ((aKeyCode == 'D') && (mWidgetManager->mKeyDown[KEYCODE_CONTROL]) && (mWidgetManager->mKeyDown[KEYCODE_MENU]))
					mDebugKeysEnabled = !mDebugKeysEnabled;

				if ((!mDebugKeysEnabled) || (!DebugKeyDown(aKeyCode)))
					mWidgetManager->KeyDown((KeyCode) aKeyCode);
			}

			int aChar = GetControlChar(theEvent.key.keysym);
			if (aChar != 0)
				mWidgetManager->KeyChar((SexyChar) aChar);
		}
		break;

	case SDL_TEXTINPUT:
		{
			mLastUserInputTick = timeGetTime();

			std::string aText = UTF8ToLatin1(theEvent.text.text);
			for (size_t i = 0; i < aText.length(); i++)
				mWidgetManager->KeyChar((SexyChar) aText[i]);
		}
		break;

	case SDL_RENDER_DEVICE_RESET:
		// every texture is gone; they are recreated on demand
		if (mDDInterface != NULL)
			mDDInterface->ReleaseTextures();
		mWidgetManager->MarkAllDirty();
		break;

	case SDL_RENDER_TARGETS_RESET:
		mWidgetManager->MarkAllDirty();
		break;
	}
}

void SexyAppBase::Done3dTesting()
{
}

// return file name that you want to upload
std::string SexyAppBase::NotifyCrashHook()
{
	return "";
}

//////////////////////////////////////////////////////////////////////////
// Window

void SexyAppBase::MakeWindow()
{
	if (mHWnd == NULL)
	{
#ifndef __EMSCRIPTEN__
		// HiDPI: let Windows report real pixels instead of bitmap-stretching the window (only effective
		// when main() hasn't initialized SDL video already)
		SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
		SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
#endif

		if ((!SDL_WasInit(SDL_INIT_VIDEO)) && (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0))
		{
			Popup(std::string("Failed to initialize the display: ") + SDL_GetError());
			DoExit(1);
		}

		int aScale = 1;
		int aPlaceX = SDL_WINDOWPOS_CENTERED;
		int aPlaceY = SDL_WINDOWPOS_CENTERED;
		Uint32 aFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

#ifdef __EMSCRIPTEN__
		// Browsers only enter fullscreen from a user gesture, so the game always starts in the page; the
		// canvas keeps its initial logical size unless the page resizes it.
		mIsWindowed = true;
#else
		// Use the desktop the remembered position is on, if it still exists
		int aDisplay = 0;
		bool hasPreferredPos = false;
		if (mPreferredX != -1)
		{
			for (int i = 0; i < SDL_GetNumVideoDisplays(); i++)
			{
				SDL_Rect aRect;
				if ((SDL_GetDisplayUsableBounds(i, &aRect) == 0) &&
					(mPreferredX >= aRect.x) && (mPreferredX < aRect.x + aRect.w) &&
					(mPreferredY >= aRect.y) && (mPreferredY < aRect.y + aRect.h))
				{
					aDisplay = i;
					hasPreferredPos = true;
					break;
				}
			}
		}

		SDL_Rect aDesktopRect;
		if (SDL_GetDisplayUsableBounds(aDisplay, &aDesktopRect) == 0)
		{
			// Scale the window up to 2x when that still fits, leaving room for the window frame
			if ((mWidth * 2 <= aDesktopRect.w - 32) && (mHeight * 2 <= aDesktopRect.h - 64))
				aScale = 2;

			if (hasPreferredPos)
			{
				int aSpacing = 4;
				aPlaceX = std::max(aDesktopRect.x + aSpacing, std::min(mPreferredX, aDesktopRect.x + aDesktopRect.w - mWidth * aScale - aSpacing));
				aPlaceY = std::max(aDesktopRect.y + aSpacing, std::min(mPreferredY, aDesktopRect.y + aDesktopRect.h - mHeight * aScale - aSpacing));
			}
		}

		if (!mIsWindowed)
			aFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif

		mHWnd = SDL_CreateWindow(Latin1ToUTF8(SexyStringToStringFast(mTitle)).c_str(), aPlaceX, aPlaceY, mWidth * aScale, mHeight * aScale, aFlags);
		if (mHWnd == NULL)
		{
			Popup(std::string("Failed to create the window: ") + SDL_GetError());
			DoExit(1);
		}

		mIsPhysWindowed = mIsWindowed;
		SDL_StartTextInput();
	}

	if (mDDInterface == NULL)
		mDDInterface = new DDInterface(this);
	mDDInterface->mForcedRenderScale = mForcedRenderScale;

	int aResult = InitDDInterface();
	if (aResult != DDInterface::RESULT_OK)
	{
		Popup(GetString("FAILED_INIT_DIRECTDRAW", _S("Failed to initialize the display: ")) + StringToSexyString(SDL_GetError()));
		DoExit(1);
	}

	ReInitImages();

	mWidgetManager->mImage = mDDInterface->mScreenImage;
	mWidgetManager->MarkAllDirty();
}

void SexyAppBase::DeleteNativeImageData()
{
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;
		aMemoryImage->DeleteNativeData();
		++anItr;
	}
}

void SexyAppBase::DeleteExtraImageData()
{
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;
		aMemoryImage->DeleteExtraBuffers();
		++anItr;
	}
}

void SexyAppBase::ReInitImages()
{
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;
		aMemoryImage->ReInit();
		++anItr;
	}
}

//////////////////////////////////////////////////////////////////////////
// Loading "thread"

void SexyAppBase::LoadingThreadProc()
{
}

void SexyAppBase::LoadingThreadCompleted()
{
}

void SexyAppBase::LoadingThreadProcStub(void* theArg)
{
	SexyAppBase* aSexyApp = (SexyAppBase*) theArg;

	aSexyApp->mLoadingThreadRunning = true;
	aSexyApp->LoadingThreadProc();
	aSexyApp->mLoadingThreadRunning = false;

	SDL_Log("Resource Loading Time: %d", (int) (GetTickCount() - aSexyApp->mTimeLoaded));

	aSexyApp->mLoadingThreadCompleted = true;
}

void SexyAppBase::StartLoadingThread()
{
	// port: no threads (the web build has none without cross-origin isolation). UpdateAppStep() runs
	// LoadingThreadProc() synchronously once the first frame is on screen, and DoUpdateFrames() then
	// calls LoadingThreadCompleted() as in 1.3.
	if (!mLoadingThreadStarted)
		mLoadingThreadStarted = true;
}

//////////////////////////////////////////////////////////////////////////
// Screen mode and cursor

void SexyAppBase::SwitchScreenMode(bool wantWindowed, bool is3d, bool force)
{
	if (mForceFullscreen)
		wantWindowed = false;

	if (mIsWindowed == wantWindowed && !force)
	{
		Set3DAcclerated(is3d);
		return;
	}

	// Set 3d acceleration preference
	Set3DAcclerated(is3d, false);

	mIsWindowed = wantWindowed;

	// port: SDL switches the existing window (desktop-resolution fullscreen) instead of recreating it
	if (mHWnd != NULL)
	{
		SDL_SetWindowFullscreen(mHWnd, mIsWindowed ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
		mIsPhysWindowed = mIsWindowed;

		if (mDDInterface != NULL)
			mDDInterface->UpdateDisplaySize();
		mWidgetManager->MarkAllDirty();
	}

	mLastTime = timeGetTime();
}

void SexyAppBase::SwitchScreenMode(bool wantWindowed)
{
	SwitchScreenMode(wantWindowed, Is3DAccelerated());
}

void SexyAppBase::SwitchScreenMode()
{
	SwitchScreenMode(mIsWindowed, Is3DAccelerated(), true);
}

void SexyAppBase::SetAlphaDisabled(bool isDisabled)
{
	if (mAlphaDisabled != isDisabled)
	{
		mAlphaDisabled = isDisabled;
		mWidgetManager->MarkAllDirty();
	}
}

void SexyAppBase::EnforceCursor()
{
	if (mDDInterface == NULL)
		return;

	Image* aCursorImage = NULL;
	int aSysCursor;

	if ((mSEHOccured) || (!mMouseIn))
		aSysCursor = CURSOR_POINTER;
	else if ((mCursorNum < 0) || (mCursorNum >= NUM_CURSORS))
		aSysCursor = CURSOR_POINTER;
	else if ((mCursorImages[mCursorNum] == NULL) ||
		((!mCustomCursorsEnabled) && (mCursorNum != CURSOR_CUSTOM)))
		aSysCursor = mCursorNum;
	else
	{
		// DDInterface draws the image cursor with each frame; hide the system one
		aCursorImage = mCursorImages[mCursorNum];
		aSysCursor = CURSOR_NONE;
	}

	if (mDDInterface->mCursorImage != aCursorImage)
	{
		mDDInterface->SetCursorImage(aCursorImage);
		mCustomCursorDirty = true;
	}
	mDDInterface->mCursorVisible = aCursorImage != NULL;
	if (aCursorImage != NULL)
		mDDInterface->SetCursorPos(mLastMouseX, mLastMouseY);

	SetSystemCursor(aSysCursor);
	mSysCursor = aCursorImage == NULL;
}

void SexyAppBase::ProcessSafeDeleteList()
{
	MTAutoDisallowRand aDisallowRand;

	WidgetSafeDeleteList::iterator anItr = mSafeDeleteList.begin();
	while (anItr != mSafeDeleteList.end())
	{
		WidgetSafeDeleteInfo* aWidgetSafeDeleteInfo = &(*anItr);
		if (mUpdateAppDepth <= aWidgetSafeDeleteInfo->mUpdateAppDepth)
		{
			delete aWidgetSafeDeleteInfo->mWidget;
			anItr = mSafeDeleteList.erase(anItr);
		}
		else
			++anItr;
	}
}

//////////////////////////////////////////////////////////////////////////
// Main loop

void SexyAppBase::UpdateFTimeAcc()
{
	DWORD aCurTime = GetClockTime();

	if (mLastTimeCheck != 0)
	{
		int aDeltaTime = aCurTime - mLastTimeCheck;

		mUpdateFTimeAcc = std::min(mUpdateFTimeAcc + aDeltaTime, 200.0);

		if (mRelaxUpdateBacklogCount > 0)
			mRelaxUpdateBacklogCount = std::max(mRelaxUpdateBacklogCount - aDeltaTime, 0);
	}

	mLastTimeCheck = aCurTime;
}

// One step of 1.3's update state machine: an update when one is due (every mFrameTime ms of game
// time), otherwise a draw when one is pending, otherwise a sleep until the next update. At most ~10
// updates run back to back before a draw is forced (the game slows down instead of skipping frames).
// port: updates are never locked to the display refresh (1.3's DirectDraw vsync timing), so the
// vsynched branches of 1.3 are gone.
bool SexyAppBase::Process(bool allowSleep)
{
	if (mLoadingFailed)
		Shutdown();

	double aFrameFTime;
	double anUpdatesPerUpdateF;

	if (mVSyncUpdates)
	{
		aFrameFTime = (1000.0 / mSyncRefreshRate) / mUpdateMultiplier;
		anUpdatesPerUpdateF = (float) (1000.0 / (mFrameTime * mSyncRefreshRate));
	}
	else
	{
		aFrameFTime = mFrameTime / mUpdateMultiplier;
		anUpdatesPerUpdateF = 1.0;
	}

	// Make sure we're not paused
	if ((!mPaused) && (mUpdateMultiplier > 0))
	{
		UpdateFTimeAcc();

		// mNonDrawCount is used to make sure we draw the screen at least
		// 10 times per second, even if it means we have to slow down
		// the updates to make it draw 10 times per second in "game time"

		bool didUpdate = false;

		if (mUpdateAppState == UPDATESTATE_PROCESS_1)
		{
			if ((++mNonDrawCount < (int) ceil(10*mUpdateMultiplier)) || (!mLoaded))
			{
				if (mUpdateFTimeAcc >= aFrameFTime)
				{
					bool hadRealUpdate = DoUpdateFrames();
					if (hadRealUpdate)
						mUpdateAppState = UPDATESTATE_PROCESS_2;

					mHasPendingDraw = true;
					didUpdate = true;
				}
			}
		}
		else if (mUpdateAppState == UPDATESTATE_PROCESS_2)
		{
			mUpdateAppState = UPDATESTATE_PROCESS_DONE;

			mPendingUpdatesAcc += anUpdatesPerUpdateF;
			mPendingUpdatesAcc -= 1.0;
			ProcessSafeDeleteList();

			// Process any extra updates
			while (mPendingUpdatesAcc >= 1.0)
			{
				++mNonDrawCount;
				bool hasRealUpdate = DoUpdateFrames();

				if (!hasRealUpdate)
					break;

				ProcessSafeDeleteList();
				mPendingUpdatesAcc -= 1.0;
			}

			DoUpdateFramesF((float) anUpdatesPerUpdateF);
			ProcessSafeDeleteList();

			mUpdateFTimeAcc -= aFrameFTime;

			if (mRelaxUpdateBacklogCount > 0)
				mUpdateFTimeAcc = 0;

			didUpdate = true;
		}

		if (!didUpdate)
		{
			mUpdateAppState = UPDATESTATE_PROCESS_DONE;

			mNonDrawCount = 0;

			// port: with -fast only every FAST_DRAW_INTERVAL-th update is drawn (and the first frame)
			if ((mHasPendingDraw) && ((!mFastMode) || (mDrawCount == 0) || (mUpdateCount - mLastFastDrawUpdate >= FAST_DRAW_INTERVAL)))
			{
				DrawDirtyStuff();
				mLastFastDrawUpdate = mUpdateCount;
			}
			else
			{
				// Let us take into account the time it took to draw dirty stuff
				int aTimeToNextFrame = (int) (aFrameFTime - mUpdateFTimeAcc);
				if (aTimeToNextFrame > 0)
				{
					if (mFastMode)
					{
						// port: -fast skips the wait on its virtual clock
						mFastClock += aTimeToNextFrame;
					}
					else
					{
						if (!allowSleep)
							return false;

						// Wait till next processing cycle
						++mSleepCount;
						SDL_Delay(aTimeToNextFrame);
					}
				}
			}
		}
	}

	ProcessSafeDeleteList();
	return true;
}

void SexyAppBase::DoMainLoop()
{
#ifdef __EMSCRIPTEN__
	// The browser drives the loop (one EmscriptenMainLoop() per animation frame); this doesn't return.
	emscripten_set_main_loop_arg(EmscriptenMainLoop, this, 0, 1);
#else
	while (!mShutdown)
	{
		if (mExitToTop)
			mExitToTop = false;
		UpdateApp();
	}
#endif
}

#ifdef __EMSCRIPTEN__
void SexyAppBase::EmscriptenMainLoop(void* theArg)
{
	SexyAppBase* anApp = (SexyAppBase*) theArg;

	// One browser frame: step until every due update ran and a frame was drawn, or until nothing is due
	// before the next animation frame (where 1.3 would have slept).
	int aDrawCount = anApp->mDrawCount;
	for (int aStep = 0; (aStep < 1000) && (!anApp->mShutdown); aStep++)
	{
		if (anApp->mExitToTop)
			anApp->mExitToTop = false;
		anApp->UpdateAppStep(NULL);

		if ((anApp->mDrawCount != aDrawCount) || (anApp->mNothingDue))
			break;
	}

	if (anApp->mShutdown)
	{
		emscripten_cancel_main_loop();
		// Deletes the board, which saves a game in progress, and writes the settings
		anApp->MainLoopFinished();
		anApp->UpdateUserDataBackup(true);
		sexyapp_notify_exit();
	}
	else
		anApp->UpdateUserDataBackup(false);
}
#endif

bool SexyAppBase::UpdateAppStep(bool* updated)
{
	if (updated != NULL)
		*updated = false;

	if (mExitToTop)
		return false;

	mNothingDue = false;

	if (mUpdateAppState == UPDATESTATE_PROCESS_DONE)
		mUpdateAppState = UPDATESTATE_MESSAGES;

	mUpdateAppDepth++;

	// We update in two stages to avoid doing a Process if our loop termination
	//  condition has already been met by processing windows messages
	if (mUpdateAppState == UPDATESTATE_MESSAGES)
	{
		// port: the loading "thread" runs here, once the first frame is on screen
		if ((mLoadingThreadStarted) && (!mLoadingThreadCompleted) && (!mLoadingThreadRunning) && (mDrawCount > 0))
		{
			LoadingThreadProcStub(this);
			// in 1.3 the main thread kept updating while loading; don't catch up on the time spent
			ClearUpdateBacklog();
		}

		if (!ProcessEvents())
			mUpdateAppState = UPDATESTATE_PROCESS_1;
	}
	else
	{
		int anOldUpdateCnt = mUpdateCount;
#ifdef __EMSCRIPTEN__
		// never sleep in the browser; tell the frame callback to return instead
		mNothingDue = !Process(false);
#else
		Process();
#endif
		if (updated != NULL)
			*updated = mUpdateCount != anOldUpdateCnt;
	}

	mUpdateAppDepth--;

	return true;
}

bool SexyAppBase::UpdateApp()
{
	bool updated;
	for (;;)
	{
		if (!UpdateAppStep(&updated))
			return false;
		if (updated)
			return true;
	}
}

int SexyAppBase::InitDDInterface()
{
	PreDDInterfaceInitHook();
	DeleteNativeImageData();

	// port: present in step with the display (updates stay at 100 Hz either way). On the web this keeps
	// the requestAnimationFrame timing of the main loop; without vsync SDL would switch it to setTimeout.
	// (-fast doesn't wait for the display either)
	int aResult = mDDInterface->Init(mHWnd, !mFastMode);
	if (DDInterface::RESULT_OK == aResult)
	{
		if (mDDInterface->mScreenImage == NULL)
		{
			DDImage* aScreenImage = new DDImage(mDDInterface);
			aScreenImage->Create(mWidth, mHeight);
			aScreenImage->mIsScreen = true;
			mDDInterface->mScreenImage = aScreenImage;
		}

		// port: DDInterface::WindowToLogical already maps window to logical coordinates, so the widget
		// manager's mouse remapping is the identity
		mScreenBounds = Rect(0, 0, mWidth, mHeight);
		mWidgetManager->Resize(mScreenBounds, mScreenBounds);
		PostDDInterfaceInitHook();
	}
	return aResult;
}

void SexyAppBase::PreTerminate()
{
}

void SexyAppBase::Start()
{
	if (mShutdown)
		return;

	if (mAutoStartLoadingThread)
		StartLoadingThread();

	SDL_ShowWindow(mHWnd);
	SDL_RaiseWindow(mHWnd);

#ifdef __EMSCRIPTEN__
	SDL_AddEventWatch(EmscriptenEventWatch, this);

	// a NULL callback removes SDL's handlers for the event
	emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_FALSE, NULL);
	emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_FALSE, NULL);
	emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EM_FALSE, EmscriptenFocusCallback);
	emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EM_FALSE, EmscriptenFocusCallback);
#endif

	DWORD aStartTime = timeGetTime();

	mRunning = true;
	mLastTime = aStartTime;
	mLastUserInputTick = aStartTime;
	mLastTimerTime = aStartTime;

	DoMainLoop();
	MainLoopFinished();
}

void SexyAppBase::MainLoopFinished()
{
	ProcessSafeDeleteList();

	mRunning = false;

	SDL_Log("Seconds = %g, Sleep Count = %d, Update Count = %d, Draw Count = %d",
		(timeGetTime() - mTimeLoaded) / 1000.0, mSleepCount, mUpdateCount, mDrawCount);

	PreTerminate();

	WriteToRegistry();
}

//////////////////////////////////////////////////////////////////////////
// Properties

bool SexyAppBase::CheckSignature(const Buffer& theBuffer, const std::string& theFileName)
{
	// Add your own signature checking code here
	return false;
}

bool SexyAppBase::LoadProperties(const std::string& theFileName, bool required, bool checkSig)
{
	Buffer aBuffer;
	if (!ReadBufferFromFile(theFileName, &aBuffer))
	{
		if (!required)
			return true;
		else
		{
			Popup(GetString("UNABLE_OPEN_PROPERTIES", _S("Unable to open properties file ")) + StringToSexyString(theFileName));
			return false;
		}
	}
	if (checkSig)
	{
		if (!CheckSignature(aBuffer, theFileName))
		{
			Popup(GetString("PROPERTIES_SIG_FAILED", _S("Signature check failed on ")) + StringToSexyString(theFileName + "'"));
			return false;
		}
	}

	PropertiesParser aPropertiesParser(this);

	// Load required language-file properties
	if (!aPropertiesParser.ParsePropertiesBuffer(aBuffer))
	{
		Popup(aPropertiesParser.GetErrorText());
		return false;
	}
	else
		return true;
}

bool SexyAppBase::LoadProperties()
{
	// Load required language-file properties
	if (!LoadProperties("properties\\default.xml", true, false))
		return false;

	// port: mods can add or replace strings, either with extra properties files (same XML format) or
	// inline in mod.json ("strings": {"EXCELLENT": "TRUE"}); later (higher-priority) mods win.
	Platform::ModManager& aMods = Platform::ModManager::Get();
	for (const std::string& aFile : aMods.GetPropertyFiles())
	{
		std::vector<uint8_t> aData;
		FILE* aFP = fopen(aFile.c_str(), "rb");
		if (aFP)
		{
			uint8_t aChunk[4096];
			size_t aRead;
			while ((aRead = fread(aChunk, 1, sizeof(aChunk), aFP)) > 0)
				aData.insert(aData.end(), aChunk, aChunk + aRead);
			fclose(aFP);
		}
		Buffer aBuffer;
		aBuffer.WriteBytes(aData.data(), (int)aData.size());
		PropertiesParser aPropertiesParser(this);
		if (aData.empty() || !aPropertiesParser.ParsePropertiesBuffer(aBuffer))
			SDL_Log("mod strings: cannot load %s %s", aFile.c_str(), aPropertiesParser.GetErrorText().c_str());
	}
	for (const auto& aString : aMods.GetStringOverrides())
		mStringProperties[aString.first] = StringToWString(aString.second);
	return true;
}

void SexyAppBase::LoadResourceManifest()
{
	if (!mResourceManager->ParseResourcesFile("properties\\resources.xml"))
		ShowResourceError(true);
}

void SexyAppBase::ShowResourceError(bool doExit)
{
	Popup(mResourceManager->GetErrorText());
	if (doExit)
		DoExit(0);
}

bool SexyAppBase::GetBoolean(const std::string& theId)
{
	StringBoolMap::iterator anItr = mBoolProperties.find(theId);

	if (anItr != mBoolProperties.end())
		return anItr->second;
	else
		return false;
}

bool SexyAppBase::GetBoolean(const std::string& theId, bool theDefault)
{
	StringBoolMap::iterator anItr = mBoolProperties.find(theId);

	if (anItr != mBoolProperties.end())
		return anItr->second;
	else
		return theDefault;
}

int SexyAppBase::GetInteger(const std::string& theId)
{
	StringIntMap::iterator anItr = mIntProperties.find(theId);

	if (anItr != mIntProperties.end())
		return anItr->second;
	else
		return false;
}

int SexyAppBase::GetInteger(const std::string& theId, int theDefault)
{
	StringIntMap::iterator anItr = mIntProperties.find(theId);

	if (anItr != mIntProperties.end())
		return anItr->second;
	else
		return theDefault;
}

double SexyAppBase::GetDouble(const std::string& theId)
{
	StringDoubleMap::iterator anItr = mDoubleProperties.find(theId);

	if (anItr != mDoubleProperties.end())
		return anItr->second;
	else
		return false;
}

double SexyAppBase::GetDouble(const std::string& theId, double theDefault)
{
	StringDoubleMap::iterator anItr = mDoubleProperties.find(theId);

	if (anItr != mDoubleProperties.end())
		return anItr->second;
	else
		return theDefault;
}

SexyString SexyAppBase::GetString(const std::string& theId)
{
	StringWStringMap::iterator anItr = mStringProperties.find(theId);

	if (anItr != mStringProperties.end())
		return WStringToSexyString(anItr->second);
	else
		return _S("");
}

SexyString SexyAppBase::GetString(const std::string& theId, const SexyString& theDefault)
{
	StringWStringMap::iterator anItr = mStringProperties.find(theId);

	if (anItr != mStringProperties.end())
		return WStringToSexyString(anItr->second);
	else
		return theDefault;
}

StringVector SexyAppBase::GetStringVector(const std::string& theId)
{
	StringStringVectorMap::iterator anItr = mStringVectorProperties.find(theId);

	if (anItr != mStringVectorProperties.end())
		return anItr->second;
	else
		return StringVector();
}

void SexyAppBase::SetString(const std::string& theId, const std::wstring& theValue)
{
	std::pair<StringWStringMap::iterator, bool> aPair = mStringProperties.insert(StringWStringMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

void SexyAppBase::SetBoolean(const std::string& theId, bool theValue)
{
	std::pair<StringBoolMap::iterator, bool> aPair = mBoolProperties.insert(StringBoolMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

void SexyAppBase::SetInteger(const std::string& theId, int theValue)
{
	std::pair<StringIntMap::iterator, bool> aPair = mIntProperties.insert(StringIntMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

void SexyAppBase::SetDouble(const std::string& theId, double theValue)
{
	std::pair<StringDoubleMap::iterator, bool> aPair = mDoubleProperties.insert(StringDoubleMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

//////////////////////////////////////////////////////////////////////////
// Command line

void SexyAppBase::SetCmdLine(int argc, char** argv)
{
	mCmdLineArgs.clear();
	for (int i = 1; i < argc; i++)
	{
		if (argv[i] != NULL)
			mCmdLineArgs.push_back(argv[i]);
	}
}

void SexyAppBase::DoParseCmdLine()
{
	// port: the arguments come from main() (SetCmdLine) instead of GetCommandLine(), already split
	for (int i = 0; i < (int) mCmdLineArgs.size(); i++)
	{
		const std::string& anArg = mCmdLineArgs[i];
		size_t anEqualsPos = anArg.find('=');

		std::string aName = anArg.substr(0, anEqualsPos);
		std::string aValue = (anEqualsPos == std::string::npos) ? "" : anArg.substr(anEqualsPos + 1);
		if ((aValue.length() >= 2) && (aValue[0] == '"') && (aValue[aValue.length() - 1] == '"'))
			aValue = aValue.substr(1, aValue.length() - 2);

		if (!aName.empty())
			HandleCmdLineParam(aName, aValue);
	}

	mCmdLineParsed = true;
}

void SexyAppBase::ParseCmdLine(const std::string& theCmdLine)
{
	// Command line example:  -play -demofile="game demo.dmo"
	// Results in HandleCmdLineParam("-play", ""); HandleCmdLineParam("-demofile", "game demo.dmo");
	std::string aCurParamName;
	std::string aCurParamValue;

	bool inQuote = false;
	bool onValue = false;

	for (int i = 0; i < (int) theCmdLine.length(); i++)
	{
		char c = theCmdLine[i];
		bool atEnd = false;

		if (c == '"')
		{
			inQuote = !inQuote;

			if (!inQuote)
				atEnd = true;
		}
		else if ((c == ' ') && (!inQuote))
			atEnd = true;
		else if (c == '=')
			onValue = true;
		else if (onValue)
			aCurParamValue += c;
		else
			aCurParamName += c;

		if (i == (int) theCmdLine.length() - 1)
			atEnd = true;

		if (atEnd && !aCurParamName.empty())
		{
			HandleCmdLineParam(aCurParamName, aCurParamValue);
			aCurParamName = "";
			aCurParamValue = "";
			onValue = false;
		}
	}
}

void SexyAppBase::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-changedir")
	{
		mChangeDirTo = theParamValue;
	}
	// port: test hooks (see mScript in SexyAppBase.h)
	else if (theParamName == "-seed")
	{
		mRandSeed = strtoul(theParamValue.c_str(), NULL, 10);
		mHasSeedParam = true;
	}
	else if (theParamName == "-script")
		mScriptFileName = theParamValue;
	else if (theParamName == "-fast")
		mFastMode = true;
	else if (theParamName == "-trace")
		mTraceMode = true;
	else if (theParamName == "-renderscale")
		mForcedRenderScale = atoi(theParamValue.c_str());
	// port: demo recording/playback (-play, -record, -demofile, ...), -crash and -screensaver are not
	// supported. Unknown parameters are ignored instead of ending the program, since OS launchers and
	// web shells may add their own.
}

//////////////////////////////////////////////////////////////////////////
// Test hooks (-seed, -script, -fast, -trace; see SexyAppBase.h)

DWORD SexyAppBase::GetClockTime()
{
	return mFastMode ? mFastClock : timeGetTime();
}

bool SexyAppBase::LoadScript(const std::string& theFileName)
{
	FILE* aFile = fopen(theFileName.c_str(), "r");
	if (aFile == NULL)
	{
		SDL_Log("script: cannot open %s", theFileName.c_str());
		return false;
	}

	mScript.clear();
	mScriptPos = 0;

	char aLine[1024];
	int aLineNum = 0;
	bool ok = true;
	while (fgets(aLine, sizeof(aLine), aFile) != NULL)
	{
		aLineNum++;
		std::string aText = aLine;
		size_t aComment = aText.find('#');
		if (aComment != std::string::npos)
			aText.erase(aComment);
		if (aText.find_first_not_of(" \t\r\n") == std::string::npos)
			continue;

		std::istringstream aStream(aText);
		ScriptCommand aCommand;
		aCommand.mLine = aLineNum;
		if ((!(aStream >> aCommand.mUpdateCount)) || (!(aStream >> aCommand.mCommand)))
		{
			SDL_Log("script: %s:%d: expected \"<update count> <command> [args]\"", theFileName.c_str(), aLineNum);
			ok = false;
			continue;
		}
		std::getline(aStream, aCommand.mArgs);
		size_t aStart = aCommand.mArgs.find_first_not_of(" \t");
		size_t anEnd = aCommand.mArgs.find_last_not_of(" \t\r\n");
		aCommand.mArgs = (aStart == std::string::npos) ? "" : aCommand.mArgs.substr(aStart, anEnd - aStart + 1);
		mScript.push_back(aCommand);
	}
	fclose(aFile);

	std::stable_sort(mScript.begin(), mScript.end(),
		[](const ScriptCommand& a, const ScriptCommand& b) { return a.mUpdateCount < b.mUpdateCount; });
	return ok;
}

// Runs the script commands that are due at this update.
void SexyAppBase::RunScript()
{
	while ((mScriptPos < mScript.size()) && (mScript[mScriptPos].mUpdateCount <= mUpdateCount) && (!mShutdown))
	{
		const ScriptCommand& aCommand = mScript[mScriptPos++];
		const std::string& aName = aCommand.mCommand;
		std::istringstream anArgs(aCommand.mArgs);
		int x = 0;
		int y = 0;

		if ((aName == "click") || (aName == "down") || (aName == "up") || (aName == "move"))
		{
			if (!(anArgs >> x >> y))
			{
				SDL_Log("script: line %d: %s needs <x> <y>", aCommand.mLine, aName.c_str());
				continue;
			}
			MouseMoveTo(x, y);
			if ((aName == "click") || (aName == "down"))
				mWidgetManager->MouseDown(x, y, 1);
			if ((aName == "click") || (aName == "up"))
				mWidgetManager->MouseUp(x, y, 1);
		}
		else if (aName == "key")
		{
			char* anEnd = NULL;
			long aNum = strtol(aCommand.mArgs.c_str(), &anEnd, 10);
			KeyCode aKeyCode = ((anEnd != aCommand.mArgs.c_str()) && (*anEnd == 0)) ? (KeyCode) aNum : GetKeyCodeFromName(aCommand.mArgs);
			if ((aKeyCode <= KEYCODE_UNKNOWN) || (aKeyCode >= 0xFF))
			{
				SDL_Log("script: line %d: unknown key '%s'", aCommand.mLine, aCommand.mArgs.c_str());
				continue;
			}
			mWidgetManager->KeyDown(aKeyCode);
			// like GetControlChar: these keys also type their control character (same value as the key code)
			if ((aKeyCode == KEYCODE_BACK) || (aKeyCode == KEYCODE_TAB) || (aKeyCode == KEYCODE_RETURN) || (aKeyCode == KEYCODE_ESCAPE))
				mWidgetManager->KeyChar((SexyChar) aKeyCode);
			mWidgetManager->KeyUp(aKeyCode);
		}
		else if ((aName == "char") && (!aCommand.mArgs.empty()))
			mWidgetManager->KeyChar((SexyChar) aCommand.mArgs[0]);
		else if ((aName == "screenshot") && (!aCommand.mArgs.empty()))
			SaveScreenshot(aCommand.mArgs);
		else if ((aName == "screenshotfull") && (!aCommand.mArgs.empty()))
			SaveScreenshot(aCommand.mArgs, true);
		else if (aName == "quit")
			Shutdown();
		else
			SDL_Log("script: line %d: unknown command '%s %s'", aCommand.mLine, aName.c_str(), aCommand.mArgs.c_str());
	}
}

// Draws the whole screen now and saves it as a PNG, at the logical size or (fullResolution) at the
// render target's size, which shows HD art (see -renderscale).
bool SexyAppBase::SaveScreenshot(const std::string& theFileName, bool fullResolution)
{
	MTAutoDisallowRand aDisallowRand;

	mIsDrawing = true;
	mDDInterface->BeginFrame();
	mWidgetManager->MarkAllDirty();
	mWidgetManager->DrawScreen();
	mIsDrawing = false;

	if (fullResolution)
	{
		std::vector<uint32_t> aFull;
		int aWidth, aHeight;
		if (!mDDInterface->ReadScreenFull(aFull, aWidth, aHeight))
		{
			SDL_Log("screenshot: cannot read the screen");
			return false;
		}
		std::vector<uchar> aRGBA((size_t) aWidth * aHeight * 4);
		for (size_t i = 0; i < aFull.size(); i++)
		{
			aRGBA[i*4 + 0] = (uchar) (aFull[i] >> 16);
			aRGBA[i*4 + 1] = (uchar) (aFull[i] >> 8);
			aRGBA[i*4 + 2] = (uchar) aFull[i];
			aRGBA[i*4 + 3] = 255;
		}
		bool success = stbi_write_png(theFileName.c_str(), aWidth, aHeight, 4, &aRGBA[0], aWidth * 4) != 0;
		SDL_Log("screenshot: %s at %dx%d (update %d)", theFileName.c_str(), aWidth, aHeight, mUpdateCount);
		return success;
	}

	std::vector<ulong> aBits(mWidth * mHeight);
	if (!mDDInterface->ReadScreen(&aBits[0]))
	{
		SDL_Log("screenshot: cannot read the screen");
		return false;
	}

	std::vector<uchar> aRGBA(mWidth * mHeight * 4);
	for (int i = 0; i < mWidth * mHeight; i++)
	{
		aRGBA[i*4 + 0] = (uchar) (aBits[i] >> 16);
		aRGBA[i*4 + 1] = (uchar) (aBits[i] >> 8);
		aRGBA[i*4 + 2] = (uchar) aBits[i];
		aRGBA[i*4 + 3] = 255;
	}

	if (!stbi_write_png(theFileName.c_str(), mWidth, mHeight, 4, &aRGBA[0], mWidth * 4))
	{
		SDL_Log("screenshot: cannot write %s", theFileName.c_str());
		return false;
	}
	SDL_Log("screenshot: %s (update %d)", theFileName.c_str(), mUpdateCount);
	return true;
}

//////////////////////////////////////////////////////////////////////////
// Init

void SexyAppBase::PreDisplayHook()
{
}

void SexyAppBase::PreDDInterfaceInitHook()
{
}

void SexyAppBase::PostDDInterfaceInitHook()
{
}

bool SexyAppBase::ChangeDirHook(const char* theIntendedPath)
{
	return false;
}

MusicInterface* SexyAppBase::CreateMusicInterface()
{
	if (mNoSoundNeeded)
		return new MusicInterface;
	else
		return new SDLMusicInterface();
}

void SexyAppBase::InitPropertiesHook()
{
}

void SexyAppBase::InitHook()
{
}

void SexyAppBase::Init()
{
	if (mShutdown)
		return;

	InitPropertiesHook();
	ReadFromRegistry();

	if (!mCmdLineParsed)
		DoParseCmdLine();

	// Change directory. port: the game data root is the VFS base dir (set up by main())
	if ((!mChangeDirTo.empty()) && (!ChangeDirHook(mChangeDirTo.c_str())))
		Platform::VFS::Get().SetBaseDir(mChangeDirTo);

	// port: -seed=N replaces the tick count
	if (!mHasSeedParam)
		mRandSeed = GetTickCount();
	SRand(mRandSeed);

	if ((!mScriptFileName.empty()) && (!LoadScript(mScriptFileName)))
		DoExit(1);

	srand(GetTickCount());

	// Let app do something before showing window, or switching to fullscreen mode
	// PreDisplayHook must call mWidgetManager->Resize if it changes mWidth or mHeight.
	PreDisplayHook();

	mWidgetManager->Resize(Rect(0, 0, mWidth, mHeight), Rect(0, 0, mWidth, mHeight));

	MakeWindow();

	if (mSoundManager == NULL)
		mSoundManager = new SDLSoundManager();

	SetSfxVolume(mSfxVolume);

	mMusicInterface = CreateMusicInterface();

	SetMusicVolume(mMusicVolume);

	InitHook();

	mInitialized = true;
}

void SexyAppBase::HandleGameAlreadyRunning()
{
	// port: no single-instance check
}

//////////////////////////////////////////////////////////////////////////
// Clipboard and cursors

void SexyAppBase::CopyToClipboard(const std::string& theString)
{
	SDL_SetClipboardText(Latin1ToUTF8(theString).c_str());
}

std::string SexyAppBase::GetClipboard()
{
	std::string aString;

	char* aText = SDL_GetClipboardText();
	if (aText != NULL)
	{
		aString = UTF8ToLatin1(aText);
		SDL_free(aText);
	}

	return aString;
}

void SexyAppBase::SetCursor(int theCursorNum)
{
	mCursorNum = theCursorNum;
	EnforceCursor();
}

int SexyAppBase::GetCursor()
{
	return mCursorNum;
}

void SexyAppBase::EnableCustomCursors(bool enabled)
{
	mCustomCursorsEnabled = enabled;
	EnforceCursor();
}

//////////////////////////////////////////////////////////////////////////
// Images

static_assert(sizeof(ulong) == sizeof(uint32_t), "the framework's ARGB pixels are 32-bit ulongs");

Sexy::DDImage* SexyAppBase::GetImage(const std::string& theFileName, bool commitBits)
{
	// Platform::LoadGameImage applies the SexyApp rules (extension probing, alpha masks) plus the mod
	// overlay and HD (@2x) variants
	Platform::LoadedImage aLoadedImage;
	if ((!Platform::LoadGameImage(theFileName, aLoadedImage, true)) || (aLoadedImage.mBits.empty()))
		return NULL;

	DDImage* anImage = new DDImage(mDDInterface);
	anImage->mFilePath = theFileName;
	anImage->SetBits((ulong*) &aLoadedImage.mBits[0], aLoadedImage.mWidth, aLoadedImage.mHeight, commitBits);
	if ((aLoadedImage.mHDScale > 1) && (!aLoadedImage.mHDBits.empty()))
		anImage->SetHDBits(&aLoadedImage.mHDBits[0], aLoadedImage.mHDWidth, aLoadedImage.mHDHeight, aLoadedImage.mHDScale);
	anImage->mFilePath = theFileName;

	return anImage;
}

Sexy::DDImage* SexyAppBase::CreateCrossfadeImage(Sexy::Image* theImage1, const Rect& theRect1, Sexy::Image* theImage2, const Rect& theRect2, double theFadeFactor)
{
	MemoryImage* aMemoryImage1 = dynamic_cast<MemoryImage*>(theImage1);
	MemoryImage* aMemoryImage2 = dynamic_cast<MemoryImage*>(theImage2);

	if ((aMemoryImage1 == NULL) || (aMemoryImage2 == NULL))
		return NULL;

	if ((theRect1.mX < 0) || (theRect1.mY < 0) ||
		(theRect1.mX + theRect1.mWidth > theImage1->GetWidth()) ||
		(theRect1.mY + theRect1.mHeight > theImage1->GetHeight()))
	{
		// Crossfade Rect1 out of bounds
		return NULL;
	}

	if ((theRect2.mX < 0) || (theRect2.mY < 0) ||
		(theRect2.mX + theRect2.mWidth > theImage2->GetWidth()) ||
		(theRect2.mY + theRect2.mHeight > theImage2->GetHeight()))
	{
		// Crossfade Rect2 out of bounds
		return NULL;
	}

	int aWidth = theRect1.mWidth;
	int aHeight = theRect1.mHeight;

	DDImage* anImage = new DDImage(mDDInterface);
	anImage->Create(aWidth, aHeight);

	ulong* aDestBits = anImage->GetBits();
	ulong* aSrcBits1 = aMemoryImage1->GetBits();
	ulong* aSrcBits2 = aMemoryImage2->GetBits();

	int aSrc1Width = aMemoryImage1->GetWidth();
	int aSrc2Width = aMemoryImage2->GetWidth();
	ulong aMult = (int) (theFadeFactor*256);
	ulong aOMM = (256 - aMult);

	for (int y = 0; y < aHeight; y++)
	{
		ulong* s1 = &aSrcBits1[(y+theRect1.mY)*aSrc1Width+theRect1.mX];
		ulong* s2 = &aSrcBits2[(y+theRect2.mY)*aSrc2Width+theRect2.mX];
		ulong* d = &aDestBits[y*aWidth];

		for (int x = 0; x < aWidth; x++)
		{
			ulong p1 = *s1++;
			ulong p2 = *s2++;

			*d++ =
				((((p1 & 0x000000FF)*aOMM + (p2 & 0x000000FF)*aMult)>>8) & 0x000000FF) |
				((((p1 & 0x0000FF00)*aOMM + (p2 & 0x0000FF00)*aMult)>>8) & 0x0000FF00) |
				((((p1 & 0x00FF0000)*aOMM + (p2 & 0x00FF0000)*aMult)>>8) & 0x00FF0000) |
				((((p1 >> 24)*aOMM + (p2 >> 24)*aMult)<<16) & 0xFF000000);
		}
	}

	anImage->BitsChanged();

	return anImage;
}

void SexyAppBase::ColorizeImage(Image* theImage, const Color& theColor)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);

	if (aSrcMemoryImage == NULL)
		return;

	ulong* aBits;
	int aNumColors;

	if (aSrcMemoryImage->mColorTable == NULL)
	{
		aBits = aSrcMemoryImage->GetBits();
		aNumColors = theImage->GetWidth()*theImage->GetHeight();
	}
	else
	{
		aBits = aSrcMemoryImage->mColorTable;
		aNumColors = 256;
	}

	if ((theColor.mAlpha <= 255) && (theColor.mRed <= 255) &&
		(theColor.mGreen <= 255) && (theColor.mBlue <= 255))
	{
		for (int i = 0; i < aNumColors; i++)
		{
			ulong aColor = aBits[i];

			aBits[i] =
				((((aColor & 0xFF000000) >> 8) * theColor.mAlpha) & 0xFF000000) |
				((((aColor & 0x00FF0000) * theColor.mRed) >> 8) & 0x00FF0000) |
				((((aColor & 0x0000FF00) * theColor.mGreen) >> 8) & 0x0000FF00)|
				((((aColor & 0x000000FF) * theColor.mBlue) >> 8) & 0x000000FF);
		}
	}
	else
	{
		for (int i = 0; i < aNumColors; i++)
		{
			ulong aColor = aBits[i];

			int aAlpha = ((aColor >> 24) * theColor.mAlpha) / 255;
			int aRed = (((aColor >> 16) & 0xFF) * theColor.mRed) / 255;
			int aGreen = (((aColor >> 8) & 0xFF) * theColor.mGreen) / 255;
			int aBlue = ((aColor & 0xFF) * theColor.mBlue) / 255;

			if (aAlpha > 255)
				aAlpha = 255;
			if (aRed > 255)
				aRed = 255;
			if (aGreen > 255)
				aGreen = 255;
			if (aBlue > 255)
				aBlue = 255;

			aBits[i] = (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | (aBlue);
		}
	}

	aSrcMemoryImage->BitsChanged();
}

DDImage* SexyAppBase::CreateColorizedImage(Image* theImage, const Color& theColor)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);

	if (aSrcMemoryImage == NULL)
		return NULL;

	DDImage* anImage = new DDImage(mDDInterface);

	anImage->Create(theImage->GetWidth(), theImage->GetHeight());

	ulong* aSrcBits;
	ulong* aDestBits;
	int aNumColors;

	if (aSrcMemoryImage->mColorTable == NULL)
	{
		aSrcBits = aSrcMemoryImage->GetBits();
		aDestBits = anImage->GetBits();
		aNumColors = theImage->GetWidth()*theImage->GetHeight();
	}
	else
	{
		aSrcBits = aSrcMemoryImage->mColorTable;
		aDestBits = anImage->mColorTable = new ulong[256];
		aNumColors = 256;

		anImage->mColorIndices = new uchar[anImage->mWidth*theImage->mHeight];
		memcpy(anImage->mColorIndices, aSrcMemoryImage->mColorIndices, anImage->mWidth*theImage->mHeight);
	}

	if ((theColor.mAlpha <= 255) && (theColor.mRed <= 255) &&
		(theColor.mGreen <= 255) && (theColor.mBlue <= 255))
	{
		for (int i = 0; i < aNumColors; i++)
		{
			ulong aColor = aSrcBits[i];

			aDestBits[i] =
				((((aColor & 0xFF000000) >> 8) * theColor.mAlpha) & 0xFF000000) |
				((((aColor & 0x00FF0000) * theColor.mRed) >> 8) & 0x00FF0000) |
				((((aColor & 0x0000FF00) * theColor.mGreen) >> 8) & 0x0000FF00)|
				((((aColor & 0x000000FF) * theColor.mBlue) >> 8) & 0x000000FF);
		}
	}
	else
	{
		for (int i = 0; i < aNumColors; i++)
		{
			ulong aColor = aSrcBits[i];

			int aAlpha = ((aColor >> 24) * theColor.mAlpha) / 255;
			int aRed = (((aColor >> 16) & 0xFF) * theColor.mRed) / 255;
			int aGreen = (((aColor >> 8) & 0xFF) * theColor.mGreen) / 255;
			int aBlue = ((aColor & 0xFF) * theColor.mBlue) / 255;

			if (aAlpha > 255)
				aAlpha = 255;
			if (aRed > 255)
				aRed = 255;
			if (aGreen > 255)
				aGreen = 255;
			if (aBlue > 255)
				aBlue = 255;

			aDestBits[i] = (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | (aBlue);
		}
	}

	anImage->BitsChanged();

	// port: HD pixels get the same colouring
	if (aSrcMemoryImage->HasValidHDBits())
	{
		std::vector<uint32_t> anHD(aSrcMemoryImage->mHDBits.size());
		for (size_t i = 0; i < anHD.size(); i++)
		{
			uint32_t aColor = aSrcMemoryImage->mHDBits[i];
			if ((theColor.mAlpha <= 255) && (theColor.mRed <= 255) && (theColor.mGreen <= 255) && (theColor.mBlue <= 255))
			{
				anHD[i] =
					((((aColor & 0xFF000000) >> 8) * theColor.mAlpha) & 0xFF000000) |
					((((aColor & 0x00FF0000) * theColor.mRed) >> 8) & 0x00FF0000) |
					((((aColor & 0x0000FF00) * theColor.mGreen) >> 8) & 0x0000FF00) |
					((((aColor & 0x000000FF) * theColor.mBlue) >> 8) & 0x000000FF);
			}
			else
			{
				int aAlpha = std::min(255, (int) ((aColor >> 24) * theColor.mAlpha) / 255);
				int aRed = std::min(255, (int) (((aColor >> 16) & 0xFF) * theColor.mRed) / 255);
				int aGreen = std::min(255, (int) (((aColor >> 8) & 0xFF) * theColor.mGreen) / 255);
				int aBlue = std::min(255, (int) ((aColor & 0xFF) * theColor.mBlue) / 255);
				anHD[i] = ((uint32_t) aAlpha << 24) | (aRed << 16) | (aGreen << 8) | aBlue;
			}
		}
		anImage->SetHDBits(&anHD[0], aSrcMemoryImage->mHDWidth, aSrcMemoryImage->mHDHeight, aSrcMemoryImage->mHDScale);
	}

	return anImage;
}

DDImage* SexyAppBase::CopyImage(Image* theImage, const Rect& theRect)
{
	DDImage* anImage = new DDImage(mDDInterface);

	anImage->Create(theRect.mWidth, theRect.mHeight);

	Graphics g(anImage);
	g.DrawImage(theImage, -theRect.mX, -theRect.mY);

	anImage->CopyAttributes(theImage);

	return anImage;
}

DDImage* SexyAppBase::CopyImage(Image* theImage)
{
	return CopyImage(theImage, Rect(0, 0, theImage->GetWidth(), theImage->GetHeight()));
}

void SexyAppBase::MirrorImage(Image* theImage)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);
	if (aSrcMemoryImage == NULL)
		return;

	ulong* aSrcBits = aSrcMemoryImage->GetBits();

	int aPhysSrcWidth = aSrcMemoryImage->mWidth;
	for (int y = 0; y < aSrcMemoryImage->mHeight; y++)
	{
		ulong* aLeftBits = aSrcBits + (y * aPhysSrcWidth);
		ulong* aRightBits = aLeftBits + (aPhysSrcWidth - 1);

		for (int x = 0; x < (aPhysSrcWidth >> 1); x++)
		{
			ulong aSwap = *aLeftBits;

			*(aLeftBits++) = *aRightBits;
			*(aRightBits--) = aSwap;
		}
	}

	aSrcMemoryImage->BitsChanged();
}

void SexyAppBase::FlipImage(Image* theImage)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);
	if (aSrcMemoryImage == NULL)
		return;

	ulong* aSrcBits = aSrcMemoryImage->GetBits();

	int aPhysSrcHeight = aSrcMemoryImage->mHeight;
	int aPhysSrcWidth = aSrcMemoryImage->mWidth;
	for (int x = 0; x < aPhysSrcWidth; x++)
	{
		ulong* aTopBits    = aSrcBits + x;
		ulong* aBottomBits = aTopBits + (aPhysSrcWidth * (aPhysSrcHeight - 1));

		for (int y = 0; y < (aPhysSrcHeight >> 1); y++)
		{
			ulong aSwap = *aTopBits;

			*aTopBits = *aBottomBits;
			aTopBits += aPhysSrcWidth;
			*aBottomBits = aSwap;
			aBottomBits -= aPhysSrcWidth;
		}
	}

	aSrcMemoryImage->BitsChanged();
}

void SexyAppBase::RotateImageHue(Sexy::MemoryImage* theImage, int theDelta)
{
	while (theDelta < 0)
		theDelta += 256;

	int aSize = theImage->mWidth * theImage->mHeight;
	ulong* aPtr = theImage->GetBits();
	for (int i=0; i<aSize; i++)
	{
		ulong aPixel = *aPtr;
		int alpha = aPixel&0xff000000;
		int r = (aPixel>>16)&0xff;
		int g = (aPixel>>8) &0xff;
		int b = aPixel&0xff;

		int maxval = std::max(r, std::max(g, b));
		int minval = std::min(r, std::min(g, b));
		int h = 0;
		int s = 0;
		int l = (minval+maxval)/2;
		int delta = maxval - minval;

		if (delta != 0)
		{
			s = (delta * 256) / ((l <= 128) ? (minval + maxval) : (512 - maxval - minval));

			if (r == maxval)
				h = (g == minval ? 1280 + (((maxval-b) * 256) / delta) :  256 - (((maxval - g) * 256) / delta));
			else if (g == maxval)
				h = (b == minval ?  256 + (((maxval-r) * 256) / delta) :  768 - (((maxval - b) * 256) / delta));
			else
				h = (r == minval ?  768 + (((maxval-g) * 256) / delta) : 1280 - (((maxval - r) * 256) / delta));

			h /= 6;
		}

		h += theDelta;
		if (h >= 256)
			h -= 256;

		double v= (l < 128) ? (l * (255+s))/255 :
				(l+s-l*s/255);

		int y = (int) (2*l-v);

		int aColorDiv = (6 * h) / 256;
		int x = (int)(y+(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
		if (x > 255)
			x = 255;

		int z = (int) (v-(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
		if (z < 0)
			z = 0;

		switch (aColorDiv)
		{
			case 0: r = (int) v; g = x; b = y; break;
			case 1: r = z; g= (int) v; b = y; break;
			case 2: r = y; g= (int) v; b = x; break;
			case 3: r = y; g = z; b = (int) v; break;
			case 4: r = x; g = y; b = (int) v; break;
			case 5: r = (int) v; g = y; b = z; break;
			default: r = (int) v; g = x; b = y; break;
		}

		*aPtr++ = alpha | (r<<16) | (g << 8) | (b);
	}

	theImage->BitsChanged();
}

ulong SexyAppBase::HSLToRGB(int h, int s, int l)
{
	int r;
	int g;
	int b;

	double v= (l < 128) ? (l * (255+s))/255 :
			(l+s-l*s/255);

	int y = (int) (2*l-v);

	int aColorDiv = (6 * h) / 256;
	int x = (int)(y+(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
	if (x > 255)
		x = 255;

	int z = (int) (v-(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
	if (z < 0)
		z = 0;

	switch (aColorDiv)
	{
		case 0: r = (int) v; g = x; b = y; break;
		case 1: r = z; g= (int) v; b = y; break;
		case 2: r = y; g= (int) v; b = x; break;
		case 3: r = y; g = z; b = (int) v; break;
		case 4: r = x; g = y; b = (int) v; break;
		case 5: r = (int) v; g = y; b = z; break;
		default: r = (int) v; g = x; b = y; break;
	}

	return 0xFF000000 | (r << 16) | (g << 8) | (b);
}

ulong SexyAppBase::RGBToHSL(int r, int g, int b)
{
	int maxval = std::max(r, std::max(g, b));
	int minval = std::min(r, std::min(g, b));
	int hue = 0;
	int saturation = 0;
	int luminosity = (minval+maxval)/2;
	int delta = maxval - minval;

	if (delta != 0)
	{
		saturation = (delta * 256) / ((luminosity <= 128) ? (minval + maxval) : (512 - maxval - minval));

		if (r == maxval)
			hue = (g == minval ? 1280 + (((maxval-b) * 256) / delta) :  256 - (((maxval - g) * 256) / delta));
		else if (g == maxval)
			hue = (b == minval ?  256 + (((maxval-r) * 256) / delta) :  768 - (((maxval - b) * 256) / delta));
		else
			hue = (r == minval ?  768 + (((maxval-g) * 256) / delta) : 1280 - (((maxval - r) * 256) / delta));

		hue /= 6;
	}

	return 0xFF000000 | (hue) | (saturation << 8) | (luminosity << 16);
}

void SexyAppBase::HSLToRGB(const ulong* theSource, ulong* theDest, int theSize)
{
	for (int i = 0; i < theSize; i++)
	{
		ulong src = theSource[i];
		theDest[i] = (src & 0xFF000000) | (HSLToRGB((src & 0xFF), (src >> 8) & 0xFF, (src >> 16) & 0xFF) & 0x00FFFFFF);
	}
}

void SexyAppBase::RGBToHSL(const ulong* theSource, ulong* theDest, int theSize)
{
	for (int i = 0; i < theSize; i++)
	{
		ulong src = theSource[i];
		theDest[i] = (src & 0xFF000000) | (RGBToHSL(((src >> 16) & 0xFF), (src >> 8) & 0xFF, (src & 0xFF)) & 0x00FFFFFF);
	}
}

// port: the run-length data is only used by the software blitters, which build it on demand. Additive
// and native-format data are not precached (the GPU path doesn't use them).
void SexyAppBase::PrecacheAdditive(MemoryImage* theImage)
{
}

void SexyAppBase::PrecacheAlpha(MemoryImage* theImage)
{
	theImage->GetRLAlphaData();
}

void SexyAppBase::PrecacheNative(MemoryImage* theImage)
{
}

//////////////////////////////////////////////////////////////////////////
// Sound

void SexyAppBase::PlaySample(int theSoundNum)
{
	if (!mSoundManager)
		return;

	SoundInstance* aSoundInstance = mSoundManager->GetSoundInstance(theSoundNum);
	if (aSoundInstance != NULL)
	{
		aSoundInstance->Play(false, true);
	}
}

void SexyAppBase::PlaySample(int theSoundNum, int thePan)
{
	if (!mSoundManager)
		return;

	SoundInstance* aSoundInstance = mSoundManager->GetSoundInstance(theSoundNum);
	if (aSoundInstance != NULL)
	{
		aSoundInstance->SetPan(thePan);
		aSoundInstance->Play(false, true);
	}
}

bool SexyAppBase::IsMuted()
{
	return mMuteCount > 0;
}

void SexyAppBase::Mute(bool autoMute)
{
	mMuteCount++;
	if (autoMute)
		mAutoMuteCount++;

	SetMusicVolume(mMusicVolume);
	SetSfxVolume(mSfxVolume);
}

void SexyAppBase::Unmute(bool autoMute)
{
	if (mMuteCount > 0)
	{
		mMuteCount--;
		if (autoMute)
			mAutoMuteCount--;
	}

	SetMusicVolume(mMusicVolume);
	SetSfxVolume(mSfxVolume);
}

double SexyAppBase::GetMusicVolume()
{
	return mMusicVolume;
}

void SexyAppBase::SetMusicVolume(double theVolume)
{
	mMusicVolume = theVolume;

	if (mMusicInterface != NULL)
		mMusicInterface->SetVolume((mMuteCount > 0) ? 0.0 : mMusicVolume);
}

double SexyAppBase::GetSfxVolume()
{
	return mSfxVolume;
}

void SexyAppBase::SetSfxVolume(double theVolume)
{
	mSfxVolume = theVolume;

	if (mSoundManager != NULL)
		mSoundManager->SetVolume((mMuteCount > 0) ? 0.0 : mSfxVolume);
}

double SexyAppBase::GetMasterVolume()
{
	return mSoundManager->GetMasterVolume();
}

void SexyAppBase::SetMasterVolume(double theMasterVolume)
{
	mSfxVolume = theMasterVolume;
	mSoundManager->SetMasterVolume(mSfxVolume);
}

//////////////////////////////////////////////////////////////////////////
// Image bookkeeping

void SexyAppBase::AddMemoryImage(MemoryImage* theMemoryImage)
{
	mMemoryImageSet.insert(theMemoryImage);
}

void SexyAppBase::RemoveMemoryImage(MemoryImage* theMemoryImage)
{
	MemoryImageSet::iterator anItr = mMemoryImageSet.find(theMemoryImage);
	if (anItr != mMemoryImageSet.end())
		mMemoryImageSet.erase(anItr);

	Remove3DData(theMemoryImage);
}

void SexyAppBase::Remove3DData(MemoryImage* theMemoryImage)
{
	if (mDDInterface)
		mDDInterface->RemoveImageData(theMemoryImage);
}

bool SexyAppBase::Is3DAccelerated()
{
	return (mDDInterface != NULL) && (mDDInterface->mIs3D);
}

bool SexyAppBase::Is3DAccelerationSupported()
{
	return Is3DAccelerated();
}

bool SexyAppBase::Is3DAccelerationRecommended()
{
	return Is3DAccelerated();
}

void SexyAppBase::Set3DAcclerated(bool is3D, bool reinit)
{
	// port: SDL's renderer is the only display path; it is accelerated whenever the system allows it
}

SharedImageRef SexyAppBase::GetSharedImage(const std::string& theFileName, const std::string& theVariant, bool* isNew)
{
	std::string anUpperFileName = StringToUpper(theFileName);
	std::string anUpperVariant = StringToUpper(theVariant);

	std::pair<SharedImageMap::iterator, bool> aResultPair;
	SharedImageRef aSharedImageRef;

	aResultPair = mSharedImageMap.insert(SharedImageMap::value_type(SharedImageMap::key_type(anUpperFileName, anUpperVariant), SharedImage()));
	aSharedImageRef = &aResultPair.first->second;

	if (isNew != NULL)
		*isNew = aResultPair.second;

	if (aResultPair.second)
	{
		// Pass in a '!' as the first char of the file name to create a new image
		if ((theFileName.length() > 0) && (theFileName[0] == '!'))
			aSharedImageRef.mSharedImage->mImage = new DDImage(mDDInterface);
		else
			aSharedImageRef.mSharedImage->mImage = GetImage(theFileName, false);
	}

	return aSharedImageRef;
}

void SexyAppBase::CleanSharedImages()
{
	if (mCleanupSharedImages)
	{
		// Delete shared images with reference counts of 0
		// This doesn't occur in ~SharedImageRef because sometimes we can not only access the image
		//  through the SharedImageRef returned by GetSharedImage, but also by calling GetSharedImage
		//  again with the same params -- so we can have instances where we do the 'final' deref on
		//  an image but immediately re-request it via GetSharedImage
		SharedImageMap::iterator aSharedImageItr = mSharedImageMap.begin();
		while (aSharedImageItr != mSharedImageMap.end())
		{
			SharedImage* aSharedImage = &aSharedImageItr->second;
			if (aSharedImage->mRefCount == 0)
			{
				delete aSharedImage->mImage;
				mSharedImageMap.erase(aSharedImageItr++);
			}
			else
				++aSharedImageItr;
		}

		mCleanupSharedImages = false;
	}
}
