// MainMenu.cpp: MainMenu (MainMenu.h).
#include "MainMenu.h"

#include <math.h>

#include "Graphics.h"
#include "HyperlinkWidget.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "PodDialog.h"
#include "Profile.h"
#include "Res.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "WidgetManager.h"
#include "WinDMApp.h"
#include "Board.h"
#include "EffectOverlay.h"

using namespace Sexy;

// 0x5c9dcd
MainMenu::MainMenu(WinDMApp* theApp)
{
	mHasAlpha = true;
	mApp = theApp;
	mBackdropClear = false;
	mBackdropImage = NULL;
	mSound = NULL;
	mPrevSound = NULL;
	mCenterGlow = 0;

	mUserLink = new HyperlinkWidget(0x14, this);
	mUserLink->SetFont(FONT_VAG_ROUNDED_11);
	mUserLink->mOverColor = Color(200, 255, 226);
	mUserLink->mLabel = "click to change user";

	// the four mode buttons
	mClassicButton = new PodButton(0, this);
	mClassicButton->mGlowColor = Color(255, 255, 0);
	mClassicButton->mDoFinger = true;
	mClassicButton->mSelectedGlowImage = IMAGE_MAIN_BIG_SELECT;
	mClassicButton->mGlowImage = IMAGE_MAIN_BIG_OVER;
	mClassicButton->mButtonImage = IMAGE_MAIN_CLASSIC;
	mClassicButton->mSounds = true;

	mActionButton = new PodButton(1, this);
	mActionButton->mGlowColor = Color(255, 255, 0);
	mActionButton->mDoFinger = true;
	mActionButton->mSelectedGlowImage = IMAGE_MAIN_BIG_SELECT;
	mActionButton->mGlowImage = IMAGE_MAIN_BIG_OVER;
	mActionButton->mButtonImage = IMAGE_MAIN_ACTION;
	mActionButton->mSounds = true;

	mPuzzleButton = new PodButton(2, this);
	mPuzzleButton->mGlowColor = Color(255, 255, 0);
	mPuzzleButton->mDoFinger = true;
	mPuzzleButton->mSelectedGlowImage = IMAGE_MAIN_BIG_SELECT;
	mPuzzleButton->mGlowImage = IMAGE_MAIN_BIG_OVER;
	mPuzzleButton->mButtonImage = IMAGE_MAIN_PUZZLE;
	mPuzzleButton->mSounds = true;

	mEndlessButton = new PodButton(3, this);
	mEndlessButton->mSelectedGlowImage = IMAGE_MAIN_BIG_SELECT;
	mEndlessButton->mGlowColor = Color(255, 255, 0);
	mEndlessButton->mDoFinger = true;
	mEndlessButton->mGlowImage = IMAGE_MAIN_BIG_OVER;
	mEndlessButton->mButtonImage = IMAGE_MAIN_ENDLESS;
	mEndlessButton->mSounds = true;

	// the secret button
	mSecretButton = new PodButton(4, this);
	mSecretButton->mGlowColor = Color(255, 192, 0);
	mSecretButton->mDoFinger = true;
	mSecretButton->mSelectedGlowImage = IMAGE_MAIN_SECRET_GLOW;
	mSecretButton->mGlowImage = IMAGE_MAIN_SECRET_GLOW;
	mSecretButton->mButtonImage = IMAGE_MAIN_SECRET_SYMBOL;
	mSecretButton->mSelectedGlow = 1.0f;
	mSecretButton->mPressedGlow = 0.5f;
	mSecretButton->mHoverGlow = 0.25f;

	// the buttons along the bottom
	mOptionsButton = new PodButton(10, this);
	mOptionsButton->mGlowColor = Color(255, 255, 255);
	mOptionsButton->mDoFinger = true;
	mOptionsButton->mGlowImage = IMAGE_MAIN_BOTTOM_GLOW;
	mOptionsButton->mButtonImage = IMAGE_MAIN_OPTIONS;

	mScoresButton = new PodButton(11, this);
	mScoresButton->mGlowColor = Color(255, 255, 255);
	mScoresButton->mDoFinger = true;
	mScoresButton->mGlowImage = IMAGE_MAIN_BOTTOM_GLOW;
	mScoresButton->mButtonImage = IMAGE_MAIN_SCORES;

	// (the original's "more games" button, id 12, is left out)

	mQuitButton = new PodButton(13, this);
	mQuitButton->mGlowColor = Color(255, 255, 255);
	mQuitButton->mDoFinger = true;
	mQuitButton->mGlowImage = IMAGE_MAIN_BOTTOM_GLOW;
	mQuitButton->mButtonImage = IMAGE_MAIN_QUIT;

	mSelectedMode = -1;
	SelectMode(mApp->mGameMode);
	mHoverCircle = -1;
	mHoverFade = 0;
	mLightFade = 0;
	mLightGlow[0] = 0;
	mLightGlow[1] = 0;
	mShutdownTimer = 0;
	mLightGlow[2] = 0;
	mState = STATE_READY;
	mLightPhase = 0;
	mHoverCircleCount = 0;
	mSlideDelay = 0;
	mStarting = false;
	mSlide.SetMode(0);
	mSlide.SetRamp(2);
	mSlide.SetOutRange(0, 700.0);
	for (int i = 0; i < 4; i++)
	{
		mModeGlow[i] = 0;
		mModeGlow2[i] = 0;
	}

	mTextImage = new MemoryImage();
	mTextImage->Create(0xf9, 0x40);
	mTextImage->SetImageMode(true, true);
	mUserDialogTimer = 0;
	mWelcomeTimer = 0;
	mStartTimer = 0;
	mPrevFont = NULL;
	mSmallFont = NULL;
	SyncProfile();
	ResizeComponents();
}

