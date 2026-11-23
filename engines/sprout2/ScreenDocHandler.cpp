#include "ScreenDocHandler.h"

#include "BltFx.h"
#include "DrawContext.h"
#include "Engine.h"
#include "Exception.h"
#include "Image.h"
#include "ComboButton.h"
#include "EditBox.h"
#include "ImageButton.h"
#include "ListBox.h"
#include "TheoraReader.h"
#include "Modifiers.h"
#include "ParticleSystem.h"
#include "Random.h"
#include "ScreenLayout.h"
#include "Theme.h"

#include <cctype>
#include <cmath>
#include <cstdio>

namespace engine
{

// 0x4d7df0 and 0x4d7e10 (feedingfrenzy2): statics' initializers, pi over 180 in a double, kept as a float
static const float DEGREES_TO_RADIANS = (float)(3.141592653589793 * 0.005555555555555556);

// 0x45b6b0 (feedingfrenzy2): a string compared with a literal
static bool equals(const std::string& theString, const char* theLiteral)
{
	return theString == theLiteral;
}

ScreenDocHandler::ScreenDocHandler(ScreenLayout* theLayout, int theDetailLevel, Theme* theTheme)
	: mLayout(theLayout), mTheme(theTheme), mDetailLevel(theDetailLevel)
{
	char aBuffer[16];
	snprintf(aBuffer, sizeof(aBuffer), "%d", theDetailLevel);
	mDetailString = aBuffer;
	mReading.push_back(true);
}

void ScreenDocHandler::begin(Params& theAttributes)
{
	mLayout->setDefaultAction(theAttributes.getString("defaultAction", ""));
	mLayout->setCancelAction(theAttributes.getString("cancelAction", ""));
	mLayout->setName(theAttributes.getString("name", "[Untitled]"));
	Vector2<int> aSize = theAttributes.getIntPair("screenSize", Vector2<int>(0, 0));
	mLayout->setScreenSize(aSize.x, aSize.y);
	std::string aTheme = theAttributes.getString("defaultTheme", "");
	if (!aTheme.empty() && mTheme == nullptr)
	{
		mTheme = new Theme();
		mTheme->load(aTheme);
	}
}

void ScreenDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	bool aRead = true;
	std::string aDetail = theAttributes.getString("detail", "");
	if (!aDetail.empty())
		aRead = aDetail.find_first_of(mDetailString) != std::string::npos;
	aRead = aRead && mReading.back();
	mReading.push_back(aRead);
	if (!aRead)
		return;

	if (theName == "image")
		readImage(theAttributes);
	else if (theName == "group")
		readGroup(theAttributes, false, false);
	else if (theName == "attachPoint")
		readGroup(theAttributes, false, true);
	else if (theName == "compositeGroup")
		readGroup(theAttributes, true, false);
	else if (theName == "modifier")
		readModifier(theAttributes);
	else if (theName == "bltFx")
		readBltFx(theAttributes);
	else if (theName == "particleSystem")
		readParticleSystem(theAttributes);
	else if (theName == "imageButton")
		readImageButton(theAttributes);
	else if (theName == "text")
		readText(theAttributes);
	else if (theName == "keyAction")
		readKeyAction(theAttributes);
	else if (theName == "comboButton")
		readComboButton(theAttributes);
	else if (theName == "editBox")
		readEditBox(theAttributes);
	else if (theName == "listBox")
		readListBox(theAttributes);
	else
	{
		// animation 0x53fffd, checkBox 0x46e570, textButton 0x421160,
		// clipGroup 0x535b3e, selector 0x535883, textSub 0x53a144,
		// include 0x53578b, radioGroup 0x4177a0, table 0x40ca40, column
		// 0x40e640, detailSelector 0x5328bf (feedingfrenzy2): not yet translated
		getEngine()->log("ScreenDocHandler: <%s> not yet translated\n", theName.c_str());
	}
}

