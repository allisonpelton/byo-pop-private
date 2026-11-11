// PowerDialog: the box that explains the pilot's weapon (see PowerDialog.h).

#include "PowerDialog.h"
#include "AstroPopApp.h"
#include "Checkbox.h"
#include "DialogButton.h"
#include "Fixes.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Res.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x48b7a0 (the base class' 0x484880 inside it). The box is given the pictures made for it: the inset of the box and the
// strip of the buttons (the buttons' three pictures sit side by side in it), and the tick box's two halves side by side.
// The lines and pictures depend on the pilot: the first (the laser) and the fourth (the click) have two, the other two
// three.
PowerDialog::PowerDialog(AstroPopApp* theApp, int thePilot, MemoryImage* theInsetImage, MemoryImage* theStripImage,
						 Image* theCheckImage) :
	Text2004<Dialog>(theInsetImage, theStripImage, 0x7b, true, GT(0x0056c388), std::string(), "OK", Dialog::BUTTONS_FOOTER)
{
	mApp = theApp;
	mResult = -1;

	int aHalfWidth = theCheckImage->mWidth / 2;
	mCheckbox = new Checkbox(NULL, NULL, 0, this);
	mCheckbox->mUncheckedImage = theCheckImage;
	mCheckbox->mCheckedImage = theCheckImage;
	mCheckbox->mCheckedRect = Rect(aHalfWidth, 0, aHalfWidth, theCheckImage->mHeight);
	mCheckbox->mUncheckedRect = Rect(0, 0, aHalfWidth, theCheckImage->mHeight);
	mCheckbox->SetChecked(false);

	mYesButton->mNormalRect = Rect(0, 0, 0x78, 0x21);
	mYesButton->mOverRect = Rect(0x78, 0, 0x78, 0x21);
	mYesButton->mDownRect = Rect(0xf0, 0, 0x78, 0x21);
	// the 2004 Dialog reads the button height from the button picture at each use (GetPreferredHeight 0x4e91d0 and Resize); the
	// port keeps the height it took at construction, so it follows the picture that replaces it
	mYesButton->mComponentImage = theStripImage;
	mButtonHeight = theStripImage->mHeight;
	if (mNoButton != NULL)
	{
		mNoButton->mNormalRect = Rect(0, 0, 0x78, 0x21);
		mNoButton->mOverRect = Rect(0x78, 0, 0x78, 0x21);
		mNoButton->mDownRect = Rect(0xf0, 0, 0x78, 0x21);
		mNoButton->mComponentImage = theStripImage;
	}

	switch (thePilot)
	{
	case 0:
		mLines.push_back(GT(0x0056c360));
		mLines.push_back(GT(0x0056c320));
		mImages.push_back(IMAGE_TH_SUPA_VECTOR1);
		mImages.push_back(IMAGE_TH_SUPA_VECTOR2);
		break;
	case 1:
		mLines.push_back(GT(0x0056c2f0));
		mLines.push_back(GT(0x0056c2cc));
		mLines.push_back(GT(0x0056c2b8));
		mImages.push_back(IMAGE_TH_SUPA_SPROCKET1);
		mImages.push_back(IMAGE_TH_SUPA_SPROCKET2);
		mImages.push_back(IMAGE_TH_SUPA_SPROCKET3);
		break;
	case 2:
		mLines.push_back(GT(0x0056c28c));
		mLines.push_back(GT(0x0056c260));
		mLines.push_back(GT(0x0056c23c));
		mImages.push_back(IMAGE_TH_SUPA_TURBOT1);
		mImages.push_back(IMAGE_TH_SUPA_TURBOT2);
		mImages.push_back(IMAGE_TH_SUPA_TURBOT3);
		break;
	case 3:
		mLines.push_back(GT(0x0056c218));
		mLines.push_back(GT(0x0056c1e0));
		mImages.push_back(IMAGE_TH_SUPA_VIXX1);
		mImages.push_back(IMAGE_TH_SUPA_VIXX2);
		break;
	}
}

// 0x48a770 (body 0x48a560, then the base's 0x4839a0 that deletes the tick box). The three pictures go with the box.
PowerDialog::~PowerDialog()
{
	delete mComponentImage;
	mComponentImage = NULL;
	if (mYesButton != NULL)
	{
		delete mYesButton->mComponentImage;
		mYesButton->mComponentImage = NULL;
	}
	delete mCheckbox->mCheckedImage;
	mCheckbox->mCheckedImage = NULL;
	mCheckbox->mUncheckedImage = NULL;
	delete mCheckbox;
	mCheckbox = NULL;
}

