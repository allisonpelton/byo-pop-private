#ifndef __MAHJONGJAPAN_MANAGER_H__
#define __MAHJONGJAPAN_MANAGER_H__

// The engine's manager (vtable 0x51e3d8, 0x42a0d0..0x42cc90), embedded in the app at +0x34: the layer stack, the
// screen fade, the lists objects register in for input and updates, the frame, and the forwards to the display
// (+4) and the sound object (+0xb0). See docs/ENGINE.md, "The manager".
//
// Port: the window (+0x28), its timer and its cursors are the host's (Host.h); the host calls Frame on its
// timer and the input handlers from its events, as the window procedure did.

#include "JBBase.h"

#include <vector>

namespace jb
{

class Display;
class Font;
class Image;
class Layer;
class Object;
class Sound;
struct ImageInfo;

class Manager
{
public:
	Display* mDisplay = nullptr;     // +0x04
	int mPeriod = 0;                 // +0x08 ms per frame
	bool mPaused = true;             // +0x0c
	bool mRunning = false;           // +0x0d
	bool mSuspended = false;         // +0x0e
	int mPauseCount = 0;             // +0x10
	bool mMinimized = false;         // +0x14
	uint32_t mStart = 0;             // +0x18 the clock's start (moved on by the time paused)
	uint32_t mPauseStart = 0;        // +0x1c
	uint32_t mLastPresent = 0;       // +0x20
	uint32_t mLastFrame = 0;         // +0x24
	bool mActive = true;             // +0x2c the app is active
	Object* mCapture = nullptr;      // +0x30 gets every mouse message
	bool mRedoHover = false;         // +0x34 the hover is tested again after the next draw
	std::vector<Layer*> mLayers;     // +0x3c the stack, top last
	ListNode* mHovers = nullptr;     // +0x48
	ListNode* mClicks = nullptr;     // +0x4c
	ListNode* mMoves = nullptr;      // +0x50
	ListNode* mKeys = nullptr;       // +0x54 WM_KEYDOWN, until one takes it
	ListNode* mChars = nullptr;      // +0x58 WM_CHAR, to all
	ListNode* mUpdaters = nullptr;   // +0x5c
	ListNode* mAnimators = nullptr;  // +0x60 darkening layers
	ListNode* mTransitions = nullptr;  // +0x64 layers fading in or out
	Object* mHover = nullptr;        // +0x68
	Object* mNextHover = nullptr;    // +0x6c
	Object* mDrag = nullptr;         // +0x70
	Rect mDragClamp;                 // +0x74
	Point mDragStart;                // +0x84 the mouse
	Point mDragOrigin;               // +0x8c the object
	Point mDragPos;                  // +0x94
	int mDragFlags = 0;              // +0x9c 1 move the object, 2 send it the moves
	int mClock = 0;                  // +0xa4 the game's clock
	uint32_t mPaceBase = 0;          // +0xa8
	uint16_t mPaceCount = 0;         // +0xac
	Sound* mSound = nullptr;         // +0xb0
	bool mCursorLocked = false;      // +0xc4 an object chose the cursor
	int mFade = 0;                   // +0xc8 1 out, 3 switch, 2 in
	int mFadeAlpha = 0;              // +0xcc
	Layer* mNextLayer = nullptr;     // +0xd0
	bool mFadeSkip = false;          // +0xd4
	int mWaitCount = 0;              // +0xd8 (nothing raises it; see 0x42c8d0)
	int mCursor = 0;                 // +0xe0 the software cursor's kind
	bool mCursorDirty = true;        // +0xe4

	// 0x42a0d0
	Manager();
	// 0x42a230
	virtual ~Manager();

