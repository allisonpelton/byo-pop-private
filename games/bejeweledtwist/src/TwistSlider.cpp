// Sexy::TwistSlider (TwistSlider.h).
#include "TwistSlider.h"

#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

TwistSlider::TwistSlider(Image* theTrackImage, Image* theThumbImage, int theId, SliderListener* theListener) :
	Slider(theTrackImage, theThumbImage, theId, theListener)
{
	mClip = false;
}

TwistSlider::~TwistSlider()
{
}

void TwistSlider::Draw(Graphics* g)
{
	if (mTrackImage != nullptr) {
		int aCelWidth;
		if (!mHorizontal)
			aCelWidth = mTrackImage->GetWidth();
		else
			aCelWidth = mTrackImage->GetWidth() / 3;
		int aCelHeight;
		if (!mHorizontal)
			aCelHeight = mTrackImage->GetHeight() / 3;
		else
			aCelHeight = mTrackImage->GetHeight();

		if (!mHorizontal) {
			g->DrawImage(mTrackImage, 0, 0, Rect(0, 0, aCelWidth, aCelHeight));
			Graphics aClipG(*g);
			aClipG.ClipRect(0, aCelHeight, aCelWidth, mHeight - aCelHeight * 2);
			int i = 0;
			int aY = aCelHeight;
			while (i < (mHeight - aCelHeight - 1) / aCelHeight) {
				aClipG.DrawImage(mTrackImage, 0, aY, Rect(0, aCelHeight, aCelWidth, aCelHeight));
				i++;
				aY += aCelHeight;
			}
			g->DrawImage(mTrackImage, 0, mHeight - aCelHeight, Rect(0, aCelHeight * 2, aCelWidth, aCelHeight));
		} else {
			int aTrackY = (mHeight - aCelHeight) / 2;
			g->DrawImage(mTrackImage, 0, aTrackY, Rect(0, 0, aCelWidth, aCelHeight));
			Graphics aClipG(*g);
			aClipG.ClipRect(aCelWidth, aTrackY, mWidth - aCelWidth * 2, aCelHeight);
			int i = 0;
			int aX = aCelWidth;
			while (i < (mWidth - aCelWidth - 1) / aCelWidth) {
				aClipG.DrawImage(mTrackImage, aX, aTrackY, Rect(aCelWidth, 0, aCelWidth, aCelHeight));
				i++;
				aX += aCelWidth;
			}
			g->DrawImage(mTrackImage, mWidth - aCelWidth, aTrackY, Rect(aCelWidth * 2, 0, aCelWidth, aCelHeight));

			// The fill up to the value: the music slider's for both sliders
			Image* aFill = IMAGE_MAINMENU_SLIDERBARFILLMUSIC;
			int aFillWidth = Ftol((double)aFill->mWidth * mVal);
			Rect aSrcRect(0, 0, aFillWidth, aFill->mHeight);
			float aLeft = (float)((double)gApp->mScale * 6.0);
			int aDestX = Ftol((double)aLeft + 0.0 + 0.0);
			float aTop = (float)((double)gApp->mScale * 5.0);
			int aDestY = Ftol((double)aTop + (double)aTrackY + 0.0);
			g->DrawImage(aFill, Rect(aDestX, aDestY, aFillWidth, aFill->mHeight), aSrcRect);
		}
	}

	int aCel = mDragging ? 1 : 0;
	if (mHorizontal) {
		if (mThumbImage != nullptr) {
			int aY = (mHeight - mThumbImage->GetCelHeight()) / 2;
			int aX = Ftol((double)(mWidth - mThumbImage->GetCelWidth()) * mVal);
			g->DrawImageCel(mThumbImage, aX, aY, aCel);
		}
		return;
	}
	if (mThumbImage == nullptr)
		return;
	int aY = Ftol((double)(mHeight - mThumbImage->GetCelHeight()) * mVal);
	int aX = (mWidth - mThumbImage->GetCelWidth()) / 2;
	g->DrawImageCel(mThumbImage, aX, aY, aCel);
}
