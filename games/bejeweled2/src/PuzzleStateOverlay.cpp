// PuzzleStateOverlay.cpp: the dome with the five pods of a puzzle pack's planet (Sexy::PuzzleStateOverlay,
// 0x5d1731-0x5d2fb2, and Sexy::PuzzlePod, 0x5d16ad-0x5d1bb3).

#include "PuzzleStateOverlay.h"
#include "WinDMApp.h"
#include "Board.h"
#include "Profile.h"
#include "GalaxyParser.h"
#include "PodDialog.h"
#include "Res.h"
#include "Graphics.h"
#include "Font.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "Rect.h"
#include "Color.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

using namespace Sexy;

// the angle of each pod round the dome's middle (the floats at 0x62d7f4: 3.66519, 2.61798, 1.57078, 0.52358 and
// 5.75958, as the bits of the original's constants, which differ in the last place from the literals)
static float GetPodAngle(int theIndex)
{
	static const uint32_t aBits[5] = { 0x406a927a, 0x40278cfd, 0x3fc90f54, 0x3f060958, 0x40b84e7b };
	float anAngle;
	memcpy(&anAngle, &aBits[theIndex], sizeof(anAngle));
	return anAngle;
}

// the colours of the gems in a thumbnail, by their number (0x6799d8: made at start-up)
static const Color gThumbColors[10] =
{
	Color(255, 255, 0), Color(255, 255, 255), Color(140, 140, 255), Color(255, 0, 0), Color(255, 0, 255),
	Color(255, 173, 45), Color(0, 200, 0), Color(92, 92, 92), Color(192, 192, 192), Color(255, 200, 180)
};

// 0x5d16ad
PuzzlePod::PuzzlePod()
{
	mIndex = 0;
	mCurve.SetRamp(CurvedVal::RAMP_SLOW_FAST_SLOW);
	mCurve.SetMode(CurvedVal::MODE_CLAMP);
	mCurve.SetOutRange(91.0, 126.0);
	mCurve.SetInVal(1.0);
	mStep = 0.0f;
	mLit = 0.0f;
	mThumb = NULL;
	mSolved = false;
	mGlow = 0.0f;
	mHold = 0;
	mDelay = 0;
}

// 0x5d171d
PuzzlePod::~PuzzlePod()
{
	delete mThumb;
}

// 0x5d1bb3: where the pod's picture is, from the dome's middle by the curve's distance (the pod's number picks the
// angle)
Point PuzzlePod::GetPos()
{
	float anAngle = GetPodAngle(mIndex);
	double aDist = mCurve.GetOutVal();
	float aSin = (float)sin((double)anAngle);
	float aCos = (float)cos((double)anAngle);
	int aX = (int)(((double)aCos * (double)(float)aDist + 151.0) - 55.0);
	int aY = (int)(((double)aSin * (double)(float)aDist + 167.0) - 55.0);
	return Point(aX, aY);
}

// 0x5d2fb2
PuzzleStateOverlay::PuzzleStateOverlay(WinDMApp* theApp)
{
	mDomeOpen = 0.0f;
	mPulse = 0.0f;
	mSelectedPod = -1;
	mPodOver = -1;
	mFlashA = 0.0f;
	mApp = theApp;
	mHasAlpha = true;
	mMouseVisible = false;
	mDomeDelay = 0;
	mDelayA = 0;
	mSolvedCount = 0;

	mDomeImage = new MemoryImage(mApp);
	int aDomeHeight = gApp->ScaleNumUp(0x43, 0);
	int aDomeWidth = gApp->ScaleNumUp(0x43, 0);
	mDomeImage->Create(aDomeWidth, aDomeHeight);
	mDomeImage->SetImageMode(true, true);

	for (int i = 0; i < 5; i++)
	{
		mPods[i].mIndex = i;
		mPods[i].mThumb = new MemoryImage(mApp);
		mPods[i].mThumb->Create(0x2c, 0x2c);
		mPods[i].mThumb->SetImageMode(true, true);
	}
	mFlashB = 0.0f;
	mDomeDirty = true;
	mDelayB = 0;

	mDomeWidget = new PodButton(0, this);
	mDomeWidget->mGlowColor = Color(255, 255, 0);
	mDomeWidget->mDoFinger = true;
	mDomeWidget->mBtnNoDraw = true;
	mDomeWidget->mGlowImage = IMAGE_PUZZLE_MAP_BUTTON_GLOW;
	mDomeWidget->mButtonImage = IMAGE_PUZZLE_MAP_BUTTON;
	mTitleImage = NULL;
	Setup(!mApp->mPuzzleFlag);
}

