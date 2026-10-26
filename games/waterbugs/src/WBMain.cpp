// The game's flow between its screens (0x40d230..0x40da50), the players and high scores it saves in cb.dat
// (0x40cab0..0x40d030), the mouse pointers and the fonts' colour codes (0x404dd0..0x404e20), and the globals of the
// data section they keep. 0x40d0e0 and 0x40d3e0 rebuild the backgrounds and fonts after the screen's format
// changed, which the port's never does.

#include "WBLevel.h"
#include "R64Sys.h"

#include <cstdio>
#include <cstring>

namespace wb
{

// The data section's (0x460000..)
int gRegistered = 1;                   // 0x460004
int gAutoFire = 1;                    // 0x460028
int gGoTo = 1;                         // 0x460034
float gBeamSpeed = 0.14f;              // 0x460038
int gFirstWeapon = 1;                  // 0x46003c
int gLives = 6;                        // 0x460040
int gTrialLeft = 3600000;              // 0x460044
int gCursorShown = 1;                  // 0x460874
int gFrame = 1;                        // 0x460a1c
// And the zeroed (0x4623e0..)
LevelRecord gLevelRecords[1000];       // 0x4623e0
Profile gProfiles[11];                 // 0x46df60
int gProfile = 0;                      // 0x46e24c
HighScore gHighScores[9];              // 0x46e250
char gUrl[0x200];                      // 0x46e3a8
int gMouseX = 0;                       // 0x46e5bc
int gMouseY = 0;                       // 0x46e5c0
int gTrial = 0;                        // 0x46e5c4
int gDemo = 0;                         // 0x46e5c8
int gQuit = 0;                         // 0x46e5d0
int gFirstRun = 0;                     // 0x46e5d4
int gDifficulty = 0;                   // 0x46e5e0
int gEscape = 0;                       // 0x46e5e4
int gScore = 0;                        // 0x46e5e8
int gLevelInSet = 0;                   // 0x46e5ec
Font gFontTreb20;                      // 0x46e5f0  "treb20_"
Font gFontTreb11;                      // 0x46f368  "treb11_"
List<LevelSet> gLevelSets = {nullptr, nullptr, nullptr}; // 0x4700d4
List<FontEntry> gFonts = {nullptr, nullptr, nullptr}; // 0x470e4c
Font gFontIcon;                        // 0x470f80  "icon_"
Font gFontSmall;                       // 0x471d10  "st_sfont_"
Image* gCursorClick = nullptr;         // 0x4735a0
Image* gCursorArrow = nullptr;         // 0x4735ac
Image* gCursorClock = nullptr;         // 0x4735b0
Image* gCursor = nullptr;              // 0x4735b4
Palette gColourCodes[10];              // 0x4735b8
char gSongName[0x40];                  // 0x6e5ab0
int gLogoCount = 0;                    // 0x6e5af0

namespace
{

// 0x4608a0: the colour codes' three colours each: 0..200 shades from the first to the second, 200..255 from the
// second to the third
const int kColourCodes[10][9] = {
	{0, 0, 0, 3, 3, 3, 6, 6, 6},
	{0, 0, 0, 180, 180, 180, 255, 255, 255},
	{40, 0, 0, 220, 0, 0, 255, 255, 255},
	{40, 18, 0, 220, 110, 0, 255, 255, 255},
	{0, 0, 0, 220, 220, 0, 255, 255, 255},
	{10, 40, 0, 70, 220, 0, 255, 255, 255},
	{0, 40, 40, 0, 220, 220, 255, 255, 255},
	{25, 25, 50, 40, 40, 220, 255, 255, 255},
	{30, 15, 60, 110, 220, 25, 255, 255, 255},
	{40, 0, 40, 220, 0, 220, 255, 255, 255},
};

// 0x458b14..0x458ad4: the default high scores' names (the code copies each literal in place)
const uint32_t kDefaultNames[8] = {GTA(0x458b14), GTA(0x458b0c), GTA(0x458b04), GTA(0x458afc), GTA(0x458af4),
	GTA(0x458aec), GTA(0x458ae0), GTA(0x458ad4)};
const int kDefaultScores[8] = {120000, 80000, 70000, 60000, 50000, 40000, 20000, 10000};

// 0x40cfe0: the higher score first (never "equal", so CrtQsort's order of ties is the original's)
int CompareHighScores(const void* a, const void* b)
{
	return ((const HighScore*)a)->score < ((const HighScore*)b)->score ? 1 : -1;
}

} // namespace

void Cursor_Show()
{
	gCursorShown = 1;
}

void Cursor_Set(int theCursor)
{
	if (theCursor == 1)
	{
		gCursor = gCursorClick;
		return;
	}
	if (theCursor != 2)
	{
		gCursor = gCursorArrow;
		return;
	}
	gCursor = gCursorClock;
}

void Cursors_Init()
{
	gCursorArrow = Bank_FindImage2(&gArtBank, "mouse");
	gCursorClick = Bank_FindImage2(&gArtBank, "mouseclick");
	gCursorClock = Bank_FindImage2(&gArtBank, "clock");
	Cursor_Show();
	Cursor_Set(0);
	for (int i = 0; i < 10; i++)
	{
		const int* c = kColourCodes[i];
		Palette_Gradient(&gColourCodes[i], 0, 200, (uint8_t)c[0], (uint8_t)c[1], (uint8_t)c[2], (uint8_t)c[3],
			(uint8_t)c[4], (uint8_t)c[5]);
		Palette_Gradient(&gColourCodes[i], 200, 0xff, (uint8_t)c[3], (uint8_t)c[4], (uint8_t)c[5], (uint8_t)c[6],
			(uint8_t)c[7], (uint8_t)c[8]);
		const PixelFormat* aFmt = Sys_ScreenFormat();
		if (aFmt->bpp == 0x10)
			PixelFormat_PackPalette16(aFmt, gColourCodes[i].p16, gColourCodes[i].rgb, 0x100);
		else
			PixelFormat_PackPalette32(aFmt, gColourCodes[i].p32, gColourCodes[i].rgb, 0x100);
	}
	Font* aFonts[4] = {&gFontSmall, &gFontTreb11, &gFontTreb20, &gFontIcon};
	for (int i = 0; i < 4; i++)
	{
		Font* aFont = aFonts[i];
		char aCode = 0;
		for (Palette* aPalette = gColourCodes; aPalette < gColourCodes + 10; aPalette++)
		{
			Font_SetColour(aFont, aCode + '0', aPalette);
			aCode = aCode + 1;
		}
		Font_SetEscapes(aFont, 1);
	}
}

// The walk stays at the font found, else goes back to the first
Font* Font_Find(const char* theName)
{
	FontEntry* anEntry = gFonts.head;
	if (gFonts.current == nullptr)
		return nullptr;
	for (;;)
	{
		gFonts.current = anEntry;
		if (std::strcmp(theName, anEntry->name) == 0)
			return &anEntry->font;
		if (anEntry->next == nullptr)
		{
			gFonts.current = gFonts.head;
			return nullptr;
		}
		anEntry = anEntry->next;
	}
}

long FileSize(const char* thePath)
{
	long aSize = 0;
	FILE* aFile = std::fopen(thePath, "rb");
	if (aFile != nullptr)
	{
		std::fseek(aFile, 0, SEEK_END);
		aSize = std::ftell(aFile);
		std::fclose(aFile);
	}
	return aSize;
}

// The current player's level, the trial's time and the players come last; the player's game in progress is
// taken (or cleared, with no lives left) when on a board. The difficulty goes to the eleventh player's slot only
// after the write, so the file has the one before.
void SaveProfiles()
{
	FILE* aFile = std::fopen(Sys_SavePath("cb.dat"), "wb");
	if (aFile != nullptr)
	{
		std::fwrite(gHighScores, 0x24, 8, aFile);
		std::fwrite(&gProfiles[gProfile].levelReached, 4, 1, aFile);
		std::fwrite(&gTrialLeft, 4, 1, aFile);
		std::fwrite(&gAutoFire, 4, 1, aFile);
		Profile* aProfile = &gProfiles[gProfile];
		aProfile->autoFire = gAutoFire;
		if (gLevels.current != nullptr && gLevels.current->kind == 0)
		{
			if (gLives < 1)
			{
				aProfile->savedLevel = 0;
				aProfile->savedScore = 0;
				aProfile->savedLives = 0;
			}
			else
			{
				aProfile->savedLevel = gLevels.current->number;
				aProfile->savedScore = gScore;
				aProfile->savedLives = gLives;
			}
		}
		std::fwrite(&gProfile, 4, 1, aFile);
		std::fwrite(gProfiles, 0x44, 0xb, aFile);
		std::fwrite(gLevelRecords, 0x30, 1000, aFile);
		gProfiles[10].riderHits = gDifficulty;
		std::fclose(aFile);
		Sys_SaveDone();
	}
}

// The default high scores, the first player named after the account (else the exe's default name) and the
// next nine empty, the eleventh after the account too; no records
void ResetProfiles()
{
	for (int i = 0; i < 8; i++)
	{
		std::strcpy(gHighScores[i].name, GT(kDefaultNames[i]));
		gHighScores[i].score = kDefaultScores[i];
	}
	gProfile = 0;
	gProfiles[0].name[0] = 0;
	Sys_UserName(gProfiles[0].name, 0x20);
	if (std::strlen(gProfiles[0].name) == 0)
		std::strcpy(gProfiles[0].name, GT(0x458acc));
	gProfiles[0].levelReached = 0;
	gProfiles[0].autoFire = 1;
	gProfiles[0].savedLevel = 0;
	gProfiles[0].savedLives = 0;
	gProfiles[0].savedScore = 0;
	gProfiles[0].riderHits = 0;
	gProfiles[0].kills = 0;
	gProfiles[0].beamsFired = 0;
	gProfiles[0].powerupsCaught = 0;
	for (Profile* aProfile = gProfiles + 1; aProfile < gProfiles + 10; aProfile++)
	{
		std::strcpy(aProfile->name, Localize(GT(0x458ac4)));
		aProfile->levelReached = 0;
		aProfile->autoFire = gAutoFire;
		aProfile->savedLevel = 0;
		aProfile->savedLives = 0;
		aProfile->savedScore = 0;
		aProfile->riderHits = 0;
		aProfile->kills = 0;
		aProfile->beamsFired = 0;
		aProfile->powerupsCaught = 0;
	}
	Sys_UserName(gProfiles[10].name, 0x20);
	gProfiles[10].levelReached = 0;
	gProfiles[10].autoFire = 1;
	gProfiles[10].savedLevel = 0;
	gProfiles[10].savedLives = 0;
	gProfiles[10].savedScore = 0;
	gProfiles[10].riderHits = 0;
	gProfiles[10].kills = 0;
	gProfiles[10].beamsFired = 0;
	gProfiles[10].powerupsCaught = 0;
	for (LevelRecord* aRecord = gLevelRecords; aRecord < gLevelRecords + 1000; aRecord++)
	{
		aRecord->fillName[0] = 0;
		aRecord->bestFill = 0;
		aRecord->shotsName[0] = 0;
		aRecord->fewestShots = 999;
	}
	SaveProfiles();
}

// cb.dat in SaveProfiles' order, or the defaults when it is missing or not its size. The trial has at least a
// minute left.
void LoadProfiles()
{
	long aSize = FileSize(Sys_SavePath("cb.dat"));
	FILE* aFile = std::fopen(Sys_SavePath("cb.dat"), "rb");
	gHighScores[8].name[0] = 0;
	gHighScores[8].score = 0;
	if (aFile == nullptr || aSize != 0xbf9c)
	{
		if (aFile != nullptr)
			std::fclose(aFile); // (the original leaves it open)
		gFirstRun = 1;
		ResetProfiles();
	}
	else
	{
		std::fread(gHighScores, 0x24, 8, aFile);
		std::fread(&gProfiles[gProfile].levelReached, 4, 1, aFile);
		std::fread(&gTrialLeft, 4, 1, aFile);
		std::fread(&gAutoFire, 4, 1, aFile);
		std::fread(&gProfile, 4, 1, aFile);
		std::fread(gProfiles, 0x44, 0xb, aFile);
		std::fread(gLevelRecords, 0x30, 1000, aFile);
		std::fclose(aFile);
		gAutoFire = gProfiles[gProfile].autoFire;
		gDifficulty = gProfiles[10].riderHits;
		// (the original closes the file a second time here)
	}
	if (gTrialLeft < 60000)
		gTrialLeft = 60000;
}

int SortHighScores()
{
	CrtQsort(gHighScores, 9, 0x24, CompareHighScores);
	SaveProfiles();
	return 1;
}

void GameInit()
{
	Cursors_Init();
	LoadProfiles();
	gUrl[0] = 0;
	Dirty_AddClipped(&gDirty, 0, 0, 800, 600, kScreenClip);
}

// The screen's dirty parts from the back buffer, then the level over them
void GameDraw()
{
	if (Screen_Lock() != 0)
	{
		Dirty_CopyTo(&gDirty, gScreenBits, gScreenPitch);
		Draw_SetTarget(gScreenBits, gScreenPitch, gScreenLock.w, gScreenLock.h, &gBackBuffer.fmt);
		if (gLevels.current != nullptr)
			Level_Draw(gLevels.current);
		UnlockScreen();
	}
}

// Onto the level's background
void Menu_DrawToBackground(Menu* theMenu)
{
	if (theMenu->image != nullptr)
	{
		Image* anImage = theMenu->image;
		Image* aBack = gLevels.current->background;
		int aY = (int)((double)theMenu->y - (double)(anImage->h / 2));
		int aX = (int)((double)theMenu->x - (double)(anImage->w / 2));
		Image_BlitAlpha(anImage, aBack->bits, aBack->pitch, &aBack->fmt, kScreenClip, aX, aY);
	}
	if (theMenu->logo != nullptr)
	{
		Image* aLogo = theMenu->logo;
		Image* aBack = gLevels.current->background;
		Image_BlitAlpha(aLogo, aBack->bits, aBack->pitch, &aBack->fmt, kScreenClip, theMenu->logoX - aLogo->w / 2,
			theMenu->logoY - aLogo->h / 2);
	}
}

// The screen's background with its menus' images into the back buffer; the board's cells once it has run a
// little, and its panel
void Screen_Redraw()
{
	if (gLevels.current != nullptr && gLevels.current->background != nullptr && gLevels.current->seed == 0)
	{
		Image_Copy(&gBackBuffer, gLevels.current->background);
		Font_SetTarget(&gFontSmall, gBackBuffer.bits, gBackBuffer.pitch, &gBackBuffer.fmt, gBackBuffer.w,
			gBackBuffer.h);
		Font_SetClipped(&gFontSmall, 1);
		// (the background covers both texts just after, so the original never shows them; 16 is the screen's
		// bits per pixel, 0x460008)
		Font_Draw(&gFontSmall, GT(0x458b3c), 10, 0x244, 0);
		char aText[256];
		std::snprintf(aText, sizeof(aText), GT(0x458b24), 800, 600, 16);
		Font_Draw(&gFontSmall, aText, 10, 0x24e, 0);
		if (gLevels.current != nullptr)
		{
			List_Each(&gLevels.current->menus, Menu_DrawToBackground);
			Image_BlitRect(gLevels.current->background, gBackBuffer.bits, gBackBuffer.pitch, &gBackBuffer.fmt, 0, 0,
				0, 0, 800, 600);
			if (10.0f < gLevels.current->age)
				Level_RedrawCells(gLevels.current);
			if (gLevels.current->panels.current != nullptr)
				Panel_Draw(gLevels.current->panels.current, 0);
		}
		Dirty_AddClippedShown(&gDirty, 0, 0, 800, 600, kScreenClip);
	}
}

// Everything the game file made
void FreeAll()
{
	while (List_RemoveCurrent(&gEnemyDefs))
	{
	}
	while (List_RemoveCurrent(&gLevels))
	{
	}
	while (List_RemoveCurrent(&gFonts))
	{
	}
	while (List_RemoveCurrent(&gAnims))
	{
	}
	while (List_RemoveCurrent(&gLevelSets))
	{
	}
}

// The screen of kind gGoTo becomes current; the high-score entry first puts the score in the ninth place under the
// player's name, and goes on to the high scores. A screen the game file did not make (a board) has the file read
// again, after the next level set's song is loaded when the set changes. The level selection's locked levels get
// their locked animation.
void GoToScreen()
{
	if (gGoTo == 2)
	{
		gHighScores[8].score = gScore;
		gGoTo = 3;
		std::strcpy(gHighScores[8].name, gProfiles[gProfile].name);
		SortHighScores();
	}
	for (;;)
	{
		if (gLevels.current != nullptr)
		{
			for (Level* aLevel = gLevels.head;; aLevel = Node_Next(aLevel))
			{
				if (aLevel->kind == gGoTo)
				{
					gLevels.current = aLevel;
					if (gGoTo != 5)
						return;
					if (aLevel->background != nullptr)
					{
						Image_Destroy(aLevel->background);
						::operator delete(aLevel->background);
					}
					Image* aBack = Bank_FindImage2(&gArtBank, aLevel->backImage);
					aLevel->background = new Image;
					Image_Copy(aLevel->background, aBack);
					List_Each(&aLevel->menus, [](Menu* aMenu) {
						aMenu->image = nullptr;
						if (aMenu->anim != nullptr && gProfiles[gProfile].levelReached + 1 < aMenu->lockedLevel)
						{
							aMenu->anim = Animation_Find(aMenu->lockedAnim, &aMenu->frameCount, &aMenu->frameTime);
							aMenu->image = Animation_Frame(aMenu->anim, 0);
						}
						Menu_DrawToBackground(aMenu);
					});
					return;
				}
				if (aLevel->next == nullptr)
					break;
			}
			gLevels.current = gLevels.head;
		}
		DrawLoadingScreen();
		Music_Free();
		if (gLevelSets.current == nullptr || gLevelInSet == gLevelSets.current->start + -1)
		{
			FreeAll();
			LoadGameFile();
		}
		else
		{
			Music_Load(gLevelSets.current->song);
			FreeAll();
			LoadGameFile();
			if (gLevels.current->kind == 0)
			{
				Music_SetVolume(0xdc);
				Music_Play();
			}
			else
			{
				Music_SetVolume(0xff);
				Music_Play();
			}
		}
	}
}

// An update: the trial's time runs on a board without a menu in use; a screen that is left (or '.' pressed)
// gives way to gGoTo's, a board to the next level of the file read again (the levels before it dropped). The
// trial's last ten updates bring the expiring nag. Esc leaves the closing nag, and the main menu when registered,
// for good; on a board it pauses (gEscape); elsewhere it goes to the main menu (the closing nag from the main menu
// when not registered).
void GameStep()
{
	gFrame = gFrame + 1;
	if (gLevels.current != nullptr)
	{
		if (gLevels.current->kind == 0 && gTrial != 0 && gLevels.current->menus.current != nullptr &&
			gLevels.current->menus.current->requesters.current == nullptr)
			gTrialLeft = gTrialLeft + -1;
		if (Level_Update(gLevels.current) == 0 || Key_Pressed(0xbe) != 0)
		{
			if (gLevels.current->kind == 0 || gLevels.current->kind == 0xe)
				List_RemoveCurrent(&gLevels);
			else
				Level_UnlinkAll(gLevels.current);
			GoToScreen();
			if (gLevels.current->kind == 0)
			{
				for (; gLevelInSet != 0; gLevelInSet = gLevelInSet + -1)
				{
					if (gLevels.current != nullptr)
						gLevels.current = gLevels.head;
					if (gLevels.current->musicPending != 0)
						gLevelInSet = gLevelInSet + 1;
					List_RemoveCurrent(&gLevels);
					if (gLevels.current != nullptr)
						gLevels.current = gLevels.head;
				}
				Level_StartSong(gLevels.current, 0);
			}
			if (gLevels.current == nullptr)
				goto escape;
			Screen_Redraw();
		}
		if (gLevels.current != nullptr && gLevels.current->kind == 0 && gRegistered == 0 && gTrial != 0 &&
			gTrialLeft < 10)
		{
			gGoTo = 10;
			GoToScreen();
			if (gLevels.current != nullptr)
				Screen_Redraw();
		}
	}
escape:
	if (Key_Pressed(0x1b) == 0)
	{
		gEscape = 0;
		return;
	}
	if ((gLevels.current != nullptr && gLevels.current->kind == 1 && gRegistered != 0) ||
		gLevels.current->kind == 9)
	{
		gQuit = 1;
		return;
	}
	if (gLevels.current->kind == 1 && gRegistered == 0)
		gGoTo = 9;
	else
	{
		if (gLevels.current->kind == 0)
		{
			gEscape = 1;
			return;
		}
		gGoTo = 1;
	}
	GoToScreen();
	if (gLevels.current != nullptr)
		Screen_Redraw();
}

} // namespace wb
