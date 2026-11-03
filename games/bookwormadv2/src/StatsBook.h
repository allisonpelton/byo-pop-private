// Sexy::StatsBook (0x10c bytes, constructor inlined in 0x47f5a0, vtables 0x6ff220 (the LuaLinkedClass part at +0),
// 0x6ff24c (the widget, at +0x28) and 0x6ff368, 0x6ff388, 0x6ff398, 0x6ff3a0 (its ButtonListener, ListListener,
// SliderListener and FlashListener parts at +0xb0 to +0xbc); code 0x47f4d0-0x481610): the book of what the player
// has seen. It is the tome of knowledge (the bestiary: the creatures met, with their attacks and defences, and the
// secrets found), the media player (the cut scenes of the game) and the page of statistics that closes a timed game,
// one Lua object (gStatsBook, scripts/statsbook) that does the drawing and the choosing; this class is what it
// stands on. It lays itself out from a data/layout_*.xml file (SetLayoutFile), has a list of lines (AddLine, with a
// second text at the right of each: AddSecondaryLine, or AddNewAreaLine in the tome) with a slider and two arrow
// buttons when the list is long, the tome's list of secrets (AddSecretLine) beside it with its own slider, a
// "Continue" button, and, for the media player, a Flash movie (PlaySWF) on its screen. It drops in from above and
// bounces once (Bounce).
//
// The media player of Volume 1 played the songs of the game and showed their spectrum; here PlayMusic only notes that
// a song is playing, and plays none. The spectrum code of Update is still there (Update, 0x480b90: the app's music
// interface draws the spectrum into a map each fourth update, JeffLib::SongPlayer::UpdateSpectrum here, and the book
// paints the map into a 320 x 240 picture), and a click on the media player's screen changes between its two kinds.
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

// Sexy::CustomListWidget (0x148 bytes, constructor 0x481650, vtable 0x6fef64): the stats book's list. A ListWidget
// that draws its lines one after the other with a bar of light behind the selected line and one under the mouse,
// a second text at the right of a line, the line named by SetRedString in red, and a "playing" mark beside the line
// that SetIconIndex says; it takes the mouse only inside its own rectangle, and passes the wheel to the book. A
// line is drawn without what follows its first "(" and, if it is still longer than 19 letters, cut to 18 with
// "..." after them.
class CustomListWidget : public ListWidget {
public:
	std::map<std::string, std::string> mSecondary;  // +0xf8: the second text of a line, by the line (AddSecondaryLine)
	std::map<std::string, std::string> mNewArea;    // +0x104: the other second text (AddNewAreaLine)
	Widget* mOwner = nullptr;      // +0x110: the book, which hears the mouse wheel first
	std::string mRedString;        // +0x114
	Rect mHitRect;                 // +0x130: where the mouse is over the list
	int mIconIndex = -1;           // +0x140
	bool mUnused144 = false;       // +0x144
	bool mFromGame;                // +0x145: the book is open after a timed game: the second texts are mSecondary's, else mNewArea's

	CustomListWidget(ListListener* theListener, bool theFromGame);  // 0x481650: id 1, FONT_LARGE

	void Draw(Graphics* g) override;                                                  // 0x481840
	void MouseWheel(int theDelta) override;                                           // 0x481e40
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;          // 0x481e70
	using ListWidget::MouseDown;
	bool IsPointVisible(int x, int y) override;                                       // 0x47f430
};

// Sexy::SecretListWidget (0x130 bytes, constructor 0x481ec0, vtable 0x6ff0c4): the tome's list of secrets, a
// ListWidget of its own (id 2) beside the creatures' list. Each of its lines is a tray with, once the secret is
// found, its name written on it ("???" while it is not). It takes no clicks and shows no hand; the wheel goes to
// the book first, as the other list's does.
class SecretListWidget : public ListWidget {
public:
	Widget* mOwner = nullptr;      // +0xf8: the book, which hears the mouse wheel first
	Rect mHitRect;                 // +0x118: where the mouse is over the list
	int mUnused128 = -1;           // +0x128
	bool mUnused12c = false;       // +0x12c

	SecretListWidget(ListListener* theListener);  // 0x481ec0: id 2, FONT_LARGE

	void Draw(Graphics* g) override;                                          // 0x481fa0
	void MouseWheel(int theDelta) override;                                   // 0x482330
	void MouseMove(int x, int y) override;                                    // 0x482360: no hand over a line
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override {}  // (an empty stub of the exe's)
	using ListWidget::MouseDown;
	bool IsPointVisible(int x, int y) override;                               // 0x47f480
};

class StatsBook : public LuaWidget, public SliderListener, public FlashListener {
public:
	GameApp* mApp;                     // (port)
	XLayout* mLayout;                  // +0xc0
	FadeButton* mContinueButton;       // +0xc4: id 1
	CustomListWidget* mList;           // +0xc8
	Slider* mSlider;                   // +0xcc: beside the list, vertical, id 1
	FadeButton* mScrollUp;             // +0xd0: id 10
	FadeButton* mScrollDown;           // +0xd4: id 11
	FlashWidget* mFlash;               // +0xd8: the media player's movie (a MyFlashWidget)
	MemoryImage* mScreenImage;         // +0xdc: 320 x 240, the spectrum's picture
	bool mBouncing = false;            // +0xe4: dropping in
	bool mFromGame;                    // +0xe5: opened from the game (after a timed game)
	bool mBestiary;                    // +0xe6: the tome of knowledge
	bool mMediaPlayer;                 // +0xe7
	bool mPlayingMusic = false;        // +0xe8
	bool mPlayingMovie = false;        // +0xe9
	int mScreenMode = 1;               // +0xec: what the screen shows during a song (a click on it changes it)
	int mCountdown = 100;              // +0xf4: updates till the visualiser starts
	float mBounceSpeed = 0.0f;         // +0xf8: of the drop
	SecretListWidget* mSecretList;     // +0xfc: the tome's secrets
	Slider* mSecretSlider;             // +0x100: beside the secrets, vertical, id 2
	FadeButton* mSecretScrollUp;       // +0x104: id 12 (the book never shows it)
	FadeButton* mSecretScrollDown;     // +0x108: id 13 (nor this one)

