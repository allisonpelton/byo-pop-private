// JPEG decoder test (tools/jpegtest): Platform::DecodeJPEG, LoadImageFromMemory and DecodeImage (platform/JPEGDecoder.cpp,
// platform/ImageLoader.cpp) against what IJG libjpeg 6b gives, which is what the games' own exes decode their JPEGs
// with. Native: jpegtest.exe. Web: jpegtest.js, run with Node (the error paths there are setjmp/longjmp through
// Emscripten's JS support, which only a wasm run shows). tools/check.sh unit and unit-web run it.
//
//  * IJG's own test files (third_party/libjpeg/test): testorig.jpg (baseline) and testprog.jpg (progressive) must give
//    testimg.ppm, byte for byte: the reference output of `djpeg -dct int -ppm`, which libjpeg's makefile compares in
//    its `make test`. That is the decoder with the default settings (accurate integer IDCT, fancy upsampling).
//  * The synthetic pictures of tools/jpegtest/data (make_vectors.py: grey, 4:4:4, 4:2:2, 4:2:0, restart markers,
//    progressive, cut off, damaged, CMYK) against a hash of the pixels libjpeg 6b gives, taken from PopCap's own copy
//    of the library (SexyAppFramework's ImageLib/jpeg, built with its stdio source and the settings of its
//    GetJPEGImage, outside this repository) and from this decoder; the two agree.
//  * The loader's routing: DecodeImage gives opaque ARGB of the same pixels, a JPEG that libjpeg cannot read (CMYK) still
//    loads through stb_image, LoadImageFromMemory's conversions are stb_image's.
//
//   jpegtest [-data <folder of data/>] [-ijg <folder of test/>]   the checks (both folders have defaults)
//   jpegtest -print <files>...                                    one line per file: the hash this test expects
//   jpegtest -hash <files>...                                     likewise, for the files of a game's folder (not a check)
// The line format is "<file> <width>x<height>x<components> <hash>" (hash: FNV-1a 64 of the pixel bytes); the oracle that
// builds SexyAppFramework's library prints the same lines.

#include "platform/ImageLoader.h"

#include <SDL.h>  // SDL_main: the test links the framework like a game (this is not an SDL program)
#include <stb_image.h>

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifndef JPEGTEST_DATA
#define JPEGTEST_DATA "tools/jpegtest/data"
#endif
#ifndef JPEGTEST_IJG
#define JPEGTEST_IJG "third_party/libjpeg/test"
#endif

static int gChecks = 0;
static int gFailed = 0;

static void Check(bool theOk, const char* theWhat)
{
	gChecks++;
	if (!theOk)
	{
		gFailed++;
		printf("FAIL: %s\n", theWhat);
	}
}

static bool ReadFile(const std::string& thePath, std::vector<uint8_t>& theData)
{
	FILE* f = fopen(thePath.c_str(), "rb");
	if (!f)
		return false;
	uint8_t buf[65536];
	size_t n;
	theData.clear();
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
		theData.insert(theData.end(), buf, buf + n);
	fclose(f);
	return true;
}

static uint64_t Fnv(const uint8_t* theData, size_t theSize)
{
	uint64_t h = 1469598103934665603ull;
	for (size_t i = 0; i < theSize; i++)
		h = (h ^ theData[i]) * 1099511628211ull;
	return h;
}

static std::string Line(const std::string& theName, int w, int h, int c, const std::vector<uint8_t>& thePixels)
{
	char buf[64];
	snprintf(buf, sizeof(buf), " %dx%dx%d %016" PRIx64, w, h, c, Fnv(thePixels.data(), thePixels.size()));
	return theName + buf;
}

static std::string Base(const std::string& thePath)
{
	size_t s = thePath.find_last_of("/\\");
	return s == std::string::npos ? thePath : thePath.substr(s + 1);
}

