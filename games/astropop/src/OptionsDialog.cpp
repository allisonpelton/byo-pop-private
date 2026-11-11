// OptionsDialog: the options box (see OptionsDialog.h).

#include "OptionsDialog.h"
#include "AstroPopApp.h"
#include "Board.h"
#include "Checkbox.h"
#include "DialogButton.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Res.h"
#include "Slider.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// 0x482510: a tick box of the box's own pictures: the two halves of the checkbox strip, each in an image of 24 x 25
Checkbox* MakeCheckbox(int theId, CheckboxListener* theListener)
{
	MemoryImage* aCheckedImage = new MemoryImage(gApp);
	aCheckedImage->Create(0x18, 0x19);
	aCheckedImage->SetImageMode(true, true);
	{
		Graphics g(aCheckedImage);
		g.DrawImage(IMAGE_CHECKBOX, -0x18, 0);
	}

	MemoryImage* anUncheckedImage = new MemoryImage(gApp);
	anUncheckedImage->Create(0x18, 0x19);
	anUncheckedImage->SetImageMode(true, true);
	{
		Graphics g(anUncheckedImage);
		g.DrawImage(IMAGE_CHECKBOX, 0, 0);
	}

	Checkbox* aCheckbox = new Checkbox(anUncheckedImage, aCheckedImage, theId, theListener);
	aCheckbox->Resize(0, 0, aCheckedImage->mWidth, aCheckedImage->mHeight);
	return aCheckbox;
}

void SetButtonRects(DialogButton* theButton)
{
	theButton->mNormalRect = Rect(0, 0, 0x78, 0x21);
	theButton->mOverRect = Rect(0x78, 0, 0x78, 0x21);
	theButton->mDownRect = Rect(0xf0, 0, 0x78, 0x21);
}

}

// A button of the box's picture with its three pictures, its label and the font of the buttons
// port: a helper of the options box
DialogButton* OptionsDialog::MakeButton(int theId, const char* theLabel)
{
	DialogButton* aButton = new DialogButton(mYesButton->mComponentImage, theId, this);
	SetButtonRects(aButton);
	aButton->mLabel = theLabel;
	aButton->SetFont(GetFontById(0xb1));
	return aButton;
}

