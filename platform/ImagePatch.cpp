#include "ImageLoader.h"
#include "VFS.h"

#include <SDL.h>
#include <algorithm>
#include <cstdio>

namespace Platform
{

struct ImagePatch
{
	std::string mTargetKey;
	std::string mPatchPath;
	int mX;
	int mY;
	int mScale;
	bool mReplace;
};

static std::vector<ImagePatch> gPatches;

// Normalized, lower-case, extensionless key so "images\\GemSheet6.png" matches "images/gemsheet6".
static std::string PatchKey(const std::string& thePath)
{
	std::string p = VFS::Normalize(thePath);
	std::transform(p.begin(), p.end(), p.begin(), ::tolower);
	size_t dot = p.find_last_of('.');
	size_t slash = p.find_last_of('/');
	if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
		p = p.substr(0, dot);
	return p;
}

void AddImagePatch(const std::string& theTarget, const std::string& thePatchPath, int theX, int theY, int theScale, bool theReplace)
{
	gPatches.push_back({PatchKey(theTarget), thePatchPath, theX, theY, std::max(1, theScale), theReplace});
}

void ClearImagePatches()
{
	gPatches.clear();
}

static inline uint32_t Premul(uint32_t p)
{
	uint32_t a = p >> 24;
	uint32_t r = ((p >> 16) & 0xFF) * a / 255, g = ((p >> 8) & 0xFF) * a / 255, b = (p & 0xFF) * a / 255;
	return (a << 24) | (r << 16) | (g << 8) | b;
}

static inline uint32_t Unpremul(uint32_t a, uint32_t r, uint32_t g, uint32_t b)
{
	if (!a)
		return 0;
	r = std::min<uint32_t>(255, r * 255 / a);
	g = std::min<uint32_t>(255, g * 255 / a);
	b = std::min<uint32_t>(255, b * 255 / a);
	return (a << 24) | (r << 16) | (g << 8) | b;
}

// Resamples ARGB pixels by an integer ratio: up = bilinear (premultiplied), down = box filter.
static std::vector<uint32_t> Resample(const std::vector<uint32_t>& theSrc, int theW, int theH, int theFrom, int theTo, int& theOutW, int& theOutH)
{
	if (theFrom == theTo)
	{
		theOutW = theW;
		theOutH = theH;
		return theSrc;
	}
	theOutW = theW * theTo / theFrom;
	theOutH = theH * theTo / theFrom;
	std::vector<uint32_t> out((size_t)theOutW * theOutH);
	if (theTo < theFrom)
	{
		int k = theFrom / theTo;
		for (int y = 0; y < theOutH; y++)
			for (int x = 0; x < theOutW; x++)
			{
				uint32_t a = 0, r = 0, g = 0, b = 0;
				for (int sy = 0; sy < k; sy++)
					for (int sx = 0; sx < k; sx++)
					{
						uint32_t p = Premul(theSrc[(size_t)(y * k + sy) * theW + x * k + sx]);
						a += p >> 24;
						r += (p >> 16) & 0xFF;
						g += (p >> 8) & 0xFF;
						b += p & 0xFF;
					}
				int n = k * k;
				out[(size_t)y * theOutW + x] = Unpremul(a / n, r / n, g / n, b / n);
			}
		return out;
	}
	double ratio = (double)theFrom / theTo;
	for (int y = 0; y < theOutH; y++)
	{
		double sy = (y + 0.5) * ratio - 0.5;
		int y0 = std::max(0, (int)sy), y1 = std::min(theH - 1, y0 + 1);
		double fy = std::max(0.0, sy - y0);
		for (int x = 0; x < theOutW; x++)
		{
			double sx = (x + 0.5) * ratio - 0.5;
			int x0 = std::max(0, (int)sx), x1 = std::min(theW - 1, x0 + 1);
			double fx = std::max(0.0, sx - x0);
			double acc[4] = {0, 0, 0, 0};
			const int xs[2] = {x0, x1}, ys[2] = {y0, y1};
			const double wx[2] = {1 - fx, fx}, wy[2] = {1 - fy, fy};
			for (int j = 0; j < 2; j++)
				for (int i = 0; i < 2; i++)
				{
					uint32_t p = Premul(theSrc[(size_t)ys[j] * theW + xs[i]]);
					double w = wx[i] * wy[j];
					acc[0] += (p >> 24) * w;
					acc[1] += ((p >> 16) & 0xFF) * w;
					acc[2] += ((p >> 8) & 0xFF) * w;
					acc[3] += (p & 0xFF) * w;
				}
			out[(size_t)y * theOutW + x] = Unpremul((uint32_t)(acc[0] + 0.5), (uint32_t)(acc[1] + 0.5), (uint32_t)(acc[2] + 0.5), (uint32_t)(acc[3] + 0.5));
		}
	}
	return out;
}

static void Paste(std::vector<uint32_t>& theDst, int theDstW, int theDstH, const std::vector<uint32_t>& theSrc, int theSrcW, int theSrcH, int theX, int theY, bool theReplace)
{
	for (int y = 0; y < theSrcH; y++)
	{
		int dy = theY + y;
		if (dy < 0 || dy >= theDstH)
			continue;
		for (int x = 0; x < theSrcW; x++)
		{
			int dx = theX + x;
			if (dx < 0 || dx >= theDstW)
				continue;
			uint32_t s = theSrc[(size_t)y * theSrcW + x];
			uint32_t& d = theDst[(size_t)dy * theDstW + dx];
			if (theReplace)
			{
				d = s;
				continue;
			}
			// source-over in premultiplied space
			uint32_t ps = Premul(s), pd = Premul(d);
			uint32_t sa = ps >> 24, inv = 255 - sa;
			uint32_t a = sa + (pd >> 24) * inv / 255;
			uint32_t r = ((ps >> 16) & 0xFF) + ((pd >> 16) & 0xFF) * inv / 255;
			uint32_t g = ((ps >> 8) & 0xFF) + ((pd >> 8) & 0xFF) * inv / 255;
			uint32_t b = (ps & 0xFF) + (pd & 0xFF) * inv / 255;
			d = Unpremul(a, r, g, b);
		}
	}
}

void ApplyImagePatches(const std::string& theGamePath, LoadedImage& theImage)
{
	if (gPatches.empty())
		return;
	std::string key = PatchKey(theGamePath);
	for (const ImagePatch& patch : gPatches)
	{
		if (patch.mTargetKey != key)
			continue;
		std::vector<uint8_t> file;
		FILE* f = fopen(patch.mPatchPath.c_str(), "rb");
		if (f)
		{
			fseek(f, 0, SEEK_END);
			long len = ftell(f);
			fseek(f, 0, SEEK_SET);
			file.resize(len > 0 ? (size_t)len : 0);
			if (len <= 0 || fread(file.data(), 1, file.size(), f) != file.size())
				file.clear();
			fclose(f);
		}
		int pw = 0, ph = 0;
		std::vector<uint32_t> pbits;
		if (file.empty() || !DecodeImage(file, pw, ph, pbits, nullptr))
		{
			SDL_Log("image patch: cannot read %s", patch.mPatchPath.c_str());
			continue;
		}

		// Promote the image to the patch's density if the patch is sharper than what we have.
		int scale = theImage.mHDBits.empty() ? 1 : theImage.mHDScale;
		if (patch.mScale > scale)
		{
			const std::vector<uint32_t>& src = theImage.mHDBits.empty() ? theImage.mBits : theImage.mHDBits;
			int sw = theImage.mHDBits.empty() ? theImage.mWidth : theImage.mHDWidth;
			int sh = theImage.mHDBits.empty() ? theImage.mHeight : theImage.mHDHeight;
			int nw, nh;
			std::vector<uint32_t> up = Resample(src, sw, sh, scale, patch.mScale, nw, nh);
			theImage.mHDBits.swap(up);
			theImage.mHDWidth = nw;
			theImage.mHDHeight = nh;
			theImage.mHDScale = scale = patch.mScale;
		}

		// HD pixels
		if (!theImage.mHDBits.empty())
		{
			int rw, rh;
			std::vector<uint32_t> hd = Resample(pbits, pw, ph, patch.mScale, scale, rw, rh);
			Paste(theImage.mHDBits, theImage.mHDWidth, theImage.mHDHeight, hd, rw, rh, patch.mX * scale, patch.mY * scale, patch.mReplace);
		}
		// 1x pixels (CPU-side copy used by software drawing, hit tests and the non-HD texture)
		int lw, lh;
		std::vector<uint32_t> one = Resample(pbits, pw, ph, patch.mScale, 1, lw, lh);
		Paste(theImage.mBits, theImage.mWidth, theImage.mHeight, one, lw, lh, patch.mX, patch.mY, patch.mReplace);
		theImage.mHasAlpha = true;
	}
}

}
