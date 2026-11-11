#ifndef __JB_OBJECT_H__
#define __JB_OBJECT_H__

// Jolly Bear's object tree: Object (vtable 0x51cdec (mahjongchina), 0x51df04 (mahjongjapan)), its attachments (vtable
// 0x51d2fc (mahjongchina), 0x51e428 (mahjongjapan)), Button (0x51cd0c (mahjongchina), 0x51de24 (mahjongjapan)), the
// toggle (0x51cd7c (mahjongchina), 0x51de94 (mahjongjapan)) and the radio group (0x51ce48 (mahjongchina),
// 0x51df8c (mahjongjapan)). docs/ENGINE.md, "The object tree", has the layouts; the offsets of the original's fields
// are noted on each member.

#include "Image.h"
#include "JBBase.h"

#include <string>
#include <vector>

namespace jb
{

class Manager;
class Layer;
class Object;
class Image;
class Font;
class RadioGroup;

// A rect of a clip region (0x14 bytes, 0x420780 (mahjongjapan))
struct RegionRect
{
	Rect mRect;
	RegionRect* mNext = nullptr;
};

// 0x423000 (mahjongchina), 0x4207a0 (mahjongjapan): a copy of theRegion moved by (-theX, -theY); returns its last rect
// (the caller keeps the first)
RegionRect* CopyRegion(RegionRect* theRegion, int theX, int theY, RegionRect** theFirst);

// A timer of an object (0x18 bytes, 0x420070 (mahjongchina), 0x420360 (mahjongjapan))
struct ObjTimer
{
	int mId;        // +0x00
	int mLast;      // +0x04 the object's time when it last fired
	int mPeriod;    // +0x08
	int mLeft;      // +0x0c firings left (0 at the start: for ever)
	int mCount;     // +0x10
	int mPerTick;   // +0x14 firings each period
};

// Something drawn with an object, in front of or behind it (vtable 0x51d2fc (mahjongchina), 0x51e428 (mahjongjapan),
// base 0x432860 (mahjongchina), 0x42eed0 (mahjongjapan))
class Attachment
{
public:
	Object* mOwner = nullptr;  // +0x04

	// 0x4297e0 (mahjongchina), 0x42ef00 (mahjongjapan) inlined here: the base destructor, which only restores its
	// vtable
	virtual ~Attachment() {}
	virtual void Load() {}                 // +0x04 (0x4221c0 (mahjongchina), 0x429700 (mahjongjapan)), when attached
	virtual bool Update(int theClock) = 0; // +0x0c the owner's clock; true when it changed
	virtual void Draw() = 0;               // +0x10
	virtual void SetPaused(bool thePaused) = 0;  // +0x14 the owner is disabled
};

class Object
{
public:
	Manager* mManager = nullptr;       // +0x04
	Layer* mLayer = nullptr;           // +0x08
	Object* mParent = nullptr;         // +0x0c
	int mX = 0, mY = 0;                // +0x10, +0x14 relative to the parent
	Rect mDirty;                       // +0x18
	RegionRect* mClip = nullptr;       // +0x28
	RegionRect* mClipTail = nullptr;   // +0x30 (never set: always null)
	RegionRect* mClipOwn = nullptr;    // +0x34
	bool mUseClip = true;              // +0x38
	bool mTrackDirty = true;           // +0x39
	std::vector<Object*> mChildren;    // +0x40
	std::vector<Attachment*> mAttachments;  // +0x50
	bool mVisible = true;              // +0x5d
	bool mVisibleByParent = true;      // +0x5e
	bool mEnabled = true;              // +0x5f
	bool mEnabledByParent = true;      // +0x60
	int mCursor = 0;                   // +0x64 the cursor over it (0x4229c0 (mahjongchina), 0x42c8f0 (mahjongjapan))
	int mAlpha = 0xff;                 // +0x68
	int mStart = 0;                    // +0x6c the manager's time at Create, moved by the paused time
	int mPauseStart = 0;               // +0x70
	bool mPaused = false;              // +0x74
	int mDelta = 0;                    // +0x78 time since the last update
	int mLastUpdate = 0;               // +0x7c
	bool mHoverReg = false;            // +0x80 the manager's hover list (+0x48)
	bool mLayerCmdReg = false;         // +0x81 the layer's command listeners (+0x4c)
	bool mClickReg = false;            // +0x82 the manager's click list (+0x4c)
	bool mMoveReg = false;             // +0x83 the manager's move list (+0x50)
	bool mKeyReg = false;              // +0x84 the manager's key list (+0x54)
	bool mCharReg = false;             // +0x85 the manager's character list (+0x58)
	bool mUpdateReg = false;           // +0x86 the manager's updaters (+0x5c)
	bool mIgnoreWhenDisabled = true;   // +0x87
	bool mUserFlag = false;            // +0x88
	ListNode* mTimers = nullptr;       // +0x8c

