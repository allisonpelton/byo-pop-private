// Sexy::CustomResourceManager (CustomResourceManager.h).
#include "CustomResourceManager.h"

#include "DDImage.h"
#include "WinTwistApp.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

namespace {

int ArtRes(SexyAppBase* theApp)
{
	return static_cast<WinTwistApp*>(theApp)->mArtRes;
}

bool StartsWithNoCase(const std::string& theString, const char* thePrefix)
{
	size_t n = strlen(thePrefix);
	if (theString.size() < n)
		return false;
	for (size_t i = 0; i < n; i++)
		if (tolower((unsigned char)theString[i]) != tolower((unsigned char)thePrefix[i]))
			return false;
	return true;
}

bool HasAttribute(const XMLParamMap& theMap, const char* theName)
{
	return theMap.find(theName) != theMap.end();
}

}  // namespace

// The original tests gD3DInterface: in 2D only the images marked 2dbig keep the 1200 art (the port is 2D only for now).
bool CustomResourceManager::DoLoadImage(ImageRes* theRes)
{
	std::string aPath = SexyStringToStringFast(theRes->mXMLAttributes["path"]);
	bool aRescale = mApp->Is3DAccelerated() || !HasAttribute(theRes->mXMLAttributes, "2dbig");
	if (aRescale && StartsWithNoCase(aPath, "images\\1200\\")) {
		char aBuf[32];
		snprintf(aBuf, sizeof(aBuf), "images\\%d\\", ArtRes(mApp));
		aPath = aBuf + aPath.substr(12);
	}
	theRes->mPath = aPath;
	if (theRes->mPath == "images\\!") {
		// a blank image for the program to draw into
		theRes->mImage = new DDImage(mApp->mDDInterface);
		theRes->mImage.mOwnsUnshared = true;
		return true;
	}
	return ResourceManager::DoLoadImage(theRes);
}

bool CustomResourceManager::DoLoadFont(FontRes* theRes)
{
	const char* aKey = "path";
	if (ArtRes(mApp) == 768 && HasAttribute(theRes->mXMLAttributes, "path768"))
		aKey = "path768";
	else if (ArtRes(mApp) == 600 && HasAttribute(theRes->mXMLAttributes, "path600"))
		aKey = "path600";
	theRes->mPath = SexyStringToStringFast(theRes->mXMLAttributes[aKey]);

	// A font made from another one: that one, a font of the Dynamic group, is loaded first if it is not yet.
	if (strncmp(theRes->mPath.c_str(), "!ref:", 5) == 0) {
		std::string aRefId = theRes->mPath.substr(5);
		if (GetFont(aRefId) == nullptr) {
			ResGroupMap::iterator aGroup = mResGroupMap.find("Dynamic");
			if (aGroup != mResGroupMap.end()) {
				for (BaseRes* aRes : aGroup->second) {
					if (aRes->mFromProgram || aRes->mType != ResType_Font)
						continue;
					FontRes* aFontRes = static_cast<FontRes*>(aRes);
					if (aFontRes->mId != aRefId || aFontRes->mFont != nullptr)
						continue;
					if (!DoLoadFont(aFontRes))
						return false;
					break;
				}
			}
		}
	}
	return ResourceManager::DoLoadFont(theRes);
}

Point CustomResourceManager::GetImageOffset(const std::string& theId)
{
	Point anOffset(0, 0);
	ResMap::iterator anItr = mImageMap.find(theId);
	if (anItr == mImageMap.end())
		return anOffset;
	XMLParamMap& anAttribs = anItr->second->mXMLAttributes;
	XMLParamMap::iterator anAttr = anAttribs.find("x");
	if (anAttr != anAttribs.end())
		anOffset.mX = (int)strtol(SexyStringToStringFast(anAttr->second).c_str(), nullptr, 10);
	anAttr = anAttribs.find("y");
	if (anAttr != anAttribs.end())
		anOffset.mY = (int)strtol(SexyStringToStringFast(anAttr->second).c_str(), nullptr, 10);
	return anOffset;
}
