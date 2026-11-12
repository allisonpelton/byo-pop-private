// Animations (0x401480..0x402850): the <animation> tags of the game files, each a run of bank images, a FLIC with
// an optional mask FLIC, or a TGA.

#include "CBGame.h"
#include "R64Flic.h"
#include "R64Sys.h"
#include "R64Blit.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace cb
{

Bank gArtBank;
Bank gArt8Bank;
List<Animation> gAnims = {nullptr, nullptr, nullptr};
Palette gGreyPalette;
int gGreyPaletteUnset = 1;

namespace
{

// 0x42b540 into its 0x24-byte result (bits, size, pitch, width, height, bpp and the colour masks). The game has no
// TGA files, and run-length TGAs (type 10, 0x42b3d0) are not translated.
struct Tga
{
	uint8_t* mBits;
	int mSize;
	int mPitch;
	int mWidth;
	int mHeight;
	int mBpp;
	uint32_t mRMask;
	uint32_t mGMask;
	uint32_t mBMask;
};

bool Tga_Load(const std::vector<uint8_t>& theFile, Tga* theTga)
{
	if (theFile.size() < 0x12)
		return false;
	const uint8_t* h = theFile.data();
	uint8_t anIdLen = h[0];
	uint8_t aType = h[2];
	uint8_t aMapEntry = h[7];
	int aWidth = h[0xc] | h[0xd] << 8;
	int aHeight = h[0xe] | h[0xf] << 8;
	uint8_t aBpp = h[0x10];
	uint8_t aDesc = h[0x11];
	// the original skips the colour map's entry size, not the map
	size_t aPos = (size_t)aMapEntry + 0x12 + anIdLen;
	int aBytes = (aBpp + 7) >> 3;
	int aSize = aHeight * aWidth * aBytes;
	if (aType == 10)
		return false;
	uint8_t* aBits = (uint8_t*)std::calloc((size_t)aSize, 1);
	theTga->mSize = aSize;
	auto aRead = [&](uint8_t* theDst, size_t n) {
		size_t aHave = aPos < theFile.size() ? theFile.size() - aPos : 0;
		if (n > aHave)
			n = aHave;
		std::memcpy(theDst, theFile.data() + aPos, n);
		aPos += n;
	};
	if ((aDesc & 0x20) == 0)
	{
		int aRow = aWidth * aBytes;
		uint8_t* p = aBits + (aSize - aRow);
		for (int y = 0; y < aHeight; y++)
		{
			aRead(p, (size_t)aRow);
			p = p - aRow;
		}
	}
	else
		aRead(aBits, (size_t)aSize);
	theTga->mBpp = aBpp;
	theTga->mWidth = aWidth;
	theTga->mBits = aBits;
	theTga->mHeight = aHeight;
	theTga->mPitch = aWidth * aBytes;
	theTga->mGMask = 0xff00;
	if (aBpp == 0x20)
	{
		theTga->mRMask = 0xff0000;
		theTga->mBMask = 0xff;
	}
	else
	{
		theTga->mBMask = 0xff0000;
		theTga->mRMask = 0xff;
	}
	return true;
}

// "animations/X" with .eco made .flc
void FlicPath(char* theOut, size_t theSize, const char* theFormat, const char* theFile)
{
	std::snprintf(theOut, theSize, theFormat, theFile);
	char* anEco = std::strstr(theOut, ".eco");
	if (anEco != nullptr)
		std::memcpy(anEco, ".flc", 4);
}

} // namespace

Animation::Animation()
{
	std::memset(mPalette.mRgb, 0, sizeof(mPalette.mRgb));
	mPalette.mP16 = (uint16_t*)mPalette.mConv;
	mPalette.mP32 = (uint32_t*)mPalette.mConv;
	std::memset(mPalette.mConv, 0, sizeof(mPalette.mConv));
	mSwap1[0] = 0;
	mSwap2[0] = 0;
	mSwap3[0] = 0;
	mFile[0] = 0;
	mRotation = 0;
	for (int i = 0; i < 0x100; i++)
		mFrames[i] = nullptr;
	mNext = nullptr;
	mPrev = nullptr;
	mName[0] = 0;
	mCount = 0;
	mFrameTime = 0;
}

Animation::~Animation()
{
	int i = 0;
	if (0 < mCount)
	{
		do
		{
			Image* aFrame = mFrames[i];
			if (aFrame != nullptr)
			{
				if (aFrame->mHasMask != 0)
				{
					Image_FreeRle(aFrame->mMask);
					if (mFrames[i]->mMask != nullptr)
					{
						Image_Destroy(mFrames[i]->mMask);
						delete mFrames[i]->mMask;
					}
					mFrames[i]->mMask = nullptr;
				}
				if (mFrames[i] != nullptr)
				{
					Image_Destroy(mFrames[i]);
					delete mFrames[i];
				}
				mFrames[i] = nullptr;
			}
			i++;
		} while (i < mCount);
	}
}

// Each tint starts as the FLIC's colours. On a 16-bit screen eight of them recolour the green-dominant entries
// (the fourth's result is worked out and dropped, as in the original, so it stays the plain colours like the
// sixth); on other screens the FLIC's own colours have their blue-dominant entries changed instead.
void Animation_BuildTints(Animation* theAnim)
{
	const PixelFormat* aFormat = Sys_ScreenFormat();
	int i = 0;
	do
	{
		for (int k = 0; k < 10; k++)
		{
			std::memcpy((uint8_t*)theAnim->mTints[k].mP32 + i * 4, (uint8_t*)theAnim->mPalette.mP32 + i * 4, 4);
			std::memcpy((uint8_t*)theAnim->mTints[k].mP16 + i * 2, (uint8_t*)theAnim->mPalette.mP16 + i * 2, 2);
		}
		if (aFormat->mBpp == 0x10)
		{
			uint32_t aPixel;
			aPixel = theAnim->mPalette.mP16[i];
			ColourShift(&aPixel, aFormat, 0, 1, 0, 0x3c, 0x3c, 0x3c, 0, 0, 0, 0x5a);
			theAnim->mTints[0].mP16[i] = (uint16_t)aPixel;
			aPixel = theAnim->mPalette.mP16[i];
			ColourShift(&aPixel, aFormat, 0, 1, 0, 0xff, 0xff, 0xff, 0, 0, 0, 0x5a);
			theAnim->mTints[1].mP16[i] = (uint16_t)aPixel;
			aPixel = theAnim->mPalette.mP16[i];
			ColourShift(&aPixel, aFormat, 0, 1, 0, 0x6e, 0x50, 0x50, 1, 0, 0, 0x5a);
			theAnim->mTints[2].mP16[i] = (uint16_t)aPixel;
			aPixel = theAnim->mPalette.mP16[i];
			ColourShift(&aPixel, aFormat, 0, 1, 0, 0x73, 0x73, 0x3c, 1, 0, 0, 0x5a);
			aPixel = theAnim->mPalette.mP16[i];
			ColourShift(&aPixel, aFormat, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0x5a);
			theAnim->mTints[4].mP16[i] = (uint16_t)aPixel;
			theAnim->mTints[5].mP16[i] = theAnim->mPalette.mP16[i];
			aPixel = theAnim->mPalette.mP16[i];
			ColourShift(&aPixel, aFormat, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0x5a);
			theAnim->mTints[6].mP16[i] = (uint16_t)aPixel;
			aPixel = theAnim->mPalette.mP16[i];
			ColourShift(&aPixel, aFormat, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0x5a);
			theAnim->mTints[7].mP16[i] = (uint16_t)aPixel;
			aPixel = theAnim->mPalette.mP16[i];
			ColourShift(&aPixel, aFormat, 0, 1, 0, 0x5a, 0x46, 0x6e, 1, 0, 1, 0x5a);
			theAnim->mTints[8].mP16[i] = (uint16_t)aPixel;
			aPixel = theAnim->mPalette.mP16[i];
			ColourShift(&aPixel, aFormat, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0x5a);
			theAnim->mTints[9].mP16[i] = (uint16_t)aPixel;
		}
		else
			ColourShift(&theAnim->mPalette.mP32[i], aFormat, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0x5a);
		i++;
	} while (i < 0x100);
}

Image* Animation_Frame(const Animation* theAnim, int theIndex)
{
	return theAnim->mFrames[theIndex];
}

Animation* Animation_Find(const char* theName, int* theCount, int* theFrameTime)
{
	Animation* anAnim = gAnims.mHead;
	if (gAnims.mCurrent == nullptr)
		return nullptr;
	while (true)
	{
		if (std::strcmp(anAnim->mName, theName) == 0)
		{
			gAnims.mCurrent = anAnim;
			*theCount = anAnim->mCount;
			*theFrameTime = gAnims.mCurrent->mFrameTime;
			return gAnims.mCurrent;
		}
		if (anAnim->mNext == nullptr)
		{
			gAnims.mCurrent = gAnims.mHead;
			return nullptr;
		}
		anAnim = anAnim->mNext;
	}
}

int Animation_Load(Animation* theAnim)
{
	char* aFile = theAnim->mFile;
	if (std::strstr(aFile, ".eco") == nullptr && std::strstr(aFile, ".tga") == nullptr)
	{
		int anIndex = Bank_IndexOf(&gArtBank, aFile);
		if (anIndex == 0)
			return 0;
		int i = 0;
		if (0 < theAnim->mCount)
		{
			do
			{
				Image* aSrc = Bank_ImageAt(&gArtBank, (uint32_t)(anIndex + i));
				theAnim->mFrames[i] = new Image();
				Image_Copy(theAnim->mFrames[i], aSrc);
				if (aSrc->mHasMask != 0)
				{
					theAnim->mFrames[i]->mHasMask = 1;
					theAnim->mFrames[i]->mMask = new Image();
					Image_Copy(theAnim->mFrames[i]->mMask, aSrc->mMask);
				}
				i++;
			} while (i < theAnim->mCount);
		}
	}
	else if (std::strstr(aFile, ".tga") == nullptr)
	{
		char aPath[0x100];
		FlicPath(aPath, sizeof(aPath), "animations/%s", aFile);
		std::vector<uint8_t> aData;
		if (Sys_ReadFile(aPath, aData))
		{
			Flic* aFlic = Flic_Open(aData.data(), aData.size());
			theAnim->mCount = aFlic->mFrames;
			PixelFormat aFmt;
			PixelFormat_Init(&aFmt, 8, 0, 0, 0);
			Image* aWork = new Image();
			Image_Create(aWork, aFlic->mWidth, aFlic->mHeight, &aFmt);
			int i = 0;
			if (aFlic->mFrames != 0)
			{
				do
				{
					Flic_NextFrame(aFlic, aWork->mBits, (uint16_t)aWork->mPitch);
					if (i == 0)
					{
						for (int c = 0; c < 0x100; c++)
						{
							theAnim->mPalette.mRgb[c * 4] = gFlicPalette[c * 3];
							theAnim->mPalette.mRgb[c * 4 + 1] = gFlicPalette[c * 3 + 1];
							theAnim->mPalette.mRgb[c * 4 + 2] = gFlicPalette[c * 3 + 2];
						}
						const PixelFormat* f = Sys_ScreenFormat();
						if (f->mBpp == 0x10)
							PixelFormat_PackPalette16(f, theAnim->mPalette.mP16, theAnim->mPalette.mRgb, 0x100);
						else
							PixelFormat_PackPalette32(f, theAnim->mPalette.mP32, theAnim->mPalette.mRgb, 0x100);
						Animation_BuildTints(theAnim);
					}
					aWork->mPalette = &theAnim->mPalette;
					theAnim->mFrames[i] = new Image();
					Image_Copy(theAnim->mFrames[i], aWork);
					Image_SetPalette(theAnim->mFrames[i], &theAnim->mPalette);
					i++;
				} while (i < (int)(uint32_t)aFlic->mFrames);
			}
			Flic_Close(aFlic);
			FlicPath(aPath, sizeof(aPath), "animations/mask_%s", theAnim->mFile);
			std::vector<uint8_t> aMaskData;
			if (!Sys_ReadFile(aPath, aMaskData))
			{
				int k = 0;
				if (0 < theAnim->mCount)
				{
					do
					{
						k++;
						theAnim->mFrames[k - 1]->mHasMask = 0;
					} while (k < theAnim->mCount);
				}
			}
			else
			{
				std::memset(aWork->mBits, 0, (size_t)aWork->mDataSize);
				aWork->mPalette = &gGreyPalette;
				aFlic = Flic_Open(aMaskData.data(), aMaskData.size());
				theAnim->mCount = aFlic->mFrames;
				int k = 0;
				if (aFlic->mFrames != 0)
				{
					do
					{
						Flic_NextFrame(aFlic, aWork->mBits, (uint16_t)aWork->mPitch);
						Image* aFrame = theAnim->mFrames[k];
						aFrame->mHasMask = 1;
						aFrame->mMask = new Image();
						Image_Copy(aFrame->mMask, aWork);
						uint32_t j = 0;
						if (aFrame->mMask->mDataSize != 0)
						{
							do
							{
								aFrame->mMask->mBits[j] = gFlicPalette[(uint32_t)aFrame->mMask->mBits[j] * 3];
								j++;
							} while (j < (uint32_t)aFrame->mMask->mDataSize);
						}
						k++;
					} while (k < (int)(uint32_t)aFlic->mFrames);
				}
				Flic_Close(aFlic);
			}
			Image_Destroy(aWork);
			delete aWork;
		}
	}
	else
	{
		char aPath[0x100];
		std::snprintf(aPath, sizeof(aPath), "animations/%s", aFile);
		std::vector<uint8_t> aData;
		if (!Sys_ReadFile(aPath, aData))
			return 0;
		Tga aTga;
		std::memset(&aTga, 0, sizeof(aTga));
		if (Tga_Load(aData, &aTga))
		{
			PixelFormat aFmt;
			PixelFormat_Init(&aFmt, aTga.mBpp, aTga.mRMask, aTga.mGMask, aTga.mBMask);
			Image* aWork = new Image();
			Image_Create(aWork, aTga.mWidth, aTga.mHeight, &aFmt);
			Copy8(aWork->mBits, 0, 0, aWork->mPitch, 0, aTga.mBits, 0, 0, aTga.mPitch, 0, aTga.mWidth * 4,
				aTga.mHeight);
			Image_ConvertTo(aWork, Sys_ScreenFormat(), nullptr);
			theAnim->mFrames[0] = new Image();
			Image_Copy(theAnim->mFrames[0], aWork);
			Image_Destroy(aWork);
			delete aWork;
			PixelFormat_Init(&aFmt, 8, 0, 0, 0);
			Image* aMask = new Image();
			Image_Create(aMask, aTga.mWidth, aTga.mHeight, &aFmt);
			aMask->mPalette = &gGreyPalette;
			int y = 0;
			if (0 < aTga.mHeight)
			{
				do
				{
					int x = 0;
					if (0 < aTga.mWidth)
					{
						do
						{
							uint32_t aPixel;
							std::memcpy(&aPixel, aTga.mBits + ((y * aTga.mPitch) / 4 + x) * 4, 4);
							aMask->mBits[aMask->mPitch * y + x] = (uint8_t)(aPixel >> 0x18);
							x++;
						} while (x < aTga.mWidth);
					}
					y++;
				} while (y < aTga.mHeight);
			}
			theAnim->mFrames[0]->mMask = new Image();
			Image_Copy(theAnim->mFrames[0]->mMask, aMask);
			theAnim->mFrames[0]->mHasMask = 1;
			Image_Destroy(aMask);
			delete aMask;
		}
		std::free(aTga.mBits);
	}
	int i = 0;
	if (0 < theAnim->mCount)
	{
		do
		{
			Image* aFrame = theAnim->mFrames[i];
			if ((aFrame->mW == aFrame->mH || theAnim->mRotation == 0xb4) && theAnim->mRotation != 0)
				RotateImage(aFrame, theAnim->mRotation);
			if (theAnim->mFrames[i]->mHasMask != 0)
				Image_BuildRle(theAnim->mFrames[i]->mMask);
			i++;
		} while (i < theAnim->mCount);
	}
	const char* aSwaps[3] = {theAnim->mSwap1, theAnim->mSwap2, theAnim->mSwap3};
	for (const char* aSwap : aSwaps)
	{
		if (*aSwap != 0 && (i = 0, 0 < theAnim->mCount))
		{
			do
			{
				SwapColours(theAnim->mFrames[i], aSwap);
				i++;
			} while (i < theAnim->mCount);
		}
	}
	return 1;
}

namespace
{

void CopyString(char* theDst, size_t theSize, const char* theSrc)
{
	std::snprintf(theDst, theSize, "%s", theSrc != nullptr ? theSrc : "");
}

} // namespace

// The original copies the attributes without bounds (and a missing one from a null pointer); here they are cut to
// their fields and a missing one is empty
int Animation_FromXml(Animation* theAnim, const XmlTag* theTag)
{
	if (gGreyPaletteUnset != 0)
	{
		int i = 0;
		do
		{
			uint8_t v = (uint8_t)i;
			gGreyPalette.mRgb[i * 4] = v;
			gGreyPalette.mRgb[i * 4 + 1] = v;
			gGreyPalette.mRgb[i * 4 + 2] = v;
			i++;
		} while (i < 0x100);
		gGreyPaletteUnset = 0;
	}
	CopyString(theAnim->mName, sizeof(theAnim->mName), Xml_GetString(theTag, "name"));
	theAnim->mFrameTime = Xml_GetInt(theTag, "frame_time");
	theAnim->mCount = Xml_GetInt(theTag, "num_frames");
	CopyString(theAnim->mFile, sizeof(theAnim->mFile), Xml_GetString(theTag, "start_frame"));
	if (std::strcmp(theAnim->mName, "panel") == 0)
		std::strcpy(theAnim->mFile, "panel_cosmicbugs");
	if (std::strcmp(theAnim->mName, "nag_logo") == 0)
	{
		std::strcpy(theAnim->mFile, "panel_cosmicbugs");
	}
	theAnim->mRotation = Xml_GetInt(theTag, "rotation");
	if (Xml_GetString(theTag, "swap1") != nullptr)
		CopyString(theAnim->mSwap1, sizeof(theAnim->mSwap1), Xml_GetString(theTag, "swap1"));
	if (Xml_GetString(theTag, "swap2") != nullptr)
		CopyString(theAnim->mSwap2, sizeof(theAnim->mSwap2), Xml_GetString(theTag, "swap2"));
	if (Xml_GetString(theTag, "swap3") != nullptr)
		CopyString(theAnim->mSwap3, sizeof(theAnim->mSwap3), Xml_GetString(theTag, "swap3"));
	return Animation_Load(theAnim);
}

} // namespace cb
