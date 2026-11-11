#include "VizManager.h"

#include "Board.h"
#include "GameText.h"
#include "GPU2D.h"
#include "Graphics.h"
#include "DDImage.h"
#include "Res.h"
#include "ResourceManager.h"
#include "TwistUtil.h"
#include "Visualizers3D.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cstring>
#include <string>
#include <vector>

using namespace Sexy;

// The visualizers in the order the constructor (0x7597cc) registers them (FUN_0075978d: name, maker, flags); only Viz2D
// (0x757fbb) is made in 2D, and MakeVisualizer3D makes the ported 3D ones by these names
static const char* const kVizNames[] = {"RedBars", "PlasmaClouds", "WindCloud", "SpaceThreads", "BlobDrift",
	"VizWarpBars", "UVWarp", "Slipstream", "GravityField", "VizMoleculeRedux", "VizMagicSnakes", "Cubes", "RainDrops",
	"ContainmentField", "ParametricFractal", "VizRainbowGlyphs", "Raptis_Pulse", "SubAtomic", "FluffyStars",
	"VizJellyBlob", "Gears", "VizKaleidoCat", "VizZenWheels", "VizBuckyShards", "Viz2D"};
static const int kNumViz = (int)(sizeof(kVizNames) / sizeof(kVizNames[0]));
// The third argument of the registration (the entry's +0x20, read by 0x759330): bits 0 to 3 keep it out of game mode
// 1 to 4, 0x10 lets it be picked without Direct3D and 0x20 keeps it from being picked with Direct3D
static const int kVizFlags[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xb, 7, 0xb, 0x30};

// 0x748f54: loads the image theId by its resource name and puts it in the table of images
// port: the original also files it under its id for GetIdByImage (0x73e8c7); nothing asks that of these images
Image* Sexy::LoadVizImage(int theId)
{
	SharedImageRef aRef = gApp->mResourceManager->LoadImage(GetStringIdById(theId));
	Image* anImage = (Image*)aRef;
	*GetImageRefById(theId) = anImage;
	return anImage;
}

// 0x46a160 with the image's resource name
void Sexy::DeleteVizImage(int theId)
{
	gApp->mResourceManager->DeleteImage(GetStringIdById(theId));
}

Visualizer::Visualizer(WinTwistApp* theApp) : mApp(theApp)
{
}

Visualizer::~Visualizer() = default;

