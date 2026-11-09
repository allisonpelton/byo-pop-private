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
#include "ModMenu.h"
#include "WebSettings.h"
#include "../platform/VFS.h"
#include "../platform/ImageLoader.h"
#include "../platform/ModManager.h"
#include "../platform/WebPersist.h"
#include "../platform/UserFiles.h"
#include "../platform/Relaunch.h"
#include "../platform/Placeholders.h"
#include "../platform/Json.h"
#include "../platform/Audio.h"

#include <SDL.h>
#include <stb_image_write.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <sys/stat.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/heap.h>
#include <malloc.h>
#include "WebProfiles.h"
#endif

// port: the player-only page's hold on the updates (byopop_hold_updates_at, below), web only; -1 holds nothing
#ifdef __EMSCRIPTEN__
static int gWebHoldAt = -1;
static bool WebUpdatesHeld(int theUpdateCount) { return (gWebHoldAt >= 0) && (theUpdateCount >= gWebHoldAt); }
// port: the player-only page's budget for one browser frame (byopop_set_frame_budget, below), web only; 0 sets none
static double gWebFrameBudget = 0;
#else
static bool WebUpdatesHeld(int) { return false; }
#endif

using namespace Sexy;

SexyAppBase* Sexy::gSexyAppBase = NULL;

static SDL_Cursor* gSystemCursors[SDL_NUM_SYSTEM_CURSORS];

// (always false where the hook does not exist)
static bool gPostedInput = false;
#ifdef _WIN32
// port: a test run played with posted window messages (BYOPOP_POSTED_INPUT, set by a driver such as Twist's realplay)
// on rundesk's private desktop, where the real cursor is never over the window. SDL answers each posted move with
// TrackMouseEvent, and the WM_MOUSELEAVE that follows at once moves its mouse to GetCursorPos (to the top edge) and drops
// the mouse focus, so a hover never lasts to the next update. Such a run notes where each posted mouse message puts the
// mouse (SDL's message hook), keeps only the moves to that point and ignores the window's leave event.
static int gPostedMouseX = -1;
static int gPostedMouseY = -1;

static void SDLCALL PostedInputHook(void* theUserData, void* theHWnd, unsigned int theMessage, Uint64 theWParam,
	Sint64 theLParam)
{
	(void) theUserData;
	(void) theHWnd;
	(void) theWParam;
	if ((theMessage >= 0x200) && (theMessage <= 0x20E) && (theMessage != 0x20A) && (theMessage != 0x20E))
	{
		// WM_MOUSEMOVE to WM_MBUTTONDBLCLK and the X buttons: the client point is in lParam (not for the wheels)
		gPostedMouseX = (short) (theLParam & 0xFFFF);
		gPostedMouseY = (short) ((theLParam >> 16) & 0xFFFF);
	}
}
#endif

//////////////////////////////////////////////////////////////////////////
// Helpers

// Platform::UserDataChanged (platform/UserFiles.h): a user file the platform layer wrote (mods.json,
// mod-settings.json, mods-status.json) goes into the next localStorage backup like the game's own saves
static void OnUserDataChanged()
{
	if (gSexyAppBase != NULL)
		gSexyAppBase->mBackupDirty = true;
}

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

// port: -realinput (SexyAppBase.h). While RunRealInput pushes an event this is set, and the filter lets through only
// those of the kinds it plays: the system's own mouse, keyboard, text and focus events never reach the game
static bool gPushingRealInput = false;
static bool gRealInputFilterSet = false;

static bool IsPlayedInput(const SDL_Event& theEvent)
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
	case SDL_TEXTEDITING:
	case SDL_FINGERDOWN:
	case SDL_FINGERUP:
	case SDL_FINGERMOTION:
		return true;
	case SDL_WINDOWEVENT:
		return (theEvent.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) || (theEvent.window.event == SDL_WINDOWEVENT_FOCUS_LOST) ||
			(theEvent.window.event == SDL_WINDOWEVENT_TAKE_FOCUS) || (theEvent.window.event == SDL_WINDOWEVENT_ENTER) ||
			(theEvent.window.event == SDL_WINDOWEVENT_LEAVE);
	}
	return false;
}

