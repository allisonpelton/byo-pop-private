#include "AtlantisTheme.h"

#include "DocHandler.h"
#include "Engine.h"
#include "Exception.h"

#include <strings.h>

namespace
{

// The pivots by name (0x64fe90), the default for a name not among them
int pivotOf(const engine::Params& theParams, const char* theName, int theDefault)
{
	std::string aValue = theParams.getString(theName, "");
	static const struct
	{
		const char* mName;
		int mPivot;
	} PIVOTS[] = {{"upperleft", 0}, {"upperright", 4}, {"lowerleft", 5}, {"lowerright", 6},
				  {"topcenter", 3}, {"bottomcenter", 2}, {"center", 1}};
	for (size_t i = 0; i < sizeof(PIVOTS) / sizeof(PIVOTS[0]); i++)
		if (strcasecmp(aValue.c_str(), PIVOTS[i].mName) == 0)
			return PIVOTS[i].mPivot;
	return theDefault;
}

// 0x607750: the alpha type of an image entry, 2 (alpha) when none is given or not recognised, 4 for "mask", 3 for "add"
// and 0 for "none"
int alphaTypeOf(const std::string& theName)
{
	if (theName.empty())
		return 2;
	if (strncasecmp(theName.c_str(), "mask", 4) == 0)
		return 4;
	if (strncasecmp(theName.c_str(), "add", 3) == 0)
		return 3;
	if (strncasecmp(theName.c_str(), "none", 4) == 0)
		return 0;
	return 2;
}

// Reads one <theme> file's elements into the theme (0x604010 and its callees)
class ThemeHandler : public engine::AbstractDocHandler
{
public:
	explicit ThemeHandler(AtlantisTheme& theTheme) : mTheme(theTheme) {}

	void startElement(const std::string& theName, engine::Params& theAttributes) override
	{
		const engine::Params& p = theAttributes;
		if (theName == "theme")
			mTheme.setBasePath(p.getString("basePath", ""));
		else if (theName == "image")
			readImage(p);
		else if (theName == "font")
			mTheme.addFont(p.getString("name", ""), entryPath(p));
		else if (theName == "color")
		{
			std::string aName = p.getString("name", "");
			engine::Color aColor = p.getColor("value", engine::Color(1.0f, 1.0f, 1.0f, 1.0f));
			if (!aName.empty())
				mTheme.addColor(aName, aColor);
		}
		else if (theName == "vector")
		{
			std::string aName = p.getString("name", "");
			engine::Vector2f aVector = p.getFloatPair("vector", engine::Vector2f());
			if (!aName.empty())
				mTheme.addVector(aName, aVector);
		}
		else if (theName == "sound" || theName == "music" || theName == "randmusic" || theName == "composite" ||
				 theName == "animdata" || theName == "animation" || theName == "particleEffect" ||
				 theName == "track" || theName == "surface")
		{
			// A track belongs to the random music before it, a surface to the composite
			std::string aKey = theName;
			if (theName == "track")
				aKey = "randmusic:" + mLastGroup;
			else if (theName == "surface")
				aKey = "composite:" + mLastGroup;
			else if (theName == "randmusic" || theName == "composite")
				mLastGroup = p.getString("name", "");
			std::string aName = p.getString("name", "");
			if (theName == "surface")
				aName = p.getString("image", "") + "#" + std::to_string(mSurfaceCount++);
			else if (theName == "track")
				aName = aName + "#" + std::to_string(mSurfaceCount++);
			if (theName != "track" && theName != "surface")
				mSurfaceCount = 0;
			mTheme.mRawEntries[aKey][aName] = p.getValues();
		}
		else
			engine::getEngine()->log("Theme: unknown element \"%s\"\n", theName.c_str());
	}

private:
	// 0x607460: the file under the theme's base path, or the "fullPath" attribute as it is
	std::string entryPath(const engine::Params& p)
	{
		std::string aFile = p.getString("file", "");
		if (aFile.empty())
			return p.getString("fullPath", "");
		return mTheme.getBasePath() + "/" + aFile;
	}

	// 0x604a50
	void readImage(const engine::Params& p)
	{
		std::string aName = p.getString("name", "");
		if (aName.empty())
			throw engine::Exception("No friendly name for theme image entry");
		int aPivot = pivotOf(p, "pivot", 1);
		int anAlphaType = alphaTypeOf(p.getString("alphatype", ""));
		float aScale = p.getFloat("scale", 1.0f);
		float aScaleX = p.getFloat("scaleX", 1.0f);
		float aScaleY = p.getFloat("scaleY", 1.0f);
		std::string aPath = entryPath(p);
		bool aDemandLoad = p.getBool("demandload", true);
		bool aTrim = p.getBool("trim", false);
		mTheme.addImageEntry(aName, aPath, aPivot, aScaleX * aScale, aScale * aScaleY, anAlphaType, aTrim, aDemandLoad);
	}

	AtlantisTheme& mTheme;
	std::string mLastGroup;
	int mSurfaceCount = 0;
};

} // namespace

void AtlantisTheme::addFile(const std::string& thePath)
{
	ThemeHandler aRootHandler(*this);
	if (!engine::getEngine()->parseXml(thePath, &aRootHandler))
		engine::getEngine()->log("COULD NOT PARSE THEME FILE \"%s\"\n", thePath.c_str());
}

void AtlantisTheme::addImageEntry(const std::string& theName, const std::string& thePath, int thePivot, float theScaleX,
								  float theScaleY, int theAlphaType, bool theTrim, bool theDemandLoad)
{
	ImageEntry anEntry;
	anEntry.mPath = thePath;
	anEntry.mPivot = thePivot;
	anEntry.mScaleX = theScaleX;
	anEntry.mScaleY = theScaleY;
	anEntry.mAlphaType = theAlphaType;
	anEntry.mTrim = theTrim;
	ImageEntry& aStored = mImageEntries[theName] = anEntry;
	if (!theDemandLoad)
		loadImage(aStored);
}

// 0x601c40: the engine's image at the path, with the entry's pivot, scale and alpha type
void AtlantisTheme::loadImage(ImageEntry& theEntry)
{
	engine::Surface* anImage = engine::getEngine()->getImage(theEntry.mPath);
	if (anImage == nullptr)
		return;
	engine::Surface* aCopy = anImage->clone();
	aCopy->setPivot(theEntry.mPivot);
	if (theEntry.mScaleX != 1.0f || theEntry.mScaleY != 1.0f)
		engine::getEngine()->log("Theme: image %s scaled by %f, %f not yet translated\n", theEntry.mPath.c_str(),
								 (double)theEntry.mScaleX, (double)theEntry.mScaleY);
	if (theEntry.mAlphaType == 4)
		engine::getEngine()->log("Theme: image %s with the alpha type mask not yet translated\n",
								 theEntry.mPath.c_str());
	else
		aCopy->setBlendMode(theEntry.mAlphaType, 0);
	theEntry.mSurface = aCopy;
}

engine::Surface* AtlantisTheme::getImage(const std::string& theName)
{
	std::map<std::string, ImageEntry>::iterator anEntry = mImageEntries.find(theName);
	if (anEntry == mImageEntries.end())
		return engine::Theme::getImage(theName);
	if (anEntry->second.mSurface == nullptr)
		loadImage(anEntry->second);
	return anEntry->second.mSurface.get();
}
