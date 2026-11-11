// Loads Water Bugs' art banks the way GameMain does (water_art8.bbk raw, water_art.bbk converted to the 16-bit screen)
// and draws every image of one onto 800x600 sheets through the engine's blitters, written as PNGs. "fonts" draws a line
// in each font water_bugs.xml names, plain, outlined and filled.
//   waterbugs_banktest <gamedir> <outdir> [art|art8|fonts|anims] [first image]
// Opens no window and plays no sound.

#include "R64.h"
#include "R64Bank.h"
#include "R64Font.h"
#include "R64Sys.h"
#include "R64Xml.h"
#include "WBGame.h"

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
		bool anOk = n == 0 || std::fread(theData.data(), (size_t)n, 1, f) == 1;
		std::fclose(f);
		return anOk;
	}

	// (nothing here draws to the screen, keeps time or saves)
	bool LockScreen(uint8_t**, int*) override
	{
		return false;
	}

	uint32_t TimeMs() override
	{
		return 0;
	}

	const char* SavePath(const char* theName) override
	{
		return theName;
	}
};

void WritePng(const char* thePath, const uint16_t* theScreen, int theWidth, int theHeight)
{
	std::vector<uint8_t> aRgb((size_t)theWidth * theHeight * 3);
	for (int i = 0; i < theWidth * theHeight; i++)
	{
		uint16_t p = theScreen[i];
		aRgb[i * 3] = (uint8_t)(((p >> 11) & 0x1f) << 3);
		aRgb[i * 3 + 1] = (uint8_t)(((p >> 5) & 0x3f) << 2);
		aRgb[i * 3 + 2] = (uint8_t)((p & 0x1f) << 3);
	}
	stbi_write_png(thePath, theWidth, theHeight, 3, aRgb.data(), theWidth * 3);
}


// false when water_bugs.xml or a font fails (a font with no height)
bool DrawFonts(Bank* theArt, Bank* theArt8, const PixelFormat* theFmt, const char* theOutDir)
{
	std::vector<uint8_t> aXml;
	if (!Sys_ReadFile("wanims\\water_bugs.xml", aXml))
	{
		std::fprintf(stderr, "water_bugs.xml failed\n");
		return false;
	}
	aXml.push_back(0);
	const int W = 800, H = 600;
	std::vector<uint16_t> aScreen((size_t)W * H, 0x2104);
	XmlTag aTag;
	const char* p = (const char*)aXml.data();
	int y = 4, aLoaded = 0, aFailed = 0, aEmpty = 0;
	std::vector<Font*> aFonts;
	while (p != nullptr && *p != 0 && y < H)
	{
		p = Xml_NextTag(p, &aTag);
		if (std::strcmp(aTag.mName, "font") != 0)
			continue;
		const char* aFace = Xml_GetString(&aTag, "face");
		const char* aStrip = Xml_GetString(&aTag, "strip");
		Bank* aBank = Xml_GetInt(&aTag, "8bit") != 0 ? theArt8 : theArt;
		Font* aFont = new Font();
		aFonts.push_back(aFont);
		if (aStrip != nullptr)
			Font_StripCut(aFont, aBank, aFace, aStrip);
		else
			Font_LoadFromBank(aFont, aBank, aFace);
		Font_SetTarget(aFont, (uint8_t*)aScreen.data(), W * 2, theFmt, W - 1, H - 1);
		Font_SetSpacing(aFont, Xml_GetInt(&aTag, "spacing"));
		Font_SetEscapes(aFont, 1);
		int aRect[4];
		Font_DrawOutlined(aFont, aRect, 4, y, "0123456789 ABCDEFGHIJ abcdefghij");
		Font_DrawFill(aFont, "0123456789", 560, y, 0, 0xff, 0x80, 0x20, 0x80);
		std::printf("font %s face %s height %d drawn %d,%d-%d,%d\n", Xml_GetString(&aTag, "name"), aFace,
			Font_Height(aFont), aRect[0], aRect[1], aRect[2], aRect[3]);
		// percent_font, score_font and lives_font name a face ("34p_") that neither bank holds; they load empty in the
		// original too, so a font with no height is a failure only when its face has images
		std::string aZero = std::string(aFace) + "048";
		if (Font_Height(aFont) > 0)
			aLoaded++;
		else if (aStrip == nullptr && Bank_IndexOf(aBank, aZero.c_str()) < 1)
		{
			std::printf("font %s has no images in the game\n", Xml_GetString(&aTag, "name"));
			aEmpty++;
		}
		else
			aFailed++;
		y += Font_Height(aFont) + 6;
	}
	char aPath[512];
	std::snprintf(aPath, sizeof(aPath), "%s/fonts.png", theOutDir);
	WritePng(aPath, aScreen.data(), W, H);
	for (Font* aFont : aFonts)
	{
		Font_Clear(aFont);
		delete aFont;
	}
	std::printf("%d fonts, %d empty as in the game, %d failed\n", aLoaded, aEmpty, aFailed);
	return aFailed == 0 && aLoaded > 0;
}


