// Sexy::Effect and Sexy::EffectsManager (EffectsManager.h).
//
// port: so far the parts every effect shares and type 0, the image that moves, fades and grows (the logo's sparkles).
// The other types (the board's explosions, fire, steam, lightning and the rest) come with the board; until then an
// effect of one of them logs once and only moves and fades.
#include "EffectsManager.h"

#include "Graphics.h"
#include "GraphicsRecorder.h"
#include "Image.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <SDL.h>

#include <cmath>
#include <set>

using namespace Sexy;

namespace {

void LogUnportedType(const char* theWhere, int theType)
{
	static std::set<int> aLogged;
	if (aLogged.insert(theType).second)
		SDL_Log("EffectsManager: effect type %d is not ported yet (%s)", theType, theWhere);
}

// Clamped away from zero: a shard seen edge on still shows a quarter of its face
float ShardSide(float theAngle)
{
	float aSide = sinf(theAngle);
	if (0.0f < aSide && aSide < 0.25f)
		aSide = 0.25f;
	if (aSide < 0.0f && -0.25f < aSide)
		aSide = -0.25f;
	return aSide;
}

// 0x6f6451's type 16: a shard tumbling about two axes, its outline glinting as it faces the screen, with a sparkle
void DrawShard(Graphics* g, Effect* theEffect, int theUpdateCnt)
{
	float aFade = theEffect->mAlpha;
	if (0.0f < aFade)
		aFade = sqrtf(aFade);
	Color aColor = theEffect->mColor;
	aColor.mAlpha = Ftol((double)aFade * 64.0);
	g->SetColorizeImages(true);
	g->SetColor(aColor);
	float aScale = theEffect->mScale;
	Transform aTransform;
	aTransform.RotateRad(theEffect->mAngle);
	float aSideX = ShardSide(theEffect->mUnknown9C);
	float aSideY = ShardSide(theEffect->mUnknownA0);
	aTransform.Scale((float)((double)aSideX * 1.4 * (double)aScale), (float)((double)aScale * (double)aSideY * 1.4));
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	float aGrow = (float)((double)theEffect->mScale * 1.15);
	aTransform.Scale(aGrow, aGrow);
	aColor.mAlpha = Ftol((double)aFade * 255.0);
	g->SetColor(aColor);
	// port: the original picks the shard's cel from the manager's update count times the effect's address, so the
	// shapes flicker differently here (a deviation without effect on play)
	int aCel = (int)((unsigned int)theUpdateCnt * (unsigned int)(uintptr_t)theEffect) & 7;
	int aWidth = IMAGE_SM_SHARDS->GetCelWidth();
	Rect aSrc(aWidth * aCel, 0, aWidth, IMAGE_SM_SHARDS->GetCelHeight());
	gGraphicsRecorder->DrawImageTransform(g, IMAGE_SM_SHARDS, aTransform, aSrc, (float)Ftol(theEffect->mX), (float)Ftol(theEffect->mY), false);
	float aFacing = sinf(theEffect->mUnknownA0);
	aFacing = fabsf(sinf(theEffect->mUnknown9C) * aFacing);
	float aDim = 2.0f - theEffect->mScale;
	if (0.2f <= aDim) {
		if (1.0f < aDim)
			aDim = 1.0f;
	} else {
		aDim = 0.2f;
	}
	g->SetColor(Color(0xffffff, Ftol((double)aDim * 255.0 * std::pow((double)aFacing, 5.0) * (double)aFade)));  // 0x670a49
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	aWidth = IMAGE_SM_SHARDS_OUTLINE->GetCelWidth();
	aSrc = Rect(aWidth * aCel, 0, aWidth, IMAGE_SM_SHARDS_OUTLINE->GetCelHeight());
	gGraphicsRecorder->DrawImageTransform(g, IMAGE_SM_SHARDS_OUTLINE, aTransform, aSrc, (float)Ftol(theEffect->mX),
		(float)Ftol(theEffect->mY), false);
	g->SetColor(Color(0xffffff, Ftol(std::pow((double)aFacing, 7.5) * 255.0 * (double)aFade)));
	int aHeight = Ftol((double)IMAGE_SPARKLE->GetCelHeight() * (double)aScale);
	aWidth = Ftol((double)IMAGE_SPARKLE->GetCelWidth() * (double)aScale);
	int aY = Ftol((double)Ftol(theEffect->mY) - (double)(IMAGE_SPARKLE->GetCelHeight() / 2) * (double)aScale);
	int aX = Ftol((double)Ftol(theEffect->mX) - (double)(IMAGE_SPARKLE->GetCelWidth() / 2) * (double)aScale);
	gGraphicsRecorder->DrawImageCel(g, IMAGE_SPARKLE, Rect(aX, aY, aWidth, aHeight), aCel);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);
}

}  // namespace

