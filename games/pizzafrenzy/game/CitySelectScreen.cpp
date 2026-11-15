#include "CitySelectScreen.h"
#include "App.h"
#include "Color4.h"
#include "GameScreen.h"
#include "GameText.h"
#include "Host.h"
#include "PizzaFrenzy.h"
#include "ScreenNodes.h"

#include <cstdio>

using namespace pizzafrenzy;

namespace
{
// Lisa's text types out in this width, at this many letters a second, with no wait after a sentence
const float LISA_WRAP = 360.0f;
const float LISA_RATE = 120.0f;
const float LISA_SENTENCE_PAUSE = 0.0f;

// The postcards: four to a row, each button's place in the row and the size it grows to and shrinks to
const int POSTCARDS_PER_ROW = 4;
const float POSTCARD_WIDTH = 122.0f;
const float POSTCARD_HEIGHT = 105.0f;
const float POSTCARD_HOVER_SCALE = 1.0f;
const float POSTCARD_NORMAL_SCALE = 0.5f;
const float POSTCARD_SCALE_SECONDS = 0.2f;

// A lock sits over its postcard, up and to the left of the button's place
const float LOCK_OFFSET_X = -52.0f;
const float LOCK_OFFSET_Y = -47.0f;

// The picture of a postcard: the city as the game screen draws it, in a canvas this big, drawn small on the card
const int CANVAS_WIDTH = 700;
const int CANVAS_HEIGHT = 600;
const float CANVAS_SCALE = 0.2857143f;  // 0x3e924925
const float CANVAS_X = 7.0f;
const float CANVAS_Y = 8.0f;
// The canvas of a city that is not there
const float BLANK_RED = 0.24f;    // 0x3e75c28f
const float BLANK_GREEN = 0.33f;  // 0x3ea8f5c3
const float BLANK_BLUE = 0.34f;   // 0x3eae147b

// The caption (a stamp the card carries) and the city's name over it
const float CAPTION_X = 109.0f;
const float CAPTION_Y = 136.0f;
const float CAPTION_SCALE = 0.9f;
const int PIVOT_CENTRE = 1;

const int ALIGN_CENTRE = 1;
const int ALIGN_MIDDLE = 1;
const int COLOR_MODE_MULTIPLY = 2;

const int NO_ALPHA_THRESHOLD = 0xff;
}

// 0x434300
void CitySelectScreen::setup(std::vector<engine::Ref<City>>& theCities)
{
	engine::Container* aGroup = static_cast<engine::Container*>(getNode("postcardGroup"));
	mPostcards = new engine::Container();
	aGroup->addChild(mPostcards);
	mLisa = getNode("Lisa");
	mLisaBubble = getNode("LisaBubble");
	engine::TextItem* aLisaText = static_cast<engine::TextItem*>(getNode("lisaText"));
	mLisaTyper = new engine::TextTyper();
	mLisaTyper->setPosition(*aLisaText->getPosition());
	mLisaTyper->setFont(aLisaText->mFont);
	mLisaTyper->setWrapAndTiming(LISA_WRAP, LISA_RATE, LISA_SENTENCE_PAUSE);
	mLisaTyper->setColor(*aLisaText->getColor());
	mLisaTyper->setColorMode(aLisaText->mColorMode);
	mLisaTyper->setLineOffsets(0, 0);
	static_cast<engine::Container*>(mLisa.get())->addChild(mLisaTyper);
	aLisaText->setText("");
	setCities(theCities);
	mLocks = new engine::Container();
	aGroup->addChild(mLocks);
	mHoveredButton = nullptr;
}

