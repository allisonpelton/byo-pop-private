// Sexy::StatsBook (0xfc bytes, constructor 0x493830, vtables 0x7b0ae0 (the LuaLinkedClass part at +0), 0x7b0b0c (the
// widget, at +0x28) and 0x7b0c28, 0x7b0c48, 0x7b0c58, 0x7b0c60 (its ButtonListener, ListListener, SliderListener
// and FlashListener parts at +0xb0 to +0xbc); code 0x493610-0x4950f0): the book of what the player has seen. It
// is the tome of knowledge (the bestiary: the creatures met, with their attacks and defences), the media player
// (the cut scenes and songs of the game) and the page of statistics that closes a timed game, one Lua object
// (gStatsBook, scripts/statsbook) that does the drawing and the choosing; this class is what it stands on. It
// lays itself out from a data/layout_*.xml file (SetLayoutFile), has a list of lines (AddLine, with a second text
// at the right of each: AddSecondaryLine) with a slider and two arrow buttons when the list is long, a "Continue"
// button, and, for the media player, a Flash movie (PlaySWF) or a song (PlayMusic) on its screen. It drops in from
// above and bounces once (Bounce).
//
// port: the media player's music visualiser (0x494a10: each update the music interface's spectrum, 0x6c50a0, is
// drawn into a 320 x 240 picture that shows on the screen while a song plays) is not ported: the port's music
// player has no spectrum, so the screen stays empty during a song.
#pragma once

#include <map>
#include <string>

#include "FlashWidget.h"
#include "ListWidget.h"
#include "LuaWidgets.h"
#include "SliderListener.h"

namespace Sexy {

class FadeButton;
class GameApp;
class MemoryImage;
class Slider;
class XLayout;

// Sexy::CustomListWidget (0x140 bytes, constructor 0x495100, vtable 0x7b0984): the stats book's list. A ListWidget
// that draws its lines one after the other with a bar of light behind the selected line and one under the mouse,
// a second text at the right of a line, the line named by SetRedString in red, and a "playing" mark beside the line
// that SetIconIndex says; it takes the mouse only inside its own rectangle, and passes the wheel to the book.
class CustomListWidget : public ListWidget {
public:
	std::map<std::string, std::string> mSecondary;  // +0xf8: the second text of a line, by the line
	Widget* mOwner = nullptr;      // +0x104: the book, which hears the mouse wheel first
	std::string mRedString;        // +0x108
	Rect mHitRect;                 // +0x124: where the mouse is over the list
	int mIconIndex = -1;           // +0x134
	bool mUnused138 = false;       // +0x138: the book sets it for the page after a timed game; nothing reads it

	explicit CustomListWidget(ListListener* theListener);  // 0x495100: id 1, FONT_LARGE

	void Draw(Graphics* g) override;                                                  // 0x4952a0
	void MouseWheel(int theDelta) override;                                           // 0x495760
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;          // 0x495790
	using ListWidget::MouseDown;
	bool IsPointVisible(int x, int y) override;                                       // 0x4936f0
};

class StatsBook : public LuaWidget, public SliderListener, public FlashListener {
public:
	GameApp* mApp;                     // (port)
	XLayout* mLayout;                  // +0xc0
	FadeButton* mContinueButton;       // +0xc4: id 1
	CustomListWidget* mList;           // +0xc8
	Slider* mSlider;                   // +0xcc: beside the list, vertical
	FadeButton* mScrollUp;             // +0xd0: id 10
	FadeButton* mScrollDown;           // +0xd4: id 11
	FlashWidget* mFlash;               // +0xd8: the media player's movie (a MyFlashWidget)
	MemoryImage* mScreenImage;         // +0xdc: 320 x 240, the visualiser's picture (not ported)
	bool mBouncing = false;            // +0xe4: dropping in
	bool mFromGame;                    // +0xe5: opened from the game (after a timed game)
	bool mBestiary;                    // +0xe6: the tome of knowledge
	bool mMediaPlayer;                 // +0xe7
	bool mPlayingMusic = false;        // +0xe8
	bool mPlayingMovie = false;        // +0xe9
	int mScreenMode = 1;               // +0xec: what the screen shows during a song (a click on it changes it)
	int mCountdown = 100;              // +0xf4: updates till the visualiser starts
	float mBounceSpeed = 0.0f;         // +0xf8: of the drop

