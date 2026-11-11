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

// Each 8-bit image is its own mask (Bank_Load)
void SelfMask8(Bank* theBank)
{
	for (Image* anImage : theBank->mImages)
	{
		if (anImage->mFmt.mBpp == 8)
		{
			anImage->mHasMask = 1;
			anImage->mDefMask = anImage;
			anImage->mMask = anImage->mDefMask;
		}
	}
}

} // namespace

// 0x401a80
int PixelFormat_PackPalette32(const PixelFormat* theFmt, uint32_t* theDst, const uint8_t* theRgb, int theCount)
{
	if (theDst != nullptr && theRgb != nullptr && -1 < theCount)
	{
		int i = 0;
		if (0 < theCount)
		{
			const uint8_t* p = theRgb + 1;
			do
			{
				i++;
				theDst[i - 1] = (uint32_t)(p[-1] >> (theFmt->mRLoss & 0x1f)) << (theFmt->mRShift & 0x1f) |
					(uint32_t)(p[1] >> (theFmt->mBLoss & 0x1f)) << (theFmt->mBShift & 0x1f) |
					(uint32_t)(*p >> (theFmt->mGLoss & 0x1f)) << (theFmt->mGShift & 0x1f);
				p += 4;
			} while (i < theCount);
		}
		return 1;
	}
	return 0;
}

// 0x401b10
int PixelFormat_PackPalette16(const PixelFormat* theFmt, uint16_t* theDst, const uint8_t* theRgb, int theCount)
{
	if (theDst != nullptr && theRgb != nullptr && -1 < theCount)
	{
		int i = 0;
		if (0 < theCount)
		{
			const uint8_t* p = theRgb + 1;
			do
			{
				i++;
				theDst[i - 1] = (uint16_t)((uint32_t)(p[-1] >> (theFmt->mRLoss & 0x1f)) << (theFmt->mRShift & 0x1f) |
					(uint32_t)(p[1] >> (theFmt->mBLoss & 0x1f)) << (theFmt->mBShift & 0x1f) |
					(uint32_t)(*p >> (theFmt->mGLoss & 0x1f)) << (theFmt->mGShift & 0x1f));
				p += 4;
			} while (i < theCount);
		}
		return 1;
	}
	return 0;
}

// 0x427790
Bank::Bank()
{
	std::memset(mDataPath, 0, sizeof(mDataPath));
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
	theBank->mMode = 0;
	theBank->mImages.clear();
	theBank->mRawPalettes.clear();
	theBank->mPalettesBuilt = 0;
	theBank->mPalettes.clear();
	theBank->mDataSize = 0;
	theBank->mData = nullptr;
}

// 0x426250
void Bank_Clear(Bank* theBank)
{
	for (Image* anImage : theBank->mImages)
	{
		if (anImage != nullptr)
		{
			Image_Destroy(anImage);
			delete anImage;
		}
	}
	theBank->mImages.clear();
	for (uint8_t* aPalette : theBank->mRawPalettes)
		std::free(aPalette);
	theBank->mRawPalettes.clear();
	for (Palette* aPalette : theBank->mPalettes)
		delete aPalette;
	theBank->mPalettes.clear();
	if (theBank->mData != nullptr)
		std::free(theBank->mData);
	Bank_Reset(theBank);
}

// 0x425de0: grows the data to theSize and clears it (1 even when malloc fails, as in the original)
int Bank_Alloc(Bank* theBank, uint32_t theSize)
{
	if (theBank->mDataSize < theSize)
	{
		if (theBank->mData != nullptr)
			std::free(theBank->mData);
		theBank->mData = (uint8_t*)std::malloc(theSize);
		if (theBank->mData == nullptr)
			return 1;
		theBank->mDataSize = theSize;
	}
	std::memset(theBank->mData, 0, theBank->mDataSize);
	return 1;
}

// 0x425eb0
Image* Bank_ImageAt(const Bank* theBank, uint32_t theIndex)
{
	if (theIndex < theBank->mImages.size())
		return theBank->mImages[theIndex];
	return nullptr;
}

// 0x425ed0
int Bank_Count(const Bank* theBank)
{
	return (int)theBank->mImages.size();
}

// 0x426430 (a negative index finds nothing)
uint8_t* Bank_RawPaletteAt(const Bank* theBank, uint32_t theIndex)
{
	if ((int)theIndex < 0 || theBank->mRawPalettes.size() <= theIndex)
		return nullptr;
	return theBank->mRawPalettes[theIndex];
}

// 0x426480
Palette* Bank_PaletteAt(const Bank* theBank, uint32_t theIndex)
{
	if ((int)theIndex < 0 || theBank->mPalettes.size() <= theIndex)
		return nullptr;
	return theBank->mPalettes[theIndex];
}

