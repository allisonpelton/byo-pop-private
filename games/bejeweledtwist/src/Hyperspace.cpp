// Sexy::Hyperspace (Hyperspace.h).
#include "Hyperspace.h"

#include "Board.h"
#include "TwistD3D.h"
#include "GameText.h"
#include "GemSoul.h"
#include "ChallengeMenu.h"
#include "DDImage.h"
#include "Graphics.h"
#include "Menu.h"
#include "Profile.h"
#include "Res.h"
#include "TwistUtil.h"
#include "VizManager.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

// 0x70e341: the world perspective camera's constructor, inline here
// 0x70e361: the off-center perspective camera's constructor, inline here
Hyperspace::Hyperspace()
{
	// The level comes from the board (+0x158 - 1) when there is one: none yet. The ctor also sets up the curves of the
	// level's world and the camera (+0x798 to +0x1008), which only the Direct3D drawing and the jumps read.
	// 0x714f98 (0x715298 to 0x7152d4): +0x155, +0x156 and +0x157 start clear, so the 3D parts (the sidebar, the cone) draw
	// only once a jump or the state 1 to 2 step sets +0x156 (0x7163f3, SetTransition 9 and 10); +0x158 is whether
	// gD3DInterface is set
	mIs3D = false;
	mDrawsTravellers = gD3DInterface != nullptr;
	if (gD3DInterface != nullptr)
		MakeOldPicture();  // 0x714f98: the picture state 11 renders into
	// 0x7152ff to 0x71544f: the cube's step along +0x8e8 (1, so it waits 50 ahead of where the jump starts), the three
	// backdrops' fields of view and the cube's faces
	mCrystalStep.SetConstant(1.0);
	mFirstBackdropFov.SetCurve(GTS(0x7e4688));
	mFirstBackdropFov.SetMode(1);
	mSecondBackdropFov.SetCurve(GTS(0x7e4658));
	mSecondBackdropFov.SetMode(1);
	mThirdBackdropFov.SetCurve(GTS(0x7e4628));
	mThirdBackdropFov.SetMode(1);
	mFirstFaceAlpha.SetCurve(GTS(0x7e4600));
	mSecondFaceAlpha.SetCurve(GTS(0x7e45d8));
	mSingleFaceAlpha.SetCurve(GTS(0x7e45b0));
	RemoveAllWidgets(true, false);
}

Hyperspace::~Hyperspace()
{
	// port: the picture MakeOldPicture made is freed (the original's dtor 0x714e6e is not ported)
	if (mOwnsOldPicture)
		delete mOldPicture;
}

void Hyperspace::MakeOldPicture()
{
	// port: the port's mOldPicture may hold a level's picture from a jump (SetTransition), which belongs to the
	// resources; the original would draw into it, the port makes its own
	if (mOldPicture != nullptr && mOwnsOldPicture)
		return;
	DDImage* anImage = new DDImage(gApp->mDDInterface);
	anImage->mD3DFlags = 0x10;  // +0x44: a render target (the VizManager draws the next level into it, ChooseViz)
	anImage->Create(gApp->mWidth, gApp->mHeight);
	anImage->SetImageMode(false, false);
	gD3DInterface->PrepareImage(anImage);  // port: made a target now, as HoldTarget does
	mOldPicture = anImage;
	mOwnsOldPicture = true;
}

