// Sexy::PokerBoard: see the header. Every function says its slot and address in the exe (bejeweled3.exe).
#include "PokerBoard.h"

#include "Bej3App.h"
#include "Common.h"
#include "EndLevelDialog.h"
#include "Graphics.h"
#include "Profile.h"
#include "Res.h"
#include "X86Math.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace Sexy {

namespace {

int Scaled(int theMeasure) { return gApp->mArtRes * theMeasure / 1200; }

// the profile's statistic the first coin flip of a player's life reads (0x6cb430: the profile's +0xcc is 0 for a player who never flipped one,
// and the flip is then safe)
const int STAT_COIN_FLIPS = 3;

// where the hand is drawn (1200ths; the exe's 0x6d90f0 draws it with its own pictures: port: placed by looking)
const int CARD_X = 0x2c;
const int CARD_TOP = 0x190;

}  // namespace

// the curves of Poker, from the exe's data: the hand's display (0x7d7c4c for a flush, 0x7d7c70 for the others), the fly-away of an eliminated
// skull (0x7d7b44), the coin flip (0x7d8298), the wait after a won flip (0x7d7c10) and the throttle between two bonuses (0x7d7bb0)
const char* const PokerBoard::HAND_CURVE_FLUSH = "b+0,1,0.002222,1,####         ~~###";
const char* const PokerBoard::HAND_CURVE = "b+0,1,0.002857,1,####         ~~###";
const char* const PokerBoard::FLY_CURVE = "b30,1,0.013333,1,#         ~~";
const char* const PokerBoard::FLIP_CURVE = "b#0,1,0.006667,1,#         ~~";
const char* const PokerBoard::AFTER_FLIP_CURVE = "b#0,1,0.004,1,#         ~~";
const char* const PokerBoard::BONUS_CURVE = "b+0,1,0.05,1,#5T8     $~###    }#=Pd";

// 0x6da1c0 (bejeweled3): QuestBoard's constructor (0x6d9000), the vftable 0x869364 and the hand's fields at +0x3248 to +0x3b98
PokerBoard::PokerBoard()
{
}

// slot 135 (0x21c), 0x6dfa40 (bejeweled3): the settings of the quest are read. The first setting (the string at 0x7d74b8) goes to +0x3968 and
// the exe's slot 154 returns it for a game that is not secret; Hands to +0x396c, StartingMoney to the score (+0xd24); the others as the header
// says (HandValues as integers, the two others as floats, each from a list the exe cuts at its commas). The cards are made empty (suit -2, type
// -1), the skull and the best hand put to -1, the counts to 0, and QuestBoard's new game (0x6ddc70) follows. The first two are read for a
// quest only in the exe; the port reads them always (the secret mode's file has neither: 0, which nothing reads)
void PokerBoard::NewGame()
{
	mScorePoints = SettingInt("StartingMoney");
	mFlameBonus = SettingInt("FlameBonus");
	mStarBonus = SettingInt("StarBonus");
	mSkullMax = SettingInt("SkullMax");
	mCountdownMultPerHand = (float)SettingDouble("CountdownMultPerHand");
	mHandValues.clear();
	mHandBuster.clear();
	mHandCountdown.clear();
	for (double aValue : SettingList("HandValues"))
		mHandValues.push_back((int)aValue);
	for (double aValue : SettingList("HandBuster"))
		mHandBuster.push_back((float)aValue);
	for (double aValue : SettingList("HandCountdown"))
		mHandCountdown.push_back((float)aValue);
	for (int i = 0; i <= HAND_KINDS; i++)
		mHandCounts[i] = 0;
	for (PokerCard& aCard : mCards) {
		aCard.mColor = -2;
		aCard.mSecondColor = -2;
		aCard.mType = PokerCard::TYPE_NONE;
	}
	mCardCount = 0;
	mFlameTag = -1;
	mStarTag = -1;
	mSkull = -1;
	mBestHand = -1;
	mBonusCursor = -1;
	mHandScore = 0;
	mGoalScore = SettingInt("Goal");   // 0x6dfa40: the first setting read; a quest's (the secret mode's has none)
	mHandsLeft = SettingInt("Hands");
	mHands = 0;
	mSkullsBusted = 0;
	mBestHandScore = 0;
	mBuster = 0.0f;
	mBusterShown = 0.0f;
	mSkullClock = 0.0f;
	mCoinLost = false;
	mHandCurve.SetConstant(0.0);   // 0x6dfa40: the four curves the update waits on start at 0
	mFlyCurve.SetConstant(0.0);
	mFlipCurve.SetConstant(0.0);
	mAfterFlipCurve.SetConstant(0.0);
	QuestBoard::NewGame();   // 0x6ddc70 (0x6dfa40 ends with it)
}

