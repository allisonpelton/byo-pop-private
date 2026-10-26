// port(variant): the game-over screen's Stats page (GameStatsPage.h). Not part of the original game.

#include "GameStatsPage.h"
#include "GameStats.h"
#include "HighScoreWidget.h"
#include "WinDMApp.h"
#include "Board.h"
#include "GameConfig.h"
#include "Res.h"
#include "Graphics.h"
#include "Font.h"
#include "ImageFont.h"
#include "Image.h"

#include <algorithm>
#include <vector>

using namespace Sexy;

namespace
{

// The game-over panel's colours (HighScoreWidget::Draw)
const Color kBack(21, 0, 56);
const Color kTitle(255, 0, 177);
const Color kLabel(252, 0, 195);
const Color kValue(255, 100, 220);
const Color kHeading(255, 193, 0);
const Color kGreen(0, 218, 0);
const Color kBar(46, 216, 0);
const Color kAxis(90, 60, 140);
const Color kNote(170, 150, 200);

const int kLeftX = 14;          // the left column, 14..210
const int kRightX = 224;        // the right column, 224..420
const int kColumnWidth = 196;
const int kFullWidth = 406;     // both columns
const int kFirstRow = 90;       // the columns' first baseline
const int kChartBottom = 292;   // the charts end here, the level's line and the BOARD rows stay below
const int kChartMin = 24;
const int kChartMax = 70;

struct Line
{
	SexyString	mLabel;
	SexyString	mValue;
};

SexyString Time(int theTicks)
{
	int aSeconds = theTicks / 100;
	return StrFormat(_S("%d:%02d"), aSeconds / 60, aSeconds % 60);
}

SexyString Count(int theCount)
{
	return StrFormat(_S("%d"), theCount);
}

SexyString CountAndPoints(int theCount, int thePoints)
{
	return (thePoints == 0) ? Count(theCount) : StrFormat(_S("%d  (%+d)"), theCount, thePoints);
}

// A value that does not fit beside its label goes under it, right-aligned
bool IsWrapped(Font* theFont, const Line& theLine)
{
	return theFont->StringWidth(theLine.mLabel) + 6 + theFont->StringWidth(theLine.mValue) > kColumnWidth;
}

int ColumnRows(Font* theFont, const std::vector<Line>& theLines)
{
	int aRows = 0;
	for (const Line& aLine : theLines)
		aRows += IsWrapped(theFont, aLine) ? 2 : 1;
	return aRows;
}

// Returns the baseline after the last row
int DrawColumn(Graphics* g, const std::vector<Line>& theLines, int theX, int theY, int theStep)
{
	int y = theY;
	for (const Line& aLine : theLines)
	{
		g->SetColor(kLabel);
		g->DrawString(aLine.mLabel, theX, y);
		if (IsWrapped(g->GetFont(), aLine))
			y += theStep;
		g->SetColor(kValue);
		g->DrawString(aLine.mValue, theX + kColumnWidth - g->GetFont()->StringWidth(aLine.mValue), y);
		y += theStep;
	}
	return y;
}

// The PLAY column: the rows a game without mods has, and the others only when the rules play them or the game did them
std::vector<Line> PlayLines(WinDMApp* theApp, Board* theBoard, const GameStats& theStats)
{
	std::vector<Line> aLines;
	aLines.push_back({theApp->GetString("STATS_MOVES", _S("Moves")), Count(theStats.mMoves)});
	aLines.push_back({theApp->GetString("STATS_SWAPS_BACK", _S("Swaps back")), Count(theStats.mSwapsBack)});
	if ((theStats.mInvalidKept > 0) || theBoard->mRules.mMoves.mAllowInvalid)
		aLines.push_back({theApp->GetString("STATS_INVALID", _S("Invalid moves kept")),
			CountAndPoints(theStats.mInvalidKept, -theStats.mInvalidPoints)});
	aLines.push_back({theApp->GetString("STATS_HINTS", _S("Hints")), CountAndPoints(theStats.mHints, -theStats.mHintPoints)});
	if ((theStats.mUndos > 0) || theBoard->mVariant.mUndo)
		aLines.push_back({theApp->GetString("STATS_UNDOS", _S("Undos")), CountAndPoints(theStats.mUndos, -theStats.mUndoPoints)});
	if (theStats.mTwists > 0)
		aLines.push_back({theApp->GetString("STATS_TWISTS", _S("Twists")), Count(theStats.mTwists)});
	if (theStats.mSlides > 0)
		aLines.push_back({theApp->GetString("STATS_SLIDES", _S("Slides")), Count(theStats.mSlides)});
	aLines.push_back({theApp->GetString("STATS_TIME", _S("Time played")), Time(theStats.mTicks)});
	SexyString aPerMove = (theStats.mMoves > 0) ? StrFormat(_S("%.1f s"), theStats.mTicks / 100.0 / theStats.mMoves) : _S("-");
	aLines.push_back({theApp->GetString("STATS_PER_MOVE", _S("Time per move")), aPerMove});
	return aLines;
}

std::vector<Line> MatchLines(WinDMApp* theApp, Board* theBoard, const GameStats& theStats)
{
	std::vector<Line> aLines;
	const char* const anIds[3] = {"STATS_RUNS_3", "STATS_RUNS_4", "STATS_RUNS_5"};
	const SexyChar* const aDefaults[3] = {_S("Runs of 3"), _S("Runs of 4"), _S("Runs of 5 or more")};
	for (int i = 0; i < 3; i++)
		aLines.push_back({theApp->GetString(anIds[i], aDefaults[i]), CountAndPoints(theStats.mRuns[i], theStats.mRunPoints[i])});
	if (theStats.mOtherPoints != 0)
		aLines.push_back({theApp->GetString("STATS_OTHER_POINTS", _S("Other points")), StrFormat(_S("%+d"), theStats.mOtherPoints)});
	// Moves by the sets of runs they cleared: 2, 3, and 4 or more are cascades
	int aCascades[3] = {};
	for (size_t i = 1; i < theStats.mCascades.size(); i++)
		aCascades[std::min((int) i - 1, 2)] += theStats.mCascades[i];
	aLines.push_back({theApp->GetString("STATS_CASCADES", _S("Cascades 2, 3, 4+")),
		StrFormat(_S("%d, %d, %d"), aCascades[0], aCascades[1], aCascades[2])});
	if ((theStats.mSpecialsMade > 0) || (theStats.mSpecialsSetOff > 0))
		aLines.push_back({theApp->GetString("STATS_SPECIALS", _S("Specials made, set off")),
			StrFormat(_S("%d, %d"), theStats.mSpecialsMade, theStats.mSpecialsSetOff)});
	if (theStats.mGoldCovered > 0)
		aLines.push_back({theApp->GetString("STATS_GOLD", _S("Gold covered")), Count(theStats.mGoldCovered)});
	// the board's own count (stage 18: saved with the arrows, the bonus's factor), so the whole game's
	if (theBoard->mVariant.mArrows)
		aLines.push_back({theApp->GetString("STATS_ARROWS", _S("Arrows cleared")), Count(theBoard->mArrowsCleared)});
	return aLines;
}

}