void ScreenDocHandler::endElement(const std::string& theName)
{
	if (mReading.back())
	{
		if (theName == "group" || theName == "selector" || theName == "compositeGroup" || theName == "attachPoint" ||
			theName == "detailSelector" || theName == "clipGroup")
		{
			if (mContainers.size() < 2)
				throw Exception("Mis-matched end group");
			// (a compositeGroup's end, the current container's slot 0x11c, comes with CompositeContainer)
			if (!mContainers.empty())
				mContainers.pop_back();
		}
		// (radioGroup's 0x4909b0 and table's end come with their elements)
	}
	if (!mReading.empty())
		mReading.pop_back();
}

void ScreenDocHandler::startDocument()
{
	mContainers.push_back(mLayout);
}

void ScreenDocHandler::endDocument()
{
	if (!mContainers.empty())
		mContainers.pop_back();
}

Surface* ScreenDocHandler::findImage(const std::string& thePath)
{
	if (mTheme != nullptr)
	{
		Surface* aSurface = mTheme->getImage(thePath);
		if (aSurface != nullptr)
			return aSurface;
	}
	return getEngine()->getImage(thePath);
}

Font* ScreenDocHandler::findFont(const std::string& thePath)
{
	if (mTheme != nullptr)
	{
		Font* aFont = mTheme->getFont(thePath);
		if (aFont != nullptr)
			return aFont;
	}
	if (thePath.empty())
		return nullptr;
	return getEngine()->getFont(thePath);
}

Color ScreenDocHandler::readColor(Params& theAttributes, const std::string& theName, const Color& theDefault)
{
	std::string aValue = theAttributes.getString(theName, "");
	if (aValue.find(',') != std::string::npos)
		return theAttributes.getColor(theName, Color(channelOf(255), channelOf(255), channelOf(255), 1.0f));
	if (mTheme != nullptr && !aValue.empty())
		return mTheme->getColor(aValue);
	return theDefault;
}

bool ScreenDocHandler::sHonorVisible = false;
bool ScreenDocHandler::sScaleImages = false;

void ScreenDocHandler::readCommon(Params& theAttributes, Component* theComponent)
{
	if (sHonorVisible && !theAttributes.getBool("visible", true))
		theComponent->setVisible(false);
	theComponent->setFilterMode(equals(theAttributes.getString("smoothType", ""), "smoothAll") ? 1 : 0);
	std::string aModifierName = theAttributes.getString("modifier", "");
	if (!aModifierName.empty())
	{
		Modifier* aModifier = mLayout->findModifier(aModifierName);
		if (aModifier != nullptr)
			theComponent->addModifier(aModifier);
	}
	std::string aBltFxName = theAttributes.getString("bltFx", "");
	if (!aBltFxName.empty())
	{
		BltFx* aBltFx = mLayout->findBltFx(aBltFxName);
		if (aBltFx != nullptr)
			theComponent->setPixelFx(aBltFx);
	}
}

