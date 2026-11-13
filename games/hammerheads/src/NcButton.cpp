#include "NcButton.h"

#include <cmath>

#include "Application.h"
#include "Color.h"
#include "DrawUtil.h"
#include "Font.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Res.h"

using namespace Sexy;

// 0x41ca80
NcButton::NcButton(Image* theComponentImage, int theId, ButtonListener* theListener) :
	DialogButton(theComponentImage, theId, theListener)
{
	mDarkensWithFade = false;
	mUnused140 = -1000;
	mSound = -1;
	mShown = true;
}

// port: the destructor (the original's is compiler made)
NcButton::~NcButton()
{
}

// 0x41cb00: DialogButton::Draw with the strip's three cels, the darkening and a label that sits a little higher
void NcButton::Draw(Graphics* g)
{
	Graphics aG(*g);
	if (mShown)
	{
		if (mDarkensWithFade && gApp->mFade > 0.0)
		{
			aG.SetColorizeImages(true);
			int aValue = ToInt(255.0 - sin(gApp->mFade * 1.5707963267948966) * 180.0);
			aG.SetColor(Color(aValue, aValue, aValue));
		}

		if (mComponentImage == nullptr)
			ButtonWidget::Draw(&aG);
		else
		{
			bool isDown = IsButtonDown();
			if (mNormalRect.mWidth == 0)
			{
				if (isDown)
					aG.Translate(mTranslateX, mTranslateY);
				aG.DrawImageBox(Rect(0, 0, mWidth, mHeight), mComponentImage);
			}
			else
			{
				const Rect& aCel = IsButtonDown() ? mDownRect : (mIsOver ? mOverRect : mNormalRect);
				aG.DrawImage(mComponentImage, 0, 0, aCel);
				if (isDown)
					aG.Translate(mTranslateX, mTranslateY);
			}

			Color aBase(255, 255, 255, 255);
			bool aWasColorizing = false;
			if (aG.GetColorizeImages())
			{
				aBase = aG.GetColor();
				aWasColorizing = true;
			}
			aG.SetColorizeImages(true);

			Color aLabel = mColors[mIsOver ? COLOR_LABEL_HILITE : COLOR_LABEL];
			aG.SetColor(Color((aBase.mRed * aLabel.mRed) / 255, (aBase.mGreen * aLabel.mGreen) / 255,
				(aBase.mBlue * aLabel.mBlue) / 255));

			if (mFont != nullptr)
			{
				aG.SetFont(mFont);
				int aX = (mWidth - mFont->StringWidth(mLabel)) / 2;
				int aY = mHeight - mFont->GetAscent() / 6 - mFont->GetAscentPadding();
				aY = (aY + mFont->GetAscent() - 1) / 2;
				aG.DrawString(mLabel, mTextOffsetX + aX, mTextOffsetY + aY);
			}
			if (!aWasColorizing)
				aG.SetColorizeImages(false);
			if (isDown)
				aG.Translate(-1, -1);
		}
		aG.SetColorizeImages(false);
	}
}

// 0x41ca20
void NcButton::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	DialogButton::MouseDown(x, y, theBtnNum, theClickCount);
	if (mSound > -1)
		gApp->PlaySample(mSound);
}

// 0x41c9b0: a point of the button's picture is on the button where the picture is not transparent
bool NcButton::IsPointVisible(int x, int y)
{
	if (mButtonImage == nullptr)
		return true;
	if (x > -1 && y > -1 && x < mButtonImage->GetWidth() && y < mButtonImage->GetHeight())
	{
		MemoryImage* anImage = static_cast<MemoryImage*>(mButtonImage);
		return (anImage->GetBits()[mButtonImage->GetWidth() * y + x] & 0xff000000) != 0;
	}
	return false;
}

// 0x41d120
void Sexy::SetupButtonCels(ButtonWidget* theButton, int theNumCels)
{
	DialogButton* aButton = static_cast<DialogButton*>(theButton);
	aButton->mTranslateX = 0;
	aButton->mTranslateY = 2;
	int aHeight = aButton->mComponentImage->GetHeight();
	int aCelWidth = aButton->mComponentImage->GetWidth() / theNumCels;
	aButton->mNormalRect = Rect(0, 0, aCelWidth, aHeight);
	aButton->mTextOffsetX = 0;
	aButton->mTextOffsetY = -2;
	aButton->mOverRect = Rect(aCelWidth, 0, aCelWidth, aHeight);
	aButton->mDownRect = Rect(aCelWidth * 2, 0, aCelWidth, aHeight);

	aButton->SetFont(FONT_PTBANANASPLIT27BUTTONS);
	aButton->SetColor(ButtonWidget::COLOR_LABEL, Color(255, 255, 255, 255));
	aButton->SetColor(ButtonWidget::COLOR_LABEL_HILITE, Color(255, 255, 255, 255));
	aButton->mHasAlpha = true;
	aButton->mHasTransparencies = true;
	if (aButton->mWidth == 0)
		aButton->Resize(aButton->mX, aButton->mY, aButton->mFont->StringWidth(aButton->mLabel), aButton->mComponentImage->GetHeight());
}

// 0x41d450
NcButton* Sexy::MakeNcButton(int theId, ButtonListener* theListener, const std::string& theLabel, Image* theImage, int theNumCels)
{
	bool aDefault = theImage == nullptr;
	if (aDefault)
		theImage = IMAGE_DIALOG_BUTTON;

	NcButton* aButton = new NcButton(theImage, theId, theListener);
	aButton->mLabel = theLabel;
	aButton->mSound = SOUND_UI_BUTTON_CLICK;
	if (!aDefault)
		aButton->Resize(aButton->mX, aButton->mY, theImage->GetWidth() / theNumCels, theImage->GetHeight());
	SetupButtonCels(aButton, theNumCels);
	if (!aDefault)
	{
		aButton->mButtonImage = theImage;
		aButton->mComponentImage = nullptr;
	}
	return aButton;
}
