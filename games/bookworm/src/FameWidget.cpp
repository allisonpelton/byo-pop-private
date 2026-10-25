#include "FameWidget.h"
#include "BookWormApp.h"
#include "Board.h"
#include "Res.h"

#include "Graphics.h"
#include "Font.h"
#include "ImageFont.h"
#include "Image.h"
#include "DDImage.h"
#include "ButtonWidget.h"
#include "Checkbox.h"
#include "WidgetManager.h"
#include "MusicInterface.h"

#include <cstdlib>

using namespace Sexy;

// Static colours of FameWidget.cpp (file-scope Colors built by the dynamic initialisers 0x4f71d0..0x4f7300)
static const Color gFameClassicScoreColor(255, 0, 0);      // 0x534a70
static const Color gFameClassicNameColor(140, 90, 0);      // 0x534a60
static const Color gFameActionScoreColor(255, 0, 0);       // 0x534a90
static const Color gFameActionNameColor(140, 90, 0);       // 0x534aa0
static const Color gFameBestWordColor(160, 60, 0);         // 0x534a80
static const Color gFameBestScoreColor(160, 60, 0);        // 0x534ad0
static const Color gFameBestNameColor(255, 130, 80);       // 0x534a50
static const Color gFameLongestWordColor(160, 60, 0);      // 0x534ab0
static const Color gFameLongestLengthColor(160, 60, 0);    // 0x534b30
static const Color gFameLongestNameColor(255, 130, 80);    // 0x534ac0

// Height of one row of the internet page's tab label strips, of one half of the period indicator
static const int FAME_TAB_ROW_HEIGHT = 23;
static const int FAME_INDICATOR_HEIGHT = 16;

