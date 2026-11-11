#ifndef __MAHJONG_LAYOUTSELECT_H__
#define __MAHJONG_LAYOUTSELECT_H__

// The list of layouts (0x402720 (mahjongchina), 0x4026c0 (mahjongjapan), 0x401a00..0x4034b0 (mahjongchina),
// 0x401a00..0x403420 (mahjongjapan)) and the choice of the list's background (0x401000..0x4019f0 (mahjongchina),
// 0x401000..0x4019f0 (mahjongjapan)). The list shows nine layouts a page; each slot draws its layout's thumbnail into
// one 0x243x0x1b9 sheet, a slot a frame, and the sheet is kept as <common folder><page>.bmp with the slots' texts in
// <page>.txt, so the next visit to the page loads it instead.

#include "Dialog.h"
#include "Layer.h"
#include "Object.h"
#include "Res.h"

#include <string>

namespace jb
{
class Font;
class Image;
}

namespace mahjong
{

class Layout;

// 0x403010 (mahjongchina), 0x402f80 (mahjongjapan): the id of the layout at a place of the list (table
// 0x51b4ec (mahjongchina), 0x51c4c4 (mahjongjapan)), 0 outside the list
int GetLayoutId(int theIndex);
// 0x4019b0 (mahjongchina), 0x4019b0 (mahjongjapan): the backgrounds open: the escape's stages reached, plus 9
int CountBackgrounds();

// 0x401000 (mahjongchina), 0x401000 (mahjongjapan) (vtable 0x51b320 (mahjongchina), 0x51c320 (mahjongjapan), 0xf4
// bytes): a background's picture, a toggle of the background box
class BackgroundPicture : public jb::Toggle
{
public:
	int mBackground = -1;  // +0xf0 the background shown, -1 none

	// 0x401000 (mahjongchina), 0x401000 (mahjongjapan)
	BackgroundPicture(jb::RadioGroup* theGroup, int theId) : Toggle(IMAGE_BACKGROUND_LOCK, theGroup, theId) {}
	// 0x401050 (mahjongchina), 0x401030 (mahjongjapan), 0x401070 (mahjongchina), 0x401050 (mahjongjapan)
	~BackgroundPicture() override {}

	// 0x401250 (mahjongchina), 0x401250 (mahjongjapan)
	bool OnLButtonDblClk(int theFlags, int theX, int theY) override;
	// 0x401080 (mahjongchina), 0x401070 (mahjongjapan)
	void DrawNormal() override;
	void OnInit() override;                                           // 0x401060 (mahjongjapan)
};

// 0x401270 (mahjongchina), 0x401270 (mahjongjapan) (vtable 0x51b390 (mahjongchina), 0x51c390 (mahjongjapan), 0x94
// bytes): the list's background, one of six shown at a time, or each in turn
class BackgroundBox : public Dialog
{
public:
	jb::RadioGroup mGroup;                     // +0x54
	BackgroundPicture* mPictures[6] = {};      // +0x6c
	jb::Button* mUp = nullptr;                 // +0x84
	jb::Button* mDown = nullptr;               // +0x88
	bool mCycle = false;                       // +0x8c each in turn
	int mFirst = 0;                            // +0x90 the first background shown

	// 0x401270 (mahjongchina), 0x401270 (mahjongjapan)
	BackgroundBox() : Dialog(0x28, 0x50) {}
	// 0x4012c0 (mahjongchina), 0x4012c0 (mahjongjapan), 0x4012e0 (mahjongchina), 0x4012e0 (mahjongjapan)
	~BackgroundBox() override {}

	void Create() override;               // 0x401340 (mahjongchina), 0x401340 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x401860 (mahjongchina), 0x401860 (mahjongjapan)

	// 0x401810 (mahjongchina), 0x401810 (mahjongjapan)
	void ShowPictures();
	// 0x401920 (mahjongchina), 0x401920 (mahjongjapan)
	void Refresh();
	// 0x4019c0 (mahjongchina), 0x4019c0 (mahjongjapan): the choice made
	void Apply();
};

// 0x401a00 (mahjongchina), 0x401a00 (mahjongjapan) (vtable 0x51b3e4 (mahjongchina), 0x51c3bc (mahjongjapan), 0x90
// bytes)
class LayoutBackdrop : public jb::Object
{
public:
	// 0x401a00 (mahjongchina), 0x401a00 (mahjongjapan)
	LayoutBackdrop() {}
	// 0x401a30 (mahjongchina), 0x401a20 (mahjongjapan), 0x401a50 (mahjongchina), 0x401a40 (mahjongjapan)
	~LayoutBackdrop() override {}

	void Draw() override;  // 0x401a60 (mahjongchina), 0x401a50 (mahjongjapan)
	void Init() override {}  // 0x42cc80 (mahjongjapan)
};

// 0x401a80 (mahjongchina), 0x401a70 (mahjongjapan) (vtable 0x51b43c (mahjongchina), 0x51c414 (mahjongjapan), 0xc0
// bytes): the title, the pages before and after, and the layouts played; while it updates, the thumbnails are drawn a
// layout a frame
class LayoutHeader : public jb::Object
{
public:
	jb::Font* mTitleFont = nullptr;  // +0x90
	jb::Font* mPageFont = nullptr;   // +0x94
	std::string mTitle;              // +0x98 (STRING_LAYOUTS_TITLE)
	jb::Font* mPlayedFont = nullptr; // +0xa0
	std::string mPrevPage;           // +0xa4
	std::string mNextPage;           // +0xac
	std::string mPlayed;             // +0xb4
	int mCount;                      // +0xbc the layouts