// 0x5c7ffa (the original leaves the four buttons along the bottom to the application: they are freed here)
MainMenu::~MainMenu()
{
	delete mTextImage;
	delete mBackdropImage;
	delete mUserLink;
	delete mClassicButton;
	delete mActionButton;
	delete mPuzzleButton;
	delete mEndlessButton;
	delete mSecretButton;
	delete mOptionsButton;
	delete mScoresButton;
	delete mQuitButton;
	if (mPrevSound != NULL)
		mPrevSound->Release();
	if (mSound != NULL)
		mSound->Release();
	delete mSmallFont;
}

// 0x5c8aa9: the menu slides in from 1000 (of 1024) pixels below its place (the constructor's 700 is only its first value)
void MainMenu::Show()
{
	mSlideDelay = 13;
	mSlide.SetInVal(0);
	mSlide.SetOutRange(1000.0, 0);
	mSlide.SetRamp(2);
	mState = STATE_SLIDING_IN;
	ResizeComponents();
}

// 0x5c80e1: and out to 1000, so that all of it has left the screen
void MainMenu::Hide()
{
	mSlide.SetInVal(0);
	mSlide.SetOutRange(0, 1000.0);
	mSlide.SetRamp(2);
	mState = STATE_SLIDING_OUT;
}

// 0x5c8128
void MainMenu::SetBackground(MemoryImage* theImage)
{
	delete mBackdropImage;
	mBackdropImage = theImage;
}

// 0x5c8148
void MainMenu::SelectMode(int theMode)
{
	mSelectedMode = theMode;
	mClassicButton->mSelected = (theMode == 0 || theMode == 4 || theMode == 8);
	mActionButton->mSelected = (theMode == 1 || theMode == 5);
	mPuzzleButton->mSelected = (theMode == 2 || theMode == 6);
	mEndlessButton->mSelected = (theMode == 3 || theMode == 7);
	MarkDirty();
}

// 0x5c81e7
void MainMenu::StartGame()
{
	if (mState == STATE_READY)
	{
		mState = STATE_STARTING;
		mStarting = true;
		mClassicButton->mSounds = false;
		mActionButton->mSounds = false;
		mPuzzleButton->mSounds = false;
		mEndlessButton->mSounds = false;
		mApp->mGameMode = mSelectedMode;
		mApp->NewGame();
	}
}

// 0x5c824d
void MainMenu::StartShutdownTimer()
{
	mShutdownTimer = 120;
	mClassicButton->mSounds = false;
	mActionButton->mSounds = false;
	mPuzzleButton->mSounds = false;
	mEndlessButton->mSounds = false;
}

