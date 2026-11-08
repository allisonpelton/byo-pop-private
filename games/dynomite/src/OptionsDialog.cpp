#include "OptionsDialog.h"
#include "Checkbox.h"
#include "DialogButton.h"
#include "FossilizedApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Sprite.h"
#include "WidgetManager.h"
#include <cstring>
#include <strings.h>

using namespace Sexy;

// 0x42ee20
OptionsDialog::OptionsDialog(FossilizedApp* theApp) :
	Dialog2004(theApp->mPicDialog->mImage, theApp->mPicDbutton->mImage, 2, true, theApp->GetString("OPTIONS"), "",
		theApp->GetString("DONE"), Dialog::BUTTONS_FOOTER)
{
	mApp = theApp;
	SetButtonFont(theApp->mWhiteFontSmall);
	SetHeaderFont(mApp->mWhiteFont);
	SetLinesFont(mApp->mWhiteFontSmall);

	mHighScoresButton = new DialogButton(theApp->mPicDbutton->mImage, 2, this);
	mHighScoresButton->SetFont(mApp->mWhiteFontSmall);
	mHighScoresButton->mLabel = mApp->GetString("HIGH_SCORES");

	mMusicSlider = new Slider2004(theApp->mPicSlider->mImage, theApp->mPicThumb->mImage, 0, this);
	mMusicSlider->SetValue(theApp->GetMusicVolume());
	mSfxSlider = new Slider2004(theApp->mPicSlider->mImage, theApp->mPicThumb->mImage, 1, this);
	mSfxSlider->SetValue(theApp->GetSfxVolume());
	mFullscreenCheckbox = new Checkbox(mApp->mPicCheckboxunchecked->mImage, mApp->mPicCheckboxchecked->mImage, 0, this);
	mFullscreenCheckbox->SetChecked(!theApp->mIsWindowed);
	mCustomCursorsCheckbox = new Checkbox(mApp->mPicCheckboxunchecked->mImage, mApp->mPicCheckboxchecked->mImage, 0,
		this);
	mCustomCursorsCheckbox->SetChecked(theApp->mCustomCursorsEnabled);

	mNextLinkId = 100;
	OptionsLink* aLink = new OptionsLink();
	aLink->mText = GT(0x4fc60c);
	aLink->mURL = "GOTO:CREDITS";
	aLink->mButton = new DialogButton(theApp->mPicDbutton->mImage, mNextLinkId++, this);
	aLink->mButton->SetFont(mApp->mWhiteFontSmall);
	aLink->mButton->mLabel = aLink->mText;
	mLinks.Add(aLink);
	// port: the original adds "check for updates" (GOTO:UPDATES) unless the property DontUpdate is set, then the
	// property NumOptionsLinks's partner links (OptionsLinkText%d and OptionsLinkURL%d, "BLANK" without one), neither
	// of which the port has
}

// 0x42eca0
OptionsDialog::~OptionsDialog()
{
	if (mHighScoresButton != NULL)
		delete mHighScoresButton;
	if (mMusicSlider != NULL)
		delete mMusicSlider;
	if (mSfxSlider != NULL)
		delete mSfxSlider;
	if (mFullscreenCheckbox != NULL)
		delete mFullscreenCheckbox;
	if (mCustomCursorsCheckbox != NULL)
		delete mCustomCursorsCheckbox;
	mLinks.ResetIter();
	OptionsLink* aLink = mLinks.Next();
	while (aLink != NULL)
	{
		mApp->SafeDeleteWidget(aLink->mButton);
		delete aLink;
		aLink = mLinks.Next();
	}
	mLinks.Clear();
}

// 0x42e5a0
int OptionsDialog::GetPreferredHeight(int theWidth)
{
	return mLinks.mCount * 0x21 + 0x111;
}

// 0x42e5c0
void OptionsDialog::SetColor(int theIdx, const Color& theColor)
{
	Dialog::SetColor(theIdx, theColor);
	if (theIdx == 3)
	{
		if (mHighScoresButton != NULL)
		{
			mHighScoresButton->SetColor(0, theColor);
			mHighScoresButton->SetColor(1, Color(0xff, 0xff, 0xff));
		}
	}
}

// 0x42e670
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mHighScoresButton);
	theWidgetManager->AddWidget(mMusicSlider);
	theWidgetManager->AddWidget(mSfxSlider);
	theWidgetManager->AddWidget(mCustomCursorsCheckbox);
	theWidgetManager->AddWidget(mFullscreenCheckbox);
	mLinks.ResetIter();
	OptionsLink* aLink = mLinks.Next();
	while (aLink != NULL)
	{
		theWidgetManager->AddWidget(aLink->mButton);
		aLink = mLinks.Next();
	}
}