	// 0x401a80 (mahjongchina), 0x401a70 (mahjongjapan)
	explicit LayoutHeader(int theCount) : mCount(theCount) {}
	// 0x401b10 (mahjongchina), 0x401b00 (mahjongjapan), 0x401b30 (mahjongchina), 0x401b20 (mahjongjapan)
	~LayoutHeader() override {}

	void Draw() override;    // 0x401bc0 (mahjongchina), 0x401bb0 (mahjongjapan)
	void Init() override;    // 0x401c60 (mahjongchina), 0x401c60 (mahjongjapan)
	bool Update() override;  // 0x401d10 (mahjongchina), 0x401d10 (mahjongjapan)

	// 0x401d30 (mahjongchina), 0x401d30 (mahjongjapan)
	void StartBuilding() { RegUpdate(true); }
	// 0x401d40 (mahjongchina), 0x401d40 (mahjongjapan): 0-based, shown from 1
	void SetPages(int thePrevFirst, int thePrevLast, int theNextFirst, int theNextLast);
};

// 0x401d90 (mahjongchina), 0x401d90 (mahjongjapan) (vtable 0x51b494 (mahjongchina), 0x51c46c (mahjongjapan), 0xd8
// bytes): a layout of the page
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
	jb::Font* mFont = nullptr;     // +0xd4 (China +0xd0) the name's
	jb::Font* mInfoFont = nullptr; // China +0xd4: the info line's (Japan's is the name's font)

	// 0x401d90 (mahjongchina), 0x401d90 (mahjongjapan)
	explicit LayoutSlot(int theIndex) : mIndex(theIndex) {}
	// 0x401e20 (mahjongchina), 0x401e20 (mahjongjapan), 0x401e40 (mahjongchina), 0x401e40 (mahjongjapan)
	~LayoutSlot() override {}

	// 0x401eb0 (mahjongchina), 0x401eb0 (mahjongjapan)
	void Draw() override;
	// 0x401fc0 (mahjongchina), 0x401fc0 (mahjongjapan)
	void Init() override;
	// 0x402080 (mahjongchina), 0x402060 (mahjongjapan)
	bool OnLButtonDown(int theFlags, int theX, int theY) override;
	// 0x4020c0 (mahjongchina), 0x4020a0 (mahjongjapan)
	bool OnLButtonDblClk(int theFlags, int theX, int theY) override;

	// 0x4020f0 (mahjongchina), 0x4020d0 (mahjongjapan): from the page's saved texts
	void SetSaved(const char* theName, int theTiles, int theFree, int theLayout);
	// 0x402140 (mahjongchina), 0x402120 (mahjongjapan)
	void Empty();
	// 0x402170 (mahjongchina), 0x402150 (mahjongjapan): the best time, the counts or the saved game
	void SetInfo(int theLayout);
	// 0x402290 (mahjongchina), 0x402270 (mahjongjapan)
	void GetSaved(std::string& theName, int* theTiles, int* theFree);
	// 0x4022c0 (mahjongchina), 0x4022a0 (mahjongjapan): the thumbnail drawn into theSheet
	void SetLayout(jb::Image* theSheet, int theLayoutId, int theLayout);
	// 0x4025a0 (mahjongchina), 0x402540 (mahjongjapan): the tiles' places in the cell and the counts
	void Measure(Layout& theLayout);
};

// 0x402720 (mahjongchina), 0x4026c0 (mahjongjapan) (vtable 0x51b6ec (mahjongchina), 0x51c754 (mahjongjapan), 0x8c
// bytes)
class LayoutSelect : public jb::Layer, public DialogListener
{
public:
	LayoutSlot* mSlots[9] = {};       // +0x54
	LayoutHeader* mHeader = nullptr;  // +0x78
	jb::Image* mSheet = nullptr;      // +0x7c
	int mPage = 0;                    // +0x80 the first layout of the page
	int mSelected = 0;                // +0x84 on the page
	int mBuilt = 0;                   // +0x88 the thumbnails drawn

	// 0x402720 (mahjongchina), 0x4026c0 (mahjongjapan)
	LayoutSelect() : Layer(0, 0) {}
	// 0x402760 (mahjongchina), 0x402700 (mahjongjapan), 0x402780 (mahjongchina), 0x402720 (mahjongjapan)
	~LayoutSelect() override;

	void Load() override;                 // 0x4027f0 (mahjongchina), 0x402780 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x402bc0 (mahjongchina), 0x402b40 (mahjongjapan)
	bool OnYes(int theId) override;       // 0x402f10 (mahjongchina), 0x402e80 (mahjongjapan)
	bool OnNo(int theId) override;        // 0x402f20 (mahjongchina), 0x402e90 (mahjongjapan)

	// 0x402d20 (mahjongchina), 0x402c90 (mahjongjapan)
	void ShowPage();
	// 0x402de0 (mahjongchina), 0x402d50 (mahjongjapan)
	void Select(int theSlot);
	// 0x402e20 (mahjongchina), 0x402d90 (mahjongjapan): the question whether to resume its saved game, or the game
	void Choose(int theSlot);
	// 0x402f30 (mahjongchina), 0x402ea0 (mahjongjapan)
	void Start(bool theResume);
	// 0x403030 (mahjongchina), 0x402fa0 (mahjongjapan): the page's saved sheet, or a new one to draw
	void LoadSheet();
	// 0x4032f0 (mahjongchina), 0x403270 (mahjongjapan): one more thumbnail; false once all are drawn (and the sheet
	// saved)
	bool BuildNext();
};

} // namespace mahjong

#endif
