#include "PizzaPopup.h"
#include "App.h"
#include "PizzaFrenzy.h"
#include "Tiles.h"

using namespace pizzafrenzy;

namespace
{
const char* const SELECTION_IMAGE = "res\\pizza\\selection-box.jpg";
const char* const TOPPING_BUTTON_IMAGE = "res/pizza/toppingButton.jpg";
const char* const PRICE_BACKGROUND_IMAGE = "res/pizza/pizzaMultiplierBkg.jpg";
const char* const POLICE_BUTTON_IMAGE = "res/pizza/policeButton.jpg";
const char* const PRICE_FONT = "res\\fonts\\hudFont.xml";
const char* const SOUND_MOUSE_OVER = "order_mouseover1";
const char* const PRICE_PREFIX = "$";

// where the pictures of a button and a price stand on the popup
const float BUTTON_X = -2.0f;
const float BUTTON_Y = 2.0f;
const float TOPPING_X = -10.0f;
const float TOPPING_Y = -12.0f;
const float PRICE_X = 24.0f;
const float PRICE_Y = -26.0f;

// the price's text: centred in the font's colour black, smooth
const int ALIGN_CENTRE = 1;
const int COLOR_MODE_REPLACE = 2;
const int SMOOTH_ON = 1;
}

// 0x455340
PizzaPopup::PizzaPopup() : mOwner(nullptr), mLeaving(false), mEnabledFlag(false)
{
}

// 0x454f40: the selection box is let go with the member
PizzaPopup::~PizzaPopup()
{
}

// 0x455000: the box that shows the mouse is over the popup is made hidden, and the popup is laid out and given its
// frame's updates
void PizzaPopup::setOwner(BuildingTile* theTile)
{
	mOwner = theTile;
	mEnabledFlag = true;
	setSmoothType(SMOOTH_ON);
	engine::Image* aBox = engine::App::instance()->createImage(SELECTION_IMAGE, false);
	aBox->setPivot(1);
	mSelection = new engine::ImageNode(aBox);
	mSelection->setPosition(0.0f, 0.0f);
	mSelection->setVisible(false);
	addChild(mSelection);
	setFlags(engine::NODE_LISTENERS);
	setFlags(engine::NODE_LAYOUT);
}

// 0x455110
void PizzaPopup::layout()
{
	mBounds = mRect;
	mSelection->layout();
	mBounds.unite(*mSelection->getBounds());
	mBounds.scale(mScale.x, mScale.y);
	mBounds.normalize();
	mBounds.offset(mPosition.x, mPosition.y);
	clearFlags(engine::NODE_LAYOUT);
}

// 0x454d60
void PizzaPopup::clearParent(engine::Node* theParent)
{
	mOwner->clearOccupant();
	Node::clearParent(theParent);
}

// 0x454c60
bool PizzaPopup::isClickable()
{
	PopupState* aState = mStates.getCurrent();
	return aState != nullptr && aState->isClickable();
}

// 0x455190
void PizzaPopup::setSelectionVisible(bool theVisible)
{
	mSelection->setVisible(theVisible);
}

// 0x452050
void PizzaPopup::leave()
{
	addOwnFlags(engine::NODE_REMOVE);
}

// 0x455260
void PizzaPopup::hover()
{
	setSelectionVisible(true);
	setOpacity(1.0f);
	setScale(1.0f);
	PizzaFrenzy::instance()->mSounds->play(SOUND_MOUSE_OVER, 1.0f, 1.0f);
}

// 0x454d30
void PizzaPopup::pick()
{
	setOpacity(1.0f);
	setScale(1.0f);
}

// 0x454d50
void PizzaPopup::serve()
{
	setSelectionVisible(false);
}

// 0x454d50
void PizzaPopup::unpick()
{
	setSelectionVisible(false);
}

// 0x454ca0
bool PizzaPopup::isPicked()
{
	PopupState* aState = mStates.getCurrent();
	return aState != nullptr && aState->isPicked();
}

// 0x4551c0
void PizzaPopup::hangUp()
{
	mStates.setState(new ClosingState(this));
}

// 0x454c50
void PizzaPopup::tick(engine::UpdateInfo& theInfo)
{
	mStates.update(theInfo);
}

// 0x454cd0
void PizzaPopup::onMouseDownEvent(engine::MouseEvent& theEvent)
{
	if (isClickable())
		mOwner->onMouseDownEvent(theEvent);
}

