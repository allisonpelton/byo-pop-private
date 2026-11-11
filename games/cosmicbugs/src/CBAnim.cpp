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
	uint8_t* bits;
	int size;
	int pitch;
	int width;
	int height;
	int bpp;
	uint32_t rMask;
	uint32_t gMask;
	uint32_t bMask;
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
	theTga->size = aSize;
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
	theTga->bpp = aBpp;
	theTga->width = aWidth;
	theTga->bits = aBits;
	theTga->height = aHeight;
	theTga->pitch = aWidth * aBytes;
	theTga->gMask = 0xff00;
	if (aBpp == 0x20)
	{
		theTga->rMask = 0xff0000;
		theTga->bMask = 0xff;
	}
	else
	{
		theTga->bMask = 0xff0000;
		theTga->rMask = 0xff;
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
	std::memset(palette.mRgb, 0, sizeof(palette.mRgb));
	palette.mP16 = (uint16_t*)palette.mConv;
	palette.mP32 = (uint32_t*)palette.mConv;
	std::memset(palette.mConv, 0, sizeof(palette.mConv));
	swap1[0] = 0;
	swap2[0] = 0;
	swap3[0] = 0;
	file[0] = 0;
	rotation = 0;
	for (int i = 0; i < 0x100; i++)
		frames[i] = nullptr;
	next = nullptr;
	prev = nullptr;
	name[0] = 0;
	count = 0;
	frameTime = 0;
}

Animation::~Animation()
{
	int i = 0;
	if (0 < count)
	{
		do
		{
			Image* aFrame = frames[i];
			if (aFrame != nullptr)
			{
				if (aFrame->mHasMask != 0)
				{
					Image_FreeRle(aFrame->mMask);
					if (frames[i]->mMask != nullptr)
					{
						Image_Destroy(frames[i]->mMask);
						delete frames[i]->mMask;
					}
					frames[i]->mMask = nullptr;
				}
				if (frames[i] != nullptr)
				{
					Image_Destroy(frames[i]);
					delete frames[i];
				}
				frames[i] = nullptr;
			}
			i++;
		} while (i < count);
	}
}

void Animation_FreeMasks(Animation* theAnim)
{
	int i = 0;
	if (0 < theAnim->count)
	{
		do
		{
			if (theAnim->frames[i]->mHasMask != 0)
			{
				Image* aMask = theAnim->frames[i]->mMask;
				if (aMask != nullptr)
				{
					Image_Destroy(aMask);
					delete aMask;
				}
				theAnim->frames[i]->mMask = nullptr;
				theAnim->frames[i]->mHasMask = 0;
			}
			i++;
		} while (i < theAnim->count);
	}
}

// Each tint starts as the FLIC's colours. On a 16-bit screen eight of them recolour the green-dominant entries
// (the fourth's result is worked out and dropped, as in the original, so it stays the plain colours like the
// sixth); on other screens the FLIC's own colours have their blue-dominant entries changed instead.
void Animation_BuildTints(Animation* theAnim)
{
	const PixelFormat* f = Sys_ScreenFormat();
	int i = 0;
	do
	{
		for (int k = 0; k < 10; k++)
		{
			std::memcpy((uint8_t*)theAnim->tints[k].mP32 + i * 4, (uint8_t*)theAnim->palette.mP32 + i * 4, 4);
			std::memcpy((uint8_t*)theAnim->tints[k].mP16 + i * 2, (uint8_t*)theAnim->palette.mP16 + i * 2, 2);
		}
		if (f->mBpp == 0x10)
		{
			uint32_t v;
			v = theAnim->palette.mP16[i];
			ColourShift(&v, f, 0, 1, 0, 0x3c, 0x3c, 0x3c, 0, 0, 0, 0x5a);
			theAnim->tints[0].mP16[i] = (uint16_t)v;
			v = theAnim->palette.mP16[i];
			ColourShift(&v, f, 0, 1, 0, 0xff, 0xff, 0xff, 0, 0, 0, 0x5a);
			theAnim->tints[1].mP16[i] = (uint16_t)v;
			v = theAnim->palette.mP16[i];
			ColourShift(&v, f, 0, 1, 0, 0x6e, 0x50, 0x50, 1, 0, 0, 0x5a);
			theAnim->tints[2].mP16[i] = (uint16_t)v;
			v = theAnim->palette.mP16[i];
			ColourShift(&v, f, 0, 1, 0, 0x73, 0x73, 0x3c, 1, 0, 0, 0x5a);
			v = theAnim->palette.mP16[i];
			ColourShift(&v, f, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0x5a);
			theAnim->tints[4].mP16[i] = (uint16_t)v;
			theAnim->tints[5].mP16[i] = theAnim->palette.mP16[i];
			v = theAnim->palette.mP16[i];
			ColourShift(&v, f, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0x5a);
			theAnim->tints[6].mP16[i] = (uint16_t)v;
			v = theAnim->palette.mP16[i];
			ColourShift(&v, f, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0x5a);
			theAnim->tints[7].mP16[i] = (uint16_t)v;
			v = theAnim->palette.mP16[i];
			ColourShift(&v, f, 0, 1, 0, 0x5a, 0x46, 0x6e, 1, 0, 1, 0x5a);
			theAnim->tints[8].mP16[i] = (uint16_t)v;
			v = theAnim->palette.mP16[i];
			ColourShift(&v, f, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0x5a);
			theAnim->tints[9].mP16[i] = (uint16_t)v;
		}
		else
			ColourShift(&theAnim->palette.mP32[i], f, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0x5a);
		i++;
	} while (i < 0x100);
}

