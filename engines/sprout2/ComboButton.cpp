#include "ComboButton.h"

#include "DrawContext.h"
#include "Image.h"
#include "Params.h"
#include "Selector.h"
#include "Surface.h"
#include "TextItem.h"

namespace engine
{

namespace
{

// The selector's children
enum ComboButtonImage
{
	IMAGE_NORMAL = 0,
	IMAGE_HIGH = 1,
	IMAGE_DOWN = 2
};

// The grey added to the normal image for a missing high image, and for a missing down image when the high one is
// missing too (the floats at 0x54ddac and 0x54dd40 (feedingfrenzy2))
const float MADE_HIGH_BRIGHTNESS = 0.3f;
const float MADE_DOWN_BRIGHTNESS = 0.4f;
// The byte value setFonts gives each colour's red, green and blue
const int RESET_COLOR_BYTE = 1;

} // namespace

ComboButton::ComboButton()
	: mText(nullptr), mImages(nullptr), mUpColor(1.0f, 1.0f, 1.0f, 1.0f), mOverColor(1.0f, 1.0f, 1.0f, 1.0f),
	  mActiveColor(1.0f, 1.0f, 1.0f, 1.0f), mHasImages(false), mHasFonts(false)
{
}

void ComboButton::updateLook()
{
	switch (mState)
	{
	case BUTTON_NORMAL:
	case BUTTON_PRESSED_OUT:
		if (mHasFonts)
		{
			mText->setFont(mUpFont.get());
			mText->setColor(mUpColor);
		}
		if (mHasImages)
			mImages->select(IMAGE_NORMAL, 0.0f, true);
		break;
	case BUTTON_OVER:
		if (mHasFonts)
		{
			mText->setFont(mOverFont.get());
			mText->setColor(mOverColor);
		}
		if (mHasImages)
			mImages->select(IMAGE_HIGH, 0.0f, true);
		break;
	case BUTTON_DOWN:
		if (mHasFonts)
		{
			mText->setFont(mActiveFont.get());
			mText->setColor(mActiveColor);
		}
		if (mHasImages)
			mImages->select(IMAGE_DOWN, 0.0f, true);
		break;
	}
}

void ComboButton::init()
{
	if (mImages == nullptr)
	{
		mImages = new Selector();
		addChild(mImages);
	}
	else
		mImages->removeAllChildren();
	if (mText == nullptr)
	{
		mText = new TextItem();
		addChild(mText);
	}
	else
		mText->setText("");
	mHasImages = false;
	mHasFonts = false;
}

void ComboButton::setFonts(Font* theUpFont, Font* theOverFont, Font* theActiveFont)
{
	mUpFont = theUpFont;
	mOverFont = theOverFont;
	mActiveFont = theActiveFont;
	if (theUpFont == nullptr)
	{
		mHasFonts = false;
		return;
	}
	mHasFonts = true;
	mText->setFont(mUpFont.get());
	mText->setColorMode(COLOR_MODE_MULTIPLY);
	addOwnState(STATE_INPUT);
	addOwnState(STATE_ANIMATED);
	computeBounds();
	float aChannel = channelOf(RESET_COLOR_BYTE);
	mUpColor.r = aChannel;
	mUpColor.g = aChannel;
	mUpColor.b = aChannel;
	mOverColor.r = aChannel;
	mOverColor.g = aChannel;
	mOverColor.b = aChannel;
	mActiveColor.r = aChannel;
	mActiveColor.g = aChannel;
	mActiveColor.b = aChannel;
	mText->setColor(mUpColor);
}

void ComboButton::setImages(Surface* theNormal, Surface* theHigh, Surface* theDown)
{
	if (theNormal == nullptr)
	{
		mHasImages = false;
		mImages->select(-1, 0.0f, true);
		return;
	}
	mHasImages = true;
	theNormal->setPivot(mPivot);
	Image* aNormalImage = new Image(theNormal);
	mImages->addChild(aNormalImage);
	mBounds = aNormalImage->getBounds();

	Image* aHighImage;
	if (theHigh != nullptr)
	{
		theHigh->setPivot(mPivot);
		aHighImage = new Image(theHigh);
	}
	else
	{
		aHighImage = new Image(theNormal);
		aHighImage->setColorMode(COLOR_MODE_ADD);
		aHighImage->setColor(MADE_HIGH_BRIGHTNESS, MADE_HIGH_BRIGHTNESS, MADE_HIGH_BRIGHTNESS);
	}
	mImages->addChild(aHighImage);

	Image* aDownImage;
	if (theDown != nullptr)
	{
		theDown->setPivot(mPivot);
		aDownImage = new Image(theDown);
	}
	else
	{
		aDownImage = new Image(theHigh != nullptr ? theHigh : theNormal);
		if (theHigh == nullptr)
		{
			aDownImage->setColorMode(COLOR_MODE_ADD);
			aDownImage->setColor(MADE_DOWN_BRIGHTNESS, MADE_DOWN_BRIGHTNESS, MADE_DOWN_BRIGHTNESS);
		}
	}
	mImages->addChild(aDownImage);

	addOwnState(STATE_INPUT);
	mImages->select(IMAGE_NORMAL, 0.0f, true);
}

void ComboButton::setText(std::string theText)
{
	mText->setText(theText);
}

void ComboButton::setAlignH(int theAlignH)
{
	mText->setAlignH(theAlignH);
}

void ComboButton::setAlignV(int theAlignV)
{
	mText->setAlignV(theAlignV);
}

void ComboButton::setTextOffset(const Vector2f& theOffset)
{
	mText->setPosition(theOffset);
}

void ComboButton::setImageOffset(const Vector2f& theOffset)
{
	mImages->setPosition(theOffset);
}

void ComboButton::setColors(const Color& theUpColor, const Color& theOverColor, const Color& theActiveColor)
{
	mUpColor = theUpColor;
	mOverColor = theOverColor;
	mActiveColor = theActiveColor;
	updateLook();
}

} // namespace engine
