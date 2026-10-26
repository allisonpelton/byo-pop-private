#include "Layout.h"
#include "App.h"
#include "Exception.h"
#include "FadeContainer.h"
#include "Font.h"
#include "Graphics.h"
#include "ImageNode.h"
#include "Particles.h"
#include "ScreenLayout.h"
#include "Stream.h"
#include "TextItem.h"
#include <cstdio>
#include <cstdlib>

using namespace engine;

// The attribute readers (Layout.h)

// 0x42abd0
std::string engine::getStringAttribute(XmlAttributes theAttributes, const std::string& theName,
	const std::string& theDefault)
{
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt == theAttributes.end())
		return theDefault;
	return anIt->second;
}

// 0x42ad20
long engine::getIntAttribute(XmlAttributes theAttributes, const std::string& theName, long theDefault)
{
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt == theAttributes.end())
		return theDefault;
	return atol(anIt->second.c_str());
}

// 0x42aeb0
float engine::getFloatAttribute(XmlAttributes theAttributes, const std::string& theName, float theDefault)
{
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt == theAttributes.end())
		return theDefault;
	return (float)atof(anIt->second.c_str());
}

// 0x42b040: "(x,y)", or one number for both
Point engine::getScaleAttribute(XmlAttributes theAttributes, const std::string& theName, Point theDefault)
{
	float x = theDefault.x;
	float y = theDefault.y;
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt != theAttributes.end())
	{
		std::string aValue = anIt->second;
		if (aValue.c_str()[0] == '(')
		{
			sscanf(aValue.c_str(), "(%f,%f)", &x, &y);
		}
		else
		{
			sscanf(aValue.c_str(), "%f", &x);
			y = x;
		}
	}
	return Point(x, y);
}

// 0x42b1c0: "x,y". The original reads into uninitialised locals, so a value that does not parse gives what the stack
// held; the port gives the default for the numbers that do not parse.
Point engine::getPointAttribute(XmlAttributes theAttributes, const std::string& theName, Point theDefault)
{
	Point aPoint = theDefault;
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt != theAttributes.end())
	{
		std::string aValue = anIt->second;
		float x = theDefault.x;
		float y = theDefault.y;
		sscanf(aValue.c_str(), "%f,%f", &x, &y);
		aPoint = Point(x, y);
	}
	return aPoint;
}

// 0x42b350: "true" or "false"; anything else is the default
bool engine::getBoolAttribute(XmlAttributes theAttributes, const std::string& theName, bool theDefault)
{
	bool aValue = theDefault;
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt != theAttributes.end())
	{
		std::string aText = anIt->second;
		if (aText.compare("true") == 0)
			aValue = true;
		else if (aText.compare("false") == 0)
			aValue = false;
	}
	return aValue;
}

// 0x42c7f0
LayoutHandler::LayoutHandler()
{
	mScreen = nullptr;
}

// 0x42c230: the screen and the groups are not counted
LayoutHandler::~LayoutHandler()
{
	mScreen = nullptr;
}

// 0x430910: an element the reader does not know is skipped (its children are still read)
void LayoutHandler::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (theName.compare("image") == 0)
	{
		addImage(theAttributes);
		return;
	}
	if (theName.compare("text") == 0)
	{
		addText(theAttributes);
		return;
	}
	if (theName.compare("imageButton") == 0)
	{
		addImageButton(theAttributes);
		return;
	}
	if (theName.compare("checkBox") == 0)
	{
		addCheckBox(theAttributes);
		return;
	}
	if (theName.compare("textButton") == 0)
	{
		addTextButton(theAttributes);
		return;
	}
	if (theName.compare("group") == 0)
	{
		addGroup(theAttributes);
		return;
	}
	if (theName.compare("particleSystem") == 0)
	{
		addParticleSystem(theAttributes);
		return;
	}
	if (theName.compare("textSub") == 0)
	{
		addTextSub(theAttributes);
		return;
	}
	if (theName.compare("include") == 0)
		addInclude(theAttributes);
}

// 0x42c4f0
void LayoutHandler::endElement(const std::string& theName)
{
	if (theName.compare("group") == 0)
	{
		if (mGroups.size() < 2)
			throw Exception("Mis-matched end group");
		if (mGroups.size() != 0)
			mGroups.pop_back();
	}
}

// 0x42c8c0
void LayoutHandler::startDocument()
{
	mGroups.push_back(mScreen);
}

// 0x42c380
void LayoutHandler::endDocument()
{
	if (mGroups.size() != 0)
		mGroups.pop_back();
}