// 0x5c828a
void MainMenu::CancelNewGame()
{
	mClassicButton->mSounds = true;
	mActionButton->mSounds = true;
	mPuzzleButton->mSounds = true;
	mEndlessButton->mSounds = true;
	mStarting = false;
	mState = STATE_READY;
}

// 0x5c82d0
void MainMenu::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	theManager->AddWidget(mUserLink);
	theManager->AddWidget(mClassicButton);
	theManager->AddWidget(mActionButton);
	theManager->AddWidget(mPuzzleButton);
	theManager->AddWidget(mEndlessButton);
	if (mSecretShown)
		theManager->AddWidget(mSecretButton);
	theManager->AddWidget(mOptionsButton);
	theManager->AddWidget(mScoresButton);
	theManager->AddWidget(mQuitButton);
}

// 0x5c8371
void MainMenu::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	theManager->RemoveWidget(mUserLink);
	theManager->RemoveWidget(mClassicButton);
	theManager->RemoveWidget(mActionButton);
	theManager->RemoveWidget(mPuzzleButton);
	theManager->RemoveWidget(mEndlessButton);
	theManager->RemoveWidget(mSecretButton);
	theManager->RemoveWidget(mOptionsButton);
	theManager->RemoveWidget(mScoresButton);
	theManager->RemoveWidget(mQuitButton);
}

// 0x5c8409
void MainMenu::ResizeComponents()
{
	int anOffset = (int)mApp->ScaleNum(mSlide.GetOutVal(), 0.0);

	mUserLink->Resize(mApp->ScaleNum(0x19f, 0), mApp->ScaleNum(0x58, -4) + anOffset, mApp->ScaleNumUp(0xc5, 0), mApp->ScaleNum(0x1a, 5));
	mClassicButton->Resize(mApp->ScaleNum(0xa1, 0), mApp->ScaleNum(0x43, 0) + anOffset, mApp->ScaleNumUp(0xce, 0), mApp->ScaleNumUp(0xcf, 0));
	mActionButton->Resize(mApp->ScaleNum(0x294, 0), mApp->ScaleNum(0x43, 0) + anOffset, mApp->ScaleNumUp(0xce, 0), mApp->ScaleNumUp(0xcf, 0));
	mPuzzleButton->Resize(mApp->ScaleNum(0xa1, 0), mApp->ScaleNum(0x196, 0) + anOffset, mApp->ScaleNumUp(0xce, 0), mApp->ScaleNumUp(0xcf, 0));
	mEndlessButton->Resize(mApp->ScaleNum(0x294, 0), mApp->ScaleNum(0x195, 0) + anOffset, mApp->ScaleNumUp(0xce, 0), mApp->ScaleNumUp(0xcf, 0));
	if (mSecretShown)
		mSecretButton->Resize(mApp->ScaleNum(0x1c2, 0), mApp->ScaleNum(0x244, 0) + anOffset, mApp->ScaleNumUp(0x88, 0), mApp->ScaleNumUp(0x58, 0));
	mOptionsButton->Resize(mApp->ScaleNum(0x2e, 0), mApp->ScaleNum(0x28f, 0) + anOffset, mApp->ScaleNumUp(0xa0, 0), mApp->ScaleNumUp(0x3c, 0));
	mScoresButton->Resize(mApp->ScaleNum(0x103, 0), mApp->ScaleNum(0x2b1, 0) + anOffset, mApp->ScaleNumUp(0xa0, 0), mApp->ScaleNumUp(0x3c, 0));
	mQuitButton->Resize(mApp->ScaleNum(0x338, 0), mApp->ScaleNum(0x28f, 0) + anOffset, mApp->ScaleNumUp(0xa0, 0), mApp->ScaleNumUp(0x3c, 0));
}