// A PPM (P6) file's pixels.
static bool ReadPPM(const std::string& thePath, int& w, int& h, std::vector<uint8_t>& thePixels)
{
	std::vector<uint8_t> d;
	if (!ReadFile(thePath, d) || d.size() < 11 || d[0] != 'P' || d[1] != '6')
		return false;
	size_t pos = 2;
	int vals[3], n = 0;
	while (n < 3 && pos < d.size())
	{
		if (d[pos] == '#')
		{
			while (pos < d.size() && d[pos] != '\n')
				pos++;
		}
		else if (d[pos] >= '0' && d[pos] <= '9')
		{
			int v = 0;
			while (pos < d.size() && d[pos] >= '0' && d[pos] <= '9')
				v = v * 10 + (d[pos++] - '0');
			vals[n++] = v;
		}
		else
			pos++;
	}
	pos++;  // the one whitespace byte after maxval
	w = vals[0];
	h = vals[1];
	if (n < 3 || vals[2] != 255 || d.size() < pos + (size_t)w * h * 3)
		return false;
	thePixels.assign(d.begin() + pos, d.begin() + pos + (size_t)w * h * 3);
	return true;
}

static int DecodeFile(const std::string& thePath, std::string* theLine)
{
	std::vector<uint8_t> data, px;
	int w = 0, h = 0, c = 0;
	if (!ReadFile(thePath, data))
	{
		*theLine = Base(thePath) + " unreadable";
		return 0;
	}
	if (!Platform::DecodeJPEG(data.data(), data.size(), w, h, c, px))
	{
		*theLine = Base(thePath) + " rejected";
		return 0;
	}
	*theLine = Line(Base(thePath), w, h, c, px);
	return 1;
}

struct Vector
{
	const char* mName;
	const char* mExpect;  // "<w>x<h>x<c> <hash>", or "rejected"
};

// The hashes libjpeg 6b gives (see the header); `jpegtest -print tools/jpegtest/data/*.jpg` prints them after a regeneration.
static const Vector kVectors[] = {
	{"gray.jpg", "67x45x1 761803c3037e4d39"},
	{"gray_prog.jpg", "67x45x1 462477d23f4b26fd"},
	{"c444.jpg", "67x45x3 12da4ab87e752afb"},
	{"c422.jpg", "67x45x3 df92e739b320e9cd"},
	{"c420.jpg", "67x45x3 a59b4f002734c653"},
	{"c420_rst.jpg", "67x45x3 8df5ef3ec1ab2804"},
	{"c420_prog.jpg", "67x45x3 8df5ef3ec1ab2804"},
	{"c420_cut.jpg", "67x45x3 9909b416fd358d57"},
	{"bad_header.jpg", "rejected"},
	{"cmyk.jpg", "rejected"},
};

static std::string Join(const std::string& theDir, const char* theName)
{
	return theDir + "/" + theName;
}

static void CheckIjgFile(const std::string& theIjg, const char* theJpeg, const std::vector<uint8_t>& theRef, int w, int h)
{
	std::vector<uint8_t> data, px;
	int jw = 0, jh = 0, jc = 0;
	std::string what = std::string("IJG ") + theJpeg + " decodes to testimg.ppm";
	bool ok = ReadFile(Join(theIjg, theJpeg), data) && Platform::DecodeJPEG(data.data(), data.size(), jw, jh, jc, px);
	Check(ok, (what + " (decoded)").c_str());
	if (!ok)
		return;
	Check(jw == w && jh == h && jc == 3, (what + " (size)").c_str());
	size_t differ = 0;
	int worst = 0;
	for (size_t i = 0; i < px.size() && i < theRef.size(); i++)
	{
		int d = abs((int)px[i] - (int)theRef[i]);
		if (d)
			differ++;
		if (d > worst)
			worst = d;
	}
	if (differ)
		printf("  %s: %zu of %zu bytes differ, by up to %d\n", theJpeg, differ, theRef.size(), worst);
	Check(px.size() == theRef.size() && differ == 0, (what + " (every byte)").c_str());

	// For the record: what stb_image makes of the same file (not a check: it differs, which is why libjpeg is here)
	int sw = 0, sh = 0, sc = 0;
	uint8_t* sp = stbi_load_from_memory(data.data(), (int)data.size(), &sw, &sh, &sc, 3);
	if (sp)
	{
		size_t sdiff = 0;
		int sworst = 0;
		for (size_t i = 0; i < theRef.size() && i < (size_t)sw * sh * 3; i++)
		{
			int d = abs((int)sp[i] - (int)theRef[i]);
			if (d)
				sdiff++;
			if (d > sworst)
				sworst = d;
		}
		printf("  (stb_image: %zu of %zu bytes differ from libjpeg's, by up to %d)\n", sdiff, theRef.size(), sworst);
		stbi_image_free(sp);
	}
}

