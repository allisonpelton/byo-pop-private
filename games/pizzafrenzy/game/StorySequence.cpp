#include "StorySequence.h"
#include "App.h"
#include "FadeContainer.h"
#include "GameText.h"
#include "ImageNode.h"
#include "Layout.h"
#include "PizzaFrenzy.h"
#include "StoryScreen.h"

using namespace pizzafrenzy;

namespace
{
// What the player presses to go on, and the sound a press that skips ahead makes
const char* const ACTION_FAST_FORWARD = "fastForwardStep";
const char* const SOUND_CLICK = "order_click1";

// The speech's text: the width it breaks at and the letters a second, and how much faster a press makes it
const float SPEECH_WRAP = 360.0f;
const float SPEECH_RATE = 120.0f;
const float SPEECH_FAST_RATE = 2400.0f;

// Who says a dialog when the file gives no speaker (and, for no text, what it says: GT(0x4ffaa8))
const char* const NO_SPEAKER = "nobody";

// The event the screen is told when a line starts
const char* const EVENT_DIALOG_START = "dialogStart";

// The picture of the city the game is in, which the story names "currentCity"
const char* const IMAGE_CURRENT_CITY = "currentCity";
const int PIVOT_CENTRE = 1;

// A picture or a layout fades in and out of its place on the screen in this long
const float IMAGE_FADE_SECONDS = 0.2f;
const float IMAGE_FADE_TARGET = 1.0f;

// A pause lasts this long when the file gives no time
const float DEFAULT_PAUSE_SECONDS = 1.0f;

void playSound(const std::string& theName)
{
	PizzaFrenzy::instance()->mSounds->play(theName, 1.0f, 1.0f);
}

// A line of a character's speech (constructor 0x44b7f0, vtable 0x4ffa70): the character's text types itself out, and
// the action is done at once (the next action, a wait for a press, keeps the sequence there); a press of the fast
// forward types the text out much faster
class DialogAction : public StoryAction
{
public:
	DialogAction(const std::string& theText, const std::string& theSpeaker) :
		mText(theText), mSpeaker(theSpeaker)
	{
	}  // 0x44b7f0

	// 0x04: 0x44b8d0: the screen is told a line starts, and the text types out
	void start() override
	{
		mSequence->handleEvent(EVENT_DIALOG_START, mSpeaker);
		mTyper->setWrapAndTiming(SPEECH_WRAP, SPEECH_RATE, 0);
		mTyper->setTypedText(mText);
		mTyper->start();
	}
	void stop() override { mTyper->stop(); }  // 0x08: 0x44b600
	// 0x0c: 0x44b590: the speaker's portrait and bubble show, with the text ready and not typing
	void setSequence(StorySequence* theSequence) override
	{
		StoryAction::setSequence(theSequence);
		mTyper = mSequence->showSpeaker(mSpeaker);
		mTyper->setTypedText(mText);
		mTyper->reset();
	}
	bool isDone() override { return true; }  // 0x14: 0x451610
	// 0x18: 0x44b9a0
	void action(const std::string& theAction) override
	{
		if (theAction.compare(ACTION_FAST_FORWARD) == 0)
		{
			mTyper->setWrapAndTiming(SPEECH_WRAP, SPEECH_FAST_RATE, 0);
			playSound(SOUND_CLICK);
		}
	}

private:
	std::string mText;                   // +0x10
	std::string mSpeaker;                // +0x2c
	engine::Ref<engine::TextTyper> mTyper;  // +0x48: the speaker's text
};

// An event for the screen (0x54 bytes, constructor 0x44bd30, vtable 0x4ffb00): done as soon as the screen has it
class EventAction : public StoryAction
{
public:
	EventAction(const std::string& theAction, const std::string& theParam) :
		mDone(false), mAction(theAction), mParam(theParam)
	{
	}  // 0x44be70

