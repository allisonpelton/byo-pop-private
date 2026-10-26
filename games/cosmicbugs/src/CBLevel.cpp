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
	int n = theLevel->number;
	const char* aSong = nullptr;
	const char* aBack;
	if (499 < n)
	{
		if (n == 500)
			aSong = "audio\\cosmo4.mo3";
		aBack = "background09";
	}
	else if (399 < n)
	{
		if (n == 400)
			aSong = "audio\\cosmo5.mo3";
		aBack = "background08";
	}
	else if (299 < n)
	{
		if (n == 300)
			aSong = "audio\\cosmo2.mo3";
		aBack = "background07";
	}
	else if (199 < n)
	{
		if (n == 200)
			aSong = "audio\\cosmo3.mo3";
		aBack = "background06";
	}
	else if (99 < n)
	{
		if (n == 100)
			aSong = "audio\\cosmo4.mo3";
		aBack = "background05";
	}
	else if (0x31 < n)
	{
		if (n == 0x32)
			aSong = "audio\\cosmo5.mo3";
		aBack = "background04";
	}
	else if (0x13 < n)
	{
		if (n == 0x14)
			aSong = "audio\\cosmo2.mo3";
		aBack = "background03";
	}
	else if (9 < n)
	{
		if (n == 10)
			aSong = "audio\\cosmo3.mo3";
		aBack = "background02";
	}
	else
		aBack = "background01";
	if (aSong != nullptr)
		std::strcpy(gSongName, aSong);
	std::strcpy(theLevel->backImage, aBack);
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
	return &gLevels.current->columns[theCol][theRow];
}

// Blocks of solid cells, mirrored by the symmetry, until enough cells are open; then the edges, the edge cells
// with three or more solid neighbours made solid until none changes, and the cells set again from their codes
void Level_MakeRandom(Level* theLevel, int theSeed)
{
	theLevel->fillQuota = 0x55;
	theLevel->seed = 0;
	SeedRand(theSeed);
	int r = RandRange(0, 0x1e);
	theLevel->totalCells = 0;
	theLevel->symmetry = (r / 10) % 3;
	int aWanted = RandRange(800, 0x4b0);
	while (theLevel->totalCells < aWanted)
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
			if (theLevel->symmetry == 2 && aQuarter != 2 && aQuarter != 0)
				aDraw = false;
			if (theLevel->symmetry == 1 && aQuarter != 0 && aQuarter != 1)
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
							if (aCell->kind == 0)
							{
								aCell->kind = 5;
								theLevel->totalCells = theLevel->totalCells + -1;
							}
						}
						else if (aCell->kind == 5)
						{
							aCell->kind = 0;
							theLevel->totalCells = theLevel->totalCells + 1;
						}
					}
				}
			}
		}
	}
	for (int y = 0; y < 0x25; y++)
	{
		for (int x = 0; x < 50; x++)
		{
			Cell* aCell = CellAt(x, y);
			aCell->col = x;
			aCell->row = y;
			if (aCell->kind == 0 && Cell_Touches(aCell, 5) != 0)
				aCell->kind = 3;
		}
	}
	bool aChanged;
	do
	{
		aChanged = false;
		for (int y = 0; y < 0x25; y++)
		{
			for (int x = 0; x < 50; x++)
			{
				Cell* aCell = &theLevel->columns[x][y];
				if (aCell->kind == 3)
				{
					unsigned int aSolid = 0;
					if (0 < x && theLevel->columns[x - 1][y].kind == 5)
						aSolid = 1;
					if (x + 1 < 50 && theLevel->columns[x + 1][y].kind == 5)
						aSolid = aSolid + 1;
					if (0 < y && aCell[-1].kind == 5)
						aSolid = aSolid + 1;
					if (y + 1 < 0x25 && aCell[1].kind == 5)
						aSolid = aSolid + 1;
					if (2 < aSolid)
					{
						aCell->kind = 5;
						aCell->image = nullptr;
						aChanged = true;
					}
				}
			}
		}
	} while (aChanged);
	theLevel->totalCells = 0;
	for (int y = 0; y < 0x25; y++)
	{
		for (int x = 0; x < 50; x++)
		{
			Cell* aCell = CellAt(x, y);
			int aKind = aCell->kind;
			if (aKind == 0)
				Cell_Set(aCell, ':', x, y);
			else if (aKind == 3)
				Cell_Set(aCell, 'X', x, y);
			else if (aKind == 5)
				Cell_Set(aCell, ' ', x, y);
		}
	}
}

