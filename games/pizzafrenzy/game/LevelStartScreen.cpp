#include "LevelStartScreen.h"
#include "App.h"
#include "GameText.h"
#include "PizzaFrenzy.h"
#include "ScreenNodes.h"

#include <cstdlib>

using namespace pizzafrenzy;

namespace
{
const char* const ACTION_LEFT = "bookLeft";
const char* const ACTION_RIGHT = "bookRight";
const char* const ACTION_SELECTION_DONE = "selectionDone";
const char* const ACTION_MENU = "menu";  // 0x4f99f0

// The names a pizza's button and a van's button have, with the number after them
const char* const NAME_PIZZA = "pizza";
const char* const NAME_VAN_BUTTON = "vanButton";
const char* const NAME_VAN_BUTTON_FORMAT = "vanButton%d";  // 0x4feb80

// The layout names its nodes with the side first
const char* const SIDE_NAMES[2] = {"left", "right"};
const int SIDE_LEFT = 0;
const int SIDE_RIGHT = 1;

// The arrows turn two pages
const int PAGES_PER_TURN = 2;

// A page holds this many toppings, three to a row at this spacing
const int TOPPINGS_PER_PAGE = 12;
const int TOPPINGS_PER_ROW = 3;
const float TOPPING_SPACING_X = 67.0f;  // 0x43
const float TOPPING_SPACING_Y = 66.0f;  // 0x42
const float TOPPING_SCALE = 0.9f;       // 0x3f666666

// The van buttons stand in a row this far apart, centred, each with its topping's picture a little to its left and
// above
const char* const VAN_BUTTON_IMAGE = "res\\pizza\\toppingButton.jpg";  // 0x4feb8c
const char* const VAN_BUTTON_MOUSEOVER_SOUND = "res\\sounds\\menu_mouseover.ogg";
const char* const VAN_BUTTON_CLICK_SOUND = "res\\sounds\\menu_clicked.ogg";
const float VAN_SPACING = 90.0f;
const float VAN_HALF_SPACING = 45.0f;
const float VAN_PICTURE_OFFSET_X = -9.0f;
const float VAN_PICTURE_OFFSET_Y = -12.0f;
const int PIVOT_CENTRE = 1;

// The caption of a topping button fades in and out in this long, and stands this far from the button's position
const float CAPTION_FADE_SECONDS = 0.1f;
const float CAPTION_FADE_TARGET = 1.0f;
const float CAPTION_OFFSET_X = 35.0f;
const float CAPTION_OFFSET_Y = 50.0f;

// The done button is dark while a van has no topping
const float DARK_COLOR = -0.2f;  // 0xbe4ccccd
const int COLOR_MODE_ADD = 1;

// The topping button's caption, its level out of four, in the exe
const unsigned int ADDRESS_LEVEL_CAPTION = 0x4feba8;
}

// 0x446f30
void LevelStartScreen::setup()
{
	for (int aSide = SIDE_LEFT; aSide <= SIDE_RIGHT; aSide++)
		mPages[aSide] = getNode(std::string(SIDE_NAMES[aSide]) + "Page");
	for (int aSide = SIDE_LEFT; aSide <= SIDE_RIGHT; aSide++)
		mButtonGroups[aSide] = static_cast<engine::Container*>(getNode(std::string(SIDE_NAMES[aSide]) + "Buttons"));
	for (int aSide = SIDE_LEFT; aSide <= SIDE_RIGHT; aSide++)
		mPageNumbers[aSide] = static_cast<engine::TextItem*>(getNode(std::string(SIDE_NAMES[aSide]) + "ToppingPage"));
	mPageTemplate = mPageNumbers[SIDE_LEFT]->mText;
	for (int aSide = SIDE_LEFT; aSide <= SIDE_RIGHT; aSide++)
		mTurnButtons[aSide] = getNode(std::string(SIDE_NAMES[aSide]) + "Button");
	mVanButtons = static_cast<engine::Container*>(getNode("vanButtons"));
	mPage = 0;
	mHeldPicture = new engine::ImageNode((engine::Image*)nullptr);
	addChild(mHeldPicture);
	mActionSignal.connect(this, &LevelStartScreen::onAction);
	engine::App* anApp = engine::App::instance();
	anApp->mMouseMoveSignal.connect(this, &LevelStartScreen::onMouseMove);
	anApp->mRightDownSignal.connect(this, &LevelStartScreen::onRightDown);
	mDoneButton = getNode("doneButton");
	updateDoneButton();
	mCaption = static_cast<engine::FadeContainer*>(getNode("caption"));
	mCaptionText = static_cast<engine::TextItem*>(getNode("captionText"));
	mCaption->setVisible(false);
}

