#include "ScreenLayout.h"

#include "Engine.h"
#include "MasterDocHandler.h"
#include "ScreenDocHandler.h"

namespace engine
{

// The keys a screen fires its default and cancel actions with (Windows' VK_RETURN and VK_ESCAPE)
static const int KEY_ENTER = 0x0d;
static const int KEY_ESCAPE = 0x1b;

ScreenLayout::ScreenLayout() : mListener(nullptr), mScreenWidth(0), mScreenHeight(0)
{
	addOwnState(STATE_ANIMATED);
}

void ScreenLayout::enable()
{
	getEngine()->mKeyDownSignal.connect(this, &ScreenLayout::keyDown);
	mOwnStates &= ~STATE_INPUT;
	Container::enable();
}

void ScreenLayout::disable()
{
	getEngine()->mKeyDownSignal.disconnect(this);
	mOwnStates |= STATE_INPUT;
	Container::disable();
}

bool ScreenLayout::load(const std::string& thePath, Theme* theTheme)
{
	Engine* anEngine = getEngine();
	int aDetailLevel = anEngine->getDetailLevel();
	clearKeyActions();
	setName(thePath);
	MasterDocHandler aMaster;
	ScreenDocHandler* aHandler = new ScreenDocHandler(this, aDetailLevel, theTheme);
	aHandler->attach(&aMaster, "screen");
	return anEngine->parseXml(thePath, &aMaster);
}

void ScreenLayout::registerComponent(Component* theComponent, const std::string& theName)
{
	mComponents[theName] = theComponent;
}

void ScreenLayout::doAction(const std::string& theAction)
{
	if (mListener != nullptr)
		mListener->layoutAction(theAction);
}

void ScreenLayout::keyDown(int theKey)
{
	if (mListener != nullptr)
	{
		if (theKey == KEY_ENTER && !mDefaultAction.empty())
		{
			mListener->layoutAction(mDefaultAction);
			return;
		}
		if (theKey == KEY_ESCAPE && !mCancelAction.empty())
		{
			mListener->layoutAction(mCancelAction);
			return;
		}
	}
	std::map<int, std::string>::iterator it = mKeyActions.find(theKey);
	if (it == mKeyActions.end())
		return;
	// The original calls through the listener without a check (a crash for a screen with key actions and no
	// listener; none is known to have one)
	if (mListener != nullptr)
		mListener->layoutAction(it->second);
}

bool ScreenLayout::addKeyAction(int theKey, const std::string& theAction)
{
	return mKeyActions.insert(std::make_pair(theKey, theAction)).second;
}

Component* ScreenLayout::findComponent(const std::string& theName)
{
	std::map<std::string, Component*>::iterator it = mComponents.find(theName);
	return it == mComponents.end() ? nullptr : it->second;
}

void ScreenLayout::registerModifier(const std::string& theName, Modifier* theModifier)
{
	mModifiers.insert(std::make_pair(theName, Ref<Modifier>(theModifier)));
}

void ScreenLayout::registerBltFx(const std::string& theName, BltFx* theBltFx)
{
	mBltFxs.insert(std::make_pair(theName, Ref<BltFx>(theBltFx)));
}

Modifier* ScreenLayout::findModifier(const std::string& theName)
{
	std::map<std::string, Ref<Modifier>>::iterator it = mModifiers.find(theName);
	return it == mModifiers.end() ? nullptr : (Modifier*)it->second;
}

BltFx* ScreenLayout::findBltFx(const std::string& theName)
{
	std::map<std::string, Ref<BltFx>>::iterator it = mBltFxs.find(theName);
	return it == mBltFxs.end() ? nullptr : (BltFx*)it->second;
}

} // namespace engine