void Level_EnemyCounts(Level* theLevel, int* theA, int* theB, int* theC, int* theD)
{
	int n = theLevel->number;
	if (n == 1)
	{
		*theA = 0;
		*theB = 1;
		*theC = 0;
		*theD = 0;
		return;
	}
	if (n == 2)
	{
		*theA = 0;
		*theB = 2;
		*theC = 0;
		*theD = 0;
		return;
	}
	if (n == 3)
	{
		*theA = 4;
		*theB = 2;
		*theC = 0;
		*theD = 0;
		return;
	}
	if (n == 4)
	{
		*theA = 2;
		*theB = 2;
		*theC = 0;
		*theD = 1;
		return;
	}
	if (n == 5)
	{
		*theA = 4;
		*theB = 2;
		*theC = 1;
		*theD = 1;
		return;
	}
	if (5 < n && n < 10)
	{
		*theA = 4;
		*theB = 3;
		*theC = 1;
		*theD = 1;
		return;
	}
	if (9 < n && n < 0xf)
	{
		*theA = 6;
		*theB = 4;
		*theC = 2;
		*theD = 2;
		return;
	}
	if (n == 0x13)
	{
		*theA = 6;
		*theB = 3;
		*theC = 2;
		*theD = 2;
		return;
	}
	if (0xe < n && n < 0x14)
	{
		*theA = 6;
		*theB = 4;
		*theC = 2;
		*theD = 2;
		return;
	}
	if (0x13 < n && n < 0x19)
	{
		*theA = 8;
		*theB = 5;
		*theC = 2;
		*theD = 3;
		return;
	}
	// (the second test of the original's 25..29 range can never be reached)
	if (0x18 < n && n < 0x1e)
	{
		*theA = 6;
		*theB = 6;
		*theC = 3;
		*theD = 3;
		return;
	}
	if (0x1d < n && n < 0x23)
	{
		*theA = 8;
		*theB = 5;
		*theC = 3;
		*theD = 4;
		return;
	}
	if (0x22 < n && n < 0x32)
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
	for (int y = 0; y < gLevels.current->rows; y++)
	{
		for (int x = 0; x < gLevels.current->cols; x++)
		{
			if (theLevel->columns[x][y].kind == 3)
				Cell_MakeEdge(&theLevel->columns[x][y]);
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
			theLevel->hPattern[i] = theH[i] + -0x31;
			i = i + 1;
		} while (i < std::strlen(theH));
	}
	theLevel->hPatternCount = (int)i;
	i = 0;
	if (std::strlen(theV) != 0)
	{
		do
		{
			theLevel->vPattern[i] = theV[i] + -0x31;
			i = i + 1;
		} while (i < std::strlen(theV));
	}
	theLevel->vPatternCount = (int)i;
}

static void LoadBeamImages(Level* theLevel, const char* theBeam)
{
	char aName[32];
	std::sprintf(aName, "%s_horizontal", theBeam);
	theLevel->beamH = Bank_FindImage2(&gArtBank, aName);
	std::sprintf(aName, "%s_vertical", theBeam);
	theLevel->beamV = Bank_FindImage2(&gArtBank, aName);
	std::sprintf(aName, "%s_middle", theBeam);
	theLevel->beamMiddle = Bank_FindImage2(&gArtBank, aName);
	theLevel->empty = Bank_FindImage2(&gArtBank, "empty");
}

void Level_LoadFillImages(Level* theLevel, const XmlTag* theTag)
{
	char aBeam[32];
	int aFrameTime;
	theLevel->fillImage = Bank_FindImage2(&gArtBank, Xml_GetString(theTag, "fill_image"));
	theLevel->hTiles = Animation_Find("horizontal_tiles", &theLevel->hTileCount, &aFrameTime);
	theLevel->vTiles = Animation_Find("vertical_tiles", &theLevel->vTileCount, &aFrameTime);
	theLevel->cornerTiles = Animation_Find("corner_tiles", &theLevel->cornerTileCount, &aFrameTime);
	std::strcpy(aBeam, Xml_GetString(theTag, "beam_image"));
	LoadBeamImages(theLevel, aBeam);
}

void Level_DefaultFillImages(Level* theLevel)
{
	char aName[32];
	int aFrameTime;
	std::sprintf(aName, "fill%02d", theLevel->number % 0x12 + 1);
	theLevel->fillImage = Bank_FindImage2(&gArtBank, aName);
	theLevel->hTiles = Animation_Find("horizontal_tiles", &theLevel->hTileCount, &aFrameTime);
	theLevel->vTiles = Animation_Find("vertical_tiles", &theLevel->vTileCount, &aFrameTime);
	theLevel->cornerTiles = Animation_Find("corner_tiles", &theLevel->cornerTileCount, &aFrameTime);
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
	if (theLevel->seed != 0)
		return;
	for (int y = gLevels.current->rows + -1; -1 < y; y--)
	{
		for (int x = gLevels.current->cols + -1; -1 < x; x--)
		{
			theLevel->columns[x][y].unkd0 = 0;
			theLevel->columns[x][y].unkcc = 0;
		}
	}
	for (int y = gLevels.current->rows + -1; -1 < y; y--)
	{
		for (int x = gLevels.current->cols + -1; -1 < x; x--)
		{
			if (theLevel->columns[x][y].image != nullptr)
				Cell_Redraw(&theLevel->columns[x][y]);
		}
	}
	if (theLevel->panels.current != nullptr)
		Panel_Draw(theLevel->panels.current, 1);
}