	// theFromGame, theBestiary and theMediaPlayer as AddStatsBook was given them
	StatsBook(GameApp* theApp, bool theFromGame, bool theBestiary, bool theMediaPlayer);  // 0x47f5a0
	~StatsBook() override;  // 0x47ff80

	const char* LuaGetRegisteredName() override { return "StatsBook"; }  // 0x47f4d0
	// The original registered the class in the constructor, every time
	static void RegisterLua();

	// The scripts' methods
	void SetLayoutFile(const std::string& theFile);  // 0x480320: reads the layout and puts the widgets in it
	void AddLine(const std::string& theLine, bool theAlphabetical);  // 0x480220: and the slider and arrows when there are more than 12
	void ClearList();                                // 0x480410
	void SelectIndex(int theIdx);                    // 0x480b60: as a click on that line
	void AddSecondaryLine(const std::string& theLine, const std::string& theText);  // 0x47f510
	void AddNewAreaLine(const std::string& theLine, const std::string& theText);    // 0x47f540
	void AddSecretLine(const std::string& theLine);  // 0x47f570: to the list of secrets
	void Bounce();                                   // 0x4801f0
	void SetIconIndex(int theIdx) { mList->mIconIndex = theIdx; }  // 0x47f4e0
	bool IsBouncing() { return mBouncing; }          // 0x47f500
	void ResetSlider();                              // 0x480150: both sliders and lists
	void ListClicked(int theId, int theIdx, int theClickCount) override;  // 0x480480
	int GetNumLines() { return (int)mList->mLines.size(); }
	void PlaySWF(const std::string& theFile);        // 0x481500
	void PlayMusic(int theSong);                     // 0x4815e0: only notes it (see the top)
	void HideArrowButtons();                         // 0x4800e0
	void SetRedString(const std::string& theLine) { mList->mRedString = theLine; }  // 0x480130
	void Rehup();                                    // 0x480530: puts the widgets where the layout says (SetLayoutFile does it too)
	void ReloadSecrets();                            // 0x481610: the list of secrets empties and the scripts' RehupSecretList fills it

	// Widget
	void AddedToManager(WidgetManager* theWidgetManager) override;  // 0x481450
	void Update() override;                                         // 0x480b90
	void Draw(Graphics* g) override;                                // 0x480e40
	void MouseMove(int x, int y) override;                          // 0x4803d0
	void MouseDown(int x, int y, int theClickCount) override;       // 0x480350
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override { Widget::MouseDown(x, y, theBtnNum, theClickCount); }
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override { Widget::MouseUp(x, y, theBtnNum, theClickCount); }
	void MouseUp(int x, int y, int theLastDownButtonId) override {}
	void MouseDrag(int x, int y) override {}
	void MouseWheel(int theDelta) override;                         // 0x4811d0
	// The original's book is a plain Widget beside its Lua link, not a LuaWidgetTpl: these are Widget's own, where
	// the port's LuaWidget would call the Lua object, which has none of them (a mouse over the book ended the game)
	bool WantsFocus() override { return Widget::WantsFocus(); }
	void DrawOverlay(Graphics* g) override { Widget::DrawOverlay(g); }
	void GotFocus() override { Widget::GotFocus(); }
	void LostFocus() override { Widget::LostFocus(); }
	void KeyChar(SexyChar theChar) override { Widget::KeyChar(theChar); }
	void KeyDown(KeyCode theKey) override { Widget::KeyDown(theKey); }
	void KeyUp(KeyCode theKey) override { Widget::KeyUp(theKey); }
	void MouseEnter() override { Widget::MouseEnter(); }
	void MouseLeave() override { Widget::MouseLeave(); }
	bool IsPointVisible(int x, int y) override { return Widget::IsPointVisible(x, y); }

	// ButtonListener (0x480930, 0x480950); the rest of it does nothing (the Lua object has none of those methods)
	void ButtonPress(int theId, int theClickCount) override;
	void ButtonDepress(int theId) override;
	void ButtonDownTick(int theId) override {}
	void ButtonMouseEnter(int theId) override {}
	void ButtonMouseLeave(int theId) override {}
	void ButtonMouseMove(int theId, int theX, int theY) override {}
	// ListListener: ListClicked above; the list closing and the line under the mouse changing are nothing to it
	void ListClosed(int theId) override {}
	void ListHiliteChanged(int theId, int theOldIdx, int theNewIdx) override {}
	// SliderListener (0x480f70): a slider moved: its list scrolls
	void SliderVal(int theId, double theVal) override;

private:
	void ScrollList(int theId, int thePosition);  // 0x481050: a list's first line, kept in range (and the line under the mouse with it)
	void ShowArrows(bool theShow);     // the slider and arrows of a long list, or not
};

}  // namespace Sexy
