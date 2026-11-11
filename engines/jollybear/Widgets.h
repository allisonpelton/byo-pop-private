#ifndef __JB_WIDGETS_H__
#define __JB_WIDGETS_H__

// The engine's widgets (0x430640..0x432860 (mahjongchina), 0x426910..0x428dc0 (mahjongjapan), and the slider
// 0x4309f0..0x430b50 (mahjongjapan)): the slider bar (0x430640 (mahjongchina), 0x426910 (mahjongjapan), vtable
// 0x51e298 (mahjongjapan)) with its thumb (0x4309f0 (mahjongjapan), vtable 0x51e5dc (mahjongjapan)), the list box
// (0x4307f0 (mahjongchina), 0x426ac0 (mahjongjapan), vtable 0x51e2f4 (mahjongjapan)) and the text edit
// (0x431430 (mahjongchina), 0x427920 (mahjongjapan), vtable 0x51e350 (mahjongjapan)).

#include "Object.h"

#include <string>
#include <vector>

namespace jb
{

class Font;
class SliderBar;

// 0x4309f0 (mahjongjapan): a button of two frames dragged between mMinX and mMaxX (its parent is a SliderBar)
class Slider : public Button
{
public:
	int mMinX;  // +0xe4
	int mMaxX;  // +0xe8

	// 0x4309f0 (mahjongjapan)
	Slider(int theImage, int theMinX, int theMaxX);
	// 0x42ac20 (mahjongchina), 0x430a60 (mahjongjapan)
	~Slider() override {}

	bool OnLButtonDown(int theFlags, int theX, int theY) override;   // 0x430a70 (mahjongjapan)
	bool OnLButtonUp(int theFlags, int theX, int theY) override { return true; }  // 0x430a30 (mahjongjapan)
	void OnDragEnd(int theFlags, int theX, int theY) override;       // 0x437ea0 (mahjongchina), 0x430aa0 (mahjongjapan)
	void OnDrag(int theFlags, int theX, int theY) override;          // 0x437ea0 (mahjongchina), 0x430aa0 (mahjongjapan)

	// 0x437eb0 (mahjongchina), 0x430ab0 (mahjongjapan): the position as 0-100, to the bar
	void ReportValue();
	// 0x430b00 (mahjongjapan)
	void SetValue(int theValue);
};

// 0x430640 (mahjongchina), 0x426910 (mahjongjapan): a track image with a slider on it; OnValue gets 0-100
class SliderBar : public Object
{
public:
	int mTrack;               // +0x90 the track's image
	int mThumb;               // +0x94 the slider's image
	int mSliderX;             // +0x98 the slider's left end
	int mSliderEnd;           // +0x9c the slider's right end, less the track's width
	Slider* mSlider = nullptr;  // +0xa4

	// 0x430640 (mahjongchina), 0x426910 (mahjongjapan)
	SliderBar(int theTrack, int theThumb, int theSliderX, int theSliderEnd);
	// 0x4306a0 (mahjongchina), 0x426970 (mahjongjapan)
	~SliderBar() override {}

	void Draw() override;     // 0x4306b0 (mahjongchina), 0x426980 (mahjongjapan)
	void Init() override;     // 0x4306d0 (mahjongchina), 0x4269a0 (mahjongjapan)
	virtual void OnValue(int theValue) {}  // +0x58 (0x428dc0 (mahjongjapan))

	// 0x4307b0 (mahjongchina), 0x426a80 (mahjongjapan)
	void SetValue(int theValue);
	// 0x4307d0 (mahjongchina), 0x426aa0 (mahjongjapan): from the slider
	void ValueChanged(int theValue);
};

// 0x4307f0 (mahjongchina), 0x426ac0 (mahjongjapan): a list of strings in rows; the first mFixed rows stay, the rest
// scroll by the up and down buttons. A click selects a row and sends the layer the command mCmd.
class ListBox : public Object
{
public:
	struct Item
	{
		std::string mText;
		int mData = 0;
	};

	int mCmd;                       // +0x90
	std::vector<Item*> mItems;      // +0x98
	int mTop;                       // +0xa4 the item in the first scrolling row
	int mSelected = -1;             // +0xa8
	bool mFramed = false;           // +0xac
	bool mScrolls = false;          // +0xad
	int mBarX = 0;                  // +0xb0 the scroll bar's line
	int mUpImage = 0;               // +0xb4
	int mDownImage = 0;             // +0xb8
	Button* mUp = nullptr;          // +0xbc
	Button* mDown = nullptr;        // +0xc0
	int mWidth;                     // +0xc4
	int mRowHeight;                 // +0xc8
	int mRows;                      // +0xcc
	int mFixed;                     // +0xd0
	Rect mFrame;                    // +0xd4
	uint32_t mSelColour = 0;        // +0xe4
	uint32_t mFrameColour3 = 0;     // +0xe8
	uint32_t mFrameColour1 = 0;     // +0xec
	uint32_t mFrameColour2 = 0;     // +0xf0
	Font* mFont = nullptr;          // +0xf4
	int mTextX = 0;                 // +0xf8
	int mTextY = 0;                 // +0xfc
	int mDisabledAlpha = 0xff;      // +0x100