static Font* ScreenFont(const char* theName)
{
	Font* aFont = Font_Find(theName);
	Font_SetEscapes(aFont, 1);
	Font_SetTarget(aFont, gScreenBits, gScreenPitch, &gScreen.fmt, gScreen.w, gScreen.h);
	Font_SetPalette(aFont, nullptr);
	aFont->alpha = 0xff;
	return aFont;
}

void Level_DrawTrialTime(Level* theLevel)
{
	char aText[32];
	int aRect[4];
	if (gRegistered == 0 && theLevel->kind == 0 && gTrial != 0)
	{
		std::sprintf(aText, GT(0x458e24), gTrialLeft / 60000, (gTrialLeft / 1000) % 0x3c);
		Font* aFont = ScreenFont("micro_font");
		aFont->align = 0;
		Font_Printf(aFont, aRect, 4, 0x1fe, aText);
		Dirty_AddClippedShown(&gDirty, 4, 0x1fe, Font_StringWidth(aFont, aText), 8, kClip);
	}
}

static void DrawCentred(Font* theFont, int theY, const char* theText)
{
	int aRect[4];
	Font_DrawOutlined(theFont, aRect, 400, theY, theText);
	int aHeight = Font_StringHeight(theFont, theText) + 2;
	int aWidth = Font_StringWidth(theFont, theText) + 2;
	Dirty_AddClippedShown(&gDirty, 399 - Font_StringWidth(theFont, theText) / 2, theY - 1, aWidth, aHeight, kClip);
}

// The original copies the lines onto the stack (the last from immediates in its code, which GT reads too)
void DrawPaused()
{
	char aText[32];
	Font* aFont = ScreenFont("main_font");
	aFont->align = 2;
	std::strcpy(aText, GT(0x458e74));
	DrawCentred(aFont, 200, aText);
	std::strcpy(aText, GT(0x458e58));
	DrawCentred(aFont, 0xfa, aText);
	std::strcpy(aText, GT(0x410580));
	DrawCentred(aFont, 300, aText);
}

int Level_IsPlaying(Level* theLevel)
{
	if ((theLevel->kind != 0 || (theLevel->menus.current != nullptr && theLevel->menus.current->active != 0) ||
			(theLevel->panels.current != nullptr && theLevel->panels.current->y <= (float)gMouseY)) &&
		100.0f < theLevel->age)
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
	List<KeyMapNode>* aList = &theLevel->keyMap;
	List_Append(aList, new KeyMapNode);
	aList->current->key = theKey;
	aList->current->a = theA;
	aList->current->b = theB;
}

void Level_FindKey(Level* theLevel, char theKey, int* theA, int* theB)
{
	List_Each(&theLevel->keyMap, [&](KeyMapNode* aNode) {
		if (aNode->key == theKey)
		{
			*theA = aNode->a;
			*theB = theLevel->keyMap.current->b;
		}
	});
}

void Pointer_Insert(List<Obj>* theList)
{
	Obj* aPointer = (Obj*)::operator new(sizeof(Obj));
	Obj_Init(aPointer);
	aPointer->depth = 0xc;
	List_InsertBefore(theList, aPointer);
}

void Pointer_Append(List<Obj>* theList)
{
	Obj* aPointer = (Obj*)::operator new(sizeof(Obj));
	Obj_Init(aPointer);
	aPointer->depth = 0xc;
	List_Append(theList, aPointer);
}

void Level_AddPointer(Level* theLevel)
{
	List<Obj>* aList = &theLevel->pointers;
	Pointer_Insert(aList);
	aList->current->anim = Animation_Find("pointer", &aList->current->frameCount, &aList->current->frameTime);
	aList->current->dirty = 1;
	aList->current->drawKind = 1;
	aList->current->depth = 200;
}