// 0x70e527: the thrust puff's reset, inline here
void Hyperspace::Update()
{
	if (mState == 0 || mState == 6) {
		Widget::Update();
		double aWarmUp = mWarmUp + 0.0010000000474974513;
		if (1.0 <= aWarmUp)
			aWarmUp = 1.0;
		mWarmUp = aWarmUp;
	}
	if (mState == 0) {
		// The idle space: the planets, the camera and the ship's thrust (0x716e2f)
		UpdateSpace();
		MarkDirty();
		return;
	}
	// port: a jump holds still while the board holds its own part of it (UpdateGame returns at once while the board is
	// paused, as when the window loses the focus, or the options are over it). The original's space ran on, so a pause
	// through a level-up left the board's rows and the next level behind it: a crash (UpdateJump read space after it was
	// gone) or a board that never filled again.
	if (gBoard != nullptr && gBoard->mHyperspace == this &&
		(0 < gBoard->mPauseCount || gApp->mOptionsDialog != nullptr)) {
		MarkDirty();
		return;
	}
	if (mState != 6) {
		double aCool = mWarmUp - 0.0024999999441206455;
		mWarmUp = 0.0 < aCool ? aCool : 0.0;
	}
	if (!mBoardBrightnessFinal.IncInVal()) {
		if (mState == 11) {
			mState = 10;
			gApp->mBoard->SetVisible(true);
			gApp->mVizManager->SetVisible(true);
		}
	} else if (mState == 11 && gBoard != nullptr) {
		gBoard->SetBoardFade((float)mBoardBrightnessFinal.GetOutVal());
	}
	if (mHoldsJump || (!mWidgets.empty() && mTransition != 9 && mTransition != 10)) {
		mJumpSkipped = true;
		MarkDirty();
		return;
	}
	Widget::Update();
	mJumpSkipped = false;
	if (gD3DInterface != nullptr) {
		// 0x715c21 with gD3DInterface set: no 2D chain, the common code of 0x716336 and the world's motion
		Update3D();
		MarkDirty();
		return;
	}
	// The 2D jumps (gD3DInterface clear)
	// (transition 14, the trial's last jump, its limit dialog and its souls fading in state 7, is registration code,
	// left out)
	if (mTransition == 8) {
		if (!mJumpLength.IncInVal()) {
			if (mFirstHalfPending) {
				// Halfway: the jump's second half
				mFirstHalfPending = false;
				mJumpLength.SetCurve(GTS(0x7e49ec));
				mBoardBrightness.SetCurve(GTS(0x7e49bc));
				gBoard->mTubeSlide.SetCurve(GTS(0x7e498c));
			} else {
				gApp->mVizManager->SetVisible(true);
				mState = 10;
			}
		}
		mBoardBrightness.IncInVal();
		gApp->mBoard->SetBoardFade((float)mBoardBrightness.GetOutVal());
		if (mBoardBrightness.CheckInThreshold((double)0.6f) && !mFirstHalfPending) {
			// The new level's gems, while the screen is white
			Board* aBoard = gApp->mBoard;
			aBoard->NewLevel();
			aBoard->ClearBoard();
			aBoard->FillInBlanks(false);
			aBoard->RetargetTravellers();
		}
	} else if (mTransition == 9 || mTransition == 10) {
		if (!mJumpLength.IncInVal()) {
			gApp->mVizManager->SetVisible(true);
			mState = 10;
		}
		mBoardBrightness.IncInVal();
		gApp->mBoard->SetBoardFade((float)mBoardBrightness.GetOutVal());
	} else if (mTransition == 11) {
		if (!mJumpLength.IncInVal()) {
			// the challenge menu goes (0x715dc7)
			if (gApp->mMainMenu->mChallengeMenu != nullptr)
				gApp->mMainMenu->mChallengeMenu->mVisible = false;
			gApp->mVizManager->SetVisible(true);
			mState = 10;
		}
		mBoardBrightness.IncInVal();
		gApp->mBoard->SetBoardFade((float)mBoardBrightness.GetOutVal());
		gApp->mBoard->ApplyBoardFade();
	} else {
		// The other transitions (in 2D 0, 12 and 13, and the menus' 1, 2, 4 and 5) run the camera's path (0x716366)
		// (with Direct3D, when +0x1e8 is done in state 1: the VizManager draws into +0x160 and the camera flies off,
		// state 2, SOUND_TRANS_SPACENOISE)
		mVizSpeed.IncInVal();
		gApp->mVizManager->mSpeed = mVizSpeed.GetOutVal();
		// (the VizManager's rectangle at +0x1bc, -1, -1, 1, 1, is for Direct3D's render target)
		Board* aBoard = gApp->mBoard;
		if (aBoard != nullptr && !aBoard->mIsReplay) {
			mBoardBrightness.IncInVal();
			aBoard->SetBoardFade((float)mBoardBrightness.GetOutVal());
			if (mTransition == 11)
				aBoard->ApplyBoardFade();
		}
		// (the curves +0x900, +0x10f0, +0x2c8 and +0x338 step here; only the 3D camera reads them)
		if (1 < mState && mState < 10) {
			// States 2 to 9 with these transitions come only from the Direct3D jumps (state 1 goes on to 2 only with
			// Direct3D; the 2D jumps set their states with transitions 8, 9, 10, 11 and 14, handled above)
			NotPorted("Hyperspace::Update: states 2 to 9 of the camera (0x716505, Direct3D)");
		}
		UpdateSpace();
	}
	MarkDirty();
}

