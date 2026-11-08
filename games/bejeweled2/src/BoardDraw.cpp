// BoardDraw.cpp: how Sexy::Board draws itself: the screen as a whole (Draw 0x5a2d15 and the 2D and 3D ways of drawing
// it), the score box and the controls beside the board, the level number and the picture that waves in the star field.
//   DrawWidgetTo 0x596a95, DrawUI 0x596acd, DrawWavyImage 0x5983e3, DrawLevelText 0x5a0a52, DrawScore 0x5a0d5e,
//   DrawAllToBackgroundCache 0x5a29bc, DrawNormal2d 0x5a2a51, DrawNormal 0x5a2c1b, Draw 0x5a2d15.
// The board itself is drawn by DrawBoard (BoardDrawBoard.cpp), the explosions and lightning over it by DrawEffects
// (BoardDrawEffects.cpp) and the 3D whirlpool by BoardDraw3D.cpp.
//
// DrawScore, DrawUI and DrawBoard each draw two halves: the part that never changes between two frames (the frames and
// the pictures; "static", skipped when theSkipStatic is set) and the part that does (the texts, the buttons; "dynamic",
// skipped when theSkipDynamic is set). The 2D game draws the static halves once into the picture m15f60 and copies it.

#include "Board.h"
#include "WinDMApp.h"
#include "Res.h"
#include "PuzzleStateOverlay.h"
#include "StarField.h"
#include "PodDialog.h"
#include "Points.h"
#include "EffectOverlay.h"
#include "Hyperspace.h"
#include "HyperlinkWidget.h"
#include "DDImage.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include "Font.h"
#include "Common.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

using namespace Sexy;

// where the score box sits: the pixel offset of its top left corner in the 640 by 480 screen is (aLeft, aTop). The puzzle
// games without the secret modes have none of it (their box is the pod dome) but the code asks anyway
static int ScoreBoxTop(const Board* theBoard)
{
	if (!theBoard->mIsPuzzle || theBoard->mIsSecret)
		return theBoard->mIsEndless ? 0x23 : 0x2a;
	return 0x1b;
}

static int ScoreBoxLeft(const Board* theBoard)
{
	if (!theBoard->mIsPuzzle || theBoard->mIsSecret)
		return theBoard->mIsEndless ? 0xf : 0x16;
	return 0;
}

// the places of the seven segments of the ring of the endless mode's meter (at 0x62972c, x then y) and of the seven lights
// in its middle (at 0x6296f4)
static const int gEndlessRing[7][2] = {{34, 42}, {18, 100}, {33, 173}, {94, 211}, {165, 172}, {203, 100}, {164, 40}};
static const int gEndlessLights[7][2] = {{106, 66}, {144, 93}, {143, 134}, {106, 161}, {66, 135}, {66, 92}, {104, 111}};

// 0x596a95: a widget is drawn at its own place
void Board::DrawWidgetTo(Graphics* g, Widget* theWidget)
{
	g->Translate(theWidget->mX, theWidget->mY);
	theWidget->Draw(g);
	g->Translate(-theWidget->mX, -theWidget->mY);
}

// 0x596acd: the frame of the buttons at the bottom of the screen and the buttons
void Board::DrawUI(Graphics* g, bool theSkipStatic, bool theSkipDynamic)
{
	if (!mIsPuzzle && !mIsAction)
	{
		if (!theSkipStatic && !m165c0)
		{
			int aY = mApp->ScaleNum(0x19e, 0);
			int aX = mApp->ScaleNum(0x1f, 0);
			g->DrawImage(IMAGE_CONTROLS_CLASSIC, aX, aY);
		}
		if (!theSkipDynamic)
		{
			DrawWidgetTo(g, mMenuButton);
			DrawWidgetTo(g, mHintButton);
		}
	}
	else
	{
		if (!theSkipStatic && !m165c0)
		{
			int aY = mApp->ScaleNum(0x153, 0);
			int aX = mApp->ScaleNum(0x1b, 0);
			g->DrawImage(IMAGE_CONTROLS_PUZZLE, aX, aY);
		}
		if (!theSkipDynamic)
		{
			DrawWidgetTo(g, mMenuButton);
			DrawWidgetTo(g, mHintButton);
			DrawWidgetTo(g, mThirdButton);
		}
	}
}

