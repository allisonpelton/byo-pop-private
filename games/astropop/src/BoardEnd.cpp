// Board: the end of a game. UpdateGameOver steps the white flash, the fading panels and the drifting sparks; once all
// of them are done the two buttons (continue or restart, and menu) slide in.

#include "Board.h"
#include "AstroPopApp.h"
#include "AstroButton.h"
#include "DDImage.h"
#include "GameText.h"
#include "MemoryImage.h"
#include "Particle.h"
#include "WidgetManager.h"
#include "Res.h"

using namespace Sexy;

// 0x45e680: one update of the end of a game (the Board's Update calls it while mFlag8E8 is set).
void Board::UpdateGameOver()
{
	mAnim5a0.Update();
	m1b34 = (m1b34 + 4) & 0xff;
	if (m905)
		return;

	if (m1948 > 0)
	{
		m1948 -= 2;
		mOverlay.SetFill(0xff, 0xff, 0xff, m1948);
		if (m1948 <= 0)
		{
			m1948 = 0;
			SetOverlayHidden(true);
		}
	}

	m1a80 = ((int) AstroRand() % 500000000) % 11 - 5;
	if (m1948 > 0xf0)
		return;

	// The panels fade one after the other: a panel runs only when every panel before it has reached its turn (counted
	// in aFront). A panel rising to 255 turns round at 255 and falls by 4; one that has reached 128 lets the next start.
	int aDone = 0;
	int aFront = 0;
	for (unsigned int i = 0; i < m1e0.size(); i++)
	{
		FadeRec* aRec = &m1e0[i];
		if (aRec->mValue == 0 && !aRec->mRising)
		{
			aDone++;
			aFront++;
			continue;
		}
		if (aFront != (int) i)
			continue;

		if (!aRec->mRising)
		{
			aFront++;
			aRec->mValue -= aRec->mStep;
			if (aRec->mValue <= 0)
				aRec->mValue = 0;
			continue;
		}

		aRec->mValue += aRec->mStep;
		if (aRec->mValue >= 0xff)
		{
			aRec->mValue = 0xff;
			aRec->mStep = 4;
			aRec->mRising = false;
		}
		if (aRec->mValue < 0x80)
			continue;
		aFront++;
	}

	for (unsigned int i = 0; i < me8.size(); i++)
		me8[i]->mAnim.SetStopped(false);
	for (unsigned int i = 0; i < me8.size(); i++)
	{
		SunParticle* aSpark = me8[i];
		if (!aSpark->mAnim.mStopped)
		{
			aSpark->mAnim.Update();
			aSpark->mX = aSpark->mVX + aSpark->mX;
			aSpark->mY = aSpark->mVY + aSpark->mY;
		}
	}

	if (aDone != (int) m1e0.size())
		return;
	if (m1948 != 0)
		return;
	m905 = true;
	if (m1a60 == 1 || m1a60 == 2)
		CreateEndButtons();
}

// 0x4593b0: the two buttons of the end of a game slide in: continue (restart in the other mode, id 0xfa4) and menu
// (id 0xfa5), both in the colour of the game mode. The original keeps the picture it draws the buttons from for ever;
// the port frees it once the buttons have their copies.
void Board::CreateEndButtons()
{
	m91d = true;
	mAnim5a0.SetNumFrames(0x1f);
	mAnim5a0.SetDelay(1);
	mAnim5a0.mPingPong = true;
	mAnim5a0.mHoldLast = false;
	mAnim5a0.SetStopped(false);

	AstroButton* aContinue = new AstroButton(0xfa4, mApp);
	m2e4 = aContinue;

	MemoryImage* anInset;
	MemoryImage* aButtonImage;
	mApp->MakeDialogImages(Color(150, 170, 0, 255), mApp->GetDialogColor(mMode + 2), &anInset, &aButtonImage, false);
	delete anInset;

	aContinue->mButtonImage = mApp->CopyImage(aButtonImage, Rect(0, 0, 0x78, 0x21));
	aContinue->mOverImage = mApp->CopyImage(aButtonImage, Rect(0x78, 0, 0x78, 0x21));
	aContinue->mDownImage = mApp->CopyImage(aButtonImage, Rect(0xf0, 0, 0x78, 0x21));
	aContinue->mLabel = (m1a60 == 1) ? GT(0x564b30) : GT(0x5654e4);
	aContinue->SetFont(FONT_DIALOG_BOX_BTN);
	aContinue->SetColor(0, Color(0xff, 0xff, 0));
	aContinue->SetColor(1, Color(0xff, 0xff, 0xff));

	AstroButton* aMenu = new AstroButton(0xfa5, mApp);
	m2e8 = aMenu;
	aMenu->mButtonImage = mApp->CopyImage(aButtonImage, Rect(0, 0, 0x78, 0x21));
	aMenu->mOverImage = mApp->CopyImage(aButtonImage, Rect(0x78, 0, 0x78, 0x21));
	aMenu->mDownImage = mApp->CopyImage(aButtonImage, Rect(0xf0, 0, 0x78, 0x21));
	aMenu->mLabel = GT(0x5654dc);
	aMenu->SetFont(FONT_DIALOG_BOX_BTN);
	aMenu->SetColor(0, Color(0xff, 0xff, 0));
	aMenu->SetColor(1, Color(0xff, 0xff, 0xff));

	aContinue->Resize(0xdc, 0x1b1, 0x78, aButtonImage->GetHeight());
	aMenu->Resize(0x1b0, 0x1b1, 0x78, aButtonImage->GetHeight());
	delete aButtonImage;

	mApp->mWidgetManager->AddWidget(aContinue);
	mApp->mWidgetManager->AddWidget(aMenu);
}
