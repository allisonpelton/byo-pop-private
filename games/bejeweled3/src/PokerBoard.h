// Sexy::PokerBoard (bejeweled3, vftable 0x869364, constructor 0x6da1c0 over Sexy::QuestBoard; the mode's start 0x774060 picks it when the quest's
// setting Challenge reads POKER, the secret quest 0 of secret.cfg, "Poker"): the secret mode where every match that comes from a swap deals a
// card (the gem's colour is its suit), five cards are a hand and the hand pays its value; each hand also fills a skull's clock and a skull of a
// level consumes the hands of its level and below, which the coin flip may end the game on. See docs/NOTES.md, "Butterflies and Poker", for the
// slots it changes (each function here says its slot and address) and for the places where the exe's animation curves, which the exe's
// update waits on, are replaced by counted updates.
#pragma once

#include "CurvedVal.h"
#include "QuestBoard.h"

#include <string>
#include <vector>

namespace Sexy {

// one of the five cards of the hand (the exe's 0xf8-byte record at +0x3248 + 0xf8 * index; the port keeps what 0x6cf460, 0x6c80a0 and the
// clear read)
struct PokerCard {
	enum { TYPE_NONE = -1, TYPE_NORMAL = 0, TYPE_WILD = 1, TYPE_FLAME = 2, TYPE_STAR = 3 };
	int mColor = -2;        // +0: the suit (0 to 6); 0x6c80a0 puts there the colour it chose for a card that can be two (-2: no card)
	int mSecondColor = -2;  // +4: the other suit of a card made by a swap that made two matches (-2 or less: none)
	int mType = TYPE_NONE;  // +8: -1 no card, 0 a card, 1 a wild card (a hypercube), 2 a flame gem's, 3 a star gem's
};

class PokerBoard : public QuestBoard {
public:
	enum { CARD_COUNT = 5, HAND_KINDS = 7 };
	// the curves the exe's update (0x6dedd0) waits on, from the strings of the exe's data (each says its address where it is set)
	static const char* const HAND_CURVE_FLUSH;        // 0x7d7c4c (0x6cb520): the display of a flush
	static const char* const HAND_CURVE;              // 0x7d7c70 (0x6cb520): the display of any other hand
	static const char* const FLY_CURVE;               // 0x7d7b44 (0x6dedd0): the cards leave while the eliminator takes a skull
	static const char* const FLIP_CURVE;              // 0x7d8298 (0x6cb430): the coin in the air
	static const char* const AFTER_FLIP_CURVE;        // 0x7d7c10 (0x6dedd0): the coin's result stays on show
	static const char* const BONUS_CURVE;             // 0x7d7bb0 (0x6dedd0): the wait after each flame or star card's bonus

	// the counter 0x6cb430 adds the coin flips to (73d170 with 3), and the one the flush adds to (0x6cb520: 0x20)
	enum { COUNTER_COIN_FLIPS = 3, COUNTER_FLUSHES = 0x20 };

	// the settings of secret.cfg (0x6dfa40)
	int mFlameBonus = 0;                  // +0x3a04: FlameBonus
	int mStarBonus = 0;                   // +0x3a08: StarBonus
	int mSkullMax = 0;                    // +0x3990: SkullMax, the highest level a skull reaches
	float mCountdownMultPerHand = 0.0f;   // +0x3998: CountdownMultPerHand
	std::vector<int> mHandValues;         // +0x39a8: HandValues, the points of each hand
	std::vector<float> mHandBuster;       // +0x39c0: HandBuster, what each hand adds to the eliminator's bar
	std::vector<float> mHandCountdown;    // +0x39d8: HandCountdown, what each hand adds to the skull's clock before the multiplier

	// the hand
	PokerCard mCards[CARD_COUNT];         // +0x3248
	int mCardCount = 0;                   // +0x3720
	int mFlameTag = -1;                   // +0x3aa8: the tag (+0x188) of the flame gem 0x6cab70 saw made, the next card of that gem's match is a flame card
	int mStarTag = -1;                    // +0x3aac: the same for the star gem (0x6ce1d0)
	int mBestHand = -1;                   // +0x3a30: the best hand the cards can make, less 1 (0 pair to 6 flush; -1 none yet)
	int mBonusCursor = -1;                // +0x3798: the card whose flame or star bonus was paid last in this hand
	int mHandScore = 0;                   // +0x3a2c: the points of this hand, the bonuses' too

