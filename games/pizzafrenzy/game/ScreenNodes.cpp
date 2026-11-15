#include "ScreenNodes.h"
#include "Node.h"
#include "TextItem.h"

#include <cstdio>
#include <vector>

using namespace pizzafrenzy;

namespace
{
// The size of the exe's buffer for a formatted text
const int FORMAT_BUFFER_SIZE = 1024;

// The node is the other one or below it
bool isBelow(engine::Node* theNode, engine::Node* theAncestor)
{
	for (engine::Node* aNode = theNode; aNode != nullptr; aNode = aNode->mParent)
	{
		if (aNode == theAncestor)
			return true;
	}
	return false;
}
}

// port: a node of a layout that leads out of the game is taken out of its parent and the screen
void pizzafrenzy::removeScreenNode(engine::ScreenLayout* theScreen, const std::string& theName)
{
	engine::Node* aNode = theScreen->getNode(theName);
	if (aNode == nullptr)
		return;
	std::vector<std::string> aNames;
	for (std::map<std::string, engine::Node*>::iterator anIt = theScreen->mNodes.begin();
		anIt != theScreen->mNodes.end(); ++anIt)
	{
		if (isBelow(anIt->second, aNode))
			aNames.push_back(anIt->first);
	}
	for (size_t i = 0; i < aNames.size(); i++)
		theScreen->mNodes.erase(aNames[i]);
	engine::Container* aParent = dynamic_cast<engine::Container*>(aNode->mParent);
	if (aParent != nullptr)
		aParent->removeChild(aNode);
}

// port: a button of a layout, and its labels, move to close the gap a removed button leaves
void pizzafrenzy::moveButtonWithLabels(engine::ScreenLayout* theScreen, const std::string& theButton, float theX)
{
	engine::Node* aButton = theScreen->getNode(theButton);
	if (aButton == nullptr)
		return;
	engine::Point anOldPosition = aButton->mPosition;
	engine::Container* aParent = dynamic_cast<engine::Container*>(aButton->mParent);
	if (aParent != nullptr)
	{
		for (size_t i = 0; i < aParent->mChildren.size(); i++)
		{
			engine::Node* aChild = aParent->mChildren[i];
			if (aChild == aButton || (dynamic_cast<engine::TextItem*>(aChild) != nullptr &&
				aChild->mPosition.x == anOldPosition.x && aChild->mPosition.y == anOldPosition.y))
				aChild->setPosition(theX, anOldPosition.y);
		}
	}
}

// 0x46db00: the exe's buffer is a fixed size; the text is cut at it
std::string pizzafrenzy::formatText(const std::string& theFormat, const std::string& theText)
{
	char aBuffer[FORMAT_BUFFER_SIZE];
	snprintf(aBuffer, sizeof(aBuffer), theFormat.c_str(), theText.c_str());
	return aBuffer;
}

// 0x46db00
std::string pizzafrenzy::formatText(const std::string& theFormat, int theNumber)
{
	char aBuffer[FORMAT_BUFFER_SIZE];
	snprintf(aBuffer, sizeof(aBuffer), theFormat.c_str(), theNumber);
	return aBuffer;
}

// 0x46db00
std::string pizzafrenzy::formatText(const std::string& theFormat, int theFirst, int theSecond)
{
	char aBuffer[FORMAT_BUFFER_SIZE];
	snprintf(aBuffer, sizeof(aBuffer), theFormat.c_str(), theFirst, theSecond);
	return aBuffer;
}

// 0x4bd850: FormatMessageA over the text (FORMAT_MESSAGE_FROM_STRING alone) with one insert, a string. Port: the
// function's rules written out: %1 to %99 are inserts, each with an optional !format! after it (an insert is a
// string, so the format is not read); %0 ends the text; %n is a line break, %r a carriage return, %t a tab; %%, %!,
// %. and "% " are the character itself. Line breaks in the text are kept.
std::string pizzafrenzy::formatMessage(const std::string& theText, const std::string& theInsert)
{
	std::string aResult;
	size_t i = 0;
	while (i < theText.size())
	{
		char aChar = theText[i];
		i++;
		if (aChar != '%' || i == theText.size())
		{
			aResult += aChar;
			continue;
		}
		char anEscape = theText[i];
		i++;
		if (anEscape >= '1' && anEscape <= '9')
		{
			int anInsert = anEscape - '0';
			if (i < theText.size() && theText[i] >= '0' && theText[i] <= '9')
			{
				anInsert = anInsert * 10 + (theText[i] - '0');
				i++;
			}
			if (i < theText.size() && theText[i] == '!')
			{
				size_t aFormatEnd = theText.find('!', i + 1);
				if (aFormatEnd != std::string::npos)
					i = aFormatEnd + 1;
			}
			if (anInsert == 1)
				aResult += theInsert;
		}
		else if (anEscape == '0')
			break;
		else if (anEscape == 'n')
			aResult += "\r\n";
		else if (anEscape == 'r')
			aResult += '\r';
		else if (anEscape == 't')
			aResult += '\t';
		else
			aResult += anEscape;
	}
	return aResult;
}