// 0x428520
FameWidget::FameWidget(BookWormApp* theApp)
{
	mApp = theApp;
	mMode = FAME_MODE_LOCAL;
	mTab = 0;
	mPeriod = 0;
	// 0x42863c mMonth, the ctor-only bool +0x78, the HTTP error code and mNetState = 0, then "has a game to
	// submit" = board game log non-empty and game score > 0, "game log signed" = false: internet, not ported

	for (int i = 0; i < NUM_FAME_TABS; i++)
	{
		mTabImages[i] = NULL;
		mTabOverImages[i] = NULL;
		mTabSelectedImages[i] = NULL;
		mTabButtons[i] = NULL;
	}
	mIndicatorOffImage = NULL;
	mIndicatorOnImage = NULL;
	mContinueButton = NULL;
	for (int i = 0; i < NUM_FAME_PERIODS; i++)
		mPeriodBoxes[i] = NULL;

	// The internet page's tab labels: ihs_tab_names holds the three normal labels, then the three selected
	// ones, in rows of 23 pixels; ihs_tab_names_hilite the three mouse-over labels
	Image* aTabNames = mApp->mImages[IMAGE_IHS_TAB_NAMES];
	int aTabWidth = aTabNames->GetWidth();
	for (int i = 0; i < NUM_FAME_TABS; i++)
		mTabImages[i] = mApp->CopyImage(aTabNames, Rect(0, FAME_TAB_ROW_HEIGHT * i, aTabWidth, FAME_TAB_ROW_HEIGHT));
	for (int i = 0; i < NUM_FAME_TABS; i++)
		mTabSelectedImages[i] = mApp->CopyImage(aTabNames,
			Rect(0, FAME_TAB_ROW_HEIGHT * (NUM_FAME_TABS + i), aTabWidth, FAME_TAB_ROW_HEIGHT));

	Image* aTabNamesHilite = mApp->mImages[IMAGE_IHS_TAB_NAMES_HILITE];
	aTabWidth = aTabNamesHilite->GetWidth();
	for (int i = 0; i < NUM_FAME_TABS; i++)
		mTabOverImages[i] = mApp->CopyImage(aTabNamesHilite,
			Rect(0, FAME_TAB_ROW_HEIGHT * i, aTabWidth, FAME_TAB_ROW_HEIGHT));

	// The period checkboxes' indicator: unchecked on top, checked below
	Image* anIndicator = mApp->mImages[IMAGE_IHS_TAB_INDICATOR];
	int anIndicatorWidth = anIndicator->GetWidth();
	mIndicatorOffImage = mApp->CopyImage(anIndicator, Rect(0, 0, anIndicatorWidth, FAME_INDICATOR_HEIGHT));
	mIndicatorOnImage = mApp->CopyImage(anIndicator,
		Rect(0, FAME_INDICATOR_HEIGHT, anIndicatorWidth, FAME_INDICATOR_HEIGHT));

	if (mContinueButton == NULL)
	{
		Image* aContinueImage = mApp->mImages[IMAGE_BUTTON_FAME_CONTINUE];
		mContinueButton = new ButtonWidget(FAME_ID_CONTINUE, this);
		mContinueButton->Resize(0, 0, aContinueImage->GetWidth(), aContinueImage->GetHeight());
		mContinueButton->mDoFinger = true;
		mContinueButton->mButtonImage = aContinueImage;
		mContinueButton->mHasAlpha = true;
		// The original sets no font: the label is empty, and the 2004 ButtonWidget creates its fallback font
		// only for a label (the port's draws nothing without a font)
	}

	// 0x42898d the internet button (id 8, button_goto_ihs), created when mApp->IsInternetScoresEnabled(),
	// which is false in the registered build: internet, not ported

	// The internet page's table tabs, created hidden
	for (int i = 0; i < NUM_FAME_TABS; i++)
	{
		if ((mTabButtons[i] == NULL) && (mTabImages[i] != NULL))
		{
			ButtonWidget* aTabButton = new ButtonWidget(FAME_ID_TAB_CLASSIC + i, this);
			mTabButtons[i] = aTabButton;
			aTabButton->mButtonImage = mTabImages[i];
			aTabButton->mOverImage = mTabOverImages[i];
			aTabButton->Resize(0, 0, mTabImages[i]->GetWidth(), mTabImages[i]->GetHeight());
			aTabButton->mDoFinger = true;
			aTabButton->mHasAlpha = true;
			aTabButton->SetVisible(false);
		}
	}

	// The internet page's period checkboxes (a radio group, CheckboxChecked), created hidden
	if ((mIndicatorOffImage != NULL) && (mIndicatorOnImage != NULL))
	{
		for (int i = 0; i < NUM_FAME_PERIODS; i++)
		{
			if (mPeriodBoxes[i] == NULL)
			{
				Checkbox* aBox = new Checkbox(mIndicatorOffImage, mIndicatorOnImage, FAME_ID_PERIOD_TODAY + i, this);
				mPeriodBoxes[i] = aBox;
				aBox->Resize(0, 0, 17, 16);
				aBox->SetVisible(false);
				aBox->mHasAlpha = true;
				aBox->mMouseInsets.mRight = -96;  // the click area reaches 96 px right, over the label text
			}
		}
	}

	FameWidget::SetTabAndPeriod(0, 0);
	// Runs CheckboxChecked(FAME_ID_PERIOD_TODAY, true): unchecks the other two and selects period 0, without
	// the click sound (the box is not under the mouse)
	mPeriodBoxes[0]->SetChecked(true, true);

	// 0x428dea the resubmit ImagelessButton (id 0, MyriadWeb10, black/red/grey, 100x20, never shown):
	// internet, not ported
	// 0x428f06 mWaitDialog = NULL: internet, not ported
}