	void start() override  // 0x04: 0x44bd10
	{
		mSequence->handleEvent(mAction, mParam);
		mDone = true;
	}
	void stop() override {}  // 0x08: 0x4d0470
	// 0x0c: 0x44bcf0
	void setSequence(StorySequence* theSequence) override
	{
		StoryAction::setSequence(theSequence);
		mDone = false;
	}
	bool isDone() override { return mDone; }  // 0x14: 0x44e1e0

private:
	bool mDone;           // +0x10
	std::string mAction;  // +0x14
	std::string mParam;   // +0x30
};

// A picture for a place of the screen (0x54 bytes, constructor 0x44c1e0, vtable 0x4ffb2c)
class ImageAction : public StoryAction
{
public:
	ImageAction(const std::string& theDestination, const std::string& theImage) :
		mDestination(theDestination), mImage(theImage), mDone(false)
	{
	}  // 0x44c1e0

	void start() override  // 0x04: 0x44c1c0
	{
		mSequence->showImage(mDestination, mImage);
		mDone = true;
	}
	void stop() override {}                   // 0x08: 0x4d0470
	bool isDone() override { return mDone; }  // 0x14: 0x44c1b0

private:
	std::string mDestination;  // +0x10: the node of the screen
	std::string mImage;        // +0x2c
	bool mDone;                // +0x48
};

// A layout for a place of the screen (0x3c bytes, constructor 0x44c5c0, vtable 0x4ffb54)
class LayoutAction : public StoryAction
{
public:
	LayoutAction(const std::string& theDestination, engine::ScreenLayout* theLayout) :
		mDestination(theDestination), mLayout(theLayout), mDone(false)
	{
	}  // 0x44c5c0

	void start() override  // 0x04: 0x44c5a0
	{
		mSequence->setNodeImage(mDestination, mLayout);
		mDone = true;
	}
	void stop() override {}                   // 0x08: 0x4d0470
	bool isDone() override { return mDone; }  // 0x14: 0x44c590

private:
	std::string mDestination;               // +0x10: the node of the screen
	engine::Ref<engine::ScreenLayout> mLayout;  // +0x2c
	bool mDone;                             // +0x30
};

// A wait (0x24 bytes, constructor 0x44cc00, vtable 0x4ffb70): the time runs only while the action is started, and a
// press of the fast forward ends it
class PauseAction : public StoryAction
{
public:
	explicit PauseAction(float theSeconds) : mRunning(false), mDone(false), mTimeLeft(0), mSeconds(theSeconds)
	{
	}  // 0x44cc00

	void start() override { mRunning = true; }  // 0x04: 0x44cae0
	void stop() override { mRunning = false; }  // 0x08: 0x44caf0
	// 0x0c: 0x44cac0
	void setSequence(StorySequence* theSequence) override
	{
		StoryAction::setSequence(theSequence);
		mTimeLeft = mSeconds;
		mRunning = false;
		mDone = false;
	}
	// 0x10: 0x44cb00
	void update(engine::UpdateInfo& theInfo) override
	{
		if (mRunning)
		{
			mTimeLeft = mTimeLeft - theInfo.mSeconds;
			if (mTimeLeft < 0.0f)
			{
				mRunning = false;
				mDone = true;
			}
		}
	}
	bool isDone() override { return mDone; }  // 0x14: 0x4797f0
	// 0x18: 0x44cca0
	void action(const std::string& theAction) override
	{
		if (theAction.compare(ACTION_FAST_FORWARD) == 0)
		{
			mTimeLeft = -1.0f;
			playSound(SOUND_CLICK);
		}
	}

private:
	bool mRunning;     // +0x10
	bool mDone;        // +0x11
	float mTimeLeft;   // +0x14
	float mSeconds;    // +0x18
};

// A sound (0x38 bytes, constructor 0x44cec0, vtable 0x4ffb8c)
class SoundAction : public StoryAction
{
public:
	explicit SoundAction(const std::string& theName) : mName(theName), mDone(false) {}  // 0x44cec0

