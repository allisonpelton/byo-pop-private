// The dialogs of Noah's Ark Deluxe (see Dialogs.h).

#include "Dialogs.h"
#include "NoahApp.h"
#include "Screens.h"
#include "GameText.h"
#include "Board.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"

#include <algorithm>
#include <cmath>

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------
// DifficultyDialog

// 0x410c00
DifficultyDialog::DifficultyDialog(NoahApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_DIFFICULTY, true,
		GT(0x00494410), GT(0x0049c510), GT(0x0049441c), BUTTONS_FOOTER),
	mApp(theApp)
{
	SetButtonFont(mApp->mFonts[FONT_SMALL]);
	SetHeaderFont(mApp->mFonts[FONT_MEDIUM]);
	SetLinesFont(mApp->mFonts[FONT_SMALL]);

	mEasyButton = new DialogButton(mApp->mImages[IMAGE_DBUTTON], 0x3c, this);
	mEasyButton->SetFont(mApp->mFonts[FONT_SMALL]);
	mEasyButton->mLabel = GT(0x00494408);
	mMediumButton = new DialogButton(mApp->mImages[IMAGE_DBUTTON], 0x3d, this);
	mMediumButton->SetFont(mApp->mFonts[FONT_SMALL]);
	mMediumButton->mLabel = GT(0x00494400);
	mHardButton = new DialogButton(mApp->mImages[IMAGE_DBUTTON], 0x3e, this);
	mHardButton->SetFont(mApp->mFonts[FONT_SMALL]);
	mHardButton->mLabel = GT(0x004943f8);
}

DifficultyDialog::~DifficultyDialog()
{
	delete mEasyButton;
	delete mMediumButton;
	delete mHardButton;
}

void DifficultyDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mEasyButton);
	theWidgetManager->AddWidget(mMediumButton);
	theWidgetManager->AddWidget(mHardButton);
}

void DifficultyDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mEasyButton);
	theWidgetManager->RemoveWidget(mMediumButton);
	theWidgetManager->RemoveWidget(mHardButton);
}

// 0x4111c0
void DifficultyDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	int aX = mX + mContentInsets.mLeft;
	int aWidth = (mWidth - mContentInsets.mRight) - mContentInsets.mLeft;
	mEasyButton->Resize(aX, mY + 0x5f, aWidth, 0x28);
	mMediumButton->Resize(aX, mY + 0x8c, aWidth, 0x28);
	mHardButton->Resize(aX, mY + 0xb9, aWidth, 0x28);
}

int DifficultyDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x140;
}

// 0x4112b0: the three buttons set the difficulty and start the game over
void DifficultyDialog::ButtonDepress(int theId)
{
	if ((theId >= 0x3c) && (theId <= 0x3e))
	{
		mApp->mDifficulty = (theId == 0x3c) ? 0 : ((theId == 0x3d) ? 4 : 8);
		mApp->DeleteSavedGame();
		mApp->StartGame(false);
	}
	else
	{
		Dialog2004::ButtonDepress(theId);
		return;
	}
	mApp->KillDialog(DIALOG_DIFFICULTY);
}

// ---------------------------------------------------------------------------------------------------------
// HighScoreDialog

// 0x4113e0
HighScoreDialog::HighScoreDialog(NoahApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_HIGH_SCORES, true,
		GT(0x0049444c), GT(0x0049c510), GT(0x0049c510), BUTTONS_FOOTER),
	mApp(theApp)
{
	mScores = new HighScoreWidget(theApp);
	SetButtonFont(mApp->mFonts[FONT_SMALL]);
	SetHeaderFont(mApp->mFonts[FONT_MEDIUM]);
	SetLinesFont(mApp->mFonts[FONT_SMALL]);
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	SetColor(COLOR_BUTTON_TEXT, Color(0xff, 0xff, 0xff));
	SetColor(COLOR_HEADER, Color(0xff, 0xff, 0xff));
}

HighScoreDialog::~HighScoreDialog()
{
	delete mScores;
}

// 0x411740
void HighScoreDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	int aY = 0x72;
	for (int i = 0; i < 5; i++)
	{
		g->SetFont(mApp->mFonts[FONT_GARAMOND_SMALL]);
		g->SetColor(Color(0, 0, 0));
		g->DrawString(mScores->mNames[i], 0x32, aY);
		g->SetFont(mApp->mFonts[FONT_SMALL]);
		g->SetColor(Color(0xff, 0xff, 0xff));
		SexyString aScore = StrFormat("%d", mScores->mScores[i]);
		g->DrawString(aScore, (mWidth - g->GetFont()->StringWidth(aScore)) - 0x32, aY);
		aY += 0x18;
	}
}