// 0x454cf0
void PizzaPopup::onMouseEnterEvent(engine::MouseEvent& theEvent)
{
	if (isClickable())
		hover();
}

// 0x454d10
void PizzaPopup::onMouseExitEvent(engine::MouseEvent& theEvent)
{
	if (isClickable())
		unpick();
}

// 0x451ab0: the base's, then the button, the topping's holder over it, the price's background with its number, and the
// popup comes up, clickable
void PizzaKitchenPopup::setOwner(BuildingTile* theTile)
{
	PizzaPopup::setOwner(theTile);
	engine::Image* aButton = engine::App::instance()->createImage(TOPPING_BUTTON_IMAGE, false);
	aButton->setPivot(1);
	mButton = new engine::ImageNode(aButton);
	mButton->setPosition(BUTTON_X, BUTTON_Y);
	addChild(mButton);
	mToppingHolder = new engine::Container();
	addChild(mToppingHolder);
	mToppingHolder->setPosition(TOPPING_X, TOPPING_Y);
	mPriceHolder = new engine::Container();
	mPriceHolder->setPosition(PRICE_X, PRICE_Y);
	addChild(mPriceHolder);
	engine::Image* aBackground = engine::App::instance()->createImage(PRICE_BACKGROUND_IMAGE, false);
	aBackground->setPivot(1);
	mPriceHolder->addChild(new engine::ImageNode(aBackground));
	mPrice = new engine::TextItem();
	mPrice->setFont(engine::App::instance()->getFont(PRICE_FONT));
	mPrice->setColor(0.0f, 0.0f, 0.0f);
	mPrice->setColorMode(COLOR_MODE_REPLACE);
	mPrice->setHAlign(ALIGN_CENTRE);
	mPrice->setVAlign(ALIGN_CENTRE);
	mPrice->setSmoothType(SMOOTH_ON);
	mPriceHolder->addChild(mPrice);
	mStates.setState(new AppearState(this, true));
}

// 0x451850
void PizzaKitchenPopup::rest()
{
	mStates.setState(new RestState(this));
}

// 0x451650
void PizzaKitchenPopup::hover()
{
	if (PizzaFrenzy::instance()->mMode->getPickCount() > 0)
	{
		PizzaPopup::hover();
		mStates.setState(new PopupState(this));
	}
}

// 0x451710
void PizzaKitchenPopup::pick()
{
	PizzaPopup::pick();
	mStates.setState(new PopupState(this));
}

// 0x4517b0
void PizzaKitchenPopup::unpick()
{
	PizzaPopup::unpick();
	mStates.setState(new RestState(this));
}

// 0x4519b0
void PizzaKitchenPopup::setTopping(Topping* theTopping)
{
	mTopping = theTopping;
	mToppingHolder->removeAllChildren();
	if (mTopping != nullptr)
	{
		mToppingHolder->addChild(new engine::ImageNode(mTopping->mScaledImage));
		Upgrade* anUpgrade = PizzaFrenzy::instance()->mGameStats->getUpgrade(theTopping);
		setPrice(theTopping->getUpgradeValue(anUpgrade));
	}
}

// 0x451920
void PizzaKitchenPopup::setPrice(int thePrice)
{
	mPrice->setNumber(thePrice, PRICE_PREFIX);
}

// 0x455650
void PolicePopup::setOwner(BuildingTile* theTile)
{
	PizzaPopup::setOwner(theTile);
	engine::Image* aButton = engine::App::instance()->createImage(POLICE_BUTTON_IMAGE, false);
	aButton->setPivot(1);
	mButton = new engine::ImageNode(aButton);
	mButton->setPosition(BUTTON_X, BUTTON_Y);
	addChild(mButton);
	mStates.setState(new AppearState(this, true));
}

// 0x455a40
void PolicePopup::rest()
{
	mStates.setState(new RestState(this));
}

// 0x4557a0
void PolicePopup::hover()
{
	if (PizzaFrenzy::instance()->mMode->getPickCount() > 0)
	{
		PizzaPopup::hover();
		mStates.setState(new PopupState(this));
	}
}

// 0x455860
void PolicePopup::pick()
{
	PizzaPopup::pick();
	mStates.setState(new PopupState(this));
}

// 0x455900
void PolicePopup::serve()
{
	PizzaPopup::serve();
	mStates.setState(new ClosingState(this));
}

// 0x4559a0
void PolicePopup::unpick()
{
	PizzaPopup::unpick();
	mStates.setState(new RestState(this));
}
