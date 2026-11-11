#ifndef __JB_MANAGER_H__
#define __JB_MANAGER_H__

// The engine's manager (vtable 0x51e3d8 (mahjongjapan), 0x4201a0..0x422d60 (mahjongchina),
// 0x42a0d0..0x42cc90 (mahjongjapan)), embedded in the app at +0x34: the layer stack, the screen fade, the lists objects
// register in for input and updates, the frame, and the forwards to the display (+4) and the sound object (+0xb0). See
// docs/ENGINE.md, "The manager".
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
	int mPauseCount = 0;             // +0x10
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
	int mWaitCount = 0;              // +0xd8 (nothing raises it; see 0x4229a0 (mahjongchina), 0x42c8d0 (mahjongjapan))
	int mCursor = 0;                 // +0xe0 the software cursor's kind

	// 0x4201a0 (mahjongchina), 0x42a0d0 (mahjongjapan)
	Manager();
	// 0x4202e0 (mahjongchina), 0x42a230 (mahjongjapan)
	virtual ~Manager();

	// 0x420380 (mahjongchina), 0x42a2c0 (mahjongjapan)
	void Stop();
	// 0x42a330 (mahjongjapan)
	int GetPeriod() const { return mPeriod; }
	// 0x42a340 (mahjongjapan): the display's active flag
	bool IsAppActive();
	// 0x42a350 (mahjongjapan).. : the registrations
	void AddHover(Object* theObject) { ListAdd(mHovers, theObject); }
	void RemoveHover(Object* theObject) { ListRemove(mHovers, theObject); }
	void AddClick(Object* theObject) { ListAdd(mClicks, theObject); }
	void RemoveClick(Object* theObject) { ListRemove(mClicks, theObject); }
	// 0x42a400 (mahjongjapan) inlined here: the add to the move list (+0x50)
	void AddMove(Object* theObject) { ListAdd(mMoves, theObject); }
	// 0x42a410 (mahjongjapan) inlined here: the removal from the move list (+0x50)
	void RemoveMove(Object* theObject) { ListRemove(mMoves, theObject); }
	// 0x42a460 (mahjongjapan) inlined here: the add to the key list (+0x54)
	void AddKey(Object* theObject) { ListAdd(mKeys, theObject); }
	// 0x42a470 (mahjongjapan) inlined here: the removal from the key list (+0x54)
	void RemoveKey(Object* theObject) { ListRemove(mKeys, theObject); }
	// 0x42a4c0 (mahjongjapan) inlined here: the add to the character list (+0x58)
	void AddChar(Object* theObject) { ListAdd(mChars, theObject); }
	// 0x42a4d0 (mahjongjapan) inlined here: the removal from the character list (+0x58)
	void RemoveChar(Object* theObject) { ListRemove(mChars, theObject); }
	// 0x42a520 (mahjongjapan) inlined here: the add to the updaters (+0x5c)
	void AddUpdater(Object* theObject) { ListAdd(mUpdaters, theObject); }
	// 0x42a530 (mahjongjapan) inlined here: the removal from the updaters (+0x5c)
	void RemoveUpdater(Object* theObject) { ListRemove(mUpdaters, theObject); }
	// 0x4220f0 (mahjongchina), 0x42c060 (mahjongjapan)
	void AddAnimator(Layer* theLayer) { ListAdd(mAnimators, theLayer); }
	// 0x42a580 (mahjongjapan)
	bool GetActive() const { return mActive; }
	// 0x42a590 (mahjongjapan)
	bool IsRunning() const { return mRunning && !mPaused; }
	// 0x4203f0 (mahjongchina), 0x42a5c0 (mahjongjapan): every layer's Slot18, then Stop
	void Shutdown();
	// 0x420430 (mahjongchina), 0x42a5f0 (mahjongjapan): the display and the sound start, the frame's clocks
	bool Init(bool theFullScreen, int thePeriod);
	// 0x4223d0 (mahjongchina), 0x42c300 (mahjongjapan): false when the display failed
	bool SetMode(bool theFullScreen, bool theUseSetting);
	// 0x420660 (mahjongchina), 0x42a800 (mahjongjapan)
	void PresentDone();
	// 0x420680 (mahjongchina), 0x42a820 (mahjongjapan): only when the screen was invalidated
	void Draw();
	// 0x420860 (mahjongchina), 0x42a9f0 (mahjongjapan): theLayer on top; theOverlay keeps the layers under it drawn,
	// theDarken darkens the one it covers, theAnimate fades it in
	void Push(Layer* theLayer, bool theOverlay, bool theDarken, bool theAnimate);
	// 0x420a80 (mahjongchina), 0x42ac00 (mahjongjapan): the top layer that is not fading out goes inactive
	void DeactivateTop(bool theDarken);
	// 0x420ad0 (mahjongchina), 0x42ac60 (mahjongjapan): the new top layer, active again
	Layer* Pop(bool theAnimate, int theCount);
	// 0x420b20 (mahjongchina), 0x42acb0 (mahjongjapan), 0x420c40 (mahjongchina), 0x42adb0 (mahjongjapan): the new top
	// layer
	Layer* PopNow(int theCount);
	Layer* PopAnimated(int theCount);
	// 0x420d20 (mahjongchina), 0x42ae80 (mahjongjapan): the screen fades to black and changes to theLayer
	void SwitchTo(Layer* theLayer);
	// 0x420d70 (mahjongchina), 0x42aed0 (mahjongjapan)
	void SwitchNow();
	// 0x420f50 (mahjongchina), 0x42b0b0 (mahjongjapan)
	void FadeDone();
	// 0x420f80 (mahjongchina), 0x42b0d0 (mahjongjapan): the top layer replaced
	void Replace(Layer* theLayer, bool theAnimateOut, bool theAnimateIn);
	// 0x421190 (mahjongchina), 0x42b2d0 (mahjongjapan): deleted, after its Slot18 when theNotify
	void DeleteLayer(Layer* theLayer, bool theNotify);
	// 0x421200 (mahjongchina), 0x42b340 (mahjongjapan)
	void ClearLayers(bool theNotify);
	// 0x421280 (mahjongchina), 0x42b3b0 (mahjongjapan), 0x4215b0 (mahjongchina), 0x42b620 (mahjongjapan),
	// 0x421680 (mahjongchina), 0x42b6e0 (mahjongjapan), 0x4217f0 (mahjongchina), 0x42b840 (mahjongjapan),
	// 0x4218d0 (mahjongchina), 0x42b910 (mahjongjapan), 0x421970 (mahjongchina), 0x42b9b0 (mahjongjapan),
	// 0x421a30 (mahjongchina), 0x42ba70 (mahjongjapan)
	void MouseMove(int theFlags, int theX, int theY);
	void LButtonDown(int theFlags, int theX, int theY);
	void LButtonDbl(int theFlags, int theX, int theY);
	void LButtonUp(int theFlags, int theX, int theY);
	void RButtonDown(int theFlags, int theX, int theY);
	void RButtonDbl(int theFlags, int theX, int theY);
	void RButtonUp(int theFlags, int theX, int theY);
	// 0x421520 (mahjongchina), 0x42b5c0 (mahjongjapan)
	void SendHover(bool theIn, int theFlags);
	// 0x421570 (mahjongchina), 0x42b5f0 (mahjongjapan)
	void SetObjectCursor(Object* theObject, int theCursor, bool theLock);
	// 0x421740 (mahjongchina), 0x42b7a0 (mahjongjapan)
	void StartDrag(Object* theObject, int theFlags, int theLeft, int theTop, int theRight, int theBottom);
	// 0x421ad0 (mahjongchina), 0x42bb10 (mahjongjapan)
	void DragTo(int theX, int theY);
	// 0x421b60 (mahjongchina), 0x42bb90 (mahjongjapan)
	void Activate(bool theActive);
	// 0x421c30 (mahjongchina), 0x42bc50 (mahjongjapan)
	void RedoHover() { mRedoHover = true; }
	// 0x421c40 (mahjongchina), 0x42bc60 (mahjongjapan), 0x421cb0 (mahjongchina), 0x42bcb0 (mahjongjapan)
	void Key(int theKey, int theRepeat, int theScan);
	void Char(int theChar, int theRepeat, int theScan);
	// 0x421d60 (mahjongchina), 0x42bd30 (mahjongjapan)
	int GetClock() const { return mClock; }
	// 0x421d70 (mahjongchina), 0x42bd40 (mahjongjapan): WM_TIMER
	void Frame();
	// 0x421e60 (mahjongchina), 0x42be10 (mahjongjapan)
	void Update(int theClock, uint32_t theDelta);
	// 0x4221d0 (mahjongchina), 0x42c110 (mahjongjapan), 0x422200 (mahjongchina), 0x42c140 (mahjongjapan)
	void Pause(bool thePause);
	void PauseClock(bool thePause);
	// 0x422260 (mahjongchina), 0x42c190 (mahjongjapan), 0x4222f0 (mahjongchina), 0x42c220 (mahjongjapan),
	// 0x422370 (mahjongchina), 0x42c2a0 (mahjongjapan): the display mode changes
	bool SetHardwareAccel(bool theOn);
	bool SetFullScreen(bool theOn, bool theReset);
	bool SetWideScreen(bool theOn);
	// 0x422430..0x422610 (mahjongchina), 0x42c370..0x42c530 (mahjongjapan): the sound object's
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
	// 0x422630 (mahjongchina), 0x42c550 (mahjongjapan), 0x4226a0 (mahjongchina), 0x42c5c0 (mahjongjapan)
	bool GetClipboard(std::string& theText);
	bool SetClipboard(const char* theText);
	// 0x422790..0x422930 (mahjongchina), 0x42c6b0..0x42c870 (mahjongjapan): the display's
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
	Font* GetFont(int theImageId, int theTextId, Module theModule);
	// 0x422950 (mahjongchina), 0x42c890 (mahjongjapan): capture on, or off (with the capture's OnRelease)
	void SetCapture(Object* theObject, bool theOn);
	// 0x4229a0 (mahjongchina), 0x42c8d0 (mahjongjapan)
	void EndWait();
	// 0x42a5b0 (mahjongjapan) (0x42ecb0 (mahjongchina), 0x434dc0 (mahjongjapan) on the display; the static wrapper
	// 0x4215e0 (mahjongjapan)): the display's +0x50, with which the present waits for the vertical blank
	// (0x42dd80 (mahjongchina), 0x433e90 (mahjongjapan)) while the map's curtain moves; nothing in the port
	void SetVBlankWait(bool theOn) {}
	// 0x4229c0 (mahjongchina), 0x42c8f0 (mahjongjapan)
	void SetCursor(int theKind);
};

// 0x5d8cac (mahjongjapan)
extern Manager* gManager;
// 0x5d8005 (mahjongchina), 0x5d8e9d (mahjongjapan): a layer was deleted during the frame's lists
extern bool gLayerDeleted;
// 0x526a00 (mahjongchina), 0x527ba0 (mahjongjapan): the screen is drawn on the next frame
extern bool gInvalidated;
// 0x420650 (mahjongchina), 0x42a7f0 (mahjongjapan)
inline void Invalidate() { gInvalidated = true; }
// 0x4228d0 (mahjongchina), 0x42c810 (mahjongjapan), 0x4228f0 (mahjongchina), 0x42c830 (mahjongjapan): the screen's
// fills (the alpha fill clips theRect to the screen, which its callers then add to the dirty rects)
void FillRectAlpha(Rect& theRect, int theAlpha, uint32_t theColour);
void FillRect(const Rect* theRect, uint32_t theColour);

} // namespace jb

#endif
