#include "GameSelector.h"
#include "StartButton.h"
#include "LockableButton.h"
#include "OverlayWidget.h"
#include "MrMoney.h"
#include "WinBMApp.h"
#include "Res.h"
#include "Compat12.h"
#include "ButtonWidget.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "DDImage.h"
#include "Image.h"

using namespace Sexy;

// MrMoney animations started by the selector (MrMoney::PlayAnim ids; 0..2 are the game types).
static const int MRMONEY_ANIM_LOVEIT = 9;
static const int MRMONEY_ANIM_WELCOME = 23;

// 0x4107c0
GameSelector::GameSelector(WinBMApp* theApp)
{
	mApp = theApp;

	for (int i = 0; i < 3; i++)
	{
		ButtonWidget* aSkillButton = new ButtonWidget(SKILL_BUTTON_ID + i, this);
		mSkillButtons[i] = aSkillButton;
		aSkillButton->mDoFinger = true;
		aSkillButton->mButtonImage = mApp->mSkillButtonImages[i][0];
		aSkillButton->mOverImage = mApp->mSkillButtonImages[i][1];
		aSkillButton->mDownImage = mApp->mSkillButtonImages[i][2];

		LockableButton* aTypeButton = new LockableButton(mApp, GAMETYPE_BUTTON_ID + i, this);
		mTypeButtons[i] = aTypeButton;
		aTypeButton->mDoFinger = true;
		aTypeButton->mButtonImage = mApp->mGameTypeButtonImages[i][0];
		aTypeButton->mOverImage = mApp->mGameTypeButtonImages[i][1];
		aTypeButton->mDownImage = mApp->mGameTypeButtonImages[i][2];
	}

	mStartButton = new StartButton(mApp, START_BUTTON_ID, this);

	// Three more buttons that nothing ever positions (0x0 rect, so never drawn or clicked); kept as in
	// the original. Only mSelectorButtonImages[1][0] is ever set by the app.
	mButton0 = new ButtonWidget(START_BUTTON_ID, this);
	mButton0->mButtonImage = mApp->mSelectorButtonImages[0][0];
	mButton0->mOverImage = mApp->mSelectorButtonImages[0][1];
	mButton0->mDownImage = mApp->mSelectorButtonImages[0][2];
	mButton0->mDoFinger = true;

	mButton1 = new ButtonWidget(BUTTON1_ID, this);
	mButton1->mButtonImage = mApp->mSelectorButtonImages[1][0];
	mButton1->mOverImage = mApp->mSelectorButtonImages[1][1];
	mButton1->mDownImage = mApp->mSelectorButtonImages[1][2];
	mButton1->mDoFinger = true;

	mButton2 = new ButtonWidget(BUTTON2_ID, this);
	mButton2->mButtonImage = mApp->mSelectorButtonImages[2][0];
	mButton2->mOverImage = mApp->mSelectorButtonImages[2][1];
	mButton2->mDownImage = mApp->mSelectorButtonImages[2][2];
	mButton2->mDoFinger = true;

	mOverlay = new OverlayWidget(mApp);

	mGameType = -1;
	mSkill = -1;
	SetGameType(mApp->mGameType);  // no voice: mUpdateCnt is still 0
	SetSkill(mApp->mSkillLevel);

	mUnused98[0] = 0;
	mUnused98[1] = 0;
	mPanelY = 0;
	mSlidingOut = false;
	mSlidingIn = false;
	mSnapshot = NULL;
	// port: initialised (the original only sets them in SlideIn/SlideOut, before Draw reads them)
	mMidHeight = 0;
	mSlideY = 0;
}

// 0x40fb90
GameSelector::~GameSelector()
{
	delete mStartButton;
	delete mButton0;
	delete mButton1;
	delete mButton2;
	for (int i = 0; i < 3; i++)
		delete mSkillButtons[i];
	for (int i = 0; i < 3; i++)
		delete mTypeButtons[i];
	delete mOverlay;
	delete mSnapshot;
}

