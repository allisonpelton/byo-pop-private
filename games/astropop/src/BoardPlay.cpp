// Board: pausing (a dialog is open), the theme buttons, and the first move that starts the play of a mode.

#include "Board.h"
#include "Brick.h"
#include "AstroPopApp.h"
#include "AstroButton.h"
#include "UserProfile.h"
#include "GameText.h"
#include "Checkbox.h"
#include "DDImage.h"
#include "SoundInstance.h"
#include "WidgetManager.h"
#include "Image.h"
#include "Res.h"
#include <algorithm>

using namespace Sexy;

// 0x450c00: the cells in order of their row, those of a row in order of their column (the original sorts by inserting
// each into a second list; the result is the same)
static bool CellBefore(const CellRef& theA, const CellRef& theB)
{
	if (theA.mRow != theB.mRow)
		return theA.mRow < theB.mRow;
	return theA.mCol < theB.mCol;
}

void Board::SortCells(std::vector<CellRef>& theCells)
{
	std::sort(theCells.begin(), theCells.end(), CellBefore);
}

// 0x459040: the Board is paused while a dialog is open. Pause(true) starts a pause (unless the pause dialog, id 100, is
// open already); Pause(false) ends it when that dialog is gone. The count of pauses is m1924; only the first pause
// stops the sounds and hides the buttons and only the last resumes them.
void Board::Pause(bool theStart, bool theFlag)
{
	m8f7 = theFlag;
	if (!theStart || mApp->GetDialog(100) != NULL)
	{
		if (mApp->GetDialog(100) == NULL)
		{
			m1924--;
			if (m1924 == 0)
			{
				if (m2e0 != NULL)
					m2e0->SetVisible(true);
				SoundInstance* aKlaxon = mApp->mLoopSounds[7];
				if (aKlaxon->IsPlaying())
					aKlaxon->SetVolume(1.0);
				m1af4 = mUpdateCnt;
				// (the original writes "End of pause" to its trace list here)
				if (!m1c09)
					mApp->SetLoopSoundsMuted(false);
				mApp->PauseMusicForFocus(false);
				m1928 = 0;
				if (!mOverlay.mFlag80)
					SetOverlayHidden(true);
				if (!mFlag8E8)
				{
					m1c0[0]->SetDisabled(false);
					m1c0[0]->SetVisible(true);
					m2ec->SetDisabled(false);
					m2ec->SetVisible(true);
				}
				if (m2e4 != NULL)
				{
					m2e4->SetDisabled(false);
					mApp->mWidgetManager->BringToFront(m2e4);
				}
				if (m2e8 != NULL)
				{
					m2e8->SetDisabled(false);
					mApp->mWidgetManager->BringToFront(m2e8);
				}
				mApp->mWidgetManager->SetFocus(this);
				if (m6e4 != NULL)
					m6e4->SetVolume(1.0);
				if (m8df)
				{
					m8df = false;
					BeginPlay();
				}
			}
		}
		return;
	}

	m1924++;
	if (m1924 != 1)
		return;
	if (m2e0 != NULL)
		m2e0->SetVisible(false);
	// (the original writes "Start of pause" to its trace list here)
	m1af0 = mUpdateCnt;
	mApp->SetLoopSoundsMuted(true);
	SetOverlayHidden(false);
	if (m6e4 != NULL)
		m6e4->SetVolume(0.0);
	SoundInstance* aKlaxon = mApp->mLoopSounds[7];
	if (aKlaxon->IsPlaying())
		aKlaxon->SetVolume(0.0);
	m1c0[0]->SetDisabled(true);
	m1c0[0]->SetVisible(false);
	m2ec->SetDisabled(true);
	m2ec->SetVisible(false);
	if (m2e4 != NULL)
	{
		m2e4->SetDisabled(true);
		mApp->mWidgetManager->PutBehind(m2e4, &mOverlay);
	}
	if (m2e8 != NULL)
	{
		m2e8->SetDisabled(true);
		mApp->mWidgetManager->PutBehind(m2e8, &mOverlay);
	}
}

