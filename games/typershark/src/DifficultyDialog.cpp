#include "DifficultyDialog.h"
#include "WinWSApp.h"
#include "UserProfile.h"
#include "GameText.h"
#include "DialogButton.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"

using namespace Sexy;

// Not translated: KeyDialog's KeyChar (slot 20, 0x439210) and the button listener slots 3 to 5 are the inherited ones;
// the EditListener at +0x138 (vftable 0x52e438) has four empty slots (0x47b200 and 0x47aff0) and nothing uses it

// 0x412430: the average of the user's last ten typing speeds that are above 0 (the sum includes the others; 0 when
// there is no speed above 0)
static int GetAverageWPM(UserProfile* theUser)
{
	int aCount = (0 < theUser->mWPM[0]) ? 1 : 0;
	if (0 < theUser->mWPM[1])
		aCount++;
	if (0 < theUser->mWPM[2])
		aCount++;
	if (0 < theUser->mWPM[3])
		aCount++;
	if (0 < theUser->mWPM[4])
		aCount++;
	if (0 < theUser->mWPM[5])
		aCount++;
	if (0 < theUser->mWPM[6])
		aCount++;
	if (0 < theUser->mWPM[7])
		aCount++;
	if (0 < theUser->mWPM[8])
		aCount++;
	if (0 < theUser->mWPM[9])
		aCount++;

	if (aCount == 0)
		return 0;
	return (theUser->mWPM[0] + theUser->mWPM[1] + theUser->mWPM[2] + theUser->mWPM[3] + theUser->mWPM[4] +
		theUser->mWPM[5] + theUser->mWPM[6] + theUser->mWPM[7] + theUser->mWPM[8] + theUser->mWPM[9]) / aCount;
}

// The button that suits the speed: under 30 the first, under 40 the second, under 60 the third, under 80 the fourth, else
// the fifth. The original repeats the chain of tests in the constructor, Resize and Draw; they all use this.
static int GetRecommendedButton(UserProfile* theUser)
{
	int aWPM = GetAverageWPM(theUser);
	if (aWPM < 0x50)
	{
		if (aWPM < 0x3c)
		{
			if (aWPM < 0x28)
			{
				if (aWPM < 0x1e)
					return 0;
				return 1;
			}
			return 2;
		}
		return 3;
	}
	return 4;
}

// 0x43f0f0: KeyDialog (0x43ec30) first, with the footer button "Cancel" (mode 3) and the keys of KeyDialog that press it
// (escape; mCloseOnKey is set at the end). The recommended button gets IMAGE_DBUTTON_HILIGHT.
DifficultyDialog::DifficultyDialog(WinWSApp* theApp) :
	KeyDialog(theApp, theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DBUTTON], DIALOG_DIFFICULTY, true, GT(0x0052ed38), "",
		GT(0x0052ed44), Dialog::BUTTONS_FOOTER),
	mFont(NULL),
	mAppCopy(theApp)
{
	SetButtonFont(mAppCopy->mFontRockwell13);
	SetHeaderFont(mAppCopy->mFontRockwell13Bold);
	SetLinesFont(mAppCopy->mFontRockwell13);
	Dialog::SetColor(COLOR_HEADER, Color(0, 0, 0));
	Dialog::SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));
	mContentInsets.mLeft = 0x28;
	mContentInsets.mRight = 0x28;
	mContentInsets.mTop = 0x14;
	mContentInsets.mBottom = 0x28;
	mFont = mAppCopy->mFontRockwell13;

	Image* aButtonImages[NUM_BUTTONS];
	int i;
	for (i = 0; i < NUM_BUTTONS; i++)
		aButtonImages[i] = mAppCopy->mImages[IMAGE_DBUTTON];
	aButtonImages[GetRecommendedButton(mAppCopy->mUser)] = mAppCopy->mImages[IMAGE_DBUTTON_HILIGHT];

	mButtons[0] = new DialogButton(aButtonImages[0], 600, mAppCopy);
	mButtons[0]->mDoFinger = true;
	mButtons[0]->SetFont(mAppCopy->mFontRockwell13);
	mButtons[0]->mLabel = GT(0x0052d280);

	mButtons[1] = new DialogButton(aButtonImages[1], 0x259, mAppCopy);
	mButtons[1]->mDoFinger = true;
	mButtons[1]->SetFont(mAppCopy->mFontRockwell13);
	mButtons[1]->mLabel = GT(0x0052d278);

	mButtons[2] = new DialogButton(aButtonImages[2], 0x25a, mAppCopy);
	mButtons[2]->mDoFinger = true;
	mButtons[2]->SetFont(mAppCopy->mFontRockwell13);
	mButtons[2]->mLabel = GT(0x0052d270);

	mButtons[3] = new DialogButton(aButtonImages[3], 0x25b, mAppCopy);
	mButtons[3]->mDoFinger = true;
	mButtons[3]->SetFont(mAppCopy->mFontRockwell13);
	mButtons[3]->mLabel = GT(0x0052d268);

	mButtons[4] = new DialogButton(aButtonImages[4], 0x25c, mAppCopy);
	mButtons[4]->mDoFinger = true;
	mButtons[4]->SetFont(mAppCopy->mFontRockwell13);
	mButtons[4]->mLabel = GT(0x0052ed2c);

	mCloseOnKey = true;
}

