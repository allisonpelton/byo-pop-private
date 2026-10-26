// The game's start and its main loop (0x41e9b0..0x41f5d0): the art and fonts, the loading screen, the game file
// (animations, sounds, levels, enemies, emitters, fonts, songs and level sets), and the loop that steps the game once
// for each millisecond passed and draws it. Then the game file's fonts (0x420ff0, 0x421080) and its translations
// (0x4212f0).

#include "WBEmitter.h"
#include "WBLevel.h"
#include "R64Sys.h"

#include <cstdio>
#include <cstring>

namespace wb
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

// 0x42bda0
void SeedCrtRand()
{
	CrtSrand(TimeMs());
}

// 0x41eb40: what the window's closing does (the original stops the sounds unless 0x46e5cc, which nothing sets): the
// page asked for goes to the browser
void GameExit()
{
	Sound_StopAll();
	if (gUrl[0] != 0)
		OpenURL(gUrl);
}

// 0x41ecd0: after the screen was opened again
void ScreenChanged()
{
	Dirty_AddClipped(&gDirty, 0, 0, 800, 600, kScreenClip);
}

// 0x41ecf0
void Animation_Append(List<Animation>* theList)
{
	List_Append(theList, new Animation());
}

// 0x41ed80
void Level_Append(List<Level>* theList)
{
	Level* aLevel = (Level*)::operator new(sizeof(Level));
	List_Append(theList, Level_Init(aLevel));
}

// 0x41ee10
void LevelSet_Append(List<LevelSet>* theList)
{
	List_Append(theList, new LevelSet());
}

// 0x420ff0
void FontEntry_Append(List<FontEntry>* theList)
{
	List_Append(theList, new FontEntry());
}

// A translation of local.xml (0x208 bytes)
struct LocalString
{
	LocalString* next;         // +0x000
	LocalString* prev;         // +0x004
	char index[0x100];         // +0x008
	char value[0x100];         // +0x108
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
	gScreenPitch = gScreenLock.pitch;
	gScreenBits = gScreenLock.bits;
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
	Bank_Load(&gArt8Bank, "water_art8.bbk", 0, Sys_ScreenFormat());
}

void LoadArtAndFonts()
{
	Bank_Load(&gArtBank, "water_art.bbk", 1, Sys_ScreenFormat());
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
		std::memset(gScreenBits, 0, gScreenPitch * 600);
		Draw_FillBlend(gScreenBits, gScreenPitch, &gBackBuffer.fmt, 0, 0, 0x31f, 0x257, 0, 0, 0, 0xb4);
		Image_BlitSubRect(anImage, gScreenBits, gScreenPitch, Sys_ScreenFormat(), 400 - anImage->w / 2,
			300 - anImage->h / 2, 0, 0, anImage->w, anImage->h);
		UnlockScreen();
	}
	Present(0);
}

// The levels are numbered in the file's order (a skipped one shares the next one's number); random_levels makes
// "number" of them. The file's buffer stays (as the original's does).
// 0x41eeb0
void LoadGameFile()
{
	XmlTag aTag;
	size_t aSize;
	int aNumber = 0;
	char* anEco = std::strstr(gGameFile, ".eco");
	if (anEco != nullptr)
		std::memcpy(anEco, ".xml", 4);
	const char* aData = (const char*)ReadWholeFile(gGameFile, &aSize);
	const char* p = Xml_NextTag(aData, &aTag);
	while (std::strcmp(aTag.name, "/game") != 0)
	{
		if (std::strcmp(aTag.name, "animation") == 0)
		{
			Animation_Append(&gAnims);
			if (Animation_FromXml(gAnims.current, &aTag) == 0)
				List_RemoveCurrent(&gAnims);
		}
		if (std::strcmp(aTag.name, "sound") == 0)
			Sound_Load(Xml_GetString(&aTag, "file"));
		if (std::strcmp(aTag.name, "random_levels") == 0)
		{
			if (Xml_GetInt(&aTag, "start_seed") != 0)
				CrtSrand(Xml_GetInt(&aTag, "start_seed"));
			int aCount = Xml_GetInt(&aTag, "number");
			if (0 < aCount)
			{
				do
				{
					Level_Append(&gLevels);
					aNumber = aNumber + 1;
					gLevels.current->number = aNumber;
					Level_Load(gLevels.current, nullptr, &aTag);
					aCount = aCount + -1;
				} while (aCount != 0);
			}
		}
		if (std::strcmp(aTag.name, "level") == 0)
		{
			Level_Append(&gLevels);
			if (Xml_GetInt(&aTag, "skip") == 0)
				aNumber = aNumber + 1;
			else
				gLevels.current->musicPending = 1;
			gLevels.current->number = aNumber;
			Level_Load(gLevels.current, p, &aTag);
		}
		else if (std::strcmp(aTag.name, "enemy") == 0)
		{
			Enemy_Append(&gEnemyDefs);
			Enemy_FromXml(gEnemyDefs.current, p, &aTag);
		}
		else if (std::strcmp(aTag.name, "emitter") == 0)
		{
			Emitter_Append(&gEnemyDefs.current->emitters);
			Emitter_FromXml((Emitter*)gEnemyDefs.current->emitters.current, &aTag);
		}
		else if (std::strcmp(aTag.name, "font") == 0)
			Font_LoadXml(p, &aTag);
		else if (std::strcmp(aTag.name, "song") == 0)
		{
			Music_Load(Xml_GetString(&aTag, "name"));
			Music_SetVolume(0xff);
			Music_Play();
		}
		else if (std::strcmp(aTag.name, "next_file") == 0)
		{
			// (the original's next file after main is bugs, whatever main names)
			std::strcpy(gGameFile, Xml_GetString(&aTag, "name"));
			if (std::strcmp(gGameFile, "wanims\\main.eco") == 0)
				std::strcpy(gGameFile, "wanims\\water_bugs.eco");
		}
		else if (std::strcmp(aTag.name, "level_set") == 0)
		{
			LevelSet_Append(&gLevelSets);
			gLevelSets.current->start = Xml_GetInt(&aTag, "start_range");
			gLevelSets.current->end = Xml_GetInt(&aTag, "end_range");
			std::strcpy(gLevelSets.current->song, Xml_GetString(&aTag, "song"));
		}
		p = Xml_NextTag(p, &aTag);
	}
	if (gLevels.current != nullptr)
		gLevels.current = gLevels.head;
}

