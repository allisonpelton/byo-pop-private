#include "Button.h"
#include "App.h"
#include "Font.h"
#include "Graphics.h"
#include "ImageNode.h"
#include "Scene.h"
#include "Selector.h"
#include "Sound.h"
#include "TextItem.h"

using namespace engine;

// port: the scene a node is in, the root of its parents (null when it is in none)
static Scene* findScene(Node* theNode)
{
	Node* aNode = theNode;
	while (aNode->mParent != nullptr)
		aNode = aNode->mParent;
	return dynamic_cast<Scene*>(aNode);
}

// 0x471230 (pizzafrenzy): the scene gives the mouse to the node until it is let go
static void captureMouse(Node* theNode)
{
	Scene* aScene = findScene(theNode);
	if (aScene != nullptr)
		aScene->setCapture(theNode);
}

// 0x471270 (pizzafrenzy)
static void releaseMouse(Node* theNode)
{
	Scene* aScene = findScene(theNode);
	if (aScene != nullptr)
		aScene->setCapture(nullptr);
}

// 0x432600 (wordharmony), 0x47ad40 (pizzafrenzy)
Button::Button()
{
	mState = STATE_UP;
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

// 0x432040 (wordharmony): the entries go one by one from the front
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

// 0x431c10 (wordharmony), 0x47a150 (pizzafrenzy): the scale follows the state (the growing and shrinking states count
// their time down first), then the flash
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

// 0x431b20 (wordharmony), 0x47a080 (pizzafrenzy)
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

// 0x431b90 (wordharmony), 0x47a0e0 (pizzafrenzy)
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

// 0x4327a0 (wordharmony), 0x47aeb0 (pizzafrenzy)
void Button::addListener(ButtonListener* theListener, std::string theName)
{
	ListenerEntry* anEntry = new ListenerEntry();
	anEntry->mName = theName;
	anEntry->mListener = theListener;
	mListeners.push_back(anEntry);
}

// 0x431f60 (wordharmony), 0x47a730 (pizzafrenzy): each sound is loaded under its own name, then a voice is made of it.
// The signal build's goes through the application's slots (the archive's file, App::loadSound and createSound).
void Button::setSounds(const std::string& theEnterSound, const std::string& thePressSound)
{
	if (gSignalBuild)
	{
		App* anApp = App::instance();
		if (theEnterSound.size() != 0)
		{
			anApp->loadSound(theEnterSound, theEnterSound);
			mEnterSound = anApp->createSound(theEnterSound, true, 0);
		}
		if (thePressSound.size() != 0)
		{
			anApp->loadSound(thePressSound, thePressSound);
			mPressSound = anApp->createSound(thePressSound, true, 0);
		}
		return;
	}
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

// 0x47a400 (pizzafrenzy)
void Button::setBaseScale(float x, float y)
{
	mBaseScale = Point(x, y);
	setScale(x, y);
}

// 0x47a4c0 (pizzafrenzy)
void Button::connect()
{
	setMouseMasked(false);
}

// 0x47a490 (pizzafrenzy): a button that was pressed or hovered is up again, and the scene's mouse is let go of
void Button::disconnect()
{
	if (mState != STATE_UP)
	{
		mState = STATE_UP;
		showState();
	}
	Scene* aScene = findScene(this);
	if (aScene != nullptr && aScene->getCapture() == this)
		aScene->setCapture(nullptr);
	setMouseMasked(true);
}

// 0x47a4d0 (pizzafrenzy): a press over the button takes the mouse and plays the press sound
void Button::mouseDownEvent(MouseEvent& e)
{
	if (mState == STATE_OVER)
	{
		mState = STATE_DOWN;
		showState();
		notifyListeners(NOTIFY_DOWN);
		captureMouse(this);
		if (mPressSound != nullptr)
			mPressSound->play();
	}
}

// 0x47a520 (pizzafrenzy): a release over the button is a click; one off it, with the button hovered no more, is only
// the end of the press. The mouse is let go either way.
void Button::mouseUpEvent(MouseEvent& e)
{
	if (mState == STATE_DOWN)
	{
		mState = STATE_OVER;
		showState();
		notifyListeners(NOTIFY_UP);
		click();
		releaseMouse(this);
		return;
	}
	if (mState == STATE_OUT_DOWN)
	{
		mState = STATE_UP;
		showState();
		if (0.0f < mFlashPeriod)
		{
			mFlashTimeLeft = mFlashPeriod;
			mFlashUp = true;
			mFlashing = true;
		}
		notifyListeners(NOTIFY_EXIT);
	}
	releaseMouse(this);
}

// 0x47a5d0 (pizzafrenzy): the growing starts from where a shrinking left off
void Button::mouseEnterEvent(MouseEvent& e)
{
	if (mState == STATE_OUT_DOWN)
	{
		mState = STATE_DOWN;
		showState();
		notifyListeners(NOTIFY_DOWN);
	}
	else if (mState == STATE_UP)
	{
		mState = STATE_OVER;
		showState();
		notifyListeners(NOTIFY_ENTER);
		if (mEnterSound != nullptr)
			mEnterSound->play();
	}
	mScaleState = SCALE_GROWING;
	if (!(mScaleTimeLeft <= 0.0f))
		mScaleTimeLeft = mScaleTime - mScaleTimeLeft;
	else
		mScaleTimeLeft = mScaleTime;
}

// 0x47a670 (pizzafrenzy): a pressed button the mouse leaves is up (to the listeners) and still held; a hovered one is
// left, and flashes again when it has a flash period
void Button::mouseExitEvent(MouseEvent& e)
{
	if (mState == STATE_DOWN)
	{
		mState = STATE_OUT_DOWN;
		showState();
		notifyListeners(NOTIFY_UP);
	}
	else if (mState == STATE_OVER)
	{
		mState = STATE_UP;
		showState();
		if (0.0f < mFlashPeriod)
		{
			mFlashTimeLeft = mFlashPeriod;
			mFlashUp = true;
			mFlashing = true;
		}
		notifyListeners(NOTIFY_EXIT);
	}
	mScaleState = SCALE_SHRINKING;
	if (!(mScaleTimeLeft <= 0.0f))
		mScaleTimeLeft = mScaleTime - mScaleTimeLeft;
	else
		mScaleTimeLeft = mScaleTime;
}

// 0x432300 (wordharmony)
void Button::onMouseDown(Point& p)
{
	mPressed = true;
	if (mPressSound != nullptr)
		mPressSound->play();
	notifyListeners(NOTIFY_PRESS);
}

// 0x431b10 (wordharmony)
void Button::onMouseUp(Point& p)
{
	mPressed = false;
}

// 0x432330 (wordharmony): the growing starts from where a shrinking left off; the flash stops at full opacity
void Button::onMouseEnter(Point& p)
{
	mScaleState = SCALE_GROWING;
	if (!(mScaleTimeLeft <= 0.0f))
		mScaleTimeLeft = mScaleTime - mScaleTimeLeft;
	else
		mScaleTimeLeft = mScaleTime;
	notifyListeners(NOTIFY_ENTER);
	if (mEnterSound != nullptr)
		mEnterSound->play();
	mFlashing = false;
	setOpacity(1.0f);
}

// 0x4323a0 (wordharmony): a button with a flash period starts flashing again
void Button::onMouseExit(Point& p)
{
	float aTime = mScaleTimeLeft;
	mScaleState = SCALE_SHRINKING;
	if (!(aTime <= 0.0f))
		mScaleTimeLeft = mScaleTime - mScaleTimeLeft;
	else
		mScaleTimeLeft = mScaleTime;
	notifyListeners(NOTIFY_EXIT);
	if (mFlashPeriod != 0.0f)
	{
		mFlashTimeLeft = mFlashPeriod;
		mFlashUp = true;
		mFlashing = true;
	}
}

// 0x432180 (wordharmony); 0x47a8f0 (pizzafrenzy), 0x47a950 (pizzafrenzy), 0x47a9b0 (pizzafrenzy),
// 0x47aa10 (pizzafrenzy), 0x47aa70 (pizzafrenzy), one for each event
void Button::notifyListeners(int theEvent)
{
	if (mListeners.size() != 0)
	{
		std::vector<ListenerEntry*>::iterator anIt = mListeners.begin();
		if (anIt != mListeners.end())
		{
			do
			{
				if (theEvent == NOTIFY_ENTER)
					(*anIt)->mListener->buttonEnter((*anIt)->mName);
				else if (theEvent == NOTIFY_PRESS)
					(*anIt)->mListener->buttonPress((*anIt)->mName);
				else if (theEvent == NOTIFY_EXIT)
					(*anIt)->mListener->buttonExit((*anIt)->mName);
				else if (theEvent == NOTIFY_DOWN)
					(*anIt)->mListener->buttonDown((*anIt)->mName);
				else if (theEvent == NOTIFY_UP)
					(*anIt)->mListener->buttonUp((*anIt)->mName);
				++anIt;
			} while (anIt != mListeners.end());
		}
	}
}

// 0x429c70 (wordharmony), 0x478c90 (pizzafrenzy)
ImageButton::ImageButton()
{
	mSelector = nullptr;
}

// 0x429cd0 (wordharmony)
void ImageButton::onMouseDown(Point& p)
{
	Button::onMouseDown(p);
	mSelector->select(2, 0, true);
}

// 0x429cf0 (wordharmony)
void ImageButton::onMouseEnter(Point& p)
{
	if (mPressed)
		mSelector->select(2, 0, true);
	else
		mSelector->select(1, 0, true);
	Button::onMouseEnter(p);
}

// 0x429d30 (wordharmony)
void ImageButton::onMouseExit(Point& p)
{
	mSelector->select(0, 0, true);
	Button::onMouseExit(p);
}

// 0x478bc0 (pizzafrenzy): the up image while up or held off the button, the over image over it, the down image down
void ImageButton::showState()
{
	switch (mState)
	{
	case STATE_UP:
	case STATE_OUT_DOWN:
		mSelector->select(0, 0, true);
		break;
	case STATE_OVER:
		mSelector->select(1, 0, true);
		break;
	case STATE_DOWN:
		mSelector->select(2, 0, true);
		break;
	}
}

// 0x429d90 (wordharmony), 0x478900 (pizzafrenzy): the button's bounds start as the up image's node's (not laid out
// yet). The signal build's shows a lightened copy of the up image for a missing over image, and of the over (or up)
// image for a missing down image, and keeps the selector it has (emptied) when it is given images again.
bool ImageButton::setImages(Image* theUp, Image* theOver, Image* theDown)
{
	if (gSignalBuild)
	{
		if (theUp == nullptr)
			return false;
		if (mSelector == nullptr)
		{
			mSelector = new Selector();
			addChild(mSelector);
		}
		else
			mSelector->removeAllChildren();
		theUp->setPivot(mPivotMode);
		ImageNode* aNode = new ImageNode(theUp);
		mSelector->addChild(aNode);
		mBounds = *aNode->getBounds();
		if (theOver == nullptr)
		{
			aNode = new ImageNode(theUp);
			aNode->setColorMode(1);
			aNode->setColor(0.3f, 0.3f, 0.3f);
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
			if (theOver == nullptr)
			{
				aNode->setColorMode(1);
				aNode->setColor(0.4f, 0.4f, 0.4f);
			}
		}
		else
		{
			theDown->setPivot(mPivotMode);
			aNode = new ImageNode(theDown);
		}
		mSelector->addChild(aNode);
		addOwnFlags(NODE_MOUSE);
		mSelector->select(0, 0, true);
		return true;
	}
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

// port: the layout reader's inline constructor, which sets neither field; setImages sets the state
CheckBox::CheckBox()
{
	mChecked = false;
	mSelector = nullptr;
}

// 0x434f10 (wordharmony): the press flips the state before the button's press is told
void CheckBox::onMouseDown(Point& p)
{
	mChecked = !mChecked;
	if (mChecked)
		mSelector->select(3, 0, true);
	else
		mSelector->select(1, 0, true);
	Button::onMouseDown(p);
}

// 0x434f60 (wordharmony)
void CheckBox::onMouseEnter(Point& p)
{
	if (mChecked)
		mSelector->select(3, 0, true);
	else
		mSelector->select(1, 0, true);
	Button::onMouseEnter(p);
}

// 0x434fa0 (wordharmony)
void CheckBox::onMouseExit(Point& p)
{
	if (mChecked)
		mSelector->select(2, 0, true);
	else
		mSelector->select(0, 0, true);
	Button::onMouseExit(p);
}

// 0x434ce0 (wordharmony), 0x4954a0 (pizzafrenzy): the pivots stay as they are, unlike the image button's; the signal
// build's gives each image the pivot mode
bool CheckBox::setImages(Image* theUnchecked, Image* theUncheckedOver, Image* theChecked, Image* theCheckedOver)
{
	if (theUnchecked == nullptr || theChecked == nullptr)
		return false;
	mSelector = new Selector();
	addChild(mSelector);
	if (gSignalBuild)
	{
		theUnchecked->setPivot(mPivotMode);
		mSelector->addChild(new ImageNode(theUnchecked));
		if (theUncheckedOver == nullptr)
			mSelector->addChild(new ImageNode(theUnchecked));
		else
		{
			theUncheckedOver->setPivot(mPivotMode);
			mSelector->addChild(new ImageNode(theUncheckedOver));
		}
		theChecked->setPivot(mPivotMode);
		mSelector->addChild(new ImageNode(theChecked));
		if (theCheckedOver == nullptr)
			mSelector->addChild(new ImageNode(theChecked));
		else
		{
			theCheckedOver->setPivot(mPivotMode);
			mSelector->addChild(new ImageNode(theCheckedOver));
		}
		addOwnFlags(NODE_MOUSE);
		mSelector->layout();
		layout();
		mSelector->select(0, 0, true);
		setChecked(false);
		return true;
	}
	mSelector->addChild(new ImageNode(theUnchecked));
	if (theUncheckedOver == nullptr)
		mSelector->addChild(new ImageNode(theUnchecked));
	else
		mSelector->addChild(new ImageNode(theUncheckedOver));
	mSelector->addChild(new ImageNode(theChecked));
	if (theCheckedOver == nullptr)
		mSelector->addChild(new ImageNode(theChecked));
	else
		mSelector->addChild(new ImageNode(theCheckedOver));
	addOwnFlags(NODE_MOUSE);
	mSelector->layout();
	layout();
	mSelector->select(0, 0, true);
	setChecked(false);
	return true;
}

// 0x495740 (pizzafrenzy): the release over the button flips it before the click is told
void CheckBox::click()
{
	mChecked = !mChecked;
	updateImage();
	Button::click();
}

// 0x435020 (wordharmony), 0x4957f0 (pizzafrenzy)
void CheckBox::setChecked(bool theChecked)
{
	mChecked = theChecked;
	updateImage();
}

// 0x434ff0 (wordharmony), 0x495770 (pizzafrenzy)
void CheckBox::updateImage()
{
	if (gSignalBuild)
	{
		switch (mState)
		{
		case STATE_UP:
		case STATE_OUT_DOWN:
			mSelector->select(mChecked ? 2 : 0, 0, true);
			break;
		case STATE_OVER:
		case STATE_DOWN:
			mSelector->select(mChecked ? 3 : 1, 0, true);
			break;
		}
		return;
	}
	if (mChecked)
	{
		mSelector->select(2, 0, true);
		return;
	}
	mSelector->select(0, 0, true);
}

