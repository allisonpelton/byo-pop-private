// The dialogs of Noah's Ark Deluxe (see Dialogs.h).

#include "Dialogs.h"
#include "NoahApp.h"
#include "Button2003.h"
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

#ifdef __EMSCRIPTEN__
#include "WebProfiles.h"
#endif

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------
// DifficultyDialog

// 0x410c00
DifficultyDialog::DifficultyDialog(NoahApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_DIFFICULTY, true,
		GT(0x00494410), GT(0x0049c510), GT(0x0049441c), BUTTONS_FOOTER),
	mApp(theApp)
{
	Use2003Buttons(this, theApp->mImages[IMAGE_DBUTTON]);
	SetButtonFont(mApp->mFonts[FONT_SMALL]);
	SetHeaderFont(mApp->mFonts[FONT_MEDIUM]);
	SetLinesFont(mApp->mFonts[FONT_SMALL]);

	mEasyButton = new DialogButton2003(mApp->mImages[IMAGE_DBUTTON], 0x3c, this);
	mEasyButton->SetFont(mApp->mFonts[FONT_SMALL]);
	mEasyButton->mLabel = GT(0x00494408);
	mMediumButton = new DialogButton2003(mApp->mImages[IMAGE_DBUTTON], 0x3d, this);
	mMediumButton->SetFont(mApp->mFonts[FONT_SMALL]);
	mMediumButton->mLabel = GT(0x00494400);
	mHardButton = new DialogButton2003(mApp->mImages[IMAGE_DBUTTON], 0x3e, this);
	mHardButton->SetFont(mApp->mFonts[FONT_SMALL]);
	mHardButton->mLabel = GT(0x004943f8);
}

// 0x4110a0: the destructor deletes the three buttons
DifficultyDialog::~DifficultyDialog()
{
	delete mEasyButton;
	delete mMediumButton;
	delete mHardButton;
}

// 0x411140
void DifficultyDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mEasyButton);
	theWidgetManager->AddWidget(mMediumButton);
	theWidgetManager->AddWidget(mHardButton);
}

// 0x411180
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

// 0x411130: slot 47 of the original, a function that only returns 0x140
int DifficultyDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x140;
}

// 0x411290: the dialog's own press, then a click through the app
void DifficultyDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	mApp->ButtonPress(0);
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
		GT(0x0049444c), GT(0x0049c510), GT(0x00494458), BUTTONS_FOOTER),
	mApp(theApp)
{
	Use2003Buttons(this, theApp->mImages[IMAGE_DBUTTON]);
	mScores = new HighScoreWidget(theApp);
	SetButtonFont(mApp->mFonts[FONT_SMALL]);
	SetHeaderFont(mApp->mFonts[FONT_MEDIUM]);
	SetLinesFont(mApp->mFonts[FONT_SMALL]);
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	SetColor(COLOR_BUTTON_TEXT, Color(0xff, 0xff, 0xff));
	SetColor(COLOR_HEADER, Color(0xff, 0xff, 0xff));
}

int HighScoreDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x12c;                                       // 0x411730
}

// 0x4116c0: the destructor deletes the dialog's list widget
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

// 0x41d750: OK and CANCEL buttons (mode 2, no footer), and an edit widget with no background of its own: Draw puts
// the edit box picture under it. Its colours are a table at 0x4960a4 (background and outline fully transparent, text
// 240/240/255, highlight white, highlighted text black), and then the highlighted text is set to white; the name may
// be 120 pixels wide (the field at +0x80), with no limit on the number of characters.
RecordScoreDialog::RecordScoreDialog(NoahApp* theApp, const std::string& thePlace) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_RECORD_SCORE, true,
		GT(0x00495ef0), thePlace, GT(0x0049c510), BUTTONS_OK_CANCEL),
	mApp(theApp)
{
	Use2003Buttons(this, theApp->mImages[IMAGE_DBUTTON]);
	SetButtonFont(mApp->mFonts[FONT_SMALL]);
	SetHeaderFont(mApp->mFonts[FONT_MEDIUM]);
	SetLinesFont(mApp->mFonts[FONT_SMALL]);
	mNameEdit = new EditWidget(0, this);
	mNameEdit->SetFont(mApp->mFonts[FONT_SMALL]);
	mNameEdit->SetColor(EditWidget::COLOR_BKG, Color(0, 0, 0, 0));
	mNameEdit->SetColor(EditWidget::COLOR_OUTLINE, Color(0, 0, 0, 0));
	mNameEdit->SetColor(EditWidget::COLOR_TEXT, Color(240, 240, 255, 255));
	mNameEdit->SetColor(EditWidget::COLOR_HILITE, Color(255, 255, 255, 255));
	mNameEdit->SetColor(EditWidget::COLOR_HILITE_TEXT, Color(0, 0, 0, 255));
	mNameEdit->mMaxPixels = 0x78;
	mNameEdit->SetText(mApp->mLastScoreName);
#ifdef __EMSCRIPTEN__
	// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with the site's player
	Byopop::SiteType("noahsark", mNameEdit, false);
#endif
	mNameEdit->SetColor(EditWidget::COLOR_HILITE_TEXT, Color(0xff, 0xff, 0xff));
}

