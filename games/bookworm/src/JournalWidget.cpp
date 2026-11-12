// port(mod): mod "word-journal", the journal's page (JournalWidget.h). Not part of the original game.

#include "JournalWidget.h"
#include "Board.h"
#include "BookWormApp.h"
#include "Compat2004.h"
#include "PlayerFiles.h"
#include "Res.h"
#include "Font.h"
#include "ImageFont.h"
#include "Graphics.h"
#include "Image.h"
#include <algorithm>
#include <cmath>

using namespace Sexy;

// The level-up book's columns (panel coordinates, as LevelUpWidget::DrawToImage): left x 34, 206 wide; right
// x 256, 204 wide
static const int LEFT_X = 34;
static const int LEFT_W = 206;
static const int RIGHT_X = 256;
static const int RIGHT_W = 204;
static const int LEVEL_ROWS = 14;
static const int FOOTER_Y = 345;        // both pages' last line, above the book's lower edge
static const int MAP_TILE_SIZE = 26;
static const int MAP_TILE_GAP = 3;
static const int MAP_TILES_PER_ROW = 7;

JournalWidget::JournalWidget(BookWormApp* theApp)
{
	mApp = theApp;
	mHasJournal = WordJournal::Load(theApp, &mStats) && (mStats.mWords > 0);
	mFirstRow = 0;
	mClosed = false;
	mHasAlpha = true;
}

// theLabel in black and theValue in blue, centred together in [theX, theX + theWidth] at baseline theY
static void DrawLabelled(Graphics* g, const SexyString& theLabel, const SexyString& theValue, int theX, int theY, int theWidth)
{
	Font* aFont = g->GetFont();
	int aLabelWidth = aFont->StringWidth(theLabel);
	int aX = theX + (theWidth - aLabelWidth - aFont->StringWidth(theValue)) / 2;
	g->SetColor(Color(0, 0, 0));
	g->DrawString(theLabel, aX, theY);
	g->SetColor(Color(0, 0, 255));
	g->DrawString(theValue, aX + aLabelWidth, theY);
	g->SetColor(Color(0, 0, 0));
}

// A tile's heat, t from -1 (played far less than dealt) through 0 to 1 (far more): cool blue, the page's paper,
// warm red
static Color HeatColor(double t)
{
	static const int aCool[3] = { 96, 146, 222 };
	static const int aPaper[3] = { 242, 230, 198 };
	static const int aWarm[3] = { 226, 72, 40 };
	const int* anEnd = (t < 0.0) ? aCool : aWarm;
	double f = std::fabs(t);
	return Color((int)(aPaper[0] + (anEnd[0] - aPaper[0]) * f), (int)(aPaper[1] + (anEnd[1] - aPaper[1]) * f),
		(int)(aPaper[2] + (anEnd[2] - aPaper[2]) * f));
}

void JournalWidget::Draw(Graphics* g)
{
	g->SetColor(Color(0, 0, 0, 160));
	g->FillRect(0, 0, mWidth, mHeight);

	Image* aBook = mApp->mImages[IMAGE_DIALOG_LEVELUP];
	int aPanelX = (mWidth - aBook->GetWidth()) / 2;
	int aPanelY = (mHeight - aBook->GetHeight()) / 2;
	g->DrawImage(aBook, aPanelX, aPanelY);
	DrawLeftPage(g, aPanelX, aPanelY);
	DrawRightPage(g, aPanelX, aPanelY);
}

