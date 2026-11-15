#ifndef __MAHJONG_PLAYLAYER_H__
#define __MAHJONG_PLAYLAYER_H__

// The play layer (0x405280..0x406960 (mahjongchina), 0x404fe0..0x406700 (mahjongjapan), vtable 0x51b8e8 (mahjongchina),
// 0x51c968 (mahjongjapan), 0xa0 bytes) and what it shows: the background (0x405190 (mahjongchina),
// 0x404ef0 (mahjongjapan)), the panel with the score, the clock and the moves (0x4090b0 (mahjongchina),
// 0x409270 (mahjongjapan)), the shading check box (0x406960 (mahjongchina), 0x406700 (mahjongjapan)), the five buttons
// and the board (Board.h). It plays both modes: 0 the escape, a level of the map (0x4100e0 (mahjongchina),
// 0x410470 (mahjongjapan)) at a time, and 1 a layout of the list (0x402720 (mahjongchina), 0x4026c0 (mahjongjapan)). It
// saves the game in progress when it is removed and resumes it when asked. The pause (0x40a140 (mahjongchina),
// 0x40a2c0 (mahjongjapan)) covers it while the app is inactive.

#include "Dialog.h"
#include "Edition.h"
#include "Layer.h"
#include "Object.h"

#include <string>

namespace jb
{
class Button;
class Font;
}

namespace mahjong
{

class Board;

// 0x405190 (mahjongchina), 0x404ef0 (mahjongjapan) (vtable 0x51b84c (mahjongchina), 0x51c8b4 (mahjongjapan), 0x94
// bytes)
class Background : public jb::Object
{
public:
	int mImage = 0;  // +0x90

	// 0x405190 (mahjongchina), 0x404ef0 (mahjongjapan)
	Background() {}
	// 0x4051b0 (mahjongchina), 0x404f10 (mahjongjapan), 0x4051d0 (mahjongchina), 0x404f30 (mahjongjapan): the image
	// freed
	~Background() override;

	void Draw() override;  // 0x405230 (mahjongchina), 0x404f90 (mahjongjapan)
	void Init() override;  // 0x405260 (mahjongchina), 0x404fc0 (mahjongjapan)
};

// 0x406960 (mahjongchina), 0x406700 (mahjongjapan) (vtable 0x51b910 (mahjongchina), 0x51c990 (mahjongjapan), 0xfc
// bytes): the check box that shades the covered tiles, with two lines of label
class ShadeBox : public jb::CheckBox
{
public:
	std::string mLine1;         // +0xe8
	std::string mLine2;         // +0xf0
	jb::Font* mFont = nullptr;  // +0xf8

	// 0x406960 (mahjongchina), 0x406700 (mahjongjapan)
	explicit ShadeBox(int theImage) : CheckBox(theImage, 6) {}
	// 0x4069d0 (mahjongchina), 0x406770 (mahjongjapan), 0x4069f0 (mahjongchina), 0x406790 (mahjongjapan)
	~ShadeBox() override {}

