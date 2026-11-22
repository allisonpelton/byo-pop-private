// port: the test bot (see Bot.h). Strategy in a level: for every stretch of ground the monkey could cut (a row between
// two walls, a column between two walls) it works out, with a copy of the level's own preview map, what the rope would
// cut off (the smaller side, as LevelObj::ShowRopePreview decides), throws it out when a bug's box is near the rope's
// line (the nearer the longer the rope takes to run), and shoots the one that cuts most. Everything else is the click
// that goes on: the menu's Play, the question window's Yes, the map's continue, the tally's click, the intro's skip.
// Weaknesses: it does not follow where a bug is heading, only how near it is now; it picks no upgrades in the shop
// (it leaves the shop at once); it never plays the casino.

#include "Bot.h"

#include "AskWin.h"
#include "BBWindow.h"
#include "Board.h"
#include "BugImg.h"
#include "BugObj.h"
#include "FillObj.h"
#include "GameApp.h"
#include "GameWidget.h"
#include "Intro.h"
#include "LevelObj.h"
#include "MainMenu.h"
#include "Map.h"
#include "MetaGame.h"
#include "NewUser.h"
#include "ObjString.h"
#include "PipeObj.h"
#include "Player.h"
#include "RopeObj.h"
#include "Slots.h"
#include "Stats.h"
#include "Transition.h"
#include "Victory.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace Sexy;

