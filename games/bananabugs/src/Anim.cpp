// Anim: one picture cut into cels, pre-scaled to the window when it loads (0x401000..0x402640 in the original).
#include <cstdio>
#include "Anim.h"

#include "Color.h"
#include "DDImage.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"

#include <cstring>

using namespace Sexy;

std::list<Anim*> Anim::sAnims;
SexyAppBase* Anim::sApp = nullptr;

bool AnimReload::sActive = false;
float AnimReload::sProgress = 0.0f;
float AnimReload::sStep = 0.0f;
float AnimReload::sTotal = 0.0f;
Image* AnimReload::sImage = nullptr;
std::list<Anim*>::iterator AnimReload::sNext;

namespace Sexy
{
float gScreenScaleX = 1.0f;
float gScreenScaleY = 1.0f;
float gImageScaleX = 1.0f;
float gImageScaleY = 1.0f;
}

// 0x401190
Anim::Anim()
{
	mRefCount = 1;
	mPersistent = true;
	mUnusedAfterName = 0;
	mImage = nullptr;
	mNoScale = false;
	memset(mName, 0, sizeof(mName));
	mCelsAcross = 0;
	mCelsDown = 0;
	mCelWidth = 0.0f;
	mCelHeight = 0.0f;
	mNumCels = 0;
	mSourceCelWidth = 0.0f;
	mSourceCelHeight = 0.0f;
	mScaleMode = ANIM_SCALE_NONE;
	mUniformScale = false;
	memset(mCels, 0, sizeof(mCels));
}

// 0x4011d0
Anim::~Anim()
{
	if (!sAnims.empty())
		Unregister(this);
	if (mImage != nullptr) {
		delete mImage;
		mImage = nullptr;
	}
}

// 0x401210
void Anim::AddRef()
{
	mRefCount++;
}

// 0x401220
void Anim::Release()
{
	mRefCount--;
	if (mRefCount < 1) {
		mRefCount = 0;
		if (!mPersistent)
			delete this;
	}
}

// 0x402200: every registry entry for this Anim goes
void Anim::Unregister(Anim* theAnim)
{
	for (std::list<Anim*>::iterator it = sAnims.begin(); it != sAnims.end();) {
		if (*it == theAnim)
			it = sAnims.erase(it);
		else
			++it;
	}
}

// 0x402080
void Anim::SetApp(SexyAppBase* theApp)
{
	sApp = theApp;
}

// 0x401270
void Anim::Load(bool noScale, const char* theName, int theMode, int theCelWidth, int theCelHeight, int theNumCels,
	bool uniformScale)
{
	mUniformScale = uniformScale;
	mNoScale = noScale;
	strcpy(mName, theName);
	Image* anImage = sApp->GetImage(theName, true);
	float aScaleX;
	float aScaleY;
	if (!mNoScale) {
		aScaleX = gImageScaleX;
		aScaleY = mUniformScale ? gImageScaleX : gImageScaleY;
	} else {
		aScaleX = 1.0f;
		aScaleY = 1.0f;
	}
	Scale(anImage, theMode, aScaleX, aScaleY, theCelWidth, theCelHeight, theNumCels);
	sAnims.push_back(this);
}

// 0x4013f0: the fill cells' sheet of 31-pixel cels, scaled smoothly by the window's own scale; its one caller loads
// FillObj::sFillSheet
void Anim::LoadFillSheet(const char* theName)
{
	float aScaleY = gScreenScaleY;
	float aScaleX = gScreenScaleX;
	strcpy(mName, theName);
	Image* anImage = sApp->GetImage(theName, true);
	mScaleMode = ANIM_SCALE_SMOOTH;
	mUniformScale = false;
	mNoScale = false;
	const int FILL_CEL_SIZE = 31;
	int aCelsAcross = anImage->GetWidth() / FILL_CEL_SIZE;
	int aCelsDown = anImage->GetHeight() / FILL_CEL_SIZE;
	mNumCels = aCelsAcross * aCelsDown;
	Scale(anImage, ANIM_SCALE_SMOOTH, aScaleX, aScaleY, FILL_CEL_SIZE, FILL_CEL_SIZE, mNumCels);
	sAnims.push_back(this);
}

