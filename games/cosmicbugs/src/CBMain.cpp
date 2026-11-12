// The game's flow between its screens (0x40d040..0x40d860), the players and high scores it saves in cb.dat
// (0x40c8c0..0x40ce40), the mouse pointers and the fonts' colour codes (0x404d20..0x404d70), and the globals of the
// data section they keep. 0x40cef0 and 0x40d1f0 rebuild the backgrounds and fonts after the screen's format
// changed, which the port's never does.

#include "CBLevel.h"
#include "R64Sys.h"

#include <cstdio>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif

namespace cb
{

// The data section's (0x460000..)
int gAutoFire = 1;                    // 0x460028
int gGoTo = 1;                         // 0x460034
float gBeamSpeed = 0.14f;              // 0x460038
int gFirstWeapon = 1;                  // 0x46003c
int gLives = 5;                        // 0x460040
int gSaveFormatField = 3600000;        // 0x460044  cb.dat's third field: only a placeholder for the file's format
int gCursorShown = 1;                  // 0x460874
int gFrame = 1;                        // 0x460a1c
// And the zeroed (0x4623e0..)
LevelRecord gLevelRecords[1000];       // 0x4623e0
Profile gProfiles[11];                 // 0x46df60
int gProfile = 0;                      // 0x46e24c
HighScore gHighScores[9];              // 0x46e250
int gMouseX = 0;                       // 0x46e5bc
int gMouseY = 0;                       // 0x46e5c0
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
char gSongName[0x40];                  // 0x6e5aa8
int gLogoCount = 0;                    // 0x6e5ae8

namespace
{

// 0x4608a0: the colour codes' three colours each: 0..200 shades from the first to the second, 200..255 from the
// second to the third
const int COLOUR_CODES[10][9] = {
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

// 0x458ad4..0x458a94: the default high scores' names (the code copies each literal in place)
const uint32_t DEFAULT_NAMES[8] = {GTA(0x458ad4), GTA(0x458acc), GTA(0x458ac4), GTA(0x458abc), GTA(0x458ab4),
	GTA(0x458aac), GTA(0x458aa0), GTA(0x458a94)};
const int DEFAULT_SCORES[8] = {120000, 80000, 70000, 60000, 50000, 40000, 20000, 10000};

// 0x40cdf0: the higher score first (never "equal", so CrtQsort's order of ties is the original's)
int CompareHighScores(const void* a, const void* b)
{
	return ((const HighScore*)a)->mScore < ((const HighScore*)b)->mScore ? 1 : -1;
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
		const int* c = COLOUR_CODES[i];
		Palette_Gradient(&gColourCodes[i], 0, 200, (uint8_t)c[0], (uint8_t)c[1], (uint8_t)c[2], (uint8_t)c[3],
			(uint8_t)c[4], (uint8_t)c[5]);
		Palette_Gradient(&gColourCodes[i], 200, 0xff, (uint8_t)c[3], (uint8_t)c[4], (uint8_t)c[5], (uint8_t)c[6],
			(uint8_t)c[7], (uint8_t)c[8]);
		const PixelFormat* aFmt = Sys_ScreenFormat();
		if (aFmt->mBpp == 0x10)
			PixelFormat_PackPalette16(aFmt, gColourCodes[i].mP16, gColourCodes[i].mRgb, 0x100);
		else
			PixelFormat_PackPalette32(aFmt, gColourCodes[i].mP32, gColourCodes[i].mRgb, 0x100);
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
	FontEntry* anEntry = gFonts.mHead;
	if (gFonts.mCurrent == nullptr)
		return nullptr;
	for (;;)
	{
		gFonts.mCurrent = anEntry;
		if (std::strcmp(theName, anEntry->mName) == 0)
			return &anEntry->mFont;
		if (anEntry->mNext == nullptr)
		{
			gFonts.mCurrent = gFonts.mHead;
			return nullptr;
		}
		anEntry = anEntry->mNext;
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

// The current player's level, the file's third field (a placeholder: what the file held, written back as it was) and the players come last; the player's game in progress is
// taken (or cleared, with no lives left) when on a board. The difficulty goes to the eleventh player's slot only
// after the write, so the file has the one before.
void SaveProfiles()
{
	FILE* aFile = std::fopen(Sys_SavePath("cb.dat"), "wb");
	if (aFile != nullptr)
	{
		std::fwrite(gHighScores, 0x24, 8, aFile);
		std::fwrite(&gProfiles[gProfile].mLevelReached, 4, 1, aFile);
		std::fwrite(&gSaveFormatField, 4, 1, aFile);
		std::fwrite(&gAutoFire, 4, 1, aFile);
		Profile* aProfile = &gProfiles[gProfile];
		aProfile->mAutoFire = gAutoFire;
		if (gLevels.mCurrent != nullptr && gLevels.mCurrent->mKind == 0)
		{
			if (gLives < 1)
			{
				aProfile->mSavedLevel = 0;
				aProfile->mSavedScore = 0;
				aProfile->mSavedLives = 0;
			}
			else
			{
				aProfile->mSavedLevel = gLevels.mCurrent->mNumber;
				aProfile->mSavedScore = gScore;
				aProfile->mSavedLives = gLives;
			}
		}
		std::fwrite(&gProfile, 4, 1, aFile);
		std::fwrite(gProfiles, 0x44, 0xb, aFile);
		std::fwrite(gLevelRecords, 0x30, 1000, aFile);
		gProfiles[10].mRiderHits = gDifficulty;
		std::fclose(aFile);
		Sys_SaveDone();
	}
#ifdef __EMSCRIPTEN__
	SiteChanged();	// the web page's players (framework/WebProfiles.h): every change of the slots ends here
#endif
}

// The default high scores, the first player named after the account (else the exe's default name) and the
// next nine empty, the eleventh after the account too; no records
void ResetProfiles()
{
	for (int i = 0; i < 8; i++)
	{
		std::strcpy(gHighScores[i].mName, GT(DEFAULT_NAMES[i]));
		gHighScores[i].mScore = DEFAULT_SCORES[i];
	}
	gProfile = 0;
	gProfiles[0].mName[0] = 0;
	Sys_UserName(gProfiles[0].mName, 0x20);
	if (std::strlen(gProfiles[0].mName) == 0)
		std::strcpy(gProfiles[0].mName, GT(0x458a8c));
	for (char* c = gProfiles[0].mName; *c != 0; c++)
		if ('a' <= *c && *c <= 'z')
			*c = (char)(*c - 0x20);
#ifdef __EMSCRIPTEN__
	SiteFirstPlayer(gProfiles[0].mName, 0x20);	// the web page's player (framework/WebProfiles.h): under "fill" and "create" it names the first player
#endif
	gProfiles[0].mLevelReached = 0;
	gProfiles[0].mAutoFire = 1;
	gProfiles[0].mSavedLevel = 0;
	gProfiles[0].mSavedLives = 0;
	gProfiles[0].mSavedScore = 0;
	gProfiles[0].mRiderHits = 0;
	gProfiles[0].mKills = 0;
	gProfiles[0].mBeamsFired = 0;
	gProfiles[0].mPowerupsCaught = 0;
	for (Profile* aProfile = gProfiles + 1; aProfile < gProfiles + 10; aProfile++)
	{
		std::strcpy(aProfile->mName, Localize(GT(0x458a84)));
		aProfile->mLevelReached = 0;
		aProfile->mAutoFire = gAutoFire;
		aProfile->mSavedLevel = 0;
		aProfile->mSavedLives = 0;
		aProfile->mSavedScore = 0;
		aProfile->mRiderHits = 0;
		aProfile->mKills = 0;
		aProfile->mBeamsFired = 0;
		aProfile->mPowerupsCaught = 0;
	}
	Sys_UserName(gProfiles[10].mName, 0x20);
	gProfiles[10].mLevelReached = 0;
	gProfiles[10].mAutoFire = 1;
	gProfiles[10].mSavedLevel = 0;
	gProfiles[10].mSavedLives = 0;
	gProfiles[10].mSavedScore = 0;
	gProfiles[10].mRiderHits = 0;
	gProfiles[10].mKills = 0;
	gProfiles[10].mBeamsFired = 0;
	gProfiles[10].mPowerupsCaught = 0;
	for (LevelRecord* aRecord = gLevelRecords; aRecord < gLevelRecords + 1000; aRecord++)
	{
		aRecord->mFillName[0] = 0;
		aRecord->mBestFill = 0;
		aRecord->mShotsName[0] = 0;
		aRecord->mFewestShots = 999;
	}
	SaveProfiles();
}

// cb.dat in SaveProfiles' order, or the defaults when it is missing or not its size.
void LoadProfiles()
{
	long aSize = FileSize(Sys_SavePath("cb.dat"));
	FILE* aFile = std::fopen(Sys_SavePath("cb.dat"), "rb");
	gHighScores[8].mName[0] = 0;
	gHighScores[8].mScore = 0;
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
		std::fread(&gProfiles[gProfile].mLevelReached, 4, 1, aFile);
		std::fread(&gSaveFormatField, 4, 1, aFile);
		std::fread(&gAutoFire, 4, 1, aFile);
		std::fread(&gProfile, 4, 1, aFile);
		std::fread(gProfiles, 0x44, 0xb, aFile);
		std::fread(gLevelRecords, 0x30, 1000, aFile);
		std::fclose(aFile);
		gAutoFire = gProfiles[gProfile].mAutoFire;
		gDifficulty = gProfiles[10].mRiderHits;
		// (the original closes the file a second time here)
	}
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
#ifdef __EMSCRIPTEN__
	SiteStart();	// the web page's players (framework/WebProfiles.h): the slots are read; what the page queued for them is applied
#endif
	Dirty_AddClipped(&gDirty, 0, 0, 800, 600, SCREEN_CLIP);
}

// The screen's dirty parts from the back buffer, then the level over them
void GameDraw()
{
	if (Screen_Lock() != 0)
	{
		Dirty_CopyTo(&gDirty, gScreenBits, gScreenPitch);
		Draw_SetTarget(gScreenBits, gScreenPitch, gScreenLock.mW, gScreenLock.mH, &gBackBuffer.mFmt);
		if (gLevels.mCurrent != nullptr)
			Level_Draw(gLevels.mCurrent);
		UnlockScreen();
	}
}

// Onto the level's background
void Menu_DrawToBackground(Menu* theMenu)
{
	if (theMenu->mImage != nullptr)
	{
		Image* anImage = theMenu->mImage;
		Image* aBack = gLevels.mCurrent->mBackground;
		int aY = (int)((double)theMenu->mY - (double)(anImage->mH / 2));
		int aX = (int)((double)theMenu->mX - (double)(anImage->mW / 2));
		Image_BlitAlpha(anImage, aBack->mBits, aBack->mPitch, &aBack->mFmt, SCREEN_CLIP, aX, aY);
	}
	if (theMenu->mLogo != nullptr)
	{
		Image* aLogo = theMenu->mLogo;
		Image* aBack = gLevels.mCurrent->mBackground;
		Image_BlitAlpha(aLogo, aBack->mBits, aBack->mPitch, &aBack->mFmt, SCREEN_CLIP, theMenu->mLogoX - aLogo->mW / 2,
			theMenu->mLogoY - aLogo->mH / 2);
	}
}

// The screen's background with its menus' images into the back buffer; the board's cells once it has run a
// little, and its panel
void Screen_Redraw()
{
	if (gLevels.mCurrent != nullptr && gLevels.mCurrent->mBackground != nullptr && gLevels.mCurrent->mSeed == 0)
	{
		Image_Copy(&gBackBuffer, gLevels.mCurrent->mBackground);
		Font_SetTarget(&gFontSmall, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, gBackBuffer.mW,
			gBackBuffer.mH);
		// (the background covers both texts just after, so the original never shows them; 16 is the screen's
		// bits per pixel, 0x460008)
		Font_Draw(&gFontSmall, GT(0x458afc), 10, 0x244, 0);
		char aText[256];
		std::snprintf(aText, sizeof(aText), GT(0x458ae4), 800, 600, 16);
		Font_Draw(&gFontSmall, aText, 10, 0x24e, 0);
		if (gLevels.mCurrent != nullptr)
		{
			List_Each(&gLevels.mCurrent->mMenus, Menu_DrawToBackground);
			Image_BlitRect(gLevels.mCurrent->mBackground, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, 0, 0,
				0, 0, 800, 600);
			if (10.0f < gLevels.mCurrent->mAge)
				Level_RedrawCells(gLevels.mCurrent);
			if (gLevels.mCurrent->mPanels.mCurrent != nullptr)
				Panel_Draw(gLevels.mCurrent->mPanels.mCurrent, 0);
		}
		Dirty_AddClippedShown(&gDirty, 0, 0, 800, 600, SCREEN_CLIP);
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
		gHighScores[8].mScore = gScore;
		gGoTo = 3;
		std::strcpy(gHighScores[8].mName, gProfiles[gProfile].mName);
		SortHighScores();
	}
	for (;;)
	{
		if (gLevels.mCurrent != nullptr)
		{
			for (Level* aLevel = gLevels.mHead;; aLevel = Node_Next(aLevel))
			{
				if (aLevel->mKind == gGoTo)
				{
					gLevels.mCurrent = aLevel;
					if (gGoTo != 5)
						return;
					if (aLevel->mBackground != nullptr)
					{
						Image_Destroy(aLevel->mBackground);
						::operator delete(aLevel->mBackground);
					}
					Image* aBack = Bank_FindImage2(&gArtBank, aLevel->mBackImage);
					aLevel->mBackground = new Image;
					Image_Copy(aLevel->mBackground, aBack);
					List_Each(&aLevel->mMenus, [](Menu* theMenu) {
						theMenu->mImage = nullptr;
						if (theMenu->mAnim != nullptr && gProfiles[gProfile].mLevelReached + 1 < theMenu->mLockedLevel)
						{
							theMenu->mAnim = Animation_Find(theMenu->mLockedAnim, &theMenu->mFrameCount, &theMenu->mFrameTime);
							theMenu->mImage = Animation_Frame(theMenu->mAnim, 0);
						}
						Menu_DrawToBackground(theMenu);
					});
					return;
				}
				if (aLevel->mNext == nullptr)
					break;
			}
			gLevels.mCurrent = gLevels.mHead;
		}
		DrawLoadingScreen();
		Music_Free();
		if (gLevelSets.mCurrent == nullptr || gLevelInSet == gLevelSets.mCurrent->mStart + -1)
		{
			FreeAll();
			LoadGameFile();
		}
		else
		{
			Music_Load(gLevelSets.mCurrent->mSong);
			FreeAll();
			LoadGameFile();
			if (gLevels.mCurrent->mKind == 0)
			{
				Music_SetVolume(0x8c);
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

// What GameStep does when the current screen is left: a board (or the logo) is dropped, any other screen is
// unlinked, gGoTo's screen becomes current, and a board has the levels before it dropped. false when no screen is
// current afterwards (the tests' commands call this as well, CBTestCommands.cpp).
bool ChangeScreen()
{
	if (gLevels.mCurrent->mKind == 0 || gLevels.mCurrent->mKind == 0xe)
		List_RemoveCurrent(&gLevels);
	else
		Level_UnlinkAll(gLevels.mCurrent);
	GoToScreen();
	if (gLevels.mCurrent->mKind == 0)
	{
		for (; gLevelInSet != 0; gLevelInSet = gLevelInSet + -1)
		{
			if (gLevels.mCurrent != nullptr)
				gLevels.mCurrent = gLevels.mHead;
			if (gLevels.mCurrent->mMusicPending != 0)
				gLevelInSet = gLevelInSet + 1;
			List_RemoveCurrent(&gLevels);
			if (gLevels.mCurrent != nullptr)
				gLevels.mCurrent = gLevels.mHead;
		}
		Level_StartSong(gLevels.mCurrent, 0);
	}
	if (gLevels.mCurrent == nullptr)
		return false;
	Screen_Redraw();
	return true;
}

// An update: a screen that is left (or '.' pressed) gives way to gGoTo's, a board to the next level of the file read
// again (the levels before it dropped). Esc leaves the main menu for good; on a board it pauses (gEscape); elsewhere it
// goes to the main menu.
void GameStep()
{
	gFrame = gFrame + 1;
	if (gLevels.mCurrent != nullptr)
	{
		if (Level_Update(gLevels.mCurrent) == 0 || Key_Pressed(0xbe) != 0)
		{
			if (!ChangeScreen())
				goto escape;
		}
	}
escape:
	// (port: the site's "Always skip start-up logos", which the player sets on the site or the exe takes as -skiplogos: on a
	// logo screen (kind 0xe, the PopCap logo, again for each portal animation file) it goes on as Esc on it does, the original's
	// own skip, to the main menu. The logos draw no random number (the seed's series is the same with Esc and without), so the
	// game's draws afterwards are the same.)
	bool aSkipLogo = gLevels.mCurrent != nullptr && gLevels.mCurrent->mKind == 0xe && Sys_SkipLogos();
	if (Key_Pressed(0x1b) == 0 && !aSkipLogo)
	{
		gEscape = 0;
		return;
	}
	if (gLevels.mCurrent != nullptr && gLevels.mCurrent->mKind == 1)
	{
		gQuit = 1;
		return;
	}
	if (gLevels.mCurrent->mKind == 0)
	{
		gEscape = 1;
		return;
	}
	gGoTo = 1;
	GoToScreen();
	if (gLevels.mCurrent != nullptr)
		Screen_Redraw();
}

} // namespace cb
