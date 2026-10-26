#include "R64Bank.h"
#include "R64Sys.h"
#include "R64Xml.h"

#include <cstdlib>
#include <cstring>

namespace r64
{

namespace
{

inline uint32_t Align4(uint32_t theValue)
{
	uint32_t aRem = theValue % 4;
	if (aRem != 0)
		theValue = theValue + (4 - aRem);
	return theValue;
}

// strcpy into a fixed buffer, cut to fit (the original copies without a bound)
void CopyName(char* theDst, size_t theSize, const char* theSrc)
{
	size_t n = std::strlen(theSrc);
	if (n >= theSize)
		n = theSize - 1;
	std::memcpy(theDst, theSrc, n);
	theDst[n] = '\0';
}

// Each 8-bit image is its own mask (Bank_Load and Bank_Reconvert)
void SelfMask8(Bank* theBank)
{
	for (Image* anImage : theBank->images)
	{
		if (anImage->fmt.bpp == 8)
		{
			anImage->hasMask = 1;
			anImage->defMask = anImage;
			anImage->mask = anImage->defMask;
		}
	}
}

} // namespace

// 0x401a80
int PixelFormat_PackPalette32(const PixelFormat* theFmt, uint32_t* dst, const uint8_t* rgb, int n)
{
	if (dst != nullptr && rgb != nullptr && -1 < n)
	{
		int i = 0;
		if (0 < n)
		{
			const uint8_t* p = rgb + 1;
			do
			{
				i++;
				dst[i - 1] = (uint32_t)(p[-1] >> (theFmt->rLoss & 0x1f)) << (theFmt->rShift & 0x1f) |
					(uint32_t)(p[1] >> (theFmt->bLoss & 0x1f)) << (theFmt->bShift & 0x1f) |
					(uint32_t)(*p >> (theFmt->gLoss & 0x1f)) << (theFmt->gShift & 0x1f);
				p += 4;
			} while (i < n);
		}
		return 1;
	}
	return 0;
}

// 0x401b10
int PixelFormat_PackPalette16(const PixelFormat* theFmt, uint16_t* dst, const uint8_t* rgb, int n)
{
	if (dst != nullptr && rgb != nullptr && -1 < n)
	{
		int i = 0;
		if (0 < n)
		{
			const uint8_t* p = rgb + 1;
			do
			{
				i++;
				dst[i - 1] = (uint16_t)((uint32_t)(p[-1] >> (theFmt->rLoss & 0x1f)) << (theFmt->rShift & 0x1f) |
					(uint32_t)(p[1] >> (theFmt->bLoss & 0x1f)) << (theFmt->bShift & 0x1f) |
					(uint32_t)(*p >> (theFmt->gLoss & 0x1f)) << (theFmt->gShift & 0x1f));
				p += 4;
			} while (i < n);
		}
		return 1;
	}
	return 0;
}

// 0x427790
Bank::Bank()
{
	std::memset(dataPath, 0, sizeof(dataPath));
	Bank_Reset(this);
}

// 0x427560
Bank::~Bank()
{
	Bank_Clear(this);
}

// 0x4261b0: forgets everything without freeing it
void Bank_Reset(Bank* theBank)
{
	theBank->mode = 0;
	theBank->images.clear();
	theBank->rawPalettes.clear();
	theBank->palettesBuilt = 0;
	theBank->palettes.clear();
	theBank->dataSize = 0;
	theBank->data = nullptr;
}

// 0x426250
void Bank_Clear(Bank* theBank)
{
	for (Image* anImage : theBank->images)
	{
		if (anImage != nullptr)
		{
			Image_Destroy(anImage);
			delete anImage;
		}
	}
	theBank->images.clear();
	for (uint8_t* aPalette : theBank->rawPalettes)
		std::free(aPalette);
	theBank->rawPalettes.clear();
	for (Palette* aPalette : theBank->palettes)
		delete aPalette;
	theBank->palettes.clear();
	if (theBank->data != nullptr)
		std::free(theBank->data);
	Bank_Reset(theBank);
}

// 0x425de0: grows the data to theSize and clears it (1 even when malloc fails, as in the original)
int Bank_Alloc(Bank* theBank, uint32_t theSize)
{
	if (theBank->dataSize < theSize)
	{
		if (theBank->data != nullptr)
			std::free(theBank->data);
		theBank->data = (uint8_t*)std::malloc(theSize);
		if (theBank->data == nullptr)
			return 1;
		theBank->dataSize = theSize;
	}
	std::memset(theBank->data, 0, theBank->dataSize);
	return 1;
}

// 0x425eb0
Image* Bank_ImageAt(const Bank* theBank, uint32_t theIndex)
{
	if (theIndex < theBank->images.size())
		return theBank->images[theIndex];
	return nullptr;
}

// 0x425ed0
int Bank_Count(const Bank* theBank)
{
	return (int)theBank->images.size();
}

// 0x426430 (a negative index finds nothing)
uint8_t* Bank_RawPaletteAt(const Bank* theBank, uint32_t theIndex)
{
	if ((int)theIndex < 0 || theBank->rawPalettes.size() <= theIndex)
		return nullptr;
	return theBank->rawPalettes[theIndex];
}

// 0x426480
Palette* Bank_PaletteAt(const Bank* theBank, uint32_t theIndex)
{
	if ((int)theIndex < 0 || theBank->palettes.size() <= theIndex)
		return nullptr;
	return theBank->palettes[theIndex];
}

// 0x4264d0: the file as it is; each image's bits follow the last one's (pitch x height)
int Bank_LoadRaw(Bank* theBank, const char* thePath)
{
	std::vector<uint8_t> aFile;
	if (!Sys_ReadFile(thePath, aFile))
		return 0;
	if (Bank_Alloc(theBank, (uint32_t)aFile.size()) != 0 && theBank->dataSize != 0 &&
		theBank->dataSize <= aFile.size()) // fread(data, dataSize, 1) == 1
	{
		std::memcpy(theBank->data, aFile.data(), theBank->dataSize);
		uint8_t* aCursor = theBank->data;
		for (Image* anImage : theBank->images)
		{
			anImage->bits = aCursor;
			aCursor = aCursor + anImage->pitch * anImage->h;
		}
		return 1;
	}
	return 0;
}

// 0x4265d0: each image converted to theFmt, and for a screen deeper than 8 bits a mask beside it: an 8-bit image's
// pixels that are not 0, or a 32-bit image's alpha. Images named mask_* stay as they are. The original reads the
// file one image at a time; a size of 0 fails there (fread returns 0), and does here.
int Bank_LoadConverted(Bank* theBank, const char* thePath, const PixelFormat* theFmt)
{
	uint32_t aBpp = (uint32_t)theFmt->bpp;
	uint32_t aTotal = 0;
	for (Image* anImage : theBank->images)
	{
		uint32_t aPitch;
		if (anImage->isMask == 1)
			aPitch = (uint32_t)anImage->bankPitch;
		else
		{
			aPitch = (uint32_t)anImage->w * (aBpp >> 3);
			if (8 < (uint32_t)theFmt->bpp)
			{
				if (anImage->bankFmt.bpp == 8)
					aTotal = aTotal + (uint32_t)anImage->bankDataSize;
				if (8 < (uint32_t)theFmt->bpp && anImage->bankFmt.bpp == 0x20)
					aTotal = aTotal + (uint32_t)anImage->h * Align4((uint32_t)anImage->w);
			}
		}
		aPitch = Align4(aPitch);
		aTotal = aTotal + (uint32_t)anImage->h * aPitch;
	}
	if (theBank->dataSize < aTotal)
	{
		if (theBank->data != nullptr)
			std::free(theBank->data);
		theBank->data = (uint8_t*)std::malloc(aTotal);
		if (theBank->data == nullptr)
			return 0; // the original goes on into a null buffer
		theBank->dataSize = aTotal;
	}
	std::memset(theBank->data, 0, theBank->dataSize);

	std::vector<uint8_t> aFile;
	if (!Sys_ReadFile(thePath, aFile))
		return 0;
	uint8_t* aCursor = theBank->data;
	size_t aRead = 0;
	for (Image* anImage : theBank->images)
	{
		uint32_t aChunk = (uint32_t)anImage->bankDataSize;
		if (aChunk == 0 || aFile.size() - aRead < aChunk)
			return 0;
		const uint8_t* aSrc = aFile.data() + aRead;
		aRead += aChunk;
		uint32_t aSize;
		if (anImage->isMask == 1)
		{
			aSize = aChunk;
			std::memcpy(aCursor, aSrc, aSize);
			anImage->bits = aCursor;
		}
		else
		{
			uint32_t aPitch = Align4((uint32_t)anImage->w * (aBpp >> 3));
			aSize = (uint32_t)anImage->h * aPitch;
			const uint8_t* aRgb = nullptr;
			if (anImage->bankFmt.bpp == 8)
				aRgb = Bank_RawPaletteAt(theBank, (uint32_t)anImage->paletteNumber);
			if (ConvertPixels(theFmt, aCursor, (int)aPitch, 0, 0, &anImage->bankFmt, aRgb, aSrc, anImage->bankPitch,
					0, 0, anImage->w, anImage->h) == 0)
				return 0;
			anImage->bits = aCursor;
			anImage->pitch = (int)aPitch;
			anImage->dataSize = (int)aSize;
			PixelFormat_Init(&anImage->fmt, theFmt->bpp, theFmt->rMask, theFmt->gMask, theFmt->bMask);
		}
		aCursor = aCursor + aSize;
		if (anImage->isMask == 0 && 8 < (uint32_t)theFmt->bpp &&
			(anImage->bankFmt.bpp == 8 || anImage->bankFmt.bpp == 0x20))
		{
			PixelFormat aMaskFmt;
			uint32_t aMaskPitch;
			uint32_t aMaskSize;
			if (anImage->bankFmt.bpp == 8)
			{
				aMaskPitch = (uint32_t)anImage->bankPitch;
				aMaskSize = (uint32_t)anImage->bankDataSize;
				aMaskFmt = anImage->bankFmt;
				std::memcpy(aCursor, aSrc, aMaskSize);
				uint8_t* p = aCursor;
				uint32_t n = aMaskSize;
				if (0 < (int)aMaskSize)
				{
					do
					{
						if (*p != 0)
							*p = 0xff;
						n--;
						p++;
					} while (n != 0);
				}
			}
			else
			{
				aMaskPitch = Align4((uint32_t)anImage->w);
				aMaskSize = (uint32_t)anImage->h * aMaskPitch;
				PixelFormat_Init(&aMaskFmt, 8, 0, 0, 0);
				ExtractAlpha(aCursor, (int)aMaskPitch, 0, 0, aSrc, anImage->bankPitch, 0, 0, anImage->w, anImage->h);
			}
			if (anImage->autoMask == nullptr)
				anImage->autoMask = new Image();
			Image* aMask = anImage->autoMask;
			aMask->notOwned = 1;
			aMask->bits = aCursor;
			aMask->w = anImage->w;
			aMask->h = anImage->h;
			aMask->pitch = (int)aMaskPitch;
			aMask->origW = anImage->origW;
			aMask->origH = anImage->origH;
			aMask->xOff = anImage->xOff;
			aMask->yOff = anImage->yOff;
			aMask->dataSize = (int)aMaskSize;
			aMask->paletteNumber = anImage->paletteNumber;
			aMask->fmt = aMaskFmt;
			aMask->autoMask = nullptr;
			std::strcpy(aMask->name, "autoMaskImage-");
			std::strncat(aMask->name, anImage->name, sizeof(aMask->name) - 1 - std::strlen(aMask->name));
			aMask->defMask = aMask;
			aMask->hasMask = 1;
			aMask->mask = aMask;
			anImage->defMask = anImage->autoMask;
			anImage->mask = anImage->defMask;
			aCursor = aCursor + aMaskSize;
			anImage->hasMask = 1;
		}
	}
	return 1;
}

// 0x427050: an image named mask_<name> becomes the mask of each image named <name>
int Bank_LinkMasks(Bank* theBank)
{
	for (Image* aMaskImage : theBank->images)
	{
		if (aMaskImage->unke4 == 1 && 5 < std::strlen(aMaskImage->name) &&
			std::strncmp(aMaskImage->name, "mask_", 5) == 0)
		{
			char aTarget[0x400];
			CopyName(aTarget, sizeof(aTarget), aMaskImage->name + 5);
			for (Image* anImage : theBank->images)
			{
				if (std::strcmp(aTarget, anImage->name) == 0)
				{
					anImage->hasMask = 1;
					anImage->defMask = aMaskImage;
					anImage->mask = anImage->defMask;
				}
			}
		}
	}
	return 1;
}

// 0x427270
int Bank_AssignPalettes(Bank* theBank)
{
	for (Image* anImage : theBank->images)
	{
		if (anImage->fmt.bpp == 8)
		{
			anImage->defPalette = Bank_PaletteAt(theBank, (uint32_t)anImage->paletteNumber);
			if (anImage->palette == nullptr)
				anImage->palette = anImage->defPalette;
		}
	}
	return 1;
}

// 0x427340
int Bank_AssignRawPalettes(Bank* theBank)
{
	for (Image* anImage : theBank->images)
	{
		if (anImage->fmt.bpp == 8)
			anImage->rawPalette = Bank_RawPaletteAt(theBank, (uint32_t)anImage->paletteNumber);
	}
	return 1;
}

// 0x4273d0: from the first image whose name starts with theName's first letter, the first named theName
Image* Bank_FindImage(const Bank* theBank, const char* theName, int* theIndex)
{
	if (theName == nullptr)
		return nullptr;
	size_t i = 0;
	int anIndex = 0;
	while (i < theBank->images.size())
	{
		if (theName[0] == theBank->images[i]->name[0])
			break;
		i++;
		anIndex++;
	}
	if (i != theBank->images.size())
	{
		while (i < theBank->images.size())
		{
			if (std::strcmp(theName, theBank->images[i]->name) == 0)
				break;
			i++;
			anIndex++;
		}
		if (i != theBank->images.size())
		{
			*theIndex = anIndex;
			return theBank->images[i];
		}
	}
	return nullptr;
}

// 0x4275c0
int Bank_IndexOf(const Bank* theBank, const char* theName)
{
	int anIndex;
	if (Bank_FindImage(theBank, theName, &anIndex) != nullptr)
		return anIndex;
	return -1;
}

// 0x427820
Image* Bank_FindImage2(const Bank* theBank, const char* theName)
{
	int anIndex;
	if (Bank_FindImage(theBank, theName, &anIndex) != nullptr && anIndex != -1 &&
		(uint32_t)anIndex < theBank->images.size())
		return theBank->images[(uint32_t)anIndex];
	return nullptr;
}

// 0x427c20: the first time makes a Palette per raw one and gives the 8-bit images theirs; every time converts the
// raw colours to theFmt (16-bit, else 32-bit)
int Bank_BuildPalettes(Bank* theBank, const PixelFormat* theFmt)
{
	if (theBank->palettesBuilt == 0)
	{
		theBank->palettes.clear(); // 0x425ee0: the list's nodes, not the palettes
		for (size_t i = 0; i < theBank->rawPalettes.size(); i++)
			theBank->palettes.push_back(new Palette());
		Bank_AssignPalettes(theBank);
		theBank->palettesBuilt = 1;
	}
	size_t j = 0;
	for (size_t i = 0; i < theBank->rawPalettes.size(); i++)
	{
		Palette* aPalette = theBank->palettes[j];
		std::memcpy(aPalette->rgb, theBank->rawPalettes[i], 0x400);
		if (theFmt->bpp == 0x10)
			PixelFormat_PackPalette16(theFmt, aPalette->p16, aPalette->rgb, 0x100);
		else
			PixelFormat_PackPalette32(theFmt, aPalette->p32, aPalette->rgb, 0x100);
		j++;
	}
	return 1;
}

// 0x427df0: reads the .bbk. The datafile is found beside it; each IMAGE becomes an Image whose bits the bank will
// own, each PALETTE 256 colours (a palette's unused entries are 0 here, unset memory in the original).
int Bank_ParseXml(Bank* theBank, const char* thePath)
{
	XmlTag aTag;
	Bank_Clear(theBank);
	std::vector<uint8_t> aFile;
	if (!Sys_ReadFile(thePath, aFile))
		return 0;
	aFile.push_back(0);
	const char* p = (const char*)aFile.data();
	p = Xml_NextTag(p, &aTag);
	const char* aDataFile = Xml_GetString(&aTag, "datafile");
	if (aDataFile == nullptr)
		return 0;
	// The .bbk's folder: its path up to the last '\', '/' or ':'
	char aDir[0x104];
	CopyName(aDir, sizeof(aDir), thePath);
	int anEnd = (int)std::strlen(aDir);
	int aCut;
	char c;
	do
	{
		aCut = anEnd;
		if (aCut + -1 < 0 || (c = aDir[aCut - 1], c == '\\') || c == '/')
			break;
		anEnd = aCut + -1;
	} while (c != ':');
	aDir[aCut] = '\0';
	CopyName(theBank->dataPath, sizeof(theBank->dataPath), aDir);
	std::strncat(theBank->dataPath, aDataFile, sizeof(theBank->dataPath) - 1 - std::strlen(theBank->dataPath));

	p = Xml_NextTag(p, &aTag); // IMAGELIST
	p = Xml_NextTag(p, &aTag);
	while (std::strcmp(aTag.name, "IMAGE") == 0)
	{
		Image* anImage = new Image();
		anImage->notOwned = 1;
		anImage->w = Xml_GetInt(&aTag, "width");
		anImage->h = Xml_GetInt(&aTag, "height");
		anImage->pitch = Xml_GetInt(&aTag, "pitch");
		anImage->origW = Xml_GetInt(&aTag, "originalWidth");
		anImage->origH = Xml_GetInt(&aTag, "originalHeight");
		anImage->xOff = Xml_GetInt(&aTag, "xOffset");
		anImage->yOff = Xml_GetInt(&aTag, "yOffset");
		anImage->dataSize = Xml_GetInt(&aTag, "dataSize");
		anImage->paletteNumber = Xml_GetInt(&aTag, "paletteNumber");
		uint32_t aBlue = (uint32_t)Xml_GetInt(&aTag, "blueMask");
		uint32_t aGreen = (uint32_t)Xml_GetInt(&aTag, "greenMask");
		uint32_t aRed = (uint32_t)Xml_GetInt(&aTag, "redMask");
		int aBpp = Xml_GetInt(&aTag, "bpp");
		PixelFormat_Init(&anImage->fmt, aBpp, aRed, aGreen, aBlue);
		const char* aName = Xml_GetString(&aTag, "name");
		CopyName(anImage->name, sizeof(anImage->name), aName != nullptr ? aName : "");
		if (5 < std::strlen(anImage->name) && std::strncmp(anImage->name, "mask_", 5) == 0)
		{
			anImage->isMask = 1;
			anImage->unke4 = 1;
		}
		anImage->bankDataSize = anImage->dataSize;
		anImage->bankPitch = anImage->pitch;
		anImage->bankFmt = anImage->fmt;
		anImage->clip[0] = 0;
		anImage->clip[1] = 0;
		anImage->clip[2] = anImage->w + -1;
		anImage->clip[3] = anImage->h + -1;
		theBank->images.push_back(anImage);
		p = Xml_NextTag(p, &aTag);
	}
	const char* aNext = Xml_NextTag(p, &aTag);
	if (std::strcmp(aTag.name, "PALETTELIST") == 0)
	{
		p = Xml_NextTag(aNext, &aTag);
		while (std::strcmp(aTag.name, "PALETTE") == 0)
		{
			int aCount = 0;
			uint8_t* aPalette = (uint8_t*)std::calloc(1, 0x400);
			int n = Xml_GetInt(&aTag, "colorsUsed");
			if (0 < n)
			{
				uint8_t* aColour = aPalette + 2;
				const char* aText = aTag.text;
				char aToken[51];
				do
				{
					aText = Xml_ReadToken(aText, aToken, aTag.textLen, &aCount);
					aColour[-2] = (uint8_t)std::atol(aToken);
					aText = Xml_ReadToken(aText, aToken, aTag.textLen, &aCount);
					aColour[-1] = (uint8_t)std::atol(aToken);
					aText = Xml_ReadToken(aText, aToken, aTag.textLen, &aCount);
					*aColour = (uint8_t)std::atol(aToken);
					aColour += 4;
					n--;
				} while (n != 0);
			}
			theBank->rawPalettes.push_back(aPalette);
			aNext = Xml_NextTag(p, &aTag); // /PALETTE
			p = Xml_NextTag(aNext, &aTag);
		}
	}
	return 1;
}

// 0x4284f0
int Bank_Load(Bank* theBank, const char* thePath, int theMode, const PixelFormat* theFmt)
{
	if (Bank_ParseXml(theBank, thePath) == 0)
		return 0;
	if (theMode == 0)
	{
		if (Bank_LoadRaw(theBank, theBank->dataPath) == 0)
			return 0;
		if (Bank_AssignRawPalettes(theBank) == 0)
			return 0;
		if (theFmt != nullptr && Bank_BuildPalettes(theBank, theFmt) == 0)
			return 0;
	}
	else
	{
		if (theFmt == nullptr)
			return 0;
		if (Bank_LoadConverted(theBank, theBank->dataPath, theFmt) == 0)
			return 0;
	}
	SelfMask8(theBank);
	if (Bank_LinkMasks(theBank) == 0)
		return 0;
	theBank->mode = theMode;
	return 1;
}

// 0x428640: after the screen's format changed
int Bank_Reconvert(Bank* theBank, const PixelFormat* theFmt)
{
	if (theBank->palettesBuilt == 1 && Bank_BuildPalettes(theBank, theFmt) == 0)
		return 0;
	if (theBank->mode != 1)
		return 1;
	if (Bank_LoadConverted(theBank, theBank->dataPath, theFmt) == 0)
		return 0;
	SelfMask8(theBank);
	if (Bank_LinkMasks(theBank) == 0)
		return 0;
	return 1;
}

} // namespace r64
