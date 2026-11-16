#include "Theme.h"

#include "Engine.h"
#include "Exception.h"
#include "Font.h"
#include "Surface.h"

#include <cstdio>
#include <cstdlib>

namespace engine
{

Theme::Theme() {}

Theme::~Theme() {}

void Theme::load(const std::string& thePath)
{
	clear();
	Engine* anEngine = getEngine();
	anEngine->releaseUnusedImages();
	anEngine->releaseUnusedFonts();
	MasterDocHandler aMaster;
	ThemeDocHandler* aHandler = new ThemeDocHandler(this);
	aHandler->attach(&aMaster, "theme");
	anEngine->parseXml(thePath, &aMaster);
}

Color Theme::getColor(const std::string& theName)
{
	std::map<std::string, Color>::const_iterator anIt = mColors.find(theName);
	if (anIt == mColors.end())
		return Color(1.0f, 1.0f, 1.0f, 1.0f);
	return anIt->second;
}

Vector2f Theme::getVector(const std::string& theName)
{
	std::map<std::string, Vector2f>::const_iterator anIt = mVectors.find(theName);
	if (anIt == mVectors.end())
		return Vector2f(0.0f, 0.0f);
	return anIt->second;
}

void Theme::clear()
{
	mImageKeys.clear();
	mImagesByKey.clear();
}

std::string Theme::imageKey(const std::string& thePath, int thePivot, float theScale, int theBlendMode)
{
	char aBuffer[64];
	std::string aKey = thePath;
	snprintf(aBuffer, sizeof(aBuffer), "%d", thePivot);
	aKey += "_";
	aKey += aBuffer;
	snprintf(aBuffer, sizeof(aBuffer), "%d", theBlendMode);
	aKey += "_";
	aKey += aBuffer;
	snprintf(aBuffer, sizeof(aBuffer), "_%f", (double)theScale);
	aKey += aBuffer;
	return aKey;
}

void Theme::addImage(const std::string& theName, const std::string& thePath, int thePivot, float theScale,
	int theBlendMode)
{
	// The original looks the key up with the map's operator[] (0x428350 (feedingfrenzy2)), leaving a null entry that
	// the store (0x432170 (feedingfrenzy2)) fills; the port looks it up and stores once
	std::string aKey = imageKey(thePath, thePivot, theScale, theBlendMode);
	Ref<Surface>& aCopy = mImagesByKey[aKey];
	if (aCopy == nullptr)
	{
		// 0x421bd0 (feedingfrenzy2): the copy's pivot, scale and mode
		Surface* aSurface = getEngine()->getImage(thePath)->clone();
		aSurface->setPivot(thePivot);
		if (theScale != 1.0f)
			// Slot 0x4c of the surface: no theme of the game gives a scale, so it is not yet translated
			getEngine()->log("Theme: image %s scaled by %f not yet translated\n", thePath.c_str(), (double)theScale);
		if (theBlendMode != BLEND_OPAQUE)
			aSurface->setBlendMode(theBlendMode, 0);
		aCopy = aSurface;
	}
	mImageKeys[theName] = aKey;
}

bool Theme::addFont(const std::string& theName, const std::string& thePath)
{
	Font* aFont = getEngine()->getFont(thePath);
	if (aFont == nullptr)
		return false;
	mFonts.insert(std::make_pair(theName, Ref<Font>(aFont)));
	return true;
}

Surface* Theme::getImage(const std::string& theName)
{
	// The original reads both maps with operator[], adding empty entries for a name it does not have; nothing reads
	// them, so the port only looks
	std::map<std::string, std::string>::const_iterator aKey = mImageKeys.find(theName);
	if (aKey == mImageKeys.end())
		return nullptr;
	std::map<std::string, Ref<Surface>>::const_iterator anImage = mImagesByKey.find(aKey->second);
	if (anImage == mImagesByKey.end())
		return nullptr;
	return anImage->second.get();
}

void ThemeDocHandler::begin(Params& theAttributes)
{
	mBasePath = theAttributes.getString("basePath", "");
	mTheme->setBasePath(mBasePath);
}

std::string ThemeDocHandler::entryPath(Params& theAttributes)
{
	std::string aFile = theAttributes.getString("file", "");
	if (aFile.empty())
		return theAttributes.getString("fullPath", "");
	char aBuffer[512];
	snprintf(aBuffer, sizeof(aBuffer), "%s/%s", mBasePath.c_str(), aFile.c_str());
	return aBuffer;
}

void ThemeDocHandler::readImage(Params& theAttributes)
{
	std::string aName = theAttributes.getString("name", "");
	if (aName.empty())
		throw Exception("No friendly name for theme image entry");
	bool aCenter = theAttributes.getString("pivot", "center") == "center";
	bool anAdd = theAttributes.getString("alphamode", "") == "add";
	float aScale = theAttributes.getFloat("scale", 1.0f);
	std::string aPath = entryPath(theAttributes);
	mTheme->addImage(aName, aPath, aCenter ? PIVOT_CENTER : PIVOT_UPPER_LEFT, aScale, anAdd ? BLEND_ADD : BLEND_OPAQUE);
}

void ThemeDocHandler::readFont(Params& theAttributes)
{
	std::string aName = theAttributes.getString("name", "");
	std::string aPath = entryPath(theAttributes);
	mTheme->addFont(aName, aPath);
}

void ThemeDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName == "image")
		readImage(theAttributes);
	else if (theName == "font")
		readFont(theAttributes);
	else if (theName == "color")
	{
		std::string aName = theAttributes.getString("name", "");
		Color aColor = theAttributes.getColor("value", Color(1.0f, 1.0f, 1.0f, 1.0f));
		if (!aName.empty())
			mTheme->addColor(aName, aColor);
	}
	else if (theName == "vector")
	{
		std::string aName = theAttributes.getString("name", "");
		Vector2f aVector = theAttributes.getFloatPair("vector", Vector2f(0.0f, 0.0f));
		if (!aName.empty())
			mTheme->addVector(aName, aVector);
	}
	else if (theName == "particleEffect" || theName == "sound" || theName == "mod" || theName == "music" ||
		theName == "composite")
		// 0x426070, 0x463210, 0x49d0a0, 0x420850 and the composite's own (feedingfrenzy2)
		getEngine()->log("Theme: %s %s not yet translated\n", theName.c_str(),
			theAttributes.getString("name", "").c_str());
}

} // namespace engine