// The fills and the particle systems are not the level's to delete: they are unlinked
void Level_UnlinkAll(Level* theLevel)
{
	DrawList_Clear();
	while (theLevel->fills.current != nullptr)
	{
		theLevel->fills.current = theLevel->fills.head;
		if (theLevel->fills.current != nullptr)
			List_Unlink(&theLevel->fills);
	}
	while (theLevel->systems.current != nullptr)
	{
		theLevel->systems.current = theLevel->systems.head;
		if (theLevel->systems.current != nullptr)
			List_Unlink(&theLevel->systems);
	}
}

Level* Level_Init(Level* theLevel)
{
	Obj_Init(theLevel);
	List_Init(&theLevel->shooters);
	List_Init(&theLevel->fills);
	List_Init(&theLevel->systems);
	List_Init(&theLevel->panels);
	List_Init(&theLevel->enemies);
	List_Init(&theLevel->keyMap);
	List_Init(&theLevel->unk430);
	List_Init(&theLevel->stars);
	List_Init(&theLevel->texts);
	List_Init(&theLevel->menus);
	List_Init(&theLevel->pointers);
	List_Init(&theLevel->unk46c);
	theLevel->totalCells = 0;
	theLevel->filledCells = 0;
	theLevel->title[0] = 0;
	theLevel->kind = 0;
	theLevel->background = nullptr;
	theLevel->gameOver = 0;
	theLevel->fillSound = 0;
	theLevel->fillEndSound = 0;
	theLevel->unk3c0 = 0;
	theLevel->seed = 0;
	theLevel->time = 0;
	theLevel->unk3d4 = 0;
	gSongName[0] = 0;
	theLevel->unk394 = 1;
	for (int i = 0; i < 50; i++)
		theLevel->columns[i] = nullptr;
	theLevel->bonus = 0;
	theLevel->musicPending = 0;
	return theLevel;
}

void Level_Destroy(Level* theLevel)
{
	Level_UnlinkAll(theLevel);
	if (theLevel->background != nullptr)
	{
		Image_Destroy(theLevel->background);
		::operator delete(theLevel->background);
	}
	for (int i = 0; i < 50; i++)
	{
		if (theLevel->columns[i] != nullptr)
			Cells_Delete(theLevel->columns[i]);
	}
	List_Clear(&theLevel->unk46c);
	List_Clear(&theLevel->pointers);
	List_Clear(&theLevel->menus);
	List_Clear(&theLevel->texts);
	List_Clear(&theLevel->stars);
	List_Clear(&theLevel->unk430);
	List_Clear(&theLevel->keyMap);
	List_Clear(&theLevel->enemies);
	List_Clear(&theLevel->panels);
	List_Clear(&theLevel->systems);
	List_Clear(&theLevel->fills);
	List_Clear(&theLevel->shooters);
	Obj_Destroy(theLevel);
}

