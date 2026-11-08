// The menus (0x414150..0x418050): a menu of the game file is a list of items, each drawn with its
// animation or as its name in a font, hot under the mouse and doing its kind's thing when clicked: going to another
// screen, changing a setting, choosing a level, a world or a player. A requester (a box with texts and buttons, such
// as "are you sure?") goes over a menu, and only it is used while it is up.

#include "WBLevel.h"
#include "R64Sys.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif

namespace wb
{

namespace
{

int gSliding = 0;                  // 0x6e5bf8  the level slider is being dragged
int gWorldChanged = 0;             // 0x6e5bfc  a world was chosen: the slider starts again
int gSavedProfile = 0;             // 0x6e5af8  the player, and the name, to go back to when a requester is cancelled
char gSavedName[0x20];             // 0x6e5b00

// The sprintf of the drawn text onto itself that the original does (it works on MSVC's, which copies as it goes)
void AppendTo(char* theDrawn, const char* theFormat, int theCode, const char* theValue)
{
	char aCopy[0x100];
	strcpy(aCopy, theDrawn);
	sprintf(theDrawn, theFormat, aCopy, theCode, theValue);
}

// A setting's value after an item's name: in the drawn text in the item's colour, and in the text measured
void AddValue(char* theShown, char* theDrawn, int theHot, const char* theValue)
{
	AppendTo(theDrawn, "%s^%d%s", (2 - theHot) * 2, Localize(theValue));
	strcat(theShown, Localize(theValue));
}

// 0x415cb0: a requester at the front of the list, made current
void Requester_Push(List<Menu>* theList)
{
	Menu* aMenu = (Menu*)::operator new(sizeof(Menu));
	Menu_Init(aMenu);
	if (theList->current == nullptr)
	{
		aMenu->next = nullptr;
		aMenu->prev = nullptr;
		theList->tail = aMenu;
	}
	else
	{
		aMenu->next = theList->head;
		aMenu->prev = nullptr;
		theList->head->prev = aMenu;
	}
	theList->current = aMenu;
	theList->head = aMenu;
}

// 0x4156b0
void MenuItem_Append(List<MenuItem>* theList)
{
	MenuItem* anItem = (MenuItem*)::operator new(sizeof(MenuItem));
	Obj_Init(anItem);
	anItem->autoClick = 0;
	anItem->image = nullptr;
	anItem->box = 0;
	anItem->showOnlyHot = 0;
	anItem->hotFont = nullptr;
	anItem->alwaysHot = 0;
	List_Append(theList, anItem);
}

// A requester's button: one of the art's animations at (x, y)
void Button_Setup(MenuItem* theItem, const char* theAnim, float x, float y)
{
	theItem->dirty = 1;
	theItem->midHandle = 1;
	theItem->drawKind = 1;
	theItem->anim = Animation_Find(theAnim, &theItem->frameCount, &theItem->frameTime);
	theItem->image = Animation_Frame(theItem->anim, 0);
	theItem->x = x;
	theItem->y = y;
	theItem->depth = 0x15e;
}

// A requester's next text, one of the level's
Text* Requester_AddText(Menu* theRequester)
{
	Text_Append(&gLevels.current->texts);
	theRequester->texts[theRequester->textCount] = gLevels.current->texts.current;
	return theRequester->texts[theRequester->textCount];
}

// 0x414150: an item's attributes; its type is its kind
void MenuItem_FromXml(MenuItem* theItem, const char* p, const XmlTag* theTag)
{
	// (the original leaves the type unset when there is none)
	char aType[0x20] = "";
	Obj_FromXml(theItem, p, theTag);
	// (the registered game takes the types and names marked fv)
	if (Xml_GetString(theTag, "fv_type") == nullptr)
	{
		if (Xml_GetString(theTag, "type") != nullptr)
			strcpy(aType, Xml_GetString(theTag, "type"));
	}
	else
		strcpy(aType, Xml_GetString(theTag, "fv_type"));
	if (strcmp(aType, "buy_now") == 0)
		theItem->kind = 0;
	else if (strcmp(aType, "help") == 0)
		theItem->kind = 4;
	else if (strcmp(aType, "help2") == 0)
		theItem->kind = 5;
	else if (strcmp(aType, "more_games") == 0)
		theItem->kind = 6;
	else if (strcmp(aType, "start") == 0)
		theItem->kind = 1;
	else if (strcmp(aType, "quit") == 0)
		theItem->kind = 3;
	else if (strcmp(aType, "credits") == 0)
		theItem->kind = 0xb;
	else if (strcmp(aType, "back") == 0)
		theItem->kind = 0x19;
	else if (strcmp(aType, "logo") == 0)
		theItem->kind = 0x16;
	else if (strcmp(aType, "options") == 0)
		theItem->kind = 2;
	else if (strcmp(aType, "music") == 0)
		theItem->kind = 0xf;
	else if (strcmp(aType, "sound") == 0)
		theItem->kind = 0xe;
	else if (strcmp(aType, "video") == 0)
		theItem->kind = 0xc;
	else if (strcmp(aType, "lcd") == 0)
		theItem->kind = 0xd;
	else if (strcmp(aType, "difficulty") == 0)
		theItem->kind = 0x10;
	else if (strcmp(aType, "high_scores") == 0)
		theItem->kind = 7;
	else if (strcmp(aType, "hs_entered") == 0)
		theItem->kind = 9;
	else if (strcmp(aType, "select_level") == 0)
		theItem->kind = 0x12;
	else if (strcmp(aType, "select_up") == 0)
		theItem->kind = 0x13;
	else if (strcmp(aType, "select_down") == 0)
		theItem->kind = 0x14;
	else if (strcmp(aType, "select_slide") == 0)
		theItem->kind = 0x15;
	else if (strcmp(aType, "shooter_mode") == 0)
		theItem->kind = 0x11;
	else if (strcmp(aType, "next_level") == 0)
		theItem->kind = 8;
	else if (strcmp(aType, "closing_nag") == 0)
		theItem->kind = 3;
	else if (strcmp(aType, "hs_stuff") == 0)
		theItem->kind = 10;
	else if (strcmp(aType, "select_world") == 0)
		theItem->kind = 0x18;
	else if (strcmp(aType, "change_user") == 0)
		theItem->kind = 0x1a;
	else if (strcmp(aType, "user_slot") == 0)
		theItem->kind = 0x1b;
	else if (strcmp(aType, "user_rename") == 0)
		theItem->kind = 0x1c;
	else if (strcmp(aType, "user_delete") == 0)
		theItem->kind = 0x1e;
	else if (strcmp(aType, "user_rename") == 0)
		theItem->kind = 0x1c;
	else if (strcmp(aType, "confirm_quit") == 0)
		theItem->kind = 0x21;
	theItem->showOnlyHot = Xml_GetInt(theTag, "show_only_hot");
	theItem->heightAdder = Xml_GetInt(theTag, "height_adder");
	theItem->box = Xml_GetInt(theTag, "box");
	theItem->boxSize = Xml_GetInt(theTag, "box_size");
	if (Xml_GetString(theTag, "fv_name") == nullptr)
		strcpy(theItem->name, Localize(Xml_GetString(theTag, "name")));
	else
		strcpy(theItem->name, Localize(Xml_GetString(theTag, "fv_name")));
	strcpy(theItem->shown, theItem->name);
	theItem->font = Font_Find(Xml_GetString(theTag, "font"));
	if (Xml_GetString(theTag, "hot_font") != nullptr)
		theItem->hotFont = Font_Find(Xml_GetString(theTag, "hot_font"));
	if (theItem->x == 0.0f && theItem->y == 0.0f)
	{
		// under the item before it
		theItem->x = theItem->prev->x;
		theItem->y = (float)((Font_Height(theItem->font) * 0xb) / 5) + theItem->prev->y;
	}
	theItem->firstParticle = Bank_IndexOf(&gArtBank, "bubble01");
	theItem->lastParticle = Bank_IndexOf(&gArtBank, "bubble02");
}

// 0x414770: the select-level slider follows the mouse while the button is held (from a click on it), from 200 to
// 600 across the level set, no further than two levels past the furthest one reached; it chooses the level
void MenuItem_Slide(MenuItem* theItem)
{
	Image* anImage = theItem->image;
	float aHalfW = (float)(anImage->w / 2);
	int aLeft = (int)((double)theItem->x - (double)aHalfW);
	int aRight = (int)((double)aHalfW + (double)theItem->x);
	float aHalfH = (float)(anImage->h / 2);
	int aTop = (int)((double)theItem->y - (double)aHalfH);
	int aBottom = (int)((double)aHalfH + (double)theItem->y);
	int aHeld = Mouse_State(1);
	if (aHeld == 0)
	{
		gSliding = aHeld;
		return;
	}
	if (gLevels.current->clicked == 0 || gMouseX <= aLeft || aRight <= gMouseX || gMouseY <= aTop ||
		aBottom <= gMouseY)
	{
		if (gSliding == 0)
			return;
	}
	else
		gSliding = 1;
	int anEnd = gLevelSets.current->end;
	int aLast = gProfiles[gProfile].levelReached + 2;
	if (anEnd <= gProfiles[gProfile].levelReached + 2)
		aLast = anEnd;
	int aX = ((aLast - gLevelSets.current->start) * 400) / (anEnd - gLevelSets.current->start) + 200;
	if (gMouseX <= aX)
		aX = gMouseX;
	float x = (float)aX;
	theItem->x = x;
	if (x < 200.0f)
		x = 200.0f;
	theItem->x = x;
	LevelSet* aSet = gLevelSets.current;
	int aStart = aSet->start;
	int aLevel = (((int)x - 200) * (aSet->end - aStart)) / 400 - 1 + aStart;
	if (aSet->end - 1 < aLevel)
		aLevel = aSet->end - 1;
	gLevelInSet = aLevel < 0 ? 0 : aLevel;
}

// 0x414920
void MenuItem_Draw(MenuItem* theItem)
{
	int aHot = theItem->hot;
	if (aHot == 0 && theItem->showOnlyHot == 1)
		return;
	if (gProfiles[gProfile].levelReached + 1 < gLevels.current->menus.current->lockedLevel)
		return;
	if (theItem->anim != nullptr)
	{
		if (theItem->frameCount != 2)
		{
			theItem->tint = aHot != 0 ? 2 : 5;
			if (theItem->kind != 0x18)
				theItem->shadow = 1;
			Obj_Draw(theItem);
			return;
		}
		// two frames: the second while hot
		theItem->frameTimer = 0;
		if (aHot != 0)
		{
			theItem->frame = 1;
			Obj_Draw(theItem);
			return;
		}
		theItem->frame = 0;
		Obj_Draw(theItem);
		return;
	}
	char aShown[0x100];
	char aDrawn[0xff];
	sprintf(aShown, "%s", theItem->name);
	sprintf(aDrawn, theItem->hot == 0 ? "^1%s" : "^4%s", theItem->name);
	int aKind = theItem->kind;
	if (aKind == 0x11)
		AddValue(aShown, aDrawn, theItem->hot, gAutoFire == 0 ? GT(0x4594d0) : GT(0x4594e0));
	else if (aKind == 0xf)
		AddValue(aShown, aDrawn, theItem->hot, Music_On() == 0 ? GT(0x4594c8) : GT(0x4594cc));
	else if (aKind == 0xe)
		AddValue(aShown, aDrawn, theItem->hot, Sound_On() == 0 ? GT(0x4594c8) : GT(0x4594cc));
	else if (aKind == 0xc)
		AddValue(aShown, aDrawn, theItem->hot, Screen_Fullscreen() == 0 ? GT(0x4594b4) : GT(0x4594bc));
	else if (aKind == 0xd)
		AddValue(aShown, aDrawn, theItem->hot, Screen_Centered() == 0 ? GT(0x45949c) : GT(0x4594a8));
	else if (aKind == 0x10)
	{
		if (gDifficulty == -1)
		{
			// (the original's format for the easiest has "5s" for "%s", so the drawn text shows "5s" there; fixes mod,
			// "easyLabel": the name, as for the other two)
			if (Sys_FixOn("easyLabel"))
				AddValue(aShown, aDrawn, theItem->hot, GT(0x459494));
			else
			{
				AppendTo(aDrawn, "%s^%d5s", (2 - theItem->hot) * 2, Localize(GT(0x459494)));
				strcat(aShown, Localize(GT(0x459494)));
			}
		}
		else if (gDifficulty == 0)
			AddValue(aShown, aDrawn, theItem->hot, GT(0x459484));
		else
			AddValue(aShown, aDrawn, theItem->hot, GT(0x45947c));
	}
	else if (aKind == 0x1a)
	{
		// the name in place of its marker, all in the font's first colour
		if (strstr(theItem->name, Localize(GT(0x459470))) != nullptr)
		{
			char* aMark = strstr(aShown, Localize(GT(0x459470)));
			strcpy(aMark, gProfiles[gProfile].name);
		}
		int i = 0;
		char c;
		do
		{
			c = aShown[i];
			aDrawn[i] = c;
			i = i + 1;
		} while (c != 0);
	}
	else if (aKind == 0x1b)
	{
		// a player's slot: the player's name, on a light bar when it is the current player
		int aSlot = atol(theItem->name);
		sprintf(aDrawn, "^%d%s", theItem->hot + 1, gProfiles[aSlot].name);
		sprintf(aShown, "%s", gProfiles[aSlot].name);
		if (aSlot == gProfile)
		{
			Draw_FillBlendTarget((int)((double)theItem->x - 92.0), (int)((double)theItem->y - 4.0),
				(int)((double)theItem->x + 91.0), (int)((double)theItem->y + 20.0), 0xff, 0xff, 0xff, 100);
			Dirty_AddClippedShown(&gDirty, (int)((double)theItem->x - 92.0), (int)((double)theItem->y - 4.0), 0xb8,
				0x19, kScreenClip);
		}
	}
	strcpy(theItem->shown, aShown);
	Font* aFont;
	if (theItem->hot == 0 || (aFont = theItem->hotFont) == nullptr)
		aFont = theItem->font;
	Font_SetEscapes(aFont, 1);
	Font_SetTarget(aFont, gScreenBits, gScreenPitch, &gBackBuffer.fmt, gBackBuffer.w, gBackBuffer.h);
	Font_SetPalette(aFont, nullptr);
	aFont->alpha = 0xff;
	aFont->align = 2;
	// a shadow a sixth of the font's height down and right
	uint32_t aShadowAlpha = 0x4b0 / Font_Height(theItem->font);
	int aShadowY = (int)((double)(Font_Height(theItem->font) / 6) + (double)theItem->y);
	int aShadowX = (int)((double)(Font_Height(theItem->font) / 6) + (double)theItem->x);
	Font_DrawFill(aFont, aShown, aShadowX, aShadowY, 2, 0, 0, 0, aShadowAlpha);
	int aRect[4];
	Font_DrawOutlined(aFont, aRect, (int)theItem->x, (int)theItem->y, aDrawn);
	double aX = (double)theItem->x - 1.0;
	int aLeft = (int)((aX - (double)(Font_StringWidth(aFont, aShown) / 2)) - 5.0);
	int aTop = (int)(((double)theItem->y - 1.0) - 5.0);
	int aRight = Font_StringWidth(aFont, aShown) + aLeft + 0xc;
	int aBottom = Font_StringHeight(aFont, aShown) + aTop + 0xc;
	int aW = aRight - aLeft;
	int aH = aBottom - aTop;
	if (theItem->box != 0)
	{
		int aSize = theItem->boxSize;
		if (aSize == 0)
		{
			Draw_Line(aLeft, aTop, aLeft, aBottom, 0xff, 0xff, 0xff, 0x80);
			Draw_Line(aRight, aTop, aRight, aBottom, 0xff, 0xff, 0xff, 0x80);
			Draw_Line(aLeft, aTop, aRight, aTop, 0xff, 0xff, 0xff, 0x80);
			Draw_Line(aLeft, aBottom, aRight, aBottom, 0xff, 0xff, 0xff, 0x80);
		}
		else
		{
			// a square of the box's size round (x, y), from half of it above y; the dirty part takes in both
			double x = theItem->x;
			double y = theItem->y;
			double aS = (float)aSize;
			int aLeftX = (int)(x - aS);
			Draw_Line(aLeftX, (int)(y - (double)(aSize / 2)), aLeftX, (int)(aS + y), 0xff, 0xff, 0xff, 0x80);
			aSize = theItem->boxSize;
			aS = (float)aSize;
			int aRightX = (int)(aS + x);
			Draw_Line(aRightX, (int)(y - (double)(aSize / 2)), aRightX, (int)(aS + y), 0xff, 0xff, 0xff, 0x80);
			aSize = theItem->boxSize;
			int aTopY = (int)(y - (double)(aSize / 2));
			aS = (float)aSize;
			Draw_Line((int)(x - aS), aTopY, (int)(aS + x), aTopY, 0xff, 0xff, 0xff, 0x80);
			aS = (float)theItem->boxSize;
			int aBottomY = (int)(y + aS);
			Draw_Line((int)(x - aS), aBottomY, (int)(aS + x), aBottomY, 0xff, 0xff, 0xff, 0x80);
			aSize = theItem->boxSize;
			aS = (float)aSize;
			double aEdge = x - aS;
			int aNewLeft = (int)((double)(float)aLeft < aEdge ? (double)(float)aLeft : aEdge);
			aEdge = aS + x;
			int aNewRight = (int)((double)(float)aRight <= aEdge ? aEdge : (double)(float)aRight);
			aEdge = y - (double)(aSize / 2);
			int aNewTop = (int)((double)(float)aTop < aEdge ? (double)(float)aTop : aEdge);
			aEdge = aS + y;
			int aNewBottom = (int)((double)(float)aBottom <= aEdge ? aEdge : (double)(float)aBottom);
			aLeft = aNewLeft;
			aTop = aNewTop;
			aW = aNewRight - aNewLeft;
			aH = aNewBottom - aNewTop;
		}
	}
	Dirty_AddClippedShown(&gDirty, aLeft, aTop, aW + 3, aH + 3, kScreenClip);
}

// 0x4159c0: an on-screen key: its character is added to the name being typed ('<' rubs one out)
void MenuItem_TypeKey(MenuItem* theItem)
{
	const char* aKey = theItem->name;
	if (theItem->name[0] == '<')
	{
		Key_Push(8);
		return;
	}
	List_Each(&gLevels.current->texts, [&](Text* aText) {
		if (aText->state == 3)
		{
			TextInput* anInput = &aText->input;
			char aName[0x100];
			aName[0] = 0;
			if (TextInput_GetText(anInput) != nullptr)
				strcpy(aName, TextInput_GetText(anInput));
			strcat(aName, aKey);
			// (the original's buffer holds 16 characters; the word after it is cleared)
			memset(aName + 16, 0, 4);
			TextInput_SetText(anInput, aName);
		}
	});
}

} // namespace

// 0x417080: the quit requester (the board's texts go first)
void MenuItem_ConfirmQuit(MenuItem* theItem)
{
	bool aMore;
	do
	{
		aMore = List_RemoveCurrent(&gLevels.current->texts);
	} while (aMore);
	int aProfile = gProfile;
	strcpy(gSavedName, gProfiles[gProfile].name);
	gSavedProfile = aProfile;
	const char* aSecond = Localize(GT(0x459668));
	Menu_ShowRequester(gLevels.current->menus.current, 3, Localize(GT(0x45964c)), aSecond);
	gLevels.current->state = 2;
	theItem->hot = 0;
}

namespace
{

// 0x417120: hot under the mouse (or always), and its kind's thing when clicked; 0 when the menu's screen is done
int MenuItem_Update(MenuItem* theItem)
{
	int aLeft = 0;
	int aRight = 0;
	int aTop = 0;
	int aResult = 1;
	int aBottom = 0;
	if (gDemo != 0)
	{
		// the demo clicks about at random
		gLevels.current->clicked = RandRange(0, 0x14) == 0 ? 1 : 0;
		gMouseX = RandRange(0, 799);
		gMouseY = RandRange(0, 599);
	}
	if (theItem->anim == nullptr)
	{
		// round its name
		float x = theItem->x;
		const char* aShown = theItem->shown;
		aLeft = (int)(((double)x - 1.0) - (double)(Font_StringWidth(theItem->font, aShown) / 2));
		aRight = (int)((double)(Font_StringWidth(theItem->font, aShown) / 2) + ((double)theItem->x + 1.0));
		float anAdder = (float)theItem->heightAdder;
		int aHeight = Font_StringHeight(theItem->font, aShown);
		aBottom = (int)((((double)aHeight + (double)theItem->y) + 1.0) + (double)anAdder);
		aTop = (int)(((double)theItem->y - (double)anAdder) + 1.0);
	}
	else
	{
		Obj_Update(theItem);
		Image* anImage = theItem->image;
		if (anImage != nullptr)
		{
			float aHalfW = (float)(anImage->w / 2);
			aLeft = (int)((double)theItem->x - (double)aHalfW);
			aRight = (int)((double)aHalfW + (double)theItem->x);
			float aHalfH = (float)(anImage->h / 2);
			aTop = (int)((double)theItem->y - (double)aHalfH);
			aBottom = (int)((double)aHalfH + (double)theItem->y);
		}
	}
	if (theItem->alwaysHot == 0 && theItem->autoClick == 0 &&
		(gMouseX <= aLeft || aRight <= gMouseX || gMouseY <= aTop || aBottom <= gMouseY))
		theItem->hot = 0;
	else
	{
		theItem->hot = 1;
		if ((gLevels.current->clicked != 0 &&
				gLevels.current->menus.current->lockedLevel <= gProfiles[gProfile].levelReached + 1) ||
			theItem->autoClick != 0)
		{
			Sound_Play(Sound_Find("audio\\click.wav"));
			Level* aLevel = gLevels.current;
			int aProfile = gProfile;
			switch (theItem->kind)
			{
			case 1:
				gGoTo = 0;
				aResult = 0;
				if (gLevels.current->kind == 6)
				{
					gProfiles[gProfile].savedLevel = 0;
					gProfiles[gProfile].savedLives = 0;
					gProfiles[gProfile].savedScore = 0;
				}
				SaveProfiles();
				break;
			case 2:
				if (gDemo == 0)
				{
					aResult = 0;
					gGoTo = 7;
				}
				break;
			case 3:
				SaveProfiles();
				if (gDemo == 0)
				{
					aResult = 0;
					gQuit = 1;
				}
				break;
			case 4:
				aResult = 0;
				gGoTo = 0xc;
				break;
			case 5:
				aResult = 0;
				gGoTo = 0xd;
				break;
			case 7:
				aResult = 0;
				if (gLevels.current->kind == 1)
					gScore = 0;
				gGoTo = (gScore <= gHighScores[7].score ? 1 : 0) + 2;
				break;
			case 8:
				aResult = 0;
				gGoTo = 0xf;
				break;
			case 9:
				aResult = 0;
				gGoTo = 3;
				SortHighScores();
				break;
			case 10:
				MenuItem_TypeKey(theItem);
				break;
			case 0xb:
				aResult = 0;
				gGoTo = 4;
				break;
			case 0xc:
				Screen_SetFullscreen(1 - Screen_Fullscreen());
				break;
			case 0xd:
				Screen_SetCentered(1 - Screen_Centered());
				break;
			case 0xe:
				Sound_SetOn(1 - Sound_On());
				break;
			case 0xf:
				Music_SetOn(1 - Music_On());
				break;
			case 0x10:
				gDifficulty = gDifficulty + 1;
				if (1 < gDifficulty)
					gDifficulty = -1;
				break;
			case 0x11:
				gAutoFire = 1 - gAutoFire;
				SaveProfiles();
				break;
			case 0x12:
				gLives = gDemo != 0 ? 299 : 6;
				aResult = 0;
				gScore = 0;
				if (gProfiles[gProfile].levelReached == 0 || gProfiles[gProfile].savedLives < 1)
					gGoTo = 5;
				else
					Menu_ShowRequester(gLevels.current->menus.tail, 5, GT(0x459728), GT(0x459744));
				break;
			case 0x13:
			{
				// up a level, no further than the furthest reached and the set's last
				int aReached = gProfiles[gProfile].levelReached;
				int aNext = gLevelInSet + 1;
				int aLevel = aNext;
				if (aReached <= aNext)
					aLevel = aReached;
				gLevelInSet = gLevelSets.current->end - 1;
				if (aLevel <= gLevelInSet)
				{
					gLevelInSet = aReached;
					if (aNext < aReached)
						gLevelInSet = aNext;
				}
				int aFirst = gLevelSets.current->start - 1;
				if (gLevelInSet < aFirst)
					gLevelInSet = aFirst;
				Level_UpdateProgressStars();
				break;
			}
			case 0x14:
			{
				if (gLevelSets.current->end < gLevelInSet)
					gLevelInSet = gLevelSets.current->end;
				int aLevel = gLevelSets.current->start - 1;
				if (aLevel <= gLevelInSet - 1)
					aLevel = gLevelInSet - 1;
				gLevelInSet = aLevel < 0 ? 0 : aLevel;
				Level_UpdateProgressStars();
				break;
			}
			case 0x16:
				aResult = 0;
				gGoTo = 0xe;
				break;
			case 0x18:
			{
				// a world: the level set holding this menu's locked level, from the furthest level reached in it
				aResult = 0;
				if (gLevelSets.current != nullptr)
				{
					gLevelSets.current = gLevelSets.head;
					do
					{
						int aLocked = aLevel->menus.current->lockedLevel;
						if (gLevelSets.current->start <= aLocked && aLocked <= gLevelSets.current->end)
						{
							gWorldChanged = 1;
							break;
						}
					} while (List_Next(&gLevelSets));
				}
				int aReached = gProfiles[gProfile].levelReached;
				gLevelInSet = aReached;
				if (gLevelSets.current->end < aReached + 1)
				{
					int aStart = gLevelSets.current->start;
					if (aStart < 1)
						aStart = 1;
					gLevelInSet = aStart - 1;
				}
				if (aReached == 0 || (gLevelInSet < gLevelSets.current->start && aReached + 1 <= gLevelSets.current->end))
				{
					gProfiles[aProfile].savedLevel = 0;
					gProfiles[aProfile].savedLives = 0;
					gGoTo = 0;
					gProfiles[aProfile].savedScore = 0;
				}
				else
					gGoTo = 6;
				break;
			}
			case 0x19:
				aResult = 0;
				gGoTo = 1;
				break;
			case 0x1a:
				aResult = 0;
				gGoTo = 0x10;
				break;
			case 0x1b:
				// a player's slot: an empty one asks for a name, the current one goes back, another is chosen
				strcpy(gSavedName, gProfiles[gProfile].name);
				gSavedProfile = aProfile;
				if (strcmp(gProfiles[atol(theItem->name)].name, Localize(GT(0x458ac4))) == 0)
				{
					theItem->hot = 0;
					gProfile = atol(theItem->name);
					Menu_ShowRequester(gLevels.current->menus.current, 2, Localize(GT(0x4596dc)), nullptr);
#ifdef __EMSCRIPTEN__
					// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with it
					SiteFillName(&gLevels.current->menus.current->requesters.current->entry->input);
#endif
				}
				else if (gProfile == atol(theItem->name))
				{
					aResult = 0;
					gGoTo = 1;
				}
				else
				{
					gProfile = atol(theItem->name);
					gAutoFire = gProfiles[gProfile].autoFire;
				}
				break;
			case 0x1c:
				// rename: the name entry starts empty
				theItem->hot = 0;
				gSavedProfile = gProfile;
				strcpy(gSavedName, gProfiles[gProfile].name);
				gProfiles[gProfile].name[0] = 0;
				Menu_ShowRequester(gLevels.current->menus.current, 2, Localize(GT(0x4596c4)), nullptr);
				TextInput_SetText(&gLevels.current->menus.current->requesters.current->entry->input,
					gProfiles[gProfile].name);
#ifdef __EMSCRIPTEN__
				SiteFillName(&gLevels.current->menus.current->requesters.current->entry->input);	// as above
#endif
				break;
			case 0x1d:
				Level_SetUserName(
					TextInput_GetText(&gLevels.current->menus.current->requesters.current->entry->input));
				break;
			case 0x1e:
			{
				// delete, unless it is the only player
				int aPlayers = 0;
				for (Profile* aSlot = gProfiles; aSlot < gProfiles + 10; aSlot++)
				{
					if (strcmp(aSlot->name, Localize(GT(0x458ac4))) != 0)
						aPlayers = aPlayers + 1;
				}
				theItem->hot = 0;
				gSavedProfile = gProfile;
				if (aPlayers < 2)
				{
					strcpy(gSavedName, gProfiles[gProfile].name);
					const char* aSecond = Localize(GT(0x45969c));
					Menu_ShowRequester(gLevels.current->menus.current, 1, Localize(GT(0x459684)), aSecond);
				}
				else
				{
					strcpy(gSavedName, gProfiles[gProfile].name);
					Menu_ShowRequester(gLevels.current->menus.current, 0, GT(0x4596b8), nullptr);
				}
				break;
			}
			case 0x1f:
				// the delete's okay: the slot is emptied and the first player left is chosen
				gProfiles[gProfile].autoFire = 1;
				gProfiles[gProfile].levelReached = 0;
				strcpy(gProfiles[gProfile].name, Localize(GT(0x458ac4)));
				for (int i = 0; i < 10; i++)
				{
					if (strcmp(gProfiles[i].name, Localize(GT(0x458ac4))) != 0)
					{
						gProfile = i;
						i = 10;
					}
				}
				SaveProfiles();
				aResult = 0;
				break;
			case 0x20:
				// cancel: back to the player and name before
				if (gLevels.current->kind == 0x10)
				{
					gProfile = gSavedProfile;
					strcpy(gProfiles[gSavedProfile].name, gSavedName);
				}
				aLevel->state = 1;
				aResult = 0;
				break;
			case 0x21:
				MenuItem_ConfirmQuit(theItem);
				break;
			case 0x22:
			{
				// resume the saved game
				int aSaved = gProfiles[gProfile].savedLevel;
				gScore = gProfiles[gProfile].savedScore;
				gLives = gProfiles[gProfile].savedLives;
				aResult = 0;
				gGoTo = 0xf;
				gLevelInSet = aSaved;
				if (gLevelSets.current != nullptr)
				{
					gLevelSets.current = gLevelSets.head;
					do
					{
						if (gLevelSets.current->start <= aSaved && aSaved <= gLevelSets.current->end)
							break;
					} while (List_Next(&gLevelSets));
				}
				break;
			}
			case 0x23:
				aResult = 0;
				gLives = gDemo != 0 ? 299 : 6;
				gScore = 0;
				gGoTo = (gProfiles[gProfile].levelReached == 0 ? 1 : 0) * 8 + 5;
				break;
			}
		}
		if (gLevels.current->menus.current->lockedLevel <= gProfiles[gProfile].levelReached + 1 &&
			RandRange(1, 0x1e) == 1)
		{
			// a bubble at the mouse
			ParticleSystem_New();
			int aLifetime = RandRange(600, 700);
			int aLast = Bank_IndexOf(&gArtBank, "bubble03");
			int aFirst = Bank_IndexOf(&gArtBank, "bubble01");
			float aVy = (float)((double)RandRange(-0x1e, 0x1e) / 200.0);
			float aVx = (float)((double)RandRange(-10, 10) / 200.0);
			ParticleSystem_Add(gLevels.current->systems.current, (float)gMouseX, (float)gMouseY, aVx, aVy, aFirst,
				aLast, 0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 400, 10, aLifetime, 0.998f, -0.0002f, 5);
		}
	}
	if (theItem->kind == 0x15)
	{
		if (gWorldChanged != 0)
		{
			gWorldChanged = 0;
			theItem->age = 0;
		}
		MenuItem_Slide(theItem);
		if ((double)theItem->age < 5.0)
			Level_UpdateProgressStars();
	}
	return aResult;
}

} // namespace

// 0x4155f0
void MenuItem_Insert(List<MenuItem>* theList)
{
	MenuItem* anItem = (MenuItem*)::operator new(sizeof(MenuItem));
	Obj_Init(anItem);
	anItem->autoClick = 0;
	anItem->image = nullptr;
	anItem->box = 0;
	anItem->showOnlyHot = 0;
	anItem->hotFont = nullptr;
	anItem->alwaysHot = 0;
	List_InsertBefore(theList, anItem);
}

void HighScore_Entered(const char*)
{
	// the high-score screen's buttons go on by themselves
	List_Each(&gLevels.current->menus, [](Menu* aMenu) {
		List_Each(&aMenu->items, [](MenuItem* anItem) {
			int aKind = anItem->kind;
			if (aKind == 9 || aKind == 1 || aKind == 0x19)
				anItem->autoClick = 1;
		});
	});
}

void Menu_FromXml(Menu* theMenu, const char* p, const XmlTag* theTag)
{
	Obj_FromXml(theMenu, p, theTag);
	theMenu->logo = nullptr;
	theMenu->lockedLevel = Xml_GetInt(theTag, "locked_level");
	if (Xml_GetString(theTag, "locked_animation") != nullptr)
		strcpy(theMenu->lockedAnim, Xml_GetString(theTag, "locked_animation"));
	if (theMenu->anim != nullptr && gProfiles[gProfile].levelReached + 1 < theMenu->lockedLevel)
	{
		theMenu->anim = Animation_Find(theMenu->lockedAnim, &theMenu->frameCount, &theMenu->frameTime);
		theMenu->image = Animation_Frame(theMenu->anim, 0);
	}
	if (Xml_GetString(theTag, "logo_animation") != nullptr)
	{
		int aCount;
		int aFrameTime;
		theMenu->logo = Animation_Frame(Animation_Find(Xml_GetString(theTag, "logo_animation"), &aCount, &aFrameTime), 0);
		theMenu->logoX = (Xml_GetInt(theTag, "logo_x") * 800) / 800;
		theMenu->logoY = (Xml_GetInt(theTag, "logo_y") * 600) / 600;
	}
	Menu_DrawToBackground(theMenu);
	XmlTag* aTag = const_cast<XmlTag*>(theTag);
	while (strcmp(aTag->name, "/menu") != 0)
	{
		if (strcmp(aTag->name, "menu_item") == 0)
		{
			MenuItem_Append(&theMenu->items);
			MenuItem_FromXml(theMenu->items.current, p, aTag);
		}
		p = Xml_NextTag(p, aTag);
	}
}

void Menu_Draw(Menu* theMenu)
{
	// the menu, then its newest requester, and that one's, ...
	while (true)
	{
		if (theMenu->kind == 1)
		{
			float anAlpha = theMenu->age <= 255.0f ? theMenu->age : 255.0f;
			theMenu->alpha = (int)anAlpha;
			Obj_Draw(theMenu);
		}
		List_Each(&theMenu->items, [](MenuItem* anItem) { MenuItem_Draw(anItem); });
		if (theMenu->requesters.current == nullptr)
			return;
		theMenu->requesters.current = theMenu->requesters.head;
		theMenu = theMenu->requesters.head;
	}
}

Menu* Menu_Init(Menu* theMenu)
{
	Obj_Init(theMenu);
	List_Init(&theMenu->items);
	List_Init(&theMenu->requesters);
	theMenu->textCount = 0;
	theMenu->entry = nullptr;
	theMenu->image = nullptr;
	theMenu->logo = nullptr;
	theMenu->lockedLevel = 0;
	theMenu->lockedAnim[0] = 0;
	return theMenu;
}

void Menu_Destroy(Menu* theMenu)
{
	// a requester's texts leave the level's
	Level* aLevel = gLevels.current;
	for (int i = 0; i < theMenu->textCount; i++)
	{
		Text* aText = theMenu->texts[i];
		List_Each(&aLevel->texts, [&](Text* aCurrent) {
			if (aText == aCurrent)
				List_RemoveCurrent(&gLevels.current->texts);
		});
		aLevel = gLevels.current;
	}
	if (theMenu->entry != nullptr)
	{
		Text* anEntry = theMenu->entry;
		List_Each(&aLevel->texts, [&](Text* aCurrent) {
			if (anEntry == aCurrent)
				List_RemoveCurrent(&gLevels.current->texts);
		});
	}
	List_Clear(&theMenu->requesters);
	List_Clear(&theMenu->items);
	Obj_Destroy(theMenu);
}

void List_DeleteNode(Menu* theMenu)
{
	Menu_Destroy(theMenu);
	::operator delete(theMenu);
}

void Menu_ShowRequester(Menu* theMenu, int theKind, const char* theText, const char* theSecond)
{
	char aBuf[0x100];
	List<Menu>* aList = &theMenu->requesters;
	Requester_Push(aList);
	Menu* aRequester = aList->current;
	aRequester->kind = 1;
	aRequester->state = theKind;
	aRequester->anim = Animation_Find("requester", &aRequester->frameCount, &aRequester->frameTime);
	aRequester->image = Animation_Frame(aRequester->anim, 0);
	aRequester->midHandle = 1;
	aRequester->dirty = 1;
	aRequester->drawKind = 1;
	aRequester->x = 400.0f;
	aRequester->y = 300.0f;
	aRequester->depth = 300;
	Text_Append(&gLevels.current->texts);
	aList->current->texts[0] = gLevels.current->texts.current;
	aList->current->textCount = 1;
	MenuItem* anItem;
	switch (theKind)
	{
	case 0:
		// deleting the player
		sprintf(aBuf, "%s%s,", Localize(GT(0x459634)), gProfiles[gProfile].name);
		Text_Make(aList->current->texts[0], aBuf, Font_Find("bonus_font"), 400, 0xfa, 0, 100, 0x986fc8, 500, 0, 0);
		Requester_AddText(aList->current);
		sprintf(aBuf, "%s", Localize(GT(0x45961c)));
		Text_Make(aList->current->texts[aList->current->textCount], aBuf, Font_Find("bonus_font"), 400, 0x10c, 0, 100,
			0x986fc8, 500, 0, 0);
		aList->current->textCount = aList->current->textCount + 1;
		MenuItem_Insert(&aList->current->items);
		anItem = aList->current->items.current;
		Button_Setup(anItem, "okay", 340.0f, 330.0f);
		anItem->midHandle = 1;
		anItem->kind = 0x1f;
		break;
	case 1:
		// a message to close
		Text_Make(aList->current->texts[0], theText, Font_Find("bonus_font"), 400, 0xfa, 0, 0, 0x986fc8, 500, 0, 0);
		if (theSecond != nullptr)
		{
			Text* aText = Requester_AddText(aList->current);
			Text_Make(aText, theSecond, Font_Find("bonus_font"), 400, 0x10c, 0, 100, 0x986fc8, 500, 0, 0);
			aList->current->textCount = aList->current->textCount + 1;
		}
		MenuItem_Append(&aList->current->items);
		anItem = aList->current->items.current;
		Button_Setup(anItem, "okay", 400.0f, 330.0f);
		anItem->midHandle = 1;
		anItem->kind = 0x20;
		strcpy(anItem->name, GT(0x4595f0));
		return;
	case 2:
	{
		// a name to type
		Text_Append(&gLevels.current->texts);
		aList->current->entry = gLevels.current->texts.current;
		Text* anEntry = aList->current->entry;
		anEntry->font = Font_Find("bonus_font");
		TextInput* anInput = &anEntry->input;
		TextInput_Init(anInput, nullptr, nullptr, Font_Find("bonus_font"), 0xc, 4, 0x113, 0x113, 0x20d, 300);
		TextInput_SetFocus(anInput, 1);
		TextInput_SetFillColour(anInput, 100, 100, 100, 100);
		TextInput_SetOutline(anInput, 0);
		TextInput_SetOnEnter(anInput, Level_SetUserName);
		anEntry->state = 3;
		Text_Make(aList->current->texts[0], theText, Font_Find("bonus_font"), 400, 0xfa, 0, 0, 0x986fc8, 500, 0, 0);
		MenuItem_Insert(&aList->current->items);
		anItem = aList->current->items.current;
		Button_Setup(anItem, "okay", 340.0f, 330.0f);
		anItem->midHandle = 1;
		anItem->kind = 0x1d;
		break;
	}
	case 3:
		// leaving the game
		Text_Make(aList->current->texts[0], theText, Font_Find("bonus_font"), 400, 0xfa, 0, 100, 0x986fc8, 500, 0, 0);
		if (theSecond != nullptr)
		{
			Text* aText = Requester_AddText(aList->current);
			Text_Make(aText, theSecond, Font_Find("bonus_font"), 400, 0x10c, 0, 100, 0x986fc8, 500, 0, 0);
			aList->current->textCount = aList->current->textCount + 1;
		}
		MenuItem_Insert(&aList->current->items);
		anItem = aList->current->items.current;
		Button_Setup(anItem, "okay", 340.0f, 330.0f);
		anItem->midHandle = 1;
		anItem->kind = 0x19;
		break;
	case 4:
	{
		// a board's end: its fill and shots against the records, and the bonus
		aRequester = aList->current;
		aRequester->anim = Animation_Find("end_level_panel", &aRequester->frameCount, &aRequester->frameTime);
		aRequester->alpha = 0;
		aRequester->drawKind = 4;
		aRequester->y = 260.0f;
		Text_Make(aRequester->texts[0], theText, Font_Find("bonus_font"), 400, 0x5c, 0, 100, 0x986fc8, 500, 0, 0);
		if (theSecond != nullptr)
		{
			Level* aLevel = gLevels.current;
			LevelRecord* aRecord;
			Text* aText = Requester_AddText(aList->current);
			Text_Make(aText, theSecond, Font_Find("bonus_font2"), 400, 0x87, 0, 0, 0x986fc8, 500, 0, 0);
			aList->current->textCount = aList->current->textCount + 1;
			sprintf(aBuf, "%d%s", (aLevel->filledCells * 100) / aLevel->totalCells, "%");
			aText = Requester_AddText(aList->current);
			Text_Make(aText, aBuf, Font_Find("bonus_font"), 400, 0x98, 300, 0x32, 0x986fc8, 500, 0, 0);
			aList->current->texts[aList->current->textCount]->appearSound = Sound_Find("audio\\pop.wav");
			aList->current->textCount = aList->current->textCount + 1;
			aText = Requester_AddText(aList->current);
			Text_Make(aText, GT(0x4595ac), Font_Find("bonus_font2"), 400, 0xb4, 0, 0, 0x986fc8, 500, 0, 0);
			aList->current->textCount = aList->current->textCount + 1;
			aRecord = &gLevelRecords[gLevels.current->number];
			sprintf(aBuf, "%s, %d%s", aRecord->fillName, aRecord->bestFill, "%");
			aText = Requester_AddText(aList->current);
			Text_Make(aText, aBuf, Font_Find("bonus_font"), 400, 0xc5, 600, 0x32, 0x986fc8, 500, 0, 0);
			aList->current->texts[aList->current->textCount]->appearSound = Sound_Find("audio\\pop.wav");
			aList->current->textCount = aList->current->textCount + 1;
			aText = Requester_AddText(aList->current);
			Text_Make(aText, GT(0x45958c), Font_Find("bonus_font2"), 400, 0xeb, 0, 0, 0x986fc8, 500, 0, 0);
			aList->current->textCount = aList->current->textCount + 1;
			sprintf(aBuf, "%d", gLevels.current->shots);
			aText = Requester_AddText(aList->current);
			Text_Make(aText, aBuf, Font_Find("bonus_font"), 400, 0xfc, 900, 0x32, 0x986fc8, 500, 0, 0);
			aList->current->texts[aList->current->textCount]->appearSound = Sound_Find("audio\\pop.wav");
			aList->current->textCount = aList->current->textCount + 1;
			aText = Requester_AddText(aList->current);
			Text_Make(aText, GT(0x45957c), Font_Find("bonus_font2"), 400, 0x118, 0, 0, 0x986fc8, 500, 0, 0);
			aList->current->textCount = aList->current->textCount + 1;
			aRecord = &gLevelRecords[gLevels.current->number];
			sprintf(aBuf, "%s, %d", aRecord->shotsName, aRecord->fewestShots);
			aText = Requester_AddText(aList->current);
			Text_Make(aText, aBuf, Font_Find("bonus_font"), 400, 0x129, 0x4b0, 0x32, 0x986fc8, 500, 0, 0);
			aList->current->texts[aList->current->textCount]->appearSound = Sound_Find("audio\\pop.wav");
			aList->current->textCount = aList->current->textCount + 1;
			aText = Requester_AddText(aList->current);
			Text_Make(aText, GT(0x459564), Font_Find("bonus_font2"), 400, 0x14f, 0, 0, 0x986fc8, 500, 0, 0);
			aList->current->textCount = aList->current->textCount + 1;
			sprintf(aBuf, "%d", gLevels.current->bonus);
			aText = Requester_AddText(aList->current);
			Text_Make(aText, aBuf, Font_Find("bonus_font"), 400, 0x160, 0x5dc, 0x32, 0x986fc8, 500, 0, 0);
			aList->current->texts[aList->current->textCount]->appearSound = Sound_Find("audio\\pop.wav");
			aList->current->textCount = aList->current->textCount + 1;
		}
		// a click anywhere goes on
		MenuItem_Append(&aList->current->items);
		anItem = aList->current->items.current;
		anItem->dirty = 1;
		anItem->midHandle = 1;
		anItem->drawKind = 1;
		anItem->font = Font_Find("bonus_font");
		anItem->hotFont = Font_Find("bonus_font");
		anItem->x = 400.0f;
		anItem->y = 416.0f;
		anItem->depth = 0x15e;
		anItem->midHandle = 1;
		anItem->kind = 8;
		anItem->name[0] = ' ';
		anItem->name[1] = 0;
		anItem->alwaysHot = 1;
		anItem->showOnlyHot = 0;
		{
			Text* aText = Requester_AddText(aList->current);
			Text_Make(aText, GT(0x45954c), Font_Find("bonus_font"), 400, 400, 0, 300, 300, 300, 1, 0);
			aList->current->textCount = aList->current->textCount + 1;
		}
		return;
	}
	case 5:
		// yes or no
		Text_Make(aList->current->texts[0], theText, Font_Find("bonus_font"), 400, 0xfa, 0, 100, 0x986fc8, 500, 0, 0);
		if (theSecond != nullptr)
		{
			Text* aText = Requester_AddText(aList->current);
			Text_Make(aText, theSecond, Font_Find("bonus_font"), 400, 0x10c, 0, 100, 0x986fc8, 500, 0, 0);
			aList->current->textCount = aList->current->textCount + 1;
		}
		MenuItem_Insert(&aList->current->items);
		anItem = aList->current->items.current;
		Button_Setup(anItem, "yes", 340.0f, 330.0f);
		anItem->midHandle = 1;
		anItem->kind = 0x22;
		strcpy(anItem->name, GT(0x416750));
		MenuItem_Append(&aList->current->items);
		anItem = aList->current->items.current;
		Button_Setup(anItem, "no", 460.0f, 330.0f);
		anItem->midHandle = 1;
		anItem->kind = 0x23;
		strcpy(anItem->name, GT(0x4595e4));
		return;
	default:
		return;
	}
	// cases 0, 2 and 3: the okay button's name and a cancel button
	strcpy(aList->current->items.current->name, GT(0x459604));
	MenuItem_Append(&aList->current->items);
	anItem = aList->current->items.current;
	Button_Setup(anItem, "cancel", 460.0f, 330.0f);
	anItem->midHandle = 1;
	anItem->kind = 0x20;
	strcpy(anItem->name, GT(0x4595f0));
}

int Menu_Update(Menu* theMenu)
{
	int aResult = 1;
	Obj_Update(theMenu);
	List<Menu>* aRequesters = &theMenu->requesters;
	if (theMenu->requesters.current == nullptr)
	{
		List_Each(&theMenu->items, [&](MenuItem* anItem) { aResult = aResult & MenuItem_Update(anItem); });
		return aResult;
	}
	aRequesters->current = theMenu->requesters.head;
	if (Menu_Update(theMenu->requesters.head) != 0)
	{
		// a requester asking for a name stays up until it has one
		const char* aName = gProfiles[gProfile].name;
		if (strcmp(aName, Localize(GT(0x458ac4))) == 0)
			return 1;
		if (aName[0] == 0)
			return 1;
		if (aRequesters->current->state != 2)
			return 1;
	}
	List_RemoveCurrent(aRequesters);
	if (gGoTo != 1 && gGoTo != 5 && gGoTo != 0xd)
	{
		if (gGoTo != 0xf)
			return 1;
		gGoTo = 0;
	}
	return 0;
}

} // namespace wb