// 0x5983e3: a picture drawn in 40 strips whose x swings like a wave (the star field and the backdrop wave when the
// nausea cheat is on); theLevel is the size of the swing. The strips outside a window of the screen are left out
void Board::DrawWavyImage(Graphics* g, Image* theImage, int theX, int theY, float theLevel)
{
	int aSrcY = 0;
	int aStripH = mApp->ScaleNum(0x300, 0);
	aStripH = aStripH / 0x28;
	int aPhase = 0;
	int aImageWidth = theImage->mWidth;
	int aLimit = -0x51 - theY;
	do
	{
		if ((unsigned int)(aLimit + theY) < 0x293)
		{
			if (mApp->Is3DAccelerated())
			{
				float aAngle = (float)((double)((mUpdateCnt + aPhase) * 2) * (double)3.14159f / 180.0);
				float aSwing = (float)((double)sinf(aAngle) * (double)theLevel);
				Rect aSrc(0, aSrcY, aImageWidth, aStripH);
				g->DrawImageF(theImage, (float)theX, (float)theY + aSwing, aSrc);
			}
			else
			{
				float aAngle = (float)((double)((mUpdateCnt + aPhase) * 2) * (double)3.14159f / 180.0);
				float aSwing = (float)((double)sinf(aAngle) * (double)theLevel);
				Rect aSrc(0, aSrcY, aImageWidth, aStripH);
				g->DrawImage(theImage, theX, FTol((double)theY + (double)aSwing), aSrc);
			}
		}
		aPhase += 0xf;
		theY += aStripH;
		aSrcY += aStripH;
	}
	while (aPhase < 600);
}

// 0x5a0a52: the number of the level in the glow font (the picture mFontGlowImage is made from it)
void Board::DrawLevelText(Graphics* g)
{
	char aBuf[256];
	sprintf(aBuf, "%d", mNextLevel);
	g->SetColor(Color(0xff, 0xff, 0xff));
	if (strlen(aBuf) < 3)
		g->SetFont(FONT_CONTINUUM_BOLD_14_NUM_GLOW);
	else
		g->SetFont(FONT_CONTINUUM_BOLD_11_NUM_GLOW);

	int aTop = ScoreBoxTop(this);
	int aLeft = ScoreBoxLeft(this);
	std::string aText = aBuf;
	Font* aFont = g->GetFont();
	int aY = mApp->ScaleNum(aTop + 0x72, 0);
	int aX = mApp->ScaleNum(aLeft + 0x77, 0);
	int aWidth = aFont->StringWidth(aText);
	g->DrawString(aText, aX - aWidth / 2, aY);
}