	// 0x4307f0 (mahjongchina), 0x426ac0 (mahjongjapan)
	ListBox(int theCmd, int theWidth, int theRowHeight, int theRows, int theFixed);
	// 0x4308c0 (mahjongchina), 0x426b90 (mahjongjapan)
	~ListBox() override;

	// 0x430bb0 (mahjongchina), 0x426eb0 (mahjongjapan)
	void Draw() override;
	// 0x4309b0 (mahjongchina), 0x426cb0 (mahjongjapan)
	void Init() override;
	// 0x430ee0 (mahjongchina), 0x4271f0 (mahjongjapan)
	bool OnCommand(int theCmd) override;
	// 0x430e40 (mahjongchina), 0x427150 (mahjongjapan)
	bool OnLButtonDown(int theFlags, int theX, int theY) override;
	// 0x430dd0 (mahjongchina), 0x4270e0 (mahjongjapan)
	void OnMouseMove(int theFlags, int theX, int theY) override;
	// +0x58 (0x4221c0 (mahjongchina), 0x429700 (mahjongjapan))
	virtual void OnInit() {}

	// 0x426c80 (mahjongjapan), 0x426ca0 (mahjongjapan)
	int GetCount() const { return (int)mItems.size(); }
	int GetSelected() const { return mSelected; }
	// 0x4309f0 (mahjongchina), 0x426cf0 (mahjongjapan)
	void SetSelColour(uint32_t theColour) { mSelColour = theColour; }
	// 0x430a00 (mahjongchina), 0x426d00 (mahjongjapan)
	void SetFrame(uint32_t theColour1, uint32_t theColour2, uint32_t theColour3);
	// 0x430a30 (mahjongchina), 0x426d30 (mahjongjapan)
	void SetFont(int theFontImage, int theFontText, int theX, int theY, int theDisabledAlpha);
	// 0x430a70 (mahjongchina), 0x426d70 (mahjongjapan): the scroll buttons (images of four frames) on the right
	void SetScroll(int theUpImage, int theDownImage);
	// 0x430f90 (mahjongchina), 0x4272a0 (mahjongjapan)
	void SetSelected(int theIndex);
	// 0x431030 (mahjongchina), 0x427340 (mahjongjapan), 0x4310b0 (mahjongchina), 0x4273c0 (mahjongjapan): the new
	// item's index
	int Add(int theStringId, int theData);
	int Add(const char* theText, int theData);
	// 0x431130 (mahjongchina), 0x427440 (mahjongjapan)
	int AddItem(Item* theItem);
	// 0x431190 (mahjongchina), 0x4274a0 (mahjongjapan)
	void UpdateButtons();
	// 0x431200 (mahjongchina), 0x427510 (mahjongjapan)
	void Remove(int theIndex);
	// 0x431320 (mahjongchina), 0x427630 (mahjongjapan)
	void ClearItems();  // not the virtual Clear (+0x0c), which stays empty
	// 0x4313c0 (mahjongchina), 0x4276d0 (mahjongjapan), 0x4313e0 (mahjongchina), 0x4276f0 (mahjongjapan)
	const std::string& GetText(int theIndex) const { return mItems[theIndex]->mText; }
	int GetData(int theIndex) const { return mItems[theIndex]->mData; }
};

// 0x431430 (mahjongchina), 0x427920 (mahjongjapan): a line of text typed in, with a caret, a selection (from the anchor
// to the cursor), the clipboard and one level of undo. A layer's focus list holds text edits; Enter goes to the layer
// (+0x24), and every change to OnChanged.
class TextEdit : public Object
{
public:
	int mTextY = 0;                  // +0x90
	int mDisabledAlpha = 0xff;       // +0x94
	bool mFramed = false;            // +0x98
	Rect mFrame;                     // +0x9c
	uint32_t mSelColour = 0xff0033;  // +0xac
	uint32_t mFrameColour3 = 0;      // +0xb0
	uint32_t mFrameColour1 = 0;      // +0xb4
	uint32_t mFrameColour2 = 0;      // +0xb8
	int mWidth;                      // +0xbc
	int mHeight;                     // +0xc0
	int mMaxLength;                  // +0xc4
	Font* mFont = nullptr;           // +0xc8
	bool mUnderline;                 // +0xcc
	int mBlinkPeriod = 0;            // +0xd0 in updates
	int mBlinkCount = 0;             // +0xd4
	bool mCaretOn = false;           // +0xd8
	std::string mText;               // +0xdc
	std::string mUndoText;           // +0xe4
	bool mFocused = false;           // +0xec
	int mCursor = 0;                 // +0xf0
	int mAnchor = -1;                // +0xf4 -1: no selection
	int mScroll = 0;                 // +0xf8 the first character shown
	int mUndoKind = -1;              // +0xfc 0 moved, 1 deleted, 2 typed, 3 pasted (-1 after an undo)
	int mUndoPos = 0;                // +0x100
	int mUndoInserted = 0;           // +0x104
	int mUndoDeleted = 0;            // +0x108

