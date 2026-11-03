// ColorCycle (ColorCycle.h).
#include "ColorCycle.h"

#include "X86Math.h"

#include <algorithm>

using namespace Sexy;

namespace {

// 0x85c608 (6 colours, the count at 0x84eca4), set by the static initialiser 0x7d5156
const Color gRainbow[6] = {Color(255, 0, 0), Color(255, 255, 0), Color(0, 255, 0), Color(0, 255, 255),
	Color(0, 0, 255), Color(255, 0, 255)};

}  // namespace

ColorCycle::ColorCycle()
{
	mColors.clear();
	for (const Color& aColor : gRainbow)
		mColors.push_back(aColor);
	mBrighten = 0;
	mLoop = true;
	mAlpha = 1.0f;
	Reset();
}

void ColorCycle::Reset()
{
	mPos = 0;
	mSpeed = 1.0f;
}

void ColorCycle::SetPosition(float thePos)
{
	while (thePos >= 1.0f)
		thePos = thePos - 1.0f;
	mPos = thePos;
	Update();
}

void ColorCycle::Update()
{
	if (mSpeed == 0.0f)
		return;
	const Color* aColor;
	if (mColors.size() == 0) {
		mColor = Color(0, 0, 0, 0);
		return;
	}
	int anIndex;
	if (mColors.size() == 1) {
		anIndex = 0;
	} else {
		mPos = (float)((double)mSpeed * 0.009999999776482582 + mPos);
		if (mPos >= 1.0f) {
			if (!mLoop) {
				mPos = 1.0f;
				anIndex = (int)mColors.size() - 1;
				aColor = &mColors[anIndex];
				mColor = *aColor;
				return;
			}
			do {
				mPos = mPos - 1.0f;
			} while (mPos >= 1.0f);
		}
		float aPlace = (float)(unsigned int)mColors.size() * mPos;
		int aFrom = Ftol(aPlace);
		int aTo = (int)((unsigned int)(aFrom + 1) % (unsigned int)mColors.size());
		if (!mLoop && aTo < aFrom)
			aTo = aFrom;
		const Color& a = mColors[aFrom];
		const Color& b = mColors[aTo];
		float aFrac = aPlace - (float)aFrom;
		int aRed = Ftol((double)a.mRed * (1.0 - aFrac) + (double)aFrac * b.mRed);
		mColor.mRed = aRed;
		int aGreen = Ftol((double)a.mGreen * (1.0 - aFrac) + (double)aFrac * b.mGreen);
		mColor.mGreen = aGreen;
		int aBlue = Ftol((double)a.mBlue * (1.0 - aFrac) + (double)aFrac * b.mBlue);
		mColor.mBlue = aBlue;
		mColor.mAlpha = Ftol(((double)aFrac * b.mAlpha + (double)a.mAlpha * (1.0 - aFrac)) * mAlpha);
		if (mBrighten == 0.0f)
			return;
		int aBrighten = Ftol((double)mBrighten * 255.0);
		if (aBrighten < 1) {
			mColor.mRed = std::max(aRed + aBrighten, 0);
			mColor.mGreen = std::max(aGreen + aBrighten, 0);
			mColor.mBlue = std::max(aBlue + aBrighten, 0);
		} else {
			mColor.mRed = std::min(aRed + aBrighten, 255);
			mColor.mGreen = std::min(aGreen + aBrighten, 255);
			mColor.mBlue = std::min(aBlue + aBrighten, 255);
		}
		return;
	}
	aColor = &mColors[anIndex];
	mColor = *aColor;
}