// slot 256 (0x6cf440), which QuestBoard's slot 140 (0x6d0af0) runs the first time the board is at rest: the first hand
// is begun, which is a clear of the hand while no hand has been cleared yet
void PokerBoard::OnQuestStart()
{
	if (mHands == 0)
		ClearHand();
}

// slot 98 (0x188), 0x6c6470 (bejeweled3): a hint (and a match, which slot 201 asks it about) only while none of the curves 0x3888, 0x3728,
// 0x3638 (never set by Poker), 0x3ab0 and 0x3b20 is running, and then as QuestBoard's (0x6c6310, which is Board's 0x73d4f0 for a secret mode)
bool PokerBoard::HintAllowed() const
{
	if (mFlyCurve.IsDoingCurve() || mHandCurve.IsDoingCurve() || mFlipCurve.IsDoingCurve() || mAfterFlipCurve.IsDoingCurve())
		return false;
	return Board::HintAllowed();
}

// slot 201 (0x324), 0x6d90a0 (bejeweled3): the match search of Board (0x75af60) only when slot 98 allows it (a game that is not secret may have it
// by slot 217 instead)
int PokerBoard::FindMatches()
{
	if (!HintAllowed())
		return 0;
	return Board::FindMatches();
}

// 0x6c7e20 (bejeweled3): how many of each suit (0 to 6) the five hold, the suits under 0 left out; the suits met twice or more are listed in
// order: one list entry of 2 is a pair, of 3 three of a kind, of 4 four of a kind, of 5 a flush; two entries are two pair, or a full house
// when the larger is 3; any other list (none, for five different suits) is the spectrum
int PokerBoard::RankColors(const int theColors[CARD_COUNT])
{
	int aCounts[7] = {};
	for (int i = 0; i < CARD_COUNT; i++) {
		if (theColors[i] > -1 && theColors[i] < 7)
			aCounts[theColors[i]]++;
	}
	std::vector<int> aGroups;
	for (int i = 0; i < 7; i++) {
		if (aCounts[i] > 1)
			aGroups.push_back(aCounts[i]);
	}
	if (aGroups.size() == 1) {
		switch (aGroups[0]) {
		case 2: return 1;
		case 3: return 4;
		case 4: return 6;
		case 5: return 7;
		default: return 2;
		}
	}
	if (aGroups.size() == 2)
		return std::max(aGroups[0], aGroups[1]) == 2 ? 3 : 5;
	return 2;
}