void ScreenDocHandler::readImage(Params& theAttributes)
{
	std::string aFile = theAttributes.getString("file", "");
	std::string aTheoraFile = theAttributes.getString("theoraFile", "");
	int aTheoraFrame = theAttributes.getInt("theoraFrame", 0);
	std::string aName = theAttributes.getString("name", "");
	Surface* aSurface = nullptr;
	Ref<Surface> aFrame;  // port: a Theora frame kept until the image holds it or its scaled copy
	if (!aFile.empty())
		aSurface = findImage(aFile);
	else if (!aTheoraFile.empty())
	{
		// A frame of a Theora animation, read from its file by a reader of its own
		TheoraReader aReader;
		aReader.open(aTheoraFile);
		aFrame = aReader.readFrameAt((unsigned int)aTheoraFrame);
		aSurface = aFrame.get();
	}
	if (aSurface == nullptr)
		return;

	float x = theAttributes.getFloat("x", 0.0f);
	float y = theAttributes.getFloat("y", 0.0f);
	float anOpacity = theAttributes.getFloat("opacity", 1.0f);
	float aScaleX = theAttributes.getFloat("scaleX", 1.0f);
	float aScaleY = theAttributes.getFloat("scaleY", 1.0f);
	float aRotation = (float)((double)theAttributes.getFloat("rotate", 0.0f) * DEGREES_TO_RADIANS);
	std::string anAlphaType = theAttributes.getString("alphaType", "");
	float anAlphaThreshold = theAttributes.getFloat("alphaThresh", 5.0f);
	if (equals(anAlphaType, "alphaTest"))
		aSurface->setBlendMode(BLEND_TEST, (int)anAlphaThreshold);
	else if (equals(anAlphaType, "alphaBlend"))
		aSurface->setBlendMode(BLEND_ALPHA, 0);
	else if (equals(anAlphaType, "alphaAdd"))
		aSurface->setBlendMode(BLEND_ADD, 0);
	else if (equals(anAlphaType, "none"))
		aSurface->setBlendMode(BLEND_OPAQUE, 0);

	if (!sScaleImages)
	{
		// Feeding Frenzy 2's reader: a scaled copy in its place, the image's own scale left to flip it
		if (fabsf(aScaleX) != 1.0f || fabsf(aScaleY) != 1.0f)
			aSurface = aSurface->scaledCopy(aScaleX, aScaleY);
		aScaleX = aScaleX < 0.0f ? -1.0f : 1.0f;
		aScaleY = aScaleY < 0.0f ? -1.0f : 1.0f;
	}

	std::string aPivot = theAttributes.getString("pivot", "");
	if (equals(aPivot, "center"))
		aSurface->setPivot(PIVOT_CENTER);
	else if (equals(aPivot, "upperleft"))
		aSurface->setPivot(PIVOT_UPPER_LEFT);

	Image* anImage = new Image(aSurface);
	anImage->setPosition(x, y);
	anImage->setOpacity(anOpacity);
	anImage->setScale(aScaleX, aScaleY);
	if (!aName.empty())
		anImage->setName(aName);
	anImage->setRotation(aRotation);
	anImage->setFilterMode(equals(theAttributes.getString("smoothType", ""), "smoothAll") ? 1 : 0);
	Color aColor = readColor(theAttributes, "color", Color(1.0f, 1.0f, 1.0f, 1.0f));
	std::string aColorMode = theAttributes.getString("colorMode", "");
	int aMode = -1;
	if (equals(aColorMode, "add"))
		aMode = COLOR_MODE_ADD;
	else if (equals(aColorMode, "multiply"))
		aMode = COLOR_MODE_MULTIPLY;
	else if (equals(aColorMode, "blend"))
		aMode = COLOR_MODE_PIXEL_FX;
	if (aMode != -1)
	{
		anImage->setColorMode(aMode);
		anImage->setColor(aColor);
	}
	mLayout->registerComponent(anImage, aName);
	readCommon(theAttributes, anImage);
	getCurrentContainer()->addChild(anImage);
}

