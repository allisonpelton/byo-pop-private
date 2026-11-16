#include "GameMapCheckpoint.h"
#include "App.h"
#include "Button.h"
#include "ImageNode.h"
#include "ScreenLayout.h"
#include "StageInfo.h"

#include <stdio.h>

using namespace engine;
using namespace feedingfrenzy;

namespace
{
// The images of the dots
const char* const OFF_IMAGE = "resources\\menus\\map\\checkpointOff.png";
const char* const CHECKED_IMAGE = "resources\\menus\\map\\checkpointChecked.png";
const char* const HIGH_IMAGE = "resources\\menus\\map\\checkpointHigh.png";
const char* const ON_IMAGE = "resources\\menus\\map\\checkpointOn.png";
// The button's sounds, and the names it is listened to under: the node's name and the action of its stage
const char* const BUTTON_ENTER_SOUND = "resources/sounds/mouseOver.wav";
const char* const BUTTON_PRESS_SOUND = "resources/sounds/mouseDown.wav";
const char* const BUTTON_NAME = "checkpointBtn";
const char* const BUTTON_ACTION_FORMAT = "mapBtn%d";
// The seconds the blinking dot keeps each look
const float BLINK_SECONDS = 0.2f;
// The pivot mode that puts the image's centre on the node's position
const int PIVOT_CENTRE = 1;

// An image of the archive that is set on its centre
ImageNode* makeDot(const char* theFile)
{
	Image* anImage = App::instance()->getImage(theFile);
	anImage->setPivot(PIVOT_CENTRE);
	return new ImageNode(anImage);
}
}

// 0x422320
GameMapCheckpoint::GameMapCheckpoint() :
    mState(CHECKPOINT_OFF), mChecked(nullptr), mOff(nullptr), mHigh(nullptr), mDots(nullptr), mButton(nullptr),
    mBlinkSeconds(0.0f), mScreen(nullptr)
{
}

// 0x421f80
void GameMapCheckpoint::build(StageInfo* theStage, ScreenLayout* theScreen)
{
	mScreen = theScreen;

	mDots = new Container();
	addChild(mDots);
	mOff = makeDot(OFF_IMAGE);
	mDots->addChild(mOff);
	mChecked = makeDot(CHECKED_IMAGE);
	mDots->addChild(mChecked);
	mChecked->setVisible(false);
	Image* aHighImage = App::instance()->getImage(HIGH_IMAGE);
	aHighImage->setPivot(PIVOT_CENTRE);
	mHigh = new ImageNode(aHighImage);
	mDots->addChild(mHigh);
	mHigh->setVisible(false);

	Image* anOnImage = App::instance()->getImage(ON_IMAGE);
	anOnImage->setPivot(PIVOT_CENTRE);
	mButton = new ImageButton();
	addChild(mButton);
	mButton->setImages(anOnImage, aHighImage, aHighImage);
	mButton->setSounds(BUTTON_ENTER_SOUND, BUTTON_PRESS_SOUND);
	mButton->setVisible(false);

	char anAction[32];
	snprintf(anAction, sizeof(anAction), BUTTON_ACTION_FORMAT, theStage->getIndex());
	mScreen->listenTo(mButton, BUTTON_NAME, anAction);
	setPosition(theStage->getMapPosition());
}

// 0x421d10
void GameMapCheckpoint::setState(int theState)
{
	mState = theState;
	switch (theState)
	{
	case CHECKPOINT_OFF:
		removeOwnFlags(NODE_LISTENERS);
		mOff->setVisible(true);
		mHigh->setVisible(false);
		mChecked->setVisible(false);
		mButton->setVisible(false);
		mButton->setEnabled(false);
		mDots->setVisible(true);
		break;
	case CHECKPOINT_PASSED:
		removeOwnFlags(NODE_LISTENERS);
		mHigh->setVisible(false);
		mOff->setVisible(false);
		mChecked->setVisible(true);
		mButton->setVisible(false);
		mButton->setEnabled(false);
		mDots->setVisible(true);
		break;
	case CHECKPOINT_LOADING:
		addOwnFlags(NODE_LISTENERS);
		mChecked->setVisible(false);
		mHigh->setVisible(true);
		mOff->setVisible(false);
		mButton->setVisible(false);
		mButton->setEnabled(false);
		mDots->setVisible(true);
		mBlinkSeconds = BLINK_SECONDS;
		break;
	case CHECKPOINT_CHOOSABLE:
		mDots->setVisible(false);
		mButton->setVisible(true);
		mButton->setEnabled(true);
		addOwnFlags(NODE_LISTENERS);
		break;
	}
}

// 0x421ea0
void GameMapCheckpoint::update(UpdateInfo& theInfo)
{
	if (mState == CHECKPOINT_LOADING)
	{
		mBlinkSeconds -= theInfo.mSeconds;
		if (mBlinkSeconds < 0.0f)
		{
			mOff->setVisible(!mOff->isVisible());
			mHigh->setVisible(!mOff->isVisible());
			mBlinkSeconds = BLINK_SECONDS;
		}
	}
	Container::update(theInfo);
}