// 0x5d294b (the original never frees the dome's image; it is freed here)
PuzzleStateOverlay::~PuzzleStateOverlay()
{
	delete mDomeWidget;
	delete mTitleImage;
	delete mDomeImage;
}

// 0x5d1731
void PuzzleStateOverlay::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	theManager->AddWidget(mDomeWidget);
}

// 0x5d1754
void PuzzleStateOverlay::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	theManager->RemoveWidget(mDomeWidget);
}

// 0x5d1777: the dome's picture is the dome with its two halves apart by how far it is open: a mask (turned by the
// opening) takes the alpha, the left half of the dome's colours is copied to the left and mirrored to the right
void PuzzleStateOverlay::RebuildDomeImage()
{
	mDomeImage->Clear();
	Graphics g(mDomeImage);
	g.DrawImageRotated(IMAGE_PUZZLE_DOME_MASK, mApp->ScaleNum(-16, 0), mApp->ScaleNum(-16, 0),
		(double)(mDomeOpen * 3.14159));

	uint32_t* aSrc = (uint32_t*)((MemoryImage*)IMAGE_PUZZLE_DOME)->GetBits();
	int aWidth = gApp->ScaleNumUp(0x43, 0);
	int aHeight = gApp->ScaleNumUp(0x43, 0);
	uint32_t* aDest = (uint32_t*)mDomeImage->GetBits();
	int aHalf = aWidth / 2;

	if (0 < aHeight)
	{
		int aRowOffset = 0;
		for (int aRow = aHeight; aRow != 0; aRow--)
		{
			uint32_t* aMask = (uint32_t*)((char*)mDomeImage->GetBits() + aRowOffset);
			if (0 < aHalf)
			{
				int aCount = aHalf;
				do
				{
					uint32_t aMaskPixel = *aMask;
					uint32_t aDomePixel = *aSrc;
					*aDest++ = ((aMaskPixel ^ aDomePixel) & 0xffffff) ^ aMaskPixel;
					aSrc++;
					aMask++;
					aCount--;
				} while (aCount != 0);
			}
			if (aHalf < aWidth)
			{
				int aCount = aWidth - aHalf;
				do
				{
					uint32_t aMaskPixel = *aMask;
					uint32_t aDomePixel = *aSrc;
					*aDest++ = ((aMaskPixel ^ aDomePixel) & 0xffffff) ^ aMaskPixel;
					aSrc++;
					aMask--;
					aCount--;
				} while (aCount != 0);
			}
			aRowOffset += aWidth * 4;
		}
	}
	mDomeImage->BitsChanged();
}

// 0x5d18eb: the mouse is over the warp button of the dome (only once the dome is open)
bool PuzzleStateOverlay::MouseOverDome(int theX, int theY)
{
	if (mDomeOpen < 0.95f)
		return false;

	int aDX = theX - gApp->ScaleNum(mX + 0x95, 0);
	int aDY = theY - gApp->ScaleNum(mY + 0xa6, 0);
	double aDist = sqrt((double)(aDX * aDX + aDY * aDY));
	return aDist <= (double)gApp->ScaleNum(33.0, 0.0);
}

// 0x5d1995
void PuzzleStateOverlay::SetPodOver(int thePod)
{
	mPodOver = thePod;
}

// 0x5d19a5
void PuzzleStateOverlay::SelectPod(int thePod)
{
	mSelectedPod = thePod;
}

