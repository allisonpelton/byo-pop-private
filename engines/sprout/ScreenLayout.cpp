#include "ScreenLayout.h"
#include "App.h"
#include "Layout.h"
#include "Stream.h"

using namespace engine;

// 0x428640 (wordharmony): a screen takes no mouse of its own outside its bounds (mEnabled clear).
// 0x466380 (pizzafrenzy): the signal build's has no mouse flag of its own to begin with, and no name
ScreenLayout::ScreenLayout()
{
	mListener = nullptr;
	if (!gSignalBuild)
		mEnabled = false;
	addOwnFlags(NODE_LISTENERS);
	if (!gSignalBuild)
		setName("ScreenLayout");
}

// 0x427cd0 (wordharmony); 0x4648f0 (pizzafrenzy): the signal build's drops the key actions first and names the
// screen after its file, which the layout's screen element names again
bool ScreenLayout::loadLayout(std::string theFile)
{
	LayoutHandler aHandler;
	if (gSignalBuild)
		clearKeyActions();
	aHandler.setScreen(this);
	if (gSignalBuild)
		setName(theFile);
	Ref<InputStream> aStream = App::instance()->openStream(theFile.c_str());
	parseXml(aHandler, aStream);
	return true;
}

// 0x4285e0 (wordharmony), 0x466320 (pizzafrenzy)
void ScreenLayout::registerNode(Node* theNode, std::string theName)
{
	mNodes[theName] = theNode;
}

// 0x427df0 (wordharmony), 0x4649f0 (pizzafrenzy)
void ScreenLayout::listenTo(Button* theButton, std::string theName, std::string theAction)
{
	theButton->addListener(this, theAction);
}

// 0x464b10 (pizzafrenzy)
void ScreenLayout::sendAction(std::string theAction)
{
	if (mListener != nullptr)
		mListener->screenAction(theAction);
}

// 0x464b60 (pizzafrenzy): Enter and Escape need a listener for their own actions. Port: the original sends a key
// action through the listener without looking whether there is one (a screen made without a listener would crash;
// every generic screen has one)
void ScreenLayout::keyAction(int theKey)
{
	const std::string* anAction;
	if (mListener != nullptr)
	{
		if (theKey == 0x0d && mDefaultAction.size() != 0)
		{
			anAction = &mDefaultAction;
			goto send;
		}
		if (mListener != nullptr && theKey == 0x1b && mCancelAction.size() != 0)
		{
			anAction = &mCancelAction;
			goto send;
		}
	}
	{
		std::map<int, std::string>::iterator anIt = mKeyActions.find(theKey);
		if (anIt == mKeyActions.end())
			return;
		anAction = &anIt->second;
	}
send:
	if (mListener != nullptr)
		mListener->screenAction(*anAction);
}

// 0x465d90 (pizzafrenzy)
bool ScreenLayout::addKeyAction(int theKey, std::string theAction)
{
	return mKeyActions.insert(std::pair<int, std::string>(theKey, theAction)).second;
}

// 0x465d10 (pizzafrenzy)
void ScreenLayout::connect()
{
	App::instance()->mKeyDownSignal.connect(this, &ScreenLayout::onKeyDown);
	setMouseMasked(false);
	connectChildren();
	refreshMouse();
}

// 0x465430 (pizzafrenzy)
void ScreenLayout::disconnect()
{
	App::instance()->mKeyDownSignal.disconnect(this);
	setMouseMasked(true);
	disconnectChildren();
	refreshMouse();
}

// 0x427f10 (wordharmony), 0x464c00 (pizzafrenzy)
Node* ScreenLayout::getNode(std::string theName)
{
	std::map<std::string, Node*>::iterator anIt = mNodes.find(theName);
	if (anIt != mNodes.end())
		return anIt->second;
	return nullptr;
}

// 0x427e90 (wordharmony), 0x464a90 (pizzafrenzy)
void ScreenLayout::buttonPress(std::string theName)
{
	if (mListener != nullptr)
		mListener->screenAction(theName);
}
