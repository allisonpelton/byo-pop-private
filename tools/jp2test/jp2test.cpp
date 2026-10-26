// JPEG 2000 test (platform/openjpeg, Platform::RegisterImageFormat): native only, no window, no sound.
//
//   jp2test <folder> [-tolerance=N]
//
// Decodes every .jp2 under <folder> that has a .png of the same stem beside it (a decode of the same image
// by another program, like the staged PNGs of Bookworm Adventures) through Platform::DecodeImage, and compares
// the pixels with the PNG's (stb_image): a file whose size differs, or whose largest channel difference is
// above the tolerance (default 0), prints one line and fails the run. Then the image loader's own rules with
// those files, in a scratch folder under the current directory:
//   * "name_.jp2" beside "name.jp2" is an alpha mask: LoadGameImage("name") takes the colour from the
//     first and the alpha from the mask's blue channel;
//   * a "name.png" beside the .jp2 wins over it (registered extensions search after the built-in ones).
// The last line is "jp2: N files, max channel difference D" (plus " (loader rules: M problems)" when a
// loader rule fails). Exit status 0 only when everything matched.
//
// The staged images are all 8-bit RGB, so the other layouts are tested with images made here: OpenJPEG's own
// encoder writes grey, grey + alpha, RGB and RGBA images (a .jp2 file and a raw .j2k codestream), 4, 12 and 16
// bits per sample, signed samples and subsampled sYCC, losslessly, and each must decode to the pixels the
// layouts of platform/openjpeg/JP2Decoder.cpp give.

#include "platform/ImageLoader.h"
#include "platform/VFS.h"
#include "platform/openjpeg/JP2Decoder.h"

#include <openjpeg.h>
#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static bool ReadFile(const fs::path& thePath, std::vector<uint8_t>& theData)
{
	FILE* f = fopen(thePath.string().c_str(), "rb");
	if (!f)
		return false;
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	theData.assign(len > 0 ? (size_t)len : 0, 0);
	size_t got = len > 0 ? fread(theData.data(), 1, theData.size(), f) : 0;
	fclose(f);
	return len > 0 && got == theData.size();
}

static std::string LowerStr(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)tolower(c); });
	return s;
}

struct Pic
{
	int mW = 0, mH = 0;
	std::vector<uint32_t> mBits;
	bool mAlpha = false;
};

static bool DecodeFile(const fs::path& thePath, Pic& thePic)
{
	std::vector<uint8_t> data;
	return ReadFile(thePath, data) && Platform::DecodeImage(data, thePic.mW, thePic.mH, thePic.mBits, &thePic.mAlpha);
}

static int MaxDiff(uint32_t a, uint32_t b)
{
	int d = 0;
	for (int s = 0; s < 32; s += 8)
		d = std::max(d, abs((int)((a >> s) & 0xFF) - (int)((b >> s) & 0xFF)));
	return d;
}

// --- images made with OpenJPEG's encoder ---

struct CompSpec
{
	int mDx, mDy, mPrec;
	bool mSigned;
};

struct Synth
{
	const char* mName;
	int mW, mH;
	std::vector<CompSpec> mComps;
	OPJ_COLOR_SPACE mSpace;
	bool mJP2;  // a .jp2 file, else a raw codestream
};

struct OutBuffer
{
	std::vector<uint8_t> mData;
	size_t mPos = 0;
};

static OPJ_SIZE_T OutWrite(void* theBuffer, OPJ_SIZE_T theCount, void* theUser)
{
	OutBuffer* b = (OutBuffer*)theUser;
	if (b->mPos + theCount > b->mData.size())
		b->mData.resize(b->mPos + theCount);
	memcpy(&b->mData[b->mPos], theBuffer, theCount);
	b->mPos += theCount;
	return theCount;
}

