// Board: the constructor (0x480020), the destructor (body 0x477b90, the scalar deleting wrapper 0x47a910) and the hook
// that the event bus calls before it delivers an event (0x4328d0).

#include "Board.h"
#include "AstroButton.h"
#include "AstroPopApp.h"
#include "Brick.h"
#include "Checkbox.h"
#include "DDImage.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Particle.h"
#include "Res.h"
#include "ResourceManager.h"
#include "SoundInstance.h"
#include "UserProfile.h"

using namespace Sexy;

// 0x480020: the field, the scenery, the hint boxes and the buttons are made; the level the player starts at is set up
// (the rules and the stages up to it are played through by AdvanceLevel) and loaded. theFlags holds the byte at +0x1968
// (when it is not 0 and the game is not the demo, the level is loaded); theFlag is kept at +0x922; theNewGame is true
// when the game starts from the first level of a player who has not played (it marks the profile's flag 0x71);
// theDemo plays the demo's script.
//
// Port: the original's group release of the resources of the three selector screens (PlanetSelector, GameSelector,
// CharSelector, 0x48f8a0) is left out (only the memory differs). The theme checkbox gets no listener yet (the
// application's CheckboxListener part follows with the dialogs).
Board::Board(AstroPopApp* theApp, EventBus* theBus, int theGameType, unsigned int theFlags, int theLevel, bool theFlag,
			 bool theNewGame, bool theDemo)
	: EventListener(theBus)
{
	m7c8 = 0xffff0000;
	m7b8 = 0xffff0000;
	m7c4 = 0xffff0000;
	m7c0 = 0xffff0000;
	m7bc = 0xffff0000;
	m800 = 0x10000;
	m7fc = 0x10000;
	m7f8 = 0x10000;
	m7f4 = 0x10000;
	m7f0 = 0x10000;
	m7ec = 0x10000;
	m7e8 = 0x10000;
	m7e4 = 0x10000;
	m7e0 = 0x10000;
	m7dc = 0x10000;
	m804 = true;
	m808.mA = -1;
	m808.mB = -1;
	m808.mDeltaA = -1;
	m808.mDeltaB = -1;
	m808.mWait = -1;
	m808.mWaitReset = -1;

	mApp = theApp;
	m1a60 = theGameType;
	if (theGameType == 1)
		mMode = theApp->mClassicPilot;
	else
		mMode = theApp->mSurvivalPilot;

	Listen(5);
	Listen(6);
	Listen(7);
	Listen(8);
	Listen(0xc);
	Listen(0xb);
	Listen(0xf);
	Listen(0x10);
	Listen(0x11);
	Listen(0x12);
	Listen(0x13);
	Listen(0xd);
	Listen(0xe);
	Listen(0x1c);
	Listen(0x1a);
	Listen(0x1b);
	Listen(0x1f);
	Listen(0x26);
	Listen(0x29);
	Listen(0x27);
	// the hook at +0x60 is OnFiring
	m8e0 = false;

	// the hint boxes: the SUPA weapon, the meter, a third that the pause fills
	m2b0.resize(3, (HintBox*) NULL);
	m2b0[0] = new HintBox();
	m2b0[0]->AddLine(GT(0x00569b80));
	m2b0[0]->AddLine(GT(0x00569b60));
	m2b0[0]->AddLine(GT(0x00569b40));
	m978 = 0;
	m2e0 = NULL;
	m8da = false;
	m968 = 0xa6;
	m96c = 4;
	m2b0[1] = new HintBox();
	std::string aMeterLine;
	if (m1a60 == 1)
	{
		m2b0[1]->AddLine(GT(0x00569b20));
		aMeterLine = GT(0x00569b0c);
	}
	else
	{
		m2b0[1]->AddLine(GT(0x00569af4));
		aMeterLine = GT(0x00569adc);
	}
	m2b0[1]->AddLine(aMeterLine);
	m2b0[2] = new HintBox();
	m8d9 = false;
	m778 = NULL;
	m8db = false;
	m8dd = false;
	m8dc = false;
	m95c = 0;
	m958 = 0;
	m948 = 0;
	m949 = 0;
	m94c = 1;
	m950 = 1;
	m8e1 = mApp->mFlag6fb;
	for (size_t i = 0; i < m2b0.size(); i++)
	{
		m2b0[i]->SetShown(false);
		m2b0[i]->SetFont(GetFontById(0xb0));
		m2b0[i]->mShowTime = 100;
		m2b0[i]->mHideTime = 800;
	}
	m2b0[2]->mShowTime = 100;
	m922 = theFlag;
	m960 = 0;
	m8df = false;
	m1b6c = 0;
	if (m1a60 == 1)
		m1bbc = 5;
	else
		m1bbc = (m1a60 != 0) ? 500000000 : 3;
	m1968 = (int) (theFlags & 0xff);
	m1940 = theLevel;
	mField193C = 1;
	m1c09 = false;
	m1b48 = -IMAGE_SUPA_BUTTON_ALERT->mWidth;
	m2e4 = NULL;
	m2e8 = NULL;
	m1be4 = 0;
	m913 = false;
	m1b30 = -1;
	m1bb8 = 0;
	m911 = false;
	m910 = false;
	m90f = false;
	m1b1c = 0;
	m1b0c = 0;
	m1b08 = 0;
	m1af4 = 0;
	m1b14 = 0;
	if (theLevel == 1 && theNewGame)
	{
		m923 = false;
		mApp->mUser->SetInt(0x71, 0);
	}
	else
	{
		m923 = true;
		m1b1c = theLevel - 1;
	}
	m754 = 0;
	m894 = IMAGE_UPGRADE_HEAD;
	m890 = IMAGE_UPGRADE_ARM;
	m838 = mApp->CopyImage(IMAGE_UPGRADE_HEAD);
	mApp->FlipImage(m838);
	m834 = mApp->CopyImage(IMAGE_UPGRADE_ARM);
	mApp->FlipImage(m834);
	SetupUpgradeAnims();
	m1b28 = 0;
	m1b2c = 0;
	m8e2 = false;
	m308 = NULL;
	m304 = NULL;
	m928 = 0.0f;
	m92c = 0.0f;
	m912 = false;
	m748 = 0;
	m1abc = 0;
	m970 = 0;
	m90c = false;
	m1ab8 = 4;
	m1ac0 = PickSpawnDelay();
	m914 = false;
	m915 = false;
	m1b74 = 0;
	m1b7c = 0;
	m1b40 = 0;
	m1b50 = 0;
	m919 = false;
	int aHalfBoost = 1;
	mApp->mUser->GetInt(mMode + 0x2a, &aHalfBoost);
	m1c08 = (aHalfBoost == 1);
	m1b88 = 0;
	m1b8c = 0;
	m1b94 = 0;
	m1b98 = 0;
	m1b9c = 0;
	m1ba0 = 0;
	m1ba4 = 0;
	m1ba8 = 0;
	m1bb0 = 0;
	m1bac = 0;
	m1c00 = 0.0f;
	m1bc0 = 0;
	m1bc4 = 0;
	m1bc8 = 0;
	m1bd0 = 0;
	m1bcc = 0;
	m918 = false;

	// the theme checkbox and button of the warp screen (their pictures are made when the stage's colour is known)
	m750 = new Checkbox(NULL, NULL, 0xfa3, NULL);
	m750->mUncheckedRect = Rect(0, 0, IMAGE_TH_CHECKBOX->mWidth / 2, IMAGE_TH_CHECKBOX->mHeight);
	m750->mCheckedRect = Rect(IMAGE_TH_CHECKBOX->mWidth / 2, 0, IMAGE_TH_CHECKBOX->mWidth / 2, IMAGE_TH_CHECKBOX->mHeight);
	int aThemeY = (IMAGE_TH_STRIP->mHeight - IMAGE_TH_CHECKBOX->mHeight) + 0xae;
	m750->Resize(0xa3, aThemeY, IMAGE_TH_CHECKBOX->mWidth / 2, IMAGE_TH_CHECKBOX->mHeight);
	m74c = new AstroButton(0xfa2, mApp);
	m74c->mOverImage = NULL;
	m74c->mDownImage = NULL;
	m74c->mButtonImage = NULL;
	m74c->mLabel = GT(0x00562024);
	m74c->SetFont(GetFontById(0xb1));
	m74c->SetColor(0, Color(0xff, 0xff, 0xff));
	m74c->SetColor(1, Color(0xff, 0xff, 0xff));
	m74c->Resize(0x208, aThemeY - 5, IMAGE_TH_BTN_OVER->mWidth, IMAGE_TH_BTN_OVER->mHeight);
	m1b58 = 0xf;
	m1b4c = 0;
	m1b34 = 0;
	m1b20 = 0;
	m91d = false;
	m1b84 = 0;
	m1b80 = 0;
	m944 = 0.0f;
	m916 = false;
	m1b70 = -1;
	m1b0c = 500000000;
	m1afc = 0;
	m1af8 = 0;
	m250.resize(14, (Image*) NULL);
	m1ac4 = 6;
	mDropTicks = 1000;
	m1af0 = 0;
	m1af4 = 0;
	m710 = GT(0x00569a90);
	int aModeImage = 0x196;
	if (mMode == 1)
		aModeImage = 0x1a4;
	else if (mMode == 2)
		aModeImage = 0x1b2;
	else if (mMode == 3)
		aModeImage = 0x1c0;
	for (size_t i = 0; i < m250.size(); i++)
		m250[i] = LoadImageById(mApp->mResourceManager, aModeImage + (int) i);
	m90b = false;
	m1aa0 = 0;
	m160.resize(5);
	m170.resize(100);
	m180.resize(1);
	m190.resize(3);
	m1c0.assign(1, (Widget*) NULL);
	m1b00 = 0;
	m19b8 = 0;
	m1b78 = 0;
	m1ab0 = 0;
	m906 = false;
	m908 = false;
	m1a24 = 0;
	m90a = false;

	mAnim3e0.SetNumFrames(0x3f);
	mAnim3e0.SetDelay(1);
	mAnim3e0.SetStopped(true);
	mAnim3e0.mPingPong = true;
	m7ec = 0xce;
	m7e8 = -206;
	mAnim4e0.SetNumFrames(0x3f);
	mAnim4e0.SetDelay(1);
	mAnim4e0.mMaxLoops = 10;
	mAnim4e0.mLoopCount = 0;
	mAnim4e0.mPingPong = true;
	mAnim4e0.SetStopped(true);
	m1acc = 0;
	m1adc = 0;
	m1ad8 = 0;
	m1ae8 = 0;
	m1ae4 = 0;
	m1ae0 = 0;
	m1ad4 = 0;
	m1ad0 = 0;
	m1aec = 0;
	mApp->mUser->GetInt(mMode + 0x33, &m1a8c);
	mApp->mUser->GetInt(mMode + 0x3b, &m1a94);
	if (mMode == 0)
		mApp->mUser->GetInt(0x37, &m1a90);
	else if (mMode == 3)
		mApp->mUser->GetInt(0x3a, &m1a90);
	else
		m1a90 = -1;
	m974 = 0;
	mOverlay.Resize(0, 0, 0x280, 0x1e0);
	mOverlay.mBoard = this;
	mOverlay.mHasTransparencies = true;
	mOverlay.mHasAlpha = true;
	m19d4 = 8;
	m314.resize(20);
	for (size_t i = 0; i < m314.size(); i++)
		m314[i].resize(10, (Brick*) NULL);
	m1a54 = 4;
	m6e4 = NULL;
	m900 = false;
	mAnim460.SetNumFrames(1);
	mAnim460.SetDelay(0xc);
	mAnim460.mHoldLast = false;
	mAnim460.mPingPong = false;
	if (mMode == 2)
	{
		mAnim520.SetNumFrames(IMAGE_TURBOT_POWERUP->mNumCols);
		mAnim520.SetDelay(6);
		mAnim520.mPingPong = false;
		mAnim520.SetStopped(true);
		mAnim520.mHoldLast = false;
	}
	else
		mAnim520.SetStopped(true);
	m1a28 = 0;
	m197c = 10000;
	m1970 = 0;
	mScore = 0;
	m8ff = false;
	m1bd8 = 0.0f;
	ResetGame();
	if (m1a60 == 1)
		SetupLevel1Rules();
	else if (m1a60 != 0)
		SetupClassicRules();
	m354 = NULL;
	m198c = 0xa6;
	m70.clear();

	// the script rows are cleared and so are the first rows of the field
	for (int aRow = 0; aRow < 100; aRow++)
	{
		for (int aCol = 0; aCol < 10; aCol++)
		{
			m984.mCode[aRow][aCol] = 0;
			if (aRow < 20)
				m314[aRow][aCol] = NULL;
		}
	}

	// the 100 streaks of the backdrop: a kind, a speed, a place (the random numbers in this order)
	for (size_t i = 0; i < 100; i++)
	{
		StreakRec& aStreak = m170[i];
		int aKind = ((int) AstroRand() % 500000000) % 3;
		if (aKind == 0)
		{
			aStreak.mColor = Color(0x50, 0x50, 0x50);
			int aRand = ((int) AstroRand() % 500000000) % 10;
			float aSpeed = (float) ((double) aRand * (double) 0.01f + (double) 0.25f);
			aStreak.mMinSpeed = aSpeed;
			aStreak.mSpeed = aSpeed;
			aStreak.mImage = mApp->mGradient[0];
		}
		else if (aKind == 1)
		{
			aStreak.mColor = Color(0x96, 0x96, 0x96);
			int aRand = ((int) AstroRand() % 500000000) % 10;
			float aSpeed = (float) ((double) aRand * (double) 0.01f + (double) 0.5f);
			aStreak.mMinSpeed = aSpeed;
			aStreak.mSpeed = aSpeed;
			aStreak.mImage = mApp->mGradient[1];
		}
		else
		{
			aStreak.mColor = Color(0xfa, 0xfa, 0xfa);
			int aRand = ((int) AstroRand() % 500000000) % 10;
			float aSpeed = (float) ((double) aRand * (double) 0.01f + (double) 1.0f);
			aStreak.mMinSpeed = aSpeed;
			aStreak.mSpeed = aSpeed;
			aStreak.mImage = mApp->mGradient[2];
		}
		aStreak.mKind = aKind;
		aStreak.mX = (float) (((int) AstroRand() % 500000000) % mApp->mWidth);
		aStreak.mY = (float) (((int) AstroRand() % 500000000) % (mApp->mHeight - 4) + 4);
	}

	m920 = false;
	m1930 = 0;
	SetupHints();
	if (1 < theLevel * 2 - 2)
	{
		int aCount = theLevel * 2 - 3;
		do
		{
			mField193C = mField193C + 1;
			AdvanceLevel(false);
			aCount--;
		} while (aCount != 0);
	}
	MarkHintsSeen(theLevel);
	if ((theFlags & 0xff) != 0 && !theDemo)
		LoadLevel(true);
	m1954 = 2;
	m8ea = false;

	// the scenery layer; the pictures of the three ids around it are released (the layer's own stays)
	m77c.mImage = LoadImageById(mApp->mResourceManager, 0x18c);
	for (int anId = 0x18b; anId < 0x18e; anId++)
	{
		if (anId != 0x18c)
			mApp->mResourceManager->DeleteImage(GetStringIdById(anId));
	}
	m77c.mI30 = 0x18c;
	m77c.mF24 = 0.0f;
	m77c.mF28 = -1.0f;
	m77c.mCur[0] = 0.0f;
	m77c.mCur[1] = (float) (m77c.mImage->mHeight - 0x1e0);
	m77c.mCur[2] = 640.0f;
	m77c.mCur[3] = 480.0f;
	m77c.mNext[0] = 0.0f;
	m77c.mNext[1] = 0.0f;
	m77c.mNext[2] = 640.0f;
	m77c.mNext[3] = 0.0f;
	m77c.mPeriod = 1;
	m77c.mSpeed = 0.02f;
	m938 = 500000000.0f;
	m93c = 500000000.0f;
	m1948 = 0;
	m19b0 = 500000000;
	m19ac = 500000000;
	m19a8 = 500000000;
	m19a4 = 500000000;
	m19a0 = 500000000;
	m199c = 500000000;
	m19ec = 500000000;
	m19e8 = 500000000;
	m19e4 = 500000000;
	m19e0 = 500000000;
	m19f8 = 500000000;
	m19f4 = 500000000;
	m19f0 = 500000000;
	m8f7 = true;
	m1c0[0] = NULL;
	m2ec = NULL;
	m1a58 = 0;
	m1a5c = 0;
	m1a34 = 0;
	m1bdc = 18000.0f;
	ApplyWeaponLevels();
	m1bd4 = 0;
	m30c = mApp->MakeRankImage(0, 0x50);

	// the text that scrolls along the top while the bonus of state 1 runs, repeated to fill the width
	std::string aBannerText;
	if (mMode == 0)
		aBannerText = GT(0x00569a80);
	else if (mMode == 1 || mMode == 2)
		aBannerText = GT(0x00569a54);
	else
		aBannerText = GT(0x00569a64);
	Font* aBannerFont = GetFontById(0xb0);
	bool aBannerDone = false;
	int aBannerX = -0x1e - aBannerFont->StringWidth(aBannerText);
	do
	{
		if (aBannerX >= aBannerFont->StringWidth(aBannerText) + 0x29e)
			aBannerDone = true;
		DrawnText aText;
		aText.mX = aBannerX;
		aText.mY = 0x1cc;
		aText.mText = aBannerText;
		aText.mFont = aBannerFont;
		m260.push_back(aText);
		aBannerX = aBannerX + 0x1e + aBannerFont->StringWidth(aBannerText);
	} while (!aBannerDone);
	m1b04 = 0;
	m1a00 = mField193C + 1;
	m1a04 = mField193C + 2;
	int aHalf = m1a04 / 2;
	int aRemainder = (aHalf + 1) % 4;
	if (aRemainder == 0)
		m1a08 = -1;
	else
		m1a08 = (aHalf - aRemainder) * 2 + 8;
	m1a0c = -1;
	mApp->mUser->GetInt(mMode + 8, &m1a0c);
	if (m1a0c * 2 - 2 < m1a08)
		m1a08 = -1;
	if (m1a08 < 1)
	{
		m1a70 = 0x17d - IMAGE_SLOW_WARP_GATE->GetCelWidth() / 2;
		m1a74 = -1;
	}
	else
	{
		m1a70 = 0xb6;
		m1a74 = 0x192;
	}

	// the 40 blinking pictures of the backdrop
	for (int i = 0; i < 0x28; i++)
	{
		BlinkRec aBlink;
		aBlink.m00 = ((int) AstroRand() % 500000000) %
			(IMAGE_BEAM_RADIAL->mWidth - IMAGE_PARTICLE_RADIAL->mWidth / IMAGE_PARTICLE_RADIAL->mNumCols);
		aBlink.mCounter = ((int) AstroRand() % 500000000) % 400;
		aBlink.mAnim.SetNumFrames(IMAGE_PARTICLE_RADIAL->mNumCols);
		aBlink.mAnim.SetDelay(5);
		aBlink.mAnim.SetFrame(((int) AstroRand() % 500000000) % aBlink.mAnim.mNumFrames);
		aBlink.mAnim.mHoldLast = false;
		aBlink.mAnim.mPingPong = false;
		aBlink.mAnim.SetStopped(false);
		md8.push_back(aBlink);
	}
	mApp->PlaySong(9);
	m903 = false;
	m902 = false;
	m901 = false;
	m980 = -1;

	// the picture of the field that the wipe at the end of a game takes away, and the wipe
	DDImage* aFieldImage = new DDImage(mApp->mDDInterface);
	m2f4 = aFieldImage;
	aFieldImage->Create(0x280, 0x1e0);
	aFieldImage->SetVolatile(true);
	m2f0 = new Graphics(m2f4);
	m310 = new ScreenWipe();
	m980 = ((int) AstroRand() % 500000000) % 4;
	if (m980 == 0)
		m1a84 = 2000;
	else
		m1a84 = (m980 != 3) * 2 + 4;
	m310->SetImage(m2f4, false);
	m310->SetKind(m980);
	m1a78 = 0;
	m904 = true;
	m1a7c = -1;
	mSirenAnim.SetNumFrames(IMAGE_SIREN->mNumCols);
	mSirenAnim.SetDelay(8);
	mSirenAnim.mHoldLast = false;

	// the siren's two pictures, flipped copies that are drawn into memory images
	m2fc = new MemoryImage(mApp);
	m300 = new MemoryImage(mApp);
	m2fc->Create(IMAGE_SIREN->mWidth, IMAGE_SIREN->mHeight);
	m300->Create(IMAGE_SIREN_FX->mWidth, IMAGE_SIREN_FX->mHeight);
	Graphics aSirenGraphics(m2fc);
	aSirenGraphics.DrawImage(IMAGE_SIREN, 0, 0);
	Graphics aSirenFxGraphics(m300);
	aSirenFxGraphics.DrawImage(IMAGE_SIREN_FX, 0, 0);
	m2fc->Palletize();
	m300->Palletize();
	mApp->FlipImage(m2fc);
	mApp->FlipImage(m300);
	m2fc->mNumCols = IMAGE_SIREN->mNumCols;
	m300->mNumCols = IMAGE_SIREN_FX->mNumCols;
	for (int i = 0; i < 3; i++)
	{
		FadeRec aFade;
		aFade.mValue = 0;
		aFade.mStep = 8;
		aFade.mRising = true;
		m1e0.push_back(aFade);
	}

	// the 40 sparks of the sun, spread by whole steps of nine degrees
	int anAngle = 0;
	for (int i = 0; i < 0x28; i++)
	{
		SunParticle* aSpark = new SunParticle();
		aSpark->mX = 357.0f;
		aSpark->mY = 215.0f;
		aSpark->mAnim.SetDelay(7);
		aSpark->mAnim.SetNumFrames(IMAGE_SUN_PARTICLE->mNumCols);
		aSpark->mAnim.mHoldLast = false;
		aSpark->mAnim.SetStopped(true);
		aSpark->mVX = gCosTable[anAngle] * 4.0f;
		aSpark->mVY = gSinTable[anAngle] * -4.0f;
		me8.push_back(aSpark);
		anAngle = (int) ((double) anAngle + 9.0);
	}
	m8d8 = false;
	if (theDemo)
	{
		m8dc = true;
		LoadDemo();
		BuildField();
	}
}

