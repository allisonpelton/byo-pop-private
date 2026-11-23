// The standard test commands (docs/TEST_COMMANDS.md) of Bejeweled 3, handled by the app's HandleScriptCommand. A command
// that is never given changes nothing. Each answers on SDL's log with one line: STATE {...} for state, OK <command> when
// it has done its work, ERROR <command>: <reason> when it cannot.
#include "Bej3App.h"

#include "Bej3Dialog.h"
#include "ButterflyBoard.h"
#include "IceStormBoard.h"
#include "PokerBoard.h"
#include "Board.h"
#include "Common.h"
#include "LightningBoard.h"
#include "MainMenu.h"
#include "PlayMenu.h"
#include "Profile.h"

#include <SDL.h>

#include <cstdlib>
#include <cstring>
#include <string>

using namespace Sexy;

namespace {

// the mode's key in the state line: the board's mode name in lower case, Lightning's (the exe's "Speed") by the name the player sees
const char* ModeKey(const Board* theBoard)
{
	const std::string aName = theBoard->GetModeName();
	if (aName == "Zen")
		return "zen";
	if (aName == "Speed")
		return "lightning";
	if (aName == "Butterflies")
		return "butterflies";
	if (aName == "Poker")
		return "poker";
	if (aName == "Icestorm")
		return "icestorm";
	return "classic";
}

void Reply(const std::string& theLine)
{
	SDL_Log("%s", theLine.c_str());
	fflush(stdout);
	fflush(stderr);
}

void ReplyOK(const ScriptCommand& theCommand)
{
	Reply("OK " + theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs));
}

void ReplyError(const ScriptCommand& theCommand, const std::string& theReason)
{
	Reply("ERROR " + theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs) + ": " + theReason);
}

// A gem's colour as a letter: red, white, green, yellow, purple, orange, blue (the order of IMAGE_GEMS_*); . for none
char ColorLetter(const Piece* thePiece)
{
	static const char aLetters[Piece::COLOR_COUNT + 1] = {'R', 'W', 'G', 'Y', 'P', 'O', 'B', '?'};
	if (thePiece == nullptr || thePiece->mColor < 0)
		return '.';
	return aLetters[thePiece->mColor < Piece::COLOR_COUNT ? thePiece->mColor : Piece::COLOR_COUNT];
}

// What a gem is, as a letter: f flame, c hypercube, s star, x star and flame, . plain (Piece::FLAG_*)
char SpecialLetter(const Piece* thePiece)
{
	if (thePiece == nullptr)
		return '.';
	bool aFlame = (thePiece->mFlags & Piece::FLAG_FLAME) != 0;
	bool aStar = (thePiece->mFlags & Piece::FLAG_STAR) != 0;
	if (aFlame && aStar)
		return 'x';
	if (aFlame)
		return 'f';
	if (aStar)
		return 's';
	if ((thePiece->mFlags & Piece::FLAG_HYPERCUBE) != 0)
		return 'c';
	if ((thePiece->mFlags & Piece::FLAG_BUTTERFLY) != 0)
		return 'b';
	return '.';
}

// The colour a letter names (see ColorLetter), or -1
int LetterColor(char theLetter)
{
	static const char aLetters[] = "RWGYPOB";
	for (int aColor = 0; aColor < Piece::COLOR_COUNT; aColor++) {
		if (aLetters[aColor] == theLetter)
			return aColor;
	}
	return -1;
}

}  // namespace

