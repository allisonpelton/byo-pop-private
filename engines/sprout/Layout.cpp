#include "Layout.h"
#include "App.h"
#include "EditBox.h"
#include "Exception.h"
#include "FadeContainer.h"
#include "Font.h"
#include "Graphics.h"
#include "ImageNode.h"
#include "ListBox.h"
#include "Particles.h"
#include "ScreenLayout.h"
#include "Stream.h"
#include "Table.h"
#include "TextButton.h"
#include "TextItem.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>

using namespace engine;

// port: whether the reader is Pizza Frenzy's (the signal build's), which the early build's reader (Feeding Frenzy's) is
// not: it has the listener build's attributes and calls
static bool hasSignalReader()
{
	return gSignalBuild && !gEarlyBuild;
}

// The attribute readers (Layout.h)

// 0x42abd0 (wordharmony), 0x475890 (pizzafrenzy)
std::string engine::getStringAttribute(XmlAttributes theAttributes, const std::string& theName,
	const std::string& theDefault)
{
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt == theAttributes.end())
		return theDefault;
	return anIt->second;
}

// 0x42ad20 (wordharmony), 0x475960 (pizzafrenzy)
long engine::getIntAttribute(XmlAttributes theAttributes, const std::string& theName, long theDefault)
{
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt == theAttributes.end())
		return theDefault;
	return atol(anIt->second.c_str());
}

// 0x42aeb0 (wordharmony), 0x475a40 (pizzafrenzy)
float engine::getFloatAttribute(XmlAttributes theAttributes, const std::string& theName, float theDefault)
{
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt == theAttributes.end())
		return theDefault;
	return (float)atof(anIt->second.c_str());
}

// 0x42b040 (wordharmony): "(x,y)", or one number for both
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

// 0x42b1c0 (wordharmony), 0x475fa0 (pizzafrenzy), 0x448540 (feedingfrenzy): "x,y" (the signal build's reads "x , y",
// spaces allowed). The listener build's and the early build's originals read into uninitialised locals, so a value
// that does not parse gives what the stack held; the port gives the default for the numbers that do not parse (the
// signal build's original starts them at the default).
Point engine::getPointAttribute(XmlAttributes theAttributes, const std::string& theName, Point theDefault)
{
	Point aPoint = theDefault;
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt != theAttributes.end())
	{
		std::string aValue = anIt->second;
		float x = theDefault.x;
		float y = theDefault.y;
		sscanf(aValue.c_str(), hasSignalReader() ? " %f , %f " : "%f,%f", &x, &y);
		aPoint = Point(x, y);
	}
	return aPoint;
}

// 0x42b350 (wordharmony), 0x475ea0 (pizzafrenzy): "true" or "false"; anything else is the default
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

// 0x4761a0 (pizzafrenzy)
Color4 engine::getColorAttribute(XmlAttributes theAttributes, const std::string& theName, Color4 theDefault)
{
	Color4 aColor = theDefault;
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt != theAttributes.end())
	{
		std::string aValue = anIt->second;
		int aRed = 0;
		int aGreen = 0;
		int aBlue = 0;
		int anAlpha = 0;
		int aCount = sscanf(aValue.c_str(), " %d , %d , %d , %d ", &aRed, &aGreen, &aBlue, &anAlpha);
		if (2 < aCount)
		{
			if (aCount == 4)
			{
				aColor = Color4((float)aRed * 0.003921569f, (float)aGreen * 0.003921569f,
					(float)aBlue * 0.003921569f, (float)anAlpha * 0.003921569f);
			}
			else
			{
				aColor.r = (float)aRed * 0.003921569f;
				aColor.g = (float)aGreen * 0.003921569f;
				aColor.b = (float)aBlue * 0.003921569f;
			}
		}
	}
	return aColor;
}

// 0x448680 (feedingfrenzy): the original reads into uninitialised locals, so a value that does not parse gives what the
// stack held; the port gives the default for the numbers that do not parse
Color engine::getColorOffsetAttribute(XmlAttributes theAttributes, const std::string& theName, Color theDefault)
{
	Color aColor = theDefault;
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt != theAttributes.end())
	{
		std::string aValue = anIt->second;
		float aRed = theDefault.r;
		float aGreen = theDefault.g;
		float aBlue = theDefault.b;
		sscanf(aValue.c_str(), "%f,%f,%f", &aRed, &aGreen, &aBlue);
		aColor = Color(aRed, aGreen, aBlue);
	}
	return aColor;
}

