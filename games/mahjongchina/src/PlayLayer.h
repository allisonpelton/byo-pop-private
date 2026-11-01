#ifndef __MAHJONGCHINA_PLAYLAYER_H__
#define __MAHJONGCHINA_PLAYLAYER_H__

// The play layer (0x404fe0..0x406700, vtable 0x51c968, 0xa0 bytes) and what it shows: the background (0x404ef0), the
// panel with the score, the clock and the moves (0x409270), the shading check box (0x406700), the five buttons and
// the board (Board.h). It plays both modes: 0 the escape, a level of the map (0x410470) at a time, and 1 a layout of
// the list (0x4026c0). It saves the game in progress when it is removed and resumes it when asked. The pause
// (0x40a2c0) covers it while the app is inactive.

#include "Dialog.h"
#include "Layer.h"
#include "Object.h"

#include <string>

namespace jb
{
class Button;
class Font;
}

namespace mahjongchina
{

class Board;

// 0x404ef0 (vtable 0x51c8b4, 0x94 bytes)
class Background : public jb::Object
{
public:
	int mImage = 0;  // +0x90

	// 0x404ef0
	Background() {}
	// 0x404f10, 0x404f30: the image freed
	~Background() override;

	void Draw() override;  // 0x404f90
	void Init() override;  // 0x404fc0
};

// 0x406700 (vtable 0x51c990, 0xfc bytes): the check box that shades the covered tiles, with two lines of label
class ShadeBox : public jb::CheckBox
{
public:
	std::string mLine1;         // +0xe8
	std::string mLine2;         // +0xf0
	jb::Font* mFont = nullptr;  // +0xf8

	// 0x406700
	explicit ShadeBox(int theImage) : CheckBox(theImage, 6) {}
	// 0x406770, 0x406790
	~ShadeBox() override {}

	void Draw() override;              // 0x406800
	void OnEnabledChanged() override;  // 0x406920
	void OnClick() override;           // 0x406900
	void OnInit() override;            // 0x4068c0
};

// 0x409270 (vtable 0x51cbfc, 0x130 bytes): the frame of the board, the title, the score, the clock, the free pairs and
// the tiles left, the records and the escape's progress
class Panel : public jb::Object
{
public:
	int mMode;                    // +0x90
	std::string mTime;            // +0x94
	std::string mScore;           // +0x9c
	std::string mPairs;           // +0xa4
	std::string mLeft;            // +0xac
	std::string mTitle;           // +0xb4
	std::string mStat1;           // +0xbc the level's (0x409b10)
	std::string mStat2;           // +0xc4
	std::string mBestTime;        // +0xcc
	std::string mBestScore;       // +0xd4
	std::string mLabelBest;       // +0xdc (0x98)
	std::string mLabelTime;       // +0xe4 (0xa1)
	std::string mLabelScore;      // +0xec (0x9e)
	std::string mLabelPairs;      // +0xf4 (0x9a)
	std::string mLabelLeft;       // +0xfc (0xa0)
	jb::Font* mClockFont = nullptr;   // +0x104
	jb::Font* mSmallFont = nullptr;   // +0x108
	jb::Font* mTitleFont = nullptr;   // +0x10c
	jb::Font* mLabelFont = nullptr;   // +0x110
	jb::Font* mEscapeTitleFont = nullptr;  // China +0x110 (0x128), the escape's title
	bool mRunning = false;        // +0x118
	int mStart = 0;               // +0x11c
	int mSeconds = 0;             // +0x120 while stopped
	int mShown = 0;               // +0x124
	int mSteps = 0;               // +0x128 the escape's progress bar
	int mStep = 0;                // +0x12c (the original leaves it unset; it is drawn only with mSteps)

	// 0x409270
	explicit Panel(int theMode);
	// 0x4093c0, 0x4093e0
	~Panel() override {}

	void Draw() override;    // 0x409510
	void Init() override;    // 0x409970
	bool Update() override;  // 0x409ab0

