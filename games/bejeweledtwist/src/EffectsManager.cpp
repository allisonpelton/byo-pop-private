// Sexy::Effect and Sexy::EffectsManager (EffectsManager.h).
//
// port: so far the parts every effect shares and type 0, the image that moves, fades and grows (the logo's sparkles).
// The other types (the board's explosions, fire, steam, lightning and the rest) come with the board; until then an
// effect of one of them logs once and only moves and fades.
#include "EffectsManager.h"

#include "Graphics.h"
#include "Image.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <SDL.h>

#include <set>

using namespace Sexy;

namespace {

void LogUnportedType(const char* theWhere, int theType)
{
	static std::set<int> aLogged;
	if (aLogged.insert(theType).second)
		SDL_Log("EffectsManager: effect type %d is not ported yet (%s)", theType, theWhere);
}

}  // namespace

Effect::Effect(int theType) : mType(theType)
{
	mColor = Color(255, 255, 255);
	switch (mType) {
	case 0:
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
