// Sexy::Logo (Logo.h).
#include "Logo.h"

#include "EffectsManager.h"
#include "GameText.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Res.h"
#include "TwistD3D.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <cstdint>

using namespace Sexy;

Logo::Logo(EffectsManager* theFXManager) : mFXManager(theFXManager)
{
	int aRand = Rand();
	mShineDelay = aRand % 100 + 100;
	mLogoImage = IMAGE_BEJTWISTLOGO;
	// The sparkles appear on the white, opaque pixels
	ulong* aBits = static_cast<MemoryImage*>(mLogoImage)->GetBits();
	if (aBits != nullptr) {
		for (int y = 0; y < mLogoImage->mHeight; y++) {
			for (int x = 0; x < mLogoImage->mWidth; x++) {
				uint32_t aPixel = (uint32_t)*aBits++;
				if ((aPixel >> 16) % 255 + (aPixel >> 8) % 255 + aPixel % 255 > 650 && (aPixel & 0xff000000) > 0xfa000000)
					mSparklePoints.push_back(Point(x, y));
			}
		}
	}
	mMouseVisible = false;
	Reset(true);
}

Logo::~Logo()
{
}

void Logo::Reset(bool theShow)
{
	mDrop.SetCurve(GTS(0x7e2d98));
	if (!theShow) {
		mDrop.mInVal = mDrop.mInMax;
		mAlpha = 0;
	} else {
		mAlpha = 1.0f;
	}
	mLogoTop = Ftol((double)gApp->mScale * 30.0);
}

void Logo::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	mAlpha = 1.0f;
}

void Logo::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
}

void Logo::Update()
{
	// Once the title is in place, sparkles now and then (with Direct3D also during a game)
	if (!mDrop.IncInVal()) {
		int aLeft = mWidth / 2 - mLogoImage->mWidth / 2;
		if (mAlpha == 1.0f && (gD3DInterface != nullptr || gApp->mBoard == nullptr)) {
			if (Rand() % 25 == 0) {
				Effect* anEffect = mFXManager->AllocEffect(0);
				float anAngle = std::fabs((float)(RandSigned() * 6.2831854820251465));
				unsigned int aCount = (unsigned int)mSparklePoints.size();
				unsigned int anIndex = (unsigned int)Rand() % aCount;
				const Point& aPoint = mSparklePoints[anIndex];
				anEffect->mImage = IMAGE_GEM_FRUIT_SPARK;
				anEffect->mX = (float)(aPoint.mX + aLeft);
				anEffect->mIsAdditive = true;
				anEffect->mFlags = Effect::FLAG_SCALE_BOUNCE;
				anEffect->mY = (float)(mLogoTop + aPoint.mY);
				anEffect->mDX = 0;
				anEffect->mDY = 0;
				anEffect->mAngle = anAngle;
				anEffect->mDScale = 0.025f;
				anEffect->mGravity = 0;
				anEffect->mAlpha = 1.0f;
				anEffect->mDAlpha = 0;
				anEffect->mScale = 0.01f;
				anEffect->mMaxScale = 1.0f;
				mFXManager->AddEffect(anEffect);
			}
			if (mSettleFade < 1.0f) {
				float aFade = (float)(mSettleFade + 0.009999999776482582);
				if (1.0f < aFade)
					aFade = 1.0f;
				mSettleFade = aFade;
			}
			if (mFlashRise < 1.0f) {
				float aFade = (float)(mFlashRise + 0.009999999776482582);
				mFlashRise = aFade;
				if (aFade >= 1.0f) {
					mFlashRise = 1.0f;
					mFlashFall = 1.0f;
				}
			}
			if (0.0f < mFlashFall) {
				float aFade = (float)(mFlashFall - 0.009999999776482582);
				if (aFade < 0.0f)
					aFade = 0;
				mFlashFall = aFade;
			}
		}
	}

	// The shine runs every 300 to 499 updates (it is drawn only with Direct3D)
	if (mShine.mInVal == 0.0 || mShine.mInVal == 1.0) {
		if (0 < mShineDelay)
			mShineDelay--;
	}
	if (!mShine.IncInVal() && mShineDelay == 0) {
		mShine.SetCurve(GTS(0x7e2df4));
		mShineGreen.SetCurve(GTS(0x7e2dd8), &mShine);
		mShineBlue.SetCurve(GTS(0x7e2db8), &mShine);
		mShineDelay = Rand() % 200 + 300;
	}
}

void Logo::Draw(Graphics* g)
{
	if (mAlpha == 0.0f)
		return;
	int aStartY = Ftol((double)gApp->mScale * 500.0);
	int anEndY = Ftol((double)gApp->mScale * 170.0);
	int aY = Ftol((double)(anEndY - aStartY) * mDrop.GetOutVal() + aStartY);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol((double)mAlpha * 255.0)));
	if (gD3DInterface == nullptr) {
		DrawImageScaledCentered(g, mLogoImage, (float)(mWidth / 2), (float)aY, ALIGN_CENTER, 1.0f, 1.0f);
	} else {
		// With Direct3D the title is drawn mLogoScale large, with the shine's pixel shader while the shine runs
		if (mShine.mInVal != 0.0) {
			TwistShader* aShader = gTwistShaders[TWISTSHADER_LOGO];
			gD3DInterface->SetPixelShader(aShader);  // slot 0xd8
			float aConstant[4] = {(float)mShine.GetOutVal(), (float)mShineGreen.GetOutVal(), (float)mShineBlue.GetOutVal(),
				0.0f};
			gD3DInterface->SetShaderConstant(aShader, 0, aConstant);  // slot 0x108
			gD3DInterface->SetTexture(1, IMAGE_BEJTWISTLOGO_NORMAL);  // slot 0xd4
		}
		DrawImageScaledCentered(g, mLogoImage, (float)(mWidth / 2), (float)aY, mLogoScale, mLogoScale);  // 0x757751
		gD3DInterface->SetPixelShader(nullptr);
	}
	mLogoTop = aY - mLogoImage->mHeight / 2;
	g->SetColor(Color(255, 255, 255, Ftol((double)mAlpha * 255.0)));
	if (mTrademarkAlpha.GetOutVal() > 0.0) {
		g->SetColor(Color(255, 255, 255, Ftol(mTrademarkAlpha.GetOutVal() * 255.0)));
		int aDY = Ftol((double)gApp->mScale * -12.0);
		int aDX = Ftol((double)gApp->mScale * -325.0);
		g->DrawImage(IMAGE_LOGOTM, mWidth / 2 - aDX, aY - aDY);
	}
}