void Level_Load(Level* theLevel, const char* p, XmlTag* theTag)
{
	char aType[32];
	int aRow = 0;
	if (p != nullptr)
	{
		for (int i = 0; i < 50; i++)
			theLevel->columns[i] = Cells_New();
	}
	if (Xml_GetString(theTag, "back_image") != nullptr)
	{
		std::strcpy(theLevel->backImage, Xml_GetString(theTag, "back_image"));
		Image* aBack = Bank_FindImage2(&gArtBank, theLevel->backImage);
		theLevel->background = new Image;
		Image_Copy(theLevel->background, aBack);
	}
	if (p != nullptr && Xml_GetString(theTag, "type") != nullptr)
	{
		std::strcpy(aType, Xml_GetString(theTag, "type"));
		if (std::strcmp(aType, "credits") == 0)
			theLevel->kind = 4;
		else if (std::strcmp(aType, "options") == 0)
			theLevel->kind = 7;
		else if (std::strcmp(aType, "high_scores") == 0)
			theLevel->kind = 3;
		else if (std::strcmp(aType, "high_score_entry") == 0)
			theLevel->kind = 2;
		else if (std::strcmp(aType, "main_menu") == 0)
			theLevel->kind = 1;
		else if (std::strcmp(aType, "select_level") == 0)
			theLevel->kind = 5;
		else if (std::strcmp(aType, "select_level2") == 0)
			theLevel->kind = 6;
		else if (std::strcmp(aType, "opening_nag") == 0)
			theLevel->kind = 8;
		else if (std::strcmp(aType, "closing_nag") == 0)
			theLevel->kind = 9;
		else if (std::strcmp(aType, "change_user") == 0)
			theLevel->kind = 0x10;
		else if (std::strcmp(aType, "expiring_nag") == 0)
			theLevel->kind = 10;
		else if (std::strcmp(aType, "logo") == 0)
			theLevel->kind = 0xe;
		else if (std::strcmp(aType, "mission_complete") == 0)
			theLevel->kind = 0xb;
		else if (std::strcmp(aType, "help") == 0)
			theLevel->kind = 0xc;
		else if (std::strcmp(aType, "help2") == 0)
			theLevel->kind = 0xd;
	}
	if (theLevel->kind == 0)
	{
		if (Xml_GetString(theTag, "name") == nullptr)
			std::sprintf(theLevel->title, GT(0x459028), 4);
		else
			std::strcpy(theLevel->title, Xml_GetString(theTag, "name"));
		theLevel->rows = 0x20;
	}
	else
		theLevel->rows = 0x25;
	theLevel->cols = 0x32;
	theLevel->fillQuota = Xml_GetInt(theTag, "fill_quota");
	theLevel->fillSound = Sound_Load("audio\\fill.wav");
	theLevel->fillEndSound = Sound_Load("audio\\fill_end.wav");
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
	while (p != nullptr && std::strcmp(theTag->name, "/level") != 0)
	{
		const char* aName = theTag->name;
		if (std::strcmp(aName, "row") == 0 && aRow < theLevel->rows)
		{
			if (aRow == 0)
				theLevel->totalCells = 0;
			const char* aCells = Xml_GetString(theTag, "cells");
			for (int x = 0; x < 50; x++)
				Cell_Set(&theLevel->columns[x][aRow], aCells[x], x, aRow);
			aRow = aRow + 1;
		}
		else if (std::strcmp(aName, "random_board") == 0)
		{
			theLevel->numWalkers = Xml_GetInt(theTag, "num_walkers");
			theLevel->numRollers = Xml_GetInt(theTag, "num_rollers");
			theLevel->seed = Xml_GetInt(theTag, "seed");
		}
		else if (std::strcmp(aName, "shooter") == 0)
		{
			Shooter_Insert(&theLevel->shooters);
			Shooter_FromXml(theLevel->shooters.current, p, theTag);
		}
		else if (std::strcmp(aName, "panel") == 0)
		{
			Panel_Append(&theLevel->panels);
			Panel_FromXml(theLevel->panels.current, p, theTag);
		}
		else if (std::strcmp(aName, "enemy") == 0)
		{
			Enemy_Insert(&theLevel->enemies);
			Enemy_FromXml(theLevel->enemies.current, p, theTag);
		}
		else if (std::strcmp(aName, "emitter") == 0)
		{
			Emitter_Append(&theLevel->enemies.current->emitters);
			Emitter_FromXml(static_cast<Emitter*>(theLevel->enemies.current->emitters.current), theTag);
		}
		else if (std::strcmp(aName, "menu") == 0)
		{
			Menu_Append(&theLevel->menus);
			Menu_FromXml(theLevel->menus.current, p, theTag);
		}
		else if (std::strcmp(aName, "pointer") == 0)
		{
			Pointer_Append(&theLevel->pointers);
			Pointer_FromXml(theLevel->pointers.current, p, theTag);
		}
		else if (std::strcmp(aName, "text") == 0)
		{
			if (Xml_GetInt(theTag, "fv") == 0 || gRegistered == 0)
			{
				Text_Append(&theLevel->texts);
				if (Text_FromXml(theLevel->texts.current, p, theTag) == 0)
					List_RemoveCurrent(&theLevel->texts);
			}
		}
		else if (std::strcmp(aName, "stars") == 0)
		{
			int n = Xml_GetInt(theTag, "number");
			if (0 < n)
			{
				do
				{
					Star_Append(&theLevel->stars);
					Pointer_FromXml(theLevel->stars.current, p, theTag);
					theLevel->stars.current->x = (float)RandRange(0, 800);
					theLevel->stars.current->y = (float)RandRange(0, 0x250);
					Obj* aStar = theLevel->stars.current;
					aStar->vx = (float)((double)RandRange(0x14, 0x32) / 25.0 * (double)aStar->vx);
					n = n + -1;
				} while (n != 0);
			}
		}
		p = Xml_NextTag(p, theTag);
	}
	if (p == nullptr)
	{
		theLevel->numWalkers = (CrtRand() % 5) * 2 + 2;
		theLevel->numRollers = (CrtRand() % 5) * 2 + 2;
		theLevel->seed = CrtRand();
	}
	if (theLevel->seed == 0)
		Level_MakeEdges(theLevel);
	theLevel->state = 1;
	if (theLevel->panels.current == nullptr && theLevel->kind == 0 && aNoPanel == 0)
	{
		Panel_Append(&theLevel->panels);
		Panel_Setup(theLevel->panels.current);
	}
	if (theLevel->shooters.current == nullptr && theLevel->kind == 0)
	{
		Shooter_Insert(&theLevel->shooters);
		Shooter_Setup(theLevel->shooters.current);
	}
	if (theLevel->stars.current == nullptr && aNoStars == 0)
		Level_AddStars(theLevel);
	if (p != nullptr)
	{
		for (int y = 1; y < gLevels.current->rows + -1; y++)
		{
			for (int x = 1; x < gLevels.current->cols + -1; x++)
				theLevel->columns[x][y].state = 6;
		}
		Cell_StartFill(&theLevel->columns[gLevels.current->cols + -2][gLevels.current->rows + -2]);
	}
}

