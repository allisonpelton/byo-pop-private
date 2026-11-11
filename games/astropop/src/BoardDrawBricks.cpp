// Board: the sub-draws of the bricks (0x438e80) and of the held pieces and the introduction of a bonus round (0x443fd0).

#include "Board.h"
#include "AstroPopApp.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "SexyAppBase.h"
#include "UserProfile.h"

using namespace Sexy;

// 0x438e80: every brick: the field row by row, the groups that fly, the bricks of ClearField, the groups that fall and the
// last list. theGlow is the second pass, which draws the glow of the bricks that have one.
void Board::DrawBricks(Graphics* g, bool theGlow)
{
	bool aDraw = mPauseCount < 1 || (!mHintSpots.empty() && mHintSpots[0]->mActive) || mApp->mTutorialOn;
	if (!aDraw)
		return;

	for (int aRow = mBonusFieldLoaded ? 1 : 0; aRow < 19; aRow++)
	{
		for (int aCol = 0; aCol < 10; aCol++)
		{
			Brick* aBrick = mFieldBricks[aRow][aCol];
			if (aBrick != NULL)
				aBrick->Draw(g, theGlow);
		}
	}

	for (std::list<std::list<Brick*> >::iterator aGroup = mFiredGroups.begin(); aGroup != mFiredGroups.end(); ++aGroup)
		for (std::list<Brick*>::iterator aBrick = aGroup->begin(); aBrick != aGroup->end(); ++aBrick)
			(*aBrick)->Draw(g, theGlow);

	for (std::list<Brick*>::iterator aBrick = mGoneBricks.begin(); aBrick != mGoneBricks.end(); ++aBrick)
		(*aBrick)->Draw(g, theGlow);

	for (std::list<std::list<Brick*> >::iterator aGroup = mFallingGroups.begin(); aGroup != mFallingGroups.end(); ++aGroup)
		for (std::list<Brick*>::iterator aBrick = aGroup->begin(); aBrick != aGroup->end(); ++aBrick)
			(*aBrick)->Draw(g, theGlow);

	for (std::list<Brick*>::iterator aBrick = mTakenBricks.begin(); aBrick != mTakenBricks.end(); ++aBrick)
		(*aBrick)->Draw(g, theGlow);
}

// 0x443fd0: while the tally runs (mClearCountRunning) the held pieces stand over the stack and a dark bar closes in from each side;
// the introduction of a bonus round (and of the second game type's stages) puts the rules over the field.
void Board::DrawBonusIntro(Graphics* g)
{
	int aPieceX = mShipX - 5;
	int aBaseY = (int) (m1bf8 + 423.0f);

	if (mClearCountRunning)
	{
		for (int aPass = 0; aPass < 2; aPass++)
		{
			int aY = aBaseY;
			for (std::list<Brick*>::iterator aBrick = mHeldBricks.begin(); aBrick != mHeldBricks.end(); ++aBrick)
			{
				aY -= 0x19;
				if (!(*aBrick)->mVisible)
					(*aBrick)->DrawAt(g, aPass != 0, (float) (aPieceX + 5), (float) aY);
			}
		}

		g->SetColor(Color(0, 0, 0, 0xaa));
		g->FillRect(0, 0xa0, m1b88, 0xe1);
		g->FillRect(mWidth - m1b88, 0xa0, m1b88, 0xe1);
	}

	if (((m1bc0 == 2 && m1bc8 == 0xff) || m1bc0 == 3) && !mBonusRoundStarted || (mBonusRoundOn && !mBonusWarpIn && !mBonusRoundStarted))
	{
		g->SetColor(Color(0, 0, 0, 0xdc));
		g->FillRect(0, 0xa0, mWidth, 0xc3);
		g->DrawImage(IMAGE_BONUS_TEXT_HEADER, 0x182 - IMAGE_BONUS_TEXT_HEADER->mWidth / 2, 0xb4);
		g->SetFont(GetFontById(FONT_DIALOG_BOX_BTN_ID));

		std::string aWorth = FormatNumber(mClearBonusWorth);
		WriteString(g, StrFormat(GTC(0x00564828), aWorth.c_str()), 0xa6, 0x104, 0x1b8, 0, true, 0, -1);
		int aY = g->GetFont()->GetHeight() + 0x104;
		std::string aPenalty = FormatNumber((int) ((float) mClearBonusWorth * 0.05f));
		WriteString(g, StrFormat(GTC(0x005647e8), aPenalty.c_str()), 0xa6, aY, 0x1b8, 0, true, 0, -1);
		aY = aY + g->GetFont()->GetHeight() * 2;

		g->SetColor(Color(0xff, 0xff, 0));
		WriteString(g, GT(0x005647cc), 0xa6, aY, 0x1b8, 0, true, 0, -1);
		aY += g->GetFont()->GetHeight() * 2;

		if (!mBricksRevealing && !mBonusRoundStarted && !mBonusWarpIn)
		{
			int aPhase = mUpdateCnt % 0x80;
			if (aPhase > 0x3f)
				aPhase = 0x7f - aPhase;
			g->SetColor(Color(0xff, 0xb9, 0, aPhase * 4));
			std::string aText = GT(0x005647bc);
			g->DrawString(aText, 0x182 - g->GetFont()->StringWidth(aText) / 2, aY);
		}
	}
}
