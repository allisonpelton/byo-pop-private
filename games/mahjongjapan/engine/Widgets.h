#ifndef __MAHJONGJAPAN_WIDGETS_H__
#define __MAHJONGJAPAN_WIDGETS_H__

// The engine's widgets (0x426910..0x428dc0, and the slider 0x4309f0..0x430b50): the slider bar (0x426910, vtable
// 0x51e298) with its thumb (0x4309f0, vtable 0x51e5dc), the list box (0x426ac0, vtable 0x51e2f4) and the text edit
// (0x427920, vtable 0x51e350).
//
// The text edit is not translated yet: only the layers' focus call is declared for it (Widgets.cpp).

#include "Object.h"

#include <string>
#include <vector>

namespace jb
{

class Font;
class SliderBar;

// 0x4309f0: a button of two frames dragged between mMinX and mMaxX (its parent is a SliderBar)
class Slider : public Button
{
public:
	int mMinX;  // +0xe4
	int mMaxX;  // +0xe8

	// 0x4309f0
	Slider(int theImage, int theMinX, int theMaxX);
	// 0x430a60
	~Slider() override {}

	bool OnLButtonDown(int theFlags, int theX, int theY) override;   // 0x430a70
	bool OnLButtonUp(int theFlags, int theX, int theY) override { return true; }  // 0x430a30
	void OnDragEnd(int theFlags, int theX, int theY) override;       // 0x430aa0
	void OnDrag(int theFlags, int theX, int theY) override;          // 0x430aa0

	// 0x430ab0: the position as 0-100, to the bar
	void ReportValue();
	// 0x430b00
	void SetValue(int theValue);
};

// 0x426910: a track image with a slider on it; OnValue gets 0-100
class SliderBar : public Object
{
public:
	int mTrack;               // +0x90 the track's image
	int mThumb;               // +0x94 the slider's image
	int mSliderX;             // +0x98 the slider's left end
	int mSliderEnd;           // +0x9c the slider's right end, less the track's width
	int mValue = 0;           // +0xa0
	Slider* mSlider = nullptr;  // +0xa4

	// 0x426910
	SliderBar(int theTrack, int theThumb, int theSliderX, int theSliderEnd);
	// 0x426970
	~SliderBar() override {}

	void Draw() override;     // 0x426980
	void Init() override;     // 0x4269a0
	virtual void OnValue(int theValue) {}  // +0x58 (0x428dc0)

	// 0x426a80
	void SetValue(int theValue);
	// 0x426aa0: from the slider
	void ValueChanged(int theValue);
};

// 0x426ac0: a list of strings in rows; the first mFixed rows stay, the rest scroll by the up and down buttons. A click
// selects a row and sends the layer the command mCmd.
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

	// 0x426ac0
	ListBox(int theCmd, int theWidth, int theRowHeight, int theRows, int theFixed);
	// 0x426b90
	~ListBox() override;

	void Draw() override;                                              // 0x426eb0
	void Init() override;                                              // 0x426cb0
	bool OnCommand(int theCmd) override;                               // 0x4271f0
	bool OnLButtonDown(int theFlags, int theX, int theY) override;     // 0x427150
	void OnMouseMove(int theFlags, int theX, int theY) override;       // 0x4270e0
	virtual void OnInit() {}                                           // +0x58 (0x429700)

	// 0x426c80, 0x426ca0
	int GetCount() const { return (int)mItems.size(); }
	int GetSelected() const { return mSelected; }
	// 0x426cf0
	void SetSelColour(uint32_t theColour) { mSelColour = theColour; }
	// 0x426d00
	void SetFrame(uint32_t theColour1, uint32_t theColour2, uint32_t theColour3);
	// 0x426d30
	void SetFont(int theFontImage, int theFontText, int theX, int theY, int theDisabledAlpha);
	// 0x426d70: the scroll buttons (images of four frames) on the right
	void SetScroll(int theUpImage, int theDownImage);
	// 0x4272a0
	void SetSelected(int theIndex);
	// 0x427340, 0x4273c0: the new item's index
	int Add(int theStringId, int theData);
	int Add(const char* theText, int theData);
	// 0x427440
	int AddItem(Item* theItem);
	// 0x4274a0
	void UpdateButtons();
	// 0x427510
	void Remove(int theIndex);
	// 0x427630
	void ClearItems();  // not the virtual Clear (+0x0c), which stays empty
	// 0x4276d0, 0x4276f0
	const std::string& GetText(int theIndex) const { return mItems[theIndex]->mText; }
	int GetData(int theIndex) const { return mItems[theIndex]->mData; }
};

// 0x4287a0: the text edit's focus on or off (the layers' focus lists hold text edits only)
void SetObjectFocus(Object* theObject, bool theFocus);

} // namespace jb

#endif
