#include "NewToppingScreen.h"
#include "PizzaFrenzy.h"
#include "ScreenNodes.h"

#include <stdexcept>

using namespace pizzafrenzy;

namespace
{
// The buttons: three to a row, fifteen at most, at this spacing, a little smaller than their size
const int BUTTONS_PER_ROW = 3;
const int MAX_BUTTONS = 15;
const float BUTTON_WIDTH = 67.0f;   // 0x43
const float BUTTON_HEIGHT = 64.0f;  // 0x40
const float BUTTON_SCALE = 0.9f;    // 0x3f666666

// The picture behind the topping, drawn at this many times the coupon's size
const float COUPON_SCALE = 2.0f;

// What an upgrade's multiplier is worth, in points: a topping that has a preview picture of its own is worth more
// (the two numbers at 0x4fa248 and 0x4fa24c)
const int VALUE_PER_MULTIPLIER = 10;
const int VALUE_PER_MULTIPLIER_WITH_PREVIEW = 15;

// The action the application's state waits for once a topping is chosen
const char* const ACTION_TOPPING_SELECTED = "toppingSelected";
}

// 0x43be30
void NewToppingScreen::setup()
{
	mButtons = static_cast<engine::Container*>(getNode("leftButtons"));
	mPictureBg = static_cast<engine::ImageNode*>(getNode("rightToppingBg"));
	mPicture = static_cast<engine::ImageNode*>(getNode("rightTopping"));
	mCaption = static_cast<engine::TextItem*>(getNode("rightToppingCaptionText"));
	mLevel = static_cast<engine::TextItem*>(getNode("rightToppingLevel"));
	mLevelTemplate = mLevel->mText;
	mValue = static_cast<engine::TextItem*>(getNode("rightToppingValue"));
	mValueTemplate = mValue->mText;
	mDescription = static_cast<engine::TextItem*>(getNode("rightToppingDesc"));
	mActionSignal.connect(this, &NewToppingScreen::onAction);
}

// 0x43b970: a topping of the manifest is the player's now, no longer new, and the player is saved; the application's
// state hears that a topping was chosen
void NewToppingScreen::onAction(std::string theAction)
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	Topping* aTopping = anApp->mTileManifest->getTopping(theAction);
	if (aTopping == nullptr)
	{
		anApp->screenAction(theAction);
		return;
	}
	anApp->mGameStats->mToppingLevels[aTopping] = 0;
	anApp->mUserGameData->removeNewTopping(aTopping);
	anApp->mUserGameData->mUnlockTopping = false;
	anApp->mUserGameData->save();
	anApp->screenAction(ACTION_TOPPING_SELECTED);
}

// 0x43b500
void NewToppingScreen::fillButtons()
{
	std::vector<engine::Ref<Topping>>& aNewToppings = PizzaFrenzy::instance()->mUserGameData->getNewToppings();
	mButtons->removeAllChildren();
	int aCount = 0;
	for (size_t i = 0; i < aNewToppings.size(); i++)
	{
		if (MAX_BUTTONS - 1 < aCount)
			break;
		ToppingButton* aButton = createButton(aNewToppings[i]);
		aButton->setPosition((float)(aCount % BUTTONS_PER_ROW) * BUTTON_WIDTH,
		                     (float)(aCount / BUTTONS_PER_ROW) * BUTTON_HEIGHT);
		mButtons->addChild(aButton);
		aCount++;
	}
	showTopping(nullptr, 0);
}

// 0x43bad0: the first of the topping's upgrades is the one that is shown, and what it is worth
void NewToppingScreen::showTopping(Topping* theTopping, int theUpgrade)
{
	if (theTopping == nullptr)
	{
		mPicture->setImage(nullptr);
		mCaption->setText("");
		mDescription->setText("");
		mLevel->setText("");
		mValue->setText("");
		return;
	}
	mPictureBg->setImage(theTopping->mCouponImage->createScaled(COUPON_SCALE, COUPON_SCALE));
	mPicture->setImage(theTopping->mImage);
	if ((int)theTopping->mUpgrades.size() <= theUpgrade)
		throw std::out_of_range("invalid vector<T> subscript");
	Upgrade* anUpgrade = theTopping->mUpgrades[theUpgrade];
	mCaption->setText(anUpgrade->mName);
	mLevel->setText(formatText(mLevelTemplate, theUpgrade + 1));
	int aValuePerMultiplier = VALUE_PER_MULTIPLIER;
	if (theTopping->mUsesImageAsPreview != 0)
		aValuePerMultiplier = VALUE_PER_MULTIPLIER_WITH_PREVIEW;
	mValue->setText(formatText(mValueTemplate, anUpgrade->mBonusMultiplier * aValuePerMultiplier));
	mDescription->setText(anUpgrade->mDescription);
}

// 0x43b3e0
ToppingButton* NewToppingScreen::createButton(Topping* theTopping)
{
	ToppingButton* aButton = new ToppingButton(theTopping, -1, this);
	aButton->setScale(BUTTON_SCALE);
	return aButton;
}
