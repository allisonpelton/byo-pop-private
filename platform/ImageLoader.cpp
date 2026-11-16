#include "ImageLoader.h"
#include "GIFDecoder.h"
#include "VFS.h"

#include <stb_image.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>

namespace Platform
{

static const char* kImageExtensions[] = {".tga", ".jpg", ".png", ".gif", ".jpeg", ".bmp", nullptr};
static const int kHDScales[] = {4, 3, 2};

// Formats a game registered with RegisterImageFormat: tried before stb, and their extensions join the
// extension search after the built-in ones (so a staged .png still wins over a .jp2 of the same name).
struct ImageFormat
{
	ImageDetector mDetect;
	ImageDecoder mDecode;
};

static std::vector<ImageFormat>& ImageFormats()
{
	static std::vector<ImageFormat> sFormats;
	return sFormats;
}

// The extension search order: the built-in extensions, then the registered ones in registration order.
static std::vector<std::string>& ImageExtensions()
{
	static std::vector<std::string> sExtensions;
	if (sExtensions.empty())
	{
		for (int i = 0; kImageExtensions[i]; i++)
			sExtensions.push_back(kImageExtensions[i]);
	}
	return sExtensions;
}

#ifndef BYOPOP_DERIVED
static std::map<std::string, int> gScaleOverrides;  // key: modId + '|' + normalized lower path
static std::map<std::string, int> gModDefaultScales;
#endif
static ImageGenerator gImageGenerator;

void SetImageGenerator(ImageGenerator theGenerator)
{
	gImageGenerator = theGenerator;
}

static std::string LowerStr(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

#ifndef BYOPOP_DERIVED
void SetImageScaleOverride(const std::string& theModId, const std::string& theGamePath, int theScale)
{
	std::string p = LowerStr(VFS::Normalize(theGamePath));
	size_t dot = p.find_last_of('.');
	size_t slash = p.find_last_of('/');
	if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
		p = p.substr(0, dot);
	gScaleOverrides[theModId + "|" + p] = theScale;
}

void SetModDefaultImageScale(const std::string& theModId, int theScale)
{
	gModDefaultScales[theModId] = theScale;
}

void ClearImageScaleOverrides()
{
	gScaleOverrides.clear();
	gModDefaultScales.clear();
}
#endif

// The game's editbox.png has a tEXt chunk before IHDR. PopCap's PNG reader accepted that, stb_image
// requires IHDR first; this copies the file with IHDR moved to the front (false if nothing to do).
static bool MoveIHDRFirst(const std::vector<uint8_t>& theData, std::vector<uint8_t>& theOut)
{
	static const uint8_t kSignature[8] = {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
	if ((theData.size() < 8 + 12) || (memcmp(theData.data(), kSignature, 8) != 0) || (memcmp(&theData[12], "IHDR", 4) == 0))
		return false;

	size_t pos = 8;
	while (pos + 12 <= theData.size())
	{
		const uint8_t* c = &theData[pos];
		size_t len = ((size_t)c[0] << 24) | ((size_t)c[1] << 16) | ((size_t)c[2] << 8) | c[3];
		if (len > theData.size() - pos - 12)
			return false;
		if (memcmp(c + 4, "IHDR", 4) == 0)
		{
			theOut.assign(theData.begin(), theData.begin() + 8);
			theOut.insert(theOut.end(), theData.begin() + pos, theData.begin() + pos + 12 + len);
			theOut.insert(theOut.end(), theData.begin() + 8, theData.begin() + pos);
			theOut.insert(theOut.end(), theData.begin() + pos + 12 + len, theData.end());
			return true;
		}
		pos += 12 + len;
	}
	return false;
}

void RegisterImageFormat(const char* theExtension, ImageDetector theDetector, ImageDecoder theDecoder)
{
	if (!theDetector || !theDecoder)
		return;
	std::string ext = LowerStr(theExtension ? theExtension : "");
	if (!ext.empty() && ext[0] != '.')
		ext = "." + ext;
	if (ext.size() > 1)
	{
		std::vector<std::string>& exts = ImageExtensions();
		if (std::find(exts.begin(), exts.end(), ext) == exts.end())
			exts.push_back(ext);
	}
	// one format under several extensions (.jp2 and .j2k) is tried once per file
	std::vector<ImageFormat>& formats = ImageFormats();
	for (const ImageFormat& f : formats)
	{
		if (f.mDetect == theDetector && f.mDecode == theDecoder)
			return;
	}
	formats.push_back({theDetector, theDecoder});
}

bool DecodeImage(const std::vector<uint8_t>& theData, int& theWidth, int& theHeight, std::vector<uint32_t>& theBits, bool* theHasAlpha)
{
	for (const ImageFormat& format : ImageFormats())
	{
		if (format.mDetect(theData) && format.mDecode(theData, theWidth, theHeight, theBits, theHasAlpha))
			return true;
	}

	// JPEGs through libjpeg 6b, as the games' own exes decode them (platform/JPEGDecoder.cpp); a file it refuses
	// (CMYK, say) is left to stb_image below
	int jw = 0, jh = 0, jcomp = 0;
	std::vector<uint8_t> jpx;
	if (IsJPEG(theData.data(), theData.size()) && DecodeJPEG(theData.data(), theData.size(), jw, jh, jcomp, jpx))
	{
		theWidth = jw;
		theHeight = jh;
		theBits.resize((size_t)jw * jh);
		for (size_t i = 0; i < theBits.size(); i++)
		{
			const uint8_t* p = &jpx[i * jcomp];
			const uint32_t r = p[0], g = (jcomp == 3) ? p[1] : p[0], b = (jcomp == 3) ? p[2] : p[0];
			theBits[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
		}
		if (theHasAlpha)
			*theHasAlpha = false;
		return true;
	}

	// port: GIFs through our own decoder, which gives stb_image's pixels in a fraction of its time (GIFDecoder.h); a file it
	// leaves (one stb would refuse or decode some other way) goes on to stb
	if ((theData.size() >= 4) && (memcmp(theData.data(), "GIF8", 4) == 0) &&
		DecodeGIF(theData.data(), theData.size(), theWidth, theHeight, theBits, theHasAlpha))
		return true;

	std::vector<uint8_t> reordered;
	const std::vector<uint8_t>& data = MoveIHDRFirst(theData, reordered) ? reordered : theData;

	int w = 0, h = 0, comp = 0;
	stbi_uc* px = stbi_load_from_memory(data.data(), (int)data.size(), &w, &h, &comp, 4);
	if (!px)
		return false;
	theWidth = w;
	theHeight = h;
	theBits.resize((size_t)w * h);
	bool alpha = false;
	for (size_t i = 0; i < theBits.size(); i++)
	{
		const stbi_uc* p = px + i * 4;
		theBits[i] = ((uint32_t)p[3] << 24) | ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
		if (p[3] != 255)
			alpha = true;
	}
	stbi_image_free(px);
	if (theHasAlpha)
		*theHasAlpha = alpha;
	return true;
}

// BYOPOP_GIF_QUANTIZE=1: GIFs decoded straight to 32 bits with no indices kept, so that Palletize quantizes them, as before
static bool QuantizeGIFs()
{
	static int aQuantize = -1;
	if (aQuantize < 0)
	{
		const char* aValue = getenv("BYOPOP_GIF_QUANTIZE");
		aQuantize = ((aValue != nullptr) && (atoi(aValue) != 0)) ? 1 : 0;
	}
	return aQuantize != 0;
}

// A GIF that DecodeImage would give to DecodeGIF, decoded to its indices (DecodeGIFIndices) with the colour of each. False
// for anything else (and for a frame that leaves pixels unreached), which DecodeImage decodes.
static bool DecodeGIFAsIndices(const std::vector<uint8_t>& theData, int& w, int& h, GIFIndices& theIndices)
{
	if (QuantizeGIFs() || (theData.size() < 4) || (memcmp(theData.data(), "GIF8", 4) != 0))
		return false;
	for (const ImageFormat& format : ImageFormats())
	{
		if (format.mDetect(theData))
			return false;  // DecodeImage tries the registered formats first
	}
	uint32_t aColors[256];
	if (!DecodeGIFIndices(theData.data(), theData.size(), w, h, theIndices.mIndices, aColors))
		return false;
	theIndices.mColors.assign(aColors, aColors + 256);
	return true;
}

static bool ReadFile(const std::string& theRealPath, std::vector<uint8_t>& theData)
{
	FILE* f = fopen(theRealPath.c_str(), "rb");
	if (!f)
		return false;
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	theData.resize(len > 0 ? (size_t)len : 0);
	size_t got = len > 0 ? fread(theData.data(), 1, theData.size(), f) : 0;
	fclose(f);
	return (got == theData.size()) && (!theData.empty());
}

// theIndices (when not null) gets a GIF's indices (DecodeGIFAsIndices), with the pixels made from them, the same pixels
// and alpha flag as DecodeGIF's; or it is emptied
static bool DecodeFile(const std::string& theRealPath, int& w, int& h, std::vector<uint32_t>& bits, bool* hasAlpha,
	GIFIndices* theIndices = nullptr)
{
	if (theIndices != nullptr)
		*theIndices = GIFIndices();
	std::vector<uint8_t> data;
	if (!ReadFile(theRealPath, data))
		return false;
	if ((theIndices != nullptr) && DecodeGIFAsIndices(data, w, h, *theIndices))
	{
		bits.resize(theIndices->mIndices.size());
		const uint8_t* anIndex = theIndices->mIndices.data();
		const uint32_t* aColors = theIndices->mColors.data();
		uint32_t* aPixel = bits.data();
		uint32_t anAll = 0xFFFFFFFF;  // every pixel ANDed: its alpha is 255 only when each one's is
		for (size_t i = 0; i < bits.size(); i++)
		{
			aPixel[i] = aColors[anIndex[i]];
			anAll &= aPixel[i];
		}
		if (hasAlpha)
			*hasAlpha = (anAll >> 24) != 0xFF;
		return true;
	}
	if (theIndices != nullptr)
		*theIndices = GIFIndices();
	return DecodeImage(data, w, h, bits, hasAlpha);
}

// A GIF mask's indices only (DecodeGIFAsIndices), for a colour picture that has its own: false for anything else
static bool DecodeMaskAsIndices(const std::string& theRealPath, int& w, int& h, GIFIndices& theIndices)
{
	std::vector<uint8_t> data;
	return ReadFile(theRealPath, data) && DecodeGIFAsIndices(data, w, h, theIndices);
}

struct Candidate
{
	std::string mPath;
	int mScale = 1;
};

// Looks for "<dir>/<prefix><stem><suffix><ext>" in one root, honouring an explicit extension.
static std::string FindVariant(const std::string& theRoot, const std::string& theDir, const std::string& theStem, const std::string& theExt)
{
	std::string base = theDir.empty() ? theStem : theDir + "/" + theStem;
	if (!theExt.empty())
		return VFS::FindIndexed(theRoot, base + theExt);
	for (const std::string& e : ImageExtensions())
	{
		std::string p = VFS::FindIndexed(theRoot, base + e);
		if (!p.empty())
			return p;
	}
	return "";
}

// Finds the colour image (best HD variant and plain 1x) for a stem in one root.
static void FindInRoot(const std::string& theRoot, const std::string& theDir, const std::string& theStem, const std::string& theExt,
	Candidate& theHD, Candidate& theOne)
{
	for (int s : kHDScales)
	{
		std::string p = FindVariant(theRoot, theDir, theStem + "@" + std::to_string(s) + "x", theExt);
		if (!p.empty())
		{
			theHD.mPath = p;
			theHD.mScale = s;
			break;
		}
	}
	theOne.mPath = FindVariant(theRoot, theDir, theStem, theExt);
	theOne.mScale = 1;
}

static void FindMaskInRoot(const std::string& theRoot, const std::string& theDir, const std::string& theStem,
	Candidate& theHD, Candidate& theOne)
{
	const std::string names[2] = {"_" + theStem, theStem + "_"};
	for (const std::string& n : names)
	{
		if (theHD.mPath.empty())
		{
			for (int s : kHDScales)
			{
				std::string p = FindVariant(theRoot, theDir, n + "@" + std::to_string(s) + "x", "");
				if (!p.empty())
				{
					theHD.mPath = p;
					theHD.mScale = s;
					break;
				}
			}
		}
		if (theOne.mPath.empty())
			theOne.mPath = FindVariant(theRoot, theDir, n, "");
	}
}

static void ApplyMask(std::vector<uint32_t>& theBits, const std::vector<uint32_t>& theMask)
{
	for (size_t i = 0; i < theBits.size(); i++)
		theBits[i] = (theBits[i] & 0x00FFFFFF) | ((theMask[i] & 0xFF) << 24);
}

static void BoxDownsample(const std::vector<uint32_t>& theSrc, int theSrcW, int theSrcH, int theScale, std::vector<uint32_t>& theDst, int& theDstW, int& theDstH)
{
	theDstW = theSrcW / theScale;
	theDstH = theSrcH / theScale;
	theDst.assign((size_t)theDstW * theDstH, 0);
	int n = theScale * theScale;
	for (int y = 0; y < theDstH; y++)
	{
		for (int x = 0; x < theDstW; x++)
		{
			// average in premultiplied space so transparent texels don't bleed colour
			uint32_t a = 0, r = 0, g = 0, b = 0;
			for (int sy = 0; sy < theScale; sy++)
			{
				for (int sx = 0; sx < theScale; sx++)
				{
					uint32_t p = theSrc[(size_t)(y * theScale + sy) * theSrcW + x * theScale + sx];
					uint32_t pa = p >> 24;
					a += pa;
					r += ((p >> 16) & 0xFF) * pa;
					g += ((p >> 8) & 0xFF) * pa;
					b += (p & 0xFF) * pa;
				}
			}
			uint32_t oa = a / n;
			uint32_t orr = a ? r / a : 0, og = a ? g / a : 0, ob = a ? b / a : 0;
			theDst[(size_t)y * theDstW + x] = (oa << 24) | (orr << 16) | (og << 8) | ob;
		}
	}
}

static bool LoadGameImageLayers(const std::string& theGamePath, LoadedImage& theOut, bool theLookForAlpha)
{
	std::string norm = VFS::Normalize(theGamePath);
	size_t slash = norm.find_last_of('/');
	std::string dir = slash == std::string::npos ? "" : norm.substr(0, slash);
	std::string file = slash == std::string::npos ? norm : norm.substr(slash + 1);
	std::string ext;
	size_t dot = file.find_last_of('.');
	if (dot != std::string::npos)
	{
		std::string e = LowerStr(file.substr(dot));
		for (const std::string& known : ImageExtensions())
		{
			if (e == known)
			{
				ext = file.substr(dot);
				file = file.substr(0, dot);
				break;
			}
		}
	}
	const std::string& stem = file;

	struct Layer
	{
#ifndef BYOPOP_DERIVED
		std::string mRoot, mModId;
	};
	std::vector<Layer> layers;
	for (const VFSRoot& r : VFS::Get().GetModRoots())
		layers.push_back({r.mPath, r.mModId});
	layers.push_back({VFS::Get().GetBaseDir(), ""});
#else
		std::string mRoot;
	};
	std::vector<Layer> layers;
	layers.push_back({VFS::Get().GetBaseDir()});
#endif

	for (size_t li = 0; li < layers.size(); li++)
	{
		const Layer& layer = layers[li];

		// A generated image stands between the mods and the game's files
		if ((li + 1 == layers.size()) && gImageGenerator)
		{
			LoadedImage generated;
			if (gImageGenerator(LowerStr(dir.empty() ? stem : dir + "/" + stem), generated) && !generated.mBits.empty())
			{
				if (generated.mHDBits.empty())
					generated.mHDScale = 1;
#ifndef BYOPOP_DERIVED
				generated.mModId.clear();
#endif
				generated.mGenerated = true;
				theOut = std::move(generated);
				return true;
			}
		}

		Candidate hd, one, maskHD, maskOne;
		FindInRoot(layer.mRoot, dir, stem, ext, hd, one);
		if (theLookForAlpha)
			FindMaskInRoot(layer.mRoot, dir, stem, maskHD, maskOne);
		if (hd.mPath.empty() && one.mPath.empty() && maskHD.mPath.empty() && maskOne.mPath.empty())
			continue;

#ifndef BYOPOP_DERIVED
		// An explicit "scale" from the mod manifest marks a plain-named file as HD.
		int explicitScale = 1;
		auto ov = gScaleOverrides.find(layer.mModId + "|" + LowerStr(dir.empty() ? stem : dir + "/" + stem));
		if (ov != gScaleOverrides.end())
			explicitScale = ov->second;
		else if (!layer.mModId.empty() && gModDefaultScales.count(layer.mModId))
			explicitScale = gModDefaultScales[layer.mModId];
		if (explicitScale > 1 && hd.mPath.empty() && !one.mPath.empty())
		{
			hd = one;
			hd.mScale = explicitScale;
			one = Candidate();
		}
#endif

		theOut = LoadedImage();
#ifndef BYOPOP_DERIVED
		theOut.mModId = layer.mModId;
#endif
		bool colourHasAlpha = false;

		// --- HD colour ---
		if (!hd.mPath.empty())
		{
			int w, h;
			bool a = false;
			if (DecodeFile(hd.mPath, w, h, theOut.mHDBits, &a))
			{
				theOut.mHDScale = hd.mScale;
				theOut.mHDWidth = w;
				theOut.mHDHeight = h;
				theOut.mSourcePath = hd.mPath;
				colourHasAlpha = a;
				if (!maskHD.mPath.empty())
				{
					int mw, mh;
					std::vector<uint32_t> mask;
					if (DecodeFile(maskHD.mPath, mw, mh, mask, nullptr) && mw == w && mh == h)
					{
						ApplyMask(theOut.mHDBits, mask);
						colourHasAlpha = true;
					}
				}
			}
		}

		// --- 1x colour ---
		if (!one.mPath.empty())
		{
			int w, h;
			bool a = false;
			// port: a GIF's indices are kept beside the pixels for Palletize (GIFIndices)
			if (DecodeFile(one.mPath, w, h, theOut.mBits, &a, &theOut.mGIFIndices))
			{
				theOut.mWidth = w;
				theOut.mHeight = h;
				if (theOut.mSourcePath.empty())
					theOut.mSourcePath = one.mPath;
				bool hasAlpha = a;
				if (!maskOne.mPath.empty())
				{
					int mw, mh;
					std::vector<uint32_t> mask;
					GIFIndices maskIndices;
					if ((!theOut.mGIFIndices.mIndices.empty()) && DecodeMaskAsIndices(maskOne.mPath, mw, mh, maskIndices) &&
						mw == w && mh == h)
					{
						// port: a GIF mask of a GIF: its alpha straight from its indices, as ApplyMask takes it from its pixels,
						// and the two pictures' indices kept together for Palletize
						uint32_t* aPixel = theOut.mBits.data();
						const uint8_t* aMaskIndex = maskIndices.mIndices.data();
						const uint32_t* aMaskColors = maskIndices.mColors.data();
						for (size_t i = 0; i < theOut.mBits.size(); i++)
							aPixel[i] = (aPixel[i] & 0x00FFFFFF) | ((aMaskColors[aMaskIndex[i]] & 0xFF) << 24);
						hasAlpha = true;
						theOut.mGIFIndices.mMaskIndices.swap(maskIndices.mIndices);
						theOut.mGIFIndices.mMaskColors.swap(maskIndices.mColors);
					}
					else if (DecodeFile(maskOne.mPath, mw, mh, mask, nullptr) && mw == w && mh == h)
					{
						ApplyMask(theOut.mBits, mask);
						hasAlpha = true;
						theOut.mGIFIndices = GIFIndices();  // the colour's indices alone no longer give the pixels
					}
				}
				colourHasAlpha = colourHasAlpha || hasAlpha;
				theOut.mHasAlpha = hasAlpha;
			}
		}
		else if (!theOut.mHDBits.empty())
		{
			BoxDownsample(theOut.mHDBits, theOut.mHDWidth, theOut.mHDHeight, theOut.mHDScale, theOut.mBits, theOut.mWidth, theOut.mHeight);
			theOut.mHasAlpha = colourHasAlpha;
		}

		// --- mask only: white image with alpha (gAlphaComposeColor). An HD mask is the texture; a 1x mask
		// next to it is the CPU copy (else the HD mask is downsampled for that) ---
		if (theOut.mBits.empty() && theOut.mHDBits.empty())
		{
			int w, h;
			std::vector<uint32_t> mask;
			if ((!maskHD.mPath.empty()) && DecodeFile(maskHD.mPath, w, h, mask, nullptr))
			{
				theOut.mSourcePath = maskHD.mPath;
				theOut.mHDScale = maskHD.mScale;
				theOut.mHDWidth = w;
				theOut.mHDHeight = h;
				theOut.mHDBits.resize(mask.size());
				for (size_t i = 0; i < mask.size(); i++)
					theOut.mHDBits[i] = ((mask[i] & 0xFF) << 24) | 0xFFFFFF;
			}
			if ((!maskOne.mPath.empty()) && DecodeFile(maskOne.mPath, w, h, mask, nullptr, &theOut.mGIFIndices))
			{
				if (theOut.mSourcePath.empty())
					theOut.mSourcePath = maskOne.mPath;
				theOut.mWidth = w;
				theOut.mHeight = h;
				theOut.mBits.resize(mask.size());
				for (size_t i = 0; i < mask.size(); i++)
					theOut.mBits[i] = ((mask[i] & 0xFF) << 24) | 0xFFFFFF;
				for (uint32_t& aColor : theOut.mGIFIndices.mColors)  // the indices' colours made white the same way
					aColor = ((aColor & 0xFF) << 24) | 0xFFFFFF;
			}
			else if (!theOut.mHDBits.empty())
				BoxDownsample(theOut.mHDBits, theOut.mHDWidth, theOut.mHDHeight, theOut.mHDScale, theOut.mBits, theOut.mWidth, theOut.mHeight);
			if (theOut.mBits.empty())
				continue;
			if (theOut.mHDBits.empty())
				theOut.mHDScale = 1;
			theOut.mHasAlpha = true;
			return true;
		}

		// --- borrow a lower layer's mask for an alpha-less same-size replacement ---
		if (theLookForAlpha && !colourHasAlpha && maskOne.mPath.empty() && maskHD.mPath.empty())
		{
			for (size_t lj = li + 1; lj < layers.size(); lj++)
			{
				Candidate lowHD, lowOne;
				FindMaskInRoot(layers[lj].mRoot, dir, stem, lowHD, lowOne);
				if (lowOne.mPath.empty())
					continue;
				int mw, mh;
				std::vector<uint32_t> mask;
				if (DecodeFile(lowOne.mPath, mw, mh, mask, nullptr) && mw == theOut.mWidth && mh == theOut.mHeight)
				{
					ApplyMask(theOut.mBits, mask);
					theOut.mHasAlpha = true;
					theOut.mGIFIndices = GIFIndices();  // a lower layer's mask is not in the indices
					if (!theOut.mHDBits.empty())
					{
						// scale the 1x mask up to the HD texture (nearest)
						int s = theOut.mHDScale;
						for (int y = 0; y < theOut.mHDHeight; y++)
							for (int x = 0; x < theOut.mHDWidth; x++)
							{
								uint32_t m = mask[(size_t)std::min(y / s, mh - 1) * mw + std::min(x / s, mw - 1)];
								uint32_t& p = theOut.mHDBits[(size_t)y * theOut.mHDWidth + x];
								p = (p & 0x00FFFFFF) | ((m & 0xFF) << 24);
							}
					}
				}
				break;
			}
		}

		if (theOut.mBits.empty())
			continue;
		if (theOut.mHDBits.empty())
			theOut.mHDScale = 1;
		return true;
	}
	return false;
}

bool LoadGameImage(const std::string& theGamePath, LoadedImage& theOut, bool theLookForAlpha)
{
#ifndef BYOPOP_DERIVED
	if (!LoadGameImageLayers(theGamePath, theOut, theLookForAlpha))
		return false;
	ApplyImagePatches(theGamePath, theOut);
	return true;
#else
	return LoadGameImageLayers(theGamePath, theOut, theLookForAlpha);
#endif
}

}