// 0x401560: cuts the picture into cels and draws each, scaled, into a new sheet. A cel's scaled size is rounded and
// made odd, so it has a middle pixel.
void Anim::Scale(Image* theImage, int theMode, float theScaleX, float theScaleY, int theCelWidth, int theCelHeight,
	int theNumCels)
{
	mNumCels = theNumCels;
	Rect aSrcRect(0, 0, 0, 0);
	mScaleMode = theMode;
	if (theCelWidth < 1 || theCelHeight < 1) {
		theCelWidth = theImage->GetWidth();
		theCelHeight = theImage->GetHeight();
	}
	mSourceCelWidth = (float)theCelWidth;
	mSourceCelHeight = (float)theCelHeight;
	if (theMode == ANIM_SCALE_NONE) {
		theScaleX = 1.0f;
		theScaleY = 1.0f;
	}
	int aCelWidth = (int)((double)(mSourceCelWidth * theScaleX) + 0.5);
	int aCelHeight = (int)((double)(mSourceCelHeight * theScaleY) + 0.5);
	if (aCelWidth % 2 == 0)
		aCelWidth++;
	if (aCelHeight % 2 == 0)
		aCelHeight++;
	int aCelsAcross = theImage->GetWidth() / theCelWidth;
	int aCelsDown = theImage->GetHeight() / theCelHeight;
	int aSheetWidth = aCelsAcross * aCelWidth;
	int aSheetHeight = aCelsDown * aCelHeight;
	if (theMode == ANIM_SCALE_NONE && theNumCels == 1) {
		mImage = theImage;
		mCels[0].mX = 0;
		mCels[0].mY = 0;
		mCels[0].mRight = aCelWidth - 1;
		mCels[0].mBottom = aCelHeight - 1;
		mCels[0].mWidth = aCelWidth;
		mCels[0].mHeight = aCelHeight;
	} else {
		MemoryImage* aSheet = new MemoryImage(sApp);
		aSheet->Create(aSheetWidth, aSheetHeight);
		{
			Graphics aSheetG(aSheet);
			aSheetG.SetFastStretch(mScaleMode == ANIM_SCALE_FAST);
			for (int i = 0; i < theNumCels; i++) {
				int aRow = i / aCelsAcross;
				int aColumn = i % aCelsAcross;
				Rect aDestRect(aColumn * aCelWidth, aRow * aCelHeight, aCelWidth, aCelHeight);
				AnimCel& aCel = mCels[i];
				aCel.mWidth = aCelWidth;
				aCel.mHeight = aCelHeight;
				aCel.mX = aDestRect.mX;
				aCel.mRight = aCelWidth + aDestRect.mX - 1;
				aCel.mY = aDestRect.mY;
				aCel.mBottom = aDestRect.mY + aCelHeight - 1;
				aSrcRect = Rect(aColumn * theCelWidth, aRow * theCelHeight, theCelWidth, theCelHeight);
				aSheetG.DrawImage(theImage, aDestRect, aSrcRect);
			}
			// The original then makes a DDImage of the sheet's bits and commits them, and its DirectDraw surface
			// (0x47ffd0); the framework's DDImage makes its texture when first drawn
			DDImage* aScaled = new DDImage(sApp->mDDInterface);
			mImage = aScaled;
			aScaled->SetBits(aSheet->GetBits(), aSheetWidth, aSheetHeight, true);
			aScaled->CommitBits();
			delete aSheet;
			if (theImage != nullptr)
				delete theImage;
		}
	}
	mCelsDown = aCelsDown;
	mCelsAcross = aCelsAcross;
	mCelWidth = (float)aCelWidth;
	mNumCels = theNumCels;
	mCelHeight = (float)aCelHeight;
}

// 0x401970: a cel at a float position; theExtraWidth and theExtraHeight widen the source rectangle
void Anim::DrawCel(int theCel, Graphics* g, float x, float y, const SexyMatrix3& theMatrix, float theExtraWidth,
	float theExtraHeight)
{
	if (theCel < 0)
		theCel = 0;
	if (theCel >= mNumCels)
		theCel = mNumCels - 1;
	const AnimCel& aCel = mCels[theCel];
	float aWidth = (float)aCel.mWidth;
	float aHeight = (float)aCel.mHeight;
	if (aWidth < 1.0f || aHeight < 1.0f)
		return;
	Rect aSrcRect(aCel.mX, aCel.mY, (int)(aWidth + theExtraWidth), (int)(aHeight + theExtraHeight));
	g->DrawImageMatrix(mImage, theMatrix, aSrcRect, x, y);
}

