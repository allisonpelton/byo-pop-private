#ifndef __PLATFORM_IMAGELOADER_H__
#define __PLATFORM_IMAGELOADER_H__

// Image loading for the port (replaces ImageLib::GetImage). Implements the SexyAppFramework rules plus
// the mod overlay:
//
//  * Extensionless names probe .tga, .jpg, .png, .gif in that order (SAF 1.3 order).
//  * The layer (mod or base game) that wins is the highest-priority one containing a colour image or
//    an alpha mask for the stem. Everything else is looked up in that same layer, so a mod's RGBA PNG
//    never picks up the base game's mask.
//  * Alpha masks: "_name" or "name_"; the mask's low (blue) byte becomes the alpha channel. A mask
//    without a colour image yields a white image with that alpha.
//  * If the winning colour image has no alpha of its own and its layer has no mask, a mask from a
//    lower layer is used when its size matches (so a same-size JPG replacement keeps the original mask).
//  * HD variants: "name@2x.png" (or @3x/@4x), or a mod.json "images" entry with "scale". The pixels are
//    kept at full resolution for the GPU texture while the image reports its logical (1x) size; a 1x
//    copy is produced for CPU-side code (hit tests, software blits, fonts).

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Platform
{

struct LoadedImage
{
	int mWidth = 0;   // logical size
	int mHeight = 0;
	std::vector<uint32_t> mBits;  // ARGB (0xAARRGGBB), logical size

	int mHDScale = 1;  // > 1 when an HD variant was loaded
	int mHDWidth = 0;
	int mHDHeight = 0;
	std::vector<uint32_t> mHDBits;  // ARGB at mHDWidth x mHDHeight

	bool mHasAlpha = false;
	std::string mSourcePath;  // real path of the colour (or mask) file
	std::string mModId;       // empty for the base game
};

// theGamePath: "images/gemsheet6", "images\\titlescreen6.jpg", "../images/afont1" relative paths etc.
bool LoadGameImage(const std::string& theGamePath, LoadedImage& theOut, bool theLookForAlpha = true);

// Generated images (a game's code-drawn art): one function that may draw any game image at load time. LoadGameImage asks
// it after the mod layers and before the game's files, so a mod's image still wins and image patches apply on top of
// the drawn one. theGamePath comes normalized and lower case, without its extension ("images/gemsheet6"). The function
// fills theOut as a decoded file would (mBits at the logical size, with their alpha; mHDBits and mHDScale for sharper
// pixels) and returns false to leave the image to the game's files. An empty function turns it off. Set it before
// loading starts: the loading thread calls it.
using ImageGenerator = std::function<bool(const std::string& theGamePath, LoadedImage& theOut)>;
void SetImageGenerator(ImageGenerator theGenerator);

// Decodes an image file from memory into ARGB. Returns false on failure.
bool DecodeImage(const std::vector<uint8_t>& theData, int& theWidth, int& theHeight, std::vector<uint32_t>& theBits, bool* theHasAlpha);

// Registers an explicit HD scale for a game path ("images/gemsheet6" -> 2), from mod manifests.
void SetImageScaleOverride(const std::string& theModId, const std::string& theGamePath, int theScale);
// Marks every plain-named image of a mod as HD ("imageScale" in mod.json).
void SetModDefaultImageScale(const std::string& theModId, int theScale);
void ClearImageScaleOverrides();

// Image patches ("imagePatches" in mod.json): paste a region over a game image after it is loaded,
// so a mod can replace one sprite in a sheet (e.g. one gem) without shipping the rest of the sheet.
//   theTarget:    game path of the image to patch ("images/gemsheet6"; extension optional)
//   thePatchPath: real host path of the patch image (PNG with alpha recommended)
//   theX, theY:   top-left of the patched region in the target's logical (1x) pixels
//   theScale:     pixel density of the patch (2 for an @2x patch); the result is promoted to the
//                 highest density involved so HD patches stay sharp
//   theReplace:   true replaces the region including alpha; false alpha-blends the patch over it
// Patches apply in registration order (register lower-priority mods first).
void AddImagePatch(const std::string& theTarget, const std::string& thePatchPath, int theX, int theY, int theScale, bool theReplace);
void ClearImagePatches();
void ApplyImagePatches(const std::string& theGamePath, LoadedImage& theImage);

}

#endif