// 0x433480
void CitySelectScreen::refresh()
{
	if (mHoveredButton != nullptr)
	{
		mPostcards->addChild(mHoveredButton);
		mHoveredButton = nullptr;
	}
	mLocks->removeAllChildren();
	int aMaxCity = PizzaFrenzy::instance()->mUserGameData->getMaxCity();
	for (int i = 0; i < (int)mButtons.size(); i++)
	{
		engine::ImageButton* aButton = mButtons[i];
		if (aMaxCity < i && !PizzaFrenzy::sDesignerUnlockOverride)
		{
			engine::Image* aLockImage = engine::App::instance()->getImage("res\\menuAssets\\lockedOverlay.jpg");
			engine::ImageNode* aLock = new engine::ImageNode(aLockImage);
			aLock->setPosition(aButton->getX() + LOCK_OFFSET_X, aButton->getY() + LOCK_OFFSET_Y);
			mLocks->addChild(aLock);
			aButton->setEnabled(false);
		}
		else
		{
			aButton->setEnabled(true);
		}
	}
}

// 0x434790: the game screen and its hud are hidden while the postcards are drawn on the game screen's account. Port:
// the game screen and the hud are made next with the game; until then the application has none, and nothing is hidden
// or drawn from it (a city's postcard is its card with no picture).
void CitySelectScreen::setCities(std::vector<engine::Ref<City>>& theCities)
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	if (anApp->mHud != nullptr)
		anApp->mHud->setVisible(false);
	if (anApp->mGameScreen != nullptr)
		anApp->mGameScreen->mHiddenWhileDrawing->setVisible(false);
	for (size_t i = 0; i < theCities.size(); i++)
	{
		engine::Ref<engine::Image> aPostcard = makePostcard(theCities[i]);
		mPostcardImages.push_back(aPostcard);
	}
	mUnknownPostcard = makePostcard(nullptr);
	if (anApp->mHud != nullptr)
		anApp->mHud->setVisible(true);
	if (anApp->mGameScreen != nullptr)
		anApp->mGameScreen->mHiddenWhileDrawing->setVisible(true);
	for (int i = 0; i < (int)mPostcardImages.size(); i++)
	{
		engine::Image* aPostcard = mPostcardImages[i];
		engine::ImageButton* aButton = new engine::ImageButton();
		aButton->setPivotMode(PIVOT_CENTRE);
		aButton->setImages(aPostcard, aPostcard, aPostcard);
		aButton->setPosition((float)(i % POSTCARDS_PER_ROW) * POSTCARD_WIDTH,
		                     (float)(i / POSTCARDS_PER_ROW) * POSTCARD_HEIGHT);
		aButton->setHoverScale(POSTCARD_HOVER_SCALE, POSTCARD_HOVER_SCALE);
		aButton->setNormalScale(POSTCARD_NORMAL_SCALE, POSTCARD_NORMAL_SCALE);
		aButton->setScaleTime(POSTCARD_SCALE_SECONDS);
		listenTo(aButton, "", formatText("citySelect%d", i));
		mButtons.push_back(aButton);
		mPostcards->addChild(aButton);
	}
}

// 0x433c30
void CitySelectScreen::connect()
{
	GenericScreen::connect();
	mLisaTyper->setTypedText(GT(0x4fd1d0));
	mLisaTyper->start();
}

// 0x4332d0: a valid button is taken out of the row of postcards and put with the locks, where it draws over them
void CitySelectScreen::buttonEnter(std::string theName)
{
	int aCity = -1;
	sscanf(theName.c_str(), "citySelect%d", &aCity);
	if (0 <= aCity && aCity < (int)mPostcardImages.size())
	{
		engine::ImageButton* aButton = mButtons[aCity];
		mPostcards->removeChild(aButton);
		mLocks->addChild(aButton);
		mHoveredButton = aButton;
	}
}

// 0x4333a0: and back into the row
void CitySelectScreen::buttonExit(std::string theName)
{
	int aCity = -1;
	sscanf(theName.c_str(), "citySelect%d", &aCity);
	if (0 <= aCity && aCity < (int)mPostcardImages.size())
	{
		engine::ImageButton* aButton = mButtons[aCity];
		mLocks->removeChild(aButton);
		mPostcards->addChild(aButton);
		mHoveredButton = nullptr;
	}
}

