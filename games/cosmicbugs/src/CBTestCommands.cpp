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
	case 0: return "board";
	case 1: return "title";
	case 2: return "highscore_entry";
	case 3: return "highscores";
	case 4: return "credits";
	case 5: return "levelselect";
	case 6: return "levelselect2";
	case 7: return "options";
	case 0xb: return "mission_complete";
	case 0xc: return "help";
	case 0xd: return "instructions";
	case 0xe: return "logo";
	case 0x10: return "change_user";
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
	Level* aLevel = gLevels.current;
	return aLevel != nullptr && aLevel->kind == 0 ? aLevel->shooters.head : nullptr;
}

int WeaponsOrbiting()
{
	int aCount = 0;
	List_Each(&gLevels.current->weapons, [&](Weapon* aWeapon) {
		if (aWeapon->state == 3)
			aCount = aCount + 1;
	});
	return aCount;
}

// the enemies Shooter_Fire refuses to fire beside (CBShooter.cpp's Shooter_EnemiesNear)
int EnemiesOnShip(Shooter* theShip)
{
	int aCount = 0;
	List_Each(&gLevels.current->enemies, [&](Enemy* anEnemy) {
		if (CircleOverlap(theShip->x, theShip->y, (float)(theShip->image->mH / 3), anEnemy->x, anEnemy->y,
				(float)(anEnemy->image->mH / 3)) != 0.0)
			aCount = aCount + 1;
	});
	return aCount;
}

// Whether a dialog (a requester: the resume question, a name, a quit or delete confirmation) is up on any of the screen's
// menus (the resume question is on the last one, not the current one)
bool HasRequester(Level* theLevel)
{
	for (Menu* aMenu = theLevel->menus.head; aMenu != nullptr; aMenu = Node_Next(aMenu))
		if (aMenu->requesters.current != nullptr)
			return true;
	return false;
}

int Fill(Level* theLevel)
{
	return theLevel->totalCells != 0 ? (theLevel->filledCells * 100) / theLevel->totalCells : 0;
}

// Whether the board cannot take a move now: the ship is not under the player's control (coming in, firing, hit), the
// board is paused or won, or cells are still being filled
bool Busy(Level* theLevel, Shooter* theShip)
{
	return theShip == nullptr || theShip->state != 3 || theLevel->state != 1 || theLevel->gameOver != 0 ||
		theLevel->fills.current != nullptr || theLevel->totalCells == 0;
}

// ----- state -----