// ---------------------------------------------------------------------------------------------------------
// RecordScoreDialog

// 0x41d750
RecordScoreDialog::RecordScoreDialog(NoahApp* theApp, const std::string& thePlace) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_RECORD_SCORE, true,
		GT(0x00495ef0), thePlace, GT(0x00495998), BUTTONS_FOOTER),
	mApp(theApp)
{
	SetButtonFont(mApp->mFonts[FONT_SMALL]);
	SetHeaderFont(mApp->mFonts[FONT_MEDIUM]);
	SetLinesFont(mApp->mFonts[FONT_SMALL]);
	mNameEdit = new EditWidget(0, this);
	mNameEdit->SetFont(mApp->mFonts[FONT_GARAMOND_SMALL]);
	mNameEdit->mMaxChars = 0x10;
	mNameEdit->SetText(mApp->mLastScoreName);
	mNameEdit->SetColor(EditWidget::COLOR_BKG, Color(0xff, 0xff, 0xff));
	mNameEdit->SetColor(EditWidget::COLOR_OUTLINE, Color(0, 0, 0));
	mNameEdit->SetColor(EditWidget::COLOR_TEXT, Color(0, 0, 0));
}

RecordScoreDialog::~RecordScoreDialog()
{
	delete mNameEdit;
}

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

void RecordScoreDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mNameEdit->Resize(theX + mContentInsets.mLeft + 10, theY + theHeight - 0x92, theWidth - mContentInsets.mLeft - mContentInsets.mRight - 20, 0x1c);
}

int RecordScoreDialog::GetPreferredHeight(int theWidth)
{
	return Dialog2004::GetPreferredHeight(theWidth) + 0x40;
}

// 0x41dac0
void RecordScoreDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mFonts[FONT_SMALL]);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(GT(0x00496120), mContentInsets.mLeft + 10, mHeight - 0xa2);
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
// OptionsDialog

// 0x41c230
OptionsDialog::OptionsDialog(NoahApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_OPTIONS, true,
		GT(0x00495fd8), GT(0x0049c510), GT(0x00494458), BUTTONS_FOOTER),
	mApp(theApp)
{
	SetButtonFont(mApp->mFonts[FONT_GARAMOND_SMALL]);
	SetHeaderFont(mApp->mFonts[FONT_MEDIUM]);
	SetLinesFont(mApp->mFonts[FONT_SMALL]);

	struct Spec { DialogButton** mButton; int mId; unsigned int mText; };
	Spec aSpecs[5] =
	{
		{&mHelpButton, 0, 0x00495fd0}, {&mRegisterButton, 1, 0x00495fc4}, {&mScoresButton, 2, 0x00495fb8},
		{&mWebButton, 3, 0x00495fa4}, {&mCreditsButton, 4, 0x00495f9c}
	};
	for (int i = 0; i < 5; i++)
	{
		DialogButton* aButton = new DialogButton(mApp->mImages[IMAGE_DBUTTON], aSpecs[i].mId, this);
		aButton->SetFont(mApp->mFonts[FONT_GARAMOND_SMALL]);
		aButton->mLabel = GT(aSpecs[i].mText);
		*aSpecs[i].mButton = aButton;
	}

	mMusicSlider = new Slider(mApp->mImages[IMAGE_SLIDER], mApp->mImages[IMAGE_SLIDERANKH], 0, this);
	mMusicSlider->SetValue(mApp->GetMusicVolume());
	mSfxSlider = new Slider(mApp->mImages[IMAGE_SLIDER], mApp->mImages[IMAGE_SLIDERANKH], 1, this);
	mSfxSlider->SetValue(mApp->GetSfxVolume());

	// the check box art: the checked picture is the left half of checked.gif, the unchecked one the right
	MemoryImage* aUnchecked = new MemoryImage(mApp);
	aUnchecked->Create(0x24, 0x25);
	{
		Graphics g(aUnchecked);
		g.DrawImage(mApp->mImages[IMAGE_CHECKED], 0, 0);
	}
	MemoryImage* aChecked = new MemoryImage(mApp);
	aChecked->Create(0x24, 0x25);
	{
		Graphics g(aChecked);
		g.DrawImage(mApp->mImages[IMAGE_CHECKED], -0x24, 0);
	}
	mFullscreenCheckbox = new Checkbox(aUnchecked, aChecked, 0, this);
	mFullscreenCheckbox->SetChecked(!mApp->mIsWindowed, false);
	mHintsCheckbox = new Checkbox(aUnchecked, aChecked, 1, this);
	mHintsCheckbox->SetChecked(mApp->mShowHints, false);
}

