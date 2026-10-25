#include "Board.h"
#include "BookWormApp.h"
#include "Res.h"
#include "Compat2004.h"
#include "UserManager.h"
#include "Lex.h"
#include "Effect.h"
#include "Points.h"
#include "OverlayWidget.h"
#include "WordBalloon.h"
#include "LevelUpWidget.h"
#include "Definitionator.h"
#include "GameText.h"
#include "ModFeatures.h"
#include "Solver.h"
#include "Undo.h"

#include "WidgetManager.h"
#include "DialogButton.h"
#include "Dialog.h"
#include "Image.h"
#include "ImageFont.h"
#include "MusicInterface.h"
#include "Common.h"
#include "platform/VFS.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <ctime>

using namespace Sexy;

// Board.cpp, the rules half: construction, the per-tick update, input, words and scoring, fire tiles,
// levels and the saved game. Spec: re/bookworm/analysis/board_logic.md (section numbers below).
// Random numbers (board_logic.md 3): gGameRand (the app's global MTRand) draws the letters, the green and
// chance-fire rolls, the special tiles, the scramble and timed fire picks and the bonus words; mRand (the
// Board's own MTRand) draws the drop heights, Lex's chatter and every particle the Board spawns, in the
// original's order (each draw is its own statement below). Sexy::Rand() only seeds mRand.

// --------------------------------------------------------------------------------------------------------
// Tables (rdata), board_logic.md 2.5

// 0x4f97e0: cumulative game score at which level n (0-based) is complete (LevelThreshold)
const int Sexy::gLevelThresholds[42] =
{
	3000, 7000, 12000, 18000, 25000, 34000, 44000, 56000, 69000, 80000, 100000, 120000, 140000, 160000,
	190000, 220000, 250000, 290000, 330000, 380000, 430000, 490000, 560000, 630000, 710000, 800000, 900000,
	1000000, 1100000, 1200000, 1300000, 1500000, 1700000, 1900000, 2100000, 2300000, 2600000, 2900000,
	3200000, 3500000, 3900000, 4300000
};

// 0x4f9888: letter values A..Z
const int Sexy::gLetterValues[26] =
{
	1, 4, 4, 3, 1, 5, 3, 5, 1, 8, 7, 2, 4, 2, 1, 4, 7, 2, 1, 2, 2, 5, 6, 8, 6, 10
};

// 0x4f98f0: WordScore::CalcScoreByLength's bonus by length (dead formula)
const int Sexy::gLengthBonus[9] = { 0, 0, 0, 50, 100, 200, 400, 600, 1000 };

// 0x4f9918: letter frequencies A..Z for RandomLetter and the timed fire pick (sum 95500)
const int Sexy::gLetterWeights[26] =
{
	9000, 2200, 2200, 4200, 12300, 2300, 3000, 2000, 7000, 900, 900, 4000, 2300, 6000, 7000, 2300, 700, 6000,
	4400, 6000, 4400, 1600, 1800, 500, 2000, 500
};

// 0x4f9980: chance (percent) of a random fire tile at each refill, classic mode
const int Sexy::gFireChanceByLevel[13] = { 0, 0, 0, 2, 3, 4, 6, 8, 12, 16, 24, 32, 30 };

// 0x4f99b8: fire meter refill when the meter runs out
const double Sexy::gFireMeterRefill[12] = { 5.25, 4.0, 3.5, 3.0, 2.5, 2.0, 1.8, 1.6, 1.5, 1.4, 1.3, 1.2 };

// 0x4f9a18: action-mode speeds, only written to a dead global (SetActionSpeed)
const int Sexy::gActionSpeed[10] = { 550, 500, 450, 400, 350, 300, 250, 200, 150, 125 };

// 0x4f9a40: fire meter change by word length (3, 4, 5, 6, 7, 8+ letters)
const double Sexy::gFireMeterByLength[6] = { -0.5, 0.35, 1.45, 1.85, 3.0, 4.75 };

// 0x4f9a70: points of the n-th bonus word of a game
const int Sexy::gBonusWordPoints[11] = { 1000, 2000, 4000, 6000, 10000, 15000, 20000, 25000, 30000, 40000, 50000 };

// 0x4f9a9c: length of the n-th bonus word handed out
const int Sexy::gBonusWordLength[10] = { 3, 3, 3, 3, 3, 4, 4, 4, 5, 5 };

// 0x52e280 / 0x52e298: submit button label colours (normal, mouse over) for an invalid / a valid word
int Sexy::gSubmitColorsInvalid[2][3] = { { 221, 152, 83 }, { 255, 200, 14 } };
int Sexy::gSubmitColorsValid[2][3] = { { 255, 200, 14 }, { 255, 200, 14 } };

// --------------------------------------------------------------------------------------------------------
// Globals of the module

int Sexy::gLivePieceCount = 0;          // 0x534720 debug counter
static int gLastLetterIdx = -1;         // 0x52e2b0 RandomLetter: the previous letter, program-wide
static int gLastRefillLevel = -1;       // 0x52e3a0 Refill: level of the previous refill, survives games
namespace Sexy { double gActionSpeedValue = 1.0; } // 0x52e278 written by SetActionSpeed, never read
static int gLen6Gate = 0;               // 0x5347c8 never written: AwardSpecialTile's 6-letter draw is dead

// The hint dialogs' texts (Update step 19), indexed by BoardHintId; only 1, 2, 4 and 10 are scheduled.
// Hint 10 has an action-mode and a classic text.
static const GameTextId gHintHeaders[11] =
{
	GT_HINT_HOW_TO_PLAY_HEADER, GT_HINT_WATCH_OUT_HEADER, GT_HINT_BONUS_TILE_HEADER, GT_HINT_SUPER_BONUS_TILE_HEADER,
	GT_HINT_WARNING_HEADER, GT_HINT_SUBMIT_BUTTON_HEADER, GT_HINT_SUBMIT_CLICKING_HEADER,
	GT_HINT_SUBMIT_DRAGGING_HEADER, GT_HINT_SUBMIT_KEYBOARD_HEADER, GT_HINT_BONUS_WORD_HEADER, GT_HINT_FIRE_ALERT_HEADER
};

static const GameTextId gHintLines[11] =
{
	GT_HINT_HOW_TO_PLAY,
	GT_HINT_WATCH_OUT,
	GT_HINT_BONUS_TILE,
	GT_HINT_SUPER_BONUS_TILE,
	GT_HINT_WARNING,
	GT_HINT_SUBMIT_BUTTON,
	GT_HINT_SUBMIT_CLICKING,
	GT_HINT_SUBMIT_DRAGGING,
	GT_HINT_SUBMIT_KEYBOARD,
	GT_HINT_BONUS_WORD,
	GT_HINT_FIRE_ALERT_ACTION
};

static const GameTextId gHintFireAlertClassic = GT_HINT_FIRE_ALERT_CLASSIC;

// --------------------------------------------------------------------------------------------------------
// Helpers

// The top row of a column: even columns hold rows 1..7, odd columns rows 0..7
static inline int TopRow(int theCol)
{
	return (theCol % 2 == 0) ? 1 : 0;
}

// gLetterWeights by letter (the original indexes 0x4f9814 + c * 4, the same table by raw char).
// port: 0 outside 'A'..'Z' (a tile only holds upper-case letters)
static inline int LetterWeight(char theLetter)
{
	int anIndex = theLetter - 'A';
	if ((anIndex < 0) || (anIndex >= 26))
		return 0;
	return gLetterWeights[anIndex];
}

// Lex::IsBusy inlined: "Lex idle" = no animation and at his target
static inline bool LexIdle(Lex* theLex)
{
	return !theLex->IsBusy();
}

// "Lex calm": idle, or sweating in danger
static inline bool LexCalm(Lex* theLex)
{
	return (!theLex->IsBusy()) || (theLex->mAnim == LEXANIM_SWEAT);
}

// 0x420e00 EmberListMgr::AddEmber leaves the ember it filled in EAX and some Board callers set more fields
// on it. Effect.h's AddEmber returns nothing, so these callers do AddEmber's steps here: NewEmber (whose
// Reset gives the defaults, gravity 0.01 and delay 0), then the eight fields.
static Ember* AddEmberGet(EmberListMgr* theMgr, int theType, double theX, double theY, double theVX, double theVY,
	bool theLoop, double theAlpha, double theAlphaDecay)
{
	Ember* anEmber = theMgr->NewEmber();
	if (anEmber != NULL)
	{
		anEmber->mType = theType;
		anEmber->mX = theX;
		anEmber->mY = theY;
		anEmber->mVX = theVX;
		anEmber->mVY = theVY;
		anEmber->mLoop = theLoop;
		anEmber->mAlpha = theAlpha;
		anEmber->mAlphaDecay = theAlphaDecay;
	}
	return anEmber;
}

// Saved-game record helpers: raw little-endian fields as the original's fwrite/fread (board_logic.md 6).
// A field the file does not hold (a truncated file) keeps its value, as with the original's fread.
static void FileWriteBytes(FILE* theFile, const void* theData, int theLength)
{
	fwrite(theData, 1, theLength, theFile);
}

static void FileWriteInt(FILE* theFile, int theValue)
{
	fwrite(&theValue, 1, 4, theFile);
}

static void FileWriteDouble(FILE* theFile, double theValue)
{
	fwrite(&theValue, 1, 8, theFile);
}

static void FileWriteBool(FILE* theFile, bool theValue)
{
	unsigned char aByte = theValue ? 1 : 0;
	fwrite(&aByte, 1, 1, theFile);
}

static void FileWriteChar(FILE* theFile, char theValue)
{
	fwrite(&theValue, 1, 1, theFile);
}

static void FileWriteString(FILE* theFile, const SexyString& theString)
{
	uint32_t aLength = (uint32_t)theString.size();
	fwrite(&aLength, 1, 4, theFile);
	fwrite(theString.data(), 1, aLength, theFile);
}

static void FileReadInt(FILE* theFile, int* theValue)
{
	fread(theValue, 1, 4, theFile);
}

static void FileReadDouble(FILE* theFile, double* theValue)
{
	fread(theValue, 1, 8, theFile);
}

static void FileReadBool(FILE* theFile, bool* theValue)
{
	unsigned char aByte = *theValue ? 1 : 0;
	fread(&aByte, 1, 1, theFile);
	*theValue = aByte != 0;
}

static void FileReadChar(FILE* theFile, char* theValue)
{
	fread(theValue, 1, 1, theFile);
}

// port: the length is 0 when the file ends here (the original then kept a stale local)
static void FileReadString(FILE* theFile, SexyString* theString)
{
	uint32_t aLength = 0;
	fread(&aLength, 1, 4, theFile);
	theString->resize(aLength, '\0');
	if (aLength > 0)
		fread(&(*theString)[0], 1, aLength, theFile);
}

// The players' saved games: the app's path ("players/<name>/classic.dat"), which the framework's VFS puts
// in the user dir. Writes of a relative path land there; reads need the "userdata/" prefix (NAMES.md L4).
static SexyString UserDataPath(const SexyString& thePath)
{
	if ((thePath.size() >= 9) && (sexy_strnicmp(thePath.c_str(), "userdata/", 9) == 0))
		return thePath;
	return "userdata/" + thePath;
}

// --------------------------------------------------------------------------------------------------------
// Piece (board_logic.md 2.2)

// 0x403f20
Piece::Piece()
{
	gLivePieceCount++;
	mX = 0.0;
	mY = 0.0;
	mFlySpeed = 0.0;
	mVY = 0.0;
	mUnk28 = 0;
	mUnk2c = 0;
	mCol = 0;
	mRow = 0;
	mUnk38 = false;
	mUnk3c = 1;
	mUnk40 = false;
	mUnk41 = false;
	mUnk48 = 0.0;
	mUnk50 = 0.0;
	mUnk58 = 0;
	mUnk5c = 0;
	mUnk60 = false;
	mUnk61 = false;
	mUnk64 = 0;
	mUnk70 = false;
	mUnk78 = 0.0;
	mUnk80 = 0.0;
	mUnk88 = 0;
	mUnk8c = 0;
	mUnk90 = 0;
	mUnk94 = 0;
	mUnk98 = false;
	mUnk68 = 0;
	mUnk6c = 0;
	mUnk9c = 0;
	mUnka0 = 0;
	mNextLetter = (char)0xff;
	mLetter = (char)0xff;
	mUnkc0 = 0;
	mScale = 1.0;
	mSelected = false;
	mBurning = false;
	mBurnProgress = 0;
	mBurnDelay = 0;
	mPendingFire = false;
	mFlicker = false;
	mFlickerFrame = 0;
	mShaking = false;
	mShakeAmount = 0.0;
	mFlipping = false;
	mFlipProgress = 0.0;
	mType = TILE_NORMAL;
	mBurnCountdown = 0;
}

// 0x402ff0 (scalar deleting dtor 0x404050)
Piece::~Piece()
{
	gLivePieceCount--;
}

// 0x403000: only a valid type; the countdown is cleared on request (and only then)
void Piece::SetType(int theType, bool resetCountdown)
{
	if ((theType >= 0) && (theType < NUM_TILE_TYPES))
	{
		mType = theType;
		if (resetCountdown)
			mBurnCountdown = 0;
	}
}

// 0x403030: charred by the fire tile above; sinks after 20 ticks
void Piece::StartBurn()
{
	mBurning = true;
	mBurnProgress = 0;
	mBurnDelay = 20;
}

// 0x404080: how many advances of the fire tile above this tile survives (fire tiles: 0)
void Piece::ResetBurnCountdown()
{
	int aType = mType;
	mBurnCountdown = 0;
	if (aType == TILE_NORMAL)
		mBurnCountdown = 1;
	else if (aType == TILE_GREEN)
		mBurnCountdown = 2;
	else if (aType == TILE_GOLD)
		mBurnCountdown = 3;
	else if (aType == TILE_SAPPHIRE)
		mBurnCountdown = 4;
	else if (aType == TILE_DIAMOND)
		mBurnCountdown = 5;
}

// 0x403f10
void Piece::Deselect(Piece* thePiece)
{
	thePiece->mSelected = false;
}

// 0x403050: the 191-byte record (board_logic.md 6.2); FlyingPieceList writes the same
void Piece::Save(FILE* theFile)
{
	FileWriteInt(theFile, mType);
	FileWriteDouble(theFile, mX);
	FileWriteDouble(theFile, mY);
	FileWriteDouble(theFile, mFlySpeed);
	FileWriteDouble(theFile, mVY);
	FileWriteInt(theFile, mUnk28);
	FileWriteInt(theFile, mUnk2c);
	FileWriteInt(theFile, mCol);
	FileWriteInt(theFile, mRow);
	FileWriteBool(theFile, mUnk38);
	FileWriteDouble(theFile, mScale);
	FileWriteInt(theFile, mUnkc0);
	FileWriteChar(theFile, mLetter);
	FileWriteChar(theFile, mNextLetter);
	FileWriteBool(theFile, mSelected);
	FileWriteBool(theFile, mBurning);
	FileWriteInt(theFile, mBurnProgress);
	FileWriteInt(theFile, mBurnDelay);
	FileWriteBool(theFile, mPendingFire);
	FileWriteInt(theFile, mBurnCountdown);
	FileWriteBool(theFile, mFlicker);
	FileWriteInt(theFile, mFlickerFrame);
	FileWriteBool(theFile, mShaking);
	FileWriteDouble(theFile, mShakeAmount);
	FileWriteBool(theFile, mFlipping);
	FileWriteDouble(theFile, mFlipProgress);
	FileWriteInt(theFile, mUnk3c);
	FileWriteBool(theFile, mUnk40);
	FileWriteBool(theFile, mUnk41);
	FileWriteDouble(theFile, mUnk48);
	FileWriteDouble(theFile, mUnk50);
	FileWriteInt(theFile, mUnk58);
	FileWriteInt(theFile, mUnk5c);
	FileWriteBool(theFile, mUnk60);
	FileWriteBool(theFile, mUnk61);
	FileWriteInt(theFile, mUnk64);
	FileWriteInt(theFile, mUnk68);
	FileWriteInt(theFile, mUnk6c);
	FileWriteBool(theFile, mUnk70);
	FileWriteDouble(theFile, mUnk78);
	FileWriteDouble(theFile, mUnk80);
	FileWriteInt(theFile, mUnk88);
	FileWriteInt(theFile, mUnk8c);
	FileWriteInt(theFile, mUnk90);
	FileWriteInt(theFile, mUnk94);
	FileWriteBool(theFile, mUnk98);
	FileWriteInt(theFile, mUnk9c);
	FileWriteInt(theFile, mUnka0);
}

// 0x403370
void Piece::Load(FILE* theFile)
{
	FileReadInt(theFile, &mType);
	FileReadDouble(theFile, &mX);
	FileReadDouble(theFile, &mY);
	FileReadDouble(theFile, &mFlySpeed);
	FileReadDouble(theFile, &mVY);
	FileReadInt(theFile, &mUnk28);
	FileReadInt(theFile, &mUnk2c);
	FileReadInt(theFile, &mCol);
	FileReadInt(theFile, &mRow);
	FileReadBool(theFile, &mUnk38);
	FileReadDouble(theFile, &mScale);
	FileReadInt(theFile, &mUnkc0);
	FileReadChar(theFile, &mLetter);
	FileReadChar(theFile, &mNextLetter);
	FileReadBool(theFile, &mSelected);
	FileReadBool(theFile, &mBurning);
	FileReadInt(theFile, &mBurnProgress);
	FileReadInt(theFile, &mBurnDelay);
	FileReadBool(theFile, &mPendingFire);
	FileReadInt(theFile, &mBurnCountdown);
	FileReadBool(theFile, &mFlicker);
	FileReadInt(theFile, &mFlickerFrame);
	FileReadBool(theFile, &mShaking);
	FileReadDouble(theFile, &mShakeAmount);
	FileReadBool(theFile, &mFlipping);
	FileReadDouble(theFile, &mFlipProgress);
	FileReadInt(theFile, &mUnk3c);
	FileReadBool(theFile, &mUnk40);
	FileReadBool(theFile, &mUnk41);
	FileReadDouble(theFile, &mUnk48);
	FileReadDouble(theFile, &mUnk50);
	FileReadInt(theFile, &mUnk58);
	FileReadInt(theFile, &mUnk5c);
	FileReadBool(theFile, &mUnk60);
	FileReadBool(theFile, &mUnk61);
	FileReadInt(theFile, &mUnk64);
	FileReadInt(theFile, &mUnk68);
	FileReadInt(theFile, &mUnk6c);
	FileReadBool(theFile, &mUnk70);
	FileReadDouble(theFile, &mUnk78);
	FileReadDouble(theFile, &mUnk80);
	FileReadInt(theFile, &mUnk88);
	FileReadInt(theFile, &mUnk8c);
	FileReadInt(theFile, &mUnk90);
	FileReadInt(theFile, &mUnk94);
	FileReadBool(theFile, &mUnk98);
	FileReadInt(theFile, &mUnk9c);
	FileReadInt(theFile, &mUnka0);
}

