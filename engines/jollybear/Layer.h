#ifndef __JB_LAYER_H__
#define __JB_LAYER_H__

// A screen or a dialog of the engine (vtable 0x51ce4c (mahjongchina), 0x51df5c (mahjongjapan)): its top objects, a
// focus ring, a fade in or out, and the darkening it draws over itself while it is inactive under a dialog. The manager
// keeps the layers in a stack.

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
	bool mXLowByteSet;                  // +0x08 the low byte of the constructor's x is not zero; nothing reads it
	std::vector<Object*> mObjects;      // +0x0c
	int mX, mY;                         // +0x18, +0x1c
	bool mActive = true;                // +0x20
	int mAlpha = 0xff;                  // +0x24
	int mDarken = 0;                    // +0x28 grows to 0x80 while inactive
	int mFade = 0;                      // +0x2c 1 fading out, 2 fading in
	bool mFadeSkip = false;             // +0x30 the first step of a fade is skipped
	bool mXLowByteSetToo;               // +0x34 the same; nothing reads it
	std::vector<Object*> mFocus;        // +0x38
	int mFocusIndex = -1;               // +0x44
	bool mDarkens = true;               // +0x48
	bool mOpaque = false;               // +0x49 the layers under it are not drawn
	bool mPauseSounds = true;           // +0x4a its activation pauses the looping sounds
	ListNode* mCmdListeners = nullptr;  // +0x4c

	// 0x4230b0 (mahjongchina), 0x420840 (mahjongjapan)
	Layer(int theX, int theY);
	// 0x423140 (mahjongchina), 0x4208d0 (mahjongjapan)
	virtual ~Layer();

	virtual void Load() = 0;                       // +0x04 when pushed (SetManager)
	virtual void OnAppActivate(bool theActive) {}  // +0x08 (0x428dc0 (mahjongjapan))
	virtual bool OnCommand(int theCmd) { return false; }  // +0x0c (0x4209e0 (mahjongjapan))
	virtual void BeforeActiveChange() {}           // +0x10
	virtual void AfterActiveChange() {}            // +0x14
	virtual void OnRemove() {}                     // +0x18 deleted from the stack, or the app shuts down
	virtual void OnEntered() {}                    // +0x1c the fade to it reached black
	virtual void OnShown() {}                      // +0x20 the fade to it ended
	// +0x24 Enter in a text edit (0x431b20 (mahjongchina), 0x428060 (mahjongjapan))
	virtual void OnTextEnter() {}

	// 0x4231c0 (mahjongchina), 0x420950 (mahjongjapan): the objects' trees, deleted
	void DestroyObjects();
	// 0x4209f0 (mahjongjapan)
	void SetPos(int theX, int theY) { mX = theX; mY = theY; }
	// 0x420a10 (mahjongjapan)
	bool IsActive() const { return mActive; }
	// 0x420a20 (mahjongjapan): the app is active
	bool AppActive();
	// 0x420a30 (mahjongjapan), 0x420a40 (mahjongjapan)
	void SetOpaque(bool theOn) { mOpaque = theOn; }
	bool IsOpaque() const { return mOpaque; }
	// 0x420a60 (mahjongjapan), 0x42a3b0 (mahjongjapan)
	void AddCmdListener(Object* theObject) { ListAdd(mCmdListeners, theObject); }
	void RemoveCmdListener(Object* theObject) { ListRemove(mCmdListeners, theObject); }
	// 0x420a70 (mahjongjapan)
	void SetPauseSounds(bool theOn) { mPauseSounds = theOn; }
	// 0x420a80 (mahjongjapan)
	bool IsFadingOut() const { return mFade == 1; }
	// 0x423250 (mahjongchina), 0x420a90 (mahjongjapan)
	void SetManager(Manager* theManager);
	// 0x423260 (mahjongchina), 0x420aa0 (mahjongjapan)
	void GetPos(int* thePos) const { thePos[0] = mX; thePos[1] = mY; }
	// 0x423280 (mahjongchina), 0x420ac0 (mahjongjapan)
	void Add(Object* theObject, int theX, int theY);
	// 0x423440 (mahjongchina), 0x420c80 (mahjongjapan)
	void AddFocusable(Object* theObject, int theX, int theY);
	// 0x423470 (mahjongchina), 0x420cb0 (mahjongjapan)
	void Draw();
	// 0x4234d0 (mahjongchina), 0x420d10 (mahjongjapan): the darkening's step; false when it is done
	bool StepDarken(int theStep);
	// 0x423510 (mahjongchina), 0x420d50 (mahjongjapan)
	void StartFade(int theFade);
	// 0x423540 (mahjongchina), 0x420d80 (mahjongjapan): false when the fade ended, with *theDelete set for a fade out
	bool StepFade(int theStep, bool* theDelete);
	// 0x4235b0 (mahjongchina), 0x420df0 (mahjongjapan)
	void SetActive(bool theActive, bool theDarken);
	// 0x423630 (mahjongchina), 0x420e70 (mahjongjapan)
	void GetBounds(Rect& theRect);
	// 0x4236a0 (mahjongchina), 0x420ee0 (mahjongjapan)
	void SetClip(struct RegionRect* theRegion);
	// 0x4236d0 (mahjongchina), 0x420f10 (mahjongjapan)
	void AppActivate(bool theActive);
	// 0x423760 (mahjongchina), 0x420fa0 (mahjongjapan)
	void AddFocus(Object* theObject);
	// 0x423920 (mahjongchina), 0x421160 (mahjongjapan), 0x4239b0 (mahjongchina), 0x4211f0 (mahjongjapan),
	// 0x423a40 (mahjongchina), 0x421280 (mahjongjapan)
	void FocusNext();
	void FocusPrev();
	void SetFocus(Object* theObject);
	// 0x423af0 (mahjongchina), 0x421330 (mahjongjapan): the layer's command, then its listeners'
	void Command(int theCmd);
};

} // namespace jb

#endif
