#include "SaveGameDialog.h"
#include "BinFile.h"
#include "CellButton.h"
#include "ButtonWidget.h"
#include "Dynomite.h"
#include "FossilizedApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Sprite.h"
#include "WidgetManager.h"
#include <cstdio>
#include <cstring>

using namespace Sexy;

// 0x433910
SaveGameDialog::SaveGameDialog(FossilizedApp* theApp) :
	Dialog2004(theApp->mPicDialog->mImage, theApp->mPicDbutton->mImage, 6, true, theApp->GetString("SAVE_GAME"), "",
		"", Dialog::BUTTONS_NONE)
{
	mApp = theApp;
	int i;
	for (i = 0; i < 8; i++)
		mSlotCells[i] = NULL;
	for (i = 0; i < 8; i++)
		mSlotPics[i] = NULL;
	// an empty slot's picture: opaque black
	unsigned int* aBits = new unsigned int[0xe10];
	for (i = 0; i < 0xe10; i++)
		aBits[i] = 0xff000000;
	// port: the original passes a fifth argument (1) that the constructor never reads
	mEmptyPic = new Pic(mApp, aBits, 0x3c, 0x3c);
	mEmptyCell = new Cell(mEmptyPic, 0x3c, 0x3c, 0);
	delete[] aBits;
	SetButtonFont(mApp->mWhiteFontSmall);
	SetHeaderFont(mApp->mWhiteFont);
	SetLinesFont(mApp->mWhiteFontSmall);
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	SetColor(3, Color(0xff, 0xff, 0xff));

	mSaveButton = new ButtonWidget(500, mApp);
	mSaveButton->mButtonImage = mApp->mPicDbutton->mImage;
	mSaveButton->mDoFinger = true;
	mSaveButton->mLabel = mApp->GetString("SAVE");
	mSaveButton->SetFont(mApp->mWhiteFontSmall);
	mSaveButton->SetColor(0, Color(0xff, 0xff, 0xff));
	mSaveButton->SetColor(1, Color(0xff, 0xff, 0xff));

	mCancelButton = new ButtonWidget(0x1f5, mApp);
	mCancelButton->mButtonImage = mApp->mPicDbutton->mImage;
	mCancelButton->mDoFinger = true;
	mCancelButton->mLabel = mApp->GetString("DIALOG_BUTTON_CANCEL");
	mCancelButton->SetFont(mApp->mWhiteFontSmall);
	mCancelButton->SetColor(0, Color(0xff, 0xff, 0xff));
	mCancelButton->SetColor(1, Color(0xff, 0xff, 0xff));

	mChosenSlot = -1;
	ReadSlots();
}

// 0x433420: the slots' buttons and pictures; the board gets the focus back. Port: as in the original, the empty
// picture and the Save and Cancel buttons are not deleted
SaveGameDialog::~SaveGameDialog()
{
	int n = 0;
	int i = 8;
	do
	{
		if (mSlotButtons[n] != NULL)
		{
			delete mSlotButtons[n];
			mSlotButtons[n] = NULL;
		}
		if (mSlotPics[n] != NULL)
		{
			delete mSlotPics[n];
			mSlotPics[n] = NULL;
		}
		if (mSlotCells[n] != NULL)
		{
			delete mSlotCells[n];
			mSlotCells[n] = NULL;
		}
		n++;
		i--;
	} while (i != 0);
	if (mApp->mBoard != NULL)
		mApp->mWidgetManager->SetFocus(mApp->mBoard);
}

// 0x433670: SLOT0.SAV-SLOT7.SAV: a slot's mode, name and picture, or "Empty" and the empty picture
void SaveGameDialog::ReadSlots()
{
	unsigned int* aBits = new unsigned int[0xe10];
	int i = 0;
	do
	{
		strcpy(mModes[i], GTC(0x4fcb00));
		strcpy(mNames[i], GTC(0x4fcb00));
		char aName[0x14];
		sprintf(aName, "SLOT%d.SAV", i);
		if (BinFile::UserFileExists(aName))
		{
			// port: the saved games are in the user folder (the original's are beside the exe); a file that cannot be
			// opened leaves the buffer as it was
			FILE* aFile = fopen(BinFile::UserPath(aName).c_str(), "rb");
			if (aFile != NULL)
			{
				fread(mModes[i], 1, 0x14, aFile);
				fread(mNames[i], 1, 0x14, aFile);
				fread(aBits, 1, 0x3840, aFile);
				fclose(aFile);
			}
			mSlotPics[i] = new Pic(mApp, aBits, 0x3c, 0x3c);
			mSlotCells[i] = new Cell(mSlotPics[i], 0x3c, 0x3c, 0);
			mSlotButtons[i] = new CellButton(i + 600, mApp);
			mSlotButtons[i]->mBtnNoDraw = false;
			mSlotButtons[i]->mDoFinger = true;
			mSlotButtons[i]->SetVisible(true);
			mSlotButtons[i]->mButtonCell = mSlotCells[i];
			mSlotButtons[i]->mOverCell = mSlotCells[i];
			mSlotButtons[i]->mDownCell = mSlotCells[i];
		}
		else
		{
			mSlotButtons[i] = new CellButton(i + 600, mApp);
			mSlotButtons[i]->mBtnNoDraw = false;
			mSlotButtons[i]->mDoFinger = true;
			mSlotButtons[i]->SetVisible(true);
			mSlotButtons[i]->mButtonCell = mEmptyCell;
			mSlotButtons[i]->mOverCell = mEmptyCell;
			mSlotButtons[i]->mDownCell = mEmptyCell;
		}
		i++;
	} while (i < 8);
	// port: the original never frees the buffer
	delete[] aBits;
}