// 0x4850a0: the box is of the colour set it is given (the game's mode + 2, or 1 with no game), and its boxes show what is
// set now. The footer button is the box's own CLOSE; in a game it is "Resume Game".
OptionsDialog::OptionsDialog(bool theHints, int theColourSet, bool theFromGame) :
	Text2004<Dialog>(IMAGE_DIALOG, IMAGE_DIALOG_BTN, 0x66, true, "OPTIONS", std::string(), "CLOSE", 3)
{
	mColourSet = theColourSet;

	Color anInsetColor;
	Color aButtonColor;
	if (theColourSet >= 0 && theColourSet <= 5)
	{
		anInsetColor = gApp->GetDialogInsetColor(theColourSet);
		aButtonColor = gApp->GetDialogColor(theColourSet);
	}

	MemoryImage* anInsetImage = NULL;
	MemoryImage* aButtonImage = NULL;
	gApp->MakeDialogImages(anInsetColor, aButtonColor, &anInsetImage, &aButtonImage, false);
	mComponentImage = anInsetImage;
	// the 2004 Dialog reads the button height from the button picture at each use (GetPreferredHeight 0x4e91d0 and Resize); the
	// port keeps the height it took at construction, so it follows the picture that replaces it
	mYesButton->mComponentImage = aButtonImage;
	mButtonHeight = aButtonImage->mHeight;

	// the 2004 slider: a press on the track outside the thumb does nothing, and SetValue does not clamp (docs/FRAMEWORK_2004.md, section 8)
	mMusicSlider = new Slider2004(IMAGE_SLIDER_TRACK, IMAGE_SLIDER_THUMB, 0, this);
	mMusicSlider->SetValue(gApp->GetMusicVolume());
	mSoundSlider = new Slider2004(IMAGE_SLIDER_TRACK, IMAGE_SLIDER_THUMB, 1, this);
	mSoundSlider->SetValue(gApp->GetSfxVolume());

	mFullscreenCheck = MakeCheckbox(2, this);
	mCursorCheck = MakeCheckbox(3, this);
	mAccelCheck = MakeCheckbox(4, this);
	mHintsCheck = MakeCheckbox(5, this);
	mColorblindCheck = MakeCheckbox(6, this);
	mFullscreenCheck->mChecked = !gApp->mIsWindowed;
	mCursorCheck->mChecked = gApp->mCustomCursorsEnabled;
	mAccelCheck->mChecked = gApp->Is3DAccelerated();
	mHintsCheck->mChecked = theHints;
	mColorblindCheck->mChecked = gApp->mColorblind;

	mHintsResetButton = MakeButton(9, GTC(0x0056b08c));
	mCreditsButton = MakeButton(0xb, "Credits");

	mLeaveButton = new DialogButton(aButtonImage, 0xfa8, this);
	SetButtonRects(mLeaveButton);
	mLeaveButton->mLabel = GT(0x0056b0b4);
	if (gApp->mBoard != NULL && gApp->mBoard->mGameType == 0)
		mLeaveButton->mLabel = GT(0x0056b0a4);
	mLeaveButton->SetColor(ButtonWidget::COLOR_LABEL, Color(0xff, 0xff, 0));
	mLeaveButton->SetColor(ButtonWidget::COLOR_LABEL_HILITE, Color(0xff, 0xff, 0xff));
	mLeaveButton->SetFont(GetFontById(0xb1));

	mHelpButton = MakeButton(10, "Help");

	if (theFromGame)
		mYesButton->mLabel = GT(0x0056b078);
	SetButtonRects(mYesButton);
	if (mNoButton != NULL)
	{
		SetButtonRects(mNoButton);
		mNoButton->mComponentImage = aButtonImage;
	}
	mFromGame = theFromGame;
}

// 0x483e20 (body 0x4822f0): the box's pictures, the sliders, the tick boxes (with their pictures) and the buttons
OptionsDialog::~OptionsDialog()
{
	if (mYesButton != NULL)
		delete mYesButton->mComponentImage;
	delete mComponentImage;
	mComponentImage = NULL;
	delete mMusicSlider;
	delete mSoundSlider;
	Checkbox* aChecks[5] = { mFullscreenCheck, mCursorCheck, mAccelCheck, mHintsCheck, mColorblindCheck };
	for (int i = 0; i < 5; i++)
	{
		delete aChecks[i]->mCheckedImage;
		delete aChecks[i]->mUncheckedImage;
		delete aChecks[i];
	}
	delete mLeaveButton;
	delete mHintsResetButton;
	delete mHelpButton;
	delete mCreditsButton;
}

// 0x482ac0: the box opens with a sound
void OptionsDialog::AddedToManager(WidgetManager* theManager)
{
	Dialog::AddedToManager(theManager);
	theManager->AddWidget(mMusicSlider);
	theManager->AddWidget(mSoundSlider);
	theManager->AddWidget(mFullscreenCheck);
	theManager->AddWidget(mCursorCheck);
	theManager->AddWidget(mAccelCheck);
	theManager->AddWidget(mHintsCheck);
	theManager->AddWidget(mColorblindCheck);
	theManager->AddWidget(mHintsResetButton);
	theManager->AddWidget(mHelpButton);
	theManager->AddWidget(mCreditsButton);
	if (mFromGame)
		theManager->AddWidget(mLeaveButton);
	gApp->PlaySample(SOUND_GOODBYE);
}