// 0x483a60: the tick box is a widget of its own on the screen, above the box
void PowerDialog::AddedToManager(WidgetManager* theManager)
{
	Dialog::AddedToManager(theManager);
	theManager->AddWidget(mCheckbox);
}

// 0x483a90
void PowerDialog::RemovedFromManager(WidgetManager* theManager)
{
	Dialog::RemovedFromManager(theManager);
	theManager->RemoveWidget(mCheckbox);
}

// 0x483bb0 (base 0x483ac0): the tick box goes above the button at the left edge of the box, and then to the middle
void PowerDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	mCheckbox->Resize(mContentInsets.mLeft + 10 + mX, (mYesButton->mY - mYesButton->mHeight) - 10,
					  mCheckbox->mCheckedImage->mWidth, mCheckbox->mCheckedImage->mHeight);
	mCheckbox->Resize((((mWidth / 2 - mContentInsets.mRight) - mContentInsets.mLeft) - mCheckbox->mWidth / 2) + mX,
					  mCheckbox->mY, mCheckbox->mWidth, mCheckbox->mHeight);
}

// 0x489190 (base 0x489070): the box with the words beside the tick box, then the pictures in a row with their lines
void PowerDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);
	Rect aBox(mBackgroundInsets.mLeft, mBackgroundInsets.mTop, (mWidth - mBackgroundInsets.mRight) - mBackgroundInsets.mLeft,
			  (mHeight - mBackgroundInsets.mBottom) - mBackgroundInsets.mTop);
	g->DrawImageBox(aBox, IMAGE_DIALOG);

	g->SetFont(mLinesFont);
	g->SetColor(mColors[COLOR_LINES]);
	g->DrawString(GT(0x0056b654), (mCheckbox->mWidth - mX) + mCheckbox->mX, ((mCheckbox->mHeight - mY) - 4) + mCheckbox->mY);

	g->SetFont(mLinesFont);
	g->SetColor(Color(0xff, 0xff, 0xff));
	for (unsigned int i = 0; i < mImages.size(); i++)
	{
		Image* anImage = mImages[i];
		int aY = mSpaceAfterHeader + mContentInsets.mTop + 0x1e;
		int aX = (anImage->mWidth + 0x50) * i + 0x32 + mContentInsets.mLeft;
		g->DrawImage(anImage, aX, aY);
		Rect aRect(aX - 0x19, anImage->mHeight + 10 + aY, anImage->mWidth + 0x32, mHeight);
		WriteWordWrapped(g, aRect, mLines[i], -1, 0);
	}
}

// 0x483a20: Return, Space and Escape press the button (the dialog's first answer: 2000 + its id)
void PowerDialog::KeyChar(SexyChar theChar)
{
	if (theChar == '\n' || theChar == '\r' || theChar == ' ' || theChar == '\x1b')
		mApp->ButtonDepress(mId + 2000);
}

// 0x483b90 (slot 0xc8 of the box's vftable 0x56b8b8; the 400 that the base's 0x483a10 returns is not asked for): twice the
// height of the weapon's pictures (all four pilots' pictures are as high as the Vixx's first) and 150 for the rest
int PowerDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return IMAGE_TH_SUPA_VIXX1->mHeight * 2 + 0x96;
}

// 0x483b50: the box's button, pressed, does what a Dialog's does and then plays the sample whose number is in the field at +0x118,
// which the constructor sets to 0 and nothing else writes: the first sound of the original's table. The original numbers its
// sounds in the order of its resource map (by name), so number 0 is SOUND_1SEC_TIMER, the first of them (the sound log of a
// whole game has its 40960-byte buffer 2, the one of timer1, played at the OK of this box: it is the first sound made). The
// port numbers them in the order of the manifest, so it asks for that sound by name.
// (fix "weaponBoxTick": the sound is not played)
void PowerDialog::ButtonPress(int theId)
{
	Dialog::ButtonPress(theId);
	if (!FixEnabled("weaponBoxTick"))
		mApp->PlaySample(SOUND_1SEC_TIMER);
}

// 0x483b20: a tick or an untick draws the box again
void PowerDialog::CheckboxChecked(int theId, bool theChecked)
{
	(void) theId;
	(void) theChecked;
	MarkDirtyFull();
	mCheckbox->MarkDirty();
}