void JournalWidget::DrawLeftPage(Graphics* g, int thePanelX, int thePanelY)
{
	int aX = thePanelX + LEFT_X;
	g->SetColor(Color(0, 0, 0));
	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS18BOLD]);
	WriteString2004(g, "Word Journal", aX, thePanelY + 50, LEFT_W, 0, true, 0, -1);

	g->SetFont(mApp->mFonts[FONT_MYRIADWEB12BOLD]);
	int aLine = g->GetFont()->GetHeight() + 2;
	int aY = thePanelY + 76;
	if (!mHasJournal)
	{
		WriteString2004(g, "No words yet: every word", aX, aY, LEFT_W, 0, true, 0, -1);
		WriteString2004(g, "you play goes in here.", aX, aY + aLine, LEFT_W, 0, true, 0, -1);
		return;
	}

	WriteString2004(g, StrFormat("%s words in %s games", CommaSeperate(mStats.mWords).c_str(),
		CommaSeperate(mStats.mGames).c_str()), aX, aY, LEFT_W, 0, true, 0, -1);
	aY += aLine;
	WriteString2004(g, StrFormat("since %s %d", DateName(mStats.mFirstDate, false).c_str(), mStats.mFirstDate / 10000),
		aX, aY, LEFT_W, 0, true, 0, -1);
	aY += aLine;
	DrawLabelled(g, "Best: ", StrFormat("%s (%s)", mStats.mBest.mWord.c_str(), CommaSeperate(mStats.mBest.mScore).c_str()),
		aX, aY, LEFT_W);
	aY += aLine;
	DrawLabelled(g, "Longest: ", mStats.mLongest.mWord, aX, aY, LEFT_W);
	aY += aLine;
	double aLetters = (double)mStats.mTiles / mStats.mWords;
	int anAverage = (int)(mStats.mPoints / mStats.mWords);
	WriteString2004(g, StrFormat("Average: %.1f tiles, %s points", aLetters, CommaSeperate(anAverage).c_str()),
		aX, aY, LEFT_W, 0, true, 0, -1);

	// the heatmap: A..Z in rows of 7, the last row centred
	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS13BOLD]);
	int aMapY = thePanelY + 180;
	WriteString2004(g, "Tiles played", aX, aMapY - 8, LEFT_W, 0, true, 0, -1);
	Font* aLetterFont = mApp->mFonts[FONT_VERDANA12BOLD];
	for (int i = 0; i < 26; i++)
	{
		int aRow = i / MAP_TILES_PER_ROW;
		int aCol = i % MAP_TILES_PER_ROW;
		int aRowTiles = std::min(MAP_TILES_PER_ROW, 26 - aRow * MAP_TILES_PER_ROW);
		int aRowWidth = aRowTiles * MAP_TILE_SIZE + (aRowTiles - 1) * MAP_TILE_GAP;
		int aTileX = aX + (LEFT_W - aRowWidth) / 2 + aCol * (MAP_TILE_SIZE + MAP_TILE_GAP);
		int aTileY = aMapY + aRow * (MAP_TILE_SIZE + MAP_TILE_GAP);

		// the letter's share of the tiles played over its share of the letters dealt, on a log scale
		double aHeat = -1.0;
		if (mStats.mTileCounts[i] > 0)
		{
			double aRatio = ((double)mStats.mTileCounts[i] / mStats.mTiles) / (gLetterWeights[i] / 95500.0);
			aHeat = std::max(-1.0, std::min(1.0, std::log(aRatio) / std::log(2.0) / 1.5));
		}
		g->SetColor(HeatColor(aHeat));
		g->FillRect(aTileX, aTileY, MAP_TILE_SIZE, MAP_TILE_SIZE);
		g->SetColor(Color(70, 50, 30));
		g->DrawRect(aTileX, aTileY, MAP_TILE_SIZE - 1, MAP_TILE_SIZE - 1);

		g->SetFont(aLetterFont);
		g->SetColor(Color(0, 0, 0));
		SexyString aLetter = (i == 'Q' - 'A') ? SexyString("Qu") : SexyString(1, (char)('A' + i));
		WriteString2004(g, aLetter, aTileX, aTileY + (MAP_TILE_SIZE + aLetterFont->GetAscent()) / 2 - 1, MAP_TILE_SIZE, 0, true, 0, -1);
	}

	// the key: cool .. warm
	g->SetFont(mApp->mFonts[FONT_MYRIADWEB10]);
	int aKeyY = aMapY + 4 * (MAP_TILE_SIZE + MAP_TILE_GAP) + 12;
	g->SetColor(Color(0, 0, 0));
	WriteString2004(g, "Warm: played more than dealt", aX, aKeyY, LEFT_W, 0, true, 0, -1);
	WriteString2004(g, "Cool: played less", aX, aKeyY + g->GetFont()->GetHeight(), LEFT_W, 0, true, 0, -1);
}