// 0x6a9ed6: the grid rectangle in pixels, inline here
void Visualizer::DrawImage(Graphics* g, Image* theImage)
{
	int anImageWidth = theImage->mWidth;
	float aScaleY = (float)theImage->mHeight / (float)mApp->mHeight;
	float anOffsetX;
	float aScaleX;
	if (mApp->mWidth < anImageWidth) {
		aScaleX = 1.0f;
		anOffsetX = (float)((anImageWidth - mApp->mWidth) / 2);
	} else {
		anOffsetX = 0.0f;
		aScaleX = (float)anImageWidth / (float)mApp->mWidth;
	}
	if (!DrawsAroundBoard()) {
		if (aScaleX == 1.0f && aScaleY == 1.0f)
			g->DrawImage(theImage, -Ftol(anOffsetX), 0);
		else
			g->DrawImage(theImage, 0, 0, mWidth, mHeight);
		return;
	}
	// The board's rectangle on the screen (0x6a9ed6), within the visualizer's
	Board* aBoard = mApp->mBoard;
	Rect aBoardRect(aBoard->GetColX(0), aBoard->GetRowY(0), aBoard->mCols * gGemWidth, aBoard->mRows * gGemHeight);
	Rect aRect = aBoardRect.Intersection(Rect(0, 0, mWidth, mHeight));
	int aWidth = mApp->mWidth;
	int aHeight = mApp->mHeight;
	int anOffset = Ftol(anOffsetX);
	if (aScaleX == 1.0f && aScaleY == 1.0f) {
		// Left of the board, above it, right of it and below it, unscaled
		g->DrawImage(theImage, 0, 0,
			Rect(anOffset, 0, Ftol((double)aRect.mX * aScaleX), Ftol((double)aHeight * aScaleY)));
		g->DrawImage(theImage, aRect.mX, 0,
			Rect(Ftol((double)aRect.mX * aScaleX + anOffset), 0, Ftol((double)aRect.mWidth * aScaleX),
				Ftol((double)aRect.mY * aScaleY)));
		int aRight = aRect.mWidth + aRect.mX;
		g->DrawImage(theImage, aRight, 0,
			Rect(Ftol((double)aRight * aScaleX + anOffset), 0, Ftol((double)(aWidth - aRect.mWidth - aRect.mX) * aScaleX),
				Ftol((double)aHeight * aScaleY)));
		int aBottom = aRect.mHeight + aRect.mY;
		g->DrawImage(theImage, aRect.mX, aBottom,
			Rect(Ftol((double)aRect.mX * aScaleX + anOffset), Ftol((double)aBottom * aScaleY),
				Ftol((double)aRect.mWidth * aScaleX), Ftol((double)(aHeight - aRect.mHeight - aRect.mY) * aScaleY)));
	} else {
		// The same four, stretched
		g->DrawImage(theImage, Rect(0, 0, aRect.mX, aHeight),
			Rect(anOffset, 0, Ftol((double)aRect.mX * aScaleX), Ftol((double)aHeight * aScaleY)));
		g->DrawImage(theImage, Rect(aRect.mX, 0, aRect.mWidth, aRect.mY),
			Rect(Ftol((double)aRect.mX * aScaleX + anOffset), 0, Ftol((double)aRect.mWidth * aScaleX),
				Ftol((double)aRect.mY * aScaleY)));
		int aRight = aRect.mWidth + aRect.mX;
		g->DrawImage(theImage, Rect(aRight, 0, aWidth - aRect.mWidth - aRect.mX, aHeight),
			Rect(Ftol((double)aRight * aScaleX + anOffset), 0, Ftol((double)(aWidth - aRect.mWidth - aRect.mX) * aScaleX),
				Ftol((double)aHeight * aScaleY)));
		int aBottom = aRect.mHeight + aRect.mY;
		g->DrawImage(theImage, Rect(aRect.mX, aBottom, aRect.mWidth, aHeight - aRect.mHeight - aRect.mY),
			Rect(Ftol((double)aRect.mX * aScaleX + anOffset), Ftol((double)aBottom * aScaleY),
				Ftol((double)aRect.mWidth * aScaleX), Ftol((double)(aHeight - aRect.mHeight - aRect.mY) * aScaleY)));
	}
}

// part of Viz2D's constructor (0x759a72) and SetImageFromIndex (0x759c67): the backdrop's image
static int BackdropId(int theBackdrop)
{
	if (theBackdrop == -1)
		return IMAGE_VIRTUTORIAL_ID;  // 0x41a
	return theBackdrop % 47 + IMAGE_ZENDROP1_ID;
}

Viz2D::Viz2D(WinTwistApp* theApp) : Visualizer(theApp)
{
	mImageId = BackdropId(theApp->mVizManager->mBackdrop);
	mImage = LoadVizImage(mImageId);
}

Viz2D::~Viz2D()
{
	if (mImageId != -1)
		DeleteVizImage(mImageId);
	if (mNextImageId != -1)
		DeleteVizImage(mNextImageId);
}

void Viz2D::Update()
{
	if (mFade.IncInVal() || !mFade.IsInitialized())
		return;
	// The new picture is in: the old one goes
	if (mImageId != -1)
		DeleteVizImage(mImageId);
	mImage = mNextImage;
	mImageId = mNextImageId;
	mNextImageId = -1;
	mNextImage = nullptr;
}