// --------------------------------------------------------------------------------------------------------
// Static helpers (board_logic.md 1, 3)

// 0x402f60: neighbours on the hex layout (odd columns sit half a tile lower); not the tile itself
bool Board::IsAdjacent(Piece* thePiece1, Piece* thePiece2)
{
	int aColDelta = thePiece1->mCol - thePiece2->mCol;
	int aRowDelta = thePiece1->mRow - thePiece2->mRow;

	if (std::abs(aColDelta) == 1)
	{
		int aParity = thePiece2->mCol % 2;
		if ((aParity == 0) && ((aRowDelta == 0) || (aRowDelta == -1)))
			return true;
		if ((aParity == 1) && ((aRowDelta == 0) || (aRowDelta == 1)))
			return true;
	}

	if ((aColDelta == 0) && (std::abs(aRowDelta) == 1))
		return true;

	return false;
}

// 0x402fd0: writes a global nothing reads
void Board::SetActionSpeed(double theSpeed)
{
	gActionSpeedValue = theSpeed;
}

// 0x403880: the score at which level n is complete; past the table it grows quadratically
int Board::LevelThreshold(unsigned int theLevel)
{
	unsigned int anIndex = (theLevel > 40) ? 41 : theLevel;
	int aThreshold = gLevelThresholds[anIndex];
	if (theLevel > 41)
		aThreshold += (int)((theLevel - 22) * (theLevel - 41) * 25000);
	return aThreshold;
}

// 0x403d10: a letter by gLetterWeights, never the same as the previous one (program-wide); every rejected
// letter costs a gGameRand draw
int Board::RandomLetter()
{
	for (;;)
	{
		int aRoll = (int)gGameRand->Next() % 95500;
		int anIndex = 0;
		while (anIndex < 25)
		{
			aRoll -= gLetterWeights[anIndex];
			if (aRoll < 0)
				break;
			anIndex++;
		}

		if (anIndex != gLastLetterIdx)
		{
			gLastLetterIdx = anIndex;
			return 'A' + anIndex;
		}
	}
}

// --------------------------------------------------------------------------------------------------------
// Construction (board_logic.md 4.1)

// 0x40ae90
Board::Board(BookWormApp* theApp)
{
	mApp = theApp;
	mWord.mApp = theApp;
	mLastWord.mApp = theApp;
	mHighlightKey = 0;
	mCtrlDown = false;
	mLevelUpWidget = NULL;
	mWord.Clear();
	mRand = NULL;
	mOverlay = NULL;
	mEffectMgr = NULL;
	mEmberMgr = NULL;
	mGameStats = NULL;
	mLevelStats = NULL;
	mRewardStreak = 0;
	mGameOverPiece = NULL;
	mFireLinePiece = NULL;
	mLastMessage = NULL;
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
			mBoard[aRow][aCol] = NULL;
	mLastWordAlpha = 0.0;
	mScoreGlow = 0.0;
	mNewHighScore = false;
	mPiecesFlying = false;
	mFlameSoundPlayed = false;
	mFireMeter = 0.0;
	mSkipRandomFire = true;
	mUnused2cc = 0;
	mUnused2ec = 0;
	mUpSoundIdx = 0;
	mUpSoundCount = 0;
	mUpSoundDelay = 0;
	mDeadSparkDelay = 0;
	mBoardBurnDelay = 0;
	mBoardBurnAlpha = 0.0;
	mBlinkDelay = 0;
	mBlinkRepeats = 0;
	mGameOverMusicDelay = 0;
	mFireLineDelay = 0;
	mFireLineAlpha = 0.0;
	mFireLineScroll = 0;
	mFireTileCount = 0;
	mActionFireTimer = 1000;
	mActionCountdown = 0;
	mWarnBeeps = 0;
	mWarnBeepDelay = 0;
	mHintDelay = 0;
	mHintId = 0;
	mSubmitBlinkWords = 3;
	// port: never initialised by the original (heap contents; mUnused1b2 is only saved, the praise flag is
	// set before it is read once a special tile is awarded)
	mUnused1b2 = 0;
	mLexPraisePending = false;
	mLexPraiseType = 0;

	mSubmitButton = MakeButton(BOARD_BUTTON_SUBMIT, Rect(6, 100, 211, 30), NULL, NULL, NULL, GameText(GT_BOARD_SUBMIT));
	DialogButton* anOptionsButton = MakeButton(BOARD_BUTTON_OPTIONS, Rect(6, 181, 211, 30), NULL, NULL, NULL, GameText(GT_OPTIONS));
	MakeButton(BOARD_BUTTON_LEAVE, Rect(6, 446, 211, 30), NULL, NULL, NULL, GameText(GT_BOARD_LEAVE_GAME));

	// port(mod): the word solver's HINT takes the right part of SUBMIT's plank
	mSolver = NULL;
	mHintButton = NULL;
	if (ModFeatureEnabled("solver"))
	{
		mSolver = new Solver(this);
		mSubmitButton->Resize(6, 100, 130, 30);
		mHintButton = MakeButton(BOARD_BUTTON_HINT, Rect(136, 100, 81, 30), NULL, NULL, NULL, "HINT");
	}

	// port(mod): undo's UNDO takes the right part of OPTIONS' plank
	mUndo = NULL;
	mUndoButton = NULL;
	if (ModFeatureEnabled("undo"))
	{
		mUndo = new UndoStack(this);
		anOptionsButton->Resize(6, 181, 130, 30);
		mUndoButton = MakeButton(BOARD_BUTTON_UNDO, Rect(136, 181, 81, 30), NULL, NULL, NULL, "UNDO");
	}

	// new MTRand(Sexy::Rand()) with the 2004 seeding; the EffectMgr and EmberListMgr ctors draw the next two
	// Sexy::Rand() seeds (NAMES.md 2.11)
	unsigned long aSeed = (unsigned long)Rand();
	mRand = new MTRand();
	SeedMTRand2004(*mRand, aSeed);
	mStartTick = GameTickCount();
	mLevelStartTick = mStartTick;
	mEffectMgr = new EffectMgr(theApp);
	mEmberMgr = new EmberListMgr(theApp, 1000);

	mLevelSeconds = 0;
	mTotalSeconds = 0;
	mLevelSecondsBanked = 0;
	mLastLandSoundTick = 0;
	mCols = BOARD_COLS;
	mRows = BOARD_ROWS;
	mBurnedDown = false;
	mState = BOARD_PLAYING;
	mSelectMode = SELECT_NONE;
	mPlayCounted = false;
	mBonusWordsGiven = 0;
	mBonusWord = "";
	mGameStats = new StatKeeper(theApp);
	mLevelStats = new StatKeeper(theApp);
	mLex = NULL;
	mLexGreetPending = true;
	mLexDangerTalkReady = false;
	mLevelUpPending = false;
	mBlinkFrames = 0;
	mBurpPending = false;
	mBurping = false;
	mBurpDelay = 0;
	mBurpAlpha = 0.0;
	mScrambleProgress = 0.0;
	mPointsAwarded = 0;
	mShownScore = 0;
	mScoreScale = 1.0;
	mMouseX = 0;
	mMouseY = 0;
	mSummaryShown = false;
	mPiecesMoving = false;
	mLevel = 0;
	mFireMeter = 5.25;
	mPauseCount = 0;
	mPauseFade = 0.0;
	mPauseNoDialogs = false;
	mBestWordThisLevel = 0;
	mBestWordThisGame = 0;
	mUnused3e8 = 0;
	mUnused3ec = 0;

	// the game log header (board_logic.md 7): version 3, mode, seed, time, level 0
	mGameLog.WriteByte(3);
	mGameLog.WriteByte((uchar)mApp->mGameMode);
	mGameLog.WriteLong((long)aSeed);
	mGameLog.WriteLong((long)time(NULL));
	mGameLog.WriteNumBits(0xe, 4);
	mGameLog.WriteShort((short)mLevel);

	NewBoard();
	mUnused3d8 = 0;
	if (mApp->mGameMode == GAMEMODE_ACTION)
		SetActionSpeed((double)gActionSpeed[(mLevel > 8) ? 9 : mLevel]);
}

// 0x4062e0 (scalar deleting dtor 0x406c10). The pieces in a FlyingPieceList belong to it; the popups were
// deleted when they were removed (SafeDeleteWidget), only their list nodes go here.
Board::~Board()
{
	RemoveLevelUpWidget();

	delete mSolver;     // port(mod)
	mSolver = NULL;
	delete mUndo;       // port(mod)
	mUndo = NULL;

	if (mRand != NULL)
	{
		delete mRand;
		mRand = NULL;
	}

	if (mOverlay != NULL)
	{
		mApp->mWidgetManager->RemoveWidget(mOverlay);
		delete mOverlay;
		mOverlay = NULL;
	}

	if (mEffectMgr != NULL)
	{
		delete mEffectMgr;
		mEffectMgr = NULL;
	}

	if (mEmberMgr != NULL)
	{
		delete mEmberMgr;
		mEmberMgr = NULL;
	}

	DeleteAllPieces();
	DeleteButtons();

	if (mGameStats != NULL)
	{
		delete mGameStats;
		mGameStats = NULL;
	}

	if (mLevelStats != NULL)
	{
		delete mLevelStats;
		mLevelStats = NULL;
	}

	if (mLex != NULL)
	{
		mApp->mWidgetManager->RemoveWidget(mLex);
		delete mLex;
		mLex = NULL;
	}

	mPopups.clear();
}

// 0x40ad00: a DialogButton with the dialog button image, the board as listener, no images of its own,
// the hand cursor, LockerGnome 14 outline and the "invalid word" colours, added to the app's manager
DialogButton* Board::MakeButton(int theId, const Rect& theRect, Image* theButtonImage, Image* theOverImage,
	Image* theDownImage, const SexyString& theLabel)
{
	DialogButton* aButton = new DialogButton(mApp->mImages[IMAGE_DIALOGBUTTON], theId, this);
	aButton->Resize(theRect);
	aButton->mButtonImage = theButtonImage;
	aButton->mDownImage = theDownImage;
	aButton->mDoFinger = true;
	aButton->mOverImage = theOverImage;
	aButton->mLabel = theLabel;
	aButton->SetFont(mApp->mFonts[FONT_LOCKERGNOME14OUTLINE]);
	aButton->SetColors(gSubmitColorsInvalid, 2);
	mApp->mWidgetManager->AddWidget(aButton);
	mButtons.push_back(aButton);
	return aButton;
}

// 0x4036e0
void Board::DeleteAllPieces()
{
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			if (mBoard[aRow][aCol] != NULL)
			{
				delete mBoard[aRow][aCol];
				mBoard[aRow][aCol] = NULL;
			}
		}
	}
}

// 0x405de0
void Board::DeleteButtons()
{
	if (mButtons.size() == 0)
		return;

	for (std::list<DialogButton*>::iterator anItr = mButtons.begin(); anItr != mButtons.end(); ++anItr)
	{
		DialogButton* aButton = *anItr;
		if (aButton != NULL)
		{
			if (mApp->mWidgetManager->HasWidget(aButton))
				mApp->mWidgetManager->RemoveWidget(aButton);
			delete aButton;
		}
	}
	mButtons.clear();
}

// 0x405810
void Board::RemoveButtons()
{
	if (mButtons.size() == 0)
		return;

	for (std::list<DialogButton*>::iterator anItr = mButtons.begin(); anItr != mButtons.end(); ++anItr)
	{
		if (*anItr != NULL)
			mApp->mWidgetManager->RemoveWidget(*anItr);
	}
}

// 0x405850
void Board::SetButtonsVisible(bool isVisible)
{
	if (mButtons.size() == 0)
		return;

	for (std::list<DialogButton*>::iterator anItr = mButtons.begin(); anItr != mButtons.end(); ++anItr)
	{
		if (*anItr != NULL)
			(*anItr)->SetVisible(isVisible);
	}
}

// 0x405890
void Board::SetButtonsDisabled(bool isDisabled)
{
	if (mButtons.size() == 0)
		return;

	for (std::list<DialogButton*>::iterator anItr = mButtons.begin(); anItr != mButtons.end(); ++anItr)
	{
		if (*anItr != NULL)
			(*anItr)->SetDisabled(isDisabled);
	}
}

// --------------------------------------------------------------------------------------------------------
// Widget plumbing

// 0x403730 slot 1: the overlay (a new one each time), and Lex the first time, sliding in from the left
void Board::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	mOverlay = new OverlayWidget(mApp);
	mOverlay->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mOverlay);

	if (mLex == NULL)
	{
		Image* anImage = mApp->mImages[IMAGE_LEX_EATING];
		mLex = new Lex(mApp);
		mLex->Resize(0, 0, anImage->GetWidth(), anImage->GetHeight());
		mWidgetManager->AddWidget(mLex);
		mLex->Move(-200, 0);
		mLex->mTargetX = 60;
		mLex->mTargetY = 210;
	}
}

// 0x405b00 slot 2
void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	RemoveButtons();

	if ((mLex != NULL) && (theWidgetManager->HasWidget(mLex)))
		theWidgetManager->RemoveWidget(mLex);
	if ((mOverlay != NULL) && (theWidgetManager->HasWidget(mOverlay)))
		theWidgetManager->RemoveWidget(mOverlay);

	for (std::list<Points*>::iterator anItr = mPopups.begin(); anItr != mPopups.end(); ++anItr)
	{
		Points* aPopup = *anItr;
		theWidgetManager->RemoveWidget(aPopup);
		mApp->SafeDeleteWidget(aPopup);
	}
	mPopups.clear();
	// port: the original keeps mLastMessage pointing at the deleted popup (ShowMessage would read it)
	mLastMessage = NULL;
}

// 0x405240 slot 3: board < buttons < Lex < overlay (no base call)
void Board::OrderInManagerChanged()
{
	if ((mLex != NULL) && (mVisible))
		mLex->SetVisible(true);

	if (mButtons.size() != 0)
	{
		for (std::list<DialogButton*>::iterator anItr = mButtons.begin(); anItr != mButtons.end(); ++anItr)
		{
			if ((*anItr != NULL) && (mLex != NULL))
				mWidgetManager->PutInfront(mLex, *anItr);
		}
	}

	if (mOverlay != NULL)
	{
		if (mLex == NULL)
			mWidgetManager->PutInfront(mOverlay, this);
		else
			mWidgetManager->PutInfront(mOverlay, mLex);
	}
}

// 0x405a50 slot 4
void Board::SetVisible(bool isVisible)
{
	Widget::SetVisible(isVisible);
	SetButtonsVisible(isVisible);
	if (mLex != NULL)
		mLex->SetVisible(isVisible);
}

// 0x405a20 slot 10: passes its argument to the buttons' and Lex's SetVisible (so SetDisabled(true) shows
// them); kept as in the original
void Board::SetDisabled(bool isDisabled)
{
	Widget::SetDisabled(isDisabled);
	SetButtonsVisible(isDisabled);
	if (mLex != NULL)
		mLex->SetVisible(isDisabled);
}

// 0x403a70 slot 24
void Board::MouseLeave()
{
	Widget::MouseLeave();
}

// 0x403a80 slot 25
void Board::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	mMouseY = y;
	mMouseX = x;
}

// 0x405d60 slot 20: space toggles the pause (and the music)
void Board::KeyChar(SexyChar theChar)
{
	if (theChar == ' ')
	{
		mApp->PauseAllMusic(mPauseCount == 0);
		Pause(mPauseCount == 0);
	}
}

// 0x403df0 slot 21: Ctrl state, and (registered builds, playing) the held letter key lights its tiles. The
// original tests the virtual key with the CRT isalpha, which also accepts 0x61..0x7a (numpad keys, F1..F11).
// No base call.
void Board::KeyDown(KeyCode theKey)
{
	if (theKey == KEYCODE_CONTROL)
		mCtrlDown = true;

	// port(mod): Tab asks the word solver for its next hint
	if ((theKey == KEYCODE_TAB) && (mSolver != NULL) && (mState == BOARD_PLAYING))
		mSolver->RequestHint();
	// port(mod): Ctrl+Z undoes the last move
	if ((theKey == (KeyCode)'Z') && (mCtrlDown) && (mUndo != NULL))
		mUndo->RequestUndo();

	if ((mApp->mIsRegistered) && (mState == BOARD_PLAYING))
	{
		// port: the CRT table lookup only for 0..255 (the original indexes it with any key code)
		int aKey = (int)theKey;
		if ((aKey >= 0) && (aKey < 256) && (isalpha(aKey)))
		{
			mHighlightKey = aKey;
			MarkDirty();
		}
	}
}

// 0x403e40 slot 22
void Board::KeyUp(KeyCode theKey)
{
	if (theKey == KEYCODE_CONTROL)
		mCtrlDown = false;

	if ((mApp->mIsRegistered) && (mState == BOARD_PLAYING))
	{
		int aKey = (int)theKey;
		if ((aKey >= 0) && (aKey < 256) && (isalpha(aKey)))
		{
			mHighlightKey = 0;
			MarkDirty();
		}
	}
}

// --------------------------------------------------------------------------------------------------------
// Geometry (board_logic.md 1)

// 0x403990
int Board::ColX(int theCol)
{
	return theCol * TILE_SIZE + 233;
}

// 0x4039b0: odd columns are 28 pixels lower
int Board::RowY(int theCol, int theRow)
{
	if (theCol % 2 == 0)
		return theRow * TILE_SIZE - 17;
	return theRow * TILE_SIZE + 11;
}

// 0x4039e0: the first tile in memory order whose current (animated) box holds the point
Piece* Board::PieceAt(int x, int y)
{
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if ((aPiece != NULL) && (aPiece->mX <= (double)x) && (aPiece->mY <= (double)y) &&
				((double)x < aPiece->mX + 57.0) && ((double)y < aPiece->mY + 57.0))
				return aPiece;
		}
	}
	return NULL;
}