// 0x5d19b5: the puzzle of the pod that is selected is solved; the pod lights up after a while and the dome opens
void PuzzleStateOverlay::PuzzleCompleted()
{
	if (!mPods[mSelectedPod].mSolved)
	{
		mPods[mSelectedPod].mSolved = true;
		mPods[mSelectedPod].mDelay = 150;
		mPods[mSelectedPod].mStep = -0.015f;
		mPods[mSelectedPod].mHold = 200;
		mSolvedCount++;
		mFlashA = 0.0f;
		mFlashB = 0.0f;
		mDomeDelay = 250;
		mDelayA = 180;
		mDelayB = 180;
	}
}

// 0x5d1a3b
void PuzzleStateOverlay::DrawBack(Graphics* g)
{
	g->DrawImage(IMAGE_PUZZLE_WIDGET, mApp->ScaleNum(0x10, 0), mApp->ScaleNum(0x19, 0));

	int aX = mApp->ScaleNum(0x94, 0) - FONT_CONTINUUM_MED_14->StringWidth(mPlanetName) / 2;
	int aY = mApp->ScaleNum(0x35, 0);
	if (mTitleImage != NULL)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0xb4, 0xb4, 0xff, 100));
		g->DrawImage(mTitleImage, 0x43, 0x1e);
		g->SetColorizeImages(false);
	}
	g->SetFont(FONT_CONTINUUM_MED_14);
	g->SetColor(Color(200, 200, 0xff));
	g->DrawString(mPlanetName, aX, aY);
}

// 0x5d1b5d: the dome's button opens the galaxy map
void PuzzleStateOverlay::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_PANEL_SLIDE);
	mApp->PlaySample(SOUND_GALAXY_MAP);
	mApp->mBoard->ShowGalaxyMap();
}

// 0x5d1c27
void PuzzleStateOverlay::Update()
{
	if (!mApp->mHasFocus)
		return;
	Widget::Update();
	if (!mVisible)
		return;

	if (mDelayA < 1)
	{
		if (0.0f < mFlashA)
		{
			float aOld = mFlashA;
			double aNew = (double)aOld + (double)0.003f;
			mFlashA = (float)aNew;
			if (1.0 < aNew)
				mFlashA = 0.0f;
		}
	}
	else
	{
		mDelayA--;
		if (mDelayA == 0)
			mFlashA = 0.0001f;
	}
	if (mDelayB < 1)
	{
		if (0.0f < mFlashB)
		{
			float aOld = mFlashB;
			double aNew = (double)aOld + (double)0.003f;
			mFlashB = (float)aNew;
			if (1.0 < aNew)
				mFlashB = 0.0f;
		}
	}
	else
	{
		mDelayB--;
		if (mDelayB == 0)
			mFlashB = 0.0001f;
	}

	for (int i = 0; i < 5; i++)
	{
		PuzzlePod& aPod = mPods[i];
		if (aPod.mDelay < 1)
			aPod.mCurve.IncInVal((double)aPod.mStep);
		else
			aPod.mDelay--;

		if (aPod.mSolved && aPod.mLit < 1.0f)
		{
			if (aPod.mHold < 1)
			{
				float aOld = aPod.mLit;
				double aNew = (double)aOld + (double)0.004f;
				aPod.mLit = (float)aNew;
				if (1.0 < aNew)
					aPod.mLit = 1.0f;
			}
			else
				aPod.mHold--;
		}

		if (mSelectedPod == i)
		{
			if (aPod.mGlow < 1.0f)
				aPod.mGlow = (float)((double)aPod.mGlow + (double)0.06f);
			if (1.0f < aPod.mGlow)
				aPod.mGlow = 1.0f;
		}
		else
		{
			if (0.0f < aPod.mGlow)
				aPod.mGlow = (float)((double)aPod.mGlow - (double)0.02f);
			if (aPod.mGlow < 0.0f)
				aPod.mGlow = 0.0f;
		}
	}

	if (mDomeDelay < 1)
	{
		float aTarget = (float)((double)(float)mSolvedCount * 0.25);
		if (1.0f < aTarget)
			aTarget = 1.0f;
		if (aTarget != mDomeOpen)
		{
			float aOpen = (float)(((double)aTarget - (double)mDomeOpen) / 200.0 + (double)0.0008f + (double)mDomeOpen);
			mDomeOpen = aOpen;
			if (aTarget < aOpen)
				mDomeOpen = aTarget;
			mDomeDirty = true;
		}
	}
	else
		mDomeDelay--;

	if (0.95f <= mDomeOpen)
	{
		float aPulse;
		bool aSet = true;
		if (!MouseOverDome(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY))
		{
			aPulse = (float)((double)mPulse + (double)0.0055f);
			mPulse = aPulse;
			if (aPulse <= 1.0f)
				aSet = false;
			else
				aPulse = (float)((double)aPulse - 1.0);
		}
		else
		{
			if (0.5f < mPulse)
				mPulse = (float)(1.0 - (double)mPulse);
			float aOld = mPulse;
			double aNew = (double)aOld + (double)0.01f;
			mPulse = (float)aNew;
			aPulse = 0.5f;
			if (aNew <= 0.5)
				aSet = false;
		}
		if (aSet)
			mPulse = aPulse;
	}
	MarkDirty();
}

