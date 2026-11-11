#include "MainMenu.h"
#include "AstroButton.h"
#include "AstroPopApp.h"
#include "Graphics.h"
#include "GameText.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Particle.h"
#include "ShipsScreen.h"
#include "WidgetManager.h"
#include "Res.h"
#include <cstdlib>
#include <cstring>

using namespace Sexy;

namespace
{
	// The right-hand column of panels in the picture of the menu: the original stacks four (Hall of Fame, Options, More
	// Games, Quit Game) from y 115 to y 443. The port has no More Games button, a link out of the game, so the three
	// that remain are stretched to fill the column: the picture of each panel, its hover and press pictures and its hit mask.
	const int kColumnX = 430;
	const int kColumnY = 0x73;
	const int kColumnWidth = 0xd2;
	const int kColumnHeight = 328;

	// One of the five buttons
	struct ButtonSpec
	{
		int		mId;                    // the button's id
		int		mResource;              // its number among the original's six (the ids of its pictures add it: More Games was 4)
		int		mRect[4];               // its hit area in the menu (static data at 0x59da98): x, y, w, h; for the panels of the column the stretched one
		int		mSourceY;               // a panel of the column: its rows in the original's picture of the menu (0 for the others)
		int		mSourceHeight;
		int		mPitch;                 // the pitch the sample that names it is shifted by
		int*	mSound;                 // the sample that names it, played on a hover
	};

	const ButtonSpec kButtons[MainMenu::NUM_BUTTONS] = {
		{MainMenu::BUTTON_ID_CLASSIC,      0, {0,   0x73,  0xdb, 0x9f}, 0,     0,  7,  &SOUND_CLASSIC_GAME},
		{MainMenu::BUTTON_ID_SURVIVAL,     1, {0,   0x111, 0xda, 0x9e}, 0,     0,  4,  &SOUND_SURVIVAL_GAME},
		{MainMenu::BUTTON_ID_HALL_OF_FAME, 2, {430, 0x73,  0xd2, 112},  0x73,  85, 9,  &SOUND_HALL_OF_FAME},
		{MainMenu::BUTTON_ID_OPTIONS,      3, {430, 227,   0xd2, 96},   0xc8,  73, 14, &SOUND_OPTIONS},
		{MainMenu::BUTTON_ID_QUIT,         5, {430, 323,   0xd2, 120},  0x160, 91, 9,  &SOUND_QUIT_GAME}
	};
	const int kUserButtonRect[4] = {0xe3, 0x55, 0xc7, 0x11};

	// The picture that shows what a panel stands for, by the panel (0 is the centre piece, which has its own)
	const int kPanelImage[MainMenu::NUM_PANELS] = {-1, IMAGE_GS_MOUSEOVER_ARCADE_ID, IMAGE_GS_MOUSEOVER_SURVIVAL_ID,
		IMAGE_GS_MOUSEOVER_HALL_ID, IMAGE_GS_MOUSEOVER_OPTIONS_ID, IMAGE_GS_MOUSEOVER_QUIT_ID};

	// Rows theSrcY .. theSrcY + theSrcH - 1 of the pixels at theSrc (theSrcX on, theW across), stretched to theDestH rows
	// at theDest: each channel of a new row is the linear mix of the two rows of the picture it lies between
	void StretchRows(const ulong* theSrc, int theSrcPitch, int theSrcX, int theSrcY, int theSrcH, int theW, ulong* theDest,
					 int theDestPitch, int theDestH)
	{
		for (int y = 0; y < theDestH; y++)
		{
			float aPos = ((float) y + 0.5f) * (float) theSrcH / (float) theDestH - 0.5f;
			if (aPos < 0.0f)
				aPos = 0.0f;
			if (aPos > (float) (theSrcH - 1))
				aPos = (float) (theSrcH - 1);
			int aRow = (int) aPos;
			int aNext = (aRow + 1 < theSrcH) ? aRow + 1 : aRow;
			int aWeight = (int) ((aPos - (float) aRow) * 256.0f);
			const ulong* aUpper = theSrc + (theSrcY + aRow) * theSrcPitch + theSrcX;
			const ulong* aLower = theSrc + (theSrcY + aNext) * theSrcPitch + theSrcX;
			ulong* aOut = theDest + y * theDestPitch;
			for (int x = 0; x < theW; x++)
			{
				ulong aPixel = 0;
				for (int aShift = 0; aShift < 32; aShift += 8)
				{
					int anUpper = (int) ((aUpper[x] >> aShift) & 0xff);
					int aLower2 = (int) ((aLower[x] >> aShift) & 0xff);
					aPixel |= (ulong) ((anUpper * (256 - aWeight) + aLower2 * aWeight + 128) >> 8) << aShift;
				}
				aOut[x] = aPixel;
			}
		}
	}

	// A picture of a panel (the whole of it) stretched to theHeight rows
	MemoryImage* StretchedCopy(AstroPopApp* theApp, Image* thePicture, int theHeight)
	{
		MemoryImage* aSource = (MemoryImage*) thePicture;
		MemoryImage* aCopy = new MemoryImage(theApp);
		aCopy->Create(aSource->GetWidth(), theHeight);
		StretchRows(aSource->GetBits(), aSource->GetWidth(), 0, 0, aSource->GetHeight(), aSource->GetWidth(), aCopy->GetBits(),
					aCopy->GetWidth(), theHeight);
		aCopy->BitsChanged();
		return aCopy;
	}
}