// 0x437910: the pictures of the theme button and the checkbox are made again in the colour of the stage
void Board::RecolorThemeButtons()
{
	delete m74c->mOverImage;
	delete m74c->mDownImage;
	delete m74c->mButtonImage;
	delete m750->mUncheckedImage;

	StageBg* aStage = m230[0]->mBg;
	Color aColor(aStage->mColor1.mRed, aStage->mColor1.mGreen, aStage->mColor1.mBlue);
	m74c->mDownImage = mApp->CreateColorizedImage(IMAGE_TH_BTN_DOWN, aColor);
	m74c->mOverImage = mApp->CreateColorizedImage(IMAGE_TH_BTN_OVER, aColor);
	m74c->mButtonImage = mApp->CreateColorizedImage(IMAGE_TH_BTN_UP, aColor);
	if (aColor.mRed != 0)
		aColor.mRed = 0xff;
	if (aColor.mGreen != 0)
		aColor.mGreen = 0xff;
	if (aColor.mBlue != 0)
		aColor.mBlue = 0xff;
	m750->mUncheckedImage = mApp->CreateColorizedImage(IMAGE_TH_CHECKBOX, aColor);
	m750->mCheckedImage = m750->mUncheckedImage;
}

// 0x45ab90: once the warp has faded in, the theme buttons (to keep or change the stage's colour theme) appear
void Board::ShowThemeButtons()
{
	if (!m230.empty() && m230[0]->m1c && m7e8 <= -m7ec)
	{
		Pause(true, false);
		RecolorThemeButtons();
		m74c->Resize(0x1c2, (IMAGE_TH_STRIP->mHeight - IMAGE_TH_CHECKBOX->mHeight) + 0xa9, IMAGE_TH_BTN_OVER->mWidth, IMAGE_TH_BTN_OVER->mHeight);
		m750->SetVisible(true);
		m74c->SetVisible(true);
		mApp->mWidgetManager->BringToFront(m74c);
		mApp->mWidgetManager->BringToFront(m750);
	}
}