// 0x6c80a0 (bejeweled3): every card has a list of suits it may be (a wild card all seven, any other its first suit and its second one when it has
// one; a place with no card is the single suit -2, which counts for nothing). The cards looked at are the ones dealt and the next place (at
// most five). The lists are walked like an odometer, the first card turning fastest, and the first walk to make a better kind than any before
// it is kept. With more than one card dealt, the walk's suits are put into the cards: a wild card takes the kept suit and a card whose second
// suit is the kept one swaps its two suits. It returns the kind (1 to 7)
int PokerBoard::BestHandRank()
{
	int aLooked = std::min(mCardCount + 1, (int)CARD_COUNT);
	std::vector<int> aChoices[CARD_COUNT];
	for (int i = 0; i < CARD_COUNT; i++) {
		const PokerCard& aCard = mCards[i];
		if (aCard.mType == PokerCard::TYPE_WILD) {
			for (int aSuit = 0; aSuit < 7; aSuit++)
				aChoices[i].push_back(aSuit);
		}
		else {
			aChoices[i].push_back(aCard.mColor);
			if (aCard.mSecondColor > -1)
				aChoices[i].push_back(aCard.mSecondColor);
		}
	}
	int aIndex[CARD_COUNT] = {};
	int aWalk[CARD_COUNT];
	int aKept[CARD_COUNT];
	for (int i = 0; i < CARD_COUNT; i++) {
		aWalk[i] = aChoices[i][0];
		aKept[i] = aWalk[i];
	}
	int aBest = -1;
	bool aDone = false;
	do {
		int aRank = RankColors(aWalk);
		if (aBest < aRank) {
			aBest = aRank;
			for (int i = 0; i < aLooked; i++)
				aKept[i] = aWalk[i];
		}
		for (int i = 0; i < aLooked; i++) {
			if (aIndex[i] < (int)aChoices[i].size() - 1) {
				aIndex[i]++;
				break;
			}
			aIndex[i] = 0;
			if (i == aLooked - 1)
				aDone = true;
		}
		for (int i = 0; i < aLooked; i++)
			aWalk[i] = aChoices[i][aIndex[i]];
	} while (!aDone);
	if (mCardCount > 1) {
		for (int i = 0; i < aLooked; i++) {
			PokerCard& aCard = mCards[i];
			if (aCard.mType == PokerCard::TYPE_WILD)
				aCard.mColor = aKept[i];
			else if (aCard.mSecondColor > -1 && aKept[i] == aCard.mSecondColor) {
				aCard.mSecondColor = aCard.mColor;
				aCard.mColor = aKept[i];
			}
		}
	}
	return aBest;
}

const char* PokerBoard::HandName(int theRank) const
{
	static const char* const aNames[HAND_KINDS + 1] = {"No Score", "Pair", "Spectrum", "2 Pair", "3 of a Kind", "Full House", "4 of a Kind", "Flush"};
	return theRank >= 0 && theRank <= HAND_KINDS ? aNames[theRank] : "";
}

// 0x6cf460 (bejeweled3): the next place of the hand takes the gem's suit (a hypercube's remembered one, and then the card is wild); a second gem
// gives the card its second suit. A card that is not wild is a flame card when its gem's tag is the one 0x6cab70 kept, a star card when it is
// the star's. The count goes up, the two kept tags go, and the best hand is looked for again. port: the card's flight (a PopAnimEffect, 0x72b040),
// its sounds and the list of suits the exe keeps at +0x1044 (read only by a text for the log, 0x6c84d0) are left out
void PokerBoard::DealCard(const Piece* theFirst, const Piece* theSecond)
{
	PokerCard& aCard = mCards[mCardCount];
	aCard.mColor = theFirst->mColor;
	if ((theFirst->mFlags & Piece::FLAG_HYPERCUBE) != 0) {
		aCard.mType = PokerCard::TYPE_WILD;
		aCard.mColor = theFirst->mRememberedColor;
	}
	if (theSecond != nullptr)
		aCard.mSecondColor = theSecond->mColor;
	if (aCard.mType != PokerCard::TYPE_WILD) {
		if (mFlameTag == theFirst->mColumnTag)
			aCard.mType = PokerCard::TYPE_FLAME;
		if (mStarTag == theFirst->mColumnTag)
			aCard.mType = PokerCard::TYPE_STAR;
	}
	mCardCount++;
	mFlameTag = -1;
	mStarTag = -1;
	mBestHand = BestHandRank() - 1;
}