// The file an animation names is in the game's folder, where Animation_Load opens it ("wanims/X", an .eco read
// as the .flc)
bool AnimFileExists(const char* theFile)
{
	std::string aPath = std::string("wanims/") + theFile;
	size_t anEco = aPath.find(".eco");
	if (anEco != std::string::npos)
		aPath.replace(anEco, 4, ".flc");
	return Sys_FileExists(aPath.c_str());
}

// Every <animation> of water_bugs.xml loaded as the game loads it, the first frame of each drawn with its mask.
// water_bugs.xml names a few files the game does not ship (they load no frame in the original either): those are
// counted as missing; false when one that is there has no first frame.
bool DrawAnims(Bank* theArt, const PixelFormat* theFmt, const char* theOutDir)
{
	if (Bank_Load(&wb::gArtBank, "water_art.bbk", 1, theFmt) == 0)
		return false;
	std::vector<uint8_t> aXml;
	if (!Sys_ReadFile("wanims\\water_bugs.xml", aXml))
		return false;
	aXml.push_back(0);
	const int W = 800, H = 600;
	std::vector<uint16_t> aScreen((size_t)W * H);
	int aClip[4] = {0, 0, W - 1, H - 1};
	XmlTag aTag;
	const char* p = (const char*)aXml.data();
	int x = 0, y = 0, aRowH = 0, aSheet = 0, n = 0, aMissing = 0, aFailed = 0;
	auto aFlush = [&]() {
		char aPath[512];
		std::snprintf(aPath, sizeof(aPath), "%s/anims_%d.png", theOutDir, aSheet++);
		WritePng(aPath, aScreen.data(), W, H);
	};
	for (size_t k = 0; k < aScreen.size(); k++)
		aScreen[k] = (uint16_t)((((k / W) / 16 + (k % W) / 16) & 1) ? 0x4208 : 0x8410);
	while (p != nullptr && *p != 0)
	{
		p = Xml_NextTag(p, &aTag);
		if (std::strcmp(aTag.mName, "animation") != 0)
			continue;
		wb::Animation* anAnim = wb::List_Append(&wb::gAnims, new wb::Animation());
		wb::Animation_FromXml(anAnim, &aTag);
		std::printf("anim %s file %s frames %d time %d\n", anAnim->mName, anAnim->mFile, anAnim->mCount,
			anAnim->mFrameTime);
		if (anAnim->mCount < 1 || anAnim->mFrames[0] == nullptr)
		{
			if ((std::strstr(anAnim->mFile, ".eco") != nullptr || std::strstr(anAnim->mFile, ".tga") != nullptr) &&
				!AnimFileExists(anAnim->mFile))
			{
				std::printf("anim %s missing (%s is not in the game)\n", anAnim->mName, anAnim->mFile);
				aMissing++;
			}
			else
			{
				std::printf("anim %s FAILED\n", anAnim->mName);
				aFailed++;
			}
			continue;
		}
		n++;
		Image* aFrame = anAnim->mFrames[0];
		if (x + aFrame->mW > W)
		{
			x = 0;
			y += aRowH + 2;
			aRowH = 0;
		}
		if (y + aFrame->mH > H)
		{
			aFlush();
			for (size_t k = 0; k < aScreen.size(); k++)
				aScreen[k] = (uint16_t)((((k / W) / 16 + (k % W) / 16) & 1) ? 0x4208 : 0x8410);
			x = 0;
			y = 0;
			aRowH = 0;
		}
		if (aFrame->mHasMask != 0)
			Image_BlitAlpha(aFrame, (uint8_t*)aScreen.data(), W * 2, theFmt, aClip, x, y);
		else
			Image_Blit(aFrame, (uint8_t*)aScreen.data(), W * 2, theFmt, aClip, x, y);
		x += aFrame->mW + 2;
		if (aFrame->mH > aRowH)
			aRowH = aFrame->mH;
	}
	aFlush();
	std::printf("%d animations, %d missing, %d failed\n", n, aMissing, aFailed);
	wb::List_Clear(&wb::gAnims);
	(void)theArt;
	return aFailed == 0 && n > 0;
}

} // namespace

