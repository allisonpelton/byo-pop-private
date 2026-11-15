// The game's start and its main loop (0x41e4b0..0x41f0e0): the art and fonts, the loading screen, the game file
// (animations, sounds, levels, enemies, emitters, fonts, songs and level sets), and the loop that steps the game once
// for each millisecond passed and draws it. Then the game file's fonts (0x420b10, 0x420ba0) and its translations
// (0x420e10).

#include "CBEmitter.h"
#include "CBLevel.h"
#include "R64Sys.h"

#include <cstdio>
#include <cstring>

namespace cb
{

char gGameFile[0x100];                 // 0x4622e0
Font gFontBase;                        // 0x4700e0
Image gBackBuffer;                     // 0x472ab0
SoundList gSounds = {nullptr, nullptr, nullptr, nullptr, 0, 0}; // 0x473150
ScreenLock gScreenLock;                // 0x46e394

namespace
{

int gLastMode = 1;                     // 0x460024  the screen mode at the last pass (0x460024's 1 before the first)
int gLastElapsed = 1;                  // 0x460030
int gLastTime = 0;                     // 0x46e5d8
int gElapsedMs = 0;                    // 0x46e5dc  since the last pass: the game steps a millisecond at a time

// 0x42b8c0
void SeedCrtRand()
{
	CrtSrand(TimeMs());
}

// 0x41e640: what the window's closing does (the original stops the sounds unless 0x46e5cc, which nothing sets)
void GameExit()
{
	Sound_StopAll();
}

// 0x41e7d0: after the screen was opened again
void ScreenChanged()
{
	Dirty_AddClipped(&gDirty, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_CLIP);
}

// 0x41e7f0
void Animation_Append(List<Animation>* theList)
{
	List_Append(theList, new Animation());
}

// 0x41e880
void Level_Append(List<Level>* theList)
{
	Level* aLevel = (Level*)::operator new(sizeof(Level));
	List_Append(theList, Level_Init(aLevel));
}

// 0x41e910
void LevelSet_Append(List<LevelSet>* theList)
{
	List_Append(theList, new LevelSet());
}

// 0x420b10
void FontEntry_Append(List<FontEntry>* theList)
{
	List_Append(theList, new FontEntry());
}

// A translation of local.xml (0x208 bytes)
struct LocalString
{
	LocalString* mNext;         // +0x000
	LocalString* mPrev;         // +0x004
	char mIndex[0x100];         // +0x008
	char mValue[0x100];         // +0x108
};

List<LocalString> gLocalStrings = {nullptr, nullptr, nullptr}; // 0x6e652c

} // namespace

// The CRT's rand and srand (the per-thread seed of MSVC's)
namespace
{
uint32_t gCrtSeed = 1;
}

int Screen_Lock()
{
	if (LockScreen(&gScreenLock) == 0)
		return 0;
	gScreenPitch = gScreenLock.mPitch;
	gScreenBits = gScreenLock.mBits;
	return 1;
}

int CrtRand()
{
	gCrtSeed = gCrtSeed * 0x343fd + 0x269ec3;
	return (int)((gCrtSeed >> 16) & 0x7fff);
}

void CrtSrand(unsigned theSeed)
{
	gCrtSeed = theSeed;
}

int Sound_Load(const char* thePath)
{
	return SoundList_Load(&gSounds, thePath);
}

int Sound_Find(const char* thePath)
{
	return SoundList_Find(&gSounds, thePath);
}

void LoadArt8()
{
	Bank_Load(&gArt8Bank, "art8.bbk", 0, Sys_ScreenFormat());
}

void LoadArtAndFonts()
{
	Bank_Load(&gArtBank, "art.bbk", 1, Sys_ScreenFormat());
	Font_LoadFromBank(&gFontSmall, &gArt8Bank, "st_sfont_");
	Font_LoadFromBank(&gFontTreb11, &gArt8Bank, "treb11_");
	Font_SetSpacing(&gFontTreb11, 1);
	Font_LoadFromBank(&gFontTreb20, &gArt8Bank, "treb20_");
	Font_LoadFromBank(&gFontIcon, &gArt8Bank, "icon_");
	Font_LoadFromBank(&gFontBase, &gArtBank, "base_");
}

// The screen darkened, with the loading image in its middle
void DrawLoadingScreen()
{
	Image* anImage = Bank_ImageAt(&gArt8Bank, Bank_IndexOf(&gArt8Bank, "loading"));
	if (Screen_Lock() != 0)
	{
		std::memset(gScreenBits, 0, gScreenPitch * SCREEN_HEIGHT);
		Draw_FillBlend(gScreenBits, gScreenPitch, &gBackBuffer.mFmt, 0, 0, 0x31f, 0x257, 0, 0, 0, 0xb4);
		Image_BlitSubRect(anImage, gScreenBits, gScreenPitch, Sys_ScreenFormat(), SCREEN_CENTRE_X - anImage->mW / 2,
			SCREEN_CENTRE_Y - anImage->mH / 2, 0, 0, anImage->mW, anImage->mH);
		UnlockScreen();
	}
	Present(0);
}

// The levels are numbered in the file's order (a skipped one shares the next one's number); random_levels makes
// "number" of them, twelve times as many (the full version's). The file's buffer stays (as the original's does).
void LoadGameFile()
{
	XmlTag aTag;
	size_t aSize;
	int aNumber = 0;
	char* anEco = std::strstr(gGameFile, ".eco");
	if (anEco != nullptr)
		std::memcpy(anEco, ".xml", 4);
	const char* aData = (const char*)ReadWholeFile(gGameFile, &aSize);
	const char* aCursor = Xml_NextTag(aData, &aTag);
	while (std::strcmp(aTag.mName, "/game") != 0)
	{
		if (std::strcmp(aTag.mName, "animation") == 0)
		{
			Animation_Append(&gAnims);
			if (Animation_FromXml(gAnims.mCurrent, &aTag) == 0)
				List_RemoveCurrent(&gAnims);
		}
		if (std::strcmp(aTag.mName, "sound") == 0)
			Sound_Load(Xml_GetString(&aTag, "file"));
		if (std::strcmp(aTag.mName, "random_levels") == 0)
		{
			if (Xml_GetInt(&aTag, "start_seed") != 0)
				CrtSrand(Xml_GetInt(&aTag, "start_seed"));
			int aCount = Xml_GetInt(&aTag, "number") * 12;
			if (0 < aCount)
			{
				do
				{
					Level_Append(&gLevels);
					aNumber = aNumber + 1;
					gLevels.mCurrent->mNumber = aNumber;
					Level_Load(gLevels.mCurrent, nullptr, &aTag);
					aCount = aCount + -1;
				} while (aCount != 0);
			}
		}
		if (std::strcmp(aTag.mName, "level") == 0)
		{
			Level_Append(&gLevels);
			if (Xml_GetInt(&aTag, "skip") == 0)
				aNumber = aNumber + 1;
			else
				gLevels.mCurrent->mMusicPending = 1;
			gLevels.mCurrent->mNumber = aNumber;
			Level_Load(gLevels.mCurrent, aCursor, &aTag);
		}
		else if (std::strcmp(aTag.mName, "enemy") == 0)
		{
			Enemy_Append(&gEnemyDefs);
			Enemy_FromXml(gEnemyDefs.mCurrent, aCursor, &aTag);
		}
		else if (std::strcmp(aTag.mName, "emitter") == 0)
		{
			Emitter_Append(&gEnemyDefs.mCurrent->mEmitters);
			Emitter_FromXml((Emitter*)gEnemyDefs.mCurrent->mEmitters.mCurrent, &aTag);
		}
		else if (std::strcmp(aTag.mName, "font") == 0)
			Font_LoadXml(aCursor, &aTag);
		else if (std::strcmp(aTag.mName, "song") == 0)
		{
			Music_Load(Xml_GetString(&aTag, "name"));
			Music_SetVolume(0xff);
			Music_Play();
		}
		else if (std::strcmp(aTag.mName, "next_file") == 0)
		{
			// (the original's next file after main is bugs, whatever main names)
			std::strcpy(gGameFile, Xml_GetString(&aTag, "name"));
			if (std::strcmp(gGameFile, "animations\\main.eco") == 0)
				std::strcpy(gGameFile, "animations\\bugs.eco");
		}
		else if (std::strcmp(aTag.mName, "level_set") == 0)
		{
			LevelSet_Append(&gLevelSets);
			gLevelSets.mCurrent->mStart = Xml_GetInt(&aTag, "start_range");
			gLevelSets.mCurrent->mEnd = Xml_GetInt(&aTag, "end_range");
			std::strcpy(gLevelSets.mCurrent->mSong, Xml_GetString(&aTag, "song"));
		}
		aCursor = Xml_NextTag(aCursor, &aTag);
	}
	if (gLevels.mCurrent != nullptr)
		gLevels.mCurrent = gLevels.mHead;
}

void GameStart()
{
	std::strcpy(gGameFile, "animations\\bugs.eco");
	gLastMode = Screen_Fullscreen() != 1 ? 1 : 0;
	// (0x428ff0 lets the window show the game's pointer: the host's)
	SeedCrtRand();
	InitSinCos();
	SeedCrtRand();
	SetScreenChangeCallback(ScreenChanged);
	Image_Create(&gBackBuffer, SCREEN_WIDTH, SCREEN_HEIGHT, Sys_ScreenFormat());
	Dirty_Init(&gDirty, 0x28, 0x28, 1, 1);
	Dirty_SetSource(&gDirty, &gBackBuffer);
	Dirty_AddClipped(&gDirty, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_CLIP);
	LoadArt8();
	DrawLoadingScreen();
	LoadArtAndFonts();
	GameInit();
	LoadGameFile();
	Screen_Redraw();
	gLastTime = (int)TimeMs();
}

// The time passed (at most 512 ms: else the last pass's) is stepped through, then the screen
// drawn and shown. (The original also times each part for nothing, and calls an empty function after the drawing.)
int GameFrame()
{
	if (gQuit != 0)
		return 0;
	if (PumpMessages() == 0)
		return 1;
	gElapsedMs = (int)TimeMs() - gLastTime;
	if (gElapsedMs < 0)
		Sys_Sleep(1);
	if (0x200 < gElapsedMs)
		gElapsedMs = gLastElapsed;
	int aNow = (int)TimeMs();
	gLastElapsed = gElapsedMs;
	gLastTime = aNow;
	int aMode = Screen_Fullscreen();
	if (gLastMode != aMode)
	{
		ScreenChanged();
		gLastMode = Screen_Fullscreen();
	}
	Mouse_Update(&gMouseX, &gMouseY);
	int i = 0;
	if (0 < gElapsedMs)
	{
		do
		{
			GameStep();
			i = i + 1;
		} while (i < gElapsedMs);
	}
	GameDraw();
	Present(1);
	return gQuit == 0 ? 1 : 0;
}

// (the original first copies the screen into an image that nothing reads, 0x41e680)
void GameEnd()
{
	if (Screen_Lock() != 0)
	{
		Draw_Fill(gScreenBits, gScreenPitch, Sys_ScreenFormat(), 0, 0, 0x31f, 0x257, 0, 0, 0);
		UnlockScreen();
	}
	GameExit();
}

// A font of art.bbk (art8.bbk when 8bit) by its images' prefix (face), or cut from one image (face) at its empty
// columns into the characters of strip
int Font_LoadXml(const char*, const XmlTag* theTag)
{
	FontEntry_Append(&gFonts);
	if (Xml_GetInt(theTag, "8bit") == 0)
	{
		if (Xml_GetString(theTag, "strip") == nullptr)
			Font_LoadFromBank(&gFonts.mCurrent->mFont, &gArtBank, Xml_GetString(theTag, "face"));
		else
		{
			Font_StripCut(&gFonts.mCurrent->mFont, &gArtBank, Xml_GetString(theTag, "face"),
				Xml_GetString(theTag, "strip"));
			std::strcpy(gFonts.mCurrent->mChars, Xml_GetString(theTag, "strip"));
		}
	}
	else
	{
		if (Xml_GetString(theTag, "strip") == nullptr)
			Font_LoadFromBank(&gFonts.mCurrent->mFont, &gArt8Bank, Xml_GetString(theTag, "face"));
		else
		{
			Font_StripCut(&gFonts.mCurrent->mFont, &gArt8Bank, Xml_GetString(theTag, "face"),
				Xml_GetString(theTag, "strip"));
			std::strcpy(gFonts.mCurrent->mChars, Xml_GetString(theTag, "strip"));
		}
		char aCode = 0;
		for (Palette* aPalette = gColourCodes; aPalette < gColourCodes + 10; aPalette++)
		{
			Font_SetColour(&gFonts.mCurrent->mFont, aCode + '0', aPalette);
			aCode = aCode + 1;
		}
		Font_SetEscapes(&gFonts.mCurrent->mFont, 1);
	}
	char aCode = 0;
	for (Palette* aPalette = gColourCodes; aPalette < gColourCodes + 10; aPalette++)
	{
		Font_SetColour(&gFonts.mCurrent->mFont, aCode + '0', aPalette);
		aCode = aCode + 1;
	}
	Font_SetAlign(&gFonts.mCurrent->mFont, 0);
	Font_SetSpacing(&gFonts.mCurrent->mFont, Xml_GetInt(theTag, "spacing"));
	Font_SetEscapes(&gFonts.mCurrent->mFont, 1);
	std::strcpy(gFonts.mCurrent->mName, Xml_GetString(theTag, "name"));
	std::strcpy(gFonts.mCurrent->mStrip, Xml_GetString(theTag, "face"));
	return 1;
}

// local.xml's string for theText (its index), read at the first call (and at each one while it has none); theText
// itself when there is none
const char* Localize(const char* theText)
{
	if (gLocalStrings.mCurrent == nullptr)
	{
		XmlTag aTag;
		size_t aSize;
		const char* p = (const char*)ReadWholeFile("animations\\local.xml", &aSize);
		p = Xml_NextTag(p, &aTag);
		while (std::strcmp(aTag.mName, "/localization") != 0)
		{
			if (std::strcmp(aTag.mName, "string") == 0)
			{
				List_Append(&gLocalStrings, new LocalString());
				std::strcpy(gLocalStrings.mCurrent->mIndex, Xml_GetString(&aTag, "index"));
				std::strcpy(gLocalStrings.mCurrent->mValue, Xml_GetString(&aTag, "value"));
			}
			p = Xml_NextTag(p, &aTag);
		}
	}
	for (LocalString* aString = gLocalStrings.mHead; aString != nullptr; aString = aString->mNext)
	{
		if (std::strcmp(aString->mIndex, theText) == 0)
			return aString->mValue;
	}
	return theText;
}

} // namespace cb