	void start() override  // 0x04: 0x44ce80
	{
		playSound(mName);
		mDone = true;
	}
	void stop() override {}                   // 0x08: 0x4d0470
	bool isDone() override { return mDone; }  // 0x14: 0x44ceb0

private:
	std::string mName;  // +0x10
	bool mDone;         // +0x2c
};

// A wait for a press (0x74 bytes, constructor 0x44e1f0, vtable 0x4ffc0c): the action it starts with is told to the
// screen, and it is done when the player presses the action it waits for; after the delay the screen is told another
class WaitEventAction : public StoryAction
{
public:
	WaitEventAction(const std::string& thePreAction, const std::string& theWaitAction,
	                const std::string& theDelayAction, float theDelay) :
		mDone(false), mPreAction(thePreAction), mWaitAction(theWaitAction), mDelayAction(theDelayAction),
		mDelay(theDelay)
	{
	}  // 0x44e1f0

	// 0x04: 0x44e480
	void start() override
	{
		mSequence->handleEvent(mPreAction, "");
		mDone = false;
	}
	void stop() override {}  // 0x08: 0x4d0470
	// 0x0c: 0x44bcf0
	void setSequence(StorySequence* theSequence) override
	{
		StoryAction::setSequence(theSequence);
		mDone = false;
	}
	// 0x10: 0x44e500
	void update(engine::UpdateInfo& theInfo) override
	{
		if (0.0f < mDelay)
		{
			mDelay = mDelay - theInfo.mSeconds;
			if (mDelay <= 0.0f)
				mSequence->handleEvent(mDelayAction, "");
		}
	}
	bool isDone() override { return mDone; }  // 0x14: 0x44e1e0
	// 0x18: 0x44e5b0
	void action(const std::string& theAction) override
	{
		if (theAction == mWaitAction)
		{
			mDone = true;
			playSound(SOUND_CLICK);
		}
	}

private:
	bool mDone;                 // +0x10
	std::string mPreAction;     // +0x14: told to the screen when the wait starts
	std::string mWaitAction;    // +0x30: the press that ends the wait
	std::string mDelayAction;   // +0x4c: told to the screen when the delay is over
	float mDelay;               // +0x68: the seconds until then
};

// The handlers of the elements of the actions (each 0x3c bytes, vtable 0x4ffc54): the element's attributes make the
// action, which goes in the sequence; the handler goes off the stack at the end of the element
class DialogActionDocHandler : public engine::BaseDocHandler
{
public:
	DialogActionDocHandler(const std::string& theName, engine::DocHandlerStack* theParent,
	                       const engine::XmlAttributes& theAttributes, StorySequence* theSequence) :
		engine::BaseDocHandler(theName, theParent)
	{
		std::string aText = engine::getStringAttribute(theAttributes, "text", GT(0x4ffaa8));
		std::string aSpeaker = engine::getStringAttribute(theAttributes, "speaker", NO_SPEAKER);
		theSequence->addAction(new DialogAction(aText, aSpeaker));
	}  // 0x44ba70
};

class PauseActionDocHandler : public engine::BaseDocHandler
{
public:
	PauseActionDocHandler(const std::string& theName, engine::DocHandlerStack* theParent,
	                      const engine::XmlAttributes& theAttributes, StorySequence* theSequence) :
		engine::BaseDocHandler(theName, theParent)
	{
		theSequence->addAction(
			new PauseAction(engine::getFloatAttribute(theAttributes, "time", DEFAULT_PAUSE_SECONDS)));
	}  // 0x44cd50
};

class WaitEventActionDocHandler : public engine::BaseDocHandler
{
public:
	WaitEventActionDocHandler(const std::string& theName, engine::DocHandlerStack* theParent,
	                          const engine::XmlAttributes& theAttributes, StorySequence* theSequence) :
		engine::BaseDocHandler(theName, theParent)
	{
		std::string aPreAction = engine::getStringAttribute(theAttributes, "preAction", "");
		std::string aWaitAction = engine::getStringAttribute(theAttributes, "waitAction", "");
		std::string aDelayAction = engine::getStringAttribute(theAttributes, "delayAction", "");
		float aDelay = engine::getFloatAttribute(theAttributes, "delay", 0.0f);
		theSequence->addAction(new WaitEventAction(aPreAction, aWaitAction, aDelayAction, aDelay));
	}  // 0x44e660
};

class ImageActionDocHandler : public engine::BaseDocHandler
{
public:
	ImageActionDocHandler(const std::string& theName, engine::DocHandlerStack* theParent,
	                      const engine::XmlAttributes& theAttributes, StorySequence* theSequence) :
		engine::BaseDocHandler(theName, theParent)
	{
		std::string aDestination = engine::getStringAttribute(theAttributes, "dest", "");
		std::string anImage = engine::getStringAttribute(theAttributes, "img", "");
		theSequence->addAction(new ImageAction(aDestination, anImage));
	}  // 0x44c320
};

class SoundActionDocHandler : public engine::BaseDocHandler
{
public:
	SoundActionDocHandler(const std::string& theName, engine::DocHandlerStack* theParent,
	                      const engine::XmlAttributes& theAttributes, StorySequence* theSequence) :
		engine::BaseDocHandler(theName, theParent)
	{
		theSequence->addAction(new SoundAction(engine::getStringAttribute(theAttributes, "name", "")));
	}  // 0x44d000
};

class LayoutActionDocHandler : public engine::BaseDocHandler
{
public:
	LayoutActionDocHandler(const std::string& theName, engine::DocHandlerStack* theParent,
	                       const engine::XmlAttributes& theAttributes, StorySequence* theSequence) :
		engine::BaseDocHandler(theName, theParent)
	{
		std::string aDestination = engine::getStringAttribute(theAttributes, "dest", "");
		std::string aFile = engine::getStringAttribute(theAttributes, "file", "");
		engine::Ref<engine::ScreenLayout> aLayout = new engine::ScreenLayout();
		aLayout->loadLayout(aFile);
		theSequence->addAction(new LayoutAction(aDestination, aLayout));
	}  // 0x44c760
};

class EventActionDocHandler : public engine::BaseDocHandler
{
public:
	EventActionDocHandler(const std::string& theName, engine::DocHandlerStack* theParent,
	                      const engine::XmlAttributes& theAttributes, StorySequence* theSequence) :
		engine::BaseDocHandler(theName, theParent)
	{
		std::string anAction = engine::getStringAttribute(theAttributes, "action", "");
		std::string aParam = engine::getStringAttribute(theAttributes, "param", "");
		theSequence->addAction(new EventAction(anAction, aParam));
	}  // 0x44bf50
};
}

