// The screen of a game: the drawing (see Board.h).

#include "Board.h"
#include "AtomicaApp.h"
#include "Atomicus.h"
#include "BonusMolecules.h"
#include "Grid.h"
#include "Swirl.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"

#include <cmath>
#include <cstdio>

using namespace Sexy;

// the level names: the original's table at 0x4cbc58 points to these texts
static const unsigned int kLevelNames[] = {
	0x4ae4c0,
	0x4af734, 0x4af728, 0x4af71c, 0x4af710, 0x4af700, 0x4af6f0, 0x4af6e0, 0x4af6d0, 0x4af6c4, 0x4af6b0,
	0x4af6a0, 0x4af690, 0x4af67c, 0x4af66c, 0x4af658, 0x4af648, 0x4af638, 0x4af628, 0x4af61c, 0x4af608,
	0x4af5fc, 0x4af5ec, 0x4af5e0, 0x4af5d0, 0x4af5bc, 0x4af5ac, 0x4af59c, 0x4af58c, 0x4af57c, 0x4af568,
	0x4af55c, 0x4af54c, 0x4af53c, 0x4af52c, 0x4af518, 0x4af504, 0x4af4f4, 0x4af4e4, 0x4af4d4, 0x4af4c0,
	0x4af4b4, 0x4af4a4
};