// 0x430280: the alpha type and the pivot change the cached image, so they hold for every user of the file
bool LayoutHandler::addImage(const XmlAttributes& theAttributes)
{
	std::string aFile = getStringAttribute(theAttributes, "file", "");
	std::string aName = getStringAttribute(theAttributes, "name", "_Image_");
	if (aFile.compare("") == 0)
		return false;
	Image* anImage = App::instance()->getImage(aFile.c_str());
	float x = getFloatAttribute(theAttributes, "x", 0);
	float y = getFloatAttribute(theAttributes, "y", 0);
	float anOpacity = getFloatAttribute(theAttributes, "opacity", 1.0f);
	float aScaleX = getFloatAttribute(theAttributes, "scaleX", 1.0f);
	float aScaleY = getFloatAttribute(theAttributes, "scaleY", 1.0f);
	std::string anAlphaType = getStringAttribute(theAttributes, "alphaType", "");
	float aThreshold = getFloatAttribute(theAttributes, "alphaThresh", 5.0f);
	if (anAlphaType.compare("alphaTest") == 0)
	{
		anImage->setAlphaMode(ALPHA_TEST, (uint8_t)(long)aThreshold);
	}
	else if (anAlphaType.compare("alphaBlend") == 0)
	{
		anImage->setAlphaMode(ALPHA_BLEND, 0);
	}
	else if (anAlphaType.compare("none") == 0)
	{
		anImage->setAlphaMode(ALPHA_NONE, 0);
	}
	std::string aPivot = getStringAttribute(theAttributes, "pivot", "upperleft");
	if (aPivot.compare("center") == 0)
		anImage->setPivot(1);
	else
		anImage->setPivot(0);
	ImageNode* aNode = new ImageNode(anImage);
	aNode->setPosition(x, y);
	aNode->setOpacity(anOpacity);
	aNode->setScale(aScaleX, aScaleY);
	aNode->setName(aName);
	mScreen->registerNode(aNode, aName);
	mGroups.back()->addChild(aNode);
	return true;
}

// 0x42ef20: the text, the font and the name must all be given
bool LayoutHandler::addText(const XmlAttributes& theAttributes)
{
	std::string aString = getStringAttribute(theAttributes, "string", "");
	std::string aFont = getStringAttribute(theAttributes, "font", "");
	std::string aName = getStringAttribute(theAttributes, "name", "_Text_");
	float x = getFloatAttribute(theAttributes, "x", 0);
	float y = getFloatAttribute(theAttributes, "y", 0);
	float anOpacity = getFloatAttribute(theAttributes, "opacity", 1.0f);
	std::string anXAlign = getStringAttribute(theAttributes, "xAlign", "");
	std::string aYAlign = getStringAttribute(theAttributes, "yAlign", "");
	std::string aStyle = getStringAttribute(theAttributes, "style", "");
	if (aString.compare("") == 0 || aName.compare("") == 0 || aFont.compare("") == 0)
		return false;
	TextItem* aText = new TextItem();
	aText->setFont(std::string(aFont.c_str()));
	aText->setText(std::string(aString.c_str()));
	if (anXAlign.compare("center") == 0)
		aText->setHAlign(1);
	else if (anXAlign.compare("right") == 0)
		aText->setHAlign(2);
	else
		aText->setHAlign(0);
	if (aYAlign.compare("middle") == 0)
		aText->setVAlign(1);
	else if (aYAlign.compare("bottom") == 0)
		aText->setVAlign(2);
	else
		aText->setVAlign(0);
	if (aStyle.compare("underline") == 0)
		aText->mStyle = TEXT_UNDERLINE;
	else if (aStyle.compare("strikethrough") == 0)
		aText->mStyle = TEXT_STRIKETHROUGH;
	aText->setPosition(x, y);
	aText->setOpacity(anOpacity);
	aText->setName(aName);
	mScreen->registerNode(aText, std::string(aName.c_str()));
	mGroups.back()->addChild(aText);
	return true;
}