// 0x4270d0 (scalar deleting dtor 0x428f20). The widgets were already removed from the manager by
// RemovedFromManager. Leaving the Hall of Fame ends the highlight of the new entries.
FameWidget::~FameWidget()
{
	delete mTabImages[0];
	delete mTabImages[1];
	delete mTabImages[2];
	delete mTabOverImages[0];
	delete mTabOverImages[1];
	delete mTabSelectedImages[0];
	delete mTabSelectedImages[1];
	delete mTabSelectedImages[2];
	delete mTabOverImages[2];
	delete mIndicatorOffImage;
	delete mIndicatorOnImage;

	// +0x26c the RESUBMIT button: not ported
	delete mContinueButton;
	// +0x274 the internet button: not ported
	for (int i = 0; i < NUM_FAME_TABS; i++)
		delete mTabButtons[i];
	for (int i = 0; i < NUM_FAME_PERIODS; i++)
		delete mPeriodBoxes[i];

	// The pulse of the entries the last game added lasts exactly one visit of the Hall of Fame
	HighScoreByScoreSet::iterator anItr;
	for (anItr = mApp->mClassicScores.begin(); anItr != mApp->mClassicScores.end(); ++anItr)
		anItr->mIsNew = false;
	for (anItr = mApp->mActionScores.begin(); anItr != mApp->mActionScores.end(); ++anItr)
		anItr->mIsNew = false;
	for (anItr = mApp->mBestWords.begin(); anItr != mApp->mBestWords.end(); ++anItr)
		anItr->mIsNew = false;
	for (HighScoreByLengthSet::iterator aLengthItr = mApp->mLongestWords.begin();
		aLengthItr != mApp->mLongestWords.end(); ++aLengthItr)
		aLengthItr->mIsNew = false;

	// 0x4273dc KillDialog of mWaitDialog (dialog 25), the new-places list, the downloaded tables, the
	// HTTPTransfer and the reply strings: internet, not ported
}

// 0x421cb0. Pauses the board under the page, puts the buttons on top of it and cross-fades from the menu
// theme to the hall-of-fame theme.
void FameWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	if (mApp->mBoard != NULL)
		mApp->mBoard->Pause(true);

	Widget::AddedToManager(theWidgetManager);

	// +0x26c the RESUBMIT button: not ported
	if (mContinueButton != NULL)
		theWidgetManager->AddWidget(mContinueButton);
	// +0x274 the internet button: not ported
	for (int i = 0; i < NUM_FAME_TABS; i++)
	{
		if (mTabButtons[i] != NULL)
			theWidgetManager->AddWidget(mTabButtons[i]);
	}
	for (int i = 0; i < NUM_FAME_PERIODS; i++)
	{
		if (mPeriodBoxes[i] != NULL)
			theWidgetManager->AddWidget(mPeriodBoxes[i]);
	}

	// With the app's double music Update each fade takes about 0.63 s
	mApp->mMusicInterface->FadeOut(SONG_MENU, false, 0.008);
	mApp->mMusicInterface->FadeIn(SONG_JINGLE, ORDER_FAME, 0.008, false);
}

// 0x421dc0
void FameWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);

	// +0x26c the RESUBMIT button: not ported
	if (mContinueButton != NULL)
		theWidgetManager->RemoveWidget(mContinueButton);
	// +0x274 the internet button: not ported
	for (int i = 0; i < NUM_FAME_TABS; i++)
	{
		if (mTabButtons[i] != NULL)
			theWidgetManager->RemoveWidget(mTabButtons[i]);
	}
	for (int i = 0; i < NUM_FAME_PERIODS; i++)
	{
		if (mPeriodBoxes[i] != NULL)
			theWidgetManager->RemoveWidget(mPeriodBoxes[i]);
	}

	if (mApp->mBoard != NULL)
		mApp->mBoard->Pause(false);
}

// 0x421990
void FameWidget::SetVisible(bool isVisible)
{
	Widget::SetVisible(isVisible);

	if (mContinueButton != NULL)
		mContinueButton->SetVisible(isVisible);
	// +0x274 the internet button: not ported

	ShowInternetControls(isVisible);
}

// 0x421950
void FameWidget::SetDisabled(bool isDisabled)
{
	Widget::SetDisabled(isDisabled);

	if (mContinueButton != NULL)
		mContinueButton->SetDisabled(isDisabled);
	// +0x274 the internet button: not ported
}

// 0x4219e0. Shows or hides the internet page's tabs and period checkboxes.
void FameWidget::ShowInternetControls(bool show)
{
	for (int i = 0; i < NUM_FAME_TABS; i++)
	{
		if (mTabButtons[i] != NULL)
			mTabButtons[i]->SetVisible(show);
	}
	for (int i = 0; i < NUM_FAME_PERIODS; i++)
	{
		if (mPeriodBoxes[i] != NULL)
			mPeriodBoxes[i]->SetVisible(show);
	}
}

