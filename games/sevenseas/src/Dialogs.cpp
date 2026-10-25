// The dialogs of Seven Seas Deluxe (see Dialogs.h).

#include "Dialogs.h"
#include "SevenSeasApp.h"
#include "Screens.h"
#include "GameText.h"
#include "Board.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"

#include <algorithm>
#include <cstdio>

using namespace Sexy;

// the common look of the larger dialogs: the framed box with the mermaids on top and the skull at the bottom
// (the first lines of the draw functions 0x4192e0 and 0x417ca0)
static void DrawFramed(Graphics* g, SevenSeasApp* theApp, int theWidth, int theHeight)
{
	g->DrawImageBox(Rect(0, 0x37, theWidth, theHeight - 0x37), theApp->mImages[IMAGE_DIALOG]);
	g->DrawImage(theApp->mImages[IMAGE_MERMAIDS], theWidth / 2 - 0x53, 0);
	g->DrawImage(theApp->mImages[IMAGE_DIALOGSKULL], theWidth / 2 - 0x14, theHeight - 0x1f);
}

// ---------------------------------------------------------------------------------------------------------
// OptionsDialog

// 0x418760
OptionsDialog::OptionsDialog(SevenSeasApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_OPTIONSBUTTON], DIALOG_OPTIONS, true,
		GT(0x004a3f6c), "", GT(0x004a3590), BUTTONS_FOOTER),
	mApp(theApp)
{
	SetButtonFont(mApp->mFonts[FONT_MT11]);
	SetHeaderFont(mApp->mFonts[FONT_FONT1]);
	SetLinesFont(mApp->mFonts[FONT_MT11]);

	struct Spec { DialogButton** mButton; int mId; unsigned int mText; };
	Spec aSpecs[4] =
	{
		{&mHelpButton, 0, 0x004a3f64}, {&mRegisterButton, 1, 0x004a3f58}, {&mScoresButton, 2, 0x004a3f4c},
		{&mWebButton, 3, 0x004a3f2c}
	};
	for (int i = 0; i < 4; i++)
	{
		DialogButton* aButton = new DialogButton(mApp->mImages[IMAGE_OPTIONSBUTTON], aSpecs[i].mId, this);
		aButton->SetFont(mApp->mFonts[FONT_MT11]);
		aButton->mLabel = GT(aSpecs[i].mText);
		*aSpecs[i].mButton = aButton;
	}
	// port: there is nothing to register
	mRegisterButton->SetVisible(false);
	mRegisterButton->SetDisabled(true);

	mMusicSlider = new Slider(mApp->mImages[IMAGE_SLIDERTRACK], mApp->mImages[IMAGE_SLIDERTHUMB], 0, this);
	mMusicSlider->SetValue(mApp->GetMusicVolume());
	mSfxSlider = new Slider(mApp->mImages[IMAGE_SLIDERTRACK], mApp->mImages[IMAGE_SLIDERTHUMB], 1, this);
	mSfxSlider->SetValue(mApp->GetSfxVolume());
	mFullscreenCheckbox = new Checkbox(mApp->mImages[IMAGE_UNCHECKED], mApp->mImages[IMAGE_CHECKED], 0, this);
	mFullscreenCheckbox->SetChecked(!mApp->mIsWindowed, false);
	mCursorsCheckbox = new Checkbox(mApp->mImages[IMAGE_UNCHECKED], mApp->mImages[IMAGE_CHECKED], 1, this);
	mCursorsCheckbox->SetChecked(mApp->mCustomCursorsEnabled, false);
}

OptionsDialog::~OptionsDialog()
{
	delete mHelpButton;
	delete mRegisterButton;
	delete mScoresButton;
	delete mWebButton;
	delete mMusicSlider;
	delete mSfxSlider;
	delete mFullscreenCheckbox;
	delete mCursorsCheckbox;
}

