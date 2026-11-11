// port(variant): the Jewel Quest gold board (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.9). Not part of
// the original game. Every gem cell a scored group takes turns gold (a blast's too, with gold.gild
// "matchesAndBlasts"); the gold is the cell's, so it stays when the gems fall. A level ends when every cell of the goal
// is gold: the gem cells, and the dirt cells with gold.countDirt (a dirt cell turns gold only once its last layer is
// gone and a group takes the gem there). The score's level end and TIME_UP are skipped and the timed drain does not
// run; instead the level may have a time limit (gold.timeLimit of the mode: base + perLevel x level seconds, at least
// min), which runs in the drain's states once the GO delay is over. The gold done in time scores timeBonus a second
// left, times the multiplier, and the vortex takes the board to the next level; time running out is TIME_UP and the
// game over. The gold layer and the time are play state (hashed, saved in GOLD and GTIM, undone); the gold tiles,
// their shine and the HUD are cosmetic.

#include "Board.h"
#include "WinDMApp.h"
#include "Res.h"
#include "Graphics.h"
#include "DDImage.h"
#include "MemoryImage.h"
#include "ImageFont.h"

#include <algorithm>

using namespace Sexy;

namespace
{

enum
{
	GOLD_SHINE_TICKS = 30       // how long a cell shines when it turns gold
};

const Color kGoldColor(235, 185, 45);

// A mod's gold tile (images/goldtile), looked up once; NULL without one
Image* ModGoldTile(WinDMApp* theApp)
{
	static bool isLookedUp = false;
	static Image* aGoldTile = NULL;
	if (!isLookedUp)
	{
		isLookedUp = true;
		aGoldTile = theApp->GetImage("images\\goldtile", true);
	}
	return aGoldTile;
}

}

bool Board::CountsForGold(int theCol, int theRow) const
{
	return mLayout.IsPlayable(theCol, theRow) || (mRules.mGold.mCountDirt && mLayout.IsDirt(theCol, theRow));
}

int Board::GoldGoal() const
{
	int aGoal = 0;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
			aGoal += CountsForGold(aCol, aRow) ? 1 : 0;
	}
	return aGoal;
}

int Board::GoldLeft() const
{
	int aLeft = 0;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
			aLeft += (CountsForGold(aCol, aRow) && !mGold[aRow][aCol]) ? 1 : 0;
	}
	return aLeft;
}

void Board::GildCells(const std::vector<MatchFinder::Cell>& theCells)
{
	for (const MatchFinder::Cell& aCell : theCells)
	{
		int aCol = aCell.first;
		int aRow = aCell.second;
		if (!mLayout.IsPlayable(aCol, aRow) || mGold[aRow][aCol])
			continue;
		mGold[aRow][aCol] = 1;
		if (mStats != NULL) mStats->OnGoldCovered(1);
		GoldShine aShine = {aCol, aRow, 0};
		mGoldShines.push_back(aShine);
		MarkDirty();
	}
}

int Board::GoldLimitTicks() const
{
	const TimeLimit& aLimit = mTimedMode ? mRules.mGold.mTimeLimitTimed : mRules.mGold.mTimeLimitNormal;
	if (!aLimit.mEnabled)
		return 0;
	return std::max(aLimit.mMin, aLimit.mBase + aLimit.mPerLevel * mLevel) * 100;
}

// SetupLevelData (0x4cae29), at the start of each level: no gold yet, the level's whole time
void Board::SetupLevelDataGold()
{
	memset(mGold, 0, sizeof(mGold));
	mGoldShines.clear();
	mGoldTickLimit = GoldLimitTicks();
	mGoldTicksLeft = mGoldTickLimit;
}

// Update's level block (0x4cdabc) on a gold board, in place of the score's level end and TIME_UP
void Board::UpdateGoldGoal()
{
	if (GoldTimed() && (mTimeDrainDelay == 0) && (mGoldTicksLeft > 0) && ((mState == STATE_PLAYING) || (mState == STATE_SWAPPING) ||
		(mState == STATE_TWISTING) || (mState == STATE_SLIDING)))
		mGoldTicksLeft--;
	if (mState != STATE_PLAYING)
		return;
	if (GoldLeft() == 0)
	{
		if (GoldTimed())
		{
			int aBonus = (int)((double) mRules.mGold.mTimeBonus * (mGoldTicksLeft / 100) * mPointMultiplier);
			mScore += aBonus;
			if (mStats != NULL) mStats->OnPoints(aBonus);
		}
		DoWhirlpool();
	}
	else if (GoldTimed() && (mGoldTicksLeft == 0))
	{
		DoBouncyText(mApp->GetString("TIME_UP"), 400);
		GameOver(200);
	}
}

