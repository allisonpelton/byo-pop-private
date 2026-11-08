#ifndef __CUSTOMRESOURCEMANAGER_H__
#define __CUSTOMRESOURCEMANAGER_H__

// CustomResourceManager.h: Bejeweled 2's resource manager (Sexy::CustomResourceManager, 0x5afd7a-0x5b4f8c; the object is 0xd0 bytes).
// The 640x480 mode of the game loads the "sm_path" art of a resource (the small version) where there is one; the
// original makes the others by scaling the 1024x768 art to 0.625 and keeping the result in the user data folder. The
// port loads the 1024x768 art only (docs/GAME_NOTES.md, deviations).

#include "ResourceManager.h"

namespace Sexy
{

class Image;

class CustomResourceManager : public ResourceManager
{
public:
	explicit CustomResourceManager(SexyAppBase* theApp) : ResourceManager(theApp) {}   // 0x5afd7a

	Image*			GetCachedImage(const std::string& thePath);     // 0x5b4852

protected:
	virtual bool	DoLoadImage(ImageRes* theRes) override;         // 0x5b4a69
	virtual bool	DoLoadFont(FontRes* theRes) override;           // 0x5b4f8c
};

}

#endif