// 0x42c7f0 (wordharmony), 0x48bfa0 (pizzafrenzy), 0x457670 (feedingfrenzy)
LayoutHandler::LayoutHandler()
{
	mScreen = nullptr;
}

// 0x42c230 (wordharmony), 0x452da0 (feedingfrenzy): the screen and the groups are not counted
LayoutHandler::~LayoutHandler()
{
	mScreen = nullptr;
}

// 0x48d250 (pizzafrenzy): the screen element sets the screen's actions and name; the elements are tried in the
// original's order, and a column goes to the open table. The text button (0x487680 (pizzafrenzy)), the selector
// (0x48c770 (pizzafrenzy)), the particle system (0x4892c0 (pizzafrenzy)), the text substitution
// (0x4896d0 (pizzafrenzy)), the include (0x48c090 (pizzafrenzy)) and the radio group (0x48cc90 (pizzafrenzy)) are
// elements of the original's reader that no layout reaches, and are not translated; their elements are skipped.
void LayoutHandler::startSignalElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (theName.compare("screen") == 0)
	{
		mScreen->setDefaultAction(getStringAttribute(theAttributes, "defaultAction", ""));
		mScreen->setCancelAction(getStringAttribute(theAttributes, "cancelAction", ""));
		mScreen->setName(getStringAttribute(theAttributes, "name", "[Untitled]"));
	}
	else if (theName.compare("image") == 0)
		addSignalImage(theAttributes);
	else if (theName.compare("text") == 0)
		addSignalText(theAttributes);
	else if (theName.compare("imageButton") == 0)
		addImageButton(theAttributes);
	else if (theName.compare("checkBox") == 0)
		addCheckBox(theAttributes);
	else if (theName.compare("group") == 0)
		addGroup(theAttributes);
	else if (theName.compare("editBox") == 0)
		addEditBox(theAttributes);
	else if (theName.compare("listBox") == 0)
		addListBox(theAttributes);
	else if (theName.compare("table") == 0)
		addTable(theAttributes);
	else if (mTable != nullptr && theName.compare("column") == 0)
		addColumn(theAttributes);
	else if (theName.compare("keyAction") == 0)
		addKeyAction(theAttributes);
}

// 0x457ee0 (feedingfrenzy): the elements are tried in the original's order. Unlike the signal build's reader, it has no
// screen element, key actions, edit box, list box, table and column, and its image and text are the listener build's.
void LayoutHandler::startEarlyElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (theName.compare("image") == 0)
		addImage(theAttributes);
	else if (theName.compare("text") == 0)
		addText(theAttributes);
	else if (theName.compare("imageButton") == 0)
		addImageButton(theAttributes);
	else if (theName.compare("checkBox") == 0)
		addCheckBox(theAttributes);
	else if (theName.compare("textButton") == 0)
		addTextButton(theAttributes);
	else if (theName.compare("group") == 0)
		addGroup(theAttributes);
	else if (theName.compare("particleSystem") == 0)
		addParticleSystem(theAttributes);
	else if (theName.compare("textSub") == 0)
		addTextSub(theAttributes);
	else if (theName.compare("include") == 0)
		addInclude(theAttributes);
}

// 0x430910 (wordharmony), 0x48d250 (pizzafrenzy), 0x457ee0 (feedingfrenzy): an element the reader does not know is
// skipped (its children are still read)
void LayoutHandler::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (gEarlyBuild)
	{
		startEarlyElement(theName, theAttributes);
		return;
	}
	if (gSignalBuild)
	{
		startSignalElement(theName, theAttributes);
		return;
	}
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
	if (theName.compare("group") == 0)
	{
		addGroup(theAttributes);
		return;
	}
	if (theName.compare("particleSystem") == 0)
		addParticleSystem(theAttributes);
}

// 0x42c4f0 (wordharmony), 0x48b7f0 (pizzafrenzy), 0x457370 (feedingfrenzy): the signal build's also ends a selector like
// a group (the selector element is not translated), lays the table out when it ends and lets go of it (its radio
// group's end is not translated either); the early build's ends only a group
void LayoutHandler::endElement(const std::string& theName)
{
	if (hasSignalReader() && theName.compare("table") == 0)
	{
		mTable->layout();
		mTable = nullptr;
		return;
	}
	if (theName.compare("group") == 0 || (hasSignalReader() && theName.compare("selector") == 0))
	{
		if (mGroups.size() < 2)
			throw Exception("Mis-matched end group");
		if (mGroups.size() != 0)
			mGroups.pop_back();
	}
}