// 0x5d1f46: the pod whose picture is under the point (the square of the pod's size), -1
int PuzzleStateOverlay::GetPodAt(int theX, int theY)
{
	for (int i = 0; i < 5; i++)
	{
		Point aPos = mPods[i].GetPos();
		int aPodY = aPos.mY + 0x37;
		int aPodX = aPos.mX + 0x37;
		int aDY = abs(theY - mApp->ScaleNum(aPodY, 0));
		int aDX = abs(theX - mApp->ScaleNum(aPodX, 0));
		int aDist;
		if (aDY < aDX)
			aDist = abs(theX - mApp->ScaleNum(aPodX, 0));
		else
			aDist = abs(theY - mApp->ScaleNum(aPodY, 0));
		if (aDist < mApp->ScaleNumUp(0x20, 0))
			return i;
	}
	return -1;
}

// 0x5d2004: the vapor behind the dome, the warp button or the dome's halves, and the five pods. The pods' unlit
// picture is drawn with g, which has the colorizing off, so its alpha does not apply (the original sets the alpha on
// a copy of g that it draws the lit picture with)
void PuzzleStateOverlay::DrawFront(Graphics* g)
{
	Graphics g1(*g);
	g1.SetColorizeImages(true);

	if (mDomeDirty)
	{
		RebuildDomeImage();
		mDomeDirty = false;
	}

	g->SetColorizeImages(true);
	int aGreen = (int)(sin((double)mUpdateCnt * 0.03279) * 55.0 + 170.0);
	int aRed = (int)(50.0 + sin((double)mUpdateCnt * 0.02) * 50.0);
	g->SetColor(Color(aRed, aGreen, 0x14));
	g->DrawImageRotated(IMAGE_PUZZLE_VAPOR1, mApp->ScaleNum(0x76, 0), mApp->ScaleNum(0x86, 0),
		(double)mUpdateCnt * 0.01);

	g->SetDrawMode(1);
	int aGreen2 = (int)(sin((double)mUpdateCnt * 0.05) * 35.0 + 100.0);
	int aRed2 = (int)(sin((double)mUpdateCnt * 0.05) * 35.0 + 100.0);
	Color aVaporColor(aRed2, aGreen2, 0x14);
	if (0.0f < mFlashB)
	{
		float anArg = (float)((double)mFlashB * (double)3.14159f * 2.0 * 2.0);
		float aSin = (float)sin((double)anArg);
		int aBoost = (int)((double)fabsf(aSin) * 160.0);
		aVaporColor.mRed += aBoost;
		if (254 < aVaporColor.mRed)
			aVaporColor.mRed = 255;
		aVaporColor.mGreen += aBoost;
		if (254 < aVaporColor.mGreen)
			aVaporColor.mGreen = 255;
		int aBlue = aBoost + aVaporColor.mBlue;
		if (aBlue < 255)
			aVaporColor.mBlue = aBlue;
		else
			aVaporColor.mBlue = 255;
	}
	g->SetColor(aVaporColor);
	g->DrawImageRotated(IMAGE_PUZZLE_VAPOR2, mApp->ScaleNum(0x76, 0), mApp->ScaleNum(0x86, 0),
		-((double)mUpdateCnt * 0.02));
	g->SetDrawMode(0);

	// the warp button, from when the dome is almost open
	float aWarpAlpha = (float)(1.0 - (1.0 - (double)mDomeOpen) * 9.0);
	if (0.0f < aWarpAlpha)
	{
		float aPulse = (float)((double)mPulse + (double)mPulse);
		aPulse = (float)((double)aPulse - 1.0);
		float aBeat = (float)((double)1 - (double)fabsf(aPulse));
		int aGlowAlpha = (int)((double)aBeat * (double)aWarpAlpha * 255.0);
		g->SetColor(Color(255, 255, 255, aGlowAlpha));
		g->DrawImage(IMAGE_PUZZLE_WARP_GLOW, mApp->ScaleNum(0x6d, 0), mApp->ScaleNum(0x80, 0));
		int aButtonAlpha = (int)((double)aWarpAlpha * 255.0);
		g->SetColor(Color(255, 255, 255, aButtonAlpha));
		g->DrawImage(IMAGE_PUZZLE_WARP_BUTTON, mApp->ScaleNum(0x75, 0), mApp->ScaleNum(0x88, 0));
	}
	g->SetColorizeImages(false);

	// the dome and its two halves
	float aDomeAlpha = (float)((1.0 - (double)mDomeOpen) * 6.0);
	if (1.0f < aDomeAlpha)
		aDomeAlpha = 1.0f;
	if (0.0f < aDomeAlpha)
	{
		g->SetColorizeImages(true);
		int aAlpha = (int)((double)aDomeAlpha * 255.0);
		g->SetColor(Color(255, 255, 255, aAlpha));
		g->DrawImage(mDomeImage, mApp->ScaleNum(0x74, 0), mApp->ScaleNum(0x85, 0));

		double anAngle = (double)mDomeOpen * 3.14159;
		Graphics aLeft(*g);
		aLeft.ClipRect(mApp->ScaleNum(0x76, 0), mApp->ScaleNum(0x86, 0), mApp->ScaleNumUp(0x21, 0),
			mApp->ScaleNumUp(0x43, 0));
		aLeft.DrawImageRotated(IMAGE_PUZZLE_DOME_SLIVER, mApp->ScaleNum(0x76, 0), mApp->ScaleNum(0x86, 0), anAngle);

		Graphics aRight(*g);
		aRight.ClipRect(mApp->ScaleNum(0x97, 0), mApp->ScaleNum(0x86, 0), mApp->ScaleNumUp(0x21, 0),
			mApp->ScaleNum(0x43, 0));
		aRight.DrawImageRotated(IMAGE_PUZZLE_DOME_SLIVER, mApp->ScaleNum(0x76, 0), mApp->ScaleNum(0x86, 0),
			3.14159 - anAngle);
		g->SetColorizeImages(false);
	}

	// the pods; srcX is where a pod's picture begins in the strip of the five
	int aSrcX = 0;
	for (int i = 0; i < 5; i++)
	{
		PuzzlePod& aPod = mPods[i];
		Point aPos = aPod.GetPos();

		if (aPod.mLit < 1.0f)
		{
			g1.SetColor(Color(255, 255, 255, (int)((1.0 - (double)aPod.mLit) * 255.0)));
			int aSize1 = mApp->ScaleNum(0x6e, 0);
			int aSize2 = mApp->ScaleNum(0x6e, 0);
			Rect aSrc(mApp->ScaleNum(aSrcX, 0), 0, aSize2, aSize1);
			g->DrawImage(IMAGE_PUZZLE_PODS, mApp->ScaleNum(aPos.mX, 0), mApp->ScaleNum(aPos.mY, 0), aSrc);
		}
		if (0.0f < aPod.mLit)
		{
			g1.SetColor(Color(255, 255, 255, (int)((double)aPod.mLit * 255.0)));
			int aSize1 = mApp->ScaleNum(0x6e, 0);
			int aSize2 = mApp->ScaleNum(0x6e, 0);
			Rect aSrc(mApp->ScaleNum(aSrcX, 0), 0, aSize2, aSize1);
			g1.DrawImage(IMAGE_PUZZLE_PODS_LIT, mApp->ScaleNum(aPos.mX, 0), mApp->ScaleNum(aPos.mY, 0), aSrc);
		}
		if (0.0f < aPod.mGlow)
		{
			g->SetDrawMode(1);
			g->SetColorizeImages(true);
			int aBlue = (int)((double)aPod.mGlow * (1.0 - (double)aPod.mLit) * 192.0);
			int aGreen3 = (int)((double)aPod.mGlow * 192.0);
			int aRed3 = (int)(192.0 * (double)aPod.mLit * (double)aPod.mGlow);
			Color aGlowColor(aRed3, aGreen3, aBlue);
			if (0.0f < mFlashA)
			{
				float anArg = (float)((double)mFlashA * (double)3.14159f * 2.0 * 2.0);
				float aSin = (float)sin((double)anArg);
				int aBoost = (int)((double)fabsf(aSin) * 160.0);
				aGlowColor.mRed += aBoost;
				if (254 < aGlowColor.mRed)
					aGlowColor.mRed = 255;
				aGlowColor.mGreen += aBoost;
				if (254 < aGlowColor.mGreen)
					aGlowColor.mGreen = 255;
				aGlowColor.mBlue += aBoost;
				if (254 < aGlowColor.mBlue)
					aGlowColor.mBlue = 255;
			}
			g->SetColor(aGlowColor);
			g->DrawImage(IMAGE_PUZZLE_PUZZLE_LIT, mApp->ScaleNum(aPos.mX, 0), mApp->ScaleNum(aPos.mY, 0));
			g->SetColorizeImages(false);
			g->SetDrawMode(0);
		}
		if (!aPod.mSolved)
			g->DrawImage(aPod.mThumb, mApp->ScaleNum(aPos.mX + 0x24, 0), mApp->ScaleNum(aPos.mY + 0x23, 0));
		else
		{
			float aFade = (float)(1.0 - (double)aPod.mLit);
			g->SetColorizeImages(true);
			int aBlueFade = (int)((double)aFade * -207.0);
			int aRedValue = 0x30 - aBlueFade;
			int anAlphaFade = (int)((double)aFade * -63.0);
			int aGreenFade = (int)((double)aFade * -163.0);
			g->SetColor(Color(aRedValue, 0x5c - aGreenFade, aRedValue, 0xc0 - anAlphaFade));
			g->DrawImage(aPod.mThumb, mApp->ScaleNum(aPos.mX + 0x24, 0), mApp->ScaleNum(aPos.mY + 0x23, 0));
			g->SetColorizeImages(false);
		}
		aSrcX += 0x6e;
	}
}

