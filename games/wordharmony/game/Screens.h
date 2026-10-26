#ifndef __WORDHARMONY_SCREENS_H__
#define __WORDHARMONY_SCREENS_H__

// The game's screens (0x409c20-0x40db70): layouts of the archive with a little code each.

#include "ScreenLayout.h"
#include <string>

namespace engine
{
class TextItem;
}

// The level's end (constructor 0x409c20, vtable 0x480e88)
class CongratsScreen : public engine::ScreenLayout
{
public:
	CongratsScreen();  // 0x409c20
};

// The game's end (constructor 0x40b0d0, vtable 0x481110)
class GameOverScreen : public engine::ScreenLayout
{
public:
	GameOverScreen();  // 0x40b0d0

	// 0x40aec0: the score, the hints used and the five longest words; the extra scores outside the puzzles
	void showResults();
};

// The name entry of a new high score (0x108 bytes, constructor 0x40b510, vtable 0x481238; its key handler at +0xd4,
// vtable 0x481230). Enter tells the controller "highScoreEntryDone".
class HighScoreEntryLayout : public engine::ScreenLayout, public engine::KeyHandler
{
public:
	HighScoreEntryLayout();             // 0x40b510
	~HighScoreEntryLayout() override;   // 0x40b410

	// 0x40b1f0: the name to edit and the score shown, the keys taken
	void startEntry(std::string theName, int theScore);

	bool keyDown(int theKey) override;  // 0x40b2e0: a printable key typed (16 letters at most), backspace, enter
	bool keyUp(int theKey) override;    // 0x40b1d0: whether the key is printable

	std::string mName;             // +0xdc
	int mScore;                    // +0xf8
	engine::TextItem* mNameText;   // +0x100
};

#endif