static void AddText(Level* theLevel, const char* theString, int theX, int theY, int theA, int theB, int theC,
	int theD, int theE, int theF)
{
	Text_Append(&theLevel->texts);
	Text_Make(theLevel->texts.current, theString, Font_Find("main_font"), theX, theY, theA, theB, theC, theD, theE,
		theF);
}

// The board is filled: the records, the frame's cells, the texts and the bonus, and every enemy and shooter told
// to leave
static void Level_Won(Level* theLevel)
{
	char aText[64];
	int aProfile = gProfile;
	LevelRecord* aRecord = &gLevelRecords[theLevel->number];
	if (theLevel->time < aRecord->bestTime)
	{
		aRecord->bestTime = theLevel->time;
		std::strcpy(aRecord->timeName, gProfiles[aProfile].name);
	}
	int aFilled = (theLevel->filledCells * 100) / theLevel->totalCells;
	if (aRecord->bestFill < aFilled)
	{
		aRecord->bestFill = aFilled;
		std::strcpy(aRecord->fillName, gProfiles[aProfile].name);
	}
	int* aReached = &gProfiles[aProfile].levelReached;
	int n = theLevel->number;
	if (*aReached < n)
	{
		bool aTrial = gRegistered == 0;
		*aReached = n;
		if (aTrial)
		{
			if (0x30 < n)
				n = 0x30;
			*aReached = n;
		}
	}
	SaveProfiles();
	theLevel->state = 3;
	for (int y = 1; y < gLevels.current->rows + -1; y++)
	{
		for (int x = 1; x < gLevels.current->cols + -1; x++)
		{
			theLevel->columns[x][y].state = 6;
			theLevel->columns[x][y].unkd4 = 0x168;
		}
	}
	Cell_StartFill(&theLevel->columns[1][1]);
	List_Each(&theLevel->texts, [&](Text* aText) {
		if (5 < std::strlen(aText->text) || aText->font == Font_Find("main_font"))
		{
			aText->state = 7;
			theLevel->texts.current->lifetime = 100.0f;
			theLevel->texts.current->age = 1.0f;
		}
	});
	int aBonus = ((theLevel->filledCells * 100) / theLevel->totalCells - theLevel->fillQuota) * 100;
	theLevel->bonus = aBonus;
	gScore = gScore + aBonus;
	std::sprintf(aText, "^1%s", Localize(GT(0x45916c)));
	AddText(theLevel, aText, 400, 100, 100, 300, 0x1863c, 500, 0, 0);
	int aY;
	if (Level_EnemiesLeft(theLevel) == 0)
	{
		theLevel->bonus = 2000;
		gScore = gScore + 2000;
		std::sprintf(aText, "^4%s", Localize(GT(0x459130)));
		AddText(theLevel, aText, 400, 0xb4, 100, 300, 0xf41dc, 500, 0, 0);
		aY = 0xf0;
	}
	else
	{
		int aCount = theLevel->filledCells;
		int aTotal = theLevel->totalCells;
		std::sprintf(aText, "^1%s", Localize(GT(0x459150)));
		AddText(theLevel, aText, 400, 0xaa, 100, 300, 0xf41dc, 500, 0, 0);
		std::sprintf(aText, "^4%d%s", (aCount * 100) / aTotal, "%");
		AddText(theLevel, aText, 400, 200, 100, 300, 0xf41dc, 500, 0, 0);
		aY = 0xfa;
	}
	std::sprintf(aText, "^1%s", Localize(GT(0x459118)));
	AddText(theLevel, aText, 400, aY, 100, 300, 0xf41dc, 500, 0, 0);
	std::sprintf(aText, "^4%d", theLevel->bonus);
	AddText(theLevel, aText, 400, aY + 0x1e, 100, 300, 0xf41dc, 500, 0, 0);
	std::sprintf(aText, "^1%s", Localize(GT(0x4590fc)));
	AddText(theLevel, aText, 400, aY + 100, 100, 400, 200, 400, 200, 0);
	List_Each(&theLevel->enemies, [&](Enemy* anEnemy) {
		anEnemy->state = 2;
		if (theLevel->enemies.current->rider != nullptr)
			theLevel->enemies.current->rider->state = 2;
	});
	List_Each(&theLevel->unk46c, [](Obj* anObj) { anObj->state = 2; });
	// the original steps through the enemies here, not the shooters: the current shooter is told once per enemy
	if (theLevel->shooters.current != nullptr)
	{
		theLevel->shooters.current = theLevel->shooters.head;
		while (true)
		{
			theLevel->shooters.current->state = 2;
			Beams_Fn411900(&theLevel->shooters.current->beams);
			Enemy* anEnemy = theLevel->enemies.current;
			if (anEnemy == nullptr)
				break;
			if (anEnemy->next == nullptr)
			{
				theLevel->enemies.current = theLevel->enemies.head;
				break;
			}
			theLevel->enemies.current = Node_Next(anEnemy);
		}
	}
}