GameStatsPage::GameStatsPage(WinDMApp* theApp, std::function<void()> theOnClose)
{
	mApp = theApp;
	mOnClose = theOnClose;
	mHoverLevel = -1;
	mChartTop = kChartBottom - kChartMax;
	mChartBottom = kChartBottom;
}

void GameStatsPage::MouseDown(int x, int y, int theClickCount)
{
	(void) x;
	(void) y;
	(void) theClickCount;
	mOnClose();
}

void GameStatsPage::MouseMove(int x, int y)
{
	Board* aBoard = mApp->mBoard;
	int aLevel = ((aBoard != NULL) && (aBoard->mStats != NULL)) ? BarAt(x, y, *aBoard->mStats) : -1;
	if (aLevel != mHoverLevel)
	{
		mHoverLevel = aLevel;
		MarkDirty();
	}
}

void GameStatsPage::MouseLeave()
{
	if (mHoverLevel != -1)
	{
		mHoverLevel = -1;
		MarkDirty();
	}
}

// The bar chart fills the left column: one bar per level, as wide as they fit
int GameStatsPage::BarAt(int x, int y, const GameStats& theStats)
{
	int aCount = (int) theStats.mLevels.size();
	if ((aCount == 0) || (x < kLeftX) || (x >= kLeftX + kColumnWidth) || (y < mChartTop) || (y > mChartBottom))
		return -1;
	return std::min(aCount - 1, (x - kLeftX) * aCount / kColumnWidth);
}

