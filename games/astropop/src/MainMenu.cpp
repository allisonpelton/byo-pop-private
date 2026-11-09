#include "MainMenu.h"
#include "AstroButton.h"
#include "AstroPopApp.h"
#include "Graphics.h"
#include "GameText.h"
#include "Image.h"
#include "MemoryImage.h"
#include "ShipsScreen.h"
#include "WidgetManager.h"
#include "Res.h"
#include <cstdlib>

using namespace Sexy;

namespace
{
	// The rectangles of the five button hit areas and the user button (static data at 0x59da98 / 0x59daf8): x, y, w, h
	const int kButtonRects[MainMenu::NUM_BUTTONS][4] = {
		{0,   0x73,  0xdb, 0x9f},
		{0,   0x111, 0xda, 0x9e},
		{430, 0x73,  0xd2, 0x55},
		{430, 0xc8,  0xd2, 0x49},
		{430, 0x111, 0xd2, 0x4f},
		{430, 0x160, 0xd2, 0x5b}
	};
	const int kUserButtonRect[4] = {0xe3, 0x55, 0xc7, 0x11};

	// The sample each button plays on a hover and the pitch it is shifted by
	const int kHoverPitch[MainMenu::NUM_BUTTONS] = {7, 4, 9, 14, 13, 9};
}

// 0x454ee0
MainMenu::MainMenu(AstroPopApp* theApp)
{
	mParticleLevel = 0;
	mApp = theApp;
	mInactive = false;
	mGoScreen = NULL;
	mButtons.resize(NUM_BUTTONS, NULL);

	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		if (i == 4) // More Games: left out
			continue;

		AstroButton* aButton = new AstroButton(BUTTON_ID_BASE + i, this);
		aButton->mButtonImage = NULL;
		aButton->mOverImage = GetImageById(IMAGE_GS_ARCADE_OVER_ID + i);
		aButton->mAnimOver.SetNumFrames(aButton->mOverImage->mNumCols);
		aButton->mAnimOver.SetDelay(10);
		aButton->mDownImage = GetImageById(IMAGE_GS_ARCADE_DOWN_ID + i);
		aButton->mHasAlpha = true;
		aButton->mHasTransparencies = true;

		MemoryImage* aMask = (MemoryImage*) GetImageById(IMAGE_GS_ARCADE_MASK_ID + i);
		aButton->mHitMask = (int*) aMask->GetBits();
		aButton->mHitMaskWidth = aMask->GetWidth();

		aButton->Resize(Rect(kButtonRects[i][0], kButtonRects[i][1], kButtonRects[i][2], kButtonRects[i][3]));
		// the voice that names the button, shifted by the button's pitch (the table runs in the order of the buttons)
		static int* const kHoverSound[NUM_BUTTONS] = {&SOUND_CLASSIC_GAME, &SOUND_SURVIVAL_GAME, &SOUND_HALL_OF_FAME,
			&SOUND_OPTIONS, &SOUND_MORE_GAMES, &SOUND_QUIT_GAME};
		aButton->mSoundId = *kHoverSound[i];
		aButton->mSoundPitch = kHoverPitch[i];
		mButtons[i] = aButton;
	}

	mUserButton = new AstroButton(BUTTON_ID_USER, this);
	mUserButton->mButtonImage = IMAGE_GS_SELECT_USER_NORMAL;
	mUserButton->mOverImage = IMAGE_GS_SELECT_USER_OVER;
	mUserButton->Resize(Rect(kUserButtonRect[0], kUserButtonRect[1], kUserButtonRect[2], kUserButtonRect[3]));

	mParticleLevel = 0;
	mEasterPending = false;
	mHovering = false;
	for (int i = 0; i < NUM_PANELS; i++)
	{
		mSpeed[i] = (i != 0) ? -8 : 12;
		mAlpha[i] = (i != 0) ? 0 : 0xff;
	}
	mHoverStep = -5;
	mEasterTimer = 0;
}

// 0x4552b0 / 0x44e5c0
MainMenu::~MainMenu()
{
	for (size_t i = 0; i < mButtons.size(); i++)
		delete mButtons[i];
	for (size_t i = 0; i < mParticles.size(); i++)
		delete mParticles[i];
	if (mGoScreen != NULL)
	{
		mApp->mWidgetManager->RemoveWidget(mGoScreen);
		delete mGoScreen;
	}
	delete mUserButton;
}

// 0x43c230
void MainMenu::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		if (mButtons[i] == NULL)
			continue;
		theWidgetManager->AddWidget(mButtons[i]);
		theWidgetManager->BringToFront(mButtons[i]);
	}
	theWidgetManager->AddWidget(mUserButton);
	theWidgetManager->BringToFront(mUserButton);
}

