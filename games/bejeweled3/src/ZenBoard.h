// Sexy::ZenBoard (bejeweled3, vftable 0x866b6c, 0x36d8 bytes, made by 0x7126f0 from the mode's start 0x773100): the game screen of Zen
// mode over Sexy::Board. The exe's ZenBoard derives from Board directly (it has no slot of ClassicBoard's), and changes 33 slots of Board; the ones
// that a game of the port reads are overridden here, each saying its slot and address. See docs/NOTES.md, "Zen and Lightning".
// port: what ZenBoard adds for the player's comfort (the mantras, the breathing exercise, the ambient sound, its own HUD curves: the
// state at +0x3170 to +0x33c0 and the pause at +0x33b0, which 0x712c60 and 0x7116c0 read) is presentation only and is not translated
#pragma once

#include "Board.h"

namespace Sexy {

class ZenBoard : public Board {
public:
	ZenBoard();   // 0x7126f0 (bejeweled3): the board's "Title" setting is "Zen" (and the board's +0x3170 and following, the mantras' state, are cleared)

	const char* GetModeName() const override { return "Zen"; }   // slot 89 (0x164): the Title; the song "Zen" is played
	// slot 113 (0x1c4), 0x70e3a0: true. The match timers, explosions and sparks take the slow branch
	bool IsSlowMode() const override { return true; }
	// slot 122 (0x1e8), 0x70e330: 1 (Classic's is the level index + 1)
	float GetLevelFactor() const override { return 1.0f; }
	// slot 154 (0x268), 0x70e450: the level index counts to 30 (0x1d < index gives 0x1e), 0x2ee more points each, from 0x9c4
	int GetLevelMax() const override;
	// slot 161 (0x284), 0x70e4a0: no game over. With no move left, the board's own generator is drawn twice (the column, then the row,
	// each & 7) and the gem there, if any, becomes a hypercube (slot 167, 0x749570, with its effect)
	void NoMoreMoves(bool theAnnounce) override;

	// slot 249 (0x3e4), 0x70fdf0: the necklace's score plate in place of Classic's top widget with the level number (the necklace itself is not drawn)
	void DrawTopWidget(Graphics* g) override;
	// slot 255 (0x3fc), 0x70e5c0: the score in the middle of the panel, "MAX SCORE" once it is at the limit
	void DrawScoreText(Graphics* g) override;
	// slot 251 (0x3ec), 0x70e500: the Zen bottom widget
	void DrawBottomWidget(Graphics* g) override;
};

}  // namespace Sexy