void Viz2D::Draw(Graphics* g)
{
	DrawImage(g, mImage);
	if (mNextImage == nullptr)
		return;
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol(mFade.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	DrawImage(g, mNextImage);
}

void Viz2D::NewLevel()
{
	mApp->mVizManager->mBackdrop = mApp->mBoard->mLevel;
	mNextImageId = mApp->mBoard->mLevel % 47 + IMAGE_ZENDROP1_ID;
	if (mImageId == mNextImageId)
		return;
	mNextImage = LoadVizImage(mNextImageId);
	mFade.SetCurve(GTS(0x7f2ccc));
}

void Viz2D::SetImageFromIndex()
{
	int anId = BackdropId(mApp->mVizManager->mBackdrop);
	if (mImageId == anId)
		return;
	if (mImageId != -1)
		DeleteVizImage(mImageId);
	mImageId = anId;
	mImage = LoadVizImage(anId);
}

VizManager::VizManager(WinTwistApp* theApp) : mApp(theApp)
{
}

VizManager::~VizManager()
{
	delete mViz;
}

int VizManager::FindViz(const char* theName)
{
	for (int i = 0; i < kNumViz; i++) {
		if (sexy_stricmp(kVizNames[i], theName) == 0)
			return i;
	}
	return -1;
}

Visualizer* VizManager::MakeViz(int theIndex)
{
	theIndex = (int)((unsigned int)theIndex % (unsigned int)kNumViz);
	mVizIndex = theIndex;
	Visualizer* aViz = nullptr;
	mIsViz2D = false;
	if (gD3DInterface != nullptr && strcmp(kVizNames[theIndex], "Viz2D") != 0) {
		aViz = MakeVisualizer3D(kVizNames[theIndex], mApp);
		if (aViz == nullptr)
		{
			static std::string aWhat;
			aWhat = std::string("3D visualizer ") + kVizNames[theIndex] + " (Viz2D stands in)";
			NotPorted(aWhat.c_str());
		}
	}
	if (aViz == nullptr) {
		aViz = new Viz2D(mApp);
		mIsViz2D = true;
	}
	// Half the manager's size when the visualizer draws into a half-size render target (flag 2)
	if (!aViz->Wants(1)) {
		aViz->mWidth = mWidth;
		aViz->mHeight = mHeight;
	} else {
		aViz->mWidth = mWidth / 2;
		aViz->mHeight = mHeight / 2;
	}
	return aViz;
}

void VizManager::SetViz(int theIndex, bool force)
{
	if (!force && theIndex == mVizIndex)
		return;
	delete mViz;
	mViz = MakeViz(theIndex);
	mClipRect[0] = -1;
	mClipRect[1] = -1;
	mClipRect[2] = 1;
	mClipRect[3] = 1;
	mViz->Init();
}

void VizManager::SetVizLater(int theIndex)
{
	delete mViz;
	mViz = nullptr;
	if (mPendingIndex != theIndex) {
		// port: the original's thread (+0x188) makes the visualizer now and TakePendingViz takes it; here
		// TakePendingViz makes it at the point it is taken, so the constructors' and Init's random draws happen there
		mPendingIndex = theIndex;
	}
}

void VizManager::TakePendingViz()
{
	if (mPendingIndex == -1)
		return;
	delete mViz;
	mViz = MakeViz(mPendingIndex);
	mClipRect[0] = -1;
	mClipRect[1] = -1;
	mClipRect[2] = 1;
	mClipRect[3] = 1;
	mViz->Init();
	mPendingIndex = -1;
}

void VizManager::SetVizForMode(int theMode, unsigned int theKey, bool theNow, bool theLater)
{
	// 0x759330: the visualizers that suit the mode (the mode's bit in their flags is not set), in the list's order
	unsigned int aModeBit = 0;
	if (theMode == 1)
		aModeBit = 1;
	else if (theMode == 2)
		aModeBit = 2;
	else if (theMode == 3)
		aModeBit = 4;
	else if (theMode == 4)
		aModeBit = 8;
	std::vector<int> aList;
	for (int i = 0; i < kNumViz; i++) {
		bool aSuits = (kVizFlags[i] & aModeBit) == 0;
		if (gD3DInterface == nullptr) {
			if ((kVizFlags[i] & 0x10) == 0)
				aSuits = false;
		} else {
			if ((kVizFlags[i] & 0x20) != 0)
				aSuits = false;
		}
		if (aSuits)
			aList.push_back(i);
	}
	int anIndex = aList[theKey % (unsigned int)aList.size()];
	mChosenIndex = anIndex;
	if (!m2D) {
		if (!theLater)
			SetViz(anIndex, theNow);
		else
			SetVizLater(anIndex);
	}
}

void VizManager::SetVizByName(const char* theName, bool theNow)
{
	mChosenIndex = FindViz(theName);
	if (!m2D)
		SetViz(mChosenIndex, theNow);
}

void VizManager::SetBackdrop(int theBackdrop, bool theNow)
{
	mBackdrop = theBackdrop;
	if (!m2D) {
		// port: a Viz2D standing in for a 3D visualizer that is not ported shows the picture of the level
		if (mViz != nullptr && mIsViz2D && theNow)
			static_cast<Viz2D*>(mViz)->SetImageFromIndex();
		return;
	}
	if (!theNow)
		return;
	if (mViz == nullptr)
		SetViz(FindViz("Viz2D"), true);
	static_cast<Viz2D*>(mViz)->SetImageFromIndex();
}

void VizManager::NextLook()
{
	if (mViz != nullptr)
		mViz->NextLook();
}

void VizManager::NewLevel()
{
	if (mViz != nullptr)
		mViz->NewLevel();
}

void VizManager::Slot7()
{
	if (mViz != nullptr)
		mViz->Slot7();
}

void VizManager::Update()
{
	if (mViz == nullptr && mPendingIndex != -1)
		return;
	// 2D or 3D follows the options' visualizers box and Direct3D (in a 2D run gD3DInterface is null, so m2D stays true)
	bool aNow2D = !mApp->mDoViz || gD3DInterface == nullptr;
	if (aNow2D != m2D) {
		if (mViz != nullptr)
			SetViz(aNow2D ? FindViz("Viz2D") : mChosenIndex, true);
		m2D = aNow2D;
	}
	if (mViz == nullptr) {
		if (!m2D)
			SetVizByName("VizKaleidoCat", false);
		else
			SetViz(FindViz("Viz2D"), true);
	}
	MarkDirty();
	if (mPaused || !(mVisible || gBoard == nullptr || gBoard->mHyperspace != nullptr))
		return;
	mRunUpdateCnt++;
	if (mViz != nullptr) {
		mUpdateFrac += mSpeed;
		while (mUpdateFrac >= 1.0) {
			mViz->mUpdateCnt++;
			mViz->Update();
			mUpdateFrac -= 1.0;
		}
	}
	MarkDirty();
}

// 0x758253: the draw hook (gD3DInterface +0x10) while the visualizer draws around the board. Each vertex of the primitive
// is placed against the board's rectangle in x and y: 1 inside, 2 before it, 4 after it (or-ed over the vertices, with
// 0.5001 added to the vertex as the original does). Inside in both (1, 1): the draw is skipped. Wholly on one side: drawn
// whole. Straddling it with the other axis inside (3 and 1, 1 and 3, 5 and 1, 1 and 5): drawn clipped to the strip of the
// screen beside the board, one of the four rectangles (0x921664, 0x921654, 0x921644, 0x921634). Anything else is drawn
// whole, with depth testing at mode 2 unless the visualizer clears the depth buffer (bit 4).
// port: the rectangles are zero-initialised statics, which is all the guard bits at 0x921674 (zeroing each one the first
// time it is used) come to, since every use sets all four numbers
bool VizManager::DrawHook(void* theContext, int thePrimType, int theCount, const GPU2DVertex* theVertices, int theStride,
	Rect** theClip)
{
	static Rect sRects[4];  // 0x921664 (left), 0x921654 (top), 0x921644 (right), 0x921634 (bottom)
	VizManager* aManager = (VizManager*)theContext;
	WinTwistApp* anApp = aManager->mApp;
	Visualizer* aViz = aManager->mViz;
	if (!aViz->Wants(0))
		return true;
	int aVertexCount = 0;
	if ((thePrimType == 5) || (thePrimType == 6))
		aVertexCount = theCount + 2;
	else if (thePrimType == 4)
		aVertexCount = theCount * 3;
	unsigned int aXFlags = 0, aYFlags = 0;
	if (aVertexCount > 0) {
		const float* aVertex = (const float*)theVertices;
		for (int i = 0; i < aVertexCount; i++, aVertex += theStride / 4) {
			if ((float)aManager->mBoardRect[0] <= aVertex[0]) {
				if ((double)(aManager->mBoardRect[2] + aManager->mBoardRect[0]) > (double)aVertex[0] + 0.5001)
					aXFlags |= 1;
				else
					aXFlags |= 4;
			} else
				aXFlags |= 2;
			if ((float)aManager->mBoardRect[1] <= aVertex[1]) {
				if ((double)(aManager->mBoardRect[3] + aManager->mBoardRect[1]) > (double)aVertex[1] + 0.5001)
					aYFlags |= 1;
				else
					aYFlags |= 4;
			} else
				aYFlags |= 2;
		}
		if ((aXFlags == 1) && (aYFlags == 1))
			return false;
	}
	if (!aViz->Wants(4)) {
		gD3DInterface->SetDepthTestMode(0);  // slot 0x114
	} else {
		aManager->mClipRect[0] = -1;
		aManager->mClipRect[1] = -1;
		aManager->mClipRect[2] = 1;
		aManager->mClipRect[3] = 1;
	}
	if ((aXFlags == 2) || (aXFlags == 4) || (aYFlags == 2) || (aYFlags == 4))
		return true;
	const int* aBoard = aManager->mBoardRect;
	Rect* aRect;
	if ((aXFlags == 3) && (aYFlags == 1))
		aRect = &sRects[0], *aRect = Rect(0, 0, aBoard[0], anApp->mHeight);
	else if ((aYFlags == 3) && (aXFlags == 1))
		aRect = &sRects[1], *aRect = Rect(0, 0, anApp->mWidth, aBoard[1]);
	else if ((aXFlags == 5) && (aYFlags == 1))
		aRect = &sRects[2], *aRect = Rect(aBoard[0] + aBoard[2], 0, (anApp->mWidth - aBoard[0]) - aBoard[2], anApp->mHeight);
	else if ((aYFlags == 5) && (aXFlags == 1))
		aRect = &sRects[3], *aRect = Rect(0, aBoard[1] + aBoard[3], anApp->mWidth, (anApp->mHeight - aBoard[1]) - aBoard[3]);
	else {
		if (!aViz->Wants(4))
			gD3DInterface->SetDepthTestMode(2);
		return true;
	}
	*theClip = aRect;
	return true;
}

void VizManager::Draw(Graphics* g)
{
	if (mViz == nullptr) {
		g->SetColor(Color::Black);
		g->FillRect(0, 0, mApp->mWidth, mApp->mHeight);
	} else {
		g->PushState();
		Board* aBoard = mApp->mBoard;
		// A board that moves, fades or jumps is drawn over the whole picture; a still one only around it
		// (the rectangles at +0x1bc and +0x1cc are for Direct3D's render target)
		if (aBoard == nullptr || aBoard->mShakeX != 0.0f || aBoard->mShakeY != 0.0f || aBoard->mBoardZoom != 1.0f ||
			aBoard->mBoardFade != 1.0f || aBoard->mHyperspace != nullptr) {
			mViz->mFlags8 &= ~1;
			mViz->mFlagsC |= 1;
			mBoardRect[0] = 0;
			mBoardRect[1] = 0;
			mBoardRect[2] = 0;
			mBoardRect[3] = 0;
			mClipRect[0] = -1;
			mClipRect[1] = -1;
			mClipRect[2] = 1;
			mClipRect[3] = 1;
		} else {
			// 0x6a9ed6: the board's rectangle on the screen
			mBoardRect[0] = aBoard->GetColX(0);
			mBoardRect[1] = aBoard->GetRowY(0);
			mBoardRect[2] = aBoard->mCols * gGemWidth;
			mBoardRect[3] = aBoard->mRows * gGemHeight;
		}
		if (gD3DInterface == nullptr) {
			mViz->Draw(g);
		} else {
			// the draw hook 0x758253 (gD3DInterface +0x10) while the visualizer draws around the board: it skips the
			// primitives lying in the board's rectangle and clips those that straddle it
			if (mViz->DrawsAroundBoard())
				gD3DInterface->SetDrawHook(&VizManager::DrawHook, this);
			// (+0x3c of the app's display, true for the Direct3D 9 path, which every 3D run of the port is, keeps the
			// visualizer from being drawn at half size when false: 0x7587a0's flag 2 is then cleared)
			if (!mViz->Wants(1)) {
				gD3DInterface->SetRenderTarget(g->mDestImage);  // slot 0x98
				if (!mViz->Wants(4)) {
					gD3DInterface->ClearDepth();                // slot 0x124
					gD3DInterface->SetDepthWriteMode(1);        // slot 0x118
					gD3DInterface->FillRect(Rect(mBoardRect[0], mBoardRect[1], mBoardRect[2], mBoardRect[3]), Color::White, 0);  // slot 0x68
					gD3DInterface->SetDepthWriteMode(0);
					for (int i = 0; i < 4; i++)
						mClipRect[i] = mBoardRect[i];
				}
				mViz->Draw(g);
				gD3DInterface->SetDrawHook(nullptr, nullptr);   // +0x10 = 0
				gD3DInterface->SetDepthTestMode(0);             // slot 0x114
			} else {
				if (mViz->DrawsAroundBoard()) {
					// the rectangle at half size, a little inside
					mBoardRect[0] = Ftol((double)mBoardRect[0] * 0.5 + 1.0);
					mBoardRect[1] = Ftol((double)mBoardRect[1] * 0.5 + 1.0);
					mBoardRect[2] = Ftol((double)mBoardRect[2] * 0.5 - 2.0);
					mBoardRect[3] = Ftol((double)mBoardRect[3] * 0.5 - 2.0);
				}
				DDImage* aHalfTarget = gD3DInterface->HoldTarget(&mHalfHold, g->mDestImage->mWidth / 2,
					g->mDestImage->mHeight / 2, 0, "VizHalfSizeRT");  // 0x4f6630
				gD3DInterface->SetRenderTarget(aHalfTarget);
				Graphics* aHalfG = new Graphics(aHalfTarget);
				if (!mViz->Wants(4)) {
					gD3DInterface->ClearDepth();
					gD3DInterface->SetDepthWriteMode(1);
					gD3DInterface->FillRect(Rect(mBoardRect[0], mBoardRect[1], mBoardRect[2], mBoardRect[3]), Color::White, 0);
					gD3DInterface->SetDepthWriteMode(0);
					for (int i = 0; i < 4; i++)
						mClipRect[i] = mBoardRect[i];
				}
				mViz->Draw(aHalfG);
				gD3DInterface->SetDrawHook(nullptr, nullptr);
				gD3DInterface->SetDepthTestMode(0);
				delete aHalfG;
				gD3DInterface->SetRenderTarget(g->mDestImage);
				g->SetLinearBlend(true);
				g->DrawImage(aHalfTarget, 0, 0, g->mDestImage->mWidth, g->mDestImage->mHeight);
				gD3DInterface->ReleaseTarget(&mHalfHold);  // 0x4f66e0
			}
		}
		mViz->mFlagsC &= ~3;
		g->PopState();
	}
	// (with the app's +0x4f4, PopCap's debug text: the visualizer's name, left out)
}