// 0x42e710
void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mHighScoresButton);
	theWidgetManager->RemoveWidget(mMusicSlider);
	theWidgetManager->RemoveWidget(mSfxSlider);
	theWidgetManager->RemoveWidget(mFullscreenCheckbox);
	theWidgetManager->RemoveWidget(mCustomCursorsCheckbox);
	mLinks.ResetIter();
	OptionsLink* aLink = mLinks.Next();
	while (aLink != NULL)
	{
		theWidgetManager->RemoveWidget(aLink->mButton);
		aLink = mLinks.Next();
	}
}

// 0x42e7b0
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mWidth - mContentInsets.mRight - mContentInsets.mLeft;
	// the high scores button across the dialog, as the registered copy has it (an unregistered one's register
	// button took its left half)
	mHighScoresButton->Resize(mContentInsets.mLeft + mX, mY + 0xaa, aWidth, 0x20);
	int aY = mY + 0xcb;
	mLinks.ResetIter();
	OptionsLink* aLink = mLinks.Next();
	while (aLink != NULL)
	{
		aLink->mButton->Resize(mContentInsets.mLeft + mX, aY, aWidth, 0x20);
		aY = aY + 0x21;
		aLink = mLinks.Next();
	}
	mMusicSlider->Resize(mX + 0x98, mY + 0x44, mWidth - 0xc0, 0x20);
	mSfxSlider->Resize(mX + 0x98, mY + 100, mWidth - 0xc0, 0x20);
	mFullscreenCheckbox->Resize(mX + 0x30, mY + 0x8a, 0x14, 0x14);
	mCustomCursorsCheckbox->Resize(mX + 0xb0, mY + 0x8a, 0x14, 0x14);
}

// 0x42fb70
void OptionsDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	g->SetFont(mApp->mWhiteFontSmall);
	g->SetColorizeImages(true);
	g->SetColor(mColors[1]);
	g->DrawString(mApp->GetString("MUSIC"), 0x30, 0x58);
	g->DrawString(mApp->GetString("SOUND_FX"), 0x30, 0x78);
	g->DrawString(mApp->GetString("FULLSCREEN"), (mFullscreenCheckbox->mX - mX) + 0x1c,
		(mFullscreenCheckbox->mY - mY) + 0xe);
	g->DrawString(mApp->GetString("CUSTOM_CURSORS"), (mCustomCursorsCheckbox->mX - mX) + 0x1c,
		(mCustomCursorsCheckbox->mY - mY) + 0xe);
	g->SetColorizeImages(false);
}

// 0x42e970
void OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == 0)
		mApp->SetMusicVolume(theVal);
	else if (theId == 1)
		mApp->SetSfxVolume(theVal);
}

// 0x42e9b0
void OptionsDialog::CheckboxChecked(int theId, bool checked)
{
	if ((checked == false) && (mApp->mForceFullscreen != false))
	{
		mApp->ShowNoWindowed();
		mFullscreenCheckbox->SetChecked(true);
	}
}

// 0x42e9e0
void OptionsDialog::ButtonPress(int theId)
{
	Dialog2004::ButtonPress(theId);
	mApp->mSndClick->Play();
}

// 0x42eb70
void OptionsDialog::ButtonDepress(int theId)
{
	char aURL[0x100];

	Dialog2004::ButtonDepress(theId);
	if (theId == 2)
	{
		mApp->ShowHighScores();
		return;
	}
	if (99 < theId)
	{
		mLinks.ResetIter();
		OptionsLink* aLink = mLinks.Next();
		while (aLink != NULL)
		{
			if (aLink->mButton->mId == theId)
			{
				strncpy(aURL, aLink->mURL.c_str(), 0xff);
				aURL[0xff] = '\0';	// port: strncpy leaves a long address unterminated
				if (strncasecmp(aURL, "GOTO:", 5) == 0)
				{
					// port: "GOTO:UPDATES" looks for an update online in the original (0x41f4c0); the port has no such
					// link
					if (strcasecmp(aURL + 5, "CREDITS") == 0)
						mApp->ShowCredits();
				}
				else
				{
					mApp->OpenURL(aLink->mURL, false);
				}
			}
			aLink = mLinks.Next();
		}
	}
}
