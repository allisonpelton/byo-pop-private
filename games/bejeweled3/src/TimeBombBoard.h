// Sexy::TimeBombBoard (bejeweled3, vftable 0x869dc4, constructor 0x6dd440, 0x3268 bytes) over Sexy::QuestBoard: the Quest boards of the Challenges "Time Bomb" and "Time Bomb
// (Real time)" (the constructor's argument is 1 for the second: the flag at +0x3262). Bombs drop in among the new gems; a bomb counts down (the moves that find a match,
// or in real time one a second) and when it is out the game is lost; matching a bomb defuses it. The exe's RealTimeBombBoard (vftable 0x869954) is never made: 0x774060
// makes this class for both names. See docs/NOTES.md, "Quest".
#pragma once

#include "QuestBoard.h"

namespace Sexy {

class TimeBombBoard : public QuestBoard {
public:
	// 0x73ccc0 (MakeBomb): the bomb's flags: 0x200 with 0x20 for a move bomb, with 0x40 for a real-time one (0x100 is the goal Clear Bombs' kind: not made)
	enum { FLAG_BOMB = 0x200, FLAG_MOVE_BOMB = 0x20, FLAG_REAL_TIME_BOMB = 0x40, FLAGS_BOMB_KIND = 0x60, FLAG_GOAL_BOMB = 0x100 };
	// 0x73ccc0: the bomb's special countdown (+0x184); 0x767e20: where the shake has run its course (20), the window in which it takes a move off the bomb (above 5 up to 10)
	// and where it goes on from (16); 0x73c710 starts the shake at phase 1 with the speed 0x3ea8f5c3 (about 0.33)
	enum { BOMB_SPECIAL_COUNTDOWN = 25, SHAKE_END = 20, SHAKE_COUNT_FROM = 5, SHAKE_COUNT_TO = 10, SHAKE_AFTER_COUNT = 16 };
	// 0x6c60b0: a bomb is made in a fill by drawing a new gem up to this many times; it has to be above this row and, with no score yet, between these columns
	enum { BOMB_PICK_TRIES = 100, BOMB_MAX_ROW = 5, BOMB_CENTRE_FIRST_COLUMN = 2, BOMB_CENTRE_LAST_COLUMN = 5 };

	int mBombCountdown = 0;     // +0x3248: "BombCountdown": the moves (or seconds) a new bomb has
	int mDropCountdown = 0;     // +0x324c: "DropCountdown": the moves between the dropping of two bombs
	int mMaxBombs = 0;          // +0x3250: "MaxBombs": no more bombs than this are on the board
	bool mStarted = false;      // +0x3254: the quest's play has begun (slot 256, 0x6c4a40): bombs count down only after it
	int mMovesUntilDrop = 0;    // +0x3258: the moves left before the next bomb
	int mBombCount = 0;         // +0x325c: the bombs on the board
	bool mDropPending = false;  // +0x3261: a bomb was made in the fill that is going on
	bool mRealTime = false;     // +0x3262: "Time Bomb (Real time)" (the constructor's argument)

	void NewGame() override;                                           // slot 135 (0x6e52e0)
	const char* GetModeName() const override { return "QuestBomb"; }   // slot 89
	void OnPieceCleared(Piece* thePiece) override;                     // slot 143 (0x6d0930)
	void OnSwapMatched(const SwapRecord& theSwap) override;            // slot 191 (0x6c61c0)
	void OnFillDone(const std::vector<Piece*>& theNewPieces) override; // slot 204 (0x6c53d0)
	bool OnFillAccepted(std::vector<Piece*>& theNewPieces) override;   // slot 185 (0x6c60b0)
	void CountDownPiece(Piece* thePiece, bool theTick) override;       // slot 212 (0x6c6090, over 0x73c710)
	void OnQuestStart() override { mStarted = true; }                  // slot 256 (0x6c4a40)
	void UpdateWarnings() override;                                    // 0x767e20's loop over the pieces
	void DrawModeOverlay(Graphics* g) override;                        // the bombs' counts and the drop's countdown

	// 0x73ccc0 (bejeweled3): the piece becomes a bomb with the given countdown
	void MakeBomb(Piece* thePiece, bool theRealTime, int theCountdown);
};

}  // namespace Sexy