// 0x5c8800: the pictures of the mode buttons: the secret button swaps in the pictures of the unlocked modes
void MainMenu::RehupGameButtons()
{
	bool aSecret = mSecretButton->mSelected;
	mClassicButton->mButtonImage = (aSecret && mApp->mProfile->IsModeUnlocked(4)) ? IMAGE_MAIN_TWILIGHT : IMAGE_MAIN_CLASSIC;
	mActionButton->mButtonImage = (aSecret && mApp->mProfile->IsModeUnlocked(5)) ? IMAGE_MAIN_HYPER : IMAGE_MAIN_ACTION;
	mPuzzleButton->mButtonImage = (aSecret && mApp->mProfile->IsModeUnlocked(6)) ? IMAGE_MAIN_COGNITO : IMAGE_MAIN_PUZZLE;
	mEndlessButton->mButtonImage = (aSecret && mApp->mProfile->IsModeUnlocked(7)) ? IMAGE_MAIN_FINITY : IMAGE_MAIN_ENDLESS;
}

// 0x5c88fe
void MainMenu::ButtonPress(int theId)
{
	if (mState == STATE_READY)
	{
		if (theId < 4)
			mApp->PlaySample(SOUND_MAIN_MOUSE_CLICK);
		else if (theId != 4)
			mApp->PlaySample(SOUND_CLICK);
	}
}

// 0x5c8933
void MainMenu::ButtonDepress(int theId)
{
	if (mShutdownTimer > 0)
		return;

	if (theId == 11)
	{
		if (!mStarting)
			mApp->DoHighScoreWidget();
	}
	else if (theId >= 0 && theId < 4)
	{
		if (mState != STATE_NO_BUTTONS)
		{
			if (mSecretButton->mSelected && mApp->mProfile->IsModeUnlocked(theId + 4))
				theId += 4;
			SelectMode(theId);
			if (mUserDialogTimer == 0)
			{
				// without 3D acceleration a few updates pass first, to draw the pressed button
				if (!mApp->Is3DAccelerated())
					mStartTimer = 10;
				else
					StartGame();
			}
		}
	}
	else if (theId == 4)
	{
		mSecretButton->mSelected = !mSecretButton->mSelected;
		if (mSecretButton->mSelected)
			mApp->PlaySample(SOUND_FAIRY_DUST);
		RehupGameButtons();
		mSecretButton->MarkDirty();
	}
	else if (theId == 10)
	{
		if (!mStarting)
			mApp->DoOptions();
	}
	else if (theId == 13)
	{
		if (!mStarting)
			mApp->ConfirmQuit();
	}
	else if (theId == 0x14)
	{
		if (!mStarting)
			mApp->DoUserDialog();
	}
}

// 0x5c8afd: the glow behind the middle of the gadgets while the mouse is over a mode button, drawn one row at a time
// (theGlow is how lit the button is, 0 to 1; theGlow2 how long it has been)
void MainMenu::DrawMouseover(Graphics* g, int theX, int theY, int theIndex, float theGlow, float theGlow2)
{
	double aSum = 0.0;
	int aHeight = mApp->ScaleNumUp(0x50, 0);
	double aGlowD = (double)theGlow;
	double aHeightD = (double)aHeight;
	int aCount = (int)(sin(aGlowD * 3.142 * 0.5) * aHeightD);
	float aUpdateCnt = (float)mUpdateCnt;
	int aStartY = (aHeight - aCount) / 2 + theY;
	double aStep1 = aUpdateCnt * 0.03;
	double aStep2 = aUpdateCnt * 0.04;
	double aStep3 = aUpdateCnt * 0.035;

	g->SetColorizeImages(true);
	if (aCount > 0)
	{
		double aDelta = aHeightD / (double)aCount;
		double aInv = 1.0 - (double)theGlow2;
		double aBase = (1.0 - aGlowD) * (1.0 - aGlowD) * 0.5;
		for (int j = 0; j < aCount; j++)
		{
			aSum = (cos((double)j * 0.1 + aStep3) * theGlow2 * 0.125 + 2.0) * aDelta + aSum;
			double aCos = cos(aSum * 0.4 + aStep1) + 1.0;
			double anAlphaF = (aCos * 0.5 * 0.25 + 0.75) * theGlow2 + aInv;
			double aSin = sin(0.5 * (0.25 * aSum + aStep2));
			int aWidth = (int)(((aSin * 0.02 + aBase) * theGlow2 + 1.0) * 290.0);
			aWidth &= ~1;
			int anAlpha = (int)(anAlphaF * 255.0 * theGlow * theGlow);
			g->SetColor(Color(255, 255, 255, anAlpha));

			if ((double)mApp->ScaleNum(0xa0, 0) > aSum)
			{
				int aSrcW = mApp->ScaleNum(0x244, 0);
				int aSrcY = mApp->ScaleNum(theIndex * 0xa0, 0) + (int)aSum;
				Rect aSrc(0, aSrcY, aSrcW, 1);
				int aDestW = mApp->ScaleNum(aWidth, 0);
				int aDestX = mApp->ScaleNum(0x122 - aWidth, 0) / 2 + theX;
				Rect aDest(aDestX, aStartY + j, aDestW, 1);
				g->DrawImage(IMAGE_MAIN_CENTER_TEXT, aDest, aSrc);
			}
		}
	}
	g->SetColorizeImages(false);
}