// 0x42c8c0 (wordharmony), 0x48cc30 (pizzafrenzy), 0x457e80 (feedingfrenzy)
void LayoutHandler::startDocument()
{
	mGroups.push_back(mScreen);
}

// 0x42c380 (wordharmony), 0x484c40 (pizzafrenzy), 0x452f50 (feedingfrenzy)
void LayoutHandler::endDocument()
{
	if (mGroups.size() != 0)
		mGroups.pop_back();
}

// 0x484ea0 (pizzafrenzy): the image of the archive (the alpha type changes the cached image, as in the listener
// build's), pre-scaled by its scale's size when that is not 1 (the node's scale then keeps only the sign), and with a
// pivot only when the file says "center" or "upperleft"; the node's smoothing comes from "smoothType"
bool LayoutHandler::addSignalImage(const XmlAttributes& theAttributes)
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
		anImage->setAlphaMode(ALPHA_TEST, (uint8_t)(long)aThreshold);
	else if (anAlphaType.compare("alphaBlend") == 0)
		anImage->setAlphaMode(ALPHA_BLEND, 0);
	else if (anAlphaType.compare("none") == 0)
		anImage->setAlphaMode(ALPHA_NONE, 0);
	if (aScaleX != 1.0f || aScaleY != 1.0f)
		anImage = anImage->createScaled(aScaleX, aScaleY);
	float aNodeScaleX = aScaleX < 0.0f ? -1.0f : 1.0f;
	float aNodeScaleY = aScaleY < 0.0f ? -1.0f : 1.0f;
	std::string aPivot = getStringAttribute(theAttributes, "pivot", "");
	if (aPivot.compare("center") == 0)
		anImage->setPivot(1);
	else if (aPivot.compare("upperleft") == 0)
		anImage->setPivot(0);
	ImageNode* aNode = new ImageNode(anImage);
	aNode->setPosition(x, y);
	aNode->setOpacity(anOpacity);
	aNode->setScale(aNodeScaleX, aNodeScaleY);
	aNode->setName(aName);
	std::string aSmoothType = getStringAttribute(theAttributes, "smoothType", "");
	aNode->setSmoothType(aSmoothType.compare("smoothAll") == 0 ? 1 : 0);
	mScreen->registerNode(aNode, aName);
	mGroups.back()->addChild(aNode);
	return true;
}

// 0x430280 (wordharmony), 0x453120 (feedingfrenzy): the alpha type and the pivot change the cached image, so they hold
// for every user of the file
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

// 0x488550 (pizzafrenzy): the text and the name may not both be missing, and the font must be given. The colour (a mode
// 2 colour of the node, set whatever it is: the original compares its components, which are 0 to 1, with 255), the wrap
// width, the style (any of "underline", "strikethrough" and "shadow" in it, not the whole value) and the shadow come
// with it.
bool LayoutHandler::addSignalText(const XmlAttributes& theAttributes)
{
	std::string aString = getStringAttribute(theAttributes, "string", "");
	std::string aFont = getStringAttribute(theAttributes, "font", "");
	std::string aName = getStringAttribute(theAttributes, "name", "_Text_");
	float x = getFloatAttribute(theAttributes, "x", 0);
	float y = getFloatAttribute(theAttributes, "y", 0);
	float anOpacity = getFloatAttribute(theAttributes, "opacity", 1.0f);
	Color4 aColor = getColorAttribute(theAttributes, "color", Color4::fromBytes(255, 255, 255));
	long aWordWrap = getIntAttribute(theAttributes, "wordWrap", 0);
	std::string anXAlign = getStringAttribute(theAttributes, "xAlign", "");
	std::string aYAlign = getStringAttribute(theAttributes, "yAlign", "");
	std::string aStyle = getStringAttribute(theAttributes, "style", "");
	if ((aString.compare("") == 0 && aName.compare("") == 0) || aFont.compare("") == 0)
		return false;
	TextItem* aText = new TextItem();
	if (0 < aWordWrap)
		aText->mWordWrap = (int)aWordWrap;
	aText->setFont(std::string(aFont.c_str()));
	aText->setText(std::string(aString.c_str()));
	if (aColor.r != 255.0f || aColor.g != 255.0f || aColor.b != 255.0f)
	{
		aText->setColorMode(2);
		aText->setColor(aColor.r, aColor.g, aColor.b);
	}
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
	if (aStyle.find("underline") != std::string::npos)
		aText->mStyle |= TEXT_UNDERLINE;
	if (aStyle.find("strikethrough") != std::string::npos)
		aText->mStyle |= TEXT_STRIKETHROUGH;
	if (aStyle.find("shadow") != std::string::npos)
		aText->mStyle |= TEXT_SHADOW;
	Point aShadowOffset = getPointAttribute(theAttributes, "shadowOffset", Point(1.0f, 1.0f));
	float aShadowOpacity = getFloatAttribute(theAttributes, "shadowOpacity", 1.0f);
	aText->setShadow(aShadowOpacity, aShadowOffset);
	aText->setPosition(x, y);
	aText->setOpacity(anOpacity);
	aText->setName(aName);
	mScreen->registerNode(aText, std::string(aName.c_str()));
	mGroups.back()->addChild(aText);
	return true;
}