// 0x403bd0
Piece* Board::PieceBelow(Piece* thePiece)
{
	if (thePiece->mRow < 7)
		return mBoard[thePiece->mRow + 1][thePiece->mCol];
	return NULL;
}

// --------------------------------------------------------------------------------------------------------
// Board contents (board_logic.md 4.2)

// 0x404150: a full board of normal tiles dropping in from above. Per column one mRand draw, then per tile
// RandomLetter (gGameRand) and one mRand draw.
void Board::NewBoard()
{
	mLevelProgress = 0.0;
	mShownProgress = 0.0;

	for (int aRow = 0; aRow < mRows; aRow++)
		for (int aCol = 0; aCol < mCols; aCol++)
			mBoard[aRow][aCol] = NULL;

	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		int aY = -64 - (int)(mRand->Next() & 63);
		int aTopRow = TopRow(aCol);
		int aX = ColX(aCol);
		for (int aRow = 7; aRow >= aTopRow; aRow--)
		{
			Piece* aPiece = new Piece();
			aPiece->mCol = aCol;
			aPiece->mY = (double)aY;
			aPiece->mRow = aRow;
			aPiece->mX = (double)aX;
			aPiece->mLetter = (char)RandomLetter();
			aPiece->mVY = 0.0;
			mBoard[aRow][aCol] = aPiece;
			aY += -57 - (int)(mRand->Next() % 80);
		}
	}
}

// 0x40b540: compacts every column, drops new tiles in, places the fire tiles that are due, and repairs the
// selection when tiles moved
void Board::Refill()
{
	bool isLevelUp = mLevel > gLastRefillLevel;
	bool aMoved = false;
	int aFireDue = 0;
	gLastRefillLevel = mLevel;

	// classic: a random fire tile (skipped once after a bonus word)
	int aChance = gFireChanceByLevel[((unsigned int)mLevel > 11) ? 12 : mLevel];
	if ((mApp->mGameMode != GAMEMODE_ACTION) && (aChance > 0) && ((int)(gGameRand->Next() % 100) < aChance))
	{
		if (!mSkipRandomFire)
			aFireDue = 1;
		else
			mSkipRandomFire = false;
	}

	// classic: the fire meter ran out (not on the first refill of a level)
	if ((!isLevelUp) && (mFireMeter <= 0.0) && (mApp->mGameMode == GAMEMODE_CLASSIC))
	{
		aFireDue++;
		mFireMeter = gFireMeterRefill[((unsigned int)mLevel > 10) ? 11 : mLevel] + mFireMeter;
	}

	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		// compact: tiles move down into the holes; the slots above keep stale pointers until the new tiles
		// overwrite them
		int aTop = 7;
		for (int aRow = 7; aRow >= 0; aRow--)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece != NULL)
			{
				if ((!aMoved) && (aPiece->mRow != aTop))
					aMoved = true;
				aPiece->mRow = aTop;
				mBoard[aTop][aPiece->mCol] = aPiece;
				aTop--;
			}
		}

		int aY = -64 - (int)(mRand->Next() & 63);
		int aTopRow = TopRow(aCol);
		for (int aRow = aTop; aRow >= aTopRow; aRow--)
		{
			Piece* aPiece = new Piece();
			aPiece->mRow = aRow;
			aPiece->mCol = aCol;
			aPiece->mX = (double)ColX(aCol);
			aPiece->mY = (double)aY;
			aPiece->mLetter = (char)RandomLetter();
			aPiece->mVY = 0.0;

			// a due fire tile goes only into a column that lost exactly one tile (the column's first empty
			// row is compared, not the current row); a leftover is dropped
			if ((aFireDue > 0) && (aTop == aTopRow))
			{
				aPiece->SetType(TILE_FIRE, true);
				mFireTileCount++;
				if (mSelection.size() != 0)
				{
					mWord.Build(&mSelection);
					mWord.CheckValid(&mApp->mDictionary);
				}
				if (!mFlameSoundPlayed)
				{
					mFlameSoundPlayed = true;
					mApp->PlaySample(SOUND_FLAMETILE);
				}
				aPiece->mLetter = (char)RandomLetter();
				aFireDue--;
			}

			if ((aPiece->mType == TILE_NORMAL) && (gGameRand->Next() % 20 == 0))
			{
				aPiece->SetType(TILE_GREEN, true);
				if (mSelection.size() != 0)
				{
					mWord.Build(&mSelection);
					mWord.CheckValid(&mApp->mDictionary);
				}
				mApp->PlaySample(SOUND_APPEAR2);
				mLex->PlayAnim(LEXANIM_OH, false);
				if ((!mApp->mGreenTileHintShown) && (mHintDelay == 0))
				{
					mApp->mGreenTileHintShown = true;
					mHintDelay = 80;
					mHintId = HINT_GREEN_TILE;
				}
			}

			mBoard[aRow][aCol] = aPiece;
			aY += -57 - (int)(mRand->Next() % 80);
		}

		// fire tiles: the tile below gets its countdown; one at the bottom schedules the fire alert hint
		for (int aRow = 7; aRow >= 0; aRow--)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if ((aPiece == NULL) || (aPiece->mType != TILE_FIRE))
				continue;

			if (aRow < 7)
			{
				Piece* aBelow = PieceBelow(aPiece);
				if ((aBelow != NULL) && (aBelow->mBurnCountdown < 1))
					aBelow->ResetBurnCountdown();
			}
			else if (!mApp->mFireAlertHintShown)
			{
				mHintDelay = 30;
				mHintId = HINT_FIRE_ALERT;
				mApp->mFireAlertHintShown = true;
			}
		}
	}

	// tiles moved: keep the selection up to the first tile that is no longer selected or adjacent. The
	// break tile is popped and re-added (AddLetter plays nextselect); alone, it is deselected and mWord is
	// left as it was.
	if (aMoved)
	{
		PieceList::iterator aPrevItr = mSelection.begin();
		if (aPrevItr == mSelection.end())
			return;

		PieceList::iterator aCurItr = aPrevItr;
		++aCurItr;
		while (aCurItr != mSelection.end())
		{
			if ((!(*aCurItr)->mSelected) || (!IsAdjacent(*aPrevItr, *aCurItr)))
				break;
			aPrevItr = aCurItr;
			++aCurItr;
		}
		if (aCurItr == mSelection.end())
			return;

		Piece* aPiece = *aPrevItr;
		TruncateAfter(aPiece);
		if (mSelection.size() != 0)
			mSelection.pop_back();
		if (mSelection.size() != 0)
			AddLetter(aPiece);
		else
			aPiece->mSelected = false;
	}
}

// 0x40b9e0: normal tiles that are neither burning nor about to catch fire (columns 0..6, bottom up)
void Board::GetNormalPieces(std::vector<Piece*>* thePieces)
{
	if (thePieces == NULL)
		return;

	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		for (int aRow = 7; aRow >= TopRow(aCol); aRow--)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if ((aPiece != NULL) && (aPiece->mType == TILE_NORMAL) && (!aPiece->mBurning) && (!aPiece->mPendingFire))
				thePieces->push_back(aPiece);
		}
	}
}

// 0x4042c0: classic: three warning beeps when a fire tile sits on the bottom row
void Board::CheckBottomFire()
{
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		for (int aRow = 7; aRow >= TopRow(aCol); aRow--)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if ((aPiece != NULL) && (aPiece->mType == TILE_FIRE) && (aRow == 7) && (mWarnBeeps == 0) &&
				(mApp->mGameMode == GAMEMODE_CLASSIC))
				mWarnBeeps = 3;
		}
	}
}

// 0x4052d0: takes the tiles off the grid; a fire tile's count goes down and the tile below gets a fresh
// countdown
void Board::RemovePieces(PieceList* thePieces)
{
	for (PieceList::iterator anItr = thePieces->begin(); anItr != thePieces->end(); ++anItr)
	{
		Piece* aPiece = *anItr;
		if (aPiece == NULL)
			continue;

		mBoard[aPiece->mRow][aPiece->mCol] = NULL;
		if (aPiece->mType == TILE_FIRE)
		{
			mFireTileCount--;
			Piece* aBelow = PieceBelow(aPiece);
			if (aBelow != NULL)
				aBelow->ResetBurnCountdown();
		}
	}
}

// --------------------------------------------------------------------------------------------------------
// Fire tiles, game over (board_logic.md 4.6)

// 0x408700: every fire tile advances on the tile below (classic: after each word and scramble; action: from
// the timer, advance = action mode); a fire tile on the bottom row ends the game (classic: unless selected;
// action: when the countdown ran out)
void Board::BurnTiles(bool advance)
{
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		for (int aRow = 7; aRow >= TopRow(aCol); aRow--)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if ((aPiece == NULL) || (aPiece->mType != TILE_FIRE))
				continue;

			bool isGameOver = false;
			if (aRow < 7)
			{
				Piece* aBelow = PieceBelow(aPiece);
				if ((aBelow != NULL) && (advance) && (aBelow->mBurnCountdown > 0) &&
					((mApp->mGameMode == GAMEMODE_ACTION) || (!aPiece->mSelected)))
				{
					if (--aBelow->mBurnCountdown == 0)
					{
						if (mApp->mGameMode == GAMEMODE_ACTION)
						{
							if ((aPiece->mSelected) || (aBelow->mSelected))
							{
								std::for_each(mSelection.begin(), mSelection.end(), Piece::Deselect);
								ClearWord();
							}
						}
						else if (aBelow->mSelected)
						{
							// classic: a selected tile does not burn; its countdown stays 0 until Refill
							continue;
						}

						mLevelStats->IncBurned();
						mGameStats->IncBurned();
						aBelow->StartBurn();
					}
				}
			}
			else if (mApp->mGameMode == GAMEMODE_ACTION)
				isGameOver = mActionCountdown < 0;
			else
				isGameOver = !aPiece->mSelected;

			if (isGameOver)
			{
				// the scan goes on: another bottom fire tile repeats these writes
				mBlinkRepeats = 1;
				mFireLineDelay = 1;
				mGameOverPiece = aPiece;
				mBoardBurnDelay = 60;
				mBlinkDelay = 210;
				mGameOverMusicDelay = 160;
				aPiece->mShaking = true;
				if (mOverlay != NULL)
				{
					mOverlay->mBalloon->Show(0);
					ShowOverlay(false);
				}
				SetDisabled(true);
				mState = BOARD_GAMEOVER_PENDING;
			}
		}
	}
}

// 0x4038c0: the game-over jingle (song 3 at its classic / action order, once)
void Board::StartGameOverMusic()
{
	mGameLog.WriteNumBits(0, 4);
	mGameLog.WriteLong(mGameStats->mScore);
	mState = BOARD_GAMEOVER_MUSIC;
	mLevelProgress = 0.0;

	MusicInterface* aMusic = mApp->mMusicInterface;
	if (mApp->mGameMode == GAMEMODE_CLASSIC)
	{
		aMusic->StopMusic(SONG_CLASSIC);
		aMusic->StopMusic(SONG_DANGER);
		aMusic->FadeIn(SONG_JINGLE, ORDER_GAMEOVER_CLASSIC, 1.0, true);
	}
	else if (mApp->mGameMode == GAMEMODE_ACTION)
	{
		aMusic->StopMusic(SONG_ACTION);
		aMusic->StopMusic(SONG_DANGER);
		aMusic->FadeIn(SONG_JINGLE, ORDER_GAMEOVER_ACTION, 1.0, true);
	}
}

// 0x40ed70 (Lex::MouseUp): turns up to n top-row tiles into fire tiles (n = scrambles this game) and gives
// every other tile a new letter (specials become normal); refused with a dialog when there are over 9 fire
// tiles or no normal top tile, and silently (classic) while a fire tile is at the bottom or a tile burns
void Board::Scramble()
{
	if (mState != BOARD_PLAYING)
		return;

	if (mFireTileCount > 9)
	{
		mApp->PlaySample(SOUND_BUZZER);
		Dialog* aDialog = mApp->DoDialog(DIALOG_HINT, true, GameText(GT_CANT_SCRAMBLE),
			GameText(GT_CANT_SCRAMBLE_TOO_MANY_FIRE), GameText(GT_DIALOG_OK), Dialog::BUTTONS_FOOTER);
		aDialog->Move(((mWidth - aDialog->mWidth) + 233) / 2, (mHeight - aDialog->mHeight) / 2);
		return;
	}

	std::vector<Piece*> aCandidates;
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		Piece* aPiece = mBoard[TopRow(aCol)][aCol];
		if ((aPiece != NULL) && (aPiece->mType == TILE_NORMAL))
			aCandidates.push_back(aPiece);
	}

	if (aCandidates.size() == 0)
	{
		mApp->PlaySample(SOUND_BUZZER);
		Dialog* aDialog = mApp->DoDialog(DIALOG_HINT, true, GameText(GT_CANT_SCRAMBLE),
			GameText(GT_CANT_SCRAMBLE_FIRE_AT_TOP), GameText(GT_DIALOG_OK), Dialog::BUTTONS_FOOTER);
		aDialog->Move(((mWidth - aDialog->mWidth) + 233) / 2, (mHeight - aDialog->mHeight) / 2);
		return;
	}

	if (mApp->mGameMode == GAMEMODE_CLASSIC)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			for (int aRow = 7; aRow >= TopRow(aCol); aRow--)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if (aPiece == NULL)
					continue;
				if ((aPiece->mType == TILE_FIRE) && (aRow == 7))
				{
					mApp->PlaySample(SOUND_BUZZER);
					return;
				}
				if (aPiece->mBurning)
					return;
			}
		}
	}

	if (mUndo != NULL)
		mUndo->Push();  // port(mod): the board before the scramble

	// play counting: never in the retail build (MaxTime 60, NAMES.md D49 / L6)
	if ((!mPlayCounted) && (mApp->mMaxTime == 0) && (mApp->mMaxPlays == 0))
	{
		mApp->mTimesPlayed++;
		mApp->mUserTimesPlayed++;
		mPlayCounted = true;
	}
	mApp->mScrambled = true;

	std::for_each(mSelection.begin(), mSelection.end(), Piece::Deselect);
	mSelection.clear();
	mLevelStats->IncScrambles();
	mGameStats->IncScrambles();

	int aCount = (int)aCandidates.size();
	if (mGameStats->mScrambles < aCount)
		aCount = mGameStats->mScrambles;
	for (int i = 0; i < aCount; i++)
	{
		unsigned int anIndex = gGameRand->Next() % (unsigned int)aCandidates.size();
		Piece* aPiece = aCandidates[anIndex];
		if (aPiece == NULL)
			continue;

		aPiece->SetType(TILE_FIRE, true);
		mFireTileCount++;
		if (mSelection.size() != 0)
		{
			mWord.Build(&mSelection);
			mWord.CheckValid(&mApp->mDictionary);
		}
		if (!mFlameSoundPlayed)
		{
			mFlameSoundPlayed = true;
			mApp->PlaySample(SOUND_FLAMETILE);
			mLex->PlayAnim(LEXANIM_SCRAMBLE, false);
			mApp->PlaySample(SOUND_LEX_SCRAMBLE);
		}
		aCandidates.erase(aCandidates.begin() + anIndex);

		// 100 sparks and embers rising from the new fire tile, six mRand draws each
		for (int j = 0; j < 100; j++)
		{
			double aDecay = (double)(mRand->Next() % 1000) * 0.001 * 0.05 + 0.02;
			double aVY = ((double)(mRand->Next() % 1000) - 500.0) * 0.004;
			double aVX = ((double)(mRand->Next() % 1000) - 500.0) * 0.004;
			double aY = (double)(mRand->Next() % 57) + aPiece->mY;
			double aX = (double)(mRand->Next() % 57) + aPiece->mX;
			int aType = (mRand->Next() % 100 < 80) ? EMBER_SPARK_SM : EMBER_EMBER_SM;
			mEmberMgr->AddEmber(aType, aX, aY, aVX, aVY, false, 1.0, aDecay);
		}
	}

	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		for (int aRow = 7; aRow >= TopRow(aCol); aRow--)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				continue;

			if (aPiece->mType == TILE_FIRE)
				aPiece->mNextLetter = aPiece->mLetter;
			else
			{
				aPiece->SetType(TILE_NORMAL, false);
				aPiece->mNextLetter = (char)RandomLetter();
				if (mSelection.size() != 0)
				{
					mWord.Build(&mSelection);
					mWord.CheckValid(&mApp->mDictionary);
				}
			}
		}
	}

	if ((!mApp->mScrambleHintShown) && (mHintDelay == 0))
	{
		mHintDelay = 100;
		mHintId = HINT_SCRAMBLE;
		mApp->mScrambleHintShown = true;
	}

	BurnTiles(mApp->mGameMode == GAMEMODE_CLASSIC);
	Refill();
	CheckBottomFire();
	mState = BOARD_SCRAMBLING;
	mScrambleProgress = 0.0;
	mFlameSoundPlayed = false;
	MarkDirty();
}

// --------------------------------------------------------------------------------------------------------
// Special tiles (board_logic.md 4.5)

