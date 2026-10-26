#include "DataDrivenClass.h"

#include <SDL.h>

#include "XMLParser.h"

using namespace JeffLib;
using namespace Sexy;

DataDrivenClass::Node::~Node()
{
	for (size_t i = 0; i < mGroups.size(); i++)
		delete mGroups[i];
}

// 0x6b2d40
DataDrivenClass::DataDrivenClass()
{
}

// 0x6b2da0 (0x6b2e60)
DataDrivenClass::~DataDrivenClass()
{
}

// 0x6b3280
DataDrivenClass::Node* DataDrivenClass::AddGroup(Node* theParent, const std::string& theName,
                                                 const GroupHandler& theStart, const GroupHandler& theEnd)
{
	mGroupNames.push_back(theName);
	if (theParent == nullptr)
		theParent = &mRoot;
	Node* aNode = new Node();
	aNode->mName = theName;
	aNode->mStart = theStart;
	aNode->mEnd = theEnd;
	theParent->mGroups.push_back(aNode);
	return aNode;
}

// 0x6b3420
void DataDrivenClass::AddElement(Node* theGroup, const std::string& theName, const ElementHandler& theHandler)
{
	if (theGroup == nullptr)
		theGroup = &mRoot;
	Element anElement;
	anElement.mName = theName;
	anElement.mHandler = theHandler;
	theGroup->mElements.push_back(anElement);
}

// 0x6b2f60: the root's name and the name of any group registered
bool DataDrivenClass::IsGroup(const std::string& theName) const
{
	if (theName == mRoot.mName)
		return true;
	size_t i = 0;
	while (i < mGroupNames.size()) {
		if (mGroupNames[i] == theName)
			return true;
		i++;
	}
	return false;
}

// 0x6b3040: theNode when it has the name, else the first of the groups below it (and theirs) that has
DataDrivenClass::Node* DataDrivenClass::FindGroup(const std::string& theName, Node* theNode)
{
	if (theNode->mName == theName)
		return theNode;
	size_t i = 0;
	while (i < theNode->mGroups.size()) {
		if (theNode->mGroups[i]->mName == theName)
			return theNode->mGroups[i];
		i++;
	}
	i = 0;
	while (i < theNode->mGroups.size()) {
		Node* aFound = FindGroup(theName, theNode->mGroups[i]);
		if (aFound != nullptr)
			return aFound;
		i++;
	}
	return nullptr;
}

// 0x6b31b0
const DataDrivenClass::Element* DataDrivenClass::FindElement(const std::string& theName, Node* theGroup)
{
	size_t i = 0;
	while (i < theGroup->mElements.size()) {
		if (theGroup->mElements[i].mName == theName)
			return &theGroup->mElements[i];
		i++;
	}
	return nullptr;
}

// 0x6b3820
bool DataDrivenClass::Load(const std::string& theFileName)
{
	XMLParser aParser;
	if (!aParser.OpenFile(theFileName)) {
		SDL_Log("DataDrivenClass: %s: %s", theFileName.c_str(), aParser.GetErrorText().c_str());
		return false;
	}
	mFileName = theFileName;

	std::vector<Node*> aStack;  // the groups open (a group that is not known is a null)
	XMLElement anElement;
	for (;;) {
		anElement.mAttributeIteratorList.clear();  // (the parser empties the attributes but not their order)
		if (!aParser.NextElement(&anElement))
			break;
		if (aParser.HasFailed())
			return false;
		if (anElement.mType == XMLElement::TYPE_START) {
			AttributeList anAttributes;
			for (XMLParamMap::iterator it = anElement.mAttributes.begin(); it != anElement.mAttributes.end(); ++it)
				anAttributes.push_back(std::make_pair(it->first, it->second));

			if (anElement.mSection.empty() && anElement.mValue == mRoot.mName) {
				// the root: nothing is open yet
				aStack.push_back(&mRoot);
				if (mRoot.mStart)
					mRoot.mStart();
			} else if (IsGroup(anElement.mValue)) {
				Node* aNode = nullptr;
				if (!aStack.empty() && aStack.back() != nullptr)
					aNode = FindGroup(anElement.mValue, aStack.back());
				aStack.push_back(aNode);
				if (aNode != nullptr && aNode->mStart)
					aNode->mStart();
			} else if (!aStack.empty() && aStack.back() != nullptr) {
				const Element* anElementNode = FindElement(anElement.mValue, aStack.back());
				if (anElementNode != nullptr && anElementNode->mHandler)
					anElementNode->mHandler(anAttributes);
			}
		} else if (anElement.mType == XMLElement::TYPE_END && IsGroup(anElement.mValue)) {
			if (!aStack.empty()) {
				Node* aNode = aStack.back();
				if (aNode != nullptr && aNode->mEnd)
					aNode->mEnd();
				aStack.pop_back();
			}
		}
	}
	// port: the original ended the loop without a word when the parser failed
	if (aParser.HasFailed()) {
		SDL_Log("DataDrivenClass: %s: %s", theFileName.c_str(), aParser.GetErrorText().c_str());
		return false;
	}
	return true;
}

// 0x6b3e50: for this object
bool DataDrivenClass::Reload()
{
	BeforeReload();
	bool aResult = Load(mFileName);
	AfterReload();
	return aResult;
}
