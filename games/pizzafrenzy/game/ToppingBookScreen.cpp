#include "ToppingBookScreen.h"
#include "App.h"
#include "FrenzyName.h"
#include "GameText.h"
#include "Host.h"
#include "PizzaFrenzy.h"
#include "ScreenNodes.h"

using namespace pizzafrenzy;

namespace
{
const char* const ACTION_LEFT = "bookLeft";
const char* const ACTION_RIGHT = "bookRight";

// The layout names its nodes with the side first
const char* const SIDE_NAMES[2] = {"left", "right"};
const int SIDE_LEFT = 0;
const int SIDE_RIGHT = 1;

// The arrows turn two pages
const int PAGES_PER_TURN = 2;

// The titles are listed in this font, one under the other, each a little closer than the font's own spacing
const char* const TITLE_FONT = "res\\fonts\\bookFont.xml";
const int TITLE_LINE_OVERLAP = 2;
const int ALIGN_CENTRE = 1;
const int COLOR_MODE_MULTIPLY = 2;

// A topping's picture with its colours gone: no saturation and no value
const float DARK_HUE = 0.0f;
const float DARK_SATURATION = -1.0f;
const float DARK_VALUE = -1.0f;

// The pivot modes of an image
const int PIVOT_CORNER = 0;
const int PIVOT_CENTRE = 1;

// The picture is drawn into a blank image that blends its alpha
const int DARK_ALPHA_THRESHOLD = 0xff;

// The string table's text of a topping the player has not met
const int STRING_UNMET_TOPPING = 0xe9;
// The text of a topping with no next upgrade, in the exe
const unsigned int ADDRESS_NO_NEXT_UPGRADE = 0x4fe91c;
}

// 0x444db0
void ToppingBookScreen::setup()
{
	mTitles = static_cast<engine::Container*>(getNode("leftTitles"));
	for (int aSide = SIDE_LEFT; aSide <= SIDE_RIGHT; aSide++)
	{
		std::string aName = SIDE_NAMES[aSide];
		mPages[aSide] = getNode(aName + "Page");
		mToppingBgs[aSide] = static_cast<engine::ImageNode*>(getNode(aName + "ToppingBg"));
		mLockedOverlays[aSide] = getNode(aName + "Locked");
		mToppingPictures[aSide] = static_cast<engine::ImageNode*>(getNode(aName + "Topping"));
		mCaptions[aSide] = static_cast<engine::TextItem*>(getNode(aName + "ToppingCaptionText"));
		mLevels[aSide] = static_cast<engine::TextItem*>(getNode(aName + "ToppingLevel"));
		mValues[aSide] = static_cast<engine::TextItem*>(getNode(aName + "ToppingValue"));
		mNextAts[aSide] = static_cast<engine::TextItem*>(getNode(aName + "ToppingNext"));
		mDescriptions[aSide] = static_cast<engine::TextItem*>(getNode(aName + "ToppingDesc"));
		mPageNumbers[aSide] = static_cast<engine::TextItem*>(getNode(aName + "ToppingPage"));
		mTurnButtons[aSide] = static_cast<engine::ImageButton*>(getNode(aName + "Button"));
	}
	mLevelTemplate = mLevels[SIDE_LEFT]->mText;
	mValueTemplate = mValues[SIDE_LEFT]->mText;
	mPageTemplate = mPageNumbers[SIDE_LEFT]->mText;
	mPage = 0;
	showPages();
	mActionSignal.connect(this, &ToppingBookScreen::onAction);

	// each topping's picture, drawn into an image of its own and given no colour
	mDarkFilter.setShift(DARK_HUE, DARK_SATURATION, DARK_VALUE);
	engine::Host* aHost = engine::getHost();
	std::vector<engine::Ref<Topping>>& aToppings = PizzaFrenzy::instance()->mTileManifest->getToppingList();
	for (size_t i = 0; i < aToppings.size(); i++)
	{
		Topping* aTopping = aToppings[i];
		engine::Image* aImage = aTopping->mImage;
		aImage->setPivot(PIVOT_CORNER);
		engine::Ref<engine::Image> aDarkImage = aHost->createBlankImage(aImage->getWidth(), aImage->getHeight());
		mDarkImages[aTopping] = aDarkImage;
		engine::Graphics* aGraphics = aHost->createImageGraphics(aDarkImage);
		aDarkImage->fill(engine::Color4(0, 0, 0, 0));
		aDarkImage->setAlphaMode(engine::ALPHA_BLEND, DARK_ALPHA_THRESHOLD);
		aGraphics->drawImage(aImage);
		aDarkImage->applyFilter(mDarkFilter);
		aImage->setPivot(PIVOT_CENTRE);
		aDarkImage->setPivot(PIVOT_CENTRE);
		delete aGraphics;
	}
}