static int SDLCALL RealInputFilter(void* theUserData, SDL_Event* theEvent)
{
	(void) theUserData;
	return ((gPushingRealInput) || (!IsPlayedInput(*theEvent))) ? 1 : 0;
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
// Module.sexySyncBusy/sexySyncPending to clear before offering a reload. port: the argument says whether the
// game asked to restart (SexyAppBase::RequestRestart: the page then reloads and starts the game again) and
// which of its arguments a restart drops (e.g. "-gameseed": the page's ?seed= goes too).
EM_JS(void, sexyapp_notify_exit, (int isRestart, const char* theDroppedJson), {
	if (typeof Module['onGameExit'] === 'function')
		Module['onGameExit']({ restart: !!isRestart, dropped: JSON.parse(UTF8ToString(theDroppedJson)) });
});

// port: the game has loaded (its loading "thread" completed). The page takes a start that got this far as one
// that no mod broke; one that never does is offered a start without mods next time (web/modpanel.js).
EM_JS(void, sexyapp_notify_loaded, (), {
	if (typeof Module['onGameLoaded'] === 'function')
		Module['onGameLoaded']();
});

// port: the player-only page's test hooks (web/testkit.js; docs/WEB_PLAYER.md, "The player-only page"), web only. The page
// reads the update count, and may hold the updates at a count: pause holds them where they are, a step of n moves the hold n
// on. -1, the start and every run but a test that holds them, leaves the loop as it was.
extern "C"
{
EMSCRIPTEN_KEEPALIVE int byopop_update_count()
{
	return (gSexyAppBase != NULL) ? gSexyAppBase->mUpdateCount : -1;
}

EMSCRIPTEN_KEEPALIVE void byopop_hold_updates_at(int theUpdate)
{
	gWebHoldAt = theUpdate;
}

// port: the player-only page's cap on one browser frame (web/testkit.js, fast runs): the frame returns to the browser once its
// steps took theMs, between two steps of the update state machine, which carries on in the next frame as it would have in this
// one, so that the page's own JS is served between frames. 0 (the default) sets no cap. It changes no update and no draw.
EMSCRIPTEN_KEEPALIVE void byopop_set_frame_budget(double theMs)
{
	gWebFrameBudget = theMs;
}

// port: the site's game speed setting (web/controls.js; docs/WEB_PLAYER.md, "Game speed"), web only: SexyAppFramework's own
// update multiplier, which runs mFrameTime's updates theSpeed times as often in real time (Process). 1, the default, and every
// run but a player's choice, leaves the game as it was. Held to 0.25 to 4. A game whose own host runs its loop (Bonnie's
// Bookstore, Ning Po Mahjong) has no SexyAppBase and is not reached; nor is a timer a game reads from the wall clock itself.
EMSCRIPTEN_KEEPALIVE void byopop_set_speed(double theSpeed)
{
	if (gSexyAppBase == NULL)
		return;
	if (!(theSpeed >= 0.25))
		theSpeed = (theSpeed > 0) ? 0.25 : 1;
	if (theSpeed > 4)
		theSpeed = 4;
	gSexyAppBase->mUpdateMultiplier = theSpeed;
}

EMSCRIPTEN_KEEPALIVE double byopop_speed()
{
	return (gSexyAppBase != NULL) ? gSexyAppBase->mUpdateMultiplier : 0;
}

// port: the site's performance panel (web/perf.js; docs/WEB_PLAYER.md, "The performance panel"), web only and read only: the
// images the framework keeps (its MemoryImage set) with the bytes of their pixels and the blitters' caches, the loaded
// sounds with the bytes of their samples, and the wasm heap's size and the bytes malloc has handed out, as one JSON line. The panel calls it about once a second while it is open; nothing
// else does.
EMSCRIPTEN_KEEPALIVE const char* byopop_perf_json()
{
	static std::string aJson;
	if (gSexyAppBase == NULL)
		return "null";
	size_t anImageBytes = 0;
	for (MemoryImage* anImage : gSexyAppBase->mMemoryImageSet)
	{
		size_t aPixels = (size_t) std::max(anImage->mWidth, 0) * (size_t) std::max(anImage->mHeight, 0);
		if (anImage->mBits != NULL) anImageBytes += aPixels * sizeof(ulong);
		if (anImage->mColorTable != NULL) anImageBytes += 256 * sizeof(ulong);
		if (anImage->mColorIndices != NULL) anImageBytes += aPixels;
		if (anImage->mNativeAlphaData != NULL) anImageBytes += aPixels * sizeof(ulong);
		if (anImage->mRLAlphaData != NULL) anImageBytes += aPixels;
		if (anImage->mRLAdditiveData != NULL) anImageBytes += aPixels;
		anImageBytes += anImage->mHDBits.capacity() * sizeof(uint32_t);
	}
	int aSounds = 0;
	size_t aSoundBytes = 0;
	SDLSoundManager* aSoundManager = dynamic_cast<SDLSoundManager*>(gSexyAppBase->mSoundManager);
	if (aSoundManager != NULL)
	{
		aSounds = aSoundManager->GetNumSounds();
		aSoundBytes = aSoundManager->GetSoundBytes();
	}
	struct mallinfo anInfo = mallinfo();
	char aBuf[320];
	snprintf(aBuf, sizeof(aBuf), "{\"updates\":%d,\"images\":%d,\"imageBytes\":%zu,\"sounds\":%d,\"soundBytes\":%zu,\"heap\":%zu,\"heapUsed\":%zu}",
		gSexyAppBase->mUpdateCount, (int) gSexyAppBase->mMemoryImageSet.size(), anImageBytes, aSounds, aSoundBytes,
		(size_t) emscripten_get_heap_size(), (size_t) anInfo.uordblks);
	aJson = aBuf;
	return aJson.c_str();
}
}

// port: the player-only page's frame hook (web/testkit.js, Module.byopopTestFrame), after each browser frame's updates and
// draw (whether it drew); no other page sets it. (The runtime's own Module.postMainLoop is compiled out of these builds.)
// It also gets the time the frame's updates and draw took (ms), which the performance panel shows (web/perf.js).
EM_JS(void, sexyapp_test_frame, (int theDrew, double theMs), {
	if (Module.byopopTestFrame) Module.byopopTestFrame(theDrew != 0, theMs);
});

EM_JS(void, sexyapp_alert, (const char* theText), {
	alert(UTF8ToString(theText));
});

EM_JS(int, sexyapp_confirm, (const char* theText), {
	return confirm(UTF8ToString(theText)) ? 1 : 0;
});

// port: a browser enters full screen only when it allows it: the page may have it (an embedding page can forbid it), and a
// gesture of the player's is still active (a key or a click in the last few seconds). Asked otherwise, SDL's window took the
// screen's size at once and the game was drawn for it, several times too big in its 640x480 canvas with its clicks off (TipTop's
// Alt+Enter from a test's DOM events, 2026-11-08), while Emscripten held the request for a gesture to come. So SwitchScreenMode
// asks only when this says the browser would take the request now; Emscripten's own test is the same user activation.
EM_JS(int, sexyapp_fullscreen_allowed, (), {
	if (!document.fullscreenEnabled && !document.webkitFullscreenEnabled) return 0;
	return (!navigator.userActivation || navigator.userActivation.isActive) ? 1 : 0;
});

// port: a browser that takes the request may still refuse it, and answers later (the Browser pane refuses every one with
// "Permissions check failed"). Emscripten sizes the canvas for the screen before it asks, and puts it back only on the
// fullscreenchange that leaves full screen, which a refusal never sends. So while a request is out (theWatch), its refusal is
// taken as leaving full screen at once: a fullscreenchange with no full-screen element, on which Emscripten's own restore gives
// the canvas its size back and SDL's own handler makes its window windowed, as when the player leaves full screen;
// SDL_WINDOWEVENT_SIZE_CHANGED then puts the framework back in its windowed geometry.
EM_JS(void, sexyapp_watch_fullscreen_refusal, (int theWatch), {
	if (Module.byopopFullscreenWatch) Module.byopopFullscreenWatch();
	if (!theWatch) return;
	var done = function () {
		document.removeEventListener('fullscreenchange', done);
		document.removeEventListener('fullscreenerror', refused);
		Module.byopopFullscreenWatch = null;
	};
	var refused = function () {
		done();
		if (!document.fullscreenElement) document.dispatchEvent(new Event('fullscreenchange'));
	};
	document.addEventListener('fullscreenchange', done);
	document.addEventListener('fullscreenerror', refused);
	Module.byopopFullscreenWatch = done;
});

// port: the page may hold the game's own full screen itself (the site's Desktop 95 puts the game's window in full screen as its
// maximize button does, a whole multiple of the game's size centred on black, web/shell.html onGameFullscreen; the user,
// 2026-11-08: "Full screen issue seems to be the difference between using the in-game full screen and pressing F11"). SDL's own
// full screen makes the canvas the full-screen element at the screen's size, which the page's window cannot frame. 1 when the
// page took the request (theFull: 1 to enter, 0 to leave), 0 to leave it to SDL.
EM_JS(int, sexyapp_page_fullscreen, (int theFull), {
	return (Module.onGameFullscreen && Module.onGameFullscreen(!!theFull)) ? 1 : 0;
});

// 1 while the page holds the game's full screen or has asked for it, 0 otherwise, -1 for a page with no full screen of its own
EM_JS(int, sexyapp_page_fullscreen_state, (), {
	return Module.gameFullscreenState ? (Module.gameFullscreenState() ? 1 : 0) : -1;
});

// the page took the game's last request for full screen: the game's windowed state follows the page's (UpdateAppStep), as the
// player may leave it, or the browser refuse it, without the canvas changing size
static bool gPageFullscreen = false;

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
	Platform::SetUserDataChangedCallback(&OnUserDataChanged);

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
	mUserChanged3DSetting = false;
	mWant3D = true;
	mDebugKeysEnabled = false;
	mNoSoundNeeded = false;
	mWantFMod = false;

	mSyncRefreshRate = 100;
	mVSyncUpdates = false;
	mWaitForVSync = false;
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
	mDrawingLogicalShot = false;
	mTraceMode = false;
	mDrawEvery = false;
	mFastClock = 0;
	mLastFastDrawUpdate = 0;
	mScriptPos = 0;
	mScriptedInput = false;
	mRealInputPos = 0;
	mRealMouseX = -1;
	mRealMouseY = -1;
	mRealButtons = 0;
	mRealClicks = 1;
	mRealMouseIn = false;
	mRealMods = 0;
	mCommandsFilePos = 0;
	mCommandsFileLine = 0;
	mCommandsWaitLogged = false;
	mLiveCommandCount = 0;
	mCommandsHold = false;
	mLiveRunUntil = 0;
	mLiveWaitUntil = -1;
	mModMenu = NULL;
	mModMenuKeyDown = false;
	mModMenuClosingKey = KEYCODE_UNKNOWN;
	mKeyInHandling = KEYCODE_UNKNOWN;
	mLastKeyPressed = KEYCODE_UNKNOWN;
	mRestartRequested = false;
	mActivationClickPending = false;
	mActivatedTime = 0;
	mActivationButtons = 0;

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

	// port: the mod menu, then its fonts and their image (while the display still exists)
	if (mModMenu != NULL)
	{
		mWidgetManager->RemoveWidget(mModMenu);
		delete mModMenu;
		mModMenu = NULL;
	}
	ModMenu::FreeResources();

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
	// port: close the audio device now. Nothing closed it before, so SDL's audio thread kept calling the
	// mixer until the process ended and could mix into the destroyed static Mixer (a rare crash at exit)
	Platform::Mixer::Get().Shutdown();

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

	Platform::SetUserDataChangedCallback(NULL);
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

		// port: a test run (tools/rundesk sets BYOPOP_TEST_RUN, the oracle BEJ_TEST_DESKTOP) never hands a
		// URL to the system: its window is hidden, but a browser or the Steam client (the Steam builds' links
		// are steam:// URLs) would open on the user's desktop. It logs the URL and carries on as if opened.
		if ((SDL_getenv("BYOPOP_TEST_RUN") != NULL) || (SDL_getenv("BEJ_TEST_DESKTOP") != NULL))
		{
			SDL_Log("OpenURL in a test run, not opened: %s", theURL.c_str());
			URLOpenSucceeded(theURL);
			return true;
		}

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
	// port: the 3D choice, once the player has made one (the original writes it the same way); a run that never changed
	// it leaves a saved value as it was
	if (mUserChanged3DSetting)
		RegistryWriteBoolean("Is3D", mWant3D);
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

void SexyAppBase::GetShutdownSaves(std::vector<ShutdownSave>* /*theSaves*/)
{
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

	std::vector<ShutdownSave> aSaves;
	GetShutdownSaves(&aSaves);
	bool hasSave = !aSaves.empty();
	std::string aSave;
	for (size_t i = 0; i < aSaves.size(); i++)
	{
		aSave += aSaves[i].mFileName;
		aSave += '\0';
		aSave += std::to_string(aSaves[i].mData.size()) + ':' + aSaves[i].mData;
	}
	std::string aRegistry = GetShutdownRegistryText();

	bool saveChanged = (hasSave != mBackupHasSave) || (aSave != mBackupSave);
	bool saveDue = (hasSave != mBackupHasSave) || (aTime - mLastBackupTime >= 2000);
	if ((!force) && (!mBackupDirty) && (aRegistry == mBackupRegistry) && ((!saveChanged) || (!saveDue)))
		return;

	std::vector<Platform::BackupFile> aFiles;
	if (!aRegistry.empty())
		aFiles.push_back(Platform::BackupFile{ UserDirRelativePath(RegistryPath()), aRegistry });
	for (size_t i = 0; i < aSaves.size(); i++)
	{
		std::string aRelative = UserDirRelativePath(aSaves[i].mFileName);
		if (!aRelative.empty())
			aFiles.push_back(Platform::BackupFile{ aRelative, aSaves[i].mData });
		else
		{
			// a name outside the user folder (an absolute path, say) cannot be kept: said once, so that a lost save shows in
			// the log (Bejeweled Twist's first hook gave absolute names, 2026-11-08)
			static std::set<std::string> sLogged;
			if (sLogged.insert(aSaves[i].mFileName).second)
				SDL_Log("shutdown save outside the user folder, not kept: %s", aSaves[i].mFileName.c_str());
		}
	}
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
	if (mRealInputPos < mRealInput.size())
		RunRealInput();
	// -commands: a look at the file each update, every few with -fast, where updates come back to back
	if ((!mCommandsFileName.empty()) && ((!mFastMode) || ((mUpdateCount % FAST_DRAW_INTERVAL) == 0)))
		PollLiveCommands();
	if ((!mLiveCommands.empty()) || (mLiveWaitUntil >= 0))
		RunLiveCommands();

	// port: a test run played with real input (a driver posting window messages, which a -script run ignores) asks for
	// a picture by writing the file BYOPOP_SHOT_REQUEST names, holding the picture's path; the game takes it and deletes
	// the file. Looked for every 10 updates once the game has loaded, only in a test run and when the variable is set.
	// (a copy: SDL_getenv's buffer on Windows is reused by its next call)
	static const std::string aShotRequest = SDL_getenv("BYOPOP_SHOT_REQUEST") ? SDL_getenv("BYOPOP_SHOT_REQUEST") : "";
	if ((!aShotRequest.empty()) && (mLoaded) && ((mUpdateCount % 10) == 0) && (IsTestRun()))
	{
		if (FILE* aFile = fopen(aShotRequest.c_str(), "rb"))
		{
			char aPath[1024] = {};
			size_t aLength = fread(aPath, 1, sizeof(aPath) - 1, aFile);
			fclose(aFile);
			while ((aLength > 0) && ((unsigned char) aPath[aLength - 1] <= 32))
				aPath[--aLength] = 0;
			remove(aShotRequest.c_str());
			if (aLength > 0)
				SaveScreenshot(aPath);
		}
	}

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
		Platform::ReportMissing();
#ifdef __EMSCRIPTEN__
		sexyapp_notify_loaded();
		Byopop::SiteLoaded(this);	// the page's players (framework/WebProfiles.h): the game's start point, if it has one
#endif
	}

	UpdateFrames();

	// port: under -fast the sound effects play on the game's clock, so that a sound ends after as many updates as it
	// lasts and not after as much real time (Platform::Mixer::SetGameClocked, set in Init): each update moves them on
	// by the game time it stands for
	if (mFastMode)
		Platform::Mixer::Get().AdvanceVoices(mFrameTime / mUpdateMultiplier);
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

	// port: a GPU display (3D, no SDL renderer) draws nothing while its window is minimised. No scene may begin then (the
	// original's PreDraw fails), so every draw would fall to the software path, which reads the whole screen back for
	// each draw and blits from images whose pixels live only on the GPU (a crash in NormalBlt). The window shows nothing
	// and the next frame is drawn whole, so the frame is skipped.
	if ((mDDInterface->mIs3D) && (mDDInterface->mRenderer == NULL) && (mHWnd != NULL) &&
		((SDL_GetWindowFlags(mHWnd) & SDL_WINDOW_MINIMIZED) != 0))
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

	// port: in a test run nobody can answer a message box (under rundesk it opens on the invisible desktop), so
	// it would wait forever: the log line is the whole message, and the answer is No, Cancel or OK.
	if (IsTestRun())
	{
		SDL_Log("%s: %s (a test run: not shown)", theTitle.c_str(), theText.c_str());
		EndPopup();
		return (aButtons == MB_YESNO) ? IDNO : (aButtons == MB_OKCANCEL) ? IDCANCEL : IDOK;
	}

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
		// port: a test run shows no box (see MsgBox), only this line
		SDL_Log("%s: %s%s", aTitle.c_str(), theString.c_str(), IsTestRun() ? " (a test run: not shown)" : "");

		if (!IsTestRun())
		{
#ifdef __EMSCRIPTEN__
			sexyapp_alert(Latin1ToUTF8(aTitle + "\n\n" + theString).c_str());
#else
			SDL_Window* aParent = ((mHWnd != NULL) && (SDL_GetWindowFlags(mHWnd) & SDL_WINDOW_SHOWN)) ? mHWnd : NULL;
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, Latin1ToUTF8(aTitle).c_str(), Latin1ToUTF8(theString).c_str(), aParent);
#endif
		}
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