// 0x5a0d5e: the score box beside the board. The classic games have the score and the level in the box; the endless game has
// a meter of rings that fills as the level goes on, and the puzzle games the dome of pods
void Board::DrawScore(Graphics* g, bool theSkipStatic, bool theSkipDynamic)
{
	if (mIsPuzzle && !mIsSecret)
	{
		if (!theSkipStatic)
			mPuzzleOverlay->DrawBack(g);
		if (!theSkipDynamic)
		{
			mPuzzleOverlay->DrawFront(g);
			int aSlide = FTol(mApp->ScaleNum(mSlide.GetOutVal(), 0.0));
			Graphics aG(*g);
			aG.Translate(mPuzzleOverlay->mDomeWidget->mX + aSlide, mPuzzleOverlay->mDomeWidget->mY);
			mPuzzleOverlay->mDomeWidget->mBtnNoDraw = false;
			mPuzzleOverlay->mDomeWidget->Draw(&aG);
			mPuzzleOverlay->mDomeWidget->mBtnNoDraw = true;
		}
		return;
	}

	if (mIsEndless)
	{
		int aCount = 0;
		if (!theSkipStatic && !m165c0)
		{
			int aTop = ScoreBoxTop(this);
			int aLeft = ScoreBoxLeft(this);
			g->DrawImage(IMAGE_ENDLESS_WIDGET, mApp->ScaleNum(aLeft, 0), mApp->ScaleNum(aTop, 0));

			// the rings: the first mSweep0 are full, then mSweep1 more grow
			aCount = mSweep1 + mSweep0;
			for (int i = 0; i < aCount; i++)
			{
				Image* anImage = GetImageById(IMAGE_ENDLESS_OUTER_1_ID + i);
				int aWidth = anImage->GetWidth();
				float aCelWidth = (float)((double)aWidth / 9.0);
				int aHeight = anImage->GetHeight();
				int aCel = (i < mSweep0) ? 7 : mSweep2;
				int aRingTop = ScoreBoxTop(this);
				int aRingLeft = ScoreBoxLeft(this);
				Rect aSrc(FTol((double)aCel * (double)aCelWidth), 0, FTol((double)aCelWidth), aHeight);
				int aY = mApp->ScaleNum(gEndlessRing[i][1] + aRingTop, 0);
				int aX = mApp->ScaleNum(gEndlessRing[i][0] + aRingLeft, 0);
				g->DrawImage(anImage, aX, aY, aSrc);
			}

			DrawEffects(g, true);

			for (int i = 0; i < aCount; i++)
			{
				Image* anImage = GetImageById(IMAGE_ENDLESS_OUTER_1_ID + i);
				int aWidth = anImage->GetWidth();
				float aCelWidth = (float)((double)aWidth / 9.0);
				int aHeight = anImage->GetHeight();
				if (i < mSweep0)
				{
					int aRingTop = ScoreBoxTop(this);
					int aRingLeft = ScoreBoxLeft(this);
					Rect aSrc(FTol((double)aCelWidth * 8.0), 0, FTol((double)aCelWidth), aHeight);
					int aY = mApp->ScaleNum(gEndlessRing[i][1] + aRingTop, 0);
					int aX = mApp->ScaleNum(gEndlessRing[i][0] + aRingLeft, 0);
					g->DrawImage(anImage, aX, aY, aSrc);
				}
			}

			// the lights in the middle
			int aLights = mSweep2;
			for (int i = 0; i < aLights; i++)
			{
				int aLightTop = ScoreBoxTop(this);
				int aLightLeft = ScoreBoxLeft(this);
				int aY = mApp->ScaleNum(gEndlessLights[i][1] + aLightTop, 0);
				int aX = mApp->ScaleNum(gEndlessLights[i][0] + aLightLeft, 0);
				g->DrawImage(GetImageById(IMAGE_ENDLESS_CENTER_1_ID + i), aX, aY);
			}
		}

		if (theSkipDynamic)
			return;

		g->SetFont(FONT_CONTINUUM_BOLD_14_NUM);
		g->SetColor(Color(0xff, 0xff, 0xff));
		std::string aScoreText = CommaSeperate(m15e40);
		int aTop = ScoreBoxTop(this);
		int aLeft = ScoreBoxLeft(this);
		Font* aFont = g->GetFont();
		int aY = mApp->ScaleNum(aTop + 0x18, 0);
		int aX = mApp->ScaleNum(aLeft + 0x7e, 0);
		int aWidth = aFont->StringWidth(aScoreText);
		g->DrawString(aScoreText, aX - (aWidth + 1) / 2, aY);

		g->SetFont(FONT_CONTINUUM_BOLD_11_NUM);
		g->SetColor(Color(0xff, 0xff, 0xff));
		std::string aLevelText = CommaSeperate(mNextLevel);
		aTop = ScoreBoxTop(this);
		aLeft = ScoreBoxLeft(this);
		aFont = g->GetFont();
		aY = mApp->ScaleNum(aTop + 0x33, 0);
		aX = mApp->ScaleNum(aLeft + 0x7e, 0);
		aWidth = aFont->StringWidth(aLevelText);
		g->DrawString(aLevelText, aX - (aWidth + 1) / 2, aY);
		return;
	}

	// the classic games
	if (!theSkipStatic && !m165c0)
	{
		int aTop = ScoreBoxTop(this);
		int aLeft = ScoreBoxLeft(this);
		g->DrawImage(IMAGE_CONTROLS_SCORE, mApp->ScaleNum(aLeft, 0), mApp->ScaleNum(aTop, 0));
	}
	if (theSkipDynamic)
		return;

	g->SetFont(FONT_SCORE);
	std::string aScoreText = CommaSeperate(m15e40);
	g->SetColor(Color(0xad, 0xe7, 0xff));
	int aHalfWidth = m15edc->StringWidth(aScoreText) / 2;
	int aY = mApp->ScaleNum(0x6d, 0);
	int aX = mApp->ScaleNum(0x8d, 0);
	g->DrawString(aScoreText, aX - aHalfWidth, aY);

	// the level number glows
	g->SetColorizeImages(true);
	g->SetColor(Color(0, 0x2a, 0x2a, 200));
	{
		int aTop = ScoreBoxTop(this);
		int aLeft = ScoreBoxLeft(this);
		int aGlowY = mApp->ScaleNum(aTop + 0x59, 0);
		int aGlowX = mApp->ScaleNum(aLeft + 0x61, 0);
		g->DrawImage(mFontGlowImage, aGlowX, aGlowY);
	}
	g->SetColorizeImages(false);
	g->SetColor(Color(200, 200, 0xff));

	char aBuf[32];
	sprintf(aBuf, "%d", mNextLevel);
	if (strlen(aBuf) < 3)
		g->SetFont(FONT_CONTINUUM_BOLD_14_NUM);
	else
		g->SetFont(FONT_CONTINUUM_BOLD_11_NUM);
	int aTop = ScoreBoxTop(this);
	int aLeft = ScoreBoxLeft(this);
	std::string aLevelText = aBuf;
	Font* aFont = g->GetFont();
	int aLevelY = mApp->ScaleNum(aTop + 0x72, 0);
	int aLevelX = mApp->ScaleNum(aLeft + 0x78, -1);
	int aWidth = aFont->StringWidth(aLevelText);
	g->DrawString(aLevelText, aLevelX - aWidth / 2, aLevelY);
}