void GameStatsPage::Draw(Graphics* g)
{
	g->SetColor(kBack);
	g->FillRect(0, 0, mWidth, mHeight);
	Board* aBoard = mApp->mBoard;
	if ((aBoard != NULL) && (aBoard->mStats != NULL))
	{
		const GameStats& aStats = *aBoard->mStats;
		g->SetFont(mApp->mHeaderFont);
		g->SetColor(kTitle);
		HighScoreWidget::DrawCenteredString(g, mApp->GetString("STATS_TITLE", _S("GAME STATS")), PAGE_WIDTH / 2, 38);
		g->SetFont(mApp->mMainFont);
		if (aStats.mFromResume)
		{
			g->SetColor(kNote);
			HighScoreWidget::DrawCenteredString(g, mApp->GetString("STATS_FROM_RESUME", _S("Counted from the resume on")),
				PAGE_WIDTH / 2, 54);
		}

		// Two columns, closer rows when either is long; then the gems across both, and the charts in what is left
		std::vector<Line> aPlay = PlayLines(mApp, aBoard, aStats);
		std::vector<Line> aMatches = MatchLines(mApp, aBoard, aStats);
		int aRows = std::max(ColumnRows(mApp->mMainFont, aPlay), ColumnRows(mApp->mMainFont, aMatches));
		int aStep = (aRows > 7) ? 13 : 15;
		g->SetColor(kHeading);
		HighScoreWidget::DrawHeading(g, mApp->GetString("STATS_PLAY", _S("PLAY")), kLeftX + kColumnWidth / 2, 72);
		HighScoreWidget::DrawHeading(g, mApp->GetString("STATS_MATCHES", _S("MATCHES")), kRightX + kColumnWidth / 2, 72);
		int y = std::max(DrawColumn(g, aPlay, kLeftX, kFirstRow, aStep), DrawColumn(g, aMatches, kRightX, kFirstRow, aStep));
		y = DrawGems(g, aStats, y);
		DrawLevels(g, aStats, y);
		DrawBoard(g);
	}
	g->SetFont(mApp->mMainFont);
	g->SetColor(kGreen);
	HighScoreWidget::DrawCenteredString(g, mApp->GetString("STATS_RETURN", _S("CLICK TO RETURN")), PAGE_WIDTH / 2, 412);

	// The board frame over the page's edge, as the game-over screen draws it over its panel
	g->DrawImage(mApp->mImages[IMAGE_ALPHABACK], -PAGE_X, -PAGE_Y);
}

// The gems cleared per colour across both columns: the gem sheet's rest pose, small, with its count under it. theY is
// the baseline after the columns; returns the baseline of the counts.
int GameStatsPage::DrawGems(Graphics* g, const GameStats& theStats, int theY)
{
	const int anIcon = 20;
	int aColors = std::min(std::max(gGameConfig.mGemColors, 1), (int) GameStats::MAX_COLORS);
	int aCell = std::min(kFullWidth / aColors, 56);
	int aLeft = kLeftX + (kFullWidth - aCell * aColors) / 2;
	int aTop = theY - 6;
	g->SetColor(kValue);
	for (int c = 0; c < aColors; c++)
	{
		int aCx = aLeft + c * aCell + aCell / 2;
		g->DrawImage(mApp->mImages[IMAGE_GEMSHEET], Rect(aCx - anIcon / 2, aTop, anIcon, anIcon), Rect(0, c * 104, 52, 52));
		HighScoreWidget::DrawCenteredString(g, Count(theStats.mGems[c]), aCx, aTop + anIcon + 12);
	}
	return aTop + anIcon + 12;
}