	void Draw() override;              // 0x406a60 (mahjongchina), 0x406800 (mahjongjapan)
	void OnEnabledChanged() override;  // 0x406b70 (mahjongchina), 0x406920 (mahjongjapan)
	void OnClick() override;           // 0x406b50 (mahjongchina), 0x406900 (mahjongjapan)
	void OnInit() override;            // 0x406b10 (mahjongchina), 0x4068c0 (mahjongjapan)
};

// 0x4090b0 (mahjongchina), 0x409270 (mahjongjapan) (vtable 0x51bb84 (mahjongchina), 0x51cbfc (mahjongjapan), 0x130
// bytes): the frame of the board, the title, the score, the clock, the free pairs and the tiles left, the records and
// the escape's progress
class Panel : public jb::Object
{
public:
	int mMode;                    // +0x90
	std::string mTime;            // +0x94
	std::string mScore;           // +0x9c
	std::string mPairs;           // +0xa4
	std::string mLeft;            // +0xac
	std::string mTitle;           // +0xb4
	std::string mStat1;           // +0xbc the level's (0x4099d0 (mahjongchina), 0x409b10 (mahjongjapan))
	std::string mStat2;           // +0xc4
	std::string mBestTime;        // +0xcc
	std::string mBestScore;       // +0xd4
	std::string mLabelBest;       // +0xdc (STRING_PANEL_BEST)
	std::string mLabelTime;       // +0xe4 (STRING_PANEL_TIME)
	std::string mLabelScore;      // +0xec (STRING_PANEL_SCORE)
	std::string mLabelPairs;      // +0xf4 (STRING_PANEL_MATCHES)
	std::string mLabelLeft;       // +0xfc (STRING_PANEL_TILES)
	jb::Font* mClockFont = nullptr;   // +0x104
	jb::Font* mSmallFont = nullptr;   // +0x108
	jb::Font* mTitleFont = nullptr;   // +0x10c
	jb::Font* mLabelFont = nullptr;   // +0x110
#if EDITION_ESCAPE_TITLE_FONT
	jb::Font* mEscapeTitleFont = nullptr;  // China +0x110 (0x128), the escape's title
#endif
	bool mRunning = false;        // +0x118
	int mStart = 0;               // +0x11c
	int mSeconds = 0;             // +0x120 while stopped
	int mShown = 0;               // +0x124
	int mSteps = 0;               // +0x128 the escape's progress bar
	int mStep = 0;                // +0x12c (the original leaves it unset; it is drawn only with mSteps)

	// 0x4090b0 (mahjongchina), 0x409270 (mahjongjapan)
	explicit Panel(int theMode);
	// 0x409200 (mahjongchina), 0x4093c0 (mahjongjapan), 0x409220 (mahjongchina), 0x4093e0 (mahjongjapan)
	~Panel() override {}

	void Draw() override;    // 0x409350 (mahjongchina), 0x409510 (mahjongjapan)
	void Init() override;    // 0x409800 (mahjongchina), 0x409970 (mahjongjapan)
	bool Update() override;  // 0x409970 (mahjongchina), 0x409ab0 (mahjongjapan)

	// 0x4099b0 (mahjongchina), 0x409af0 (mahjongjapan)
	void SetTitle(const char* theTitle) { mTitle = theTitle; }
	// 0x4099d0 (mahjongchina), 0x409b10 (mahjongjapan): a negative number shows as its own text
	void SetLevel(const char* theTitle, int theStat1, int theStat2);
	// 0x409a50 (mahjongchina), 0x409b90 (mahjongjapan): -1 is empty
	void SetMoves(int thePairs, int theLeft);
	// 0x409aa0 (mahjongchina), 0x409be0 (mahjongjapan), 0x409ac0 (mahjongchina), 0x409c00 (mahjongjapan)
	void SetScore(int theScore) { mScore = jb::FormatNumber(theScore); }
	void SetBestScore(int theScore) { mBestScore = jb::FormatNumber(theScore); }
	// 0x409ae0 (mahjongchina), 0x409c20 (mahjongjapan): 36000 is none
	void SetBestTime(int theSeconds);
	// 0x409b10 (mahjongchina), 0x409c50 (mahjongjapan)
	void ResetClock();
	// 0x409b40 (mahjongchina), 0x409c80 (mahjongjapan): running from theSeconds
	void StartClock(int theSeconds);
	// 0x409b70 (mahjongchina), 0x409cb0 (mahjongjapan)
	void StopClock();
	// 0x409bb0 (mahjongchina), 0x409cf0 (mahjongjapan): at most 35999
	int GetSeconds();
	// 0x409bf0 (mahjongchina), 0x409d30 (mahjongjapan): the shuffle's penalty (the original adds it as milliseconds to
	// the seconds of a stopped clock)
	void AddSeconds(int theSeconds);
};

// 0x409fa0 (mahjongchina), 0x40a0f0 (mahjongjapan) (vtable 0x51bc08 (mahjongchina), 0x51cc80 (mahjongjapan), 0x9c
// bytes): the pause's text; a click ends it
class PauseText : public jb::Object
{
public:
	std::string mText;          // +0x90
	jb::Font* mFont = nullptr;  // +0x98