// 0x44d240
void StoryAction::setSequence(StorySequence* theSequence)
{
	mSequence = theSequence;
}

// 0x44dcb0
StorySequence::StorySequence() : mName(""), mCurrent(0), mRunning(false), mDone(false)
{
}

// 0x44dbc0
StorySequence::~StorySequence()
{
	clear();
}

// 0x44d360
void StorySequence::setScreen(StoryScreen* theScreen)
{
	mRunning = false;
	mCurrent = 0;
	mDone = false;
	mScreen = theScreen;
}

// 0x44d630
void StorySequence::start()
{
	mCurrent = 0;
	if (!mActions.empty())
	{
		mActions[0]->setSequence(this);
		mActions[mCurrent]->start();
		mRunning = true;
		mDone = false;
	}
}

// 0x44d680
void StorySequence::stop()
{
	if (!mActions.empty())
	{
		if (mCurrent < (int)mActions.size())
			mActions[mCurrent]->stop();
	}
	mRunning = false;
}

// 0x44d6d0
void StorySequence::next()
{
	mCurrent = mCurrent + 1;
	if ((int)mActions.size() <= mCurrent)
	{
		if (mRunning)
			mDone = true;
		mRunning = false;
		return;
	}
	mActions[mCurrent]->setSequence(this);
	mActions[mCurrent]->start();
}

// 0x44d730
void StorySequence::action(const std::string& theAction)
{
	if (mRunning && !mActions.empty() && mCurrent < (int)mActions.size())
		mActions[mCurrent]->action(theAction);
}

// 0x44d780
void StorySequence::update(engine::UpdateInfo& theInfo)
{
	if ((int)mActions.size() <= mCurrent)
	{
		if (mRunning)
			mDone = true;
		mRunning = false;
		return;
	}
	if (mRunning && !mActions.empty())
	{
		mActions[mCurrent]->update(theInfo);
		if (mActions[mCurrent]->isDone())
			next();
	}
}

