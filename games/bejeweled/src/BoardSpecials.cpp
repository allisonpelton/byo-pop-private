// port(variant): flame and lightning gems on a board (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.8, stage 11). Not
// part of the original game. ProcessMatchesVariant scores the groups MatchFinder found and marks their gems dying;
// ResolveSpecials then keeps one gem of each group a special's createdBy names alive as that special and sets off
// the specials the groups took (Specials.h decides which cells; this file scores and draws them). Everything
// resolves within the tick, so UpdateClearing shrinks the blasts' gems with the groups'. A blast is one scoring
// group: its special's points for each gem it took, times the multiplier, towards the move's score and EXCELLENT
// but not the set value's count (mMoveMatchCount). The overlays and the blast effects are cosmetic: they read no
// RNG, are not saved, and only the pieces' kinds are play state (hashed, saved in a KIND chunk, snapshotted).

#include "Board.h"
#include "WinDMApp.h"
#include "GameConfig.h"
#include "Res.h"
#include "SoundManager.h"
#include "Graphics.h"
#include "DDImage.h"

#include <algorithm>

using namespace Sexy;

namespace
{

enum
{
	BLAST_TICKS = 36,           // how long a blast's effect shows
	FLAME_PULSE_TICKS = 40      // a flame gem's glow: one pulse
};

const Color kFlameColor(255, 140, 40);
const Color kLightningColor(150, 200, 255);

}

void Board::MovedCells(std::vector<MatchFinder::Cell>* theCells)
{
	theCells->clear();
	if (IsVariantMoveState(mState))
	{
		MoveSet::Cell aFrom[MoveSet::MAX_MOVED];
		MoveSet::Cell aTo[MoveSet::MAX_MOVED];
		int aCount = MoveSet::MovedCells(mMoveInPlay, aFrom, aTo);
		for (int i = 0; i < aCount; i++)
			theCells->push_back(aTo[i]);
	}
	else if (((mState == STATE_SWAPPING) || (mState == STATE_SWAPPING_BACK)) && (mSwapPiece1 != NULL) && (mSwapPiece2 != NULL))
	{
		theCells->push_back(MatchFinder::Cell(mSwapPiece1->mCol, mSwapPiece1->mRow));
		theCells->push_back(MatchFinder::Cell(mSwapPiece2->mCol, mSwapPiece2->mRow));
	}
}

void Board::ResolveSpecials(const std::vector<MatchFinder::Group>& theGroups)
{
	Specials::Grid aGrid;
	aGrid.mCols = mCols;
	aGrid.mRows = mRows;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			aGrid.mGem[aRow][aCol] = (aPiece != NULL);
			aGrid.mKind[aRow][aCol] = (aPiece != NULL) ? aPiece->mKind : (int) Specials::KIND_NONE;
			aGrid.mDying[aRow][aCol] = (aPiece != NULL) && aPiece->mDying;
		}
	}

	// The move's own resolve is the first after it (mCascadeCount 1); specials.createIn "move" makes them only there
	std::vector<Specials::Creation> aCreations;
	if ((mCascadeCount == 1) || !mRules.mSpecials.mCreateInMoveOnly)
	{
		std::vector<MatchFinder::Cell> aMoved;
		if (mCascadeCount == 1)
			MovedCells(&aMoved);
		Specials::Creations(theGroups, mMatchSpec, mRules.mSpecials, aMoved, aGrid, &aCreations);
	}
	for (const Specials::Creation& c : aCreations)
	{
		Piece* aPiece = mBoard[c.mCell.second][c.mCell.first];
		aPiece->mDying = false;
		aPiece->mDyingFrame = 0;
		ClearArrow(aPiece); // stage 18: an arrow that turns special is cleared (BoardArrows.cpp)
		aPiece->mKind = c.mKind;
		aPiece->mKindFrame = 0;
		aGrid.mDying[c.mCell.second][c.mCell.first] = false;
		mSpecialsMade[c.mKind]++;
		if (mStats != NULL) mStats->OnSpecialMade();
	}

	std::vector<Specials::Blast> aBlasts;
	Specials::Blasts(theGroups, mLayout, mRules.mSpecials, aCreations, &aGrid, &aBlasts);
	bool isSounded[3] = {};
	for (const Specials::Blast& aBlast : aBlasts)
	{
		ScoreBlast(aBlast);
		if (!isSounded[aBlast.mKind])
		{
			isSounded[aBlast.mKind] = true;
			PlaySpecialSound(aBlast.mKind, GetPanForX(GetColX(aBlast.mCell.first) + 24));
		}
	}
	if (!aCreations.empty() || !aBlasts.empty())
		MarkDirty();
}