// 0x4264d0: the file as it is; each image's bits follow the last one's (pitch x height)
int Bank_LoadRaw(Bank* theBank, const char* thePath)
{
	std::vector<uint8_t> aFile;
	if (!Sys_ReadFile(thePath, aFile))
		return 0;
	if (Bank_Alloc(theBank, (uint32_t)aFile.size()) != 0 && theBank->mDataSize != 0 &&
		theBank->mDataSize <= aFile.size()) // fread(data, dataSize, 1) == 1
	{
		std::memcpy(theBank->mData, aFile.data(), theBank->mDataSize);
		uint8_t* aCursor = theBank->mData;
		for (Image* anImage : theBank->mImages)
		{
			anImage->mBits = aCursor;
			aCursor = aCursor + anImage->mPitch * anImage->mH;
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
	uint32_t aBpp = (uint32_t)theFmt->mBpp;
	uint32_t aTotal = 0;
	for (Image* anImage : theBank->mImages)
	{
		uint32_t aPitch;
		if (anImage->mIsMask == 1)
			aPitch = (uint32_t)anImage->mBankPitch;
		else
		{
			aPitch = (uint32_t)anImage->mW * (aBpp >> 3);
			if (8 < (uint32_t)theFmt->mBpp)
			{
				if (anImage->mBankFmt.mBpp == 8)
					aTotal = aTotal + (uint32_t)anImage->mBankDataSize;
				if (8 < (uint32_t)theFmt->mBpp && anImage->mBankFmt.mBpp == 0x20)
					aTotal = aTotal + (uint32_t)anImage->mH * Align4((uint32_t)anImage->mW);
			}
		}
		aPitch = Align4(aPitch);
		aTotal = aTotal + (uint32_t)anImage->mH * aPitch;
	}
	if (theBank->mDataSize < aTotal)
	{
		if (theBank->mData != nullptr)
			std::free(theBank->mData);
		theBank->mData = (uint8_t*)std::malloc(aTotal);
		if (theBank->mData == nullptr)
			return 0; // the original goes on into a null buffer
		theBank->mDataSize = aTotal;
	}
	std::memset(theBank->mData, 0, theBank->mDataSize);

	std::vector<uint8_t> aFile;
	if (!Sys_ReadFile(thePath, aFile))
		return 0;
	uint8_t* aCursor = theBank->mData;
	size_t aRead = 0;
	for (Image* anImage : theBank->mImages)
	{
		uint32_t aChunk = (uint32_t)anImage->mBankDataSize;
		if (aChunk == 0 || aFile.size() - aRead < aChunk)
			return 0;
		const uint8_t* aSrc = aFile.data() + aRead;
		aRead += aChunk;
		uint32_t aSize;
		if (anImage->mIsMask == 1)
		{
			aSize = aChunk;
			std::memcpy(aCursor, aSrc, aSize);
			anImage->mBits = aCursor;
		}
		else
		{
			uint32_t aPitch = Align4((uint32_t)anImage->mW * (aBpp >> 3));
			aSize = (uint32_t)anImage->mH * aPitch;
			const uint8_t* aRgb = nullptr;
			if (anImage->mBankFmt.mBpp == 8)
				aRgb = Bank_RawPaletteAt(theBank, (uint32_t)anImage->mPaletteNumber);
			if (ConvertPixels(theFmt, aCursor, (int)aPitch, 0, 0, &anImage->mBankFmt, aRgb, aSrc, anImage->mBankPitch,
					0, 0, anImage->mW, anImage->mH) == 0)
				return 0;
			anImage->mBits = aCursor;
			anImage->mPitch = (int)aPitch;
			anImage->mDataSize = (int)aSize;
			PixelFormat_Init(&anImage->mFmt, theFmt->mBpp, theFmt->mRMask, theFmt->mGMask, theFmt->mBMask);
		}
		aCursor = aCursor + aSize;
		if (anImage->mIsMask == 0 && 8 < (uint32_t)theFmt->mBpp &&
			(anImage->mBankFmt.mBpp == 8 || anImage->mBankFmt.mBpp == 0x20))
		{
			PixelFormat aMaskFmt;
			uint32_t aMaskPitch;
			uint32_t aMaskSize;
			if (anImage->mBankFmt.mBpp == 8)
			{
				aMaskPitch = (uint32_t)anImage->mBankPitch;
				aMaskSize = (uint32_t)anImage->mBankDataSize;
				aMaskFmt = anImage->mBankFmt;
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
				aMaskPitch = Align4((uint32_t)anImage->mW);
				aMaskSize = (uint32_t)anImage->mH * aMaskPitch;
				PixelFormat_Init(&aMaskFmt, 8, 0, 0, 0);
				ExtractAlpha(aCursor, (int)aMaskPitch, 0, 0, aSrc, anImage->mBankPitch, 0, 0, anImage->mW, anImage->mH);
			}
			if (anImage->mAutoMask == nullptr)
				anImage->mAutoMask = new Image();
			Image* aMask = anImage->mAutoMask;
			aMask->mNotOwned = 1;
			aMask->mBits = aCursor;
			aMask->mW = anImage->mW;
			aMask->mH = anImage->mH;
			aMask->mPitch = (int)aMaskPitch;
			aMask->mOrigW = anImage->mOrigW;
			aMask->mOrigH = anImage->mOrigH;
			aMask->mXOff = anImage->mXOff;
			aMask->mYOff = anImage->mYOff;
			aMask->mDataSize = (int)aMaskSize;
			aMask->mPaletteNumber = anImage->mPaletteNumber;
			aMask->mFmt = aMaskFmt;
			aMask->mAutoMask = nullptr;
			std::strcpy(aMask->mName, "autoMaskImage-");
			std::strncat(aMask->mName, anImage->mName, sizeof(aMask->mName) - 1 - std::strlen(aMask->mName));
			aMask->mDefMask = aMask;
			aMask->mHasMask = 1;
			aMask->mMask = aMask;
			anImage->mDefMask = anImage->mAutoMask;
			anImage->mMask = anImage->mDefMask;
			aCursor = aCursor + aMaskSize;
			anImage->mHasMask = 1;
		}
	}
	return 1;
}

// 0x427050: an image named mask_<name> becomes the mask of each image named <name>
int Bank_LinkMasks(Bank* theBank)
{
	for (Image* aMaskImage : theBank->mImages)
	{
		if (aMaskImage->mMaskNamed == 1 && 5 < std::strlen(aMaskImage->mName) &&
			std::strncmp(aMaskImage->mName, "mask_", 5) == 0)
		{
			char aTarget[0x400];
			CopyName(aTarget, sizeof(aTarget), aMaskImage->mName + 5);
			for (Image* anImage : theBank->mImages)
			{
				if (std::strcmp(aTarget, anImage->mName) == 0)
				{
					anImage->mHasMask = 1;
					anImage->mDefMask = aMaskImage;
					anImage->mMask = anImage->mDefMask;
				}
			}
		}
	}
	return 1;
}

// 0x427270
int Bank_AssignPalettes(Bank* theBank)
{
	for (Image* anImage : theBank->mImages)
	{
		if (anImage->mFmt.mBpp == 8)
		{
			anImage->mDefPalette = Bank_PaletteAt(theBank, (uint32_t)anImage->mPaletteNumber);
			if (anImage->mPalette == nullptr)
				anImage->mPalette = anImage->mDefPalette;
		}
	}
	return 1;
}

// 0x427340
int Bank_AssignRawPalettes(Bank* theBank)
{
	for (Image* anImage : theBank->mImages)
	{
		if (anImage->mFmt.mBpp == 8)
			anImage->mRawPalette = Bank_RawPaletteAt(theBank, (uint32_t)anImage->mPaletteNumber);
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
	while (i < theBank->mImages.size())
	{
		if (theName[0] == theBank->mImages[i]->mName[0])
			break;
		i++;
		anIndex++;
	}
	if (i != theBank->mImages.size())
	{
		while (i < theBank->mImages.size())
		{
			if (std::strcmp(theName, theBank->mImages[i]->mName) == 0)
				break;
			i++;
			anIndex++;
		}
		if (i != theBank->mImages.size())
		{
			*theIndex = anIndex;
			return theBank->mImages[i];
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
		(uint32_t)anIndex < theBank->mImages.size())
		return theBank->mImages[(uint32_t)anIndex];
	return nullptr;
}

// 0x427c20: the first time makes a Palette per raw one and gives the 8-bit images theirs; every time converts the
// raw colours to theFmt (16-bit, else 32-bit)
int Bank_BuildPalettes(Bank* theBank, const PixelFormat* theFmt)
{
	if (theBank->mPalettesBuilt == 0)
	{
		theBank->mPalettes.clear(); // 0x425ee0: the list's nodes, not the palettes
		for (size_t i = 0; i < theBank->mRawPalettes.size(); i++)
			theBank->mPalettes.push_back(new Palette());
		Bank_AssignPalettes(theBank);
		theBank->mPalettesBuilt = 1;
	}
	size_t j = 0;
	for (size_t i = 0; i < theBank->mRawPalettes.size(); i++)
	{
		Palette* aPalette = theBank->mPalettes[j];
		std::memcpy(aPalette->mRgb, theBank->mRawPalettes[i], 0x400);
		if (theFmt->mBpp == 0x10)
			PixelFormat_PackPalette16(theFmt, aPalette->mP16, aPalette->mRgb, 0x100);
		else
			PixelFormat_PackPalette32(theFmt, aPalette->mP32, aPalette->mRgb, 0x100);
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
	const char* aCursor = (const char*)aFile.data();
	aCursor = Xml_NextTag(aCursor, &aTag);
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
	CopyName(theBank->mDataPath, sizeof(theBank->mDataPath), aDir);
	std::strncat(theBank->mDataPath, aDataFile, sizeof(theBank->mDataPath) - 1 - std::strlen(theBank->mDataPath));

	aCursor = Xml_NextTag(aCursor, &aTag); // IMAGELIST
	aCursor = Xml_NextTag(aCursor, &aTag);
	while (std::strcmp(aTag.mName, "IMAGE") == 0)
	{
		Image* anImage = new Image();
		anImage->mNotOwned = 1;
		anImage->mW = Xml_GetInt(&aTag, "width");
		anImage->mH = Xml_GetInt(&aTag, "height");
		anImage->mPitch = Xml_GetInt(&aTag, "pitch");
		anImage->mOrigW = Xml_GetInt(&aTag, "originalWidth");
		anImage->mOrigH = Xml_GetInt(&aTag, "originalHeight");
		anImage->mXOff = Xml_GetInt(&aTag, "xOffset");
		anImage->mYOff = Xml_GetInt(&aTag, "yOffset");
		anImage->mDataSize = Xml_GetInt(&aTag, "dataSize");
		anImage->mPaletteNumber = Xml_GetInt(&aTag, "paletteNumber");
		uint32_t aBlue = (uint32_t)Xml_GetInt(&aTag, "blueMask");
		uint32_t aGreen = (uint32_t)Xml_GetInt(&aTag, "greenMask");
		uint32_t aRed = (uint32_t)Xml_GetInt(&aTag, "redMask");
		int aBpp = Xml_GetInt(&aTag, "bpp");
		PixelFormat_Init(&anImage->mFmt, aBpp, aRed, aGreen, aBlue);
		const char* aName = Xml_GetString(&aTag, "name");
		CopyName(anImage->mName, sizeof(anImage->mName), aName != nullptr ? aName : "");
		if (5 < std::strlen(anImage->mName) && std::strncmp(anImage->mName, "mask_", 5) == 0)
		{
			anImage->mIsMask = 1;
			anImage->mMaskNamed = 1;
		}
		anImage->mBankDataSize = anImage->mDataSize;
		anImage->mBankPitch = anImage->mPitch;
		anImage->mBankFmt = anImage->mFmt;
		anImage->mClip[0] = 0;
		anImage->mClip[1] = 0;
		anImage->mClip[2] = anImage->mW + -1;
		anImage->mClip[3] = anImage->mH + -1;
		theBank->mImages.push_back(anImage);
		aCursor = Xml_NextTag(aCursor, &aTag);
	}
	const char* aNext = Xml_NextTag(aCursor, &aTag);
	if (std::strcmp(aTag.mName, "PALETTELIST") == 0)
	{
		aCursor = Xml_NextTag(aNext, &aTag);
		while (std::strcmp(aTag.mName, "PALETTE") == 0)
		{
			int aCount = 0;
			uint8_t* aPalette = (uint8_t*)std::calloc(1, 0x400);
			int aColourCount = Xml_GetInt(&aTag, "colorsUsed");
			if (0 < aColourCount)
			{
				uint8_t* aColour = aPalette + 2;
				const char* aText = aTag.mText;
				char aToken[51];
				do
				{
					aText = Xml_ReadToken(aText, aToken, aTag.mTextLen, &aCount);
					aColour[-2] = (uint8_t)std::atol(aToken);
					aText = Xml_ReadToken(aText, aToken, aTag.mTextLen, &aCount);
					aColour[-1] = (uint8_t)std::atol(aToken);
					aText = Xml_ReadToken(aText, aToken, aTag.mTextLen, &aCount);
					*aColour = (uint8_t)std::atol(aToken);
					aColour += 4;
					aColourCount--;
				} while (aColourCount != 0);
			}
			theBank->mRawPalettes.push_back(aPalette);
			aNext = Xml_NextTag(aCursor, &aTag); // /PALETTE
			aCursor = Xml_NextTag(aNext, &aTag);
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
		if (Bank_LoadRaw(theBank, theBank->mDataPath) == 0)
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
		if (Bank_LoadConverted(theBank, theBank->mDataPath, theFmt) == 0)
			return 0;
	}
	SelfMask8(theBank);
	if (Bank_LinkMasks(theBank) == 0)
		return 0;
	theBank->mMode = theMode;
	return 1;
}

} // namespace r64
