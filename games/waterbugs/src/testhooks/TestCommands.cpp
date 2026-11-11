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
	return theLevel != nullptr && theLevel->kind == 0 && theLevel->totalCells == 0;
}

const char* ScreenName(const Level* theLevel)
{
	if (theLevel == nullptr)
		return "loading";
	switch (theLevel->kind)
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
	MenuItem* item;
	Menu* menu;
	bool requester;
};

// the menu a requester is up on (the main menu's "resume your previous game?" is on its last menu, the others' on the current one)
Menu* RequesterMenu(Level* theLevel)
{
	if (theLevel->menus.current != nullptr && theLevel->menus.current->requesters.current != nullptr)
		return theLevel->menus.current;
	for (Menu* aMenu = theLevel->menus.head; aMenu != nullptr; aMenu = Node_Next(aMenu))
		if (aMenu->requesters.current != nullptr)
			return aMenu;
	return nullptr;
}

std::vector<ItemRef> ActiveItems(Level* theLevel)
{
	std::vector<ItemRef> aResult;
	Menu* aTop = RequesterMenu(theLevel);
	if (aTop != nullptr)
	{
		Menu* aRequester = aTop->requesters.current;
		for (MenuItem* anItem = aRequester->items.head; anItem != nullptr; anItem = Node_Next(anItem))
			aResult.push_back({anItem, aRequester, true});
		return aResult;
	}
	for (Menu* aMenu = theLevel->menus.head; aMenu != nullptr; aMenu = Node_Next(aMenu))
		for (MenuItem* anItem = aMenu->items.head; anItem != nullptr; anItem = Node_Next(anItem))
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
	theX = (int)theItem->x;
	theY = (int)theItem->y;
	if (theItem->anim == nullptr && theItem->font != nullptr)
		theY += Font_StringHeight(theItem->font, theItem->shown) / 2;
}

// ---------------------------------------------------------------------------------------------------------------------
// The board

struct Shot
{
	bool valid = false;
	int col = 0;
	int row = 0;
	int across = 0;
	int lo = 0;			// the open stretch through the cell, in cells along the beam
	int hi = 0;
	int fill = 0;		// the cells the game would fill
	int cellsA = 0;
	int cellsB = 0;
	double time = 0;	// updates the beams take to stop
	double risk = 0;	// 0 safe .. the sum of how early each bug could reach a growing beam
	double score = -1e9;
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

	explicit BoardBot(Level* theLevel) : mLevel(theLevel), mShip(theLevel->shooters.current)
	{
		mCols = std::min(theLevel->cols, 50);
		mRows = std::min(theLevel->rows, 37);
		std::memset(mEnemyAt, 0, sizeof(mEnemyAt));
		std::memset(mMark, 0, sizeof(mMark));
		for (int c = 0; c < mCols; c++)
			for (int r = 0; r < mRows; r++)
				mOpen[c][r] = theLevel->columns[c][r].kind == 0;
		for (Enemy* e = theLevel->enemies.head; e != nullptr; e = Node_Next(e))
		{
			int c = (int)e->x / 16;
			int r = (int)e->y / 16;
			if (0 <= c && c < mCols && 0 <= r && r < mRows)
				mEnemyAt[c][r]++;
		}
	}

	bool Open(int c, int r) const { return 0 <= c && c < mCols && 0 <= r && r < mRows && mOpen[c][r]; }

	// the open stretch through (col, row) along the beam
	void Stretch(int col, int row, int across, int& lo, int& hi) const
	{
		if (across != 0)
		{
			lo = hi = col;
			while (Open(lo - 1, row)) lo--;
			while (Open(hi + 1, row)) hi++;
		}
		else
		{
			lo = hi = row;
			while (Open(col, lo - 1)) lo--;
			while (Open(col, hi + 1)) hi++;
		}
	}