namespace
{

// The map screen's state (MetaGame::mState; the enum is private to MetaGame.cpp)
enum BotMetaState
{
	META_WAIT_START = 0,
	META_INTRO = 1,
	META_SLOTS_START = 2,
	META_SLOTS = 3,
	META_LIVES_START = 4,
	META_LIVES = 5,
	META_SHOP = 6,
	META_MAP = 7,
	META_LEAVE = 9,
	META_LEAVING = 10,
};

// The map screen's done button (MetaGame's button id) and the slots' done button are found as objects; this is the name
// the bot types when the new-player window is open
const char* const BOT_PLAYER_NAME = "BOT";

// The preview map's places and what is in them (LevelObj.cpp's constants)
const int PV_COLUMNS = 0x50;
const int PV_ROWS = 0x40;
const int PV_FREE = 1;
const int PV_WALL = 2;
const int PV_FILLED = 9;
const int PV_SMALL_AREA = 0x20;

// The game's cell, in design pixels (FILL_CELL_SPACING), and the shift the monkey's point has to the cell it aims at
const float CELL_PX = 31.0f;
const float AIM_SHIFT_Y = 24.0f;

// A bug near the monkey stops the shot: Player::UpdateAim's startle reach, and a margin over it for a bug that moves
const float STARTLE_REACH_PX = 64.0f;
const float STARTLE_MARGIN_PX = 40.0f;

// How far a bug's box must be from the rope's line (design pixels): this much at least, and more the longer the rope is
// (its flight takes shot-time * length frames, in which a bug walks on). The second tier drops the part for the length
const float LINE_MARGIN_BASE_PX = 36.0f;
const float LINE_MARGIN_PER_PX = 0.3f; // 0.16 lost three lives in two levels on lines of 465 to 713 px with gaps of 124 to 195
// Whether the bot turns the rope with a right click when the best line lies across the way the game aims by itself
// (the shorter stretch); the second button turns it (GameWidget::MouseDown passes the click count, negative for it)
const bool BOT_TURNS_ROPE = true;

// The waits with no safe line (20 updates each) before the bot takes the least risky line there is
const int BOT_WAITS_BEFORE_RISK = 15;

// The rope's own thickness and the reach of its arrows (RopeObj::HitBox: 4 by 6)
const float ROPE_HALF_THICK_PX = 6.0f;

// The click's steps: the pointer settles on the point first (the object under it is found when the frame is drawn, and a
// -fast run draws only every 10th update, so the wait is longer than that), then the button is held over some updates (a
// press is seen only when the button is down during an update)
const int CLICK_SETTLE_UPDATES = 12;
const int CLICK_HOLD_UPDATES = 4;

std::string Num(float theValue)
{
	char aBuffer[32];
	snprintf(aBuffer, sizeof(aBuffer), "%d", (int)lroundf(theValue));
	return aBuffer;
}

std::string Point(int theX, int theY)
{
	return "[" + std::to_string(theX) + "," + std::to_string(theY) + "]";
}

void DesignToWindow(float theDesignX, float theDesignY, int* theX, int* theY)
{
	*theX = (int)lroundf((theDesignX + 960.0f) / gApp->mWindowToDesignX);
	*theY = (int)lroundf((theDesignY + 600.0f) / gApp->mWindowToDesignY);
}

// A move: the steps, the updates to wait after, and a word for what it is
std::string Move(const std::string& theSteps, int theUpdates, const char* theWhat)
{
	return std::string("{\"what\":\"") + theWhat + "\",\"input\":[" + theSteps + "],\"updates\":" +
		std::to_string(theUpdates) + "}";
}

std::string Wait(int theUpdates, const char* theWhat)
{
	return Move("", theUpdates, theWhat);
}

std::string ClickSteps(int theX, int theY, bool theRight)
{
	std::string aPoint = Point(theX, theY);
	std::string aButton = theRight ? ",\"button\":\"right\"" : "";
	return "{\"move\":" + aPoint + "},{\"wait\":" + std::to_string(CLICK_SETTLE_UPDATES) + "},{\"press\":" + aPoint +
		aButton + "},{\"wait\":" + std::to_string(CLICK_HOLD_UPDATES) + "},{\"release\":" + aPoint + aButton + "}";
}

// A click on a point of the window, then a few updates for what it does
std::string ClickAt(int theX, int theY, const char* theWhat, int theUpdates = 10)
{
	return Move(ClickSteps(theX, theY, false), theUpdates, theWhat);
}

// A click on an object, where the game's design space puts it
std::string ClickObject(const ObjBase* theObject, const char* theWhat, int theUpdates = 10)
{
	int aX;
	int aY;
	DesignToWindow(theObject->mScreenX, theObject->mScreenY, &aX, &aY);
	return ClickAt(aX, aY, theWhat, theUpdates);
}

GameWidget* TheWidget()
{
	return gApp != nullptr ? gApp->mGameWidget : nullptr;
}

LevelObj* CurrentLevel()
{
	GameWidget* aWidget = TheWidget();
	if (aWidget == nullptr || aWidget->mScreen != SCREEN_LEVEL || aWidget->mBoard == nullptr)
		return nullptr;
	return aWidget->mBoard->mLevel;
}

bool PipeOn(int theColumn, int theRow)
{
	if (theColumn < 0 || theColumn >= FILL_GRID_COLUMNS || theRow < 0 || theRow >= FILL_GRID_ROWS)
		return false;
	PipeObj* aPipe = gFillGrid[theColumn][theRow].mPipe;
	return aPipe != nullptr && aPipe->IsActive();
}

bool CellFree(int theColumn, int theRow)
{
	FillObj* aCell = gFillGrid[theColumn][theRow].mCell;
	return aCell != nullptr && aCell->IsActive() && !aCell->mCut;
}

// ---- the preview map, a copy of LevelObj::BuildPreviewMap and ShowRopePreview that changes nothing ----

struct Preview
{
	unsigned char mMap[PV_COLUMNS][PV_ROWS];
};

void BuildPreview(Preview* thePreview)
{
	memset(thePreview->mMap, 0, sizeof(thePreview->mMap));
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			if (CellFree(aColumn, aRow)) {
				thePreview->mMap[aColumn * 2 + 1][aRow * 2 + 1] = PV_FREE;
				thePreview->mMap[aColumn * 2 + 2][aRow * 2 + 1] = PV_FREE;
				thePreview->mMap[aColumn * 2 + 1][aRow * 2 + 2] = PV_FREE;
				thePreview->mMap[aColumn * 2 + 2][aRow * 2 + 2] = PV_FREE;
			}
			if (!PipeOn(aColumn, aRow))
				continue;
			PipeObj* aPipe = gFillGrid[aColumn][aRow].mPipe;
			thePreview->mMap[aColumn * 2][aRow * 2] = PV_WALL;
			if (aPipe->mKind == FILL_KIND_CORNER) {
				PipeObj* anUp = PipeOn(aColumn, aRow - 1) ? gFillGrid[aColumn][aRow - 1].mPipe : nullptr;
				PipeObj* aLeft = PipeOn(aColumn - 1, aRow) ? gFillGrid[aColumn - 1][aRow].mPipe : nullptr;
				PipeObj* aDown = PipeOn(aColumn, aRow + 1) ? gFillGrid[aColumn][aRow + 1].mPipe : nullptr;
				PipeObj* aRight = PipeOn(aColumn + 1, aRow) ? gFillGrid[aColumn + 1][aRow].mPipe : nullptr;
				if (anUp != nullptr && anUp->mKind != FILL_KIND_WALL_ACROSS)
					thePreview->mMap[aColumn * 2][aRow * 2 - 1] = PV_WALL;
				if (aDown != nullptr && aDown->mKind != FILL_KIND_WALL_ACROSS)
					thePreview->mMap[aColumn * 2][aRow * 2 + 1] = PV_WALL;
				if (aRight != nullptr && aRight->mKind != FILL_KIND_WALL_DOWN)
					thePreview->mMap[aColumn * 2 + 1][aRow * 2] = PV_WALL;
				if (aLeft != nullptr && aLeft->mKind != FILL_KIND_WALL_DOWN)
					thePreview->mMap[aColumn * 2 - 1][aRow * 2] = PV_WALL;
			} else if (aPipe->mKind == FILL_KIND_WALL_ACROSS) {
				thePreview->mMap[aColumn * 2 + 1][aRow * 2] = PV_WALL;
			} else if (aPipe->mKind == FILL_KIND_WALL_DOWN) {
				thePreview->mMap[aColumn * 2][aRow * 2 + 1] = PV_WALL;
			}
		}
	}
}