int Level_Update(Level* theLevel)
{
	int aResult = 1;
	if (gLevels.current->kind == 0xe && theLevel->texts.current == nullptr)
	{
		gLogoCount = gLogoCount + 1;
		gGoTo = (gLogoCount <= PortalCount()) ? 0xe : 1;
		return 0;
	}
	if (theLevel->kind == 5 && theLevel->age < 3.0f)
		Fn40d040();
	if (gLeave != 0 && theLevel->kind == 0 && theLevel->state != 2)
	{
		theLevel->state = 2;
		gLeave = 0;
	}
	if (theLevel->state == 2)
		return Level_UpdateLeaving(theLevel);
	if (gDemo == 0 || theLevel->kind != 0)
	{
		theLevel->clicked = Mouse_Pressed(1);
		int aRight;
		if (Mouse_Pressed(2) == 0)
			aRight = Key_Pressed(0x20) != 0 ? 1 : 0;
		else
			aRight = 1;
		theLevel->rightClicked = aRight;
	}
	else
	{
		int r = RandRange(1, 200);
		gLevels.current->clicked = r == 4 ? 1 : 0;
		gLevels.current->rightClicked = r == 5 ? 1 : 0;
	}
	if (theLevel->seed == 0)
	{
		if (theLevel->musicPending != 0)
		{
			List_Each(&gLevelSets, [&](LevelSet* aSet) {
				if (aSet->start == theLevel->number + 1)
				{
					std::strcpy(gSongName, aSet->song);
					DrawLoadingScreen();
					Music_Free();
					Music_Load(gSongName);
					Music_SetVolume(0x8c);
					Music_Play();
					gSongName[0] = 0;
				}
			});
			theLevel->musicPending = 0;
		}
	}
	else
		Level_UpdateRandom(theLevel);
	Obj_Update(theLevel);
	List_Each(&theLevel->systems, [&](ParticleSystem* aSystem) {
		if (ParticleSystem_Update(aSystem) == 0)
			List_Unlink(&theLevel->systems);
	});
	List_Each(&theLevel->stars, [&](Obj* aStar) {
		if (Star_Update(aStar) == 0)
			List_RemoveCurrent(&theLevel->stars);
	});
	if (theLevel->shooters.current != nullptr)
		Shooter_Update(theLevel->shooters.current);
	List_Each(&theLevel->enemies, [&](Enemy* anEnemy) {
		if (Enemy_Update(anEnemy) == 0)
			List_RemoveCurrent(&theLevel->enemies);
	});
	gFlag46003c = 1;
	List_Each(&theLevel->unk46c, [&](Obj* anObj) {
		if (Fn41ae10(anObj) == 0)
			List_RemoveCurrent(&theLevel->unk46c);
	});
	List_Each(&theLevel->pointers, [&](Obj* aPointer) {
		if (Pointer_Update(aPointer) == 0)
			List_RemoveCurrent(&theLevel->pointers);
	});
	List_Each(&theLevel->texts, [&](Text* aText) {
		if (Text_Update(aText) == 0)
			List_RemoveCurrent(&theLevel->texts);
	});
	List_Each(&theLevel->menus, [&](Menu* aMenu) { aResult = aResult & Menu_Update(aMenu); });
	if (theLevel->panels.current != nullptr)
	{
		theLevel->panels.current = theLevel->panels.head;
		Panel_Update(theLevel->panels.head);
	}
	if (aResult != 0)
	{
		if (theLevel->state == 1)
		{
			List<Obj>* aFills = &theLevel->fills;
			if (aFills->current == nullptr)
			{
				if (theLevel->kind == 0 && theLevel->unk430.current == nullptr && theLevel->totalCells != 0 &&
					(theLevel->fillQuota <= (theLevel->filledCells * 100) / theLevel->totalCells ||
						(gDemo != 0 && 0x32 < theLevel->time)) &&
					(theLevel->menus.current == nullptr || theLevel->menus.current->active == 0))
					Level_Won(theLevel);
				else
				{
					theLevel->unk394 = 1;
					theLevel->unk3c0 = 0;
				}
			}
			else
			{
				if ((int)theLevel->age % 0x78 == 0 && theLevel->kind == 0 && gLevels.current->kind != 0xe &&
					theLevel->totalCells != 0)
					Sound_Play(theLevel->fillSound);
				List_Each(aFills, [&](Obj* aFill) {
					if (theLevel->totalCells != 0 && Fill_Update(aFill) == 0)
					{
						List_Unlink(aFills);
						if (List_Count(aFills) == 1 && theLevel->kind == 0)
							Sound_Play(theLevel->fillEndSound);
					}
				});
			}
			List_Each(&theLevel->unk430, [&](Obj* anObj) {
				if (Fn419770(anObj) == 0)
					List_RemoveCurrent(&theLevel->unk430);
			});
		}
		else if (theLevel->state == 3)
		{
			List<Obj>* aFills = &theLevel->fills;
			if (aFills->current == nullptr)
			{
				if (theLevel->texts.head == nullptr)
					aResult = 0;
			}
			else
			{
				List_Each(aFills, [&](Obj* aFill) {
					if (Fill_Update(aFill) == 0)
						List_Unlink(aFills);
				});
			}
			// a text is clicked away once the one it goes with (else itself) is shown
			List_Each(&theLevel->texts, [&](Text* aText) {
				bool aShown = false;
				if (aText->unk1c4 == 0)
					aShown = 5 < aText->state;
				else if (aText->prev != nullptr)
					aShown = 5 < aText->prev->state;
				else if (aText->next != nullptr)
				{
					aShown = 5 < aText->next->state;
					aText = theLevel->texts.current;
				}
				if (theLevel->clicked != 0 && aShown)
				{
					if (aText->state != 7)
						aText->age = 0.0f;
					theLevel->texts.current->lifetime = (float)theLevel->texts.current->unk1b8;
					theLevel->texts.current->state = 7;
					theLevel->texts.current->unk1c4 = 0;
				}
			});
		}
	}
	if (theLevel->gameOver != 0)
	{
		while (List_RemoveCurrent(&theLevel->shooters))
			;
		if (theLevel->clicked != 0)
		{
			gGoTo = (gScore <= gLowestHighScore) + 2;
			return 0;
		}
	}
	return aResult;
}

