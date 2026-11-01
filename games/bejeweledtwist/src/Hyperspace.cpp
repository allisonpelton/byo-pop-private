// Sexy::Hyperspace (Hyperspace.h).
#include "Hyperspace.h"

#include "Board.h"
#include "GameText.h"
#include "GemSoul.h"
#include "Graphics.h"
#include "Profile.h"
#include "Res.h"
#include "TwistUtil.h"
#include "VizManager.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

Hyperspace::Hyperspace()
{
	// The level comes from the board (+0x158 - 1) when there is one: none yet. The ctor also sets up the curves of the
	// level's world and the camera (+0x798 to +0x1008), which only the Direct3D drawing and the jumps read.
	mIs3D = gApp->Is3DAccelerated();
	RemoveAllWidgets(true, false);
}

Hyperspace::~Hyperspace() = default;

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
		// port: the idle space (0x716e2f: the planets at +0xc68, the camera) comes with the board's drawing
		return;
	}
	if (mState != 6) {
		double aCool = mWarmUp - 0.0024999999441206455;
		mWarmUp = 0.0 < aCool ? aCool : 0.0;
	}
	if (!mUnknown12B0.IncInVal()) {
		if (mState == 11) {
			mState = 10;
			gApp->mBoard->SetVisible(true);
			gApp->mVizManager->SetVisible(true);
		}
	} else if (mState == 11 && gBoard != nullptr) {
		gBoard->SetUnknown3924((float)mUnknown12B0.GetOutVal());
	}
	// The trial's limit dialog (+0x152c) is left out: the copy is registered
	if (mFlag15E || (!mWidgets.empty() && mTransition != 9 && mTransition != 10)) {
		mFlag15C = true;
		MarkDirty();
		return;
	}
	Widget::Update();
	mFlag15C = false;
	// The 2D jumps (the port has no Direct3D)
	if (mTransition == 8 || mTransition == 14) {
		if (!mUnknown1570.IncInVal()) {
			if (mFlag15B) {
				// Halfway: the jump's second half
				mFlag15B = false;
				if (mTransition == 14) {
					mState = 6;
					mUnknownC58 = 150;
				} else {
					mUnknown1570.SetCurve(GTS(0x7e49ec));
					mUnknown258.SetCurve(GTS(0x7e49bc));
					gBoard->mUnknown3880.SetCurve(GTS(0x7e498c));
				}
			} else if (mState != 6 && mState != 7) {
				gApp->mVizManager->SetVisible(true);
				mState = 10;
			}
		}
		if (0 < mUnknownC58 && --mUnknownC58 == 0) {
			// The trial's limit (SOUND_TRIALLIMIT and its dialog, 0x73a458) after transition 14: left out, the copy
			// is registered
		}
		if (mState == 7) {
			// The souls fade out
			for (GemSoul* aSoul : gBoard->mUnknown26B0) {
				double aBright = (double)aSoul->mUnknown684 - 0.029999999329447746;
				if (!(0.0 < aBright || 0.0 == aBright))
					aBright = 0.0;
				aSoul->mUnknown684 = (float)aBright;
			}
		}
		mUnknown258.IncInVal();
		gApp->mBoard->SetUnknown3924((float)mUnknown258.GetOutVal());
		if (mUnknown258.CheckInThreshold((double)0.6f) && !mFlag15B) {
			// The new level's gems, while the screen is white
			Board* aBoard = gApp->mBoard;
			aBoard->NewLevel();
			aBoard->ClearBoard();
			aBoard->FillInBlanks(false);
			aBoard->RetargetTravellers();
		}
	} else if (mTransition == 9 || mTransition == 10) {
		if (!mUnknown1570.IncInVal()) {
			gApp->mVizManager->SetVisible(true);
			mState = 10;
		}
		mUnknown258.IncInVal();
		gApp->mBoard->SetUnknown3924((float)mUnknown258.GetOutVal());
	} else if (mTransition == 11) {
		if (!mUnknown1570.IncInVal()) {
			// port: app +0x864's +0x4a8 (a widget it hides) once it is ported
			gApp->mVizManager->SetVisible(true);
			mState = 10;
		}
		mUnknown258.IncInVal();
		gApp->mBoard->SetUnknown3924((float)mUnknown258.GetOutVal());
		gApp->mBoard->ApplyUnknown3924();
	} else {
		// port: the other transitions (0, 12, 13) run the camera and the jump's curves (0x716366 on); they come with
		// the board's drawing
		NotPorted("Hyperspace::Update: the camera (0x716366)");
	}
	MarkDirty();
}