// 0x41d9d0: the destructor deletes the edit widget
RecordScoreDialog::~RecordScoreDialog()
{
	delete mNameEdit;
}

// 0x41da40: the edit widget is added and takes the focus
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

// 0x41da70
void RecordScoreDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mNameEdit->Resize(mX + 0x30, mY + mHeight - 0x78, mWidth - 0x60, 0x18);       // 0x41da70
}

// slot 47 of the dialog classes is a function that returns a constant (Ghidra shows none): 0x411730, shared by this
// dialog and the high-score list, 300; the Options and Tutorial dialogs share 0x4209f0, 438
int RecordScoreDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x12c;
}

// 0x41dac0: the edit box picture around the edit widget (4 pixels more on each side), and the prompt over it
void RecordScoreDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	DrawEditBox(g, mNameEdit->mX - mX - 4, mNameEdit->mY - mY - 4, mNameEdit->mWidth + 8);
	g->SetFont(mApp->mFonts[FONT_GARAMOND_SMALL]);
	g->SetColor(Color(0xa4, 0x2c, 0));
	SexyString aPrompt = GT(0x00496120);
	g->DrawString(aPrompt, (mWidth - g->GetFont()->StringWidth(aPrompt)) / 2, mNameEdit->mY - mY - 0x13);
}

// 0x41dca0: a strip of the edit box picture (a left end, a right end and the middle piece repeated between them,
// all 0x20 high) from x to x + theWidth. The middle pieces are clipped to the part right of the left end, and are
// drawn after the right end.
void RecordScoreDialog::DrawEditBox(Graphics* g, int theX, int theY, int theWidth)
{
	Image* aBox = mApp->mImages[IMAGE_EDITBOX];
	g->DrawImage(aBox, theX, theY, Rect(0, 0, 0x20, 0x20));
	g->DrawImage(aBox, theWidth - 0x20 + theX, theY, Rect(0x40, 0, 0x20, 0x20));
	Graphics aClipped(*g);
	aClipped.ClipRect(theX + 0x20, theY, theWidth - 0x20, 0x20);
	int aCount = (theWidth - 0x21) / 0x20;
	for (int i = 0; i < aCount; i++)
		aClipped.DrawImage(aBox, theX + 0x20 + i * 0x20, theY, Rect(0x20, 0, 0x20, 0x20));
}