// 0x5a29bc: the static halves go into the picture m15f60 (the star field covers it, so the picture is cleared with its
// transparency on)
void Board::DrawAllToBackgroundCache()
{
	if (m15f60 == NULL)
		return;

	if (m165bc != NULL)
	{
		m15f60->SetImageMode(true, true);
		m15f60->Clear();
	}
	Graphics aG(m15f60);
	DrawScore(&aG, true, false);
	DrawUI(&aG, true, false);
	DrawBoard(&aG, true, false);
}

// 0x5a2a51: the 2D way of drawing the screen: the static halves wait in the picture m15f60, which is made again when
// m15f69 is set
void Board::DrawNormal2d(Graphics* g)
{
	int aSlide = FTol(mApp->ScaleNum(mSlide.GetOutVal(), 0.0));
	if (m15f60 == NULL)
	{
		DDImage* anImage = new DDImage(mApp->mDDInterface);
		m15f60 = anImage;
		anImage->Create(mWidth, mHeight);
		anImage->SetImageMode(false, false);
		anImage->mPurgeBits = true;
		anImage->PurgeBits();
	}

	if (m15f69)
	{
		Graphics aG(m15f60);
		m15f60->SetImageMode(false, false);
		aG.DrawImage(mBackdropImage, 0, 0);
		DrawScore(&aG, false, true);
		DrawUI(&aG, false, true);
		DrawBoard(&aG, false, true);
		m15f69 = false;
	}

	if (m165bc == NULL)
	{
		g->DrawImage(m15f60, 0, 0);
		if (m165b8)
			DrawWavyImage(g, m15f60, 0, 0, 2.0f);
	}
	else
	{
		m165bc->Draw(g, m165b8);
	}

	g->Translate(-aSlide, 0);
	DrawScore(g, true, false);
	g->Translate(aSlide * 2, 0);
	if (!m15f24)
		DrawBoard(g, true, false);
	g->Translate(-aSlide, 0);
}

