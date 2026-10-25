#include "OverlayWidget.h"
#include "BookWormApp.h"
#include "Res.h"
#include "GameText.h"
#include "Board.h"
#include "Piece.h"
#include "WordScore.h"
#include "Lex.h"
#include "WordBalloon.h"
#include "Effect.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"

#include <math.h>

using namespace Sexy;

// 0x52f640: the balloon's colours while it shows the bonus word (Widget::SetColors [][4]): white, black,
// black, black
static int gBonusBalloonColors[4][4] =
{
	{255, 255, 255, 255},
	{0, 0, 0, 255},
	{0, 0, 0, 255},
	{0, 0, 0, 255}
};

// 0x430c20
OverlayWidget::OverlayWidget(BookWormApp* theApp)
{
	mApp = theApp;
	mMouseVisible = false;
	mHasAlpha = true;
	mBalloon = NULL;
	mShowBalloon = false;
	mBalloonUp = false;
	mBannerTargetY = 0;
	mBannerY = 0;
	mBannerActive = false;
	mArrowFade = 0.0;
	mBannerRect = Rect(0, 0, 0, 0);
	mEffectMgr = new EffectMgr(mApp); // one Sexy::Rand() draw (NAMES.md 2.11)
}

// 0x430970
OverlayWidget::~OverlayWidget()
{
	delete mEffectMgr;
	mEffectMgr = NULL;

	if (mBalloon != NULL)
	{
		mApp->mWidgetManager->RemoveWidget(mBalloon);
		delete mBalloon;
		mBalloon = NULL;
	}
}

// 0x430d10: creates Lex's balloon (the thought / speech balloon every Lex message uses) above this widget
void OverlayWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	if (mBalloon == NULL)
	{
		Image** anImages = mApp->mImages;
		mBalloon = new WordBalloon(mApp, anImages[IMAGE_BACKDROP_WB_TEXT], anImages[IMAGE_WB_TEXT_TALK_TAIL],
			Point((int)(0.75 * BALLOON_WIDTH), -2));
		mBalloon->Resize(0, 0, BALLOON_WIDTH, mBalloon->GetPreferredHeight(BALLOON_WIDTH));
		theWidgetManager->AddWidget(mBalloon);
	}

	Image* aBanner = mApp->mImages[IMAGE_BACKDROP_BONUS];
	mBannerRect = Rect(10, mBannerY, aBanner->GetWidth(), aBanner->GetHeight());
}

// 0x430bb0
void OverlayWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	if (mBalloon != NULL)
		theWidgetManager->RemoveWidget(mBalloon);
}

// 0x430b90
void OverlayWidget::OrderInManagerChanged()
{
	if (mBalloon != NULL)
		mWidgetManager->PutInfront(mBalloon, this);
}

// 0x430a00
bool OverlayWidget::IsBannerMoving()
{
	return mBannerY != mBannerTargetY;
}

// 0x430a20: the effect is owned by the manager from here on (deleted when there is none)
void OverlayWidget::AddEffect(Effect* theEffect)
{
	// 0x430a29: OutputDebugString("(mEffectMgr == NULL)!") when there is no manager (port: debugger output, dropped)
	if (mEffectMgr != NULL)
	{
		theEffect->mEffectMgr = mEffectMgr;
		mEffectMgr->AddEffect(theEffect);
		return;
	}

	delete theEffect;
}

// 0x430a70
std::list<Effect*>* OverlayWidget::GetEffects(int theType)
{
	if (mEffectMgr == NULL)
		return NULL;
	return mEffectMgr->GetEffectsOfType(theType);
}

// 0x430a90
bool OverlayWidget::HasEffects(int theType)
{
	if (mEffectMgr == NULL)
		return false;
	return mEffectMgr->HasEffectOfType(theType);
}