// 0x43c290
void MainMenu::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		if (mButtons[i] != NULL)
			theWidgetManager->RemoveWidget(mButtons[i]);
	}
	theWidgetManager->RemoveWidget(mUserButton);
}

// 0x4461b0
void MainMenu::Draw(Graphics* g)
{
	std::string aName = GT(0x5649d4); // New User

	g->DrawImage(IMAGE_GAME_SELECTOR, 0, 0);
	g->SetColor(Color(255, 255, 255));
	g->SetFont(GetFontById(0x120));
	if (mApp->mUser != NULL)
		mApp->mUser->GetString(PROFILE_NAME, &aName);
	WriteString(g, aName, 0xaf, 0x3c, 0x122, 0, true, 0, -1);

	for (int i = 0; i < NUM_PANELS; i++)
	{
		if (mAlpha[i] > 0)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, mAlpha[i]));
			if (i == 0)
				g->DrawImage(IMAGE_GS_CENTER_PIECE, 0xdb, 0xb1);
			else
				g->DrawImage(GetImageById(i + 0xc6), 199, 0xe7);
			g->SetColorizeImages(false);
		}
	}

	if (mApp->Is3DAccelerated())
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		for (size_t i = 0; i < mParticles.size(); i++)
		{
			MenuParticle* aParticle = mParticles[i];
			g->SetColor(Color(255, 255, 255, aParticle->mAlpha));
			g->DrawImageCel(IMAGE_PARTICLE_RADIAL, aParticle->mX, aParticle->mY,
				(aParticle->mAnim.mNumFrames == 1) ? 0 : aParticle->mAnim.mFrame);
		}
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	g->DrawImage(IMAGE_GS_PROJECTOR, 0xd7, 0x1b3);
}

// 0x4552e0
void MainMenu::Update()
{
	Widget::Update();
	if (mInactive)
		return;

	for (int i = 0; i < NUM_PANELS; i++)
	{
		if (mSpeed[i] < 0)
		{
			if (mAlpha[i] > 0)
			{
				MarkDirty();
				int anAlpha = mAlpha[i];
				mAlpha[i] = anAlpha + mSpeed[i];
				if (anAlpha + mSpeed[i] < 1)
					mAlpha[i] = 0;
			}
		}
		else if (mAlpha[i] < 0xff)
		{
			MarkDirty();
			int anAlpha = mAlpha[i];
			mAlpha[i] = anAlpha + mSpeed[i];
			if (0xfe < anAlpha + mSpeed[i])
				mAlpha[i] = 0xff;
		}
	}

	// the particle level follows the hover
	if (!mHovering)
	{
		if (mParticleLevel > 0 && mUpdateCnt % 15 == 0)
			mParticleLevel--;
	}
	else if (mApp->Is3DAccelerated() && mParticleLevel <= 4 && mUpdateCnt % 15 == 0)
		mParticleLevel++;

	if (mApp->Is3DAccelerated())
	{
		if (mUpdateCnt % 3 == 0 && mParticleLevel >= 0)
		{
			int n = 0;
			do
			{
				MenuParticle* aParticle = new MenuParticle();
				int aCelWidth = IMAGE_PARTICLE_RADIAL->GetCelWidth();
				unsigned long aRand = Rand();
				aParticle->mX = ((int) aRand % 500000000) % (0xcd - aCelWidth) + 0xd7;
				aParticle->mY = 0x1b3;
				aParticle->mAlpha = 0xff;
				aParticle->mAnim.SetNumFrames(IMAGE_PARTICLE_RADIAL->mNumCols);
				aParticle->mAnim.SetDelay(13);
				aParticle->mAnim.mHoldLast = true;
				aParticle->mAnim.SetStopped(false);
				mParticles.push_back(aParticle);
				n++;
			} while (n <= mParticleLevel);
		}
	}

	for (size_t i = 0; i < mParticles.size(); i++)
	{
		MenuParticle* aParticle = mParticles[i];
		if (mUpdateCnt % 2 != 0)
		{
			float aSpeed = mHovering ? ((float) mParticleLevel * 0.5f + 1.0f) * 1.0f : 1.0f;
			if (aSpeed > 1.5f)
				aSpeed = 1.5f;
			aParticle->mY = (int) ((double) aParticle->mY - (double) aSpeed);
		}
		aParticle->mAlpha -= 2;
		aParticle->mAnim.Update();
		if (aParticle->mAlpha < 1 || aParticle->mAnim.mDone ||
			(aParticle->mAnim.mMaxLoops > 0 && aParticle->mAnim.mMaxLoops <= aParticle->mAnim.mLoopCount) ||
			aParticle->mAnim.mStopped)
		{
			delete aParticle;
			mParticles.erase(mParticles.begin() + i);
			i--;
		}
	}

	if (mEasterPending && ++mEasterTimer >= 50)
	{
		mEasterTimer = 0;
		mEasterPending = false;
		mApp->PlaySample(SOUND_SLOWDOWN);
		mGoScreen = new ShipsScreen(this);
		mGoScreen->Resize(0, 0, mWidth, mHeight);
		mApp->mWidgetManager->AddWidget(mGoScreen);
	}

	MarkDirty();
}