	// 0x41e710 (mahjongchina), 0x41ec10 (mahjongjapan) (reseeds the C runtime's generator from the clock, as the
	// original)
	Object();
	// 0x41e7f0 (mahjongchina), 0x41ed00 (mahjongjapan)
	virtual ~Object();

	virtual void Draw() = 0;                                              // +0x04
	virtual void Init() = 0;                                              // +0x08
	// +0x0c (0x4221c0 (mahjongchina), 0x429700 (mahjongjapan))
	virtual void Clear() {}
	virtual bool OnCommand(int theCmd) { return false; }                 // +0x10 (0x4209e0 (mahjongjapan))
	virtual bool OnLButtonDown(int theFlags, int theX, int theY) { return false; }    // +0x14
	virtual bool OnLButtonDblClk(int theFlags, int theX, int theY) { return false; }  // +0x18
	virtual bool OnLButtonUp(int theFlags, int theX, int theY) { return false; }      // +0x1c
	// 0x420490 (mahjongjapan) inlined here: the default that returns false, shared with OnRButtonDblClk, OnRButtonUp
	// and OnKeyDown
	virtual bool OnRButtonDown(int theFlags, int theX, int theY) { return false; }    // +0x20
	virtual bool OnRButtonDblClk(int theFlags, int theX, int theY) { return false; }  // +0x24
	virtual bool OnRButtonUp(int theFlags, int theX, int theY) { return false; }      // +0x28
	// 0x4204a0 (mahjongjapan) inlined here: the default that does nothing, shared with OnDragEnd, OnDrag and OnChar
	virtual void OnMouseMove(int theFlags, int theX, int theY) {}                    // +0x2c
	virtual void OnDragEnd(int theFlags, int theX, int theY) {}                      // +0x30
	virtual void OnDrag(int theFlags, int theX, int theY) {}                         // +0x34
	virtual void OnHover(bool theIn, int theFlags) {}                                // +0x38 (0x4204b0 (mahjongjapan))
	virtual bool OnKeyDown(int theKey, int theRepeat, int theScan) { return false; }  // +0x3c WM_KEYDOWN
	virtual void OnChar(int theChar, int theRepeat, int theScan) {}                   // +0x40 WM_CHAR
	// +0x44 (0x4204c0 (mahjongjapan)) true: redraw
	virtual bool Update() { return false; }
	// +0x48 (0x4204d0 (mahjongjapan)) false: stop it
	virtual bool OnTimer(int theId, int theIndex) { return false; }
	virtual void OnVisibleChanged() {}                                    // +0x4c
	virtual void OnEnabledChanged() {}                                    // +0x50
	virtual void OnRelease() {}                                           // +0x54 the capture ended

	// 0x41e830 (mahjongchina), 0x41ed40 (mahjongjapan): Clear, then every child's tree (deleted), the attachments, the
	// registrations and the timers
	void DestroyTree();
	// 0x41e990 (mahjongchina), 0x41eea0 (mahjongjapan)
	void FreeClipOwn();
	// 0x41e9c0 (mahjongchina), 0x41eed0 (mahjongjapan)
	void SetVisible(bool theVisible);
	// 0x41ea20 (mahjongchina), 0x41ef30 (mahjongjapan)
	void SetVisibleByParent(bool theVisible);
	// 0x41ea70 (mahjongchina), 0x41ef80 (mahjongjapan)
	void SetEnabled(bool theEnabled);
	// 0x41ead0 (mahjongchina), 0x41efe0 (mahjongjapan)
	void SetEnabledByParent(bool theEnabled);
	// 0x41eb10 (mahjongchina), 0x41f020 (mahjongjapan): the attachments and the object's clock pause while it is
	// disabled
	void PauseClock(bool thePaused);
	// 0x41eb80 (mahjongchina), 0x41f090 (mahjongjapan)
	void SetCursorKind(int theCursor, bool theForce);
	// 0x41eba0 (mahjongchina), 0x41f0b0 (mahjongjapan)
	void SetClip(RegionRect* theRegion);