// 0x454ee0
MainMenu::MainMenu(AstroPopApp* theApp)
{
	mParticleLevel = 0;
	mApp = theApp;
	mInactive = false;
	mGoScreen = NULL;
	mButtons.resize(NUM_BUTTONS, NULL);

	// the picture of the column: the stretched panels, in the place of the four of the original's picture of the menu
	mColumn = new MemoryImage(theApp);
	mColumn->Create(kColumnWidth, kColumnHeight);
	MemoryImage* aSelector = (MemoryImage*) IMAGE_GAME_SELECTOR;

	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		const ButtonSpec& aSpec = kButtons[i];
		bool isStretched = aSpec.mSourceHeight != 0;
		int aRectHeight = aSpec.mRect[3];

		AstroButton* aButton = new AstroButton(aSpec.mId, this);
		aButton->mButtonImage = NULL;
		aButton->mOverImage = GetImageById(IMAGE_GS_ARCADE_OVER_ID + aSpec.mResource);
		aButton->mDownImage = GetImageById(IMAGE_GS_ARCADE_DOWN_ID + aSpec.mResource);
		MemoryImage* aMask = (MemoryImage*) GetImageById(IMAGE_GS_ARCADE_MASK_ID + aSpec.mResource);
		aButton->mHitMask = (int*) aMask->GetBits();
		aButton->mHitMaskWidth = aMask->GetWidth();

		if (isStretched)
		{
			StretchRows(aSelector->GetBits(), aSelector->GetWidth(), kColumnX, aSpec.mSourceY, aSpec.mSourceHeight,
						kColumnWidth, mColumn->GetBits() + (aSpec.mRect[1] - kColumnY) * kColumnWidth, kColumnWidth, aRectHeight);

			mOwnedImages.push_back(StretchedCopy(theApp, aButton->mOverImage, aRectHeight));
			aButton->mOverImage = mOwnedImages.back();
			mOwnedImages.push_back(StretchedCopy(theApp, aButton->mDownImage, aRectHeight));
			aButton->mDownImage = mOwnedImages.back();

			// (the pixel of the mask a row of the stretched button stands on: the nearest)
			std::vector<int>& aStretchedMask = mOwnedMasks[i];
			aStretchedMask.resize((size_t) aMask->GetWidth() * aRectHeight);
			for (int y = 0; y < aRectHeight; y++)
				memcpy(&aStretchedMask[(size_t) y * aMask->GetWidth()],
					   aMask->GetBits() + (size_t) ((y * aMask->GetHeight()) / aRectHeight) * aMask->GetWidth(),
					   aMask->GetWidth() * sizeof(int));
			aButton->mHitMask = &aStretchedMask[0];
		}

		aButton->mAnimOver.SetNumFrames(aButton->mOverImage->mNumCols);
		aButton->mAnimOver.SetDelay(10);
		aButton->mHasAlpha = true;
		aButton->mHasTransparencies = true;

		aButton->Resize(Rect(aSpec.mRect[0], aSpec.mRect[1], aSpec.mRect[2], aRectHeight));
		// the voice that names the button, shifted by the button's pitch
		aButton->mSoundId = *aSpec.mSound;
		aButton->mSoundPitch = aSpec.mPitch;
		mButtons[i] = aButton;
	}
	mColumn->BitsChanged();

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
	for (size_t i = 0; i < mOwnedImages.size(); i++)
		delete mOwnedImages[i];
	delete mColumn;
}

// 0x43c230
void MainMenu::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
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
		theWidgetManager->RemoveWidget(mButtons[i]);
	theWidgetManager->RemoveWidget(mUserButton);
}

// 0x4461b0
void MainMenu::Draw(Graphics* g)
{
	std::string aName = GT(0x5649d4); // New User

	g->DrawImage(IMAGE_GAME_SELECTOR, 0, 0);
	g->DrawImage(mColumn, kColumnX, kColumnY);
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
				g->DrawImage(GetImageById(kPanelImage[i]), 199, 0xe7);
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
				unsigned long aRand = AstroRand();
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
			aParticle->mY = (int) ((float) aParticle->mY - aSpeed);
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
	if (theId == BUTTON_ID_CLASSIC)
	{
		mHoverStep = (mHoverStep == 1) ? 2 : -5;
		aPanel = 1;
	}
	else if (theId == BUTTON_ID_SURVIVAL)
	{
		if (mHoverStep == 3)
			mHoverStep = 4;
		else
			mHoverStep = (mHoverStep == 4) ? 5 : -5;
		aPanel = 2;
	}
	else if (theId == BUTTON_ID_HALL_OF_FAME)
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
	else if (theId == BUTTON_ID_OPTIONS)
		mHoverStep = (mHoverStep == 6) ? 7 : -5;
	else
	{
		if (theId != BUTTON_ID_QUIT)
			return;
		int aStep = mHoverStep;
		mHoverStep = -5;
		if (aStep == 7)     // (the original's sequence has one more step, More Games between Options and Quit Game)
			mEasterPending = true;
		aPanel = 5;
	}

	for (int i = 0; i < NUM_PANELS; i++)
		mSpeed[i] = (i == aPanel) ? 12 : -8;
	mHovering = true;
}

// 0x434ef0
void MainMenu::ButtonMouseLeave(int theId)
{
	int aPanel;
	if (theId == BUTTON_ID_CLASSIC)
		aPanel = 1;
	else if (theId == BUTTON_ID_SURVIVAL)
		aPanel = 2;
	else if (theId == BUTTON_ID_HALL_OF_FAME)
		aPanel = 3;
	else if (theId == BUTTON_ID_OPTIONS)
		aPanel = 4;
	else
	{
		if (theId != BUTTON_ID_QUIT)
			return;
		aPanel = 5;
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
		}
		mApp->mWidgetManager->SetFocus(this);
	}
}