void Hyperspace::UpdateSpace()
{
	// 0x716e2f: the crystals, the camera, the ship and the saucer are only drawn with Direct3D
	Board* aBoard = gApp->mBoard;
	if (aBoard == nullptr || aBoard->mHyperspace != this)
		return;
	if (gD3DInterface != nullptr) {
		UpdateWorld3D();  // 0x716e31 to 0x717...
		PlaceShip();      // 0x715480
		PlaceSaucer();    // 0x7158f8
	}
	UpdateThrust();  // 0x715c21 (0x7172be to 0x717dbe): in 2D and 3D alike, the random numbers in the original's order
}

void Hyperspace::StartEndGame()
{
	// 0x714883
	mTransition = 13;
	mState = 12;
	mBoardBrightnessFinal.SetCurve(GTS(0x7d8610));
	if (gD3DInterface == nullptr && gApp->mBoard->mGameMode != Board::MODE_CHALLENGE) {
		// 2D, not a challenge: space fades in over the board
		mBoardBrightness.SetConstant(1.0);
		mZOrder = gApp->mBoard->mZOrder;
		mWidgetManager->PutInfront(this, gApp->mBoard);
		gApp->mVizManager->SetVisible(true);
		gApp->mBoard->SetVisible(true);
		mHasAlpha = true;
		mDrawsOwnBackground = false;
	} else {
		// Direct3D, or a challenge: the VizManager and the board are drawn into the old level's picture, which the
		// challenge menu draws over itself while space comes back (0x714960)
		MakeOldPicture();
		Graphics aG(mOldPicture);
		gApp->mVizManager->Draw(&aG);
		ModalFlags aFlags;
		mWidgetManager->InitModalFlags(&aFlags);
		gApp->mBoard->DrawAll(&aFlags, &aG);
		mWidgetManager->mCurG = &aG;
		mWidgetManager->FlushDeferredOverlayWidgets(0x7fffffff);
		mWidgetManager->mCurG = nullptr;
		mDrawsOwnBackground = true;
		mLevel = 0;
		gApp->mVizManager->SetVisible(false);
		mDrawsFinalFade = gD3DInterface != nullptr;
	}
}

