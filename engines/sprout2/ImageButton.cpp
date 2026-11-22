#include "ImageButton.h"

#include "DrawContext.h"
#include "Image.h"
#include "Selector.h"
#include "Surface.h"

namespace engine
{

namespace
{

// The selector's children
enum ImageButtonImage
{
	IMAGE_NORMAL = 0,
	IMAGE_HIGH = 1,
	IMAGE_DOWN = 2
};

// A flash's fade between the images (the float at 0x54ddf8 (feedingfrenzy2))
const float FLASH_FADE_SECONDS = 0.2f;
// The grey added to the normal image for a missing high image, and for a missing down image when the high one is
// missing too (the floats at 0x54ddac and 0x54dd40 (feedingfrenzy2))
const float MADE_HIGH_BRIGHTNESS = 0.3f;
const float MADE_DOWN_BRIGHTNESS = 0.4f;

} // namespace

void ImageButton::updateLook()
{
	switch (mState)
	{
	case BUTTON_NORMAL:
	case BUTTON_PRESSED_OUT:
		mImages->select(IMAGE_NORMAL, 0.0f, true);
		break;
	case BUTTON_OVER:
		mImages->select(IMAGE_HIGH, 0.0f, true);
		break;
	case BUTTON_DOWN:
		mImages->select(IMAGE_DOWN, 0.0f, true);
		break;
	}
}

void ImageButton::applyFlash(float theFlash)
{
	if (theFlash <= 0.0f)
		mImages->select(IMAGE_HIGH, FLASH_FADE_SECONDS, true);
	else if (1.0f <= theFlash)
		mImages->select(IMAGE_NORMAL, FLASH_FADE_SECONDS, false);
}

bool ImageButton::setImages(Surface* theNormal, Surface* theHigh, Surface* theDown)
{
	if (theNormal == nullptr)
		return false;
	if (mImages == nullptr)
	{
		mImages = new Selector();
		addChild(mImages);
	}
	else
		mImages->removeAllChildren();

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
	else if (theHigh != nullptr)
		aDownImage = new Image(theHigh);
	else
	{
		aDownImage = new Image(theNormal);
		aDownImage->setColorMode(COLOR_MODE_ADD);
		aDownImage->setColor(MADE_DOWN_BRIGHTNESS, MADE_DOWN_BRIGHTNESS, MADE_DOWN_BRIGHTNESS);
	}
	mImages->addChild(aDownImage);

	addOwnState(STATE_INPUT);
	mImages->select(IMAGE_NORMAL, 0.0f, true);
	return true;
}

} // namespace engine