// 0x419030 / 0x4190c0
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mHelpButton);
	theWidgetManager->AddWidget(mRegisterButton);
	theWidgetManager->AddWidget(mScoresButton);
	theWidgetManager->AddWidget(mMusicSlider);
	theWidgetManager->AddWidget(mSfxSlider);
	theWidgetManager->AddWidget(mCursorsCheckbox);
	theWidgetManager->AddWidget(mFullscreenCheckbox);
	theWidgetManager->AddWidget(mWebButton);
}

void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mHelpButton);
	theWidgetManager->RemoveWidget(mRegisterButton);
	theWidgetManager->RemoveWidget(mScoresButton);
	theWidgetManager->RemoveWidget(mMusicSlider);
	theWidgetManager->RemoveWidget(mSfxSlider);
	theWidgetManager->RemoveWidget(mFullscreenCheckbox);
	theWidgetManager->RemoveWidget(mCursorsCheckbox);
	theWidgetManager->RemoveWidget(mWebButton);
}

// 0x419150
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	int aThird = (((mWidth - mContentInsets.mLeft) - mContentInsets.mRight) - 0xc) / 3;
	int aFooterY = (mYesButton != NULL) ? mYesButton->mY : mY + mHeight - 0x44;
	mHelpButton->Resize(mX + mContentInsets.mLeft, aFooterY - 0x44, aThird - 6, 0x22);
	mScoresButton->Resize(mContentInsets.mLeft + mX - 6 + aThird, aFooterY - 0x44, aThird + 0xf, 0x22);
	mRegisterButton->Resize(mContentInsets.mLeft + aThird * 2 + 9 + mX, aFooterY - 0x44, aThird - 3, 0x22);
	mWebButton->Resize(mX + mContentInsets.mLeft, aFooterY - 0x22, (mWidth - mContentInsets.mLeft) - mContentInsets.mRight, 0x22);
	mMusicSlider->Resize(mX + 0x98, mY + 0x7b, mWidth - 0xc0, 0x20);
	mSfxSlider->Resize(mX + 0x98, mY + 0x9b, mWidth - 0xc0, 0x20);
	mFullscreenCheckbox->Resize(mX + 0x30, mY + 0xc1, 0x14, 0x14);
	mCursorsCheckbox->Resize(mX + 0xb0, mY + 0xc1, 0x14, 0x14);
}

int OptionsDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x1a0;
}

// 0x4192e0
void OptionsDialog::Draw(Graphics* g)
{
	DrawFramed(g, mApp, mWidth, mHeight);
	g->SetFont(mApp->mFonts[FONT_FONT1]);
	g->SetColor(Color(0xff, 0, 0));
	g->DrawString(GT(0x004a3fa4), 100, 0x73);
	g->SetFont(mApp->mFonts[FONT_MT11]);
	g->SetColorizeImages(true);
	g->SetColor(Color(0xff, 0xff, 0));
	g->DrawString(GT(0x004a3f9c), 0x30, 0x8f);
	g->DrawString(GT(0x004a3f90), 0x30, 0xaf);
	g->DrawString(GT(0x004a3f84), (mFullscreenCheckbox->mX - mX) + 0x1c, (mFullscreenCheckbox->mY - mY) + 0xe);
	g->DrawString(GT(0x004a3f74), (mCursorsCheckbox->mX - mX) + 0x1c, (mCursorsCheckbox->mY - mY) + 0xe);
	g->SetColorizeImages(false);
}

// 0x4196f0: the buttons of the dialog (the footer button is the app's: ButtonDepress(2004))
void OptionsDialog::ButtonDepress(int theId)
{
	Dialog2004::ButtonDepress(theId);
	switch (theId)
	{
	case 0:
		mApp->ShowHelp();
		break;
	case 2:
		mApp->ShowHighScores();
		break;
	case 3:
		mApp->OpenURL("http://www.popcap.com", false);
		break;
	}
}

// 0x419730 / 0x419770
void OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == 0)
		mApp->SetMusicVolume(theVal);
	else if (theId == 1)
		mApp->SetSfxVolume(theVal);
}

// ---------------------------------------------------------------------------------------------------------
// NextLevelDialog