// 0x421860. Every tab gets its normal label, the selected one (0..2) its selected label; no MarkDirty.
void FameWidget::SetTab(int theTab)
{
	mTab = theTab;

	for (int i = 0; i < NUM_FAME_TABS; i++)
	{
		if (mTabButtons[i] != NULL)
			mTabButtons[i]->mButtonImage = mTabImages[i];
	}

	if ((theTab >= 0) && (theTab < NUM_FAME_TABS) && (mTabButtons[theTab] != NULL))
		mTabButtons[theTab]->mButtonImage = mTabSelectedImages[theTab];
}

// 0x421910
void FameWidget::SetPeriod(int thePeriod)
{
	mPeriod = thePeriod;
}

// 0x421920
void FameWidget::SetTabAndPeriod(int theTab, int thePeriod)
{
	SetTab(theTab);
	SetPeriod(thePeriod);
}

// 0x421a50. The layout (the "/ 2" are signed integer divisions): Continue centred 16 px above the bottom
// ((167, 422) at 640x480), the hidden tabs and checkboxes where the internet page shows them.
void FameWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);

	// 0x421a6d the RESUBMIT button Move(275, 140): not ported

	if (mContinueButton != NULL)
		mContinueButton->Move((mWidth - mContinueButton->mWidth) / 2, mHeight - mContinueButton->mHeight - 16);

	if (mMode == FAME_MODE_LOCAL)
	{
		// 0x421ac0 the internet button's image (button_goto_ihs, or button_submit_ihs with a game to submit):
		// not ported
		ShowInternetControls(false);
	}
	else if (mMode == FAME_MODE_INTERNET)
	{
		// 0x421aed the internet button's image (button_goto_fame): not ported
		ShowInternetControls(true);
	}

	// 0x421b47 the internet button Resize(592 - w / 2, 40 - h / 2, w, h): not ported

	static const int aTabCenterX[NUM_FAME_TABS] = { 140, 322, 495 };
	for (int i = 0; i < NUM_FAME_TABS; i++)
	{
		ButtonWidget* aTabButton = mTabButtons[i];
		if (aTabButton != NULL)
			aTabButton->Move(aTabCenterX[i] - aTabButton->mWidth / 2, 83 - aTabButton->mHeight / 2);
	}

	static const int aBoxX[NUM_FAME_PERIODS] = { 102, 277, 436 };
	for (int i = 0; i < NUM_FAME_PERIODS; i++)
	{
		if (mPeriodBoxes[i] != NULL)
			mPeriodBoxes[i]->Move(aBoxX[i], 117);
	}
}

// 0x421f00
void FameWidget::Draw(Graphics* g)
{
	if (mMode == FAME_MODE_LOCAL)
		DrawLocalScores(g);
	else if (mMode == FAME_MODE_INTERNET)
	{
		// 0x421f1b DrawInternetScores (slot 48, 0x425880): internet, not ported
	}
}