	// 0x409fa0 (mahjongchina), 0x40a0f0 (mahjongjapan)
	PauseText() {}
	// 0x409ff0 (mahjongchina), 0x40a140 (mahjongjapan), 0x40a010 (mahjongchina), 0x40a160 (mahjongjapan)
	~PauseText() override {}

	void Draw() override;                                         // 0x40a070 (mahjongchina), 0x40a1c0 (mahjongjapan)
	void Init() override;                                         // 0x40a0e0 (mahjongchina), 0x40a240 (mahjongjapan)
	bool OnLButtonUp(int theFlags, int theX, int theY) override;  // 0x40a120 (mahjongchina), 0x40a290 (mahjongjapan)
};

// 0x40a140 (mahjongchina), 0x40a2c0 (mahjongjapan) (vtable 0x51bc60 (mahjongchina), 0x51ccd8 (mahjongjapan), 0x54
// bytes)
class PauseLayer : public jb::Layer
{
public:
	PauseText* mText = nullptr;  // +0x50

	// 0x40a140 (mahjongchina), 0x40a2c0 (mahjongjapan)
	PauseLayer() : Layer(0, 0) {}
	// 0x40a160 (mahjongchina), 0x40a2e0 (mahjongjapan), 0x40a180 (mahjongchina), 0x40a300 (mahjongjapan)
	~PauseLayer() override {}

	void Load() override;  // 0x40a190 (mahjongchina), 0x40a310 (mahjongjapan)
};

// 0x405280 (mahjongchina), 0x404fe0 (mahjongjapan)
class PlayLayer : public jb::Layer, public DialogListener
{
public:
	Background* mBackground = nullptr;  // +0x54
	Panel* mPanel = nullptr;            // +0x58
	Board* mBoard = nullptr;            // +0x5c
	ShadeBox* mShade = nullptr;         // +0x60
	jb::Button* mUndo = nullptr;        // +0x64
	jb::Button* mHint = nullptr;        // +0x68
	jb::Button* mShuffle = nullptr;     // +0x6c
	jb::Button* mMenu = nullptr;        // +0x70
	jb::Button* mPause = nullptr;       // +0x74
	int mMode;                          // +0x78 0 the escape, 1 the list
	int mLevel = 0;                     // +0x7c the escape's level or the layout's place in the list
	int mTip = -1;                      // +0x80 the tip shown when the tiles are in
	int mScore = 0;                     // +0x84
	bool mNewBestTime = false;          // +0x88
	bool mNewBestScore = false;         // +0x89
	bool mResume = false;               // +0x8a the saved game goes on
	bool mProgressed = false;           // +0x8b the level won was the next of the escape
	std::string mLevelTitle;            // +0x8c
	int mLayoutId = 0;                  // +0x94
	std::string mLayoutName;            // +0x98

	// 0x405280 (mahjongchina), 0x404fe0 (mahjongjapan)
	explicit PlayLayer(int theMode);
	// 0x405310 (mahjongchina), 0x405070 (mahjongjapan), 0x405330 (mahjongchina), 0x405090 (mahjongjapan)
	~PlayLayer() override;

	void Load() override;                           // 0x405830 (mahjongchina), 0x405590 (mahjongjapan)
	void OnAppActivate(bool theActive) override;   // 0x405dd0 (mahjongchina), 0x405ba0 (mahjongjapan)
	bool OnCommand(int theCmd) override;           // 0x405c60 (mahjongchina), 0x405a30 (mahjongjapan)
	void OnRemove() override { SaveGame(); }       // 0x405e70 (mahjongchina), 0x405c40 (mahjongjapan)
	void OnEntered() override;                     // 0x405fb0 (mahjongchina), 0x405d80 (mahjongjapan)
	void OnShown() override;                       // 0x406130 (mahjongchina), 0x405f00 (mahjongjapan)
	bool OnYes(int theId) override;                // 0x4065c0 (mahjongchina), 0x4063e0 (mahjongjapan)
	bool OnNo(int theId) override;                 // 0x406780 (mahjongchina), 0x4064f0 (mahjongjapan)

