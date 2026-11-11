// Test hooks: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script or live command
// channel (-commands, byopop_test_command) through HandleScriptCommand. A command that is never given changes nothing:
// no random number is drawn and no state moves (the bot behind `move` reads the game and draws none). Each answers on
// the log with one line: STATE {...}, MOVE {...} or MOVE null, OK <command> or ERROR <command>: <reason>.
//
//   state              the game's state as one JSON line (the keys are listed in games/waterbugs/docs/GAME_NOTES.md)
//   goto <screen>      menu (title), options, help, help2, highscores, credits, map, changeuser, logo
//   level <n>          board n of the game (1 is the first; 0 too), through the game's own level change
//   unlock all         every level of the campaign counts as reached in the current profile
//   set <name> <v>     score, lives, difficulty (-1, 0, 1), shootermode (0 manual, 1 auto), reached (levels finished)
//   win, lose          the board ends as the game ends it when it is filled, or when the last life goes
//   seed <n>           seeds the game's generator (SeedRand) and the C library's (CrtSrand)
//   move               the move the bot would make next (board: where to aim, turn or fire; every other screen: the
//                      click that gets it out of the way)

#include "TestCommands.h"

#include "../../../../platform/MemReport.h"

#include "WBGlobals.h"
#include "WBLevel.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <SDL.h>

using namespace Sexy;

namespace wb
{

namespace
{

std::string Fmt(const char* theFormat, ...)
{
	char aBuffer[1024];
	va_list anArgs;
	va_start(anArgs, theFormat);
	std::vsnprintf(aBuffer, sizeof(aBuffer), theFormat, anArgs);
	va_end(anArgs);
	return aBuffer;
}

void Reply(const std::string& theLine)
{
	SDL_Log("%s", theLine.c_str());
}

std::string CommandText(const ScriptCommand& theCommand)
{
	return theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);
}

void ReplyOK(const ScriptCommand& theCommand)
{
	Reply("OK " + CommandText(theCommand));
}

void ReplyError(const ScriptCommand& theCommand, const std::string& theReason)
{
	Reply("ERROR " + CommandText(theCommand) + ": " + theReason);
}

std::string Quote(const char* theText)
{
	std::string aResult = "\"";
	for (const char* p = theText; *p != 0; p++)
	{
		if (*p == '"' || *p == '\\')
			aResult += '\\';
		if ((unsigned char)*p >= 0x20)
			aResult += *p;
	}
	return aResult + "\"";
}

// a board's story screen (the world's intro: no cells yet, and its own start button) is a board of the game's levels
bool IsStory(const Level* theLevel)
{
	return theLevel != nullptr && theLevel->mKind == 0 && theLevel->mTotalCells == 0;
}

const char* ScreenName(const Level* theLevel)
{
	if (theLevel == nullptr)
		return "loading";
	switch (theLevel->mKind)
	{
	case 0: return IsStory(theLevel) ? "story" : "board";
	case 1: return "menu";
	case 2: return "nameentry";
	case 3: return "highscores";
	case 4: return "credits";
	case 5: return "map";
	case 6: return "map2";
	case 7: return "options";
	case 8: case 9: case 10: return "nag";
	case 0xb: return "complete";
	case 0xc: return "help";
	case 0xd: return "help2";
	case 0xe: return "logo";
	case 0x10: return "changeuser";
	}
	return "unknown";
}

// ---------------------------------------------------------------------------------------------------------------------
// Menus: the items a click can reach on the current screen, and where to click each

struct ItemRef
{
	MenuItem* mItem;
	Menu* mMenu;
	bool mRequester;
};

// the menu a requester is up on (the main menu's "resume your previous game?" is on its last menu, the others' on the
// current one)
Menu* RequesterMenu(Level* theLevel)
{
	if (theLevel->mMenus.mCurrent != nullptr && theLevel->mMenus.mCurrent->mRequesters.mCurrent != nullptr)
		return theLevel->mMenus.mCurrent;
	for (Menu* aMenu = theLevel->mMenus.mHead; aMenu != nullptr; aMenu = Node_Next(aMenu))
		if (aMenu->mRequesters.mCurrent != nullptr)
			return aMenu;
	return nullptr;
}

std::vector<ItemRef> ActiveItems(Level* theLevel)
{
	std::vector<ItemRef> aResult;
	Menu* aTop = RequesterMenu(theLevel);
	if (aTop != nullptr)
	{
		Menu* aRequester = aTop->mRequesters.mCurrent;
		for (MenuItem* anItem = aRequester->mItems.mHead; anItem != nullptr; anItem = Node_Next(anItem))
			aResult.push_back({anItem, aRequester, true});
		return aResult;
	}
	for (Menu* aMenu = theLevel->mMenus.mHead; aMenu != nullptr; aMenu = Node_Next(aMenu))
		for (MenuItem* anItem = aMenu->mItems.mHead; anItem != nullptr; anItem = Node_Next(anItem))
			aResult.push_back({anItem, aMenu, false});
	return aResult;
}

bool HasRequester(Level* theLevel)
{
	return RequesterMenu(theLevel) != nullptr;
}

// the point inside an item's hit box that MenuItem_Update tests: its animation's image about (x, y), else its text
void ItemPoint(const MenuItem* theItem, int& theX, int& theY)
{
	theX = (int)theItem->mX;
	theY = (int)theItem->mY;
	if (theItem->mAnim == nullptr && theItem->mFont != nullptr)
		theY += Font_StringHeight(theItem->mFont, theItem->mShown) / 2;
}

// ---------------------------------------------------------------------------------------------------------------------
// The board

struct Shot
{
	bool mValid = false;
	int mCol = 0;
	int mRow = 0;
	int mAcross = 0;
	int mLo = 0;			// the open stretch through the cell, in cells along the beam
	int mHi = 0;
	int mFill = 0;		// the cells the game would fill
	int mCellsA = 0;
	int mCellsB = 0;
	double mTime = 0;	// updates the beams take to stop
	double mRisk = 0;	// 0 safe .. the sum of how early each bug could reach a growing beam
	double mScore = -1e9;
};

int gBan[2][64];		// per beam direction and cell line: the shots along it that cost a life on this board
int gLastLives = -1;
int gLastShotAcross = -1;
int gLastShotLine = -1;

class BoardBot
{
public:
	Level* mLevel;
	Shooter* mShip;
	int mCols;
	int mRows;
	bool mOpen[50][37];
	int mEnemyAt[50][37];
	int mMark[50][37];
	int mGen = 0;