// 0x401a50: the same at a whole-pixel position
void Anim::DrawCelAt(int theCel, Graphics* g, int x, int y, const SexyMatrix3& theMatrix, float theExtraWidth,
	float theExtraHeight)
{
	if (theCel < 0)
		theCel = 0;
	if (theCel >= mNumCels)
		theCel = mNumCels - 1;
	const AnimCel& aCel = mCels[theCel];
	float aWidth = (float)aCel.mWidth;
	float aHeight = (float)aCel.mHeight;
	if (aWidth < 1.0f || aHeight < 1.0f)
		return;
	Rect aSrcRect(aCel.mX, aCel.mY, (int)(aWidth + theExtraWidth), (int)(aHeight + theExtraHeight));
	g->DrawImageMatrix(mImage, theMatrix, aSrcRect, (float)x, (float)y);
}

// 0x401b30: the picture again, at the window's new scale
void Anim::Reload()
{
	Image* anImage = sApp->GetImage(mName, true);
	float aScaleX = gImageScaleX;
	float aScaleY;
	if (!mNoScale) {
		aScaleY = mUniformScale ? gImageScaleX : gImageScaleY;
	} else {
		aScaleY = 1.0f;
		aScaleX = 1.0f;
	}
	Scale(anImage, mScaleMode, aScaleX, aScaleY, (int)mSourceCelWidth, (int)mSourceCelHeight, mNumCels);
}

// 0x401c80: the registry's Anim of that picture, mode and cel count, or a new one
Anim* Anim::Get(const char* theName, int theMode, bool noScale, int theCelWidth, int theCelHeight, int theNumCels,
	bool uniformScale)
{
	if (sApp == nullptr)
		return nullptr;
	for (Anim* anAnim : sAnims) {
		if (_stricmp(anAnim->mName, theName) == 0 && anAnim->mScaleMode == theMode && anAnim->mNumCels == theNumCels) {
			anAnim->AddRef();
			return anAnim;
		}
	}
	Anim* anAnim = new Anim();
	anAnim->Load(noScale, theName, theMode, theCelWidth, theCelHeight, theNumCels, uniformScale);
	return anAnim;
}

// 0x401f90
void AnimReload::Begin()
{
	if (Anim::sAnims.empty())
		return;
	sImage = Anim::sApp->GetImage("images/reload", true);
	sProgress = 1.0f;
	sTotal = (float)(unsigned)Anim::sAnims.size();
	sNext = Anim::sAnims.begin();
	sActive = true;
	sStep = 1.0f;
}

// 0x401d80: one picture a call
void AnimReload::Step()
{
	if (sNext == Anim::sAnims.end()) {
		sActive = false;
		return;
	}
	if (!sActive)
		return;
	Anim* anAnim = *sNext;
	if (anAnim != nullptr && anAnim->mImage != nullptr) {
		delete anAnim->mImage;
		anAnim->Reload();
	}
	sProgress = sStep + sProgress;
	++sNext;
}

// 0x401e50: the reload screen, a bar of four pixels a picture with the "images/reload" caption above it
void AnimReload::Draw(Graphics* g)
{
	const int BAR_HEIGHT = 32;
	const int CAPTION_ABOVE_BAR = 96;
	g->SetColor(Color(32, 32, 0, 255));
	g->FillRect(0, 0, Anim::sApp->mWidth, Anim::sApp->mHeight);
	float aBarWidth = (float)(sTotal * 4.0);
	float aBarX = (float)(Anim::sApp->mWidth * 0.5 - aBarWidth * 0.5);
	float aBarY = (float)(0.5 * Anim::sApp->mHeight);
	g->SetColor(Color(64, 60, 0, 255));
	int aX = (int)aBarX;
	int aY = (int)aBarY;
	g->FillRect(aX, aY, (int)aBarWidth, BAR_HEIGHT);
	g->SetColor(Color(255, 245, 0, 255));
	g->FillRect(aX, aY, (int)sProgress * 4, BAR_HEIGHT);
	aY -= CAPTION_ABOVE_BAR;
	g->DrawImage(sImage, Anim::sApp->mWidth / 2 - sImage->GetWidth() / 2, aY);
}
