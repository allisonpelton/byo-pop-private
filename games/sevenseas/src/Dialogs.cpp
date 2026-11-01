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

#ifdef __EMSCRIPTEN__
#include "WebProfiles.h"
#endif

using namespace Sexy;

// 0x428360 (Widget slot 31): a line of text centred in a widget of the given width, in the current font
static void DrawCentered(Graphics* g, int theWidth, const SexyString& theText, int theY)
{
	g->DrawString(theText, (theWidth - g->GetFont()->StringWidth(theText)) / 2, theY);
}

// the common look of the larger dialogs: the framed box with the mermaids on top and the skull at the bottom
// (the first lines of the draw functions 0x4192e0 and 0x417ca0)
static void DrawFramed(Graphics* g, SevenSeasApp* theApp, int theWidth, int theHeight)
{
	g->DrawImageBox(Rect(0, 0x37, theWidth, theHeight - 0x37), theApp->mImages[IMAGE_DIALOG]);
	g->DrawImage(theApp->mImages[IMAGE_MERMAIDS], theWidth / 2 - 0x53, 0);
	g->DrawImage(theApp->mImages[IMAGE_DIALOGSKULL], theWidth / 2 - 0x14, theHeight - 0x1f);
}

// ---------------------------------------------------------------------------------------------------------
// GameDialog

// 0x425540
GameDialog::GameDialog(SevenSeasApp* theApp, int theId, bool isModal, const SexyString& theHeader,
	const SexyString& theLines, const SexyString& theFooter, int theButtonMode) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTONBLACK], theId, isModal, theHeader, theLines,
		theFooter, theButtonMode),
	mApp(theApp)
{
	Use2003Buttons(this, theApp->mImages[IMAGE_DBUTTONBLACK]);
}

// 0x425620
void GameDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->DrawImage(mApp->mImages[IMAGE_DIALOGSKULL], mWidth / 2 - 0x14, 0);
}

// 0x425680: the mouse position goes to the side panel too
void GameDialog::MouseMove(int x, int y)
{
	Dialog2004::MouseMove(x, y);
	mApp->FollowMouse(mX + x, y);
}

// ---------------------------------------------------------------------------------------------------------
// OptionsDialog

// 0x418760
OptionsDialog::OptionsDialog(SevenSeasApp* theApp) :
	Dialog2004(theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_OPTIONSBUTTON], DIALOG_OPTIONS, true,
		GT(0x004a3f6c), "", GT(0x004a3590), BUTTONS_FOOTER),
	mApp(theApp)
{
	Use2003Buttons(this, theApp->mImages[IMAGE_OPTIONSBUTTON]);
	SetButtonFont(mApp->mFonts[FONT_MT11]);
	SetHeaderFont(mApp->mFonts[FONT_FONT1]);
	SetLinesFont(mApp->mFonts[FONT_MT11]);
	// the caller's part (0x420620, ShowOptions): the colours and the insets
	SetColor(COLOR_HEADER, Color(0x66, 0x5c, 0x38));
	SetColor(COLOR_LINES, Color(0, 0, 0));
	SetColor(COLOR_BUTTON_TEXT, Color(0xe0, 0xb7, 0x28));
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);

	struct Spec { DialogButton** mButton; int mId; unsigned int mText; };
	Spec aSpecs[4] =
	{
		{&mHelpButton, 0, 0x004a3f64}, {&mRegisterButton, 1, 0x004a3f58}, {&mScoresButton, 2, 0x004a3f4c},
		{&mWebButton, 3, 0x004a3f2c}
	};
	for (int i = 0; i < 4; i++)
	{
		DialogButton* aButton = new DialogButton2003(mApp->mImages[IMAGE_OPTIONSBUTTON], aSpecs[i].mId, this);
		aButton->SetFont(mApp->mFonts[FONT_MT11]);
		aButton->mLabel = GT(aSpecs[i].mText);
		// measured against the original: its labels are the dialog's gold
		aButton->SetColor(DialogButton::COLOR_LABEL, Color(0xe0, 0xb7, 0x28));
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

// 0x418fb0: the height is a constant (mov eax, 0x16b)
int OptionsDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x16b;
}

// 0x419970: the mouse position goes to the side panel too
void OptionsDialog::MouseMove(int x, int y)
{
	Dialog2004::MouseMove(x, y);
	mApp->FollowMouse(mX + x, y);
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
	Use2003Buttons(this, theApp->mImages[IMAGE_OPTIONSBUTTON]);
	SetButtonFont(mApp->mFonts[FONT_MT11]);
	SetHeaderFont(mApp->mFonts[FONT_FONT1]);
	SetLinesFont(mApp->mFonts[FONT_MT11]);
	SetColor(COLOR_HEADER, Color(0x66, 0x5c, 0x38));
	SetColor(COLOR_LINES, Color(0, 0, 0));
	SetColor(COLOR_BUTTON_TEXT, Color(0xe0, 0xb7, 0x28));
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
}