// slot 191 (0x2fc), 0x6cfaa0 (bejeweled3): a swap has found a match. With a place left in the hand, a card is dealt: when the two gems of the swap
// were matched by runs with ids one apart (and neither is a hypercube) it is one card that may be either suit (the first gem's suit and the
// second's); else it is a card of the gem whose run has the larger id (the second gem when the first has no run, the second's run is the
// larger, or it is a hypercube)
void PokerBoard::OnSwapMatched(const SwapRecord& theSwap)
{
	const Piece* aFirst = theSwap.mFirst;
	const Piece* aSecond = theSwap.mSecond;
	if (aFirst == nullptr || aSecond == nullptr || mCardCount >= CARD_COUNT)
		return;
	bool aTwo = aFirst->mGroupId >= 0 && aSecond->mGroupId >= 0 && std::abs(aFirst->mGroupId - aSecond->mGroupId) == 1 &&
		(aFirst->mFlags & Piece::FLAG_HYPERCUBE) == 0 && (aSecond->mFlags & Piece::FLAG_HYPERCUBE) == 0;
	if (aTwo) {
		DealCard(aFirst, aSecond);
		return;
	}
	const Piece* aPick = aFirst;
	if (aFirst->mGroupId == -1 || aFirst->mGroupId < aSecond->mGroupId || (aSecond->mFlags & Piece::FLAG_HYPERCUBE) != 0)
		aPick = aSecond;
	DealCard(aPick, nullptr);
}

// slot 179 (0x2cc), 0x6cfa50 (bejeweled3): a hypercube is swapped with a gem: the gem's suit is dealt as a card (with a place left), then Board's
void PokerBoard::StartHypercube(Piece* theCube, Piece* thePartner)
{
	if (mCardCount < CARD_COUNT)
		DealCard(thePartner, nullptr);
	Board::StartHypercube(theCube, thePartner);
}

// slot 166 (0x298), 0x6cab70 (bejeweled3): the tag of the new flame gem is kept, then Board's 0x745ca0
void PokerBoard::MakeFlameGem(Piece* thePiece)
{
	mFlameTag = thePiece->mColumnTag;
	Board::MakeFlameGem(thePiece);
}

// slot 168 (0x2a0), 0x6ce1d0 (bejeweled3): the same for the star gem (Board's 0x7494d0)
void PokerBoard::MakeStarGem(Piece* thePiece)
{
	mStarTag = thePiece->mColumnTag;
	Board::MakeStarGem(thePiece);
}

// 0x6c7560 (bejeweled3): the score and the counter of points go up by the hand's value; with a skull on the board (and a hand) the eliminator's bar
// goes up by HandBuster of the hand, at most to 1; the most one hand paid is kept; the hand's points are kept for the bonuses to add to
void PokerBoard::AddHandPoints(int thePoints)
{
	mScorePoints += thePoints;
	mCounters[COUNTER_POINTS] += thePoints;
	if (mBestHand > -1 && mSkull > -1 && mBestHand < (int)mHandBuster.size()) {
		float aSum = mHandBuster[mBestHand] + mBuster;
		mBuster = aSum <= 1.0f ? aSum : 1.0f;
	}
	if (mBestHandScore < thePoints)
		mBestHandScore = thePoints;
	mHandScore = thePoints < 1 ? 0 : thePoints;
}

// 0x6cb520 (bejeweled3): the hand is looked at again and scored: HandValues of its kind (a flush also counts in counter 0x20), the count of
// its kind goes up. port: the name's text and the sounds (0x56f730) are not made
void PokerBoard::ScoreHand()
{
	int aRank = BestHandRank();
	mHandCurve.SetCurve(aRank == 7 ? HAND_CURVE_FLUSH : HAND_CURVE);   // 450 updates for a flush, 350 for the others
	if (aRank == 7)
		AddCounter(COUNTER_FLUSHES, 1, -1);
	int aPoints = 0;
	if (aRank > 0 && aRank - 1 < (int)mHandValues.size())
		aPoints = mHandValues[aRank - 1];
	AddHandPoints(aPoints);
	mHandCounts[aRank]++;
}