Image* Animation_Frame(const Animation* theAnim, int theIndex)
{
	return theAnim->frames[theIndex];
}

Animation* Animation_Find(const char* theName, int* theCount, int* theFrameTime)
{
	Animation* anAnim = gAnims.head;
	if (gAnims.current == nullptr)
		return nullptr;
	while (true)
	{
		if (std::strcmp(anAnim->name, theName) == 0)
		{
			gAnims.current = anAnim;
			*theCount = anAnim->count;
			*theFrameTime = gAnims.current->frameTime;
			return gAnims.current;
		}
		if (anAnim->next == nullptr)
		{
			gAnims.current = gAnims.head;
			return nullptr;
		}
		anAnim = anAnim->next;
	}
}

int Animation_Load(Animation* theAnim, int theReload)
{
	char* aFile = theAnim->file;
	if (std::strstr(aFile, ".eco") == nullptr && std::strstr(aFile, ".tga") == nullptr)
	{
		int anIndex = Bank_IndexOf(&gArtBank, aFile);
		if (anIndex < 1)
		{
			Log("%s\n");
			if (anIndex == 0)
				return 0;
		}
		int i = 0;
		if (0 < theAnim->count)
		{
			do
			{
				Image* aSrc = Bank_ImageAt(&gArtBank, (uint32_t)(anIndex + i));
				if (theReload == 0)
					theAnim->frames[i] = new Image();
				Image_Copy(theAnim->frames[i], aSrc);
				if (aSrc->mHasMask != 0)
				{
					theAnim->frames[i]->mHasMask = 1;
					theAnim->frames[i]->mMask = new Image();
					Image_Copy(theAnim->frames[i]->mMask, aSrc->mMask);
				}
				i++;
			} while (i < theAnim->count);
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
			theAnim->count = aFlic->mFrames;
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
							theAnim->palette.mRgb[c * 4] = gFlicPalette[c * 3];
							theAnim->palette.mRgb[c * 4 + 1] = gFlicPalette[c * 3 + 1];
							theAnim->palette.mRgb[c * 4 + 2] = gFlicPalette[c * 3 + 2];
						}
						const PixelFormat* f = Sys_ScreenFormat();
						if (f->mBpp == 0x10)
							PixelFormat_PackPalette16(f, theAnim->palette.mP16, theAnim->palette.mRgb, 0x100);
						else
							PixelFormat_PackPalette32(f, theAnim->palette.mP32, theAnim->palette.mRgb, 0x100);
						Animation_BuildTints(theAnim);
					}
					aWork->mPalette = &theAnim->palette;
					if (theReload == 0)
						theAnim->frames[i] = new Image();
					Image_Copy(theAnim->frames[i], aWork);
					Image_SetPalette(theAnim->frames[i], &theAnim->palette);
					i++;
				} while (i < (int)(uint32_t)aFlic->mFrames);
			}
			Flic_Close(aFlic);
			FlicPath(aPath, sizeof(aPath), "animations/mask_%s", theAnim->file);
			std::vector<uint8_t> aMaskData;
			if (!Sys_ReadFile(aPath, aMaskData))
			{
				int k = 0;
				if (0 < theAnim->count)
				{
					do
					{
						k++;
						theAnim->frames[k - 1]->mHasMask = 0;
					} while (k < theAnim->count);
				}
			}
			else
			{
				std::memset(aWork->mBits, 0, (size_t)aWork->mDataSize);
				aWork->mPalette = &gGreyPalette;
				aFlic = Flic_Open(aMaskData.data(), aMaskData.size());
				theAnim->count = aFlic->mFrames;
				int k = 0;
				if (aFlic->mFrames != 0)
				{
					do
					{
						Flic_NextFrame(aFlic, aWork->mBits, (uint16_t)aWork->mPitch);
						Image* aFrame = theAnim->frames[k];
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
			PixelFormat_Init(&aFmt, aTga.bpp, aTga.rMask, aTga.gMask, aTga.bMask);
			Image* aWork = new Image();
			Image_Create(aWork, aTga.width, aTga.height, &aFmt);
			Copy8(aWork->mBits, 0, 0, aWork->mPitch, 0, aTga.bits, 0, 0, aTga.pitch, 0, aTga.width * 4, aTga.height);
			Image_ConvertTo(aWork, Sys_ScreenFormat(), nullptr);
			if (theReload == 0)
				theAnim->frames[0] = new Image();
			Image_Copy(theAnim->frames[0], aWork);
			Image_Destroy(aWork);
			delete aWork;
			PixelFormat_Init(&aFmt, 8, 0, 0, 0);
			Image* aMask = new Image();
			Image_Create(aMask, aTga.width, aTga.height, &aFmt);
			aMask->mPalette = &gGreyPalette;
			int y = 0;
			if (0 < aTga.height)
			{
				do
				{
					int x = 0;
					if (0 < aTga.width)
					{
						do
						{
							uint32_t aPixel;
							std::memcpy(&aPixel, aTga.bits + ((y * aTga.pitch) / 4 + x) * 4, 4);
							aMask->mBits[aMask->mPitch * y + x] = (uint8_t)(aPixel >> 0x18);
							x++;
						} while (x < aTga.width);
					}
					y++;
				} while (y < aTga.height);
			}
			theAnim->frames[0]->mMask = new Image();
			Image_Copy(theAnim->frames[0]->mMask, aMask);
			theAnim->frames[0]->mHasMask = 1;
			Image_Destroy(aMask);
			delete aMask;
		}
		std::free(aTga.bits);
	}
	int i = 0;
	if (0 < theAnim->count)
	{
		do
		{
			Image* aFrame = theAnim->frames[i];
			if ((aFrame->mW == aFrame->mH || theAnim->rotation == 0xb4) && theAnim->rotation != 0)
				RotateImage(aFrame, theAnim->rotation);
			if (theAnim->frames[i]->mHasMask != 0)
				Image_BuildRle(theAnim->frames[i]->mMask);
			i++;
		} while (i < theAnim->count);
	}
	const char* aSwaps[3] = {theAnim->swap1, theAnim->swap2, theAnim->swap3};
	for (const char* aSwap : aSwaps)
	{
		if (*aSwap != 0 && (i = 0, 0 < theAnim->count))
		{
			do
			{
				SwapColours(theAnim->frames[i], aSwap);
				i++;
			} while (i < theAnim->count);
		}
	}
	return 1;
}