// 0x5c9572: the user's name (smaller when it is long) and the greeting under it
void MainMenu::DrawTopText(Graphics* g)
{
	Font* anOldFont = g->GetFont();
	Profile* aProfile = mApp->mProfile;

	if (FONT_CONTINUUM_MED_18->StringWidth(aProfile->mName) >= mApp->ScaleNum(0xd0, 0))
	{
		if (mPrevFont != g->GetFont())
		{
			delete mSmallFont;
			mSmallFont = g->GetFont()->Duplicate();
			static_cast<ImageFont*>(mSmallFont)->mScale *= 0.85;
			g->SetFont(mSmallFont);
		}
	}

	if (aProfile->mName.empty())
	{
		std::string aText = "Welcome!";
		int aWidth = g->GetFont()->StringWidth(aText);
		g->DrawString(aText, mApp->ScaleNum(0x201, 0) - aWidth / 2, mApp->ScaleNum(0x52, 0));
	}
	else
	{
		std::string aText = aProfile->mName + ",";
		int aWidth = g->GetFont()->StringWidth(aText);
		g->DrawString(aText, mApp->ScaleNum(0x201, 0) - aWidth / 2, mApp->ScaleNum(0x3c, 0));
		g->SetFont(anOldFont);

		aText = aProfile->mIsNew ? "Welcome!" : "Welcome back.";
		aWidth = g->GetFont()->StringWidth(aText);
		g->DrawString(aText, mApp->ScaleNum(0x201, 0) - aWidth / 2, mApp->ScaleNum(0x55, 0));
	}
}

// 0x5c9783: the greeting in the glow font, made into a picture that Draw puts behind the real text
void MainMenu::CreateFontGlowImages()
{
	mTextImage->Clear();
	Graphics g(mTextImage);
	g.SetColor(Color(255, 255, 255));
	g.SetFont(FONT_CONTINUUM_MED_18_GLOW);
	g.Translate(-0x185, -0x1e);
	DrawTopText(&g);
}

// 0x5c981c
void MainMenu::SyncProfile()
{
	mUserLink->SetVisible(!mApp->mProfile->mName.empty());
	CreateFontGlowImages();
	mSecretButton->mSelected = false;
	mSecretButton->MarkDirty();
	bool a7 = mApp->mProfile->IsModeUnlocked(7);
	bool a6 = mApp->mProfile->IsModeUnlocked(6);
	bool a5 = mApp->mProfile->IsModeUnlocked(5);
	bool a4 = mApp->mProfile->IsModeUnlocked(4);
	mSecretShown = a4 || a5 || a7 || a6;
	RehupGameButtons();
	if (mWidgetManager != NULL)
	{
		mWidgetManager->RemoveWidget(mSecretButton);
		if (mSecretShown)
			mWidgetManager->AddWidget(mSecretButton);
	}
}