std::string StateJson()
{
	Level* aLevel = gLevels.current;
	int aKind = aLevel != nullptr ? aLevel->kind : -1;
	std::string s = Fmt("{\"screen\":\"%s\",\"kind\":%d,\"mode\":\"%s-%s\",\"difficulty\":\"%s\",\"shooter\":\"%s\","
		"\"score\":%d,\"lives\":%d", aLevel != nullptr ? ScreenName(aKind) : "none", aKind, DifficultyName(),
		ShooterName(), DifficultyName(), ShooterName(), gScore, gLives);
	bool aRequester = aLevel != nullptr && HasRequester(aLevel);
	s += Fmt(",\"requester\":%s,\"fullscreen\":%s,\"stretched\":%s,\"updates\":%d", aRequester ? "true" : "false",
		r64::Screen_Fullscreen() != 0 ? "true" : "false", r64::Screen_Centered() == 0 ? "true" : "false", gFrame);
	if (aLevel == nullptr || aKind != 0)
	{
		s += Fmt(",\"busy\":false,\"over\":%s}", aKind == 0xb ? "true" : "false");
		return s;
	}
	Shooter* aShip = Ship();
	bool aBusy = Busy(aLevel, aShip);
	s += Fmt(",\"level\":%d,\"fill\":%d,\"quota\":%d,\"shots\":%d,\"cells\":%d,\"filled\":%d", aLevel->number, Fill(aLevel),
		aLevel->fillQuota, aLevel->shots, aLevel->totalCells, aLevel->filledCells);
	s += Fmt(",\"playing\":%s,\"busy\":%s,\"paused\":%s,\"won\":%s,\"over\":%s", !aBusy ? "true" : "false",
		aBusy ? "true" : "false", aLevel->state == 2 ? "true" : "false", aLevel->state == 3 ? "true" : "false",
		aLevel->gameOver != 0 ? "true" : "false");
	if (aShip != nullptr)
		s += Fmt(",\"ship\":{\"x\":%d,\"y\":%d,\"state\":%d,\"across\":%s,\"weapons\":%d}", (int)aShip->x, (int)aShip->y,
			aShip->state, aShip->beamAcross != 0 ? "true" : "false", WeaponsOrbiting());
	s += ",\"enemies\":[";
	int aCount = 0;
	List_Each(&aLevel->enemies, [&](Enemy* anEnemy) {
		if (aCount < 30)
			s += Fmt("%s[%d,%d,%d,%d]", aCount > 0 ? "," : "", (int)anEnemy->x, (int)anEnemy->y, anEnemy->kind, anEnemy->state);
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

const int kMaxCols = 50;
const int kMaxRows = 38;

struct Grid
{
	int cols;
	int rows;
	unsigned char open[kMaxCols][kMaxRows];     // the cell is open (kind 0)
	unsigned char fireable[kMaxCols][kMaxRows]; // open and not yet reached by a flood (Shooter_Fire's test)
};

struct Plan
{
	Level* level = nullptr;
	bool valid = false;
	int col = 0;
	int row = 0;
	bool across = false;
	int gain = 0;
	int failures = 0;  // calls in a row that found no safe line on this board
	bool risky = false; // the plan was taken without the safety test (after 30 failures)
};
Plan gPlan;

void ReadGrid(Level* theLevel, Grid& theGrid)
{
	theGrid.cols = std::min(theLevel->cols, kMaxCols);
	theGrid.rows = std::min(theLevel->rows, kMaxRows);
	for (int c = 0; c < theGrid.cols; c++)
	{
		for (int r = 0; r < theGrid.rows; r++)
		{
			Cell* aCell = &theLevel->columns[c][r];
			theGrid.open[c][r] = aCell->kind == 0;
			theGrid.fireable[c][r] = aCell->kind == 0 && aCell->unmarked != 0;
		}
	}
}

// A candidate line through (theCol, theRow): its ends, the cells it fills and the time its beams take
struct Line
{
	int from;       // first and last open cell along the line (columns when across, rows when not)
	int to;
	int gain;       // cells that fill, -1 when the line cuts nothing off
	int ticks;      // updates until both beams have stopped
};

void Flood(const Grid& theGrid, unsigned char (&theMark)[kMaxCols][kMaxRows], int theCol, int theRow, int theLabel,
	int& theCells)
{
	static std::vector<std::pair<int, int>> aStack;
	aStack.clear();
	if (!theGrid.open[theCol][theRow] || theMark[theCol][theRow] != 0)
		return;
	theMark[theCol][theRow] = (unsigned char)theLabel;
	aStack.push_back({theCol, theRow});
	while (!aStack.empty())
	{
		std::pair<int, int> aCell = aStack.back();
		aStack.pop_back();
		theCells++;
		static const int kStep[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
		for (const auto& d : kStep)
		{
			int c = aCell.first + d[0];
			int r = aCell.second + d[1];
			if (c < 0 || r < 0 || c >= theGrid.cols || r >= theGrid.rows || !theGrid.open[c][r] || theMark[c][r] != 0)
				continue;
			theMark[c][r] = (unsigned char)theLabel;
			aStack.push_back({c, r});
		}
	}
}

Line Evaluate(const Grid& theGrid, int theCol, int theRow, bool theAcross, int theNumber, float theSpeed,
	const std::vector<std::pair<int, int>>& theEnemyCells)
{
	Line aLine;
	aLine.gain = -1;
	// the open run through the cell
	int aFrom = theAcross ? theCol : theRow;
	int aTo = aFrom;
	int aLimit = theAcross ? theGrid.cols : theGrid.rows;
	while (aFrom - 1 >= 0 && (theAcross ? theGrid.open[aFrom - 1][theRow] : theGrid.open[theCol][aFrom - 1]))
		aFrom--;
	while (aTo + 1 < aLimit && (theAcross ? theGrid.open[aTo + 1][theRow] : theGrid.open[theCol][aTo + 1]))
		aTo++;
	aLine.from = aFrom;
	aLine.to = aTo;
	int aMid = theAcross ? theCol : theRow;
	float aReach = (float)std::max(aMid - aFrom, aTo - aMid) * 16.0f + 8.0f;
	aLine.ticks = theSpeed > 0.0f ? (int)(aReach / theSpeed) : 1000000;

	// the line's cells are blocked (2); the cells touching it on one side are flooded as 1, on the other as 4
	static unsigned char aMark[kMaxCols][kMaxRows];
	std::memset(aMark, 0, sizeof(aMark));
	for (int i = aFrom; i <= aTo; i++)
	{
		if (theAcross)
			aMark[i][theRow] = 2;
		else
			aMark[theCol][i] = 2;
	}
	int aCells[2] = {0, 0};
	for (int aSide = 0; aSide < 2; aSide++)
	{
		int aOffset = aSide == 0 ? -1 : 1;
		for (int i = aFrom; i <= aTo; i++)
		{
			int c = theAcross ? i : theCol + aOffset;
			int r = theAcross ? theRow + aOffset : i;
			if (c < 0 || r < 0 || c >= theGrid.cols || r >= theGrid.rows || !theGrid.open[c][r])
				continue;
			// side two meeting side one's region: the line cuts nothing off
			if (aSide == 1 && aMark[c][r] == 1)
				return aLine;
			Flood(theGrid, aMark, c, r, aSide == 0 ? 1 : 4, aCells[aSide]);
		}
	}
	if (aCells[0] == 0 || aCells[1] == 0)
		return aLine;
	int anEnemies[2] = {0, 0};
	for (const auto& e : theEnemyCells)
	{
		if (e.first < 0 || e.second < 0 || e.first >= theGrid.cols || e.second >= theGrid.rows)
			continue;
		if (aMark[e.first][e.second] == 1)
			anEnemies[0]++;
		else if (aMark[e.first][e.second] == 4)
			anEnemies[1]++;
	}
	if (theNumber > 2 && anEnemies[0] == 0 && anEnemies[1] == 0)
		aLine.gain = aCells[0] + aCells[1];
	else
		aLine.gain = std::min(aCells[0], aCells[1]);
	return aLine;
}

bool Rolls(const Enemy* theEnemy)
{
	switch (theEnemy->kind)
	{
	case 0: case 1: case 2: case 3: case 4: case 0x10: case 0x11:
	case 6: case 0xc: case 0xd:
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
	float aLo = (float)(theLine.from * 16);
	float aHi = (float)(theLine.to * 16 + 15);
	bool aSafe = true;
	List_Each(&theLevel->enemies, [&](Enemy* anEnemy) {
		if (!aSafe || anEnemy->image == nullptr || anEnemy->ice != nullptr)
			return;
		float aRadius = std::min((float)anEnemy->image->mW * 0.5f, 22.0f);
		float aLx = theAcross ? aLo : aSx;
		float aLy = theAcross ? aSy : aLo;
		float aHx = theAcross ? aHi : aSx;
		float aHy = theAcross ? aSy : aHi;
		if (!Rolls(anEnemy))
		{
			if (SegmentDist(anEnemy->x, anEnemy->y, aLx, aLy, aHx, aHy) < aRadius + 40.0f)
				aSafe = false;
			return;
		}
		float aX = anEnemy->x;
		float aY = anEnemy->y;
		float aVx = anEnemy->vx;
		float aVy = anEnemy->vy;
		const int kStep = 25;
		for (int t = 0; t <= theLine.ticks + kStep; t += kStep)
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
			float aNx = aX + aVx * (float)kStep;
			float aNy = aY + aVy * (float)kStep;
			int c = (int)aNx / 16;
			int r = (int)aY / 16;
			if (aNx < 0.0f || c >= theGrid.cols || r >= theGrid.rows || !theGrid.open[c][r])
			{
				aVx = -aVx;
				aNx = aX;
			}
			c = (int)aNx / 16;
			r = (int)aNy / 16;
			if (aNy < 0.0f || c >= theGrid.cols || r >= theGrid.rows || !theGrid.open[c][r])
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
	if (gPlan.level != theLevel)
		gPlan.failures = 0;
	Grid aGrid;
	ReadGrid(theLevel, aGrid);
	std::vector<std::pair<int, int>> anEnemyCells;
	List_Each(&theLevel->enemies, [&](Enemy* anEnemy) { anEnemyCells.push_back({(int)anEnemy->x / 16, (int)anEnemy->y / 16}); });
	float aSpeed = theShip->beamSpeed > 0.0f ? theShip->beamSpeed : 0.14f;
	int aPanelRows = theLevel->rows;
	if (theLevel->panels.head != nullptr)
		aPanelRows = std::min(aPanelRows, (int)(theLevel->panels.head->y - 8.0f) / 16);
	struct Candidate
	{
		int col, row;
		bool across;
		Line line;
	};
	std::vector<Candidate> aList;
	for (int c = 1; c < aGrid.cols - 1; c++)
	{
		for (int r = 1; r < aGrid.rows - 1 && r < aPanelRows; r++)
		{
			if (!aGrid.fireable[c][r])
				continue;
			for (int aAcross = 0; aAcross < 2; aAcross++)
			{
				Line aLine = Evaluate(aGrid, c, r, aAcross != 0, theLevel->number, aSpeed, anEnemyCells);
				if (aLine.gain > 0)
					aList.push_back({c, r, aAcross != 0, aLine});
			}
		}
	}
	// most cells first; of equal gain, the quicker line
	std::sort(aList.begin(), aList.end(), [](const Candidate& a, const Candidate& b) {
		if (a.line.gain != b.line.gain)
			return a.line.gain > b.line.gain;
		return a.line.ticks < b.line.ticks;
	});
	gPlan.level = theLevel;
	gPlan.valid = false;
	if (!theRisky)
		gPlan.failures = gPlan.failures + 1;
	int aTried = 0;
	for (const Candidate& aCandidate : aList)
	{
		if (aTried++ > 4000)
			break;
		if (theRisky || Safe(theLevel, aGrid, aCandidate.col, aCandidate.row, aCandidate.across, aCandidate.line, aSpeed))
		{
			gPlan.valid = true;
			gPlan.risky = theRisky;
			if (!theRisky)
				gPlan.failures = 0;
			gPlan.col = aCandidate.col;
			gPlan.row = aCandidate.row;
			gPlan.across = aCandidate.across;
			gPlan.gain = aCandidate.line.gain;
			return true;
		}
	}
	return false;
}

std::string Move()
{
	Level* aLevel = gLevels.current;
	Shooter* aShip = Ship();
	// a won board shows its bonus texts, each clicked away in turn; the click after the last one goes on to the next board
	if (aLevel != nullptr && aLevel->kind == 0 && aLevel->state == 3 && aLevel->gameOver == 0 && aLevel->texts.head != nullptr)
		return "MOVE {\"kind\":\"continue\",\"input\":[{\"click\":[400,300]},{\"wait\":20}]}";
	// the board waits while the pointer is over its panel (Level_IsPlaying): a click that started the board, on the
	// instructions page's Play button, leaves it there, and a player moves it up into the board
	if (aLevel != nullptr && aLevel->kind == 0 && aLevel->state == 1 && aLevel->gameOver == 0 && aLevel->totalCells != 0 &&
		Level_IsPlaying(aLevel) == 0 && (aLevel->menus.current == nullptr || aLevel->menus.current->requesters.current == nullptr))
	{
		int aY = 300;
		if (aLevel->panels.current != nullptr && (float)aY >= aLevel->panels.current->y)
			aY = (int)aLevel->panels.current->y - 40;
		return Fmt("MOVE {\"kind\":\"hover\",\"input\":[{\"move\":[400,%d]},{\"wait\":4}]}", aY);
	}
	if (aLevel == nullptr || aShip == nullptr || Busy(aLevel, aShip) || Fill(aLevel) >= aLevel->fillQuota)
		return "MOVE null";
	int aSx = (int)aShip->x;
	int aSy = (int)aShip->y;
	// a power-up weapon orbiting the ship: a click launches it, straight out the way it is from the ship
	if (WeaponsOrbiting() != 0)
	{
		bool aTooClose = false;
		List_Each(&aLevel->weapons, [&](Weapon* aWeapon) {
			if (aWeapon->state == 3 && aWeapon->tooClose != 0)
				aTooClose = true;
		});
		if (aTooClose)
			return "MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}";
		return Fmt("MOVE {\"kind\":\"launch\",\"input\":[{\"click\":[%d,%d]},{\"wait\":4}]}", aSx, aSy);
	}
	bool aPlanOk = gPlan.valid && gPlan.level == aLevel;
	if (aPlanOk)
	{
		Cell* aCell = &aLevel->columns[gPlan.col][gPlan.row];
		aPlanOk = aCell->kind == 0 && aCell->unmarked != 0;
	}
	if (!aPlanOk && !ChoosePlan(aLevel, aShip, gPlan.level == aLevel && gPlan.failures >= 30))
		return "MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}";
	int aX = gPlan.col * 16 + 8;
	int aY = gPlan.row * 16 + 8;
	int aCol = aSx / 16;
	int aRow = aSy / 16;
	if (aCol != gPlan.col || aRow != gPlan.row)
		return Fmt("MOVE {\"kind\":\"aim\",\"col\":%d,\"row\":%d,\"across\":%s,\"input\":[{\"move\":[%d,%d]},{\"wait\":8}]}",
			gPlan.col, gPlan.row, gPlan.across ? "true" : "false", aX, aY);
	if ((aShip->beamAcross != 0) != gPlan.across)
		return Fmt("MOVE {\"kind\":\"turn\",\"input\":[{\"click\":[%d,%d],\"button\":\"right\"},{\"wait\":3}]}", aX, aY);
	// the ship is in its cell, turned the right way: fire, unless an enemy sits on it (the game only buzzes)
	if (EnemiesOnShip(aShip) != 0)
		return "MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}";
	Grid aGrid;
	ReadGrid(aLevel, aGrid);
	std::vector<std::pair<int, int>> anEnemyCells;
	List_Each(&aLevel->enemies, [&](Enemy* anEnemy) { anEnemyCells.push_back({(int)anEnemy->x / 16, (int)anEnemy->y / 16}); });
	float aSpeed = aShip->beamSpeed > 0.0f ? aShip->beamSpeed : 0.14f;
	Line aLine = Evaluate(aGrid, gPlan.col, gPlan.row, gPlan.across, aLevel->number, aSpeed, anEnemyCells);
	if (aLine.gain <= 0 || (!gPlan.risky && !Safe(aLevel, aGrid, gPlan.col, gPlan.row, gPlan.across, aLine, aSpeed)))
	{
		// the board moved on: look again
		gPlan.valid = false;
		return "MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}";
	}
	gPlan.valid = false;
	gPlan.failures = 0;
	return Fmt("MOVE {\"kind\":\"shot\",\"col\":%d,\"row\":%d,\"across\":%s,\"gain\":%d,\"input\":[{\"click\":[%d,%d]},{\"wait\":3}]}",
		aX / 16, aY / 16, gPlan.across ? "true" : "false", aLine.gain, aX, aY);
}

// ----- the other commands -----

LevelSet* SetHolding(int theLevelIndex)
{
	for (LevelSet* aSet = gLevelSets.head; aSet != nullptr; aSet = Node_Next(aSet))
		if (aSet->start <= theLevelIndex && theLevelIndex <= aSet->end)
			return aSet;
	return nullptr;
}

bool HasScreen(int theKind)
{
	for (Level* aLevel = gLevels.head; aLevel != nullptr; aLevel = Node_Next(aLevel))
		if (aLevel->kind == theKind)
			return true;
	return false;
}

// Starts board theNumber (1 up) as the level selection's Play does (CBMenu.cpp, kind 1): the board's index and level
// set chosen, then the screen left for gGoTo 0, which reads the board file and drops the boards before the chosen one
// (GameStep). From a board the title comes first, as Esc takes it there, since the board file is then loaded again
// from the menus. A new game's lives and score are set when the current screen is not a board.
std::string StartBoard(int theNumber, bool theNewGame)
{
	if (gLevels.current == nullptr)
		return "no screen is current";
	bool aFromBoard = gLevels.current->kind == 0;
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
		if (!ChangeScreen() || gLevels.current->kind == 0)
			return "the title did not come";
	}
	gLives = aLives;
	gScore = aScore;
	// (the level sets are those of the file read for the title: from a board, only after the title came)
	LevelSet* aSet = SetHolding(theNumber - 1);
	if (aSet == nullptr)
		return "no such level";
	gLevelInSet = theNumber - 1;
	gLevelSets.current = aSet;
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
	gProfiles[gProfile].autoFire = gAutoFire;
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
	Level* aLevel = gLevels.current;
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
		LevelSet* aLast = gLevelSets.tail;
		gProfiles[gProfile].levelReached = aLast != nullptr ? aLast->end : 577;
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
			gProfiles[gProfile].levelReached = std::atoi(aV.c_str());
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
		if (aLevel->kind != 0 || aShip == nullptr || aLevel->state != 1 || aLevel->totalCells == 0 || aLevel->gameOver != 0)
		{
			Error(theReplies, theCommand, theArgs, "no board is being played");
			return true;
		}
		if (theCommand == "win")
		{
			// the board's own ending runs once the fill has reached the quota and nothing is filling
			int aNeed = (aLevel->fillQuota * aLevel->totalCells + 99) / 100;
			if (aLevel->filledCells < aNeed)
				aLevel->filledCells = aNeed;
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
		aKind = 1;
	else if (aWhere == "options")
		aKind = 7;
	else if (aWhere == "help")
		aKind = 0xc;
	else if (aWhere == "instructions" || aWhere == "help2")
		aKind = 0xd;
	else if (aWhere == "highscores")
		aKind = 3;
	else if (aWhere == "credits")
		aKind = 4;
	else if (aWhere == "levelselect")
		aKind = 5;
	else if (aWhere == "changeuser")
		aKind = 0x10;
	if (aKind < 0)
	{
		Error(theReplies, theCommand, theArgs, "no such screen (title, options, help, instructions, highscores, credits, levelselect, changeuser, game, easy, normal, hard, manual, auto)");
		return true;
	}
	// a board's list holds the board file's screens only: from a board only the title is reached, as Esc does
	if ((aLevel->kind == 0 && aKind != 1) || (aLevel->kind != 0 && !HasScreen(aKind)))
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
