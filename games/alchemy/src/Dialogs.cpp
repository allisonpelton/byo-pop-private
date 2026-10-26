// The dialogs of Alchemy Deluxe that are classes of their own (see Dialogs.h).

#include "Dialogs.h"
#include "AlchemyApp.h"
#include "HighScores.h"
#include "Graphics.h"
#include "Font.h"
#include "ImageFont.h"
#include "WidgetManager.h"

#include <stdio.h>

using namespace Sexy;

// the colours of the name box (0x510158: background and outline are clear, the frame comes from the picture)
static int kNameEditColors[5][4] =
{
	{0, 0, 0, 0}, {0, 0, 0, 0}, {0xf0, 0xf0, 0xff, 0xff}, {0xff, 0xff, 0xff, 0xff}, {0, 0, 0, 0xff}
};

// ---------------------------------------------------------------------------------------------------------
// AllTimeDialog

// 0x4096c8
AllTimeDialog::AllTimeDialog(AlchemyApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_HIGH_SCORES, true,
		theApp->GetString("HIGH_SCORES"), "", theApp->GetString("DONE"), BUTTONS_FOOTER),
	mScores(new HighScoreWidget(theApp)),
	mApp(theApp)
{
	SetButtonFont(mApp->mFont1);
	SetHeaderFont(mApp->mTitleFont);
	SetLinesFont(mApp->mFont1);
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0xe0, 0xb7, 0x28));
}

AllTimeDialog::~AllTimeDialog()
{
	delete mScores;
}

// 0x4096a4: the rows of the table are drawn below the header, so the height is fixed
int AllTimeDialog::GetPreferredHeight(int theWidth)
{
	return 0x156;
}

// 0x409854: two columns (strategic, time trial) of the best of the day, then the five best of all time
void AllTimeDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mFont1);

	for (int aMode = 0; aMode < HighScoreWidget::NUM_MODES; aMode++)
	{
		int aCenter = (aMode != 0 ? 1 : -1) * 0x5a + mWidth / 2;
		g->SetFont(mApp->mFont1);

		g->SetColor(Color(0x5c, 0x56, 0x44));
		SexyString aText = mApp->GetString(aMode == 0 ? "NORMAL" : "TIME_TRIAL");
		int aWidth = g->GetFont()->StringWidth(aText);
		g->DrawString(aText, aCenter - aWidth / 2, 0x55);
		g->FillRect(aCenter - aWidth / 2, 0x56, aWidth, 1);

		g->SetColor(Color(0x2e, 0x2b, 0x22));
		aText = mApp->GetString("TODAYS");
		aWidth = g->GetFont()->StringWidth(aText);
		g->DrawString(aText, aCenter - aWidth / 2, 0x6f);
		g->FillRect(aCenter - aWidth / 2, 0x72, aWidth, 1);

		g->SetColor(Color(0, 0, 0));
		if (mScores->mTodayScore[aMode] < 1)
		{
			aText = mApp->GetString("EMPTY");
			g->DrawString(aText, aCenter - g->GetFont()->StringWidth(aText) / 2, 0x89);
		}
		else
		{
			g->DrawString(mScores->mTodayName[aMode], aCenter - 0x4a, 0x89);
			char aBuffer[32];
			sprintf(aBuffer, "%d", mScores->mTodayScore[aMode]);
			g->DrawString(aBuffer, (aCenter - g->GetFont()->StringWidth(aBuffer)) + 0x4a, 0x89);
		}

		g->SetColor(Color(0x2e, 0x2b, 0x22));
		aText = mApp->GetString("ALL_TIME");
		aWidth = g->GetFont()->StringWidth(aText);
		g->DrawString(aText, aCenter - aWidth / 2, 0xa7);
		g->FillRect(aCenter - aWidth / 2, 0xaa, aWidth, 1);

		g->SetColor(Color(0, 0, 0));
		int aY = 0xbf;
		for (int i = 0; i < HighScoreWidget::NUM_ENTRIES; i++)
		{
			g->DrawString(mScores->mName[aMode][i], aCenter - 0x4a, aY);
			char aBuffer[32];
			sprintf(aBuffer, "%d", mScores->mScore[aMode][i]);
			g->DrawString(aBuffer, (aCenter - g->GetFont()->StringWidth(aBuffer)) + 0x4a, aY);
			aY += 0x11;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------
// OptionsDialog

// 0x40d836
OptionsDialog::OptionsDialog(AlchemyApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_OPTIONS, true,
		theApp->GetString("OPTIONS"), "", theApp->GetString("OPTDLG_DONE"), BUTTONS_FOOTER),
	mApp(theApp)
{
	SetButtonFont(mApp->mFont1);
	SetHeaderFont(mApp->mTitleFont);
	SetLinesFont(mApp->mFont1);

	mScoresButton = new DialogButton(mApp->mImages[IMAGE_DBUTTON], 2, this);
	mScoresButton->SetFont(mApp->mFont1);
	mScoresButton->mLabel = mApp->GetString("OPTDLG_HIGH_SCORES");

	mHelpButton = new DialogButton(mApp->mImages[IMAGE_DBUTTON], 4, this);
	mHelpButton->SetFont(mApp->mFont1);
	mHelpButton->mLabel = mApp->GetString("HELP");

	mMusicSlider = new Slider2004(mApp->mImages[IMAGE_SLIDER], mApp->mImages[IMAGE_SLIDERGEM], 0, this);
	mMusicSlider->SetValue(mApp->GetMusicVolume());
	mSfxSlider = new Slider2004(mApp->mImages[IMAGE_SLIDER], mApp->mImages[IMAGE_SLIDERGEM], 1, this);
	mSfxSlider->SetValue(mApp->GetSfxVolume());

	// both boxes have the id 0 (CheckboxChecked does not tell them apart)
	mFullscreenBox = new Checkbox(mApp->mImages[IMAGE_UNCHECKED], mApp->mImages[IMAGE_CHECKED], 0, this);
	mFullscreenBox->SetChecked(!mApp->mIsWindowed, true);
	mCursorsBox = new Checkbox(mApp->mImages[IMAGE_UNCHECKED], mApp->mImages[IMAGE_CHECKED], 0, this);
	mCursorsBox->SetChecked(mApp->mCustomCursorsEnabled, true);
}

// 0x40d528
OptionsDialog::~OptionsDialog()
{
	delete mScoresButton;
	delete mHelpButton;
	delete mMusicSlider;
	delete mSfxSlider;
	delete mFullscreenBox;
	delete mCursorsBox;
}

// 0x40d679
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mScoresButton);
	theWidgetManager->AddWidget(mHelpButton);
	theWidgetManager->AddWidget(mMusicSlider);
	theWidgetManager->AddWidget(mSfxSlider);
	theWidgetManager->AddWidget(mCursorsBox);
	theWidgetManager->AddWidget(mFullscreenBox);
}