// 0x5c8d7f
void MainMenu::Update()
{
	if (!mApp->mHasFocus)
		return;

	if (mStartTimer > 0 && --mStartTimer == 0)
		StartGame();

	if (mUserDialogTimer > 0 && --mUserDialogTimer == 0)
		mApp->DoCreateUserDialog();

	if (mWelcomeTimer > 0 && --mWelcomeTimer == 0)
	{
		Dialog* aDialog;
		int aHeaderWidth;
		if (!mApp->mIsDiamondMine)
		{
			aDialog = mApp->DoDialog(DIALOG_WELCOME, true, "Welcome to Bejeweled 2!",
				"If you have never played Bejeweled 2 you may want to start off playing Classic mode before trying the other game modes.", "OK", 3);
			aHeaderWidth = 0x17c;
		}
		else
		{
			aDialog = mApp->DoDialog(DIALOG_WELCOME, true, "Welcome to Diamond Mine 2!",
				"If you have never played Diamond Mine 2 you may want to start off playing Classic mode before trying the other game modes.", "OK", 3);
			aHeaderWidth = 0x1a4;
		}
		static_cast<PodDialog*>(aDialog)->mHeaderWidth = mApp->ScaleNum(aHeaderWidth, 0);
		int aWidth = mApp->ScaleNum(0x20, 0) + aDialog->mWidth;
		int aHeight = aDialog->GetPreferredHeight(aWidth);
		aDialog->Resize(aDialog->mX - mApp->ScaleNum(0x10, 0), mApp->ScaleNum(8, 0) + aDialog->mY, aWidth, aHeight);
	}

	float aTarget = 0;
	Widget::Update();

	// the mouse going around the four mode buttons in order, 32 times in a row, starts the hidden mode
	int aHovered;
	if (mClassicButton->mIsOver)
		aHovered = 0;
	else if (mActionButton->mIsOver)
		aHovered = 1;
	else if (mEndlessButton->mIsOver)
		aHovered = 2;
	else
		aHovered = mPuzzleButton->mIsOver ? 3 : -1;
	if (aHovered != mHoverCircle && aHovered != -1)
	{
		if (aHovered == (mHoverCircle + 1) % 4)
			mHoverCircleCount++;
		else
			mHoverCircleCount = 0;
		if (mHoverCircleCount == 0x20)
		{
			SelectMode(8);
			StartGame();
		}
		mHoverCircle = aHovered;
	}

	// the glow behind the gadgets
	int aState = mState;
	if (aState == STATE_STARTING ||
		(aState == STATE_READY &&
			((mClassicButton->mSelected && mClassicButton->mIsOver) || (mActionButton->mSelected && mActionButton->mIsOver) ||
			 (mPuzzleButton->mSelected && mPuzzleButton->mIsOver) || (mEndlessButton->mSelected && mEndlessButton->mIsOver))))
	{
		mHoverFade = mHoverFade + 0.01;
		if (mHoverFade > 1.0f)
			mHoverFade = 1.0f;
		mLightFade = mLightFade + 0.03;
		if (mLightFade > 1.0f)
			mLightFade = 1.0f;
	}
	else
	{
		mHoverFade = mHoverFade - 0.02;
		if (mHoverFade < 0.0f)
			mHoverFade = 0.0f;
		mLightFade = mLightFade - 0.02;
		if (mLightFade < 0.0f)
			mLightFade = 0.0f;
	}

	// the three pairs of lights run one after the other, with a sound
	if (mLightFade <= 0.0f)
		mLightPhase = 0.0f;
	else
	{
		if ((aState == STATE_READY || aState == STATE_STARTING) && mLightPhase == 0.0f)
		{
			if (mPrevSound != NULL)
				mPrevSound->Release();
			mPrevSound = mSound;
			mSound = mApp->mSoundManager->GetSoundInstance(SOUND_MAIN_LANDING_LIGHTS);
			if (mSound != NULL)
			{
				if (mHoverFade < 0.5f)
					mHoverFade = 0.5f;
				mSound->SetVolume(mHoverFade);
				mSound->Play(false, false);
			}
		}
		mLightPhase = mLightPhase + 0.07f;
		float aMax = (mLightPhase < 2.0f || 5.0f <= mLightPhase) ? 0.75f : 1.0f;
		aTarget = aMax;
		for (int i = 0; i < 3; i++)
		{
			float aGlow;
			if ((int)mLightPhase == i)
			{
				aGlow = mLightGlow[i] + 0.05f;
				if (1.0f <= aGlow)
					aGlow = 1.0f;
			}
			else
			{
				aGlow = mLightGlow[i] - 0.0075f;
				if (aGlow <= 0.0f)
					aGlow = 0.0f;
			}
			mLightGlow[i] = aGlow;
		}
		if (8.0f < mLightPhase)
			mLightPhase = 0.0f;
	}
	if (mHoverFade > 0.1f && mSound != NULL)
		mSound->SetVolume(mHoverFade);

	// the menu slides
	if ((mState == STATE_SLIDING_IN && (mApp->GetDialogCount() == 0 || mApp->GetDialog(DIALOG_HELP) != NULL)) ||
		mState == STATE_SLIDING_OUT)
	{
		if (mSlideDelay < 1)
		{
			bool aSlideDone = !mSlide.IncInVal(0.01);
			if (aSlideDone)
			{
				if (mState == STATE_SLIDING_IN)
				{
					mState = STATE_READY;
					if (mApp->mBoard != NULL)
					{
						// the menu takes over the picture of the game that ends
						mBackdropClear = false;
						SetBackground(mApp->mBoard->mBackdropImage);
						mApp->mBoard->mBackdropImage = NULL;
						mApp->KillBoard();
					}
				}
				else
					mState = STATE_HIDDEN;
			}
		}
		else
			mSlideDelay--;
	}
	ResizeComponents();

	if (mStartTimer == 0)
	{
		PodButton* aButtons[4] = {mClassicButton, mActionButton, mPuzzleButton, mEndlessButton};
		for (int i = 0; i < 4; i++)
		{
			if (mSecretButton->mSelected && mApp->mProfile->IsModeUnlocked(i + 4))
				continue;
			if (!aButtons[i]->mIsOver || mState != STATE_READY)
				mModeGlow[i] = mModeGlow[i] - 0.02;
			else
				mModeGlow[i] = mModeGlow[i] + 0.02;
		}

		for (int i = 0; i < 4; i++)
		{
			if (mModeGlow[i] <= 1.0f)
			{
				if (mModeGlow[i] < 0.0f)
				{
					mModeGlow[i] = 0.0f;
					mModeGlow2[i] = 0.0f;
				}
				if (0.5f < mModeGlow[i])
				{
					if (aTarget <= 0.5f)
						aTarget = 0.5f;
				}
			}
			else
			{
				mModeGlow[i] = 1.0f;
				mModeGlow2[i] = mModeGlow2[i] + 0.005;
				if (1.0f < mModeGlow2[i])
					mModeGlow2[i] = 1.0f;
				if (aTarget <= 0.5f)
					aTarget = 0.5f;
			}
		}

		if (aTarget <= mCenterGlow)
		{
			mCenterGlow = mCenterGlow - 0.0075;
			if (mCenterGlow < aTarget)
				mCenterGlow = aTarget;
		}
		else
		{
			mCenterGlow = mCenterGlow + 0.01;
			if (aTarget < mCenterGlow)
				mCenterGlow = aTarget;
		}
	}

	if (mShutdownTimer > 0 && --mShutdownTimer == 0)
		mApp->Shutdown();

	MarkDirty();
}