Effect::Effect(int theType) : mType(theType)
{
	mColor = Color(255, 255, 255);
	switch (mType) {
	case 0:
		break;
	case 11:  // the gates' sparks
	case 12:  // the sparks where the points' smoke reaches the tube (0x74f457)
		mGravity = (float)(gApp->mScale * 0.004999999888241291);
		mDX = (float)(RandSigned() * gApp->mScale);
		mDY = (float)(RandSigned() * gApp->mScale);
		mScale = 0.2f;
		mAlpha = 1.0f;
		mDAlpha = -0.005f;
		// With Direct3D a random angle: RandSigned() * pi
		mAngle = 0;
		break;
	case 16:  // a gem's shards (Board::BlowUpPiece)
		mFrame = (int)(Rand() % 40);
		mAlpha = 1.0f;
		mUpdateDiv = 0;
		mDAlpha = (float)(-0.004999999888241291 - (double)fabsf(RandSigned()) * 0.009999999776482582);
		mUnknown38 = 1.0f;
		break;
	case 26:  // smoke (Board::BlowUpPiece)
		mImage = IMAGE_FX_STEAM;
		mGravity = (float)((double)gApp->mScale * -0.005);
		mAngle = (float)((double)RandSigned() * 3.1415927410125732);
		mDAngle = (float)((double)RandSigned() * 0.03999999910593033);
		mAlpha = 0.85f;
		mDAlpha = 0;
		mUnknown9C = 0.5f;
		mUnknownA0 = -0.02f;
		mUnknownA4 = 0.95f;
		break;
	case 17:  // the hint's sparkles (Board::UpdateHint)
		mUpdateDiv = (int)(Rand() % 4) + 3;
		mDX = (float)((double)gApp->mScale * ((double)(float)(int)(Rand() % 20) * 0.1 - 1.0));
		mDY = (float)((double)gApp->mScale * (double)(float)(int)(Rand() % 50) * 0.1);
		break;
	default:
		LogUnportedType("constructor", mType);
		break;
	}
	mDeleteMe = false;
}

Effect::~Effect()
{
	if (mOwnsImage && mImage != nullptr)
		delete mImage;
}

EffectsManager::EffectsManager()
{
	mHasAlpha = true;
	mX = 0;
	mY = 0;
	mClip = false;
	mMouseVisible = false;
}

EffectsManager::~EffectsManager()
{
	for (auto& anEntry : mEffects)
		delete anEntry.second;
	mEffects.clear();
}

Effect* EffectsManager::AllocEffect(int theType)
{
	return new Effect(theType);
}

void EffectsManager::AddEffect(Effect* theEffect)
{
	mEffects.insert(std::make_pair(theEffect->mType, theEffect));
	theEffect->mFXManager = this;
}