// theY is the baseline of the gems' counts; the charts end at kChartBottom, as tall as fits up to kChartMax
void GameStatsPage::DrawLevels(Graphics* g, const GameStats& theStats, int theY)
{
	int aHeadingY = std::max(theY + 20, kChartBottom - kChartMax - 10);
	g->SetColor(kHeading);
	HighScoreWidget::DrawHeading(g, mApp->GetString("STATS_LEVELS", _S("LEVELS")), PAGE_WIDTH / 2, aHeadingY);
	mChartTop = std::min(aHeadingY + 10, kChartBottom - kChartMin);
	mChartBottom = kChartBottom;
	int aHeight = mChartBottom - mChartTop;
	int aBottom = mChartBottom;
	g->SetColor(kAxis);
	g->FillRect(kLeftX, aBottom, kColumnWidth, 1);
	g->FillRect(kRightX, aBottom, kColumnWidth, 1);
	g->FillRect(kRightX, mChartTop, 1, aHeight);

	// Points per level, as bars
	int aCount = (int) theStats.mLevels.size();
	int aMaxPoints = 1;
	for (const GameStats::Level& aLevel : theStats.mLevels)
		aMaxPoints = std::max(aMaxPoints, aLevel.mPoints);
	for (int i = 0; i < aCount; i++)
	{
		int x0 = kLeftX + i * kColumnWidth / aCount;
		int x1 = kLeftX + (i + 1) * kColumnWidth / aCount;
		int aBarHeight = (int) ((double) std::max(theStats.mLevels[i].mPoints, 0) * (aHeight - 2) / aMaxPoints);
		g->SetColor((i == mHoverLevel) ? Color::White : kBar);
		g->FillRect(x0, aBottom - aBarHeight, std::max(1, x1 - x0 - ((x1 - x0 > 3) ? 1 : 0)), aBarHeight);
	}

	// The score over the play time, with the level-ups marked
	const std::vector<GameStats::Point>& aLine = theStats.mTimeline;
	int anEndTick = std::max(1, std::max(theStats.mTicks, aLine.empty() ? 0 : aLine.back().mTick));
	int aMaxScore = 1;
	for (const GameStats::Point& aPoint : aLine)
		aMaxScore = std::max(aMaxScore, aPoint.mScore);
	g->SetColor(kHeading);
	for (int aTick : theStats.mLevelUpTicks)
	{
		int x = kRightX + (int) ((double) aTick * (kColumnWidth - 1) / anEndTick);
		for (int y = mChartTop; y < aBottom; y += 4)
			g->FillRect(x, y, 1, 2);
	}
	g->SetColor(kValue);
	for (size_t i = 1; i < aLine.size(); i++)
	{
		int x0 = kRightX + (int) ((double) aLine[i - 1].mTick * (kColumnWidth - 1) / anEndTick);
		int x1 = kRightX + (int) ((double) aLine[i].mTick * (kColumnWidth - 1) / anEndTick);
		int y0 = aBottom - (int) ((double) std::max(aLine[i - 1].mScore, 0) * (aHeight - 2) / aMaxScore);
		int y1 = aBottom - (int) ((double) std::max(aLine[i].mScore, 0) * (aHeight - 2) / aMaxScore);
		g->DrawLine(x0, y0, x1, y1);
	}

	// One level in words: the one under the mouse, else the last
	if (aCount > 0)
	{
		int anIndex = (mHoverLevel >= 0) ? mHoverLevel : aCount - 1;
		const GameStats::Level& aLevel = theStats.mLevels[anIndex];
		SexyString aText = StrFormat(_S("%s %d:  %d %s, %s, %d %s"), mApp->GetString("STATS_LEVEL", _S("Level")).c_str(),
			aLevel.mLevel + 1, aLevel.mPoints, mApp->GetString("STATS_POINTS", _S("points")).c_str(), Time(aLevel.mTicks).c_str(),
			aLevel.mMoves, mApp->GetString("STATS_LEVEL_MOVES", _S("moves")).c_str());
		g->SetColor((mHoverLevel >= 0) ? Color::White : kValue);
		HighScoreWidget::DrawCenteredString(g, aText, PAGE_WIDTH / 2, aBottom + 18);
	}
}

void GameStatsPage::DrawBoard(Graphics* g)
{
	Board* aBoard = mApp->mBoard;
	g->SetColor(kHeading);
	HighScoreWidget::DrawHeading(g, mApp->GetString("STATS_BOARD", _S("BOARD")), PAGE_WIDTH / 2, 334);
	// Rules with a gameplay change have their own score tables (GameConfig::ScopedFileName)
	bool isOriginal = GameConfig::ScopedFileName("x.dat") == "x.dat";
	SexyString aRules = isOriginal ? mApp->GetString("STATS_RULES_ORIGINAL", _S("the original")) :
		StringToSexyStringFast(aBoard->mRules.RulesIdHex().substr(0, 8));
	SexyString aMode = aBoard->mTimedMode ? mApp->GetString("STATS_TIMED", _S("time trial")) : mApp->GetString("STATS_NORMAL", _S("normal"));
	std::vector<Line> aLeft = {{mApp->GetString("STATS_RULES", _S("Rules")), aRules},
		{mApp->GetString("STATS_SIZE", _S("Board")), StrFormat(_S("%d x %d"), aBoard->mCols, aBoard->mRows)}};
	std::vector<Line> aRight = {{mApp->GetString("STATS_MODE", _S("Game")), aMode}};
	// A replay's board always owns an MTRand; its seed is 0 for a game that played on the global stream (as the game had)
	if ((aBoard->mOwnRng != NULL) && (!aBoard->mPlaybackOnly || (aBoard->mGameSeed != 0)))
		aRight.push_back({mApp->GetString("STATS_SEED", _S("Seed")), StrFormat(_S("%u"), (unsigned) aBoard->mGameSeed)});
	DrawColumn(g, aLeft, kLeftX, 352, 15);
	DrawColumn(g, aRight, kRightX, 352, 15);
}
