#ifndef __MAHJONG_LEVELDONE_H__
#define __MAHJONG_LEVELDONE_H__

// The level won of the escape, as China's game shows it (0x407cd0..0x408670; the other edition's is another
// dialog: Japan's classes are in its own LevelDone.h): the dialog and its scroll of the next level's text.

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

// 0x408270 (mahjongchina) (vtable 0x51ba5c (mahjongjapan), 0xac bytes): the level won's scroll of the next level's
// text, which fades in after 0.6 s. China's: Japan's scroll also held the time, the score, the bests and the records
class LevelDoneScroll : public jb::Object
{
public:
	jb::Font* mTextFont = nullptr;     // +0x8c (FONT_RESULT)
	std::string mNextLabel;            // +0x90 (STRING_WISDOM_UNLOCKED)
	std::string mNextTitle;            // +0x98 the next level's text
	int mDelay = 600;                  // +0xa0
	int mAlpha = 0;                    // +0xa4
	int mLines = 0;                    // +0xa8 of the next level's text

	// 0x408270 (mahjongchina)
	explicit LevelDoneScroll(const char* theNextTitle) : mNextTitle(theNextTitle) {}
	// 0x4082f0 (mahjongchina), 0x408310 (mahjongchina)
	~LevelDoneScroll() override {}

	void Draw() override;    // 0x408380 (mahjongchina)
	void Init() override;    // 0x408410 (mahjongchina)
	bool Update() override;  // 0x408470 (mahjongchina)
};

// 0x407f10 (mahjongchina) (vtable 0x51ba30 (mahjongjapan), 0x70 bytes): the escape's level won, a dialog with the time
// and the score. China's: Japan's was a layer of its own with the bests and the records on the scroll
class LevelDone : public Dialog
{
public:
	std::string mTime;       // +0x54
	std::string mScore;      // +0x5c
	std::string mNextTitle;  // +0x64

	// 0x407f10 (mahjongchina)
	LevelDone(int theTime, int theBestTime, bool theNewTime, int theScore, int theBestScore, bool theNewScore,
		const char* theNextTitle);
	// 0x407fd0 (mahjongchina)
	~LevelDone() override {}

	void Create() override;               // 0x408040 (mahjongchina)
	bool OnCommand(int theCmd) override;  // 0x408240 (mahjongchina)
};

} // namespace mahjong

#endif