OptionsDialog::~OptionsDialog()
{
	delete mHelpButton;
	delete mRegisterButton;
	delete mScoresButton;
	delete mWebButton;
	delete mCreditsButton;
	delete mMusicSlider;
	delete mSfxSlider;
	delete mFullscreenCheckbox;
	delete mHintsCheckbox;
}

void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	DialogButton* aButtons[5] = {mHelpButton, mRegisterButton, mScoresButton, mWebButton, mCreditsButton};
	for (int i = 0; i < 5; i++)
		theWidgetManager->AddWidget(aButtons[i]);
	theWidgetManager->AddWidget(mMusicSlider);
	theWidgetManager->AddWidget(mSfxSlider);
	theWidgetManager->AddWidget(mFullscreenCheckbox);
	theWidgetManager->AddWidget(mHintsCheckbox);
}

void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	DialogButton* aButtons[5] = {mHelpButton, mRegisterButton, mScoresButton, mWebButton, mCreditsButton};
	for (int i = 0; i < 5; i++)
		theWidgetManager->RemoveWidget(aButtons[i]);
	theWidgetManager->RemoveWidget(mMusicSlider);
	theWidgetManager->RemoveWidget(mSfxSlider);
	theWidgetManager->RemoveWidget(mFullscreenCheckbox);
	theWidgetManager->RemoveWidget(mHintsCheckbox);
}

// 0x41cf00
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	int aContent = (mWidth - mContentInsets.mRight) - mContentInsets.mLeft;
	int aHalf = (aContent - 6) / 2;
	int aFooterY = (mYesButton != NULL) ? mYesButton->mY : mY + mHeight - 0x50;
	mWebButton->Resize(mX + mContentInsets.mLeft, aFooterY - 0x7e, aContent, 0x24);
	mHelpButton->Resize(mX + mContentInsets.mLeft, aFooterY - 0x54, aHalf, 0x24);
	mRegisterButton->Resize(mX + mContentInsets.mLeft + 6 + aHalf, aFooterY - 0x54, aHalf, 0x24);
	mScoresButton->Resize(mX + mContentInsets.mLeft, aFooterY - 0x2a, aHalf, 0x24);
	mCreditsButton->Resize(mX + mContentInsets.mLeft + 6 + aHalf, aFooterY - 0x2a, aHalf, 0x24);
	mMusicSlider->Resize(mX + 0x98, mY + 0x4e, mWidth - 0xc0, 0x20);
	mSfxSlider->Resize(mX + 0x98, mY + 0x6e, mWidth - 0xc0, 0x20);
	mFullscreenCheckbox->Resize(mX + 0x30, mY + 0x94, 0x24, 0x25);
	mHintsCheckbox->Resize(mX + 0x30, mY + 0xc2, 0x24, 0x25);
}

int OptionsDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 400;
}

void OptionsDialog::SetColor(int theIdx, const Color& theColor)
{
	Dialog2004::SetColor(theIdx, theColor);
	if (theIdx == COLOR_BUTTON_TEXT)
	{
		DialogButton* aButtons[5] = {mHelpButton, mRegisterButton, mScoresButton, mWebButton, mCreditsButton};
		for (int i = 0; i < 5; i++)
			if (aButtons[i] != NULL)
				aButtons[i]->SetColor(0, theColor);
	}
}

// 0x41d090
void OptionsDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mFonts[FONT_GARAMOND_SMALL]);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(GT(0x00495ff8), 0x30, 0x62);
	g->DrawString(GT(0x00495fec), 0x30, 0x82);
	g->DrawString(GT(0x00495940), (mFullscreenCheckbox->mX - mX) + 0x30, (mFullscreenCheckbox->mY - mY) + 0x1a);
	g->DrawString(GT(0x00495fe0), (mHintsCheckbox->mX - mX) + 0x30, (mHintsCheckbox->mY - mY) + 0x1a);
}

// 0x41d410: the buttons of the dialog (the footer button is the app's: ButtonDepress(2003))
void OptionsDialog::ButtonDepress(int theId)
{
	Dialog2004::ButtonDepress(theId);
	switch (theId)
	{
	case 0:
		mApp->ShowModeIntroDialog(mApp->mGameMode);
		break;
	case 1:
		mApp->ShowRegisteredDialog();
		break;
	case 2:
		mApp->ShowHighScores();
		break;
	case 3:
		mApp->OpenURL("http://www.popcap.com", false);
		break;
	case 4:
		mApp->ShowCredits();
		break;
	}
}

