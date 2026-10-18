// Renders a properties string with the banner font the way Board::ShowText does (white text into a
// MemoryImage sized StringWidth x GetHeight, baseline at the ascent), using the configured mods.
//   bannertest <gamedir> <modsdir> <userdir> <string id> <out.png> [crossbar]
// "crossbar" applies the mods-in-use G crossbar (src/game/ModIndicators.cpp) to the font.
#include "SexyAppBase.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "platform/ImageLoader.h"
#include "platform/ModManager.h"
#include "platform/VFS.h"
#include "game/ModIndicators.h"

#include <stb_image_write.h>
#include <cstdio>
#include <vector>

using namespace Sexy;

int main(int argc, char** argv)
{
	if (argc < 6)
	{
		printf("usage: bannertest <gamedir> <modsdir> <userdir> <id> <out.png>\n");
		return 2;
	}
	Platform::VFS::Get().SetBaseDir(argv[1]);
	Platform::VFS::Get().SetUserDir(argv[3]);
	Platform::ModManager::Get().SetModsDir(argv[2]);
	Platform::ModManager::Get().Refresh();
	SexyAppBase* anApp = new SexyAppBase();
	anApp->LoadProperties();
	SexyString aText = anApp->GetString(argv[4]);

	Platform::LoadedImage aLoaded;
	if (!Platform::LoadGameImage("images\\overfont2", aLoaded))
		return 1;
	MemoryImage* aFontImage = new MemoryImage(anApp);
	aFontImage->SetBits((ulong*)aLoaded.mBits.data(), aLoaded.mWidth, aLoaded.mHeight);
	ImageFont* aFont = new ImageFont(aFontImage, "data\\overfont2.txt");
	if ((argc > 6) && (std::string(argv[6]) == "crossbar"))
		AddModsCrossbarToG(aFont);

	MemoryImage aBanner(anApp);
	aBanner.Create(aFont->StringWidth(aText), aFont->GetHeight());
	aBanner.SetImageMode(true, true);
	{
		Graphics g(&aBanner);
		g.SetFont(aFont);
		g.SetColor(Color(255, 255, 255));
		g.DrawString(aText, 0, aFont->GetAscent());
	}
	int w = aBanner.GetWidth(), h = aBanner.GetHeight();
	ulong* aBits = aBanner.GetBits();
	std::vector<unsigned char> aRGBA((size_t)w * h * 4);
	for (int i = 0; i < w * h; i++)
	{
		aRGBA[i * 4 + 0] = (aBits[i] >> 16) & 0xFF;
		aRGBA[i * 4 + 1] = (aBits[i] >> 8) & 0xFF;
		aRGBA[i * 4 + 2] = aBits[i] & 0xFF;
		aRGBA[i * 4 + 3] = aBits[i] >> 24;
	}
	stbi_write_png(argv[5], w, h, 4, aRGBA.data(), w * 4);
	printf("\"%s\" -> %dx%d\n", aText.c_str(), w, h);
	return 0;
}