	// the game
	int mGoalScore = 0;                   // +0x3968: the quest's first setting, Goal (a number: the score the level asks for); GetLevelMax is it in a quest and 0 in the secret mode
	int mHandsLeft = 0;                   // +0x396c: Hands, the hands a quest allows; each clear of a hand takes one and a hand past the last ends the game (0x6cdce0)
	int mHands = 0;                       // +0x3970: the hands cleared (a game's first clear, 0x6cf440, makes it 1)
	int mSkullsBusted = 0;                // +0x3974: the skulls the eliminator has removed
	int mBestHandScore = 0;               // +0x3980: the most one hand has paid before the bonuses
	float mBuster = 0.0f;                 // +0x3984: the eliminator's bar (1 removes a skull at the next clear)
	float mBusterShown = 0.0f;            // +0x3988: the bar as drawn, which follows the bar by a twentieth an update
	int mSkull = -1;                      // +0x398c: the skull's level (-1 none; it consumes the hands up to this index)
	float mSkullClock = 0.0f;             // +0x3994: the skull's clock (1 raises the skull's level by one)
	int mHandCounts[HAND_KINDS + 1] = {}; // +0x39e4: how many of each hand were made (0 the ones a skull took, 1 to 7 as the hands rank)
	bool mCoinLost = false;               // +0x3b90: the coin flip started by 0x6cb430 ends the game

	// the curves (members of the exe's board)
	mutable CurvedVal mHandCurve;         // +0x3728: the hand's display, which the scoring starts and the clear stops (0 at rest)
	mutable CurvedVal mFlyCurve;          // +0x3888: the cards' leaving with a skull eliminated (0 at rest)
	mutable CurvedVal mFlipCurve;         // +0x3ab0: the coin flip (0 at rest)
	mutable CurvedVal mAfterFlipCurve;    // +0x3b20: the result of a coin flip that was won (0 at rest)
	mutable CurvedVal mBonusCurve;        // +0x37a0: the wait between two bonuses (never set until the first flame or star card pays)

	PokerBoard();   // 0x6da1c0 (bejeweled3)

	const char* GetModeName() const override { return "Poker"; }           // slot 89 (0x6c7870): the leaderboard's and the music's name; its save is "poker.sav"
	void NewGame() override;                                               // slot 135 (0x6dfa40)
	void UpdateGame() override;                                            // slot 22 (0x6dedd0): the exe runs 0x6dd8a0 first, which the port's QuestBoard::Update (inherited) is; the rest runs here, once per step as the speed gives them
	int GetLevelMax() const override { return mIsSecretMode ? 0 : mGoalScore; }   // slot 154 (0x6c4d70)
	bool HintAllowed() const override;                                     // slot 98 (0x6c6470)
	int FindMatches() override;                                            // slot 201 (0x6d90a0): nothing is matched while a hand is being paid
	int GetGridY() const override { return 0x5a; }                         // slot 172 (0x6c4d00): +0x2c50 and 0x5a
	void OnQuestStart() override;                                          // slot 256 (0x6cf440)
	void OnSwapMatched(const SwapRecord& theSwap) override;                // slot 191 (0x6cfaa0)
	void StartHypercube(Piece* theCube, Piece* thePartner) override;       // slot 179 (0x6cfa50)
	void MakeFlameGem(Piece* thePiece) override;                           // slot 166 (0x6cab70)
	void MakeStarGem(Piece* thePiece) override;                            // slot 168 (0x6ce1d0)
	ClassicEndLevelDialog* MakeEndLevelDialog() override;                  // slot 257 (0x6d1540)
	void DrawModeOverlay(Graphics* g) override;                            // slot 43 (0x6d90f0): the hand, the skull and the bars

	// 0x6c7e20 (bejeweled3): the kind (1 pair, 2 spectrum, 3 two pair, 4 three of a kind, 5 full house, 6 four of a kind, 7 flush) five suits make
	static int RankColors(const int theColors[CARD_COUNT]);
	// 0x6c80a0 (bejeweled3): the best kind the cards can make with each card's choice of suit (a wild card is any of the seven); it also
	// settles the suit of each card
	int BestHandRank();
	const char* HandName(int theRank) const;   // the names of secret.cfg's comment: the exe's table at 0x8d222c

	// 0x6cf460 (bejeweled3): a card is dealt for a gem (with the colour of a second gem when the swap made two matches)
	void DealCard(const Piece* theFirst, const Piece* theSecond);
	// 0x6cdce0 (bejeweled3, a secret mode): the hand is cleared: the skull's clock, the eliminator and the hands counted, the cards gone
	void ClearHand();
	// 0x6cb520 (bejeweled3): the hand is scored (HandValues of its kind), 0x6c7560 with it
	void ScoreHand();
	// 0x6cb430 (bejeweled3): a skull takes the hand: the coin is flipped (the result is kept in mCoinLost)
	void FlipCoin();
	// 0x6c7560 (bejeweled3): points are added to the score and the eliminator's bar goes up for the hand
	void AddHandPoints(int thePoints);
	// 0x6dedd0 (bejeweled3): the flame and star cards of the hand pay their bonuses, one at a time (each call pays the next one; the cursor reaches 5 when none is left)
	void PayBonus();
	// the cards as a text for the test command state: each card's colours and type, ';' between cards, '-' for an empty place
	std::string DescribeCards() const;
};

}  // namespace Sexy
