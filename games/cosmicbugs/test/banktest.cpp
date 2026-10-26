// Loads Cosmic Bugs' art banks the way GameMain does (art8.bbk raw, art.bbk converted to the 16-bit screen) and
// draws every image of one onto 800x600 sheets through the engine's blitters, written as PNGs.
//   cosmicbugs_banktest <gamedir> <outdir> [art|art8] [first image]
// Opens no window and plays no sound.

#include "R64.h"
#include "R64Bank.h"
#include "R64Sys.h"

#include "stb_image_write.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace r64;

namespace
{

class DirHost : public SysHost
{
public:
	std::string mDir;

	bool ReadFile(const char* thePath, std::vector<uint8_t>& theData) override
	{
		std::string aPath = mDir + "/" + thePath;
		for (char& c : aPath)
			if (c == '\\')
				c = '/';
		FILE* f = std::fopen(aPath.c_str(), "rb");
		if (f == nullptr)
			return false;
		std::fseek(f, 0, SEEK_END);
		long n = std::ftell(f);
		std::fseek(f, 0, SEEK_SET);
		theData.resize((size_t)n);
		bool ok = n == 0 || std::fread(theData.data(), (size_t)n, 1, f) == 1;
		std::fclose(f);
		return ok;
	}
};

void WritePng(const char* thePath, const uint16_t* theScreen, int w, int h)
{
	std::vector<uint8_t> aRgb((size_t)w * h * 3);
	for (int i = 0; i < w * h; i++)
	{
		uint16_t p = theScreen[i];
		aRgb[i * 3] = (uint8_t)(((p >> 11) & 0x1f) << 3);
		aRgb[i * 3 + 1] = (uint8_t)(((p >> 5) & 0x3f) << 2);
		aRgb[i * 3 + 2] = (uint8_t)((p & 0x1f) << 3);
	}
	stbi_write_png(thePath, w, h, 3, aRgb.data(), w * 3);
}

} // namespace

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		std::fprintf(stderr, "usage: cosmicbugs_banktest <gamedir> <outdir> [art|art8] [first image]\n");
		return 2;
	}
	DirHost aHost;
	aHost.mDir = argv[1];
	Sys_SetHost(&aHost);
	InitSinCos();
	const char* aWhich = argc > 3 ? argv[3] : "art";
	int aFirst = argc > 4 ? std::atoi(argv[4]) : 0;

	PixelFormat aScreenFmt;
	PixelFormat_Init(&aScreenFmt, 16, 0xf800, 0x7e0, 0x1f);
	Bank* aBank8 = new Bank();
	Bank* aBank = new Bank();
	if (Bank_Load(aBank8, "art8.bbk", 0, &aScreenFmt) == 0)
	{
		std::fprintf(stderr, "art8.bbk failed\n");
		return 1;
	}
	if (Bank_Load(aBank, "art.bbk", 1, &aScreenFmt) == 0)
	{
		std::fprintf(stderr, "art.bbk failed\n");
		return 1;
	}
	std::printf("art8: %d images, %d palettes; art: %d images, %u bytes\n", Bank_Count(aBank8),
		(int)aBank8->palettes.size(), Bank_Count(aBank), aBank->dataSize);

	Bank* aShow = std::strcmp(aWhich, "art8") == 0 ? aBank8 : aBank;
	const int W = 800, H = 600;
	std::vector<uint16_t> aScreen((size_t)W * H);
	int aClip[4] = {0, 0, W - 1, H - 1};
	int aSheet = 0;
	int i = aFirst;
	while (i < Bank_Count(aShow) && aSheet < 4)
	{
		for (size_t k = 0; k < aScreen.size(); k++)
			aScreen[k] = (uint16_t)((((k / W) / 16 + (k % W) / 16) & 1) ? 0x4208 : 0x8410);
		int x = 0, y = 0, aRowH = 0;
		for (; i < Bank_Count(aShow); i++)
		{
			Image* anImage = Bank_ImageAt(aShow, (uint32_t)i);
			int w = anImage->origW > 0 ? anImage->origW : anImage->w;
			int h = anImage->origH > 0 ? anImage->origH : anImage->h;
			if (x + w > W)
			{
				x = 0;
				y += aRowH + 2;
				aRowH = 0;
			}
			if (y + h > H && aRowH > 0)
				break;
			if (anImage->isMask)
				Image_Blit(anImage, (uint8_t*)aScreen.data(), W * 2, &aScreenFmt, aClip, x, y);
			else if (anImage->fmt.bpp == 8)
				Image_BlitMasked(anImage, (uint8_t*)aScreen.data(), W * 2, &aScreenFmt, aClip, x, y);
			else
				Image_BlitAlpha(anImage, (uint8_t*)aScreen.data(), W * 2, &aScreenFmt, aClip, x, y);
			std::printf("%d %s %dx%d bpp %d mask %d at %d,%d\n", i, anImage->name, anImage->w, anImage->h,
				anImage->bankFmt.bpp, anImage->hasMask, x, y);
			x += w + 2;
			if (h > aRowH)
				aRowH = h;
		}
		char aPath[512];
		std::snprintf(aPath, sizeof(aPath), "%s/%s_%d.png", argv[2], aWhich, aSheet);
		WritePng(aPath, aScreen.data(), W, H);
		aSheet++;
	}
	delete aBank;
	delete aBank8;
	return 0;
}