// 0x42c920: the button is pressed under its action; the over and down images are optional
bool LayoutHandler::addImageButton(const XmlAttributes& theAttributes)
{
	float x = getFloatAttribute(theAttributes, "x", 0);
	float y = getFloatAttribute(theAttributes, "y", 0);
	float anOpacity = getFloatAttribute(theAttributes, "opacity", 1.0f);
	float aScaleX = getFloatAttribute(theAttributes, "scaleX", 1.0f);
	float aScaleY = getFloatAttribute(theAttributes, "scaleY", 1.0f);
	std::string anUpFile = getStringAttribute(theAttributes, "upImg", "");
	std::string anOverFile = getStringAttribute(theAttributes, "overImg", "");
	std::string anActiveFile = getStringAttribute(theAttributes, "activeImg", "");
	std::string aName = getStringAttribute(theAttributes, "name", "_ImgButton_");
	std::string anAction = getStringAttribute(theAttributes, "action", "defaultAction");
	std::string anOverSound = getStringAttribute(theAttributes, "overSound", "");
	std::string anActiveSound = getStringAttribute(theAttributes, "activeSound", "");
	Image* anUp = App::instance()->getImage(anUpFile.c_str());
	Point anUpScale = getPointAttribute(theAttributes, "upScale", Point(1.0f, 1.0f));
	Point anOverScale = getPointAttribute(theAttributes, "overScale", Point(1.0f, 1.0f));
	float aScaleTime = getFloatAttribute(theAttributes, "scaleTime", 0.5f);
	std::string aPivot = getStringAttribute(theAttributes, "pivot", "upperLeft");
	Image* anOver = nullptr;
	if (anOverFile.compare("") != 0)
		anOver = App::instance()->getImage(anOverFile.c_str());
	Image* anActive = nullptr;
	if (anActiveFile.compare("") != 0)
		anActive = App::instance()->getImage(anActiveFile.c_str());
	if (anUp == nullptr)
		return false;
	ImageButton* aButton = new ImageButton();
	if (aPivot.compare("center") == 0)
		aButton->setPivotMode(1);
	if (!aButton->setImages(anUp, anOver, anActive))
		return false;
	aButton->setPosition(x, y);
	aButton->setOpacity(anOpacity);
	aButton->setScale(aScaleX, aScaleY);
	aButton->setHoverScale(anOverScale.x, anOverScale.y);
	aButton->setNormalScale(anUpScale.x, anUpScale.y);
	aButton->setSounds(anOverSound, anActiveSound);
	aButton->setScaleTime(aScaleTime);
	aButton->setName(aName);
	mScreen->listenTo(aButton, aName, anAction);
	mScreen->registerNode(aButton, aName);
	mGroups.back()->addChild(aButton);
	return true;
}

// 0x42d6f0: like the image button, with an unchecked and a checked image (both needed) and their over images
bool LayoutHandler::addCheckBox(const XmlAttributes& theAttributes)
{
	float x = getFloatAttribute(theAttributes, "x", 0);
	float y = getFloatAttribute(theAttributes, "y", 0);
	float anOpacity = getFloatAttribute(theAttributes, "opacity", 1.0f);
	float aScaleX = getFloatAttribute(theAttributes, "scaleX", 1.0f);
	float aScaleY = getFloatAttribute(theAttributes, "scaleY", 1.0f);
	std::string anUncheckedFile = getStringAttribute(theAttributes, "uncheckedImg", "");
	std::string anUncheckedOverFile = getStringAttribute(theAttributes, "uncheckedOverImg", "");
	std::string aCheckedFile = getStringAttribute(theAttributes, "checkedImg", "");
	std::string aCheckedOverFile = getStringAttribute(theAttributes, "checkedOverImg", "");
	std::string anOverSound = getStringAttribute(theAttributes, "overSound", "");
	std::string anActiveSound = getStringAttribute(theAttributes, "activeSound", "");
	std::string aName = getStringAttribute(theAttributes, "name", "_ImgButton_");
	std::string anAction = getStringAttribute(theAttributes, "action", "defaultAction");
	Image* anUnchecked = App::instance()->getImage(anUncheckedFile.c_str());
	Image* aChecked = App::instance()->getImage(aCheckedFile.c_str());
	Image* anUncheckedOver = nullptr;
	Image* aCheckedOver = nullptr;
	if (anUncheckedOverFile.size() != 0)
		anUncheckedOver = App::instance()->getImage(anUncheckedOverFile.c_str());
	if (aCheckedOverFile.size() != 0)
		aCheckedOver = App::instance()->getImage(aCheckedOverFile.c_str());
	if (aChecked == nullptr || anUnchecked == nullptr)
		return false;
	CheckBox* aBox = new CheckBox();
	if (!aBox->setImages(anUnchecked, anUncheckedOver, aChecked, aCheckedOver))
		return false;
	aBox->setPosition(x, y);
	aBox->setOpacity(anOpacity);
	aBox->setScale(aScaleX, aScaleY);
	aBox->setSounds(anOverSound, anActiveSound);
	aBox->setName(aName);
	mScreen->listenTo(aBox, aName, anAction);
	mScreen->registerNode(aBox, aName);
	mGroups.back()->addChild(aBox);
	return true;
}