static OPJ_OFF_T OutSkip(OPJ_OFF_T theCount, void* theUser)
{
	OutBuffer* b = (OutBuffer*)theUser;
	if ((OPJ_OFF_T)b->mPos + theCount < 0)
		return -1;
	b->mPos = (size_t)((OPJ_OFF_T)b->mPos + theCount);
	if (b->mPos > b->mData.size())
		b->mData.resize(b->mPos);
	return theCount;
}

static OPJ_BOOL OutSeek(OPJ_OFF_T theOffset, void* theUser)
{
	OutBuffer* b = (OutBuffer*)theUser;
	if (theOffset < 0)
		return OPJ_FALSE;
	b->mPos = (size_t)theOffset;
	if (b->mPos > b->mData.size())
		b->mData.resize(b->mPos);
	return OPJ_TRUE;
}

// A sample of component c at its own pixel (x, y): any value its precision and sign allow, varying from pixel to
// pixel, with the extremes at some pixels.
static int RawSample(const Synth& s, int c, int x, int y)
{
	const CompSpec& k = s.mComps[c];
	uint32_t h = (uint32_t)(x * 73856093) ^ (uint32_t)(y * 19349663) ^ (uint32_t)((c + 1) * 83492791);
	h ^= h >> 13;
	h *= 0x5bd1e995u;
	h ^= h >> 15;
	int range = 1 << k.mPrec;
	int v = (int)(h % (uint32_t)range);
	if (((x + y) & 7) == 0)
		v = (x & 1) ? range - 1 : 0;
	return k.mSigned ? v - range / 2 : v;
}

static bool Encode(const Synth& s, std::vector<uint8_t>& theOut)
{
	opj_cparameters_t params;
	opj_set_default_encoder_parameters(&params);
	params.tcp_numlayers = 1;
	params.cp_disto_alloc = 1;
	params.tcp_rates[0] = 0;  // lossless
	params.irreversible = 0;
	params.numresolution = 2;
	params.tcp_mct = 0;
	if (s.mComps.size() >= 3 && s.mComps[1].mDx == 1 && s.mComps[2].mDx == 1)
		params.tcp_mct = 1;  // the reversible colour transform

	std::vector<opj_image_cmptparm_t> cp(s.mComps.size());
	for (size_t i = 0; i < cp.size(); i++)
	{
		memset(&cp[i], 0, sizeof(cp[i]));
		cp[i].dx = (OPJ_UINT32)s.mComps[i].mDx;
		cp[i].dy = (OPJ_UINT32)s.mComps[i].mDy;
		cp[i].w = (OPJ_UINT32)((s.mW + s.mComps[i].mDx - 1) / s.mComps[i].mDx);
		cp[i].h = (OPJ_UINT32)((s.mH + s.mComps[i].mDy - 1) / s.mComps[i].mDy);
		cp[i].prec = (OPJ_UINT32)s.mComps[i].mPrec;
		cp[i].sgnd = s.mComps[i].mSigned ? 1 : 0;
	}
	opj_image_t* image = opj_image_create((OPJ_UINT32)cp.size(), cp.data(), s.mSpace);
	if (!image)
		return false;
	image->x0 = 0;
	image->y0 = 0;
	image->x1 = (OPJ_UINT32)s.mW;
	image->y1 = (OPJ_UINT32)s.mH;
	for (size_t c = 0; c < cp.size(); c++)
	{
		for (OPJ_UINT32 y = 0; y < cp[c].h; y++)
		{
			for (OPJ_UINT32 x = 0; x < cp[c].w; x++)
				image->comps[c].data[y * cp[c].w + x] = RawSample(s, (int)c, (int)x, (int)y);
		}
	}

	bool ok = false;
	opj_codec_t* codec = opj_create_compress(s.mJP2 ? OPJ_CODEC_JP2 : OPJ_CODEC_J2K);
	OutBuffer out;
	opj_stream_t* stream = opj_stream_create(0x10000, OPJ_FALSE);
	if (codec && stream && opj_setup_encoder(codec, &params, image))
	{
		opj_stream_set_write_function(stream, OutWrite);
		opj_stream_set_skip_function(stream, OutSkip);
		opj_stream_set_seek_function(stream, OutSeek);
		opj_stream_set_user_data(stream, &out, nullptr);
		ok = opj_start_compress(codec, image, stream) && opj_encode(codec, stream) && opj_end_compress(codec, stream);
	}
	if (stream)
		opj_stream_destroy(stream);
	if (codec)
		opj_destroy_codec(codec);
	opj_image_destroy(image);
	if (ok)
		theOut = out.mData;
	return ok;
}