// 0x40ba80: a special tile for a long word or a word with special tiles, on a random normal tile
void Board::AwardSpecialTile(const WordScore& theWord)
{
	int aLength = (int)theWord.mWord.size();
	unsigned int aType = TILE_NORMAL;

	if (aLength >= 11)
		aType = TILE_DIAMOND;
	else if (aLength == 10)
		aType = (gGameRand->Next() & 1) | TILE_SAPPHIRE;
	else if (aLength == 9)
		aType = TILE_SAPPHIRE;
	else if (aLength == 8)
		aType = ((gGameRand->Next() & 1) != 0) ? TILE_SAPPHIRE : TILE_GOLD;
	else if (aLength == 7)
		aType = ((gGameRand->Next() & 3) != 0) ? TILE_SAPPHIRE : TILE_GOLD;
	else if (aLength == 6)
	{
		if (gLen6Gate < 6)
			aType = TILE_GOLD;
		else
			aType = ((gGameRand->Next() & 1) != 0) ? TILE_SAPPHIRE : TILE_GOLD;
	}
	else if (aLength == 5)
	{
		if (gGameRand->Next() % 5 == 0)
			aType = TILE_GOLD;
	}
	else if (aLength == 4)
	{
		if (gGameRand->Next() % 100 == 0)
			aType = TILE_GOLD;
	}

	if (theWord.mTypeCount[TILE_DIAMOND] > 1)
	{
		mLexPraisePending = true;
		mLexPraiseType = (int)aType;
	}

	// combinations of special tiles in the word, then the streak of rewarded words (registered builds)
	bool isRewarded;
	if (theWord.mTypeCount[TILE_SAPPHIRE] >= 2)
	{
		aType = TILE_DIAMOND;
		isRewarded = true;
	}
	else if (theWord.mTypeCount[TILE_GOLD] > 2)
	{
		aType = TILE_DIAMOND;
		isRewarded = true;
	}
	else if (theWord.mTypeCount[TILE_GOLD] == 2)
	{
		if (aType < TILE_DIAMOND)
			aType = TILE_SAPPHIRE;
		isRewarded = true;
	}
	else
	{
		if (theWord.mTypeCount[TILE_GREEN] > 2)
		{
			unsigned int aLimit = ((gGameRand->Next() & 1) != 0) ? TILE_SAPPHIRE : TILE_GOLD;
			if (aType <= aLimit)
				aType = ((gGameRand->Next() & 1) != 0) ? TILE_SAPPHIRE : TILE_GOLD;
		}
		isRewarded = aType != TILE_NORMAL;
	}

	if ((isRewarded) && (mApp->mIsRegistered))
		mRewardStreak++;
	else
		mRewardStreak = 0;

	if (mRewardStreak >= 5)
	{
		if (aType < 6)
			aType = TILE_DIAMOND;
	}
	else if ((mRewardStreak > 2) && (aType < TILE_DIAMOND))
		aType = TILE_SAPPHIRE;
	else if (aType == TILE_NORMAL)
		return;

	std::vector<Piece*> aCandidates;
	GetNormalPieces(&aCandidates);
	if (aCandidates.size() == 0)
		return;

	Piece* aPiece = aCandidates[gGameRand->Next() % (unsigned int)aCandidates.size()];
	if (aPiece == NULL)
		return;

	aPiece->SetType((int)aType, true);
	if (mSelection.size() != 0)
	{
		mWord.Build(&mSelection);
		mWord.CheckValid(&mApp->mDictionary);
	}
	mApp->PlaySample(SOUND_SPINSUPERBONUS);
	mLexPraisePending = true;
	mLexPraiseType = (int)aType;
	aPiece->mNextLetter = aPiece->mLetter;
	aPiece->mFlipProgress = 0.0;
	aPiece->mFlipping = true;
}

// --------------------------------------------------------------------------------------------------------
// Selection and words (board_logic.md 4.3, 4.4)

// 0x409c60: appends a tile (a tile already selected first cuts the selection back to before it), rebuilds
// the word, colours the submit button and plays bonuscomplete when the tile completes a valid word with special tiles
void Board::AddLetter(Piece* thePiece)
{
	PieceList::iterator anItr = std::find(mSelection.begin(), mSelection.end(), thePiece);
	if (anItr != mSelection.end())
	{
		std::for_each(anItr, mSelection.end(), Piece::Deselect);
		mSelection.erase(anItr, mSelection.end());
	}

	bool wasSelected = thePiece->mSelected;
	thePiece->mSelected = true;
	mSelection.push_back(thePiece);
	mWord.Build(&mSelection);

	bool hasSpecial = (mWord.mTypeCount[TILE_GREEN] > 0) || (mWord.mTypeCount[TILE_GOLD] > 0) ||
		(mWord.mTypeCount[TILE_SAPPHIRE] > 0) || (mWord.mTypeCount[TILE_DIAMOND] > 0);
	mWord.CheckValid(&mApp->mDictionary);

	if (!mWord.mValid)
		mSubmitButton->SetColors(gSubmitColorsInvalid, 2);
	else
		mSubmitButton->SetColors(gSubmitColorsValid, 2);

	if ((mWord.mValid) && (!wasSelected) && (hasSpecial))
		mApp->PlaySample(SOUND_BONUSCOMPLETE);
	else
		mApp->PlaySample(SOUND_NEXTSELECT);
	MarkDirty();
}

// port(mod): the selection becomes thePath, as clicks on its tiles one after another make it (MouseDown's
// AddLetter, without its sound per tile): the old selection is dropped, the word built and checked, SUBMIT
// lit for a word, and a click on the last tile submits it (SELECT_CLICKED)
void Board::SelectTiles(const std::vector<Piece*>& thePath)
{
	std::for_each(mSelection.begin(), mSelection.end(), Piece::Deselect);
	mSelection.clear();
	for (size_t i = 0; i < thePath.size(); i++)
	{
		thePath[i]->mSelected = true;
		mSelection.push_back(thePath[i]);
	}
	mWord.Build(&mSelection);
	mWord.CheckValid(&mApp->mDictionary);
	mSubmitButton->SetColors(mWord.mValid ? gSubmitColorsValid : gSubmitColorsInvalid, 2);
	// ClearWord, after the last word, marks the button not under the mouse even where it still is; a
	// button takes a click only under the mouse, so a hint asked for by key and then Submit clicked without
	// leaving the button would do nothing
	mSubmitButton->mIsOver = (mWidgetManager != NULL) && (mWidgetManager->mOverWidget == mSubmitButton);
	mSelectMode = SELECT_CLICKED;
	mApp->PlaySample(SOUND_NEXTSELECT);
	MarkDirty();
}

// 0x405cf0: deselects and drops every tile after thePiece
void Board::TruncateAfter(Piece* thePiece)
{
	PieceList::iterator anItr = std::find(mSelection.begin(), mSelection.end(), thePiece);
	if (anItr == mSelection.end())
		return;

	++anItr;
	std::for_each(anItr, mSelection.end(), Piece::Deselect);
	mSelection.erase(anItr, mSelection.end());
}

// 0x407bf0: empties the selection (without deselecting the tiles) and the word
void Board::ClearWord()
{
	mSelection.clear();
	mWord.Clear();
	mSelectMode = SELECT_NONE;
	mSubmitButton->SetColors(gSubmitColorsInvalid, 2);
	mSubmitButton->mIsOver = false;
	MarkDirty();
}

// 0x40f5b0: scores the selection when it is a valid word (definition, stats, log, fire meter, bonus word,
// the tiles fly to Lex, the level bar, refill, special tile, popup); an invalid word buzzes when a submit
// was requested
void Board::Submit(int x, int y)
{
	if (((int)mWord.mWord.size() < 9) || (mApp->mIsRegistered))
	{
		if (!mWord.mValid)
		{
			if ((mSelectMode == SELECT_SUBMIT) && (mSelection.size() != 0))
				mApp->PlaySample(SOUND_BUZZER);
			mSelectMode = SELECT_CLICKED;
		}
		else
		{
			if (mUndo != NULL)
				mUndo->Push();  // port(mod): the board before the word
			if ((mApp->mShowDefs) && (mApp->mDefinitionator != NULL))
			{
				SexyString aDefinition = mApp->mDefinitionator->GetDefinition(mWord.mWord);
				if (aDefinition.size() != 0)
					mLex->SayDefinition(mWord.mWord, mApp->mFonts[FONT_MYRIADWEB12BOLD], aDefinition,
						mApp->mFonts[FONT_MYRIADWEB10], (int)aDefinition.size() * 3 + 400);
			}

			if (mSubmitBlinkWords > 0)
				mSubmitBlinkWords--;

			// play counting: never in the retail build (MaxTime 60, NAMES.md D49 / L6)
			if ((!mPlayCounted) && (mApp->mMaxTime == 0) && (mApp->mMaxPlays == 0))
			{
				mApp->mTimesPlayed++;
				mApp->mUserTimesPlayed++;
				mPlayCounted = true;
			}

			mLevelStats->AddWord(mWord);
			mGameStats->AddWord(mWord);

			// log: the word, and the tile types when it has special tiles
			int aLength = (int)mWord.mWord.size();
			mGameLog.WriteNumBits(0xd, 4);
			mGameLog.WriteLong(mWord.mScore);
			mGameLog.WriteNumBits(aLength, 4);
			for (int i = 0; i < aLength; i++)
				mGameLog.WriteNumBits((char)mWord.mWord[i] - 'A', 5);
			if (mWord.mTypeCount[TILE_NORMAL] < mWord.mNumPieces)
			{
				mGameLog.WriteNumBits(0xc, 4);
				for (PieceList::iterator anItr = mSelection.begin(); anItr != mSelection.end(); ++anItr)
				{
					if (*anItr != NULL)
						mGameLog.WriteNumBits((*anItr)->mType, 3);
				}
			}

			if (mWord.mTypeCount[TILE_FIRE] > 1)
				mBurpPending = true;

			// fire meter: a 3-letter word lowers it unless it used fire tiles
			unsigned int aLengthIndex = (unsigned int)(aLength - 3);
			unsigned int aMeterIndex = (aLengthIndex > 4) ? 5 : aLengthIndex;
			if ((mWord.mTypeCount[TILE_FIRE] == 0) || (gFireMeterByLength[aMeterIndex] > 0.0))
				mFireMeter = gFireMeterByLength[aMeterIndex] + mFireMeter;

			if (_stricmp(mWord.mWord.c_str(), mBonusWord.c_str()) == 0)
			{
				unsigned int aBonusIndex = (unsigned int)mGameStats->mBonusWords;
				int aPoints = gBonusWordPoints[(aBonusIndex > 9) ? 10 : aBonusIndex];
				BonusWordScored(aPoints, x, y);
				mSkipRandomFire = true;
				mFireMeter = (double)aLength + mFireMeter;
				mGameLog.WriteNumBits(0xb, 4);
				mGameLog.WriteLong(aPoints);
			}

			mLastWordAlpha = 1.0;
			mLastWord = mWord;

			BurnTiles(mApp->mGameMode == GAMEMODE_CLASSIC);
			mUpSoundIdx = 0;
			mFireMeter = mFireMeter - 1.0;
			mUpSoundCount = aLength;
			mUpSoundDelay = 20;
			if (mWord.mTypeCount[TILE_FIRE] == 0)
				mApp->PlaySample(SOUND_WORDCOMPLETE);
			else
				mApp->PlaySample(SOUND_REDTILECLEARED);

			// the tiles leave the grid and fly to Lex (the FlyingPieceList owns them now)
			RemovePieces(&mSelection);
			FlyingPieceList* aFlyingPieces = new FlyingPieceList(mOverlay->mEffectMgr, &mSelection);
			mOverlay->AddEffect(aFlyingPieces);

			int aPrevThreshold = 0;
			if (mLevel != 0)
				aPrevThreshold = LevelThreshold((unsigned int)mLevel - 1);
			int aThreshold = LevelThreshold((unsigned int)mLevel);
			mLevelProgress = (double)mLevelStats->mScore / (double)(aThreshold - aPrevThreshold);
			if ((mGameStats->mScore >= aThreshold) && (mState == BOARD_PLAYING))
			{
				mOverlay->mBalloon->Show(0);
				mState = BOARD_LEVEL_COMPLETE;
				mLevelUpPending = true;
			}

			Refill();
			AwardSpecialTile(mWord);
			AddScorePopup(mWord.mScore, x, y);
			ClearWord();
			CheckBottomFire();
			mSelectMode = SELECT_NONE;
		}
	}
	else
	{
		// not ported: the unregistered build's dictionary upsell dialog (dialog 17, sets mRegSource) for
		// words of 9+ letters; the retail build is registered
	}

	mFlameSoundPlayed = false;
	MarkDirty();
}

// 0x40abd0: bonus word message and stats, the next bonus word is picked and the banner slides away
void Board::BonusWordScored(int thePoints, int x, int y)
{
	mApp->PlaySample(SOUND_BONUSWORD);
	mLex->PlayAnim(LEXANIM_BONUS, false);
	ShowMessage(StrFormat(GameTextC(GT_BOARD_BONUS_FORMAT), thePoints), y + 60);
	mLevelStats->AddBonusWord(thePoints);
	mGameStats->AddBonusWord(thePoints);

	if (mBonusWord != "")
	{
		mApp->mUsedBonusWords.insert(mBonusWord);
		PickBonusWord();
		mBonusWord = "";
	}

	if (mOverlay != NULL)
	{
		ShowOverlay(false);
		mOverlay->mBannerTargetY = mHeight;
	}
}

// 0x408030 (also the app's StartGame): the next bonus word, of the length the count of words handed out
// asks for and not the current one, into the app's mBonusWord (the banner picks it up in Update); gives up
// silently after as many tries as there are words
void Board::PickBonusWord()
{
	unsigned int aGivenIndex = (unsigned int)mBonusWordsGiven;
	int aWantedLength = gBonusWordLength[(aGivenIndex > 8) ? 9 : aGivenIndex];
	int aTries = (int)mApp->mBonusWords.size();

	SexyString aWord = "";
	while (aTries >= 1)
	{
		unsigned int aCount = (unsigned int)mApp->mBonusWords.size();
		aWord = mApp->mBonusWords[gGameRand->Next() % aCount];
		if ((_stricmp(aWord.c_str(), mApp->mBonusWord.c_str()) != 0) && ((int)aWord.size() == aWantedLength))
		{
			mApp->mBonusWord = aWord;
			mBonusWordsGiven++;
			return;
		}
		aTries--;
	}
}

// 0x409970: the "+N" popup of a word at (x, y) on the board; lives score / 3 ticks (150..600)
void Board::AddScorePopup(int theScore, int x, int y)
{
	int aShownScore = (int)((double)theScore * mScoreScale);
	int aLife = (int)((double)theScore * 0.3333333333333333);
	if (aLife > 600)
		aLife = 600;
	else if (aLife < 150)
		aLife = 150;

	Points* aPopup = new Points(mApp, mApp->mFonts[FONT_GOLDSCORE], StrFormat("+%d", aShownScore), mX + x, mY + y,
		aLife, Points::JUSTIFY_CENTER);
	mPopups.push_back(aPopup);
	mWidgetManager->AddWidget(aPopup);

	if (mBestWordThisLevel < aShownScore)
		mBestWordThisLevel = aShownScore;
	if (mBestWordThisGame < aShownScore)
		mBestWordThisGame = aShownScore;
	mPointsAwarded += aShownScore;
}

// 0x409b70: a left-aligned message at x 233; stacks under the previous one while that one lives
void Board::ShowMessage(const SexyString& theText, int theY)
{
	int aY = theY;
	if ((mLastMessage != NULL) && (mLastMessage->mLife > 0) && (theY < mLastMessage->mY + 36))
		aY = mLastMessage->mY + 36;

	Points* aPopup = new Points(mApp, mApp->mFonts[FONT_LOCKERGNOMESMALLCOLOR], theText, 233, mY + aY, 200, -1);
	mPopups.push_back(aPopup);
	mWidgetManager->AddWidget(aPopup);
	mLastMessage = aPopup;
}

// 0x405a80
void Board::ClearPopups()
{
	for (std::list<Points*>::iterator anItr = mPopups.begin(); anItr != mPopups.end(); ++anItr)
	{
		Points* aPopup = *anItr;
		mWidgetManager->RemoveWidget(aPopup);
		mApp->SafeDeleteWidget(aPopup);
	}
	mPopups.clear();
	// port: the original keeps mLastMessage pointing at the deleted popup (ShowMessage would read it)
	mLastMessage = NULL;
}

// --------------------------------------------------------------------------------------------------------
// Input (board_logic.md 4.3). Mouse input needs: not paused, 15 updates since the last unpause (app
// mResumeUpdateCnt), and play.

// 0x409de0 slot 27: a click while paused resumes. Right button: clear. Otherwise select / extend / cut the
// selection, or request a submit (double click, click on the last tile).
void Board::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);

	if (mPauseCount >= 1)
	{
		Pause(false);
		if (mPauseCount == 0)
		{
			mApp->PauseAllMusic(false);
			mApp->mResumeUpdateCnt = mApp->mUpdateCount;
		}
		return;
	}

	if ((mApp->mUpdateCount - mApp->mResumeUpdateCnt <= 14) || (mState != BOARD_PLAYING))
		return;

	if (theClickCount < 0)
	{
		std::for_each(mSelection.begin(), mSelection.end(), Piece::Deselect);
		ClearWord();
		mLastWord = mWord;
		return;
	}

	Piece* aPiece = PieceAt(x, y);
	if (aPiece == NULL)
		return;

	// nothing while a tile below it in its column burns away
	for (int aRow = aPiece->mRow; aRow < BOARD_ROWS; aRow++)
	{
		Piece* aBelow = mBoard[aRow][aPiece->mCol];
		if ((aBelow != NULL) && (aBelow->mBurning))
			return;
	}

	if (mSelectMode != SELECT_NONE)
	{
		if (theClickCount > 1)
		{
			if (mSelection.size() < 2)
			{
				// mWord is not cleared: mLastWord gets the stale word
				std::for_each(mSelection.begin(), mSelection.end(), Piece::Deselect);
				mSelection.clear();
				mLastWord = mWord;
				return;
			}
			mSelectMode = SELECT_SUBMIT;
			return;
		}

		if (mSelection.size() != 0)
		{
			if (aPiece == mSelection.front())
			{
				if (mSelection.size() < 2)
				{
					std::for_each(mSelection.begin(), mSelection.end(), Piece::Deselect);
					mSelection.clear();
					mSelectMode = SELECT_NONE;
					return;
				}
				AddLetter(aPiece);
				mSelectMode = SELECT_NONE;
				return;
			}

			if (aPiece == mSelection.back())
			{
				if (mSelection.size() < 2)
				{
					std::for_each(mSelection.begin(), mSelection.end(), Piece::Deselect);
					mSelection.clear();
					return;
				}
				mSelectMode = SELECT_SUBMIT;
				return;
			}

			// from the end back: cut after the first tile next to the clicked one (or at it, when selected)
			PieceList::iterator anItr = mSelection.end();
			while (anItr != mSelection.begin())
			{
				--anItr;
				Piece* aSelected = *anItr;
				if ((IsAdjacent(aSelected, aPiece)) || (aPiece->mSelected))
				{
					TruncateAfter(aSelected);
					AddLetter(aPiece);
					return;
				}
			}

			std::for_each(mSelection.begin(), mSelection.end(), Piece::Deselect);
			mSelection.clear();
		}
	}

	AddLetter(aPiece);
}