// 0x5d2a83: draws the thumbnail of each pod's puzzle (the gems in their colours, on a grid of 8 by 8)
void PuzzleStateOverlay::GenerateThumbnails()
{
	bool aSmall = gApp->mWidth == 640;
	for (int i = 0; i < 5; i++)
	{
		mPods[i].mThumb->Clear();
		PuzzleProgress* aProgress = mApp->mProfile->GetPuzzleProgress();
		int aPuzzleNum = (aProgress->mPuzzleNum / 5) * 5 + i;
		GalaxyParser* aGalaxy = mApp->mGalaxyParser;
		std::string aFile = PuzzlePath(aGalaxy->GetPlanet(aPuzzleNum / 5).GetPuzzle(aPuzzleNum % 5).mFile,
			"puzzles\\" + aGalaxy->mPuzzlePack);

		int aThumb[8][8];
		if (!Board::LoadPuzzleThumb(aFile, aThumb))
			continue;

		Graphics g(mPods[i].mThumb);
		g.SetColorizeImages(true);
		int aY3 = 0;
		int aY5 = 0;
		for (int aRow = 0; aRow < 8; aRow++)
		{
			int aX3 = 0;
			int aX5 = 0;
			for (int aCol = 0; aCol < 8; aCol++)
			{
				int aType = aThumb[aRow][aCol];
				if (aType != -1)
				{
					if (!aSmall)
					{
						if (aType < 10)
							g.SetColor(gThumbColors[aType]);
						else
							g.SetColor(Color(0, 0, 0));
						g.DrawImage(IMAGE_PUZZLE_THUMB_GEM, aX5, aY5);
					}
					else
					{
						if (aType < 10)
							g.SetColor(Color(gThumbColors[aType].mRed, gThumbColors[aType].mGreen,
								gThumbColors[aType].mBlue, 0xf0));
						else
							g.SetColor(Color(0, 0, 0, 200));
						g.FillRect(aX3, aY3, 2, 2);
					}
				}
				aX3 += 3;
				aX5 += 5;
			}
			aY3 += 3;
			aY5 += 5;
		}
	}
}