// 0x430ab0: part of the board's saved game (Board::SaveGame): 33 bytes, little-endian int mBannerTargetY,
// int mBannerY, byte mBannerActive, double mArrowFade, 4 ints mBannerRect, then the effects
void OverlayWidget::SaveState(FILE* theFile)
{
	fwrite(&mBannerTargetY, 1, 4, theFile);
	fwrite(&mBannerY, 1, 4, theFile);
	fwrite(&mBannerActive, 1, 1, theFile);
	fwrite(&mArrowFade, 1, 8, theFile);
	fwrite(&mBannerRect.mX, 1, 4, theFile);
	fwrite(&mBannerRect.mY, 1, 4, theFile);
	fwrite(&mBannerRect.mWidth, 1, 4, theFile);
	fwrite(&mBannerRect.mHeight, 1, 4, theFile);
	mEffectMgr->Save(theFile);
}

// 0x430b20
void OverlayWidget::LoadState(FILE* theFile)
{
	fread(&mBannerTargetY, 1, 4, theFile);
	fread(&mBannerY, 1, 4, theFile);
	fread(&mBannerActive, 1, 1, theFile);
	fread(&mArrowFade, 1, 8, theFile);
	fread(&mBannerRect.mX, 1, 4, theFile);
	fread(&mBannerRect.mY, 1, 4, theFile);
	fread(&mBannerRect.mWidth, 1, 4, theFile);
	fread(&mBannerRect.mHeight, 1, 4, theFile);
	mEffectMgr->Clear();
	mEffectMgr->Load(theFile);
}

// 0x430e20
void OverlayWidget::Update()
{
	// Nothing moves (not even mUpdateCnt) without a board or while it is paused.
	Board* aBoard = mApp->mBoard;
	if (aBoard == NULL)
		return;
	Lex* aLex = aBoard->mLex;
	if (aBoard->mPauseCount > 0)
		return;

	Widget::Update();

	if ((aLex != NULL) && (mBalloon != NULL))
	{
		Point aMouse(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY);
		if (mBalloon->mVisible)
		{
			// Keep the balloon centred above Lex; the bonus-word text closes when the mouse leaves the banner
			mBalloon->Move((aLex->mWidth - mBalloon->mWidth) / 2 + aLex->mX, aLex->mY - mBalloon->mHeight + 30);
			if (mBalloonUp && !mBannerRect.Contains(aMouse))
			{
				mBalloonUp = false;
				mBalloon->Show(0);
			}
		}
		else if (mBannerActive && !mShowBalloon && !aBoard->mSummaryShown && mBannerRect.Contains(aMouse))
		{
			mShowBalloon = true;
		}
	}

	if (mEffectMgr != NULL)
		mEffectMgr->Update();

	if (mArrowFade > 0.0)
	{
		double aFade = mArrowFade - 0.025;
		if (aFade < 0.0)
			aFade = 0.0;
		mArrowFade = aFade;
		MarkDirty();
	}

	// The banner slides 2 pixels every other update. An overshoot moving down (only possible with an odd
	// distance) also deactivates the banner; 406 <-> 480 never overshoots, so a hidden banner stays active.
	if ((mBannerY != mBannerTargetY) && (mUpdateCnt % 2 == 0))
	{
		if (mBannerY < mBannerTargetY)
		{
			mBannerY += 2;
			if (mBannerY > mBannerTargetY)
			{
				mBannerActive = false;
				mBannerY = mBannerTargetY;
			}
		}
		else
		{
			mBannerY -= 2;
			if (mBannerY < mBannerTargetY)
				mBannerY = mBannerTargetY;
		}
		mBannerRect.mX = 10;
		mBannerRect.mY = mBannerY;
		MarkDirty();
	}

	if (mShowBalloon)
	{
		mShowBalloon = false;
		// The bonus word in dark green inside Lex's thought balloon, until the mouse leaves the banner
		SexyString aText = StrFormat(GameTextC(GT_BONUS_WORD_BALLOON_FORMAT), aBoard->mBonusWord.c_str());
		aLex->Think(aText, mApp->mFonts[FONT_VERDANA12BOLD], -1);
		mBalloon->mTextInsets = Insets(-5, -10, -5, 0);
		mBalloon->SetColors(gBonusBalloonColors, 4);
		mBalloonUp = true;
	}
}

