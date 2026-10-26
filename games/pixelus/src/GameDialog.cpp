#include "GameDialog.h"
#include "PixelusApp.h"
#include "Board.h"
#include "Res.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "Font.h"
#include "ImageFont.h"
#include "Graphics.h"
#include "Image.h"
#include "MusicInterface.h"
#include "Insets.h"

using namespace Sexy;

// the picture of a sheet of three frames as an image button (0x4044e0 and 0x40a7d0 with 0x431670)
DialogButton* Sexy::MakeStripButton(PixelusApp* theApp, ButtonListener* theListener, int theId, int theImage, int theX, int theY)
{
	Image* anImage = theApp->mImages[theImage];
	DialogButton* aButton = new DialogButton(anImage, theId, theListener);
	int aWidth = anImage->GetWidth() / 3;
	int aHeight = anImage->GetHeight();
	aButton->mNormalRect = Rect(0, 0, aWidth, aHeight);
	aButton->mOverRect = Rect(aWidth, 0, aWidth, aHeight);
	aButton->mDownRect = Rect(aWidth * 2, 0, aWidth, aHeight);
	aButton->mTranslateX = 0;
	aButton->mTranslateY = 0;
	aButton->mHasAlpha = true;
	aButton->mHasTransparencies = true;
	aButton->Resize(theX, theY, aWidth, aHeight);
	return aButton;
}

// 0x41c260
GameDialog::GameDialog(PixelusApp* theApp, Image* theComponentImage, Image* theButtonComponentImage, int theId,
	bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines,
	const SexyString& theDialogFooter, int theButtonMode, Image* theDecoration) :
	Dialog2004(theComponentImage, theButtonComponentImage, theId, isModal, theDialogHeader, theDialogLines,
		theDialogFooter, theButtonMode)
{
	mDecoration = theDecoration;
	mApp = theApp;

	int aTop = 0x1c;
	if (theDecoration != NULL)
	{
		// a dialog with a decoration starts 80 pixels lower, so the decoration sticks out above its frame
		mBackgroundInsets = Insets(0, 0x50, 0, 0);
		aTop = 0x34;
	}
	mContentInsets = Insets(0x24, aTop, 0x24, 0x24);

	if (mYesButton != NULL)
		StyleButton(theApp, mYesButton, 3);
	if (mNoButton != NULL)
		StyleButton(theApp, mNoButton, 3);
}

// 0x41c380
int GameDialog::GetContentLeft()
{
	return mContentInsets.mLeft + mBackgroundInsets.mLeft + mX;
}

// 0x41c3a0
int GameDialog::GetContentTop()
{
	return mContentInsets.mTop + mBackgroundInsets.mTop + 0x36 + mY;
}

// 0x41c3c0
int GameDialog::GetContentWidth()
{
	return mWidth - mContentInsets.mRight - mContentInsets.mLeft - mBackgroundInsets.mRight - mBackgroundInsets.mLeft;
}

// 0x41c3e0: a press in the strip above the frame only counts over the decoration
void GameDialog::MouseDown(int x, int y, int theClickCount)
{
	if (mDecoration != NULL)
	{
		int aWidth = mDecoration->GetWidth();
		int aLeft = (mWidth - aWidth) / 2;
		if (mBackgroundInsets.mTop <= y || (aLeft <= x && x <= aLeft + aWidth))
			Dialog2004::MouseDown(x, y, theClickCount);
		return;
	}
	Dialog2004::MouseDown(x, y, theClickCount);
}

// 0x41c440: the listener's ButtonPress(id); the dialog's own ids (YES = 2000 + id, NO = 3000 + id) click
void GameDialog::ButtonPress(int theId)
{
	if (theId >= 2000)
		mApp->PlaySample(SOUND_UI_LITTLE_BEEP_05B);
}

// 0x41c470
void GameDialog::Move(int theNewX, int theNewY)
{
	Dialog2004::Move(theNewX, theNewY);
	if (mApp->mBoard != NULL)
		mApp->mBoard->MarkDirty();
}

// 0x41c4d0
void GameDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);

	if (!mDialogHeader.empty())
	{
		// the two ornaments left and right of the header, at least 2 and at most 12 pixels away from it
		int aHeaderWidth = mHeaderFont->StringWidth(mDialogHeader);
		Image* aLeft = mApp->mImages[IMAGE_DECOLEFT];
		int aOrnamentWidth = aLeft->GetWidth();
		int aLeftX = (mWidth - aHeaderWidth) / 2 - aOrnamentWidth;
		if (aLeftX > mContentInsets.mLeft)
		{
			int aGap = aLeftX - mContentInsets.mLeft;
			if (aGap >= 12)
				aGap = 12;
			else if (aGap < 2)
				aGap = 2;

			int aY = mBackgroundInsets.mTop + mContentInsets.mTop - mHeaderFont->GetAscentPadding() +
				mHeaderFont->GetAscent() + (3 - aLeft->GetHeight());
			g->DrawImage(aLeft, aLeftX - aGap, aY);
			g->DrawImage(mApp->mImages[IMAGE_DECORIGHT], (mWidth + aHeaderWidth) / 2 + aGap, aY);
		}
	}

	if (mDecoration != NULL)
		g->DrawImage(mDecoration, (mWidth - mDecoration->GetWidth()) / 2, 0);
}