// 0x5a2c1b: the way of drawing the screen when the board is not the 2D game's: everything is drawn every frame
void Board::DrawNormal(Graphics* g)
{
	int aSlide = FTol(mApp->ScaleNum(mSlide.GetOutVal(), 0.0));
	if (m165bc == NULL)
	{
		g->DrawImage(mBackdropImage, 0, 0);
		if (m165b8)
		{
			mApp->Is3DAccelerated();
			DrawWavyImage(g, mBackdropImage, 0, 0, 3.5f);
		}
	}
	else
	{
		m165bc->Draw(g, m165b8);
	}

	g->Translate(-aSlide, 0);
	DrawScore(g, false, m191c);
	DrawUI(g, false, true);
	if (!m15f24)
	{
		g->Translate(aSlide * 2, 0);
		DrawBoard(g, false, false);
		g->Translate(-aSlide, 0);
	}
	else
	{
		DrawBoard(g, false, true);
	}
}

// a line of the hints in Draw: centred on theCenterX, theY in the 640 by 480 screen (each of the original's lines is
// written out the same way: the font of the graphics measures it)
static void DrawHintLine(WinDMApp* theApp, Graphics* g, const std::string& theText, int theCenterX, int theY)
{
	Font* aFont = g->GetFont();
	int aY = theApp->ScaleNum(theY, 0);
	g->DrawString(theText, theCenterX - aFont->StringWidth(theText) / 2, aY);
}

