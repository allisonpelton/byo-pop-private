#ifndef __MAHJONG_LEVELDONE_H__
#define __MAHJONG_LEVELDONE_H__

// The level won of the escape, as Japan's game shows it (0x407cd0..0x408670 (mahjongjapan); the other edition's is
// another dialog: China's classes are in its own LevelDone.h): the dialog and its scroll of the next level's text.

#include "Dialog.h"
#include "Layer.h"
#include "Object.h"

#include <string>

namespace jb
{
class Font;
}

namespace mahjong
{

// 0x408020 (mahjongjapan) (vtable 0x51cad8 (mahjongjapan), 0x108 bytes): the scroll of the level won; the next level's
// text fades in after 0.6 s
class LevelDoneScroll : public jb::Object
{
public:
	jb::Font* mTextFont = nullptr;     // +0x90 (FONT_RESULT)
	jb::Font* mHeadFont = nullptr;     // +0x94 (FONT_HEADING)
	jb::Font* mValueFont = nullptr;    // +0x98 (FONT_TEXT)
	jb::Font* mClockFont = nullptr;    // +0x9c (FONT_CLOCK); nothing reads it
	jb::Font* mSmallFont = nullptr;    // +0xa0 (FONT_SMALL)
	std::string mHeading;              // +0xa4 (STRING_LEVEL_COMPLETED)
	std::string mLabelTime;            // +0xac (STRING_TIME_LABEL)
	std::string mTime;                 // +0xb4
	std::string mLabelScore;           // +0xbc (STRING_SCORE_LABEL)
	std::string mScore;                // +0xc4
	std::string mNextLabel;            // +0xcc (STRING_WISDOM_UNLOCKED)
	std::string mNextTitle;            // +0xd4 the next level's text
	std::string mLabelBest;            // +0xdc (STRING_BEST_PREFIX)
	std::string mBestTime;             // +0xe4
	std::string mBestScore;            // +0xec
	std::string mMessage;              // +0xf4
	int mDelay = 600;                  // +0xfc
	int mAlpha = 0;                    // +0x100
	int mLines = 0;                    // +0x104 of the next level's text

	// 0x408020 (mahjongjapan)
	LevelDoneScroll(const char* theNextTitle, const char* theTime, const char* theScore, const char* theBestTime,
		const char* theBestScore, const char* theMessage);
	// 0x408150 (mahjongjapan), 0x408170 (mahjongjapan)
	~LevelDoneScroll() override {}

	void Draw() override;    // 0x408270 (mahjongjapan)
	void Init() override;    // 0x4084e0 (mahjongjapan)
	bool Update() override;  // 0x4085f0 (mahjongjapan)
};

// 0x407cd0 (mahjongjapan) (vtable 0x51cab0 (mahjongjapan), 0x84 bytes): the escape's level won
class LevelDone : public jb::Layer
{
public:
	std::string mTime;       // +0x50
	std::string mScore;      // +0x58
	std::string mNextTitle;  // +0x60
	std::string mBestTime;   // +0x68
	std::string mBestScore;  // +0x70
	std::string mMessage;    // +0x78

	// 0x407cd0 (mahjongjapan)
	LevelDone(int theTime, int theBestTime, bool theNewTime, int theScore, int theBestScore, bool theNewScore,
		const char* theNextTitle);
	// 0x407e20 (mahjongjapan), 0x407e40 (mahjongjapan)
	~LevelDone() override {}

	void Load() override;                 // 0x407ee0 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x407ff0 (mahjongjapan)
};

} // namespace mahjong

#endif