	// 0x409af0
	void SetTitle(const char* theTitle) { mTitle = theTitle; }
	// 0x409b10: a negative number shows as its own text
	void SetLevel(const char* theTitle, int theStat1, int theStat2);
	// 0x409b90: -1 is empty
	void SetMoves(int thePairs, int theLeft);
	// 0x409be0, 0x409c00
	void SetScore(int theScore) { mScore = jb::FormatNumber(theScore); }
	void SetBestScore(int theScore) { mBestScore = jb::FormatNumber(theScore); }
	// 0x409c20: 36000 is none
	void SetBestTime(int theSeconds);
	// 0x409c50
	void ResetClock();
	// 0x409c80: running from theSeconds
	void StartClock(int theSeconds);
	// 0x409cb0
	void StopClock();
	// 0x409cf0: at most 35999
	int GetSeconds();
	// 0x409d30: the shuffle's penalty (the original adds it as milliseconds to the seconds of a stopped clock)
	void AddSeconds(int theSeconds);
};

// 0x40a0f0 (vtable 0x51cc80, 0x9c bytes): the pause's text; a click ends it
class PauseText : public jb::Object
{
public:
	std::string mText;          // +0x90
	jb::Font* mFont = nullptr;  // +0x98

	// 0x40a0f0
	PauseText() {}
	// 0x40a140, 0x40a160
	~PauseText() override {}

	void Draw() override;                                         // 0x40a1c0
	void Init() override;                                         // 0x40a240
	bool OnLButtonUp(int theFlags, int theX, int theY) override;  // 0x40a290
};

// 0x40a2c0 (vtable 0x51ccd8, 0x54 bytes)
class PauseLayer : public jb::Layer
{
public:
	PauseText* mText = nullptr;  // +0x50

	// 0x40a2c0
	PauseLayer() : Layer(0, 0) {}
	// 0x40a2e0, 0x40a300
	~PauseLayer() override {}

	void Load() override;  // 0x40a310
};

// 0x404fe0
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

	// 0x404fe0
	explicit PlayLayer(int theMode);
	// 0x405070, 0x405090
	~PlayLayer() override;

	void Load() override;                           // 0x405590
	void OnAppActivate(bool theActive) override;   // 0x405ba0
	bool OnCommand(int theCmd) override;           // 0x405a30
	void OnRemove() override { SaveGame(); }       // 0x405c40
	void OnEntered() override;                     // 0x405d80
	void OnShown() override;                       // 0x405f00
	bool OnYes(int theId) override;                // 0x4063e0
	bool OnNo(int theId) override;                 // 0x4064f0

	// 0x405110: a layout of the list
	void SetLayout(int theLevel, bool theResume, int theLayoutId, const char* theName);
	// 0x405160: the next layout of the list, or the question whether to resume its saved game
	void PlayNext();
	// 0x405280
	void StartLayout(bool theResume);
	// 0x405390: a level of the escape
	void StartLevel(int theLevel, bool theResume, int theLayoutId, const char* theTitle, int theBackground,
		int theStat1, int theStat2, int theQuadPercent, uint32_t theSeed, const char* theLevelTitle);
	// 0x4054e0
	void SetProgress(int theSteps, int theStep);
	// 0x405500
	void Begin();
	// 0x405530
	void Restart();
	// 0x405570, 0x405580
	void Undo();
	void Shuffle();
	// 0x405a00
	void SetStyle();
	// 0x405bd0
	void ShowPause();
	// 0x405c50: the saved game loaded, or a new score and clock; the saved file deleted
	void ResetGame();
	// 0x405cb0
	void SetMoves(int thePairs, int theLeft);
	// 0x405cd0: never below 0
	void AddScore(int thePoints);
	// 0x405d00
	void AddSeconds(int theSeconds);
	// 0x405d10
	void SetClockRunning(bool theOn);
	// 0x405d30
	void EnableButtons(bool theOn);
	// 0x405ea0
	void NextBackground();
	// 0x405f10: the tiles are in
	void OnBoardShown();
	// 0x405f50, 0x405f90: the escape's records
	void CheckBestTime();
	void CheckBestScore();
	// 0x405fd0
	void ShowMap();
	// 0x406050
	bool HasSave();
	// 0x406060
	void SaveGame();
	// 0x406190
	bool LoadSave();
	// 0x406330
	void DeleteSave();
	// 0x406510: the list's game is won
	void ShowResult();
	// 0x406610: the escape's level is won
	void ShowLevelDone();
	// 0x4066d0
	void Advance();
};

} // namespace mahjongchina

#endif
