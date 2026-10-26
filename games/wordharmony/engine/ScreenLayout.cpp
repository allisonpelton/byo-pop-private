#include "ScreenLayout.h"
#include "App.h"
#include "Layout.h"
#include "Stream.h"

using namespace engine;

// 0x428640: a screen takes no mouse of its own outside its bounds (mEnabled clear)
ScreenLayout::ScreenLayout()
{
	mListener = nullptr;
	mEnabled = false;
	addOwnFlags(NODE_LISTENERS);
	setName("ScreenLayout");
}

// 0x427cd0
bool ScreenLayout::loadLayout(std::string theFile)
{
	LayoutHandler aHandler;
	aHandler.setScreen(this);
	Ref<InputStream> aStream = App::instance()->openStream(theFile.c_str());
	parseXml(aHandler, aStream);
	return true;
}

// 0x4285e0
void ScreenLayout::registerNode(Node* theNode, std::string theName)
{
	mNodes[theName] = theNode;
}

// 0x427df0
void ScreenLayout::listenTo(Button* theButton, std::string theName, std::string theAction)
{
	theButton->addListener(this, theAction);
}

// 0x427f10
Node* ScreenLayout::getNode(std::string theName)
{
	std::map<std::string, Node*>::iterator anIt = mNodes.find(theName);
	if (anIt != mNodes.end())
		return anIt->second;
	return nullptr;
}

// 0x427e90
void ScreenLayout::buttonPress(std::string theName)
{
	if (mListener != nullptr)
		mListener->screenAction(theName);
}