// 0x6cb430 (bejeweled3): a skull takes the hand. The coin is lost (the game ends when its curve is over) for a player whose profile has flipped
// coins before, when this is the third hand a skull has taken (+0x39e4 is 2), and else by a draw of the app's Rand() modulo the count of hands
// a skull has taken before, plus 2, which has to come out above 0. Counter 3 goes up by one and so does the count
void PokerBoard::FlipCoin()
{
	Profile* aProfile = gApp->mProfile;
	if (aProfile == nullptr || aProfile->mStats[STAT_COIN_FLIPS] == 0)
		mCoinLost = false;
	else if (mHandCounts[0] == 2)
		mCoinLost = true;
	else
		mCoinLost = (unsigned int)Rand() % (unsigned int)(mHandCounts[0] + 2) > 0;
	mFlipCurve.SetCurve(FLIP_CURVE);
	mFlipCurve.SetMode(0);
	AddCounter(COUNTER_COIN_FLIPS, 1, -1);
	if (aProfile != nullptr)
		aProfile->mStats[STAT_COIN_FLIPS]++;
	mHandCounts[0]++;
}

// 0x6cdce0 (bejeweled3, a secret mode): the hand is cleared. With a best hand the skull's clock goes up by that hand's HandCountdown times
// (the hands cleared times CountdownMultPerHand, plus 1). With the eliminator's bar short of 1, a clock at 1 raises the skull's level (to
// SkullMax) and starts the clock again; with the bar at 1 and a skull on the board, the skull's level goes down by one, a skull is counted as
// busted and the clock and the bar start again. The hands cleared go up by one, and the cards, the best hand, the bonus cursor and the hand's
// points go. A quest (the secret flag clear) does only the first part of its own: see below. port: the effects, the sounds and the cards' flight are left out
void PokerBoard::ClearHand()
{
	if (!mIsSecretMode) {
		// 0x6cdce0, the quest's path: the progress is the score; a score at the level's size ends here with the cards left (the level is full); else a hand is used up and
		// one past the last ends the game (slot 161, NoMoreMoves) with the cards left too
		mLevelProgress = mScorePoints;
		if (GetLevelMax() <= mScorePoints)
			return;
		mHandsLeft--;
		if (mHandsLeft < 0) {
			NoMoreMoves(true);
			return;
		}
	}
	if (mIsSecretMode) {
		if (mBestHand > -1 && mBestHand < (int)mHandCountdown.size())
			mSkullClock = ((float)mHands * mCountdownMultPerHand + 1.0f) * mHandCountdown[mBestHand] + mSkullClock;
		if (mBuster < 1.0f) {
			if (mSkullClock >= 1.0f) {
				mSkull = std::min(mSkull + 1, mSkullMax);
				mSkullClock = 0.0f;
			}
		}
		else if (mSkull > -1) {
			mSkull = std::max(mSkull - 1, -1);
			mSkullsBusted++;
			mSkullClock = 0.0f;
			mBuster = 0.0f;
			mBusterShown = 0.0f;
			mFlyCurve.SetConstant(0.0);
		}
		mHands++;
	}
	mCardCount = 0;
	mBestHand = -1;
	mBonusCursor = -1;
	mHandScore = 0;
	for (PokerCard& aCard : mCards) {
		aCard.mType = PokerCard::TYPE_NORMAL;
		aCard.mColor = -2;
		aCard.mSecondColor = -2;
	}
	mHandCurve.SetConstant(0.0);
}

// 0x6dedd0 (bejeweled3): while the hand's display is past 0.35, with a card left and no throttle running, the cursor goes on to the next card
// that is not a normal or a wild one; that starts the throttle (BONUS_CURVE), and a flame or star card pays its bonus (FlameBonus, StarBonus), which
// goes straight to the score, the level bar's number and the counter of points, and into the hand's points; a card of another type pays nothing
void PokerBoard::PayBonus()
{
	if (!(mHandCurve.GetOutVal() > 0.35) || mBonusCursor >= CARD_COUNT || mBonusCurve.IsDoingCurve())
		return;
	mBonusCursor++;
	while (mBonusCursor < CARD_COUNT) {
		int aType = mCards[mBonusCursor].mType;
		if (aType != PokerCard::TYPE_NORMAL && aType != PokerCard::TYPE_WILD) {
			mBonusCurve.SetCurve(BONUS_CURVE);
			int aBonus;
			if (aType == PokerCard::TYPE_FLAME)
				aBonus = mFlameBonus;
			else if (aType == PokerCard::TYPE_STAR)
				aBonus = mStarBonus;
			else
				break;
			mScorePoints += aBonus;
			mLevelProgress += aBonus;
			mCounters[COUNTER_POINTS] += aBonus;
			mHandScore += aBonus;
			break;
		}
		mBonusCursor++;
	}
}

