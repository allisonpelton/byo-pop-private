#include "GameMapLayout.h"
#include "FishFood.h"
#include "GameMapCheckpoint.h"
#include "GameSession.h"
#include "GameStrings.h"
#include "GrowMeter.h"
#include "ScoreList.h"
#include "StageInfo.h"
#include "TextItem.h"

#include <stdio.h>

using namespace engine;
using namespace feedingfrenzy;

namespace
{
// The layout file
const char* const MAP_LAYOUT_FILE = "config\\mapLayout.xml";
// The nodes of the layout the screen keeps, by name
const char* const LOADING_TEXT_NAME = "loadingText";
const char* const STAGE_LABEL_NAME = "stageLabel";
const char* const NEXT_STAGE_TEXT_NAME = "nextStageText";
const char* const CHOOSE_HEADER_NAME = "chooseHeader";
const char* const DONE_BUTTON_NAME = "doneBtn";
const char* const BACK_BUTTON_GROUP_NAME = "backBtnGrp";
const char* const BACK_BUTTON_NAME = "backBtn";
const char* const LOADING_TEXT_GROUP_NAME = "loadingTextGroup";
const char* const DONE_BUTTON_GROUP_NAME = "doneBtnGroup";
const char* const LOAD_BAR_GROUP_NAME = "loadBarGroup";
// The actions of the buttons: the one that leaves the map, the one that shows the stage is loaded, and the stages'
// ("mapBtn" and the stage's number)
const char* const ABORT_ACTION = "abortNewGame";
const char* const DONE_ACTION = "mapDone";
const int STAGE_ACTION_PREFIX_LENGTH = 6;
const int STAGE_NUMBER_LENGTH = 4;
// The game's texts
const char* const CHOOSE_STAGE_TEXT = "chooseStage";
const char* const NEXT_UP_TEXT = "nextUpFormat";
// The names of a fish's nodes on the map are made from its name and these
const char* const FISH_DARK_IMAGE_FORMAT = "%sDarkImg";
const char* const FISH_NORMAL_IMAGE_FORMAT = "%sNormalImg";
const char* const FISH_BUTTON_FORMAT = "%sButton";
// The fish of the map, in the order of the manifest's changes of the player's fish
const char* const FISH_NAMES[] = {"angel", "lion", "angler", "jd", "orca"};
const int FISH_COUNT = 5;
// The map's screen leaves over this many seconds when the player turns it down or chooses a stage
const float MAP_FADE_OUT_SECONDS = 0.1f;
// The load bar's colour (red, green, blue)
const float LOAD_BAR_RED = 0.0f;
const float LOAD_BAR_GREEN = 0.4268f;
const float LOAD_BAR_BLUE = 0.27f;
// The text of a node that is blank
const char* const BLANK_TEXT = " ";
const char* const EMPTY_TEXT = "";

// The text of a node of the screen is put
void setNodeText(ScreenLayout* theScreen, const char* theNodeName, const std::string& theText)
{
	static_cast<TextItem*>(theScreen->getNode(theNodeName))->setText(theText);
}
}

// 0x423bd0
GameMapLayout::GameMapLayout() : mLoadBar(nullptr), mIsMap(false)
{
}

// 0x423820: the map is the one to choose a stage on, or the one that shows while a stage loads
void GameMapLayout::setIsMap(bool theIsMap)
{
	mIsMap = theIsMap;
	loadLayout(MAP_LAYOUT_FILE);
	addOwnFlags(NODE_LISTENERS);

	FishFood* aFood = FishFood::instance();
	int aStageCount = aFood->getStageCount();
	for (int aStage = 0; aStage < aStageCount; aStage++)
	{
		GameMapCheckpoint* aCheckpoint = new GameMapCheckpoint();
		addChild(aCheckpoint);
		aCheckpoint->build(aFood->getStageInfo(aStage), this);
		mCheckpoints.push_back(aCheckpoint);
	}

	mLoadBar = new GrowMeter();
	Container* aBarGroup = static_cast<Container*>(getNode(LOAD_BAR_GROUP_NAME));
	aBarGroup->addChild(mLoadBar);
	mLoadBar->setBarColor(LOAD_BAR_RED, LOAD_BAR_GREEN, LOAD_BAR_BLUE);
	aBarGroup->layout();
	float aWidth = aBarGroup->getBounds()->width();
	float aHeight = aBarGroup->getBounds()->height();
	mLoadBar->setRect(Rect(0.0f, 0.0f, aWidth, aHeight));
	mLoadBar->setValue(0.0f);
}

// 0x423210
void GameMapLayout::prepare()
{
	if (!mIsMap)
	{
		showStageProgress();
		showNextStage();
	}
	else
	{
		showChoosableStages();
		showChooseHeader();
	}
	for (int aFish = 0; aFish < FISH_COUNT; aFish++)
		showFish(aFish, FISH_NAMES[aFish]);
}

// 0x422b90: the loading text shows until the stage has loaded, and then the button that goes on to it
void GameMapLayout::setLoaded(bool theLoaded)
{
	Node* aLoadingGroup = getNode(LOADING_TEXT_GROUP_NAME);
	Node* aDoneGroup = getNode(DONE_BUTTON_GROUP_NAME);
	if (!theLoaded)
	{
		aLoadingGroup->setVisible(true);
		aDoneGroup->setVisible(false);
		aDoneGroup->setEnabled(false);
	}
	else
	{
		aLoadingGroup->setVisible(false);
		aDoneGroup->setVisible(true);
		aDoneGroup->setEnabled(true);
	}
}

// 0x4223a0
void GameMapLayout::update(UpdateInfo& theInfo)
{
	mLoadBar->setValue(FishFood::instance()->getLoadProgress());
	ScreenLayout::update(theInfo);
}

