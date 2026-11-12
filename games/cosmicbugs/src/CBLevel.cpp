// Levels (0x40f170..0x413d10, in progress)

#include "CBLevel.h"

#include <cstdio>
#include <cstring>

namespace cb
{

List<Level> gLevels;
List<Enemy> gEnemyDefs;

// The default song and background of a level number: the song changes only at the first level of its range
void Level_SetBackground(Level* theLevel)
{
	int aNumber = theLevel->mNumber;
	const char* aSong = nullptr;
	const char* aBack;
	if (499 < aNumber)
	{
		if (aNumber == 500)
			aSong = "audio\\cosmo4.mo3";
		aBack = "background09";
	}
	else if (399 < aNumber)
	{
		if (aNumber == 400)
			aSong = "audio\\cosmo5.mo3";
		aBack = "background08";
	}
	else if (299 < aNumber)
	{
		if (aNumber == 300)
			aSong = "audio\\cosmo2.mo3";
		aBack = "background07";
	}
	else if (199 < aNumber)
	{
		if (aNumber == 200)
			aSong = "audio\\cosmo3.mo3";
		aBack = "background06";
	}
	else if (99 < aNumber)
	{
		if (aNumber == 100)
			aSong = "audio\\cosmo4.mo3";
		aBack = "background05";
	}
	else if (0x31 < aNumber)
	{
		if (aNumber == 0x32)
			aSong = "audio\\cosmo5.mo3";
		aBack = "background04";
	}
	else if (0x13 < aNumber)
	{
		if (aNumber == 0x14)
			aSong = "audio\\cosmo2.mo3";
		aBack = "background03";
	}
	else if (9 < aNumber)
	{
		if (aNumber == 10)
			aSong = "audio\\cosmo3.mo3";
		aBack = "background02";
	}
	else
		aBack = "background01";
	if (aSong != nullptr)
		std::strcpy(gSongName, aSong);
	std::strcpy(theLevel->mBackImage, aBack);
}

void Level_StartSong(Level* theLevel, int thePlay)
{
	gSongName[0] = 0;
	Level_SetBackground(theLevel);
	if (thePlay != 0 && std::strlen(gSongName) != 0)
	{
		DrawLoadingScreen();
		Music_Free();
		Music_Load(gSongName);
		Music_SetVolume(0x8c);
		Music_Play();
		gSongName[0] = 0;
	}
}

static Cell* CellAt(int theCol, int theRow)
{
	return &gLevels.mCurrent->mColumns[theCol][theRow];
}

// Blocks of solid cells, mirrored by the symmetry, until enough cells are open; then the edges, the edge cells
// with three or more solid neighbours made solid until none changes, and the cells set again from their codes
void Level_MakeRandom(Level* theLevel, int theSeed)
{
	theLevel->mFillQuota = 0x55;
	theLevel->mSeed = 0;
	SeedRand(theSeed);
	int r = RandRange(0, 0x1e);
	theLevel->mTotalCells = 0;
	theLevel->mSymmetry = (r / 10) % 3;
	int aWanted = RandRange(800, 0x4b0);
	while (theLevel->mTotalCells < aWanted)
	{
		int aHalfW = RandRange(3, 10);
		int aHalfH = RandRange(2, 6);
		int aCol = RandRange(aHalfW + 2, 0x30 - aHalfW);
		int aRow = RandRange(aHalfH + 2, 0x1d - aHalfH);
		int aClear = RandRange(0, 2);
		for (int aQuarter = 0; aQuarter < 4; aQuarter++)
		{
			if (aQuarter == 1 || aQuarter == 3)
				aCol = 0x31 - aCol;
			if (aQuarter == 2)
				aRow = 0x20 - aRow;
			bool aDraw = true;
			if (theLevel->mSymmetry == 2 && aQuarter != 2 && aQuarter != 0)
				aDraw = false;
			if (theLevel->mSymmetry == 1 && aQuarter != 0 && aQuarter != 1)
				aDraw = false;
			if (aDraw)
			{
				for (int y = aRow - aHalfH; y <= aRow + aHalfH; y++)
				{
					for (int x = aCol - aHalfW; x <= aCol + aHalfW; x++)
					{
						Cell* aCell = CellAt(x, y);
						if (aClear == 0)
						{
							if (aCell->mKind == CELL_OPEN)
							{
								aCell->mKind = CELL_SOLID;
								theLevel->mTotalCells = theLevel->mTotalCells + -1;
							}
						}
						else if (aCell->mKind == CELL_SOLID)
						{
							aCell->mKind = CELL_OPEN;
							theLevel->mTotalCells = theLevel->mTotalCells + 1;
						}
					}
				}
			}
		}
	}
	for (int y = 0; y < GRID_ROWS; y++)
	{
		for (int x = 0; x < GRID_COLUMNS; x++)
		{
			Cell* aCell = CellAt(x, y);
			aCell->mCol = x;
			aCell->mRow = y;
			if (aCell->mKind == CELL_OPEN && Cell_Touches(aCell, CELL_SOLID) != 0)
				aCell->mKind = CELL_EDGE;
		}
	}
	bool aChanged;
	do
	{
		aChanged = false;
		for (int y = 0; y < GRID_ROWS; y++)
		{
			for (int x = 0; x < GRID_COLUMNS; x++)
			{
				Cell* aCell = &theLevel->mColumns[x][y];
				if (aCell->mKind == CELL_EDGE)
				{
					unsigned int aSolid = 0;
					if (0 < x && theLevel->mColumns[x - 1][y].mKind == CELL_SOLID)
						aSolid = 1;
					if (x + 1 < GRID_COLUMNS && theLevel->mColumns[x + 1][y].mKind == CELL_SOLID)
						aSolid = aSolid + 1;
					if (0 < y && aCell[-1].mKind == CELL_SOLID)
						aSolid = aSolid + 1;
					if (y + 1 < GRID_ROWS && aCell[1].mKind == CELL_SOLID)
						aSolid = aSolid + 1;
					if (2 < aSolid)
					{
						aCell->mKind = CELL_SOLID;
						aCell->mImage = nullptr;
						aChanged = true;
					}
				}
			}
		}
	} while (aChanged);
	theLevel->mTotalCells = 0;
	for (int y = 0; y < GRID_ROWS; y++)
	{
		for (int x = 0; x < GRID_COLUMNS; x++)
		{
			Cell* aCell = CellAt(x, y);
			int aKind = aCell->mKind;
			if (aKind == CELL_OPEN)
				Cell_Set(aCell, ':', x, y);
			else if (aKind == CELL_EDGE)
				Cell_Set(aCell, 'X', x, y);
			else if (aKind == CELL_SOLID)
				Cell_Set(aCell, ' ', x, y);
		}
	}
}

void Level_EnemyCounts(Level* theLevel, int* theA, int* theB, int* theC, int* theD)
{
	int aNumber = theLevel->mNumber;
	if (aNumber == 1)
	{
		*theA = 0;
		*theB = 1;
		*theC = 0;
		*theD = 0;
		return;
	}
	if (aNumber == 2)
	{
		*theA = 0;
		*theB = 2;
		*theC = 0;
		*theD = 0;
		return;
	}
	if (aNumber == 3)
	{
		*theA = 4;
		*theB = 2;
		*theC = 0;
		*theD = 0;
		return;
	}
	if (aNumber == 4)
	{
		*theA = 2;
		*theB = 2;
		*theC = 0;
		*theD = 1;
		return;
	}
	if (aNumber == 5)
	{
		*theA = 4;
		*theB = 2;
		*theC = 1;
		*theD = 1;
		return;
	}
	if (5 < aNumber && aNumber < 10)
	{
		*theA = 4;
		*theB = 3;
		*theC = 1;
		*theD = 1;
		return;
	}
	if (9 < aNumber && aNumber < 0xf)
	{
		*theA = 6;
		*theB = 4;
		*theC = 2;
		*theD = 2;
		return;
	}
	if (aNumber == 0x13)
	{
		*theA = 6;
		*theB = 3;
		*theC = 2;
		*theD = 2;
		return;
	}
	if (0xe < aNumber && aNumber < 0x14)
	{
		*theA = 6;
		*theB = 4;
		*theC = 2;
		*theD = 2;
		return;
	}
	if (0x13 < aNumber && aNumber < 0x19)
	{
		*theA = 8;
		*theB = 5;
		*theC = 2;
		*theD = 3;
		return;
	}
	// (the second test of the original's 25..29 range can never be reached)
	if (0x18 < aNumber && aNumber < 0x1e)
	{
		*theA = 6;
		*theB = 6;
		*theC = 3;
		*theD = 3;
		return;
	}
	if (0x1d < aNumber && aNumber < 0x23)
	{
		*theA = 8;
		*theB = 5;
		*theC = 3;
		*theD = 4;
		return;
	}
	if (0x22 < aNumber && aNumber < 0x32)
	{
		*theA = 8;
		*theB = 6;
		*theC = 4;
		*theD = 4;
		return;
	}
	*theC = 4;
	*theD = 5;
	if (*theB < 4)
		*theB = 4;
	if (*theA < 4)
		*theA = 4;
}

void Level_MakeEdges(Level* theLevel)
{
	for (int y = 0; y < gLevels.mCurrent->mRows; y++)
	{
		for (int x = 0; x < gLevels.mCurrent->mCols; x++)
		{
			if (theLevel->mColumns[x][y].mKind == CELL_EDGE)
				Cell_PickImage(&theLevel->mColumns[x][y]);
		}
	}
}

// The digits 1.. of a pattern, as tile numbers 0..
void Level_SetPatterns(Level* theLevel, const char* theH, const char* theV)
{
	unsigned int i = 0;
	if (std::strlen(theH) != 0)
	{
		do
		{
			theLevel->mHPattern[i] = theH[i] + -0x31;
			i = i + 1;
		} while (i < std::strlen(theH));
	}
	theLevel->mHPatternCount = (int)i;
	i = 0;
	if (std::strlen(theV) != 0)
	{
		do
		{
			theLevel->mVPattern[i] = theV[i] + -0x31;
			i = i + 1;
		} while (i < std::strlen(theV));
	}
	theLevel->mVPatternCount = (int)i;
}

static void LoadBeamImages(Level* theLevel, const char* theBeam)
{
	char aName[32];
	std::sprintf(aName, "%s_horizontal", theBeam);
	theLevel->mBeamH = Bank_FindImage2(&gArtBank, aName);
	std::sprintf(aName, "%s_vertical", theBeam);
	theLevel->mBeamV = Bank_FindImage2(&gArtBank, aName);
	std::sprintf(aName, "%s_middle", theBeam);
	theLevel->mBeamMiddle = Bank_FindImage2(&gArtBank, aName);
	theLevel->mEmpty = Bank_FindImage2(&gArtBank, "empty");
}

void Level_LoadFillImages(Level* theLevel, const XmlTag* theTag)
{
	char aBeam[32];
	int aFrameTime;
	theLevel->mFillImage = Bank_FindImage2(&gArtBank, Xml_GetString(theTag, "fill_image"));
	theLevel->mHTiles = Animation_Find("horizontal_tiles", &theLevel->mHTileCount, &aFrameTime);
	theLevel->mVTiles = Animation_Find("vertical_tiles", &theLevel->mVTileCount, &aFrameTime);
	theLevel->mCornerTiles = Animation_Find("corner_tiles", &theLevel->mCornerTileCount, &aFrameTime);
	std::strcpy(aBeam, Xml_GetString(theTag, "beam_image"));
	LoadBeamImages(theLevel, aBeam);
}

void Level_DefaultFillImages(Level* theLevel)
{
	char aName[32];
	int aFrameTime;
	std::sprintf(aName, "fill%02d", theLevel->mNumber % 0x12 + 1);
	theLevel->mFillImage = Bank_FindImage2(&gArtBank, aName);
	theLevel->mHTiles = Animation_Find("horizontal_tiles", &theLevel->mHTileCount, &aFrameTime);
	theLevel->mVTiles = Animation_Find("vertical_tiles", &theLevel->mVTileCount, &aFrameTime);
	theLevel->mCornerTiles = Animation_Find("corner_tiles", &theLevel->mCornerTileCount, &aFrameTime);
	LoadBeamImages(theLevel, "beam");
}

// port: the files are looked for in the game folder through the host
int PortalCount()
{
	char aPath[256];
	int aCount = 0;
	for (int i = 1; i < 5; i++)
	{
		std::sprintf(aPath, "animations\\portal%d.tga", i);
		if (Sys_FileExists(aPath))
			aCount = aCount + 1;
	}
	return aCount;
}

void Level_RedrawCells(Level* theLevel)
{
	if (theLevel->mSeed != 0)
		return;
	for (int y = gLevels.mCurrent->mRows + -1; -1 < y; y--)
	{
		for (int x = gLevels.mCurrent->mCols + -1; -1 < x; x--)
		{
			theLevel->mColumns[x][y].mShadowBelow = 0;
			theLevel->mColumns[x][y].mShadowRight = 0;
		}
	}
	for (int y = gLevels.mCurrent->mRows + -1; -1 < y; y--)
	{
		for (int x = gLevels.mCurrent->mCols + -1; -1 < x; x--)
		{
			if (theLevel->mColumns[x][y].mImage != nullptr)
				Cell_Redraw(&theLevel->mColumns[x][y]);
		}
	}
	if (theLevel->mPanels.mCurrent != nullptr)
		Panel_Draw(theLevel->mPanels.mCurrent, 1);
}

static Font* ScreenFont(const char* theName)
{
	Font* aFont = Font_Find(theName);
	Font_SetEscapes(aFont, 1);
	Font_SetTarget(aFont, gScreenBits, gScreenPitch, &gBackBuffer.mFmt, gBackBuffer.mW, gBackBuffer.mH);
	Font_SetPalette(aFont, nullptr);
	aFont->mAlpha = 0xff;
	return aFont;
}

static void DrawCentred(Font* theFont, int theY, const char* theText)
{
	int aRect[4];
	Font_DrawOutlined(theFont, aRect, SCREEN_CENTRE_X, theY, theText);
	int aHeight = Font_StringHeight(theFont, theText) + 2;
	int aWidth = Font_StringWidth(theFont, theText) + 2;
	Dirty_AddClippedShown(&gDirty, SCREEN_CENTRE_X - 1 - Font_StringWidth(theFont, theText) / 2, theY - 1, aWidth, aHeight, SCREEN_CLIP);
}

// The original copies the lines onto the stack (the last a dword at a time)
void DrawPaused()
{
	char aText[32];
	Font* aFont = ScreenFont("main_font");
	aFont->mAlign = 2;
	std::strcpy(aText, GT(0x458e74));
	DrawCentred(aFont, 200, aText);
	std::strcpy(aText, GT(0x458e58));
	DrawCentred(aFont, 0xfa, aText);
	std::strcpy(aText, GT(0x458e44));
	DrawCentred(aFont, SCREEN_CENTRE_Y, aText);
}

int Level_IsPlaying(Level* theLevel)
{
	if ((theLevel->mKind != LEVEL_BOARD || (theLevel->mMenus.mCurrent != nullptr && theLevel->mMenus.mCurrent->mRequesters.mCurrent != nullptr) ||
			(theLevel->mPanels.mCurrent != nullptr && theLevel->mPanels.mCurrent->mY <= (float)gMouseY)) &&
		100.0f < theLevel->mAge)
		return 0;
	return 1;
}

void Enemy_Insert(List<Enemy>* theList)
{
	Enemy* anEnemy = (Enemy*)::operator new(sizeof(Enemy));
	List_InsertBefore(theList, Enemy_Init(anEnemy));
}

void Menu_Append(List<Menu>* theList)
{
	Menu* aMenu = (Menu*)::operator new(sizeof(Menu));
	List_Append(theList, Menu_Init(aMenu));
}

void Shooter_Insert(List<Shooter>* theList)
{
	Shooter* aShooter = (Shooter*)::operator new(sizeof(Shooter));
	List_InsertBefore(theList, Shooter_Init(aShooter));
}

void Level_AddKey(Level* theLevel, char theKey, int theA, int theB)
{
	List<KeyMapNode>* aList = &theLevel->mKeyMap;
	List_Append(aList, new KeyMapNode);
	aList->mCurrent->mKey = theKey;
	aList->mCurrent->mA = theA;
	aList->mCurrent->mB = theB;
}

void Level_FindKey(Level* theLevel, char theKey, int* theA, int* theB)
{
	List_Each(&theLevel->mKeyMap, [&](KeyMapNode* theNode) {
		if (theNode->mKey == theKey)
		{
			*theA = theNode->mA;
			*theB = theLevel->mKeyMap.mCurrent->mB;
		}
	});
}

void Pointer_Insert(List<Obj>* theList)
{
	Obj* aPointer = (Obj*)::operator new(sizeof(Obj));
	Obj_Init(aPointer);
	aPointer->mDepth = 0xc;
	List_InsertBefore(theList, aPointer);
}

void Pointer_Append(List<Obj>* theList)
{
	Obj* aPointer = (Obj*)::operator new(sizeof(Obj));
	Obj_Init(aPointer);
	aPointer->mDepth = 0xc;
	List_Append(theList, aPointer);
}

void Level_AddPointer(Level* theLevel)
{
	List<Obj>* aList = &theLevel->mPointers;
	Pointer_Insert(aList);
	aList->mCurrent->mAnim = Animation_Find("pointer", &aList->mCurrent->mFrameCount, &aList->mCurrent->mFrameTime);
	aList->mCurrent->mDirty = 1;
	aList->mCurrent->mDrawKind = 1;
	aList->mCurrent->mDepth = 200;
}

// The fills and the particle systems are not the level's to delete: they are unlinked
void Level_UnlinkAll(Level* theLevel)
{
	DrawList_Clear();
	while (theLevel->mFills.mCurrent != nullptr)
	{
		theLevel->mFills.mCurrent = theLevel->mFills.mHead;
		if (theLevel->mFills.mCurrent != nullptr)
			List_Unlink(&theLevel->mFills);
	}
	while (theLevel->mSystems.mCurrent != nullptr)
	{
		theLevel->mSystems.mCurrent = theLevel->mSystems.mHead;
		if (theLevel->mSystems.mCurrent != nullptr)
			List_Unlink(&theLevel->mSystems);
	}
}

Level* Level_Init(Level* theLevel)
{
	Obj_Init(theLevel);
	List_Init(&theLevel->mShooters);
	List_Init(&theLevel->mFills);
	List_Init(&theLevel->mSystems);
	List_Init(&theLevel->mPanels);
	List_Init(&theLevel->mEnemies);
	List_Init(&theLevel->mKeyMap);
	List_Init(&theLevel->mPowerups);
	List_Init(&theLevel->mStars);
	List_Init(&theLevel->mTexts);
	List_Init(&theLevel->mMenus);
	List_Init(&theLevel->mPointers);
	List_Init(&theLevel->mWeapons);
	theLevel->mTotalCells = 0;
	theLevel->mFilledCells = 0;
	theLevel->mTitle[0] = 0;
	theLevel->mKind = LEVEL_BOARD;
	theLevel->mBackground = nullptr;
	theLevel->mGameOver = 0;
	theLevel->mFillSound = 0;
	theLevel->mFillEndSound = 0;
	theLevel->mFloodedCells = 0;
	theLevel->mSeed = 0;
	theLevel->mShots = 0;
	theLevel->mLines = 0;
	gSongName[0] = 0;
	theLevel->mChain = 1;
	for (int i = 0; i < GRID_COLUMNS; i++)
		theLevel->mColumns[i] = nullptr;
	theLevel->mBonus = 0;
	theLevel->mMusicPending = 0;
	return theLevel;
}

void Level_Destroy(Level* theLevel)
{
	Level_UnlinkAll(theLevel);
	if (theLevel->mBackground != nullptr)
	{
		Image_Destroy(theLevel->mBackground);
		::operator delete(theLevel->mBackground);
	}
	for (int i = 0; i < GRID_COLUMNS; i++)
	{
		if (theLevel->mColumns[i] != nullptr)
			Cells_Delete(theLevel->mColumns[i]);
	}
	List_Clear(&theLevel->mWeapons);
	List_Clear(&theLevel->mPointers);
	List_Clear(&theLevel->mMenus);
	List_Clear(&theLevel->mTexts);
	List_Clear(&theLevel->mStars);
	List_Clear(&theLevel->mPowerups);
	List_Clear(&theLevel->mKeyMap);
	List_Clear(&theLevel->mEnemies);
	List_Clear(&theLevel->mPanels);
	List_Clear(&theLevel->mSystems);
	List_Clear(&theLevel->mFills);
	List_Clear(&theLevel->mShooters);
	Obj_Destroy(theLevel);
}

void Level_Load(Level* theLevel, const char* theCursor, XmlTag* theTag)
{
	char aType[32];
	int aRow = 0;
	if (theCursor != nullptr)
	{
		for (int i = 0; i < GRID_COLUMNS; i++)
			theLevel->mColumns[i] = Cells_New();
	}
	if (Xml_GetString(theTag, "back_image") != nullptr)
	{
		std::strcpy(theLevel->mBackImage, Xml_GetString(theTag, "back_image"));
		Image* aBack = Bank_FindImage2(&gArtBank, theLevel->mBackImage);
		theLevel->mBackground = new Image;
		Image_Copy(theLevel->mBackground, aBack);
	}
	if (theCursor != nullptr && Xml_GetString(theTag, "type") != nullptr)
	{
		std::strcpy(aType, Xml_GetString(theTag, "type"));
		if (std::strcmp(aType, "credits") == 0)
			theLevel->mKind = LEVEL_CREDITS;
		else if (std::strcmp(aType, "options") == 0)
			theLevel->mKind = LEVEL_OPTIONS;
		else if (std::strcmp(aType, "high_scores") == 0)
			theLevel->mKind = LEVEL_HIGH_SCORES;
		else if (std::strcmp(aType, "high_score_entry") == 0)
			theLevel->mKind = LEVEL_HIGH_SCORE_ENTRY;
		else if (std::strcmp(aType, "main_menu") == 0)
			theLevel->mKind = LEVEL_MAIN_MENU;
		else if (std::strcmp(aType, "select_level") == 0)
			theLevel->mKind = LEVEL_SELECT_LEVEL;
		else if (std::strcmp(aType, "select_level2") == 0)
			theLevel->mKind = LEVEL_SELECT_LEVEL2;
		// (the trial's nag screens are still read, as inert levels nothing goes to: loading them draws from the game's
		// generator, so leaving them out would move every draw after them)
		else if (std::strcmp(aType, "opening_nag") == 0)
			theLevel->mKind = LEVEL_OPENING_NAG;
		else if (std::strcmp(aType, "closing_nag") == 0)
			theLevel->mKind = LEVEL_CLOSING_NAG;
		else if (std::strcmp(aType, "expiring_nag") == 0)
			theLevel->mKind = LEVEL_EXPIRING_NAG;
		else if (std::strcmp(aType, "change_user") == 0)
			theLevel->mKind = LEVEL_CHANGE_USER;
		else if (std::strcmp(aType, "logo") == 0)
			theLevel->mKind = LEVEL_LOGO;
		else if (std::strcmp(aType, "mission_complete") == 0)
			theLevel->mKind = LEVEL_MISSION_COMPLETE;
		else if (std::strcmp(aType, "help") == 0)
			theLevel->mKind = LEVEL_HELP;
		else if (std::strcmp(aType, "help2") == 0)
			theLevel->mKind = LEVEL_HELP2;
	}
	if (theLevel->mKind == LEVEL_BOARD)
	{
		if (Xml_GetString(theTag, "name") == nullptr)
			std::sprintf(theLevel->mTitle, GT(0x459028), 4);
		else
			std::strcpy(theLevel->mTitle, Xml_GetString(theTag, "name"));
		theLevel->mRows = 0x20;
	}
	else
		theLevel->mRows = GRID_ROWS;
	theLevel->mCols = GRID_COLUMNS;
	theLevel->mFillQuota = Xml_GetInt(theTag, "fill_quota");
	theLevel->mFillSound = Sound_Load("audio\\fill.wav");
	theLevel->mFillEndSound = Sound_Load("audio\\fill_end.wav");
	const char* aV;
	const char* aH;
	if (Xml_GetString(theTag, "fill_image") == nullptr)
	{
		Level_DefaultFillImages(theLevel);
		aV = "14556456282173331456";
		aH = "145645562271333148556";
	}
	else
	{
		Level_LoadFillImages(theLevel, theTag);
		aV = Xml_GetString(theTag, "v_pattern");
		aH = Xml_GetString(theTag, "h_pattern");
	}
	Level_SetPatterns(theLevel, aH, aV);
	int aNoPanel = Xml_GetInt(theTag, "no_panel");
	int aNoStars = Xml_GetInt(theTag, "no_stars");
	while (theCursor != nullptr && std::strcmp(theTag->mName, "/level") != 0)
	{
		const char* aName = theTag->mName;
		if (std::strcmp(aName, "row") == 0 && aRow < theLevel->mRows)
		{
			if (aRow == 0)
				theLevel->mTotalCells = 0;
			const char* aCells = Xml_GetString(theTag, "cells");
			for (int x = 0; x < GRID_COLUMNS; x++)
				Cell_Set(&theLevel->mColumns[x][aRow], aCells[x], x, aRow);
			aRow = aRow + 1;
		}
		else if (std::strcmp(aName, "random_board") == 0)
		{
			theLevel->mNumWalkers = Xml_GetInt(theTag, "num_walkers");
			theLevel->mNumRollers = Xml_GetInt(theTag, "num_rollers");
			theLevel->mSeed = Xml_GetInt(theTag, "seed");
		}
		else if (std::strcmp(aName, "shooter") == 0)
		{
			Shooter_Insert(&theLevel->mShooters);
			Shooter_FromXml(theLevel->mShooters.mCurrent, theCursor, theTag);
		}
		else if (std::strcmp(aName, "panel") == 0)
		{
			Panel_Append(&theLevel->mPanels);
			Panel_FromXml(theLevel->mPanels.mCurrent, theCursor, theTag);
		}
		else if (std::strcmp(aName, "enemy") == 0)
		{
			Enemy_Insert(&theLevel->mEnemies);
			Enemy_FromXml(theLevel->mEnemies.mCurrent, theCursor, theTag);
		}
		else if (std::strcmp(aName, "emitter") == 0)
		{
			Emitter_Append(&theLevel->mEnemies.mCurrent->mEmitters);
			Emitter_FromXml(static_cast<Emitter*>(theLevel->mEnemies.mCurrent->mEmitters.mCurrent), theTag);
		}
		else if (std::strcmp(aName, "menu") == 0)
		{
			Menu_Append(&theLevel->mMenus);
			Menu_FromXml(theLevel->mMenus.mCurrent, theCursor, theTag);
		}
		else if (std::strcmp(aName, "pointer") == 0)
		{
			Pointer_Append(&theLevel->mPointers);
			Obj_FromXml(theLevel->mPointers.mCurrent, theCursor, theTag);
		}
		else if (std::strcmp(aName, "text") == 0)
		{
			// (a text for the full version's advertising, "fv", is left out)
			if (Xml_GetInt(theTag, "fv") == 0)
			{
				Text_Append(&theLevel->mTexts);
				if (Text_FromXml(theLevel->mTexts.mCurrent, theCursor, theTag) == 0)
					List_RemoveCurrent(&theLevel->mTexts);
			}
		}
		else if (std::strcmp(aName, "stars") == 0)
		{
			int aNumber = Xml_GetInt(theTag, "number");
			if (0 < aNumber)
			{
				do
				{
					Star_Append(&theLevel->mStars);
					Obj_FromXml(theLevel->mStars.mCurrent, theCursor, theTag);
					theLevel->mStars.mCurrent->mX = (float)RandRange(0, SCREEN_WIDTH);
					theLevel->mStars.mCurrent->mY = (float)RandRange(0, BOARD_HEIGHT);
					Obj* aStar = theLevel->mStars.mCurrent;
					aStar->mVx = (float)((double)RandRange(0x14, 0x32) / 25.0 * (double)aStar->mVx);
					aNumber = aNumber + -1;
				} while (aNumber != 0);
			}
		}
		theCursor = Xml_NextTag(theCursor, theTag);
	}
	if (theCursor == nullptr)
	{
		theLevel->mNumWalkers = (CrtRand() % 5) * 2 + 2;
		theLevel->mNumRollers = (CrtRand() % 5) * 2 + 2;
		theLevel->mSeed = CrtRand();
	}
	if (theLevel->mSeed == 0)
		Level_MakeEdges(theLevel);
	theLevel->mState = LEVEL_PLAYING;
	if (theLevel->mPanels.mCurrent == nullptr && theLevel->mKind == LEVEL_BOARD && aNoPanel == 0)
	{
		Panel_Append(&theLevel->mPanels);
		Panel_Setup(theLevel->mPanels.mCurrent);
	}
	if (theLevel->mShooters.mCurrent == nullptr && theLevel->mKind == LEVEL_BOARD)
	{
		Shooter_Insert(&theLevel->mShooters);
		Shooter_Setup(theLevel->mShooters.mCurrent);
	}
	if (theLevel->mStars.mCurrent == nullptr && aNoStars == 0)
		Level_AddStars(theLevel);
	if (theCursor != nullptr)
	{
		for (int y = 1; y < gLevels.mCurrent->mRows + -1; y++)
		{
			for (int x = 1; x < gLevels.mCurrent->mCols + -1; x++)
				theLevel->mColumns[x][y].mState = CELL_WAITING;
		}
		Cell_StartFill(&theLevel->mColumns[gLevels.mCurrent->mCols + -2][gLevels.mCurrent->mRows + -2]);
	}
}

static void AddText(Level* theLevel, const char* theString, int theX, int theY, int theA, int theB, int theC,
	int theD, int theE, int theF)
{
	Text_Append(&theLevel->mTexts);
	Text_Make(theLevel->mTexts.mCurrent, theString, Font_Find("main_font"), theX, theY, theA, theB, theC, theD, theE,
		theF);
}

// The board is filled: the records, the frame's cells, the texts and the bonus, and every enemy and shooter told
// to leave
static void Level_Won(Level* theLevel)
{
	char aText[64];
	int aProfile = gProfile;
	LevelRecord* aRecord = &gLevelRecords[theLevel->mNumber];
	if (theLevel->mShots < aRecord->mFewestShots)
	{
		aRecord->mFewestShots = theLevel->mShots;
		std::strcpy(aRecord->mShotsName, gProfiles[aProfile].mName);
	}
	int aFilled = (theLevel->mFilledCells * 100) / theLevel->mTotalCells;
	if (aRecord->mBestFill < aFilled)
	{
		aRecord->mBestFill = aFilled;
		std::strcpy(aRecord->mFillName, gProfiles[aProfile].mName);
	}
	int* aReached = &gProfiles[aProfile].mLevelReached;
	int n = theLevel->mNumber;
	if (*aReached < n)
	{
		*aReached = n;
	}
	SaveProfiles();
	theLevel->mState = LEVEL_WON;
	for (int y = 1; y < gLevels.mCurrent->mRows + -1; y++)
	{
		for (int x = 1; x < gLevels.mCurrent->mCols + -1; x++)
		{
			theLevel->mColumns[x][y].mState = CELL_WAITING;
			theLevel->mColumns[x][y].mSteps = 0x168;
		}
	}
	Cell_StartFill(&theLevel->mColumns[1][1]);
	List_Each(&theLevel->mTexts, [&](Text* theText) {
		if (5 < std::strlen(theText->mText) || theText->mFont == Font_Find("main_font"))
		{
			theText->mState = TEXT_FADING_OUT;
			theLevel->mTexts.mCurrent->mLifetime = 100.0f;
			theLevel->mTexts.mCurrent->mAge = 1.0f;
		}
	});
	int aBonus = ((theLevel->mFilledCells * 100) / theLevel->mTotalCells - theLevel->mFillQuota) * 100;
	theLevel->mBonus = aBonus;
	gScore = gScore + aBonus;
	std::sprintf(aText, "^1%s", Localize(GT(0x45916c)));
	AddText(theLevel, aText, SCREEN_CENTRE_X, 100, 100, 300, 0x1863c, 500, 0, 0);
	int aY;
	if (Level_EnemiesLeft(theLevel) == 0)
	{
		theLevel->mBonus = 2000;
		gScore = gScore + 2000;
		std::sprintf(aText, "^4%s", Localize(GT(0x459130)));
		AddText(theLevel, aText, SCREEN_CENTRE_X, 0xb4, 100, 300, 0xf41dc, 500, 0, 0);
		aY = 0xf0;
	}
	else
	{
		int aCount = theLevel->mFilledCells;
		int aTotal = theLevel->mTotalCells;
		std::sprintf(aText, "^1%s", Localize(GT(0x459150)));
		AddText(theLevel, aText, SCREEN_CENTRE_X, 0xaa, 100, 300, 0xf41dc, 500, 0, 0);
		std::sprintf(aText, "^4%d%s", (aCount * 100) / aTotal, "%");
		AddText(theLevel, aText, SCREEN_CENTRE_X, 200, 100, 300, 0xf41dc, 500, 0, 0);
		aY = 0xfa;
	}
	std::sprintf(aText, "^1%s", Localize(GT(0x459118)));
	AddText(theLevel, aText, SCREEN_CENTRE_X, aY, 100, 300, 0xf41dc, 500, 0, 0);
	std::sprintf(aText, "^4%d", theLevel->mBonus);
	AddText(theLevel, aText, SCREEN_CENTRE_X, aY + 0x1e, 100, 300, 0xf41dc, 500, 0, 0);
	std::sprintf(aText, "^1%s", Localize(GT(0x4590fc)));
	AddText(theLevel, aText, SCREEN_CENTRE_X, aY + 100, 100, 400, 200, 400, 200, 0);
	List_Each(&theLevel->mEnemies, [&](Enemy* theEnemy) {
		theEnemy->mState = ENEMY_FADING;
		if (theLevel->mEnemies.mCurrent->mIce != nullptr)
			theLevel->mEnemies.mCurrent->mIce->mState = 2;
	});
	List_Each(&theLevel->mWeapons, [](Obj* theObj) { theObj->mState = 2; });
	// the original steps through the enemies here, not the shooters: the current shooter is told once per enemy
	if (theLevel->mShooters.mCurrent != nullptr)
	{
		theLevel->mShooters.mCurrent = theLevel->mShooters.mHead;
		while (true)
		{
			theLevel->mShooters.mCurrent->mState = SHIP_IDLE;
			Beams_RemoveAll(&theLevel->mShooters.mCurrent->mBeams);
			Enemy* anEnemy = theLevel->mEnemies.mCurrent;
			if (anEnemy == nullptr)
				break;
			if (anEnemy->mNext == nullptr)
			{
				theLevel->mEnemies.mCurrent = theLevel->mEnemies.mHead;
				break;
			}
			theLevel->mEnemies.mCurrent = Node_Next(anEnemy);
		}
	}
}

int Level_Update(Level* theLevel)
{
	int aResult = 1;
	if (gLevels.mCurrent->mKind == LEVEL_LOGO && theLevel->mTexts.mCurrent == nullptr)
	{
		gLogoCount = gLogoCount + 1;
		gGoTo = (gLogoCount <= PortalCount()) ? 0xe : 1;
		return 0;
	}
	if (theLevel->mKind == LEVEL_SELECT_LEVEL && theLevel->mAge < 3.0f)
		Screen_Redraw();
	if (gEscape != 0 && theLevel->mKind == LEVEL_BOARD && theLevel->mState != LEVEL_PAUSED)
	{
		theLevel->mState = LEVEL_PAUSED;
		gEscape = 0;
	}
	if (theLevel->mState == LEVEL_PAUSED)
		return Level_UpdatePaused(theLevel);
	theLevel->mClicked = Mouse_Pressed(1);
	int aRight;
	if (Mouse_Pressed(2) == 0)
		aRight = Key_Pressed(0x20) != 0 ? 1 : 0;
	else
		aRight = 1;
	theLevel->mRightClicked = aRight;
	if (theLevel->mSeed == 0)
	{
		if (theLevel->mMusicPending != 0)
		{
			List_Each(&gLevelSets, [&](LevelSet* theSet) {
				if (theSet->mStart == theLevel->mNumber + 1)
				{
					std::strcpy(gSongName, theSet->mSong);
					DrawLoadingScreen();
					Music_Free();
					Music_Load(gSongName);
					Music_SetVolume(0x8c);
					Music_Play();
					gSongName[0] = 0;
				}
			});
			theLevel->mMusicPending = 0;
		}
	}
	else
		Level_StartRandom(theLevel);
	Obj_Update(theLevel);
	List_Each(&theLevel->mSystems, [&](ParticleSystem* theSystem) {
		if (ParticleSystem_Update(theSystem) == 0)
			List_Unlink(&theLevel->mSystems);
	});
	List_Each(&theLevel->mStars, [&](Obj* theStar) {
		if (Star_Update(theStar) == 0)
			List_RemoveCurrent(&theLevel->mStars);
	});
	if (theLevel->mShooters.mCurrent != nullptr)
		Shooter_Update(theLevel->mShooters.mCurrent);
	List_Each(&theLevel->mEnemies, [&](Enemy* theEnemy) {
		if (Enemy_Update(theEnemy) == 0)
			List_RemoveCurrent(&theLevel->mEnemies);
	});
	gFirstWeapon = 1;
	List_Each(&theLevel->mWeapons, [&](Weapon* theWeapon) {
		if (Weapon_Update(theWeapon) == 0)
			List_RemoveCurrent(&theLevel->mWeapons);
	});
	List_Each(&theLevel->mPointers, [&](Obj* thePointer) {
		if (Pointer_Update(thePointer) == 0)
			List_RemoveCurrent(&theLevel->mPointers);
	});
	List_Each(&theLevel->mTexts, [&](Text* theText) {
		if (Text_Update(theText) == 0)
			List_RemoveCurrent(&theLevel->mTexts);
	});
	List_Each(&theLevel->mMenus, [&](Menu* theMenu) { aResult = aResult & Menu_Update(theMenu); });
	if (theLevel->mPanels.mCurrent != nullptr)
	{
		theLevel->mPanels.mCurrent = theLevel->mPanels.mHead;
		Panel_Update(theLevel->mPanels.mHead);
	}
	if (aResult != 0)
	{
		if (theLevel->mState == LEVEL_PLAYING)
		{
			List<Cell>* aFills = &theLevel->mFills;
			if (aFills->mCurrent == nullptr)
			{
				if (theLevel->mKind == LEVEL_BOARD && theLevel->mPowerups.mCurrent == nullptr && theLevel->mTotalCells != 0 &&
					theLevel->mFillQuota <= (theLevel->mFilledCells * 100) / theLevel->mTotalCells &&
					(theLevel->mMenus.mCurrent == nullptr || theLevel->mMenus.mCurrent->mRequesters.mCurrent == nullptr))
					Level_Won(theLevel);
				else
				{
					theLevel->mChain = 1;
					theLevel->mFloodedCells = 0;
				}
			}
			else
			{
				if ((int)theLevel->mAge % 0x78 == 0 && theLevel->mKind == LEVEL_BOARD && gLevels.mCurrent->mKind != LEVEL_LOGO &&
					theLevel->mTotalCells != 0)
					Sound_Play(theLevel->mFillSound);
				List_Each(aFills, [&](Cell* theFill) {
					if (theLevel->mTotalCells != 0 && Fill_Update(theFill) == 0)
					{
						List_Unlink(aFills);
						if (List_Count(aFills) == 1 && theLevel->mKind == LEVEL_BOARD)
							Sound_Play(theLevel->mFillEndSound);
					}
				});
			}
			List_Each(&theLevel->mPowerups, [&](Powerup* thePowerup) {
				if (Powerup_Update(thePowerup) == 0)
					List_RemoveCurrent(&theLevel->mPowerups);
			});
		}
		else if (theLevel->mState == LEVEL_WON)
		{
			List<Cell>* aFills = &theLevel->mFills;
			if (aFills->mCurrent == nullptr)
			{
				if (theLevel->mTexts.mHead == nullptr)
					aResult = 0;
			}
			else
			{
				List_Each(aFills, [&](Cell* theFill) {
					if (Fill_Update(theFill) == 0)
						List_Unlink(aFills);
				});
			}
			// a text is clicked away once the one it goes with (else itself) is shown
			List_Each(&theLevel->mTexts, [&](Text* theText) {
				bool aShown = false;
				if (theText->mLoopTime == 0)
					aShown = TEXT_FADING_IN < theText->mState;
				else if (theText->mPrev != nullptr)
					aShown = 5 < theText->mPrev->mState;
				else if (theText->mNext != nullptr)
				{
					aShown = 5 < theText->mNext->mState;
					theText = theLevel->mTexts.mCurrent;
				}
				if (theLevel->mClicked != 0 && aShown)
				{
					if (theText->mState != TEXT_FADING_OUT)
						theText->mAge = 0.0f;
					theLevel->mTexts.mCurrent->mLifetime = (float)theLevel->mTexts.mCurrent->mFadeOut;
					theLevel->mTexts.mCurrent->mState = TEXT_FADING_OUT;
					theLevel->mTexts.mCurrent->mLoopTime = 0;
				}
			});
		}
	}
	if (theLevel->mGameOver != 0)
	{
		while (List_RemoveCurrent(&theLevel->mShooters))
			;
		if (theLevel->mClicked != 0)
		{
			gGoTo = (gScore <= gHighScores[7].mScore) + 2;
			return 0;
		}
	}
	return aResult;
}

void Level_Draw(Level* theLevel)
{
	if (theLevel->mSeed != 0)
		return;
	Shooter* aShooter = theLevel->mShooters.mCurrent;
	if (aShooter != nullptr && aShooter->mImage != nullptr)
		Shooter_Draw(aShooter);
	List_Each(&theLevel->mFills, [](Cell* theCell) { Cell_Redraw(theCell); });
	List_Each(&theLevel->mSystems, [](ParticleSystem* theSystem) { ParticleSystem_Draw(theSystem); });
	List_Each(&theLevel->mEnemies, [](Enemy* theEnemy) { Enemy_Draw(theEnemy); });
	List_Each(&theLevel->mWeapons, [](Weapon* theWeapon) { Weapon_Draw(theWeapon); });
	List_Each(&theLevel->mPowerups, [](Powerup* thePowerup) { Powerup_Draw(thePowerup); });
	List_Each(&theLevel->mStars, [](Obj* theStar) { Star_Draw(theStar); });
	if (theLevel->mPanels.mCurrent != nullptr)
	{
		theLevel->mPanels.mCurrent = theLevel->mPanels.mHead;
		Panel_Draw(theLevel->mPanels.mHead, theLevel->mAge < 300.0f);
	}
	List_Each(&theLevel->mMenus, [](Menu* theMenu) { Menu_Draw(theMenu); });
	DrawList_Flush();
	// the first orbiting one (state 3) says so when an enemy is too close to launch it, and ends the walk
	List_Each(&theLevel->mWeapons, [&](Weapon* theWeapon) {
		if (theWeapon->mState == WEAPON_ORBITING)
		{
			Weapon_DrawTooClose(theWeapon);
			if (theLevel->mWeapons.mCurrent != nullptr)
				theLevel->mWeapons.mCurrent = theLevel->mWeapons.mTail;
		}
	});
	List_Each(&theLevel->mTexts, [](Text* theText) { Text_Draw(theText); });
	List_Each(&theLevel->mPointers, [](Obj* thePointer) { Pointer_Draw(thePointer); });
	DrawList_Flush();
	if (theLevel->mState == LEVEL_PAUSED)
		DrawPaused();
}

// The enemies not in state 1
int Level_EnemiesLeft(Level* theLevel)
{
	int aCount = 0;
	List_Each(&theLevel->mEnemies, [&](Enemy* theEnemy) {
		if (theEnemy->mState != ENEMY_DYING)
			aCount = aCount + 1;
	});
	return aCount;
}

// 0x4111a0: a new star (a plain object) before the current one
void Star_Append(List<Obj>* theList)
{
	Obj* aStar = (Obj*)::operator new(sizeof(Obj));
	Obj_Init(aStar);
	List_InsertBefore(theList, aStar);
}

void Level_AddStars(Level* theLevel)
{
	List<Obj>* aList = &theLevel->mStars;
	int aStarsLeft = 0x28;
	do
	{
		Star_Append(aList);
		aList->mCurrent->mX = (float)RandRange(0, SCREEN_WIDTH);
		aList->mCurrent->mY = (float)RandRange(0, BOARD_HEIGHT);
		aList->mCurrent->mVx = (float)((double)RandRange(0x14, 0x32) / 25.0 * 0.035);
		aList->mCurrent->mDrawKind = 2;
		aList->mCurrent->mMidHandle = 1;
		aList->mCurrent->mDirty = 1;
		Animation* anAnim = Animation_Find("star", &aList->mCurrent->mFrameCount, &aList->mCurrent->mFrameTime);
		aStarsLeft = aStarsLeft + -1;
		aList->mCurrent->mAnim = anAnim;
	} while (aStarsLeft != 0);
}

// The first walk places mirrored pairs (kinds 7 on), the second single enemies (kinds 0 on); an enemy that finds no
// place is removed
void Level_AddRandomEnemies(Level* theLevel, int theFirst, int theSecond)
{
	int aTint = RandRange(2, 8);
	int aFirstKinds;
	int aSecondKinds;
	Level_EnemyCounts(theLevel, &theFirst, &theSecond, &aFirstKinds, &aSecondKinds);
	int aKind = theFirst;
	for (int i = 0; i < theFirst; i = i + 1)
	{
		if (i % 2 == 0)
			aKind = RandRange(ENEMY_LINE_WALKER, aFirstKinds + ENEMY_LINE_WALKER);
		List_Each(&gEnemyDefs, [&](Enemy* theDef) {
			if (theDef->mKind != aKind)
				return;
			List<Enemy>* anEnemies = &theLevel->mEnemies;
			Enemy_Append(anEnemies);
			Enemy_CopyDef(anEnemies->mCurrent, gEnemyDefs.mCurrent);
			Enemy* anEnemy = anEnemies->mCurrent;
			if (anEnemy->mPrev == nullptr || i % 2 == 0)
			{
				if (Enemy_PlaceFirst(anEnemy) == 0)
				{
					List_RemoveCurrent(anEnemies);
					i = i + 1;
				}
				else
				{
					int r = RandRange(-1, 1);
					anEnemies->mCurrent->mTint = r + aTint;
				}
			}
			else
			{
				// the second of a pair mirrors the first
				int aSymmetry = gLevels.mCurrent->mSymmetry;
				if (aSymmetry == 0)
				{
					int r = RandRange(0, 99);
					anEnemies->mCurrent->mX = (float)SCREEN_WIDTH - Node_Prev(anEnemies->mCurrent)->mX;
					if (r < 0x33)
						anEnemies->mCurrent->mY = Node_Prev(anEnemies->mCurrent)->mY;
					else
						anEnemies->mCurrent->mY = 528.0f - Node_Prev(anEnemies->mCurrent)->mY;
				}
				else if (aSymmetry == 1)
				{
					anEnemies->mCurrent->mX = (float)SCREEN_WIDTH - Node_Prev(anEnemies->mCurrent)->mX;
					anEnemies->mCurrent->mY = Node_Prev(anEnemies->mCurrent)->mY;
				}
				else if (aSymmetry == 2)
				{
					anEnemies->mCurrent->mX = (float)SCREEN_WIDTH - Node_Prev(anEnemies->mCurrent)->mX;
					anEnemies->mCurrent->mY = 528.0f - Node_Prev(anEnemies->mCurrent)->mY;
				}
				anEnemies->mCurrent->mTint = Node_Prev(anEnemies->mCurrent)->mTint;
			}
			if (gEnemyDefs.mCurrent != nullptr)
				gEnemyDefs.mCurrent = gEnemyDefs.mTail;
		});
	}
	for (int i = 0; i < theSecond; i = i + 1)
	{
		int aSecondKind = RandRange(ENEMY_ROLLER, aSecondKinds);
		List_Each(&gEnemyDefs, [&](Enemy* theDef) {
			if (theDef->mKind != aSecondKind)
				return;
			List<Enemy>* anEnemies = &theLevel->mEnemies;
			Enemy_Append(anEnemies);
			Enemy_CopyDef(anEnemies->mCurrent, gEnemyDefs.mCurrent);
			if (Enemy_Place(anEnemies->mCurrent) == 0)
				List_RemoveCurrent(anEnemies);
			else
			{
				int r = RandRange(-1, 1);
				anEnemies->mCurrent->mTint = r + aTint;
				anEnemies->mCurrent->mTint = r + aTint;
			}
			if (gEnemyDefs.mCurrent != nullptr)
				gEnemyDefs.mCurrent = gEnemyDefs.mTail;
		});
	}
}

// _strupr in the C locale
static void StrUpper(char* theText)
{
	for (char* c = theText; *c != 0; c++)
	{
		if (*c >= 'a' && *c <= 'z')
			*c = (char)(*c - 0x20);
	}
}

void Level_ShowStart(Level* theLevel)
{
	char aText[64];
	if (theLevel->mKind == LEVEL_BOARD)
	{
		std::sprintf(aText, GT(0x458ee0), (int)theLevel->mTitle[0], (int)theLevel->mTitle[1], theLevel->mNumber);
		List<Text>* aTexts = &theLevel->mTexts;
		Text_Append(aTexts);
		StrUpper(aText);
		Text_Make(aTexts->mCurrent, aText, Font_Find("main_font"), SCREEN_CENTRE_X, 200, 0, 200, 4000, 200, 0, 0);
		std::sprintf(aText, GT(0x458ec8), theLevel->mFillQuota, "%");
		Text_Append(aTexts);
		StrUpper(aText);
		Text_Make(aTexts->mCurrent, aText, Font_Find("main_font"), SCREEN_CENTRE_X, 0x104, 0, 200, 4000, 200, 0, 0);
	}
}

void Beams_RemoveAll(List<Beam>* theList)
{
	while (List_RemoveCurrent(theList))
		;
}

void Level_StartRandom(Level* theLevel)
{
	for (int i = 0; i < GRID_COLUMNS; i++)
		theLevel->mColumns[i] = Cells_New();
	Level_StartSong(theLevel, 1);
	Image* aBack = Bank_FindImage2(&gArtBank, theLevel->mBackImage);
	theLevel->mBackground = new Image;
	Image_Copy(theLevel->mBackground, aBack);
	Level_MakeRandom(theLevel, theLevel->mSeed);
	Level_AddRandomEnemies(theLevel, theLevel->mNumWalkers, theLevel->mNumRollers);
	Level_MakeEdges(theLevel);
	Screen_Redraw();
	Level_AddPointer(theLevel);
	Dirty_AddClipped(&gDirty, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_CLIP);
	if (theLevel->mNext != nullptr && Node_Next(theLevel)->mKind == LEVEL_MISSION_COMPLETE)
		gGoTo = LEVEL_MISSION_COMPLETE;
	// inside the frame the cells start as state 6 (not yet reached by the fill); the fill starts in the corner
	for (int aRow = 1; aRow < gLevels.mCurrent->mRows - 1; aRow++)
	{
		for (int aCol = 1; aCol < gLevels.mCurrent->mCols - 1; aCol++)
			theLevel->mColumns[aCol][aRow].mState = CELL_WAITING;
	}
	Cell_StartFill(&theLevel->mColumns[gLevels.mCurrent->mCols - 2][gLevels.mCurrent->mRows - 2]);
}

int Level_UpdatePaused(Level* theLevel)
{
	while (List_RemoveCurrent(&theLevel->mTexts))
		;
	if (gEscape != 0)
	{
		gGoTo = LEVEL_MAIN_MENU;
		return 0;
	}
	if (Key_Pressed(0xd) != 0)
		theLevel->mState = LEVEL_PLAYING;
	return 1;
}

void Level_GameOver(Level* theLevel)
{
	char aText[32];
	List<Text>* aTexts = &theLevel->mTexts;
	theLevel->mGameOver = (int)(int64_t)theLevel->mAge;
	while (List_RemoveCurrent(aTexts))
		;
	Text_Append(aTexts);
	int aColour = RandRange(2, 9);
	std::sprintf(aText, GT(0x458f08), aColour);
	Text_Make(aTexts->mCurrent, aText, Font_Find("main_font"), SCREEN_CENTRE_X, 0xeb, 0, 500, 999999, 200, 0, 0);
	Text_Append(aTexts);
	std::sprintf(aText, GT(0x458ef0), aColour);
	Text_Make(aTexts->mCurrent, aText, Font_Find("main_font"), SCREEN_CENTRE_X, 0x109, 100, 500, 999999, 200, 0, 0);
	SaveProfiles();
}

void Panel_Append(List<Panel>* theList)
{
	Panel* aPanel = (Panel*)::operator new(sizeof(Panel));
	Obj_Init(aPanel);
	List_Init(&aPanel->mItems);
	List_InsertBefore(theList, aPanel);
}

void Panel_Destroy(Panel* thePanel)
{
	List_Clear(&thePanel->mItems);
	Obj_Destroy(thePanel);
}

void List_DeleteNode(Panel* thePanel)
{
	Panel_Destroy(thePanel);
	::operator delete(thePanel);
}

// The select-level screen's marker (an item of kind 0x15) along the current level set, up to two levels past the
// furthest one reached
void Level_UpdateProgressStars()
{
	for (Obj* anItem = gLevels.mCurrent->mMenus.mCurrent->mItems.mHead; anItem != nullptr; anItem = anItem->mNext)
	{
		if (anItem->mKind == 0x15)
		{
			LevelSet* aSet = gLevelSets.mCurrent;
			int aLast = gProfiles[gProfile].mLevelReached + 2;
			if (aSet->mEnd <= gProfiles[gProfile].mLevelReached + 2)
				aLast = aSet->mEnd;
			int aStart = aSet->mStart;
			int aRange = aSet->mEnd - aStart;
			float x = (float)((((gLevelInSet - aStart) + 1) * 400) / aRange + 200);
			anItem->mX = x;
			float aMax = (float)(((aLast - aStart) * 400) / aRange + 200);
			if (aMax < x)
				x = aMax;
			anItem->mX = x;
			if (x < 200.0f)
				x = 200.0f;
			anItem->mX = x;
		}
	}
}

void Level_SetUserName(const char* theName)
{
	if (std::strlen(theName) != 0 && std::strcmp(theName, Localize(GT(0x458a84))) != 0)
	{
		std::strcpy(gProfiles[gProfile].mName, theName);
		gAutoFire = gProfiles[gProfile].mAutoFire;
		SaveProfiles();
	}
}

int Pointer_Update(Obj* thePointer)
{
	if (Level_IsPlaying(gLevels.mCurrent) == 0 || gLevels.mCurrent->mTotalCells == 0)
	{
		thePointer->mShadow = 1;
		thePointer->mAlpha = 0xff;
		int v;
		if (gMouseX < SCREEN_WIDTH + 1)
			v = gMouseX < 0 ? 0 : gMouseX;
		else
			v = SCREEN_WIDTH;
		thePointer->mX = (float)v;
		if (gMouseY < SCREEN_HEIGHT + 1)
			v = gMouseY < 0 ? 0 : gMouseY;
		else
			v = SCREEN_HEIGHT;
		thePointer->mY = (float)v;
		Obj_Update(thePointer);
	}
	return 1;
}

void Pointer_Draw(Obj* thePointer)
{
	if (Level_IsPlaying(gLevels.mCurrent) != 0 && gLevels.mCurrent->mTotalCells != 0)
		return;
	Obj_Draw(thePointer);
}

} // namespace cb