// 0x40fcb0
void GameSelector::SetGameType(int theType)
{
	mGameType = theType;

	// The selected type shows its "down" image in every state.
	for (int i = 0; i < 3; i++)
	{
		LockableButton* aButton = mTypeButtons[i];
		aButton->mButtonImage = (i == mGameType) ? mApp->mGameTypeButtonImages[i][2] : mApp->mGameTypeButtonImages[i][0];
		aButton->mOverImage = (i == mGameType) ? mApp->mGameTypeButtonImages[i][2] : mApp->mGameTypeButtonImages[i][1];
		aButton->mDownImage = mApp->mGameTypeButtonImages[i][2];
		aButton->MarkDirty();
	}

	// Voice and Mr. Money animation, unless he is saying one of the greetings.
	if (mUpdateCnt > 0)
	{
		int anAnim = mApp->mMrMoney->mAnim;
		if (anAnim != MRMONEY_ANIM_WELCOME && anAnim != MRMONEY_ANIM_LOVEIT)
		{
			mApp->PlaySample(SOUND_ACTION + theType);
			mApp->mMrMoney->PlayAnim(theType);
		}
	}
}

// 0x40fdc0
void GameSelector::SetSkill(int theSkill)
{
	mSkill = theSkill;

	for (int i = 0; i < 3; i++)
	{
		ButtonWidget* aButton = mSkillButtons[i];
		aButton->mButtonImage = (i == mSkill - 1) ? mApp->mSkillButtonImages[i][2] : mApp->mSkillButtonImages[i][0];
		aButton->mOverImage = (i == mSkill - 1) ? mApp->mSkillButtonImages[i][2] : mApp->mSkillButtonImages[i][1];
		aButton->mDownImage = mApp->mSkillButtonImages[i][2];
		aButton->MarkDirty();
	}
}

// 0x40fe90
void GameSelector::Update()
{
	Widget::Update();

	if (mUpdateCnt == 100)
	{
		if (!mApp->mWelcomePlayed)
		{
			mApp->PlaySample(SOUND_WELCOME);
			mApp->mMrMoney->PlayAnim(MRMONEY_ANIM_WELCOME);
			mApp->mWelcomePlayed = true;
		}
		else if (CrtRand() % 10 == 0)
		{
			mApp->PlaySample(SOUND_LOVEIT);
			mApp->mMrMoney->PlayAnim(MRMONEY_ANIM_LOVEIT);
		}
	}

	if (mSlidingOut)
	{
		// Roll up (8 px per update), then fade the overlay out and let the app delete this selector.
		mSlideY -= 2;
		if (mMidHeight > 0)
			mMidHeight -= 8;
		if (mMidHeight <= 0)
		{
			mMidHeight = 0;
			mOverlay->mAlpha -= 0.05;
			if (mOverlay->mAlpha <= 0.0)
				mApp->RemoveGameSelector(); // SafeDeleteWidget: this stays valid until the frame ends
		}
		MarkDirty();
		return;
	}

	if (mSlidingIn)
	{
		mOverlay->mAlpha += 0.05;
		if (mOverlay->mAlpha > 1.0)
			mOverlay->mAlpha = 1.0;

		mSlideY += 2;
		if (mSlideY > 0)
		{
			// In place: bring the buttons back and delete the finished board underneath.
			mSlideY = 0;
			mSlidingIn = false;
			mWidgetManager->AddWidget(mButton0);
			mWidgetManager->AddWidget(mButton1);
			mWidgetManager->AddWidget(mButton2);
			mWidgetManager->AddWidget(mStartButton);
			for (int i = 0; i < 3; i++)
			{
				mWidgetManager->AddWidget(mTypeButtons[i]);
				mWidgetManager->AddWidget(mSkillButtons[i]);
			}
			OrderInManagerChanged();
			mApp->RemoveBoard();
		}

		mMidHeight += 8;
		if (mMidHeight > SLIDE_MID_H)
			mMidHeight = SLIDE_MID_H;
		MarkDirty();
	}
}