// 0x42ef20 (wordharmony), 0x4562f0 (feedingfrenzy): the text, the font and the name must all be given
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

// 0x42c920 (wordharmony), 0x4858a0 (pizzafrenzy), 0x453930 (feedingfrenzy): the button is pressed under its action (the
// screen listens to it with listenTo); the over and down images are optional. The signal build's also gives the button
// its scale as the base scale its hover scales multiply.
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
	if (hasSignalReader())
		aButton->setBaseScale(aScaleX, aScaleY);
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

// 0x42d6f0 (wordharmony), 0x4867a0 (pizzafrenzy), 0x4547b0 (feedingfrenzy): like the image button, with an unchecked
// and a checked image (both needed) and their over images. The signal build's has a pivot like the image button's and
// can start checked ("startOn"); its radio group, which no layout opens, is not translated (the screen listens to the
// box).
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
	std::string aPivot = getStringAttribute(theAttributes, "pivot", "upperLeft");
	Image* anUnchecked = App::instance()->getImage(anUncheckedFile.c_str());
	Image* aChecked = App::instance()->getImage(aCheckedFile.c_str());
	Image* anUncheckedOver = nullptr;
	Image* aCheckedOver = nullptr;
	if (anUncheckedOverFile.size() != 0)
		anUncheckedOver = App::instance()->getImage(anUncheckedOverFile.c_str());
	if (aCheckedOverFile.size() != 0)
		aCheckedOver = App::instance()->getImage(aCheckedOverFile.c_str());
	bool aStartOn = false;
	if (hasSignalReader())
		aStartOn = getBoolAttribute(theAttributes, "startOn", false);
	if (aChecked == nullptr || anUnchecked == nullptr)
		return false;
	CheckBox* aBox = new CheckBox();
	if (hasSignalReader() && aPivot.compare("center") == 0)
		aBox->setPivotMode(1);
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
	if (hasSignalReader())
		aBox->setChecked(aStartOn);
	return true;
}

// 0x42fa90 (wordharmony), 0x48c2a0 (pizzafrenzy), 0x457990 (feedingfrenzy): a fading container; its rectangle is
// (0, 0, width, height), which the signal build's lays out at once
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
	if (hasSignalReader())
		aGroup->layout();
	mGroups.back()->addChild(aGroup);
	mScreen->registerNode(aGroup, aName);
	mGroups.push_back(aGroup);
	return true;
}

// 0x42f760 (wordharmony), 0x456d20 (feedingfrenzy): a system with a file is started at once (after a warm-up of the
// whole seconds given); it is named but not kept by the screen
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

// 0x455440 (feedingfrenzy): the font and the over font must be given, and the active font is the over font unless the
// file says otherwise. The three colours are offsets of the text's colour in the up, over and active states ("r,g,b",
// none by default). The original reads no opacity and no style.
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
	Color anUpColor = getColorOffsetAttribute(theAttributes, "upAddColor", Color(0, 0, 0));
	Color anOverColor = getColorOffsetAttribute(theAttributes, "overAddColor", Color(0, 0, 0));
	Color anActiveColor = getColorOffsetAttribute(theAttributes, "activeAddColor", Color(0, 0, 0));
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
	aButton->setAddColors(anUpColor, anOverColor, anActiveColor);
	mScreen->listenTo(aButton, aName, anAction);
	mScreen->registerNode(aButton, aName);
	mGroups.back()->addChild(aButton);
	return true;
}