void JournalWidget::DrawRightPage(Graphics* g, int thePanelX, int thePanelY)
{
	int aX = thePanelX + RIGHT_X;
	g->SetColor(Color(0, 0, 0));
	g->SetFont(mApp->mFonts[FONT_SCRIPPSCOLLEGEOS18BOLD]);
	WriteString2004(g, "Best by Level", aX, thePanelY + 50, RIGHT_W, 0, true, 0, -1);

	Font* aFont = mApp->mFonts[FONT_MYRIADWEB12BOLD];
	g->SetFont(aFont);
	int aLine = aFont->GetHeight() + 1;
	int aY = thePanelY + 76;
	int aRow = 0;
	int aCount = (int)mStats.mLevelBests.size();
	for (std::map<int, JournalWord>::const_iterator anIt = mStats.mLevelBests.begin(); anIt != mStats.mLevelBests.end();
		++anIt, ++aRow)
	{
		if (aRow < mFirstRow)
			continue;
		if (aRow >= mFirstRow + LEVEL_ROWS)
			break;
		const JournalWord& aWord = anIt->second;
		g->SetColor(Color(0, 0, 0));
		WriteString2004(g, StrFormat("%d", anIt->first), aX, aY, 24, 1, true, 0, -1);
		SexyString aPoints = CommaSeperate(aWord.mScore);
		WriteString2004(g, aPoints, aX + RIGHT_W - 60, aY, 60, 1, true, 0, -1);
		g->SetColor(Color(0, 0, 255));
		g->DrawString(aWord.mWord, aX + 34, aY);
		aY += aLine;
	}

	g->SetColor(Color(0, 0, 0));
	g->SetFont(mApp->mFonts[FONT_MYRIADWEB10]);
	int aFooterY = thePanelY + FOOTER_Y;
	if (aCount > LEVEL_ROWS)
	{
		int aLast = std::min(aCount, mFirstRow + LEVEL_ROWS);
		WriteString2004(g, StrFormat("Levels %d-%d of %d: click here for more", mFirstRow + 1, aLast, aCount),
			aX, aFooterY, RIGHT_W, 0, true, 0, -1);
	}
	WriteString2004(g, "Click anywhere else to close", thePanelX + LEFT_X, aFooterY, LEFT_W, 0, true, 0, -1);
}

void JournalWidget::MouseDown(int x, int y, int theClickCount)
{
	Image* aBook = mApp->mImages[IMAGE_DIALOG_LEVELUP];
	int aPanelX = (mWidth - aBook->GetWidth()) / 2;
	int aPanelY = (mHeight - aBook->GetHeight()) / 2;
	int aCount = (int)mStats.mLevelBests.size();
	bool onRightPage = (x >= aPanelX + RIGHT_X) && (x < aPanelX + RIGHT_X + RIGHT_W) && (y >= aPanelY) &&
		(y < aPanelY + aBook->GetHeight());
	if ((onRightPage) && (aCount > LEVEL_ROWS))
	{
		mFirstRow += LEVEL_ROWS;
		if (mFirstRow >= aCount)
			mFirstRow = 0;
		mApp->PlaySample(SOUND_BUTTONCLICK);
		MarkDirty();
		return;
	}
	mClosed = true;
	mApp->PlaySample(SOUND_BUTTONCLICK);
}