	// theFromGame, theBestiary and theMediaPlayer as AddStatsBook was given them
	StatsBook(GameApp* theApp, bool theFromGame, bool theBestiary, bool theMediaPlayer);  // 0x493830
	~StatsBook() override;  // 0x493fa0

	const char* LuaGetRegisteredName() override { return "StatsBook"; }  // 0x493770
	// The original registered the class in the constructor, every time
	static void RegisterLua();

	// The scripts' methods
	void SetLayoutFile(const std::string& theFile);  // 0x4942f0: reads the layout and puts the widgets in it
	void AddLine(const std::string& theLine, bool theAlphabetical);  // 0x4941f0: and the slider and arrows when there are more than 14
	void ClearList();                                // 0x494420
	void SelectIndex(int theIdx);                    // 0x4949b0: as a click on that line
	void AddSecondaryLine(const std::string& theLine, const std::string& theText);  // 0x4937c0
	void Bounce();                                   // 0x4941c0
	void SetIconIndex(int theIdx) { mList->mIconIndex = theIdx; }  // 0x493790
	bool IsBouncing() { return mBouncing; }          // 0x4937b0
	void ResetSlider();                              // 0x494170
	void ListClicked(int theId, int theIdx, int theClickCount) override;  // 0x494490
	int GetNumLines() { return (int)mList->mLines.size(); }  // 0x4957e0
	void PlaySWF(const std::string& theFile);        // 0x495820
	void PlayMusic(int theSong);                     // 0x495910
	void HideArrowButtons();                         // 0x494100
	void SetRedString(const std::string& theLine) { mList->mRedString = theLine; }  // 0x494150

	// Widget
	void AddedToManager(WidgetManager* theWidgetManager) override;  // 0x495090
	void Update() override;                                         // 0x494a10
	void Draw(Graphics* g) override;                                // 0x494cc0
	void MouseMove(int x, int y) override;                          // 0x4943e0
	void MouseDown(int x, int y, int theClickCount) override;       // 0x494340
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override { Widget::MouseDown(x, y, theBtnNum, theClickCount); }
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override { Widget::MouseUp(x, y, theBtnNum, theClickCount); }
	void MouseDrag(int x, int y) override {}                        // 0x4943d0
	void MouseWheel(int theDelta) override;                         // 0x494f30

	// ButtonListener (0x4947c0, 0x4947e0); the rest of it does nothing (the Lua object has none of those methods)
	void ButtonPress(int theId, int theClickCount) override;
	void ButtonDepress(int theId) override;
	void ButtonDownTick(int theId) override {}
	void ButtonMouseEnter(int theId) override {}
	void ButtonMouseLeave(int theId) override {}
	void ButtonMouseMove(int theId, int theX, int theY) override {}
	// ListListener: ListClicked above; the list closing and the line under the mouse changing are nothing to it
	void ListClosed(int theId) override {}
	void ListHiliteChanged(int theId, int theOldIdx, int theNewIdx) override {}
	// SliderListener (0x494df0): the slider moved: the list scrolls
	void SliderVal(int theId, double theVal) override;

private:
	void ScrollList(int thePosition);  // 0x494e60: the list's first line, kept in range (and the line under the mouse with it)
	void PositionWidgets();            // 0x494540: the layout's places for the continue button, the list, the slider and the arrows
	void ShowArrows(bool theShow);     // the slider and arrows of a long list, or not
};

}  // namespace Sexy
