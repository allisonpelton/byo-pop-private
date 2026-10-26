#include "Button.h"
#include "App.h"
#include "Font.h"
#include "Graphics.h"
#include "ImageNode.h"
#include "Selector.h"
#include "Sound.h"

using namespace engine;

// 0x432600
Button::Button()
{
	mPressed = false;
	mEnterSound = nullptr;
	mPressSound = nullptr;
	mScaling = false;
	mNormalScale = Point(1.0f, 1.0f);
	mHoverScale = Point(1.0f, 1.0f);
	mBaseScale = Point(1.0f, 1.0f);
	mScaleTimeLeft = 0;
	mPivotMode = 0;
	mFlashPeriod = 0;
	mFlashTimeLeft = 0;
	mFlashing = false;
	mScaleTime = 0.5f;
	mScaleState = SCALE_NORMAL;
	mFlashUp = true;
}

// 0x432040: the entries go one by one from the front
Button::~Button()
{
	std::vector<ListenerEntry*>::iterator anIt = mListeners.begin();
	if (anIt != mListeners.end())
	{
		do
		{
			ListenerEntry* anEntry = *anIt;
			anIt = mListeners.erase(anIt);
			delete anEntry;
		} while (anIt != mListeners.end());
	}
}

// 0x431c10: the scale follows the state (the growing and shrinking states count their time down first), then the flash
void Button::update(UpdateInfo& theInfo)
{
	if (mScaling)
	{
		int aState = mScaleState;
		if (aState == SCALE_GROWING || aState == SCALE_SHRINKING)
		{
			float aTime = mScaleTimeLeft - theInfo.mSeconds;
			mScaleTimeLeft = aTime;
			if (aTime < 0.0f)
				mScaleTimeLeft = 0;
		}
		float aFraction;
		switch (aState)
		{
		case SCALE_HOVER:
			setScale(mBaseScale.x * mHoverScale.x, mBaseScale.y * mHoverScale.y);
			break;
		case SCALE_GROWING:
			aFraction = 1.0f - mScaleTimeLeft / mScaleTime;
			setScale(((mHoverScale.x - mNormalScale.x) * aFraction + mNormalScale.x) * mBaseScale.x,
				((mHoverScale.y - mNormalScale.y) * aFraction + mNormalScale.y) * mBaseScale.y);
			if (1.0f <= aFraction)
				mScaleState = SCALE_HOVER;
			break;
		case SCALE_SHRINKING:
			aFraction = mScaleTimeLeft / mScaleTime;
			setScale(((mHoverScale.x - mNormalScale.x) * aFraction + mNormalScale.x) * mBaseScale.x,
				((mHoverScale.y - mNormalScale.y) * aFraction + mNormalScale.y) * mBaseScale.y);
			if (aFraction <= 0.0f)
				mScaleState = SCALE_NORMAL;
			break;
		case SCALE_NORMAL:
			setScale(mBaseScale.x * mNormalScale.x, mBaseScale.y * mNormalScale.y);
			break;
		}
	}
	if (mFlashing)
	{
		float aTime = mFlashTimeLeft - theInfo.mSeconds;
		mFlashTimeLeft = aTime;
		if (aTime < 0.0f)
			mFlashTimeLeft = 0;
		float aFraction = mFlashTimeLeft / mFlashPeriod;
		if (mFlashUp)
		{
			setOpacity(aFraction);
			if (mFlashTimeLeft <= 0.0f)
			{
				mFlashTimeLeft = mFlashPeriod;
				mFlashUp = false;
			}
		}
		else
		{
			setOpacity(1.0f - aFraction);
			if (mFlashTimeLeft <= 0.0f)
			{
				mFlashUp = true;
				mFlashTimeLeft = mFlashPeriod;
			}
		}
	}
	Container::update(theInfo);
}

// 0x431e90
void Button::setBaseScale(float x, float y)
{
	mBaseScale = Point(x, y);
	Node::setScale(x, y);
}

// 0x431b20
void Button::setNormalScale(float x, float y)
{
	mNormalScale = Point(x, y);
	if (mNormalScale.x == mHoverScale.x && mNormalScale.y == mHoverScale.y)
	{
		mScaling = false;
		return;
	}
	mScaling = true;
}

// 0x431b90
void Button::setHoverScale(float x, float y)
{
	mHoverScale = Point(x, y);
	if (mNormalScale.x == mHoverScale.x && mNormalScale.y == mHoverScale.y)
	{
		mScaling = false;
		return;
	}
	mScaling = true;
}

// 0x4327a0
void Button::addListener(ButtonListener* theListener, std::string theName)
{
	ListenerEntry* anEntry = new ListenerEntry();
	anEntry->mName = theName;
	anEntry->mListener = theListener;
	mListeners.push_back(anEntry);
}

