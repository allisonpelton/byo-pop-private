#include "AnimImage.h"
#include "App.h"
#include "ImageNode.h"
#include <cstdio>

using namespace engine;

namespace
{
const float DEFAULT_RATE = 10.0f;  // pictures a second
}

// 0x481e50 (pizzafrenzy)
AnimImage::AnimImage() : mRate(DEFAULT_RATE), mPosition(0), mPlaying(false), mLoop(true), mFirst(0), mLast(0)
{
	select(-1, 0.0f, true);
}

// 0x481c80 (pizzafrenzy)
void AnimImage::update(UpdateInfo& theInfo)
{
	int aLast = mLast;
	int aSpan = aLast - mFirst + 1;
	if (getCount() < 1)
	{
		Selector::update(theInfo);
		return;
	}
	if (mPlaying)
	{
		float aPosition = mRate * theInfo.mSeconds + mPosition;
		mPosition = aPosition;
		int aFrame = (int)aPosition;
		if (aLast < aFrame)
		{
			if (!mLoop)
			{
				aFrame = mLast;
				mPlaying = false;
				mPosition = (float)aFrame;
			}
			else
			{
				if (0.0f < aPosition)
				{
					do
					{
						if (mPosition <= (float)aLast)
							break;
						mPosition = mPosition - (float)aSpan;
					} while (0.0f < mPosition);
				}
				aFrame = (int)mPosition;
			}
		}
		else if (aFrame < mFirst)
		{
			aFrame = mFirst;
			if (!mLoop)
			{
				mPlaying = false;
				mPosition = (float)aFrame;
			}
			else
			{
				if (0.0f < aPosition)
				{
					do
					{
						if ((float)mFirst <= mPosition)
							break;
						mPosition = (float)aSpan + mPosition;
					} while (0.0f < mPosition);
				}
				aFrame = (int)mPosition;
			}
		}
		select(aFrame, 0.0f, true);
	}
	Selector::update(theInfo);
}

// 0x482080 (pizzafrenzy)
int AnimImage::load(const char* thePath, const char* theExtension, int theFirst)
{
	char aName[512];
	App* anApp = App::instance();
	std::string anExtension;
	if (theExtension == nullptr)
	{
		snprintf(aName, sizeof(aName), "%s%d%s", thePath, theFirst, ".jpg");
		anExtension = anApp->exists(aName) ? ".jpg" : ".png";
	}
	else
		anExtension = theExtension;
	int aNumber = theFirst;
	while (true)
	{
		snprintf(aName, sizeof(aName), "%s%d%s", thePath, aNumber, anExtension.c_str());
		aNumber++;
		if (!anApp->exists(aName))
			break;
		Image* anImage = anApp->getImage(aName);
		if (anImage == nullptr)
			break;
		ImageNode* aNode = new ImageNode(anImage);
		aNode->layout();
		addChild(aNode);
	}
	finishLoading();
	return getCount();
}

// 0x482210 (pizzafrenzy)
void AnimImage::finishLoading()
{
	if (!mChildren.empty())
	{
		select(0, 0.0f, true);
		mBounds = *mChildren.front()->getBounds();
	}
	addOwnFlags(NODE_DRAWABLE | NODE_LISTENERS);
	resetRange();
}

// 0x481e30 (pizzafrenzy)
void AnimImage::resetRange()
{
	mFirst = 0;
	mLast = getCount() - 1;
}
