// ImageMgr.h: a cache of pictures by name (the original's Sexy::ImageMgr). The game keeps one for the level
// backgrounds and each CharacterMgr keeps one for its pictures. It owns the pictures it holds.

#ifndef __IMAGEMGR_H__
#define __IMAGEMGR_H__

#include "Common.h"

#include <map>
#include <string>

namespace Sexy
{

class Image;

class ImageMgr
{
public:
	ImageMgr() {}
	~ImageMgr();

	void Clear(); // 0x457610: deletes every picture
	// 0x457760: the picture of that name; when it is not in the cache and theLoad is set it is loaded from the file of
	// that name and kept. theWasCached tells whether it was there already.
	Image* GetImage(const std::string& theName, bool* theWasCached, bool theLoad);
	// 0x45c560: puts a picture in the cache (deleting the one that was there when theDeleteOld is set)
	void PutImage(const std::string& theName, Image* theImage, bool theDeleteOld);

private:
	std::map<std::string, Image*, StringLessNoCase> mImages;
};

} // namespace Sexy

#endif