void Board::ScoreBlast(const Specials::Blast& theBlast)
{
	for (const Specials::Cell& c : theBlast.mDestroyed)
	{
		Piece* aPiece = mBoard[c.second][c.first];
		aPiece->mDyingFrame = 0;
		aPiece->mDying = true;
	}
	mBlastCount++;
	if (mStats != NULL) mStats->OnSpecialSetOff();

	int aCount = (int) theBlast.mDestroyed.size();
	const SpecialRule& aRule = (theBlast.mKind == Specials::KIND_FLAME) ? mRules.mSpecials.mFlame : mRules.mSpecials.mLightning;
	int aPoints = (int)((double) aRule.mPoints * aCount * mPointMultiplier);
	if (aCount > 0)
	{
		ScorePopup aPopup = {GetColX(theBlast.mCell.first) + 24, GetRowY(theBlast.mCell.second) + 16, 110, -1, aPoints};
		mScorePopups.push_back(aPopup);
		mGemsCleared += aCount;
		mScore += aPoints;
		if (mStats != NULL) mStats->OnPoints(aPoints);
		AddMoveScore(aPoints);
	}
	// The cells of the gems it took turn gold with gold.gild "matchesAndBlasts" (4.9)
	if (mVariant.mGold && (mRules.mGold.mGild != GameRules::GILD_MATCHES))
		GildCells(theBlast.mDestroyed);
	// Dirt in the blast's reach loses a layer (4.14), once for the blast
	if (mVariant.mDirt)
		ClearDirtCells(theBlast.mArea);

	BlastEffect anEffect = {theBlast.mKind, theBlast.mCell.first, theBlast.mCell.second, 0};
	mBlastEffects.push_back(anEffect);
}

void Board::PlaySpecialSound(int theKind, int thePan)
{
	// A mod's sounds/flame and sounds/lightning, looked up once
	static int aModSounds[3] = {-2, -2, -2};
	if (aModSounds[theKind] == -2)
		aModSounds[theKind] = (mApp->mSoundManager != NULL) ? mApp->mSoundManager->LoadSound((theKind == Specials::KIND_FLAME) ? "sounds\\flame" : "sounds\\lightning") : -1;
	if (aModSounds[theKind] >= 0)
		mApp->PlaySample(aModSounds[theKind], thePan);
	else
		mApp->PlaySample((theKind == Specials::KIND_FLAME) ? SOUND_EXPLODE : SOUND_FIRECRACKLE, thePan);
}

void Board::UpdateSpecials()
{
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if ((aPiece != NULL) && (aPiece->mKind != Specials::KIND_NONE))
			{
				aPiece->mKindFrame++;
				MarkDirty();
			}
		}
	}
	for (BlastEffect& anEffect : mBlastEffects)
		anEffect.mTick++;
	size_t aBefore = mBlastEffects.size();
	mBlastEffects.erase(std::remove_if(mBlastEffects.begin(), mBlastEffects.end(), [](const BlastEffect& e) { return e.mTick >= BLAST_TICKS; }),
		mBlastEffects.end());
	if (!mBlastEffects.empty() || (mBlastEffects.size() != aBefore))
		MarkDirty();
}