// The places a flood fill from one place reaches (4 ways), marking them filled; the count of them
int Flood(Preview* thePreview, int theX, int theY)
{
	int aCount = 0;
	std::vector<std::pair<int, int>> aStack;
	aStack.push_back(std::make_pair(theX, theY));
	while (!aStack.empty()) {
		std::pair<int, int> aPlace = aStack.back();
		aStack.pop_back();
		int aX = aPlace.first;
		int aY = aPlace.second;
		if (aX < 0 || aY < 0 || aX >= PV_COLUMNS || aY >= PV_ROWS || thePreview->mMap[aX][aY] != PV_FREE)
			continue;
		thePreview->mMap[aX][aY] = PV_FILLED;
		aCount++;
		aStack.push_back(std::make_pair(aX + 1, aY));
		aStack.push_back(std::make_pair(aX - 1, aY));
		aStack.push_back(std::make_pair(aX, aY + 1));
		aStack.push_back(std::make_pair(aX, aY - 1));
	}
	return aCount;
}

// What a rope along a stretch would cut: the places of the preview map that go (4 to a cell), 0 when nothing would. The
// rule is the game's: the side with fewer places goes (the first side when they are as many), both when neither has more
// than 32, both when no bug is alive
int CutPlaces(const Preview& theBase, int theVertical, int theLine, int theFrom, int theTo)
{
	Preview aPreview = theBase;
	for (int i = theFrom; i <= theTo; i++) {
		if (theVertical == 0) {
			aPreview.mMap[i * 2][theLine * 2] = PV_WALL;
			aPreview.mMap[i * 2 + 1][theLine * 2] = PV_WALL;
		} else {
			aPreview.mMap[theLine * 2][i * 2] = PV_WALL;
			aPreview.mMap[theLine * 2][i * 2 + 1] = PV_WALL;
		}
	}
	int aFlooded = 0;
	for (int i = theFrom; i <= theTo; i++)
		aFlooded += theVertical == 0 ? Flood(&aPreview, i * 2, theLine * 2 + 1) : Flood(&aPreview, theLine * 2 + 1, i * 2);
	int aSideB = aFlooded - theFrom + 1 + theTo;
	int aSideA = 0;
	for (int i = theFrom; i <= theTo; i++)
		aSideA += theVertical == 0 ? Flood(&aPreview, i * 2, theLine * 2 - 1) : Flood(&aPreview, theLine * 2 - 1, i * 2);
	int aCut = 0;
	if (aSideB > 0 && aSideA > 0) {
		aCut = aSideB < aSideA ? aSideB : aSideA;
		if (aSideB <= PV_SMALL_AREA && aSideA <= PV_SMALL_AREA)
			aCut = aSideA + aSideB;
		if (BugObj::sAlive < 1)
			aCut = aSideA + aSideB;
	}
	return aCut;
}

// ---- the bugs ----

struct BugBox
{
	float mX;
	float mY;
	float mHalfWidth;
	float mHalfHeight;
};

// The kinds that fly in straight dashes across the open ground (the skeeter, the dragonfly, the fly and the moth, which
// shares the fly's kind number, and the bee): their boxes grow by this much each way, as they cross a gap the walkers
// cannot in a rope's flight (with only the length margin the bot lost two lives in two levels to skeeters 160 px away)
const float FLIER_REACH_PX = 80.0f;

bool IsFlier(int theKind)
{
	return theKind == BUG_SKEETER || theKind == BUG_DRAGONFLY || theKind == BUG_FLY || theKind == BUG_BEE;
}

std::vector<BugBox> AliveBugs()
{
	std::vector<BugBox> aBugs;
	for (int i = 0; i < BugObj::sCount; i++) {
		BugObj* aBug = BugObj::Get(i);
		if (aBug == nullptr || !aBug->mAlive || aBug->mFlying)
			continue;
		BugBox aBox;
		if (aBug->mBody != nullptr) {
			aBox.mX = aBug->mBody->mScreenX;
			aBox.mY = aBug->mBody->mScreenY;
			aBox.mHalfWidth = aBug->mBody->mHitWidth * 0.5f;
			aBox.mHalfHeight = aBug->mBody->mHitHeight * 0.5f;
		} else {
			aBox.mX = aBug->mScreenX;
			aBox.mY = aBug->mScreenY;
			aBox.mHalfWidth = 27.0f;
			aBox.mHalfHeight = 27.0f;
		}
		if (IsFlier(aBug->mKind)) {
			aBox.mHalfWidth += FLIER_REACH_PX;
			aBox.mHalfHeight += FLIER_REACH_PX;
		}
		aBugs.push_back(aBox);
	}
	return aBugs;
}

// The gap between a bug's box and a rope's rectangle (0 when they touch)
float Gap(const BugBox& theBug, float theLeft, float theTop, float theRight, float theBottom)
{
	float aGapX = std::max(std::max(theLeft - (theBug.mX + theBug.mHalfWidth), (theBug.mX - theBug.mHalfWidth) - theRight), 0.0f);
	float aGapY = std::max(std::max(theTop - (theBug.mY + theBug.mHalfHeight), (theBug.mY - theBug.mHalfHeight) - theBottom), 0.0f);
	return sqrtf(aGapX * aGapX + aGapY * aGapY);
}