// 0x4334e0
int SaveGameDialog::GetPreferredHeight(int theWidth)
{
	return 0x1c2;
}

// 0x4334f0
void SaveGameDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mSaveButton);
	theWidgetManager->AddWidget(mCancelButton);
	int i = 0;
	do
	{
		theWidgetManager->AddWidget(mSlotButtons[i]);
		i++;
	} while (i < 8);
}

// 0x433540
void SaveGameDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mSaveButton);
	theWidgetManager->RemoveWidget(mCancelButton);
	int i = 0;
	do
	{
		theWidgetManager->RemoveWidget(mSlotButtons[i]);
		i++;
	} while (i < 8);
}

// 0x433590: the slots in two rows of four
void SaveGameDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mSaveButton->Resize(mX + 0x74, mY + mHeight - 0x52, 0x60, 0x20);
	mCancelButton->Resize(mX + 0xd8, mY + mHeight - 0x52, 0x60, 0x20);
	int aX = 0x3e;
	CellButton** aButton = &mSlotButtons[0];
	int i = 4;
	do
	{
		(*aButton)->Resize(mX + aX + 2, mY + 0xa2, 0x3c, 0x3c);
		aX += 0x50;
		aButton++;
		i--;
	} while (i != 0);
	aX = 0x3e;
	aButton = &mSlotButtons[4];
	i = 4;
	do
	{
		(*aButton)->Resize(mX + aX + 2, mY + 0x106, 0x3c, 0x3c);
		aX += 0x50;
		aButton++;
		i--;
	} while (i != 0);
}

// 0x433fe0: the instructions, then each slot's frame (white when chosen) under its mode and over its name
void SaveGameDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mWhiteFontSmall);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(mApp->GetString("SAVE_INSTRUCTIONS1"), 0x32, 0x55);
	g->DrawString(mApp->GetString("SAVE_INSTRUCTIONS2"), 0x32, 0x69);
	g->DrawString(mApp->GetString("SAVE_INSTRUCTIONS3"), 0x5e, 0x78);
	g->SetColor(Color(0xff, 0xff, 0xc8));
	g->DrawString(mApp->GetString("SAVE_INSTRUCTIONS1"), 0x30, 0x53);
	g->DrawString(mApp->GetString("SAVE_INSTRUCTIONS2"), 0x30, 0x67);
	g->DrawString(mApp->GetString("SAVE_INSTRUCTIONS3"), 0x5c, 0x76);
	// the original calls this through the app's mSavePic; it only sets the graphics the tiny font draws with
	SetDrawGraphics(g);
	int aX = 0x3e;
	g->SetColor(Color(0, 0, 0, 0xa0));
	g->FillRect(0x34, 0x8e, 0x143, 0x10c);
	g->SetColor(Color(0xff, 0xff, 0xc8));
	int i = 0;
	do
	{
		if (i == mChosenSlot)
			g->SetColor(Color(0xff, 0xff, 0xff));
		else
			g->SetColor(Color(0x80, 0x80, 0x64, 0x80));
		g->FillRect(aX, 0xa0, 0x40, 0x40);
		mApp->mTinyFont->Draw(mModes[i], aX - mApp->mTinyFont->Width(mModes[i]) / 2 + 0x20, 0x96);
		mApp->mTinyFont->Draw(mNames[i], aX - mApp->mTinyFont->Width(mNames[i]) / 2 + 0x20, 0xe3);
		aX += 0x50;
		i++;
	} while (i < 4);
	aX = 0x3e;
	int aCount = 4;
	do
	{
		if (i == mChosenSlot)
			g->SetColor(Color(0xff, 0xff, 0xff));
		else
			g->SetColor(Color(0x80, 0x80, 0x64, 0x80));
		g->FillRect(aX, 0x104, 0x40, 0x40);
		mApp->mTinyFont->Draw(mModes[i], aX - mApp->mTinyFont->Width(mModes[i]) / 2 + 0x20, 0xfa);
		mApp->mTinyFont->Draw(mNames[i], aX - mApp->mTinyFont->Width(mNames[i]) / 2 + 0x20, 0x147);
		aX += 0x50;
		i++;
		aCount--;
	} while (aCount != 0);
}