// A sample scaled to 8 bits (rounded down from more bits, stretched up from fewer; signed samples shifted up).
static int To8(const CompSpec& k, int v)
{
	int64_t u = (int64_t)v + (k.mSigned ? ((int64_t)1 << (k.mPrec - 1)) : 0);
	int64_t max = ((int64_t)1 << k.mPrec) - 1;
	if (k.mPrec == 8)
		return (int)u;
	if (k.mPrec > 8)
		return (int)std::min<int64_t>((u + ((int64_t)1 << (k.mPrec - 9))) >> (k.mPrec - 8), 255);
	return (int)((u * 255 + max / 2) / max);
}

static uint32_t ExpectedPixel(const Synth& s, int x, int y, bool& theAlpha)
{
	int v[4] = {0, 0, 0, 255};
	for (size_t c = 0; c < s.mComps.size(); c++)
		v[c] = To8(s.mComps[c], RawSample(s, (int)c, x / s.mComps[c].mDx, y / s.mComps[c].mDy));
	int r, g, b, a = 255;
	if (s.mComps.size() <= 2)
	{
		r = g = b = v[0];
		if (s.mComps.size() == 2)
			a = v[1];
	}
	else
	{
		r = v[0];
		g = v[1];
		b = v[2];
		a = v[3];
		if (s.mSpace == OPJ_CLRSPC_SYCC)  // the conversion of OpenJPEG's own tools
		{
			int cb = v[1] - 128, cr = v[2] - 128;
			r = v[0] + (int)(1.402 * cr);
			g = v[0] - (int)(0.344 * cb + 0.714 * cr);
			b = v[0] + (int)(1.772 * cb);
		}
	}
	auto clamp = [](int n) { return (uint32_t)(n < 0 ? 0 : n > 255 ? 255 : n); };
	if (a != 255)
		theAlpha = true;
	return (clamp(a) << 24) | (clamp(r) << 16) | (clamp(g) << 8) | clamp(b);
}