// 0x424c30. The Hall of Fame page: the backdrop (which holds the titles) and the four tables of the app.
// Every entry is copied before use. Strings are drawn at their baseline; an entry added by the last game
// (mIsNew) gets a black drop shadow whose alpha pulses.
void FameWidget::DrawLocalScores(Graphics* g)
{
	// A triangle wave 0..255..0 with a period of 127.5 updates (C %: mUpdateCnt is never negative)
	int aPulse = 255 - abs((mUpdateCnt * 4) % 510 - 255);

	g->DrawImage(mApp->mImages[IMAGE_BACKDROP_FAME], 0, 0);
	Widget::Draw(g);

	g->SetFont(mApp->mFonts[FONT_LEGACYSERIFMDSCITCTT12]);
	int aFontHeight = g->GetFont()->GetHeight();

	// Classic top 10, left column: score right-aligned at x 96, name (cut to 90 px) from x 106. The row
	// step is (int)(h * -1.5) (truncated toward zero) times the row: 152 + 27 * row for h = 18.
	int aRow = 0;
	for (HighScoreByScoreSet::const_iterator anItr = mApp->mClassicScores.begin();
		anItr != mApp->mClassicScores.end(); ++anItr, aRow++)
	{
		if (aRow > 9)
			break;

		HighScoreEntry anEntry = *anItr;
		SexyString aText = StrFormat("%d", anEntry.mScore);
		int aY = 152 - (int)(aFontHeight * -1.5) * aRow;
		DrawShadowedString(g, 96 - g->StringWidth(aText), aY, aText, gFameClassicScoreColor, anEntry.mIsNew,
			Color(0, 0, 0, aPulse));

		aText = anEntry.mName;
		aText = TruncateToWidth(g, aText, 90, "...");
		DrawShadowedString(g, 106, aY, aText, gFameClassicNameColor, anEntry.mIsNew, Color(0, 0, 0, aPulse));
	}

	// Action top 10, right column: name (cut to 90 px) right-aligned at x 538, score from x 545. The name's
	// y truncates the whole expression once, the score's is the classic column's: for an odd font height
	// they differ by up to row / 2 px (both are 152 + 27 * row for h = 18).
	aRow = 0;
	for (HighScoreByScoreSet::const_iterator anItr = mApp->mActionScores.begin();
		anItr != mApp->mActionScores.end(); ++anItr, aRow++)
	{
		if (aRow > 9)
			break;

		HighScoreEntry anEntry = *anItr;
		SexyString aText = anEntry.mName;
		aText = TruncateToWidth(g, aText, 90, "...");
		int aNameY = (int)((double)aFontHeight * 1.5 * aRow + 152.0);
		DrawShadowedString(g, 538 - g->StringWidth(aText), aNameY, aText, gFameActionNameColor, anEntry.mIsNew,
			Color(0, 0, 0, aPulse));

		aText = StrFormat("%d", anEntry.mScore);
		DrawShadowedString(g, 545, 152 - (int)(aFontHeight * -1.5) * aRow, aText, gFameActionScoreColor,
			anEntry.mIsNew, Color(0, 0, 0, aPulse));
	}

	// The 3 best words, centre: the word centred at y 122 + 2h * row, below it the score right-aligned at
	// x 400 and the name from x 242, cut so that it ends 140 px from x 242 minus the score's width
	aRow = 0;
	int aWordY = 122;
	int aLineY = 122 + aFontHeight;
	for (HighScoreByScoreSet::const_iterator anItr = mApp->mBestWords.begin();
		anItr != mApp->mBestWords.end(); ++anItr)
	{
		if (aRow > 2)
			break;

		HighScoreEntry anEntry = *anItr;
		SexyString aText = anEntry.mWord;
		DrawShadowedString(g, (mWidth - g->StringWidth(aText)) / 2, aWordY, aText, gFameBestWordColor,
			anEntry.mIsNew, Color(0, 0, 0, aPulse));

		aText = StrFormat("%d", anEntry.mScore);
		DrawShadowedString(g, 400 - g->StringWidth(aText), aLineY, aText, gFameBestScoreColor, anEntry.mIsNew,
			Color(0, 0, 0, aPulse));
		int aScoreWidth = g->StringWidth(aText);

		aText = anEntry.mName;
		aText = TruncateToWidth(g, aText, 140 - aScoreWidth, "...");
		DrawShadowedString(g, 242, aLineY, aText, gFameBestNameColor, anEntry.mIsNew, Color(0, 0, 0, aPulse));

		aRow++;
		aWordY += aFontHeight * 2;
		aLineY += aFontHeight * 2;
	}

	// The 3 longest words: the same layout from y 287; the number is the word's length, not the stored score
	aRow = 0;
	aWordY = 287;
	aLineY = 287 + aFontHeight;
	for (HighScoreByLengthSet::const_iterator anItr = mApp->mLongestWords.begin();
		anItr != mApp->mLongestWords.end(); ++anItr)
	{
		if (aRow > 2)
			break;

		HighScoreEntry anEntry = *anItr;
		SexyString aText = anEntry.mWord;
		DrawShadowedString(g, (mWidth - g->StringWidth(aText)) / 2, aWordY, aText, gFameLongestWordColor,
			anEntry.mIsNew, Color(0, 0, 0, aPulse));

		aText = StrFormat("%d", (int)anEntry.mWord.size());
		DrawShadowedString(g, 400 - g->StringWidth(aText), aLineY, aText, gFameLongestLengthColor,
			anEntry.mIsNew, Color(0, 0, 0, aPulse));
		int aLengthWidth = g->StringWidth(aText);

		aText = anEntry.mName;
		aText = TruncateToWidth(g, aText, 140 - aLengthWidth, "...");
		DrawShadowedString(g, 242, aLineY, aText, gFameLongestNameColor, anEntry.mIsNew, Color(0, 0, 0, aPulse));

		aRow++;
		aWordY += aFontHeight * 2;
		aLineY += aFontHeight * 2;
	}
}