// 0x434f80: the screen of the ships goes and the menu has the focus again
void MainMenu::CloseGoScreen()
{
	mApp->mWidgetManager->RemoveWidget(mGoScreen);
	mApp->SafeDeleteWidget(mGoScreen);
	mGoScreen = NULL;
	mApp->mWidgetManager->SetFocus(this);
}

// 0x434f60
void MainMenu::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTON_DOWN);
}

// 0x434dc0
void MainMenu::ButtonMouseEnter(int theId)
{
	int aPanel = 4;
	if (theId == 0xfa9)
	{
		mHoverStep = (mHoverStep == 1) ? 2 : -5;
		aPanel = 1;
	}
	else if (theId == 0xfaa)
	{
		if (mHoverStep == 3)
			mHoverStep = 4;
		else
			mHoverStep = (mHoverStep == 4) ? 5 : -5;
		aPanel = 2;
	}
	else if (theId == 0xfab)
	{
		aPanel = 3;
		if (mHoverStep < 0)
			mHoverStep = 0;
		else if (mHoverStep == 0)
			mHoverStep = 1;
		else if (mHoverStep == 2)
			mHoverStep = 3;
		else
			mHoverStep = (mHoverStep == 5) ? 6 : -5;
	}
	else if (theId == 0xfac)
		mHoverStep = (mHoverStep == 6) ? 7 : -5;
	else if (theId == 0xfad)
	{
		aPanel = 5;
		mHoverStep = (mHoverStep == 7) ? 8 : -5;
	}
	else
	{
		if (theId != 0xfae)
			return;
		int aStep = mHoverStep;
		mHoverStep = -5;
		if (aStep == 8)
			mEasterPending = true;
		aPanel = 6;
	}

	for (int i = 0; i < NUM_PANELS; i++)
		mSpeed[i] = (i == aPanel) ? 12 : -8;
	mHovering = true;
}

// 0x434ef0
void MainMenu::ButtonMouseLeave(int theId)
{
	int aPanel;
	if (theId == 0xfa9)
		aPanel = 1;
	else if (theId == 0xfaa)
		aPanel = 2;
	else if (theId == 0xfab)
		aPanel = 3;
	else if (theId == 0xfac)
		aPanel = 4;
	else if (theId == 0xfad)
		aPanel = 5;
	else
	{
		if (theId != 0xfae)
			return;
		aPanel = 6;
	}
	mSpeed[aPanel] = -8;
	mSpeed[0] = 12;
	mHovering = false;
}

// 0x446410
void MainMenu::ButtonDepress(int theId)
{
	const SexyString* aPilots[4] = {&GT(0x564b64), &GT(0x564b58), &GT(0x564b50), &GT(0x564b48)};

	if (theId == BUTTON_ID_CLASSIC)
	{
		// a saved game (setting 0x4f holds its level, -1 for none) is offered before it is lost
		int aSavedLevel = 0;
		mApp->mUser->GetInt(0x4f, &aSavedLevel);
		if (aSavedLevel < 2)
		{
			mApp->mUser->ResetGame();
			mApp->StartGame(false);
		}
		else
		{
			int aPilot = 0;
			mApp->mUser->GetInt(0x53, &aPilot);
			if (aPilot < 0 || aPilot > 3)
				aPilot = 0;
			mApp->ShowContinueDialog(*aPilots[aPilot], aSavedLevel);
		}
	}
	else
	{
		if (theId == BUTTON_ID_SURVIVAL)
			mApp->ShowCharSelector(2);
		else
		{
			if (theId == BUTTON_ID_HALL_OF_FAME)
			{
				mApp->ShowHallOfFame();
				return;
			}
			if (theId == BUTTON_ID_QUIT)
				mApp->ShowQuitDialog();
			else if (theId == BUTTON_ID_USER)
				mApp->ShowUserSelect();
			else if (theId == BUTTON_ID_OPTIONS)
				mApp->ShowOptions(false);
			// BUTTON_ID_MORE_GAMES (0xfad): a link out of the game, left out
		}
		mApp->mWidgetManager->SetFocus(this);
	}
}