	// 0x41ec70 (mahjongchina), 0x41f190 (mahjongjapan): an image of the manager's table (Resource.dll's), drawn
	// relative to the object when theRelative
	void DrawImage(int theX, int theY, int theImageId, const Rect* theSrc, bool theRelative, int theScaleX,
		int theScaleY, int theAlpha);
	// 0x41ecc0 (mahjongchina), 0x41f1e0 (mahjongjapan)
	void DrawImagePtr(int theX, int theY, Image* theImage, const Rect* theSrc, bool theRelative, int theScaleX,
		int theScaleY, int theAlpha);
	// 0x41ed00 (mahjongchina), 0x41f220 (mahjongjapan): with theKey as the colour key
	void DrawImageKeyed(int theX, int theY, int theImageId, uint32_t theKey, const Rect* theSrc, bool theRelative,
		int theScaleX, int theScaleY, int theAlpha);
	// 0x41ed50 (mahjongchina), 0x41f270 (mahjongjapan)
	void DrawWorker(int theMode, int theX, int theY, Image* theImage, bool theRelative, uint32_t theKey,
		const Rect* theSrc, int theScaleX, int theScaleY, int theAlpha);
	// 0x41f150 (mahjongchina), 0x41f6a0 (mahjongjapan)
	void FillRect(Rect theRect, uint32_t theColour, bool theRelative);
	// 0x41f220 (mahjongchina), 0x41f770 (mahjongjapan)
	void FillRectAlpha(Rect theRect, int theAlpha, uint32_t theColour, bool theRelative);
	// 0x41f2c0 (mahjongchina), 0x41f810 (mahjongjapan)
	void DrawLine(int theX1, int theY1, int theX2, int theY2, uint32_t theColour, bool theRelative);
	// 0x41f350 (mahjongchina), 0x41f8a0 (mahjongjapan)
	void DrawFrame(const Rect& theRect, uint32_t theColour, bool theRelative);
	// 0x41f3c0 (mahjongchina), 0x41f910 (mahjongjapan)
	void AddDirty(const Rect& theRect);
	// 0x41f3e0 (mahjongchina), 0x41f930 (mahjongjapan)
	void Create(Manager* theManager, Layer* theLayer, int theX, int theY, Object* theParent);
	// 0x41f420 (mahjongchina), 0x41f970 (mahjongjapan)
	void AddChild(Object* theChild, int theX, int theY);
	// 0x41f5e0 (mahjongchina), 0x41fb30 (mahjongjapan)
	void Attach(Attachment* theAttachment);
	// 0x41f7a0 (mahjongchina), 0x41fcf0 (mahjongjapan)
	void DrawTree();
	// 0x41f820..0x41fae0 (mahjongchina), 0x41fd70..0x41fe90 (mahjongjapan)
	void RegHover(bool theOn);
	void RegLayerCmd(bool theOn);
	void RegClick(bool theOn);
	void RegMove(bool theOn);
	void RegKey(bool theOn);
	void RegChar(bool theOn);
	void RegUpdate(bool theOn);
	// 0x41fba0 (mahjongchina), 0x41fee0 (mahjongjapan): the position on the screen
	void GetScreenPos(int* thePos);
	// 0x41fc50 (mahjongchina), 0x41ff90 (mahjongjapan): the dirty rects of the tree
	void GetTreeDirty(Rect& theRect);
	// 0x41fcc0 (mahjongchina), 0x420000 (mahjongjapan): a mouse message in the object's coordinates
	bool DispatchMouse(int theMsg, int theFlags, int theX, int theY);
	// 0x41fdd0 (mahjongchina), 0x420100 (mahjongjapan), 0x41fe40 (mahjongchina), 0x420150 (mahjongjapan),
	// 0x41fe80 (mahjongchina), 0x420190 (mahjongjapan)
	void DispatchMouseMove(int theFlags, int theX, int theY);
	void DispatchDragEnd(int theFlags, int theX, int theY);
	void DispatchDrag(int theFlags, int theX, int theY);
	// 0x41fec0 (mahjongchina), 0x4201d0 (mahjongjapan): the manager's clock
	int ManagerTime();
	// 0x41fed0 (mahjongchina), 0x4201e0 (mahjongjapan): the object's own clock
	int GetTime();
	// 0x41ff00 (mahjongchina), 0x420210 (mahjongjapan)
	int GetDelta() const { return mDelta; }
	// 0x41ff10 (mahjongchina), 0x420220 (mahjongjapan): capture the input (the manager's +0x30)
	void Capture(bool theOn);
	// 0x41ff30 (mahjongchina), 0x420240 (mahjongjapan): the timers, the attachments and Update
	bool DoUpdate(int theClock);
	// 0x420070 (mahjongchina), 0x420360 (mahjongjapan)
	void AddTimer(int theId, int thePeriod, int theCount, int thePerTick);
	// 0x4200c0 (mahjongchina), 0x4203b0 (mahjongjapan)
	bool StepTimer(ObjTimer* theTimer, int theTime);
	// 0x420110 (mahjongchina), 0x420400 (mahjongjapan)
	bool FireTimer(ObjTimer* theTimer);
	// 0x4204e0 (mahjongjapan), 0x420500 (mahjongjapan), 0x420520 (mahjongjapan)
	bool IsEnabled() const { return mEnabled && mEnabledByParent; }
	bool IsVisible() const { return mVisible && mVisibleByParent; }
	bool IsActive() const { return IsVisible() && IsEnabled(); }
	// 0x420550 (mahjongjapan), 0x420560 (mahjongjapan)
	void SetUserFlag(bool theFlag) { mUserFlag = theFlag; }
	bool GetUserFlag() const { return mUserFlag; }
	// 0x420570 (mahjongjapan), 0x4205a0 (mahjongjapan), 0x4205d0 (mahjongjapan), 0x420600 (mahjongjapan): the virtuals,
	// only while the object takes input
	bool DispatchCommand(int theCmd);
	void DispatchHover(bool theIn, int theFlags);
	bool DispatchKey(int theKey, int theRepeat, int theScan);
	void DispatchChar(int theChar, int theRepeat, int theScan);
	// 0x41fb80 (mahjongchina), 0x420630 (mahjongjapan)
	void GetPos(int* thePos) const { thePos[0] = mX; thePos[1] = mY; }
	// 0x4206c0 (mahjongjapan)
	bool TakesInput() const;
	// 0x420690 (mahjongjapan), 0x4206f0 (mahjongjapan)
	void SetTrackDirty(bool theOn) { mTrackDirty = theOn; }
	void SetUseClip(bool theOn) { mUseClip = theOn; }
	// 0x40a360 (mahjongchina), 0x40a4e0 (mahjongjapan) (in the game's code): moves the object
	void SetPos(int theX, int theY);
};

// 0x420650 (mahjongjapan), 0x420670 (mahjongjapan), 0x420680 (mahjongjapan): the sound object's load, play and stop
void LoadSound(int theId, bool theLoop);
void PlaySound(int theId);
void StopSound(int theId);
// 0x4206a0 (mahjongjapan): rand() % theRange, 0 for 0
int Random(int theRange);
// 0x41f130 (mahjongchina), 0x41f680 (mahjongjapan): the size of an image of the table (ImageInfo in Image.h)
void GetImageInfo(int theImageId, ImageInfo& theInfo);
// 0x41ec10 (mahjongchina), 0x41f120 (mahjongjapan), 0x41ec40 (mahjongchina), 0x41f150 (mahjongjapan): a font of the exe
// or of Resource.dll
Font* GetExeFont(int theImageId, int theTextId);
Font* GetDllFont(int theImageId, int theTextId);
// 0x41f180 (mahjongjapan), 0x420440 (mahjongjapan), 0x420460 (mahjongjapan), 0x420470 (mahjongjapan)
void FreeImage(int theImageId);
Image* CreateImage(int theWidth, int theHeight);
Image* LoadDllImage(int theImageId);
Image* LoadDllImageKeyed(int theImageId, uint32_t theKey);

class Button : public Object
{
public:
	int mImage;              // +0x90
	int mCmd;                // +0x94
	int mFrames;             // +0x98
	bool mHover = false;     // +0x9c
	bool mPressed = false;   // +0x9d
	int mFrameWidth = 0;     // +0xa0
	int mHeight = 0;         // +0xa4
	std::string mLabel;      // +0xa8
	Font* mFont = nullptr;   // +0xb0
	int mLabelX = 0;         // +0xb4
	int mLabelRight = 0;     // +0xb8
	int mLabelY = 0;         // +0xbc
	int mLabelLines = 0;     // +0xc0 0: one line
	int mPressDX = 0;        // +0xc4
	int mPressDY = 0;        // +0xc8
	int mDisabledAlpha = 0xff;  // +0xcc
	bool mFixedSize;         // +0xd0
	bool mShowDisabled = true;  // +0xd1
	int mHoverSound = 0;     // +0xd4