void ScreenDocHandler::readImageButton(Params& theAttributes)
{
	float x = theAttributes.getFloat("x", 0.0f);
	float y = theAttributes.getFloat("y", 0.0f);
	float anOpacity = theAttributes.getFloat("opacity", 1.0f);
	float aScaleX = theAttributes.getFloat("scaleX", 1.0f);
	float aScaleY = theAttributes.getFloat("scaleY", 1.0f);
	std::string anUpImage = theAttributes.getString("upImg", "");
	std::string anOverImage = theAttributes.getString("overImg", "");
	std::string anActiveImage = theAttributes.getString("activeImg", "");
	std::string aName = theAttributes.getString("name", "_ImgButton_");
	std::string anAction = theAttributes.getString("action", "defaultAction");
	std::string anOverSound = theAttributes.getString("overSound", "");
	std::string anActiveSound = theAttributes.getString("activeSound", "");
	Surface* anUpSurface = findImage(anUpImage);
	Vector2f anUpScale = theAttributes.getFloatPair("upScale", Vector2f(1.0f, 1.0f));
	Vector2f anOverScale = theAttributes.getFloatPair("overScale", Vector2f(1.0f, 1.0f));
	float aScaleTime = theAttributes.getFloat("scaleTime", 0.5f);
	std::string aPivot = theAttributes.getString("pivot", "upperLeft");
	Surface* anOverSurface = !equals(anOverImage, "") ? findImage(anOverImage) : nullptr;
	Surface* anActiveSurface = !equals(anActiveImage, "") ? findImage(anActiveImage) : nullptr;
	if (anUpSurface == nullptr)
		return;

	ImageButton* aButton = new ImageButton();
	if (equals(aPivot, "center"))
		aButton->setPivot(PIVOT_CENTER);
	// (A button whose images fail is left made and not added, as in the original)
	if (!aButton->setImages(anUpSurface, anOverSurface, anActiveSurface))
		return;
	aButton->setPosition(x, y);
	aButton->setOpacity(anOpacity);
	aButton->setScale(aScaleX, aScaleY);
	aButton->setHighScale(anOverScale);
	aButton->setNormalScale(anUpScale);
	aButton->setBaseScale(aScaleX, aScaleY);
	aButton->setSounds(anOverSound, anActiveSound);
	aButton->setScaleTime(aScaleTime);
	aButton->setName(aName);
	mLayout->addButton(aButton, aName, anAction);
	mLayout->registerComponent(aButton, aName);
	readCommon(theAttributes, aButton);
	getCurrentContainer()->addChild(aButton);
}

void ScreenDocHandler::readComboButton(Params& theAttributes)
{
	std::string aString = theAttributes.getString("string", "");
	std::string aName = theAttributes.getString("name", "_ComboBtn_");
	std::string anAlignH = theAttributes.getString("xAlign", "");
	std::string anAlignV = theAttributes.getString("yAlign", "");
	std::string aFontName = theAttributes.getString("font", "");
	std::string anOverFontName = theAttributes.getString("overFont", aFontName);
	std::string anActiveFontName = theAttributes.getString("activeFont", anOverFontName);
	Color anUpColor = readColor(theAttributes, "upColor", Color(channelOf(255), channelOf(255), channelOf(255), 1.0f));
	Color anOverColor = readColor(theAttributes, "overColor", anUpColor);
	Color anActiveColor = readColor(theAttributes, "activeColor", anOverColor);

	ComboButton* aButton = new ComboButton();
	aButton->init();
	// (The fonts are found last first)
	Font* anActiveFont = findFont(anActiveFontName);
	Font* anOverFont = findFont(anOverFontName);
	aButton->setFonts(findFont(aFontName), anOverFont, anActiveFont);
	aButton->setText(aString);
	if (equals(anAlignH, "center"))
		aButton->setAlignH(TEXT_ALIGN_CENTER);
	else
		aButton->setAlignH(equals(anAlignH, "right") ? TEXT_ALIGN_RIGHT : TEXT_ALIGN_LEFT);
	if (equals(anAlignV, "middle"))
		aButton->setAlignV(TEXT_ALIGN_MIDDLE);
	else
		aButton->setAlignV(equals(anAlignV, "bottom") ? TEXT_ALIGN_BOTTOM : TEXT_ALIGN_TOP);
	aButton->setColors(anUpColor, anOverColor, anActiveColor);
	aButton->setTextOffset(theAttributes.getFloatPair("textOffset", Vector2f(0.0f, 0.0f)));

	std::string anUpImage = theAttributes.getString("upImg", "");
	std::string anOverImage = theAttributes.getString("overImg", "");
	std::string anActiveImage = theAttributes.getString("activeImg", "");
	std::string aPivot = theAttributes.getString("pivot", "upperLeft");
	Surface* anUpSurface = !equals(anUpImage, "") ? findImage(anUpImage) : nullptr;
	Surface* anOverSurface = !equals(anOverImage, "") ? findImage(anOverImage) : nullptr;
	Surface* anActiveSurface = !equals(anActiveImage, "") ? findImage(anActiveImage) : nullptr;
	if (equals(aPivot, "center"))
		aButton->setPivot(PIVOT_CENTER);
	aButton->setImages(anUpSurface, anOverSurface, anActiveSurface);
	aButton->setImageOffset(theAttributes.getFloatPair("imageOffset", Vector2f(0.0f, 0.0f)));

	float x = theAttributes.getFloat("x", 0.0f);
	float y = theAttributes.getFloat("y", 0.0f);
	float anOpacity = theAttributes.getFloat("opacity", 1.0f);
	float aScale = theAttributes.getFloat("scale", 1.0f);
	std::string anAction = theAttributes.getString("action", "defaultAction");
	std::string anOverSound = theAttributes.getString("overSound", "");
	std::string anActiveSound = theAttributes.getString("activeSound", "");
	aButton->setPosition(x, y);
	aButton->setSounds(anOverSound, anActiveSound);
	aButton->setName(aName);
	aButton->setOpacity(anOpacity);
	aButton->setScale(aScale);
	mLayout->addButton(aButton, aName, anAction);
	mLayout->registerComponent(aButton, aName);
	readCommon(theAttributes, aButton);
	getCurrentContainer()->addChild(aButton);
}