int main(int argc, char** argv)
{
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	if (argc < 3)
	{
		std::fprintf(stderr, "usage: waterbugs_banktest <gamedir> <outdir> [art|art8] [first image]\n");
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
	if (Bank_Load(aBank8, "water_art8.bbk", 0, &aScreenFmt) == 0)
	{
		std::fprintf(stderr, "water_art8.bbk failed\n");
		return 1;
	}
	if (Bank_Load(aBank, "water_art.bbk", 1, &aScreenFmt) == 0)
	{
		std::fprintf(stderr, "water_art.bbk failed\n");
		return 1;
	}
	std::printf("art8: %d images, %d palettes; art: %d images, %u bytes\n", Bank_Count(aBank8),
		(int)aBank8->mPalettes.size(), Bank_Count(aBank), aBank->mDataSize);

	if (std::strcmp(aWhich, "anims") == 0)
	{
		Sys_SetScreenFormat(&aScreenFmt);
		bool anOk = DrawAnims(aBank, &aScreenFmt, argv[2]);
		delete aBank;
		delete aBank8;
		return anOk ? 0 : 1;
	}
	if (std::strcmp(aWhich, "fonts") == 0)
	{
		bool anOk = DrawFonts(aBank, aBank8, &aScreenFmt, argv[2]);
		delete aBank;
		delete aBank8;
		return anOk ? 0 : 1;
	}
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
			int w = anImage->mOrigW > 0 ? anImage->mOrigW : anImage->mW;
			int h = anImage->mOrigH > 0 ? anImage->mOrigH : anImage->mH;
			if (x + w > W)
			{
				x = 0;
				y += aRowH + 2;
				aRowH = 0;
			}
			if (y + h > H && aRowH > 0)
				break;
			if (anImage->mIsMask)
				Image_Blit(anImage, (uint8_t*)aScreen.data(), W * 2, &aScreenFmt, aClip, x, y);
			else if (anImage->mFmt.mBpp == 8)
				Image_BlitMasked(anImage, (uint8_t*)aScreen.data(), W * 2, &aScreenFmt, aClip, x, y);
			else
				Image_BlitAlpha(anImage, (uint8_t*)aScreen.data(), W * 2, &aScreenFmt, aClip, x, y);
			std::printf("%d %s %dx%d bpp %d mask %d at %d,%d\n", i, anImage->mName, anImage->mW, anImage->mH,
				anImage->mBankFmt.mBpp, anImage->mHasMask, x, y);
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