	// 0x41dea0 (mahjongchina), 0x41e240 (mahjongjapan)
	Button(int theImage, int theFrames, bool theFixedSize);
	// 0x41df70 (mahjongchina), 0x41e310 (mahjongjapan)
	~Button() override;

	void Draw() override;                                            // 0x41e0b0 (mahjongchina), 0x41e4b0 (mahjongjapan)
	void Init() override;                                            // 0x41dfd0 (mahjongchina), 0x41e3c0 (mahjongjapan)
	bool OnLButtonDown(int theFlags, int theX, int theY) override;   // 0x41e410 (mahjongchina), 0x41e930 (mahjongjapan)
	bool OnLButtonUp(int theFlags, int theX, int theY) override;     // 0x41e440 (mahjongchina), 0x41e960 (mahjongjapan)
	void OnHover(bool theIn, int theFlags) override;                 // 0x41e470 (mahjongchina), 0x41e990 (mahjongjapan)
	void OnVisibleChanged() override;                                // 0x41e4d0 (mahjongchina), 0x41e9f0 (mahjongjapan)
	void OnEnabledChanged() override;                                // 0x41e4b0 (mahjongchina), 0x41e9d0 (mahjongjapan)
	void OnRelease() override;                                       // 0x41e4f0 (mahjongchina), 0x41ea10 (mahjongjapan)
	// +0x58 0x41e510 (mahjongchina), 0x41ea30 (mahjongjapan): the layer gets the command
	virtual void OnClick();
	virtual void DrawNormal();        // +0x5c 0x41e170 (mahjongchina), 0x41e560 (mahjongjapan)
	virtual void DrawHover();         // +0x60 0x41e210 (mahjongchina), 0x41e660 (mahjongjapan)
	virtual void DrawPressed();       // +0x64 0x41e280 (mahjongchina), 0x41e6d0 (mahjongjapan)
	virtual void DrawDisabled();      // +0x68 0x41e350 (mahjongchina), 0x41e810 (mahjongjapan)
	virtual void OnInit() {}          // +0x6c (0x4221c0 (mahjongchina), 0x429700 (mahjongjapan))