bool Bej3App::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aCommand = theCommand.mCommand;
	if (aCommand == "seed") {
		// the app's Rand() is what seeds each board (Board::Board), so a seed given before a game starts fixes its first deal
		if (theCommand.mArgs.empty()) {
			ReplyError(theCommand, "a number is needed");
			return true;
		}
		SRand((ulong)strtoul(theCommand.mArgs.c_str(), nullptr, 10));
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "name") {
		// the first player's name, as typed into the welcome dialog (a folder with a player already has none: nothing happens)
		if (theCommand.mArgs.empty()) {
			ReplyError(theCommand, "a name is needed");
			return true;
		}
		if (mProfile != nullptr && mProfile->mName.empty()) {
			mScriptName = theCommand.mArgs;
			EnterScriptName();
		}
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "unlock") {
		// unlock all: every mode the play menu keeps shut is opened through the profile's own flags (+0x180: the four modes that start locked, Poker,
		// Butterflies, Ice Storm and Diamond Mine, by the ball's number less 4)
		if (theCommand.mArgs != "all" || mProfile == nullptr) {
			ReplyError(theCommand, "unlock all, with a player");
			return true;
		}
		for (int i = 0; i < 4; i++)
			mProfile->mTutorialFlags[i] = 1;
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "goto") {
		if (theCommand.mArgs == "butterflies" || theCommand.mArgs == "poker" || theCommand.mArgs == "icestorm") {
			// as the play menu starts it: the ball of the mode must be open (unlock all), else the exe asks why it is shut instead
			int aIndex = theCommand.mArgs == "poker" ? 0 : theCommand.mArgs == "icestorm" ? 2 : 1;
			if (mProfile == nullptr || mProfile->mTutorialFlags[aIndex] == 0) {
				ReplyError(theCommand, "that mode is locked: unlock all first");
				return true;
			}
			if (mMainMenu != nullptr && mMainMenu->mPlayMenu != nullptr && !mMainMenu->mPlayMenu->IsActive())
				mMainMenu->OpenPlayMenu();
			if (!StartSecretMode(aIndex)) {
				ReplyError(theCommand, "that mode is not made yet");
				return true;
			}
			ReplyOK(theCommand);
			return true;
		}
		if (theCommand.mArgs == "classic" || theCommand.mArgs == "zen" || theCommand.mArgs == "lightning") {
			// the game is reached as from the screens: PLAY is pressed (the play menu opens) and then the mode's ball
			if (mMainMenu != nullptr && mMainMenu->mPlayMenu != nullptr && !mMainMenu->mPlayMenu->IsActive())
				mMainMenu->OpenPlayMenu();
			if (theCommand.mArgs == "zen")
				StartZen();
			else if (theCommand.mArgs == "lightning")
				StartLightning();
			else
				StartClassic();
			ReplyOK(theCommand);
		}
		else
			ReplyError(theCommand, "only classic, zen, lightning, butterflies, poker and icestorm are made yet");
		return true;
	}
	if (aCommand == "swap") {
		// swap <col> <row> <up|down|left|right>: asks for the move the player would make, and prints the board and score
		int aCol = -1;
		int aRow = -1;
		char aDir[16] = "";
		if (mBoard == nullptr || sscanf(theCommand.mArgs.c_str(), "%d %d %15s", &aCol, &aRow, aDir) != 3 || aCol < 0 ||
			aCol >= Board::COLS || aRow < 0 || aRow >= Board::ROWS) {
			ReplyError(theCommand, "swap <col> <row> <up|down|left|right> on a board");
			return true;
		}
		int aToRow = aRow;
		int aToCol = aCol;
		std::string aWay = aDir;
		if (aWay == "up")
			aToRow--;
		else if (aWay == "down")
			aToRow++;
		else if (aWay == "left")
			aToCol--;
		else if (aWay == "right")
			aToCol++;
		if (aToRow < 0 || aToRow >= Board::ROWS || aToCol < 0 || aToCol >= Board::COLS || mBoard->mGrid[aRow][aCol] == nullptr ||
			!mBoard->TrySwap(mBoard->mGrid[aRow][aCol], aToRow, aToCol)) {
			ReplyError(theCommand, "that swap is not allowed now");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "board") {
		// board <8 words of 8 colour letters, R W G Y P O B, top row first>: puts those colours on the gems of a board that has
		// settled, takes the specials off them and leaves the rest alone (the way a test makes a run). A test command of its own
		char aRows[Board::ROWS][32];
		bool aOk = mBoard != nullptr;
		if (aOk) {
			int aRead = sscanf(theCommand.mArgs.c_str(), "%31s %31s %31s %31s %31s %31s %31s %31s", aRows[0], aRows[1], aRows[2], aRows[3],
				aRows[4], aRows[5], aRows[6], aRows[7]);
			aOk = aRead == Board::ROWS;
			for (int aRow = 0; aOk && aRow < Board::ROWS; aRow++) {
				aOk = strlen(aRows[aRow]) == (size_t)Board::COLS;
				for (int aCol = 0; aOk && aCol < Board::COLS; aCol++)
					aOk = LetterColor(aRows[aRow][aCol]) >= 0 && mBoard->mGrid[aRow][aCol] != nullptr;
			}
		}
		if (!aOk) {
			ReplyError(theCommand, "board <8 words of 8 colour letters RWGYPOB> on a full board");
			return true;
		}
		for (int aRow = 0; aRow < Board::ROWS; aRow++) {
			for (int aCol = 0; aCol < Board::COLS; aCol++) {
				Piece* aPiece = mBoard->mGrid[aRow][aCol];
				aPiece->mColor = LetterColor(aRows[aRow][aCol]);
				aPiece->mFlags = 0;
				aPiece->mSpecialCountdown = 0;
			}
		}
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "put") {
		// put <col> <row> <letter> [flame|cube|star]: gives the gem at that cell a colour and, when named, makes it that special
		// through the same makers a run uses, then lets it go off at once (its countdown, +0x184, is put to 0); a fifth word "hold" leaves the
		// countdown as the maker set it, so the special stays until a match or a swap uses it
		int aCol = -1;
		int aRow = -1;
		char aLetter[8] = "";
		char aKind[16] = "";
		char aHold[16] = "";
		int aRead = mBoard == nullptr ? 0 : sscanf(theCommand.mArgs.c_str(), "%d %d %7s %15s %15s", &aCol, &aRow, aLetter, aKind, aHold);
		if (aRead < 3 || aCol < 0 || aCol >= Board::COLS || aRow < 0 || aRow >= Board::ROWS || mBoard->mGrid[aRow][aCol] == nullptr ||
			LetterColor(aLetter[0]) < 0) {
			ReplyError(theCommand, "put <col> <row> <RWGYPOB> [flame|cube|star] on a board");
			return true;
		}
		Piece* aPiece = mBoard->mGrid[aRow][aCol];
		aPiece->mColor = LetterColor(aLetter[0]);
		aPiece->mFlags = 0;
		std::string aWhat = aKind;
		if (aWhat == "flame")
			mBoard->MakeFlameGem(aPiece);
		else if (aWhat == "star")
			mBoard->MakeStarGem(aPiece);
		else if (aWhat == "cube")
			mBoard->MakeHypercube(aPiece, true);
		else if (!aWhat.empty()) {
			ReplyError(theCommand, "the kind is flame, cube or star");
			return true;
		}
		if (strcmp(aHold, "hold") != 0)
			aPiece->mSpecialCountdown = 0;
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "move") {
		// move: the bot's move, the first the exe's move search (0x75a8e0, rows from the top) finds, as the two clicks of the player
		// (the gem is picked up, then its neighbour is clicked); it changes nothing. MOVE null when no move is left or the board is not at rest
		BoardMove aMove;
		if (mBoard == nullptr || mBoard->mGameOver || !mBoard->IsSettled() || !mBoard->FindMove(&aMove, 0, false, nullptr, false)) {
			Reply("MOVE null");
			return true;
		}
		const int aCell = 128;
		auto aPixel = [&](int theMeasure) { return mArtRes * theMeasure / 1200; };
		int aFromX = aPixel(mBoard->GetGridX() + aMove.mFromCol * aCell + aCell / 2);
		int aFromY = aPixel(mBoard->GetGridY() + aMove.mFromRow * aCell + aCell / 2);
		int aToX = aPixel(mBoard->GetGridX() + aMove.mToCol * aCell + aCell / 2);
		int aToY = aPixel(mBoard->GetGridY() + aMove.mToRow * aCell + aCell / 2);
		Reply("MOVE {\"swap\":[" + std::to_string(aMove.mFromCol) + "," + std::to_string(aMove.mFromRow) + "," + std::to_string(aMove.mToCol) +
			"," + std::to_string(aMove.mToRow) + "],\"input\":[{\"click\":[" + std::to_string(aFromX) + "," + std::to_string(aFromY) +
			"]},{\"click\":[" + std::to_string(aToX) + "," + std::to_string(aToY) + "]}]}");
		return true;
	}
	if (aCommand == "bot") {
		// bot [n]: from now on the board plays the bot's move each time it is at rest, n moves (no n: until the game is over)
		if (mBoard == nullptr) {
			ReplyError(theCommand, "bot needs a board");
			return true;
		}
		mBoard->mBotMoves = theCommand.mArgs.empty() ? -1 : atoi(theCommand.mArgs.c_str());
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "hint") {
		// hint: the hint button as the player presses it (draws the app's Rand() twice, as the exe's search does)
		if (mBoard == nullptr || !mBoard->HintAllowed()) {
			ReplyError(theCommand, "no hint now");
			return true;
		}
		mBoard->ShowHint(true);
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "set") {
		// set <progress|points|index> <n>: the level bar's progress, the score and the level's index, to reach the states quickly
		char aName[16] = "";
		int aValue = 0;
		if (mBoard == nullptr || sscanf(theCommand.mArgs.c_str(), "%15s %d", aName, &aValue) != 2) {
			ReplyError(theCommand, "set <progress|points|index> <n> on a board");
			return true;
		}
		std::string aWhat = aName;
		if (aWhat == "progress")
			mBoard->mLevelProgress = aValue;
		else if (aWhat == "points")
			mBoard->mScorePoints = aValue;
		else if (aWhat == "index")
			mBoard->mLevelIndex = aValue;
		else {
			ReplyError(theCommand, "set <progress|points|index> <n>");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "state") {
		std::string aLine = "STATE {\"screen\":\"";
		aLine += mBoard != nullptr ? ModeKey(mBoard) : "menu";
		aLine += "\"";
		if (mBoard != nullptr) {
			aLine += std::string(",\"mode\":\"") + ModeKey(mBoard) + "\",\"score\":" + std::to_string(mBoard->mScore) + ",\"points\":" + std::to_string(mBoard->mScorePoints) +
				",\"movematches\":" + std::to_string(mBoard->mMoveRecords.empty() ? 0 : mBoard->mMoveRecords.back().mCounters[COUNTER_MATCHES]) +
				",\"matches\":" + std::to_string(mBoard->mMatchGroups.size()) + ",\"swaps\":" + std::to_string(mBoard->mSwaps.size()) +
				",\"board\":[";
			for (int aRow = 0; aRow < Board::ROWS; aRow++) {
				aLine += aRow == 0 ? "\"" : ",\"";
				for (int aCol = 0; aCol < Board::COLS; aCol++)
					aLine += ColorLetter(mBoard->mGrid[aRow][aCol]);
				aLine += "\"";
			}
			aLine += "],\"specials\":[";
			for (int aRow = 0; aRow < Board::ROWS; aRow++) {
				aLine += aRow == 0 ? "\"" : ",\"";
				for (int aCol = 0; aCol < Board::COLS; aCol++)
					aLine += SpecialLetter(mBoard->mGrid[aRow][aCol]);
				aLine += "\"";
			}
			aLine += "],\"matchcounter\":" + std::to_string(mBoard->mMatchCounter) + ",\"pieces\":" + std::to_string(mBoard->mPieceCounter);
			aLine += ",\"level\":" + std::to_string(mBoard->mLevelIndex + 1) + ",\"levelnumber\":" + std::to_string(mBoard->mLevelNumber) +
				",\"progress\":" + std::to_string(mBoard->mLevelProgress) + ",\"levelmax\":" + std::to_string(mBoard->GetLevelMax()) +
				",\"bar\":" + std::to_string((int)(mBoard->mLevelBar * 10000.0f)) + ",\"levelchange\":" +
				std::to_string(mBoard->mHyperspace != nullptr ? mBoard->mHyperspace->mAge : (mBoard->mLevelUpPending ? -1 : 0)) +
				",\"gameover\":" + std::to_string(mBoard->mGameOver ? 1 : 0) + ",\"gameoverticks\":" + std::to_string(mBoard->mGameOverTicks) +
				",\"endscreen\":" + std::to_string(mBoard->mEndScreenShown ? 1 : 0) + ",\"hinttimer\":" + std::to_string(mBoard->mHintTimer) +
				",\"botmoves\":" + std::to_string(mBoard->mBotMoveCount) + ",\"highscores\":[";
			for (size_t aIndex = 0; aIndex < mBoard->mHighScores.size(); aIndex++)
				aLine += (aIndex == 0 ? "" : ",") + std::to_string(mBoard->mHighScores[aIndex]);
			aLine += "]";
			aLine += ",\"background\":" + std::to_string(mBoard->mBackgroundIndex);
			// how far the fall has come: the update count, the pieces not at rest, the fill's landing estimate and the
			// sums of the pieces' y and speeds (as whole thousandths, so a run can be compared with another)
			int aMoving = 0;
			long long aYSum = 0;
			long long aSpeedSum = 0;
			for (int aRow = 0; aRow < Board::ROWS; aRow++) {
				for (int aCol = 0; aCol < Board::COLS; aCol++) {
					const Piece* aPiece = mBoard->mGrid[aRow][aCol];
					if (aPiece == nullptr)
						continue;
					if (!Board::PieceIsSettled(aPiece))
						aMoving++;
					aYSum += (long long)(aPiece->mY * 1000.0f);
					aSpeedSum += (long long)(aPiece->mFallSpeed * 1000.0f);
				}
			}
			aLine += ",\"update\":" + std::to_string(mBoard->mUpdateCnt) + ",\"moving\":" + std::to_string(aMoving) +
				",\"landticks\":" + std::to_string(mBoard->mLandTicks) + ",\"ysum\":" + std::to_string(aYSum) +
				",\"speedsum\":" + std::to_string(aSpeedSum);
			if (const QuestBoard* aQuest = dynamic_cast<const QuestBoard*>(mBoard)) {
				// the quest boards' update counters (+0x3198 and +0x31a0, QuestBoard::Update)
				aLine += ",\"updatesplayed\":" + std::to_string(aQuest->mUpdatesPlayed) + ",\"updatesrun\":" + std::to_string(aQuest->mUpdatesRun);
			}
			if (const ButterflyBoard* aButterflies = dynamic_cast<const ButterflyBoard*>(mBoard)) {
				// Butterflies: the moves each butterfly waits before it climbs (a digit, 9 for more; . where there is none), a caught one
				// as its digit in capitals' place (a letter from A: its count), the spider's column in hundredths, where it goes, the
				// drop's countdown, the credit and the count of a drop in thousandths, the chance of a side column in thousandths, and the
				// butterflies freed and the best number freed by one move
				aLine += ",\"butterflies\":[";
				for (int aRow = 0; aRow < Board::ROWS; aRow++) {
					aLine += aRow == 0 ? "\"" : ",\"";
					for (int aCol = 0; aCol < Board::COLS; aCol++) {
						const Piece* aPiece = aButterflies->mGrid[aRow][aCol];
						if (aPiece == nullptr || (aPiece->mFlags & Piece::FLAG_BUTTERFLY) == 0)
							aLine += '.';
						else {
							int aLeft = aPiece->mTimeGemSeconds < 0 ? 0 : aPiece->mTimeGemSeconds > 9 ? 9 : aPiece->mTimeGemSeconds;
							aLine += aPiece->mSpecialState == -1 ? (char)('A' + aLeft) : (char)('0' + aLeft);
						}
					}
					aLine += "\"";
				}
				aLine += "],\"spiderx\":" + std::to_string((int)(aButterflies->mSpiderX * 100.0f)) + ",\"spiderto\":" +
					std::to_string((int)aButterflies->mSpiderTo) + ",\"dropcountdown\":" + std::to_string(aButterflies->mDropCountdown) +
					",\"credit\":" + std::to_string((int)(aButterflies->mSpawnCredit * 1000.0f)) + ",\"spawncount\":" +
					std::to_string((int)(aButterflies->mSpawnCount * 1000.0f)) + ",\"sidechance\":" +
					std::to_string((int)(aButterflies->mSideSpawnChance * 1000.0f)) + ",\"freed\":" +
					std::to_string(aButterflies->mCounters[ButterflyBoard::COUNTER_BUTTERFLIES_FREED]) + ",\"bestcombo\":" +
					std::to_string(aButterflies->mCounters[ButterflyBoard::COUNTER_BEST_COMBO]);
			}
			if (const IceStormBoard* anIce = dynamic_cast<const IceStormBoard*>(mBoard)) {
				// Ice Storm: for each of the eight columns the ice's height in thousandths of a row (-1 where no ice is made; a double's second
				// column too), how many columns wide it is, whether it is crushed (1) and its crack ticks (-1 none); then the ice taken off the bar
				// and the bar's need in thousandths of a row, the multiplier, the stage, whether the ice is paused, the ice's ticks, the columns
				// crushed, the best chain and the chain's length
				std::string aHeights;
				std::string aSpans;
				std::string aCrushed;
				std::string aCracks;
				for (int aColumn = 0; aColumn < IceStormBoard::COLUMN_COUNT; aColumn++) {
					const IceStormBoard::Column& aState = anIce->mColumns[aColumn];
					const bool anAnchor = aState.mOwner == aColumn && aState.mActive;
					const char* aSeparator = aColumn == 0 ? "" : ",";
					aHeights += aSeparator + std::to_string(anAnchor ? (int)(aState.mPosition * 1000.0f) : -1);
					aSpans += aSeparator + std::to_string(anAnchor ? aState.mSpan : 0);
					aCrushed += aSeparator + std::to_string(anAnchor && aState.mCrushed ? 1 : 0);
					aCracks += aSeparator + std::to_string(anAnchor ? aState.mCrackTicks : -1);
				}
				aLine += ",\"ice\":[" + aHeights + "],\"icespan\":[" + aSpans + "],\"icecrushed\":[" + aCrushed + "],\"icecrack\":[" + aCracks +
					"],\"iceremoved\":" + std::to_string((int)(anIce->mIceRemoved * 1000.0f)) + ",\"icebar\":" +
					std::to_string((int)(anIce->mMultiplierIceThreshold * 1000.0f)) + ",\"icemult\":" + std::to_string(anIce->mLevelNumber) +
					",\"icestage\":" + std::to_string(anIce->mStage) + ",\"icepaused\":" + std::to_string(anIce->mPaused ? 1 : 0) +
					",\"iceticks\":" + std::to_string(anIce->mTicks) + ",\"icecrushedcount\":" +
					std::to_string(anIce->mCounters[IceStormBoard::COUNTER_COLUMNS_CRUSHED]) + ",\"icebestchain\":" +
					std::to_string(anIce->mBestChain) + ",\"icechain\":" + std::to_string(anIce->mChainCount);
			}
			if (const PokerBoard* aPoker = dynamic_cast<const PokerBoard*>(mBoard)) {
				// Poker: the hand (each card's suit, "/" and its second suit, a letter for a wild (w), flame (f) or star (s) card; - for no card),
				// the best hand's index (-1 none), the skull's level (-1 none) and its clock and the eliminator's bar in thousandths, the hands
				// cleared, the skulls busted, the counts of each kind (the first the hands a skull took), the curves of the hand, the flip and the fly-away in thousandths, the last hand's
				// points and the coin flips
				aLine += ",\"hand\":\"" + aPoker->DescribeCards() + "\",\"besthand\":" + std::to_string(aPoker->mBestHand) + ",\"skull\":" +
					std::to_string(aPoker->mSkull) + ",\"skullclock\":" + std::to_string((int)(aPoker->mSkullClock * 1000.0f)) + ",\"buster\":" +
					std::to_string((int)(aPoker->mBuster * 1000.0f)) + ",\"hands\":" + std::to_string(aPoker->mHands) + ",\"busted\":" +
					std::to_string(aPoker->mSkullsBusted) + ",\"counts\":\"";
				for (int i = 0; i <= PokerBoard::HAND_KINDS; i++)
					aLine += (i > 0 ? "," : "") + std::to_string(aPoker->mHandCounts[i]);
				aLine += "\",\"handcurve\":" + std::to_string((int)(aPoker->mHandCurve.GetOutVal() * 1000.0)) + ",\"flipcurve\":" + std::to_string((int)(aPoker->mFlipCurve.GetOutVal() * 1000.0)) + ",\"flycurve\":" + std::to_string((int)(aPoker->mFlyCurve.GetOutVal() * 1000.0)) + ",\"handscore\":" + std::to_string(aPoker->mHandScore) +
					",\"flips\":" + std::to_string(aPoker->mCounters[PokerBoard::COUNTER_COIN_FLIPS]);
			}
			if (const LightningBoard* aLightning = dynamic_cast<const LightningBoard*>(mBoard)) {
				// Lightning: the game ticks left of the round, the clock, the seconds the matched time gems hold (this round's account and
				// the game's), the multiplier, whether the time-up is shown, the chances' counters and the time gems on the board (each
				// cell its seconds as a letter: 5 or a for 10, . for none)
				aLine += ",\"timeleft\":" + std::to_string(aLightning->GetTimeLeft()) + ",\"clock\":" + std::to_string((int)(aLightning->mClock * 100.0f)) +
					",\"limit\":" + std::to_string(aLightning->mTimeLimitTicks) + ",\"bonus\":" + std::to_string(aLightning->mBonusSeconds) +
					",\"bonustotal\":" + std::to_string(aLightning->mBonusSecondsTotal) + ",\"timeup\":" + std::to_string(aLightning->mTimeUpShown ? 1 : 0) +
					",\"gameticks\":" + std::to_string(aLightning->mGameTicks) + ",\"chance5\":" + std::to_string((int)(aLightning->mChance5.mChance * 1000000.0f)) +
					",\"chance10\":" + std::to_string((int)(aLightning->mChance10.mChance * 1000000.0f)) + ",\"timegems\":[";
				for (int aRow = 0; aRow < Board::ROWS; aRow++) {
					aLine += aRow == 0 ? "\"" : ",\"";
					for (int aCol = 0; aCol < Board::COLS; aCol++) {
						const Piece* aPiece = aLightning->mGrid[aRow][aCol];
						int aSeconds = aPiece != nullptr && (aPiece->mFlags & Piece::FLAG_TIME_GEM) != 0 ? aPiece->mTimeGemSeconds : 0;
						aLine += aSeconds == 0 ? '.' : aSeconds == 5 ? '5' : 'a';
					}
					aLine += "\"";
				}
				aLine += "]";
			}
		}
		aLine += "}";
		Reply(aLine);
		return true;
	}
	return SexyApp::HandleScriptCommand(theCommand);
}