// 0x457120 (feedingfrenzy): replaces the text of a named node of the screen, which the original takes to be a text
// node; a name the screen does not have is skipped (the port also leaves a node that is not a text node alone)
bool LayoutHandler::addTextSub(const XmlAttributes& theAttributes)
{
	std::string aName = getStringAttribute(theAttributes, "name", "");
	std::string aString = getStringAttribute(theAttributes, "string", "");
	TextItem* aText = dynamic_cast<TextItem*>(mScreen->getNode(aName));
	if (aText != nullptr)
		aText->setText(aString);
	return true;
}

// 0x457740 (feedingfrenzy): another layout file of the archive read into the same screen, by a handler of its own (its
// elements go into the screen, not into the group that is open here, and its groups are its own)
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

// 0x489910 (pizzafrenzy): the box is a child of the open group before its attributes are read, and its name is the
// screen's key for it only (the node has none)
bool LayoutHandler::addEditBox(const XmlAttributes& theAttributes)
{
	EditBox* aBox = new EditBox();
	mGroups.back()->addChild(aBox);
	std::string aName = getStringAttribute(theAttributes, "name", "_EditBox_");
	std::string aFont = getStringAttribute(theAttributes, "font", "");
	std::string aBackgroundImage = getStringAttribute(theAttributes, "bgImg", "");
	std::string aCursorImage = getStringAttribute(theAttributes, "cursorImg", "");
	aBox->setImages(aBackgroundImage, aCursorImage, aFont);
	std::string anAction = getStringAttribute(theAttributes, "action", "");
	aBox->setAction(anAction);
	float x = getFloatAttribute(theAttributes, "x", 0);
	float y = getFloatAttribute(theAttributes, "y", 0);
	float anOpacity = getFloatAttribute(theAttributes, "opacity", 1.0f);
	aBox->setPosition(x, y);
	aBox->setOpacity(anOpacity);
	Point aCursorOffset = getPointAttribute(theAttributes, "cursorOffset", Point(0, 0));
	aBox->setCursorOffset(aCursorOffset);
	float aBlinkTime = getFloatAttribute(theAttributes, "cursorBlinkTime", 1.0f);
	aBox->setCursorBlinkTime(aBlinkTime);
	Point aTextOffset = getPointAttribute(theAttributes, "textOffset", Point(0, 0));
	aBox->setTextOffset(aTextOffset);
	long aMaxLength = getIntAttribute(theAttributes, "strLen", 12);
	aBox->setMaxLength((int)aMaxLength);
	mScreen->registerNode(aBox, aName);
	return true;
}

// 0x48a160 (pizzafrenzy): a colour attribute has the box's colour as its default; the selection colour only gives the
// default of the selection background colour, and the selection text and highlight colours default to the text and
// highlight colours read
bool LayoutHandler::addListBox(const XmlAttributes& theAttributes)
{
	ListBox* aBox = new ListBox();
	std::string aName = getStringAttribute(theAttributes, "name", "_ListBox_");
	float x = getFloatAttribute(theAttributes, "x", 0);
	float y = getFloatAttribute(theAttributes, "y", 0);
	float anOpacity = getFloatAttribute(theAttributes, "opacity", 1.0f);
	float aScaleX = getFloatAttribute(theAttributes, "scaleX", 1.0f);
	float aScaleY = getFloatAttribute(theAttributes, "scaleY", 1.0f);
	float aWidth = getFloatAttribute(theAttributes, "width", 0);
	float aHeight = getFloatAttribute(theAttributes, "height", 0);
	long anIndent = getIntAttribute(theAttributes, "indent", 0);
	std::string anOverSound = getStringAttribute(theAttributes, "overSound", "");
	std::string anActiveSound = getStringAttribute(theAttributes, "activeSound", "");
	Color4 aBackgroundColor = getColorAttribute(theAttributes, "backgroundColor", aBox->getBackgroundColor());
	Color4 aTextColor = getColorAttribute(theAttributes, "textColor", aBox->getTextColor());
	Color4 aHighlightColor = getColorAttribute(theAttributes, "highlightColor", aBox->getHighlightColor());
	Color4 aSelectionColor = getColorAttribute(theAttributes, "selectionColor", aBox->getSelectionBackgroundColor());
	aSelectionColor = getColorAttribute(theAttributes, "selectionBackgroundColor", aSelectionColor);
	Color4 aSelectionTextColor = getColorAttribute(theAttributes, "selectionTextColor", aTextColor);
	Color4 aSelectionHighlightColor = getColorAttribute(theAttributes, "selectionHighlightColor", aHighlightColor);
	std::string aFontName = getStringAttribute(theAttributes, "font", "");
	Font* aFont = App::instance()->getFont(aFontName.c_str());
	aBox->setPosition(x, y);
	aBox->setScale(aScaleX, aScaleY);
	aBox->setOpacity(anOpacity);
	aBox->setName(aName);
	aBox->setSize(aWidth, aHeight);
	aBox->setFont(aFont);
	aBox->setBackgroundColor(aBackgroundColor);
	aBox->setTextColor(aTextColor);
	aBox->setHighlightColor(aHighlightColor);
	aBox->setSelectionBackgroundColor(aSelectionColor);
	aBox->setSelectionTextColor(aSelectionTextColor);
	aBox->setSelectionHighlightColor(aSelectionHighlightColor);
	aBox->setIndent((int)anIndent);
	aBox->setSounds(anOverSound, anActiveSound);
	aBox->layout();
	mGroups.back()->addChild(aBox);
	mScreen->registerNode(aBox, aName);
	return true;
}