// 0x433f10
void Board::Draw(Graphics* g)
{
	Graphics* aSaved = g;
	AtomicaApp* anApp = mApp;
	(void) aSaved;

	// the background: the swirls, each as the band between its ring and the ring of the one before (the original first clears
	// the screen when its detail setting is not 1)
	{
		mSwirls.Reset();
		Swirl* aPrevious = NULL;
		Swirl* aSwirl = mSwirls.Next();
		while (aSwirl != NULL)
		{
			aSwirl->Draw(g, aPrevious);
			if (!aSwirl->mClone)
				aPrevious = aSwirl;
			aSwirl = mSwirls.Next();
		}
	}

	// the field (a copy of the graphics moved to the field's place; the bomb shakes it)
	if (mStartCountdown == 0.0f)
	{
		Graphics aFieldG(*g);
		aFieldG.Translate(mGrid->mX, mGrid->mY);
		if (mBombOn && mBombShake)
			aFieldG.Translate(anApp->mRand.Get(5, true), anApp->mRand.Get(5, true));
		mGrid->DrawField(&aFieldG);
	}
	if (mBombOn && mBombShake)
		g->Translate(anApp->mRand.Get(5, true), anApp->mRand.Get(5, true));

	Graphics* aPlain = g;
	(void) aPlain;
#define TINT_ON() if (mTintOn) { g->SetColorizeImages(true); g->SetColor(mTint); }
#define TINT_OFF() if (mTintOn) { g->SetColorizeImages(false); g->SetColor(Color(255, 255, 255, 255)); }

	TINT_ON();
	anApp->mMachineRim->Draw(g, mGrid->mX - 0x15, mGrid->mY);
	TINT_OFF();

	// the effects
	mEffects.Reset();
	Effect* anEffect = mEffects.Next();
	while (anEffect != NULL)
	{
		anEffect->Draw(g);
		anEffect = mEffects.Next();
	}

	// the top of the machine
	g->SetColor(Color(0, 0, 0, 255));
	g->FillRect(0, 0, 0x280, 10);
	TINT_ON();
	anApp->mMachineLevelMeter->Draw(g, 0x280 - anApp->mMachineLevelMeter->mWidth, 0);
	TINT_OFF();
	if (0.0f < mGeigerGlow)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int) mGeigerGlow));
		anApp->mGeigerGlow->Draw(g, 0x161, 0);
		g->SetColorizeImages(false);
	}
	TINT_ON();
	mMeter.Draw(g);
	TINT_OFF();

	// the score and the level
	TINT_ON();
	anApp->mMachineScore->Draw(g, 0, 0);
	anApp->mLevelAndScore->Draw(g, 0x21, 0x23);
	anApp->mLevelWord->Draw(g, 0xa4, 0xd);
	{
		char aLevel[16];
		snprintf(aLevel, sizeof(aLevel), "%.2d", mMeter.mLevel);
		anApp->mLevelNumber[aLevel[0] - '0'][0]->Draw(g, 0xa5, 0x1a);
		anApp->mLevelNumber[aLevel[1] - '0'][1]->Draw(g, 0xae, 0x1a);
		char aScore[16];
		snprintf(aScore, sizeof(aScore), "%d", mScore);
		g->SetFont(anApp->mScoreFont);
		g->SetColor(Color(0, 255, 0, 255));
		std::string aScoreText = aScore;
		g->DrawString(aScoreText, 0x41 - anApp->mScoreFont->StringWidth(aScoreText) / 2, 0x41);
	}
	TINT_OFF();

	// the bomb
	TINT_ON();
	anApp->mMachineBomber->Draw(g, 0, 0x1e0 - anApp->mMachineBomber->mHeight);
	if (0.999f < mBombPhase)
	{
		if (!mButtonBomb->mIsDown)
		{
			int aFrame = (int) mBombPhase;
			if (aFrame >= 1)
				anApp->mMachineButtonSeq[aFrame - 1]->Draw(g, 0x164, 0x1ab);
		}
		else
			anApp->mMachineButtonPressed->Draw(g, 0x158, 0x1b0);
	}
	if (mBombFrame < 24.0f)
		anApp->mBomberCap[(int) mBombFrame]->Draw(g, 0x158, 0x183);
	TINT_OFF();
	TINT_ON();
	anApp->mBomberCharging[0]->DrawPartial(g, 0xd6, 0x1b8, mProgress * 0.020833334f, 1, 0);
	anApp->mBomberCharging[1]->DrawPartial(g, 0x1b6, 0x1b8, mProgress * 0.020833334f, -1, 0);
	anApp->mMachineBomberMeter->Draw(g, 0xde, 0x1b8);
	anApp->mMachineMonitor->Draw(g, 0, mMonitorSlide + 0xf3);
	mAtomicus->Draw(g, -3, mMonitorSlide + 0x117);
	if (mMonitorSlide < 1)
	{
		if (mButtonOptions->mIsDown)
			anApp->mSideButtons[5]->Draw(g, 0x16, 0x1c0);
		else if (mButtonOptions->mIsOver)
			anApp->mSideButtons[4]->Draw(g, 0x16, 0x1c0);
		if (mButtonMenu->mIsDown)
			anApp->mSideButtons[3]->Draw(g, 0x16, 0xfe);
		else if (mButtonMenu->mIsOver)
			anApp->mSideButtons[2]->Draw(g, 0x16, 0xfe);
	}
	TINT_OFF();

	// the magnet levels' arrows along the field's edges
	if (0 < mGrid->mArrowX)
	{
		anApp->mMagnet[2]->Draw(g, 0x233, 0x32);
		anApp->mMagnet[6]->Draw(g, 0x233, 0x161);
	}
	if (mGrid->mArrowX < 0)
	{
		anApp->mMagnet[3]->Draw(g, 0, 0x32);
		anApp->mMagnet[7]->Draw(g, 0, 0x161);
	}
	if (0 < mGrid->mArrowY)
	{
		anApp->mMagnet[0]->Draw(g, 0xac, 0x193);
		anApp->mMagnet[1]->Draw(g, 0x20f, 0x193);
	}
	if (mGrid->mArrowY < 0)
	{
		anApp->mMagnet[4]->Draw(g, 0xac, 0);
		anApp->mMagnet[5]->Draw(g, 0x20f, 0);
	}

	// the geiger counter's needle: a line from 18 to 30 pixels from the middle of the dial (389, 60), at the angle of the
	// combo (-52 to +67 degrees)
	{
		int aAngle = (int) mNeedle - 0x34;
		if (FULL_TURN - 1 < aAngle)
			aAngle -= FULL_TURN;
		if (aAngle < 0)
			aAngle += FULL_TURN;
		float aSin = SinDeg(aAngle);
		float aCos = NegCosDeg(aAngle);
		int aX1 = (int) (aSin * 18.0f + 389.0f);
		int aY1 = (int) (60.0f + aCos * 18.0f);
		int aX2 = (int) (aSin * 30.0f + 389.0f);
		int aY2 = (int) (60.0f + aCos * 30.0f);
		g->SetColor(Color(0, 0, 0, 0x80));
		g->DrawLine(aX1 + 1, aY1 + 1, aX2 + 1, aY2 + 1);
		g->SetColor(Color(255, 0, 0, 255));
		g->DrawLine(aX1, aY1, aX2, aY2);
		g->DrawLine(aX1, aY1, aX2 + 1, aY2);
		g->DrawLine(aX1, aY1, aX2 - 1, aY2);
	}

	if (0 < mFreezeProgress)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, mFreezeProgress));
		anApp->mFreeze->Draw(g, 0, 0);
		g->SetColorizeImages(false);
	}
	mBonus->Draw(g);

	// the signs over the field
	int aCenterX = mGrid->mX + mGrid->mWidth / 2;
	int aCenterY = mGrid->mY + mGrid->mHeight / 2;
	if (0 < mGetReadyAlpha)
	{
		int anAlpha = mGetReadyAlpha > 254 ? 255 : mGetReadyAlpha;
		g->SetColorizeImages(true);
		g->SetColor(Color(200, 255, 200, anAlpha));
		Sprite* aSign = anApp->mBigWords[2];
		aSign->Draw(g, aCenterX - aSign->mWidth / 2, aCenterY - aSign->mHeight / 2);
		if (anApp->mMode == 0)
		{
			char aLevel[32];
			snprintf(aLevel, sizeof(aLevel), "LEVEL %d", mMeter.mLevel);
			const std::string& aName = GT(kLevelNames[mMeter.mLevel]);
			g->SetFont(anApp->mAnnounceFont);
			g->SetColor(Color(0, 0, 0, anAlpha));
			std::string aLevelText = aLevel;
			g->DrawString(aLevelText, aCenterX - anApp->mAnnounceFont->StringWidth(aLevelText) / 2 + 4,
				mGrid->mHeight / 2 - 0x2e);
			g->DrawString(aName, aCenterX - anApp->mAnnounceFont->StringWidth(aName) / 2 + 4, mGrid->mHeight / 2 + 0x9a);
			g->SetColor(Color(0x96, 0xbf, 0x96, anAlpha));
			g->DrawString(aLevelText, aCenterX - anApp->mAnnounceFont->StringWidth(aLevelText) / 2,
				mGrid->mHeight / 2 - 0x32);
			g->DrawString(aName, aCenterX - anApp->mAnnounceFont->StringWidth(aName) / 2, mGrid->mHeight / 2 + 0x96);
		}
		g->SetColorizeImages(false);
	}
	if (0 < mLevelUpAlpha)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(200, 255, 200, mLevelUpAlpha));
		Sprite* aSign = anApp->mBigWords[0];
		aSign->Draw(g, aCenterX - aSign->mWidth / 2, aCenterY - aSign->mHeight / 2);
		g->SetColorizeImages(false);
	}
	if (mGrid->mGameOver && (300 < mGrid->mGameOverTick))
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(200, 255, 200, mGameOverAlpha));
		anApp->mBigWords[1]->DrawCentered(g, aCenterX, aCenterY);
		g->SetColorizeImages(false);
	}
	if (mBombSignAlpha != 0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, mBombSignAlpha));
		Sprite* aSymbol = anApp->mBombSymbol;
		aSymbol->Draw(g, aCenterX - aSymbol->mWidth / 2, aCenterY - aSymbol->mHeight / 2);
		g->SetColor(Color(255, 175, 64, mBombSignAlpha));
		g->SetFont(anApp->mAnnounceFont);
		std::string aAtomicWord = "ATOMIC";
		std::string aBombWord = "BOMB";
		g->DrawString(aAtomicWord, aCenterX - anApp->mAnnounceFont->StringWidth(aAtomicWord) / 2, 0x78);
		g->DrawString(aBombWord, aCenterX - anApp->mAnnounceFont->StringWidth(aBombWord) / 2, 0x168);
		g->SetColorizeImages(false);
	}

	// the text for an atom the mouse rests on
	if (mGrid->mTooltipAtom != NULL)
	{
		Atom* anAtom = mGrid->mTooltipAtom;
		g->Translate(mGrid->mX, mGrid->mY);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 150, 255, mGrid->mTooltipAlpha));
		anApp->mMouseOver->Draw(g, anAtom->mX - anApp->mMouseOver->mWidth / 2, anAtom->mY - anApp->mMouseOver->mHeight / 2);
		g->SetColorizeImages(false);
		g->SetFont(anApp->mMouseoverFont);
		if (!anAtom->IsElement())
		{
			const char* aLine1 = NULL;
			const char* aLine2 = NULL;
			const char* aLine3 = NULL;
			if (anAtom->mKind == KIND_BIOHAZARD)
			{
				aLine1 = GTC(0x4b209c);
				aLine2 = GTC(0x4b30a0);
				aLine3 = GTC(0x4b3090);
			}
			else if (anAtom->mKind == KIND_CHROMIUM)
			{
				aLine1 = GTC(0x4b1f6c);
				aLine2 = GTC(0x4b3084);
				aLine3 = GTC(0x4b3070);
			}
			if (aLine1 != NULL)
			{
				g->SetColor(Color(255, 150, 255, mGrid->mTooltipAlpha));
				std::string aText1 = aLine1;
				std::string aText2 = aLine2;
				std::string aText3 = aLine3;
				g->DrawString(aText1, anAtom->mX - anApp->mMouseoverFont->StringWidth(aText1) / 2, anAtom->mY - 0x28);
				g->DrawString(aText2, anAtom->mX - anApp->mMouseoverFont->StringWidth(aText2) / 2, anAtom->mY + 0x37);
				g->DrawString(aText3, anAtom->mX - anApp->mMouseoverFont->StringWidth(aText3) / 2, anAtom->mY + 0x46);
			}
		}
		else
		{
			g->SetColor(Color(255, 150, 255, mGrid->mTooltipAlpha));
			const char* aLines[4] = {GTC(0x4b30d8), GTC(0x4b30cc), GTC(0x4b30bc), GTC(0x4b30b0)};
			static const int aOffsets[4] = {-0x37, -0x28, 0x37, 0x46};
			for (int i = 0; i < 4; i++)
			{
				std::string aText = aLines[i];
				g->DrawString(aText, anAtom->mX - anApp->mMouseoverFont->StringWidth(aText) / 2, anAtom->mY + aOffsets[i]);
			}
		}
		g->Translate(-mGrid->mX, -mGrid->mY);
	}

	// the explanation of what the mouse is over (the combo, the molecules of the level)
	if (mIsOver && (mTooltip[0] != 0))
	{
		g->SetFont(anApp->mMonitorFontSmall);
		std::string aText = mTooltip;
		int aWidth = anApp->mMonitorFontSmall->StringWidth(aText);
		g->SetColor(Color(0, 0, 0, 0x80));
		g->FillRect(mMouseX - aWidth / 2 - 5, mMouseY + 0x14, aWidth + 10, anApp->mMonitorFontSmall->GetHeight() + 4);
		g->SetColor(Color(255, 255, 255, 255));
		g->DrawString(aText, mMouseX - aWidth / 2, mMouseY + 0x14 + anApp->mMonitorFontSmall->GetAscent() + 2);
	}

	// pause
	if ((mGrid != NULL) && mGrid->mPausedByKey)
	{
		g->SetFont(anApp->mAnnounceFont);
		std::string aPaused = "PAUSED";
		int aX = aCenterX - anApp->mAnnounceFont->StringWidth(aPaused) / 2;
		g->SetColor(Color(0, 0, 0, 255));
		g->DrawString(aPaused, aX + 5, mHeight / 2 + 5);
		g->SetColor(Color(0xeb, 0xeb, 0xff, 255));
		g->DrawString(aPaused, aX, mHeight / 2);
		g->SetFont(anApp->mMouseoverFont);
		const std::string& aContinue = GT(0x4b3050);
		int aX2 = aCenterX - anApp->mMouseoverFont->StringWidth(aContinue) / 2;
		g->SetColor(Color(0, 0, 0, 255));
		g->DrawString(aContinue, aX2 + 5, mHeight / 2 + 0x1e);
		g->SetColor(Color(0xeb, 0xeb, 0xff, 255));
		g->DrawString(aContinue, aX2, mHeight / 2 + 0x19);
	}
#undef TINT_ON
#undef TINT_OFF
}