	// 0x42a2c0
	void Stop();
	// 0x42a330
	int GetPeriod() const { return mPeriod; }
	// 0x42a340: the display's active flag
	bool IsAppActive();
	// 0x42a350.. : the registrations
	void AddHover(Object* theObject) { ListAdd(mHovers, theObject); }
	void RemoveHover(Object* theObject) { ListRemove(mHovers, theObject); }
	void AddClick(Object* theObject) { ListAdd(mClicks, theObject); }
	void RemoveClick(Object* theObject) { ListRemove(mClicks, theObject); }
	// 0x42a400 inlined here: the add to the move list (+0x50)
	void AddMove(Object* theObject) { ListAdd(mMoves, theObject); }
	// 0x42a410 inlined here: the removal from the move list (+0x50)
	void RemoveMove(Object* theObject) { ListRemove(mMoves, theObject); }
	// 0x42a460 inlined here: the add to the key list (+0x54)
	void AddKey(Object* theObject) { ListAdd(mKeys, theObject); }
	// 0x42a470 inlined here: the removal from the key list (+0x54)
	void RemoveKey(Object* theObject) { ListRemove(mKeys, theObject); }
	// 0x42a4c0 inlined here: the add to the character list (+0x58)
	void AddChar(Object* theObject) { ListAdd(mChars, theObject); }
	// 0x42a4d0 inlined here: the removal from the character list (+0x58)
	void RemoveChar(Object* theObject) { ListRemove(mChars, theObject); }
	// 0x42a520 inlined here: the add to the updaters (+0x5c)
	void AddUpdater(Object* theObject) { ListAdd(mUpdaters, theObject); }
	// 0x42a530 inlined here: the removal from the updaters (+0x5c)
	void RemoveUpdater(Object* theObject) { ListRemove(mUpdaters, theObject); }
	// 0x42c060
	void AddAnimator(Layer* theLayer) { ListAdd(mAnimators, theLayer); }
	// 0x42a580
	bool GetActive() const { return mActive; }
	// 0x42a590
	bool IsRunning() const { return mRunning && !mPaused; }
	// 0x42a5c0: every layer's Slot18, then Stop
	void Shutdown();
	// 0x42a5f0: the display and the sound start, the frame's clocks
	bool Init(bool theFullScreen, int thePeriod);
	// 0x42c300: false when the display failed
	bool SetMode(bool theFullScreen, bool theUseSetting);
	// 0x42a800
	void PresentDone();
	// 0x42a820: only when the screen was invalidated
	void Draw();
	// 0x42a9f0: theLayer on top; theOverlay keeps the layers under it drawn, theDarken darkens the one it covers,
	// theAnimate fades it in
	void Push(Layer* theLayer, bool theOverlay, bool theDarken, bool theAnimate);
	// 0x42ac00: the top layer that is not fading out goes inactive
	void DeactivateTop(bool theDarken);
	// 0x42ac60: the new top layer, active again
	Layer* Pop(bool theAnimate, int theCount);
	// 0x42acb0, 0x42adb0: the new top layer
	Layer* PopNow(int theCount);
	Layer* PopAnimated(int theCount);
	// 0x42ae80: the screen fades to black and changes to theLayer
	void SwitchTo(Layer* theLayer);
	// 0x42aed0
	void SwitchNow();
	// 0x42b0b0
	void FadeDone();
	// 0x42b0d0: the top layer replaced
	void Replace(Layer* theLayer, bool theAnimateOut, bool theAnimateIn);
	// 0x42b2d0: deleted, after its Slot18 when theNotify
	void DeleteLayer(Layer* theLayer, bool theNotify);
	// 0x42b340
	void ClearLayers(bool theNotify);
	// 0x42b3b0, 0x42b620, 0x42b6e0, 0x42b840, 0x42b910, 0x42b9b0, 0x42ba70
	void MouseMove(int theFlags, int theX, int theY);
	void LButtonDown(int theFlags, int theX, int theY);
	void LButtonDbl(int theFlags, int theX, int theY);
	void LButtonUp(int theFlags, int theX, int theY);
	void RButtonDown(int theFlags, int theX, int theY);
	void RButtonDbl(int theFlags, int theX, int theY);
	void RButtonUp(int theFlags, int theX, int theY);
	// 0x42b5c0
	void SendHover(bool theIn, int theFlags);
	// 0x42b5f0
	void SetObjectCursor(Object* theObject, int theCursor, bool theLock);
	// 0x42b7a0
	void StartDrag(Object* theObject, int theFlags, int theLeft, int theTop, int theRight, int theBottom);
	// 0x42bb10
	void DragTo(int theX, int theY);
	// 0x42bb90
	void Activate(bool theActive);
	// 0x42bc50
	void RedoHover() { mRedoHover = true; }
	// 0x42bc60, 0x42bcb0
	void Key(int theKey, int theRepeat, int theScan);
	void Char(int theChar, int theRepeat, int theScan);
	// 0x42bd30
	int GetClock() const { return mClock; }
	// 0x42bd40: WM_TIMER
	void Frame();
	// 0x42be10
	void Update(int theClock, uint32_t theDelta);
	// 0x42c090: WM_SIZE
	void OnSize(int theType);
	// 0x42c0f0
	void Redraw();
	// 0x42c110, 0x42c140
	void Pause(bool thePause);
	void PauseClock(bool thePause);
	// 0x42c190, 0x42c220, 0x42c2a0: the display mode changes
	bool SetHardwareAccel(bool theOn);
	bool SetFullScreen(bool theOn, bool theReset);
	bool SetWideScreen(bool theOn);
	// 0x42c370..0x42c530: the sound object's
	void LoadSound(int theId, bool theLoop);
	void PlaySound(int theId);
	void StopSound(int theId);
	void StopSoundsExcept(int theId);
	void PauseSounds(bool thePause, bool theLoopingOnly, int theExcept);
	void LoadMusic(int theId);
	void PlayMusic(int theId, bool theRestart, int theFadeMs, int theOrder);
	void PauseMusic(bool thePause);
	void SetSoundVolume(int theVolume);
	void SetMusicVolume(int theVolume);
	void SlideMusic(int theVolume, int theMs);
	void SetMusicLoop(int theRowStep, int theOrder);
	// 0x42c550, 0x42c5c0
	bool GetClipboard(std::string& theText);
	bool SetClipboard(const char* theText);
	// 0x42c6b0..0x42c870: the display's
	bool LoadFileImage(int theId, const char* thePath);
	void PreloadExeImage(int theId);
	void PreloadDllImage(int theId);
	void FreeImage(int theId);
	Image* CreateImage(int theWidth, int theHeight);
	Image* GetImage(int theId, Module theModule);
	void DrawImage(int theX, int theY, Image* theImage, const Rect* theSrc, int theScaleX, int theScaleY,
		int theAlpha, Rect& theOut);
	void DrawImageKeyed(int theX, int theY, Image* theImage, uint32_t theKey, const Rect* theSrc, int theScaleX,
		int theScaleY, int theAlpha, Rect& theOut);
	void GetImageInfo(int theId, ImageInfo& theInfo);
	Font* GetFont(int theImageId, int theTextId, int theKind, Module theModule);
	// 0x42c890: capture on, or off (with the capture's OnRelease)
	void SetCapture(Object* theObject, bool theOn);
	// 0x42c8d0
	void EndWait();
	// 0x42a5b0 (0x434dc0 on the display; the static wrapper 0x4215e0): the display's +0x50, with which the present
	// waits for the vertical blank (0x433e90) while the map's curtain moves; nothing in the port
	void SetVBlankWait(bool theOn) {}
	// 0x42c8f0
	void SetCursor(int theKind);
};

// 0x5d8cac
extern Manager* gManager;
// 0x5d8e9d: a layer was deleted during the frame's lists
extern bool gLayerDeleted;
// 0x527ba0: the screen is drawn on the next frame
extern bool gInvalidated;
// 0x42a7f0
inline void Invalidate() { gInvalidated = true; }
// 0x42c810, 0x42c830: the screen's fills (the alpha fill clips theRect to the screen, which its callers then add
// to the dirty rects)
void FillRectAlpha(Rect& theRect, int theAlpha, uint32_t theColour);
void FillRect(const Rect* theRect, uint32_t theColour);

} // namespace jb

#endif