// 0x43a330 (body; the scalar deleting destructor is 0x43a440): the five buttons go, then KeyDialog's body (0x43a2d0)
DifficultyDialog::~DifficultyDialog()
{
	delete mButtons[0];
	delete mButtons[1];
	delete mButtons[2];
	delete mButtons[3];
	delete mButtons[4];
}

// 0x437e00 slot 50: room for the five buttons and the recommendation line
int DifficultyDialog::GetPreferredHeight(int theWidth)
{
	return mFont->GetHeight() * 2 + 0x181;
}

// 0x437e20 slot 1
void DifficultyDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	KeyDialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mButtons[0]);
	theWidgetManager->AddWidget(mButtons[1]);
	theWidgetManager->AddWidget(mButtons[2]);
	theWidgetManager->AddWidget(mButtons[3]);
	theWidgetManager->AddWidget(mButtons[4]);
}

// 0x437e80 slot 2
void DifficultyDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	KeyDialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mButtons[0]);
	theWidgetManager->RemoveWidget(mButtons[1]);
	theWidgetManager->RemoveWidget(mButtons[2]);
	theWidgetManager->RemoveWidget(mButtons[3]);
	theWidgetManager->RemoveWidget(mButtons[4]);
}

// 0x437ee0 slot 13: the buttons are 0x28 high, 0x2d apart, from y 0x41; the recommended one and the ones after it are
// moved down two font heights, which leaves room for the text above the recommended button
void DifficultyDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	KeyDialog::Resize(theX, theY, theWidth, theHeight);

	int aShift[NUM_BUTTONS];
	int i;
	for (i = 0; i < NUM_BUTTONS; i++)
		aShift[i] = 0;

	int aRecommended = GetRecommendedButton(mAppCopy->mUser);
	for (i = aRecommended; i < NUM_BUTTONS; i++)
		aShift[i] = mFont->GetHeight() << 1;

	mButtons[0]->Resize(mX + mContentInsets.mLeft, aShift[0] + 0x41 + mY, (mWidth - mContentInsets.mLeft) - mContentInsets.mRight, 0x28);
	mButtons[1]->Resize(mX + mContentInsets.mLeft, aShift[1] + 0x6e + mY, (mWidth - mContentInsets.mLeft) - mContentInsets.mRight, 0x28);
	mButtons[2]->Resize(mX + mContentInsets.mLeft, aShift[2] + 0x9b + mY, (mWidth - mContentInsets.mLeft) - mContentInsets.mRight, 0x28);
	mButtons[3]->Resize(mX + mContentInsets.mLeft, aShift[3] + 200 + mY, (mWidth - mContentInsets.mLeft) - mContentInsets.mRight, 0x28);
	mButtons[4]->Resize(mX + mContentInsets.mLeft, aShift[4] + 0xf5 + mY, (mWidth - mContentInsets.mLeft) - mContentInsets.mRight, 0x28);
}

// 0x43ac70 slot 16: the line "Recommended difficulty:" in black, 13 pixels under the top of the recommended button
// less the font height (in dialog coordinates), at the left edge of the content
void DifficultyDialog::Draw(Graphics* g)
{
	KeyDialog::Draw(g);

	int aRecommended = GetRecommendedButton(mAppCopy->mUser);
	int aFontHeight = mFont->GetHeight();
	DialogButton* aButton = mButtons[aRecommended];
	g->SetColor(Color(0, 0, 0));
	g->DrawString(GT(0x0052e5d4), mContentInsets.mLeft + 4, ((aButton->mY - aFontHeight) - mY) + 0xd);
}