// 0x40a0c0 slot 31: dragging onto a tile next to the last one adds it (no selected test: dragging back
// over a selected tile cuts the selection to it)
void Board::MouseDrag(int x, int y)
{
	Widget::MouseDrag(x, y);

	if ((mPauseCount < 1) && (mApp->mUpdateCount - mApp->mResumeUpdateCnt > 14) && (mState == BOARD_PLAYING) &&
		(mSelection.size() != 0))
	{
		Piece* aPiece = PieceAt(x, y);
		Piece* aLast = mSelection.back();
		if ((aPiece != NULL) && (aLast != aPiece))
		{
			mSelectMode = SELECT_DRAGGING;
			if ((IsAdjacent(aPiece, aLast)) && (!aPiece->mBurning))
				AddLetter(aPiece);
		}
	}
}

// 0x40fc40 slot 29: releasing off the board (or the right button) clears; a click selection waits for a
// second click; a drag or a submit request submits at the last tile's centre
void Board::MouseUp(int x, int y, int theClickCount)
{
	Widget::MouseUp(x, y, theClickCount);
	if (mPauseCount > 0)
		return;

	if (mApp->mUpdateCount - mApp->mResumeUpdateCnt <= 14)
		return;

	if (mState == BOARD_PLAYING)
	{
		if ((theClickCount < 0) || (PieceAt(x, y) == NULL))
		{
			std::for_each(mSelection.begin(), mSelection.end(), Piece::Deselect);
			mSelection.clear();
			mLastWord = mWord;
		}
		else if (theClickCount > 0)
		{
			if (mSelectMode == SELECT_NONE)
			{
				if (mSelection.size() != 0)
				{
					mSelectMode = SELECT_CLICKED;
					MarkDirty();
					return;
				}
			}
			else if (((mSelectMode == SELECT_DRAGGING) || (mSelectMode == SELECT_SUBMIT)) && (mSelection.size() != 0))
			{
				Piece* aLast = mSelection.back();
				if (aLast == NULL)
					Submit(x, y);
				else
					Submit((int)(aLast->mX + 28.0), (int)(aLast->mY + 28.0));
				MarkDirty();
				return;
			}
		}
	}

	MarkDirty();
}

// 0x4081d0 listener slot 2: the click sound; a right click on the submit button clears the selection
void Board::ButtonPress(int theId, int theClickCount)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
	if ((theClickCount < 0) && (theId == BOARD_BUTTON_SUBMIT))
	{
		std::for_each(mSelection.begin(), mSelection.end(), Piece::Deselect);
		ClearWord();
		mLastWord = mWord;
	}
}

// 0x40fdc0 listener slot 4: while the level summary is up any button goes on to the next level
void Board::ButtonDepress(int theId)
{
	if ((mState != BOARD_GAMEOVER) && (mSummaryShown))
	{
		mState = BOARD_NEXT_LEVEL;
		return;
	}

	if (theId == BOARD_BUTTON_SUBMIT)
	{
		if ((mSelection.size() != 0) && (mSelection.back() != NULL))
		{
			Piece* aLast = mSelection.back();
			int aX = (int)(aLast->mX + 28.0);
			int aY = (int)(aLast->mY + 28.0);
			mSelectMode = SELECT_SUBMIT;
			Submit(aX, aY);
			mSelectMode = SELECT_CLICKED;
		}
	}
	else if (theId == BOARD_BUTTON_OPTIONS)
		mApp->ShowOptions();
	else if ((theId == BOARD_BUTTON_HINT) && (mSolver != NULL))
		mSolver->RequestHint();     // port(mod)
	else if ((theId == BOARD_BUTTON_UNDO) && (mUndo != NULL))
		mUndo->RequestUndo();       // port(mod)
	else if ((theId == BOARD_BUTTON_LEAVE) && (IsInProgress()))
		mApp->GoToMainMenu();
}

// --------------------------------------------------------------------------------------------------------
// Overlay, pause, levels (board_logic.md 4.1, 4.8)

// 0x403d80: slides the bonus-word banner in (406) or out (the board's height)
void Board::ShowOverlay(bool show)
{
	if (mOverlay == NULL)
		return;

	if (mOverlay->mBannerY == 0)
		mOverlay->mBannerY = mHeight;
	if (show)
	{
		mOverlay->mBannerTargetY = 406;
		MarkDirty();
		mOverlay->mBannerActive = true;
	}
	else
		mOverlay->mBannerTargetY = mHeight;
	mOverlay->mArrowFade = 10.0;
}

// 0x405be0: counted pause; the popups hide while paused; the level clock is banked at the first pause and
// restarts at the last unpause
void Board::Pause(bool pause)
{
	int anOldCount = mPauseCount;
	if (pause)
		mPauseCount = anOldCount + 1;
	else
	{
		mPauseCount = anOldCount - 1;
		if (mPauseCount < 0)
			mPauseCount = 0;
	}

	for (std::list<Points*>::iterator anItr = mPopups.begin(); anItr != mPopups.end(); ++anItr)
		(*anItr)->SetVisible(mPauseCount == 0);

	if (mPauseCount < 1)
	{
		if (anOldCount > 0)
		{
			SetButtonsDisabled(false);
			mLevelStartTick = GameTickCount();
		}
	}
	else if (anOldCount < 1)
	{
		SetButtonsDisabled(true);
		mLevelSecondsBanked = (int)((unsigned long)mLevelSecondsBanked +
			((unsigned long)GameTickCount() - mLevelStartTick) / 1000);
	}
	MarkDirty();
}

// 0x403ae0: the pause shade fades in while paused, out after
void Board::UpdatePauseFade()
{
	if (mPauseCount < 1)
	{
		if (mPauseFade > 0.0)
		{
			mPauseFade = mPauseFade - 0.05;
			if (mPauseFade < 0.0)
				mPauseFade = 0.0;
			MarkDirty();
		}
	}
	else
	{
		mPauseNoDialogs = (mApp->GetDialogCount() == 0) && (mApp->mHelpWidget == NULL);
		if (mPauseFade < 1.0)
		{
			mPauseFade = mPauseFade + 0.05;
			if (mPauseFade > 1.0)
				mPauseFade = 1.0;
			MarkDirty();
		}
	}
}

// 0x403ea0
void Board::RemoveLevelUpWidget()
{
	if (mLevelUpWidget == NULL)
		return;

	mApp->mWidgetManager->RemoveWidget(mLevelUpWidget);
	delete mLevelUpWidget;
	mLevelUpWidget = NULL;
}

// 0x404b40: closes the level clock and shows the level / game summary (it follows the help screen's state)
void Board::ShowSummary()
{
	mApp->mLevelReached = true;
	RemoveLevelUpWidget();

	int aLevelSeconds = (int)((unsigned long)mLevelSecondsBanked +
		((unsigned long)GameTickCount() - mLevelStartTick) / 1000);
	mLevelSeconds = aLevelSeconds;
	mTotalSeconds += aLevelSeconds;

	mLevelUpWidget = new LevelUpWidget(mApp);
	mLevelUpWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mLevelUpWidget);
	mWidgetManager->SetFocus(mLevelUpWidget);
	mSummaryShown = true;

	if (mApp->mHelpWidget != NULL)
	{
		mLevelUpWidget->SetDisabled(mDisabled);
		mLevelUpWidget->SetVisible(mVisible);
	}
}

// 0x407a50: next level: the level stats restart with the score carried over the threshold
void Board::LevelUp()
{
	mLevelStats->Reset();
	mLevelStats->mScore = mGameStats->mScore - LevelThreshold((unsigned int)mLevel);
	mLevel++;
	mGameLog.WriteNumBits(0xe, 4);
	mGameLog.WriteShort((short)mLevel);
	mLevelProgress = 0.0;
	mActionFireTimer = 1000;

	if (mApp->mGameMode == GAMEMODE_ACTION)
	{
		SetActionSpeed((double)gActionSpeed[((unsigned int)mLevel > 8) ? 9 : mLevel]);
		mLexGreetPending = true;
	}

	mState = BOARD_PLAYING;
	int aPrevThreshold = 0;
	if (mLevel != 0)
		aPrevThreshold = LevelThreshold((unsigned int)mLevel - 1);
	int aThreshold = LevelThreshold((unsigned int)mLevel);
	mShownProgress = 0.0;
	mBestWordThisLevel = 0;
	mUnused3e8 = 0;
	mLevelProgress = (double)mLevelStats->mScore / (double)(aThreshold - aPrevThreshold);
}

// 0x408000
void Board::NextLevel()
{
	mSummaryShown = false;
	mLevelSecondsBanked = 0;
	mLevelStartTick = GameTickCount();
	LevelUp();
}

// 0x405cd0 (the app's ShowGameSelector)
void Board::Abandon()
{
	ClearPopups();
	mState = BOARD_INACTIVE;
}

// 0x403720
bool Board::IsInProgress()
{
	return mState != BOARD_GAMEOVER;
}

// --------------------------------------------------------------------------------------------------------
// The tick (board_logic.md 4.7; step numbers in the comments)