	// 0x41e370 (mahjongjapan), 0x41e380 (mahjongjapan), 0x41e390 (mahjongjapan), 0x41e3a0 (mahjongjapan),
	// 0x41e3b0 (mahjongjapan)
	int GetFrameWidth() const { return mFrameWidth; }
	bool GetShowDisabled() const { return mShowDisabled; }
	int GetImage() const { return mImage; }
	bool IsHover() const { return mHover; }
	void SetShowDisabled(bool theOn) { mShowDisabled = theOn; }
	// 0x41e040 (mahjongchina), 0x41e430 (mahjongjapan): the label, a string of Resource.dll in a font of Resource.dll
	void SetLabel(int theStringId, int theFontImage, int theFontText, int theX, int theRight, int theY, int thePressDX,
		int thePressDY, int theDisabledAlpha);
	// 0x41e1d0 (mahjongchina), 0x41e5c0 (mahjongjapan), 0x41e300 (mahjongchina), 0x41e750 (mahjongjapan),
	// 0x41e3d0 (mahjongchina), 0x41e890 (mahjongjapan)
	void DrawLabel();
	void DrawLabelPressed();
	void DrawLabelDisabled();
	// 0x41e500 (mahjongchina), 0x41ea20 (mahjongjapan)
	void SetCmd(int theCmd) { mCmd = theCmd; }
};

class Toggle : public Button
{
public:
	bool mChecked = false;        // +0xe4
	RadioGroup* mGroup;           // +0xe8
	int mId;                      // +0xec