void Animation_ReloadAll()
{
	Animation* anAnim = gAnims.head;
	if (gAnims.current != nullptr)
	{
		while (true)
		{
			gAnims.current = anAnim;
			Animation_FreeMasks(anAnim);
			Animation_Load(gAnims.current, 1);
			if (gAnims.current == nullptr)
				break;
			if (gAnims.current->next == nullptr)
			{
				gAnims.current = gAnims.head;
				return;
			}
			anAnim = gAnims.current->next;
		}
	}
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
	CopyString(theAnim->name, sizeof(theAnim->name), Xml_GetString(theTag, "name"));
	theAnim->frameTime = Xml_GetInt(theTag, "frame_time");
	theAnim->count = Xml_GetInt(theTag, "num_frames");
	CopyString(theAnim->file, sizeof(theAnim->file), Xml_GetString(theTag, "start_frame"));
	if (std::strcmp(theAnim->name, "panel") == 0)
		std::strcpy(theAnim->file, "panel_cosmicbugs");
	if (std::strcmp(theAnim->name, "nag_logo") == 0)
	{
		std::strcpy(theAnim->file, "panel_cosmicbugs");
	}
	theAnim->rotation = Xml_GetInt(theTag, "rotation");
	if (Xml_GetString(theTag, "swap1") != nullptr)
		CopyString(theAnim->swap1, sizeof(theAnim->swap1), Xml_GetString(theTag, "swap1"));
	if (Xml_GetString(theTag, "swap2") != nullptr)
		CopyString(theAnim->swap2, sizeof(theAnim->swap2), Xml_GetString(theTag, "swap2"));
	if (Xml_GetString(theTag, "swap3") != nullptr)
		CopyString(theAnim->swap3, sizeof(theAnim->swap3), Xml_GetString(theTag, "swap3"));
	return Animation_Load(theAnim, 0);
}

} // namespace cb