// The select-level screen's marker (an item of kind 0x15) along the current level set, up to two levels past the
// furthest one reached
void Level_UpdateProgressStars()
{
	for (Obj* anItem = gLevels.current->menus.current->items.head; anItem != nullptr; anItem = anItem->next)
	{
		if (anItem->kind == 0x15)
		{
			LevelSet* aSet = gLevelSets.current;
			int aLast = gProfiles[gProfile].levelReached + 2;
			if (aSet->end <= gProfiles[gProfile].levelReached + 2)
				aLast = aSet->end;
			int aStart = aSet->start;
			int aRange = aSet->end - aStart;
			float x = (float)((((gLevelInSet - aStart) + 1) * 400) / aRange + 200);
			anItem->x = x;
			float aMax = (float)(((aLast - aStart) * 400) / aRange + 200);
			if (aMax < x)
				x = aMax;
			anItem->x = x;
			if (x < 200.0f)
				x = 200.0f;
			anItem->x = x;
		}
	}
}

void Level_SetUserName(const char* theName)
{
	if (std::strlen(theName) != 0 && std::strcmp(theName, Localize(GT(0x458a84))) != 0)
	{
		std::strcpy(gProfiles[gProfile].name, theName);
		gUnk460028 = gProfiles[gProfile].unk24;
		SaveProfiles();
	}
}

int Pointer_Update(Obj* thePointer)
{
	if (Level_IsPlaying(gLevels.current) == 0 || gLevels.current->totalCells == 0)
	{
		thePointer->shadow = 1;
		thePointer->alpha = 0xff;
		if (gDemo != 0)
		{
			thePointer->x = (float)RandRange(0, 799);
			thePointer->y = (float)RandRange(0, 599);
			Obj_Update(thePointer);
			return 1;
		}
		int v;
		if (gMouseX < 0x321)
			v = gMouseX < 0 ? 0 : gMouseX;
		else
			v = 800;
		thePointer->x = (float)v;
		if (gMouseY < 0x259)
			v = gMouseY < 0 ? 0 : gMouseY;
		else
			v = 600;
		thePointer->y = (float)v;
		Obj_Update(thePointer);
	}
	return 1;
}

void Pointer_Draw(Obj* thePointer)
{
	if (Level_IsPlaying(gLevels.current) != 0 && gLevels.current->totalCells != 0)
		return;
	Obj_Draw(thePointer);
}

} // namespace cb