	// 0x4053b0 (mahjongchina), 0x405110 (mahjongjapan): a layout of the list
	void SetLayout(int theLevel, bool theResume, int theLayoutId, const char* theName);
	// 0x405400 (mahjongchina), 0x405160 (mahjongjapan): the next layout of the list, or the question whether to resume
	// its saved game
	void PlayNext();
	// 0x405520 (mahjongchina), 0x405280 (mahjongjapan)
	void StartLayout(bool theResume);
	// 0x405630 (mahjongchina), 0x405390 (mahjongjapan): a level of the escape
	void StartLevel(int theLevel, bool theResume, int theLayoutId, const char* theTitle, int theBackground,
		int theStat1, int theStat2, int theQuadPercent, uint32_t theSeed, const char* theLevelTitle);
	// 0x405780 (mahjongchina), 0x4054e0 (mahjongjapan)
	void SetProgress(int theSteps, int theStep);
	// 0x4057a0 (mahjongchina), 0x405500 (mahjongjapan)
	void Begin();
	// 0x4057d0 (mahjongchina), 0x405530 (mahjongjapan)
	void Restart();
	// 0x405810 (mahjongchina), 0x405570 (mahjongjapan), 0x405820 (mahjongchina), 0x405580 (mahjongjapan)
	void Undo();
	void Shuffle();
	// 0x405c30 (mahjongchina), 0x405a00 (mahjongjapan)
	void SetStyle();
	// 0x405e00 (mahjongchina), 0x405bd0 (mahjongjapan)
	void ShowPause();
	// 0x405e80 (mahjongchina), 0x405c50 (mahjongjapan): the saved game loaded, or a new score and clock; the saved file
	// deleted
	void ResetGame();
	// 0x405ee0 (mahjongchina), 0x405cb0 (mahjongjapan)
	void SetMoves(int thePairs, int theLeft);
	// 0x405f00 (mahjongchina), 0x405cd0 (mahjongjapan): never below 0
	void AddScore(int thePoints);
	// 0x405f30 (mahjongchina), 0x405d00 (mahjongjapan)
	void AddSeconds(int theSeconds);
	// 0x405f40 (mahjongchina), 0x405d10 (mahjongjapan)
	void SetClockRunning(bool theOn);
	// 0x405f60 (mahjongchina), 0x405d30 (mahjongjapan)
	void EnableButtons(bool theOn);
	// 0x4060d0 (mahjongchina), 0x405ea0 (mahjongjapan)
	void NextBackground();
	// 0x406140 (mahjongchina), 0x405f10 (mahjongjapan): the tiles are in
	void OnBoardShown();
	// 0x406180 (mahjongchina), 0x405f50 (mahjongjapan), 0x4061b0 (mahjongchina), 0x405f90 (mahjongjapan): the escape's
	// records
	void CheckBestTime();
	void CheckBestScore();
	// 0x4061e0 (mahjongchina), 0x405fd0 (mahjongjapan)
	void ShowMap();
	// 0x406260 (mahjongchina), 0x406050 (mahjongjapan)
	bool HasSave();
	// 0x406270 (mahjongchina), 0x406060 (mahjongjapan)
	void SaveGame();
	// 0x406390 (mahjongchina), 0x406190 (mahjongjapan)
	bool LoadSave();
	// 0x406530 (mahjongchina), 0x406330 (mahjongjapan)
	void DeleteSave();
#if EDITION_TIPS_OFF
	// 0x4063c0 (mahjongjapan): no tips once the level has a best score
	bool TipsOff();
#endif
	// 0x4067a0 (mahjongchina), 0x406510 (mahjongjapan): the list's game is won
	void ShowResult();
	// 0x4068a0 (mahjongchina), 0x406610 (mahjongjapan): the escape's level is won
	void ShowLevelDone();
	// 0x406930 (mahjongchina), 0x4066d0 (mahjongjapan)
	void Advance();
};

} // namespace mahjong

#endif
