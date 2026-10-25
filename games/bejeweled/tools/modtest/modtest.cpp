// Loads a game image through the real mod pipeline (VFS overlay + ModManager + ImageLoader + patches)
// and writes the result to PNG. Used to check mods without running the game.
//   modtest <gamedir> <modsdir> <userdir> <image path> <out-prefix>
// Writes <out-prefix>-1x.png (CPU copy) and, for HD results, <out-prefix>-hd.png (texture pixels).
#include "platform/ImageLoader.h"
#include "platform/ModManager.h"
#include "platform/VFS.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stb_image_write.h>
#include <cstdio>
#include <vector>

static bool WritePng(const std::string& thePath, const std::vector<uint32_t>& theBits, int theW, int theH)
{
	std::vector<unsigned char> rgba((size_t)theW * theH * 4);
	for (size_t i = 0; i < theBits.size(); i++)
	{
		uint32_t p = theBits[i];
		rgba[i * 4 + 0] = (p >> 16) & 0xFF;
		rgba[i * 4 + 1] = (p >> 8) & 0xFF;
		rgba[i * 4 + 2] = p & 0xFF;
		rgba[i * 4 + 3] = p >> 24;
	}
	return stbi_write_png(thePath.c_str(), theW, theH, 4, rgba.data(), theW * 4) != 0;
}

int main(int argc, char** argv)
{
	if (argc < 6)
	{
		printf("usage: modtest <gamedir> <modsdir> <userdir> <image> <out-prefix>\n");
		return 2;
	}
	Platform::VFS::Get().SetBaseDir(argv[1]);
	Platform::VFS::Get().SetUserDir(argv[3]);
	Platform::ModManager::Get().SetModsDir(argv[2]);
	Platform::ModManager::Get().Refresh();
	for (const Platform::ModInfo& m : Platform::ModManager::Get().GetMods())
		printf("mod %-24s enabled=%d priority=%d %s\n", m.mId.c_str(), m.mEnabled, m.mPriority, m.mError.c_str());

	Platform::LoadedImage img;
	if (!Platform::LoadGameImage(argv[4], img))
	{
		printf("failed to load %s\n", argv[4]);
		return 1;
	}
	printf("loaded %s: %dx%d from %s (mod '%s'), HD scale %d (%dx%d), alpha=%d\n", argv[4], img.mWidth, img.mHeight,
		img.mSourcePath.c_str(), img.mModId.c_str(), img.mHDScale, img.mHDWidth, img.mHDHeight, img.mHasAlpha);
	std::string prefix = argv[5];
	WritePng(prefix + "-1x.png", img.mBits, img.mWidth, img.mHeight);
	if (!img.mHDBits.empty())
		WritePng(prefix + "-hd.png", img.mHDBits, img.mHDWidth, img.mHDHeight);
	return 0;
}