// ---- the level's shot ----

struct Candidate
{
	int mVertical;
	int mLine;          // the row of a stretch along a row, the column of one along a column
	int mFrom;
	int mTo;
	int mCellX;         // the cell the monkey points at
	int mCellY;
	int mCut;           // places cut
	float mLength;      // the rope's length in design pixels
	float mDistance;    // the nearest bug's gap to the rope's line
};

// Where the game's aim (Player::UpdateAim) puts the stretch for a cell: false when the cell takes no rope
bool AimAt(int theColumn, int theRow, int* theLeft, int* theRight, int* theTop, int* theBottom)
{
	if (!CellFree(theColumn, theRow) || PipeOn(theColumn, theRow))
		return false;
	int aRight = theColumn;
	int aLeft = theColumn;
	int aBottom = theRow;
	int aTop = theRow;
	if (theColumn < FILL_GRID_COLUMNS - 1) {
		for (int i = theColumn; i < FILL_GRID_COLUMNS; i++) {
			if (PipeOn(i, theRow)) {
				aRight = i;
				break;
			}
		}
	}
	if (theColumn > 0) {
		for (int i = theColumn; i >= 0; i--) {
			if (PipeOn(i, theRow)) {
				aLeft = i;
				break;
			}
		}
	}
	if (theRow < FILL_GRID_ROWS - 1) {
		for (int j = theRow; j < FILL_GRID_ROWS; j++) {
			if (PipeOn(theColumn, j)) {
				aBottom = j;
				break;
			}
		}
	}
	if (theRow > 0) {
		for (int j = theRow; j >= 0; j--) {
			if (PipeOn(theColumn, j)) {
				aTop = j;
				break;
			}
		}
	}
	*theLeft = aLeft;
	*theRight = aRight;
	*theTop = aTop;
	*theBottom = aBottom;
	return true;
}

// The rope's line in the design space, from the pipes at its two ends
void LineRect(const Candidate& theCandidate, float* theLeft, float* theTop, float* theRight, float* theBottom)
{
	PipeObj* aStart = theCandidate.mVertical == 0 ? gFillGrid[theCandidate.mFrom][theCandidate.mLine].mPipe
												  : gFillGrid[theCandidate.mLine][theCandidate.mFrom].mPipe;
	PipeObj* anEnd = theCandidate.mVertical == 0 ? gFillGrid[theCandidate.mTo][theCandidate.mLine].mPipe
												 : gFillGrid[theCandidate.mLine][theCandidate.mTo].mPipe;
	*theLeft = std::min(aStart->mScreenX, anEnd->mScreenX) - ROPE_HALF_THICK_PX;
	*theRight = std::max(aStart->mScreenX, anEnd->mScreenX) + ROPE_HALF_THICK_PX;
	*theTop = std::min(aStart->mScreenY, anEnd->mScreenY) - ROPE_HALF_THICK_PX;
	*theBottom = std::max(aStart->mScreenY, anEnd->mScreenY) + ROPE_HALF_THICK_PX;
}

// The monkey's place in the design space when it points at a cell
void CellDesign(int theColumn, int theRow, float* theX, float* theY)
{
	FillObj* aCell = gFillGrid[theColumn][theRow].mCell;
	*theX = aCell->mScreenX;
	*theY = aCell->mScreenY;
}