// Returns the number of cases that failed.
static int RunSynthetic(int& theCases, std::vector<std::vector<uint8_t>>& theFiles)
{
	const CompSpec u8 = {1, 1, 8, false};
	const CompSpec u12 = {1, 1, 12, false};
	const Synth cases[] = {
		{"grey 8-bit jp2", 13, 9, {u8}, OPJ_CLRSPC_GRAY, true},
		{"grey 8-bit j2k", 13, 9, {u8}, OPJ_CLRSPC_GRAY, false},
		{"grey+alpha 8-bit jp2", 13, 9, {u8, u8}, OPJ_CLRSPC_GRAY, true},
		{"grey+alpha 8-bit j2k", 13, 9, {u8, u8}, OPJ_CLRSPC_GRAY, false},
		{"rgb 8-bit jp2", 13, 9, {u8, u8, u8}, OPJ_CLRSPC_SRGB, true},
		{"rgb 8-bit j2k", 13, 9, {u8, u8, u8}, OPJ_CLRSPC_SRGB, false},
		{"rgba 8-bit jp2", 13, 9, {u8, u8, u8, u8}, OPJ_CLRSPC_SRGB, true},
		{"rgba 8-bit j2k", 13, 9, {u8, u8, u8, u8}, OPJ_CLRSPC_SRGB, false},
		{"grey 4-bit", 16, 8, {{1, 1, 4, false}}, OPJ_CLRSPC_GRAY, true},
		{"grey 12-bit", 16, 8, {u12}, OPJ_CLRSPC_GRAY, true},
		{"grey 16-bit", 16, 8, {{1, 1, 16, false}}, OPJ_CLRSPC_GRAY, true},
		{"rgba 12-bit", 11, 7, {u12, u12, u12, u12}, OPJ_CLRSPC_SRGB, true},
		{"grey signed 8-bit", 16, 8, {{1, 1, 8, true}}, OPJ_CLRSPC_GRAY, true},
		{"grey+alpha signed 5-bit", 16, 8, {{1, 1, 5, true}, {1, 1, 5, true}}, OPJ_CLRSPC_GRAY, true},
		{"sycc 4:2:0 jp2", 13, 9, {u8, {2, 2, 8, false}, {2, 2, 8, false}}, OPJ_CLRSPC_SYCC, true},
		{"sycc 4:2:2 j2k", 13, 9, {u8, {2, 1, 8, false}, {2, 1, 8, false}}, OPJ_CLRSPC_SYCC, false},
	};
	int failed = 0;
	for (const Synth& s : cases)
	{
		theCases++;
		std::vector<uint8_t> data;
		if (!Encode(s, data))
		{
			printf("synthetic %s: the OpenJPEG encoder failed\n", s.mName);
			failed++;
			continue;
		}
		theFiles.push_back(data);
		Pic p;
		if (!Platform::DecodeImage(data, p.mW, p.mH, p.mBits, &p.mAlpha))
		{
			printf("synthetic %s: does not decode\n", s.mName);
			failed++;
			continue;
		}
		if (p.mW != s.mW || p.mH != s.mH)
		{
			printf("synthetic %s: size %dx%d, expected %dx%d\n", s.mName, p.mW, p.mH, s.mW, s.mH);
			failed++;
			continue;
		}
		bool alpha = false, bad = false;
		for (int y = 0; y < s.mH && !bad; y++)
		{
			for (int x = 0; x < s.mW; x++)
			{
				uint32_t e = ExpectedPixel(s, x, y, alpha);
				if (p.mBits[(size_t)y * s.mW + x] != e)
				{
					printf("synthetic %s: pixel (%d,%d) is %08x, expected %08x\n", s.mName, x, y, p.mBits[(size_t)y * s.mW + x], e);
					bad = true;
					break;
				}
			}
		}
		if (!bad && alpha != p.mAlpha)
		{
			printf("synthetic %s: hasAlpha is %d, expected %d\n", s.mName, p.mAlpha, alpha);
			bad = true;
		}
		if (bad)
			failed++;
	}
	return failed;
}

// --- damaged files ---

static int sLogLines = 0;

static void CountLog(void*, int, SDL_LogPriority, const char*)
{
	sLogLines++;
}

// Decodes truncated and corrupted copies of the files: none may crash or hang (run it under AddressSanitizer for
// the memory errors), and a file that fails to decode logs one line at most. Returns the number of files that
// logged more.
static int RunDamaged(const std::vector<std::vector<uint8_t>>& theFiles, int& theDecodes, int& theFailed)
{
	SDL_LogOutputFunction oldFunc;
	void* oldData;
	SDL_LogGetOutputFunction(&oldFunc, &oldData);
	SDL_LogSetOutputFunction(CountLog, nullptr);
	uint32_t seed = 12345;
	int tooMany = 0;
	for (const std::vector<uint8_t>& file : theFiles)
	{
		for (int v = 0; v < 20; v++)
		{
			std::vector<uint8_t> copy = file;
			if (v < 12)
				copy.resize(file.size() * (size_t)v / 12);  // truncated, down to nothing
			else
			{
				for (int k = 0; k < 3; k++)
				{
					seed = seed * 1664525u + 1013904223u;
					size_t at = 12 + (seed >> 8) % (file.size() > 12 ? file.size() - 12 : 1);  // past the signature
					seed = seed * 1664525u + 1013904223u;
					if (at < copy.size())
						copy[at] = (uint8_t)(seed >> 16);
				}
			}
			int w, h;
			std::vector<uint32_t> bits;
			bool alpha;
			sLogLines = 0;
			bool ok = Platform::DecodeImage(copy, w, h, bits, &alpha);
			theDecodes++;
			if (!ok)
				theFailed++;
			if (sLogLines > 1 || (ok && sLogLines))
				tooMany++;
		}
	}
	SDL_LogSetOutputFunction(oldFunc, oldData);
	return tooMany;
}

