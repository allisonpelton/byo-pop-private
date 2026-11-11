// UpgradeDialog: the box that tells that the pilot's weapon has been upgraded (see UpgradeDialog.h).

#include "UpgradeDialog.h"
#include "AstroPopApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "UserProfile.h"

using namespace Sexy;

// 0x48a110. The boxes' colours are the green inset and yellow-green buttons of this box alone; the title picture sits 15
// below the top. The gauges are drawn from the profile: how many levels of each upgrade the pilot has, over how many there
// are (the first pilot's: 4, 3 and 4 levels, the second's 7 and 3, the third's 5 and 5, the fourth's 3, 4 and 4); the
// bar of the upgrade that was just awarded starts one level lower and fills up.
UpgradeDialog::UpgradeDialog(AstroPopApp* theApp, int theMode, int theSetting, const std::string& theName) :
	AstroDialog(theApp, 0x76, IMAGE_UPGRADE_TITLE, 0xf, Dialog::BUTTONS_FOOTER, true, Color(35, 115, 40, 255), Color(150, 165, 0, 255))
{
	mText = GT(0x0056b758) + theName;
	mOverlay = IMAGE_UPGRADE_OVERLAY1;
	for (int i = 0; i < NUM_BARS; i++)
	{
		mBarSpot[i].mX = 0;
		mBarSpot[i].mY = 0;
	}
	mSoundPlayed = false;
	mSoundDelay = 100;

	int aSetting1 = theMode + 0x33;
	int aSetting2 = theMode + 0x3b;
	int aLevel1 = 0;
	int aLevel2 = 0;
	int aLevel3 = 0;
	theApp->mUser->GetInt(aSetting1, &aLevel1);
	theApp->mUser->GetInt(aSetting2, &aLevel2);
	if (theMode == 0 || theMode == 3)
		theApp->mUser->GetInt(theMode + 0x37, &aLevel3);

	// how many levels each upgrade has
	float aLevels1 = 1.0f;
	float aLevels2 = 1.0f;
	float aLevels3 = 1.0f;
	switch (theMode)
	{
	case 0:
		aLevels1 = 4.0f;
		aLevels2 = 3.0f;
		break;
	case 1:
		aLevels1 = 7.0f;
		aLevels2 = 3.0f;
		break;
	case 2:
		aLevels1 = 5.0f;
		aLevels2 = 5.0f;
		break;
	case 3:
		aLevels1 = 3.0f;
		aLevels2 = 4.0f;
		break;
	}
	if (theMode == 0 || theMode == 3)
	{
		// three bars
		mOverlay = IMAGE_UPGRADE_OVERLAY1;
		mBarSpot[0].mX = 0x80;
		mBarSpot[0].mY = 0xb;
		mBarSpot[1].mX = 0x80;
		mBarSpot[1].mY = 0x1e;
		mBarSpot[2].mX = 0x80;
		mBarSpot[2].mY = 0x31;
		aLevels3 = 4.0f;
	}
	else if (theMode == 1 || theMode == 2)
	{
		// two bars
		mOverlay = IMAGE_UPGRADE_OVERLAY2;
		mBarSpot[0].mX = 0x80;
		mBarSpot[0].mY = 0x10;
		mBarSpot[1].mX = 0x80;
		mBarSpot[1].mY = 0x29;
		mBarSpot[2].mX = -1;
		mBarSpot[2].mY = -1;
	}

	float aPart1 = (float) (1.0 / (double) aLevels1);
	float aPart2 = (float) (1.0 / (double) aLevels2);
	float aPart3 = (float) (1.0 / (double) aLevels3);
	mBarNow[0] = mBarGoal[0] = (float) ((double) aLevel1 * (double) aPart1);
	mBarNow[1] = mBarGoal[1] = (float) ((double) aLevel2 * (double) aPart2);
	mBarNow[2] = mBarGoal[2] = (float) ((double) aLevel3 * (double) aPart3);
	if (theSetting == aSetting1)
		mBarNow[0] = (float) ((double) (aLevel1 - 1) * (double) aPart1);
	else if (theSetting == aSetting2)
		mBarNow[1] = (float) ((double) (aLevel2 - 1) * (double) aPart2);
	else
		mBarNow[2] = (float) ((double) (aLevel3 - 1) * (double) aPart3);

	for (int i = 0; i < NUM_BARS; i++)
	{
		if (1.0f < mBarNow[i])
			mBarNow[i] = 1.0f;
		if (1.0f < mBarGoal[i])
			mBarGoal[i] = 1.0f;
	}
}