void ScreenDocHandler::readEditBox(Params& theAttributes)
{
	EditBox* aBox = new EditBox();
	readCommon(theAttributes, aBox);
	getCurrentContainer()->addChild(aBox);
	std::string aName = theAttributes.getString("name", "_EditBox_");
	std::string aFontName = theAttributes.getString("font", "");
	std::string aBackground = theAttributes.getString("bgImg", "");
	std::string aCursor = theAttributes.getString("cursorImg", "");
	Surface* aBackgroundSurface = findImage(aBackground);
	Surface* aCursorSurface = findImage(aCursor);
	aBox->setLook(aBackgroundSurface, aCursorSurface, findFont(aFontName));
	Color aColor = readColor(theAttributes, "color", Color(1.0f, 1.0f, 1.0f, 1.0f));
	if (aColor.r != 1.0f || aColor.g != 1.0f || aColor.b != 1.0f)
		aBox->setTextColor(aColor);
	aBox->setAction(theAttributes.getString("action", ""));
	float x = theAttributes.getFloat("x", 0.0f);
	float y = theAttributes.getFloat("y", 0.0f);
	float anOpacity = theAttributes.getFloat("opacity", 1.0f);
	aBox->setPosition(x, y);
	aBox->setOpacity(anOpacity);
	aBox->setCursorOffset(theAttributes.getFloatPair("cursorOffset", Vector2f(0.0f, 0.0f)));
	aBox->setBlinkTime(theAttributes.getFloat("cursorBlinkTime", 1.0f));
	aBox->setTextOffset(theAttributes.getFloatPair("textOffset", Vector2f(0.0f, 0.0f)));
	aBox->setMaxLength(theAttributes.getInt("strLen", 12));
	mLayout->registerComponent(aBox, aName);
}

