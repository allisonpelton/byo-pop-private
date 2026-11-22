// ImageMgr.cpp: see ImageMgr.h.

#include "ImageMgr.h"

#include "DDImage.h"
#include "Image.h"
#include "SexyAppBase.h"

using namespace Sexy;

ImageMgr::~ImageMgr()
{
	Clear();
}

// 0x457610
void ImageMgr::Clear()
{
	for (std::map<std::string, Image*, StringLessNoCase>::iterator anItr = mImages.begin(); anItr != mImages.end(); ++anItr)
		delete anItr->second;
	mImages.clear();
}

// 0x457760
Image* ImageMgr::GetImage(const std::string& theName, bool* theWasCached, bool theLoad)
{
	Image*& anImage = mImages[theName];
	if (anImage == nullptr)
	{
		if (theLoad)
			anImage = gSexyAppBase->GetImage(theName, true);
		if (theWasCached != nullptr)
			*theWasCached = false;
	}
	else if (theWasCached != nullptr)
		*theWasCached = true;
	return anImage;
}

// 0x45c560
void ImageMgr::PutImage(const std::string& theName, Image* theImage, bool theDeleteOld)
{
	Image*& anImage = mImages[theName];
	if (anImage != theImage)
	{
		if (theDeleteOld)
			delete anImage;
		anImage = theImage;
	}
}