// 0x48a670
UpgradeDialog::~UpgradeDialog()
{
}

// 0x487f00: the announcement, the picture of the gauges, and under it either the words that upgrades come every four levels
// with the bars, or (when no bar has anything left to gain) the words that the weapon is fully upgraded with the full bar
void UpgradeDialog::Draw(Graphics* g)
{
	AstroDialog::Draw(g);
	g->SetFont(GetFontById(0xb0));
	g->SetColor(Color(0xff, 0xff, 0xff));

	int aTextTop = mPicture->mHeight + 0x1e;
	if (!mSettled)
		aTextTop += m124;
	Rect aTextRect(0x46, aTextTop, 300, 0x32);
	int aTextHeight = WriteWordWrapped(g, aTextRect, mText, -1, -1);

	int aOverlayX = mWidth / 2 - mOverlay->mWidth / 2;
	int aOverlayY = aTextHeight + 0x14 + aTextTop;
	g->DrawImage(mOverlay, aOverlayX, aOverlayY);
	g->SetColor(Color(0xff, 0xff, 0));

	bool aHasThird = mBarSpot[2].mX != -1;
	if (mBarGoal[0] < 1.0f || mBarGoal[1] < 1.0f || (mBarGoal[2] < 1.0f && aHasThird))
	{
		WriteString(g, GT(0x0056b4c0), 0x46, mOverlay->mHeight + aTextHeight + 0x23 + aTextTop, 300, 0, true, 0, -1);
		for (int i = 0; i < NUM_BARS; i++)
		{
			if (i == 2 && !aHasThird)
				break;
			int aWidth = (int) ((float) IMAGE_UPGRADE_BAR->mWidth * mBarNow[i]);
			g->DrawImage(IMAGE_UPGRADE_BAR, mBarSpot[i].mX + aOverlayX, mBarSpot[i].mY + aOverlayY,
						 Rect(0, 0, aWidth, IMAGE_UPGRADE_BAR->mHeight));
		}
		return;
	}

	WriteString(g, GT(0x0056b4e0), 0x3c, mOverlay->mHeight + aTextHeight + 0x23 + aTextTop, 300, 0, true, 0, -1);
	int aBarY = mBarSpot[0].mY + aOverlayY;
	if (aHasThird)
		g->DrawImage(IMAGE_UPGRADE_BAR_MAX3, mBarSpot[0].mX + aOverlayX, aBarY);
	else
		g->DrawImage(IMAGE_UPGRADE_BAR_MAX2, mBarSpot[0].mX + aOverlayX, aBarY);
}

// 0x484530: once the box has come in, the sound comes after 100 updates, and each bar fills by 0.002 an update up to its goal
void UpgradeDialog::Update()
{
	AstroDialog::Update();
	if (!mSettled)
		return;

	if (!mSoundPlayed)
	{
		mSoundDelay--;
		if (mSoundDelay <= 0)
		{
			mSoundPlayed = true;
			mApp->PlaySample(SOUND_SUPA_UPGRADE);
		}
	}

	static const int kOrder[NUM_BARS] = { 0, 2, 1 };
	for (int i = 0; i < NUM_BARS; i++)
	{
		int aBar = kOrder[i];
		float aSum = mBarNow[aBar] + 0.002f;
		mBarNow[aBar] = aSum;
		if (aSum > mBarGoal[aBar])
			mBarNow[aBar] = mBarGoal[aBar];
	}
}