void Hyperspace::Draw(Graphics* g)
{
	DeferOverlay(0);
	// With Direct3D: the level's world in 3D (0x71042e and on); comes with Direct3D, which the port does not have
	if ((mTransition == 12 || mTransition == 13) && !mFlag155) {
		// The end of a jump: space fades out over the board
		if (gBoard == nullptr || gBoard->mUnknown3464 == nullptr)
			mWidgetManager->FlushDeferredOverlayWidgets(0x7fffffff);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol((1.0 - mUnknown12B0.GetOutVal()) * 255.0)));
		DrawImageScaledCentered(g, IMAGE_STATIC_SPACE, (float)(gApp->mWidth / 2), (float)(gApp->mHeight / 2), 3, 1.0f,
			1.0f);
	} else if (mTransition == 12 || mTransition == 13) {
		DrawImageScaledCentered(g, IMAGE_STATIC_SPACE, (float)(gApp->mWidth / 2), (float)(gApp->mHeight / 2), 3, 1.0f,
			1.0f);
		if (mFlag159)
			DrawOldPicture(g);
	} else {
		DrawImageScaledCentered(g, IMAGE_STATIC_SPACE, (float)(gApp->mWidth / 2), (float)(gApp->mHeight / 2), 3, 1.0f,
			1.0f);
	}
	if (mTransition == 8 || mTransition == 14 || mTransition == 9 || mTransition == 10)
		DrawTunnel(g);
	if (mFlash.GetOutVal() == 0.0)
		return;
	g->SetColor(Color(255, 255, 255, Ftol(mFlash.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	g->FillRect(0, 0, mWidth, mHeight);
}

void Hyperspace::DrawTunnel(Graphics* g)
{
	VizManager* aVizManager = gApp->mVizManager;
	if (aVizManager->mVisible)
		return;
	// In 2D the level's picture, the old one for the jump's first half, fading in; Direct3D's tunnel is left out
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol(mUnknown1570.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	int aWidth = Ftol((double)gApp->mScale * 1920.0);
	g->DrawImage(mFlag15B ? mUnknown160 : static_cast<Viz2D*>(aVizManager->mViz)->mImage, (gApp->mWidth - aWidth) / 2,
		0);
}

void Hyperspace::DrawOldPicture(Graphics* g)
{
	// (with Direct3D and state 11 the VizManager draws into +0x160 first)
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol(mUnknown12B0.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	g->DrawImage(mUnknown160, 0, 0);
}

void Hyperspace::DrawOverlay(Graphics* g)
{
	// Direct3D only (+0x156 and +0x157): the planets, the saucer and the tube
	(void)g;
}

void Hyperspace::SetTransition(int theTransition)
{
	mTransition = theTransition;
	// The 2D jumps only: the port has no Direct3D, whose jumps also set up the level's world and the camera
	if (theTransition == 14 || theTransition == 8) {
		// The old level's picture stays for the jump; the VizManager takes the next level's
		Viz2D* aViz = static_cast<Viz2D*>(gApp->mVizManager->mViz);
		mUnknown160 = aViz->mImage;
		mUnknown164 = aViz->mImageId;
		aViz->mImage = nullptr;
		aViz->mImageId = -1;
		gBoard->mUnknown158++;
		ChooseViz(true);
		gBoard->mUnknown158--;
		gApp->mVizManager->SetVisible(false);
		mUnknown1570.SetCurve(GTS(0x7e5de0));
		mUnknown258.SetCurve(GTS(0x7e5db8));
		gBoard->mUnknown3880.SetCurve(GTS(0x7e5d88));
	} else if (theTransition == 9) {
		mFlag155 = true;
		ChooseViz(true);
		mState = 1;
		mUnknown1570.SetCurve(GTS(0x7e4b8c));
		mUnknown258.SetCurve(GTS(0x7e4b60));
		gBoard->mUnknown3880.SetCurve(GTS(0x7e4b34));
		Update();
		gBoard->Update();
		mFlag15B = false;
	} else if (theTransition == 10) {
		SetTransition(9);
		mTransition = 10;
		return;
	} else if (theTransition == 12) {
		// port: the level's world's eleven curves set to constants (0x71bedb on) are only for Direct3D
		mIs3D = false;
		ChooseViz(true);
		mTransition = 12;
		mState = 11;
		mFlag159 = true;
		mLevel = 0;
		mUnknown12B0.SetCurve(GTS(0x7e4a1c));
		gBoard->SetUnknown3924(0.0f);
		mFlag155 = false;
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
	if (aBoard->mGameMode == Board::MODE_CLASSIC && !gApp->mProfile->GetFlag1E4(14)) {
		// The first classic game: the tutorial's picture
		aVizManager->SetBackdrop(-1, true);
		aVizManager->SetVizByName("VizWarpBars", true);
	} else if (aBoard->mGameMode == Board::MODE_BLITZ) {
		// (with Direct3D: VizBuckyShards after an even number of blitz games, when the card can)
		int aGames = gApp->mProfile->mUnknown140[3];
		aVizManager->SetVizByName("VizKaleidoCat", true);
		aVizManager->SetBackdrop(46 - aGames % 47, true);
	} else if (aBoard->mGameMode == Board::MODE_ZEN) {
		aVizManager->SetBackdrop(aBoard->mUnknown158, true);
		aVizManager->SetVizByName("VizZenWheels", true);
	} else if (aBoard->mGameMode != Board::MODE_CHALLENGE) {
		aVizManager->SetBackdrop(aBoard->mUnknown158, false);
		if (aVizManager->mViz == nullptr)
			aVizManager->SetViz(VizManager::FindViz("Viz2D"), true);
		static_cast<Viz2D*>(aVizManager->mViz)->SetImageFromIndex();
		// port: 0x7593ea picks the 3D visualizer for the mode and level (+0x190); 2D never shows it
	}
	// (with Direct3D and theDraw: the VizManager draws into the jump's picture, +0x160)
	(void)theDraw;
	gApp->ClearUpdateBacklog(false);
}