// 0x41de20: Enter in the name box is a click on the dialog's OK button
void RecordScoreDialog::EditWidgetText(int theId, const SexyString& theString)
{
	(void) theId;
	(void) theString;
	mApp->ButtonDepress(mId + 2000);
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
	Use2003Buttons(this, theApp->mImages[IMAGE_DBUTTON]);
	SetButtonFont(mApp->mFonts[FONT_GARAMOND_SMALL]);
	SetHeaderFont(mApp->mFonts[FONT_MEDIUM]);
	SetLinesFont(mApp->mFonts[FONT_SMALL]);

	// (the original's Register and Web buttons, ids 1 and 3, are gone; the others keep their ids)
	struct Spec { DialogButton** mButton; int mId; unsigned int mText; };
	Spec aSpecs[3] =
	{
		{&mHelpButton, 0, 0x00495fd0}, {&mScoresButton, 2, 0x00495fb8}, {&mCreditsButton, 4, 0x00495f9c}
	};
	for (int i = 0; i < 3; i++)
	{
		DialogButton* aButton = new DialogButton2003(mApp->mImages[IMAGE_DBUTTON], aSpecs[i].mId, this);
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

// 0x41cc40: the destructor deletes the buttons, the two sliders and the two check boxes (the original also deletes its
// Register and Web buttons, which the port does not have)
OptionsDialog::~OptionsDialog()
{
	delete mHelpButton;
	delete mScoresButton;
	delete mCreditsButton;
	delete mMusicSlider;
	delete mSfxSlider;
	delete mFullscreenCheckbox;
	delete mHintsCheckbox;
}

// 0x41cdc0
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog2004::AddedToManager(theWidgetManager);
	DialogButton* aButtons[3] = {mHelpButton, mScoresButton, mCreditsButton};
	for (int i = 0; i < 3; i++)
		theWidgetManager->AddWidget(aButtons[i]);
	theWidgetManager->AddWidget(mMusicSlider);
	theWidgetManager->AddWidget(mSfxSlider);
	theWidgetManager->AddWidget(mFullscreenCheckbox);
	theWidgetManager->AddWidget(mHintsCheckbox);
}

// 0x41ce60
void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog2004::RemovedFromManager(theWidgetManager);
	DialogButton* aButtons[3] = {mHelpButton, mScoresButton, mCreditsButton};
	for (int i = 0; i < 3; i++)
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
	// Help and Scores side by side, Credits across: the rows of Register and Web are gone, the dialog is that much shorter
	mHelpButton->Resize(mX + mContentInsets.mLeft, aFooterY - 0x54, aHalf, 0x24);
	mScoresButton->Resize(mX + mContentInsets.mLeft + 6 + aHalf, aFooterY - 0x54, aHalf, 0x24);
	mCreditsButton->Resize(mX + mContentInsets.mLeft, aFooterY - 0x2a, aContent, 0x24);
	mMusicSlider->Resize(mX + 0x98, mY + 0x4e, mWidth - 0xc0, 0x20);
	mSfxSlider->Resize(mX + 0x98, mY + 0x6e, mWidth - 0xc0, 0x20);
	mFullscreenCheckbox->Resize(mX + 0x30, mY + 0x94, 0x24, 0x25);
	mHintsCheckbox->Resize(mX + 0x30, mY + 0xc2, 0x24, 0x25);
}

int OptionsDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x18c;                                       // 0x4209f0 gives 0x1b6; less the row of the Web button
}

// 0x41cd40: slot 7 of the original: the button text colour (index 3) also goes to its buttons
void OptionsDialog::SetColor(int theIdx, const Color& theColor)
{
	Dialog2004::SetColor(theIdx, theColor);
	if (theIdx == COLOR_BUTTON_TEXT)
	{
		DialogButton* aButtons[3] = {mHelpButton, mScoresButton, mCreditsButton};
		for (int i = 0; i < 3; i++)
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

// 0x41d3e0: the dialog's own press, then a click through the app
void OptionsDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	mApp->ButtonPress(0);
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
	case 2:
		mApp->ShowHighScores();
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

// 0x4205b0: the dialog has no header and no lines of its own (Draw writes the heading), and its button reads READY
TutorialDialog::TutorialDialog(NoahApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_TUTORIAL, true,
		GT(0x0049c510), GT(0x0049c510), GT(0x004962e4), BUTTONS_FOOTER),
	mApp(theApp),
	mWait(0),
	mStep(0),
	mCursorX(0x14),
	mCursorY(0x4e),
	mClicked(false)
{
	Use2003Buttons(this, theApp->mImages[IMAGE_DBUTTON]);
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

// 0x420980: the destructor deletes the check box
TutorialDialog::~TutorialDialog()
{
	delete mAgainCheckbox;
}

// 0x411290: the dialog's own press, then a click through the app
void TutorialDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	mApp->ButtonPress(0);
}

// 0x420a00
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
	mAgainCheckbox->Resize(mX + 0x30, mY + mWidth - 0x3c, 0x24, 0x25);   // the width, not the height: as the original has it
}

int TutorialDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x1b6;                                       // 0x4209f0, the same function as the Options dialog's
}