// slot 22 (0x58), 0x6dedd0 (bejeweled3): Board's step first. The curves of the throttle, of the wait after a won flip and of the flip are stepped
// (the others follow the app's updates by themselves). With the fly-away curve at 1 and the bar caught up, the hand is cleared. With five cards
// and the game running: with the hand's curve at rest at 0 the hand is scored (starting the curve); with it at rest at 1 and both flip curves at 0,
// a hand at or under the skull's level has its coin flipped, and a hand over it is cleared at once when the bar (once caught up) is short of 1 and
// else starts the fly-away. While the hand's curve runs it is stepped and the bonuses are paid. The end of the wait after a won flip clears the
// hand (the skull's clock put back to 0 first); the end of the flip itself ends the game (slot 161) when it is lost and else starts that wait. The
// eliminator's bar as drawn goes a twentieth of the way to the bar while slot 98 allows. port: the card list at +0x26b8 is taken as empty
void PokerBoard::UpdateGame()
{
	QuestBoard::UpdateGame();
	if (mLeaving)
		return;
	mBonusCurve.IncInVal();
	mAfterFlipCurve.IncInVal();
	mFlipCurve.IncInVal();
	if (mFlyCurve.GetOutVal() == 1.0 && mBusterShown == mBuster && 0.0f < mBuster)
		ClearHand();
	if (mCardCount == CARD_COUNT && mGameOverTicks == 0) {
		if (!mHandCurve.IsDoingCurve()) {
			if (mHandCurve.GetOutVal() == 0.0 && mQuestClock == 0)
				ScoreHand();
			if (mHandCurve.GetOutVal() == 1.0 && mFlipCurve.GetOutVal() == 0.0 && mAfterFlipCurve.GetOutVal() == 0.0) {
				if (mSkull >= mBestHand)
					FlipCoin();
				else if (mBusterShown == mBuster) {
					if (mBuster < 1.0f)
						ClearHand();
					else if (mFlyCurve.GetOutVal() == 0.0)
						mFlyCurve.SetCurve(FLY_CURVE);
				}
			}
		}
	}
	if (mHandCurve.IsDoingCurve()) {
		mHandCurve.IncInVal();
		PayBonus();
	}
	if (mAfterFlipCurve.GetOutVal() == 1.0) {
		mFlipCurve.SetConstant(0.0);
		mAfterFlipCurve.SetConstant(0.0);
		mSkullClock = 0.0f;
		ClearHand();
	}
	if (mFlipCurve.GetOutVal() == 1.0 && mGameOverTicks == 0) {
		if (mCoinLost)
			NoMoreMoves(true);
		else if (!mAfterFlipCurve.IsDoingCurve()) {
			mAfterFlipCurve.SetCurve(AFTER_FLIP_CURVE);
			mAfterFlipCurve.SetMode(0);
		}
	}
	if (HintAllowed()) {
		mBusterShown = (mBuster - mBusterShown) / 20.0f + mBusterShown;
		if (std::fabs(mBuster - mBusterShown) < 0.01f)
			mBusterShown = mBuster;
	}
}

// slot 257 (0x404), 0x6d1540 (bejeweled3): the end dialog of the mode (0x782cf0, vftable 0x86c4c4, the leaderboard of the name "Poker")
ClassicEndLevelDialog* PokerBoard::MakeEndLevelDialog()
{
	return new PokerEndLevelDialog(this);
}