// 0x4446c0: the left page shows the topping before the page, the right page the topping at it, each with the number
// of its page; the right page goes when there is no such topping
void ToppingBookScreen::showPages()
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	std::vector<engine::Ref<Topping>>& aToppings = anApp->mTileManifest->getToppingList();
	UserGameData* aUserData = anApp->mUserGameData;
	if (mPage < 1)
	{
		mPages[SIDE_LEFT]->setVisible(false);
		showTitles();
	}
	else
	{
		mTitles->removeAllChildren();
		Topping* aLeftTopping = aToppings.at(mPage - 1);
		showPage(aLeftTopping, aUserData->getToppingLevel(aLeftTopping), SIDE_LEFT, mPage);
	}
	if ((int)aToppings.size() < mPage + 1)
	{
		mPages[SIDE_RIGHT]->setVisible(false);
		return;
	}
	Topping* aRightTopping = aToppings.at(mPage);
	showPage(aRightTopping, aUserData->getToppingLevel(aRightTopping), SIDE_RIGHT, mPage + 1);
}

// 0x443d80: the right arrow does not turn past the last topping
void ToppingBookScreen::onAction(std::string theAction)
{
	if (theAction == ACTION_LEFT)
	{
		if (-1 < mPage - PAGES_PER_TURN)
			mPage = mPage - PAGES_PER_TURN;
		showPages();
		return;
	}
	if (theAction == ACTION_RIGHT)
	{
		int aToppingCount = (int)PizzaFrenzy::instance()->mTileManifest->getToppingList().size();
		if (mPage + PAGES_PER_TURN <= aToppingCount)
			mPage = mPage + PAGES_PER_TURN;
		showPages();
		return;
	}
	PizzaFrenzy::instance()->screenAction(theAction);
}