// 0x48ade0 (pizzafrenzy): the table stays open (mTable) until its end element, for its columns; the selection colours
// default to the background and text colours read
bool LayoutHandler::addTable(const XmlAttributes& theAttributes)
{
	mTable = new Table();
	std::string aName = getStringAttribute(theAttributes, "name", "_Table_");
	float x = getFloatAttribute(theAttributes, "x", 0);
	float y = getFloatAttribute(theAttributes, "y", 0);
	long aRows = getIntAttribute(theAttributes, "rows", 10);
	Color4 aBackgroundColor = getColorAttribute(theAttributes, "backgroundColor", mTable->getBackgroundColor());
	Color4 aTextColor = getColorAttribute(theAttributes, "textColor", mTable->getTextColor());
	Color4 aSelectionBackgroundColor = getColorAttribute(theAttributes, "selectionBackgroundColor", aBackgroundColor);
	Color4 aSelectionTextColor = getColorAttribute(theAttributes, "selectionTextColor", aTextColor);
	std::string aFontName = getStringAttribute(theAttributes, "font", "");
	Font* aFont = App::instance()->getFont(aFontName.c_str());
	mTable->setName(aName);
	mTable->setPosition(x, y);
	mTable->setFont(aFont);
	mTable->setRows((int)aRows);
	mTable->setBackgroundColor(aBackgroundColor);
	mTable->setTextColor(aTextColor);
	mTable->setSelectionBackgroundColor(aSelectionBackgroundColor);
	mTable->setSelectionTextColor(aSelectionTextColor);
	mGroups.back()->addChild(mTable);
	mScreen->registerNode(mTable, aName);
	return true;
}

// 0x48b4f0 (pizzafrenzy): a column of the open table; "center" and "right" are the aligns 1 and 2, anything else 0
bool LayoutHandler::addColumn(const XmlAttributes& theAttributes)
{
	std::string aName = getStringAttribute(theAttributes, "name", "");
	long aWidth = getIntAttribute(theAttributes, "width", 0);
	std::string anXAlign = getStringAttribute(theAttributes, "xAlign", "");
	int anAlign;
	if (anXAlign.compare("center") == 0)
		anAlign = 1;
	else if (anXAlign.compare("right") == 0)
		anAlign = 2;
	else
		anAlign = 0;
	int aColumn = mTable->addColumn();
	mTable->setColumnName(aColumn, aName);
	mTable->setColumnWidth(aColumn, (int)aWidth);
	mTable->setColumnAlign(aColumn, anAlign);
	return true;
}

// 0x48b990 (pizzafrenzy): a key (the first character of the "key" attribute, upper-cased, or with type other than
// "ascii" the number in it) to an action; both must be given, and a key that has an action already is logged
bool LayoutHandler::addKeyAction(const XmlAttributes& theAttributes)
{
	std::string anAction = getStringAttribute(theAttributes, "action", "");
	std::string aType = getStringAttribute(theAttributes, "type", "ascii");
	std::string aKey = getStringAttribute(theAttributes, "key", "");
	if (aKey.size() != 0 && anAction.size() != 0)
	{
		int aKeyCode;
		if (aType.compare("ascii") == 0)
			aKeyCode = toupper((int)aKey[0]);
		else
			aKeyCode = (int)getIntAttribute(theAttributes, "key", 0);
		if (0 < aKeyCode)
		{
			if (!mScreen->addKeyAction(aKeyCode, anAction))
			{
				char aMessage[96];
				snprintf(aMessage, sizeof(aMessage), "Warning: duplicate action for key code: %d\n", aKeyCode);
				App::instance()->log(aMessage);
			}
		}
	}
	return true;
}