// Every stretch the monkey could shoot along, with the cell to point at for it
std::vector<Candidate> Candidates(const std::vector<BugBox>& theBugs)
{
	Preview aBase;
	BuildPreview(&aBase);
	std::vector<Candidate> aList;
	for (int aVertical = 0; aVertical < 2; aVertical++) {
		int aLines = aVertical == 0 ? FILL_GRID_ROWS : FILL_GRID_COLUMNS;
		int aAlong = aVertical == 0 ? FILL_GRID_COLUMNS : FILL_GRID_ROWS;
		for (int aLine = 0; aLine < aLines; aLine++) {
			int aFrom = -1;
			for (int i = 0; i < aAlong; i++) {
				bool aWall = aVertical == 0 ? PipeOn(i, aLine) : PipeOn(aLine, i);
				if (!aWall)
					continue;
				int aPrevious = aFrom;
				aFrom = i;
				if (aPrevious < 0 || i - aPrevious < 2)
					continue;
				Candidate aCandidate;
				aCandidate.mVertical = aVertical;
				aCandidate.mLine = aLine;
				aCandidate.mFrom = aPrevious;
				aCandidate.mTo = i;
				aCandidate.mCut = CutPlaces(aBase, aVertical, aLine, aPrevious, i);
				if (aCandidate.mCut <= 0)
					continue;
				aCandidate.mLength = (float)(i - aPrevious) * CELL_PX;
				float aLeft;
				float aTop;
				float aRight;
				float aBottom;
				LineRect(aCandidate, &aLeft, &aTop, &aRight, &aBottom);
				aCandidate.mDistance = 100000.0f;
				for (size_t b = 0; b < theBugs.size(); b++)
					aCandidate.mDistance = std::min(aCandidate.mDistance, Gap(theBugs[b], aLeft, aTop, aRight, aBottom));
				// the cell to point at: between the two walls, free, and the farthest from any bug (the monkey must
				// not startle: a bug within the reach of the monkey's place stops the shot)
				float aBestFar = -1.0f;
				aCandidate.mCellX = -1;
				aCandidate.mCellY = -1;
				for (int c = aPrevious + 1; c < i; c++) {
					int aColumn = aVertical == 0 ? c : aLine;
					int aRow = aVertical == 0 ? aLine : c;
					int aL, aR, aT, aB;
					if (!AimAt(aColumn, aRow, &aL, &aR, &aT, &aB))
						continue;
					int aWantFrom = aVertical == 0 ? aL : aT;
					int aWantTo = aVertical == 0 ? aR : aB;
					if (aWantFrom != aPrevious || aWantTo != i)
						continue;
					// the way the game aims at this cell, unless the bot may turn the rope
					if (!BOT_TURNS_ROPE && ((aR - aL < aB - aT) ? 0 : 1) != aVertical)
						continue;
					float aX;
					float aY;
					CellDesign(aColumn, aRow, &aX, &aY);
					float aNearest = 100000.0f;
					for (size_t b = 0; b < theBugs.size(); b++)
						aNearest = std::min(aNearest, sqrtf((theBugs[b].mX - aX) * (theBugs[b].mX - aX) +
							(theBugs[b].mY - aY) * (theBugs[b].mY - aY)));
					if (aNearest > aBestFar) {
						aBestFar = aNearest;
						aCandidate.mCellX = aColumn;
						aCandidate.mCellY = aRow;
					}
				}
				if (aCandidate.mCellX < 0 || aBestFar < STARTLE_REACH_PX + STARTLE_MARGIN_PX)
					continue;
				aList.push_back(aCandidate);
			}
		}
	}
	return aList;
}

// Whether the monkey may shoot now, and if not why not (nullptr when it may)
const char* ShotBlocked(Player* thePlayer, LevelObj* theLevel)
{
	if (theLevel->mState != LEVEL_STATE_PLAYING)
		return "level not playing";
	if (thePlayer->mMonkeyState != MONKEY_PLAYING)
		return "monkey busy";
	if (thePlayer->mShotPhase != SHOT_NONE)
		return "shot in progress";
	if (thePlayer->mShotCooldown > 0)
		return "cooling down";
	if (!FillObj::IsReady() || !FillObj::IsIdle())
		return "ground busy";
	RopeObj* aRope = theLevel->mRope;
	if (aRope != nullptr && aRope->mState == ROPE_FLYING) // (a rope that has cut or snapped stays in its state until the next shot)
		return "rope busy";
	if (thePlayer->mPercent >= Player::GetTargetPercent())
		return "target reached";
	if (!thePlayer->IsActive() || thePlayer->mHidden)
		return "monkey away";
	return nullptr;
}