// 0x431380
void Sexy::StyleButton(PixelusApp* theApp, DialogButton* theButton, int theFrames)
{
	if (theButton == NULL)
		return;

	Image* anImage = theButton->mComponentImage;
	int aWidth = anImage->GetWidth() / theFrames;
	int aHeight = anImage->GetHeight();

	theButton->mTranslateX = 0;
	theButton->mTranslateY = 2;
	theButton->mTextOffsetX = 0;
	theButton->mTextOffsetY = -3;
	theButton->mNormalRect = Rect(0, 0, aWidth, aHeight);
	theButton->mOverRect = Rect(aWidth, 0, aWidth, aHeight);
	theButton->mDownRect = Rect(aWidth * 2, 0, aWidth, aHeight);

	theButton->SetFont(theApp->mFonts[FONT_VESPASIANO15]);
	theButton->SetColor(0, Color(255, 255, 255, 255));
	theButton->SetColor(1, Color(0xff, 0xc0, 0x40));
	theButton->mHasAlpha = true;
	theButton->mHasTransparencies = true;

	// an unsized button gets its label's width
	if (theButton->mWidth == 0)
		theButton->Resize(theButton->mX, theButton->mY, theButton->mFont->StringWidth(theButton->mLabel), aHeight);
}

// 0x4314e0
void Sexy::SizeDialog(PixelusApp* theApp, Dialog* theDialog, int theMinWidth)
{
	theDialog->SetHeaderFont(theApp->mFonts[FONT_GARAMOND20]);
	theDialog->SetLinesFont(theApp->mFonts[FONT_GARAMOND14]);
	theDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255, 255));
	theDialog->SetColor(Dialog::COLOR_LINES, Color(255, 255, 255, 255));

	int aWidth = theApp->mFonts[FONT_GARAMOND20]->StringWidth(theDialog->mDialogHeader) +
		theDialog->mContentInsets.mRight + theDialog->mContentInsets.mLeft +
		theDialog->mBackgroundInsets.mRight + theDialog->mBackgroundInsets.mLeft + 0x14;
	if (theMinWidth > aWidth)
		aWidth = theMinWidth;

	// a dialog opened over another one cascades 20 pixels below it
	Dialog* anOther = NULL;
	if (!theApp->mDialogMap.empty())
		anOther = theApp->mDialogMap.begin()->second;
	int aY = 0x5a - theDialog->mBackgroundInsets.mTop;
	if (anOther != NULL)
		aY += anOther->mY + 0x14;

	int aHeight = theDialog->GetPreferredHeight(aWidth);
	if (aY + aHeight > 0x1d6)
		aY = 10;
	theDialog->Resize((PixelusApp::GAME_WIDTH - aWidth) / 2, aY, aWidth, aHeight);

	StyleButton(theApp, theDialog->mYesButton, 3);
	StyleButton(theApp, theDialog->mNoButton, 3);
}

// 0x431160
void Sexy::SetDialogColors(Widget* theWidget)
{
	theWidget->SetColor(0, Color(0x294918));
	theWidget->SetColor(1, Color(0));
	theWidget->SetColor(2, Color(0xcee321));
	theWidget->SetColor(3, Color(0x9b, 0xb0, 0x8e));
	theWidget->SetColor(4, Color(255, 255, 255, 255));
}

// 0x431230
void Sexy::FadeOutAllMusic(PixelusApp* theApp)
{
	theApp->mMusicInterface->FadeOutAll(true, 0.008);
}

// 0x431250: switch to another song; a song that is already playing is faded in again from where it is
void Sexy::StartMusic(PixelusApp* theApp, int theSongId, bool theSlow)
{
	bool aPlaying = theApp->mMusicInterface->IsPlaying(theSongId);
	theApp->mMusicInterface->FadeOutAll(true, 0.008);
	theApp->mMusicInterface->FadeIn(theSongId, aPlaying ? -1 : 0, theSlow ? 0.002 : 1.0, false);
}

// 0x4311f0
void Sexy::FadeOutMusicIfPlaying(PixelusApp* theApp, int theSongId)
{
	if (theApp->mMusicInterface->IsPlaying(theSongId))
		theApp->mMusicInterface->FadeOut(theSongId, true, 0.008);
}
