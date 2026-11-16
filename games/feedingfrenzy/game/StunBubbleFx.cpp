#include "StunBubbleFx.h"
#include "FishFood.h"
#include "StageMgr.h"

using namespace engine;

namespace feedingfrenzy
{

// The seconds the bubble grows for, and how many times its picture's size it reaches
const float BUBBLE_SECONDS = 0.21f;
const float BUBBLE_FULL_SCALE = 2.3f;
// Where the particles play, in the stage
const float PARTICLES_X = 400.0f;
const float PARTICLES_Y = 240.0f;
// The files, and the sound
const char* const BUBBLE_IMAGE = "resources\\fx\\stun\\stunBubbleBurst.png";
const char* const PARTICLES_FILE = "config\\fx\\stunnedEatWorldFx.xml";
const char* const BUBBLE_SOUND = "stunBubble";
const float BUBBLE_SOUND_VOLUME = 1.0f;
const float BUBBLE_SOUND_PITCH = 1.0f;

// 0x41a5c0
StunBubbleFx::StunBubbleFx() : mSecondsLeft(0.0f), mDuration(BUBBLE_SECONDS)
{
}

// 0x41a390
void StunBubbleFx::init()
{
	Image* anImage = App::instance()->getImage(BUBBLE_IMAGE);
	anImage->setPivot(1);
	mBubble = new ImageNode(anImage);
	mBubble->setVisible(false);
	addChild(mBubble);
	FishFood::instance()->getStageMgr()->addToPlayGroup(this);

	mParticles = new ParticleSystem();
	mParticles->load(PARTICLES_FILE);
	FishFood::instance()->getStageMgr()->addToPlayGroup(mParticles);
	mParticles->setPosition(PARTICLES_X, PARTICLES_Y);
}

// 0x41a4f0
void StunBubbleFx::start(const Point& thePlace)
{
	setFlags(NODE_LISTENERS);
	mBubble->setVisible(true);
	mBubble->setPosition(thePlace);
	mSecondsLeft = mDuration;
	FishFood::instance()->playSound(BUBBLE_SOUND, BUBBLE_SOUND_VOLUME, BUBBLE_SOUND_PITCH);
	mParticles->startEmitting();
}

// 0x41a2a0
void StunBubbleFx::stop()
{
	mBubble->setVisible(false);
	clearFlags(NODE_LISTENERS);
}

// 0x41a310
void StunBubbleFx::update(UpdateInfo& theInfo)
{
	mSecondsLeft -= theInfo.mSeconds;
	if (mSecondsLeft < 0.0f)
	{
		mSecondsLeft = 0.0f;
		mBubble->setVisible(false);
		clearFlags(NODE_LISTENERS);
		return;
	}
	float aScale = (1.0f - mSecondsLeft / mDuration) * BUBBLE_FULL_SCALE;
	mBubble->setScale(aScale, aScale);
	Container::update(theInfo);
}

} // namespace feedingfrenzy
