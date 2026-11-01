#ifndef __MAHJONGCHINA_LAYOUTSELECT_H__
#define __MAHJONGCHINA_LAYOUTSELECT_H__

// The list of layouts (0x4026c0, 0x401a00..0x403420) and the choice of the list's background (0x401000..0x4019f0).
// The list shows nine layouts a page; each slot draws its layout's thumbnail into one 0x243x0x1b9 sheet, a slot a
// frame, and the sheet is kept as <common folder><page>.bmp with the slots' texts in <page>.txt, so the next visit
// to the page loads it instead.

#include "Dialog.h"
#include "Layer.h"
#include "Object.h"

#include <string>

namespace jb
{
class Font;
class Image;
}

namespace mahjongchina
{

class Layout;

// 0x402f80: the id of the layout at a place of the list (table 0x51c4c4), 0 outside the list
int GetLayoutId(int theIndex);
// 0x4019b0: the backgrounds open: the escape's stages reached, plus 9
int CountBackgrounds();

// 0x401000 (vtable 0x51c320, 0xf4 bytes): a background's picture, a toggle of the background box
class BackgroundPicture : public jb::Toggle
{
public:
	int mBackground = -1;  // +0xf0 the background shown, -1 none

	// 0x401000
	BackgroundPicture(jb::RadioGroup* theGroup, int theId) : Toggle(0x142, theGroup, theId) {}
	// 0x401030, 0x401050
	~BackgroundPicture() override {}

	bool OnLButtonDblClk(int theFlags, int theX, int theY) override;  // 0x401250
	void DrawNormal() override;                                       // 0x401070
	void OnInit() override;                                           // 0x401060
};

// 0x401270 (vtable 0x51c390, 0x94 bytes): the list's background, one of six shown at a time, or each in turn
class BackgroundBox : public Dialog
{
public:
	jb::RadioGroup mGroup;                     // +0x54
	BackgroundPicture* mPictures[6] = {};      // +0x6c
	jb::Button* mUp = nullptr;                 // +0x84
	jb::Button* mDown = nullptr;               // +0x88
	bool mCycle = false;                       // +0x8c each in turn
	int mFirst = 0;                            // +0x90 the first background shown

	// 0x401270
	BackgroundBox() : Dialog(0x28, 0x50) {}
	// 0x4012c0, 0x4012e0
	~BackgroundBox() override {}

	void Create() override;               // 0x401340
	bool OnCommand(int theCmd) override;  // 0x401860

	// 0x401810
	void ShowPictures();
	// 0x401920
	void Refresh();
	// 0x4019c0: the choice made
	void Apply();
};

// 0x401a00 (vtable 0x51c3bc, 0x90 bytes)
class LayoutBackdrop : public jb::Object
{
public:
	// 0x401a00
	LayoutBackdrop() {}
	// 0x401a20, 0x401a40
	~LayoutBackdrop() override {}

	void Draw() override;  // 0x401a50
	void Init() override {}  // 0x42cc80
};

// 0x401a70 (vtable 0x51c414, 0xc0 bytes): the title, the pages before and after, and the layouts played; while it
// updates, the thumbnails are drawn a layout a frame
class LayoutHeader : public jb::Object
{
public:
	jb::Font* mTitleFont = nullptr;  // +0x90
	jb::Font* mPageFont = nullptr;   // +0x94
	std::string mTitle;              // +0x98 (0x10)
	jb::Font* mPlayedFont = nullptr; // +0xa0
	std::string mPrevPage;           // +0xa4
	std::string mNextPage;           // +0xac
	std::string mPlayed;             // +0xb4
	int mCount;                      // +0xbc the layouts

	// 0x401a70
	explicit LayoutHeader(int theCount) : mCount(theCount) {}
	// 0x401b00, 0x401b20
	~LayoutHeader() override {}

	void Draw() override;    // 0x401bb0
	void Init() override;    // 0x401c60
	bool Update() override;  // 0x401d10

	// 0x401d30
	void StartBuilding() { RegUpdate(true); }
	// 0x401d40: 0-based, shown from 1
	void SetPages(int thePrevFirst, int thePrevLast, int theNextFirst, int theNextLast);
};

// 0x401d90 (vtable 0x51c46c, 0xd8 bytes): a layout of the page
class LayoutSlot : public jb::Object
{
public:
	int mTiles = 0;                // +0x90
	int mFree = 0;                 // +0x94 the tiles free at the start
	std::string mName;             // +0x98
	std::string mInfo;             // +0xa0
	int mIndex;                    // +0xa8 on the page, 0-8
	bool mSelected = false;        // +0xac
	int mState = 0;                // +0xb0 0 empty, 1 drawn from the saved sheet, 2 drawn into mSheet
	jb::Image* mSheet = nullptr;   // +0xb4
	jb::Rect mCell;                // +0xb8 its frame of the sheet
	int mOffX = 0;                 // +0xc8
	int mOffY = 0;                 // +0xcc
	bool mSaved = false;           // +0xd0 a saved game of it exists
	bool mHasBest = false;         // +0xd1
	jb::Font* mFont = nullptr;     // +0xd0 the name's
	jb::Font* mInfoFont = nullptr; // +0xd4 the info line's

	// 0x401d90
	explicit LayoutSlot(int theIndex) : mIndex(theIndex) {}
	// 0x401e20, 0x401e40
	~LayoutSlot() override {}

	void Draw() override;                                             // 0x401eb0
	void Init() override;                                             // 0x401fc0
	bool OnLButtonDown(int theFlags, int theX, int theY) override;    // 0x402060
	bool OnLButtonDblClk(int theFlags, int theX, int theY) override;  // 0x4020a0

	// 0x4020d0: from the page's saved texts
	void SetSaved(const char* theName, int theTiles, int theFree, int theLayout);
	// 0x402120
	void Empty();
	// 0x402150: the best time, the counts or the saved game
	void SetInfo(int theLayout);
	// 0x402270
	void GetSaved(std::string& theName, int* theTiles, int* theFree);
	// 0x4022a0: the thumbnail drawn into theSheet
	void SetLayout(jb::Image* theSheet, int theLayoutId, int theLayout);
	// 0x402540: the tiles' places in the cell and the counts
	void Measure(Layout& theLayout);
};

// 0x4026c0 (vtable 0x51c754, 0x8c bytes)
class LayoutSelect : public jb::Layer, public DialogListener
{
public:
	LayoutSlot* mSlots[9] = {};       // +0x54
	LayoutHeader* mHeader = nullptr;  // +0x78
	jb::Image* mSheet = nullptr;      // +0x7c
	int mPage = 0;                    // +0x80 the first layout of the page
	int mSelected = 0;                // +0x84 on the page
	int mBuilt = 0;                   // +0x88 the thumbnails drawn

	// 0x4026c0
	LayoutSelect() : Layer(0, 0) {}
	// 0x402700, 0x402720
	~LayoutSelect() override;

	void Load() override;                 // 0x402780
	bool OnCommand(int theCmd) override;  // 0x402b40
	bool OnYes(int theId) override;       // 0x402e80
	bool OnNo(int theId) override;        // 0x402e90

	// 0x402c90
	void ShowPage();
	// 0x402d50
	void Select(int theSlot);
	// 0x402d90: the question whether to resume its saved game, or the game
	void Choose(int theSlot);
	// 0x402ea0
	void Start(bool theResume);
	// 0x402fa0: the page's saved sheet, or a new one to draw
	void LoadSheet();
	// 0x403270: one more thumbnail; false once all are drawn (and the sheet saved)
	bool BuildNext();
};

} // namespace mahjongchina

#endif