// 0x40bd40 slot 17
void Board::Update()
{
	// 1.
	if (mState == BOARD_INACTIVE)
		return;

	UpdatePauseFade();
	if (mPauseCount > 0)
		return;

	// 2.
	mWidgetManager->BringToBack(this);
	mPiecesFlying = false;
	Widget::Update();
	if (mSolver != NULL)
		mSolver->Update();  // port(mod): a hint asked for while the tiles moved
	if (mUndo != NULL)
		mUndo->Update();    // port(mod): an undo asked for while the tiles moved
	mApp->mPlayTicks++;

	PieceList aDeadPieces;  // tiles burned away this tick, deleted at the end

	// 3. playing only: bonus-word banner, letter highlight blink, Lex's danger talk, action countdown, greeting
	if (mState == BOARD_PLAYING)
	{
		if ((mBonusWord.size() == 0) && ((mLevel > 1) || ((mLevel == 1) && (mLevelStats->mWords >= 1))) &&
			(!mOverlay->IsBannerMoving()))
		{
			mBonusWord = mApp->mBonusWord;
			if (mBonusWord.size() != 0)
				ShowOverlay(true);
		}

		if ((mHighlightKey != 0) && (mUpdateCnt % 32 == 0))
			MarkDirty();

		if (!mLex->mSweating)
			mLexDangerTalkReady = true;
		else
		{
			if ((mLexDangerTalkReady) && (!mLex->IsMoving()))
			{
				mLexDangerTalkReady = false;
				switch (mRand->Next() & 3)
				{
				case 0:
					mLex->PlayAnim(LEXANIM_CAREFUL, false);
					mApp->PlaySample(SOUND_LEX_CAREFUL);
					break;
				case 1:
					mLex->PlayAnim(LEXANIM_DANGER, false);
					mApp->PlaySample(SOUND_LEX_DANGER);
					break;
				case 2:
					mLex->PlayAnim(LEXANIM_LOOKOUT, false);
					mApp->PlaySample(SOUND_LEX_LOOKOUT);
					break;
				case 3:
					mLex->PlayAnim(LEXANIM_WATCHOUT, false);
					mApp->PlaySample(SOUND_LEX_WATCHOUT);
					break;
				}
			}

			if (!mLex->IsBusy())
				mLex->PlayAnim(LEXANIM_SWEAT, true);
		}

		if (mApp->mGameMode == GAMEMODE_ACTION)
		{
			if (!mLex->mSweating)
			{
				if (mActionCountdown < 10)
					mActionCountdown = 10;
			}
			else if (mUpdateCnt % 100 == 0)
			{
				ShowMessage(StrFormat("%d", mActionCountdown), 180);
				mApp->PlaySample(SOUND_WARNING);
				if (--mActionCountdown < 0)
				{
					std::for_each(mSelection.begin(), mSelection.end(), Piece::Deselect);
					ClearWord();
				}
			}
		}

		if ((mLexGreetPending) && (!mLex->IsMoving()))
		{
			mLexGreetPending = false;
			if (mApp->mGameMode == GAMEMODE_ACTION)
			{
				mLex->PlayAnim(LEXANIM_GETREADY, false);
				mApp->PlaySample(SOUND_LEX_GETREADY);
			}
			else if (mRand->Next() % 100 < 50)
			{
				mLex->PlayAnim(LEXANIM_GO, false);
				mApp->PlaySample(SOUND_LEX_GO);
			}
			else
			{
				mLex->PlayAnim(LEXANIM_LETSGO, false);
				mApp->PlaySample(SOUND_LEX_LETSGO);
			}
		}
	}

	// 4. the summary's continue button asked for the next level
	mPiecesMoving = false;
	if ((mState == BOARD_NEXT_LEVEL) && (mApp->GetDialogCount() == 0))
	{
		mSummaryShown = false;
		NextLevel();
		if (mActionCountdown < 10)
		{
			mActionCountdown = 10;
			if ((mApp->mGameMode == GAMEMODE_ACTION) && (mLex->mSweating))
			{
				ShowMessage(GameText(GT_BOARD_TIME_BONUS), 160);
				mLex->PlayAnim(LEXANIM_BONUS, false);
				mApp->PlaySample(SOUND_WORDCOMPLETE);
			}
		}
		mState = BOARD_PLAYING;
	}
	else if ((mState == BOARD_PLAYING) || (mState == BOARD_LEVEL_COMPLETE) || (mState == BOARD_GAMEOVER_PENDING) ||
		(mState == BOARD_GAMEOVER))
	{
		// 5. the tile pass
		bool aBurn = false;             // BurnTiles this tick
		bool aRefill = false;           // a tile left the grid or caught fire
		bool aBurnSoundPlayed = false;  // tileburn at most once per tick
		bool aFireAtBottom = false;     // a fire tile sits on the bottom row: danger

		if ((mState == BOARD_PLAYING) && (mApp->mGameMode == GAMEMODE_ACTION))
		{
			mActionFireTimer--;
			if (mActionCountdown < 0)
			{
				aBurn = true;
				aRefill = true;
			}

			if (mActionFireTimer <= 0)
			{
				// timed fire: a top-row tile picked by letter frequency (normal or green) will catch fire
				aBurn = true;
				aRefill = true;
				mActionFireTimer = 1000;

				unsigned int aTotal = 0;
				for (int aCol = 0; aCol < BOARD_COLS; aCol++)
				{
					Piece* aPiece = mBoard[TopRow(aCol)][aCol];
					if (aPiece != NULL)
						aTotal += (unsigned int)LetterWeight(aPiece->mLetter);
				}

				int aPick = (int)(gGameRand->Next() % aTotal);
				bool isPicked = false;
				for (int aPass = 0; (aPass < 7) && (!isPicked); aPass++)
				{
					// a hit on a tile of another type restarts the scan from column 0 (up to 7 passes); the
					// first tile then hits again
					for (int aCol = 0; aCol < BOARD_COLS; aCol++)
					{
						Piece* aPiece = mBoard[TopRow(aCol)][aCol];
						if (aPiece == NULL)
							continue;
						aPick -= LetterWeight(aPiece->mLetter);
						if (aPick < 1)
						{
							if ((aPiece->mType == TILE_NORMAL) || (aPiece->mType == TILE_GREEN))
							{
								aPiece->mPendingFire = true;
								isPicked = true;
							}
							break;
						}
					}
				}
			}
		}

		// The submit button blinks for the first valid words of the game
		if ((mSubmitBlinkWords > 0) && (mWord.mValid) && (mSelectMode != SELECT_DRAGGING) && (mUpdateCnt % 32 == 0))
		{
			if (mSubmitButton->GetColor(0) == Color(&gSubmitColorsInvalid[0][0]))
				mSubmitButton->SetColors(gSubmitColorsValid, 2);
			else
				mSubmitButton->SetColors(gSubmitColorsInvalid, 2);
			MarkDirty();
		}

		bool hasSpecial = false;
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			int aTargetY = RowY(aCol, 7);
			for (int aRow = 7; aRow >= 0; aRow--)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if (aPiece == NULL)
					continue;

				// a.
				if (aPiece->mType != TILE_NORMAL)
					hasSpecial = true;

				// b. action: a tile about to catch fire smokes; it ignites 334 ticks after it was picked
				if (aPiece->mPendingFire)
				{
					if ((mRand->Next() & 7) == 0)
					{
						double aDecay = (double)(mRand->Next() % 1000) * 0.001 * 0.025 + 0.01;
						double aVY = ((double)(mRand->Next() % 1000) - 1000.0) * 0.0006666666666666666;
						double aVX = ((double)(mRand->Next() % 1000) - 500.0) * 0.00125;
						double aY = (aPiece->mY + 57.0) - 3.0;
						double aX = (double)(mRand->Next() % 57) + aPiece->mX;
						int aType = (mRand->Next() % 100 >= 80) ? EMBER_SPARK_MED : EMBER_SPARK_SM;
						Ember* anEmber = AddEmberGet(mEmberMgr, aType, aX, aY, aVX, aVY, true, 1.0, aDecay);
						double aPuffDecay = (double)(mRand->Next() % 1000) * 0.001 * 0.01 + 0.004;
						double aPuffAlpha = (double)(mRand->Next() % 1000) * 0.001;
						Ember* aPuff = AddEmberGet(mEmberMgr, EMBER_SMOKE_PUFF, anEmber->mX - 8.0, anEmber->mY - 8.0, 0.0,
							-0.25, false, aPuffAlpha, aPuffDecay);
						aPuff->mGravity = 0.0;
					}

					if (mActionFireTimer < 666)
					{
						aPiece->mPendingFire = false;
						aPiece->SetType(TILE_FIRE, true);
						if (mSelection.size() != 0)
						{
							mWord.Build(&mSelection);
							mWord.CheckValid(&mApp->mDictionary);
						}
						aPiece->mNextLetter = aPiece->mLetter;
						aPiece->mFlipProgress = 0.0;
						aPiece->mFlipping = true;
						if (!mFlameSoundPlayed)
						{
							mFlameSoundPlayed = true;
							mApp->PlaySample(SOUND_FLAMETILE);
						}
						mFireTileCount++;
						aRefill = true;

						// 20 bursts of sparks (three layered embers each) and smoke along the tile's bottom
						for (int i = 0; i < 20; i++)
						{
							double anAngle = (double)i * 0.05 * -3.14159;
							double aDecay = (double)(mRand->Next() % 1000) * 0.001 * 0.01 + 0.02;
							double aVY = sin(anAngle) * 2.5;
							double aVX = cos(anAngle) * 1.5;
							double aY = aPiece->mY + 57.0;
							double aX = (double)(mRand->Next() % 57) + aPiece->mX;
							Ember* aSpark = AddEmberGet(mEmberMgr, EMBER_SPARK_LRG, aX, aY, aVX, aVY, true, 1.0, aDecay);
							aSpark->mDelay = 8;
							Ember* aCopy = AddEmberGet(mEmberMgr, EMBER_SPARK_MED, aSpark->mX, aSpark->mY, aSpark->mVX,
								aSpark->mVY, true, aSpark->mAlpha, aSpark->mAlphaDecay);
							aCopy->mDelay = 4;
							aCopy = AddEmberGet(mEmberMgr, EMBER_SPARK_LRG, aSpark->mX, aSpark->mY, aSpark->mVX, aSpark->mVY,
								true, aSpark->mAlpha, aSpark->mAlphaDecay);
							aCopy->mDelay = 0;

							double aPuffDecay = (double)(mRand->Next() % 1000) * 0.001 * 0.01 + 0.004;
							double aPuffAlpha = (double)(mRand->Next() % 1000) * 0.001;
							double aPuffY = (aPiece->mY + 57.0) - 3.0;
							double aPuffX = (double)(mRand->Next() % 57) + aPiece->mX;
							Ember* aPuff = AddEmberGet(mEmberMgr, EMBER_SMOKE_PUFF, aPuffX, aPuffY, 0.0, -0.25, false,
								aPuffAlpha, aPuffDecay);
							aPuff->mGravity = 0.0;
						}
					}
				}

				// c. sapphire and diamond tiles sparkle now and then
				if (((aPiece->mType == TILE_SAPPHIRE) || (aPiece->mType == TILE_DIAMOND)) && (mUpdateCnt % 16 == 0) &&
					(mRand->Next() % 10 == 0))
				{
					Sparkle* aSparkle = new Sparkle(mEffectMgr);
					aSparkle->mX = (double)(mRand->Next() % 57) + aPiece->mX - 28.0;
					aSparkle->mY = (double)(mRand->Next() % 57) + aPiece->mY - 28.0;
					mEffectMgr->AddEffect(aSparkle);
				}

				// d. a fire tile shakes the gold / sapphire / diamond tile below it (harder as its countdown
				// runs out); another shaking tile stops when the tile above is not a fire tile
				if ((aPiece->mType == TILE_FIRE) && (aRow < 7))
				{
					Piece* aBelow = PieceBelow(aPiece);
					if ((aBelow != NULL) && (aBelow->mType != TILE_FIRE) && (aBelow->mType != TILE_NORMAL) &&
						(aBelow->mType != TILE_GREEN))
					{
						aBelow->mShaking = true;
						int aCountdown = aBelow->mBurnCountdown;
						if (aCountdown <= 1)
							aCountdown = 1;
						aBelow->mShakeAmount = 2.5 / (double)aCountdown;
					}
				}
				else if (aPiece->mShaking)
				{
					// port: row 0 has no tile above (the original reads the Board fields before mBoard there;
					// no shaking tile reaches row 0)
					Piece* anAbove = (aRow > 0) ? mBoard[aRow - 1][aCol] : NULL;
					if ((anAbove != NULL) && (anAbove->mType != TILE_FIRE))
					{
						aPiece->mShaking = false;
						aPiece->mShakeAmount = 0.0;
						aPiece->mX = (double)ColX(aPiece->mCol);
						aPiece->mY = (double)RowY(aPiece->mCol, aPiece->mRow);
					}
				}

				// e.
				if (aPiece->mFlipping)
				{
					aPiece->mFlipProgress += 0.01;
					if (aPiece->mFlipProgress >= 1.0)
					{
						aPiece->mFlipProgress = 0.0;
						aPiece->mFlipping = false;
					}
					MarkDirty();
				}

				// f.
				if ((aPiece->mFlicker) && (mUpdateCnt % 4 == 0))
				{
					aPiece->mFlickerFrame = (aPiece->mFlickerFrame + 1) % 8;
					MarkDirty();
				}

				// g. the burned board starts flickering tile by tile
				if ((mState == BOARD_GAMEOVER_PENDING) && (mBurnedDown) && (!aPiece->mShaking))
				{
					if ((mRand->Next() & 0xff) == 0)
						aPiece->mFlicker = true;
				}

				// h. a fire tile on the bottom row: sparks under it, danger
				if ((aRow == 7) && (aPiece->mType == TILE_FIRE))
				{
					if (mState == BOARD_GAMEOVER)
					{
						// the original loops mState (2) times
						for (int i = 0; i < 2; i++)
						{
							Ember* anEmber = mEmberMgr->NewEmber();
							anEmber->mType = (mRand->Next() % 100 >= 80) ? EMBER_SPARK_MED : EMBER_SPARK_SM;
							anEmber->mX = (double)(mRand->Next() % 57) + aPiece->mX;
							anEmber->mY = aPiece->mY + 57.0;
							anEmber->mVX = ((double)(mRand->Next() % 1000) - 500.0) * 0.0033333333333333335;
							anEmber->mVY = ((double)(mRand->Next() % 500) - 500.0) * 0.0033333333333333335;
							anEmber->mAlpha = 0.5;
							anEmber->mLoop = true;
							anEmber->mAlphaDecay = ((double)(mRand->Next() % 1000) * 0.001 + 1.0) * 0.002;
						}
						mFireLinePiece = aPiece;
						aPiece->mShakeAmount += 0.075;
					}
					else if (mState == BOARD_PLAYING)
					{
						if ((mRand->Next() & 7) == 0)
						{
							Ember* anEmber = mEmberMgr->NewEmber();
							anEmber->mType = EMBER_SPARK_SM;
							anEmber->mX = (double)(mRand->Next() % 57) + aPiece->mX;
							anEmber->mY = (aPiece->mY + 57.0) - 3.0;
							anEmber->mVX = ((double)(mRand->Next() % 1000) - 500.0) * 0.00125;
							anEmber->mVY = ((double)(mRand->Next() % 1000) - 1000.0) * 0.0006666666666666666;
							anEmber->mAlpha = 0.65;
							anEmber->mAlphaDecay = ((double)(mRand->Next() % 1000) * 0.001 + 1.0) * 0.002;
						}

						if ((mRand->Next() & 7) == 0)
						{
							Ember* anEmber = mEmberMgr->NewEmber();
							anEmber->mType = EMBER_SMOKE_PUFF;
							anEmber->mX = (double)(mRand->Next() % 57) + aPiece->mX;
							anEmber->mVX = 0.0;
							anEmber->mVY = -0.25;
							anEmber->mGravity = 0.0;
							anEmber->mY = (aPiece->mY + 57.0) - 3.0;
							anEmber->mAlpha = (double)(mRand->Next() % 1000) * 0.001;
							anEmber->mAlphaDecay = (double)(mRand->Next() % 1000) * 0.001 * 0.01 + 0.004;
						}
					}

					if (!aPiece->mShaking)
					{
						aPiece->mShaking = true;
						aPiece->mShakeAmount = 1.5;
					}
					aFireAtBottom = true;
				}

				double aTarget = (double)aTargetY;
				if (aPiece->mY < aTarget)
				{
					// i. falling
					aPiece->mVY += 0.2;
					aPiece->mY += aPiece->mVY;
					MarkDirty();
					if (!aPiece->mShaking)
						mPiecesMoving = true;
				}
				else
				{
					// i. at rest: jitter, burning away
					if ((aPiece->mShaking) && (mUpdateCnt % 2 == 0))
					{
						double aJitterX = ((double)(mRand->Next() % 1000) - 500.0) * aPiece->mShakeAmount * 0.002;
						aPiece->mX = aJitterX + (double)ColX(aPiece->mCol);
						double aRowY = (double)RowY(aPiece->mCol, aPiece->mRow);
						aPiece->mY = ((double)(mRand->Next() % 1000) - 500.0) * aPiece->mShakeAmount * 0.002 + aRowY;
						MarkDirty();
					}

					if (aPiece->mBurning)
					{
						if (aPiece->mBurnDelay > 0)
							aPiece->mBurnDelay--;
						else
						{
							if ((aPiece->mBurnProgress == 0) && (!aBurnSoundPlayed))
							{
								mApp->PlaySample(SOUND_TILEBURN);
								aBurnSoundPlayed = true;
							}
							aPiece->mBurnProgress++;
						}

						if (aPiece->mBurnProgress >= 57)
						{
							mBoard[aPiece->mRow][aPiece->mCol] = NULL;
							aDeadPieces.push_back(aPiece);
							aRefill = true;
						}

						// embers every tick while burning (delay included)
						for (int i = 0; i < 8; i++)
						{
							Ember* anEmber = mEmberMgr->NewEmber();
							anEmber->mType = ((mRand->Next() & 15) != 0) ? EMBER_SPARK_SM : EMBER_EMBER_SM;
							int anOffset = (int)(mRand->Next() % 57);
							anEmber->mX = (double)anOffset + aPiece->mX;
							anEmber->mY = (double)aPiece->mBurnProgress + aPiece->mY;
							anEmber->mVX = (double)(anOffset - 28) * 0.07017543859649122;
							anEmber->mVY = ((double)(mRand->Next() % 1000) - 300.0) * 0.001;
							anEmber->mAlpha = 1.0;
							anEmber->mAlphaDecay = (double)(mRand->Next() % 1000) * 0.001 * 0.05 + 0.02;
						}
					}
				}

				// j. landed
				if (aPiece->mY > aTarget)
				{
					if ((aPiece->mVY > 4.0) && (mUpdateCnt - mLastLandSoundTick >= 4))
					{
						mApp->PlaySample(SOUND_PIECE_HIT);
						mLastLandSoundTick = mUpdateCnt;
					}
					aPiece->mVY = 0.0;
					aPiece->mY = aTarget;
					MarkDirty();
					if (!aPiece->mShaking)
						mPiecesMoving = true;
				}
				if (aPiece->mY == aTarget)
					aPiece->mVY = 0.0;

				// k. the tiles above a burning tile sink with it
				aTargetY = aTargetY + aPiece->mBurnProgress - 57;
			}
		}

		if ((hasSpecial) && (mUpdateCnt % 12 == 0))
			MarkDirty();

		if ((!mPiecesMoving) && (mFireTileCount > 0) && (!mApp->mWatchOutHintShown) && (mHintDelay == 0))
		{
			mWarnBeeps = 1;
			mHintDelay = 80;
			mHintId = HINT_WATCH_OUT;
			mApp->mWatchOutHintShown = true;
		}

		if (aBurn)
			BurnTiles(mApp->mGameMode == GAMEMODE_ACTION);
		if (aRefill)
		{
			Refill();
			CheckBottomFire();
		}
		else if (aBurn)
			CheckBottomFire();

		// the danger theme while a fire tile is on the bottom row
		MusicInterface* aMusic = mApp->mMusicInterface;
		if (aFireAtBottom)
		{
			mLex->SetSweating(true);
			if (!aMusic->IsPlaying(SONG_DANGER))
			{
				if (mApp->mGameMode == GAMEMODE_ACTION)
				{
					aMusic->FadeOut(SONG_ACTION, false, 0.008);
					aMusic->FadeIn(SONG_DANGER, ORDER_DANGER_ACTION, 0.008, false);
				}
				else if (mApp->mGameMode == GAMEMODE_CLASSIC)
				{
					aMusic->FadeOut(SONG_CLASSIC, false, 0.008);
					aMusic->FadeIn(SONG_DANGER, ORDER_DANGER_CLASSIC, 0.008, false);
				}
			}
		}
		else
		{
			mLex->SetSweating(false);
			if (aMusic->IsPlaying(SONG_DANGER))
			{
				aMusic->FadeOut(SONG_DANGER, true, 0.008);
				if (mApp->mGameMode == GAMEMODE_ACTION)
					aMusic->FadeIn(SONG_ACTION, -1, 0.008, false);
				else if (mApp->mGameMode == GAMEMODE_CLASSIC)
					aMusic->FadeIn(SONG_CLASSIC, -1, 0.008, false);
			}
		}
	}

	// 6.
	if (((mState == BOARD_GAMEOVER) || (mState == BOARD_GAMEOVER_MUSIC)) && (!mBurnedDown) && (LexIdle(mLex)))
		mLex->SetSweating(true);

	// 7. game over, one tick: high scores and the summary once the tiles have reached Lex
	bool isFlying = mOverlay->HasEffects(EFFECT_FLYING_PIECES);
	if (mState == BOARD_GAMEOVER_MUSIC)
	{
		if ((LexIdle(mLex)) && (mLex->mSweating) && (!mBurnedDown))
			mLex->PlayAnim(LEXANIM_SWEAT, true);
		if ((!mSummaryShown) && (!isFlying) && (LexCalm(mLex)))
		{
			mApp->KillDialog(DIALOG_HINT);
			mApp->SubmitHighScores(mGameStats);
			ShowSummary();
		}
		mState = BOARD_GAMEOVER;
	}

	// 8. a fire tile burned through: game over once Lex is calm; the burp after a word with fire tiles
	if (!isFlying)
	{
		if ((mState == BOARD_GAMEOVER_PENDING) && (LexCalm(mLex)))
		{
			if ((LexIdle(mLex)) && (mLex->mSweating) && (!mBurnedDown))
				mLex->PlayAnim(LEXANIM_SWEAT, true);
			mState = BOARD_GAMEOVER;
		}

		if ((mBurpDelay > 0) && (LexIdle(mLex)))
		{
			if ((--mBurpDelay == 0) && (mBurpPending))
			{
				if (!mBurnedDown)
				{
					mBurping = true;
					mBurpAlpha = 1.0;
					mLex->PlayAnim(LEXANIM_BURP, false);
					mApp->PlaySample(SOUND_LEX_BURP);
				}
				mBurpPending = false;
			}
		}
	}

	// 9. the burp smoke fades
	if (mBurpAlpha > 0.0)
	{
		mBurpAlpha = mBurpAlpha - 0.005;
		if ((mBurpAlpha < 0.9) && (mLex->mAnim == LEXANIM_BURP))
			mLex->PlayAnim(LEXANIM_EAT_END, false);
		if (mBurpAlpha < 0.0)
			mBurping = false;
		MarkDirty();
	}

	// 10. warning beeps
	if ((mWarnBeeps > 0) && (mState == BOARD_PLAYING))
	{
		if (mWarnBeepDelay > 0)
			mWarnBeepDelay--;
		if (mWarnBeepDelay == 0)
		{
			mWarnBeeps--;
			mApp->PlaySample(SOUND_WARNING);
			if (mWarnBeeps > 0)
				mWarnBeepDelay = 64;
		}
	}

	// 11. the rising notes of the last word
	if (mUpSoundIdx < mUpSoundCount)
	{
		if (mUpSoundDelay > 0)
			mUpSoundDelay--;
		if (mUpSoundDelay == 0)
		{
			mApp->PlaySample(SOUND_UP1 + ((mUpSoundIdx < 7) ? mUpSoundIdx : 7));
			mUpSoundIdx++;
			mUpSoundDelay = 10;
		}
	}

	// 12. tiles still flying to Lex
	int aPending = 0;
	if (mOverlay != NULL)
	{
		std::list<Effect*>* anEffects = mOverlay->GetEffects(EFFECT_FLYING_PIECES);
		if (anEffects != NULL)
		{
			for (std::list<Effect*>::iterator anItr = anEffects->begin(); anItr != anEffects->end(); ++anItr)
				aPending += (int)((FlyingPieceList*)*anItr)->mPieces.size();
			delete anEffects;
		}
	}

	// 13. the level bar (empties when the board burned)
	if ((mShownProgress != mLevelProgress) && (mUpdateCnt % 2 == 0) && (aPending <= 3))
	{
		MarkDirty();
		double aDelta = mLevelProgress - mShownProgress;
		if (mBurnedDown)
			aDelta = -mShownProgress;

		if (aDelta > 0.0)
			mShownProgress = aDelta * 0.04 + mShownProgress + 0.01;
		else if (aDelta < 0.0)
		{
			mShownProgress = mShownProgress - 0.05;
			if (mShownProgress < mLevelProgress)
				mShownProgress = mLevelProgress;
		}

		if ((mLevelProgress > 0.0) && (mShownProgress > mLevelProgress))
			mShownProgress = mLevelProgress;
	}

	// 14. the score rolls up and glows
	if ((mShownScore != mGameStats->mScore) && (mUpdateCnt % 2 == 0))
	{
		int aScore = mGameStats->mScore;
		double aScaledDiff = (double)(aScore - mShownScore) / mScoreScale;
		if (mShownScore < aScore)
		{
			mShownScore = (int)((aScaledDiff * 0.01 + 4.0) * mScoreScale) + mShownScore;
			double aGlow = mScoreGlow + 0.00375;
			if (aGlow >= 0.85)
				aGlow = 0.85;
			mScoreGlow = aGlow;
		}
		if (mShownScore > mGameStats->mScore)
			mShownScore = mGameStats->mScore;
		MarkDirty();
	}
	else if ((mScoreGlow > 0.0) && (mUpdateCnt % 4 == 0))
	{
		double aGlow = mScoreGlow - 0.00675;
		if (aGlow <= 0.0)
			aGlow = 0.0;
		mScoreGlow = aGlow;
		MarkDirty();
	}

	// 15. sparks over a glowing score
	if ((mScoreGlow > 0.385) && (mUpdateCnt % 8 == 0))
	{
		SexyString aScoreText = StrFormat("%d", mShownScore);
		int aWidth = mApp->mFonts[FONT_GOLDSCORE]->StringWidth(aScoreText);
		int aLeft = 108 - aWidth / 2;
		int aX = (int)(mRand->Next() % (unsigned int)aWidth) + aLeft;
		unsigned int aTypeRoll = mRand->Next() % 100;
		if (mShownScore != mGameStats->mScore)
		{
			double aDecay = (double)(mRand->Next() % 1000) * 0.001 * 0.025 + 0.01;
			double aVY = ((double)(mRand->Next() % 1000) - 1000.0) * 0.0006666666666666666;
			double aVX = ((double)(mRand->Next() % 1000) - 500.0) * 0.00125;
			mEmberMgr->AddEmber((aTypeRoll >= 80) ? EMBER_SPARK_MED : EMBER_SPARK_SM, (double)aX, 31.0, aVX, aVY, true,
				1.0, aDecay);
		}
		double aPuffDecay = (double)(mRand->Next() % 1000) * 0.001 * 0.01 + 0.004;
		double aPuffAlpha = (double)(mRand->Next() % 1000) * 0.001;
		Ember* aPuff = AddEmberGet(mEmberMgr, EMBER_SMOKE_PUFF, (double)(aX - 8), 23.0, 0.0, -0.25, false, aPuffAlpha,
			aPuffDecay);
		aPuff->mGravity = 0.0;
	}

	// 16.
	if (mEffectMgr->Update())
		MarkDirty();
	if (mEmberMgr->Update())
		MarkDirty();

	// 17. the popups drift up and die
	std::list<Points*>::iterator aPopupItr = mPopups.begin();
	while (aPopupItr != mPopups.end())
	{
		Points* aPopup = *aPopupItr;
		aPopup->MarkDirty();
		aPopup->mHue = (aPopup->mHue + 7) % 255;
		if ((mUpdateCnt % 2 == 0) && (aPopup->mY > 0))
			aPopup->Move(aPopup->mX, aPopup->mY - 1);

		if (--aPopup->mLife <= 0)
		{
			if (mLastMessage == aPopup)
				mLastMessage = NULL;
			aPopupItr = mPopups.erase(aPopupItr);
			mWidgetManager->RemoveWidget(aPopup);
			mApp->SafeDeleteWidget(aPopup);
		}
		else
			++aPopupItr;
	}

	// 18. the word above the board: the selection's while selecting, fading after
	if (mSelection.size() != 0)
	{
		mLastWord = mWord;
		mLastWordAlpha = 1.0;
	}
	else if ((mLastWordAlpha > 0.0) && (mUpdateCnt % 8 == 0))
	{
		mLastWordAlpha = mLastWordAlpha - 0.08;
		MarkDirty();
	}

	// 19. a scheduled hint (the app shows it only with the hints option)
	if ((mHintDelay > 0) && (mSelectMode != SELECT_DRAGGING) && (mState == BOARD_PLAYING) &&
		(mApp->GetDialogCount() == 0))
	{
		if (--mHintDelay == 0)
		{
			if ((mHintId >= HINT_HOW_TO_PLAY) && (mHintId <= HINT_FIRE_ALERT))
			{
				GameTextId aLines = gHintLines[mHintId];
				if ((mHintId == HINT_FIRE_ALERT) && (mApp->mGameMode != GAMEMODE_ACTION))
					aLines = gHintFireAlertClassic;
				mApp->ShowHint(mHintId, GameText(gHintHeaders[mHintId]), GameText(aLines), GameText(GT_HINT_CONTINUE),
					Dialog::BUTTONS_FOOTER);
			}
		}
	}

	// 20. the scramble's letter flip ends
	if (mState == BOARD_SCRAMBLING)
	{
		mScrambleProgress += 0.01;
		if (mScrambleProgress >= 1.0)
		{
			for (int aCol = 0; aCol < BOARD_COLS; aCol++)
			{
				for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
				{
					if (mBoard[aRow][aCol] != NULL)
						mBoard[aRow][aCol]->mLetter = mBoard[aRow][aCol]->mNextLetter;
				}
			}
			mScrambleProgress = 0.0;
			mState = BOARD_PLAYING;
		}
		MarkDirty();
	}

	// 21. Lex's blinks (drawn by BoardDraw)
	if (mBlinkDelay < 1)
	{
		if (mBlinkRepeats < 1)
			mBlinkDelay = (int)(mRand->Next() % 300);
		else
		{
			mBlinkRepeats--;
			mBlinkDelay = 40;
		}
	}
	else if (--mBlinkDelay == 0)
	{
		mBlinkFrames = 20;
		MarkDirty();
	}

	if ((mBlinkFrames > 0) && (--mBlinkFrames == 0))
		MarkDirty();

	// 22. dead: a spark burst around the game-over tile (nothing sets mDeadSparkDelay above 0)
	if ((mDeadSparkDelay > 0) && (--mDeadSparkDelay == 0))
	{
		for (int i = 0; i < 20; i++)
		{
			double anAngle = (double)i * 0.05 * -3.14159;
			Ember* anEmber = mEmberMgr->NewEmber();
			anEmber->mType = EMBER_EMBER_SM;
			anEmber->mX = (double)(mRand->Next() % 57) + mGameOverPiece->mX;
			anEmber->mAlpha = 1.0;
			anEmber->mY = mGameOverPiece->mY + 57.0;
			anEmber->mVX = cos(anAngle) * 4.5;
			anEmber->mVY = sin(anAngle) * 4.5;
			int aRoll = (int)(mRand->Next() % 1000);
			anEmber->mLoop = true;
			anEmber->mDelay = 8;
			anEmber->mAlphaDecay = (double)aRoll * 0.001 * 0.004 + 0.005;

			Ember* aCopy = mEmberMgr->NewEmber();
			aCopy->mType = EMBER_SPARK_MED;
			aCopy->mX = anEmber->mX;
			aCopy->mY = anEmber->mY;
			aCopy->mVX = anEmber->mVX;
			aCopy->mVY = anEmber->mVY;
			aCopy->mAlpha = anEmber->mAlpha;
			aCopy->mAlphaDecay = anEmber->mAlphaDecay;
			aCopy->mLoop = true;
			aCopy->mDelay = 4;

			Ember* aCopy2 = mEmberMgr->NewEmber();
			aCopy2->mType = EMBER_SPARK_LRG;
			aCopy2->mX = aCopy->mX;
			aCopy2->mY = aCopy->mY;
			aCopy2->mVX = aCopy->mVX;
			aCopy2->mVY = aCopy->mVY;
			aCopy2->mAlpha = aCopy->mAlpha;
			aCopy2->mAlphaDecay = aCopy->mAlphaDecay;
			aCopy2->mLoop = true;
			aCopy2->mDelay = 0;
		}
	}

	// 23. the game-over sequence, each step only while Lex is calm: the white flash fades, the jingle, and
	// the whole board burns (tiles snap home and flicker, 700 embers)
	if ((mBoardBurnAlpha > 0.0) && (LexCalm(mLex)))
	{
		mBoardBurnAlpha = mBoardBurnAlpha - 0.01;
		MarkDirty();
	}

	if ((mGameOverMusicDelay > 0) && (LexCalm(mLex)) && (--mGameOverMusicDelay == 0))
		StartGameOverMusic();

	if ((mBoardBurnDelay > 0) && (LexCalm(mLex)) && (--mBoardBurnDelay == 0))
	{
		for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
		{
			for (int aCol = 0; aCol < BOARD_COLS; aCol++)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if (aPiece == NULL)
					continue;

				// the draw is taken before the row test
				if (((mRand->Next() & 15) == 0) || (aRow == 0))
				{
					aPiece->mFlicker = true;
					aPiece->mFlickerFrame = (int)(mRand->Next() & 7);
				}
				aPiece->mX = (double)ColX(aPiece->mCol);
				aPiece->mBurning = false;
				aPiece->mShaking = false;
				aPiece->mY = (double)RowY(aPiece->mCol, aPiece->mRow);
				aPiece->mType = TILE_NORMAL;
			}
		}

		for (int i = 0; i < 700; i++)
		{
			Ember* anEmber = mEmberMgr->NewEmber();
			anEmber->mType = EMBER_SPARK_SM;
			anEmber->mX = (double)(mRand->Next() % (unsigned int)mWidth);
			anEmber->mY = (double)(mRand->Next() % (unsigned int)mHeight);
			anEmber->mVX = 0.0;
			anEmber->mVY = 0.0;
			int aRoll = (int)(mRand->Next() % 1000);
			anEmber->mAlpha = 1.0;
			anEmber->mGravity = (double)aRoll * 0.001 * 0.005 + 0.002;
			anEmber->mAlphaDecay = (double)(mRand->Next() % 1000) * 0.001 * 0.001 + 0.00075;
			if (mRand->Next() % 30 == 0)
			{
				anEmber->mType = EMBER_SPARK_LRG;
				anEmber->mGravity = (double)(mRand->Next() % 1000) * 0.001 * 0.005 + 0.008;
			}
			else if (mRand->Next() % 20 == 0)
			{
				anEmber->mType = EMBER_SPARK_MED;
				anEmber->mGravity = ((double)(mRand->Next() % 1000) * 0.001 + 1.0) * 0.005;
			}
		}

		SetButtonsVisible(false);
		ClearPopups();
		mFireLinePiece = NULL;
		mFireLineAlpha = 0.0;
		mBoardBurnAlpha = 1.0;
		mBurnedDown = true;
		if (LexCalm(mLex))
			mLex->PlayAnim(LEXANIM_BURNT_BLINK, false);
		MarkDirty();
	}

	// 24. the fire column: the screen-burn sound once Lex is idle (action: at once; or nothing flies), then
	// the column fades in and scrolls
	if ((mFireLineDelay >= 1) &&
		((LexIdle(mLex)) || (mApp->mGameMode == GAMEMODE_ACTION) || (aPending == 0)))
	{
		if (--mFireLineDelay == 0)
			mApp->PlaySample(SOUND_SCREENBURNGAMEOVER);
		MarkDirty();
	}
	else if ((mFireLinePiece != NULL) && (mFireLineDelay == 0))
	{
		if (mFireLineAlpha < 1.0)
		{
			mFireLineAlpha = mFireLineAlpha + 0.0125;
			if (mFireLineAlpha > 1.0)
				mFireLineAlpha = 1.0;
		}

		if (mUpdateCnt % 8 == 0)
		{
			Image* aFireLineImage = mApp->mImages[IMAGE_FX_FIRELINE];
			unsigned int aRoll = mRand->Next() % 48;
			mFireLineScroll = (int)((aRoll + 48 + (unsigned int)mFireLineScroll) % (unsigned int)aFireLineImage->GetWidth());
		}
		MarkDirty();
	}

	// 25. the tiles that burned away this tick
	for (PieceList::iterator anItr = aDeadPieces.begin(); anItr != aDeadPieces.end(); ++anItr)
	{
		if (*anItr != NULL)
			delete *anItr;
	}
	aDeadPieces.clear();

	// 26. level complete: Lex announces the level up (GT_LEVEL_UP), then the summary
	if (mState == BOARD_LEVEL_COMPLETE)
	{
		mLexPraisePending = false;
		if ((mApp->GetDialogCount() == 0) && (LexIdle(mLex)) && (!isFlying))
		{
			if (!mLevelUpPending)
			{
				if (!mSummaryShown)
					ShowSummary();
			}
			else
			{
				mApp->PlaySample(SOUND_LEX_LEVELUP);
				mLex->PlayAnim(LEXANIM_LEVELUP, false);
				mLex->Say(GameText(GT_LEVEL_UP), mApp->mFonts[FONT_VERDANA12BOLD], 75);
				mLevelUpPending = false;
			}
		}
	}

	// 27. Lex praises a special tile (the draw is spent for other types)
	if ((mLexPraisePending) && (!isFlying) && (LexIdle(mLex)) &&
		((mState == BOARD_PLAYING) || (mState == BOARD_LEVEL_COMPLETE) || (mState == BOARD_GAMEOVER_PENDING)))
	{
		mLexPraisePending = false;
		unsigned int aRoll = mRand->Next() % 100;
		int anAnim = LEXANIM_NONE;
		if (mLexPraiseType == TILE_GOLD)
		{
			if (aRoll < 33)
			{
				mApp->PlaySample(SOUND_LEX_VERYGOOD);
				anAnim = LEXANIM_VERYGOOD;
			}
			else if (aRoll < 66)
			{
				mApp->PlaySample(SOUND_LEX_EXCELLENT);
				anAnim = LEXANIM_EXCELLENT;
			}
			else
			{
				mApp->PlaySample(SOUND_LEX_AWESOME);
				anAnim = LEXANIM_AWESOME;
			}
		}
		else if (mLexPraiseType == TILE_SAPPHIRE)
		{
			if (aRoll < 50)
			{
				mApp->PlaySample(SOUND_LEX_FANTASTIC);
				anAnim = LEXANIM_FANTASTIC;
			}
			else
			{
				mApp->PlaySample(SOUND_LEX_ASTONISHING);
				anAnim = LEXANIM_ASTONISHING;
			}
		}
		else if (mLexPraiseType == TILE_DIAMOND)
		{
			if (aRoll < 50)
			{
				mApp->PlaySample(SOUND_LEX_INCREDIBLE);
				anAnim = LEXANIM_INCREDIBLE;
			}
			else
			{
				mApp->PlaySample(SOUND_LEX_UNBELIEVABLE);
				anAnim = LEXANIM_UNBELIEVABLE;
			}
		}

		if (anAnim != LEXANIM_NONE)
			mLex->PlayAnim(anAnim, false);
	}
}