// 0x417a30
NextLevelDialog::NextLevelDialog(SevenSeasApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_OPTIONSBUTTON], DIALOG_NEXT_LEVEL, true,
		GT(0x004a3e6c), "", GT(0x004a3e78), BUTTONS_FOOTER),
	mApp(theApp)
{
	SetButtonFont(mApp->mFonts[FONT_MT11]);
	SetHeaderFont(mApp->mFonts[FONT_FONT1]);
	SetLinesFont(mApp->mFonts[FONT_MT11]);
	SetColor(COLOR_HEADER, Color(0x66, 0x5c, 0x38));
	SetColor(COLOR_LINES, Color(0, 0, 0));
	SetColor(COLOR_BUTTON_TEXT, Color(0xe0, 0xb7, 0x28));
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
}

int NextLevelDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x14c;
}

// 0x417ca0
void NextLevelDialog::Draw(Graphics* g)
{
	DrawFramed(g, mApp, mWidth, mHeight);
	Board* aBoard = mApp->mBoard;
	char aBuf[128];
	g->SetFont(mApp->mFonts[FONT_MT22]);
	g->SetColor(Color(0xff, 0xff, 0xff));
	sprintf(aBuf, GT(0x004a3f18).c_str(), aBoard->mLevel);
	g->DrawString(aBuf, (mWidth - g->GetFont()->StringWidth(aBuf)) / 2, 0x78);

	g->SetFont(mApp->mFonts[FONT_MEDIUM]);
	int aBonus = aBoard->mKills * 10;
	if ((aBoard->mLevel < 10) && (mApp->mDifficulty == 1))
		aBonus = aBonus / 2;
	// "Pirates Sunk: n  Bonus Points: m" in two colours: the labels, then the numbers
	sprintf(aBuf, GT(0x004a3ef4).c_str(), aBoard->mKills, aBonus);
	int aX = (mWidth - g->GetFont()->StringWidth(aBuf)) / 2;
	g->SetColor(Color(0xff, 0xc2, 0));
	sprintf(aBuf, "%s", GT(0x004a3ee4).c_str());
	g->DrawString(aBuf, aX, 0xa0);
	aX += g->GetFont()->StringWidth(aBuf);
	g->SetColor(Color(0xff, 0xff, 0));
	sprintf(aBuf, "%d", aBoard->mKills);
	g->DrawString(aBuf, aX, 0xa0);
	aX += g->GetFont()->StringWidth(aBuf);
	g->SetColor(Color(0xff, 0xc2, 0));
	sprintf(aBuf, "%s", GT(0x004a3ec4).c_str() + 16);
	g->DrawString(aBuf, aX, 0xa0);
	aX += g->GetFont()->StringWidth(aBuf);
	g->SetColor(Color(0xff, 0xff, 0));
	sprintf(aBuf, "%d", aBonus);
	g->DrawString(aBuf, aX, 0xa0);

	int aScore = aBoard->mScore;
	g->SetColor(Color(0xff, 0xc2, 0));
	sprintf(aBuf, GT(0x004a3eb4).c_str(), HighScoreWidget::RankName(aScore));
	g->DrawString(aBuf, (mWidth - g->GetFont()->StringWidth(aBuf)) / 2, 0xc0);
	static const int kNext[12] = {100, 200, 500, 1000, 2000, 4000, 6000, 8000, 10000, 12500, 15000, 18000};
	int aIdx = 0;
	if (kNext[0] < aScore)
	{
		do
		{
			aIdx++;
		}
		while ((aIdx < 11) && (kNext[aIdx] < aScore));
	}
	g->SetColor(Color(0xff, 0xff, 0));
	sprintf(aBuf, GT(0x004a3e98).c_str(), kNext[aIdx]);
	g->DrawString(aBuf, (mWidth - g->GetFont()->StringWidth(aBuf)) / 2, 0xe0);
}

// ---------------------------------------------------------------------------------------------------------
// RecordScoreDialog

