// port: the standard test commands (docs/TEST_COMMANDS.md) and the bot behind `move`. Nothing in here is reached by a
// normal run, and nothing here draws from the game's generator except `seed`.

#include "CBTestCommands.h"
#include "CBGlobals.h"
#include "CBLevel.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace cb
{

namespace
{

std::string Fmt(const char* theFormat, ...)
{
	char aBuffer[2048];
	va_list anArgs;
	va_start(anArgs, theFormat);
	vsnprintf(aBuffer, sizeof(aBuffer), theFormat, anArgs);
	va_end(anArgs);
	return aBuffer;
}

const char* ScreenName(int theKind)
{
	switch (theKind)
	{
	case LEVEL_BOARD: return "board";
	case LEVEL_MAIN_MENU: return "title";
	case LEVEL_HIGH_SCORE_ENTRY: return "highscore_entry";
	case LEVEL_HIGH_SCORES: return "highscores";
	case LEVEL_CREDITS: return "credits";
	case LEVEL_SELECT_LEVEL: return "levelselect";
	case LEVEL_SELECT_LEVEL2: return "levelselect2";
	case LEVEL_OPTIONS: return "options";
	case LEVEL_MISSION_COMPLETE: return "mission_complete";
	case LEVEL_HELP: return "help";
	case LEVEL_HELP2: return "instructions";
	case LEVEL_LOGO: return "logo";
	case LEVEL_CHANGE_USER: return "change_user";
	}
	return "unknown";
}

const char* DifficultyName()
{
	return gDifficulty < 0 ? "easy" : (gDifficulty == 0 ? "normal" : "hard");
}

const char* ShooterName()
{
	return gAutoFire == 0 ? "manual" : "auto";
}

Shooter* Ship()
{
	Level* aLevel = gLevels.mCurrent;
	return aLevel != nullptr && aLevel->mKind == LEVEL_BOARD ? aLevel->mShooters.mHead : nullptr;
}

int WeaponsOrbiting()
{
	int aCount = 0;
	List_Each(&gLevels.mCurrent->mWeapons, [&](Weapon* theWeapon) {
		if (theWeapon->mState == WEAPON_ORBITING)
			aCount = aCount + 1;
	});
	return aCount;
}

// the enemies Shooter_Fire refuses to fire beside (CBShooter.cpp's Shooter_EnemiesNear)
int EnemiesOnShip(Shooter* theShip)
{
	int aCount = 0;
	List_Each(&gLevels.mCurrent->mEnemies, [&](Enemy* theEnemy) {
		if (CircleOverlap(theShip->mX, theShip->mY, (float)(theShip->mImage->mH / 3), theEnemy->mX, theEnemy->mY,
				(float)(theEnemy->mImage->mH / 3)) != 0.0)
			aCount = aCount + 1;
	});
	return aCount;
}

// Whether a dialog (a requester: the resume question, a name, a quit or delete confirmation) is up on any of the screen's
// menus (the resume question is on the last one, not the current one)
bool HasRequester(Level* theLevel)
{
	for (Menu* aMenu = theLevel->mMenus.mHead; aMenu != nullptr; aMenu = Node_Next(aMenu))
		if (aMenu->mRequesters.mCurrent != nullptr)
			return true;
	return false;
}

int Fill(Level* theLevel)
{
	return theLevel->mTotalCells != 0 ? (theLevel->mFilledCells * 100) / theLevel->mTotalCells : 0;
}

// Whether the board cannot take a move now: the ship is not under the player's control (coming in, firing, hit), the
// board is paused or won, or cells are still being filled
bool Busy(Level* theLevel, Shooter* theShip)
{
	return theShip == nullptr || theShip->mState != SHIP_CONTROLLED || theLevel->mState != LEVEL_PLAYING || theLevel->mGameOver != 0 ||
		theLevel->mFills.mCurrent != nullptr || theLevel->mTotalCells == 0;
}

// ----- state -----

std::string StateJson()
{
	Level* aLevel = gLevels.mCurrent;
	int aKind = aLevel != nullptr ? aLevel->mKind : -1;
	std::string s = Fmt("{\"screen\":\"%s\",\"kind\":%d,\"mode\":\"%s-%s\",\"difficulty\":\"%s\",\"shooter\":\"%s\","
		"\"score\":%d,\"lives\":%d", aLevel != nullptr ? ScreenName(aKind) : "none", aKind, DifficultyName(),
		ShooterName(), DifficultyName(), ShooterName(), gScore, gLives);
	bool aRequester = aLevel != nullptr && HasRequester(aLevel);
	s += Fmt(",\"requester\":%s,\"fullscreen\":%s,\"stretched\":%s,\"updates\":%d", aRequester ? "true" : "false",
		r64::Screen_Fullscreen() != 0 ? "true" : "false", r64::Screen_Centered() == 0 ? "true" : "false", gFrame);
	if (aLevel == nullptr || aKind != LEVEL_BOARD)
	{
		s += Fmt(",\"busy\":false,\"over\":%s}", aKind == LEVEL_MISSION_COMPLETE ? "true" : "false");
		return s;
	}
	Shooter* aShip = Ship();
	bool aBusy = Busy(aLevel, aShip);
	s += Fmt(",\"level\":%d,\"fill\":%d,\"quota\":%d,\"shots\":%d,\"cells\":%d,\"filled\":%d", aLevel->mNumber, Fill(aLevel),
		aLevel->mFillQuota, aLevel->mShots, aLevel->mTotalCells, aLevel->mFilledCells);
	s += Fmt(",\"playing\":%s,\"busy\":%s,\"paused\":%s,\"won\":%s,\"over\":%s", !aBusy ? "true" : "false",
		aBusy ? "true" : "false", aLevel->mState == LEVEL_PAUSED ? "true" : "false", aLevel->mState == LEVEL_WON ? "true" : "false",
		aLevel->mGameOver != 0 ? "true" : "false");
	if (aShip != nullptr)
		s += Fmt(",\"ship\":{\"x\":%d,\"y\":%d,\"state\":%d,\"across\":%s,\"weapons\":%d}", (int)aShip->mX, (int)aShip->mY,
			aShip->mState, aShip->mBeamAcross != 0 ? "true" : "false", WeaponsOrbiting());
	s += ",\"enemies\":[";
	int aCount = 0;
	List_Each(&aLevel->mEnemies, [&](Enemy* theEnemy) {
		if (aCount < 30)
			s += Fmt("%s[%d,%d,%d,%d]", aCount > 0 ? "," : "", (int)theEnemy->mX, (int)theEnemy->mY, theEnemy->mKind, theEnemy->mState);
		aCount++;
	});
	s += Fmt("],\"enemyCount\":%d}", aCount);
	return s;
}

// ----- the bot behind move -----
//
// The board is a grid of 16-pixel cells. A beam pair fired from an open cell runs both ways along its row (or column)
// until the cell ahead is not open; the open cells it cuts off on each side are counted, and the smaller side fills
// (both when the board is past its second and neither holds an enemy), as Shooter_Firing decides. The bot takes the
// line that fills the most cells and that no enemy is expected to reach while the beams run, by a straight-line guess
// of each enemy's path. It reads the board and the enemies and changes nothing, and it uses no random numbers.

const int MAX_COLS = 50;
const int MAX_ROWS = 38;

struct Grid
{
	int mCols;
	int mRows;
	unsigned char mOpen[MAX_COLS][MAX_ROWS];     // the cell is open (kind 0)
	unsigned char mFireable[MAX_COLS][MAX_ROWS]; // open and not yet reached by a flood (Shooter_Fire's test)
};

struct Plan
{
	Level* mLevel = nullptr;
	bool mValid = false;
	int mCol = 0;
	int mRow = 0;
	bool mAcross = false;
	int mGain = 0;
	int mFailures = 0;  // calls in a row that found no safe line on this board
	bool mRisky = false; // the plan was taken without the safety test (after 30 failures)
};
Plan gPlan;

void ReadGrid(Level* theLevel, Grid& theGrid)
{
	theGrid.mCols = std::min(theLevel->mCols, MAX_COLS);
	theGrid.mRows = std::min(theLevel->mRows, MAX_ROWS);
	for (int c = 0; c < theGrid.mCols; c++)
	{
		for (int r = 0; r < theGrid.mRows; r++)
		{
			Cell* aCell = &theLevel->mColumns[c][r];
			theGrid.mOpen[c][r] = aCell->mKind == CELL_OPEN;
			theGrid.mFireable[c][r] = aCell->mKind == CELL_OPEN && aCell->mUnmarked != 0;
		}
	}
}

// A candidate line through (theCol, theRow): its ends, the cells it fills and the time its beams take
struct Line
{
	int mFrom;       // first and last open cell along the line (columns when across, rows when not)
	int mTo;
	int mGain;       // cells that fill, -1 when the line cuts nothing off
	int mTicks;      // updates until both beams have stopped
};

void Flood(const Grid& theGrid, unsigned char (&theMark)[MAX_COLS][MAX_ROWS], int theCol, int theRow, int theLabel,
	int& theCells)
{
	static std::vector<std::pair<int, int>> sAStack;
	sAStack.clear();
	if (!theGrid.mOpen[theCol][theRow] || theMark[theCol][theRow] != 0)
		return;
	theMark[theCol][theRow] = (unsigned char)theLabel;
	sAStack.push_back({theCol, theRow});
	while (!sAStack.empty())
	{
		std::pair<int, int> aCell = sAStack.back();
		sAStack.pop_back();
		theCells++;
		static const int STEP[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
		for (const auto& d : STEP)
		{
			int c = aCell.first + d[0];
			int r = aCell.second + d[1];
			if (c < 0 || r < 0 || c >= theGrid.mCols || r >= theGrid.mRows || !theGrid.mOpen[c][r] || theMark[c][r] != 0)
				continue;
			theMark[c][r] = (unsigned char)theLabel;
			sAStack.push_back({c, r});
		}
	}
}

Line Evaluate(const Grid& theGrid, int theCol, int theRow, bool theAcross, int theNumber, float theSpeed,
	const std::vector<std::pair<int, int>>& theEnemyCells)
{
	Line aLine;
	aLine.mGain = -1;
	// the open run through the cell
	int aFrom = theAcross ? theCol : theRow;
	int aTo = aFrom;
	int aLimit = theAcross ? theGrid.mCols : theGrid.mRows;
	while (aFrom - 1 >= 0 && (theAcross ? theGrid.mOpen[aFrom - 1][theRow] : theGrid.mOpen[theCol][aFrom - 1]))
		aFrom--;
	while (aTo + 1 < aLimit && (theAcross ? theGrid.mOpen[aTo + 1][theRow] : theGrid.mOpen[theCol][aTo + 1]))
		aTo++;
	aLine.mFrom = aFrom;
	aLine.mTo = aTo;
	int aMid = theAcross ? theCol : theRow;
	float aReach = (float)std::max(aMid - aFrom, aTo - aMid) * 16.0f + 8.0f;
	aLine.mTicks = theSpeed > 0.0f ? (int)(aReach / theSpeed) : 1000000;

	// the line's cells are blocked (2); the cells touching it on one side are flooded as 1, on the other as 4
	static unsigned char sAMark[MAX_COLS][MAX_ROWS];
	std::memset(sAMark, 0, sizeof(sAMark));
	for (int i = aFrom; i <= aTo; i++)
	{
		if (theAcross)
			sAMark[i][theRow] = 2;
		else
			sAMark[theCol][i] = 2;
	}
	int aCells[2] = {0, 0};
	for (int aSide = 0; aSide < 2; aSide++)
	{
		int aOffset = aSide == 0 ? -1 : 1;
		for (int i = aFrom; i <= aTo; i++)
		{
			int c = theAcross ? i : theCol + aOffset;
			int r = theAcross ? theRow + aOffset : i;
			if (c < 0 || r < 0 || c >= theGrid.mCols || r >= theGrid.mRows || !theGrid.mOpen[c][r])
				continue;
			// side two meeting side one's region: the line cuts nothing off
			if (aSide == 1 && sAMark[c][r] == 1)
				return aLine;
			Flood(theGrid, sAMark, c, r, aSide == 0 ? 1 : 4, aCells[aSide]);
		}
	}
	if (aCells[0] == 0 || aCells[1] == 0)
		return aLine;
	int anEnemies[2] = {0, 0};
	for (const auto& e : theEnemyCells)
	{
		if (e.first < 0 || e.second < 0 || e.first >= theGrid.mCols || e.second >= theGrid.mRows)
			continue;
		if (sAMark[e.first][e.second] == 1)
			anEnemies[0]++;
		else if (sAMark[e.first][e.second] == 4)
			anEnemies[1]++;
	}
	if (theNumber > 2 && anEnemies[0] == 0 && anEnemies[1] == 0)
		aLine.mGain = aCells[0] + aCells[1];
	else
		aLine.mGain = std::min(aCells[0], aCells[1]);
	return aLine;
}

bool Rolls(const Enemy* theEnemy)
{
	switch (theEnemy->mKind)
	{
	case ENEMY_ROLLER: case ENEMY_BURST_ROLLER: case ENEMY_SPIN_ROLLER: case ENEMY_TURN_ROLLER:
	case ENEMY_BUMP_ROLLER: case ENEMY_TELEPORTER_ROLLER: case ENEMY_JELLYFISH_ROLLER:
	case ENEMY_SNAKE: case ENEMY_CANNON_BALL: case ENEMY_LASER_SHOT:
		return true;
	}
	return false;
}

// Whether an enemy is expected to touch the beams of a line fired from (theCol, theRow) before they stop. Rollers and
// shots go on in a straight line (turning back at cells that are not open); the walkers keep to the walls and may
// come anywhere along the line's ends, so they only have to stay clear of it.
bool Safe(Level* theLevel, const Grid& theGrid, int theCol, int theRow, bool theAcross, const Line& theLine, float theSpeed)
{
	float aSx = (float)(theCol * 16 + 8);
	float aSy = (float)(theRow * 16 + 8);
	// the two ends of the line (the beams stop at the walls)
	float aLo = (float)(theLine.mFrom * 16);
	float aHi = (float)(theLine.mTo * 16 + 15);
	bool aSafe = true;
	List_Each(&theLevel->mEnemies, [&](Enemy* theEnemy) {
		if (!aSafe || theEnemy->mImage == nullptr || theEnemy->mIce != nullptr)
			return;
		float aRadius = std::min((float)theEnemy->mImage->mW * 0.5f, 22.0f);
		float aLx = theAcross ? aLo : aSx;
		float aLy = theAcross ? aSy : aLo;
		float aHx = theAcross ? aHi : aSx;
		float aHy = theAcross ? aSy : aHi;
		if (!Rolls(theEnemy))
		{
			if (SegmentDist(theEnemy->mX, theEnemy->mY, aLx, aLy, aHx, aHy) < aRadius + 40.0f)
				aSafe = false;
			return;
		}
		float aX = theEnemy->mX;
		float aY = theEnemy->mY;
		float aVx = theEnemy->mVx;
		float aVy = theEnemy->mVy;
		const int STEP = 25;
		for (int t = 0; t <= theLine.mTicks + STEP; t += STEP)
		{
			float aReach = theSpeed * (float)t + 8.0f;
			float aBx0 = theAcross ? std::max(aLo, aSx - aReach) : aSx;
			float aBx1 = theAcross ? std::min(aHi, aSx + aReach) : aSx;
			float aBy0 = theAcross ? aSy : std::max(aLo, aSy - aReach);
			float aBy1 = theAcross ? aSy : std::min(aHi, aSy + aReach);
			if (SegmentDist(aX, aY, aBx0, aBy0, aBx1, aBy1) < aRadius + 6.0f)
			{
				aSafe = false;
				return;
			}
			float aNx = aX + aVx * (float)STEP;
			float aNy = aY + aVy * (float)STEP;
			int c = (int)aNx / 16;
			int r = (int)aY / 16;
			if (aNx < 0.0f || c >= theGrid.mCols || r >= theGrid.mRows || !theGrid.mOpen[c][r])
			{
				aVx = -aVx;
				aNx = aX;
			}
			c = (int)aNx / 16;
			r = (int)aNy / 16;
			if (aNy < 0.0f || c >= theGrid.mCols || r >= theGrid.mRows || !theGrid.mOpen[c][r])
			{
				aVy = -aVy;
				aNy = aY;
			}
			aX = aNx;
			aY = aNy;
		}
	});
	return aSafe;
}

// theRisky takes the line that fills the most whatever the enemies may do, after a board has offered no safe line for a
// while (a player does not wait for ever either)
bool ChoosePlan(Level* theLevel, Shooter* theShip, bool theRisky)
{
	if (gPlan.mLevel != theLevel)
		gPlan.mFailures = 0;
	Grid aGrid;
	ReadGrid(theLevel, aGrid);
	std::vector<std::pair<int, int>> anEnemyCells;
	List_Each(&theLevel->mEnemies, [&](Enemy* theEnemy) { anEnemyCells.push_back({(int)theEnemy->mX / 16, (int)theEnemy->mY / 16}); });
	float aSpeed = theShip->mBeamSpeed > 0.0f ? theShip->mBeamSpeed : 0.14f;
	int aPanelRows = theLevel->mRows;
	if (theLevel->mPanels.mHead != nullptr)
		aPanelRows = std::min(aPanelRows, (int)(theLevel->mPanels.mHead->mY - 8.0f) / 16);
	struct Candidate
	{
		int mCol, mRow;
		bool mAcross;
		Line mLine;
	};
	std::vector<Candidate> aList;
	for (int c = 1; c < aGrid.mCols - 1; c++)
	{
		for (int r = 1; r < aGrid.mRows - 1 && r < aPanelRows; r++)
		{
			if (!aGrid.mFireable[c][r])
				continue;
			for (int aAcross = 0; aAcross < 2; aAcross++)
			{
				Line aLine = Evaluate(aGrid, c, r, aAcross != 0, theLevel->mNumber, aSpeed, anEnemyCells);
				if (aLine.mGain > 0)
					aList.push_back({c, r, aAcross != 0, aLine});
			}
		}
	}
	// most cells first; of equal gain, the quicker line
	std::sort(aList.begin(), aList.end(), [](const Candidate& a, const Candidate& b) {
		if (a.mLine.mGain != b.mLine.mGain)
			return a.mLine.mGain > b.mLine.mGain;
		return a.mLine.mTicks < b.mLine.mTicks;
	});
	gPlan.mLevel = theLevel;
	gPlan.mValid = false;
	if (!theRisky)
		gPlan.mFailures = gPlan.mFailures + 1;
	int aTried = 0;
	for (const Candidate& aCandidate : aList)
	{
		if (aTried++ > 4000)
			break;
		if (theRisky || Safe(theLevel, aGrid, aCandidate.mCol, aCandidate.mRow, aCandidate.mAcross, aCandidate.mLine, aSpeed))
		{
			gPlan.mValid = true;
			gPlan.mRisky = theRisky;
			if (!theRisky)
				gPlan.mFailures = 0;
			gPlan.mCol = aCandidate.mCol;
			gPlan.mRow = aCandidate.mRow;
			gPlan.mAcross = aCandidate.mAcross;
			gPlan.mGain = aCandidate.mLine.mGain;
			return true;
		}
	}
	return false;
}

std::string Move()
{
	Level* aLevel = gLevels.mCurrent;
	Shooter* aShip = Ship();
	// a won board shows its bonus texts, each clicked away in turn; the click after the last one goes on to the next board
	if (aLevel != nullptr && aLevel->mKind == LEVEL_BOARD && aLevel->mState == LEVEL_WON && aLevel->mGameOver == 0 && aLevel->mTexts.mHead != nullptr)
		return "MOVE {\"kind\":\"continue\",\"input\":[{\"click\":[400,300]},{\"wait\":20}]}";
	// the board waits while the pointer is over its panel (Level_IsPlaying): a click that started the board, on the
	// instructions page's Play button, leaves it there, and a player moves it up into the board
	if (aLevel != nullptr && aLevel->mKind == LEVEL_BOARD && aLevel->mState == LEVEL_PLAYING && aLevel->mGameOver == 0 && aLevel->mTotalCells != 0 &&
		Level_IsPlaying(aLevel) == 0 && (aLevel->mMenus.mCurrent == nullptr || aLevel->mMenus.mCurrent->mRequesters.mCurrent == nullptr))
	{
		int aY = 300;
		if (aLevel->mPanels.mCurrent != nullptr && (float)aY >= aLevel->mPanels.mCurrent->mY)
			aY = (int)aLevel->mPanels.mCurrent->mY - 40;
		return Fmt("MOVE {\"kind\":\"hover\",\"input\":[{\"move\":[400,%d]},{\"wait\":4}]}", aY);
	}
	if (aLevel == nullptr || aShip == nullptr || Busy(aLevel, aShip) || Fill(aLevel) >= aLevel->mFillQuota)
		return "MOVE null";
	int aSx = (int)aShip->mX;
	int aSy = (int)aShip->mY;
	// a power-up weapon orbiting the ship: a click launches it, straight out the way it is from the ship
	if (WeaponsOrbiting() != 0)
	{
		bool aTooClose = false;
		List_Each(&aLevel->mWeapons, [&](Weapon* theWeapon) {
			if (theWeapon->mState == WEAPON_ORBITING && theWeapon->mTooClose != 0)
				aTooClose = true;
		});
		if (aTooClose)
			return "MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}";
		return Fmt("MOVE {\"kind\":\"launch\",\"input\":[{\"click\":[%d,%d]},{\"wait\":4}]}", aSx, aSy);
	}
	bool aPlanOk = gPlan.mValid && gPlan.mLevel == aLevel;
	if (aPlanOk)
	{
		Cell* aCell = &aLevel->mColumns[gPlan.mCol][gPlan.mRow];
		aPlanOk = aCell->mKind == CELL_OPEN && aCell->mUnmarked != 0;
	}
	if (!aPlanOk && !ChoosePlan(aLevel, aShip, gPlan.mLevel == aLevel && gPlan.mFailures >= 30))
		return "MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}";
	int aX = gPlan.mCol * 16 + 8;
	int aY = gPlan.mRow * 16 + 8;
	int aCol = aSx / 16;
	int aRow = aSy / 16;
	if (aCol != gPlan.mCol || aRow != gPlan.mRow)
		return Fmt("MOVE {\"kind\":\"aim\",\"col\":%d,\"row\":%d,\"across\":%s,\"input\":[{\"move\":[%d,%d]},{\"wait\":8}]}",
			gPlan.mCol, gPlan.mRow, gPlan.mAcross ? "true" : "false", aX, aY);
	if ((aShip->mBeamAcross != 0) != gPlan.mAcross)
		return Fmt("MOVE {\"kind\":\"turn\",\"input\":[{\"click\":[%d,%d],\"button\":\"right\"},{\"wait\":3}]}", aX, aY);
	// the ship is in its cell, turned the right way: fire, unless an enemy sits on it (the game only buzzes)
	if (EnemiesOnShip(aShip) != 0)
		return "MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}";
	Grid aGrid;
	ReadGrid(aLevel, aGrid);
	std::vector<std::pair<int, int>> anEnemyCells;
	List_Each(&aLevel->mEnemies, [&](Enemy* theEnemy) { anEnemyCells.push_back({(int)theEnemy->mX / 16, (int)theEnemy->mY / 16}); });
	float aSpeed = aShip->mBeamSpeed > 0.0f ? aShip->mBeamSpeed : 0.14f;
	Line aLine = Evaluate(aGrid, gPlan.mCol, gPlan.mRow, gPlan.mAcross, aLevel->mNumber, aSpeed, anEnemyCells);
	if (aLine.mGain <= 0 || (!gPlan.mRisky && !Safe(aLevel, aGrid, gPlan.mCol, gPlan.mRow, gPlan.mAcross, aLine, aSpeed)))
	{
		// the board moved on: look again
		gPlan.mValid = false;
		return "MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}";
	}
	gPlan.mValid = false;
	gPlan.mFailures = 0;
	return Fmt("MOVE {\"kind\":\"shot\",\"col\":%d,\"row\":%d,\"across\":%s,\"gain\":%d,\"input\":[{\"click\":[%d,%d]},{\"wait\":3}]}",
		aX / 16, aY / 16, gPlan.mAcross ? "true" : "false", aLine.mGain, aX, aY);
}

// ----- the other commands -----

LevelSet* SetHolding(int theLevelIndex)
{
	for (LevelSet* aSet = gLevelSets.mHead; aSet != nullptr; aSet = Node_Next(aSet))
		if (aSet->mStart <= theLevelIndex && theLevelIndex <= aSet->mEnd)
			return aSet;
	return nullptr;
}

bool HasScreen(int theKind)
{
	for (Level* aLevel = gLevels.mHead; aLevel != nullptr; aLevel = Node_Next(aLevel))
		if (aLevel->mKind == theKind)
			return true;
	return false;
}

// Starts board theNumber (1 up) as the level selection's Play does (CBMenu.cpp, kind 1): the board's index and level
// set chosen, then the screen left for gGoTo 0, which reads the board file and drops the boards before the chosen one
// (GameStep). From a board the title comes first, as Esc takes it there, since the board file is then loaded again
// from the menus. A new game's lives and score are set when the current screen is not a board.
std::string StartBoard(int theNumber, bool theNewGame)
{
	if (gLevels.mCurrent == nullptr)
		return "no screen is current";
	bool aFromBoard = gLevels.mCurrent->mKind == LEVEL_BOARD;
	if (theNewGame || !aFromBoard || gLives <= 0)
	{
		gLives = 6;
		gScore = 0;
	}
	int aLives = gLives;
	int aScore = gScore;
	if (aFromBoard)
	{
		gGoTo = 1;
		if (!ChangeScreen() || gLevels.mCurrent->mKind == LEVEL_BOARD)
			return "the title did not come";
	}
	gLives = aLives;
	gScore = aScore;
	// (the level sets are those of the file read for the title: from a board, only after the title came)
	LevelSet* aSet = SetHolding(theNumber - 1);
	if (aSet == nullptr)
		return "no such level";
	gLevelInSet = theNumber - 1;
	gLevelSets.mCurrent = aSet;
	gGoTo = 0;
	ChangeScreen();
	return "";
}

bool SetDifficulty(const std::string& theValue)
{
	if (theValue == "easy" || theValue == "-1")
		gDifficulty = -1;
	else if (theValue == "normal" || theValue == "0")
		gDifficulty = 0;
	else if (theValue == "hard" || theValue == "1")
		gDifficulty = 1;
	else
		return false;
	SaveProfiles();
	return true;
}

bool SetShooter(const std::string& theValue)
{
	if (theValue == "manual" || theValue == "0")
		gAutoFire = 0;
	else if (theValue == "auto" || theValue == "automatic" || theValue == "1")
		gAutoFire = 1;
	else
		return false;
	gProfiles[gProfile].mAutoFire = gAutoFire;
	SaveProfiles();
	return true;
}

void Ok(std::vector<std::string>& theReplies, const std::string& theCommand, const std::string& theArgs)
{
	theReplies.push_back("OK " + theCommand + (theArgs.empty() ? "" : " " + theArgs));
}

void Error(std::vector<std::string>& theReplies, const std::string& theCommand, const std::string& theArgs,
	const std::string& theReason)
{
	theReplies.push_back("ERROR " + theCommand + (theArgs.empty() ? "" : " " + theArgs) + ": " + theReason);
}

} // namespace

bool TestCommand(const std::string& theCommand, const std::string& theArgs, std::vector<std::string>& theReplies)
{
	Level* aLevel = gLevels.mCurrent;
	if (theCommand == "state")
	{
		theReplies.push_back("STATE " + StateJson());
		return true;
	}
	if (theCommand == "move")
	{
		theReplies.push_back(Move());
		return true;
	}
	if (theCommand != "goto" && theCommand != "level" && theCommand != "unlock" && theCommand != "set" &&
		theCommand != "win" && theCommand != "lose" && theCommand != "seed")
		return false;
	if (theCommand == "seed")
	{
		if (theArgs.empty())
		{
			Error(theReplies, theCommand, theArgs, "seed needs a number");
			return true;
		}
		SeedRand(std::atoi(theArgs.c_str()));
		Ok(theReplies, theCommand, theArgs);
		return true;
	}
	if (aLevel == nullptr)
	{
		Error(theReplies, theCommand, theArgs, "no screen is current yet");
		return true;
	}
	if (theCommand == "unlock")
	{
		if (theArgs != "all")
		{
			Error(theReplies, theCommand, theArgs, "unlock needs \"all\"");
			return true;
		}
		LevelSet* aLast = gLevelSets.mTail;
		gProfiles[gProfile].mLevelReached = aLast != nullptr ? aLast->mEnd : 577;
		SaveProfiles();
		Ok(theReplies, theCommand, theArgs);
		return true;
	}
	if (theCommand == "set")
	{
		char aName[64] = "";
		char aValue[64] = "";
		std::sscanf(theArgs.c_str(), "%63s %63s", aName, aValue);
		std::string aN = aName, aV = aValue;
		if (aN == "score" && !aV.empty())
			gScore = std::atoi(aV.c_str());
		else if (aN == "lives" && !aV.empty())
			gLives = std::atoi(aV.c_str());
		else if (aN == "reached" && !aV.empty())
		{
			gProfiles[gProfile].mLevelReached = std::atoi(aV.c_str());
			SaveProfiles();
		}
		else if ((aN == "difficulty" || aN == "mode") && SetDifficulty(aV))
			;
		else if (aN == "shooter" && SetShooter(aV))
			;
		else
		{
			Error(theReplies, theCommand, theArgs, "set takes score, lives, reached, difficulty (easy|normal|hard) or shooter (manual|auto) and a value");
			return true;
		}
		Ok(theReplies, theCommand, theArgs);
		return true;
	}
	if (theCommand == "win" || theCommand == "lose")
	{
		Shooter* aShip = Ship();
		if (aLevel->mKind != LEVEL_BOARD || aShip == nullptr || aLevel->mState != LEVEL_PLAYING || aLevel->mTotalCells == 0 || aLevel->mGameOver != 0)
		{
			Error(theReplies, theCommand, theArgs, "no board is being played");
			return true;
		}
		if (theCommand == "win")
		{
			// the board's own ending runs once the fill has reached the quota and nothing is filling
			int aNeed = (aLevel->mFillQuota * aLevel->mTotalCells + 99) / 100;
			if (aLevel->mFilledCells < aNeed)
				aLevel->mFilledCells = aNeed;
		}
		else
		{
			gLives = 0;
			Level_GameOver(aLevel);
		}
		Ok(theReplies, theCommand, theArgs);
		return true;
	}
	if (theCommand == "level")
	{
		int aNumber = std::atoi(theArgs.c_str());
		if (aNumber < 1)
		{
			Error(theReplies, theCommand, theArgs, "level needs a board number from 1");
			return true;
		}
		std::string aReason = StartBoard(aNumber, false);
		if (!aReason.empty())
			Error(theReplies, theCommand, theArgs, aReason);
		else
			Ok(theReplies, theCommand, theArgs);
		return true;
	}
	// goto <screen>: a screen, or a mode's name to start a game in it
	std::string aWhere = theArgs;
	if (aWhere == "game" || aWhere == "easy" || aWhere == "normal" || aWhere == "hard" || aWhere == "manual" || aWhere == "auto")
	{
		if (aWhere == "easy" || aWhere == "normal" || aWhere == "hard")
			SetDifficulty(aWhere);
		else if (aWhere == "manual" || aWhere == "auto")
			SetShooter(aWhere);
		std::string aReason = StartBoard(1, true);
		if (!aReason.empty())
			Error(theReplies, theCommand, theArgs, aReason);
		else
			Ok(theReplies, theCommand, theArgs);
		return true;
	}
	int aKind = -1;
	if (aWhere == "title" || aWhere == "menu")
		aKind = LEVEL_MAIN_MENU;
	else if (aWhere == "options")
		aKind = LEVEL_OPTIONS;
	else if (aWhere == "help")
		aKind = LEVEL_HELP;
	else if (aWhere == "instructions" || aWhere == "help2")
		aKind = LEVEL_HELP2;
	else if (aWhere == "highscores")
		aKind = LEVEL_HIGH_SCORES;
	else if (aWhere == "credits")
		aKind = LEVEL_CREDITS;
	else if (aWhere == "levelselect")
		aKind = LEVEL_SELECT_LEVEL;
	else if (aWhere == "changeuser")
		aKind = LEVEL_CHANGE_USER;
	if (aKind < 0)
	{
		Error(theReplies, theCommand, theArgs, "no such screen (title, options, help, instructions, highscores, credits, levelselect, changeuser, game, easy, normal, hard, manual, auto)");
		return true;
	}
	// a board's list holds the board file's screens only: from a board only the title is reached, as Esc does
	if ((aLevel->mKind == LEVEL_BOARD && aKind != LEVEL_MAIN_MENU) || (aLevel->mKind != LEVEL_BOARD && !HasScreen(aKind)))
	{
		Error(theReplies, theCommand, theArgs, "that screen cannot be reached from here (from a board, go to the title first)");
		return true;
	}
	gGoTo = aKind;
	ChangeScreen();
	Ok(theReplies, theCommand, theArgs);
	return true;
}

} // namespace cb