// 0x417c90: a constant
int NextLevelDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x140;
}

// 0x425680: the mouse position goes to the side panel too
void NextLevelDialog::MouseMove(int x, int y)
{
	Dialog2004::MouseMove(x, y);
	mApp->FollowMouse(mX + x, y);
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
	DrawCentered(g, mWidth, aBuf, 0x73);

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
	int aRankX = (mWidth - g->GetFont()->StringWidth(aBuf)) / 2;
	g->DrawString(aBuf, aRankX, 0xc0);
	// the name again in yellow over the gold one
	g->SetColor(Color(0xff, 0xff, 0));
	g->DrawString(HighScoreWidget::RankName(aScore), aRankX + g->GetFont()->StringWidth(GT(0x004a3ea8)), 0xc0);
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
	g->SetColor(Color(0xd3, 0x98, 0));
	sprintf(aBuf, GT(0x004a3e98).c_str(), kNext[aIdx]);
	DrawCentered(g, mWidth, aBuf, 0xe0);
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
	Use2003Buttons(this, theApp->mImages[IMAGE_OPTIONSBUTTON]);
	SetButtonFont(mApp->mFonts[FONT_MT11]);
	SetHeaderFont(mApp->mFonts[FONT_FONT1]);
	SetLinesFont(mApp->mFonts[FONT_MT11]);
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	SetColor(COLOR_LINES, Color(0xf3, 0xb4, 0x4d));
	SetColor(COLOR_BUTTON_TEXT, Color(0xe0, 0xb7, 0x28));
	mNameEdit = new EditWidget(0, this);
	mNameEdit->SetFont(mApp->mFonts[FONT_MT11]);
	// background and outline clear (the dialog draws the box picture under it), text and selection light
	static int sEditColors[5][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {240, 240, 255, 255}, {255, 255, 255, 255}, {0, 0, 0, 255}};
	mNameEdit->SetColors(sEditColors, 5);
	mNameEdit->mMaxPixels = 0x68;
	mNameEdit->SetText(mApp->mLastScoreName);
#ifdef __EMSCRIPTEN__
	// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with the site's player
	Byopop::SiteType("sevenseas", mNameEdit, false);
#endif
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

// 0x41a260: the base dialog's height (0x437750) plus 0x38
int RecordScoreDialog::GetPreferredHeight(int theWidth)
{
	return Dialog2004::GetPreferredHeight(theWidth) + 0x38;
}

// 0x41a480: the edit box picture (96 x 32) in three slices: the left cap, the right cap, and the middle slice
// repeated between them under a clip
static void DrawEditBox(SevenSeasApp* theApp, Graphics* g, int theX, int theY, int theWidth)
{
	Image* anImage = theApp->mImages[IMAGE_EDITBOX];
	g->DrawImage(anImage, theX, theY, Rect(0, 0, 0x20, 0x20));
	g->DrawImage(anImage, theX + theWidth - 0x20, theY, Rect(0x40, 0, 0x20, 0x20));
	Graphics g2(*g);
	g2.ClipRect(theX + 0x20, theY, theWidth - 0x20, 0x20);
	int aCount = (theWidth - 0x21) / 32;
	for (int i = 0; i < aCount; i++)
		g2.DrawImage(anImage, theX + 0x20 + i * 0x20, theY, Rect(0x20, 0, 0x20, 0x20));
}

// 0x41a2f0
void RecordScoreDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	// the box around the name
	DrawEditBox(mApp, g, (mNameEdit->mX - mX) - 4, (mNameEdit->mY - mY) - 4, mNameEdit->mWidth + 8);
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
	Use2003Buttons(this, theApp->mImages[IMAGE_OPTIONSBUTTON]);
	mScores = new HighScoreWidget(theApp);
	SetButtonFont(mApp->mFonts[FONT_MT11]);
	SetHeaderFont(mApp->mFonts[FONT_FONT1]);
	SetLinesFont(mApp->mFonts[FONT_MT11]);
	mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	SetColor(COLOR_BUTTON_TEXT, Color(0xe0, 0xb7, 0x28));
}

HighScoreDialog::~HighScoreDialog()
{
	delete mScores;
}

// 0x40fae0: a constant (the argument the creator gives, 0x1ac, is ignored)
int HighScoreDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return 0x156;
}