// 0x477b90 (the scalar deleting wrapper at 0x47a910). What the original does not delete: the pictures of the
// resources, the bricks of the lists (ClearField does that), the members it clears and frees in place. The members at
// +0x2c0 (a vector of pairs), +0x748 and +0x754 (objects) are never set by anything, so their deletes are left out.
Board::~Board()
{
	if (m6e4 != NULL)
		m6e4->Release();

	for (size_t i = 0; i < m2a0.size(); i++)
		delete m2a0[i];
	m2a0.clear();

	if (mApp->mLoopSounds.size() > 7)
	{
		SoundInstance* aKlaxon = mApp->mLoopSounds[7];
		if (aKlaxon->IsPlaying())
			aKlaxon->Stop();
	}

	if (m778 != NULL)
	{
		if (m1a60 == 2)
		{
			delete m778->mImage;
			m778->mImage = NULL;
		}
		delete m778;
	}
	delete m2e0;
	delete m308;
	delete m304;

	if (m74c != NULL)
	{
		delete m74c->mOverImage;
		delete m74c->mDownImage;
		delete m74c->mButtonImage;
	}
	if (m750 != NULL)
		delete m750->mUncheckedImage;
	delete m74c;
	delete m750;

	for (size_t i = 0; i < m230.size(); i++)
		delete m230[i];
	for (size_t i = 0; i < m2b0.size(); i++)
		delete m2b0[i];

	// the two buttons of the end of a game keep their pictures
	if (m2e8 != NULL)
	{
		ButtonWidget* aButton = static_cast<ButtonWidget*>(m2e8);
		delete aButton->mButtonImage;
		delete aButton->mOverImage;
		delete aButton->mDownImage;
	}
	if (m2e4 != NULL)
	{
		ButtonWidget* aButton = static_cast<ButtonWidget*>(m2e4);
		delete aButton->mButtonImage;
		delete aButton->mOverImage;
		delete aButton->mDownImage;
	}
	delete m2e8;
	delete m2e4;

	// the panels of the hints; the first one's chain of two more is deleted with it
	for (size_t i = 0; i < m210.size(); i++)
	{
		if (i == 0 && m210[0]->mNext != NULL)
		{
			delete m210[0]->mNext->mNext;
			delete m210[0]->mNext;
		}
		delete m210[i];
	}

	ClearField();

	if (!m1c0.empty() && m1c0[0] != NULL)
	{
		AstroButton* aMainButton = static_cast<AstroButton*>(m1c0[0]);
		delete aMainButton->mButtonImage;
		delete aMainButton;
	}
	delete m2ec;

	for (size_t i = 0; i < m140.size(); i++)
		delete m140[i];
	for (size_t i = 0; i < m150.size(); i++)
		delete m150[i];

	StopListeningToAll();

	delete m310;
	delete m2f4;
	delete m2f0;

	for (size_t i = 0; i < me8.size(); i++)
		delete me8[i];
	delete m2fc;
	delete m300;
	delete m354;

	if (m88c)
	{
		delete m890;
		delete m894;
	}
	if (m830)
	{
		delete m834;
		delete m838;
	}
}

// 0x4328d0: the hook that the event bus calls for the Board (as a source) before it delivers an event: the events of
// the play clear the flag that SaveGameState sets
void Board::OnFiring(int theEvent)
{
	switch (theEvent)
	{
	case 1:
	case 2:
	case 4:
	case 9:
	case 10:
	case 0xc:
	case 0xd:
	case 0xf:
	case 0x10:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x1f:
	case 0x26:
	case 0x27:
	case 0x29:
		gFlag5A5100 = false;
		break;
	}
}
