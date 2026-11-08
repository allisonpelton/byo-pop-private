#include "TitleScreen.h"
#include "WinWSApp.h"
#include "GameText.h"
#include "ButtonWidget.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "Image.h"
#include "DDImage.h"
#include "ImageFont.h"
#include "Res.h"

using namespace Sexy;

// 0x448e10: Widget first. The property NoReg (read as a boolean, default false) is true in the retail partner.xml, so the
// original makes no register button (+0x6c stays 0) and the port leaves it out; only the play button (id 0x191) is made.
// The images and the font are loaded by AddControls (the original leaves their fields unset until then)
TitleScreen::TitleScreen(WinWSApp* theApp)
{
	mApp = theApp;
	mTitleImage = NULL;	// port: the original leaves +0x58..+0x70 unset until AddControls
	mBarAndLogo = NULL;
	mGoldBar = NULL;
	mFont = NULL;
	mPlayButton = new ButtonWidget(PLAY_BUTTON_ID, mApp);
}

// 0x4414c0 (deleting destructor 0x441ff0, slot 0): the play button's two images go before the button, then the three
// pictures and the font. The original reads the button's images without testing the button (the constructor always makes
// it); the register button's turn (images, button) is not ported. The two stamp images are never freed by the original
// (they are not loaded in the port)
TitleScreen::~TitleScreen()
{
	if (mPlayButton != NULL && mPlayButton->mButtonImage != NULL)	// port: tested
		delete mPlayButton->mButtonImage;
	if (mPlayButton != NULL && mPlayButton->mOverImage != NULL)
		delete mPlayButton->mOverImage;
	if (mPlayButton != NULL)
		delete mPlayButton;
	if (mTitleImage != NULL)
		delete mTitleImage;
	if (mBarAndLogo != NULL)
		delete mBarAndLogo;
	if (mGoldBar != NULL)
		delete mGoldBar;
	if (mFont != NULL)
		delete mFont;
}

// 0x448f40: the 12-point font first (made with its description file, not yet prepared), then the three pictures of the
// screen, the register button's two images (not ported), the play button's two images (normal and over) and the two stamps
// (not ported). When the title picture, the bar and logo, the gold bar and the font are all there the font is prepared;
// otherwise the app is told to quit (Popup with the text, then mLoadingFailed). A missing play button image is not
// noticed here: the button would draw nothing
void TitleScreen::AddControls()
{
	mFont = new ImageFont(mApp, "data\\12ptfont.txt");
	mTitleImage = mApp->GetImage("images\\titlescreen");
	mBarAndLogo = mApp->GetImage("images\\barandlogo");
	mGoldBar = mApp->GetImage("images\\goldbar");
	// 0x448f40 not ported: images\title_register and images\title_register_over for the register button (+0x6c)
	mPlayButton->mButtonImage = mApp->GetImage("images\\title_continue");
	mPlayButton->mOverImage = mApp->GetImage("images\\title_continue_over");
	// 0x448f40 not ported: images\stamp_registered (+0x64) and images\stamp_trial (+0x68)

	if (mTitleImage != NULL && mBarAndLogo != NULL && mGoldBar != NULL && mFont != NULL)
	{
		mFont->Prepare();	// slot 22
		return;
	}

	mApp->Popup(GT(0x0052fb90));	// slot 21
	mApp->mLoadingFailed = true;	// +0x528
}

// 0x449400 slot 1: the register button (not ported; hidden, in the font, "REGISTER TYPERSHARK!", at (0xcd, 0x15a, 0xe1,
// 0x1c)) would be set up here for an unregistered game with that button. The play button starts hidden and disabled; its
// font is a copy of the screen's, and its label text is only measured (the original calls the font's StringWidth with the
// text and drops the result; the pictures carry the words). The button is placed at (200, 0x1b0), 0xeb by 0x1c
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	// 0x449400 not ported: the register button, for a game that is not registered and has one

	mPlayButton->SetVisible(false);
	mPlayButton->SetFont(mFont);
	mPlayButton->mHasAlpha = true;
	(void) mPlayButton->mFont->StringWidth(GT(0x0052fc7c));	// the result is dropped; the label itself (mLabel) stays empty
	mPlayButton->Resize(200, 0x1b0, 0xeb, 0x1c);
	theWidgetManager->AddWidget(mPlayButton);
	mPlayButton->mDoFinger = true;
	mPlayButton->SetDisabled(true);
}

// 0x4415d0 slot 2: the register button (not ported) and then the play button leave the manager
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayButton);
}

// 0x4495c0 slot 16: the title picture, the bar and logo at (0x7e, 0x162), and the gold bar cut at the width the loading
// progress gives (the progress, 0 to 1, times the bar's width, truncated; nothing when that is 0) at (0x95, 0x171).
// Then the original draws the registered or trial stamp at (0x13, 0x7e) and, for a trial, the minutes left of MaxTime, but
// only when the property NoReg is false; the retail partner.xml makes it true, so that part is not ported
void TitleScreen::Draw(Graphics* g)
{
	g->DrawImage(mTitleImage, 0, 0);
	g->DrawImage(mBarAndLogo, 0x7e, 0x162);

	Image* aGoldBar = mGoldBar;
	int aBarWidth = (int) (mApp->GetLoadingThreadProgress() * aGoldBar->mWidth);	// slot 51, then _ftol
	if (aBarWidth > 0)
	{
		Rect aSrcRect(0, 0, aBarWidth, aGoldBar->mHeight);
		g->DrawImage(aGoldBar, 0x95, 0x171, aSrcRect);
	}

	// 0x4495c0 not ported: the stamp (GetBoolean("NoReg") is true in the retail game) and the "%d minute(s) remaining" text
}
