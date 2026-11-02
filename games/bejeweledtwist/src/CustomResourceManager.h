// Sexy::CustomResourceManager (0xdc bytes, vtable 0x7e1d44; 0x6f3b75-0x6f4105): the game's resource manager. Its art
// is drawn for a 1600x1200 screen in images\1200\ and scaled copies of it are in images\768\ and images\600\: an image
// whose path starts images\1200\ loads from the folder of the app's art size (WinTwistApp::mArtRes), and a font has
// path768 and path600 for the smaller sizes.
#pragma once

#include "ResourceManager.h"
#include "Point.h"

namespace Sexy {

class CustomResourceManager : public ResourceManager {
public:
	explicit CustomResourceManager(SexyAppBase* theApp) : ResourceManager(theApp) {}  // 0x6f4095

	// 0x46a220 (the base's in the exe; the framework has none): the image's x= and y= in resources.xml, 0 if none
	Point GetImageOffset(const std::string& theId);

protected:
	bool DoLoadImage(ImageRes* theRes) override;  // 0x6f3b75
	bool DoLoadFont(FontRes* theRes) override;    // 0x6f3d65
};

}  // namespace Sexy