// 0x482bd0
void OptionsDialog::RemovedFromManager(WidgetManager* theManager)
{
	Dialog::RemovedFromManager(theManager);
	theManager->RemoveWidget(mMusicSlider);
	theManager->RemoveWidget(mSoundSlider);
	theManager->RemoveWidget(mFullscreenCheck);
	theManager->RemoveWidget(mCursorCheck);
	theManager->RemoveWidget(mAccelCheck);
	theManager->RemoveWidget(mHintsCheck);
	theManager->RemoveWidget(mColorblindCheck);
	theManager->RemoveWidget(mHintsResetButton);
	theManager->RemoveWidget(mHelpButton);
	theManager->RemoveWidget(mCreditsButton);
	if (mFromGame)
		theManager->RemoveWidget(mLeaveButton);
}

// 0x4826d0 (with its three small sums 0x482670, 0x482690 and 0x4826b0, written inline): the sliders at the top, the boxes in two columns under them, then the buttons
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);

	int aLeft = mContentInsets.mLeft + mBackgroundInsets.mLeft + mX;
	int aWidth = (((mWidth - mContentInsets.mRight) - mContentInsets.mLeft) - mBackgroundInsets.mRight) - mBackgroundInsets.mLeft;
	int aTop = mContentInsets.mTop + mBackgroundInsets.mTop + 0x28 + mY;
	mMusicSlider->Resize(aLeft + 0x74, aTop, aWidth - 0x74, 0x24);
	mSoundSlider->Layout(LAY_SameSize | LAY_SameLeft | LAY_Below, mMusicSlider, 0, 0, 0, 0);
	mFullscreenCheck->Layout(LAY_SetLeft | LAY_Below, mSoundSlider, aLeft, 7, 0, 0);
	mCursorCheck->Layout(LAY_Right | LAY_SameTop, mFullscreenCheck, 0xb4, 0, 0, 0);
	mAccelCheck->Layout(LAY_SameLeft | LAY_Below, mFullscreenCheck, 0, 0, 0, 0);
	mHintsCheck->Layout(LAY_Right | LAY_SameTop, mAccelCheck, 0xb4, 0, 0, 0);
	mColorblindCheck->Layout(LAY_SameLeft | LAY_Below, mAccelCheck, 0, 0, 0, 0);

	// the buttons (the original's layout when it is registered and has no update button)
	mHintsResetButton->Layout(LAY_SameSize, mYesButton, 0, 0, 0, 0);
	mHintsResetButton->Layout(LAY_SameLeft | LAY_Below, mColorblindCheck, 0, 0x14, 0, 0);
	mHintsResetButton->Resize(mHintsResetButton->mX, mHintsResetButton->mY, mHintsResetButton->mWidth / 2,
							  mHintsResetButton->mHeight);
	mHelpButton->Layout(LAY_SameSize, mHintsResetButton, 0, 0, 0, 0);
	mHelpButton->Layout(LAY_Right | LAY_SameTop, mHintsResetButton, 0, 0, 0, 0);

	if (mFromGame)
	{
		mYesButton->Resize(mYesButton->mX, mYesButton->mY, mYesButton->mWidth / 2, mYesButton->mHeight);
		mLeaveButton->Layout(LAY_SameSize, mYesButton, 0, 0, 0, 0);
		mLeaveButton->Layout(LAY_SameBottom | LAY_Right, mYesButton, 0, 0, 0, 0);
		mYesButton->Layout(LAY_SameLeft, mLeaveButton, 0, 0, 0, 0);
		mLeaveButton->Layout(LAY_Left, mYesButton, 0, 0, 0, 0);
	}

	mCreditsButton->Layout(LAY_SameSize, mYesButton, 0, 0, 0, 0);
	if (mFromGame)
		mCreditsButton->Resize(mCreditsButton->mX, mCreditsButton->mY, mCreditsButton->mWidth * 2, mCreditsButton->mHeight);
	mCreditsButton->Layout(LAY_SameLeft | LAY_Below, mHintsResetButton, 0, 0, 0, 0);
}

// 0x482a80
int OptionsDialog::GetPreferredHeight(int theWidth)
{
	int aHeight = Dialog::GetPreferredHeight(theWidth);
	aHeight += 200;
	return aHeight + IMAGE_DIALOG_BTN->mHeight * 2;
}