// 0x447c60: the toppings the player picked last time are the first picks
void LevelStartScreen::setLevel(Level* theLevel)
{
	mLevel = theLevel;
	int aVanCount = (int)theLevel->mKitchens.size();
	mVanButtons->removeAllChildren();
	mVanButtonList.clear();
	mVanPictures.clear();
	mChosen.clear();
	std::vector<engine::Ref<Topping>>& aPicks = PizzaFrenzy::instance()->mUserGameData->mPickToppings;
	for (size_t i = 0; i < aPicks.size(); i++)
		mChosen.push_back(aPicks[i]);
	mChosen.resize(aVanCount, nullptr);
	float aX = (float)(1 - aVanCount) * VAN_HALF_SPACING;
	for (int i = 0; i < aVanCount; i++)
	{
		engine::ImageButton* aButton = new engine::ImageButton();
		aButton->setPivotMode(PIVOT_CENTRE);
		aButton->setImages(engine::App::instance()->getImage(VAN_BUTTON_IMAGE), nullptr, nullptr);
		aButton->setEnabled(false);
		aButton->setPosition(aX, 0);
		std::string aName = formatText(NAME_VAN_BUTTON_FORMAT, i);
		listenTo(aButton, aName, aName);
		registerNode(aButton, aName);
		aButton->setSounds(VAN_BUTTON_MOUSEOVER_SOUND, VAN_BUTTON_CLICK_SOUND);
		mVanButtons->addChild(aButton);
		mVanButtonList.push_back(aButton);

		engine::ImageNode* aPicture = new engine::ImageNode((engine::Image*)nullptr);
		if (mChosen[i] != nullptr)
			aPicture->setImage(mChosen[i]->mScaledImage);
		aPicture->setPosition(aX + VAN_PICTURE_OFFSET_X, VAN_PICTURE_OFFSET_Y);
		mVanButtons->addChild(aPicture);
		mVanPictures.push_back(aPicture);
		aX = aX + VAN_SPACING;
	}
	updateDoneButton();
	mPage = 0;
	showPages();
}

// 0x445ed0
void LevelStartScreen::onMouseMove(engine::Point& thePoint)
{
	mHeldPicture->setPosition(thePoint);
}

// 0x445db0
void LevelStartScreen::onRightDown(engine::Point& thePoint)
{
	putBack();
}

// 0x4464d0: the second page is not shown when the first holds the last topping
void LevelStartScreen::showPages()
{
	putBack();
	mCaption->setVisible(false);
	mToppingButtons.clear();
	showPage(SIDE_LEFT, mPage);
	if ((int)PizzaFrenzy::instance()->mGameStats->mToppingLevels.size() <= (mPage + 1) * TOPPINGS_PER_PAGE)
	{
		mPages[SIDE_RIGHT]->setVisible(false);
		return;
	}
	showPage(SIDE_RIGHT, mPage + 1);
}

