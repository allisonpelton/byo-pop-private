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
#include "UserProfile.h"

using namespace Sexy;

// 0x45e680: one update of the end of a game (the Board's Update calls it while mGameOver is set).
void Board::UpdateGameOver()
{
	mResultPulseAnim.Update();
	m1b34 = (m1b34 + 4) & 0xff;
	if (mGameOverAnimDone)
		return;

	if (mEndFlashAlpha > 0)
	{
		mEndFlashAlpha -= 2;
		mOverlay.SetFill(0xff, 0xff, 0xff, mEndFlashAlpha);
		if (mEndFlashAlpha <= 0)
		{
			mEndFlashAlpha = 0;
			SetOverlayHidden(true);
		}
	}

	mScreenShakeX = ((int) AstroRand() % 500000000) % 11 - 5;
	if (mEndFlashAlpha > 0xf0)
		return;

	// The panels fade one after the other: a panel runs only when every panel before it has reached its turn (counted
	// in aFront). A panel rising to 255 turns round at 255 and falls by 4; one that has reached 128 lets the next start.
	int aDone = 0;
	int aFront = 0;
	for (unsigned int i = 0; i < mEndBlastFades.size(); i++)
	{
		FadeRec* aRec = &mEndBlastFades[i];
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

	for (unsigned int i = 0; i < mSunSparks.size(); i++)
		mSunSparks[i]->mAnim.SetStopped(false);
	for (unsigned int i = 0; i < mSunSparks.size(); i++)
	{
		SunParticle* aSpark = mSunSparks[i];
		if (!aSpark->mAnim.mStopped)
		{
			aSpark->mAnim.Update();
			aSpark->mX = aSpark->mVX + aSpark->mX;
			aSpark->mY = aSpark->mVY + aSpark->mY;
		}
	}

	if (aDone != (int) mEndBlastFades.size())
		return;
	if (mEndFlashAlpha != 0)
		return;
	mGameOverAnimDone = true;
	if (mGameType == 1 || mGameType == 2)
		CreateEndButtons();
}

// 0x4593b0: the two buttons of the end of a game slide in: continue (restart in the other mode, id 0xfa4) and menu
// (id 0xfa5), both in the colour of the game mode. The original keeps the picture it draws the buttons from for ever;
// the port frees it once the buttons have their copies.
void Board::CreateEndButtons()
{
	mEndPageShown = true;
	mResultPulseAnim.SetNumFrames(0x1f);
	mResultPulseAnim.SetDelay(1);
	mResultPulseAnim.mPingPong = true;
	mResultPulseAnim.mHoldLast = false;
	mResultPulseAnim.SetStopped(false);

	AstroButton* aContinue = new AstroButton(0xfa4, mApp);
	mContinueButton = aContinue;

	MemoryImage* anInset;
	MemoryImage* aButtonImage;
	mApp->MakeDialogImages(Color(150, 170, 0, 255), mApp->GetDialogColor(mMode + 2), &anInset, &aButtonImage, false);
	delete anInset;

	aContinue->mButtonImage = mApp->CopyImage(aButtonImage, Rect(0, 0, 0x78, 0x21));
	aContinue->mOverImage = mApp->CopyImage(aButtonImage, Rect(0x78, 0, 0x78, 0x21));
	aContinue->mDownImage = mApp->CopyImage(aButtonImage, Rect(0xf0, 0, 0x78, 0x21));
	aContinue->mLabel = (mGameType == 1) ? GT(0x564b30) : GT(0x5654e4);
	aContinue->SetFont(FONT_DIALOG_BOX_BTN);
	aContinue->SetColor(0, Color(0xff, 0xff, 0));
	aContinue->SetColor(1, Color(0xff, 0xff, 0xff));

	AstroButton* aMenu = new AstroButton(0xfa5, mApp);
	mMenuButton = aMenu;
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

// 0x432e90: how many upgrades of the weapons the player has taken in this mode (the levels above 1 of the settings 0x33 +
// mode, 0x3b + mode and, for modes 0 and 3, 0x37 + mode)
int Board::WeaponLevelSum()
{
	int aPower = 1;
	int aSpeed = 1;
	mApp->mUser->GetInt(mMode + 0x33, &aPower);
	aPower--;
	if (aPower < 0)
		aPower = 0;
	if (mMode == 0 || mMode == 3)
	{
		mApp->mUser->GetInt(mMode + 0x37, &aSpeed);
		aSpeed--;
		if (aSpeed < 0)
			aSpeed = 0;
		aPower += aSpeed;
	}
	aSpeed = 0;
	mApp->mUser->GetInt(mMode + 0x3b, &aSpeed);
	aSpeed--;
	if (aSpeed < 0)
		aSpeed = 0;
	return aPower + aSpeed;
}

// 0x43e050: the result of a game is kept. After a game over it goes into the hall of fame and the player's best score and
// record lists (and the saved game is dropped); when the game is left in the middle the progress is stored instead.
void Board::SaveFinalScore()
{
	if (mGameOver)
	{
		std::string aName;
		mApp->mUser->GetString(PROFILE_NAME, &aName);
		mInHallOfFame = mApp->AddHallOfFame(aName, mScore, mLevelNumber, mMode, 1);

		int aBest = mScore;
		mApp->mUser->GetInt(mMode, &aBest);
		if (mScore > aBest)
		{
			mApp->mUser->SetInt(mMode, mScore);
			mApp->mUser->SetInt(mMode + 4, mLevelNumber);
		}

		ProfileRecord aRecord;
		aRecord.mField[0] = 1;
		aRecord.mField[1] = mMode;
		aRecord.mField[2] = mScore;
		aRecord.mField[3] = mLevelNumber;
		aRecord.mField[4] = -1;
		aRecord.mField[5] = WeaponLevelSum();
		m1b30 = mApp->mUser->AddRecord(aRecord);
		mApp->mUser->ResetGame();
	}
	else
		StoreProgress();
	mApp->mUser->Save();
}

// 0x450f80: the game is over. The buttons of the field go, the result is kept (a game of the first kind: the best level
// of the mode, rounded down to a multiple of four, is looked up for the screen and the hints are reset; a timed game:
// the time goes into the lists, the hall of fame and the mode's best time), the field is cleared, and the screen
// flashes white.
void Board::GameOver()
{
	// (the original writes a line with the level to its trace list)
	mApp->PlaySample(SOUND_VGAMEOVER);
	mApp->PlaySample(SOUND_MEGA_EXPLOSION);
	mGameOver = true;
	for (unsigned int i = 0; i < mFieldButtons.size(); i++)
		mFieldButtons[i]->SetDisabled(true);
	mHelpButton->SetDisabled(true);
	for (unsigned int i = 0; i < mFieldButtons.size(); i++)
		mFieldButtons[i]->SetVisible(false);
	mHelpButton->SetVisible(false);

	if (mGameType == 1)
	{
		int aBestLevel = 0;
		mApp->mUser->GetInt(mMode + 8, &aBestLevel);
		if (aBestLevel < 4)
			m1b6c = -1;
		else
			m1b6c = (aBestLevel / 4) * 4;
		mApp->mUser->GetInt(0x5d, &mBestChain);
		mApp->mUser->SetInt(0x5d, 0);
		mApp->mUser->ResetHintsShown();
		SaveFinalScore();
	}
	else if (mGameType == 2)
	{
		mPlayTime = mRampTime;
		ProfileRecord aRecord;
		aRecord.mField[0] = 2;
		aRecord.mField[1] = mMode;
		aRecord.mField[2] = mScore;
		aRecord.mField[3] = -1;
		aRecord.mField[4] = mRampTime / 100;
		aRecord.mField[5] = WeaponLevelSum();
		m1b30 = mApp->mUser->AddRecord(aRecord);

		std::string aName;
		mApp->mUser->GetString(PROFILE_NAME, &aName);
		mInHallOfFame = mApp->AddHallOfFame(aName, mScore, mPlayTime / 100, mMode, 2);

		int aBestTime = 0;
		mApp->mUser->GetInt(mMode + 0x67, &aBestTime);
		if (aBestTime < mPlayTime / 100)
		{
			aBestTime = mPlayTime / 100;
			mApp->mUser->SetInt(mMode + 0x67, aBestTime);
		}
	}

	mApp->PlaySong(0x27);
	mEndFlashAlpha = 0xff;
	mOverlay.SetFill(255, 255, 255, 255);
	SetOverlayHidden(false);
	ClearField();
	mPlayedBonusStages.clear();
}
