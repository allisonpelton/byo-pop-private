// Levels (0x40f340..0x414150, in progress)

#include "WBLevel.h"

#include <cstdio>
#include <cstring>

namespace wb
{

List<Level> gLevels;
List<Enemy> gEnemyDefs;

// The default song and background of a level number: the song changes only at the first level of its range
void Level_SetBackground(Level* theLevel)
{
	int aLevelNumber = theLevel->mNumber;
	const char* aSong = nullptr;
	const char* aBack;
	if (499 < aLevelNumber)
	{
		if (aLevelNumber == 500)
			aSong = "audio\\cosmo4.mo3";
		aBack = "finalshowdown";
	}
	else if (399 < aLevelNumber)
	{
		if (aLevelNumber == 400)
			aSong = "audio\\cosmo5.mo3";
		aBack = "facility";
	}
	else if (299 < aLevelNumber)
	{
		if (aLevelNumber == 300)
			aSong = "audio\\cosmo2.mo3";
		aBack = "arctic";
	}
	else if (199 < aLevelNumber)
	{
		if (aLevelNumber == 200)
			aSong = "audio\\cosmo3.mo3";
		aBack = "minefield";
	}
	else if (99 < aLevelNumber)
	{
		if (aLevelNumber == 100)
			aSong = "audio\\cosmo4.mo3";
		aBack = "lostcity";
	}
	else if (0x31 < aLevelNumber)
	{
		if (aLevelNumber == 0x32)
			aSong = "audio\\cosmo5.mo3";
		aBack = "pirateship";
	}
	else if (0x13 < aLevelNumber)
	{
		if (aLevelNumber == 0x14)
			aSong = "audio\\cosmo2.mo3";
		aBack = "coralreef";
	}
	else if (9 < aLevelNumber)
	{
		if (aLevelNumber == 10)
			aSong = "audio\\cosmo3.mo3";
		aBack = "kelpjungle";
	}
	else
		aBack = "crashsite";
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
		int aHalfW = RandRange(2, 10);
		int aHalfH = RandRange(1, 6);
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

// 0x40fc90: by level number, how many walkers (theA, placed in mirrored pairs, kinds 7 on) and rollers (theB,
// singles, kinds 0 on) a random board gets, theC and theD how far the kinds of each reach above its first kind, and
// theE and theF their tints (0: one near the level's). Past the table, or for a level number of 0 or less, theA and
// theB are the level file's numbers, at least 4 each, theC and theD are 5 and 6, and the tints stay 0.
void Level_EnemyCounts(Level* theLevel, int* theA, int* theB, int* theC, int* theD, int* theE, int* theF)
{
	int aLevelNumber = theLevel->mNumber;
	if (aLevelNumber == 1)
	{
		*theA = 0;
		*theB = 0;
	}
	else if (aLevelNumber == 2)
	{
		*theA = 0;
		*theB = 1;
	}
	else if (aLevelNumber == 3)
	{
		*theA = 0;
		*theB = 2;
	}
	else if (aLevelNumber == 4 || aLevelNumber == 5 || aLevelNumber == 6)
	{
		*theA = 2;
		*theB = 2;
		*theC = 0;
		*theD = 1;
		*theE = 8;
		*theF = 5;
		return;
	}
	else if (7 <= aLevelNumber && aLevelNumber < 10)
	{
		*theA = 3;
		*theB = 2;
		*theC = 1;
		*theD = 1;
		*theE = 8;
		*theF = 5;
		return;
	}
	else if (10 <= aLevelNumber && aLevelNumber <= 0xe)
	{
		*theA = 4;
		*theB = 3;
		*theC = 1;
		*theD = 2;
		*theE = 2;
		*theF = 7;
		return;
	}
	else if (0xf <= aLevelNumber && aLevelNumber <= 0x13)
	{
		*theA = 4;
		*theB = 4;
		*theC = 2;
		*theD = 2;
		*theE = 2;
		*theF = 7;
		return;
	}
	else if (0x14 <= aLevelNumber && aLevelNumber <= 0x18)
	{
		*theA = 6;
		*theB = 4;
		*theC = 2;
		*theD = 3;
		*theE = 7;
		*theF = 6;
		return;
	}
	else if (0x19 <= aLevelNumber && aLevelNumber <= 0x1d)
	{
		*theA = 6;
		*theB = 5;
		*theC = 2;
		*theD = 3;
		*theE = 7;
		*theF = 6;
		return;
	}
	else if (0x1e <= aLevelNumber && aLevelNumber <= 0x22)
	{
		*theA = 8;
		*theB = 5;
		*theC = 3;
		*theD = 3;
		*theE = 3;
		*theF = 1;
		return;
	}
	else if (0x23 <= aLevelNumber && aLevelNumber <= 0x31)
	{
		*theA = 8;
		*theB = 6;
		*theC = 4;
		*theD = 3;
		*theE = 3;
		*theF = 1;
		return;
	}
	else if (0x32 <= aLevelNumber && aLevelNumber <= 0x3c)
	{
		*theA = 8;
		*theB = 6;
		*theC = 4;
		*theD = 4;
		*theE = 5;
		*theF = 9;
		return;
	}
	else if (0x3c <= aLevelNumber && aLevelNumber <= 0x46)
	{
		*theA = 8;
		*theB = 6;
		*theC = 5;
		*theD = 5;
		*theE = 1;
		*theF = 6;
		return;
	}
	else
	{
		*theC = 5;
		*theD = 6;
		if (*theB < 4)
			*theB = 4;
		if (*theA < 4)
			*theA = 4;
		return;
	}
	// levels 1 to 3 end here
	*theC = 0;
	*theD = 0;
	*theE = 8;
	*theF = 5;
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
	std::sprintf(aName, "fill%02d", theLevel->mNumber % 0xb + 1);
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
		std::sprintf(aPath, "wanims\\portal%d.tga", i);
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

int Level_IsPlaying(Level* theLevel)
{
	if ((theLevel->mKind != 0 ||
		(theLevel->mMenus.mCurrent != nullptr && theLevel->mMenus.mCurrent->mRequesters.mCurrent != nullptr) ||
		(theLevel->mPanels.mCurrent != nullptr && theLevel->mPanels.mCurrent->mY <= (float)gMouseY)) && 100.0f <
		theLevel->mAge)
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

// 0x418200
void Menu_Insert(List<Menu>* theList)
{
	Menu* aMenu = (Menu*)::operator new(sizeof(Menu));
	List_InsertBefore(theList, Menu_Init(aMenu));
}

void Shooter_Insert(List<Shooter>* theList)
{
	Shooter* aShooter = (Shooter*)::operator new(sizeof(Shooter));
	List_InsertBefore(theList, Shooter_Init(aShooter));
}

void Level_AddKey(Level* theLevel, char theKey, int theCol, int theRow)
{
	List<KeyMapNode>* aList = &theLevel->mKeyMap;
	List_Append(aList, new KeyMapNode);
	aList->mCurrent->mKey = theKey;
	aList->mCurrent->mCol = theCol;
	aList->mCurrent->mRow = theRow;
}

void Level_FindKey(Level* theLevel, char theKey, int* theCol, int* theRow)
{
	List_Each(&theLevel->mKeyMap, [&](KeyMapNode* theANode) {
		if (theANode->mKey == theKey)
		{
			*theCol = theANode->mCol;
			*theRow = theLevel->mKeyMap.mCurrent->mRow;
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
	theLevel->mKind = 0;
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

// 0x4011a0's delete: the level's destructor, then the memory
void List_DeleteNode(Level* theLevel)
{
	Level_Destroy(theLevel);
	::operator delete((void*)theLevel);
}

void Level_Load(Level* theLevel, const char* theXmlCursor, XmlTag* theTag)
{
	char aType[32];
	int aRow = 0;
	if (theXmlCursor != nullptr)
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
	if (theXmlCursor != nullptr && Xml_GetString(theTag, "type") != nullptr)
	{
		std::strcpy(aType, Xml_GetString(theTag, "type"));
		if (std::strcmp(aType, "credits") == 0)
			theLevel->mKind = 4;
		else if (std::strcmp(aType, "options") == 0)
			theLevel->mKind = 7;
		else if (std::strcmp(aType, "high_scores") == 0)
			theLevel->mKind = 3;
		else if (std::strcmp(aType, "high_score_entry") == 0)
			theLevel->mKind = 2;
		else if (std::strcmp(aType, "main_menu") == 0)
			theLevel->mKind = 1;
		else if (std::strcmp(aType, "select_level") == 0)
			theLevel->mKind = 5;
		else if (std::strcmp(aType, "select_level2") == 0)
			theLevel->mKind = 6;
		else if (std::strcmp(aType, "opening_nag") == 0)
			theLevel->mKind = 8;
		else if (std::strcmp(aType, "closing_nag") == 0)
			theLevel->mKind = 9;
		else if (std::strcmp(aType, "change_user") == 0)
			theLevel->mKind = 0x10;
		else if (std::strcmp(aType, "expiring_nag") == 0)
			theLevel->mKind = 10;
		else if (std::strcmp(aType, "logo") == 0)
			theLevel->mKind = 0xe;
		else if (std::strcmp(aType, "mission_complete") == 0)
			theLevel->mKind = 0xb;
		else if (std::strcmp(aType, "help") == 0)
			theLevel->mKind = 0xc;
		else if (std::strcmp(aType, "help2") == 0)
			theLevel->mKind = 0xd;
	}
	if (theLevel->mKind == 0)
	{
		if (Xml_GetString(theTag, "name") == nullptr)
			std::sprintf(theLevel->mTitle, GT(0x4591f4), 4);
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
	while (theXmlCursor != nullptr && std::strcmp(theTag->mName, "/level") != 0)
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
			Shooter_FromXml(theLevel->mShooters.mCurrent, theXmlCursor, theTag);
		}
		else if (std::strcmp(aName, "panel") == 0)
		{
			Panel_Append(&theLevel->mPanels);
			Panel_FromXml(theLevel->mPanels.mCurrent, theXmlCursor, theTag);
		}
		else if (std::strcmp(aName, "enemy") == 0)
		{
			Enemy_Insert(&theLevel->mEnemies);
			Enemy_FromXml(theLevel->mEnemies.mCurrent, theXmlCursor, theTag);
		}
		else if (std::strcmp(aName, "emitter") == 0)
		{
			Emitter_Append(&theLevel->mEnemies.mCurrent->mEmitters);
			Emitter_FromXml(static_cast<Emitter*>(theLevel->mEnemies.mCurrent->mEmitters.mCurrent), theTag);
		}
		else if (std::strcmp(aName, "menu") == 0)
		{
			Menu_Append(&theLevel->mMenus);
			Menu_FromXml(theLevel->mMenus.mCurrent, theXmlCursor, theTag);
		}
		else if (std::strcmp(aName, "pointer") == 0)
		{
			Pointer_Append(&theLevel->mPointers);
			Obj_FromXml(theLevel->mPointers.mCurrent, theXmlCursor, theTag);
		}
		else if (std::strcmp(aName, "text") == 0)
		{
			// (a text marked fv is the trial's: the registered game leaves it out)
			if (Xml_GetInt(theTag, "fv") == 0)
			{
				Text_Append(&theLevel->mTexts);
				if (Text_FromXml(theLevel->mTexts.mCurrent, theXmlCursor, theTag) == 0)
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
					Obj_FromXml(theLevel->mStars.mCurrent, theXmlCursor, theTag);
					theLevel->mStars.mCurrent->mX = (float)RandRange(0, SCREEN_WIDTH);
					theLevel->mStars.mCurrent->mY = (float)RandRange(0, 0x250);
					Obj* aStar = theLevel->mStars.mCurrent;
					aStar->mVx = (float)((double)RandRange(0x14, 0x32) / 25.0 * (double)aStar->mVx);
					aNumber = aNumber + -1;
				} while (aNumber != 0);
			}
		}
		theXmlCursor = Xml_NextTag(theXmlCursor, theTag);
	}
	if (theXmlCursor == nullptr)
	{
		theLevel->mNumWalkers = (CrtRand() % 5) * 2 + 2;
		theLevel->mNumRollers = (CrtRand() % 5) * 2 + 2;
		theLevel->mSeed = CrtRand();
	}
	if (theLevel->mSeed == 0)
		Level_MakeEdges(theLevel);
	theLevel->mState = 1;
	if (theLevel->mPanels.mCurrent == nullptr && theLevel->mKind == 0 && aNoPanel == 0)
	{
		Panel_Append(&theLevel->mPanels);
		Panel_Setup(theLevel->mPanels.mCurrent);
	}
	if (theLevel->mShooters.mCurrent == nullptr && theLevel->mKind == 0)
	{
		Shooter_Insert(&theLevel->mShooters);
		Shooter_Setup(theLevel->mShooters.mCurrent);
	}
	if (theLevel->mStars.mCurrent == nullptr && aNoStars == 0)
		Level_AddStars(theLevel);
	if (theXmlCursor != nullptr)
	{
		for (int y = 1; y < gLevels.mCurrent->mRows + -1; y++)
		{
			for (int x = 1; x < gLevels.mCurrent->mCols + -1; x++)
				theLevel->mColumns[x][y].mState = 6;
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
void Level_Won(Level* theLevel)
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
	theLevel->mState = 3;
	for (int y = 1; y < gLevels.mCurrent->mRows + -1; y++)
	{
		for (int x = 1; x < gLevels.mCurrent->mCols + -1; x++)
		{
			theLevel->mColumns[x][y].mState = 6;
			theLevel->mColumns[x][y].mSteps = 0x168;
		}
	}
	Cell_StartFill(&theLevel->mColumns[gLevels.mCurrent->mCols + -2][gLevels.mCurrent->mRows + -2]);
	List_Each(&theLevel->mTexts, [&](Text* theAText) {
		if (5 < std::strlen(theAText->mText) || theAText->mFont == Font_Find("main_font"))
		{
			theAText->mState = 7;
			theLevel->mTexts.mCurrent->mLifetime = 100.0f;
			theLevel->mTexts.mCurrent->mAge = 1.0f;
		}
	});
	int aBonus = ((theLevel->mFilledCells * 100) / theLevel->mTotalCells - theLevel->mFillQuota) * 100;
	theLevel->mBonus = aBonus;
	gScore = gScore + aBonus;
	std::sprintf(aText, "^1%s", Localize(GT(0x4592f4)));
	AddText(theLevel, aText, SCREEN_CENTRE_X, 0xf0, 100, 300, 1000, 500, 0, 0);
	List_Each(&theLevel->mEnemies, [](Enemy* theAnEnemy) { theAnEnemy->mState = ENEMY_FADING; });
	List_Each(&theLevel->mWeapons, [](Obj* theAnObj) { theAnObj->mState = 2; });
	// the original steps through the enemies here, not the shooters: the current shooter is told once per enemy
	if (theLevel->mShooters.mCurrent != nullptr)
	{
		theLevel->mShooters.mCurrent = theLevel->mShooters.mHead;
		while (true)
		{
			theLevel->mShooters.mCurrent->mState = 2;
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
	if (gLevels.mCurrent->mKind == 0xe && theLevel->mTexts.mCurrent == nullptr)
	{
		gLogoCount = gLogoCount + 1;
		gGoTo = (gLogoCount <= PortalCount()) ? 0xe : 1;
		return 0;
	}
	// Esc on a board asks whether to quit, unless a requester is open already
	if (gEscape != 0 && theLevel->mKind == 0 && theLevel->mMenus.mCurrent != nullptr &&
		theLevel->mMenus.mCurrent->mRequesters.mCurrent == nullptr)
		MenuItem_ConfirmQuit(theLevel->mMenus.mCurrent->mItems.mCurrent);
	if (theLevel->mState == 2)
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
			List_Each(&gLevelSets, [&](LevelSet* theASet) {
				if (theASet->mStart == theLevel->mNumber + 1)
				{
					std::strcpy(gSongName, theASet->mSong);
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
	List_Each(&theLevel->mSystems, [&](ParticleSystem* theASystem) {
		if (ParticleSystem_Update(theASystem) == 0)
			List_Unlink(&theLevel->mSystems);
	});
	List_Each(&theLevel->mStars, [&](Obj* theAStar) {
		if (Star_Update(theAStar) == 0)
			List_RemoveCurrent(&theLevel->mStars);
	});
	if (theLevel->mShooters.mCurrent != nullptr)
		Shooter_Update(theLevel->mShooters.mCurrent);
	List_Each(&theLevel->mEnemies, [&](Enemy* theAnEnemy) {
		if (Enemy_Update(theAnEnemy) == 0)
			List_RemoveCurrent(&theLevel->mEnemies);
	});
	gFirstWeapon = 1;
	List_Each(&theLevel->mWeapons, [&](Weapon* theAWeapon) {
		if (Weapon_Update(theAWeapon) == 0)
			List_RemoveCurrent(&theLevel->mWeapons);
	});
	List_Each(&theLevel->mPointers, [&](Obj* theAPointer) {
		if (Pointer_Update(theAPointer) == 0)
			List_RemoveCurrent(&theLevel->mPointers);
	});
	List_Each(&theLevel->mTexts, [&](Text* theAText) {
		if (Text_Update(theAText) == 0)
			List_RemoveCurrent(&theLevel->mTexts);
	});
	List_Each(&theLevel->mMenus, [&](Menu* theAMenu) { aResult = aResult & Menu_Update(theAMenu); });
	if (theLevel->mPanels.mCurrent != nullptr)
	{
		theLevel->mPanels.mCurrent = theLevel->mPanels.mHead;
		Panel_Update(theLevel->mPanels.mHead);
	}
	if (aResult != 0)
	{
		if (theLevel->mState == 1)
		{
			List<Cell>* aFills = &theLevel->mFills;
			if (aFills->mCurrent == nullptr)
			{
				if (theLevel->mKind == 0 && theLevel->mPowerups.mCurrent == nullptr && theLevel->mTotalCells != 0 &&
					theLevel->mFillQuota <= (theLevel->mFilledCells * 100) / theLevel->mTotalCells &&
					(theLevel->mMenus.mCurrent == nullptr ||
						theLevel->mMenus.mCurrent->mRequesters.mCurrent == nullptr))
					Level_Won(theLevel);
				else
				{
					theLevel->mChain = 1;
					theLevel->mFloodedCells = 0;
				}
			}
			else
			{
				if ((int)theLevel->mAge % 0xb4 == 0 && theLevel->mKind == 0 && gLevels.mCurrent->mKind != 0xe &&
					theLevel->mTotalCells != 0)
					Sound_Play(theLevel->mFillSound);
				List_Each(aFills, [&](Cell* theAFill) {
					if (theLevel->mTotalCells != 0 && Fill_Update(theAFill) == 0)
					{
						List_Unlink(aFills);
						if (List_Count(aFills) == 1 && theLevel->mKind == 0)
						{
							Sound_Play(theLevel->mFillEndSound);
							Level_Tutorial(theLevel);
						}
					}
				});
			}
			List_Each(&theLevel->mPowerups, [&](Powerup* theAPowerup) {
				if (Powerup_Update(theAPowerup) == 0)
					List_RemoveCurrent(&theLevel->mPowerups);
			});
		}
		else if (theLevel->mState == 3)
		{
			List<Cell>* aFills = &theLevel->mFills;
			if (aFills->mCurrent == nullptr)
			{
				// the texts gone, the results requester comes up
				if (theLevel->mTexts.mHead == nullptr && theLevel->mMenus.mCurrent->mRequesters.mCurrent == nullptr)
					Menu_ShowRequester(theLevel->mMenus.mCurrent, 4, Localize(GT(0x4592c8)), Localize(GT(0x4592dc)));
			}
			else
			{
				List_Each(aFills, [&](Cell* theAFill) {
					if (Fill_Update(theAFill) == 0)
						List_Unlink(aFills);
				});
			}
			// a click, once the text it goes with (else itself) is shown, clears the texts for the requester
			List_Each(&theLevel->mTexts, [&](Text* theAText) {
				bool aShown = false;
				if (theAText->mLoopTime == 0)
					aShown = 5 < theAText->mState;
				else if (theAText->mPrev != nullptr)
					aShown = 5 < theAText->mPrev->mState;
				else if (theAText->mNext != nullptr)
				{
					aShown = 5 < theAText->mNext->mState;
					theAText = theLevel->mTexts.mCurrent;
				}
				if (theLevel->mClicked != 0 && theLevel->mMenus.mCurrent->mRequesters.mCurrent == nullptr && aShown)
				{
					Texts_RemoveAll(&theLevel->mTexts);
					Menu_ShowRequester(theLevel->mMenus.mCurrent, 4, Localize(GT(0x4592c8)), Localize(GT(0x4592dc)));
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
	List_Each(&theLevel->mFills, [](Cell* theACell) { Cell_Redraw(theACell); });
	List_Each(&theLevel->mSystems, [](ParticleSystem* theASystem) { ParticleSystem_Draw(theASystem); });
	List_Each(&theLevel->mEnemies, [](Enemy* theAnEnemy) { Enemy_Draw(theAnEnemy); });
	List_Each(&theLevel->mWeapons, [](Weapon* theAWeapon) { Weapon_Draw(theAWeapon); });
	List_Each(&theLevel->mPowerups, [](Powerup* theAPowerup) { Powerup_Draw(theAPowerup); });
	List_Each(&theLevel->mStars, [](Obj* theAStar) { Star_Draw(theAStar); });
	if (theLevel->mPanels.mCurrent != nullptr)
	{
		theLevel->mPanels.mCurrent = theLevel->mPanels.mHead;
		Panel_Draw(theLevel->mPanels.mHead, theLevel->mAge < 300.0f);
	}
	List_Each(&theLevel->mMenus, [](Menu* theAMenu) { Menu_Draw(theAMenu); });
	DrawList_Flush();
	// the first orbiting one (state 3) says so when an enemy is too close to launch it, and ends the walk
	List_Each(&theLevel->mWeapons, [&](Weapon* theAWeapon) {
		if (theAWeapon->mState == 3)
		{
			Weapon_DrawTooClose(theAWeapon);
			if (theLevel->mWeapons.mCurrent != nullptr)
				theLevel->mWeapons.mCurrent = theLevel->mWeapons.mTail;
		}
	});
	List_Each(&theLevel->mTexts, [](Text* theAText) { Text_Draw(theAText); });
	List_Each(&theLevel->mPointers, [](Obj* theAPointer) { Pointer_Draw(theAPointer); });
	DrawList_Flush();
	// (while it is paused the original calls 0x401410, which is empty: Cosmic Bugs' texts are gone)
}

// 0x4110b0: a new star (a plain object) before the current one
void Star_Append(List<Obj>* theList)
{
	Obj* aStar = (Obj*)::operator new(sizeof(Obj));
	Obj_Init(aStar);
	List_InsertBefore(theList, aStar);
}

// 0x411350: the bubbles that rise behind the board (Cosmic Bugs' stars drifted sideways)
void Level_AddStars(Level* theLevel)
{
	List<Obj>* aList = &theLevel->mStars;
	char aName[32];
	int aStarsLeft = 0x28;
	do
	{
		Star_Append(aList);
		aList->mCurrent->mX = (float)RandRange(0, SCREEN_WIDTH);
		aList->mCurrent->mY = (float)RandRange(0, 0x250);
		aList->mCurrent->mVy = (float)((double)RandRange(0x14, 0x32) / 25.0 * 0.035);
		aList->mCurrent->mVy = -aList->mCurrent->mVy;
		aList->mCurrent->mDrawKind = 2;
		aList->mCurrent->mMidHandle = 1;
		aList->mCurrent->mDirty = 1;
		std::sprintf(aName, "bubble%d", RandRange(1, 3));
		Animation* anAnim = Animation_Find(aName, &aList->mCurrent->mFrameCount, &aList->mCurrent->mFrameTime);
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
	int aFirstTint = 0;
	int aSecondTint = 0;
	Level_EnemyCounts(theLevel, &theFirst, &theSecond, &aFirstKinds, &aSecondKinds, &aFirstTint, &aSecondTint);
	int aKind = theFirst;
	for (int i = 0; i < theFirst; i = i + 1)
	{
		if (i % 2 == 0)
			aKind = RandRange(7, aFirstKinds + 7);
		List_Each(&gEnemyDefs, [&](Enemy* theADef) {
			if (theADef->mKind != aKind)
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
					int aNewTint = aFirstTint;
					if (aFirstTint == 0)
					{
						int r = RandRange(-1, 1);
						aNewTint = r + aTint;
					}
					anEnemies->mCurrent->mTint = aNewTint;
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
				if (aFirstTint == 0)
					anEnemies->mCurrent->mTint = Node_Prev(anEnemies->mCurrent)->mTint;
				else
					anEnemies->mCurrent->mTint = aFirstTint;
			}
			if (gEnemyDefs.mCurrent != nullptr)
				gEnemyDefs.mCurrent = gEnemyDefs.mTail;
		});
	}
	for (int i = 0; i < theSecond; i = i + 1)
	{
		int aSecondKind = RandRange(0, aSecondKinds);
		List_Each(&gEnemyDefs, [&](Enemy* theADef) {
			if (theADef->mKind != aSecondKind)
				return;
			List<Enemy>* anEnemies = &theLevel->mEnemies;
			Enemy_Append(anEnemies);
			Enemy_CopyDef(anEnemies->mCurrent, gEnemyDefs.mCurrent);
			if (Enemy_Place(anEnemies->mCurrent) == 0)
				List_RemoveCurrent(anEnemies);
			else
			{
				int aNewTint = aSecondTint;
				if (aSecondTint == 0)
				{
					int r = RandRange(-1, 1);
					aNewTint = r + aTint;
				}
				anEnemies->mCurrent->mTint = aNewTint;
				anEnemies->mCurrent->mTint = aNewTint;
			}
			if (gEnemyDefs.mCurrent != nullptr)
				gEnemyDefs.mCurrent = gEnemyDefs.mTail;
		});
	}
}

// 0x411700: the level number and quota (Cosmic Bugs' were in capitals)
void Level_ShowStart(Level* theLevel)
{
	char aText[64];
	if (theLevel->mKind == 0)
	{
		std::sprintf(aText, GT(0x458ebc), (int)theLevel->mTitle[0], (int)theLevel->mTitle[1], theLevel->mNumber);
		List<Text>* aTexts = &theLevel->mTexts;
		Text_Append(aTexts);
		Text_Make(aTexts->mCurrent, aText, Font_Find("main_font"), SCREEN_CENTRE_X, 200, 0, 200, 4000, 200, 0, 0);
		std::sprintf(aText, GT(0x458e98), theLevel->mFillQuota, "%");
		Text_Append(aTexts);
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
	Level_StartSong(theLevel, 0);
	Image* aBack = Bank_FindImage2(&gArtBank, theLevel->mBackImage);
	theLevel->mBackground = new Image;
	Image_Copy(theLevel->mBackground, aBack);
	Level_MakeRandom(theLevel, theLevel->mSeed);
	Level_AddRandomEnemies(theLevel, theLevel->mNumWalkers, theLevel->mNumRollers);
	Level_MakeEdges(theLevel);
	Screen_Redraw();
	Level_AddPointer(theLevel);
	Dirty_AddClipped(&gDirty, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_CLIP);
	if (theLevel->mNext != nullptr && Node_Next(theLevel)->mKind == 0xb)
		gGoTo = 0xb;
	// inside the frame the cells start as state 6 (not yet reached by the fill); the fill starts in the corner
	for (int aRow = 1; aRow < gLevels.mCurrent->mRows - 1; aRow++)
	{
		for (int aCol = 1; aCol < gLevels.mCurrent->mCols - 1; aCol++)
			theLevel->mColumns[aCol][aRow].mState = 6;
	}
	Cell_StartFill(&theLevel->mColumns[gLevels.mCurrent->mCols - 2][gLevels.mCurrent->mRows - 2]);
}

// 0x4118d0
void Texts_RemoveAll(List<Text>* theList)
{
	while (List_RemoveCurrent(theList))
		;
}

// 0x412100: while the board waits on its quit requester only the texts, the menus and the pointers go on; 0 when the
// menu's screen is done
int Level_UpdatePaused(Level* theLevel)
{
	theLevel->mClicked = Mouse_Pressed(1);
	List_Each(&theLevel->mTexts, [&](Text* theAText) {
		if (Text_Update(theAText) == 0)
			List_RemoveCurrent(&theLevel->mTexts);
	});
	List<Menu>* aMenus = &theLevel->mMenus;
	if (aMenus->mCurrent != nullptr)
	{
		aMenus->mCurrent = aMenus->mHead;
		while (true)
		{
			if (Menu_Update(aMenus->mCurrent) == 0)
				return 0;
			Menu* aMenu = aMenus->mCurrent;
			if (aMenu == nullptr)
				break;
			if (aMenu->mNext == nullptr)
			{
				aMenus->mCurrent = aMenus->mHead;
				break;
			}
			aMenus->mCurrent = Node_Next(aMenu);
		}
	}
	List_Each(&theLevel->mPointers, [](Obj* theAPointer) { Pointer_Update(theAPointer); });
	return 1;
}

// A hint line in the bonus font: it fades in and out over 200 and stays 4000
static void AddHint(Level* theLevel, const char* theString, int theY)
{
	Text_Append(&theLevel->mTexts);
	Text_Make(theLevel->mTexts.mCurrent, theString, Font_Find("bonus_font"), SCREEN_CENTRE_X, theY, 0, 200, 4000, 200,
		0, 0);
}

// 0x411b10: the first levels' hints, put up when a fill ends. They go by the level and the beam pairs that have
// stopped, and stop once the board is mostly filled.
void Level_Tutorial(Level* theLevel)
{
	int aNumber = theLevel->mNumber;
	if (3 < aNumber)
		return;
	List<Text>* aTexts = &theLevel->mTexts;
	if (theLevel->mPowerups.mCurrent != nullptr)
	{
		Texts_RemoveAll(aTexts);
		AddHint(theLevel, GT(0x45909c), 0xe6);
		return;
	}
	int aLines = theLevel->mLines;
	if (aLines == 0 && aNumber == 1 && theLevel->mMenus.mCurrent != nullptr)
	{
		Texts_RemoveAll(aTexts);
		AddHint(theLevel, Localize(GT(0x459084)), 0xe6);
		AddHint(theLevel, Localize(GT(0x45905c)), 0xfa);
		return;
	}
	if (aNumber == 1)
	{
		if (aLines == 1)
		{
			Texts_RemoveAll(aTexts);
			AddHint(theLevel, Localize(GT(0x459040)), 0xe6);
			AddHint(theLevel, Localize(GT(0x459020)), 0xfa);
			return;
		}
		if (aLines == 2)
		{
			Texts_RemoveAll(aTexts);
			AddHint(theLevel, Localize(GT(0x459008)), 0xe6);
			AddHint(theLevel, Localize(GT(0x458fec)), 0xfa);
			return;
		}
		if (aLines == 3)
		{
			Texts_RemoveAll(aTexts);
			AddHint(theLevel, GT(0x458fd0), 0xe6);
			AddHint(theLevel, GT(0x458fbc), 0xfa);
			return;
		}
		if (0x54 < (theLevel->mFilledCells * 100) / theLevel->mTotalCells)
			return;
	}
	if (aNumber == 2)
	{
		if (aLines == 0)
		{
			Texts_RemoveAll(aTexts);
			AddHint(theLevel, GT(0x458fa4), 0xe6);
			AddHint(theLevel, GT(0x458f84), 0xfa);
			return;
		}
		if (aLines == 1)
		{
			Texts_RemoveAll(aTexts);
			AddHint(theLevel, GT(0x458f64), 0xe6);
			AddHint(theLevel, GT(0x458f44), 0xfa);
			return;
		}
		if (aLines == 2)
		{
			Texts_RemoveAll(aTexts);
			AddHint(theLevel, GT(0x458f24), 0xe6);
			AddHint(theLevel, GT(0x458f04), 0xfa);
			return;
		}
		if (0x54 < (theLevel->mFilledCells * 100) / theLevel->mTotalCells)
			return;
	}
	if (aNumber == 3 && aLines == 0)
	{
		Texts_RemoveAll(aTexts);
		AddHint(theLevel, GT(0x458ee8), 0xe6);
		AddHint(theLevel, GT(0x458ecc), 0xfa);
	}
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
	std::sprintf(aText, GT(0x4590cc), aColour);
	Text_Make(aTexts->mCurrent, aText, Font_Find("main_font"), SCREEN_CENTRE_X, 0xeb, 0, 500, 999999, 200, 0, 0);
	Text_Append(aTexts);
	std::sprintf(aText, GT(0x4590b4), aColour);
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
	if (std::strlen(theName) != 0 && std::strcmp(theName, Localize(GT(TEXT_EMPTY_PROFILE))) != 0)
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

} // namespace wb
