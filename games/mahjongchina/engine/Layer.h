#ifndef __MAHJONGCHINA_LAYER_H__
#define __MAHJONGCHINA_LAYER_H__

// A screen or a dialog of the engine (vtable 0x51df5c): its top objects, a focus ring, a fade in or out, and the
// darkening it draws over itself while it is inactive under a dialog. The manager keeps the layers in a stack.

#include "JBBase.h"

#include <vector>

namespace jb
{

class Manager;
class Object;

class Layer
{
public:
	Manager* mManager = nullptr;        // +0x04
	bool mFlag8;                        // +0x08 the low byte of the x passed to the constructor (unused)
	std::vector<Object*> mObjects;      // +0x0c
	int mX, mY;                         // +0x18, +0x1c
	bool mActive = true;                // +0x20
	int mAlpha = 0xff;                  // +0x24
	int mDarken = 0;                    // +0x28 grows to 0x80 while inactive
	int mFade = 0;                      // +0x2c 1 fading out, 2 fading in
	bool mFadeSkip = false;             // +0x30 the first step of a fade is skipped
	bool mFlag34;                       // +0x34 as +0x08
	std::vector<Object*> mFocus;        // +0x38
	int mFocusIndex = -1;               // +0x44
	bool mDarkens = true;               // +0x48
	bool mOpaque = false;               // +0x49 the layers under it are not drawn
	bool mPauseSounds = true;           // +0x4a its activation pauses the looping sounds
	ListNode* mCmdListeners = nullptr;  // +0x4c

	// 0x420840
	Layer(int theX, int theY);
	// 0x4208d0
	virtual ~Layer();

	virtual void Load() = 0;                       // +0x04 when pushed (SetManager)
	virtual void OnAppActivate(bool theActive) {}  // +0x08 (0x428dc0)
	virtual bool OnCommand(int theCmd) { return false; }  // +0x0c (0x4209e0)
	virtual void BeforeActiveChange() {}           // +0x10
	virtual void AfterActiveChange() {}            // +0x14
	virtual void OnRemove() {}                     // +0x18 deleted from the stack, or the app shuts down
	virtual void OnEntered() {}                    // +0x1c the fade to it reached black
	virtual void OnShown() {}                      // +0x20 the fade to it ended
	virtual void OnTextEnter() {}                  // +0x24 Enter in a text edit (0x428060)

	// 0x420950: the objects' trees, deleted
	void DestroyObjects();
	// 0x4209f0
	void SetPos(int theX, int theY) { mX = theX; mY = theY; }
	// 0x420a10
	bool IsActive() const { return mActive; }
	// 0x420a20: the app is active
	bool AppActive();
	// 0x420a30, 0x420a40
	void SetOpaque(bool theOn) { mOpaque = theOn; }
	bool IsOpaque() const { return mOpaque; }
	// 0x420a60, 0x42a3b0
	void AddCmdListener(Object* theObject) { ListAdd(mCmdListeners, theObject); }
	void RemoveCmdListener(Object* theObject) { ListRemove(mCmdListeners, theObject); }
	// 0x420a70
	void SetPauseSounds(bool theOn) { mPauseSounds = theOn; }
	// 0x420a80
	bool IsFadingOut() const { return mFade == 1; }
	// 0x420a90
	void SetManager(Manager* theManager);
	// 0x420aa0
	void GetPos(int* thePos) const { thePos[0] = mX; thePos[1] = mY; }
	// 0x420ac0
	void Add(Object* theObject, int theX, int theY);
	// 0x420c80
	void AddFocusable(Object* theObject, int theX, int theY);
	// 0x420cb0
	void Draw();
	// 0x420d10: the darkening's step; false when it is done
	bool StepDarken(int theStep);
	// 0x420d50
	void StartFade(int theFade);
	// 0x420d80: false when the fade ended, with *theDelete set for a fade out
	bool StepFade(int theStep, bool* theDelete);
	// 0x420df0
	void SetActive(bool theActive, bool theDarken);
	// 0x420e70
	void GetBounds(Rect& theRect);
	// 0x420ee0
	void SetClip(struct RegionRect* theRegion);
	// 0x420f10
	void AppActivate(bool theActive);
	// 0x420fa0
	void AddFocus(Object* theObject);
	// 0x421160, 0x4211f0, 0x421280
	void FocusNext();
	void FocusPrev();
	void SetFocus(Object* theObject);
	// 0x421330: the layer's command, then its listeners'
	void Command(int theCmd);
};

} // namespace jb

#endif
