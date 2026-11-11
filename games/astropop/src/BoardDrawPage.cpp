// Board: the page at the end of a game (the part of Draw at 0x461e50 that mEndPageShown turns on): the player's five best results of
// the mode, each with its pilot and a gauge, and under them the totals of the game that has ended.

#include "Board.h"
#include "GameText.h"
#include "AstroPopApp.h"
#include "Font.h"
#include "FrameAnim.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "SexyAppBase.h"
#include "UserProfile.h"

using namespace Sexy;

// a time in minutes and seconds with one of two formats: the first has a nought in front of the seconds and is used when they
// are under ten
// port: a helper of Board::DrawEndPage, which the original writes out twice
static std::string MinutesSeconds(const std::string& theFormatWithNought, const std::string& theFormat, int theSeconds)
{
	int aRest = theSeconds % 60;
	return StrFormat((aRest < 10) ? theFormatWithNought.c_str() : theFormat.c_str(), theSeconds / 60, aRest);
}

// the page itself
// port: a layer of Board::Draw (0x461e50)
void Board::DrawEndPage(Graphics* g)
{
	int aRowHeight = IMAGE_PERSONAL_VECTOR_GAUGE->mHeight + 2;

	// (the original reads the player's name here and does not use it)
	g->DrawImage(IMAGE_PERSONAL_HIGHSCORES, 0xa5, 3);
	g->SetFont(GetFontById(FONT_DIALOG_BOX_HEADER_ID));
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->SetFont(GetFontById(FONT_DIALOG_BOX_BTN_ID));

	// the five best results of the mode: the pilot, a gauge of how far the pilot has got and the score (or the time)
	ProfileRecord aRecord;
	for (int i = 0; i < 6; i++)
		aRecord.mField[i] = -1;
	int aY = 0x37;
	for (int aRow = 0; aRow < 5; aRow++)
	{
		if (mApp->mUser->GetRecord(mGameType, aRow, &aRecord))
		{
			int aPilot = aRecord.mField[1];
			int aScore = aRecord.mField[2];
			int aTime = aRecord.mField[4];
			int aLevel = aRecord.mField[5];

			// the result of the game that has ended is lit up
			if (aRow == mNewRecordRow)
			{
				g->SetColor(Color(0xff, 0xff, 0xff, mResultPulseAnim.GetCel() + 0x28));
				g->FillRect(0xb0, aY, 0x1a4, IMAGE_PERSONAL_VECTOR_GAUGE->mHeight);
				g->SetColorizeImages(true);
				g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
			}
			g->DrawImage(GetImageById(IMAGE_HOF_VECTOR_ID + aPilot), 0xba, aY + 1);

			// the gauge: a picture of the pilot's bar, filled in eighths as far as the pilot has got (the second half of
			// the picture is the full bar)
			int aBarWidth = IMAGE_CS_VECTOR_STATFILL->mWidth;
			int aBarX = 100;
			switch (aPilot)
			{
			case 0:
			case 2:
				aBarX = 100;
				break;
			case 1:
				aBarX = 0x65;
				break;
			case 3:
				aBarX = 0x62;
				break;
			}
			bool aFull = aLevel == 8;
			int aFilled = aBarWidth;
			if (aLevel < 8)
				aFilled = aLevel * (aBarWidth / 8);
			int aBarHeight = IMAGE_CS_VECTOR_STATFILL->mHeight / 2;
			g->DrawImage(GetImageById(IMAGE_PERSONAL_VECTOR_GAUGE_ID + aPilot), 0xe7, aY);
			g->DrawImage(GetImageById(IMAGE_CS_VECTOR_STATFILL_ID + aPilot), aBarX + 0xe7, aY + 2,
				Rect(0, (aFull ? 1 : 0) * aBarHeight, aFilled, aBarHeight));
			if (aRow == mNewRecordRow)
				g->SetColorizeImages(false);

			g->SetColor(Color(0xff, 0xff, 0, 0xff));
			if (mGameType == 1)
				WriteString(g, CommaSeperate(aScore), 0x1b2, aY + 0x18, 0x99, 1, true, 0, -1);
			else
			{
				WriteString(g, MinutesSeconds("%d:0%d", "%d:%d", aTime), 0x19c, aY + 0x18, 0x45, -1, true, 0, -1);
				g->SetColor(Color(0xff, 0xff, 0x1e, 0xff));
				WriteString(g, CommaSeperate(aScore), 0x1b1, aY + 0x18, 0x9a, 1, true, 0, -1);
			}
		}
		aY += aRowHeight;
	}

	// the totals of the game: each line has its name on the left and its value, in the other colour, on the right
	Font* aHeaderFont = GetFontById(FONT_DIALOG_BOX_HEADER_ID);
	g->SetFont(aHeaderFont);
	g->SetColor(Color(0xff, 0xff, 0xff));
	int aLineY = aHeaderFont->GetAscent() + 0x104;
	Font* aFont = GetFontById(FONT_DIALOG_BOX_BTN_ID);
	g->SetFont(aFont);

	if (mGameType == 1)
	{
		WriteString(g, GT(0x00565d24), 0xe8, aLineY, -1, -1, true, 0, -1);
		WriteString(g, StrFormat(GTC(0x00565d18), mLevelNumber), 0x21f, aLineY, -1, 1, true, 0, -1);
	}
	else
	{
		aLineY = aHeaderFont->GetAscent() + 0x10d;
		WriteString(g, GT(0x00565d04), 0xe8, aLineY, -1, -1, true, 0, -1);
		WriteString(g, MinutesSeconds(GT(0x00565cf4), GT(0x00565ce4), mPlayTime / 100), 0x21f, aLineY, -1, 1, true, 0, -1);
	}

	aLineY += aFont->GetAscent() * 2;
	WriteString(g, GT(0x00565cc4), 0xe8, aLineY, -1, -1, true, 0, -1);
	WriteString(g, StrFormat(GTC(0x00565cb8), CommaSeperate(mScore).c_str()), 0x21f, aLineY, -1, 1, true, 0, -1);

	aLineY += aFont->GetAscent() * 2;
	WriteString(g, GT(0x00565ca0), 0xe8, aLineY, -1, -1, true, 0, -1);
	WriteString(g, StrFormat(GTC(0x00565d18), mBestChain), 0x21f, aLineY, -1, 1, true, 0, -1);

	aLineY += aFont->GetAscent() * 2;
	if (mGameType == 1)
	{
		WriteString(g, GT(0x00565c84), 0xe8, aLineY, -1, -1, true, 0, -1);
		WriteString(g, MinutesSeconds(GT(0x00565cf4), GT(0x00565ce4), mPlayTime / 100), 0x21f, aLineY, -1, 1, true, 0, -1);
		aLineY += aFont->GetAscent();

		// the classic game goes on from its last checkpoint (the ones at levels four, eight and so on)
		Rect aRect(0xb0, aLineY + 0x18, 0x1a4, 100);
		if (mCheckpointLevel < 4)
			WriteWordWrapped(g, aRect, GT(0x00565c3c), -1, 0);
		else
			WriteWordWrapped(g, aRect, StrFormat(GTC(0x00565c64), mCheckpointLevel + 1), -1, 0);
	}
	else
	{
		// (the original asks first if the game is registered; the port's game is the whole game)
		UserProfile* aUser = mApp->mUser;
		Rect aRect(0xb0, aLineY + 0x1e, 0x1a4, 100);
		if (!aUser->HasModeUnlocked(2) || !aUser->HasModeUnlocked(3) || aUser->GetPilotLevel(0) < 8 ||
			aUser->GetPilotLevel(1) < 8 || aUser->GetPilotLevel(2) < 8 || aUser->GetPilotLevel(3) < 8)
			WriteWordWrapped(g, aRect, GT(0x00565bc0), -1, 0);
		else
			WriteWordWrapped(g, aRect, GT(0x00565c10), -1, 0);
	}
}