// 0x420ab0: the picture's little animation. The cursor goes to the left animal (step 0) and clicks it (1), goes to the
// matching animal (2) and clicks it (3), then the pair walks to the ark and the picture waits (4), and starts again.
void TutorialDialog::Update()
{
	Dialog2004::Update();
	int aHalf = (mWidth - 10) / 2;
	switch (mStep)
	{
	case 0:
		if ((aHalf - 0x32) / 2 + 0x19 <= mCursorX)
		{
			mApp->PlaySample(SOUND_CLICK);
			mStep = 1;
			mClicked = true;
			MarkDirty();
			return;
		}
		mCursorX += 2;
		if (mCursorY < 100)
		{
			mCursorY++;
			MarkDirty();
			return;
		}
		break;
	case 1:
	case 3:
		if (mClicked)
		{
			if (mCursorY < 0x6a)
			{
				mCursorX++;
				mCursorY++;
				MarkDirty();
				return;
			}
			mClicked = false;
			MarkDirty();
			return;
		}
		if (100 < mCursorY)
		{
			mCursorY--;
			mCursorX--;
			MarkDirty();
			return;
		}
		if (mStep == 1)
		{
			mStep = 2;
			MarkDirty();
			return;
		}
		mStep = 4;
		mWait = 0;
		mApp->PlaySample(SOUND_WINELEPHANT);
		MarkDirty();
		return;
	case 2:
		if (mCursorX < (aHalf - 0x32) / 2 + 0x19 + aHalf)
		{
			mCursorX += 2;
			MarkDirty();
			return;
		}
		mApp->PlaySample(SOUND_CLICK);
		mStep = 3;
		mClicked = true;
		MarkDirty();
		return;
	case 4:
	{
		bool aDone = (aHalf - 0x32) / 2 + 0x4b + aHalf <= mCursorX;
		if (!aDone)
			mCursorX += 2;
		mWait++;
		if (299 < mWait)
		{
			mCursorX = 0x14;
			mCursorY = 0x4e;
			mStep = 0;
		}
		if (aDone)
			return;
		break;
	}
	default:
		return;
	}
	MarkDirty();
}

