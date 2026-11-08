// CustomResourceManager.cpp: CustomResourceManager (CustomResourceManager.h).
#include "CustomResourceManager.h"

#include "DDImage.h"
#include "SexyAppBase.h"

using namespace Sexy;

static bool HasAttribute(const XMLParamMap& theMap, const char* theName)
{
	return theMap.find(theName) != theMap.end();
}

// 0x5b4852: an image of the game's data (a backdrop). The 640x480 mode would take the scaled copy it keeps in the user
// data folder.
Image* CustomResourceManager::GetCachedImage(const std::string& thePath)
{
	return mApp->GetImage(thePath, true);
}

// 0x5b4a69
bool CustomResourceManager::DoLoadImage(ImageRes* theRes)
{
	if (HasAttribute(theRes->mXMLAttributes, "sm_path"))
	{
		// the small art is for the 640x480 mode; the others take the path
		const char* aKey = (mApp->mWidth == 640) ? "sm_path" : "path";
		theRes->mPath = "images\\" + SexyStringToStringFast(theRes->mXMLAttributes[aKey]);
	}

	if (theRes->mPath == "images\!")
	{
		// a blank image for the program to draw into
		theRes->mImage = new DDImage(mApp->mDDInterface);
		theRes->mImage.mOwnsUnshared = true;
		return true;
	}
	return ResourceManager::DoLoadImage(theRes);
}

// 0x5b4f8c
bool CustomResourceManager::DoLoadFont(FontRes* theRes)
{
	if (HasAttribute(theRes->mXMLAttributes, "sm_path"))
	{
		const char* aKey = (mApp->mWidth == 640) ? "sm_path" : "path";
		theRes->mPath = "data\\" + SexyStringToStringFast(theRes->mXMLAttributes[aKey]);
	}
	return ResourceManager::DoLoadFont(theRes);
}