void EffectsManager::Update()
{
	Widget::Update();
	mIs3D = gApp->Is3DAccelerated();
	mWidth = gApp->mWidth;
	mHeight = gApp->mHeight;
	// The heat waves of the list at +0x94 come here: Direct3D only

	auto anIt = mEffects.begin();
	while (anIt != mEffects.end()) {
		Effect* anEffect = anIt->second;
		int aType = anEffect->mType;
		if (aType == 44) {
			anEffect->Update();
			++anIt;
			continue;
		}
		int aDiv = anEffect->mUpdateDiv != 0 ? anEffect->mUpdateDiv : 1;
		float aDivF = (float)aDiv;
		anEffect->mX = anEffect->mDX / aDivF + anEffect->mX;
		anEffect->mY = anEffect->mDY / aDivF + anEffect->mY;
		if (mUpdateCnt % aDiv == 0) {
			anEffect->mDX = anEffect->mDXScalar * anEffect->mDX;
			float aDY = anEffect->mGravity + anEffect->mDY;
			anEffect->mDY = aDY * anEffect->mDYScalar;
			if ((anEffect->mFlags & Effect::FLAG_DELAY) == 0 || anEffect->mDelay <= 0.0f) {
				anEffect->mAlpha = anEffect->mDAlpha + anEffect->mAlpha;
				anEffect->mScale = anEffect->mDScale + anEffect->mScale;
			} else {
				float aDelay = (float)(anEffect->mDelay - 0.009999999776482582);
				anEffect->mDelay = aDelay;
				if (aDelay <= 0.0f)
					anEffect->mDelay = 0;
			}
		}

		switch (aType) {
		case 0:
			if (anEffect->mUpdateDiv != 0 && mUpdateCnt % anEffect->mUpdateDiv != 0)
				break;
			if (anEffect->mIsCyclingColor) {
				anEffect->mCurHue = (anEffect->mCurHue + 8) % 256;
				anEffect->mColor = Color((int)gApp->HSLToRGB(anEffect->mCurHue, 255, 128));
			}
			if (anEffect->mY < 0.0f || (float)mHeight <= anEffect->mY || anEffect->mX < 0.0f ||
				(float)mWidth <= anEffect->mX)
				anEffect->mDeleteMe = true;
			anEffect->mAngle = anEffect->mDAngle + anEffect->mAngle;
			break;
		case 11:
			if (anEffect->mScale > 0.85f)
				anEffect->mScale = 0.85f;
			break;
		case 12: {
			anEffect->mDY = anEffect->mGravity + anEffect->mDY;
			// With Direct3D the angle also turns by mDY * 0.02
			float aScale = (float)((double)anEffect->mScale + 0.05);
			anEffect->mScale = aScale;
			if (0.5f < aScale)
				anEffect->mScale = 0.5f;
			break;
		}
		case 16:
			if (mUpdateCnt % 2 == 0)
				anEffect->mFrame = (anEffect->mFrame + 1) % 40;
			anEffect->mDX = anEffect->mDX * anEffect->mUnknown38;
			anEffect->mDY = anEffect->mUnknown38 * anEffect->mDY;
			anEffect->mAngle = anEffect->mDAngle + anEffect->mAngle;
			anEffect->mUnknown9C = anEffect->mUnknownA4 + anEffect->mUnknown9C;
			anEffect->mUnknownA0 = anEffect->mUnknownA8 + anEffect->mUnknownA0;
			break;
		case 26: {
			// it slows, and once slow it fades
			float aSpeed = sqrtf(anEffect->mDX * anEffect->mDX + anEffect->mDY * anEffect->mDY);
			anEffect->mDX = anEffect->mDX * anEffect->mUnknownA4;
			if (anEffect->mUnknown9C < aSpeed)
				anEffect->mDY = anEffect->mUnknownA4 * anEffect->mDY;
			else
				anEffect->mDAlpha = anEffect->mUnknownA0;
			anEffect->mAngle = anEffect->mDAngle + anEffect->mAngle;
			anEffect->mScale = anEffect->mDScale + anEffect->mScale;
			break;
		}
		case 17:
			anEffect->mDX = (float)((double)anEffect->mDX * 0.98);
			anEffect->mDY = (float)((double)anEffect->mDY * 0.98);
			if (mUpdateCnt % anEffect->mUpdateDiv == 0)
				anEffect->mFrame = (anEffect->mFrame + 1) % 40;
			if (anEffect->mFrame == 14)
				anEffect->mDeleteMe = true;
			if (anEffect->mY <= (float)mHeight && 0.0f <= anEffect->mY && anEffect->mX <= (float)mWidth &&
				0.0f <= anEffect->mX) {
				if (!(0.0f < anEffect->mAlpha))
					anEffect->mDeleteMe = true;
			} else {
				anEffect->mDeleteMe = true;
			}
			break;
		default:
			LogUnportedType("Update", aType);
			break;
		}

		if (anEffect->mMinScale <= anEffect->mScale) {
			if (anEffect->mMaxScale < anEffect->mScale) {
				anEffect->mScale = anEffect->mMaxScale;
				if ((anEffect->mFlags & Effect::FLAG_SCALE_BOUNCE) != 0)
					anEffect->mDScale = -anEffect->mDScale;
			}
		} else {
			anEffect->mDeleteMe = true;
			anEffect->mScale = anEffect->mMinScale;
		}
		if (anEffect->mAlpha <= anEffect->mMaxAlpha) {
			if (anEffect->mAlpha <= 0.0f)
				anEffect->mDeleteMe = true;
		} else {
			anEffect->mAlpha = anEffect->mMaxAlpha;
			if ((anEffect->mFlags & Effect::FLAG_ALPHA_BOUNCE) != 0)
				anEffect->mDAlpha = -anEffect->mDAlpha;
			else
				anEffect->mDeleteMe = true;
		}

		if (!anEffect->mDeleteMe) {
			++anIt;
		} else {
			delete anEffect;
			anIt = mEffects.erase(anIt);
		}
	}
}