void GameStart()
{
	std::strcpy(gGameFile, "wanims\\water_bugs.eco");
	gLastMode = Screen_Fullscreen() != 1 ? 1 : 0;
	// (0x4294d0 lets the window show the game's pointer: the host's)
	SeedCrtRand();
	InitSinCos();
	SeedCrtRand();
	SetScreenChangeCallback(ScreenChanged);
	Image_Create(&gBackBuffer, 800, 600, Sys_ScreenFormat());
	Dirty_Init(&gDirty, 0x28, 0x28, 1, 1);
	Dirty_SetSource(&gDirty, &gBackBuffer);
	Dirty_AddClipped(&gDirty, 0, 0, 800, 600, kScreenClip);
	LoadArt8();
	DrawLoadingScreen();
	LoadArtAndFonts();
	GameInit();
	LoadGameFile();
	Screen_Redraw();
	gLastTime = (int)TimeMs();
}

// The time passed (at most 512 ms: else the last pass's; the demo's always 202) is stepped through, then the screen
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
	if (gDemo != 0)
		gElapsedMs = 0xca;
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

// (the original first copies the screen into an image that nothing reads, 0x41eb80)
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
			Font_LoadFromBank(&gFonts.current->font, &gArtBank, Xml_GetString(theTag, "face"));
		else
		{
			Font_StripCut(&gFonts.current->font, &gArtBank, Xml_GetString(theTag, "face"),
				Xml_GetString(theTag, "strip"));
			std::strcpy(gFonts.current->chars, Xml_GetString(theTag, "strip"));
		}
	}
	else
	{
		if (Xml_GetString(theTag, "strip") == nullptr)
			Font_LoadFromBank(&gFonts.current->font, &gArt8Bank, Xml_GetString(theTag, "face"));
		else
		{
			Font_StripCut(&gFonts.current->font, &gArt8Bank, Xml_GetString(theTag, "face"),
				Xml_GetString(theTag, "strip"));
			std::strcpy(gFonts.current->chars, Xml_GetString(theTag, "strip"));
		}
		char aCode = 0;
		for (Palette* aPalette = gColourCodes; aPalette < gColourCodes + 10; aPalette++)
		{
			Font_SetColour(&gFonts.current->font, aCode + '0', aPalette);
			aCode = aCode + 1;
		}
		Font_SetEscapes(&gFonts.current->font, 1);
	}
	char aCode = 0;
	for (Palette* aPalette = gColourCodes; aPalette < gColourCodes + 10; aPalette++)
	{
		Font_SetColour(&gFonts.current->font, aCode + '0', aPalette);
		aCode = aCode + 1;
	}
	Font_SetAlign(&gFonts.current->font, 0);
	Font_SetSpacing(&gFonts.current->font, Xml_GetInt(theTag, "spacing"));
	Font_SetClipped(&gFonts.current->font, 1);
	Font_SetEscapes(&gFonts.current->font, 1);
	std::strcpy(gFonts.current->name, Xml_GetString(theTag, "name"));
	std::strcpy(gFonts.current->strip, Xml_GetString(theTag, "face"));
	return 1;
}

// local.xml's string for theText (its index), read at the first call (and at each one while it has none); theText
// itself when there is none
const char* Localize(const char* theText)
{
	if (gLocalStrings.current == nullptr)
	{
		XmlTag aTag;
		size_t aSize;
		const char* p = (const char*)ReadWholeFile("wanims\\local.xml", &aSize);
		p = Xml_NextTag(p, &aTag);
		while (std::strcmp(aTag.name, "/localization") != 0)
		{
			if (std::strcmp(aTag.name, "string") == 0)
			{
				List_Append(&gLocalStrings, new LocalString());
				std::strcpy(gLocalStrings.current->index, Xml_GetString(&aTag, "index"));
				std::strcpy(gLocalStrings.current->value, Xml_GetString(&aTag, "value"));
			}
			p = Xml_NextTag(p, &aTag);
		}
	}
	for (LocalString* aString = gLocalStrings.head; aString != nullptr; aString = aString->next)
	{
		if (std::strcmp(aString->index, theText) == 0)
			return aString->value;
	}
	return theText;
}

} // namespace wb