void ScreenDocHandler::readText(Params& theAttributes)
{
	std::string aString = theAttributes.getString("string", "");
	std::string aFontName = theAttributes.getString("font", "");
	std::string aName = theAttributes.getString("name", "_Text_");
	float x = theAttributes.getFloat("x", 0.0f);
	float y = theAttributes.getFloat("y", 0.0f);
	float anOpacity = theAttributes.getFloat("opacity", 1.0f);
	Color aColor = readColor(theAttributes, "color", Color(1.0f, 1.0f, 1.0f, 1.0f));
	int aWrapWidth = theAttributes.getInt("wordWrap", 0);
	float aScale = theAttributes.getFloat("scale", 1.0f);
	std::string anAlignH = theAttributes.getString("xAlign", "");
	std::string anAlignV = theAttributes.getString("yAlign", "");
	std::string aStyle = theAttributes.getString("style", "");
	if ((equals(aString, "") && equals(aName, "")) || equals(aFontName, ""))
		return;

	TextItem* aText = new TextItem();
	if (aWrapWidth > 0)
		aText->setWrapWidth(aWrapWidth);
	// 0x471b10 (feedingfrenzy2)
	aText->setFont(findFont(aFontName));
	aText->addState(STATE_BOUNDS_DIRTY);
	aText->setText(aString);
	// The colour is read in 0 to 1 and compared with 255, so every text multiplies by its colour (white without one)
	if (aColor.r != 255.0f || aColor.g != 255.0f || aColor.b != 255.0f)
	{
		aText->setColorMode(COLOR_MODE_MULTIPLY);
		aText->setColor(aColor);
	}
	int aHorizontal = TEXT_ALIGN_LEFT;
	if (equals(anAlignH, "center"))
		aHorizontal = TEXT_ALIGN_CENTER;
	else if (equals(anAlignH, "right"))
		aHorizontal = TEXT_ALIGN_RIGHT;
	int aVertical = TEXT_ALIGN_TOP;
	if (equals(anAlignV, "middle"))
		aVertical = TEXT_ALIGN_MIDDLE;
	else if (equals(anAlignV, "bottom"))
		aVertical = TEXT_ALIGN_BOTTOM;
	aText->setAlignment(aHorizontal, aVertical);
	if (aStyle.find("underline") != std::string::npos)
		aText->addFlags(TEXT_ITEM_UNDERLINE);
	if (aStyle.find("strikethrough") != std::string::npos)
		aText->addFlags(TEXT_ITEM_STRIKE);
	if (aStyle.find("shadow") != std::string::npos)
		aText->addFlags(TEXT_ITEM_SHADOW);
	Vector2f aShadowOffset = theAttributes.getFloatPair("shadowOffset", Vector2f(1.0f, 1.0f));
	float aShadowOpacity = theAttributes.getFloat("shadowOpacity", 1.0f);
	aText->setShadow(aShadowOpacity, aShadowOffset);
	aText->setPosition(x, y);
	aText->setFilterMode(FILTER_MODE_SMOOTH);
	aText->setScale(aScale);
	aText->setOpacity(anOpacity);
	aText->setName(aName);
	mLayout->registerComponent(aText, aName);
	if (!equals(aName, "_Text_"))
		mLayout->registerText(aText, aName);
	readCommon(theAttributes, aText);
	getCurrentContainer()->addChild(aText);
}

void ScreenDocHandler::readKeyAction(Params& theAttributes)
{
	std::string anAction = theAttributes.getString("action", "");
	std::string aType = theAttributes.getString("type", "ascii");
	std::string aKey = theAttributes.getString("key", "");
	if (aKey.empty() || anAction.empty())
		return;
	// A letter's key code is its capital's; else the key is the code as a number
	int aKeyCode = equals(aType, "ascii") ? toupper(aKey[0]) : theAttributes.getInt("key", 0);
	if (aKeyCode <= 0)
		return;
	if (!mLayout->addKeyAction(aKeyCode, anAction))
		getEngine()->log("Warning: duplicate action for key code: %d\n", aKeyCode);
}