// 0x5a2d15: the whole screen: the board and its buttons (the 3D game draws the whirlpool and the end of an interface
// pause on its own), the score texts, the explosions and lightning over them, and then the texts that tell what to do
void Board::Draw(Graphics* g)
{
	bool a3D = mApp->Is3DAccelerated();
	if (mBackdropImage == NULL)
		return;

	int aShakeY = mApp->ScaleNum(mShakeY, 0);
	int aShakeX = mApp->ScaleNum(mShakeX, 0);
	g->Translate(aShakeX, aShakeY);
	int aCenterX, aCenterY;
	GetBoardCenter(&aCenterX, &aCenterY);
	if (m1750)
		m1750 = false;

	if (mState == STATE_WHIRLPOOL)
	{
		if (a3D)
		{
			Draw3DWhirlpoolState(g);
		}
		else if (mHyperspace == NULL || !mHyperspace->mReady)
		{
			DrawNormal2d(g);
			DrawUI(g, true, false);
		}
		else if (m165bc != NULL)
		{
			m165bc->Draw(g, m165b8);
		}
	}
	else if (mState == STATE_POST_WARP)
	{
		if (a3D)
			Draw3DInterfaceRestoreState(g);
	}
	else
	{
		int aSlide = FTol(mApp->ScaleNum(mSlide.GetOutVal(), 0.0));
		if ((aSlide == 0 || m165bc != NULL) && !a3D)
			DrawNormal2d(g);
		else
			DrawNormal(g);
	}

	if (mPauseCount != mInterfacePauseCount || mInterfacePauseCount == 0)
	{
		DrawEffects(g, false);
		g->SetColor(Color::White);
		for (std::vector<Points*>::iterator anItr = mPoints.begin(); anItr != mPoints.end(); ++anItr)
		{
			Points* aPoints = *anItr;
			g->SetFont(FONT_DIGIT);
			int anAlpha = (aPoints->mLife * 0xff) / 0x12;
			if (anAlpha > 0xff)
				anAlpha = 0xff;
			g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
			int aY = mApp->ScaleNum(aPoints->mY, 0);
			int aX = mApp->ScaleNum(aPoints->mX, 0);
			g->Translate(aX, aY);
			aPoints->Draw(g);
			int aBackY = mApp->ScaleNum(-aPoints->mY, 0);
			int aBackX = mApp->ScaleNum(-aPoints->mX, 0);
			g->Translate(aBackX, aBackY);
		}
		DrawLightning(g);
	}

	// the word PAUSED over the board while a dialog covers the game (it fades with m15fb4)
	if (m15fb4 < 1.0f && m15fb8 && mScreenState == SCREEN_SHOWN)
	{
		g->SetFont(FONT_CONT_MED);
		g->SetColor(Color(0xff, 0xff, 0xff, FTol((1.0 - (double)m15fb4) * 255.0)));
		int aCX, aCY;
		GetBoardCenter(&aCX, &aCY);
		aCX = mApp->ScaleNum(aCX, 0);
		int aScaledCY = mApp->ScaleNum(aCY, 0);
		aCY = aScaledCY - mApp->ScaleNum(0x14, 0);

		std::string aText = mApp->GetString("GAME_PAUSED");
		Font* aFont = g->GetFont();
		Font* aWidthFont = g->GetFont();
		int aY = aCY - aFont->GetHeight() / 2;
		g->DrawString(aText, aCX - aWidthFont->StringWidth(aText) / 2, aY);

		aText = mApp->GetString("CLICK_HERE_TO_CONTINUE");
		aFont = g->GetFont();
		aWidthFont = g->GetFont();
		g->DrawString(aText, aCX - aWidthFont->StringWidth(aText) / 2, aFont->GetHeight() + aCY);
	}

	// the hints that the puzzle games write over the board
	g->SetFont(FONT_DEBUG);
	int aTextX = mApp->ScaleNum(0x28e, -3);
	if (mScreenState == SCREEN_SHOWN && mState != STATE_WHIRLPOOL)
	{
		float aFade = mApp->mPaused ? 0.3f : 1.0f;
		aFade = m15fac * aFade;
		if (!mApp->mPaused)
		{
			if (!mIsPlanetIntro)
			{
				if (mState == STATE_PUZZLE_SOLVED && !mIsSecret)
				{
					// the text runs through the colours, and pulses
					int aLum = 200 - FTol(cos((double)mUpdateCnt * 0.03) * -40.0);
					int aHue = FTol((double)mUpdateCnt * 2.5) % 0xff;
					g->SetColor(Color(mApp->HSLToRGB(aHue, 200, aLum)));
					int aSolved = mPuzzleOverlay->mSolvedCount;
					if (aSolved == 5)
					{
						DrawHintLine(mApp, g, "Press WARP to advance to the next planet!", aTextX, 0xaa);
					}
					else if (aSolved < 4)
					{
						DrawHintLine(mApp, g, "Select a new puzzle pod from the left", aTextX, 0xaa);
					}
					else
					{
						DrawHintLine(mApp, g, "Press WARP to advance to the next planet or", aTextX, 0x94);
						DrawHintLine(mApp, g, "beat the last puzzle for complete victory!", aTextX, 0xad);
					}
				}
				else if (mState == STATE_PUZZLE_STUCK)
				{
					if (mEffects.size() == 0)
					{
						g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)aFade * 255.0)));
						if (!mIsSecret)
						{
							DrawHintLine(mApp, g, "No more moves can be made.", aTextX, 0x96);
							DrawHintLine(mApp, g, "UNDO some of your moves with the UNDO button or", aTextX, 200);
							DrawHintLine(mApp, g, "RESET by clicking the indicated pod to the left.", aTextX, 0xe6);
							if (mSolutionValid)
								DrawHintLine(mApp, g, "Press HINT any time you are stuck on a puzzle!", aTextX, 0x122);
						}
						else
						{
							DrawHintLine(mApp, g, "No more moves can be made.", aTextX, 0x96);
							DrawHintLine(mApp, g, "UNDO some of your moves with the UNDO button,", aTextX, 200);
							DrawHintLine(mApp, g, "press HINT to undo to the last solvable state,", aTextX, 0xfa);
							DrawHintLine(mApp, g, "OR", aTextX, 300);
						}
					}
				}
				else if (mState == STATE_SECRET_FINISHED && mEffects.size() == 0)
				{
					g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)aFade * 255.0)));
					DrawHintLine(mApp, g, "Cognito Mode Complete", aTextX, 0x96);
					DrawHintLine(mApp, g, StrFormat("%d out of %d puzzles solved without hints.", m17a8, m17a4), aTextX, 200);
				}
			}
			else if (mState != STATE_POST_WARP)
			{
				g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)aFade * 255.0)));
				for (int i = 0; i < (int)mTutorialText.size(); i++)
				{
					Font* aFont = g->GetFont();
					int aOffset = mApp->ScaleNum(0x19, 0) * i;
					int aY = mApp->ScaleNum(0x94, 0) + aOffset;
					g->DrawString(mTutorialText[i], aTextX - aFont->StringWidth(mTutorialText[i]) / 2, aY);
				}
			}
		}
	}

	// the end of the galaxy fades in
	if (mState == STATE_GALAXY || mState == STATE_FADE)
	{
		int aTimer = mEffectOverlay->mEndTimer - 0x2008;
		int anAlpha = aTimer;
		if (aTimer >= 0xff)
			anAlpha = 0xff;
		if (anAlpha > 0)
		{
			g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)mEffectOverlay->mFade * (double)anAlpha)));
			g->SetFont(FONT_HUGE);
			std::string aText = "GALAXY COMPLETED";
			Font* aFont = g->GetFont();
			int aY = mApp->ScaleNum(200, 0);
			g->DrawString(aText, mWidth / 2 - aFont->StringWidth(aText) / 2, aY);
		}
		if (mBackLink != NULL)
		{
			anAlpha = aTimer / 2;
			if (anAlpha >= 0xff)
				anAlpha = 0xff;
			if ((unsigned int)anAlpha <= 0xff)
			{
				mBackLink->mColor = Color(0xff, 0xff, 0xff, anAlpha);
				mBackLink->mOverColor = Color(0xff, 0xff, 0xff, anAlpha);
			}
		}
	}

	// the test displays that the cheats switch on
	if (m1724)
	{
		if (mInUndoPreview)
		{
			g->SetColor(Color(0xff, 0xff, 0));
			g->DrawString("CREATING MODE", 0x12, 600);
		}
		if (mIsPuzzle)
		{
			g->SetColor(Color(100, 100, 0xff));
			g->DrawString("PUZZLE MODE", 0x12, 0x276);
			if (!mInUndoPreview)
			{
				std::string aText;
				if (!mSolutionValid)
				{
					g->SetColor(Color(0xff, 0xff, 0xff));
					aText = "Solution Unknown";
				}
				else if (mSolutionMoves < 1)
				{
					g->SetColor(Color(0xff, 0xff, 0xff));
					aText = "No Longer Solvable";
				}
				else
				{
					char aBuf[256];
					sprintf(aBuf, "%d Valid Moves", mSolutionMoves);
					g->SetColor(Color(0xff, 0xff, 0xff));
					aText = aBuf;
				}
				g->DrawString(aText, 0x12, 0x294);
			}
		}
		if (m1723)
		{
			g->SetColor(Color(0, 0xff, 0));
			g->DrawString("FREE MODE", 0x12, 0x2b2);
		}
		if (mIsAction)
		{
			g->SetColor(Color(0xff, 100, 100));
			g->DrawString("TIMED MODE", 0x12, 0x2d0);
		}
	}

	if (m1751)
	{
		mApp->ClearUpdateBacklog(false);
		m1751 = false;
	}
	if (m15de8)
	{
		mApp->PlaySample(SOUND_WHIRLPOOL);
		mApp->ClearUpdateBacklog(false);
		m15de8 = false;
	}

	// the arrow that points at the hint button while the tip dialog is up
	if (mApp->GetDialog(0x1a) != NULL)
	{
		float aPulse = (float)(1.0 - cos(m1860));
		aPulse = (float)((double)aPulse * 0.5);
		int aY = 0x28e - FTol((double)aPulse * -6.0);
		g->DrawImage(IMAGE_HELP_ARROW, mApp->ScaleNum(0x251, 0), mApp->ScaleNum(aY, 0));
		g->SetColorizeImages(true);
		g->SetDrawMode(1);
		g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)aPulse * 255.0)));
		int aSize = mApp->ScaleNumUp(0x28, 0);
		Rect aSrc(0, 0, mApp->ScaleNumUp(0x28, 0), aSize);
		g->DrawImage(IMAGE_HELP_INDICATOR_ARROW, mApp->ScaleNum(0x26f, 0), mApp->ScaleNum(aY + 0x14, 0), aSrc);
		g->SetDrawMode(0);
		g->SetColorizeImages(false);
	}

	int aBackShakeY = -mApp->ScaleNum(mShakeY, 0);
	int aBackShakeX = -mApp->ScaleNum(mShakeX, 0);
	g->Translate(aBackShakeX, aBackShakeY);
}