// The bar: the time left of a level with a limit, else the gold so far
int Board::GoldBarTarget() const
{
	if (GoldTimed())
		return (int)((int64_t) BAR_WIDTH * mGoldTicksLeft / mGoldTickLimit);
	int aGoal = GoldGoal();
	if (aGoal == 0)
		return BAR_WIDTH;
	return (int)((int64_t) BAR_WIDTH * (aGoal - GoldLeft()) / aGoal);
}

void Board::UpdateGold()
{
	if (mGoldShines.empty())
		return;
	for (GoldShine& aShine : mGoldShines)
		aShine.mTick++;
	mGoldShines.erase(std::remove_if(mGoldShines.begin(), mGoldShines.end(), [](const GoldShine& s) { return s.mTick >= GOLD_SHINE_TICKS; }),
		mGoldShines.end());
	MarkDirty();
}

// Under the gems: each gold cell, the level's checker tile A or B tinted gold (a mod's images/goldtile in its place,
// when there is one), made once per level. Over them: a cell gilded lately shines, the sparkle sheet in gold, fading
void Board::DrawGold(Graphics* g, bool isOver)
{
	if (isOver)
	{
		Image* aSparkle = mApp->mImages[IMAGE_SPARKLE];
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		for (const GoldShine& aShine : mGoldShines)
		{
			int anAlpha = 255 * (GOLD_SHINE_TICKS - aShine.mTick) / GOLD_SHINE_TICKS;
			g->SetColor(Color(kGoldColor.mRed, kGoldColor.mGreen, kGoldColor.mBlue, anAlpha));
			g->DrawImage(aSparkle, GetColX(aShine.mCol) + 6, GetRowY(aShine.mRow) + 6, Rect((aShine.mTick / 2 % 14) * 40, 0, 40, 40));
		}
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		return;
	}

	Image* aModTile = ModGoldTile(mApp);
	if ((aModTile == NULL) && (mGoldTilesLevel != mLevel))
	{
		for (int i = 0; i < 2; i++)
		{
			delete mGoldTiles[i];
			MemoryImage aTexture(mApp);
			aTexture.Create(CELL_SIZE, CELL_SIZE);
			Graphics aTextureG(&aTexture);
			aTextureG.DrawImage(mApp->mImages[IMAGE_CHECKERTEXTURES], 0, 0, Rect((mLevel % 5) * 102 + i * CELL_SIZE, 0, CELL_SIZE, CELL_SIZE));
			mGoldTiles[i] = mApp->CreateColorizedImage(&aTexture, kGoldColor);
		}
		mGoldTilesLevel = mLevel;
	}
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			if (!mGold[aRow][aCol])
				continue;
			Image* aTile = (aModTile != NULL) ? aModTile : mGoldTiles[(aRow + aCol) % 2];
			g->DrawImage(aTile, GetColX(aCol) - 1, GetRowY(aRow), Rect(0, 0, std::min((int) CELL_SIZE, aTile->GetWidth()),
				std::min((int) CELL_SIZE, aTile->GetHeight())));
		}
	}
}

// GOLD n/m in the main font, centred over the bar (237, 444, 340 x 25); GOLD_PROGRESS is a format of the two numbers
void Board::DrawGoldHud(Graphics* g)
{
	int aGoal = GoldGoal();
	SexyString aText = StrFormat(mApp->GetString("GOLD_PROGRESS", _S("GOLD %d/%d")).c_str(), aGoal - GoldLeft(), aGoal);
	g->SetFont(mApp->mMainFont);
	g->SetColor(Color::White);
	g->DrawString(aText, 237 + BAR_WIDTH / 2 - mApp->mMainFont->StringWidth(aText) / 2, 444 + 12 + mApp->mMainFont->GetAscent() / 2);
}