// 0x4100b0
void GameSelector::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mButton0);
	theWidgetManager->AddWidget(mButton1);
	theWidgetManager->AddWidget(mButton2);
	theWidgetManager->AddWidget(mStartButton);
	for (int i = 0; i < 3; i++)
	{
		theWidgetManager->AddWidget(mTypeButtons[i]);
		theWidgetManager->AddWidget(mSkillButtons[i]);
	}
	theWidgetManager->AddWidget(mOverlay);
}

// 0x410140
void GameSelector::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mButton0);
	theWidgetManager->RemoveWidget(mButton1);
	theWidgetManager->RemoveWidget(mButton2);
	theWidgetManager->RemoveWidget(mStartButton);
	for (int i = 0; i < 3; i++)
	{
		theWidgetManager->RemoveWidget(mTypeButtons[i]);
		theWidgetManager->RemoveWidget(mSkillButtons[i]);
	}
	theWidgetManager->RemoveWidget(mOverlay);
}

// 0x4101d0
void GameSelector::OrderInManagerChanged()
{
	// Each call puts the widget directly above this one, so the overlay ends up on top of everything.
	mWidgetManager->PutInfront(mOverlay, this);
	for (int i = 0; i < 3; i++)
	{
		mWidgetManager->PutInfront(mTypeButtons[i], this);
		mWidgetManager->PutInfront(mSkillButtons[i], this);
	}
	mWidgetManager->PutInfront(mStartButton, this);
	mWidgetManager->PutInfront(mButton2, this);
	mWidgetManager->PutInfront(mButton1, this);
	mWidgetManager->PutInfront(mButton0, this);
}

// 0x410260
void GameSelector::LayoutButtons()
{
	int i = 0;
	for (int aX = 178; aX < 640; aX += 154)
		mTypeButtons[i++]->Resize(aX, mPanelY + 58, 154, 266);

	static const int aSkillX[3] = { 29, 186, 339 };
	for (i = 0; i < 3; i++)
		mSkillButtons[i]->Resize(aSkillX[i] + PANEL_X, mPanelY + 359, 113, 52);

	mStartButton->Resize(180, mPanelY + 429, 455, 38);
	mOverlay->Resize(0, 0, mWidth, mHeight);
	// mButton0..2 are not positioned (original).
}

// 0x410d50
void GameSelector::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	LayoutButtons();
}

// 0x410330
void GameSelector::ButtonPress(int theId, int theClickCount)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x410730
void GameSelector::ButtonDepress(int theId)
{
	switch (theId)
	{
	case START_BUTTON_ID:
		StartGame();
		break;

	case SKILL_BUTTON_ID:
	case SKILL_BUTTON_ID + 1:
	case SKILL_BUTTON_ID + 2:
		// A second click on the selected skill starts the game.
		if (mSkill == theId - 10)
			StartGame();
		else
			SetSkill(theId - 10);
		break;

	case GAMETYPE_BUTTON_ID:
	case GAMETYPE_BUTTON_ID + 1:
	case GAMETYPE_BUTTON_ID + 2:
		if (mGameType == theId - GAMETYPE_BUTTON_ID)
			StartGame();
		else
			SetGameType(theId - GAMETYPE_BUTTON_ID);
		break;
	}
}

// 0x410340
void GameSelector::StartGame()
{
	if (mGameType == -1)
		mGameType = 0;
	if (mSkill == -1)
		mSkill = 1;

	mApp->mGameType = mGameType;
	mApp->mSkillLevel = mSkill;
	mApp->StartGame(); // help screen first, or a new board

	for (int i = 0; i < 3; i++)
	{
		mSkillButtons[i]->SetDisabled(true);
		mTypeButtons[i]->SetDisabled(true);
	}
	mButton1->SetDisabled(true); // original: mButton1, not mStartButton
}