// Under (with the gems, clipped to the board): each special's overlay, a mod's images/flamegem or
// images/lightninggem strip (additive, one 52 px frame per 2 ticks), else a flame gem's pulsing glow of hint
// sparkles in fire colours and a lightning gem's level-wipe strip running across it. Over: the blasts, a flame's
// sparkles over the cells it took and lightning's strip along its row and column, fading.
void Board::DrawSpecials(Graphics* g, bool isOver)
{
	static bool isLookedUp = false;
	static Image* aModArt[3] = {};
	if (!isLookedUp)
	{
		isLookedUp = true;
		aModArt[Specials::KIND_FLAME] = mApp->GetImage("images\\flamegem", true);
		aModArt[Specials::KIND_LIGHTNING] = mApp->GetImage("images\\lightninggem", true);
	}
	Image* aSparkle = mApp->mImages[IMAGE_SPARKLE];
	Image* anElect = mApp->mImages[IMAGE_ELECT];

	if (!isOver)
	{
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if ((aPiece == NULL) || (aPiece->mKind == Specials::KIND_NONE) || aPiece->mDying || (mVariant.mMoves && DrawsSlideCell(aCol, aRow)))
					continue;
				int x = (int) aPiece->mX;
				int y = (int) aPiece->mY;
				Image* anArt = aModArt[aPiece->mKind];
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				if (anArt != NULL)
				{
					int aFrames = std::max(1, anArt->GetWidth() / GEM_SIZE);
					g->DrawImage(anArt, x, y, Rect((aPiece->mKindFrame / 2 % aFrames) * GEM_SIZE, 0, GEM_SIZE, std::min((int) GEM_SIZE, anArt->GetHeight())));
				}
				else if (aPiece->mKind == Specials::KIND_FLAME)
				{
					int aPulse = aPiece->mKindFrame % FLAME_PULSE_TICKS;
					int anAlpha = 140 + 115 * ((aPulse < FLAME_PULSE_TICKS / 2) ? aPulse : FLAME_PULSE_TICKS - aPulse) / (FLAME_PULSE_TICKS / 2);
					g->SetColorizeImages(true);
					g->SetColor(Color(kFlameColor.mRed, kFlameColor.mGreen, kFlameColor.mBlue, anAlpha));
					g->DrawImage(aSparkle, x + 6, y + 6, Rect((aPiece->mKindFrame / 3 % 14) * 40, 0, 40, 40));
					g->DrawImage(aSparkle, x - 4, y + 12, Rect(((aPiece->mKindFrame / 3 + 7) % 14) * 40, 0, 40, 40));
					g->SetColorizeImages(false);
				}
				else
				{
					Graphics aGemG(*g);
					aGemG.ClipRect(x + 4, y + 4, GEM_SIZE - 8, GEM_SIZE - 8);
					aGemG.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
					int anOffset = (aPiece->mKindFrame * 3) % 128;
					aGemG.DrawImage(anElect, x - anOffset, y + 9);
					aGemG.DrawImage(anElect, x + 128 - anOffset, y + 9);
					aGemG.DrawImage(anElect, x - 64 + anOffset, y + 26);
					aGemG.DrawImage(anElect, x - 192 + anOffset, y + 26);
				}
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			}
		}
		return;
	}

	for (const BlastEffect& anEffect : mBlastEffects)
	{
		int anAlpha = 255 * (BLAST_TICKS - anEffect.mTick) / BLAST_TICKS;
		int aCenterX = GetColX(anEffect.mCol) + 26;
		int aCenterY = GetRowY(anEffect.mRow) + 26;
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		if (anEffect.mKind == Specials::KIND_FLAME)
		{
			int aRadius = mRules.mSpecials.mFlame.mRadius;
			g->SetColor(Color(kFlameColor.mRed, kFlameColor.mGreen, kFlameColor.mBlue, anAlpha));
			for (int dr = -aRadius; dr <= aRadius; dr++)
			{
				for (int dc = -aRadius; dc <= aRadius; dc++)
				{
					int aCol = anEffect.mCol + dc;
					int aRow = anEffect.mRow + dr;
					if (!mLayout.IsPlayable(aCol, aRow) && !mLayout.IsDirt(aCol, aRow))
						continue;
					int aFrame = (anEffect.mTick / 2 + (dc + dr + 2 * aRadius)) % 14;
					g->DrawImage(aSparkle, GetColX(aCol) + 6, GetRowY(aRow) + 6, Rect(aFrame * 40, 0, 40, 40));
				}
			}
		}
		else
		{
			g->SetColor(Color(kLightningColor.mRed, kLightningColor.mGreen, kLightningColor.mBlue, anAlpha));
			int aLeft = GetColX(0);
			int aRight = GetColX(mCols - 1) + CELL_SIZE;
			int aTop = GetRowY(0);
			int aBottom = GetRowY(mRows - 1) + CELL_SIZE;
			int anOffset = (anEffect.mTick * 8) % 128;
			for (int x = aLeft - anOffset; x < aRight; x += 128)
				g->DrawImage(anElect, x, aCenterY - 8);
			for (int y = aTop - anOffset; y < aBottom; y += 128)
				g->DrawImageRotated(anElect, aCenterX - 64, y + 56, 3.14159265358979 / 2, 64, 8);
		}
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}