// 0x42e2b0: the font and the over font are needed; the active font defaults to the over font
bool LayoutHandler::addTextButton(const XmlAttributes& theAttributes)
{
	std::string aString = getStringAttribute(theAttributes, "string", "");
	std::string aName = getStringAttribute(theAttributes, "name", "_Text_");
	float x = getFloatAttribute(theAttributes, "x", 0);
	float y = getFloatAttribute(theAttributes, "y", 0);
	std::string anXAlign = getStringAttribute(theAttributes, "xAlign", "");
	std::string aYAlign = getStringAttribute(theAttributes, "yAlign", "");
	std::string aFont = getStringAttribute(theAttributes, "font", "");
	std::string anOverFont = getStringAttribute(theAttributes, "overFont", "");
	std::string anActiveFont = getStringAttribute(theAttributes, "activeFont", anOverFont);
	std::string anAction = getStringAttribute(theAttributes, "action", "defaultAction");
	std::string anOverSound = getStringAttribute(theAttributes, "overSound", "");
	std::string anActiveSound = getStringAttribute(theAttributes, "activeSound", "");
	if (aFont.size() == 0 || anOverFont.size() == 0)
		return false;
	TextButton* aButton = new TextButton();
	if (!aButton->setFonts(aFont, anOverFont, anActiveFont))
		return false;
	aButton->setText(aString);
	if (anXAlign.compare("center") == 0)
		aButton->setHAlign(1);
	else if (anXAlign.compare("right") == 0)
		aButton->setHAlign(2);
	else
		aButton->setHAlign(0);
	if (aYAlign.compare("middle") == 0)
		aButton->setVAlign(1);
	else if (aYAlign.compare("bottom") == 0)
		aButton->setVAlign(2);
	else
		aButton->setVAlign(0);
	aButton->setPosition(x, y);
	aButton->setSounds(anOverSound, anActiveSound);
	aButton->setName(aName);
	mScreen->listenTo(aButton, aName, anAction);
	mScreen->registerNode(aButton, aName);
	mGroups.back()->addChild(aButton);
	return true;
}

// 0x42fa90: a fading container; its rectangle is (0, 0, width, height)
bool LayoutHandler::addGroup(const XmlAttributes& theAttributes)
{
	FadeContainer* aGroup = new FadeContainer();
	std::string aName = getStringAttribute(theAttributes, "name", "_Group_");
	float x = getFloatAttribute(theAttributes, "x", 0);
	float y = getFloatAttribute(theAttributes, "y", 0);
	float anOpacity = getFloatAttribute(theAttributes, "opacity", 1.0f);
	float aScaleX = getFloatAttribute(theAttributes, "scaleX", 1.0f);
	float aScaleY = getFloatAttribute(theAttributes, "scaleY", 1.0f);
	float aWidth = getFloatAttribute(theAttributes, "width", 0);
	float aHeight = getFloatAttribute(theAttributes, "height", 0);
	aGroup->setPosition(x, y);
	aGroup->setScale(aScaleX, aScaleY);
	aGroup->setOpacity(anOpacity);
	aGroup->setName(aName);
	aGroup->setRect(0, 0, aWidth, aHeight);
	mGroups.back()->addChild(aGroup);
	mScreen->registerNode(aGroup, aName);
	mGroups.push_back(aGroup);
	return true;
}

// 0x42f760: a system with a file is started at once (after a warm-up of the whole seconds given); it is named but
// not kept by the screen
bool LayoutHandler::addParticleSystem(const XmlAttributes& theAttributes)
{
	float x = getFloatAttribute(theAttributes, "x", 0);
	float y = getFloatAttribute(theAttributes, "y", 0);
	std::string aFile = getStringAttribute(theAttributes, "file", "");
	std::string aName = getStringAttribute(theAttributes, "name", "ParticleSystem");
	long aWarmup = getIntAttribute(theAttributes, "warmup", 0);
	if (aFile.compare("") != 0)
	{
		ParticleSystem* aSystem = new ParticleSystem();
		mGroups.back()->addChild(aSystem);
		aSystem->load(aFile);
		aSystem->setPosition(x, y);
		aSystem->startEmitting();
		aSystem->setName(aName);
		if (0 < aWarmup)
			aSystem->warmup((float)aWarmup);
	}
	return true;
}

// 0x4300c0: replaces the text of a named node, taken to be a text node as in the original (no layout uses it)
bool LayoutHandler::addTextSub(const XmlAttributes& theAttributes)
{
	std::string aName = getStringAttribute(theAttributes, "name", "");
	std::string aString = getStringAttribute(theAttributes, "string", "");
	Node* aNode = mScreen->getNode(aName);
	if (aNode != nullptr)
		static_cast<TextItem*>(aNode)->setText(aString);
	return true;
}

// 0x42fed0: another layout file read into the same screen, with groups of its own (its elements go to the screen,
// not to the group that is open here)
bool LayoutHandler::addInclude(const XmlAttributes& theAttributes)
{
	LayoutHandler aHandler;
	aHandler.setScreen(mScreen);
	std::string aFile = getStringAttribute(theAttributes, "file", "");
	if (aFile.compare("") != 0)
	{
		Ref<InputStream> aStream = App::instance()->openStream(aFile.c_str());
		parseXml(aHandler, aStream);
	}
	return true;
}