std::string LevelMove(LevelObj* theLevel)
{
	Player* aPlayer = Player::sCurrent;
	if (Transition::GetPhase() != TRANSITION_OPEN)
		return Wait(10, "wait for the doors");
	switch (theLevel->mState) {
	case LEVEL_STATE_INTRO:
		if (!Intro::IsReady() && theLevel->mIntro != nullptr) {
			// a press in the intro's first 100 updates is thrown away; the click is the skip
			return ClickAt(400, 300, "skip the intro", 20);
		}
		return Wait(10, "wait for the level");
	case LEVEL_STATE_STATS:
		if (theLevel->mStats != nullptr && theLevel->mStats->IsReady())
			return ClickAt(400, 300, "click the tally on", 20);
		return Wait(10, "wait for the tally");
	case LEVEL_STATE_VICTORY:
		return ClickAt(400, 300, "click the victory away", 20);
	case LEVEL_STATE_PLAYING:
		break;
	default:
		return Wait(20, "wait");
	}

	// a held bonus: put it down where the bugs are
	if (aPlayer->mHolding == 1 && aPlayer->mHeld != nullptr) {
		std::vector<BugBox> aBugs = AliveBugs();
		if (aBugs.empty())
			return Wait(10, "held bonus, no bug");
		int aPick = (int)((BugObj::sCount + aPlayer->mHeldCount) % (int)aBugs.size());
		int aX;
		int aY;
		DesignToWindow(aBugs[aPick].mX, aBugs[aPick].mY, &aX, &aY);
		std::string aPoint = Point(aX, aY);
		return Move("{\"move\":" + Point(aX - 1, aY) + "},{\"wait\":4},{\"move\":" + aPoint + "},{\"wait\":14},{\"press\":" + aPoint +
			"},{\"wait\":3},{\"release\":" + aPoint + "}", 20, "put the bonus down");
	}
	const char* aBlocked = ShotBlocked(aPlayer, theLevel);
	if (aBlocked != nullptr)
		return Wait(aBlocked[0] == 'm' ? 30 : 20, aBlocked);

	std::vector<BugBox> aBugs = AliveBugs();
	std::vector<Candidate> aList = Candidates(aBugs);
	const Candidate* aBest = nullptr;
	// the first tier: the margin grows with the length of the rope; the best is the one that cuts most
	for (size_t i = 0; i < aList.size(); i++) {
		const Candidate& aCandidate = aList[i];
		if (aCandidate.mDistance < LINE_MARGIN_BASE_PX + LINE_MARGIN_PER_PX * aCandidate.mLength)
			continue;
		if (aBest == nullptr || aCandidate.mCut > aBest->mCut ||
			(aCandidate.mCut == aBest->mCut && aCandidate.mLength < aBest->mLength))
			aBest = &aCandidate;
	}
	// the second: the shortest line with the base margin
	if (aBest == nullptr) {
		for (size_t i = 0; i < aList.size(); i++) {
			const Candidate& aCandidate = aList[i];
			if (aCandidate.mDistance < LINE_MARGIN_BASE_PX)
				continue;
			if (aBest == nullptr || aCandidate.mLength < aBest->mLength)
				aBest = &aCandidate;
		}
	}
	// the third, after waiting long enough for the bugs to move off: the line farthest from every bug, the shortest of
	// equals (a last bug in a small area is never far enough for the others; a playbymove run waited 2,500 moves for one)
	static int sWaitsWithoutLine = 0;
	if (aBest == nullptr && sWaitsWithoutLine >= BOT_WAITS_BEFORE_RISK) {
		for (size_t i = 0; i < aList.size(); i++) {
			const Candidate& aCandidate = aList[i];
			if (aBest == nullptr || aCandidate.mDistance > aBest->mDistance ||
				(aCandidate.mDistance == aBest->mDistance && aCandidate.mLength < aBest->mLength))
				aBest = &aCandidate;
		}
	}
	if (aBest == nullptr) {
		sWaitsWithoutLine++;
		return Wait(20, "no safe line");
	}
	sWaitsWithoutLine = 0;

	int aX;
	int aY;
	if (!BotCellToWindow(aBest->mCellX, aBest->mCellY, &aX, &aY))
		return Wait(10, "no level");
	// which way the game aims there: the shorter stretch, unless the aim is locked by a turn
	int aL, aR, aT, aB;
	AimAt(aBest->mCellX, aBest->mCellY, &aL, &aR, &aT, &aB);
	int aGameVertical = (aR - aL < aB - aT) ? 0 : 1;
	if (aPlayer->mSegLocked != 0)
		aGameVertical = aPlayer->mSegVertical;
	std::string aPoint = Point(aX, aY);
	// the pointer goes there from the pixel beside it (the monkey follows the mean of the last two places), settles, turns the rope when it
	// aims the other way (a right click), settles again, and shoots (a left click)
	// (a second move to the same point sends no event, so the first is a pixel beside it)
	std::string aSteps = "{\"move\":" + Point(aX - 1, aY) + "},{\"wait\":2},{\"move\":" + aPoint + "},{\"wait\":4}";
	if (BOT_TURNS_ROPE && aGameVertical != aBest->mVertical)
		aSteps += ",{\"press\":" + aPoint + ",\"button\":\"right\"},{\"wait\":3},{\"release\":" + aPoint +
			",\"button\":\"right\"},{\"wait\":3}";
	aSteps += ",{\"press\":" + aPoint + "},{\"wait\":3},{\"release\":" + aPoint + "}";
	std::string aShot = Move(aSteps, 10, aBest->mVertical == 0 ? "shoot along a row" : "shoot along a column");
	// the choice, for the log: the cell, the stretch, what it cuts (places of the preview map), its length and the gap to the nearest bug
	aShot.insert(aShot.size() - 1, ",\"cell\":[" + std::to_string(aBest->mCellX) + "," + std::to_string(aBest->mCellY) +
		"],\"line\":" + std::to_string(aBest->mLine) + ",\"from\":" + std::to_string(aBest->mFrom) + ",\"to\":" +
		std::to_string(aBest->mTo) + ",\"cut\":" + std::to_string(aBest->mCut) + ",\"length\":" + Num(aBest->mLength) +
		",\"gap\":" + Num(aBest->mDistance));
	return aShot;
}

// ---- the other screens ----