// 0x482cc0: a text to the right of a tick box
void OptionsDialog::DrawCheckLabel(Graphics* g, const std::string& theText, Checkbox* theCheck)
{
	Font* aFont = g->GetFont();
	int anAscent = aFont->GetAscent(); // the original calls the font's slot 1 (+4), the ascent, not its height (slot 4)
	g->DrawString(theText, (theCheck->mWidth + theCheck->mX) - mX, anAscent + 10 + (theCheck->mY - mY));
}

// 0x485d40
void OptionsDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);
	Rect aBox(mBackgroundInsets.mLeft, mBackgroundInsets.mTop, (mWidth - mBackgroundInsets.mRight) - mBackgroundInsets.mLeft,
			  (mHeight - mBackgroundInsets.mBottom) - mBackgroundInsets.mTop);
	g->DrawImageBox(aBox, IMAGE_DIALOG);

	Font* aFont = GetFontById(0xb0);
	g->SetFont(aFont);
	g->SetColor(mColors[COLOR_LINES]);
	int aLeft = mContentInsets.mLeft + mBackgroundInsets.mLeft;
	g->DrawString("Music", aLeft, ((mMusicSlider->mHeight + mMusicSlider->mY) - mY) - aFont->GetAscent());
	g->DrawString(GT(0x0056b148), aLeft, ((mSoundSlider->mHeight + mSoundSlider->mY) - mY) - aFont->GetAscent());
	DrawCheckLabel(g, "Fullscreen", mFullscreenCheck);
	DrawCheckLabel(g, GT(0x0056b12c), mCursorCheck);
	DrawCheckLabel(g, GT(0x0056b114), mAccelCheck);
	DrawCheckLabel(g, GT(0x0056b108), mHintsCheck);
	DrawCheckLabel(g, GT(0x56b0e0), mColorblindCheck);
}

// 0x482d10: a slider moved: the volume of the music or of the sounds. A click on the sound slider (not a drag) is answered
// with a sound to hear it by.
void OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == 0)
		gApp->SetMusicVolume(theVal);
	else if (theId == 1)
	{
		gApp->SetSfxVolume(theVal);
		if (!mSoundSlider->mDragging)
			gApp->PlaySample(SOUND_1SEC_TIMER);
	}
}

// 0x4860e0: a box ticked or cleared. Windowed mode needs a desktop of 16 or 32 bits (the original tells a player whose
// desktop is not; the port's is always) and hardware acceleration a card that can do it (it is switched off again, and told,
// when not; a card that may be too slow is told too).
void OptionsDialog::CheckboxChecked(int theId, bool theChecked)
{
	if (gApp->mBoard != NULL)
		gApp->mBoard->MarkDirty();

	if (theId == 4 && theChecked)
	{
		if (!gApp->Is3DAccelerationSupported())
		{
			mAccelCheck->SetChecked(false, false);
			gApp->MakeDialog(gApp->GetDialogSet(), false, 0x69, 400, -1, 3, GT(0x0056b270), GT(0x56b280), "OK", std::string(), true);
			return;
		}
		if (!gApp->Is3DAccelerationRecommended())
			gApp->MakeDialog(gApp->GetDialogSet(), false, 0x68, -1, -1, 3, "Warning", GT(0x56b1f0), "OK", std::string(), true);
	}
}

// 0x486350: the buttons of the box. (Register and Update are left out.)
void OptionsDialog::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 9:
		gApp->MakeDialog(mColourSet, false, 0x75, -1, -1, 1, "WARNING", GT(0x56b308), "Yes", "No", true);
		break;
	case 0xfa8:
		gApp->ButtonDepress(0xfa8);
		break;
	case 10:
		gApp->ShowHelp();
		break;
	case 0xb:
		gApp->ShowCredits();
		break;
	default:
		Dialog::ButtonDepress(theId);
		break;
	}
}