// 0x41d370
void OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == 0)
		mApp->SetMusicVolume(theVal);
	else if (theId == 1)
		mApp->SetSfxVolume(theVal);
}

// ---------------------------------------------------------------------------------------------------------
// TutorialDialog

// 0x4205b0
TutorialDialog::TutorialDialog(NoahApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_TUTORIAL, true,
		GT(0x00496384), GT(0x0049c510), GT(0x00495998), BUTTONS_FOOTER),
	mApp(theApp),
	mStep(0),
	mCursorX(0x14),
	mCursorY(0x4e)
{
	SetButtonFont(mApp->mFonts[FONT_GARAMOND_SMALL]);
	SetHeaderFont(mApp->mFonts[FONT_MEDIUM]);
	SetLinesFont(mApp->mFonts[FONT_SMALL]);
	MemoryImage* aUnchecked = new MemoryImage(mApp);
	aUnchecked->Create(0x24, 0x25);
	{
		Graphics g(aUnchecked);
		g.DrawImage(mApp->mImages[IMAGE_CHECKED], 0, 0);
	}
	MemoryImage* aChecked = new MemoryImage(mApp);
	aChecked->Create(0x24, 0x25);
	{
		Graphics g(aChecked);
		g.DrawImage(mApp->mImages[IMAGE_CHECKED], -0x24, 0);
	}
	mAgainCheckbox = new Checkbox(aUnchecked, aChecked, 0, this);
	mAgainCheckbox->SetChecked(false, false);
}

TutorialDialog::~TutorialDialog()
{
	delete mAgainCheckbox;
}

void TutorialDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mAgainCheckbox);
}

void TutorialDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mAgainCheckbox);
}

// 0x420a60
void TutorialDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mAgainCheckbox->Resize(mX + 0x30, mY + mHeight - 0x70, 0x24, 0x25);
}

int TutorialDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x17c;
}

// 0x420ab0: the picture's little animation (a cursor goes to an animal, clicks, goes to the match)
void TutorialDialog::Update()
{
	Dialog2004::Update();
	mUpdateCnt++;
	if (mUpdateCnt % 40 == 0)
	{
		mStep = (mStep + 1) % 4;
		MarkDirty();
	}
}

// 0x420d10
void TutorialDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mFonts[FONT_GARAMOND_TINY]);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(GT(0x00496364), mAgainCheckbox->mX - mX + 0x30, mAgainCheckbox->mY - mY + 0x1a);

	int aHalf = ((mWidth - 10) / 2 - 0x32) / 2;
	Image* anAnimals = mApp->mImages[IMAGE_ANIMALS];
	// the two animals of one kind and the way between them
	g->DrawImage(anAnimals, aHalf, 0x5a, Rect(0, 0, 0x32, 0x32));
	g->DrawImage(anAnimals, aHalf + 0x96, 0x5a, Rect(0, 0, 0x32, 0x32));
	g->DrawImage(mApp->mImages[IMAGE_TRAILLR], aHalf + 0x32, 0x5a);
	g->DrawImage(mApp->mImages[IMAGE_TRAILLR], aHalf + 100, 0x5a);
	g->DrawImage(mApp->mImages[IMAGE_TRAILARROWRIGHT], aHalf + 0x96 - 1, 0x5a);
	int aCursorX = aHalf + 0x1c + mStep * 0x28;
	g->DrawImage(mApp->mImages[IMAGE_ARROW], aCursorX, 0x5a + 0x1c);

	g->SetFont(mApp->mFonts[FONT_GARAMOND_TINY]);
	g->SetColor(Color(0, 0, 0));
	SexyString aLine1 = GT(0x00496328);
	SexyString aLine2 = GT(0x00496318);
	SexyString aLine3 = GT(0x00496304);
	SexyString aLine4 = GT(0x004962f0);
	int aCenter = (mWidth - 10) / 2;
	g->DrawString(aLine1, (aCenter - g->GetFont()->StringWidth(aLine1)) / 2, 0xa0);
	g->DrawString(aLine2, (aCenter - g->GetFont()->StringWidth(aLine2)) / 2, 0xb2);
	g->DrawString(aLine3, (aCenter - g->GetFont()->StringWidth(aLine3)) / 2 + aCenter, 0xa0);
	g->DrawString(aLine4, (aCenter - g->GetFont()->StringWidth(aLine4)) / 2 + aCenter, 0xb2);
	SexyString aClosing = GT(0x0049633c);
	g->DrawString(aClosing, (mWidth - 10 - g->GetFont()->StringWidth(aClosing)) / 2, 0x112);
}