// 0x457930: the player's first move of a level. Which kind of play starts depends on the mode: mode 0 simply plays;
// mode 1 (the SUPA weapon is the sprocket) marks the bricks it will destroy, mode 2 (the turbot) marks the stone and
// steel ones, mode 3 lights the cell under the pointer. Nothing starts while the field is not at rest.
void Board::BeginPlay()
{
	if (mf8.empty() && m1a34 != 1)
	{
		int aValue = 0;
		mApp->mUser->GetInt(mMode + 0x2a, &aValue);
		m1c08 = aValue == 1;

		if (mMode == 0)
			m1a34 = 1;
		else if (mMode == 1)
		{
			Fire(0x28, 1);
			Fire(0x2a, 1);
			m8f9 = false;

			// the idle bricks with free space under them (or on the bottom row) and those that are buried
			std::vector<CellRef> anExposed;
			std::vector<CellRef> aBuried;
			for (int aCol = 0; aCol < 10; aCol++)
			{
				for (unsigned int aRow = 1; aRow < 0x14; aRow++)
				{
					Brick* aBrick = m314[aRow][aCol];
					if (aBrick == NULL)
						break;
					if (aBrick->IsIdle())
					{
						CellRef aCell;
						aCell.mCol = aCol;
						aCell.mRow = aRow;
						unsigned int aNumRows = (unsigned int) m314.size();
						if (aRow < aNumRows - 1 && m314[aRow + 1][aCol] == NULL)
							anExposed.push_back(aCell);
						else if (aRow == aNumRows - 1)
							anExposed.push_back(aCell);
						else if (aRow < aNumRows - 2 && m314[aRow + 2][aCol] == NULL && m314[aRow + 1][aCol] != NULL)
							anExposed.push_back(aCell);
						else
							aBuried.push_back(aCell);
					}
				}
			}

			if (anExposed.empty() && aBuried.empty())
			{
				mApp->ShowMessageDialog("", GT(0x56547c));
				return;
			}

			m1a34 = 1;

			// the buried ones are thinned out at random: only the share that the power level asks for is kept
			float aKeepShare = 0.0f;
			switch (m1a8c)
			{
			case 1:		aKeepShare = 0.05f;	break;
			case 2:		aKeepShare = 0.1f;	break;
			case 3:		aKeepShare = 0.18f;	break;
			case 4:		aKeepShare = 0.24f;	break;
			case 5:		aKeepShare = 0.28f;	break;
			case 6:		aKeepShare = 0.31f;	break;
			case 7:		aKeepShare = 0.35f;	break;
			}
			int aRemove = (int) ((1.0 - (double) aKeepShare) * (double) aBuried.size());
			while (aRemove > 0 && !aBuried.empty())
			{
				unsigned int anIndex = ((unsigned int) ((int) AstroRand() % 500000000)) % (unsigned int) aBuried.size();
				aBuried.erase(aBuried.begin() + anIndex);
				aRemove--;
			}

			for (unsigned int i = 0; i < anExposed.size(); i++)
				aBuried.push_back(anExposed[i]);
			if (!aBuried.empty())
			{
				SortCells(aBuried);
				for (unsigned int i = 0; i < aBuried.size(); i++)
				{
					CellRef3 aMark;
					aMark.mCol = aBuried[i].mCol;
					aMark.mRow = aBuried[i].mRow;
					aMark.mBrick = m314[aMark.mRow][aMark.mCol];
					m1a0.push_back(aMark);
				}
			}
			m1a3c = 0;
		}
		else if (mMode == 2)
		{
			Fire(0x28, 1);
			Fire(0x2a, 1);
			m1b58 = 0xf;
			m290.clear();
			m1b54 = 0;
			m1b4c = 0;
			m1b50 = 0;
			m1a4c = 0;
			mOverlay.SetFill(255, 255, 255, 0);
			SetOverlayHidden(false);
			m1a48 = mHeight;
			mAnim520.SetStopped(false);
			m1aa0 = 0;
			m907 = false;
			for (int aCol = 0; aCol < 10; aCol++)
			{
				for (int aRow = 1; aRow < 0x14; aRow++)
				{
					Brick* aBrick = m314[aRow][aCol];
					if (aBrick == NULL)
						break;
					int aMutated = aBrick->mMutated;
					if ((aMutated != 1 && (aBrick->mType == 7 || (aMutated != 1 && aBrick->mType == 9))) || aMutated == 0 || aMutated == 1)
					{
						int aType = 0xc;
						if (aMutated != 1)
							aType = aBrick->mType;
						if ((1 < m1a8c && (aType == 7 || aType == 9)) || (3 < m1a8c && (aMutated == 0 || aMutated == 1)))
						{
							CellRef aCell;
							aCell.mCol = aCol;
							aCell.mRow = aRow;
							mc8.push_back(aCell);
							if (4 < m1a8c)
							{
								aBrick->mFlag187 = true;
								aBrick->AdvanceParticleCount();
							}
						}
					}
				}
			}
			m1a34 = 1;
			SortCells(mc8);
			mApp->PlaySample(SOUND_EXPLOSION);
		}
		else if (mMode == 3)
		{
			int aX = m1958;
			m1a34 = 1;
			if (aX > 0xa5 && aX < 0x25e)
			{
				unsigned int aRow = 0;
				if (m195c >= 0x22)
					aRow = (m195c - 0x22) / 0x19 + 1;
				HoverCell((aX - 0xa6) / 0x2c, aRow, false);
			}
		}

		mApp->PlaySample(SOUND_POWER_ACTIVATED);
		m6fc = 0;
		m6f4 = 0;
		m6f0 = 0;
		m6f8 = 0x35;
		if (!m8fc)
		{
			if (mMode != 1 && mMode != 2)
			{
				Fire(0x2a, 0);
				Fire(0x28, 0);
			}
			if (!m8fc)
				m1960 = 0xff;
		}
		m8fc = true;
		m922 = false;
		m1b24 = 0;
		m1ab8 = 6;
		m1ac0 = PickSpawnDelay();
		m1ad0 = 0;
		m1aec = 0;
		m1ae0 = 0xff;
		m1adc = 0;
		m1ad8 = -IMAGE_SUPA_BRACKET_LEFT->mHeight;
		m1ad4 = 1;
		aValue = 0;
		mApp->mUser->GetInt(0x2e, &aValue);
		if (aValue == 1)
		{
			mApp->mUser->SetInt(0x2e, 0);
			mApp->mUser->Save();
			return;
		}
	}
	else if (!m90b)
		mApp->PlaySample(SOUND_CANT_CLICK);
}