// 0x419f40
RecordScoreDialog::RecordScoreDialog(SevenSeasApp* theApp, const SexyString& thePlace) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_OPTIONSBUTTON], DIALOG_RECORD_SCORE, true,
		GT(0x004a5958), thePlace, "", BUTTONS_OK_CANCEL),
	mApp(theApp),
	mPlace(thePlace)
{
	SetButtonFont(mApp->mFonts[FONT_MT11]);
	SetHeaderFont(mApp->mFonts[FONT_FONT1]);
	SetLinesFont(mApp->mFonts[FONT_MT11]);
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	SetColor(COLOR_LINES, Color(0xf3, 0xb4, 0x4d));
	SetColor(COLOR_BUTTON_TEXT, Color(0xe0, 0xb7, 0x28));
	mNameEdit = new EditWidget(0, this);
	mNameEdit->SetFont(mApp->mFonts[FONT_MT11]);
	mNameEdit->mMaxChars = 0x10;
	mNameEdit->SetText(mApp->mLastScoreName);
}

RecordScoreDialog::~RecordScoreDialog()
{
	delete mNameEdit;
}

// 0x41a270
void RecordScoreDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNameEdit);
	theWidgetManager->SetFocus(mNameEdit);
}

void RecordScoreDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNameEdit);
}

// 0x41a2a0
void RecordScoreDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mNameEdit->Resize(mX + 0x30, mHeight - 0x78 + mY, mWidth - 0x60, 0x18);
}

int RecordScoreDialog::GetPreferredHeight(int theWidth)
{
	return Dialog2004::GetPreferredHeight(theWidth) + 0x40;
}

// 0x41a2f0
void RecordScoreDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	// the box around the name
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->FillRect((mNameEdit->mX - mX) - 4, (mNameEdit->mY - mY) - 4, mNameEdit->mWidth + 8, mNameEdit->mHeight + 8);
	g->SetFont(mApp->mFonts[FONT_MT11]);
	g->SetColor(Color(0xff, 0, 0));
	g->DrawString(GT(0x004a5964), mNameEdit->mX - mX, (mNameEdit->mY - mY) - 0x10);
}

void RecordScoreDialog::EditWidgetText(int theId, const SexyString& theString)
{
	(void) theId;
	(void) theString;
}

bool RecordScoreDialog::AllowChar(int theId, SexyChar theChar)
{
	(void) theId;
	return (theChar >= 32) && (theChar < 127);
}

// ---------------------------------------------------------------------------------------------------------
// HighScoreDialog

// 0x40f7b0
HighScoreDialog::HighScoreDialog(SevenSeasApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_OPTIONSBUTTON], DIALOG_HIGH_SCORES, true,
		GT(0x004a3584), "", GT(0x004a3590), BUTTONS_FOOTER),
	mApp(theApp)
{
	mScores = new HighScoreWidget(theApp);
	SetButtonFont(mApp->mFonts[FONT_MT11]);
	SetHeaderFont(mApp->mFonts[FONT_FONT1]);
	SetLinesFont(mApp->mFonts[FONT_MT11]);
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
}

HighScoreDialog::~HighScoreDialog()
{
	delete mScores;
}

int HighScoreDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x1ac;
}

// 0x40faf0
void HighScoreDialog::Draw(Graphics* g)
{
	DrawFramed(g, mApp, mWidth, mHeight);
	g->SetFont(mApp->mFonts[FONT_FONT1]);
	g->SetColor(Color(0xff, 0, 0));
	SexyString aHeader = GT(0x004a3584);
	g->DrawString(aHeader, (mWidth - g->GetFont()->StringWidth(aHeader)) / 2, 0x73);
	g->SetFont(mApp->mFonts[FONT_MT11]);
	int aY = 0x95;
	for (int i = 0; i < 5; i++)
	{
		g->SetColor(Color(0xff, 0xff, 0));
		if (mScores->mScores[i] == 0)
			g->DrawString(GT(0x004a35a4), 0x30, aY);
		else
		{
			char aBuf[128];
			g->DrawString(mScores->mNames[i], 0x30, aY);
			sprintf(aBuf, "%d", mScores->mScores[i]);
			g->DrawString(aBuf, mWidth - 0x30 - g->GetFont()->StringWidth(aBuf), aY);
		}
		aY += 0x1c;
	}
}