// 0x5d2cd6: the pods show the planet that the player is on: which of the five puzzles are solved, which one is
// selected and how far the dome is open. theKeepPuzzle is false when the profile's puzzle moves to the first solved
// one of the planet if the current one is solved
void PuzzleStateOverlay::Setup(bool theKeepPuzzle)
{
	PuzzleProgress* aProgress = mApp->mProfile->GetPuzzleProgress();
	int aPlanetNum = aProgress->mPuzzleNum / 5;
	int aMoonNum = aProgress->mPuzzleNum % 5;
	Planet& aPlanet = mApp->mGalaxyParser->GetPlanet(aPlanetNum);
	int aBase = aPlanetNum * 5;
	bool aCurrentSolved = false;
	int aFirstSolved = 0;
	mSolvedCount = 0;
	int aIndex = aBase;
	for (int i = 0; i < 5; i++)
	{
		PuzzlePod& aPod = mPods[i];
		if (aIndex < (int)aProgress->mSolved.size() && aProgress->mSolved[aIndex] != 0)
		{
			mSolvedCount++;
			if (aFirstSolved == 0)
				aFirstSolved = i;
			if (i == aMoonNum)
				aCurrentSolved = true;
			aPod.mCurve.SetInVal(0.0);
			aPod.mStep = 0.0f;
			aPod.mSolved = true;
			aPod.mLit = 1.0f;
		}
		else
		{
			aPod.mCurve.SetInVal(1.0);
			aPod.mStep = 0.0f;
			aPod.mSolved = false;
			aPod.mLit = 0.0f;
		}
		aPod.mHold = 0;
		aPod.mGlow = 0.0f;
		aPod.mDelay = 0;
		aIndex++;
	}
	if (aCurrentSolved && !theKeepPuzzle)
		aProgress->mPuzzleNum = (short)(aBase + aFirstSolved);

	if (aPlanet.mTutorialFile.empty() || mSolvedCount != 0)
	{
		mSelectedPod = aProgress->mPuzzleNum % 5;
		if (mSelectedPod == -1)
			mSelectedPod = 0;
	}
	else
		mSelectedPod = -1;

	float anOpen = (float)((double)(float)mSolvedCount * 0.25);
	mDomeOpen = anOpen;
	if (1.0 < anOpen)
		mDomeOpen = 1.0f;
	mPulse = 0.0f;
	mDomeDirty = true;
	GenerateThumbnails();

	mPlanetName = mApp->mGalaxyParser->GetPlanet(aPlanetNum).mName;
	if (mTitleImage == NULL)
	{
		mTitleImage = new MemoryImage(mApp);
		mTitleImage->Create(0xa8, 0x21);
		mTitleImage->SetImageMode(true, true);
	}
	int aWidth = FONT_CONTINUUM_MED_14->StringWidth(mPlanetName);
	int aX = mApp->ScaleNum(0x94, 0);
	int aY = mApp->ScaleNum(0x35, 0);
	mTitleImage->Clear();
	Graphics g(mTitleImage);
	g.SetFont(FONT_CONTINUUM_MED_14_GLOW);
	g.SetColor(Color(255, 255, 255));
	g.DrawString(mPlanetName, (aX - aWidth / 2) - 0x43, aY - 0x1e);
}