// --------------------------------------------------------------------------------------------------------
// Saved games (board_logic.md 6): players/<user>/classic.dat and action.dat, version 5

// 0x406c40 (the app's ShutdownWidgets and 2028): the prologue drops every effect and Lex's animation,
// closes the help screen and banks the level clock; then the record of board_logic.md 6.1
bool Board::SaveGame(int theMode)
{
	if (mEffectMgr != NULL)
		mEffectMgr->Clear();
	if (mEmberMgr != NULL)
		mEmberMgr->Clear();
	if ((mOverlay != NULL) && (mOverlay->mEffectMgr != NULL))
		mOverlay->mEffectMgr->Clear();
	if (mLex != NULL)
		mLex->Reset();

	mShownScore = mGameStats->mScore;
	bool isHelpOpen = (!mApp->mShowHelp) && (mApp->mHelpWidget != NULL);
	mApp->HideHelp();
	Pause(true);
	Pause(false);

	// port: the original builds "players\<name>\" + file, creates both folders (_mkdir) and fopens it next
	// to the exe; the framework's VFS puts the file (and its folders) in the user dir
	SexyString aPath = BookWormApp::GetSaveGamePath(mApp->mUserManager->GetCurrentUserName(), theMode);
	if (aPath.size() == 0)
		return false;

	FILE* aFile = Platform::VFS::Get().Open(aPath, "wb");
	if (aFile == NULL)
		return false;

	WriteSavedGame(aFile, theMode, isHelpOpen);
	fclose(aFile);
	mApp->SyncUserData();  // port: web persistence of the user dir
	return true;
}

// port (BookWormApp::GetShutdownSaves, web persistence): the bytes SaveGame(theMode) would write now, without
// changing the board: the prologue's effects on the record are applied for the write and undone after
// (the effect lists set aside, Lex's animation fields, the shown score, the pause count and banked level
// seconds HideHelp and the pause / unpause would leave)
bool Board::SaveGameSnapshot(int theMode, Buffer* theBuffer)
{
	SexyString aPath = BookWormApp::GetSaveGamePath(mApp->mUserManager->GetCurrentUserName(), theMode);
	if ((aPath.size() == 0) || (mLex == NULL) || (mOverlay == NULL))
		return false;

	FILE* aFile = tmpfile();
	if (aFile == NULL)
		return false;

	std::list<Effect*> aBoardEffects;
	std::list<Effect*> anOverlayEffects;
	std::vector<Ember*> anEmbers;
	if (mEffectMgr != NULL)
		aBoardEffects.swap(mEffectMgr->mEffects);
	if (mEmberMgr != NULL)
		anEmbers.swap(mEmberMgr->mEmbers);
	if (mOverlay->mEffectMgr != NULL)
		anOverlayEffects.swap(mOverlay->mEffectMgr->mEffects);

	int aLexAnim = mLex->mAnim;
	int aLexAnimStep = mLex->mAnimStep;
	int aLexAnimTick = mLex->mAnimTick;
	int aLexFrame = mLex->mFrame;
	bool aLexAnimLoop = mLex->mAnimLoop;
	bool aLexSweating = mLex->mSweating;
	bool aLexDoFinger = mLex->mDoFinger;
	int aLexSweatFrame = mLex->mSweatFrame;
	mLex->Reset();

	int aShownScore = mShownScore;
	int aPauseCount = mPauseCount;
	int aLevelSecondsBanked = mLevelSecondsBanked;
	mShownScore = mGameStats->mScore;
	bool isHelpOpen = (!mApp->mShowHelp) && (mApp->mHelpWidget != NULL);

	// HideHelp unpauses the board once when the help screen is up; then Pause(true) / Pause(false) bank the
	// level clock when that leaves the board unpaused (after HideHelp's unpause the clock restarted: 0)
	unsigned long aNow = GameTickCount();
	unsigned long aStartTick = mLevelStartTick;
	if ((mApp->mHelpWidget != NULL) && (mApp->mBoard == this))
	{
		int anOldCount = mPauseCount;
		mPauseCount = (anOldCount - 1 < 0) ? 0 : anOldCount - 1;
		if ((mPauseCount < 1) && (anOldCount > 0))
			aStartTick = aNow;
	}
	if (mPauseCount < 1)
		mLevelSecondsBanked = (int)((unsigned long)mLevelSecondsBanked + (aNow - aStartTick) / 1000);

	WriteSavedGame(aFile, theMode, isHelpOpen);

	mLevelSecondsBanked = aLevelSecondsBanked;
	mPauseCount = aPauseCount;
	mShownScore = aShownScore;
	mLex->mAnim = aLexAnim;
	mLex->mAnimStep = aLexAnimStep;
	mLex->mAnimTick = aLexAnimTick;
	mLex->mFrame = aLexFrame;
	mLex->mAnimLoop = aLexAnimLoop;
	mLex->mSweating = aLexSweating;
	mLex->mDoFinger = aLexDoFinger;
	mLex->mSweatFrame = aLexSweatFrame;
	if (mOverlay->mEffectMgr != NULL)
		anOverlayEffects.swap(mOverlay->mEffectMgr->mEffects);
	if (mEmberMgr != NULL)
		anEmbers.swap(mEmberMgr->mEmbers);
	if (mEffectMgr != NULL)
		aBoardEffects.swap(mEffectMgr->mEffects);

	long aLength = ftell(aFile);
	bool aResult = aLength >= 0;
	if (aResult)
	{
		std::vector<uchar> aData((size_t)aLength);
		fseek(aFile, 0, SEEK_SET);
		aResult = (aLength == 0) || (fread(&aData[0], 1, (size_t)aLength, aFile) == (size_t)aLength);
		if (aResult)
		{
			theBuffer->Clear();
			if (aLength > 0)
				theBuffer->SetData(&aData[0], (int)aLength);
		}
	}
	fclose(aFile);
	return aResult;
}