void SexyAppBase::WarpMouse(int theX, int theY)
{
#ifdef __EMSCRIPTEN__
	(void) theX;
	(void) theY;
#else
	if (IsTestRun() || (mHWnd == NULL) || (mDDInterface == NULL))
		return;
	int aWindowX;
	int aWindowY;
	mDDInterface->LogicalToWindow(theX, theY, &aWindowX, &aWindowY);
	SDL_WarpMouseInWindow(mHWnd, aWindowX, aWindowY);
#endif
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
	// game) are ignored too, so the run is the same whatever happens to the window (also while a test
	// bot drives the game, see mScriptedInput). A -script run played with posted input (gPostedInput) keeps its mouse and
	// keys, but not the focus changes: its driver asks the game through the script (the test commands) and plays with
	// real messages, on a desktop where the window is never active. A -script run is one with a script file (natively and in
	// a web build under Node; the web page never passes one): a test command that retries through AddScriptCommand also puts
	// lines in mScript, and those must not stop real input (one waiting goto on the live channel kept every real click out of
	// Bookworm Adventures, 2026-11-08).
	if ((!mScriptFileName.empty()) || (mScriptedInput))
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
			if ((gPostedInput) && (!mScriptedInput))
				break;
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
	// The press that activates the page is not passed on, nor its release: natively SDL drops a click
	// that activates the window (SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH), and a game that pauses in the
	// background (Big Money) took the click that resumed it as a move. The browser can send its focus
	// event just before that press, so a press soon after the app became active counts too. A touch
	// arms it the same way, for the mouse press SDL makes of it.
	if ((!mActive) && (!mSEHOccured) && (!mShutdown))
	{
		bool isPress = (theEvent.type == SDL_MOUSEBUTTONDOWN) || (theEvent.type == SDL_FINGERDOWN);
		if ((isPress) || ((theEvent.type == SDL_WINDOWEVENT) && (theEvent.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)))
		{
			mActivationClickPending = true;
			mActivatedTime = theEvent.common.timestamp;
		}

		if ((isPress) || (theEvent.type == SDL_KEYDOWN))
		{
			mActive = true;
			RehupFocus();
		}
	}

	if ((theEvent.type == SDL_MOUSEBUTTONDOWN) && (mActivationClickPending))
	{
		mActivationClickPending = false;
		if ((DWORD) (theEvent.button.timestamp - mActivatedTime) <= 150)
		{
			mActivationButtons |= SDL_BUTTON(theEvent.button.button);
			return;
		}
	}
	else if ((theEvent.type == SDL_MOUSEBUTTONUP) && ((mActivationButtons & SDL_BUTTON(theEvent.button.button)) != 0))
	{
		mActivationButtons &= ~SDL_BUTTON(theEvent.button.button);
		return;
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
#ifdef __EMSCRIPTEN__
			// SDL's window stays windowed while the page holds the full screen
			if ((gPageFullscreen) && (sexyapp_page_fullscreen_state() == 1))
				mIsWindowed = false;
#endif
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
#ifdef _WIN32
			if (gPostedInput)
				break;  // (PostedInputHook)
#endif
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
#ifdef _WIN32
		// port: a run played with posted window messages keeps only the moves that were posted (PostedInputHook)
		if ((gPostedInput) && (theEvent.type == SDL_MOUSEMOTION) &&
			((theEvent.motion.x != gPostedMouseX) || (theEvent.motion.y != gPostedMouseY)))
			break;
#endif
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

			// port: F2 opens and closes the mod menu (docs/MOD_MENU_DESIGN.md 5.1): a press without modifiers
			// that is not a key repeat. The game sees neither that press nor its release. (A -script run never
			// gets here: its real input is dropped above.)
			if ((aKeyCode == KEYCODE_F2) && (mInitialized) && (!mShutdown))
			{
				if ((keyDown) && (!theEvent.key.repeat) &&
					((theEvent.key.keysym.mod & (KMOD_CTRL | KMOD_SHIFT | KMOD_ALT | KMOD_GUI)) == 0))
				{
					mModMenuKeyDown = true;
					if (mModMenu != NULL)
					{
						mModMenu->CommitEdit();
						CloseModMenu();
					}
					else
						OpenModMenu();
					break;
				}
				if (mModMenuKeyDown)
				{
					if (!keyDown)
						mModMenuKeyDown = false;
					break;
				}
			}

			// port: the key that closed the mod menu, held down: its repeats would reach the game below it
			// (Bejeweled's board pauses on any character). They go nowhere until it is released.
			if ((mModMenuClosingKey != KEYCODE_UNKNOWN) && (aKeyCode == mModMenuClosingKey))
			{
				if ((keyDown) && (theEvent.key.repeat))
					break;
				mModMenuClosingKey = KEYCODE_UNKNOWN;
			}
			if (keyDown)
				mLastKeyPressed = aKeyCode;
			else if (aKeyCode == mLastKeyPressed)
				mLastKeyPressed = KEYCODE_UNKNOWN;

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

			mKeyInHandling = aKeyCode;
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
			mKeyInHandling = KEYCODE_UNKNOWN;
		}
		break;

	case SDL_TEXTINPUT:
		{
			mLastUserInputTick = timeGetTime();

			// port: the text a held key that closed the mod menu types again (Space on its Close button; its
			// repeated presses are dropped above, so no other key was pressed since)
			if ((mModMenuClosingKey != KEYCODE_UNKNOWN) && (mModMenuClosingKey == mLastKeyPressed))
				break;
			mKeyInHandling = mLastKeyPressed;
			std::string aText = UTF8ToLatin1(theEvent.text.text);
			for (size_t i = 0; i < aText.length(); i++)
				mWidgetManager->KeyChar((SexyChar) aText[i]);
			mKeyInHandling = KEYCODE_UNKNOWN;
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

DDInterface* SexyAppBase::CreateDDInterface()
{
	return new DDInterface(this);
}

bool SexyAppBase::DisplayInitFailed(int theResult)
{
	(void)theResult;
	return false;
}

uint32_t SexyAppBase::GetExtraWindowFlags()
{
	return 0;
}

void SexyAppBase::MakeWindow()
{
	if (mHWnd == NULL)
	{
#ifndef __EMSCRIPTEN__
		// HiDPI: let Windows report real pixels instead of bitmap-stretching the window (only effective
		// when main() hasn't initialized SDL video already)
		SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
		SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
		// Full screen is a borderless window over the whole desktop at the desktop's own mode (SDL_WINDOW_FULLSCREEN_DESKTOP,
		// below and in SwitchScreenMode), never the originals' exclusive mode change (the user, 2026-11-07: "I'd like all
		// the native ports to have their fullscreen modes made borderless without mods"). Alt-Tab only moves the focus:
		// the window is not minimized when it loses it, whatever SDL's default.
		SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
#endif

		if ((!SDL_WasInit(SDL_INIT_VIDEO)) && (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0))
		{
			Popup(std::string("Failed to initialize the display: ") + SDL_GetError());
			DoExit(1);
		}

		// -realinput: from here on only its own input reaches the game. SDL_SetEventFilter also empties SDL's
		// queue, so it is set once, before the window exists.
		if ((!mRealInputFileName.empty()) && (!gRealInputFilterSet))
		{
			SDL_SetEventFilter(RealInputFilter, NULL);
			gRealInputFilterSet = true;
		}

		int aScale = 1;
		int aPlaceX = SDL_WINDOWPOS_CENTERED;
		int aPlaceY = SDL_WINDOWPOS_CENTERED;
		Uint32 aFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | GetExtraWindowFlags();

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

#ifndef __EMSCRIPTEN__
		// port: the window can't be made smaller than the game's own screen (the user, 2026-11-07: "a minimum window size
		// match the game's native resolution"): mWidth x mHeight in the desktop's points, the units the window is sized in,
		// which is the size the original's window has on a scaled desktop. A desktop too small for that keeps what fits.
		{
			int aMinWidth = mWidth;
			int aMinHeight = mHeight;
			SDL_Rect aUsable;
			int aTop = 0, aLeft = 0, aBottom = 0, aRight = 0;
			if (SDL_GetDisplayUsableBounds(aDisplay, &aUsable) == 0)
			{
				SDL_GetWindowBordersSize(mHWnd, &aTop, &aLeft, &aBottom, &aRight);
				aMinWidth = std::max(1, std::min(aMinWidth, aUsable.w - aLeft - aRight));
				aMinHeight = std::max(1, std::min(aMinHeight, aUsable.h - aTop - aBottom));
			}
			SDL_SetWindowMinimumSize(mHWnd, aMinWidth, aMinHeight);
		}
#endif

		mIsPhysWindowed = mIsWindowed;
		SDL_StartTextInput();
#ifdef _WIN32
		if ((!gPostedInput) && (SDL_getenv("BYOPOP_POSTED_INPUT") != NULL) && (IsTestRun()))
		{
			gPostedInput = true;
			SDL_SetWindowsMessageHook(PostedInputHook, NULL);
		}
#endif
#ifdef __EMSCRIPTEN__
		// the web page's stage takes the game's own size (web/shell.html, onGameScreen: its shape and the site's pixel-perfect)
		EM_ASM({ if (typeof Module['onGameScreen'] === 'function') Module['onGameScreen']($0, $1); }, mWidth, mHeight);
#endif
	}

	if (mDDInterface == NULL)
		mDDInterface = CreateDDInterface();
	mDDInterface->mForcedRenderScale = mForcedRenderScale;

	int aResult = InitDDInterface();
	if ((aResult != DDInterface::RESULT_OK) && DisplayInitFailed(aResult))
		return;
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

#ifdef __EMSCRIPTEN__
	// port: a browser that would refuse full screen now leaves the game windowed (sexyapp_fullscreen_allowed)
	if ((!wantWindowed) && (mIsWindowed) && (!sexyapp_fullscreen_allowed()))
	{
		SDL_Log("SwitchScreenMode: the browser would refuse full screen now (no player's gesture, or none in this page): staying windowed");
		wantWindowed = true;
	}
#endif

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
#ifdef __EMSCRIPTEN__
		// the page's own full screen (sexyapp_page_fullscreen), or SDL's
		if (sexyapp_page_fullscreen(mIsWindowed ? 0 : 1))
		{
			gPageFullscreen = !mIsWindowed;
		}
		else
		{
			SDL_SetWindowFullscreen(mHWnd, mIsWindowed ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
			// a page that has no full screen at all refuses at once, and SDL leaves its window windowed; a later refusal is watched
			mIsWindowed = (SDL_GetWindowFlags(mHWnd) & SDL_WINDOW_FULLSCREEN) == 0;
			sexyapp_watch_fullscreen_refusal(mIsWindowed ? 0 : 1);
		}
#else
		SDL_SetWindowFullscreen(mHWnd, mIsWindowed ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
#endif
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
		bool isHeld = false;

		if (mUpdateAppState == UPDATESTATE_PROCESS_1)
		{
			// port: with -drawevery an update that has not been drawn yet is drawn before the next one
			if (((++mNonDrawCount < (int) ceil(10*mUpdateMultiplier)) || (!mLoaded)) && ((!mDrawEvery) || (!mHasPendingDraw)))
			{
				// port: a test may hold the updates (byopop_hold_updates_at on the web, -hold natively): the frame is still
				// drawn, and -fast's clock stops at the held update's time
				if ((mUpdateFTimeAcc >= aFrameFTime) && (!WebUpdatesHeld(mUpdateCount)) && (!(isHeld = LiveUpdatesHeld())))
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
			if ((mHasPendingDraw) && ((!mFastMode) || (mDrawEvery) || (mDrawCount == 0) || (mUpdateCount - mLastFastDrawUpdate >= FAST_DRAW_INTERVAL)))
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
			// port: -hold waits for the driver's next line without spinning (no clock moves meanwhile)
			if ((isHeld) && (allowSleep))
				SDL_Delay(1);
		}
	}

	ProcessSafeDeleteList();
	return true;
}

void SexyAppBase::DoMainLoop()
{
#ifdef __EMSCRIPTEN__
	// port: a hidden page gets no animation frames, so the focus loss that SDL queues when the tab is left would wait in
	// SDL's queue until the tab is shown again, while the music and sounds play on. The game takes it at once instead, as
	// the native port takes WM_ACTIVATEAPP: it mutes and does whatever else its LostFocus does. The focus coming back
	// reaches it through SDL's queue as usual, at the first frame.
	emscripten_set_visibilitychange_callback(this, false, [](int, const EmscriptenVisibilityChangeEvent* theEvent, void* theApp) -> EM_BOOL {
		if ((theEvent != NULL) && (theEvent->hidden))
			EmscriptenPageHidden((SexyAppBase*) theApp);
		return false;
	});
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
void SexyAppBase::EmscriptenPageHidden(SexyAppBase* theApp)
{
	// as SDL_WINDOWEVENT_FOCUS_LOST does in HandleEvent (WM_ACTIVATEAPP)
	if ((theApp->mSEHOccured) || (theApp->mShutdown) || (!theApp->mActive))
		return;
	theApp->mActive = false;
	theApp->RehupFocus();
	if ((theApp->mIsOpeningURL) && (!theApp->mActive))
		theApp->URLOpenSucceeded(theApp->mOpeningURL);
}

void SexyAppBase::EmscriptenMainLoop(void* theArg)
{
	SexyAppBase* anApp = (SexyAppBase*) theArg;

	// One browser frame: step until every due update ran and a frame was drawn, or until nothing is due
	// before the next animation frame (where 1.3 would have slept).
	int aDrawCount = anApp->mDrawCount;
	double aStart = emscripten_get_now();
	for (int aStep = 0; (aStep < 1000) && (!anApp->mShutdown); aStep++)
	{
		if (anApp->mExitToTop)
			anApp->mExitToTop = false;
		anApp->UpdateAppStep(NULL);

		if ((anApp->mDrawCount != aDrawCount) || (anApp->mNothingDue))
			break;
		// port: a test's cap on the frame (byopop_set_frame_budget)
		if ((gWebFrameBudget > 0) && (emscripten_get_now() - aStart >= gWebFrameBudget))
			break;
	}
	sexyapp_test_frame(anApp->mDrawCount != aDrawCount, emscripten_get_now() - aStart);

	if (anApp->mShutdown)
	{
		emscripten_cancel_main_loop();
		// Deletes the board, which saves a game in progress, and writes the settings
		anApp->MainLoopFinished();
		anApp->UpdateUserDataBackup(true);
		// port: a restart (RequestRestart) reloads the page, which starts the game again with its own arguments
		// less those a restart drops
		Platform::JsonValue aDropped = Platform::JsonValue::MakeArray();
		if (anApp->mRestartRequested)
		{
			std::vector<std::string> aKept;
			anApp->GetRestartArgs(&aKept);
			std::vector<std::string> aNames;
			for (const std::string& anArg : anApp->mCmdLineArgs)
			{
				std::string aName = anArg.substr(0, anArg.find('='));
				if ((std::find(aKept.begin(), aKept.end(), anArg) == aKept.end()) && (std::find(aNames.begin(), aNames.end(), aName) == aNames.end()))
				{
					aNames.push_back(aName);
					aDropped.Push(Platform::JsonValue(aName));
				}
			}
			SDL_Log("restart: the page reloads");
		}
		sexyapp_notify_exit(anApp->mRestartRequested ? 1 : 0, aDropped.Dump(0).c_str());
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

#ifdef __EMSCRIPTEN__
		// the page's full screen, left or refused (gPageFullscreen)
		if ((gPageFullscreen) && (sexyapp_page_fullscreen_state() != 1))
		{
			gPageFullscreen = false;
			mIsWindowed = true;
			mIsPhysWindowed = true;
			mWidgetManager->MarkAllDirty();
		}
#endif

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

		// port: the player's 3D choice: this run's (Set3DAcclerated) or the saved "Is3D" (the original reads it as its
		// 2D mode's switch). Only 3D off changes anything: a default start, or a display without acceleration, is as it was
		bool aWant3D = mWant3D;
		if (!mUserChanged3DSetting)
		{
			aWant3D = true;
			RegistryReadBoolean("Is3D", &aWant3D);
#ifdef __EMSCRIPTEN__
			// the site's "3D acceleration" (always 1, never 0; -1 leaves it to the game) decides this run in place of the saved
			// value, and writes nothing (mUserChanged3DSetting stays false); mCan3D still makes "always" 2D without WebGL2
			if (Byopop::SiteAccel3D() >= 0)
				aWant3D = Byopop::SiteAccel3D() != 0;
#endif
		}
		if (!aWant3D)
			mDDInterface->mIs3D = false;

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
	Platform::ModManager::Get().SetGameRunning(true);
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
	Platform::ModManager::Get().SetGameRunning(false);

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

// The printf conversions of a text, in order ("d;s;" for "LEVEL %d of %s"; "%%" is none). Malformed ones count
// as "?", so two texts match only if printf reads the same arguments from both.
static std::string FormatSignature(const std::wstring& theText)
{
	std::string aSignature;
	for (size_t i = 0; i < theText.size(); i++)
	{
		if (theText[i] != L'%')
			continue;
		if ((i + 1 < theText.size()) && (theText[i + 1] == L'%'))
		{
			i++;
			continue;
		}
		size_t j = i + 1;
		while ((j < theText.size()) && wcschr(L"-+ #0", theText[j]) && (theText[j] != 0))
			j++;
		for (int aPart = 0; aPart < 2; aPart++)
		{
			// width, then .precision; '*' takes an argument of its own
			if ((aPart == 1) && (j < theText.size()) && (theText[j] == L'.'))
				j++;
			else if (aPart == 1)
				break;
			if ((j < theText.size()) && (theText[j] == L'*'))
			{
				aSignature += "*;";
				j++;
			}
			else
				while ((j < theText.size()) && (theText[j] >= L'0') && (theText[j] <= L'9'))
					j++;
		}
		std::string aLength;
		while ((j < theText.size()) && wcschr(L"hlLqjzt", theText[j]) && (theText[j] != 0))
			aLength += (char) theText[j++];
		if ((j < theText.size()) && wcschr(L"diouxXeEfFgGaAcspn", theText[j]) && (theText[j] != 0))
			aSignature += aLength + (char) theText[j] + ";";
		else
			aSignature += "?;";
		i = (j < theText.size()) ? j : theText.size();
	}
	return aSignature;
}

bool SexyAppBase::LoadProperties()
{
	// Load required language-file properties
	if (!LoadProperties("properties\\default.xml", true, false))
		return false;
	// port: what the game's own texts say, to check the mods' texts against
	StringWStringMap aGameStrings = mStringProperties;

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

	// The game passes some texts to printf as formats (Board: StrFormat(GetString("LEVEL"), level)): a mod's
	// text for a key whose game text has % codes must ask for the same arguments, or it could read past them or
	// write through %n. Such a text is not used. Every other key (texts the game only draws, keys a mod adds)
	// takes the mod's text as it is, as before the check. (A mod menu value never has a %: ModSettings.)
	for (StringWStringMap::iterator anItr = mStringProperties.begin(); anItr != mStringProperties.end(); ++anItr)
	{
		StringWStringMap::const_iterator aGame = aGameStrings.find(anItr->first);
		if ((aGame == aGameStrings.end()) || (anItr->second == aGame->second))
			continue;
		std::string aGameSignature = FormatSignature(aGame->second);
		if (aGameSignature.empty() || (FormatSignature(anItr->second) == aGameSignature))
			continue;
		SDL_Log("mod strings: %s: its %% codes differ from the game's text, so the game's text is used", anItr->first.c_str());
		anItr->second = aGame->second;
	}
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

static void LiveLogOutput(void* theData, int theCategory, SDL_LogPriority thePriority, const char* theMessage);

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
	else if (theParamName == "-realinput")
		mRealInputFileName = theParamValue;
	else if (theParamName == "-commands")
	{
		mCommandsFileName = theParamValue;
		SDL_LogSetOutputFunction(LiveLogOutput, NULL);
	}
	else if (theParamName == "-hold")
		mCommandsHold = true;
	else if (theParamName == "-fast")
		mFastMode = true;
	// port: the site's "Always skip start-up logos" natively, for testing a game's wiring (framework/WebSettings.h)
	else if (theParamName == "-skiplogos")
		Byopop::SetSiteSkipLogos(true);
	else if (theParamName == "-trace")
		mTraceMode = true;
	else if (theParamName == "-drawevery")
		mDrawEvery = true;
	else if (theParamName == "-renderscale")
		mForcedRenderScale = atoi(theParamValue.c_str());
	// port: stand-ins for missing game files (platform/Placeholders.h)
	else if (theParamName == "-placeholders")
		Platform::SetPlaceholders(true);
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
	if (!LoadScriptFile(theFileName, "script", &mScript))
		return false;
	mScriptPos = 0;
	return true;
}

// Reads a -script file (also -realinput's) into theCommands, sorted by update count; theWhat starts its messages
static std::string StripScriptComment(const std::string& theLine);
static std::string TrimScriptArgs(const std::string& theRest);

bool SexyAppBase::LoadScriptFile(const std::string& theFileName, const char* theWhat, std::vector<ScriptCommand>* theCommands)
{
	FILE* aFile = fopen(theFileName.c_str(), "r");
	if (aFile == NULL)
	{
		SDL_Log("%s: cannot open %s", theWhat, theFileName.c_str());
		return false;
	}

	theCommands->clear();

	char aLine[1024];
	int aLineNum = 0;
	bool ok = true;
	while (fgets(aLine, sizeof(aLine), aFile) != NULL)
	{
		aLineNum++;
		std::string aText = StripScriptComment(aLine);
		if (aText.find_first_not_of(" \t\r\n") == std::string::npos)
			continue;

		std::istringstream aStream(aText);
		ScriptCommand aCommand;
		aCommand.mLine = aLineNum;
		if ((!(aStream >> aCommand.mUpdateCount)) || (!(aStream >> aCommand.mCommand)))
		{
			SDL_Log("%s: %s:%d: expected \"<update count> <command> [args]\"", theWhat, theFileName.c_str(), aLineNum);
			ok = false;
			continue;
		}
		std::string aRest;
		std::getline(aStream, aRest);
		aCommand.mArgs = TrimScriptArgs(aRest);
		theCommands->push_back(aCommand);
	}
	fclose(aFile);

	std::stable_sort(theCommands->begin(), theCommands->end(),
		[](const ScriptCommand& a, const ScriptCommand& b) { return a.mUpdateCount < b.mUpdateCount; });
	return ok;
}

// Runs the script commands that are due at this update.
void SexyAppBase::RunScript()
{
	while ((mScriptPos < mScript.size()) && (mScript[mScriptPos].mUpdateCount <= mUpdateCount) && (!mShutdown))
	{
		// A copy: the command reaches game code (HandleScriptCommand, widgets), which may queue more
		// commands with AddScriptCommand and so reallocate mScript
		const ScriptCommand aCommand = mScript[mScriptPos++];
		std::string anError;
		int aResult = RunScriptCommand(aCommand, &anError);
		if (aResult == SCRIPT_FAILED)
			SDL_Log("script: line %d: %s", aCommand.mLine, anError.c_str());
		else if (aResult == SCRIPT_UNKNOWN)
			SDL_Log("script: line %d: unknown command '%s %s'", aCommand.mLine, aCommand.mCommand.c_str(), aCommand.mArgs.c_str());
	}
}

// One -script command (or a live one, RunLiveCommands): the framework's own commands, then the game's
// HandleScriptCommand. theError gets the reason of a SCRIPT_FAILED.
int SexyAppBase::RunScriptCommand(const ScriptCommand& theCommand, std::string* theError)
{
	const ScriptCommand& aCommand = theCommand;
	const std::string& aName = aCommand.mCommand;
	std::istringstream anArgs(aCommand.mArgs);
	int x = 0;
	int y = 0;

	// a bare "move" is the test command that asks the game's bot for its next move (docs/TEST_COMMANDS.md); "move
	// <x> <y>" moves the mouse
	if ((aName == "click") || (aName == "rclick") || (aName == "down") || (aName == "up") ||
		((aName == "move") && (aCommand.mArgs.find_first_not_of(" \t") != std::string::npos)))
	{
		if (!(anArgs >> x >> y))
		{
			*theError = aName + " needs <x> <y>";
			return SCRIPT_FAILED;
		}
		MouseMoveTo(x, y);
		// a real click is a press and a release, each preceded by the move to its point (a drag when the button is
		// down: Rocket Mania's original drags the pressed tile on the release), so the click does the same
		if ((aName == "click") || (aName == "down"))
			mWidgetManager->MouseDown(x, y, 1);
		if ((aName == "click") || (aName == "up"))
		{
			if (aName == "click")
				MouseMoveTo(x, y);
			mWidgetManager->MouseUp(x, y, 1);
		}
		if (aName == "rclick")
		{
			// right button: click code -1, as GetClickCode gives a real right click
			mWidgetManager->MouseDown(x, y, -1);
			MouseMoveTo(x, y);
			mWidgetManager->MouseUp(x, y, -1);
		}
	}
	else if (aName == "key")
	{
		char* anEnd = NULL;
		long aNum = strtol(aCommand.mArgs.c_str(), &anEnd, 10);
		KeyCode aKeyCode = ((anEnd != aCommand.mArgs.c_str()) && (*anEnd == 0)) ? (KeyCode) aNum : GetKeyCodeFromName(aCommand.mArgs);
		if ((aKeyCode <= KEYCODE_UNKNOWN) || (aKeyCode >= 0xFF))
		{
			*theError = "unknown key '" + aCommand.mArgs + "'";
			return SCRIPT_FAILED;
		}
		mWidgetManager->KeyDown(aKeyCode);
		// like GetControlChar: these keys also type their control character (same value as the key code)
		if ((aKeyCode == KEYCODE_BACK) || (aKeyCode == KEYCODE_TAB) || (aKeyCode == KEYCODE_RETURN) || (aKeyCode == KEYCODE_ESCAPE))
			mWidgetManager->KeyChar((SexyChar) aKeyCode);
		mWidgetManager->KeyUp(aKeyCode);
	}
	else if ((aName == "keydown") || (aName == "keyup"))
	{
		// port: one half of a key press, e.g. to hold SHIFT or MENU (Alt) around another key; types nothing
		char* anEnd = NULL;
		long aNum = strtol(aCommand.mArgs.c_str(), &anEnd, 10);
		KeyCode aKeyCode = ((anEnd != aCommand.mArgs.c_str()) && (*anEnd == 0)) ? (KeyCode) aNum : GetKeyCodeFromName(aCommand.mArgs);
		if ((aKeyCode <= KEYCODE_UNKNOWN) || (aKeyCode >= 0xFF))
		{
			*theError = "unknown key '" + aCommand.mArgs + "'";
			return SCRIPT_FAILED;
		}
		if (aName == "keydown")
			mWidgetManager->KeyDown(aKeyCode);
		else
			mWidgetManager->KeyUp(aKeyCode);
	}
	else if (aName == "modmenu")
		ModMenu::RunScriptCommand(this, aCommand);
	else if ((aName == "char") && (!aCommand.mArgs.empty()))
	{
		// The arguments are trimmed, so a space is spelled "char SPACE" (exactly; anything else types
		// its first character, as before). "key SPACE" types nothing: only the keys GetControlChar
		// covers do.
		if (aCommand.mArgs == "SPACE")
			mWidgetManager->KeyChar((SexyChar) ' ');
		else
			mWidgetManager->KeyChar((SexyChar) aCommand.mArgs[0]);
	}
	else if ((aName == "screenshot") && (!aCommand.mArgs.empty()))
		SaveScreenshot(aCommand.mArgs);
	else if ((aName == "screenshotfull") && (!aCommand.mArgs.empty()))
		SaveScreenshot(aCommand.mArgs, true);
	else if ((aName == "focus") && ((aCommand.mArgs == "lose") || (aCommand.mArgs == "regain")))
	{
		// as the window's focus event does (WM_ACTIVATEAPP), like origrun's "focus lose" and "focus regain"
		mActive = aCommand.mArgs == "regain";
		RehupFocus();

		if ((mActive) && (!mIsWindowed))
			mWidgetManager->MarkAllDirty();
	}
	else if (aName == "quit")
		Shutdown();
	else if ((aName == "pointer") && (mWidgetManager != NULL))
	{
		// the mouse as the widgets last saw it, in game pixels, answered on the log: a test reads where its input landed
		// (tools/web/pagecheck.js, a click after the page's canvas changed size)
		SDL_Log("POINTER {\"x\":%d,\"y\":%d,\"in\":%s}", mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY,
			mWidgetManager->mMouseIn ? "true" : "false");
		return SCRIPT_GAME;
	}
	else if (HandleScriptCommand(aCommand))
		return SCRIPT_GAME;
	else
		return SCRIPT_UNKNOWN;
	return SCRIPT_RAN;
}

bool SexyAppBase::HandleScriptCommand(const ScriptCommand& theCommand)
{
	return false;
}

void SexyAppBase::AddScriptCommand(int theUpdateCount, const std::string& theCommand, const std::string& theArgs)
{
	ScriptCommand aCommand;
	aCommand.mUpdateCount = theUpdateCount;
	aCommand.mCommand = theCommand;
	aCommand.mArgs = theArgs;
	aCommand.mLine = 0;

	// mScript stays sorted by update count (RunScript relies on it). Only commands from mScriptPos on
	// move, so this may also run inside RunScript (which works on a copy of the current command).
	std::vector<ScriptCommand>::iterator anItr = mScript.begin() + mScriptPos;
	while ((anItr != mScript.end()) && (anItr->mUpdateCount <= theUpdateCount))
		++anItr;
	mScript.insert(anItr, aCommand);
}

//////////////////////////////////////////////////////////////////////////
// port: live commands (see SexyAppBase.h, docs/TEST_COMMANDS.md "Sending them"): a driver that decides as it goes
// sends "<command> [args]" lines, natively by appending them to the -commands file and on the web through
// byopop_test_command; each runs at the next update through -script's dispatch (-realinput's when it is on) and
// is answered on the log

// A script line without its comment: '#' starts one, except inside "..." (a colour: modmenu set <mod> tint "#ff8800")
static std::string StripScriptComment(const std::string& theLine)
{
	std::string aText = theLine;
	bool isQuoted = false;
	for (size_t i = 0; i < aText.size(); i++)
	{
		if (aText[i] == '"')
			isQuoted = !isQuoted;
		else if ((aText[i] == '#') && (!isQuoted))
		{
			aText.erase(i);
			break;
		}
	}
	return aText;
}

// The rest of a script line, trimmed: a command's arguments
static std::string TrimScriptArgs(const std::string& theRest)
{
	size_t aStart = theRest.find_first_not_of(" \t");
	size_t anEnd = theRest.find_last_not_of(" \t\r\n");
	return (aStart == std::string::npos) ? "" : theRest.substr(aStart, anEnd - aStart + 1);
}

// -commands: the log as SDL writes it ("INFO: ..." on stderr), flushed line by line. SDL2.dll writes through its own C
// runtime, which keeps a redirected log until exit, and a driver reads each answer as it comes.
static void LiveLogOutput(void* theData, int theCategory, SDL_LogPriority thePriority, const char* theMessage)
{
	static const char* const kPrefixes[SDL_NUM_LOG_PRIORITIES] = { NULL, "VERBOSE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL" };
	const char* aPrefix = ((thePriority > 0) && (thePriority < SDL_NUM_LOG_PRIORITIES)) ? kPrefixes[thePriority] : "INFO";
	fprintf(stderr, "%s: %s\n", aPrefix, theMessage);
	fflush(stderr);
}

bool SexyAppBase::QueueLiveCommand(const std::string& theLine, int theLineNum)
{
	std::string aText = StripScriptComment(theLine);
	std::istringstream aStream(aText);
	ScriptCommand aCommand;
	aCommand.mUpdateCount = 0;
	aCommand.mLine = theLineNum;
	if (!(aStream >> aCommand.mCommand))
		return false;  // a blank line or a comment
	if (mLiveCommands.size() >= 1024)
	{
		SDL_Log("commands: more than 1024 commands wait for the next update; '%s' dropped", aText.c_str());
		return false;
	}
	std::string aRest;
	std::getline(aStream, aRest);
	aCommand.mArgs = TrimScriptArgs(aRest);
	mLiveCommands.push_back(aCommand);
	return true;
}

// -commands: the file's new whole lines, read from where the last look stopped (a line still being written waits
// for its end of line). A file not there yet is waited for; one that got shorter was started again, and is read from
// its start.
void SexyAppBase::PollLiveCommands()
{
	struct stat aStat;
	if (stat(mCommandsFileName.c_str(), &aStat) != 0)
	{
		if (!mCommandsWaitLogged)
		{
			SDL_Log("commands: waiting for %s", mCommandsFileName.c_str());
			fflush(stdout);
			fflush(stderr);
		}
		mCommandsWaitLogged = true;
		return;
	}
	long long aSize = (long long) aStat.st_size;
	if (aSize < mCommandsFilePos)
	{
		SDL_Log("commands: %s got shorter; reading it from its start", mCommandsFileName.c_str());
		mCommandsFilePos = 0;
		mCommandsFileLine = 0;
	}
	if (aSize == mCommandsFilePos)
		return;

	FILE* aFile = fopen(mCommandsFileName.c_str(), "rb");
	if (aFile == NULL)
		return;
	std::string aData;
	if (fseek(aFile, (long) mCommandsFilePos, SEEK_SET) == 0)
	{
		char aBuf[4096];
		size_t aRead;
		while ((aRead = fread(aBuf, 1, sizeof(aBuf), aFile)) > 0)
			aData.append(aBuf, aRead);
	}
	fclose(aFile);

	size_t aLast = aData.rfind('\n');
	if (aLast == std::string::npos)
		return;
	size_t aPos = 0;
	while (aPos <= aLast)
	{
		size_t anEnd = aData.find('\n', aPos);
		mCommandsFileLine++;
		QueueLiveCommand(aData.substr(aPos, anEnd - aPos), mCommandsFileLine);
		aPos = anEnd + 1;
	}
	mCommandsFilePos += (long long) (aLast + 1);
}

// -hold (natively, with -commands): once the game has loaded, an update runs only when the driver lets it, so a game
// under -fast does not run on while its driver decides (and a run with the same lines plays the same way). New lines
// let one update run them; "wait <n>" lets n run. Held, the file is looked at here, each pass of the main loop.
bool SexyAppBase::LiveUpdatesHeld()
{
	if ((!mCommandsHold) || (mCommandsFileName.empty()) || (!mLoaded) || (mShutdown) || (mUpdateCount < mLiveRunUntil))
		return false;
	PollLiveCommands();
	if (mLiveCommands.empty())
		return true;
	mLiveRunUntil = mUpdateCount + 1;
	return false;
}

// The queued live commands, in order, each answered on the log: "OK <command>" when the framework did it,
// "ERROR <command>: <reason>" when it could not or nobody knows the command; a game's command answers itself
// (docs/TEST_COMMANDS.md)
void SexyAppBase::RunLiveCommands()
{
	// "wait <n>" (the channel's own command, never a game's): the lines after it wait n updates, and it is answered
	// when they are up
	if (mLiveWaitUntil >= 0)
	{
		if (mUpdateCount < mLiveWaitUntil)
			return;
		mLiveWaitUntil = -1;
		SDL_Log("OK %s", mLiveWaitLine.c_str());
		fflush(stdout);
		fflush(stderr);
	}
	// a copy: a command runs game code, which may queue more (they wait for the next update)
	std::vector<ScriptCommand> aCommands;
	aCommands.swap(mLiveCommands);
	for (size_t i = 0; (i < aCommands.size()) && (!mShutdown); i++)
	{
		const ScriptCommand& aCommand = aCommands[i];
		std::string aLine = aCommand.mArgs.empty() ? aCommand.mCommand : (aCommand.mCommand + " " + aCommand.mArgs);
		if (aCommand.mCommand == "wait")
		{
			int aCount = -1;
			char aRest = 0;
			if ((sscanf(aCommand.mArgs.c_str(), "%d %c", &aCount, &aRest) != 1) || (aCount < 0))
				SDL_Log("ERROR %s: wait needs <updates>", aLine.c_str());
			else if (aCount == 0)
				SDL_Log("OK %s", aLine.c_str());
			else
			{
				// the rest go back before any a command queued meanwhile; with -hold the updates run until then
				mLiveWaitUntil = mUpdateCount + aCount;
				mLiveWaitLine = aLine;
				if (mLiveRunUntil < mLiveWaitUntil)
					mLiveRunUntil = mLiveWaitUntil;
				mLiveCommands.insert(mLiveCommands.begin(), aCommands.begin() + i + 1, aCommands.end());
				return;
			}
			fflush(stdout);
			fflush(stderr);
			continue;
		}
		std::string anError;
		int aResult = mRealInputFileName.empty() ? RunScriptCommand(aCommand, &anError) : RunRealInputCommand(aCommand, &anError);
		if (aResult == SCRIPT_RAN)
			SDL_Log("OK %s", aLine.c_str());
		else if (aResult == SCRIPT_FAILED)
			SDL_Log("ERROR %s: %s", aLine.c_str(), anError.c_str());
		else if (aResult == SCRIPT_UNKNOWN)
			SDL_Log("ERROR %s: unknown command", aLine.c_str());
		// the driver reads the answer from the log as it comes: a log redirected to a file is buffered until exit
		fflush(stdout);
		fflush(stderr);
	}
}

#ifdef __EMSCRIPTEN__
extern "C"
{

// The player-only test page's live command (byopop.test.command(line), docs/WEB_PLAYER.md; cmake/ByopopWeb.cmake
// exports it): queues one "<command> [args]" line for the next update, as -commands does natively. Its answer is
// printed on the log when it runs. Returns 1 when queued, 0 when refused: no game runs, the line holds no command,
// or too many wait.
EMSCRIPTEN_KEEPALIVE int byopop_test_command(const char* theLine)
{
	SexyAppBase* anApp = gSexyAppBase;
	if ((anApp == NULL) || (!anApp->mInitialized) || (anApp->mShutdown) || (theLine == NULL))
		return 0;
	anApp->mLiveCommandCount++;
	return anApp->QueueLiveCommand(theLine, anApp->mLiveCommandCount) ? 1 : 0;
}

}
#endif

//////////////////////////////////////////////////////////////////////////
// port: -realinput (see SexyAppBase.h, docs/REAL_INPUT.md): timed SDL events pushed into SDL's own queue, so that
// they reach the game the way a hand's events do, through SDL_PollEvent and HandleEvent

// "<x> <y>" in window pixels, or "game <x> <y>" in the game's, mapped to the window as it shows the game now
static bool ReadRealPoint(SexyAppBase* theApp, std::istringstream& theArgs, int* theX, int* theY)
{
	std::streampos aStart = theArgs.tellg();
	std::string aWord;
	bool isGame = false;
	if ((theArgs >> aWord) && (aWord == "game"))
		isGame = true;
	else
	{
		theArgs.clear();
		theArgs.seekg(aStart);
	}

	int x = 0;
	int y = 0;
	if (!(theArgs >> x >> y))
		return false;
	if ((isGame) && (theApp->mDDInterface != NULL))
		theApp->mDDInterface->LogicalToWindow(x, y, &x, &y);
	*theX = x;
	*theY = y;
	return true;
}

// [button]: left (also when there is none), right, middle, x1 or x2; 0 for another word
static int ReadRealButton(std::istringstream& theArgs)
{
	std::string aWord;
	if (!(theArgs >> aWord))
		return SDL_BUTTON_LEFT;
	if (aWord == "left")
		return SDL_BUTTON_LEFT;
	if (aWord == "right")
		return SDL_BUTTON_RIGHT;
	if (aWord == "middle")
		return SDL_BUTTON_MIDDLE;
	if (aWord == "x1")
		return SDL_BUTTON_X1;
	if (aWord == "x2")
		return SDL_BUTTON_X2;
	return 0;
}

// The argument without one pair of "..." around it
static std::string UnquoteRealText(const std::string& theText)
{
	if ((theText.length() >= 2) && (theText[0] == '"') && (theText[theText.length() - 1] == '"'))
		return theText.substr(1, theText.length() - 2);
	return theText;
}

// The modifier a key is, as SDL keeps them
static Uint16 RealKeyModifier(SDL_Keycode theKey)
{
	switch (theKey)
	{
	case SDLK_LSHIFT:	return KMOD_LSHIFT;
	case SDLK_RSHIFT:	return KMOD_RSHIFT;
	case SDLK_LCTRL:	return KMOD_LCTRL;
	case SDLK_RCTRL:	return KMOD_RCTRL;
	case SDLK_LALT:		return KMOD_LALT;
	case SDLK_RALT:		return KMOD_RALT;
	case SDLK_LGUI:		return KMOD_LGUI;
	case SDLK_RGUI:		return KMOD_RGUI;
	case SDLK_MODE:		return KMOD_MODE;
	}
	return KMOD_NONE;
}

static const char kRealPlainKeys[] = "`1234567890-=[]\\;',./";
static const char kRealShiftedKeys[] = "~!@#$%^&*()_+{}|:\"<>?";

// The character a key types with these modifiers on a US layout, as Windows' TranslateMessage makes WM_CHAR of it, or 0.
// With Ctrl, Alt or the Windows key held it types none that SDL passes on as text (Enter, Tab, Backspace and Esc are
// control characters, never text: GetControlChar types them).
static char RealKeyText(SDL_Keycode theKey, Uint16 theMods)
{
	if ((theMods & (KMOD_CTRL | KMOD_ALT | KMOD_GUI)) != 0)
		return 0;

	bool isShift = (theMods & KMOD_SHIFT) != 0;
	if ((theKey >= SDLK_a) && (theKey <= SDLK_z))
	{
		bool isUpper = isShift != ((theMods & KMOD_CAPS) != 0);
		return (char) (isUpper ? (theKey - SDLK_a + 'A') : theKey);
	}

	switch (theKey)
	{
	case SDLK_KP_DIVIDE:	return '/';
	case SDLK_KP_MULTIPLY:	return '*';
	case SDLK_KP_MINUS:		return '-';
	case SDLK_KP_PLUS:		return '+';
	}
	if ((theMods & KMOD_NUM) != 0)
	{
		if ((theKey >= SDLK_KP_1) && (theKey <= SDLK_KP_9))
			return (char) ('1' + (theKey - SDLK_KP_1));
		if (theKey == SDLK_KP_0)
			return '0';
		if (theKey == SDLK_KP_PERIOD)
			return '.';
	}

	if ((theKey < 0x20) || (theKey > 0x7E))
		return 0;
	if (isShift)
	{
		const char* aPos = strchr(kRealPlainKeys, (char) theKey);
		if (aPos != NULL)
			return kRealShiftedKeys[aPos - kRealPlainKeys];
	}
	return (char) theKey;
}

void SexyAppBase::PushRealEvent(SDL_Event* theEvent)
{
	gPushingRealInput = true;
	if (SDL_PushEvent(theEvent) < 0)
		SDL_Log("realinput: SDL_PushEvent failed: %s", SDL_GetError());
	gPushingRealInput = false;
}

void SexyAppBase::PushRealWindowEvent(int theWindowEvent)
{
	SDL_Event anEvent;
	SDL_zero(anEvent);
	anEvent.type = SDL_WINDOWEVENT;
	anEvent.window.windowID = (mHWnd != NULL) ? SDL_GetWindowID(mHWnd) : 0;
	anEvent.window.event = (Uint8) theWindowEvent;
	PushRealEvent(&anEvent);
}

// A move to the point, after the window's ENTER when the mouse was not on it. No move when the point is the same,
// as SDL drops a motion that moves nowhere.
void SexyAppBase::PushRealMouseMove(int theX, int theY)
{
	if (!mRealMouseIn)
	{
		PushRealWindowEvent(SDL_WINDOWEVENT_ENTER);
		mRealMouseIn = true;
	}
	if ((theX == mRealMouseX) && (theY == mRealMouseY))
		return;

	SDL_Event anEvent;
	SDL_zero(anEvent);
	anEvent.type = SDL_MOUSEMOTION;
	anEvent.motion.windowID = (mHWnd != NULL) ? SDL_GetWindowID(mHWnd) : 0;
	anEvent.motion.which = 0;
	anEvent.motion.state = mRealButtons;
	anEvent.motion.x = theX;
	anEvent.motion.y = theY;
	anEvent.motion.xrel = (mRealMouseX < 0) ? 0 : theX - mRealMouseX;
	anEvent.motion.yrel = (mRealMouseY < 0) ? 0 : theY - mRealMouseY;
	mRealMouseX = theX;
	mRealMouseY = theY;
	PushRealEvent(&anEvent);
}

// A press or release at the point (window pixels), after the move to it: SDL gives a button event the mouse's last
// position, which Windows' WM_MOUSEMOVE before the press set
void SexyAppBase::PushRealButton(int theButton, bool isDown, int theX, int theY, int theClicks)
{
	PushRealMouseMove(theX, theY);
	if (isDown)
	{
		mRealButtons |= SDL_BUTTON(theButton);
		mRealClicks = theClicks;
	}
	else
		mRealButtons &= ~SDL_BUTTON(theButton);

	SDL_Event anEvent;
	SDL_zero(anEvent);
	anEvent.type = isDown ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
	anEvent.button.windowID = (mHWnd != NULL) ? SDL_GetWindowID(mHWnd) : 0;
	anEvent.button.which = 0;
	anEvent.button.button = (Uint8) theButton;
	anEvent.button.state = isDown ? SDL_PRESSED : SDL_RELEASED;
	anEvent.button.clicks = (Uint8) mRealClicks;
	anEvent.button.x = theX;
	anEvent.button.y = theY;
	PushRealEvent(&anEvent);
}

// A key's press or release as SDL sends it: the modifier state is updated first (and not by a repeat), so a modifier's
// own press carries it and its release does not; a lock key toggles on its press. A press that types sends its text next.
void SexyAppBase::PushRealKey(int theKey, bool isDown, bool isRepeat)
{
	SDL_Keycode aKey = (SDL_Keycode) theKey;
	std::vector<int>::iterator anItr = std::find(mRealKeysDown.begin(), mRealKeysDown.end(), theKey);
	if (!isRepeat)
	{
		Uint16 aModifier = RealKeyModifier(aKey);
		if (isDown)
		{
			if (aKey == SDLK_CAPSLOCK)
				mRealMods ^= KMOD_CAPS;
			else if (aKey == SDLK_NUMLOCKCLEAR)
				mRealMods ^= KMOD_NUM;
			else if (aKey == SDLK_SCROLLLOCK)
				mRealMods ^= KMOD_SCROLL;
			else
				mRealMods |= aModifier;
			if (anItr == mRealKeysDown.end())
				mRealKeysDown.push_back(theKey);
		}
		else
		{
			mRealMods &= ~aModifier;
			if (anItr != mRealKeysDown.end())
				mRealKeysDown.erase(anItr);
		}
	}

	SDL_Event anEvent;
	SDL_zero(anEvent);
	anEvent.type = isDown ? SDL_KEYDOWN : SDL_KEYUP;
	anEvent.key.windowID = (mHWnd != NULL) ? SDL_GetWindowID(mHWnd) : 0;
	anEvent.key.state = isDown ? SDL_PRESSED : SDL_RELEASED;
	anEvent.key.repeat = isRepeat ? 1 : 0;
	anEvent.key.keysym.scancode = SDL_GetScancodeFromKey(aKey);
	anEvent.key.keysym.sym = aKey;
	anEvent.key.keysym.mod = mRealMods;
	PushRealEvent(&anEvent);

	if (isDown)
	{
		char aChar = RealKeyText(aKey, mRealMods);
		if (aChar != 0)
			PushRealText(std::string(1, aChar));
	}
}

// SDL_TEXTINPUT, in pieces that fit its 32 bytes without splitting a UTF-8 character
void SexyAppBase::PushRealText(const std::string& theText)
{
	size_t aPos = 0;
	while (aPos < theText.length())
	{
		size_t aLen = std::min(theText.length() - aPos, (size_t) (SDL_TEXTINPUTEVENT_TEXT_SIZE - 1));
		while ((aPos + aLen < theText.length()) && (aLen > 1) && ((((unsigned char) theText[aPos + aLen]) & 0xC0) == 0x80))
			aLen--;

		SDL_Event anEvent;
		SDL_zero(anEvent);
		anEvent.type = SDL_TEXTINPUT;
		anEvent.text.windowID = (mHWnd != NULL) ? SDL_GetWindowID(mHWnd) : 0;
		memcpy(anEvent.text.text, theText.data() + aPos, aLen);
		anEvent.text.text[aLen] = 0;
		PushRealEvent(&anEvent);
		aPos += aLen;
	}
}

// Pushes the events due at this update; HandleEvent gets them when the update is done, before the next one
void SexyAppBase::RunRealInput()
{
	static bool sWarnedScriptedInput = false;
	if ((mScriptedInput) && (!sWarnedScriptedInput))
	{
		SDL_Log("realinput: the game's own test bot is on (mScriptedInput), which drops real input");
		sWarnedScriptedInput = true;
	}

	while ((mRealInputPos < mRealInput.size()) && (mRealInput[mRealInputPos].mUpdateCount <= mUpdateCount) && (!mShutdown))
	{
		// a copy: HandleScriptCommand may run game code
		const ScriptCommand aCommand = mRealInput[mRealInputPos++];
		std::string anError;
		int aResult = RunRealInputCommand(aCommand, &anError);
		if (aResult == SCRIPT_FAILED)
			SDL_Log("realinput: line %d: %s", aCommand.mLine, anError.c_str());
		else if (aResult == SCRIPT_UNKNOWN)
			SDL_Log("realinput: line %d: unknown command '%s %s'", aCommand.mLine, aCommand.mCommand.c_str(), aCommand.mArgs.c_str());
	}
}

// One -realinput command (or a live one when -realinput is on, RunLiveCommands): its events pushed, or the game's
// HandleScriptCommand. theError gets the reason of a SCRIPT_FAILED.
int SexyAppBase::RunRealInputCommand(const ScriptCommand& theCommand, std::string* theError)
{
	const ScriptCommand& aCommand = theCommand;
	const std::string& aName = aCommand.mCommand;
	std::istringstream anArgs(aCommand.mArgs);
	int x = 0;
	int y = 0;

	// a bare "move" is the test command that asks the game's bot for its next move (docs/TEST_COMMANDS.md), as in
	// RunScriptCommand, so a driver on the live channel with -realinput can ask for it; "move <point>" moves the mouse
	if ((aName == "move") && (aCommand.mArgs.find_first_not_of(" \t") == std::string::npos))
		return HandleScriptCommand(aCommand) ? SCRIPT_GAME : SCRIPT_UNKNOWN;

	if ((aName == "move") || (aName == "down") || (aName == "up") || (aName == "click") || (aName == "dblclick"))
	{
		if (!ReadRealPoint(this, anArgs, &x, &y))
		{
			*theError = StrFormat("%s needs <x> <y> or game <x> <y>", aName.c_str());
			return SCRIPT_FAILED;
		}
		if (aName == "move")
		{
			PushRealMouseMove(x, y);
			return SCRIPT_RAN;
		}

		int aButton = ReadRealButton(anArgs);
		int aClicks = 1;
		if ((aName == "down") && (!(anArgs >> aClicks)))
			aClicks = 1;
		if ((aButton == 0) || (aClicks < 1) || (aClicks > 255))
		{
			*theError = StrFormat("'%s %s': [button] is left, right, middle, x1 or x2", aName.c_str(), aCommand.mArgs.c_str());
			return SCRIPT_FAILED;
		}

		if (aName == "down")
			PushRealButton(aButton, true, x, y, aClicks);
		else if (aName == "up")
			PushRealButton(aButton, false, x, y, mRealClicks);
		else
		{
			PushRealButton(aButton, true, x, y, 1);
			PushRealButton(aButton, false, x, y, 1);
			if (aName == "dblclick")
			{
				PushRealButton(aButton, true, x, y, 2);
				PushRealButton(aButton, false, x, y, 2);
			}
		}
	}
	else if (aName == "wheel")
	{
		int aDeltaY = 0;
		int aDeltaX = 0;
		if (!(anArgs >> aDeltaY))
		{
			*theError = StrFormat("wheel needs <dy> [dx]");
			return SCRIPT_FAILED;
		}
		anArgs >> aDeltaX;

		SDL_Event anEvent;
		SDL_zero(anEvent);
		anEvent.type = SDL_MOUSEWHEEL;
		anEvent.wheel.windowID = (mHWnd != NULL) ? SDL_GetWindowID(mHWnd) : 0;
		anEvent.wheel.which = 0;
		anEvent.wheel.x = aDeltaX;
		anEvent.wheel.y = aDeltaY;
		anEvent.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
		anEvent.wheel.preciseX = (float) aDeltaX;
		anEvent.wheel.preciseY = (float) aDeltaY;
		anEvent.wheel.mouseX = mRealMouseX;
		anEvent.wheel.mouseY = mRealMouseY;
		PushRealEvent(&anEvent);
	}
	else if ((aName == "enter") || (aName == "leave"))
	{
		mRealMouseIn = aName == "enter";
		// a mouse that comes back has moved: its next move is pushed even to the point it left from
		if (!mRealMouseIn)
		{
			mRealMouseX = -1;
			mRealMouseY = -1;
		}
		PushRealWindowEvent(mRealMouseIn ? SDL_WINDOWEVENT_ENTER : SDL_WINDOWEVENT_LEAVE);
	}
	else if ((aName == "keydown") || (aName == "keyup") || (aName == "key") || (aName == "keyrepeat"))
	{
		SDL_Keycode aKey = SDL_GetKeyFromName(aCommand.mArgs.c_str());
		if (aKey == SDLK_UNKNOWN)
		{
			*theError = StrFormat("unknown SDL key name '%s'", aCommand.mArgs.c_str());
			return SCRIPT_FAILED;
		}
		if (aName == "keyrepeat")
			PushRealKey(aKey, true, true);
		else
		{
			if (aName != "keyup")
				PushRealKey(aKey, true, false);
			if (aName != "keydown")
				PushRealKey(aKey, false, false);
		}
	}
	else if (aName == "type")
	{
		std::string aText = UnquoteRealText(aCommand.mArgs);
		for (size_t i = 0; i < aText.length(); i++)
		{
			char aChar = aText[i];
			SDL_Keycode aKey = SDLK_UNKNOWN;
			bool needsShift = false;
			const char* aShifted = (aChar != 0) ? strchr(kRealShiftedKeys, aChar) : NULL;
			if ((aChar >= 'A') && (aChar <= 'Z'))
			{
				aKey = SDLK_a + (aChar - 'A');
				needsShift = true;
			}
			else if (aShifted != NULL)
			{
				aKey = kRealPlainKeys[aShifted - kRealShiftedKeys];
				needsShift = true;
			}
			else if ((aChar >= 0x20) && (aChar <= 0x7E))
				aKey = aChar;
			else
			{
				*theError = "type takes printable ASCII only (text sends any)";
				return SCRIPT_FAILED;
			}
			// with Caps Lock on a capital needs no Shift and a small letter does
			if ((aKey >= SDLK_a) && (aKey <= SDLK_z) && ((mRealMods & KMOD_CAPS) != 0))
				needsShift = !needsShift;

			bool pressShift = (needsShift) && ((mRealMods & KMOD_SHIFT) == 0);
			if (pressShift)
				PushRealKey(SDLK_LSHIFT, true, false);
			PushRealKey(aKey, true, false);
			PushRealKey(aKey, false, false);
			if (pressShift)
				PushRealKey(SDLK_LSHIFT, false, false);
		}
	}
	else if (aName == "text")
		PushRealText(UnquoteRealText(aCommand.mArgs));
	else if ((aName == "focus") && ((aCommand.mArgs == "lost") || (aCommand.mArgs == "lose") ||
		(aCommand.mArgs == "gained") || (aCommand.mArgs == "regain")))
	{
		bool isLost = (aCommand.mArgs == "lost") || (aCommand.mArgs == "lose");
		if (isLost)
		{
			// SDL_ResetKeyboard: every held key is released, in scancode order, before the window's FOCUS_LOST
			std::vector<int> aKeys = mRealKeysDown;
			std::sort(aKeys.begin(), aKeys.end(),
				[](int a, int b) { return SDL_GetScancodeFromKey(a) < SDL_GetScancodeFromKey(b); });
			for (size_t i = 0; i < aKeys.size(); i++)
				PushRealKey(aKeys[i], false, false);
		}
		PushRealWindowEvent(isLost ? SDL_WINDOWEVENT_FOCUS_LOST : SDL_WINDOWEVENT_FOCUS_GAINED);
	}
	else if ((aName == "minimize") || (aName == "restore") || (aName == "maximize") || (aName == "resize"))
	{
		if (mHWnd == NULL)
		{
			*theError = "no window yet";
			return SCRIPT_FAILED;
		}
		if (aName == "minimize")
			SDL_MinimizeWindow(mHWnd);
		else if (aName == "restore")
			SDL_RestoreWindow(mHWnd);
		else if (aName == "maximize")
			SDL_MaximizeWindow(mHWnd);
		else if (anArgs >> x >> y)
			SDL_SetWindowSize(mHWnd, x, y);
		else
		{
			*theError = "resize needs <w> <h>";
			return SCRIPT_FAILED;
		}
	}
	else if (aName == "close")
	{
		PushRealWindowEvent(SDL_WINDOWEVENT_CLOSE);
		SDL_Event anEvent;
		SDL_zero(anEvent);
		anEvent.type = SDL_QUIT;
		PushRealEvent(&anEvent);
	}
	else if ((aName == "screenshot") && (!aCommand.mArgs.empty()))
		SaveScreenshot(aCommand.mArgs);
	else if ((aName == "screenshotfull") && (!aCommand.mArgs.empty()))
		SaveScreenshot(aCommand.mArgs, true);
	else if (aName == "quit")
		Shutdown();
	else if ((aName == "pointer") && (mWidgetManager != NULL))
	{
		// the mouse as the widgets last saw it, in game pixels, answered on the log: a test reads where its input landed
		// (tools/web/pagecheck.js, a click after the page's canvas changed size)
		SDL_Log("POINTER {\"x\":%d,\"y\":%d,\"in\":%s}", mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY,
			mWidgetManager->mMouseIn ? "true" : "false");
		return SCRIPT_GAME;
	}
	else if (HandleScriptCommand(aCommand))
		return SCRIPT_GAME;
	else
		return SCRIPT_UNKNOWN;
	return SCRIPT_RAN;
}

//////////////////////////////////////////////////////////////////////////
// port: the mod menu and restarts (docs/MOD_MENU_DESIGN.md 5, 6)

void SexyAppBase::OpenModMenu()
{
	if ((mModMenu != NULL) || (mShutdown) || (!mInitialized) || (mWidgetManager == NULL))
		return;

	MTAutoDisallowRand aDisallowRand;
	mModMenu = new ModMenu(this);
	mModMenu->Resize(0, 0, mWidth, mHeight);
	// Above every deferred overlay a game uses (Bookworm Adventures' title logo at 1, its board at 10000): the widget
	// manager draws an overlay before the first widget whose priority is above the overlay's, so at the default 0 the
	// menu would lie under them all
	mModMenu->mPriority = 0x7FFFFFF0;
	mWidgetManager->AddWidget(mModMenu);
	mWidgetManager->BringToFront(mModMenu);
	// As AddDialog does for a modal dialog: the widgets below get no mouse or keys, and the game pauses
	mWidgetManager->AddBaseModal(mModMenu);
	mWidgetManager->SetFocus(mModMenu);
	ModalOpen();
}

void SexyAppBase::CloseModMenu()
{
	if (mModMenu == NULL)
		return;

	MTAutoDisallowRand aDisallowRand;
	ModMenu* aMenu = mModMenu;
	mModMenu = NULL;
	// Closed by a real key (Esc, Enter or Space typed into it): that key's repeats stay out of the game
	if ((mKeyInHandling != KEYCODE_UNKNOWN) && (mKeyInHandling != KEYCODE_F2))
		mModMenuClosingKey = mKeyInHandling;
	aMenu->Closing();
	// KillDialog's order: the widget, the game's modal hook, then the focus back to what had it
	mWidgetManager->RemoveWidget(aMenu);
	ModalClose();
	mWidgetManager->RemoveBaseModal(aMenu);
	SafeDeleteWidget(aMenu);
}

bool SexyAppBase::CanRestartNow(SexyString* /*theReason*/)
{
	return true;
}

bool SexyAppBase::HasGameInProgress()
{
	return false;
}

// Strips the leading dashes and the value: "-gamedir=x" -> "gamedir"
static std::string CmdLineName(const std::string& theArg)
{
	size_t aStart = theArg.find_first_not_of('-');
	if (aStart == std::string::npos)
		return "";
	size_t anEnd = theArg.find('=');
	return theArg.substr(aStart, (anEnd == std::string::npos) ? std::string::npos : anEnd - aStart);
}

void SexyAppBase::GetRestartArgs(std::vector<std::string>* theArgs)
{
	// The same game files, user dir and mods; the session's render scale and placeholders. Not -nomods (the restart
	// is how the menu applies mods), not -seed (a test hook: the same boards again) and no other test hook.
	static const char* const aKept[] = { "gamedir", "userdir", "modsdir", "changedir", "renderscale", "placeholders" };
	for (const std::string& anArg : mCmdLineArgs)
	{
		std::string aName = CmdLineName(anArg);
		for (const char* aKeptName : aKept)
		{
			if (aName == aKeptName)
			{
				theArgs->push_back(anArg);
				break;
			}
		}
	}
}

void SexyAppBase::GetModMessages(std::vector<ModMessage>* theMessages)
{
	// port: a game that has its own messages adds these first (SexyAppBase::GetModMessages(theMessages))
	size_t aMissing = Platform::GetMissing().size();
	if (aMissing > 0)
	{
		ModMessage aMessage;
		aMessage.mModId = "";
		aMessage.mText = (aMissing == 1) ? "1 game file is missing; a placeholder stands in for it" :
			std::to_string(aMissing) + " game files are missing; placeholders stand in for them";
#ifndef __EMSCRIPTEN__
		aMessage.mText += " (missing-files.txt in the user folder)";
#endif
		aMessage.mIsError = false;
		theMessages->push_back(aMessage);
	}
}

bool SexyAppBase::IsTestRun()
{
	return (SDL_getenv("BYOPOP_TEST_RUN") != NULL) || (SDL_getenv("BEJ_TEST_DESKTOP") != NULL) || (!mScriptFileName.empty()) ||
		(mScriptedInput) || (!mRealInputFileName.empty()) || (!mCommandsFileName.empty());
}

int SexyAppBase::RequestRestart(bool isConfirmed, SexyString* theMessage)
{
	SexyString aMessage;
	if ((mShutdown) || (!CanRestartNow(&aMessage)))
	{
		if (theMessage != NULL)
			*theMessage = aMessage;
		return RESTART_BLOCKED;
	}
	if ((!isConfirmed) && (HasGameInProgress()))
	{
		// Where the game in progress goes: it is saved either way, but a score key change files it with the mods it
		// was played with. The key is the one the start applies: values held for the next fresh game stay held.
		Platform::ModManager& aMods = Platform::ModManager::Get();
		if (aMods.GetRestartScoreKey() != aMods.GetScoreKey())
			aMessage = GetString("MODMENU_RESTART_ASK_KEY", _S("Restart now? Your game stays saved with the current mods and continues when you switch back to them."));
		else
			aMessage = GetString("MODMENU_RESTART_ASK", _S("Restart now? Your game is saved and you can continue it after the restart."));
		if (theMessage != NULL)
			*theMessage = aMessage;
		return RESTART_CONFIRM;
	}

	mRestartRequested = true;
	SDL_Log("restart: shutting down to start again");
	// The new window should come to the front, but Shutdown hides this one at once (Windows only)
	Platform::AllowRelaunchForeground();
	// The game's own shutdown: its save and the settings, as on Quit
	Shutdown();
	return RESTART_STARTED;
}

bool SexyAppBase::TakeRelaunch(std::vector<std::string>* theArgs)
{
	theArgs->clear();
	if (!mRestartRequested)
		return false;

	GetRestartArgs(theArgs);
	bool isTestRun = IsTestRun();
	const char* aTestArgs = SDL_getenv("BYOPOP_RELAUNCH_TEST_ARGS");
	bool hasTestArgs = (aTestArgs != NULL) && (*aTestArgs != 0);
	if (isTestRun && hasTestArgs)
	{
		// Words; "..." for one with spaces
		std::string aText = aTestArgs;
		size_t i = 0;
		while (i < aText.size())
		{
			while ((i < aText.size()) && isspace((uchar) aText[i]))
				i++;
			if (i >= aText.size())
				break;
			std::string aWord;
			bool isQuoted = false;
			for (; (i < aText.size()) && (isQuoted || !isspace((uchar) aText[i])); i++)
			{
				if (aText[i] == '"')
					isQuoted = !isQuoted;
				else
					aWord += aText[i];
			}
			theArgs->push_back(aWord);
		}
	}

	std::string aLine;
	for (const std::string& anArg : *theArgs)
		aLine += " " + Platform::QuoteCommandLineArg(anArg);
	if (isTestRun && !hasTestArgs)
	{
		SDL_Log("restart:%s (a test run: the game does not start again)", aLine.c_str());
		return false;
	}
	SDL_Log("restart:%s", aLine.c_str());
	return true;
}

// Draws the whole screen now and saves it as a PNG, at the logical size or (fullResolution) at the
// render target's size, which shows HD art (see -renderscale).
bool SexyAppBase::SaveScreenshot(const std::string& theFileName, bool fullResolution)
{
	MTAutoDisallowRand aDisallowRand;

	mIsDrawing = true;
	mDrawingLogicalShot = !fullResolution;
	mDDInterface->BeginFrame();
	mWidgetManager->MarkAllDirty();
	mWidgetManager->DrawScreen();
	mDrawingLogicalShot = false;
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

	// port: a run with a test flag only on rundesk's private desktop (CLAUDE.md, "Running games in tests"; rundesk sets
	// BYOPOP_TEST_RUN for everything it starts). Without that mark the run stops here, before its window can open on the
	// user's screen: a test save starts full screen in some games, and Bejeweled 2's took over the user's monitor
	// (2026-11-08). ORACLE_VISIBLE=1 is the user's own visible run (games/bejeweled/tools/oracle/compare.sh).
#ifndef __EMSCRIPTEN__
	{
		bool hasTestFlag = (!mScriptFileName.empty()) || (mScriptedInput) || (!mRealInputFileName.empty()) ||
			(!mCommandsFileName.empty()) || (mCommandsHold) || (mFastMode) || (mHasSeedParam) || (mTraceMode) || (mDrawEvery);
		const char* aVisible = SDL_getenv("ORACLE_VISIBLE");
		bool isMarked = (SDL_getenv("BYOPOP_TEST_RUN") != NULL) || (SDL_getenv("BEJ_TEST_DESKTOP") != NULL) ||
			((aVisible != NULL) && (aVisible[0] != 0));
		if (hasTestFlag && !isMarked)
		{
			const char* aText = "a test flag (-script, -autoplay, -realinput, -commands, -hold, -fast, -seed, -trace or "
				"-drawevery) outside rundesk: start the game, or the script that starts it, under build/native/rundesk.exe "
				"(CLAUDE.md, \"Running games in tests\")";
			fprintf(stderr, "%s\n", aText);
			SDL_Log("%s", aText);
			DoExit(3);
		}
	}
#endif

	// port: -fast moves the sound effects on in DoUpdateFrames instead of in real time
	if (mFastMode)
		Platform::Mixer::Get().SetGameClocked(true);

	// Change directory. port: the game data root is the VFS base dir (set up by main())
	if ((!mChangeDirTo.empty()) && (!ChangeDirHook(mChangeDirTo.c_str())))
		Platform::VFS::Get().SetBaseDir(mChangeDirTo);

	// port: -seed=N replaces the tick count
	if (!mHasSeedParam)
		mRandSeed = GetTickCount();
	SRand(mRandSeed);

	if ((!mScriptFileName.empty()) && (!LoadScript(mScriptFileName)))
		DoExit(1);
	// -realinput: not with -script, whose filter in HandleEvent drops every real event
	if (!mRealInputFileName.empty())
	{
		if (!mScriptFileName.empty())
		{
			SDL_Log("realinput: -realinput does not go with -script, which drops real input");
			DoExit(1);
		}
		if (!LoadScriptFile(mRealInputFileName, "realinput", &mRealInput))
			DoExit(1);
		mRealInputPos = 0;
	}

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
	DDImage* anImage = GetOptionalImage(theFileName, commitBits);
	if ((anImage == NULL) && (Platform::Placeholders()))
	{
		// port: -placeholders (platform/Placeholders.h): a stand-in at the size the game expects, so its cels and
		// layout stay where they were; a new image each time, since every game deletes its own
		int aWidth = 64;
		int aHeight = 64;
		GetPlaceholderSize(theFileName, &aWidth, &aHeight);
		anImage = new DDImage(mDDInterface);
		anImage->mFilePath = theFileName;
		anImage->mPlaceholder = true;
		GrowPlaceholder(anImage, (aWidth >= 1) ? aWidth : 64, (aHeight >= 1) ? aHeight : 64);
		Platform::NoteMissing(theFileName);
	}
	return anImage;
}

Sexy::DDImage* SexyAppBase::GetOptionalImage(const std::string& theFileName, bool commitBits)
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

void SexyAppBase::GetPlaceholderSize(const std::string& /*theFileName*/, int* /*theWidth*/, int* /*theHeight*/)
{
}

void SexyAppBase::GrowPlaceholder(Image* theImage, int theWidth, int theHeight)
{
	// port: a magenta and black checkerboard, opaque; its squares a quarter of the shorter side, 2 to 16 px
	MemoryImage* anImage = dynamic_cast<MemoryImage*>(theImage);
	if ((anImage == NULL) || (!anImage->mPlaceholder))
		return;
	int aWidth = std::max(theWidth, anImage->mWidth);
	int aHeight = std::max(theHeight, anImage->mHeight);
	if ((aWidth == anImage->mWidth) && (aHeight == anImage->mHeight) && (aWidth > 0) && (aHeight > 0))
		return;
	int aSquare = std::min(16, std::max(2, std::min(aWidth, aHeight) / 4));
	std::vector<ulong> aBits((size_t) aWidth * aHeight);
	for (int y = 0; y < aHeight; y++)
	{
		for (int x = 0; x < aWidth; x++)
			aBits[(size_t) y * aWidth + x] = (((x / aSquare) + (y / aSquare)) % 2 == 0) ? 0xFFFF00FF : 0xFF000000;
	}
	anImage->SetBits(&aBits[0], aWidth, aHeight, true);
}

Sexy::DDImage* SexyAppBase::CreateCrossfadeImage(Sexy::Image* theImage1, const Rect& theRect1, Sexy::Image* theImage2, const Rect& theRect2, double theFadeFactor)
{
	MemoryImage* aMemoryImage1 = dynamic_cast<MemoryImage*>(theImage1);
	MemoryImage* aMemoryImage2 = dynamic_cast<MemoryImage*>(theImage2);

	// port: a placeholder (-placeholders) grows to hold the parts asked for (see FitPlaceholder in Graphics.cpp)
	GrowPlaceholder(theImage1, theRect1.mX + theRect1.mWidth, theRect1.mY + theRect1.mHeight);
	GrowPlaceholder(theImage2, theRect2.mX + theRect2.mWidth, theRect2.mY + theRect2.mHeight);

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

	// port: HD sources give the crossfade HD pixels too (the same blend on them), so a hovered gem stays
	// sharp at HD render scales. 1x output does not change: at render scale 1 the 1x pixels stay the
	// texture, as in the original, and so they do for a screenshot at the logical size, which is drawn
	// at the render scale and box-downsampled (the regression oracle's screenshots).
	if ((mDDInterface != NULL) && (mDDInterface->mRenderScale > 1) && (!mDrawingLogicalShot))
	{
		MemoryImage* aSources[2] = { aMemoryImage1, aMemoryImage2 };
		const Rect* aRects[2] = { &theRect1, &theRect2 };
		int aScales[2];
		const uint32_t* aPixels[2];  // each source's pixels at its own scale
		int aPitches[2];
		int aScale = 1;
		for (int i = 0; i < 2; i++)
		{
			bool isHD = aSources[i]->HasMirrorableHDBits();
			aScales[i] = isHD ? aSources[i]->mHDScale : 1;
			aPixels[i] = isHD ? &aSources[i]->mHDBits[0] : (const uint32_t*) (i == 0 ? aSrcBits1 : aSrcBits2);
			aPitches[i] = isHD ? aSources[i]->mHDWidth : aSources[i]->mWidth;
			aScale = std::max(aScale, aScales[i]);
		}
		// a source at another density is sampled down to its own pixels (nearest); one that does not
		// divide the HD scale leaves the crossfade at 1x
		bool aCanHD = (aScale > 1) && (aScale % aScales[0] == 0) && (aScale % aScales[1] == 0);
		if (aCanHD)
		{
			int aHDWidth = aWidth * aScale;
			int aHDHeight = aHeight * aScale;
			std::vector<uint32_t> anHD((size_t) aHDWidth * aHDHeight);
			const uint32_t* aRows[2];
			for (int y = 0; y < aHDHeight; y++)
			{
				for (int i = 0; i < 2; i++)
				{
					int aSrcY = (aRects[i]->mY * aScale + y) * aScales[i] / aScale;
					aRows[i] = aPixels[i] + (size_t) aSrcY * aPitches[i];
				}
				uint32_t* d = &anHD[(size_t) y * aHDWidth];
				for (int x = 0; x < aHDWidth; x++)
				{
					uint32_t p1 = aRows[0][(theRect1.mX * aScale + x) * aScales[0] / aScale];
					uint32_t p2 = aRows[1][(theRect2.mX * aScale + x) * aScales[1] / aScale];
					*d++ =
						((((p1 & 0x000000FF)*aOMM + (p2 & 0x000000FF)*aMult)>>8) & 0x000000FF) |
						((((p1 & 0x0000FF00)*aOMM + (p2 & 0x0000FF00)*aMult)>>8) & 0x0000FF00) |
						((((p1 & 0x00FF0000)*aOMM + (p2 & 0x00FF0000)*aMult)>>8) & 0x00FF0000) |
						((((p1 >> 24)*aOMM + (p2 >> 24)*aMult)<<16) & 0xFF000000);
				}
			}
			anImage->SetHDBits(&anHD[0], aHDWidth, aHDHeight, aScale);
		}
	}

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
	// the web page's output has sound mixed ahead: dropped, so that the mute is heard at once, as natively
	Platform::Mixer::Get().DropQueued();
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
	// port: SDL's renderer is the only display path, and it stays the GPU's whenever the system allows it. What 3D off
	// changes is what the game and the framework's own drawing see (Is3DAccelerated, Graphics' mIs3D): the software
	// drawing paths of PopCap's 2D mode. The choice is kept (mWant3D), written as "Is3D" and applied again each time the
	// display is made; no re-init is needed, so reinit has nothing to do
	if (mDDInterface == NULL)
		return;
	bool aIs3D = is3D && mDDInterface->mCan3D;
	if (mDDInterface->mIs3D == aIs3D && mWant3D == is3D)
		return;
	mUserChanged3DSetting = true;
	mWant3D = is3D;
	mDDInterface->mIs3D = aIs3D;
	mWidgetManager->MarkAllDirty();
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