// 0x4455d0: a topping the player has (or has been given and not chosen yet, which shows its first upgrade under the
// lock) shows its picture, the upgrade's name, level and value, what the next upgrade needs and its description; any
// other shows its picture dark, with no figures
void ToppingBookScreen::showPage(Topping* theTopping, int theLevel, int theSide, int thePage)
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	mPages[theSide]->setVisible(true);
	engine::ImageNode* aCoupon = mToppingBgs[theSide];
	int aCouponScale = aCoupon->mImage->getWidth() / theTopping->mCouponImage->getWidth();
	aCoupon->setImage(theTopping->mCouponImage->createScaled((float)aCouponScale, (float)aCouponScale));
	int aToppingCount = (int)anApp->mTileManifest->getToppingList().size();
	mPageNumbers[theSide]->setText(formatText(mPageTemplate, thePage, aToppingCount));
	if ((thePage < 1 && theSide == SIDE_LEFT) || (aToppingCount <= thePage && theSide == SIDE_RIGHT))
	{
		mTurnButtons[theSide]->setVisible(false);
		mTurnButtons[theSide]->setEnabled(false);
	}
	else
	{
		mTurnButtons[theSide]->setVisible(true);
		mTurnButtons[theSide]->setEnabled(true);
	}

	// a topping the player does not have, that is among the new ones, shows its first upgrade
	bool aIsNew = false;
	bool aShowUpgrade = true;
	int anUpgradeIndex = theLevel;
	if (theLevel < 0)
	{
		std::vector<engine::Ref<Topping>>& aNewToppings = anApp->mUserGameData->getNewToppings();
		std::vector<engine::Ref<Topping>>::iterator aNewIt = aNewToppings.begin();
		while (aNewIt != aNewToppings.end() && *aNewIt != theTopping)
			++aNewIt;
		aIsNew = aNewIt != aNewToppings.end();
		aShowUpgrade = aIsNew;
		anUpgradeIndex = 0;
	}
	if (aShowUpgrade && anUpgradeIndex < (int)theTopping->mUpgrades.size())
	{
		mToppingPictures[theSide]->setImage(theTopping->mImage);
		Upgrade* anUpgrade = theTopping->mUpgrades[anUpgradeIndex];
		mCaptions[theSide]->setText(anUpgrade->mName);
		mLevels[theSide]->setText(formatText(mLevelTemplate, anUpgradeIndex + 1));
		mValues[theSide]->setText(formatText(mValueTemplate, theTopping->getUpgradeValue(anUpgrade)));
		if ((int)theTopping->mUpgrades.size() <= anUpgradeIndex + 1)
			mNextAts[theSide]->setText(GT(ADDRESS_NO_NEXT_UPGRADE));
		else
			mNextAts[theSide]->setText(getFrenzyName(theTopping->mUpgrades[anUpgradeIndex + 1]->mCombo));
		mDescriptions[theSide]->setText(anUpgrade->mDescription);
		mPageNumbers[theSide]->setText(formatText(mPageTemplate, thePage, aToppingCount));
	}
	else
	{
		mToppingPictures[theSide]->setImage(mDarkImages[theTopping]);
		mCaptions[theSide]->setText(engine::App::instance()->loadString(STRING_UNMET_TOPPING));
		mLevels[theSide]->setText(formatText(mLevelTemplate, 0));
		mValues[theSide]->setText(formatText(mValueTemplate, 0));
		mNextAts[theSide]->setText(GT(ADDRESS_NO_NEXT_UPGRADE));
		mDescriptions[theSide]->setText("");
	}
	mLockedOverlays[theSide]->setVisible(aIsNew);
}

// 0x443e30: the titles the player's points reach are lines of the left page, each under the one before
void ToppingBookScreen::showTitles()
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	mTitles->removeAllChildren();
	engine::Font* aFont = engine::App::instance()->getFont(TITLE_FONT);
	int aLineY = 0;
	int aPoints = 0;
	ToppingMap<int>& aLevels = anApp->mUserGameData->mToppingLevels;
	for (ToppingMap<int>::iterator aLevelIt = aLevels.begin(); aLevelIt != aLevels.end(); ++aLevelIt)
	{
		if (aLevelIt->first->mUsesImageAsPreview == 0)
			aPoints = aPoints + 1 + aLevelIt->second;
	}
	std::vector<engine::Ref<ScoreTitle>>& aTitles = anApp->mTileManifest->getTitles();
	ScoreTitle* aLastTitle = nullptr;
	for (size_t i = 0; i < aTitles.size(); i++)
	{
		ScoreTitle* aTitle = aTitles[i];
		if (aPoints < aTitle->mPoints)
			break;
		if (aLastTitle != nullptr)
		{
			// the title before this one is a line, in black
			engine::TextItem* aLine = new engine::TextItem();
			aLine->setFont(aFont);
			aLine->setHAlign(ALIGN_CENTRE);
			aLine->setText(aLastTitle->mText);
			aLine->setColor(0, 0, 0);
			aLine->setColorMode(COLOR_MODE_MULTIPLY);
			aLine->setPosition(0, (float)aLineY);
			mTitles->addChild(aLine);
			aLineY = aLineY - TITLE_LINE_OVERLAP + aFont->getLineHeight();
		}
		aLastTitle = aTitle;
	}
	if (aLastTitle != nullptr)
	{
		// the last title the points reach is the player's own, in red
		engine::TextItem* aLine = new engine::TextItem();
		aLine->setFont(aFont);
		aLine->setHAlign(ALIGN_CENTRE);
		aLine->setText(aLastTitle->mText);
		aLine->setColor(1.0f, 0, 0);
		aLine->setColorMode(COLOR_MODE_MULTIPLY);
		aLine->setPosition(0, (float)aLineY);
		mTitles->addChild(aLine);
	}
}