	// 0x431430 (mahjongchina), 0x427920 (mahjongjapan)
	TextEdit(int theWidth, int theHeight, int theMaxLength, bool theUnderline);
	// 0x431530 (mahjongchina), 0x427a20 (mahjongjapan), 0x431550 (mahjongchina), 0x427a40 (mahjongjapan)
	~TextEdit() override {}

	// 0x431640 (mahjongchina), 0x427b60 (mahjongjapan)
	void Draw() override;
	// 0x431a60 (mahjongchina), 0x427fa0 (mahjongjapan)
	void Init() override;
	// 0x431f40 (mahjongchina), 0x428490 (mahjongjapan)
	bool OnLButtonDown(int theFlags, int theX, int theY) override;
	// 0x432000 (mahjongchina), 0x428550 (mahjongjapan)
	bool OnLButtonDblClk(int theFlags, int theX, int theY) override;
	// 0x431fb0 (mahjongchina), 0x428500 (mahjongjapan)
	void OnDragEnd(int theFlags, int theX, int theY) override;
	// 0x431fc0 (mahjongchina), 0x428510 (mahjongjapan)
	void OnDrag(int theFlags, int theX, int theY) override;
	// 0x431cd0 (mahjongchina), 0x428210 (mahjongjapan)
	bool OnKeyDown(int theKey, int theRepeat, int theScan) override;
	// 0x431b20 (mahjongchina), 0x428060 (mahjongjapan)
	void OnChar(int theChar, int theRepeat, int theScan) override;
	// 0x431ac0 (mahjongchina), 0x428010 (mahjongjapan) the caret blinks
	bool Update() override;
	virtual void OnChanged(const char* theText) {}                     // +0x58 (0x428dc0 (mahjongjapan))

	// 0x427ab0 (mahjongjapan)
	std::string GetText() const { return mText; }
	// 0x4315c0 (mahjongchina), 0x427ae0 (mahjongjapan)
	void SetSelColour(uint32_t theColour) { mSelColour = theColour; }
	// 0x4315d0 (mahjongchina), 0x427af0 (mahjongjapan)
	void SetFrame(uint32_t theColour1, uint32_t theColour2, uint32_t theColour3);
	// 0x431600 (mahjongchina), 0x427b20 (mahjongjapan)
	void SetFont(int theFontImage, int theFontText, int theTextY, int theDisabledAlpha);
	// 0x432240 (mahjongchina), 0x4287a0 (mahjongjapan): on, the whole text is selected
	void SetFocus(bool theFocus);

	// 0x432080 (mahjongchina), 0x4285d0 (mahjongjapan), 0x4320f0 (mahjongchina), 0x428640 (mahjongjapan)
	int WordEnd(int thePos);
	int WordStart(int thePos);
	// 0x432160 (mahjongchina), 0x4286b0 (mahjongjapan): the character position at theX
	int HitTest(int theX);
	// 0x432290 (mahjongchina), 0x4287f0 (mahjongjapan)
	bool DeleteSelection();
	// 0x4322e0 (mahjongchina), 0x428840 (mahjongjapan)
	bool Copy();
	// 0x432380 (mahjongchina), 0x4288e0 (mahjongjapan)
	void RecordUndo(int theKind, int theArg);
	// 0x4324e0 (mahjongchina), 0x428a40 (mahjongjapan)
	void Undo();
	// 0x4325a0 (mahjongchina), 0x428b00 (mahjongjapan)
	void Changed();
	// 0x4325c0 (mahjongchina), 0x428b30 (mahjongjapan)
	bool Cut();
	// 0x432600 (mahjongchina), 0x428b70 (mahjongjapan)
	bool Paste();
	// 0x432720 (mahjongchina), 0x428c90 (mahjongjapan): theText may replace the selection (the length and the width)
	bool Fits(const std::string& theText);
};

// 0x432240 (mahjongchina), 0x4287a0 (mahjongjapan) through a layer's focus list (it holds text edits only)
void SetObjectFocus(Object* theObject, bool theFocus);

} // namespace jb

#endif