// 0x431f60: each sound is loaded under its own name, then a voice is made of it
void Button::setSounds(const std::string& theEnterSound, const std::string& thePressSound)
{
	if (theEnterSound.size() != 0 && App::getSoundSystem() != nullptr)
	{
		App::getSoundSystem()->load(theEnterSound.c_str(), theEnterSound.c_str());
		mEnterSound = App::getSoundSystem()->createSound(theEnterSound.c_str(), 1, 0);
	}
	if (thePressSound.size() != 0 && App::getSoundSystem() != nullptr)
	{
		App::getSoundSystem()->load(thePressSound.c_str(), thePressSound.c_str());
		mPressSound = App::getSoundSystem()->createSound(thePressSound.c_str(), 1, 0);
	}
}

// 0x431f00
void Button::setFlash(float thePeriod)
{
	if (mFlashPeriod != thePeriod)
	{
		mFlashTimeLeft = thePeriod;
		mFlashPeriod = thePeriod;
		mFlashUp = true;
		if (0.0f < thePeriod)
		{
			mFlashing = true;
			return;
		}
		mFlashing = false;
	}
}

// 0x432300
void Button::onMouseDown(Point& p)
{
	mPressed = true;
	if (mPressSound != nullptr)
		mPressSound->play();
	notifyListeners(1);
}

// 0x431b10
void Button::onMouseUp(Point& p)
{
	mPressed = false;
}

// 0x432330: the growing starts from where a shrinking left off; the flash stops at full opacity
void Button::onMouseEnter(Point& p)
{
	mScaleState = SCALE_GROWING;
	if (!(mScaleTimeLeft <= 0.0f))
		mScaleTimeLeft = mScaleTime - mScaleTimeLeft;
	else
		mScaleTimeLeft = mScaleTime;
	notifyListeners(0);
	if (mEnterSound != nullptr)
		mEnterSound->play();
	mFlashing = false;
	setOpacity(1.0f);
}

// 0x4323a0: a button with a flash period starts flashing again
void Button::onMouseExit(Point& p)
{
	float aTime = mScaleTimeLeft;
	mScaleState = SCALE_SHRINKING;
	if (!(aTime <= 0.0f))
		mScaleTimeLeft = mScaleTime - mScaleTimeLeft;
	else
		mScaleTimeLeft = mScaleTime;
	notifyListeners(2);
	if (mFlashPeriod != 0.0f)
	{
		mFlashTimeLeft = mFlashPeriod;
		mFlashUp = true;
		mFlashing = true;
	}
}

// 0x432180
void Button::notifyListeners(int theEvent)
{
	if (mListeners.size() != 0)
	{
		std::vector<ListenerEntry*>::iterator anIt = mListeners.begin();
		if (anIt != mListeners.end())
		{
			do
			{
				if (theEvent == 0)
					(*anIt)->mListener->buttonEnter((*anIt)->mName);
				else if (theEvent == 1)
					(*anIt)->mListener->buttonPress((*anIt)->mName);
				else if (theEvent == 2)
					(*anIt)->mListener->buttonExit((*anIt)->mName);
				++anIt;
			} while (anIt != mListeners.end());
		}
	}
}

// 0x429c70
ImageButton::ImageButton()
{
	mSelector = nullptr;
}

// 0x429cd0
void ImageButton::onMouseDown(Point& p)
{
	Button::onMouseDown(p);
	mSelector->select(2, 0, true);
}

// 0x429cf0
void ImageButton::onMouseEnter(Point& p)
{
	if (mPressed)
		mSelector->select(2, 0, true);
	else
		mSelector->select(1, 0, true);
	Button::onMouseEnter(p);
}

// 0x429d30
void ImageButton::onMouseExit(Point& p)
{
	mSelector->select(0, 0, true);
	Button::onMouseExit(p);
}

// 0x429d90: the button's bounds start as the up image's node's (not laid out yet)
bool ImageButton::setImages(Image* theUp, Image* theOver, Image* theDown)
{
	if (theUp == nullptr)
		return false;
	mSelector = new Selector();
	addChild(mSelector);
	theUp->setPivot(mPivotMode);
	ImageNode* aNode = new ImageNode(theUp);
	mSelector->addChild(aNode);
	mBounds = *aNode->getBounds();
	if (theOver == nullptr)
	{
		aNode = new ImageNode(theUp);
	}
	else
	{
		theOver->setPivot(mPivotMode);
		aNode = new ImageNode(theOver);
	}
	mSelector->addChild(aNode);
	if (theDown == nullptr)
	{
		aNode = new ImageNode(theOver == nullptr ? theUp : theOver);
	}
	else
	{
		theDown->setPivot(mPivotMode);
		aNode = new ImageNode(theDown);
	}
	mSelector->addChild(aNode);
	addOwnFlags(NODE_MOUSE);
	mSelector->select(0, 0, true);
	setName("ImageButton");
	return true;
}