std::string PokerBoard::DescribeCards() const
{
	std::string aText;
	for (int i = 0; i < CARD_COUNT; i++) {
		if (i > 0)
			aText += ';';
		const PokerCard& aCard = mCards[i];
		if (aCard.mColor < 0)
			aText += '-';
		else {
			aText += (char)('0' + aCard.mColor);
			if (aCard.mSecondColor > -1) {
				aText += '/';
				aText += (char)('0' + aCard.mSecondColor);
			}
			static const char aTypes[4] = {'n', 'w', 'f', 's'};
			if (aCard.mType >= 0 && aCard.mType < 4 && aCard.mType != PokerCard::TYPE_NORMAL)
				aText += aTypes[aCard.mType];
		}
	}
	return aText;
}

// slot 43 (0x2c), 0x6d90f0 (bejeweled3): the screen's own pass. port: the five cards (the picture of a card, its suit's face, wild and flame or star
// marks), the name of the hand and the skull's and the eliminator's numbers are drawn with the game's own pictures and fonts but are placed by
// looking, not from the exe's positions; the flights of the cards and the skull's pictures are not translated
void PokerBoard::DrawModeOverlay(Graphics* g)
{
	const int aAlphaByte = Ftol(mCoverCurve.GetOutVal() * 255.0);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, aAlphaByte));
	// the cards are drawn at four fifths of their size so that five fit in the strip left of the grid
	int aCardWidth = IMAGE_CARDS_FRONT != nullptr ? IMAGE_CARDS_FRONT->mWidth * 4 / 5 : Scaled(0x60);
	int aCardHeight = IMAGE_CARDS_FRONT != nullptr ? IMAGE_CARDS_FRONT->mHeight * 4 / 5 : Scaled(0x70);
	int aStep = aCardHeight + Scaled(4);
	for (int i = 0; i < CARD_COUNT; i++) {
		const PokerCard& aCard = mCards[i];
		Rect aDest(Scaled(CARD_X), Scaled(CARD_TOP) + i * aStep, aCardWidth, aCardHeight);
		if (aCard.mColor < 0) {
			if (IMAGE_CARDS_BACK != nullptr)
				g->DrawImage(IMAGE_CARDS_BACK, aDest.mX, aDest.mY, aDest.mWidth, aDest.mHeight);
			continue;
		}
		if (IMAGE_CARDS_FRONT != nullptr)
			g->DrawImage(IMAGE_CARDS_FRONT, aDest.mX, aDest.mY, aDest.mWidth, aDest.mHeight);
		if (aCard.mType == PokerCard::TYPE_WILD && IMAGE_CARDS_WILD != nullptr)
			g->DrawImage(IMAGE_CARDS_WILD, aDest.mX, aDest.mY, aDest.mWidth, aDest.mHeight);
		else if (IMAGE_CARDS_FACE != nullptr && aCard.mColor < 7)
			g->DrawImageCel(IMAGE_CARDS_FACE, aDest, aCard.mColor);
		if (aCard.mType == PokerCard::TYPE_FLAME || aCard.mType == PokerCard::TYPE_STAR) {
			g->SetFont(FONT_CARD_COUNT_SMALL);
			g->WriteString(aCard.mType == PokerCard::TYPE_FLAME ? "F" : "S", aDest.mX + aCardWidth / 2, aDest.mY + aCardHeight / 2, -1, 0, true, 0, -1, -1);
		}
	}
	g->SetFont(FONT_CARD_COUNT_SMALL);
	if (mCardCount > 0)
		g->WriteString(HandName(mBestHand + 1), Scaled(0xbb), Scaled(0x11c), -1, 0, true, 0, -1, -1);
	char aLine[64];
	snprintf(aLine, sizeof(aLine), "Skull %d  Bar %d%%", mSkull + 1, Ftol(mBusterShown * 100.0f));
	g->WriteString(aLine, Scaled(0xbb), Scaled(0x178), -1, 0, true, 0, -1, -1);
}

}  // namespace Sexy