// 0x44d8a0
void StorySequence::clear()
{
	mActions.clear();
	mScreen = nullptr;
}

// 0x44e160
void StorySequence::addAction(StoryAction* theAction)
{
	mActions.push_back(theAction);
}

// 0x44d3b0
engine::TextTyper* StorySequence::showSpeaker(const std::string& theSpeaker)
{
	return mScreen->showSpeaker(theSpeaker);
}

// 0x44d3c0
void StorySequence::handleEvent(const std::string& theEvent, const std::string& theParam)
{
	mScreen->handleEvent(theEvent, theParam);
}

// 0x44d3d0
void StorySequence::setNodeImage(const std::string& theNode, engine::Node* theImage)
{
	engine::FadeContainer* aGroup = static_cast<engine::FadeContainer*>(mScreen->getNode(theNode));
	if (aGroup != nullptr)
	{
		if (theImage != nullptr)
		{
			aGroup->removeAllChildren();
			aGroup->addChild(theImage);
			aGroup->setOpacity(0);
			aGroup->fade(true, IMAGE_FADE_SECONDS, IMAGE_FADE_TARGET, false);
			return;
		}
		aGroup->fade(false, IMAGE_FADE_SECONDS, IMAGE_FADE_TARGET, false);
	}
}

// 0x44d450
void StorySequence::showImage(const std::string& theNode, const std::string& theImage)
{
	engine::Image* anImage = findImage(theImage);
	if (anImage != nullptr)
	{
		engine::Ref<engine::ImageNode> aNode = new engine::ImageNode(anImage);
		setNodeImage(theNode, aNode);
		return;
	}
	engine::FadeContainer* aGroup = static_cast<engine::FadeContainer*>(mScreen->getNode(theNode));
	if (aGroup != nullptr)
		aGroup->fade(false, IMAGE_FADE_SECONDS, IMAGE_FADE_TARGET, false);
}

// 0x43ca00
engine::Image* StorySequence::findImage(const std::string& theName)
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	if (theName.compare(IMAGE_CURRENT_CITY) == 0)
		return anApp->mCitySelect->getPostcardImage(anApp->mCurrentCity);
	if (theName.empty())
		return nullptr;
	engine::Image* anImage = engine::App::instance()->getImage(theName.c_str());
	anImage->setPivot(PIVOT_CENTRE);
	return anImage;
}

// 0x44dd60: a "sequence" element makes the sequence; the other elements are the actions, each with a handler of its
// own on the stack for the element
void StorySequenceDocHandler::startElement(const std::string& theName, const engine::XmlAttributes& theAttributes)
{
	engine::DocHandlerStack* aStack = getParent();
	if (theName.compare("sequence") == 0)
	{
		std::string aName = engine::getStringAttribute(theAttributes, "name", "No Name");
		mSequence = new StorySequence();
		mSequence->mName = aName;
	}
	engine::Ref<engine::BaseDocHandler> aHandler;
	if (theName.compare("dialog") == 0)
		aHandler = new DialogActionDocHandler(theName, aStack, theAttributes, mSequence);
	else if (theName.compare("pause") == 0)
		aHandler = new PauseActionDocHandler(theName, aStack, theAttributes, mSequence);
	else if (theName == "waitEvent")
		aHandler = new WaitEventActionDocHandler(theName, aStack, theAttributes, mSequence);
	else if (theName == "image")
		aHandler = new ImageActionDocHandler(theName, aStack, theAttributes, mSequence);
	else if (theName == "sound")
		aHandler = new SoundActionDocHandler(theName, aStack, theAttributes, mSequence);
	else if (theName == "layout")
		aHandler = new LayoutActionDocHandler(theName, aStack, theAttributes, mSequence);
	else if (theName == "event")
		aHandler = new EventActionDocHandler(theName, aStack, theAttributes, mSequence);
	else
		return;
	aStack->addHandler(aHandler);
}