// 0x5c98f5
void MainMenu::Draw(Graphics* g)
{
	bool is3d = mApp->Is3DAccelerated();
	if (mApp->mBoard != NULL && mApp->mBoard->mEffectOverlay->mPortalOpen && !is3d)
		return;

	int anOffset = (int)mApp->ScaleNum(mSlide.GetOutVal(), 0.0);
	if (!mBackdropClear && mBackdropImage != NULL)
		g->DrawImage(mBackdropImage, 0, 0);
	if (is3d)
	{
		// the 3D game's picture of the gadgets holds the pill of the "More Games" button, which the port leaves out (the button's
		// label is the button's own picture), so the rectangle round the pill is the one part of it that is not drawn; the 2D game's
		// picture has no such pill
		int aCutLeft = mApp->ScaleNum(600, 0);
		int aCutRight = mApp->ScaleNum(788, 0);
		int aCutTop = mApp->ScaleNum(660, 0);
		int aWidth = IMAGE_MAIN_GADGETS->mWidth;
		int aHeight = IMAGE_MAIN_GADGETS->mHeight;
		g->DrawImage(IMAGE_MAIN_GADGETS, 0, anOffset, Rect(0, 0, aCutLeft, aHeight));
		g->DrawImage(IMAGE_MAIN_GADGETS, aCutRight, anOffset, Rect(aCutRight, 0, aWidth - aCutRight, aHeight));
		g->DrawImage(IMAGE_MAIN_GADGETS, aCutLeft, anOffset, Rect(aCutLeft, 0, aCutRight - aCutLeft, aCutTop));
	}
	else
		g->DrawImage(IMAGE_MAIN_GADGETS, 0, anOffset);
	if (mSecretShown)
		g->DrawImage(IMAGE_MAIN_SECRET_FRAME, mApp->ScaleNum(0x174, 0), mApp->ScaleNum(0x213, 0) + anOffset);

	// the three pairs of lights
	for (int i = 0; i < 3; i++)
	{
		float aGlow = mLightGlow[i] * mLightFade;
		if (aGlow > 0.0f)
		{
			g->SetColorizeImages(true);
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			int anAlpha = (int)(aGlow * 255.0);
			if (i == 0)
			{
				g->SetColor(Color(0x40, 0xb4, 0x40, anAlpha));
				g->DrawImage(IMAGE_MAIN_LIGHT_BIG, mApp->ScaleNum(0x7b, 0), mApp->ScaleNum(0x129, 0));
				g->DrawImage(IMAGE_MAIN_LIGHT_BIG, mApp->ScaleNum(0x32d, 0), mApp->ScaleNum(0x129, 0));
			}
			else if (i == 1)
			{
				g->SetColor(Color(0x96, 0xdc, 0x40, anAlpha));
				g->DrawImage(IMAGE_MAIN_LIGHT_MED, mApp->ScaleNum(0xe1, 0), mApp->ScaleNum(0x132, 0));
				g->DrawImage(IMAGE_MAIN_LIGHT_MED, mApp->ScaleNum(0x2d7, 0), mApp->ScaleNum(0x132, 0));
			}
			else
			{
				g->SetColor(Color(0xb4, 0xf0, 0x20, anAlpha));
				g->DrawImage(IMAGE_MAIN_LIGHT_SMALL, mApp->ScaleNum(0x132, 0), mApp->ScaleNum(0x140, 0));
				g->DrawImage(IMAGE_MAIN_LIGHT_SMALL, mApp->ScaleNum(0x2a1, 0), mApp->ScaleNum(0x140, 0));
			}
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g->SetColorizeImages(false);
		}
	}

	// the greeting: the glow picture behind the text
	g->SetFont(FONT_CONTINUUM_MED_18);
	g->Translate(0, anOffset);
	g->SetColorizeImages(true);
	g->SetColor(Color(0x78, 0x78, 0xff, 0xdc));
	g->DrawImage(mTextImage, mApp->ScaleNum(0x185, 0), mApp->ScaleNum(0x1e, 0));
	g->SetColorizeImages(false);
	g->SetColor(Color(0xf2, 0xd4, 0xff));
	DrawTopText(g);
	g->Translate(0, -anOffset);

	// the top row of the picture: it is drawn over the strip above the gadgets
	if (!mBackdropClear)
	{
		Rect aSrc(0, 0, mApp->ScaleNum(0x400, 0), 1);
		g->DrawImage(mBackdropImage != NULL ? mBackdropImage : IMAGE_MAIN_BKG, 0, 0, aSrc);
	}

	for (int i = 0; i < 4; i++)
	{
		float aGlow = (mModeGlow[i] - 0.5f) + (mModeGlow[i] - 0.5f);
		if (aGlow > 0.0f)
			DrawMouseover(g, mApp->ScaleNum(0x16f, 0), mApp->ScaleNum(0x132, 0), i, aGlow, mModeGlow2[i]);
	}

	if (mCenterGlow > 0.0f)
	{
		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		int anAlpha = (int)(mCenterGlow * 255.0);
		g->SetColor(Color(255, 255, 255, anAlpha));
		g->DrawImage(IMAGE_MAIN_CENTER_GLOW, mApp->ScaleNum(0x155, 0), mApp->ScaleNum(0xaa, 0) + anOffset);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}
}