// 0x427f30
void FameWidget::Update()
{
	Widget::Update();

	// What animates the pulse: about 8.3 redraws a second while the app has the focus, the alpha being
	// taken from mUpdateCnt at draw time
	if ((mApp->mHasFocus) && (mUpdateCnt % 12 == 0))
		MarkDirty();

	// 0x427f7e the internet button's blink (button_submit_ihs and its hilite, (mUpdateCnt % 64) < 24): not
	// ported
	// 0x427fe9 switch (mNetState): the transfer states 1..6 (download, post, parse, error dialogs, switch to
	// the internet page); 0 and 7 do nothing: internet, not ported
}

// 0x4220c0 (the same body as HelpWidget's)
void FameWidget::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x426c10
void FameWidget::ButtonDepress(int theId)
{
	// 0x426c32 the resubmit button (id 0, never visible): reads the first line of thing.txt with an fstream
	// and posts it with "&clientver=" to SubmitScoreURL: not ported

	switch (theId)
	{
	case FAME_ID_CONTINUE:
	{
		// RemoveFameWidget queues this widget for a deferred delete (SafeDeleteWidget)
		BookWormApp* anApp = mApp;

		// Back to the menu theme, from where it was
		anApp->mMusicInterface->FadeOut(SONG_JINGLE, true, 0.008);
		anApp->mMusicInterface->FadeIn(SONG_MENU, -1, 0.008, false);
		SetVisible(false);
		SetDisabled(true);
		anApp->RemoveFameWidget();

		// Opened from the game selector, the selector is still underneath; after a game over it is not
		if (anApp->mGameSelector == NULL)
			anApp->ShowGameSelector();
		break;
	}

	case FAME_ID_INTERNET:
		// 0x426eb4 the internet button: the register nag (dialog 17), the internet scores or the alias
		// prompt, switching between the pages: internet, not ported
		break;

	case FAME_ID_TAB_CLASSIC:
	case FAME_ID_TAB_ACTION:
	case FAME_ID_TAB_WORDS:
		SetTabAndPeriod(theId - FAME_ID_TAB_CLASSIC, mPeriod);
		break;

	default:
		break;
	}
}

// 0x421f50. The period checkboxes are a radio group: checking one unchecks the other two (with the click
// sound when it was clicked); clicking the checked one checks it again, while a box unchecked by code (not
// under the mouse) stays unchecked.
void FameWidget::CheckboxChecked(int theId, bool checked)
{
	int aPeriod = theId - FAME_ID_PERIOD_TODAY;
	if ((aPeriod < 0) || (aPeriod >= NUM_FAME_PERIODS))
		return;

	Checkbox* aBox = mPeriodBoxes[aPeriod];
	if (checked)
	{
		for (int i = 0; i < NUM_FAME_PERIODS; i++)
		{
			if ((i != aPeriod) && (mPeriodBoxes[i] != NULL))
				mPeriodBoxes[i]->SetChecked(false, true);
		}
		SetPeriod(aPeriod);

		if ((aBox != NULL) && (aBox->mIsOver))
			mApp->PlaySample(SOUND_BUTTONCLICK);
	}
	else if ((aBox != NULL) && (aBox->mIsOver))
		aBox->SetChecked(true, true);
}