// 0x40d701
void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mScoresButton);
	theWidgetManager->RemoveWidget(mHelpButton);
	theWidgetManager->RemoveWidget(mMusicSlider);
	theWidgetManager->RemoveWidget(mSfxSlider);
	theWidgetManager->RemoveWidget(mFullscreenBox);
	theWidgetManager->RemoveWidget(mCursorsBox);
}

// 0x40de70: the original is 0x22 higher with the button to the web site, and 0x22 more with the update check's
int OptionsDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x112;
}

// 0x40d602: the button colour reaches the dialog's own buttons
void OptionsDialog::SetColor(int theIdx, const Color& theColor)
{
	Dialog2004::SetColor(theIdx, theColor);
	if (theIdx == COLOR_BUTTON_TEXT)
	{
		DialogButton* aButtons[2] = {mScoresButton, mHelpButton};
		for (int i = 0; i < 2; i++)
			if (aButtons[i] != NULL)
				aButtons[i]->SetColor(ButtonWidget::COLOR_LABEL, theColor);
	}
}

// 0x40dee4: the help and the scores side by side above the footer button (the original has the button to the web site
// between them, and the registration button next to them when the copy is not registered)
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);

	int aFooterY = (mYesButton != NULL) ? mYesButton->mY : mY + mHeight - 0x50;
	int aContent = (mWidth - mContentInsets.mRight) - mContentInsets.mLeft;
	int aY = aFooterY - 0x22;
	int aHalf = (aContent - 3) / 2;
	mHelpButton->Resize(mX + mContentInsets.mLeft, aY, aHalf, 0x20);
	mScoresButton->Resize(mX + mContentInsets.mLeft + 3 + aHalf, aY, aHalf, 0x20);

	mMusicSlider->Resize(mX + 0x98, mY + 0x44, mWidth - 0xc0, 0x20);
	mSfxSlider->Resize(mX + 0x98, mY + 0x64, mWidth - 0xc0, 0x20);
	mFullscreenBox->Resize(mX + 0x30, mY + 0x8a, 0x14, 0x14);
	mCursorsBox->Resize(mX + 0xb0, mY + 0x8a, 0x14, 0x14);
}

// 0x40e18e
void OptionsDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mFont1);
	g->SetColorizeImages(true);
	g->SetColor(GetColor(COLOR_LINES));
	g->DrawString(mApp->GetString("MUSIC"), 0x30, 0x58);
	g->DrawString(mApp->GetString("SOUND_FX"), 0x30, 0x78);
	g->DrawString(mApp->GetString("FULLSCREEN"), (mFullscreenBox->mX - mX) + 0x1c, (mFullscreenBox->mY - mY) + 0xe);
	g->DrawString(mApp->GetString("CUSTOM_CURSORS"), (mCursorsBox->mX - mX) + 0x1c, (mCursorsBox->mY - mY) + 0xe);
	g->SetColorizeImages(false);
}

