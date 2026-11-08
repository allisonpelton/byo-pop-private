// JPEG decoding through IJG libjpeg 6b (third_party/libjpeg): the library the games' own exes decode their JPEGs with,
// so the pixels are the originals' (stb_image's decoder rounds some pixels differently, by up to 3 levels on about 1% of
// them). The calls and settings are those of PopCap's ImageLib (GetJPEGImage in SexyAppFramework's ImageLib.cpp):
// jpeg_create_decompress, jpeg_read_header(TRUE), jpeg_start_decompress with nothing set in between, so the defaults
// apply: the accurate integer IDCT (JDCT_ISLOW), fancy (triangle-filter) upsampling of the chroma, block smoothing
// for progressive files, YCbCr to RGB by libjpeg's own tables, and grey files as one component. The data comes from
// memory (a source manager like ImageLib's file one: when the data ends early it supplies an EOI marker, so a cut-off
// file decodes as far as it goes), and the library's warnings are dropped instead of printed.
//
// This file is the only one that includes libjpeg's headers: they define `boolean`, which Windows' headers define
// too, so no game code should include them with this file's headers.

#include "ImageLoader.h"

#include <stb_image.h>
#include <csetjmp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "jpeglib.h"
}

namespace Platform
{

namespace
{

struct JpegErrorManager
{
	jpeg_error_mgr mPub;  // first: libjpeg's err pointer is cast back to this
	jmp_buf mJump;
};

struct JpegMemorySource
{
	jpeg_source_mgr mPub;  // first, likewise
	JOCTET mEndOfImage[2];  // the EOI marker fill_input_buffer hands out when the data has run out
};

void ErrorExit(j_common_ptr theInfo)
{
	longjmp(((JpegErrorManager*)theInfo->err)->mJump, 1);
}

void OutputMessage(j_common_ptr)
{
	// warnings (corrupt data, missing EOI) are not printed; the decoder carries on as it does in the games
}

void InitSource(j_decompress_ptr)
{
}

boolean FillInputBuffer(j_decompress_ptr theInfo)
{
	// Only reached when the data ends before an EOI marker: libjpeg's own jdatasrc.c, and PopCap's pak reader, give
	// it a fake EOI (with a warning) so the image is finished from what was read.
	JpegMemorySource* aSource = (JpegMemorySource*)theInfo->src;
	aSource->mEndOfImage[0] = (JOCTET)0xFF;
	aSource->mEndOfImage[1] = (JOCTET)JPEG_EOI;
	aSource->mPub.next_input_byte = aSource->mEndOfImage;
	aSource->mPub.bytes_in_buffer = 2;
	return TRUE;
}

void SkipInputData(j_decompress_ptr theInfo, long theCount)
{
	if (theCount <= 0)
		return;
	JpegMemorySource* aSource = (JpegMemorySource*)theInfo->src;
	while (theCount > (long)aSource->mPub.bytes_in_buffer)
	{
		theCount -= (long)aSource->mPub.bytes_in_buffer;
		FillInputBuffer(theInfo);
	}
	aSource->mPub.next_input_byte += theCount;
	aSource->mPub.bytes_in_buffer -= (size_t)theCount;
}

void TermSource(j_decompress_ptr)
{
}

// Decodes into a malloc'd buffer of theWidth * theHeight * theComponents bytes (RGB or grey), or returns null. All
// state that must survive a longjmp lives in plain structs or volatile pointers here (no C++ objects).
uint8_t* DecodeCore(const uint8_t* theData, size_t theSize, int* theWidth, int* theHeight, int* theComponents)
{
	jpeg_decompress_struct aInfo;
	JpegErrorManager anError;
	JpegMemorySource aSource;
	uint8_t* volatile aPixels = nullptr;

	memset(&aInfo, 0, sizeof(aInfo));
	aInfo.err = jpeg_std_error(&anError.mPub);
	anError.mPub.error_exit = ErrorExit;
	anError.mPub.output_message = OutputMessage;

	if (setjmp(anError.mJump))
	{
		jpeg_destroy_decompress(&aInfo);
		free(aPixels);
		return nullptr;
	}

	jpeg_create_decompress(&aInfo);
	aSource.mPub.init_source = InitSource;
	aSource.mPub.fill_input_buffer = FillInputBuffer;
	aSource.mPub.skip_input_data = SkipInputData;
	aSource.mPub.resync_to_restart = jpeg_resync_to_restart;
	aSource.mPub.term_source = TermSource;
	aSource.mPub.bytes_in_buffer = theSize;
	aSource.mPub.next_input_byte = theData;
	aInfo.src = &aSource.mPub;

	jpeg_read_header(&aInfo, TRUE);
	jpeg_start_decompress(&aInfo);

	const int aComponents = aInfo.output_components;
	if ((aComponents != 1 && aComponents != 3) || aInfo.output_width == 0 || aInfo.output_height == 0 ||
		(uint64_t)aInfo.output_width * aInfo.output_height * aComponents > 0x20000000ull)
	{
		// CMYK/YCCK files (four components) are not what the games' ImageLib could show either
		jpeg_destroy_decompress(&aInfo);
		return nullptr;
	}

	const size_t aStride = (size_t)aInfo.output_width * aComponents;
	aPixels = (uint8_t*)malloc(aStride * aInfo.output_height);
	if (aPixels == nullptr)
	{
		jpeg_destroy_decompress(&aInfo);
		return nullptr;
	}

	while (aInfo.output_scanline < aInfo.output_height)
	{
		JSAMPROW aRow = aPixels + (size_t)aInfo.output_scanline * aStride;
		jpeg_read_scanlines(&aInfo, &aRow, 1);
	}

	*theWidth = (int)aInfo.output_width;
	*theHeight = (int)aInfo.output_height;
	*theComponents = aComponents;

	jpeg_finish_decompress(&aInfo);
	jpeg_destroy_decompress(&aInfo);
	return aPixels;
}

}  // namespace

bool IsJPEG(const uint8_t* theData, size_t theSize)
{
	return theData != nullptr && theSize >= 3 && theData[0] == 0xFF && theData[1] == 0xD8 && theData[2] == 0xFF;
}

bool DecodeJPEG(const uint8_t* theData, size_t theSize, int& theWidth, int& theHeight, int& theComponents,
	std::vector<uint8_t>& thePixels)
{
	if (!IsJPEG(theData, theSize))
		return false;

	int aWidth = 0, aHeight = 0, aComponents = 0;
	uint8_t* aPixels = DecodeCore(theData, theSize, &aWidth, &aHeight, &aComponents);
	if (aPixels == nullptr)
		return false;

	thePixels.assign(aPixels, aPixels + (size_t)aWidth * aHeight * aComponents);
	free(aPixels);
	theWidth = aWidth;
	theHeight = aHeight;
	theComponents = aComponents;
	return true;
}

uint8_t* LoadImageFromMemory(const uint8_t* theData, int theSize, int* theWidth, int* theHeight, int* theComponents,
	int theRequired)
{
	int aWidth = 0, aHeight = 0, aComponents = 0;
	uint8_t* aPixels = IsJPEG(theData, theSize > 0 ? (size_t)theSize : 0)
		? DecodeCore(theData, (size_t)theSize, &aWidth, &aHeight, &aComponents)
		: nullptr;
	if (aPixels == nullptr)
		return stbi_load_from_memory(theData, theSize, theWidth, theHeight, theComponents, theRequired);

	if (theWidth)
		*theWidth = aWidth;
	if (theHeight)
		*theHeight = aHeight;
	if (theComponents)
		*theComponents = aComponents;
	if (theRequired == 0 || theRequired == aComponents)
		return aPixels;
	if (theRequired < 1 || theRequired > 4)
	{
		free(aPixels);
		return nullptr;
	}

	// the conversions stb_image makes: grey to RGB repeats the value, a new alpha channel is 255, RGB to grey is
	// (77 r + 150 g + 29 b) / 256
	const size_t aCount = (size_t)aWidth * aHeight;
	uint8_t* aOut = (uint8_t*)malloc(aCount * theRequired);
	if (aOut == nullptr)
	{
		free(aPixels);
		return nullptr;
	}
	for (size_t i = 0; i < aCount; i++)
	{
		const uint8_t* s = aPixels + i * aComponents;
		uint8_t* d = aOut + i * theRequired;
		const uint8_t r = s[0];
		const uint8_t g = aComponents == 3 ? s[1] : s[0];
		const uint8_t b = aComponents == 3 ? s[2] : s[0];
		if (theRequired >= 3)
		{
			d[0] = r;
			d[1] = g;
			d[2] = b;
			if (theRequired == 4)
				d[3] = 255;
		}
		else
		{
			d[0] = aComponents == 3 ? (uint8_t)((r * 77 + g * 150 + 29 * b) >> 8) : r;
			if (theRequired == 2)
				d[1] = 255;
		}
	}
	free(aPixels);
	return aOut;
}

}  // namespace Platform