	explicit BoardBot(Level* theLevel) : mLevel(theLevel), mShip(theLevel->mShooters.mCurrent)
	{
		mCols = std::min(theLevel->mCols, 50);
		mRows = std::min(theLevel->mRows, 37);
		std::memset(mEnemyAt, 0, sizeof(mEnemyAt));
		std::memset(mMark, 0, sizeof(mMark));
		for (int c = 0; c < mCols; c++)
			for (int r = 0; r < mRows; r++)
				mOpen[c][r] = theLevel->mColumns[c][r].mKind == 0;
		for (Enemy* e = theLevel->mEnemies.mHead; e != nullptr; e = Node_Next(e))
		{
			int c = (int)e->mX / 16;
			int r = (int)e->mY / 16;
			if (0 <= c && c < mCols && 0 <= r && r < mRows)
				mEnemyAt[c][r]++;
		}
	}

	bool Open(int c, int r) const { return 0 <= c && c < mCols && 0 <= r && r < mRows && mOpen[c][r]; }

	// the open stretch through (col, row) along the beam
	void Stretch(int theCol, int theRow, int theAcross, int& theLo, int& theHi) const
	{
		if (theAcross != 0)
		{
			theLo = theHi = theCol;
			while (Open(theLo - 1, theRow)) theLo--;
			while (Open(theHi + 1, theRow)) theHi++;
		}
		else
		{
			theLo = theHi = theRow;
			while (Open(theCol, theLo - 1)) theLo--;
			while (Open(theCol, theHi + 1)) theHi++;
		}
	}