// 0x4103e0
void GameSelector::SlideIn()
{
	if (mSlidingIn)
		return;

	// Capture the screen (this selector drawn normally, without the overlay and Mr. Money) for the
	// window-shade animation in Draw.
	delete mSnapshot;
	mSnapshot = new DDImage(mApp->mDDInterface);
	mSnapshot->Create(mWidth, mHeight);
	mSnapshot->SetImageMode(false, false);
	mOverlay->SetVisible(false);
	mApp->mMrMoney->SetVisible(false);
	Graphics aSnapshotG(mSnapshot);
	mWidgetManager->DrawWidgetsTo(&aSnapshotG);
	mOverlay->SetVisible(true);
	mApp->mMrMoney->SetVisible(true);

	mOverlay->mAlpha = 0.0;
	mHasTransparencies = true;
	mHasAlpha = true;
	mSlidingIn = true;
	mMidHeight = 0;
	mSlideY = -100;

	mWidgetManager->RemoveWidget(mButton0);
	mWidgetManager->RemoveWidget(mButton1);
	mWidgetManager->RemoveWidget(mButton2);
	mWidgetManager->RemoveWidget(mStartButton);
	for (int i = 0; i < 3; i++)
	{
		mWidgetManager->RemoveWidget(mTypeButtons[i]);
		mWidgetManager->RemoveWidget(mSkillButtons[i]);
	}
}

// 0x410590
void GameSelector::SlideOut()
{
	if (mSlidingOut)
		return;

	delete mSnapshot;
	mSnapshot = new DDImage(mApp->mDDInterface);
	mSnapshot->Create(mWidth, mHeight);
	mSnapshot->SetImageMode(false, false);
	mOverlay->SetVisible(false);
	mApp->mMrMoney->SetVisible(false);
	Graphics aSnapshotG(mSnapshot);
	mWidgetManager->DrawWidgetsTo(&aSnapshotG);
	mOverlay->SetVisible(true);
	mApp->mMrMoney->SetVisible(true);

	// The overlay's alpha is left as it is; Update fades it out after the roll-up.
	mHasTransparencies = true;
	mHasAlpha = true;
	mMouseVisible = false;
	mSlidingOut = true;
	mMidHeight = SLIDE_MID_H;
	mSlideY = 0;

	mWidgetManager->RemoveWidget(mButton0);
	mWidgetManager->RemoveWidget(mButton1);
	mWidgetManager->RemoveWidget(mButton2);
	mWidgetManager->RemoveWidget(mStartButton);
	for (int i = 0; i < 3; i++)
	{
		mWidgetManager->RemoveWidget(mTypeButtons[i]);
		mWidgetManager->RemoveWidget(mSkillButtons[i]);
	}
}

// 0x410bf0
void GameSelector::Draw(Graphics* g)
{
	if (!mSlidingOut && !mSlidingIn)
	{
		g->DrawImage(mApp->mImages[IMAGE_SELECTBACK], PANEL_X, mPanelY);
		return;
	}

	// Sliding: the captured panel as a top strip at mSlideY, a middle strip stretched to mMidHeight and
	// the bottom strip below it.
	int aSlideY = mSlideY;
	g->SetFastStretch(true);
	g->DrawImage(mSnapshot, Rect(PANEL_X, aSlideY, 467, SLIDE_TOP_H), Rect(PANEL_X, 0, 467, SLIDE_TOP_H));
	g->DrawImage(mSnapshot, Rect(PANEL_X, aSlideY + SLIDE_TOP_H, 467, mMidHeight), Rect(PANEL_X, SLIDE_TOP_H, 467, SLIDE_MID_H));
	g->DrawImage(mSnapshot, Rect(PANEL_X, mMidHeight + aSlideY + SLIDE_TOP_H, 467, SLIDE_BOTTOM_H), Rect(PANEL_X, SLIDE_BOTTOM_SRC_Y, 467, SLIDE_BOTTOM_H));
	g->SetFastStretch(false);
}