std::string MenuMove(MainMenu* theMenu)
{
	if (Transition::GetPhase() != TRANSITION_OPEN || !theMenu->mReady || theMenu->mButtonsArrived < MENU_BUTTONS)
		return Wait(10, "wait for the menu");
	switch (theMenu->mMenuState) {
	case MENU_STATE_NEW_USER:
		if (theMenu->mNewUser != nullptr && theMenu->mNewUser->GetState() == BBWINDOW_OPEN &&
			theMenu->mNewUser->mAskWin == nullptr) {
			std::string aSteps;
			for (const char* c = BOT_PLAYER_NAME; *c != 0; c++)
				aSteps += std::string("{\"key\":\"") + *c + "\"},{\"wait\":1},";
			aSteps += "{\"key\":\"Enter\"}";
			return Move(aSteps, 20, "type a name");
		}
		if (theMenu->mNewUser != nullptr && theMenu->mNewUser->mAskWin != nullptr)
			return ClickObject(theMenu->mNewUser->mAskWin->mButtons[0], "answer the name window");
		return Wait(10, "wait for the name window");
	case MENU_STATE_CONTINUE_ASK:
		if (theMenu->mAskWin != nullptr && theMenu->mAskWin->mChoice == ASK_CHOICE_NONE && !theMenu->mAskWin->mClosed)
			return ClickObject(theMenu->mAskWin->mButtons[0], "continue the last game");
		return Wait(10, "wait for the question");
	case MENU_STATE_MAP:
		if (theMenu->mMap != nullptr && theMenu->mMap->mPlayButton != nullptr && theMenu->mMap->mWalkDelay < 1)
			return ClickObject(theMenu->mMap->mPlayButton, "map: continue", 30);
		return Wait(10, "wait for the map");
	case MENU_STATE_IDLE:
		for (int i = 0; i < MENU_BUTTONS; i++) {
			if (theMenu->mButtons[i] != nullptr && theMenu->mButtons[i]->mButtonId == 100)
				return ClickObject(theMenu->mButtons[i], "Play", 30);
		}
		return Wait(10, "no Play button");
	case MENU_STATE_EXIT_ASK:
		if (theMenu->mAskWin != nullptr && theMenu->mAskWin->mChoice == ASK_CHOICE_NONE)
			return ClickObject(theMenu->mAskWin->mButtons[1], "do not exit");
		return Wait(10, "wait");
	default:
		return Wait(10, "wait");
	}
}

std::string MetaMove(MetaGame* theMeta)
{
	if (Transition::GetPhase() != TRANSITION_OPEN)
		return Wait(10, "wait for the doors");
	if (theMeta->mGameMenu != nullptr)
		return Wait(10, "game menu open");
	switch (theMeta->mState) {
	case META_INTRO:
		return ClickAt(400, 300, "skip the first picture", 20);
	case META_SLOTS:
		if (theMeta->mSlots != nullptr && theMeta->mSlots->mDoneButton != nullptr && theMeta->mSlots->mState == SLOTS_IDLE)
			return ClickObject(theMeta->mSlots->mDoneButton, "leave the slots", 30);
		return Wait(20, "wait for the slots");
	case META_SHOP:
		if (theMeta->mDoneButton != nullptr && theMeta->mDoneButton->IsActive())
			return ClickObject(theMeta->mDoneButton, "shop: done", 30);
		return Wait(10, "wait for the shop");
	case META_MAP: {
		Map* aMap = theMeta->mMap;
		if (aMap != nullptr && aMap->mPlayButton != nullptr && aMap->mWalkDelay < 1)
			return ClickObject(aMap->mPlayButton, "map: continue", 30);
		return Wait(10, "wait for the map");
	}
	default:
		return Wait(10, "wait");
	}
}

} // namespace