	// the open region about (sc, sr) outside the beam's cells: its cell count and enemies (the flood of Region_Count)
	void Flood(int theSc, int theSr, int theCol, int theRow, int theAcross, int theLo, int theHi, int theGen,
		int& theCells, int& theEnemies)
	{
		theCells = 0;
		theEnemies = 0;
		if (!Open(theSc, theSr) || OnLine(theSc, theSr, theCol, theRow, theAcross, theLo, theHi))
			return;
		static int sAStack[50 * 37 * 2];
		int aTop = 0;
		mMark[theSc][theSr] = theGen;
		sAStack[aTop++] = theSc * 64 + theSr;
		while (aTop > 0)
		{
			int v = sAStack[--aTop];
			int c = v / 64, r = v % 64;
			theCells++;
			theEnemies += mEnemyAt[c][r];
			static const int NEIGHBOUR_STEPS[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
			for (int d = 0; d < 4; d++)
			{
				int aNc = c + NEIGHBOUR_STEPS[d][0], aNr = r + NEIGHBOUR_STEPS[d][1];
				if (Open(aNc, aNr) && mMark[aNc][aNr] != theGen && !OnLine(aNc, aNr, theCol, theRow, theAcross, theLo,
					theHi))
				{
					mMark[aNc][aNr] = theGen;
					sAStack[aTop++] = aNc * 64 + aNr;
				}
			}
		}
	}

	static bool OnLine(int c, int r, int theCol, int theRow, int theAcross, int theLo, int theHi)
	{
		return theAcross != 0 ? (r == theRow && theLo <= c && c <= theHi) : (c == theCol && theLo <= r && r <= theHi);
	}

	Shot Evaluate(int theCol, int theRow, int theAcross)
	{
		Shot aShot;
		aShot.mCol = theCol;
		aShot.mRow = theRow;
		aShot.mAcross = theAcross;
		if (!Open(theCol, theRow) || mLevel->mColumns[theCol][theRow].mUnmarked == 0)
			return aShot;
		Stretch(theCol, theRow, theAcross, aShot.mLo, aShot.mHi);
		int aGenA = ++mGen;
		int aSideAC = theAcross != 0 ? theCol : theCol - 1, aSideAR = theAcross != 0 ? theRow - 1 : theRow;
		int aSideBC = theAcross != 0 ? theCol : theCol + 1, aSideBR = theAcross != 0 ? theRow + 1 : theRow;
		int anEnemiesA = 0, anEnemiesB = 0;
		Flood(aSideAC, aSideAR, theCol, theRow, theAcross, aShot.mLo, aShot.mHi, aGenA, aShot.mCellsA, anEnemiesA);
		if (aShot.mCellsA == 0 || aShot.mCellsB < 0)
			return aShot;
		if (Open(aSideBC, aSideBR) && mMark[aSideBC][aSideBR] == aGenA)
			return aShot;		// the beam does not part the two sides
		int aGenB = ++mGen;
		Flood(aSideBC, aSideBR, theCol, theRow, theAcross, aShot.mLo, aShot.mHi, aGenB, aShot.mCellsB, anEnemiesB);
		if (aShot.mCellsB == 0)
			return aShot;
		if (mLevel->mNumber > 2 && anEnemiesA == 0 && anEnemiesB == 0)
			aShot.mFill = aShot.mCellsA + aShot.mCellsB;
		else
			aShot.mFill = std::min(aShot.mCellsA, aShot.mCellsB);
		aShot.mValid = true;
		// the time the beams take, and the bugs that could reach a growing beam before they stop
		double aSpeed = mShip->mBeamSpeed > 0.01f ? mShip->mBeamSpeed : 0.14f;
		double aHalfLow = (theCol - 0) * 16.0 + 8.0, aHalfHigh = 0;
		double aShotX = theCol * 16.0 + 8.0, aShotY = theRow * 16.0 + 8.0;
		if (theAcross != 0)
		{
			aHalfLow = aShotX - aShot.mLo * 16.0;
			aHalfHigh = (aShot.mHi + 1) * 16.0 - aShotX;
		}
		else
		{
			aHalfLow = aShotY - aShot.mLo * 16.0;
			aHalfHigh = (aShot.mHi + 1) * 16.0 - aShotY;
		}
		aShot.mTime = std::max(aHalfLow, aHalfHigh) / aSpeed;
		double aLeft = theAcross != 0 ? aShot.mLo * 16.0 : theCol * 16.0 + 4.0;
		double aRight = theAcross != 0 ? (aShot.mHi + 1) * 16.0 : theCol * 16.0 + 12.0;
		double aTop = theAcross != 0 ? theRow * 16.0 + 4.0 : aShot.mLo * 16.0;
		double aBottom = theAcross != 0 ? theRow * 16.0 + 12.0 : (aShot.mHi + 1) * 16.0;
		double aRisk = 0;
		for (Enemy* anEnemy = mLevel->mEnemies.mHead; anEnemy != nullptr; anEnemy = Node_Next(anEnemy))
		{
			double aRadius = anEnemy->mImage != nullptr ? anEnemy->mImage->mH / 3.0 : 8.0;
			double aPx = std::min(std::max((double)anEnemy->mX, aLeft), aRight);
			double aPy = std::min(std::max((double)anEnemy->mY, aTop), aBottom);
			double aDist = std::sqrt(((double)anEnemy->mX - aPx) * ((double)anEnemy->mX - aPx) +
				((double)anEnemy->mY - aPy) * ((double)anEnemy->mY - aPy)) - aRadius;
			double v = std::sqrt((double)anEnemy->mVx * anEnemy->mVx + (double)anEnemy->mVy * anEnemy->mVy) * 1.3 +
				0.02;
			double t = std::max(0.0, aDist) / v;
			double aAlong = theAcross != 0 ? aPx - aShotX : aPy - aShotY;
			if (t < aShot.mTime && std::fabs(aAlong) <= aSpeed * t + 12.0)
				aRisk += (aShot.mTime - t) / aShot.mTime;
			// a bug in the band of the beam, anywhere along it, is cut by the beam unless it leaves the band first
			double aPerp = theAcross != 0 ? std::fabs((double)anEnemy->mY - (theRow * 16.0 + 8.0)) :
				std::fabs((double)anEnemy->mX - (theCol * 16.0 + 8.0));
			double aPos = theAcross != 0 ? (double)anEnemy->mX : (double)anEnemy->mY;
			double aBandLow = (theAcross != 0 ? aShot.mLo : aShot.mLo) * 16.0 - aRadius;
			double aBandHigh = (aShot.mHi + 1) * 16.0 + aRadius;
			if (aPerp <= aRadius + 10.0 && aBandLow <= aPos && aPos <= aBandHigh)
				aRisk += 1.0;
		}
		aShot.mRisk = aRisk;
		double aFill = std::min<double>(aShot.mFill, 400);
		aShot.mScore = (aRisk < 0.03) ? aFill : aFill * std::max(0.0, 1.0 - aRisk * 2.0) - 1000.0;
		return aShot;
	}

	// the best shot on the board: every second cell, both ways (or the way the ship turns itself)
	Shot Best(bool theAuto)
	{
		Shot aBest;
		for (int aCol = 1; aCol < mCols - 1; aCol += 2)
		{
			for (int aRow = 1; aRow < mRows - 1; aRow += 2)
			{
				if (!mOpen[aCol][aRow])
					continue;
				int aLoA, aHiA, aLoD, aHiD;
				Stretch(aCol, aRow, 1, aLoA, aHiA);
				Stretch(aCol, aRow, 0, aLoD, aHiD);
				int aSpanA = aHiA - aLoA, aSpanD = aHiD - aLoD;
				for (int anAcross = 0; anAcross < 2; anAcross++)
				{
					if (gBan[anAcross][anAcross != 0 ? aRow : aCol] > 0)
						continue;
					if (theAuto && ((anAcross != 0 && aSpanA > aSpanD) || (anAcross == 0 && aSpanD > aSpanA)))
						continue;
					Shot s = Evaluate(aCol, aRow, anAcross);
					if (s.mValid && (!aBest.mValid || s.mScore > aBest.mScore))
						aBest = s;
				}
			}
		}
		return aBest;
	}
};

int OrbitingWeapons(Level* theLevel)
{
	int aCount = 0;
	for (Weapon* w = theLevel->mWeapons.mHead; w != nullptr; w = Node_Next(w))
		if (w->mState == 3)
			aCount++;
	return aCount;
}

std::string ClickSteps(int theX, int theY, int theWait = 2)
{
	return Fmt("{\"press\":[%d,%d]},{\"wait\":%d},{\"release\":[%d,%d]}", theX, theY, theWait, theX, theY);
}

std::string MoveJson(const char* theKind, const std::string& theSteps, int theUpdates, const std::string& theExtra = "")
{
	return Fmt("MOVE {\"kind\":\"%s\",\"input\":[%s],\"updates\":%d", theKind, theSteps.c_str(),
		theUpdates) + theExtra + "}";
}

// the aim the bot keeps between its calls (the shot it moved the ship for), so that a fresh look at the board does not
// send the ship back and forth
struct Aim
{
	bool mSet = false;
	int mCol = 0;
	int mRow = 0;
	int mAcross = 0;
	int mUpdate = 0;
};
Aim gAim;
int gUpdateCount = 0;	// the framework's update (10 ms each), as the last command saw it
int gLastBoardNumber = -1;
int gRealX = 0, gRealY = 0;	// the driver's pointer (window pixels), as the last command saw it
int gWrongWay = 0;		// the calls in a row that found the ship over its target cell turned the wrong way

// what the bot keeps between its calls, so that `state` can ask it without changing it (state's busy is the bot's)
struct BotMemory
{
	Aim mAim;
	int mLastBoardNumber, mLastLives, mLastShotAcross, mLastShotLine, mWrongWay;
	int mBan[2][64];
};

BotMemory SaveBot()
{
	BotMemory m;
	m.mAim = gAim;
	m.mLastBoardNumber = gLastBoardNumber;
	m.mLastLives = gLastLives;
	m.mLastShotAcross = gLastShotAcross;
	m.mLastShotLine = gLastShotLine;
	m.mWrongWay = gWrongWay;
	std::memcpy(m.mBan, gBan, sizeof(gBan));
	return m;
}

void RestoreBot(const BotMemory& m)
{
	gAim = m.mAim;
	gLastBoardNumber = m.mLastBoardNumber;
	gLastLives = m.mLastLives;
	gLastShotAcross = m.mLastShotAcross;
	gLastShotLine = m.mLastShotLine;
	gWrongWay = m.mWrongWay;
	std::memcpy(gBan, m.mBan, sizeof(gBan));
}

std::string MenuMove(Level* theLevel, bool& theBusy);

// the ship's cell and its pixel position
std::string BoardMove(Level* theLevel, bool& theBusy)
{
	theBusy = false;
	if (theLevel->mNumber != gLastBoardNumber)
	{
		gLastBoardNumber = theLevel->mNumber;
		gAim.mSet = false;
		std::memset(gBan, 0, sizeof(gBan));
		gLastShotLine = -1;
	}
	if (gLives < gLastLives && gLastShotLine >= 0)
	{
		// the last shot cost a life: not that line again
		gBan[gLastShotAcross][gLastShotLine] = 1;
		gLastShotLine = -1;
	}
	gLastLives = gLives;
	if (theLevel->mGameOver != 0)
	{
		// the game over: a click leaves it
		return MoveJson("gameover", ClickSteps(400, 300), 5);
	}
	if (theLevel->mState == 3)
	{
		// won: the fill and its texts, then a click shows the results, and the results' button leaves
		if (HasRequester(theLevel))
		{
			std::vector<ItemRef> anItems = ActiveItems(theLevel);
			for (const ItemRef& r : anItems)
			{
				int x, y;
				ItemPoint(r.mItem, x, y);
				return MoveJson("next", ClickSteps(x, y), 5);
			}
		}
		if (theLevel->mTexts.mHead != nullptr && theLevel->mClicked == 0)
			return MoveJson("results", ClickSteps(400, 300), 5);
		theBusy = true;
		return "";
	}
	if (HasRequester(theLevel))
	{
		// the quit requester or a prompt: the first button that is not a yes
		std::vector<ItemRef> anItems = ActiveItems(theLevel);
		if (!anItems.empty())
		{
			int x, y;
			ItemPoint(anItems.back().mItem, x, y);
			return MoveJson("dialog", ClickSteps(x, y), 5);
		}
		theBusy = true;
		return "";
	}
	Shooter* aShip = theLevel->mShooters.mCurrent;
	if (aShip == nullptr || aShip->mState != 3 || theLevel->mState != 1)
	{
		theBusy = true;
		return "";
	}
	int aShipX = (int)aShip->mX, aShipY = (int)aShip->mY;
	if (OrbitingWeapons(theLevel) > 0)
		return MoveJson("launch", ClickSteps(aShipX, aShipY), 3);
	// the quota is passed: the level is won once the fill is done and no power-up is falling (the ship takes it up)
	if (theLevel->mTotalCells > 0 && theLevel->mFillQuota <= (theLevel->mFilledCells * 100) / theLevel->mTotalCells)
	{
		Powerup* aPowerup = theLevel->mPowerups.mHead;
		if (aPowerup != nullptr && aPowerup->mState == 4)
		{
			int aX = std::min(std::max((int)aPowerup->mX, 0), 799),
				aY = std::min(std::max((int)aPowerup->mY + 24, 0), 599);
			if (std::abs(aX - aShipX) > 6 || std::abs(aY - aShipY) > 6)
				return MoveJson("powerup", Fmt("{\"move\":[%d,%d]},{\"wait\":2}", aX, aY), 1);
		}
		theBusy = true;
		return "";
	}
	BoardBot aBot(theLevel);
	bool anAuto = aShip->mNoAutoTurn == 0;
	Shot aBest = aBot.Best(anAuto);
	if (!aBest.mValid)
	{
		// nothing parts the board any more: fire where the ship is, the quota may be a few cells away
		theBusy = true;
		return "";
	}
	int aShipCol = aShipX / 16, aShipRow = aShipY / 16;
	Shot aHere = aBot.Evaluate(aShipCol, aShipRow, aShip->mBeamAcross);
	Shot aKept;
	if (gAim.mSet && gBan[gAim.mAcross][gAim.mAcross != 0 ? gAim.mRow : gAim.mCol] == 0)
		aKept = aBot.Evaluate(gAim.mCol, gAim.mRow, gAim.mAcross);
	// the aim: the kept one while it is still worth a shot, else the best
	Shot aTarget = (aKept.mValid && aKept.mScore >= 0.7 * aBest.mScore) ? aKept : aBest;
	bool aAtTarget = aShipCol == aTarget.mCol && aShipRow == aTarget.mRow && aShip->mBeamAcross == aTarget.mAcross;
	bool aHereBanned = gBan[aShip->mBeamAcross != 0 ? 1 : 0][aShip->mBeamAcross != 0 ? aShipRow : aShipCol] > 0;
	if (aHere.mValid && !aHereBanned &&
		(aAtTarget || (aHere.mScore >= 0.7 * aBest.mScore && aHere.mRisk <= aBest.mRisk + 0.05)))
	{
		gAim.mSet = false;
		gLastShotAcross = aShip->mBeamAcross;
		gLastShotLine = aShip->mBeamAcross != 0 ? aShipRow : aShipCol;
		return MoveJson("shot", ClickSteps(aShipX, aShipY),
			1, Fmt(",\"col\":%d,\"row\":%d,\"across\":%d,\"fill\":%d,\"risk\":%.2f", aShipCol, aShipRow,
				aShip->mBeamAcross, aHere.mFill, aHere.mRisk));
	}
	int aTargetX = aTarget.mCol * 16 + 8, aTargetY = aTarget.mRow * 16 + 8;
	if (aShipCol == aTarget.mCol && aShipRow == aTarget.mRow && aShip->mBeamAcross != aTarget.mAcross)
	{
		// the ship is over the cell but turned the wrong way: a right click turns it, after the ship has had one look
		// at the span and turned itself (it keeps its way when the two spans are equal)
		if (!anAuto || gWrongWay++ > 0)
		{
			gWrongWay = 0;
			return MoveJson("turn", Fmt("{\"press\":[%d,%d],\"button\":\"right\"},{\"wait\":2},{\"release\":[%d,%d],\"button\":\"right\"}", aShipX, aShipY, aShipX, aShipY), 2);
		}
	}
	else
		gWrongWay = 0;
	gAim.mSet = true;
	gAim.mCol = aTarget.mCol;
	gAim.mRow = aTarget.mRow;
	gAim.mAcross = aTarget.mAcross;
	double aDist = std::sqrt((double)(aTargetX - aShipX) * (aTargetX - aShipX) + (double)(aTargetY - aShipY) *
		(aTargetY - aShipY));
	int aWait = 3 + (int)std::ceil(std::log2(std::max(1.0, aDist)));
	return MoveJson("aim", Fmt("{\"move\":[%d,%d]},{\"wait\":%d}", aTargetX, aTargetY, aWait), 1,
		Fmt(",\"col\":%d,\"row\":%d,\"across\":%d,\"fill\":%d,\"risk\":%.2f,\"here\":[%d,%d,%d,%d,%.1f]", aTarget.mCol,
			aTarget.mRow, aTarget.mAcross, aTarget.mFill, aTarget.mRisk, aShipCol, aShipRow, aShip->mBeamAcross,
			aHere.mValid ? 1 : 0, aHere.mScore));
}

// ---------------------------------------------------------------------------------------------------------------------
// The state

bool gPlayed = false;		// a board has been up since the game started (the high scores are then the game's end)

bool IsOver(Level* theLevel)
{
	if (theLevel == nullptr || !gPlayed)
		return false;
	return theLevel->mKind == 2 || theLevel->mKind == 3 || theLevel->mKind == 0xb;
}

bool IsBusy(Level* theLevel)
{
	if (theLevel == nullptr)
		return true;
	if (theLevel->mKind == 0xe)
		return true;
	return false;
}

std::string StateJson()
{
	Level* aLevel = gLevels.mCurrent;
	if (aLevel != nullptr && aLevel->mKind == 0 && !IsStory(aLevel))
		gPlayed = true;
	std::string s = Fmt("{\"screen\":\"%s\",\"mode\":\"campaign\",\"update\":%d,\"frame\":%d,\"score\":%d,\"lives\":%d,\"level\":%d,\"reached\":%d,"
		"\"difficulty\":%d,\"shootermode\":%d,\"rand\":%d,\"over\":%s",
		ScreenName(aLevel), gUpdateCount, gFrame, gScore, gLives,
			aLevel != nullptr && aLevel->mKind == 0 && !IsStory(aLevel) ? aLevel->mNumber : -1,
		gProfiles[gProfile].mLevelReached, gDifficulty, gAutoFire, RandValue(), IsOver(aLevel) ? "true" : "false");
	if (aLevel == nullptr)
		return s + "}";
	bool aBusy = IsBusy(aLevel);
	if (aLevel->mKind == 0 && !IsStory(aLevel))
	{
		Shooter* aShip = aLevel->mShooters.mCurrent;
		s += Fmt(",\"percent\":%d,\"quota\":%d",
			aLevel->mTotalCells > 0 ? (aLevel->mFilledCells * 100) / aLevel->mTotalCells : 0, aLevel->mFillQuota);
		s += Fmt(",\"board\":{\"cols\":%d,\"rows\":%d,\"state\":%d,\"gameover\":%s,\"shots\":%d,\"lines\":%d,\"cells\":%d,\"filled\":%d,\"weapons\":%d,\"texts\":%d", aLevel->mCols, aLevel->mRows, aLevel->mState, aLevel->mGameOver != 0 ? "true" : "false", aLevel->mShots, aLevel->mLines, aLevel->mTotalCells, aLevel->mFilledCells, OrbitingWeapons(aLevel), aLevel->mTexts.mHead != nullptr ? 1 : 0);
		if (aShip != nullptr)
			s += Fmt(",\"ship\":{\"x\":%d,\"y\":%d,\"state\":%d,\"across\":%d,\"noauto\":%d,\"speed\":%.3f}",
				(int)aShip->mX, (int)aShip->mY, aShip->mState, aShip->mBeamAcross, aShip->mNoAutoTurn,
				aShip->mBeamSpeed);
		s += Fmt(",\"mouse\":[%d,%d],\"pointer\":[%d,%d]", gMouseX, gMouseY, gRealX, gRealY);
		s += ",\"powerups\":[";
		int p = 0;
		for (Powerup* u = aLevel->mPowerups.mHead; u != nullptr && p < 20; u = Node_Next(u), p++)
			s += Fmt("%s[%d,%d,%d]", p > 0 ? "," : "", (int)u->mX, (int)u->mY, u->mState);
		s += "]";
		s += ",\"enemies\":[";
		int n = 0;
		for (Enemy* e = aLevel->mEnemies.mHead; e != nullptr && n < 60; e = Node_Next(e), n++)
			s += Fmt("%s[%d,%d,%.3f,%.3f,%d,%d]", n > 0 ? "," : "", (int)e->mX, (int)e->mY, e->mVx, e->mVy, e->mKind,
				e->mState);
		s += "]}";
	}
	if (aLevel->mKind != 0 || IsStory(aLevel) || HasRequester(aLevel))
	{
		s += Fmt(",\"dialog\":%s,\"items\":[", HasRequester(aLevel) ? "true" : "false");
		std::vector<ItemRef> anItems = ActiveItems(aLevel);
		for (size_t i = 0; i < anItems.size(); i++)
		{
			int x, y;
			ItemPoint(anItems[i].mItem, x, y);
			s += Fmt("%s[%d,%d,%d,%s,%d]", i > 0 ? "," : "", anItems[i].mItem->mKind, x, y,
				Quote(anItems[i].mItem->mName).c_str(), anItems[i].mMenu->mLockedLevel);
		}
		s += "]";
		s += ",\"sets\":[";
		for (LevelSet* l = gLevelSets.mHead; l != nullptr; l = Node_Next(l))
			s += Fmt("%s[%d,%d]", l == gLevelSets.mHead ? "" : ",", l->mStart, l->mEnd);
		s += "]";
	}
	{
		// busy: the bot has nothing to do but wait (it is asked without being changed)
		BotMemory aMemory = SaveBot();
		bool aWaiting = false;
		std::string aMove = (aLevel->mKind == 0 && !IsStory(aLevel)) ? BoardMove(aLevel, aWaiting) : MenuMove(aLevel,
			aWaiting);
		RestoreBot(aMemory);
		aBusy = aBusy || (aMove.empty() && aWaiting);
	}
	s += Fmt(",\"busy\":%s}", aBusy ? "true" : "false");
	return s;
}

// ---------------------------------------------------------------------------------------------------------------------
// Every other screen: the click that gets it out of the way

const ItemRef* FindKind(const std::vector<ItemRef>& theItems, int theKind)
{
	for (const ItemRef& r : theItems)
		if (r.mItem->mKind == theKind)
			return &r;
	return nullptr;
}

std::string ClickItem(const char* theKind, const MenuItem* theItem)
{
	int x, y;
	ItemPoint(theItem, x, y);
	return MoveJson(theKind, ClickSteps(x, y), 5);
}

std::string MenuMove(Level* theLevel, bool& theBusy)
{
	theBusy = false;
	std::vector<ItemRef> anItems = ActiveItems(theLevel);
	if (theLevel->mKind == 0xe || anItems.empty())
	{
		// the logo screens move on by themselves
		theBusy = true;
		return "";
	}
	if (HasRequester(theLevel))
		// the last button: not the yes of "resume your previous game?"
		return ClickItem("dialog", anItems.back().mItem);
	const ItemRef* aRef = nullptr;
	switch (theLevel->mKind)
	{
	case 1:
		aRef = FindKind(anItems, 0x12);
		if (aRef != nullptr)
			return ClickItem("start", aRef->mItem);
		break;
	case 0:
		// a world's story screen: its start button
		aRef = FindKind(anItems, 1);
		if (aRef != nullptr)
			return ClickItem("story", aRef->mItem);
		break;
	case 5:
	{
		// the world map: the furthest world that is open (its first level is no further than the next one to play)
		const ItemRef* aBest = nullptr;
		for (const ItemRef& i : anItems)
			if (i.mItem->mKind == 0x18 && i.mMenu->mLockedLevel <= gProfiles[gProfile].mLevelReached + 1 &&
				(aBest == nullptr || i.mMenu->mLockedLevel > aBest->mMenu->mLockedLevel))
				aBest = &i;
		if (aBest != nullptr)
			return ClickItem("world", aBest->mItem);
		break;
	}
	}
	return ClickItem("menu", anItems[0].mItem);
}

// ---------------------------------------------------------------------------------------------------------------------
// The commands

int ScreenKind(const std::string& theName)
{
	static const struct { const char* mName; int mKind; } SCREEN_NAMES[] = {{"menu", 1}, {"title", 1}, {"options", 7},
		{"help", 0xc},
		{"help2", 0xd}, {"highscores", 3}, {"credits", 4}, {"map", 5}, {"changeuser", 0x10}, {"logo", 0xe}};
	for (const auto& s : SCREEN_NAMES)
		if (theName == s.mName)
			return s.mKind;
	return -1;
}

LevelSet* SetOfLevel(int theLevel)
{
	for (LevelSet* s = gLevelSets.mHead; s != nullptr; s = Node_Next(s))
		if (s->mStart <= theLevel && theLevel <= s->mEnd)
			return s;
	return nullptr;
}

}	// namespace

// The memory report's holders of the engine: what the engine's own lists keep, which the framework cannot see
void AddMemReportHolders(Platform::MemReport* theReport)
{
	size_t aBankBytes = (size_t)gArtBank.mDataSize + (size_t)gArt8Bank.mDataSize;
	size_t aFrameBytes = 0, aMaskBytes = 0, aRleBytes = 0, aPaletteBytes = 0;
	int aFrames = 0;
	auto aAddRle = [&](const r64::Image* theImage)
	{
		if (theImage != nullptr && theImage->mRle != nullptr)
			aRleBytes += ((const RleHeader*)theImage->mRle)->mSize;
	};
	for (const Bank* aBank : {&gArtBank, &gArt8Bank})
	{
		for (const r64::Image* anImage : aBank->mImages)
			aAddRle(anImage->mMask != nullptr ? anImage->mMask : anImage->mAutoMask);
		for (const Palette* aPalette : aBank->mPalettes)
			aPaletteBytes += sizeof(Palette) + (aPalette->mP16 != nullptr ? 512 : 0) +
				(aPalette->mP32 != nullptr ? 1024 : 0);
		aPaletteBytes += aBank->mRawPalettes.size() * 0x400;
	}
	for (const Animation* anAnim = gAnims.mHead; anAnim != nullptr; anAnim = Node_Next(anAnim))
	{
		aPaletteBytes += sizeof(Animation) - sizeof(Animation::mFrames);
		for (int i = 0; i < anAnim->mCount; i++)
		{
			const r64::Image* aFrame = anAnim->mFrames[i];
			if (aFrame == nullptr)
				continue;
			aFrames++;
			if (aFrame->mNotOwned == 0)
				aFrameBytes += (size_t)aFrame->mDataSize;
			if (aFrame->mHasMask != 0 && aFrame->mMask != nullptr)
			{
				if (aFrame->mMask->mNotOwned == 0)
					aMaskBytes += (size_t)aFrame->mMask->mDataSize;
				aAddRle(aFrame->mMask);
			}
		}
	}
	theReport->mOthers.push_back({"art banks' pixels and masks", aBankBytes});
	theReport->mOthers.push_back({"animation frames (" + std::to_string(aFrames) + ")", aFrameBytes});
	theReport->mOthers.push_back({"animation masks", aMaskBytes});
	theReport->mOthers.push_back({"run-length masks", aRleBytes});
	theReport->mOthers.push_back({"palettes and animation records", aPaletteBytes});
	theReport->mOthers.push_back({"back buffer", (size_t)gBackBuffer.mDataSize});
}

bool HandleTestCommand(SexyAppBase* theApp, const ScriptCommand& theCommand)
{
	const std::string& aName = theCommand.mCommand;
	gUpdateCount = theApp->mUpdateCount;
	gRealX = theApp->mRealMouseX;
	gRealY = theApp->mRealMouseY;
	Level* aLevel = gLevels.mCurrent;
	if (aName == "state")
	{
		Reply("STATE " + StateJson());
		return true;
	}
	if (aName == "grid")
	{
		// the board as text, one line per row: '.' open, '+' open and already marked by the pending beam, '#' not open,
		// 'e' a bug
		if (aLevel == nullptr || aLevel->mKind != 0)
		{
			ReplyError(theCommand, "there is no board");
			return true;
		}
		std::string g;
		for (int r = 0; r < aLevel->mRows && r < 37; r++)
		{
			std::string aRow;
			for (int c = 0; c < aLevel->mCols && c < 50; c++)
				aRow += aLevel->mColumns[c][r].mKind != 0 ? '#' : (aLevel->mColumns[c][r].mUnmarked == 0 ? '+' : '.');
			for (Enemy* e = aLevel->mEnemies.mHead; e != nullptr; e = Node_Next(e))
				if ((int)e->mY / 16 == r && 0 <= (int)e->mX / 16 && (int)e->mX / 16 < (int)aRow.size())
					aRow[(int)e->mX / 16] = 'e';
			g += aRow + (r + 1 < aLevel->mRows ? "|" : "");
		}
		Reply("GRID " + g);
		return true;
	}
	if (aName == "move")
	{
		if (aLevel == nullptr)
		{
			Reply("MOVE null");
			return true;
		}
		bool aBusy = false;
		bool aBoard = aLevel->mKind == 0 && !IsStory(aLevel);
		std::string aMove = aBoard ? BoardMove(aLevel, aBusy) : MenuMove(aLevel, aBusy);
		if (aBoard)
			gPlayed = true;
		Reply(aMove.empty() ? "MOVE null" : aMove);
		return true;
	}
	if (aName == "goto")
	{
		int aKind = ScreenKind(theCommand.mArgs);
		if (aKind < 0)
			ReplyError(theCommand, "screens are menu, options, help, help2, highscores, credits, map, changeuser, logo");
		else if (aLevel == nullptr)
			ReplyError(theCommand, "the game has no screen yet");
		else
		{
			gGoTo = aKind;
			if (!LeaveScreen())
				ReplyError(theCommand, "no such screen");
			else
				ReplyOK(theCommand);
		}
		return true;
	}
	if (aName == "level")
	{
		// board n of the game (0 is the first as well); the story screen that comes before a world's first board comes
		// first. The game's own count of levels left behind is one less (Start after n boards played goes to board n +
		// 1).
		int aCount = std::max(std::atoi(theCommand.mArgs.c_str()), 1);
		if (aLevel == nullptr)
			ReplyError(theCommand, "the game has no screen yet");
		else if (SetOfLevel(aCount) == nullptr)
			ReplyError(theCommand, "no such level");
		else
		{
			bool aFromBoard = aLevel->mKind == 0;
			if (aFromBoard)
			{
				// back to the main menu first, as Esc's quit does, so that the boards are read afresh (that reads the
				// game file again, which makes the level sets new: the set is found after it)
				gGoTo = 1;
				LeaveScreen();
			}
			else
			{
				gLives = 6;
				gScore = 0;
			}
			gLevelSets.mCurrent = SetOfLevel(aCount);
			gLevelInSet = aCount - 1;
			gGoTo = 0;
			gPlayed = true;
			if (!LeaveScreen())
				ReplyError(theCommand, "no such level");
			else
				ReplyOK(theCommand);
		}
		return true;
	}
	if (aName == "unlock")
	{
		if (theCommand.mArgs != "all")
			ReplyError(theCommand, "unlock needs all");
		else
		{
			int aLast = 0;
			for (LevelSet* s = gLevelSets.mHead; s != nullptr; s = Node_Next(s))
				aLast = std::max(aLast, s->mEnd);
			gProfiles[gProfile].mLevelReached = std::max(gProfiles[gProfile].mLevelReached, aLast);
			SaveProfiles();
			ReplyOK(theCommand);
		}
		return true;
	}
	if (aName == "set")
	{
		char aKey[32] = "";
		int aValue = 0;
		if (std::sscanf(theCommand.mArgs.c_str(), "%31s %d", aKey, &aValue) != 2)
			ReplyError(theCommand, "set needs <name> <value>");
		else if (!std::strcmp(aKey, "score"))
			gScore = aValue, ReplyOK(theCommand);
		else if (!std::strcmp(aKey, "lives"))
			gLives = aValue, ReplyOK(theCommand);
		else if (!std::strcmp(aKey, "difficulty") && -1 <= aValue && aValue <= 1)
			gDifficulty = aValue, ReplyOK(theCommand);
		else if (!std::strcmp(aKey, "shootermode") && (aValue == 0 || aValue == 1))
		{
			gAutoFire = aValue;
			gProfiles[gProfile].mAutoFire = aValue;
			ReplyOK(theCommand);
		}
		else if (!std::strcmp(aKey, "reached") && aValue >= 0)
			gProfiles[gProfile].mLevelReached = aValue, ReplyOK(theCommand);
		else
			ReplyError(theCommand, "names are score, lives, difficulty (-1..1), shootermode (0, 1), reached");
		return true;
	}
	if (aName == "win")
	{
		if (aLevel == nullptr || aLevel->mKind != 0 || aLevel->mState != 1)
			ReplyError(theCommand, "no board is being played");
		else
		{
			aLevel->mFilledCells = aLevel->mTotalCells;
			Level_Won(aLevel);
			ReplyOK(theCommand);
		}
		return true;
	}
	if (aName == "lose")
	{
		if (aLevel == nullptr || aLevel->mKind != 0 || aLevel->mState != 1 || aLevel->mGameOver != 0)
			ReplyError(theCommand, "no board is being played");
		else
		{
			gLives = 0;
			Level_GameOver(aLevel);
			ReplyOK(theCommand);
		}
		return true;
	}
	if (aName == "seed")
	{
		unsigned n = (unsigned)std::strtoul(theCommand.mArgs.c_str(), nullptr, 0);
		SeedRand((int)n);
		CrtSrand(n);
		ReplyOK(theCommand);
		return true;
	}
	return false;
}

} // namespace wb