// 0x420d10
void TutorialDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	Image* anAnimals = mApp->mImages[IMAGE_ANIMALS];
	int aHalf = (mWidth - 10) / 2;                      // each half of the picture
	int aMidLeft = (aHalf - 0x32) / 2;                  // an animal centred in the left half

	// the heading and the check box's label, then the line above the picture (0x422330 centres a line on the dialog)
	g->SetFont(mApp->mFonts[FONT_MEDIUM]);
	g->SetColor(Color(0xff, 0xff, 0xff));
	SexyString aHeading = GT(0x00496384);
	g->DrawString(aHeading, (mWidth - g->GetFont()->StringWidth(aHeading)) / 2, 0x30);
	g->SetFont(mApp->mFonts[FONT_GARAMOND_TINY]);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(GT(0x00496364), mAgainCheckbox->mX - mX + 0x30, mAgainCheckbox->mY - mY + 0x1a);
	g->SetFont(mApp->mFonts[FONT_GARAMOND_TINY]);
	SexyString aClosing = GT(0x0049633c);
	g->DrawString(aClosing, (mWidth - g->GetFont()->StringWidth(aClosing)) / 2, 0x4b);

	// the two animals of one kind: each shows its picked (second) row after the click on it; the way between them
	// and its arrow appear with the second click
	g->DrawImage(anAnimals, aMidLeft, 0x5a, Rect(0x96, (mStep < 1) ? 0 : 0x12c, 0x32, 0x32));
	g->DrawImage(anAnimals, aMidLeft + aHalf, 0x5a, Rect(0x96, (mStep < 3) ? 0 : 0x12c, 0x32, 0x32));
	if (mStep >= 3)
	{
		g->DrawImage(mApp->mImages[IMAGE_TRAILLR], aMidLeft + 0x32, 0x5a);
		g->DrawImage(mApp->mImages[IMAGE_TRAILLR], aMidLeft + 100, 0x5a);
		g->DrawImage(mApp->mImages[IMAGE_TRAILLR], aMidLeft + 0x96, 0x5a);
		g->DrawImage(mApp->mImages[IMAGE_TRAILARROWRIGHT], aMidLeft + aHalf, 0x5a);
	}

	g->SetFont(mApp->mFonts[FONT_GARAMOND_TINY]);
	SexyString aLine1 = GT(0x00496328);
	SexyString aLine2 = GT(0x00496318);
	SexyString aLine3 = GT(0x00496304);
	SexyString aLine4 = GT(0x004962f0);
	g->DrawString(aLine1, (aHalf - g->GetFont()->StringWidth(aLine1)) / 2, 0xa0);
	g->DrawString(aLine2, (aHalf - g->GetFont()->StringWidth(aLine2)) / 2, 0xb0);
	g->DrawString(aLine3, (aHalf - g->GetFont()->StringWidth(aLine3)) / 2 + aHalf, 0xa0);
	g->DrawString(aLine4, (aHalf - g->GetFont()->StringWidth(aLine4)) / 2 + aHalf, 0xb0);

	// the two little panels (the tutorial box picture, 0x148) with a 3 by 2 field in each: on the left a pair that
	// can be joined, on the right two that cannot (a red X on the animal in the way)
	Image* aBox = mApp->mImages[IMAGE_TUTORIALBOX];
	int aPanel = (aHalf - aBox->GetWidth()) / 2;
	g->DrawImage(aBox, aPanel, 0xc0);
	g->DrawImage(anAnimals, aPanel + 5, 0xc5, Rect(0x96, 0, 0x32, 0x32));
	g->DrawImage(anAnimals, aPanel + 5, 0xf7, Rect(100, 0, 0x32, 0x32));
	g->DrawImage(anAnimals, aPanel + 0x37, 0xf7, Rect(200, 0, 0x32, 0x32));
	g->DrawImage(anAnimals, aPanel + 0x69, 0xf7, Rect(0x96, 0, 0x32, 0x32));
	g->DrawImage(mApp->mImages[IMAGE_TRAILLR], aPanel + 0x37, 0xc5);
	g->DrawImage(mApp->mImages[IMAGE_TRAILLD], aPanel + 0x69, 0xc5);
	g->SetColorizeImages(true);
	g->SetColor(Color(0x32, 0x32, 0xb9));
	g->DrawImage(mApp->mImages[IMAGE_TRAILLD_OVER], aPanel + 0x69, 0xc5);
	g->SetColorizeImages(false);
	g->DrawImage(mApp->mImages[IMAGE_TRAILARROWDOWN], aPanel + 0x69, 0xf7);

	aPanel = (aHalf - aBox->GetWidth()) / 2 + aHalf;
	g->DrawImage(aBox, aPanel, 0xc0);
	g->DrawImage(anAnimals, aPanel + 5, 0xc5, Rect(0x96, 0, 0x32, 0x32));
	g->DrawImage(anAnimals, aPanel + 0x37, 0xc5, Rect(0x32, 0, 0x32, 0x32));
	g->DrawImage(anAnimals, aPanel + 5, 0xf7, Rect(100, 0, 0x32, 0x32));
	g->DrawImage(anAnimals, aPanel + 0x37, 0xf7, Rect(200, 0, 0x32, 0x32));
	g->DrawImage(anAnimals, aPanel + 0x69, 0xf7, Rect(0x96, 0, 0x32, 0x32));
	g->DrawImage(mApp->mImages[IMAGE_TRAILARROWRIGHT], aPanel + 0x37, 0xc5);
	g->DrawImage(mApp->mImages[IMAGE_TRAILARROWDOWN], aPanel + 5, 0xf7);
	g->SetColor(Color(0xff, 0, 0));
	g->SetFont(mApp->mFonts[FONT_GARAMOND_MED]);
	SexyString anX = GT(0x004962ec);
	g->DrawString(anX, (0x32 - g->GetFont()->StringWidth(anX)) / 2 + 0x37 + aPanel, 0xe8);
	g->DrawString(anX, (0x32 - g->GetFont()->StringWidth(anX)) / 2 + 5 + aPanel, 0x11f);

	g->DrawImage(mApp->mImages[IMAGE_ARROW], mCursorX, mCursorY);
}
