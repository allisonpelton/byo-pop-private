#include "VizManager.h"

#include "Board.h"
#include "GameText.h"
#include "Graphics.h"
#include "Res.h"
#include "ResourceManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

// The visualizers in the order the constructor (0x7597cc) registers them; only Viz2D (0x757fbb) is made in 2D
static const char* const kVizNames[] = {"RedBars", "PlasmaClouds", "WindCloud", "SpaceThreads", "BlobDrift",
	"VizWarpBars", "UVWarp", "Slipstream", "GravityField", "VizMoleculeRedux", "VizMagicSnakes", "Cubes", "RainDrops",
	"ContainmentField", "ParametricFractal", "VizRainbowGlyphs", "Raptis_Pulse", "SubAtomic", "FluffyStars",
	"VizJellyBlob", "Gears", "VizKaleidoCat", "VizZenWheels", "VizBuckyShards", "Viz2D"};
static const int kNumViz = (int)(sizeof(kVizNames) / sizeof(kVizNames[0]));

// 0x748f54: loads the image theId by its resource name and puts it in the table of images
// port: the original also files it under its id for GetIdByImage (0x73e8c7); nothing asks that of these images
static Image* LoadImageById(int theId)
{
	SharedImageRef aRef = gApp->mResourceManager->LoadImage(GetStringIdById(theId));
	Image* anImage = (Image*)aRef;
	*GetImageRefById(theId) = anImage;
	return anImage;
}

// 0x46a160 with the image's resource name
static void DeleteImageById(int theId)
{
	gApp->mResourceManager->DeleteImage(GetStringIdById(theId));
}

Visualizer::Visualizer(WinTwistApp* theApp) : mApp(theApp)
{
}

Visualizer::~Visualizer() = default;

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

static int BackdropId(int theBackdrop)
{
	if (theBackdrop == -1)
		return IMAGE_VIRTUTORIAL_ID;  // 0x41a
	return theBackdrop % 47 + IMAGE_ZENDROP1_ID;
}

Viz2D::Viz2D(WinTwistApp* theApp) : Visualizer(theApp)
{
	mImageId = BackdropId(theApp->mVizManager->mBackdrop);
	mImage = LoadImageById(mImageId);
}

Viz2D::~Viz2D()
{
	if (mImageId != -1)
		DeleteImageById(mImageId);
	if (mNextImageId != -1)
		DeleteImageById(mNextImageId);
}

void Viz2D::Update()
{
	if (mFade.IncInVal() || !mFade.IsInitialized())
		return;
	// The new picture is in: the old one goes
	if (mImageId != -1)
		DeleteImageById(mImageId);
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
	mApp->mVizManager->mBackdrop = mApp->mBoard->mUnknown158;
	mNextImageId = mApp->mBoard->mUnknown158 % 47 + IMAGE_ZENDROP1_ID;
	if (mImageId == mNextImageId)
		return;
	mNextImage = LoadImageById(mNextImageId);
	mFade.SetCurve(GTS(0x7f2ccc));
}

void Viz2D::SetImageFromIndex()
{
	int anId = BackdropId(mApp->mVizManager->mBackdrop);
	if (mImageId == anId)
		return;
	if (mImageId != -1)
		DeleteImageById(mImageId);
	mImageId = anId;
	mImage = LoadImageById(anId);
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

void VizManager::SetViz(int theIndex, bool force)
{
	if (!force && theIndex == mVizIndex)
		return;
	delete mViz;
	// 0x758dcf: made by the list's maker (the 3D ones would draw at half size into a render target)
	mVizIndex = (int)((unsigned int)theIndex % (unsigned int)kNumViz);
	mViz = new Viz2D(mApp);
	mViz->mWidth = mWidth;
	mViz->mHeight = mHeight;
	mViz->Init();
}

void VizManager::SetVizByName(const char* theName, bool theNow)
{
	mUnknown190 = FindViz(theName);
	if (!m2D)
		SetViz(mUnknown190, theNow);
}

void VizManager::SetBackdrop(int theBackdrop, bool theNow)
{
	mBackdrop = theBackdrop;
	if (!m2D || !theNow)
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
	if (mViz == nullptr && mUnknown18C != -1)
		return;
	// (2D or 3D follows the options here: in 2D the port never changes)
	if (mViz == nullptr) {
		if (m2D)
			SetViz(FindViz("Viz2D"), true);
		else
			SetVizByName("VizKaleidoCat", false);
	}
	MarkDirty();
	if (mPaused || !(mVisible || gBoard == nullptr || gBoard->mHyperspace != nullptr))
		return;
	mUpdateCnt148++;
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
		if (aBoard == nullptr || aBoard->mShakeX != 0.0f || aBoard->mShakeY != 0.0f || aBoard->mUnknown268C != 1.0f ||
			aBoard->mUnknown3924 != 1.0f || aBoard->mHyperspace != nullptr) {
			mViz->mFlags8 &= ~1;
			mViz->mFlagsC |= 1;
		}
		mViz->Draw(g);
		mViz->mFlagsC &= ~3;
		g->PopState();
	}
	// (with the app's +0x4f4, PopCap's debug text: the visualizer's name, left out)
}