void ScreenDocHandler::readGroup(Params& theAttributes, bool theComposite, bool theAttachPoint)
{
	Container* aGroup;
	if (theComposite)
	{
		// A CompositeContainer (0xe8 bytes) and its alphaType: not yet translated
		getEngine()->log("ScreenDocHandler: compositeGroup not yet translated\n");
		aGroup = new FadeContainer();
	}
	else
		aGroup = new FadeContainer();
	std::string aName = theAttributes.getString("name", "_Group_");
	if (theAttachPoint && aName == "_Group_")
		throw Exception("attachPoint with no name!");
	float x = theAttributes.getFloat("x", 0.0f);
	float y = theAttributes.getFloat("y", 0.0f);
	float anOpacity = theAttributes.getFloat("opacity", 1.0f);
	float aScaleX = theAttributes.getFloat("scaleX", 1.0f);
	float aScaleY = theAttributes.getFloat("scaleY", 1.0f);
	float aWidth = theAttributes.getFloat("width", 0.0f);
	float aHeight = theAttributes.getFloat("height", 0.0f);
	aGroup->setPosition(x, y);
	aGroup->setScale(aScaleX, aScaleY);
	aGroup->setOpacity(anOpacity);
	aGroup->setName(aName);
	aGroup->setClipRect(0.0f, 0.0f, aWidth, aHeight);
	aGroup->computeBounds();
	readCommon(theAttributes, aGroup);
	getCurrentContainer()->addChild(aGroup);
	mLayout->registerComponent(aGroup, aName);
	if (theAttachPoint)
	{
		// The layout's attach points (0x520fe7 (feedingfrenzy2)): not yet translated
		getEngine()->log("ScreenDocHandler: attachPoint %s not yet translated\n", aName.c_str());
	}
	mContainers.push_back(aGroup);
}

void ScreenDocHandler::readModifier(Params& theAttributes)
{
	std::string aName = theAttributes.getString("name", "WM");
	std::string aType = theAttributes.getString("type", "");
	float anXPeriod = theAttributes.getFloat("xPeriod", 0.0f);
	float anXAmplitude = theAttributes.getFloat("xAmplitude", 0.0f);
	float aYPeriod = theAttributes.getFloat("yPeriod", 0.0f);
	float aYAmplitude = theAttributes.getFloat("yAmplitude", 0.0f);
	theAttributes.getFloat("minX", 0.0f);
	theAttributes.getFloat("maxX", 0.0f);
	theAttributes.getFloat("minY", 0.0f);
	theAttributes.getFloat("maxY", 0.0f);
	theAttributes.getFloat("rate", 0.0f);
	theAttributes.getFloat("angle", 0.0f);
	// The default phase is drawn whether or not the element has one
	float aRandomPhase = (float)randomFloat(0.0f, 100.0f);
	float aPhase = theAttributes.getFloat("phase", aRandomPhase);
	Modifier* aModifier;
	if (aType == "wave")
		aModifier = new WaveModifier(anXPeriod, aYPeriod, anXAmplitude, aYAmplitude, aPhase);
	else
	{
		// "scale" (ScaleModifier, 0x44 bytes, its constructor 0x5146d4) and "rotate" (RotateModifier, 0x30 bytes, its
		// constructor 0x514642) (feedingfrenzy2): not yet translated
		getEngine()->log("ScreenDocHandler: modifier type %s not yet translated\n", aType.c_str());
		return;
	}
	mLayout->registerModifier(aName, aModifier);
}

void ScreenDocHandler::readBltFx(Params& theAttributes)
{
	std::string aName = theAttributes.getString("name", "BFX");
	std::string aType = theAttributes.getString("type", "");
	if (aType != "wave")
		return;
	WaveFx* aWave = new WaveFx();
	float aPeriod = theAttributes.getFloat("period", 1.0f);
	int aHeight = theAttributes.getInt("height", 10);
	float aWidth = theAttributes.getFloat("width", 5.0f);
	aWave->setShape(aPeriod, aWidth, aHeight);
	mLayout->registerBltFx(aName, aWave);
}