// 0x447990: the van's button puts the topping that is held on that van; "selectionDone" and the menu action go on
// to the application's state, and every other action is a button of a topping, which is picked up
void LevelStartScreen::onAction(std::string theAction)
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	if (theAction == ACTION_LEFT)
	{
		if (-1 < mPage - PAGES_PER_TURN)
			mPage = mPage - PAGES_PER_TURN;
		showPages();
		return;
	}
	if (theAction == ACTION_RIGHT)
	{
		int aToppingCount = (int)anApp->mGameStats->mToppingLevels.size();
		int aPageCount = aToppingCount / TOPPINGS_PER_PAGE;
		if (aToppingCount % TOPPINGS_PER_PAGE > 0)
			aPageCount++;
		if (mPage + PAGES_PER_TURN < aPageCount)
			mPage = mPage + PAGES_PER_TURN;
		showPages();
		return;
	}
	if (theAction == ACTION_SELECTION_DONE)
		selectionDone();
	else if (theAction != ACTION_MENU)
	{
		Topping* aTopping = anApp->mTileManifest->getTopping(theAction);
		if (aTopping != nullptr)
		{
			pickUp(aTopping);
			return;
		}
		if (theAction.compare(0, 5, NAME_PIZZA) == 0)
		{
			int aPizzaId = atol(theAction.substr(5).c_str());
			pickUp(anApp->mPizzaToppings[aPizzaId]);
			return;
		}
		if (theAction.compare(0, 9, NAME_VAN_BUTTON) != 0)
			return;
		Topping* aHeld = mHeld;
		if (aHeld == nullptr)
			return;
		long aVan = atol(theAction.substr(9).c_str());
		if (-1 < aVan && aVan < (long)mVanButtonList.size())
		{
			if (mChosen[aVan] != nullptr)
				setToppingAvailable(mChosen[aVan], true);
			mChosen[aVan] = aHeld;
			mVanPictures[aVan]->setImage(aHeld->mScaledImage);
			putBack();
			setToppingAvailable(aHeld, false);
			updateDoneButton();
		}
		return;
	}
	anApp->screenAction(theAction);
}

// 0x446210: the page's toppings are the player's, in the order of the player's map, those before the page skipped
void LevelStartScreen::showPage(int theSide, int thePage)
{
	ToppingMap<int>& aLevels = PizzaFrenzy::instance()->mGameStats->mToppingLevels;
	int aToppingCount = (int)aLevels.size();
	mPages[theSide]->setVisible(true);
	int aPageCount = aToppingCount / TOPPINGS_PER_PAGE;
	if (aToppingCount % TOPPINGS_PER_PAGE > 0)
		aPageCount++;
	mPageNumbers[theSide]->setText(formatText(mPageTemplate, thePage + 1, aPageCount));
	if ((thePage < 1 && theSide == SIDE_LEFT) || (aPageCount <= thePage && theSide == SIDE_RIGHT))
	{
		mTurnButtons[theSide]->setVisible(false);
		mTurnButtons[theSide]->setEnabled(false);
	}
	else
	{
		mTurnButtons[theSide]->setVisible(true);
		mTurnButtons[theSide]->setEnabled(true);
	}
	mButtonGroups[theSide]->removeAllChildren();
	ToppingMap<int>::iterator aLevelIt = aLevels.begin();
	int aSkipped = thePage * TOPPINGS_PER_PAGE;
	for (int i = 0; i < aSkipped; i++)
	{
		if (aToppingCount <= aSkipped)
			break;
		++aLevelIt;
	}
	int aToppingIndex = aSkipped;
	int aPlaced = 0;
	while (aToppingIndex < aToppingCount)
	{
		Topping* aTopping = aLevelIt->first;
		if (aTopping != nullptr)
		{
			ToppingButton* aButton = createToppingButton(aTopping, aLevelIt->second);
			aButton->setPosition((float)(aPlaced % TOPPINGS_PER_ROW) * TOPPING_SPACING_X,
			                     (float)(aPlaced / TOPPINGS_PER_ROW) * TOPPING_SPACING_Y);
			mButtonGroups[theSide]->addChild(aButton);
			std::vector<engine::Ref<Topping>>::iterator aChosenIt = mChosen.begin();
			while (aChosenIt != mChosen.end() && *aChosenIt != aTopping)
				++aChosenIt;
			if (aChosenIt != mChosen.end())
				setToppingAvailable(aTopping, false);
			aPlaced++;
			aToppingIndex++;
		}
		if (aLevelIt != aLevels.end())
			++aLevelIt;
		if (aPlaced >= TOPPINGS_PER_PAGE)
			break;
	}
}