// 0x433690
engine::Image* CitySelectScreen::makePostcard(City* theCity)
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	engine::Host* aHost = engine::getHost();
	Level* aLevel = nullptr;
	if (theCity != nullptr)
	{
		for (size_t i = 0; i < theCity->mLevels.size(); i++)
		{
			aLevel = theCity->mLevels[i];
			if (!aLevel->mBonusRound)
				break;
		}
	}

	// the canvas: the city as the game screen draws it, or a plain colour with no city
	engine::Ref<engine::Image> aCanvas = aHost->createBlankImage(CANVAS_WIDTH, CANVAS_HEIGHT);
	engine::Graphics* aCanvasGraphics = aHost->createImageGraphics(aCanvas);
	if (aLevel == nullptr)
	{
		aCanvasGraphics->setColor(BLANK_RED, BLANK_GREEN, BLANK_BLUE);
		aCanvasGraphics->fillRect(engine::Rect(0, 0, (float)CANVAS_WIDTH, (float)CANVAS_HEIGHT));
	}
	else
	{
		anApp->loadCityFile(aLevel->getCityFile(), anApp->mGameScreen);
		if (anApp->mGameScreen != nullptr)
		{
			anApp->mGameScreen->setOpacity(1.0f);
			anApp->mGameScreen->applyTransform(aCanvasGraphics);
			anApp->mGameScreen->draw(aCanvasGraphics);
		}
	}
	delete aCanvasGraphics;

	// the card: the frame, with the canvas small inside it, and the caption and the name over it
	engine::Image* aFrame = engine::App::instance()->createImage("res\\menuAssets\\postcardFrame.jpg", false);
	engine::Image* aCard = aHost->createBlankImage(aFrame->getWidth(), aFrame->getHeight());
	aCard->fill(engine::Color4(0, 0, 0, 0));
	aCard->setAlphaMode(engine::ALPHA_BLEND, NO_ALPHA_THRESHOLD);
	engine::Graphics* aCardGraphics = aHost->createImageGraphics(aCard);

	engine::Ref<engine::ImageNode> aCanvasNode = new engine::ImageNode(aCanvas);
	aCanvasNode->setSmoothType(1);
	aCanvasNode->setScale(CANVAS_SCALE, CANVAS_SCALE);
	aCanvasNode->setPosition(CANVAS_X, CANVAS_Y);
	aCardGraphics->pushState();
	aCanvasNode->applyTransform(aCardGraphics);
	aCanvasNode->draw(aCardGraphics);
	aCardGraphics->popState();

	engine::Ref<engine::ImageNode> aFrameNode = new engine::ImageNode(aFrame);
	aCardGraphics->pushState();
	aFrameNode->applyTransform(aCardGraphics);
	aFrameNode->draw(aCardGraphics);
	aCardGraphics->popState();

	engine::Image* aCaption = engine::App::instance()->createImage("res\\menuAssets\\toppingUpgradeCaption.jpg", false);
	aCaption->setPivot(PIVOT_CENTRE);
	engine::Ref<engine::ImageNode> aCaptionNode = new engine::ImageNode(aCaption);
	aCaptionNode->setPosition(CAPTION_X, CAPTION_Y);
	aCaptionNode->setScale(CAPTION_SCALE, CAPTION_SCALE);
	aCardGraphics->pushState();
	aCaptionNode->applyTransform(aCardGraphics);
	aCaptionNode->draw(aCardGraphics);
	aCardGraphics->popState();

	engine::Ref<engine::TextItem> aName = new engine::TextItem();
	aName->setHAlign(ALIGN_CENTRE);
	aName->setVAlign(ALIGN_MIDDLE);
	aName->setFont("res\\fonts\\buttonFont.xml");
	aName->setColor(0, 0, 0);
	aName->setColorMode(COLOR_MODE_MULTIPLY);
	if (theCity != nullptr)
		aName->setText(theCity->getName());
	else
		aName->setText(GT(0x4fd17c));
	aName->setPosition(CAPTION_X, CAPTION_Y);
	aCardGraphics->pushState();
	aName->applyTransform(aCardGraphics);
	aName->draw(aCardGraphics);
	aCardGraphics->popState();
	delete aCardGraphics;
	return aCard;
}