void Hyperspace::Draw(Graphics* g)
{
	DeferOverlay(0);
	if (gD3DInterface != nullptr) {
		Draw3D(g);  // 0x713e2d's 3D branch (HyperspaceD3D.cpp)
		return;
	}
	// With Direct3D: the level's world in 3D (0x71042e and on); comes with Direct3D, which the port does not have
	if ((mTransition == 12 || mTransition == 13) && !mDrawsOwnBackground) {
		// The end of a jump: space fades out over the board
		if (gBoard == nullptr || gBoard->mFinalScoreLabel == nullptr)
			mWidgetManager->FlushDeferredOverlayWidgets(0x7fffffff);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol((1.0 - mBoardBrightnessFinal.GetOutVal()) * 255.0)));
		DrawImageScaledCentered(g, IMAGE_STATIC_SPACE, (float)(gApp->mWidth / 2), (float)(gApp->mHeight / 2), 3, 1.0f,
			1.0f);
	} else if (mTransition == 12 || mTransition == 13) {
		DrawImageScaledCentered(g, IMAGE_STATIC_SPACE, (float)(gApp->mWidth / 2), (float)(gApp->mHeight / 2), 3, 1.0f,
			1.0f);
		if (mDrawsFinalFade)
			DrawOldPicture(g);
	} else {
		DrawImageScaledCentered(g, IMAGE_STATIC_SPACE, (float)(gApp->mWidth / 2), (float)(gApp->mHeight / 2), 3, 1.0f,
			1.0f);
	}
	if (mTransition == 8 || mTransition == 9 || mTransition == 10)
		DrawTunnel(g);
	if (mFlash.GetOutVal() == 0.0)
		return;
	g->SetColor(Color(255, 255, 255, Ftol(mFlash.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	g->FillRect(0, 0, mWidth, mHeight);
}

void Hyperspace::DrawTunnel(Graphics* g)
{
	if (gD3DInterface != nullptr) {
		DrawTunnel3D(g);
		return;
	}
	VizManager* aVizManager = gApp->mVizManager;
	if (aVizManager->mVisible)
		return;
	// In 2D the level's picture, the old one for the jump's first half, fading in; Direct3D's tunnel is left out
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol(mJumpLength.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	int aWidth = Ftol((double)gApp->mScale * 1920.0);
	g->DrawImage(mFirstHalfPending ? mOldPicture : static_cast<Viz2D*>(aVizManager->mViz)->mImage, (gApp->mWidth - aWidth) / 2,
		0);
}

void Hyperspace::DrawOldPicture(Graphics* g)
{
	if (mState == 11 && gD3DInterface != nullptr) {
		// 0x70f836: with 3D the old picture is drawn afresh, the VizManager and then the board into +0x160
		Graphics aG(mOldPicture);
		gApp->mVizManager->Draw(&aG);
		ModalFlags aFlags;
		mWidgetManager->InitModalFlags(&aFlags);
		gApp->mBoard->DrawAll(&aFlags, &aG);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol(mBoardBrightnessFinal.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	g->DrawImage(mOldPicture, 0, 0);
}

void Hyperspace::DrawOverlay(Graphics* g)
{
	// Direct3D only (+0x156 and +0x157): the planets, the saucer and the tube
	(void)g;
	if (gD3DInterface != nullptr && mIs3D && mIsNewGameJump) {
		// 0x713d82: with +0x157 set the sidebar, the cone and the thrust are drawn on top, as the overlay
		Draw3DSidebar();
		DrawCone();
		DrawParticles();
	}
}

void Hyperspace::StartChallenge()
{
	mState = 3;
	mTransition = 11;
	// The 2D branch only (the 3D one flies the camera in to the planet)
	mFirstHalfPending = false;
	ChooseViz(false);
	mJumpLength.SetCurve(GTS(0x7e6288));
	mBoardBrightness.SetCurve(GTS(0x7e6258));
	gBoard->mTubeSlide.SetCurve(GTS(0x7e6228));
	gBoard->Update();
	SetTransition(11);
}

void Hyperspace::SetTransition(int theTransition)
{
	mTransition = theTransition;
	if (gD3DInterface != nullptr) {
		SetTransition3D(theTransition);
		return;
	}
	// The 2D jumps only: the port has no Direct3D, whose jumps also set up the level's world and the camera
	if (theTransition == 8) {
		// The old level's picture stays for the jump; the VizManager takes the next level's
		Viz2D* aViz = static_cast<Viz2D*>(gApp->mVizManager->mViz);
		// (the original deletes the old picture first; the port deletes only the one it made)
		if (mOwnsOldPicture) {
			delete mOldPicture;
			mOwnsOldPicture = false;
		}
		mOldPicture = aViz->mImage;
		mOldPictureId = aViz->mImageId;
		aViz->mImage = nullptr;
		aViz->mImageId = -1;
		gBoard->mLevel++;
		ChooseViz(true);
		gBoard->mLevel--;
		gApp->mVizManager->SetVisible(false);
		mJumpLength.SetCurve(GTS(0x7e5de0));
		mBoardBrightness.SetCurve(GTS(0x7e5db8));
		gBoard->mTubeSlide.SetCurve(GTS(0x7e5d88));
	} else if (theTransition == 9) {
		mDrawsOwnBackground = true;
		ChooseViz(true);
		mState = 1;
		mJumpLength.SetCurve(GTS(0x7e4b8c));
		mBoardBrightness.SetCurve(GTS(0x7e4b60));
		gBoard->mTubeSlide.SetCurve(GTS(0x7e4b34));
		Update();
		gBoard->Update();
		mFirstHalfPending = false;
	} else if (theTransition == 10) {
		SetTransition(9);
		mTransition = 10;
		return;
	} else if (theTransition == 12) {
		// The level's world's eleven curves are set to constants (0x71bedb): of them only the board's brightness
		// (+0x258) and the VizManager's speed (+0x1e8) count in 2D, the rest are the 3D camera's
		mBoardBrightness.SetConstant(1.0);
		mVizSpeed.SetConstant(1.0);
		mIs3D = false;
		ChooseViz(true);
		mTransition = 12;
		mState = 11;
		mDrawsFinalFade = true;
		mLevel = 0;
		mBoardBrightnessFinal.SetCurve(GTS(0x7e4a1c));
		gBoard->SetBoardFade(0.0f);
		mDrawsOwnBackground = false;
		mHasAlpha = true;
		mZOrder = gApp->mBoard->mZOrder + 1;
		mWidgetManager->BringToFront(this);
		gApp->mVizManager->SetVisible(true);
	}
	Update();
}

void Hyperspace::ChooseViz(bool theDraw)
{
	Board* aBoard = gApp->mBoard;
	VizManager* aVizManager = gApp->mVizManager;
	if (aBoard->mGameMode == Board::MODE_CLASSIC && !gApp->mProfile->HasSeenTip(14)) {
		// The first classic game: the tutorial's picture
		aVizManager->SetBackdrop(-1, true);
		aVizManager->SetVizByName("VizWarpBars", true);
	} else if (aBoard->mGameMode == Board::MODE_BLITZ) {
		// With Direct3D VizBuckyShards after an even number of blitz games, when the card can (slot 4's bit 0x20 set)
		int aGames = gApp->mProfile->mGamesByMode[3];
		const char* aVizName = "VizKaleidoCat";
		if (aGames % 2 == 0 && gD3DInterface != nullptr && (gD3DInterface->GetCaps(0) & 0x20) != 0)
			aVizName = "VizBuckyShards";
		aVizManager->SetVizByName(aVizName, true);
		aVizManager->SetBackdrop(46 - aGames % 47, true);
	} else if (aBoard->mGameMode == Board::MODE_ZEN) {
		aVizManager->SetBackdrop(aBoard->mLevel, true);
		aVizManager->SetVizByName("VizZenWheels", true);
	} else if (aBoard->mGameMode != Board::MODE_CHALLENGE) {
		aVizManager->SetBackdrop(aBoard->mLevel, gD3DInterface != nullptr);
		if (gD3DInterface == nullptr) {
			if (aVizManager->mViz == nullptr)
				aVizManager->SetViz(VizManager::FindViz("Viz2D"), true);
			static_cast<Viz2D*>(aVizManager->mViz)->SetImageFromIndex();
		}
		// 0x7593ea picks the 3D visualizer for the mode and level (+0x190); 2D never shows it, so only 3D calls it
		if (gD3DInterface != nullptr) {
			int aKey = aBoard->mLevel + 1;
			if (aKey < 0)
				aKey = -aKey;  // 0x77e769
			aVizManager->SetVizForMode(aBoard->mGameMode, (unsigned int)aKey, true, !theDraw);
		}
	}
	// With Direct3D and theDraw the VizManager draws into the jump's picture, +0x160
	if (gD3DInterface != nullptr && theDraw) {
		Graphics aG(mOldPicture);
		aVizManager->Draw(&aG);
	}
	gApp->ClearUpdateBacklog(false);
}