int main(int argc, char** argv)
{
	std::string folder;
	int tolerance = 0;
	for (int i = 1; i < argc; i++)
	{
		if (strncmp(argv[i], "-tolerance=", 11) == 0)
			tolerance = atoi(argv[i] + 11);
		else
			folder = argv[i];
	}
	if (folder.empty() || !fs::is_directory(folder))
	{
		fprintf(stderr, "usage: jp2test <folder> [-tolerance=N]\n");
		return 2;
	}

	Platform::RegisterOpenJPEGImages();
	Platform::RegisterOpenJPEGImages();  // twice: must register once

	std::vector<fs::path> jp2s;
	for (const fs::directory_entry& e : fs::recursive_directory_iterator(folder))
	{
		if (e.is_regular_file() && LowerStr(e.path().extension().string()) == ".jp2")
			jp2s.push_back(e.path());
	}
	std::sort(jp2s.begin(), jp2s.end());

	int files = 0, skipped = 0, failures = 0, maxDiff = 0;
	fs::path maskedPair;  // a "name.jp2" with a "name_.jp2" mask beside it, for the loader rules
	for (const fs::path& jp2 : jp2s)
	{
		fs::path png = jp2;
		png.replace_extension(".png");
		if (!fs::exists(png))
		{
			skipped++;
			continue;
		}
		files++;
		Pic a, b;
		std::string name = fs::relative(jp2, folder).generic_string();
		if (!DecodeFile(jp2, a))
		{
			printf("%s: the jp2 does not decode\n", name.c_str());
			failures++;
			continue;
		}
		if (!DecodeFile(png, b))
		{
			printf("%s: the png does not decode\n", name.c_str());
			failures++;
			continue;
		}
		if (a.mW != b.mW || a.mH != b.mH)
		{
			printf("%s: size %dx%d, the png has %dx%d\n", name.c_str(), a.mW, a.mH, b.mW, b.mH);
			failures++;
			continue;
		}
		int d = 0, at = 0;
		for (size_t i = 0; i < a.mBits.size(); i++)
		{
			int pd = MaxDiff(a.mBits[i], b.mBits[i]);
			if (pd > d)
			{
				d = pd;
				at = (int)i;
			}
		}
		if (d > 0 || a.mAlpha != b.mAlpha)
		{
			printf("%s: max channel difference %d at (%d,%d): %08x vs %08x; alpha %d vs %d\n", name.c_str(), d, at % a.mW, at / a.mW,
				a.mBits[at], b.mBits[at], a.mAlpha, b.mAlpha);
			if (d > tolerance || a.mAlpha != b.mAlpha)
				failures++;
		}
		maxDiff = std::max(maxDiff, d);

		fs::path mask = jp2.parent_path() / (jp2.stem().string() + "_.jp2");
		if (maskedPair.empty() && fs::exists(mask) && fs::exists(png))
			maskedPair = jp2;
	}
	if (skipped)
		printf("jp2: %d files without a png beside them were not compared\n", skipped);

	// --- the loader's rules with jp2 files ---
	int problems = 0;
	if (!maskedPair.empty())
	{
		fs::path root = fs::absolute("jp2test-scratch");
		std::error_code ec;
		fs::remove_all(root, ec);
		fs::create_directories(root / "jp2only" / "images");
		fs::create_directories(root / "withpng" / "images");
		std::string stem = maskedPair.stem().string();
		fs::path mask = maskedPair.parent_path() / (stem + "_.jp2");
		fs::path png = maskedPair;
		png.replace_extension(".png");
		fs::copy_file(maskedPair, root / "jp2only" / "images" / (stem + ".jp2"));
		fs::copy_file(mask, root / "jp2only" / "images" / (stem + "_.jp2"));
		fs::copy_file(maskedPair, root / "withpng" / "images" / (stem + ".jp2"));
		fs::copy_file(png, root / "withpng" / "images" / (stem + ".png"));
		fs::copy_file(mask, root / "withpng" / "images" / (stem + "_.jp2"));

		Pic colour, maskPic;
		DecodeFile(maskedPair, colour);
		DecodeFile(mask, maskPic);

		// 1. colour .jp2 + mask .jp2, no .png: the colour is the jp2's, the alpha the mask's blue channel
		Platform::VFS::Get().SetBaseDir((root / "jp2only").string());
		Platform::LoadedImage img;
		if (!Platform::LoadGameImage("images/" + stem, img))
		{
			printf("loader: images/%s (jp2 with a jp2 mask) does not load\n", stem.c_str());
			problems++;
		}
		else
		{
			bool ok = img.mWidth == colour.mW && img.mHeight == colour.mH && img.mBits.size() == colour.mBits.size() && img.mHasAlpha &&
				fs::path(img.mSourcePath).extension() == ".jp2" && maskPic.mW == colour.mW && maskPic.mH == colour.mH;
			for (size_t i = 0; ok && i < img.mBits.size(); i++)
				ok = (img.mBits[i] & 0xFFFFFF) == (colour.mBits[i] & 0xFFFFFF) && (img.mBits[i] >> 24) == (maskPic.mBits[i] & 0xFF);
			if (!ok)
			{
				printf("loader: images/%s: the jp2 mask is not applied as the alpha channel\n", stem.c_str());
				problems++;
			}
		}

		// 2. the same, with the extension given
		Platform::LoadedImage img2;
		if (!Platform::LoadGameImage("images/" + stem + ".jp2", img2) || img2.mBits != img.mBits)
		{
			printf("loader: images/%s.jp2 (extension given) differs from the extensionless load\n", stem.c_str());
			problems++;
		}

		// 3. a .png of the same name wins over the .jp2
		Platform::VFS::Get().SetBaseDir((root / "withpng").string());
		Platform::LoadedImage img3;
		if (!Platform::LoadGameImage("images/" + stem, img3) || fs::path(img3.mSourcePath).extension() != ".png")
		{
			printf("loader: images/%s: the .png does not win over the .jp2 (source %s)\n", stem.c_str(), img3.mSourcePath.c_str());
			problems++;
		}
		fs::remove_all(root, ec);
	}
	else
		printf("jp2: no name.jp2 with a name_.jp2 mask and a png beside them: the loader rules were not tested\n");

	int synthCases = 0;
	std::vector<std::vector<uint8_t>> damageable;
	int synthFailed = RunSynthetic(synthCases, damageable);
	problems += synthFailed;
	printf("jp2: %d synthetic images (grey, grey+alpha, RGB, RGBA, 4 to 16 bits, signed, subsampled sYCC), %d wrong\n", synthCases,
		synthFailed);

	for (size_t i = 0; i < jp2s.size() && i < 3; i++)  // a few of the real files too
	{
		std::vector<uint8_t> data;
		if (ReadFile(jp2s[i], data))
			damageable.push_back(data);
	}
	int damagedDecodes = 0, damagedFailed = 0;
	int tooManyLogs = RunDamaged(damageable, damagedDecodes, damagedFailed);
	problems += tooManyLogs;
	printf("jp2: %d truncated or corrupted files decoded without a crash (%d failed, %d logged more than once or on success)\n",
		damagedDecodes, damagedFailed, tooManyLogs);

	printf("jp2: %d files, max channel difference %d%s\n", files, maxDiff, problems ? " (loader rules or synthetic cases failed)" : "");
	return (failures || problems || files == 0) ? 1 : 0;
}