// ---------------------------------------------------------------------------------------------------------
// NagDialog

// 0x40c7d0 (the bonus screen) and 0x4262c0 (the warnings)
NagDialog::NagDialog(SevenSeasApp* theApp, int theDialogId, int theWhich) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_OPTIONSBUTTON], theDialogId, true,
		(theWhich < 0) ? GT(0x004a30c0) : GT(0x004a6730), "", (theWhich < 0) ? GT(0x004a30cc) : GT(0x004a3590),
		BUTTONS_FOOTER),
	mApp(theApp),
	mWhich(theWhich)
{
	SetButtonFont(mApp->mFonts[FONT_MT11]);
	SetHeaderFont(mApp->mFonts[FONT_FONT1]);
	SetLinesFont(mApp->mFonts[FONT_MT11]);
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	mCheckbox = new Checkbox(mApp->mImages[IMAGE_UNCHECKED], mApp->mImages[IMAGE_CHECKED], 0, this);
	mCheckbox->SetChecked(false, false);
}

NagDialog::~NagDialog()
{
	delete mCheckbox;
}

void NagDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mCheckbox);
}

void NagDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mCheckbox);
}

void NagDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	int aFooterY = (mYesButton != NULL) ? mYesButton->mY : mY + mHeight - 0x44;
	mCheckbox->Resize(mX + 0x30, aFooterY - 0x2c, 0x14, 0x14);
}

int NagDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return (mWhich < 0) ? 0x17c : 0x14c;
}

// 0x40cbe0 / 0x4266f0
void NagDialog::Draw(Graphics* g)
{
	DrawFramed(g, mApp, mWidth, mHeight);
	g->SetFont(mApp->mFonts[FONT_FONT1]);
	g->SetColor(Color(0xff, 0, 0));
	if (mWhich < 0)
	{
		SexyString aTitle = GT(0x004a3284);
		g->DrawString(aTitle, (mWidth - g->GetFont()->StringWidth(aTitle)) / 2, 0x73);
		aTitle = GT(0x004a3270);
		g->DrawString(aTitle, (mWidth - g->GetFont()->StringWidth(aTitle)) / 2, 0x93);
	}
	else
	{
		SexyString aTitle = GT(0x004a6ac4);
		g->DrawString(aTitle, (mWidth - g->GetFont()->StringWidth(aTitle)) / 2, 0x73);
	}
	g->SetFont(mApp->mFonts[FONT_MT11]);
	g->SetColor(Color(0xe0, 0xb7, 0x28));
	static const unsigned int kBonusLines[7] = {0x004a323c, 0x004a3208, 0x004a31d4, 0x004a31a0, 0x004a316c, 0x004a3138, 0x004a3104};
	static const unsigned int kWarnLines[4][3] =
	{
		{0x004a6a34, 0x004a6a0c, 0x004a6a00},
		{0x004a6a9c, 0x004a6a74, 0x004a6a5c},
		{0x004a69d8, 0x004a69b0, 0x004a6994},
		{0x004a696c, 0x004a6940, 0x004a6918}
	};
	int aY = 0xb0;
	if (mWhich < 0)
	{
		for (int i = 0; i < 7; i++)
		{
			const SexyString& aLine = GT(kBonusLines[i]);
			g->DrawString(aLine, (mWidth - g->GetFont()->StringWidth(aLine)) / 2, aY);
			aY += 0x14;
		}
	}
	else if (mWhich < 4)
	{
		aY = 0x93;
		for (int i = 0; i < 3; i++)
		{
			const SexyString& aLine = GT(kWarnLines[mWhich][i]);
			g->DrawString(aLine, (mWidth - g->GetFont()->StringWidth(aLine)) / 2, aY);
			aY += 0x1c;
		}
	}
	g->SetColor(Color(0xff, 0xff, 0));
	g->DrawString(GT(0x004a30e4), (mCheckbox->mX - mX) + 0x1c, (mCheckbox->mY - mY) + 0xe);
}