// 0x406c40 (second half): the record of board_logic.md 6.1 from the board as it is
void Board::WriteSavedGame(FILE* theFile, int theMode, bool isHelpOpen)
{
	// 1-7 header
	FileWriteInt(theFile, 5);
	FileWriteString(theFile, mApp->mProductVersion);
	FileWriteInt(theFile, mApp->mBuildNum);
	FileWriteString(theFile, mApp->mBuildDate);
	FileWriteInt(theFile, theMode);
	FileWriteBool(theFile, mApp->mShowHints);
	FileWriteString(theFile, mRand->Serialize());

	// 8 the grid in memory order
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			FileWriteBool(theFile, aPiece != NULL);
			if (aPiece != NULL)
				aPiece->Save(theFile);
		}
	}

	// 9-11
	FileWriteInt(theFile, mLevelSeconds);
	FileWriteInt(theFile, mTotalSeconds);
	FileWriteInt(theFile, mLevelSecondsBanked);
	mOverlay->SaveState(theFile);
	mEffectMgr->Save(theFile);
	mEmberMgr->Save(theFile);

	// 12-13
	FileWriteInt(theFile, mState);
	FileWriteInt(theFile, mSelectMode);
	FileWriteInt(theFile, (int)mSelection.size());
	for (PieceList::iterator anItr = mSelection.begin(); anItr != mSelection.end(); ++anItr)
	{
		FileWriteInt(theFile, (*anItr)->mRow);
		FileWriteInt(theFile, (*anItr)->mCol);
	}

	// 14-15
	FileWriteDouble(theFile, mFireMeter);
	FileWriteBool(theFile, mSkipRandomFire);
	FileWriteBool(theFile, mBurnedDown);
	FileWriteBytes(theFile, &mUnused1b2, 1);
	FileWriteBool(theFile, mPiecesFlying);
	FileWriteBool(theFile, mFlameSoundPlayed);
	FileWriteInt(theFile, mRewardStreak);
	FileWriteDouble(theFile, mScrambleProgress);
	FileWriteDouble(theFile, mLevelProgress);
	FileWriteDouble(theFile, mShownProgress);

	// 16-18
	mWord.Save(theFile);
	mLastWord.Save(theFile);
	FileWriteInt(theFile, mLastWordColor.mRed);
	FileWriteInt(theFile, mLastWordColor.mGreen);
	FileWriteInt(theFile, mLastWordColor.mBlue);
	FileWriteInt(theFile, mLastWordColor.mAlpha);
	FileWriteDouble(theFile, mLastWordAlpha);
	FileWriteBool(theFile, mNewHighScore);
	FileWriteInt(theFile, mBonusWordsGiven);
	FileWriteString(theFile, mBonusWord);

	// 19-21
	mGameStats->Save(theFile);
	mLevelStats->Save(theFile);
	FileWriteString(theFile, mUnusedStr2b0);
	FileWriteInt(theFile, mUnused2cc);
	FileWriteString(theFile, mUnusedStr2d0);
	FileWriteInt(theFile, mUnused2ec);
	FileWriteInt(theFile, mUpSoundIdx);
	FileWriteInt(theFile, mUpSoundCount);
	FileWriteInt(theFile, mUpSoundDelay);

	// 22-25
	FileWriteBool(theFile, mGameOverPiece != NULL);
	if (mGameOverPiece != NULL)
	{
		FileWriteInt(theFile, mGameOverPiece->mRow);
		FileWriteInt(theFile, mGameOverPiece->mCol);
	}
	FileWriteInt(theFile, mDeadSparkDelay);
	FileWriteInt(theFile, mBoardBurnDelay);
	FileWriteDouble(theFile, mBoardBurnAlpha);
	FileWriteInt(theFile, mBlinkDelay);
	FileWriteInt(theFile, mBlinkRepeats);
	FileWriteInt(theFile, mGameOverMusicDelay);
	FileWriteInt(theFile, mFireLineDelay);
	FileWriteBool(theFile, mFireLinePiece != NULL);
	if (mFireLinePiece != NULL)
	{
		FileWriteInt(theFile, mFireLinePiece->mRow);
		FileWriteInt(theFile, mFireLinePiece->mCol);
	}
	FileWriteDouble(theFile, mFireLineAlpha);
	FileWriteInt(theFile, mFireLineScroll);
	FileWriteInt(theFile, mFireTileCount);
	FileWriteInt(theFile, mActionFireTimer);
	FileWriteInt(theFile, mActionCountdown);
	FileWriteInt(theFile, mWarnBeeps);
	FileWriteInt(theFile, mWarnBeepDelay);
	FileWriteInt(theFile, mHintDelay);
	FileWriteInt(theFile, mHintId);

	// 26 Lex
	FileWriteBool(theFile, mLex != NULL);
	// port: the original calls Save on a missing Lex too (a null call; a board in play always has him)
	if (mLex != NULL)
		mLex->Save(theFile);

	// 27-29
	FileWriteBool(theFile, mLexGreetPending);
	FileWriteBool(theFile, mLexDangerTalkReady);
	FileWriteBool(theFile, mLevelUpPending);
	FileWriteInt(theFile, mBlinkFrames);
	FileWriteBool(theFile, mBurpPending);
	FileWriteBool(theFile, mBurping);
	FileWriteInt(theFile, mBurpDelay);
	FileWriteDouble(theFile, mBurpAlpha);
	FileWriteInt(theFile, mHighlightKey);
	FileWriteBool(theFile, mCtrlDown);
	FileWriteInt(theFile, mSubmitBlinkWords);
	FileWriteInt(theFile, mCols);
	FileWriteInt(theFile, mRows);
	FileWriteInt(theFile, mPointsAwarded);
	FileWriteInt(theFile, mShownScore);
	FileWriteDouble(theFile, mScoreScale);
	FileWriteBool(theFile, mSummaryShown);
	FileWriteBool(theFile, mPiecesMoving);
	FileWriteInt(theFile, mLevel);
	FileWriteInt(theFile, mPauseCount);
	FileWriteDouble(theFile, mPauseFade);
	FileWriteInt(theFile, mUnused3d8);
	FileWriteBool(theFile, mPauseNoDialogs);
	FileWriteInt(theFile, mBestWordThisLevel);
	FileWriteInt(theFile, mBestWordThisGame);
	FileWriteInt(theFile, mUnused3e8);
	FileWriteInt(theFile, mUnused3ec);

	// 30-31
	FileWriteString(theFile, mGameLog.ToWebString());
	FileWriteBool(theFile, isHelpOpen);
}

// 0x408ab0 (the app's 2005, after StartGame made a fresh board): reads the record into this board. Versions
// 2-4 lack fields 2-4, version 2 also the level seconds, versions up to 3 the help byte.
bool Board::LoadGame(int theMode)
{
	mApp->HideHelp();

	SexyString aPath = BookWormApp::GetSaveGamePath(mApp->mUserManager->GetCurrentUserName(), theMode);
	if (aPath.size() == 0)
		return false;

	FILE* aFile = Platform::VFS::Get().Open(UserDataPath(aPath), "rb");
	if (aFile == NULL)
		return false;

	bool isHelpOpen = false;
	bool isRead = ReadSavedGame(aFile, &isHelpOpen);
	fclose(aFile);
	if (!isRead)
		return false;

	mPlayCounted = false;

	// the danger theme when Lex was sweating
	MusicInterface* aMusic = mApp->mMusicInterface;
	if ((mLex->mSweating) && (!aMusic->IsPlaying(SONG_DANGER)))
	{
		if (mApp->mGameMode == GAMEMODE_ACTION)
		{
			aMusic->FadeOut(SONG_ACTION, false, 0.008);
			aMusic->FadeIn(SONG_DANGER, ORDER_DANGER_ACTION, 0.008, false);
		}
		else if (mApp->mGameMode == GAMEMODE_CLASSIC)
		{
			aMusic->FadeOut(SONG_CLASSIC, false, 0.008);
			aMusic->FadeIn(SONG_DANGER, ORDER_DANGER_CLASSIC, 0.008, false);
		}
	}

	if (mApp->mGameMode == GAMEMODE_ACTION)
		SetActionSpeed((double)gActionSpeed[((unsigned int)mLevel > 8) ? 9 : mLevel]);

	// the summary was up: show it again with the saved times
	if (mSummaryShown)
	{
		int aLevelSeconds = mLevelSeconds;
		int aTotalSeconds = mTotalSeconds;
		ShowSummary();
		mLevelSeconds = aLevelSeconds;
		mTotalSeconds = aTotalSeconds;
	}

	if ((mApp->mShowHelp) || (isHelpOpen))
		mApp->ShowHelp();

	return true;
}

// port: LoadGame's reading of the record (board_logic.md 6.1) into this board. Versions 2-4 lack fields 2-4,
// version 2 also the level seconds, versions up to 3 the help byte.
bool Board::ReadSavedGame(FILE* theFile, bool* isHelpOpen)
{
	int aVersion = 0;
	FileReadInt(theFile, &aVersion);
	if (aVersion < 2)
		return false;

	// port: the original's OutputDebugString "BookWorm: loading and converting obsolete saved game." for
	// versions 2-4 is dropped
	SexyString aString;
	if (aVersion > 4)
	{
		// product version, build number, build date: read and dropped
		int aBuildNum = 0;
		FileReadString(theFile, &aString);
		FileReadInt(theFile, &aBuildNum);
		FileReadString(theFile, &aString);
	}

	FileReadInt(theFile, &mApp->mGameMode);
	FileReadBool(theFile, &mApp->mShowHints);
	FileReadString(theFile, &aString);
	mRand->SRand(aString);  // the state words; the index of the next draw stays the fresh generator's

	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = NULL;
			bool isPresent = false;
			FileReadBool(theFile, &isPresent);
			if (isPresent)
			{
				aPiece = new Piece();
				aPiece->Load(theFile);
			}
			if (mBoard[aRow][aCol] != NULL)
				delete mBoard[aRow][aCol];
			mBoard[aRow][aCol] = aPiece;
		}
	}

	if (aVersion > 2)
		FileReadInt(theFile, &mLevelSeconds);
	FileReadInt(theFile, &mTotalSeconds);
	FileReadInt(theFile, &mLevelSecondsBanked);
	mOverlay->LoadState(theFile);
	mEffectMgr->Clear();
	mEffectMgr->Load(theFile);
	mEmberMgr->Clear();
	mEmberMgr->Load(theFile);

	FileReadInt(theFile, &mState);
	FileReadInt(theFile, &mSelectMode);
	int aCount = 0;
	FileReadInt(theFile, &aCount);
	mSelection.clear();
	for (int i = 0; i < aCount; i++)
	{
		int aRow = 0;
		int aCol = 0;
		FileReadInt(theFile, &aRow);
		FileReadInt(theFile, &aCol);
		// port: a cell outside the grid (a damaged file) is skipped like an empty one
		if ((aRow < 0) || (aRow >= BOARD_ROWS) || (aCol < 0) || (aCol >= BOARD_COLS))
			continue;
		if (mBoard[aRow][aCol] != NULL)
			mSelection.push_back(mBoard[aRow][aCol]);
	}

	FileReadDouble(theFile, &mFireMeter);
	FileReadBool(theFile, &mSkipRandomFire);
	FileReadBool(theFile, &mBurnedDown);
	fread(&mUnused1b2, 1, 1, theFile);
	FileReadBool(theFile, &mPiecesFlying);
	FileReadBool(theFile, &mFlameSoundPlayed);
	FileReadInt(theFile, &mRewardStreak);
	FileReadDouble(theFile, &mScrambleProgress);
	FileReadDouble(theFile, &mLevelProgress);
	FileReadDouble(theFile, &mShownProgress);

	mWord.Load(theFile);
	mLastWord.Load(theFile);
	FileReadInt(theFile, &mLastWordColor.mRed);
	FileReadInt(theFile, &mLastWordColor.mGreen);
	FileReadInt(theFile, &mLastWordColor.mBlue);
	FileReadInt(theFile, &mLastWordColor.mAlpha);
	FileReadDouble(theFile, &mLastWordAlpha);
	FileReadBool(theFile, &mNewHighScore);
	FileReadInt(theFile, &mBonusWordsGiven);
	FileReadString(theFile, &mBonusWord);

	mGameStats->Load(theFile);
	mLevelStats->Load(theFile);
	FileReadString(theFile, &mUnusedStr2b0);
	FileReadInt(theFile, &mUnused2cc);
	FileReadString(theFile, &mUnusedStr2d0);
	FileReadInt(theFile, &mUnused2ec);
	FileReadInt(theFile, &mUpSoundIdx);
	FileReadInt(theFile, &mUpSoundCount);
	FileReadInt(theFile, &mUpSoundDelay);

	bool isPresent = false;
	FileReadBool(theFile, &isPresent);
	if (isPresent)
	{
		int aRow = 0;
		int aCol = 0;
		FileReadInt(theFile, &aRow);
		FileReadInt(theFile, &aCol);
		// port: NULL for a cell outside the grid (a damaged file)
		mGameOverPiece = ((aRow >= 0) && (aRow < BOARD_ROWS) && (aCol >= 0) && (aCol < BOARD_COLS)) ?
			mBoard[aRow][aCol] : NULL;
	}
	FileReadInt(theFile, &mDeadSparkDelay);
	FileReadInt(theFile, &mBoardBurnDelay);
	FileReadDouble(theFile, &mBoardBurnAlpha);
	FileReadInt(theFile, &mBlinkDelay);
	FileReadInt(theFile, &mBlinkRepeats);
	FileReadInt(theFile, &mGameOverMusicDelay);
	FileReadInt(theFile, &mFireLineDelay);

	isPresent = false;
	FileReadBool(theFile, &isPresent);
	if (isPresent)
	{
		int aRow = 0;
		int aCol = 0;
		FileReadInt(theFile, &aRow);
		FileReadInt(theFile, &aCol);
		mFireLinePiece = ((aRow >= 0) && (aRow < BOARD_ROWS) && (aCol >= 0) && (aCol < BOARD_COLS)) ?
			mBoard[aRow][aCol] : NULL;
	}
	FileReadDouble(theFile, &mFireLineAlpha);
	FileReadInt(theFile, &mFireLineScroll);
	FileReadInt(theFile, &mFireTileCount);
	FileReadInt(theFile, &mActionFireTimer);
	FileReadInt(theFile, &mActionCountdown);
	FileReadInt(theFile, &mWarnBeeps);
	FileReadInt(theFile, &mWarnBeepDelay);
	FileReadInt(theFile, &mHintDelay);
	FileReadInt(theFile, &mHintId);

	// Lex: a new one reads his record
	isPresent = false;
	FileReadBool(theFile, &isPresent);
	if (isPresent)
	{
		if (mLex != NULL)
		{
			if (mApp->mWidgetManager->HasWidget(mLex))
				mApp->mWidgetManager->RemoveWidget(mLex);
			delete mLex;
			mLex = NULL;
		}

		Image* anImage = mApp->mImages[IMAGE_LEX_EATING];
		mLex = new Lex(mApp);
		mLex->Resize(0, 0, anImage->GetWidth(), anImage->GetHeight());
		mWidgetManager->AddWidget(mLex);
		mLex->Load(theFile);
		mLex->SetVisible(mVisible);
	}

	FileReadBool(theFile, &mLexGreetPending);
	FileReadBool(theFile, &mLexDangerTalkReady);
	FileReadBool(theFile, &mLevelUpPending);
	FileReadInt(theFile, &mBlinkFrames);
	FileReadBool(theFile, &mBurpPending);
	FileReadBool(theFile, &mBurping);
	FileReadInt(theFile, &mBurpDelay);
	FileReadDouble(theFile, &mBurpAlpha);
	FileReadInt(theFile, &mHighlightKey);
	mHighlightKey = 0;
	FileReadBool(theFile, &mCtrlDown);
	mCtrlDown = false;
	FileReadInt(theFile, &mSubmitBlinkWords);
	mSubmitBlinkWords = 3;
	FileReadInt(theFile, &mCols);
	FileReadInt(theFile, &mRows);
	FileReadInt(theFile, &mPointsAwarded);
	FileReadInt(theFile, &mShownScore);
	FileReadDouble(theFile, &mScoreScale);
	FileReadBool(theFile, &mSummaryShown);
	FileReadBool(theFile, &mPiecesMoving);
	FileReadInt(theFile, &mLevel);
	FileReadInt(theFile, &mPauseCount);
	FileReadDouble(theFile, &mPauseFade);
	FileReadInt(theFile, &mUnused3d8);
	FileReadBool(theFile, &mPauseNoDialogs);
	FileReadInt(theFile, &mBestWordThisLevel);
	FileReadInt(theFile, &mBestWordThisGame);
	FileReadInt(theFile, &mUnused3e8);
	FileReadInt(theFile, &mUnused3ec);

	// the game log, as a web string
	uint32_t aLogLength = 0;
	fread(&aLogLength, 1, 4, theFile);
	if ((int)aLogLength > 0)
	{
		mGameLog.Clear();
		aString.resize(aLogLength, '\0');
		fread(&aString[0], 1, aLogLength, theFile);
		mGameLog.FromWebString(aString);
	}

	*isHelpOpen = false;
	if (aVersion > 3)
		FileReadBool(theFile, isHelpOpen);
	return true;
}

void Board::GetGlobalState(int* theLastLetterIdx, int* theLastRefillLevel)
{
	*theLastLetterIdx = gLastLetterIdx;
	*theLastRefillLevel = gLastRefillLevel;
}

void Board::SetGlobalState(int theLastLetterIdx, int theLastRefillLevel)
{
	gLastLetterIdx = theLastLetterIdx;
	gLastRefillLevel = theLastRefillLevel;
}

// port(mod): a record SaveGameSnapshot made, read back (the undo mod, Undo.h)
bool Board::ReadSavedGame(const Buffer& theRecord)
{
	FILE* aFile = tmpfile();
	if (aFile == NULL)
		return false;
	if (theRecord.GetDataLen() > 0)
		fwrite(theRecord.GetDataPtr(), 1, (size_t)theRecord.GetDataLen(), aFile);
	fseek(aFile, 0, SEEK_SET);
	bool isHelpOpen = false;
	bool isRead = ReadSavedGame(aFile, &isHelpOpen);
	fclose(aFile);
	return isRead;
}

// 0x4078b0 (the app's 2005 / 3005, LevelUpWidget): the saved game is single-use
bool Board::DeleteSavedGame(int theMode)
{
	SexyString aPath = BookWormApp::GetSaveGamePath(mApp->mUserManager->GetCurrentUserName(), theMode);
	if (aPath.size() == 0)
		return false;

	// port: DeleteFileA next to the exe -> the framework's EraseFile in the user dir
	return mApp->EraseFile(aPath);
}
