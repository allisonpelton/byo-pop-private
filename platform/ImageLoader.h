#ifndef __PLATFORM_IMAGELOADER_H__
#define __PLATFORM_IMAGELOADER_H__

// Image loading for the port (replaces ImageLib::GetImage). Implements the SexyAppFramework rules plus
// the mod overlay:
//
//  * Extensionless names probe .tga, .jpg, .png, .gif in that order (SAF 1.3 order); the extensions of
//    formats a game registered (RegisterImageFormat, e.g. .jp2) follow, so a .png of the same name wins.
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

// Decodes an image file from memory into ARGB. Returns false on failure. Registered formats
// (RegisterImageFormat) whose detector accepts the data are tried first, then stb_image (PNG, JPEG, GIF,
// BMP, TGA, ...). theHasAlpha is set when any pixel's alpha is not 255.
bool DecodeImage(const std::vector<uint8_t>& theData, int& theWidth, int& theHeight, std::vector<uint32_t>& theBits, bool* theHasAlpha);

// An image format besides the ones stb_image reads. A game that needs one (JPEG 2000: Platform::
// RegisterOpenJPEGImages in platform/openjpeg/JP2Decoder.h) registers it once before it loads images; games
// that register none never link that library, as with Mixer::RegisterModuleFormat.
//   theDetector: true when the file's bytes are this format (a signature check, not a full parse)
//   theDecoder:  decodes the file into theArgb (0xAARRGGBB, theWidth x theHeight pixels, row by row from the
//                top) and sets *theHasAlpha (when not null) to whether any pixel's alpha is not 255, exactly
//                as DecodeImage does for the built-in formats; false on failure
//   theExtension: ".jp2" (the dot is optional, case is ignored). It joins the extension search after the
//                built-in extensions, so an extensionless name finds a staged .png before a .jp2, and a name
//                that ends in it is taken to give its extension. Alpha masks ("name_.jp2" beside "name.jp2")
//                go through the same path as any other mask file. One detector and decoder pair registered
//                under several extensions is tried once per file.
typedef bool (*ImageDetector)(const std::vector<uint8_t>& theData);
typedef bool (*ImageDecoder)(const std::vector<uint8_t>& theData, int& theWidth, int& theHeight, std::vector<uint32_t>& theArgb,
	bool* theHasAlpha);
void RegisterImageFormat(const char* theExtension, ImageDetector theDetector, ImageDecoder theDecoder);

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