int main(int argc, char** argv)
{
	std::string aData = JPEGTEST_DATA, anIjg = JPEGTEST_IJG;
	bool print = false, hash = false;
	std::vector<std::string> files;
	for (int i = 1; i < argc; i++)
	{
		std::string a = argv[i];
		if (a == "-data" && i + 1 < argc)
			aData = argv[++i];
		else if (a == "-ijg" && i + 1 < argc)
			anIjg = argv[++i];
		else if (a == "-print")
			print = true;
		else if (a == "-hash")
			hash = true;
		else
			files.push_back(a);
	}

	if (print || hash)
	{
		int bad = 0;
		for (const std::string& f : files)
		{
			std::string line;
			bad += DecodeFile(f, &line) ? 0 : 1;
			printf("%s\n", line.c_str());
		}
		return (hash || !bad) ? 0 : 1;
	}

	// ---- IJG's own reference output
	{
		int w = 0, h = 0;
		std::vector<uint8_t> ref;
		bool have = ReadPPM(Join(anIjg, "testimg.ppm"), w, h, ref);
		Check(have, "IJG testimg.ppm is there");
		if (have)
		{
			CheckIjgFile(anIjg, "testorig.jpg", ref, w, h);
			CheckIjgFile(anIjg, "testprog.jpg", ref, w, h);
		}
	}

	// ---- the synthetic pictures
	for (const Vector& v : kVectors)
	{
		std::string line;
		DecodeFile(Join(aData, v.mName), &line);
		std::string want = std::string(v.mName) + " " + v.mExpect;
		Check(line == want, (want + "   got: " + line).c_str());
	}

	// ---- DecodeImage: opaque ARGB of the same pixels; CMYK falls back to stb_image
	{
		std::vector<uint8_t> data, px;
		std::vector<uint32_t> argb;
		int w = 0, h = 0, c = 0, aw = 0, ah = 0;
		bool alpha = true;
		ReadFile(Join(aData, "c420.jpg"), data);
		bool ok = Platform::DecodeJPEG(data.data(), data.size(), w, h, c, px);
		Check(ok && Platform::DecodeImage(data, aw, ah, argb, &alpha), "DecodeImage reads a JPEG");
		if (ok)
		{
			bool same = (aw == w && ah == h && argb.size() == (size_t)w * h && !alpha);
			for (size_t i = 0; same && i < argb.size(); i++)
				same = argb[i] == (0xFF000000u | ((uint32_t)px[i * 3] << 16) | ((uint32_t)px[i * 3 + 1] << 8) | px[i * 3 + 2]);
			Check(same, "DecodeImage: the decoder's pixels, alpha 255, no alpha reported");
		}

		ReadFile(Join(aData, "gray.jpg"), data);
		ok = Platform::DecodeJPEG(data.data(), data.size(), w, h, c, px);
		Check(ok && c == 1 && Platform::DecodeImage(data, aw, ah, argb, &alpha) && !alpha, "DecodeImage reads a grey JPEG");
		if (ok && !argb.empty())
			Check(argb[0] == (0xFF000000u | (px[0] * 0x010101u)) && argb.back() == (0xFF000000u | (px.back() * 0x010101u)),
				"DecodeImage: grey as r = g = b");

		ReadFile(Join(aData, "cmyk.jpg"), data);
		Check(!Platform::DecodeJPEG(data.data(), data.size(), w, h, c, px), "CMYK is not for DecodeJPEG");
		Check(Platform::DecodeImage(data, aw, ah, argb, &alpha) && aw == 67 && ah == 45, "CMYK still loads through stb_image");

		ReadFile(Join(aData, "bad_header.jpg"), data);
		Check(!Platform::DecodeImage(data, aw, ah, argb, &alpha), "a damaged header fails in DecodeImage too");
		std::vector<uint8_t> notjpeg = {'n', 'o', 't', ' ', 'a', ' ', 'p', 'i', 'c'};
		Check(!Platform::IsJPEG(notjpeg.data(), notjpeg.size()) && !Platform::DecodeImage(notjpeg, aw, ah, argb, &alpha),
			"not a JPEG: rejected");
		Check(!Platform::IsJPEG(data.data(), 2), "a bare SOI is not enough");
	}

	// ---- LoadImageFromMemory: libjpeg's pixels with stb_image's component conversions
	{
		std::vector<uint8_t> data, px, gpx;
		int w = 0, h = 0, c = 0, gw = 0, gh = 0, gc = 0;
		ReadFile(Join(aData, "c420.jpg"), data);
		Platform::DecodeJPEG(data.data(), data.size(), w, h, c, px);
		int lw = 0, lh = 0, lc = 0;
		uint8_t* p0 = Platform::LoadImageFromMemory(data.data(), (int)data.size(), &lw, &lh, &lc, 0);
		Check(p0 && lw == w && lh == h && lc == 3 && memcmp(p0, px.data(), px.size()) == 0, "LoadImageFromMemory, required 0");
		stbi_image_free(p0);
		uint8_t* p4 = Platform::LoadImageFromMemory(data.data(), (int)data.size(), &lw, &lh, &lc, 4);
		bool rgba = p4 && lc == 3;
		for (size_t i = 0; rgba && i < (size_t)w * h; i++)
			rgba = p4[i * 4] == px[i * 3] && p4[i * 4 + 1] == px[i * 3 + 1] && p4[i * 4 + 2] == px[i * 3 + 2] && p4[i * 4 + 3] == 255;
		Check(rgba, "LoadImageFromMemory, required 4: RGB and alpha 255");
		free(p4);
		uint8_t* p1 = Platform::LoadImageFromMemory(data.data(), (int)data.size(), &lw, &lh, &lc, 1);
		bool grey = p1 != nullptr;
		for (size_t i = 0; grey && i < (size_t)w * h; i++)
			grey = p1[i] == (uint8_t)((px[i * 3] * 77 + px[i * 3 + 1] * 150 + 29 * px[i * 3 + 2]) >> 8);
		Check(grey, "LoadImageFromMemory, required 1: stb_image's weights");
		free(p1);

		ReadFile(Join(aData, "gray.jpg"), data);
		Platform::DecodeJPEG(data.data(), data.size(), gw, gh, gc, gpx);
		uint8_t* g3 = Platform::LoadImageFromMemory(data.data(), (int)data.size(), &lw, &lh, &lc, 3);
		bool g3ok = g3 && lc == 1;
		for (size_t i = 0; g3ok && i < (size_t)gw * gh; i++)
			g3ok = g3[i * 3] == gpx[i] && g3[i * 3 + 1] == gpx[i] && g3[i * 3 + 2] == gpx[i];
		Check(g3ok, "LoadImageFromMemory: a grey file asked for RGB repeats the value");
		free(g3);

		// not a JPEG, or one libjpeg refuses: stb_image's answer, same contract
		ReadFile(Join(aData, "cmyk.jpg"), data);
		uint8_t* pc = Platform::LoadImageFromMemory(data.data(), (int)data.size(), &lw, &lh, &lc, 3);
		Check(pc && lw == 67 && lh == 45, "LoadImageFromMemory: CMYK through stb_image");
		stbi_image_free(pc);
		ReadFile(Join(aData, "bad_header.jpg"), data);
		Check(Platform::LoadImageFromMemory(data.data(), (int)data.size(), &lw, &lh, &lc, 0) == nullptr, "LoadImageFromMemory: damaged file fails");
		Check(Platform::LoadImageFromMemory(nullptr, 0, &lw, &lh, &lc, 0) == nullptr, "LoadImageFromMemory: no data fails");
	}

	printf("jpeg: %d checks, %d failed\n", gChecks, gFailed);
	return gFailed ? 1 : 0;
}
