#include "ScreenDocHandler.h"

#include "BltFx.h"
#include "DrawContext.h"
#include "Engine.h"
#include "Exception.h"
#include "Image.h"
#include "Modifiers.h"
#include "ParticleSystem.h"
#include "Random.h"
#include "ScreenLayout.h"
#include "Theme.h"

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
	else
	{
		// animation 0x53fffd, text 0x4548c0, imageButton 0x432b10, checkBox 0x46e570, textButton 0x421160,
		// comboButton 0x45c5a0, clipGroup 0x535b3e, selector 0x535883, textSub 0x53a144,
		// editBox 0x412680, include 0x53578b, radioGroup 0x4177a0, listBox 0x4804a0, table 0x40ca40, column
		// 0x40e640, keyAction 0x426cb0, detailSelector 0x5328bf (feedingfrenzy2): not yet translated
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

Color ScreenDocHandler::readColor(Params& theAttributes, const std::string& theName, const Color& theDefault)
{
	std::string aValue = theAttributes.getString(theName, "");
	if (aValue.find(',') != std::string::npos)
		return theAttributes.getColor(theName, Color(channelOf(255), channelOf(255), channelOf(255), 1.0f));
	if (mTheme != nullptr && !aValue.empty())
		return mTheme->getColor(aValue);
	return theDefault;
}

void ScreenDocHandler::readCommon(Params& theAttributes, Component* theComponent)
{
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
	theAttributes.getInt("theoraFrame", 0);
	std::string aName = theAttributes.getString("name", "");
	Surface* aSurface = nullptr;
	if (!aFile.empty())
		aSurface = findImage(aFile);
	else if (!aTheoraFile.empty())
	{
		// A frame of a Theora animation (0x415ba0, 0x47b930 and 0x415b40 (feedingfrenzy2)): not yet translated
		getEngine()->log("ScreenDocHandler: theoraFile %s not yet translated\n", aTheoraFile.c_str());
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

	if (fabsf(aScaleX) != 1.0f || fabsf(aScaleY) != 1.0f)
		// A scaled copy of the surface: not yet translated
		getEngine()->log("ScreenDocHandler: %s scaled by %f, %f not yet translated\n", aSurface->getName().c_str(),
			(double)aScaleX, (double)aScaleY);
	aScaleX = aScaleX < 0.0f ? -1.0f : 1.0f;
	aScaleY = aScaleY < 0.0f ? -1.0f : 1.0f;

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

	// 0x41b1c0 (feedingfrenzy2): with a theme, the theme's pool of the effect (0x475950 (feedingfrenzy2)) gives it;
	// that is not yet translated, and the effect is loaded on its own as without one
	if (mTheme != nullptr)
		getEngine()->log("ScreenDocHandler: the theme's particle effect %s not yet translated\n", aFile.c_str());
	Ref<ParticleSystem> aSystem = new ParticleSystem();
	aSystem->load(aFile);
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

} // namespace engine