// 0x422700
void GameMapLayout::showChooseHeader()
{
	setNodeText(this, NEXT_STAGE_TEXT_NAME, BLANK_TEXT);
	setNodeText(this, STAGE_LABEL_NAME, BLANK_TEXT);
	setNodeText(this, LOADING_TEXT_NAME, FishFood::instance()->getStrings().get(CHOOSE_STAGE_TEXT));
	getNode(CHOOSE_HEADER_NAME)->setVisible(true);
	hideButton(DONE_BUTTON_NAME);
}

// 0x422960
void GameMapLayout::showNextStage()
{
	FishFood* aFood = FishFood::instance();
	StageInfo* aStage = aFood->getStageInfo(aFood->getStageIndex());
	setNodeText(this, NEXT_STAGE_TEXT_NAME,
	            aFood->getStrings().format(NEXT_UP_TEXT, {aStage->getIndex(), aStage->getName()}));
	setNodeText(this, STAGE_LABEL_NAME, EMPTY_TEXT);
	getNode(CHOOSE_HEADER_NAME)->setVisible(false);
	getNode(BACK_BUTTON_GROUP_NAME)->setVisible(false);
	Node* aBackButton = getNode(BACK_BUTTON_NAME);
	if (aBackButton != nullptr)
		aBackButton->removeOwnFlags(NODE_MOUSE);
}

// 0x422470: the stages up to the one after the furthest that is reached can be chosen
void GameMapLayout::showChoosableStages()
{
	FishFood* aFood = FishFood::instance();
	int aStageCount = aFood->getStageCount();
	int aFurthest = aFood->getScoreList(aFood->getSession()->getMode()).getFurthestStage();
	for (int aStage = 0; aStage < aStageCount; aStage++)
		mCheckpoints[aStage]->setState(aFurthest < aStage ? CHECKPOINT_OFF : CHECKPOINT_CHOOSABLE);
}

// 0x422400
void GameMapLayout::showStageProgress()
{
	FishFood* aFood = FishFood::instance();
	int aStageCount = aFood->getStageCount();
	for (int aStage = 0; aStage < aStageCount; aStage++)
	{
		int aState;
		if (aStage < aFood->getStageIndex())
			aState = CHECKPOINT_PASSED;
		else
			aState = aFood->getStageIndex() == aStage ? CHECKPOINT_LOADING : CHECKPOINT_OFF;
		mCheckpoints[aStage]->setState(aState);
	}
}

// 0x422660
void GameMapLayout::hideButton(const std::string& theName)
{
	Node* aNode = getNode(theName);
	if (aNode != nullptr)
	{
		aNode->setVisible(false);
		aNode->removeOwnFlags(NODE_MOUSE);
		aNode->setEnabled(false);
	}
}

// 0x423030
void GameMapLayout::showFish(int theFishIndex, std::string theName)
{
	FishFood* aFood = FishFood::instance();
	int aFurthest = aFood->getScoreList(aFood->getSession()->getMode()).getFurthestStage();

	char aNodeName[64];
	snprintf(aNodeName, sizeof(aNodeName), FISH_DARK_IMAGE_FORMAT, theName.c_str());
	Node* aDarkImage = getNode(aNodeName);
	snprintf(aNodeName, sizeof(aNodeName), FISH_NORMAL_IMAGE_FORMAT, theName.c_str());
	Node* aNormalImage = getNode(aNodeName);
	snprintf(aNodeName, sizeof(aNodeName), FISH_BUTTON_FORMAT, theName.c_str());

	FishChange* aChange = aFood->getFishChange(theFishIndex);
	if (aChange == nullptr || aFurthest < aChange->mFirstStage)
	{
		hideButton(aNodeName);
		aNormalImage->setVisible(false);
		aDarkImage->setVisible(true);
	}
	else
	{
		aNormalImage->setVisible(true);
		aDarkImage->setVisible(false);
		hideButton(aNodeName);
	}
}

// 0x422cf0
void GameMapLayout::buttonEnter(std::string theName)
{
	if (!mIsMap || theName == ABORT_ACTION || (int)theName.size() < STAGE_ACTION_PREFIX_LENGTH)
		return;
	int aNumber = 0;
	sscanf(theName.substr(STAGE_ACTION_PREFIX_LENGTH).c_str(), "%d", &aNumber);
	FishFood* aFood = FishFood::instance();
	if (aNumber < 1 || aNumber > aFood->getStageCount())
		return;
	setNodeText(this, LOADING_TEXT_NAME, aFood->getStageInfo(aNumber - 1)->getName());
}

// 0x4224e0
void GameMapLayout::buttonExit(std::string theName)
{
	if (mIsMap)
	{
		setNodeText(this, LOADING_TEXT_NAME, FishFood::instance()->getStrings().get(CHOOSE_STAGE_TEXT));
		setNodeText(this, STAGE_LABEL_NAME, BLANK_TEXT);
	}
}

// 0x422ec0
void GameMapLayout::buttonPress(std::string theName)
{
	FishFood* aFood = FishFood::instance();
	if (!mIsMap)
	{
		if (theName == DONE_ACTION)
			ScreenLayout::buttonPress(theName);
	}
	else if (theName == ABORT_ACTION)
	{
		aFood->hideScreen(this, MAP_FADE_OUT_SECONDS);
	}
	else if ((int)theName.size() >= STAGE_ACTION_PREFIX_LENGTH)
	{
		int aNumber = 0;
		sscanf(theName.substr(STAGE_ACTION_PREFIX_LENGTH, STAGE_NUMBER_LENGTH).c_str(), "%d", &aNumber);
		aFood->hideScreen(this, MAP_FADE_OUT_SECONDS);
		aFood->startGame(aFood->getSession()->getMode(), aNumber - 1);
	}
}
