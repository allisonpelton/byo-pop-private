#include "PlayerThoughtBubble.h"
#include "App.h"
#include "FishAnimImage.h"
#include "FishFood.h"
#include "FishSpawner.h"
#include "PlayerFish.h"
#include "StageMgr.h"

using namespace engine;

namespace feedingfrenzy
{

// The pictures of the bubble: the red one waits, the green one shows when a kind does
const char* const RED_BUBBLE_IMAGE = "resources\\Hud\\playerBubbleRed.png";
const char* const GREEN_BUBBLE_IMAGE = "resources\\Hud\\playerBubbleGreen.png";
// The seconds a kind shows for, and the seconds a fade takes
const float KIND_SECONDS = 2.2f;
const float FADE_SECONDS = 0.2f;
// Where the kind's picture sits in the bubble
const float KIND_X = 50.0f;
const float KIND_Y = 45.0f;
// No kind to show yet, and the level above any kind's
const float NO_SECONDS = -1.0f;
const float ABOVE_ALL_LEVELS = 9999.0f;

// 0x40bc00
PlayerThoughtBubble::PlayerThoughtBubble() :
    mPlayer(nullptr), mSecondsLeft(0.0f), mSpawner(nullptr), mShown(false), mForce(false), mAllShown(false)
{
	setFlags(NODE_LISTENERS);
}

// 0x40bd30
void PlayerThoughtBubble::init(PlayerFish* thePlayer)
{
	mRedBubble = new ImageNode(App::instance()->getImage(RED_BUBBLE_IMAGE));
	addChild(mRedBubble);
	mRedBubble->setVisible(false);

	mGreenBubble = new ImageNode(App::instance()->getImage(GREEN_BUBBLE_IMAGE));
	addChild(mGreenBubble);
	mGreenBubble->setVisible(false);

	setVisible(false);
	mPlayer = thePlayer;
}

// 0x40bad0
void PlayerThoughtBubble::reset()
{
	mAllShown = false;
	mSpawner = nullptr;
	mSecondsLeft = NO_SECONDS;
	if (mFishImage != nullptr)
	{
		removeChild(mFishImage);
		mFishImage = nullptr;
	}
}

// 0x40bb10
void PlayerThoughtBubble::show(bool theShow, bool theForce)
{
	if (!theShow)
	{
		if (mShown && getOpacity() > 0.0f)
			fade(false, FADE_SECONDS, 1.0f, false);
	}
	else
		reset();
	mShown = theShow;
	mForce = theForce;
	setVisible(mPlayer->isVisible());
}

// 0x40c370
void PlayerThoughtBubble::update(UpdateInfo& theInfo)
{
	FadeContainer::update(theInfo);
	setFlags(NODE_LISTENERS);
	if (mShown)
	{
		if (mSecondsLeft > 0.0f)
			mSecondsLeft -= theInfo.mSeconds;
		if (mSecondsLeft < 0.0f)
			pickKind();
	}
}

// 0x40c240
void PlayerThoughtBubble::pickKind()
{
	float aPlayerLevel = mPlayer->getLevel();
	std::vector<FishSpawner*>& aSpawners = FishFood::instance()->getStageMgr()->getSpawners();
	if (!mAllShown)
	{
		float aLevelShown = mSpawner == nullptr ? ABOVE_ALL_LEVELS : mSpawner->getLevel();
		std::vector<FishSpawner*>::iterator aSpawner = aSpawners.begin();
		while (aSpawner != aSpawners.end())
		{
			FishSpawner* aCandidate = *aSpawner;
			if (aCandidate->getDisplayMenuImage() && aCandidate->getLevel() < aLevelShown &&
			    aCandidate->getLevel() < aPlayerLevel)
			{
				if (aCandidate->getEatenCount() < 1 || mForce)
				{
					showKind(aCandidate);
					return;
				}
			}
			++aSpawner;
		}
		mAllShown = true;
		mSpawner = nullptr;
	}
	if (mShown && getOpacity() > 0.0f)
		fade(false, FADE_SECONDS, 1.0f, false);
	mForce = false;
	mShown = false;
	setVisible(mPlayer->isVisible());
}

// 0x40bf00
void PlayerThoughtBubble::showKind(FishSpawner* theSpawner)
{
	if (mFishImage != nullptr)
	{
		removeChild(mFishImage);
		mFishImage = nullptr;
	}
	mRedBubble->setVisible(false);
	mGreenBubble->setVisible(true);

	Image* anImage = nullptr;
	if (theSpawner->getColorShift() != nullptr)
		anImage = App::instance()->findImage(
		    FishAnimImage::getShiftedName(theSpawner->getMenuImage(), *theSpawner->getColorShift()).c_str());
	if (anImage == nullptr)
		anImage = App::instance()->getImage(theSpawner->getMenuImage().c_str());
	anImage->setPivot(1);
	mFishImage = new ImageNode(anImage);
	mFishImage->setScale(1.0f, 1.0f);
	mFishImage->setPosition(KIND_X, KIND_Y);
	addChild(mFishImage);
	mSpawner = theSpawner;
	mSecondsLeft = KIND_SECONDS;
	if (getOpacity() <= 0.0f)
		fade(true, FADE_SECONDS, 1.0f, false);
}

} // namespace feedingfrenzy