void EffectsManager::DrawEffects(Graphics* g, bool theOverlay)
{
	// With Direct3D the manager may first switch off the z buffer (+0xa1)
	for (auto anIt = mEffects.begin(); anIt != mEffects.end(); ++anIt) {
		Effect* anEffect = anIt->second;
		if (anEffect->mOverlay != theOverlay)
			continue;
		if (anEffect->mPieceId != -1) {
			// Drawn relative to the board's piece (app +0x7e4): comes with the board. Without a board the original
			// skips the effect too.
			continue;
		}
		if (anEffect->mType == 44) {
			anEffect->Draw(g);
			continue;
		}
		float anAlpha = anEffect->mAlpha <= 1.0f ? anEffect->mAlpha : 1.0f;
		float anOpacity = mAlpha * anAlpha;
		switch (anEffect->mType) {
		case 0: {
			g->SetColorizeImages(true);
			Color aColor = anEffect->mColor;
			aColor.mAlpha = Ftol(anOpacity * 255.0);
			g->SetColor(aColor);
			g->SetDrawMode(anEffect->mIsAdditive ? Graphics::DRAWMODE_ADDITIVE : Graphics::DRAWMODE_NORMAL);
			if (anEffect->mImage != nullptr) {
				DrawImageCentered(g, anEffect->mImage, anEffect->mImage->GetCelRect(anEffect->mFrame), anEffect->mX,
					anEffect->mY, (double)anEffect->mAngle, anEffect->mScale, anEffect->mScale);
				g->SetColorizeImages(false);
			}
			break;
		}
		case 11: {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			Color aColor = anEffect->mColor;
			aColor.mAlpha = Ftol(anOpacity * 255.0);
			g->SetColor(aColor);
			float aScale = (float)std::pow((double)anOpacity, (double)0.65f) * anEffect->mScale;  // 0x670a26
			DrawImageCentered(g, IMAGE_GATEPARTICLE, anEffect->mX, anEffect->mY, (double)anEffect->mAngle, aScale,
				aScale);
			break;
		}
		case 12: {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, Ftol(anOpacity * 255.0)));
			float aScale = anEffect->mScale * anOpacity;
			DrawImageCentered(g, IMAGE_GEM_FRUIT_SPARK, anEffect->mX, anEffect->mY, (double)anEffect->mAngle, aScale,
				aScale);
			break;
		}
		case 16:
			DrawShard(g, anEffect, mUpdateCnt);
			break;
		case 26: {
			g->SetDrawMode(anEffect->mIsAdditive ? Graphics::DRAWMODE_ADDITIVE : Graphics::DRAWMODE_NORMAL);
			g->SetColorizeImages(true);
			Color aColor = anEffect->mColor;
			aColor.mAlpha = Ftol(anOpacity * 255.0);
			g->SetColor(aColor);
			DrawImageCentered(g, anEffect->mImage, anEffect->mImage->GetCelRect(anEffect->mFrame), anEffect->mX,
				anEffect->mY, (double)anEffect->mAngle, anEffect->mScale, anEffect->mScale);
			break;
		}
		case 17: {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			anEffect->mColor.mAlpha = Ftol(anOpacity * 255.0);
			g->SetColor(anEffect->mColor);
			int aWidth = Ftol((double)IMAGE_SPARKLE->GetCelWidth() * (double)anEffect->mScale);
			int aHeight = Ftol((double)IMAGE_SPARKLE->GetCelHeight() * (double)anEffect->mScale);
			Rect aRect(Ftol(anEffect->mX) - aWidth / 2, Ftol(anEffect->mY) - aHeight / 2, aWidth, aHeight);
			gGraphicsRecorder->DrawImageCel(g, IMAGE_SPARKLE, aRect, anEffect->mFrame);
			g->SetColorizeImages(false);
			break;
		}
		default:
			LogUnportedType("Draw", anEffect->mType);
			break;
		}
	}
}

void EffectsManager::Draw(Graphics* g)
{
	// With Direct3D the board's distortion target (0x85aef4) is set around the drawing
	DrawEffects(g, false);
	if (mWidgetManager != nullptr)
		DeferOverlay(0x31);
}

void EffectsManager::DrawOverlay(Graphics* g)
{
	DrawEffects(g, true);
	// The heat waves and the distortion pass (0x6f7ab4, 0x6f55b0, flags +0xb4/+0xb5) are Direct3D only
}