void ScreenDocHandler::readParticleSystem(Params& theAttributes)
{
	float x = theAttributes.getFloat("x", 0.0f);
	float y = theAttributes.getFloat("y", 0.0f);
	float anOpacity = theAttributes.getFloat("opacity", 1.0f);
	RectI aBounds = theAttributes.getRect("bounds", RectI(0, 0, 0, 0));
	std::string aFile = theAttributes.getString("file", "");
	std::string aName = theAttributes.getString("name", "");
	float aWarmup = theAttributes.getFloat("warmup", 0.0f);
	if (aFile.empty())
		return;

	// 0x41b1c0 (feedingfrenzy2): the theme's effect of that name, else the file loaded
	Ref<ParticleSystem> aSystem;
	if (mTheme != nullptr)
		aSystem = mTheme->getParticleSystem(aFile);
	if (aSystem == nullptr)
	{
		aSystem = new ParticleSystem();
		aSystem->load(aFile);
	}
	aSystem->setName(aFile);

	aSystem->setOpacity(anOpacity);
	readCommon(theAttributes, aSystem);
	getCurrentContainer()->addChild(aSystem);
	aSystem->setPosition(x, y);
	aSystem->start();
	if (!aName.empty())
		aSystem->setName(aName);
	aSystem->setParticleBounds(aBounds);
	if (aWarmup > 0.0f)
		aSystem->warmup(aWarmup);
	mLayout->registerComponent(aSystem, aName);
}


void ScreenDocHandler::readListBox(Params& theAttributes)
{
	ListBox* aBox = new ListBox();
	std::string aName = theAttributes.getString("name", "_ListBox_");
	float x = theAttributes.getFloat("x", 0.0f);
	float y = theAttributes.getFloat("y", 0.0f);
	float anOpacity = theAttributes.getFloat("opacity", 1.0f);
	float aScaleX = theAttributes.getFloat("scaleX", 1.0f);
	float aScaleY = theAttributes.getFloat("scaleY", 1.0f);
	float aWidth = theAttributes.getFloat("width", 0.0f);
	float aHeight = theAttributes.getFloat("height", 0.0f);
	int anIndent = theAttributes.getInt("indent", 0);
	// The sounds of the mouse coming over a row and of a click (0x496190 (feedingfrenzy2) makes their instances):
	// not yet translated, as the sounds are not
	std::string anOverSound = theAttributes.getString("overSound", "");
	std::string anActiveSound = theAttributes.getString("activeSound", "");
	std::string anAlign = theAttributes.getString("xAlign", "left");
	if (equals(anAlign, "center"))
		aBox->setAlignH(TEXT_ALIGN_CENTER);
	else
		aBox->setAlignH(equals(anAlign, "right") ? TEXT_ALIGN_RIGHT : TEXT_ALIGN_LEFT);
	Color aBackground = readColor(theAttributes, "backgroundColor", aBox->getBackgroundColor());
	Color aText = readColor(theAttributes, "textColor", aBox->getTextColor());
	Color aHighlight = readColor(theAttributes, "highlightColor", aBox->getHighlightColor());
	Color aSelection = readColor(theAttributes, "selectionColor", aBox->getSelectionColor());
	// The selected row's line: selectionBackgroundColor where the file has it
	aSelection = readColor(theAttributes, "selectionBackgroundColor", aSelection);
	Color aSelectionText = readColor(theAttributes, "selectionTextColor", aText);
	Color aSelectionHighlight = readColor(theAttributes, "selectionHighlightColor", aHighlight);
	Font* aFont = findFont(theAttributes.getString("font", ""));
	aBox->setPosition(x, y);
	aBox->setScale(aScaleX, aScaleY);
	aBox->setOpacity(anOpacity);
	aBox->setName(aName);
	aBox->setSize(aWidth, aHeight);
	aBox->setFont(aFont);
	aBox->setColors(aBackground, aText, aHighlight, aSelection, aSelectionText, aSelectionHighlight);
	aBox->setIndent(anIndent);
	aBox->computeBounds();
	readCommon(theAttributes, aBox);
	getCurrentContainer()->addChild(aBox);
	mLayout->registerComponent(aBox, aName);
}

} // namespace engine