namespace Sexy
{

bool BotCellToWindow(int theColumn, int theRow, int* theX, int* theY)
{
	if (ObjBase::sLevel == nullptr || gApp == nullptr)
		return false;
	// the monkey's point is the mouse in the design space; the level's own origin is taken off, and the aim adds a shift
	float aDesignX = ObjBase::sLevel->mState[STATE_X] + (float)theColumn * CELL_PX;
	float aDesignY = ObjBase::sLevel->mState[STATE_Y] + (float)theRow * CELL_PX - AIM_SHIFT_Y;
	DesignToWindow(aDesignX, aDesignY, theX, theY);
	return true;
}

const char* BotScreen()
{
	GameWidget* aWidget = TheWidget();
	if (aWidget == nullptr)
		return "none";
	switch (aWidget->mScreen) {
	case SCREEN_POPCAP:
		return "popcap";
	case SCREEN_LOADING:
		return "loading";
	case SCREEN_TO_MENU:
	case SCREEN_TO_PLAY:
		return "transition";
	case SCREEN_MENU:
		return "menu";
	case SCREEN_LEVEL: {
		LevelObj* aLevel = CurrentLevel();
		if (aLevel == nullptr)
			return "level";
		switch (aLevel->mState) {
		case LEVEL_STATE_INTRO:
			return (!Intro::IsReady() && aLevel->mIntro != nullptr) ? "intro" : "level";
		case LEVEL_STATE_STATS:
			return "stats";
		case LEVEL_STATE_VICTORY:
			return "victory";
		default:
			return "level";
		}
	}
	case SCREEN_MAP: {
		MetaGame* aMeta = aWidget->mMetaGame;
		if (aMeta == nullptr)
			return "map";
		if (aMeta->mState == META_INTRO)
			return "intro";
		if (aMeta->mState == META_SLOTS_START || aMeta->mState == META_SLOTS)
			return "slots";
		return "map";
	}
	}
	return "unknown";
}

const char* BotSubState()
{
	GameWidget* aWidget = TheWidget();
	if (aWidget == nullptr)
		return "none";
	switch (aWidget->mScreen) {
	case SCREEN_MENU: {
		MainMenu* aMenu = aWidget->mMainMenu;
		if (aMenu == nullptr)
			return "none";
		static const char* const aNames[] = { "idle", "continue_ask", "continue_closing", "start_map", "map", "options",
			"high_scores", "help", "new_user", "user_select", "exit_ask", "exit_no", "exit_yes", "left" };
		int aState = aMenu->mMenuState;
		return aState >= 0 && aState < 14 ? aNames[aState] : "unknown";
	}
	case SCREEN_LEVEL: {
		LevelObj* aLevel = CurrentLevel();
		if (aLevel == nullptr)
			return "none";
		static const char* const aNames[] = { "intro", "settling", "playing", "stats_wait", "celebrate", "stats", "done",
			"game_over", "victory", "leaving" };
		int aState = aLevel->mState;
		return aState >= 0 && aState < 10 ? aNames[aState] : "unknown";
	}
	case SCREEN_MAP: {
		MetaGame* aMeta = aWidget->mMetaGame;
		if (aMeta == nullptr)
			return "none";
		switch (aMeta->mState) {
		case META_WAIT_START:
			return "wait_start";
		case META_INTRO:
			return "intro";
		case META_SLOTS_START:
			return "slots_start";
		case META_SLOTS:
			return "slots";
		case META_LIVES_START:
			return "lives_start";
		case META_LIVES:
			return "lives";
		case META_SHOP:
			return "shop";
		case META_MAP:
			return "world_map";
		case META_LEAVE:
			return "leave";
		case META_LEAVING:
			return "leaving";
		}
		return "unknown";
	}
	}
	return "none";
}

bool BotGameOver()
{
	LevelObj* aLevel = CurrentLevel();
	if (aLevel == nullptr)
		return false;
	if (aLevel->mState == LEVEL_STATE_GAME_OVER || aLevel->mState == LEVEL_STATE_VICTORY || aLevel->mVictoryShown)
		return true;
	if (aLevel->mState == LEVEL_STATE_LEAVING && !aLevel->mQuitConfirmed)
		return true;
	Player* aPlayer = Player::sCurrent;
	return aPlayer != nullptr && aPlayer->mInLevel && aPlayer->mLives < 1 && aPlayer->mLostLife;
}

std::string BotBugListJson()
{
	std::string aJson = "[";
	bool aFirst = true;
	for (int i = 0; i < BugObj::sCount; i++) {
		BugObj* aBug = BugObj::Get(i);
		if (aBug == nullptr)
			continue;
		float aX = aBug->mBody != nullptr ? aBug->mBody->mScreenX : aBug->mScreenX;
		float aY = aBug->mBody != nullptr ? aBug->mBody->mScreenY : aBug->mScreenY;
		float aW = aBug->mBody != nullptr ? aBug->mBody->mHitWidth : 0.0f;
		float aH = aBug->mBody != nullptr ? aBug->mBody->mHitHeight : 0.0f;
		aJson += std::string(aFirst ? "" : ",") + "[" + std::to_string(aBug->mKind) + "," + Num(aX) + "," + Num(aY) + "," +
			Num(aW) + "," + Num(aH) + "," + (aBug->mAlive ? "1" : "0") + "]";
		aFirst = false;
	}
	return aJson + "]";
}

std::string BotMenuJson()
{
	GameWidget* aWidget = TheWidget();
	if (aWidget == nullptr || aWidget->mScreen != SCREEN_MENU || aWidget->mMainMenu == nullptr)
		return "[]";
	std::string aJson = "[";
	for (int i = 0; i < MENU_BUTTONS; i++) {
		ObjBase* aButton = aWidget->mMainMenu->mButtons[i];
		if (aButton == nullptr)
			continue;
		int aX;
		int aY;
		DesignToWindow(aButton->mScreenX, aButton->mScreenY, &aX, &aY);
		aJson += std::string(i > 0 ? "," : "") + "[" + std::to_string(aButton->mButtonId) + "," + std::to_string(aX) + "," +
			std::to_string(aY) + "]";
	}
	return aJson + "]";
}

std::string BotMoveJson()
{
	GameWidget* aWidget = TheWidget();
	if (aWidget == nullptr)
		return "null";
	if (BotGameOver())
		return "null";
	switch (aWidget->mScreen) {
	case SCREEN_POPCAP:
	case SCREEN_LOADING:
	case SCREEN_TO_MENU:
	case SCREEN_TO_PLAY:
		return Wait(30, "wait");
	case SCREEN_MENU:
		return aWidget->mMainMenu != nullptr ? MenuMove(aWidget->mMainMenu) : Wait(10, "wait");
	case SCREEN_LEVEL: {
		LevelObj* aLevel = CurrentLevel();
		if (aLevel == nullptr || Player::sCurrent == nullptr)
			return Wait(10, "wait");
		return LevelMove(aLevel);
	}
	case SCREEN_MAP:
		return aWidget->mMetaGame != nullptr ? MetaMove(aWidget->mMetaGame) : Wait(10, "wait");
	}
	return Wait(10, "wait");
}

} // namespace Sexy