// 0x40d7f0: the dialog's own buttons click (the footer button's id is 1000 and over)
void OptionsDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	if (theId < 1000)
		mApp->PlaySample(SOUND_CLICK);
}

// 0x40e34b: the dialog's own buttons (the footer button is the app's: ButtonDepress(2004))
void OptionsDialog::ButtonDepress(int theId)
{
	Dialog2004::ButtonDepress(theId);
	switch (theId)
	{
	case 2:
		mApp->ShowAllTimeScores();
		break;
	case 4:
		mApp->ShowHelp();
		break;
	}
}

// 0x40d789
void OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == 0)
		mApp->SetMusicVolume(theVal);
	else if (theId == 1)
		mApp->SetSfxVolume(theVal);
}

// 0x40d7c2: a box that is cleared while the game is forced to full screen (a desktop of too few colours) is an error, and
// full screen is checked again. Both boxes have the id 0, so either of them does it.
void OptionsDialog::CheckboxChecked(int theId, bool checked)
{
	(void) theId;
	if (!checked && mApp->mForceFullscreen)
	{
		mApp->DoDialog(DIALOG_COLOR_DEPTH, true, mApp->GetString("COLOR_DEPTH_ERROR1"), mApp->GetString("COLOR_DEPTH_ERROR2"),
			mApp->GetString("COLOR_DEPTH_ERROR3"), BUTTONS_FOOTER);
		mFullscreenBox->SetChecked(true, true);
	}
}

// ---------------------------------------------------------------------------------------------------------
// NameEntryDialog

// 0x40ed00
NameEntryDialog::NameEntryDialog(AlchemyApp* theApp, const SexyString& theLines) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_NAME_ENTRY, true,
		theApp->GetString("HIGH_SCORE"), theLines, "", BUTTONS_OK_CANCEL),
	mApp(theApp)
{
	SetButtonFont(mApp->mFont1);
	SetHeaderFont(mApp->mTitleFont);
	SetLinesFont(mApp->mFont1);
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	SetColor(Dialog::COLOR_LINES, Color(0, 0, 0));
	SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0xe0, 0xb7, 0x28));

	mEdit = new EditWidget(0, this);
	mEdit->SetFont(mApp->mFont1);
	mEdit->SetColors(kNameEditColors, 5);
	mEdit->mMaxPixels = 0x68;
	mEdit->SetText(mApp->mLastScoreName);
}

// 0x40ea5d
NameEntryDialog::~NameEntryDialog()
{
	delete mEdit;
}

SexyString NameEntryDialog::GetName()
{
	return mEdit->mString;
}

// 0x40eac0
void NameEntryDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mEdit);
	theWidgetManager->SetFocus(mEdit);
}

// 0x40eaeb
void NameEntryDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mEdit);
}

// 0x40eb0a
void NameEntryDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mEdit->Resize(mX + 0x30, mHeight + mY - 0x64, mWidth - 0x60, 0x18);
}

// 0x40eab1
int NameEntryDialog::GetPreferredHeight(int theWidth)
{
	return Dialog2004::GetPreferredHeight(theWidth) + 0x38;
}

// 0x40eb9f: the box of a name: the left end of the picture, a run of its middle and its right end
static void DrawNameBox(Graphics* g, Image* theBox, const Rect& theRect)
{
	g->DrawImage(theBox, theRect.mX, theRect.mY, Rect(0, 0, 0x20, 0x20));
	g->DrawImage(theBox, theRect.mX + theRect.mWidth - 0x20, theRect.mY, Rect(0x40, 0, 0x20, 0x20));
	Graphics aMiddle(*g);
	aMiddle.ClipRect(theRect.mX + 0x20, theRect.mY, theRect.mWidth - 0x20, 0x20);
	int aTiles = (theRect.mWidth - 0x21) / 0x20;
	for (int i = 0; i < aTiles; i++)
		aMiddle.DrawImage(theBox, theRect.mX + 0x20 + i * 0x20, theRect.mY, Rect(0x20, 0, 0x20, 0x20));
}

// 0x40eed1: the box of the name (4 pixels more on each side than the edit widget) and the prompt over it
void NameEntryDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	Rect aBox(mEdit->mX - mX - 4, mEdit->mY - mY - 4, mEdit->mWidth + 8, mEdit->mHeight + 8);
	DrawNameBox(g, mApp->mImages[IMAGE_EDITBOX], aBox);
	g->SetFont(mApp->mFont1);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(mApp->GetString("ENTER_YOUR_NAME"), mEdit->mX - mX, (mEdit->mY - mY) - 8);
}

// 0x40eb4e: Enter in the box answers the dialog's OK button
void NameEntryDialog::EditWidgetText(int theId, const SexyString& theString)
{
	(void) theId;
	(void) theString;
	mApp->ButtonDepress(mId + 2000);
}