	// the open region about (sc, sr) outside the beam's cells: its cell count and enemies (the flood of Region_Count)
	void Flood(int sc, int sr, int col, int row, int across, int lo, int hi, int theGen, int& theCells, int& theEnemies)
	{
		theCells = 0;
		theEnemies = 0;
		if (!Open(sc, sr) || OnLine(sc, sr, col, row, across, lo, hi))
			return;
		static int aStack[50 * 37 * 2];
		int aTop = 0;
		mMark[sc][sr] = theGen;
		aStack[aTop++] = sc * 64 + sr;
		while (aTop > 0)
		{
			int v = aStack[--aTop];
			int c = v / 64, r = v % 64;
			theCells++;
			theEnemies += mEnemyAt[c][r];
			static const int aD[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
			for (int d = 0; d < 4; d++)
			{
				int nc = c + aD[d][0], nr = r + aD[d][1];
				if (Open(nc, nr) && mMark[nc][nr] != theGen && !OnLine(nc, nr, col, row, across, lo, hi))
				{
					mMark[nc][nr] = theGen;
					aStack[aTop++] = nc * 64 + nr;
				}
			}
		}
	}

	static bool OnLine(int c, int r, int col, int row, int across, int lo, int hi)
	{
		return across != 0 ? (r == row && lo <= c && c <= hi) : (c == col && lo <= r && r <= hi);
	}

	Shot Evaluate(int col, int row, int across)
	{
		Shot s;
		s.col = col;
		s.row = row;
		s.across = across;
		if (!Open(col, row) || mLevel->columns[col][row].unmarked == 0)
			return s;
		Stretch(col, row, across, s.lo, s.hi);
		int aGenA = ++mGen;
		int aSideAC = across != 0 ? col : col - 1, aSideAR = across != 0 ? row - 1 : row;
		int aSideBC = across != 0 ? col : col + 1, aSideBR = across != 0 ? row + 1 : row;
		int anEnemiesA = 0, anEnemiesB = 0;
		Flood(aSideAC, aSideAR, col, row, across, s.lo, s.hi, aGenA, s.cellsA, anEnemiesA);
		if (s.cellsA == 0 || s.cellsB < 0)
			return s;
		if (Open(aSideBC, aSideBR) && mMark[aSideBC][aSideBR] == aGenA)
			return s;		// the beam does not part the two sides
		int aGenB = ++mGen;
		Flood(aSideBC, aSideBR, col, row, across, s.lo, s.hi, aGenB, s.cellsB, anEnemiesB);
		if (s.cellsB == 0)
			return s;
		if (mLevel->number > 2 && anEnemiesA == 0 && anEnemiesB == 0)
			s.fill = s.cellsA + s.cellsB;
		else
			s.fill = std::min(s.cellsA, s.cellsB);
		s.valid = true;
		// the time the beams take, and the bugs that could reach a growing beam before they stop
		double aSpeed = mShip->beamSpeed > 0.01f ? mShip->beamSpeed : 0.14f;
		double aHalfLow = (col - 0) * 16.0 + 8.0, aHalfHigh = 0;
		double aShotX = col * 16.0 + 8.0, aShotY = row * 16.0 + 8.0;
		if (across != 0)
		{
			aHalfLow = aShotX - s.lo * 16.0;
			aHalfHigh = (s.hi + 1) * 16.0 - aShotX;
		}
		else
		{
			aHalfLow = aShotY - s.lo * 16.0;
			aHalfHigh = (s.hi + 1) * 16.0 - aShotY;
		}
		s.time = std::max(aHalfLow, aHalfHigh) / aSpeed;
		double aLeft = across != 0 ? s.lo * 16.0 : col * 16.0 + 4.0;
		double aRight = across != 0 ? (s.hi + 1) * 16.0 : col * 16.0 + 12.0;
		double aTop = across != 0 ? row * 16.0 + 4.0 : s.lo * 16.0;
		double aBottom = across != 0 ? row * 16.0 + 12.0 : (s.hi + 1) * 16.0;
		double aRisk = 0;
		for (Enemy* e = mLevel->enemies.head; e != nullptr; e = Node_Next(e))
		{
			double aRadius = e->image != nullptr ? e->image->h / 3.0 : 8.0;
			double px = std::min(std::max((double)e->x, aLeft), aRight);
			double py = std::min(std::max((double)e->y, aTop), aBottom);
			double aDist = std::sqrt(((double)e->x - px) * ((double)e->x - px) + ((double)e->y - py) * ((double)e->y - py)) - aRadius;
			double v = std::sqrt((double)e->vx * e->vx + (double)e->vy * e->vy) * 1.3 + 0.02;
			double t = std::max(0.0, aDist) / v;
			double aAlong = across != 0 ? px - aShotX : py - aShotY;
			if (t < s.time && std::fabs(aAlong) <= aSpeed * t + 12.0)
				aRisk += (s.time - t) / s.time;
			// a bug in the band of the beam, anywhere along it, is cut by the beam unless it leaves the band first
			double aPerp = across != 0 ? std::fabs((double)e->y - (row * 16.0 + 8.0)) : std::fabs((double)e->x - (col * 16.0 + 8.0));
			double aPos = across != 0 ? (double)e->x : (double)e->y;
			double aBandLow = (across != 0 ? s.lo : s.lo) * 16.0 - aRadius;
			double aBandHigh = (s.hi + 1) * 16.0 + aRadius;
			if (aPerp <= aRadius + 10.0 && aBandLow <= aPos && aPos <= aBandHigh)
				aRisk += 1.0;
		}
		s.risk = aRisk;
		double aFill = std::min<double>(s.fill, 400);
		s.score = (aRisk < 0.03) ? aFill : aFill * std::max(0.0, 1.0 - aRisk * 2.0) - 1000.0;
		return s;
	}

	// the best shot on the board: every second cell, both ways (or the way the ship turns itself)
	Shot Best(bool theAuto)
	{
		Shot aBest;
		for (int c = 1; c < mCols - 1; c += 2)
		{
			for (int r = 1; r < mRows - 1; r += 2)
			{
				if (!mOpen[c][r])
					continue;
				int aLoA, aHiA, aLoD, aHiD;
				Stretch(c, r, 1, aLoA, aHiA);
				Stretch(c, r, 0, aLoD, aHiD);
				int aSpanA = aHiA - aLoA, aSpanD = aHiD - aLoD;
				for (int across = 0; across < 2; across++)
				{
					if (gBan[across][across != 0 ? r : c] > 0)
						continue;
					if (theAuto && ((across != 0 && aSpanA > aSpanD) || (across == 0 && aSpanD > aSpanA)))
						continue;
					Shot s = Evaluate(c, r, across);
					if (s.valid && (!aBest.valid || s.score > aBest.score))
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
	for (Weapon* w = theLevel->weapons.head; w != nullptr; w = Node_Next(w))
		if (w->state == 3)
			aCount++;
	return aCount;
}

std::string ClickSteps(int theX, int theY, int theWait = 2)
{
	return Fmt("{\"press\":[%d,%d]},{\"wait\":%d},{\"release\":[%d,%d]}", theX, theY, theWait, theX, theY);
}

std::string MoveJson(const char* theKind, const std::string& theSteps, int theUpdates, const std::string& theExtra = "")
{
	return Fmt("MOVE {\"kind\":\"%s\",\"input\":[%s],\"updates\":%d", theKind, theSteps.c_str(), theUpdates) + theExtra + "}";
}

// the aim the bot keeps between its calls (the shot it moved the ship for), so that a fresh look at the board does not
// send the ship back and forth
struct Aim
{
	bool set = false;
	int col = 0;
	int row = 0;
	int across = 0;
	int update = 0;
};
Aim gAim;
int gUpdateCount = 0;	// the framework's update (10 ms each), as the last command saw it
int gLastBoardNumber = -1;
int gRealX = 0, gRealY = 0;	// the driver's pointer (window pixels), as the last command saw it
int gWrongWay = 0;		// the calls in a row that found the ship over its target cell turned the wrong way

// what the bot keeps between its calls, so that `state` can ask it without changing it (state's busy is the bot's)
struct BotMemory
{
	Aim aim;
	int lastBoardNumber, lastLives, lastShotAcross, lastShotLine, wrongWay;
	int ban[2][64];
};

BotMemory SaveBot()
{
	BotMemory m;
	m.aim = gAim;
	m.lastBoardNumber = gLastBoardNumber;
	m.lastLives = gLastLives;
	m.lastShotAcross = gLastShotAcross;
	m.lastShotLine = gLastShotLine;
	m.wrongWay = gWrongWay;
	std::memcpy(m.ban, gBan, sizeof(gBan));
	return m;
}

void RestoreBot(const BotMemory& m)
{
	gAim = m.aim;
	gLastBoardNumber = m.lastBoardNumber;
	gLastLives = m.lastLives;
	gLastShotAcross = m.lastShotAcross;
	gLastShotLine = m.lastShotLine;
	gWrongWay = m.wrongWay;
	std::memcpy(gBan, m.ban, sizeof(gBan));
}

std::string MenuMove(Level* theLevel, bool& theBusy);

// the ship's cell and its pixel position
std::string BoardMove(Level* theLevel, bool& theBusy)
{
	theBusy = false;
	if (theLevel->number != gLastBoardNumber)
	{
		gLastBoardNumber = theLevel->number;
		gAim.set = false;
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
	if (theLevel->gameOver != 0)
	{
		// the game over: a click leaves it
		return MoveJson("gameover", ClickSteps(400, 300), 5);
	}
	if (theLevel->state == 3)
	{
		// won: the fill and its texts, then a click shows the results, and the results' button leaves
		if (HasRequester(theLevel))
		{
			std::vector<ItemRef> anItems = ActiveItems(theLevel);
			for (const ItemRef& r : anItems)
			{
				int x, y;
				ItemPoint(r.item, x, y);
				return MoveJson("next", ClickSteps(x, y), 5);
			}
		}
		if (theLevel->texts.head != nullptr && theLevel->clicked == 0)
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
			ItemPoint(anItems.back().item, x, y);
			return MoveJson("dialog", ClickSteps(x, y), 5);
		}
		theBusy = true;
		return "";
	}
	Shooter* aShip = theLevel->shooters.current;
	if (aShip == nullptr || aShip->state != 3 || theLevel->state != 1)
	{
		theBusy = true;
		return "";
	}
	int aShipX = (int)aShip->x, aShipY = (int)aShip->y;
	if (OrbitingWeapons(theLevel) > 0)
		return MoveJson("launch", ClickSteps(aShipX, aShipY), 3);
	// the quota is passed: the level is won once the fill is done and no power-up is falling (the ship takes it up)
	if (theLevel->totalCells > 0 && theLevel->fillQuota <= (theLevel->filledCells * 100) / theLevel->totalCells)
	{
		Powerup* aPowerup = theLevel->powerups.head;
		if (aPowerup != nullptr && aPowerup->state == 4)
		{
			int aX = std::min(std::max((int)aPowerup->x, 0), 799), aY = std::min(std::max((int)aPowerup->y + 24, 0), 599);
			if (std::abs(aX - aShipX) > 6 || std::abs(aY - aShipY) > 6)
				return MoveJson("powerup", Fmt("{\"move\":[%d,%d]},{\"wait\":2}", aX, aY), 1);
		}
		theBusy = true;
		return "";
	}
	BoardBot aBot(theLevel);
	bool anAuto = aShip->noAutoTurn == 0;
	Shot aBest = aBot.Best(anAuto);
	if (!aBest.valid)
	{
		// nothing parts the board any more: fire where the ship is, the quota may be a few cells away
		theBusy = true;
		return "";
	}
	int aShipCol = aShipX / 16, aShipRow = aShipY / 16;
	Shot aHere = aBot.Evaluate(aShipCol, aShipRow, aShip->beamAcross);
	Shot aKept;
	if (gAim.set && gBan[gAim.across][gAim.across != 0 ? gAim.row : gAim.col] == 0)
		aKept = aBot.Evaluate(gAim.col, gAim.row, gAim.across);
	// the aim: the kept one while it is still worth a shot, else the best
	Shot aTarget = (aKept.valid && aKept.score >= 0.7 * aBest.score) ? aKept : aBest;
	bool aAtTarget = aShipCol == aTarget.col && aShipRow == aTarget.row && aShip->beamAcross == aTarget.across;
	bool aHereBanned = gBan[aShip->beamAcross != 0 ? 1 : 0][aShip->beamAcross != 0 ? aShipRow : aShipCol] > 0;
	if (aHere.valid && !aHereBanned && (aAtTarget || (aHere.score >= 0.7 * aBest.score && aHere.risk <= aBest.risk + 0.05)))
	{
		gAim.set = false;
		gLastShotAcross = aShip->beamAcross;
		gLastShotLine = aShip->beamAcross != 0 ? aShipRow : aShipCol;
		return MoveJson("shot", ClickSteps(aShipX, aShipY),
			1, Fmt(",\"col\":%d,\"row\":%d,\"across\":%d,\"fill\":%d,\"risk\":%.2f", aShipCol, aShipRow, aShip->beamAcross, aHere.fill, aHere.risk));
	}
	int aTargetX = aTarget.col * 16 + 8, aTargetY = aTarget.row * 16 + 8;
	if (aShipCol == aTarget.col && aShipRow == aTarget.row && aShip->beamAcross != aTarget.across)
	{
		// the ship is over the cell but turned the wrong way: a right click turns it, after the ship has had one look
		// at the span and turned itself (it keeps its way when the two spans are equal)
		if (!anAuto || gWrongWay++ > 0)
		{
			gWrongWay = 0;
			return MoveJson("turn", Fmt("{\"press\":[%d,%d],\"button\":\"right\"},{\"wait\":2},{\"release\":[%d,%d],\"button\":\"right\"}",
				aShipX, aShipY, aShipX, aShipY), 2);
		}
	}
	else
		gWrongWay = 0;
	gAim.set = true;
	gAim.col = aTarget.col;
	gAim.row = aTarget.row;
	gAim.across = aTarget.across;
	double aDist = std::sqrt((double)(aTargetX - aShipX) * (aTargetX - aShipX) + (double)(aTargetY - aShipY) * (aTargetY - aShipY));
	int aWait = 3 + (int)std::ceil(std::log2(std::max(1.0, aDist)));
	return MoveJson("aim", Fmt("{\"move\":[%d,%d]},{\"wait\":%d}", aTargetX, aTargetY, aWait), 1,
		Fmt(",\"col\":%d,\"row\":%d,\"across\":%d,\"fill\":%d,\"risk\":%.2f,\"here\":[%d,%d,%d,%d,%.1f]", aTarget.col, aTarget.row, aTarget.across, aTarget.fill, aTarget.risk,
			aShipCol, aShipRow, aShip->beamAcross, aHere.valid ? 1 : 0, aHere.score));
}

// ---------------------------------------------------------------------------------------------------------------------
// The state

bool gPlayed = false;		// a board has been up since the game started (the high scores are then the game's end)

bool IsOver(Level* theLevel)
{
	if (theLevel == nullptr || !gPlayed)
		return false;
	return theLevel->kind == 2 || theLevel->kind == 3 || theLevel->kind == 0xb;
}

bool IsBusy(Level* theLevel)
{
	if (theLevel == nullptr)
		return true;
	if (theLevel->kind == 0xe)
		return true;
	return false;
}

std::string StateJson()
{
	Level* aLevel = gLevels.current;
	if (aLevel != nullptr && aLevel->kind == 0 && !IsStory(aLevel))
		gPlayed = true;
	std::string s = Fmt("{\"screen\":\"%s\",\"mode\":\"campaign\",\"update\":%d,\"frame\":%d,\"score\":%d,\"lives\":%d,\"level\":%d,\"reached\":%d,"
		"\"difficulty\":%d,\"shootermode\":%d,\"rand\":%d,\"over\":%s",
		ScreenName(aLevel), gUpdateCount, gFrame, gScore, gLives, aLevel != nullptr && aLevel->kind == 0 && !IsStory(aLevel) ? aLevel->number : -1,
		gProfiles[gProfile].levelReached, gDifficulty, gAutoFire, RandValue(), IsOver(aLevel) ? "true" : "false");
	if (aLevel == nullptr)
		return s + "}";
	bool aBusy = IsBusy(aLevel);
	if (aLevel->kind == 0 && !IsStory(aLevel))
	{
		Shooter* aShip = aLevel->shooters.current;
		s += Fmt(",\"percent\":%d,\"quota\":%d", aLevel->totalCells > 0 ? (aLevel->filledCells * 100) / aLevel->totalCells : 0, aLevel->fillQuota);
		s += Fmt(",\"board\":{\"cols\":%d,\"rows\":%d,\"state\":%d,\"gameover\":%s,\"shots\":%d,\"lines\":%d,\"cells\":%d,\"filled\":%d,\"weapons\":%d,\"texts\":%d",
			aLevel->cols, aLevel->rows, aLevel->state, aLevel->gameOver != 0 ? "true" : "false", aLevel->shots, aLevel->lines, aLevel->totalCells,
			aLevel->filledCells, OrbitingWeapons(aLevel), aLevel->texts.head != nullptr ? 1 : 0);
		if (aShip != nullptr)
			s += Fmt(",\"ship\":{\"x\":%d,\"y\":%d,\"state\":%d,\"across\":%d,\"noauto\":%d,\"speed\":%.3f}", (int)aShip->x, (int)aShip->y, aShip->state,
				aShip->beamAcross, aShip->noAutoTurn, aShip->beamSpeed);
		s += Fmt(",\"mouse\":[%d,%d],\"pointer\":[%d,%d]", gMouseX, gMouseY, gRealX, gRealY);
		s += ",\"powerups\":[";
		int p = 0;
		for (Powerup* u = aLevel->powerups.head; u != nullptr && p < 20; u = Node_Next(u), p++)
			s += Fmt("%s[%d,%d,%d]", p > 0 ? "," : "", (int)u->x, (int)u->y, u->state);
		s += "]";
		s += ",\"enemies\":[";
		int n = 0;
		for (Enemy* e = aLevel->enemies.head; e != nullptr && n < 60; e = Node_Next(e), n++)
			s += Fmt("%s[%d,%d,%.3f,%.3f,%d,%d]", n > 0 ? "," : "", (int)e->x, (int)e->y, e->vx, e->vy, e->kind, e->state);
		s += "]}";
	}
	if (aLevel->kind != 0 || IsStory(aLevel) || HasRequester(aLevel))
	{
		s += Fmt(",\"dialog\":%s,\"items\":[", HasRequester(aLevel) ? "true" : "false");
		std::vector<ItemRef> anItems = ActiveItems(aLevel);
		for (size_t i = 0; i < anItems.size(); i++)
		{
			int x, y;
			ItemPoint(anItems[i].item, x, y);
			s += Fmt("%s[%d,%d,%d,%s,%d]", i > 0 ? "," : "", anItems[i].item->kind, x, y, Quote(anItems[i].item->name).c_str(), anItems[i].menu->lockedLevel);
		}
		s += "]";
		s += ",\"sets\":[";
		for (LevelSet* l = gLevelSets.head; l != nullptr; l = Node_Next(l))
			s += Fmt("%s[%d,%d]", l == gLevelSets.head ? "" : ",", l->start, l->end);
		s += "]";
	}
	{
		// busy: the bot has nothing to do but wait (it is asked without being changed)
		BotMemory aMemory = SaveBot();
		bool aWaiting = false;
		std::string aMove = (aLevel->kind == 0 && !IsStory(aLevel)) ? BoardMove(aLevel, aWaiting) : MenuMove(aLevel, aWaiting);
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
		if (r.item->kind == theKind)
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
	if (theLevel->kind == 0xe || anItems.empty())
	{
		// the logo screens move on by themselves
		theBusy = true;
		return "";
	}
	if (HasRequester(theLevel))
		return ClickItem("dialog", anItems.back().item);	// the last button: not the yes of "resume your previous game?"
	const ItemRef* r = nullptr;
	switch (theLevel->kind)
	{
	case 1:
		r = FindKind(anItems, 0x12);
		if (r != nullptr)
			return ClickItem("start", r->item);
		break;
	case 0:
		// a world's story screen: its start button
		r = FindKind(anItems, 1);
		if (r != nullptr)
			return ClickItem("story", r->item);
		break;
	case 5:
	{
		// the world map: the furthest world that is open (its first level is no further than the next one to play)
		const ItemRef* aBest = nullptr;
		for (const ItemRef& i : anItems)
			if (i.item->kind == 0x18 && i.menu->lockedLevel <= gProfiles[gProfile].levelReached + 1 &&
				(aBest == nullptr || i.menu->lockedLevel > aBest->menu->lockedLevel))
				aBest = &i;
		if (aBest != nullptr)
			return ClickItem("world", aBest->item);
		break;
	}
	}
	return ClickItem("menu", anItems[0].item);
}

// ---------------------------------------------------------------------------------------------------------------------
// The commands

int ScreenKind(const std::string& theName)
{
	static const struct { const char* name; int kind; } kScreens[] = {{"menu", 1}, {"title", 1}, {"options", 7}, {"help", 0xc},
		{"help2", 0xd}, {"highscores", 3}, {"credits", 4}, {"map", 5}, {"changeuser", 0x10}, {"logo", 0xe}};
	for (const auto& s : kScreens)
		if (theName == s.name)
			return s.kind;
	return -1;
}

LevelSet* SetOfLevel(int theLevel)
{
	for (LevelSet* s = gLevelSets.head; s != nullptr; s = Node_Next(s))
		if (s->start <= theLevel && theLevel <= s->end)
			return s;
	return nullptr;
}

}	// namespace

// The memory report's holders of the engine: what the engine's own lists keep, which the framework cannot see
void AddMemReportHolders(Platform::MemReport* theReport)
{
	size_t aBankBytes = (size_t)gArtBank.dataSize + (size_t)gArt8Bank.dataSize;
	size_t aFrameBytes = 0, aMaskBytes = 0, aRleBytes = 0, aPaletteBytes = 0;
	int aFrames = 0;
	auto aAddRle = [&](const r64::Image* theImage)
	{
		if (theImage != nullptr && theImage->rle != nullptr)
			aRleBytes += ((const RleHeader*)theImage->rle)->size;
	};
	for (const Bank* aBank : {&gArtBank, &gArt8Bank})
	{
		for (const r64::Image* anImage : aBank->images)
			aAddRle(anImage->mask != nullptr ? anImage->mask : anImage->autoMask);
		for (const Palette* aPalette : aBank->palettes)
			aPaletteBytes += sizeof(Palette) + (aPalette->p16 != nullptr ? 512 : 0) + (aPalette->p32 != nullptr ? 1024 : 0);
		aPaletteBytes += aBank->rawPalettes.size() * 0x400;
	}
	for (const Animation* anAnim = gAnims.head; anAnim != nullptr; anAnim = Node_Next(anAnim))
	{
		aPaletteBytes += sizeof(Animation) - sizeof(Animation::frames);
		for (int i = 0; i < anAnim->count; i++)
		{
			const r64::Image* aFrame = anAnim->frames[i];
			if (aFrame == nullptr)
				continue;
			aFrames++;
			if (aFrame->notOwned == 0)
				aFrameBytes += (size_t)aFrame->dataSize;
			if (aFrame->hasMask != 0 && aFrame->mask != nullptr)
			{
				if (aFrame->mask->notOwned == 0)
					aMaskBytes += (size_t)aFrame->mask->dataSize;
				aAddRle(aFrame->mask);
			}
		}
	}
	theReport->mOthers.push_back({"art banks' pixels and masks", aBankBytes});
	theReport->mOthers.push_back({"animation frames (" + std::to_string(aFrames) + ")", aFrameBytes});
	theReport->mOthers.push_back({"animation masks", aMaskBytes});
	theReport->mOthers.push_back({"run-length masks", aRleBytes});
	theReport->mOthers.push_back({"palettes and animation records", aPaletteBytes});
	theReport->mOthers.push_back({"back buffer", (size_t)gBackBuffer.dataSize});
}

bool HandleTestCommand(SexyAppBase* theApp, const ScriptCommand& theCommand)
{
	const std::string& aName = theCommand.mCommand;
	gUpdateCount = theApp->mUpdateCount;
	gRealX = theApp->mRealMouseX;
	gRealY = theApp->mRealMouseY;
	Level* aLevel = gLevels.current;
	if (aName == "state")
	{
		Reply("STATE " + StateJson());
		return true;
	}
	if (aName == "grid")
	{
		// the board as text, one line per row: '.' open, '+' open and already marked by the pending beam, '#' not open, 'e' a bug
		if (aLevel == nullptr || aLevel->kind != 0)
		{
			ReplyError(theCommand, "there is no board");
			return true;
		}
		std::string g;
		for (int r = 0; r < aLevel->rows && r < 37; r++)
		{
			std::string aRow;
			for (int c = 0; c < aLevel->cols && c < 50; c++)
				aRow += aLevel->columns[c][r].kind != 0 ? '#' : (aLevel->columns[c][r].unmarked == 0 ? '+' : '.');
			for (Enemy* e = aLevel->enemies.head; e != nullptr; e = Node_Next(e))
				if ((int)e->y / 16 == r && 0 <= (int)e->x / 16 && (int)e->x / 16 < (int)aRow.size())
					aRow[(int)e->x / 16] = 'e';
			g += aRow + (r + 1 < aLevel->rows ? "|" : "");
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
		bool aBoard = aLevel->kind == 0 && !IsStory(aLevel);
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
		// board n of the game (0 is the first as well); the story screen that comes before a world's first board comes first.
		// The game's own count of levels left behind is one less (Start after n boards played goes to board n + 1).
		int n = std::max(std::atoi(theCommand.mArgs.c_str()), 1);
		if (aLevel == nullptr)
			ReplyError(theCommand, "the game has no screen yet");
		else if (SetOfLevel(n) == nullptr)
			ReplyError(theCommand, "no such level");
		else
		{
			bool aFromBoard = aLevel->kind == 0;
			if (aFromBoard)
			{
				// back to the main menu first, as Esc's quit does, so that the boards are read afresh (that reads the game
				// file again, which makes the level sets new: the set is found after it)
				gGoTo = 1;
				LeaveScreen();
			}
			else
			{
				gLives = 6;
				gScore = 0;
			}
			gLevelSets.current = SetOfLevel(n);
			gLevelInSet = n - 1;
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
			for (LevelSet* s = gLevelSets.head; s != nullptr; s = Node_Next(s))
				aLast = std::max(aLast, s->end);
			gProfiles[gProfile].levelReached = std::max(gProfiles[gProfile].levelReached, aLast);
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
			gProfiles[gProfile].autoFire = aValue;
			ReplyOK(theCommand);
		}
		else if (!std::strcmp(aKey, "reached") && aValue >= 0)
			gProfiles[gProfile].levelReached = aValue, ReplyOK(theCommand);
		else
			ReplyError(theCommand, "names are score, lives, difficulty (-1..1), shootermode (0, 1), reached");
		return true;
	}
	if (aName == "win")
	{
		if (aLevel == nullptr || aLevel->kind != 0 || aLevel->state != 1)
			ReplyError(theCommand, "no board is being played");
		else
		{
			aLevel->filledCells = aLevel->totalCells;
			Level_Won(aLevel);
			ReplyOK(theCommand);
		}
		return true;
	}
	if (aName == "lose")
	{
		if (aLevel == nullptr || aLevel->kind != 0 || aLevel->state != 1 || aLevel->gameOver != 0)
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
