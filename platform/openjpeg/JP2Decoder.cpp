#include "JP2Decoder.h"
#include "platform/ImageLoader.h"

#include <openjpeg.h>
#include <SDL.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace Platform
{

namespace
{

// --- the file in memory, as an OpenJPEG stream ---

struct MemoryStream
{
	const uint8_t* mData;
	size_t mSize;
	size_t mPos;
};

OPJ_SIZE_T StreamRead(void* theBuffer, OPJ_SIZE_T theCount, void* theUser)
{
	MemoryStream* aStream = (MemoryStream*)theUser;
	size_t aLeft = aStream->mSize - aStream->mPos;
	if (aLeft == 0)
		return (OPJ_SIZE_T)-1;  // end of stream
	size_t aCount = std::min((size_t)theCount, aLeft);
	memcpy(theBuffer, aStream->mData + aStream->mPos, aCount);
	aStream->mPos += aCount;
	return (OPJ_SIZE_T)aCount;
}

OPJ_OFF_T StreamSkip(OPJ_OFF_T theCount, void* theUser)
{
	MemoryStream* aStream = (MemoryStream*)theUser;
	int64_t aPos = (int64_t)aStream->mPos + (int64_t)theCount;
	if (aPos < 0 || aPos > (int64_t)aStream->mSize)
	{
		aStream->mPos = aPos < 0 ? 0 : aStream->mSize;
		return (OPJ_OFF_T)-1;
	}
	aStream->mPos = (size_t)aPos;
	return theCount;
}

OPJ_BOOL StreamSeek(OPJ_OFF_T theOffset, void* theUser)
{
	MemoryStream* aStream = (MemoryStream*)theUser;
	if (theOffset < 0 || (uint64_t)theOffset > (uint64_t)aStream->mSize)
		return OPJ_FALSE;
	aStream->mPos = (size_t)theOffset;
	return OPJ_TRUE;
}

// --- OpenJPEG's messages: info and warnings dropped, errors kept for one log line per file ---

void SilentHandler(const char*, void*) {}

void ErrorHandler(const char* theMessage, void* theUser)
{
	std::string* anErrors = (std::string*)theUser;
	std::string aMessage = theMessage ? theMessage : "";
	while (!aMessage.empty() && (aMessage.back() == '\n' || aMessage.back() == '\r' || aMessage.back() == ' '))
		aMessage.pop_back();
	if (aMessage.empty())
		return;
	if (!anErrors->empty())
		*anErrors += "; ";
	*anErrors += aMessage;
}

struct StreamDeleter { void operator()(opj_stream_t* theStream) const { opj_stream_destroy(theStream); } };
struct CodecDeleter { void operator()(opj_codec_t* theCodec) const { opj_destroy_codec(theCodec); } };
struct ImageDeleter { void operator()(opj_image_t* theImage) const { opj_image_destroy(theImage); } };

bool IsJP2(const std::vector<uint8_t>& theData)
{
	static const uint8_t kSignatureBox[8] = {0x00, 0x00, 0x00, 0x0C, 0x6A, 0x50, 0x20, 0x20};  // length 12, "jP  "
	static const uint8_t kCodestream[4] = {0xFF, 0x4F, 0xFF, 0x51};                            // SOC, SIZ
	return theData.size() >= 12 && (memcmp(theData.data(), kSignatureBox, 8) == 0 || memcmp(theData.data(), kCodestream, 4) == 0);
}

// --- one component's samples as 8-bit values ---

struct Component
{
	const opj_image_comp_t* mComp;
	int64_t mOffset;  // added to signed samples
	int64_t mMax;     // largest value at this precision
	int mShift;       // prec - 8
	int64_t mX0, mY0;
};

// The component's sample at reference-grid pixel (theX, theY), counted from the image's corner, scaled to 0..255:
// signed samples are first shifted up by half their range; other precisions are rounded down to 8 bits or
// stretched up to them. A subsampled component (dx, dy) is read at x/dx, y/dy.
inline int Sample(const Component& theComp, const opj_image_t* theImage, int theX, int theY)
{
	const opj_image_comp_t* c = theComp.mComp;
	int64_t cx = ((int64_t)theImage->x0 + theX) / (int64_t)c->dx - theComp.mX0;
	int64_t cy = ((int64_t)theImage->y0 + theY) / (int64_t)c->dy - theComp.mY0;
	cx = std::min<int64_t>(std::max<int64_t>(cx, 0), (int64_t)c->w - 1);
	cy = std::min<int64_t>(std::max<int64_t>(cy, 0), (int64_t)c->h - 1);
	int64_t v = (int64_t)c->data[(size_t)cy * c->w + (size_t)cx] + theComp.mOffset;
	v = std::min<int64_t>(std::max<int64_t>(v, 0), theComp.mMax);
	if (theComp.mShift > 0)
		return (int)std::min<int64_t>((v + ((int64_t)1 << (theComp.mShift - 1))) >> theComp.mShift, 255);
	if (theComp.mShift < 0)
		return (int)((v * 255 + theComp.mMax / 2) / theComp.mMax);
	return (int)v;
}

inline uint8_t Clamp8(int theValue)
{
	return (uint8_t)(theValue < 0 ? 0 : theValue > 255 ? 255 : theValue);
}

// The pixels of a decoded image: 1 component is grey, 2 grey and alpha, 3 RGB, 4 RGBA (sYCC colour is converted
// to RGB, as OpenJPEG's own tools and Pillow do). Components may be subsampled and of any precision.
bool ConvertPixels(const opj_image_t* theImage, int& theWidth, int& theHeight, std::vector<uint32_t>& theArgb, bool* theHasAlpha,
	std::string& theError)
{
	if (theImage->x1 <= theImage->x0 || theImage->y1 <= theImage->y0)
	{
		theError = "empty image";
		return false;
	}
	uint64_t aWidth = (uint64_t)theImage->x1 - theImage->x0;
	uint64_t aHeight = (uint64_t)theImage->y1 - theImage->y0;
	if (aWidth > 32768 || aHeight > 32768 || aWidth * aHeight > ((uint64_t)1 << 27))
	{
		theError = "image too large (" + std::to_string(aWidth) + "x" + std::to_string(aHeight) + ")";
		return false;
	}
	uint32_t aCount = theImage->numcomps;
	if (aCount < 1 || aCount > 4 || theImage->comps == nullptr)
	{
		theError = std::to_string(aCount) + " components are not supported";
		return false;
	}
	if (theImage->color_space == OPJ_CLRSPC_CMYK)
	{
		theError = "CMYK images are not supported";
		return false;
	}

	Component aComps[4];
	for (uint32_t i = 0; i < aCount; i++)
	{
		const opj_image_comp_t* c = &theImage->comps[i];
		if (c->data == nullptr || c->w == 0 || c->h == 0 || c->dx == 0 || c->dy == 0 || c->prec == 0 || c->prec > 31)
		{
			theError = "component " + std::to_string(i) + " is unusable";
			return false;
		}
		Component& k = aComps[i];
		k.mComp = c;
		k.mOffset = c->sgnd ? ((int64_t)1 << (c->prec - 1)) : 0;
		k.mMax = ((int64_t)1 << c->prec) - 1;
		k.mShift = (int)c->prec - 8;
		k.mX0 = c->x0;
		k.mY0 = c->y0;
	}

	int w = (int)aWidth, h = (int)aHeight;
	std::vector<uint32_t> aBits((size_t)w * h);
	// colour that is sYCC by its colour space, or by its chroma being subsampled (a codestream says nothing else;
	// OpenJPEG's opj_decompress and Pillow decide the same way)
	bool aYCC = aCount == 3 && (theImage->color_space == OPJ_CLRSPC_SYCC ||
		(theImage->comps[0].dx == theImage->comps[0].dy && theImage->comps[1].dx != 1));
	bool anAlpha = false;
	for (int y = 0; y < h; y++)
	{
		uint32_t* aRow = &aBits[(size_t)y * w];
		for (int x = 0; x < w; x++)
		{
			int r, g, b, a = 255;
			if (aCount <= 2)
			{
				r = g = b = Sample(aComps[0], theImage, x, y);
				if (aCount == 2)
					a = Sample(aComps[1], theImage, x, y);
			}
			else
			{
				r = Sample(aComps[0], theImage, x, y);
				g = Sample(aComps[1], theImage, x, y);
				b = Sample(aComps[2], theImage, x, y);
				if (aCount == 4)
					a = Sample(aComps[3], theImage, x, y);
				if (aYCC)
				{
					int aY = r, aCb = g - 128, aCr = b - 128;
					r = aY + (int)(1.402 * aCr);
					g = aY - (int)(0.344 * aCb + 0.714 * aCr);
					b = aY + (int)(1.772 * aCb);
				}
			}
			if (a != 255)
				anAlpha = true;
			aRow[x] = ((uint32_t)Clamp8(a) << 24) | ((uint32_t)Clamp8(r) << 16) | ((uint32_t)Clamp8(g) << 8) | Clamp8(b);
		}
	}
	theWidth = w;
	theHeight = h;
	theArgb.swap(aBits);
	if (theHasAlpha)
		*theHasAlpha = anAlpha;
	return true;
}

bool Decode(const std::vector<uint8_t>& theData, int& theWidth, int& theHeight, std::vector<uint32_t>& theArgb, bool* theHasAlpha,
	std::string& theErrors)
{
	bool aRawCodestream = theData.size() >= 4 && theData[0] == 0xFF;
	std::unique_ptr<opj_codec_t, CodecDeleter> aCodec(opj_create_decompress(aRawCodestream ? OPJ_CODEC_J2K : OPJ_CODEC_JP2));
	if (!aCodec)
	{
		theErrors = "no decoder";
		return false;
	}
	opj_set_info_handler(aCodec.get(), SilentHandler, nullptr);
	opj_set_warning_handler(aCodec.get(), SilentHandler, nullptr);
	opj_set_error_handler(aCodec.get(), ErrorHandler, &theErrors);

	opj_dparameters_t aParams;
	opj_set_default_decoder_parameters(&aParams);
	if (!opj_setup_decoder(aCodec.get(), &aParams))
	{
		if (theErrors.empty())
			theErrors = "cannot set up the decoder";
		return false;
	}

	MemoryStream aMemory = {theData.data(), theData.size(), 0};
	std::unique_ptr<opj_stream_t, StreamDeleter> aStream(
		opj_stream_create((OPJ_SIZE_T)std::min<size_t>(std::max<size_t>(theData.size(), 4096), 0x100000), OPJ_TRUE));
	if (!aStream)
	{
		theErrors = "out of memory";
		return false;
	}
	opj_stream_set_read_function(aStream.get(), StreamRead);
	opj_stream_set_skip_function(aStream.get(), StreamSkip);
	opj_stream_set_seek_function(aStream.get(), StreamSeek);
	opj_stream_set_user_data(aStream.get(), &aMemory, nullptr);
	opj_stream_set_user_data_length(aStream.get(), (OPJ_UINT64)theData.size());

	// opj_read_header may leave an image behind when it fails: it is destroyed either way
	opj_image_t* aRawImage = nullptr;
	bool aHeaderOk = opj_read_header(aStream.get(), aCodec.get(), &aRawImage) != 0;
	std::unique_ptr<opj_image_t, ImageDeleter> anImage(aRawImage);
	if (!aHeaderOk || !anImage)
	{
		if (theErrors.empty())
			theErrors = "cannot read the header";
		return false;
	}
	if (!opj_decode(aCodec.get(), aStream.get(), anImage.get()))
	{
		if (theErrors.empty())
			theErrors = "cannot decode the image";
		return false;
	}
	opj_end_decompress(aCodec.get(), aStream.get());
	return ConvertPixels(anImage.get(), theWidth, theHeight, theArgb, theHasAlpha, theErrors);
}

// The ImageDecoder registered with Platform::RegisterImageFormat: one log line per file that cannot be decoded.
bool DecodeJP2(const std::vector<uint8_t>& theData, int& theWidth, int& theHeight, std::vector<uint32_t>& theArgb, bool* theHasAlpha)
{
	std::string anErrors;
	if (Decode(theData, theWidth, theHeight, theArgb, theHasAlpha, anErrors))
		return true;
	SDL_Log("jp2: cannot decode a %u-byte file: %s", (unsigned)theData.size(), anErrors.c_str());
	return false;
}

}

void RegisterOpenJPEGImages()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	RegisterImageFormat(".jp2", IsJP2, DecodeJP2);
	RegisterImageFormat(".j2k", IsJP2, DecodeJP2);
}

}