// 0x446010
void LevelStartScreen::pickUp(Topping* theTopping)
{
	if (mHeld != nullptr)
		setToppingAvailable(mHeld, true);
	setToppingAvailable(theTopping, false);
	mHeld = theTopping;
	mHeldPicture->setImage(theTopping->mScaledImage);
	for (size_t i = 0; i < mVanButtonList.size(); i++)
		mVanButtonList[i]->setEnabled(true);
}

// 0x4460b0
void LevelStartScreen::putBack()
{
	if (mHeld == nullptr)
		return;
	setToppingAvailable(mHeld, true);
	mHeld = nullptr;
	mHeldPicture->setImage(nullptr);
	for (size_t i = 0; i < mVanButtonList.size(); i++)
		mVanButtonList[i]->setEnabled(false);
}

// 0x446130
void LevelStartScreen::updateDoneButton()
{
	for (size_t i = 0; i < mChosen.size(); i++)
	{
		if (mChosen[i] == nullptr)
		{
			mDoneButton->setEnabled(false);
			mDoneButton->setColor(DARK_COLOR, DARK_COLOR, DARK_COLOR);
			mDoneButton->setColorMode(COLOR_MODE_ADD);
			return;
		}
	}
	mDoneButton->setEnabled(true);
	mDoneButton->setColor(0, 0, 0);
}

// 0x448300
void LevelStartScreen::selectionDone()
{
	UserGameData* aUserData = PizzaFrenzy::instance()->mUserGameData;
	std::vector<LevelKitchen>& aKitchens = mLevel->mKitchens;
	for (size_t i = 0; i < aKitchens.size(); i++)
		aKitchens[i].mTopping = nullptr;
	for (size_t i = 0; i < aKitchens.size(); i++)
		aKitchens[i].mTopping = mChosen[i];
	aUserData->mPickToppings.clear();
	for (size_t i = 0; i < mChosen.size(); i++)
	{
		if (mChosen[i] != nullptr)
			aUserData->mPickToppings.push_back(mChosen[i]);
	}
	aUserData->save();
}

// 0x4484f0
void LevelStartScreen::setToppingAvailable(Topping* theTopping, bool theAvailable)
{
	ToppingButton* aButton = mToppingButtons[theTopping];
	if (aButton == nullptr)
		return;
	if (!theAvailable)
		aButton->showDimmed();
	else
		aButton->showPlain();
}

// 0x448530
ToppingButton* LevelStartScreen::createToppingButton(Topping* theTopping, int theLevel)
{
	ToppingButton* aButton = new ToppingButton(theTopping, theLevel + 1, this);
	aButton->setCaption(formatText(GT(ADDRESS_LEVEL_CAPTION), theLevel + 1));
	aButton->setScale(TOPPING_SCALE);
	mToppingButtons[theTopping] = aButton;
	return aButton;
}

// 0x448660: the caption has the name of the button's topping and comes in beside the button
void LevelStartScreen::buttonEnter(std::string theName)
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	Topping* aTopping = anApp->mTileManifest->getTopping(theName);
	ToppingButton* aButton = nullptr;
	if (aTopping != nullptr)
		aButton = mToppingButtons[aTopping];
	else if (theName.compare(0, 5, NAME_PIZZA) == 0)
	{
		aTopping = anApp->mPizzaToppings[atol(theName.substr(5).c_str())];
		aButton = mToppingButtons[aTopping];
	}
	if (aButton == nullptr || aTopping == nullptr)
		return;
	mCaptionText->setText(aTopping->mDisplayName);
	mCaption->fade(true, CAPTION_FADE_SECONDS, CAPTION_FADE_TARGET, false);
	engine::Point* aPosition = aButton->getAbsPosition();
	mCaption->setPosition(aPosition->x + CAPTION_OFFSET_X, aPosition->y + CAPTION_OFFSET_Y);
}

// 0x4461b0
void LevelStartScreen::buttonExit(std::string theName)
{
	mCaption->fade(false, CAPTION_FADE_SECONDS, CAPTION_FADE_TARGET, false);
}
