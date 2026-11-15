#include "Hud.h"
#include "App.h"

using namespace pizzafrenzy;

namespace
{
const char* const HUD_LAYOUT_FILE = "res\\screenLayouts\\hudScreen.xml";
const char* const STAR_IMAGE = "res\\hud\\star-full.jpg";
const char* const WARNING_STAR_IMAGE = "res\\hud\\star-warning.jpg";
// the stars stand one tenth of the group's height apart, the first one at a tenth
const float STAR_SPACING = 0.1f;
const int STAR_FIRST_STEP = 1;
const int STAR_STEP_INCREMENT = 2;
}

// 0x438ac0
Hud::Hud()
{
}

// 0x439ba0
void Hud::setup(GameStats* theStats)
{
	loadLayout(HUD_LAYOUT_FILE);
	mStats = theStats;
	mOrderCount = getNode("orderCount");
	mProgressText = getNode("progressText");
	mTipJarCount = getNode("tipJarCount");
	mTipJarMax = getNode("tipJarMax");
	mScoreBackground = getNode("scoreBkg");
	mScoreBackground->setVisible(false);
	mTipJarCountPosition = *mTipJarCount->getPosition();
	mTipJarContents = static_cast<engine::ImageNode*>(getNode("tipJarContents"));
	mTipJarContents->setClipped(true);
	engine::Node* aTipJar = getNode("tipJar");
	mTipJarRect = *aTipJar->getBounds();
	// the tip jar's coins (the particle effect "TipJarCoins") are made here and added to the group, and the sound
	// "tip_jarRollup" is got: left out, see the class
	mProgressStack = getNode("progressStack");
	engine::Node* aStars = getNode("stars");
	mStarsRect = *aStars->getBounds();
	mWarningStars = new engine::Container();
	mWarningStars->setVisible(false);
	addChild(mWarningStars);
	engine::Image* aStarImage = engine::App::instance()->createImage(STAR_IMAGE, false);
	aStarImage->setPivot(1);
	engine::Image* aWarningImage = engine::App::instance()->createImage(WARNING_STAR_IMAGE, false);
	aWarningImage->setPivot(1);
	int aStep = STAR_FIRST_STEP;
	for (int i = 0; i < STAR_COUNT; i++)
	{
		float aStarX = mStarsRect.left;
		float aStarY = mStarsRect.bottom - mStarsRect.height() * STAR_SPACING * (float)aStep;
		mStars[i] = new engine::ImageNode(aStarImage);
		mStars[i]->setPosition(aStarX, aStarY);
		mStars[i]->setClipped(true);  // the part of the star drawn is the game's to set
		addChild(mStars[i]);
		engine::ImageNode* aWarningStar = new engine::ImageNode(aWarningImage);
		aWarningStar->setPosition(aStarX, aStarY);
		mWarningStars->addChild(aWarningStar);
		aStep += STAR_STEP_INCREMENT;
	}
	// the burst effect ("BurstFx") is made and put in the middle of the stars' top here: left out, see the class
}

// slot 0x190: translated next with the hud's displays
void Hud::updateHud(engine::UpdateInfo& theInfo)
{
}

// 0x43a5b0: translated next with the hud's displays
void Hud::showVehicles()
{
}

// 0x438270: the sound of the tip jar's roll is left out (see the class)
void Hud::pauseTipJarSound(bool thePaused)
{
}