// 0x40faf0: the base dialog, the skull on top, then the best of the day and the five best, each line centred
void HighScoreDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->DrawImage(mApp->mImages[IMAGE_DIALOGSKULL], mWidth / 2 - 0x14, 0);
	g->SetFont(mApp->mFonts[FONT_MT11]);
	g->SetColor(Color(0xff, 0xff, 0));
	DrawCentered(g, mWidth, GT(0x004a35b8), 0x60);
	g->FillRect(mWidth / 2 - 0x28, 99, 0x50, 1);
	g->SetColor(Color(0xff, 0, 0));
	char aBuf[128];
	if (mScores->mTodayScore < 1)
		DrawCentered(g, mWidth, GT(0x004a35a4), 0x79);
	else
	{
		sprintf(aBuf, GT(0x004a35b0).c_str(), mScores->mTodayName.c_str(), mScores->mTodayScore);
		DrawCentered(g, mWidth, aBuf, 0x79);
	}
	g->SetColor(Color(0xff, 0xff, 0));
	DrawCentered(g, mWidth, GT(0x004a3598), 0x9b);
	g->FillRect(mWidth / 2 - 0x28, 0x9e, 0x50, 1);
	g->SetColor(Color(0xff, 0, 0));
	int aY = 0xb4;
	for (int i = 0; i < 5; i++)
	{
		sprintf(aBuf, GT(0x004a35b0).c_str(), mScores->mNames[i].c_str(), mScores->mScores[i]);
		DrawCentered(g, mWidth, aBuf, aY);
		aY += 0x11;
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
	Use2003Buttons(this, theApp->mImages[IMAGE_OPTIONSBUTTON]);
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

// 0x40cb90 (the bonus screen's: the check box at 0x5f, 0x140) and 0x4266a0 (the warnings': 0x41, 200)
void NagDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	if (mWhich < 0)
		mCheckbox->Resize(mX + 0x5f, mY + 0x140, 0x14, 0x14);
	else
		mCheckbox->Resize(mX + 0x41, mY + 200, 0x14, 0x14);
}

int NagDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return (mWhich < 0) ? 0x1ae : 0x136;   // 0x40cb20 (the bonus screen's) and 0x426630 (the warnings')
}

// 0x40d2b0: the mouse position goes to the side panel too
void NagDialog::MouseMove(int x, int y)
{
	Dialog2004::MouseMove(x, y);
	mApp->FollowMouse(mX + x, mY + y);
}

// 0x40cbe0 / 0x4266f0: the framed box, the title in white (the warnings: BEWARE!) and the text in gold, every line
// centred at its own height
void NagDialog::Draw(Graphics* g)
{
	DrawFramed(g, mApp, mWidth, mHeight);
	g->SetFont(mApp->mFonts[FONT_MT22]);
	g->SetColor(Color(0xff, 0xff, 0xff));
	if (mWhich < 0)
	{
		DrawCentered(g, mWidth, GT(0x004a3284), 0x69);
		DrawCentered(g, mWidth, GT(0x004a3270), 0x87);
	}
	else
		DrawCentered(g, mWidth, GT(0x004a6ac4), 0x70);
	g->SetFont(mApp->mFonts[FONT_TUT]);
	g->SetColor(Color(0xff, 0xc2, 0));
	static const unsigned int kBonusLines[7] = {0x004a323c, 0x004a3208, 0x004a31d4, 0x004a31a0, 0x004a316c, 0x004a3138, 0x004a3104};
	static const int kBonusY[7] = {0xaf, 0xc3, 0xd7, 0xf5, 0x109, 0x11d, 0x131};
	static const unsigned int kWarnLines[4][3] =
	{
		{0x004a6a34, 0x004a6a0c, 0x004a6a00},
		{0x004a6a9c, 0x004a6a74, 0x004a6a5c},
		{0x004a69d8, 0x004a69b0, 0x004a6994},
		{0x004a696c, 0x004a6940, 0x004a6918}
	};
	if (mWhich < 0)
	{
		for (int i = 0; i < 7; i++)
			DrawCentered(g, mWidth, GT(kBonusLines[i]), kBonusY[i]);
	}
	else if (mWhich < 4)
	{
		static const int kWarnY[3] = {0x91, 0xa5, 0xb9};
		for (int i = 0; i < 3; i++)
			DrawCentered(g, mWidth, GT(kWarnLines[mWhich][i]), kWarnY[i]);
	}
	g->SetColor(Color(0xff, 0xff, 0));
	g->DrawString(GT(0x004a30e4), (mCheckbox->mX - mX) + 0x1c, (mCheckbox->mY - mY) + 0xe);
}