// 0x431080
void OverlayWidget::Draw(Graphics* g)
{
	Board* aBoard = mApp->mBoard;

	if (mEffectMgr != NULL)
		mEffectMgr->Draw(g);

	if ((mApp == NULL) || (aBoard == NULL))
		return;

	Image** anImages = mApp->mImages;

	// The bonus-word banner: the word in gold, the letters of the current selection that start it
	// overprinted in white, and while playing the three arrows pointing at it, pulsing and fading out
	if (mBannerActive)
	{
		g->DrawImage(anImages[IMAGE_BACKDROP_BONUS], 10, mBannerY);
		if (aBoard->mBonusWord.length() != 0)
		{
			SexyString aWord = Upper(aBoard->mBonusWord);
			g->SetFont(mApp->mFonts[FONT_COURIERFINALDRAFT20BOLD]);
			g->SetColor(Color(216, 194, 46));
			g->DrawString(aWord, 112 - g->GetFont()->StringWidth(aWord) / 2, mBannerY + 25);

			if (aBoard->mWord.mWord.length() != 0)
			{
				SexyString aPrefix = aBoard->mWord.CommonPrefix(aWord);
				if (aPrefix.length() != 0)
				{
					g->SetColor(Color(255, 255, 255, 255));
					g->DrawString(Upper(aPrefix), 112 - g->GetFont()->StringWidth(aWord) / 2, mBannerY + 25);
				}
			}

			if ((aBoard->mState == BOARD_PLAYING) && (mArrowFade > 0.0) && (mBannerY != mApp->mHeight))
			{
				// port: cos() for the original's x87 fcos. 6.28318 is not 2 pi (the original's constant).
				double aWave = cos((double)(mUpdateCnt % 48) * 0.020833333333333332 * -6.28318) * 2.5;
				double aLeftX = 4.0 + aWave;
				double aRightX = 200.0 - aWave;
				double aTopY = (double)(mBannerY - 38) + aWave;
				int anAlpha = (int)(mArrowFade * 255.0);
				if (anAlpha >= 256)
					anAlpha = 255;

				g->SetColorizeImages(true);
				g->SetColor(Color(255, 255, 255, anAlpha));
				g->DrawImage(anImages[IMAGE_BONUSARROW_LEFT], (int)aLeftX, mBannerY);
				g->DrawImage(anImages[IMAGE_BONUSARROW_RIGHT], (int)aRightX, mBannerY);
				g->DrawImage(anImages[IMAGE_BONUSARROW_TOP], 100, (int)aTopY);
				g->SetColorizeImages(false);
			}
		}
	}

	// Lex's sweat drops (8 frames stacked vertically) while he sweats in danger, unless the board burned
	Lex* aLex = mApp->mBoard->mLex;
	if ((aLex != NULL) && aLex->mSweating && (aLex->mAnim == LEXANIM_SWEAT) && !mApp->mBoard->mBurnedDown)
	{
		Image* aSweat = anImages[IMAGE_FX_LEX_SWEAT_SEQ];
		int aFrameHeight = aSweat->GetHeight() / 8;
		g->DrawImage(aSweat, aLex->mX, aLex->mY,
			Rect(0, aLex->mSweatFrame * aFrameHeight, aSweat->GetWidth(), aFrameHeight));
	}

	// The pause box over the right part of the screen, while paused with no dialog or help screen up
	if ((mApp->mBoard->mPauseFade > 0.0) && mApp->mBoard->mPauseNoDialogs)
	{
		g->DrawImageBox(Rect(233, 150, mWidth - 233, 150), anImages[IMAGE_DIALOG]);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int)(mApp->mBoard->mPauseFade * 255.0)));

		ImageFont* aFont = mApp->mFonts[FONT_LOCKERGNOMESMALLCOLOR];
		g->SetFont(aFont);
		SexyString aString = GameText(GT_PAUSED);
		g->DrawString(aString, ((mWidth - aFont->StringWidth(aString)) + 233) / 2, 210);

		aFont = mApp->mFonts[FONT_LOCKERGNOME18];
		g->SetFont(aFont);
		aString = GameText(GT_PAUSED_CONTINUE);
		g->DrawString(aString, ((mWidth - aFont->StringWidth(aString)) + 233) / 2, 250);
		g->SetColorizeImages(false);
	}

	// The fire column above the bottom fire tile: a horizontally scrolling flame strip clipped to a column
	// mFireLineAlpha * 2 * mWidth wide, whose 8-pixel edge strips are squashed to 3/4, 2/4 and 1/4 height
	Piece* aFirePiece = aBoard->mFireLinePiece;
	if (aFirePiece != NULL)
	{
		double aHalfWidth = (double)mWidth * aBoard->mFireLineAlpha;
		int aColumnWidth = (int)(aHalfWidth + aHalfWidth);
		int aColumnX = aBoard->ColX(aFirePiece->mCol) + 28 - aColumnWidth / 2;
		int aHeight = mHeight;
		Image* aFire = anImages[IMAGE_FX_FIRELINE];
		int aLeftX = aColumnX + 16;
		int aRightX = aColumnX - 24 + aColumnWidth;

		for (int aQuarters = 3; aQuarters > 0; aQuarters--)
		{
			for (int aSide = 0; aSide < 2; aSide++)
			{
				int aStripX = (aSide != 0) ? aRightX : aLeftX;
				Graphics aStripG(*g);
				aStripG.ClipRect(aColumnX, 0, aColumnWidth, aHeight);
				aStripG.ClipRect(aStripX, 0, 8, aHeight);
				int aStripHeight = (aFire->GetHeight() * aQuarters) / 4;
				int aY = (57 - (aStripHeight * 50) / aFire->GetHeight()) + aBoard->RowY(aFirePiece->mCol, aFirePiece->mRow);
				aStripG.SetFastStretch(true);
				aStripG.DrawImage(aFire, Rect(-aBoard->mFireLineScroll, aY, aFire->GetWidth(), aStripHeight),
					Rect(0, 0, aFire->GetWidth(), aFire->GetHeight()));
				aStripG.DrawImage(aFire, Rect(aFire->GetWidth() - aBoard->mFireLineScroll, aY, aFire->GetWidth(), aStripHeight),
					Rect(0, 0, aFire->GetWidth(), aFire->GetHeight()));
				aStripG.SetFastStretch(false);
			}
			aLeftX -= 8;
			aRightX += 8;
		}

		Graphics aCoreG(*g);
		aCoreG.ClipRect(aColumnX + 24, 0, aColumnWidth - 48, aHeight);
		int aY = (57 - (aFire->GetHeight() * 50) / aFire->GetHeight()) + aBoard->RowY(aFirePiece->mCol, aFirePiece->mRow);
		aCoreG.DrawImage(aFire, -aBoard->mFireLineScroll, aY, Rect(0, 0, aFire->GetWidth(), aFire->GetHeight()));
		aCoreG.DrawImage(aFire, aFire->GetWidth() - aBoard->mFireLineScroll, aY, Rect(0, 0, aFire->GetWidth(), aFire->GetHeight()));
	}

	// The white flash when the board burns
	if (aBoard->mBoardBurnAlpha > 0.0)
	{
		int aLevel = (int)(aBoard->mBoardBurnAlpha * 255.0);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(aLevel, aLevel, aLevel));
		g->FillRect(0, 0, mWidth, mHeight);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}