	// 0x41e520 (mahjongchina), 0x41ea40 (mahjongjapan)
	Toggle(int theImage, RadioGroup* theGroup, int theId);
	// 0x41e5b0 (mahjongchina), 0x41ead0 (mahjongjapan)
	~Toggle() override {}

	void OnClick() override;      // 0x41e6e0 (mahjongchina), 0x41ebd0 (mahjongjapan)
	void DrawNormal() override;   // 0x41e610 (mahjongchina), 0x41eaf0 (mahjongjapan)
	void DrawHover() override;    // 0x41e6d0 (mahjongchina), 0x41ebc0 (mahjongjapan)

	// 0x41eae0 (mahjongjapan), 0x41e700 (mahjongchina), 0x41ebf0 (mahjongjapan), 0x41ec00 (mahjongjapan)
	bool IsChecked() const { return mChecked; }
	void SetChecked(bool theOn) { mChecked = theOn; }
	int GetId() const { return mId; }
};

// A check box (vtable 0x51dfbc (mahjongjapan)): a button whose frames are unchecked, then checked, with its label at
// the right 0x41e5c0 (mahjongchina), 0x424b40 (mahjongjapan): CheckBox's label, set only through Button's fields; the
// game sets the radio toggles' labels with it too
void SetCheckLabel(Button* theButton, int theStringId, int theFontImage, int theFontText, int theX, int theY,
	int theDisabledAlpha);

class CheckBox : public Button
{
public:
	bool mChecked = false;  // +0xe4

	// 0x42a310 (mahjongchina), 0x424ae0 (mahjongjapan)
	CheckBox(int theImage, int theFrames);
	// 0x424b30 (mahjongjapan)
	~CheckBox() override {}

	void OnClick() override;       // 0x42a450 (mahjongchina), 0x424c70 (mahjongjapan)
	void DrawNormal() override;    // 0x42a370 (mahjongchina), 0x424b90 (mahjongjapan)
	void DrawHover() override;     // 0x42a410 (mahjongchina), 0x424be0 (mahjongjapan)
	void DrawPressed() override;   // 0x424c20 (mahjongjapan)
	void DrawDisabled() override;  // 0x42a3c0 (mahjongchina), 0x424c30 (mahjongjapan)

	// 0x41eae0 (mahjongjapan), 0x41e700 (mahjongchina), 0x41ebf0 (mahjongjapan) (Toggle's, the same field)
	bool IsChecked() const { return mChecked; }
	void SetChecked(bool theOn) { mChecked = theOn; }
	// 0x41e5c0 (mahjongchina), 0x424b40 (mahjongjapan): a string of Resource.dll, in a font of Resource.dll
	void SetLabel(int theStringId, int theFontImage, int theFontText, int theX, int theY, int theDisabledAlpha)
	{
		SetCheckLabel(this, theStringId, theFontImage, theFontText, theX, theY, theDisabledAlpha);
	}
	// 0x42a470 (mahjongchina), 0x424c90 (mahjongjapan): frame theFrame of the image, with the label
	void DrawFrame(int theFrame);
};

class RadioGroup
{
public:
	std::vector<Toggle*> mToggles;   // +0x08
	int mSelected = -1;              // +0x14

	// 0x422d60 (mahjongchina), 0x422020 (mahjongjapan), 0x422db0 (mahjongchina), 0x422070 (mahjongjapan)
	RadioGroup() {}
	virtual ~RadioGroup() {}

	// 0x4220a0 (mahjongjapan)
	int GetSelected() const { return mSelected; }
	// 0x422de0 (mahjongchina), 0x4220b0 (mahjongjapan)
	void Add(Toggle* theToggle) { mToggles.push_back(theToggle); }
	// 0x422f90 (mahjongchina), 0x422260 (mahjongjapan)
	void Select(int theId);
};

} // namespace jb

#endif
