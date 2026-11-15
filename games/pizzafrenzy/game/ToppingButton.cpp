#include "ToppingButton.h"
#include "App.h"

using namespace pizzafrenzy;

namespace
{
const char* const CARD_IMAGE = "res\\pizza\\orderButton.jpg";
const char* const CARD_FONT = "res\\fonts\\buttonFont.xml";
const char* const CLICK_SOUND = "res\\sounds\\menu_clicked.ogg";

const int PIVOT_CENTRE = 1;
const int ALIGN_CENTRE = 1;
const int ALIGN_BOTTOM = 2;
const int COLOR_MODE_ADD = 1;
const int COLOR_MODE_MULTIPLY = 2;

// Where the picture and the number stand on the card, at the scale 1
const float CAPTION_Y = 30.0f;
const float PICTURE_Y = -17.0f;
// A card with no number has the picture lower; at another scale the picture and the number keep their share
const float PICTURE_Y_NO_NUMBER = -5.0f;
const float PICTURE_Y_SCALED = -18.0f;
const float DIMMED_COLOR = -0.2f;  // 0xbe4ccccd
}

// 0x427210
ToppingButton::ToppingButton(Topping* theTopping, int theNumber, engine::ScreenLayout* theScreen) :
	mTopping(theTopping), mNumber(theNumber)
{
	engine::App* anApp = engine::App::instance();
	mButton = new engine::ImageButton();
	mButton->setPivotMode(PIVOT_CENTRE);
	mButton->setImages(anApp->getImage(CARD_IMAGE), nullptr, nullptr);
	mButton->setSounds("", CLICK_SOUND);
	mButton->setName(theTopping->mName);
	addChild(mButton);
	theScreen->listenTo(mButton, theTopping->mName, theTopping->mName);
	theScreen->registerNode(mButton, theTopping->mName);

	mCaption = new engine::TextItem();
	mCaption->setFont(anApp->getFont(CARD_FONT));
	mCaption->setHAlign(ALIGN_CENTRE);
	mCaption->setVAlign(ALIGN_BOTTOM);
	mCaption->setColorMode(COLOR_MODE_MULTIPLY);
	mCaption->setColor(0, 0, 0);
	mCaption->setNumber(theNumber, "");
	mCaption->setPosition(0, CAPTION_Y);
	addChild(mCaption);

	mPicture = new engine::ImageNode(theTopping->mScaledImage);
	mPicture->setPosition(0, PICTURE_Y);
	addChild(mPicture);
	if (theNumber < 0)
	{
		mCaption->setVisible(false);
		mPicture->setPosition(0, PICTURE_Y_NO_NUMBER);
	}
}

// 0x427150
void ToppingButton::setScale(float theScale)
{
	mButton->setBaseScale(theScale, theScale);
	if (mNumber < 0)
		mPicture->setPosition(0, theScale * PICTURE_Y_NO_NUMBER);
	else
		mPicture->setPosition(0, theScale * PICTURE_Y_SCALED);
	mCaption->setPosition(0, theScale * CAPTION_Y);
}

// 0x4271e0
void ToppingButton::setEnabled(bool theEnabled)
{
	engine::Container::setEnabled(theEnabled);
	mButton->setEnabled(theEnabled);
}

// 0x427100
void ToppingButton::showPlain()
{
	setEnabled(true);
	setColor(0, 0, 0);
}

// 0x427120
void ToppingButton::showDimmed()
{
	setEnabled(false);
	setColor(DIMMED_COLOR, DIMMED_COLOR, DIMMED_COLOR);
	setColorMode(COLOR_MODE_ADD);
}
