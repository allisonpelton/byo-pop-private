// The menus (0x413d10..0x417d40, and 0x40cf70): a menu of the game file is a list of items, each drawn with its
// animation or as its name in a font, hot under the mouse and doing its kind's thing when clicked: going to another
// screen, changing a setting, choosing a level, a world or a player. A requester (a box with texts and buttons, such
// as "are you sure?") goes over a menu, and only it is used while it is up.

#include "CBLevel.h"
#include "R64Sys.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif

namespace cb
{

namespace
{

int gSliding = 0;                  // 0x6e5bf8  the level slider is being dragged
int gWorldChanged = 0;             // 0x6e5bfc  a world was chosen: the slider starts again
int gSavedProfile = 0;             // 0x6e5af0  the player, and the name, to go back to when a requester is cancelled
char gSavedName[0x20];             // 0x6e5af8

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

// 0x415870: a requester at the front of the list, made current
void Requester_Push(List<Menu>* theList)
{
	Menu* aMenu = (Menu*)::operator new(sizeof(Menu));
	Menu_Init(aMenu);
	if (theList->mCurrent == nullptr)
	{
		aMenu->mNext = nullptr;
		aMenu->mPrev = nullptr;
		theList->mTail = aMenu;
	}
	else
	{
		aMenu->mNext = theList->mHead;
		aMenu->mPrev = nullptr;
		theList->mHead->mPrev = aMenu;
	}
	theList->mCurrent = aMenu;
	theList->mHead = aMenu;
}

// 0x415270
void MenuItem_Append(List<MenuItem>* theList)
{
	MenuItem* anItem = (MenuItem*)::operator new(sizeof(MenuItem));
	Obj_Init(anItem);
	anItem->mAutoClick = 0;
	anItem->mImage = nullptr;
	anItem->mBox = 0;
	anItem->mShowOnlyHot = 0;
	anItem->mHotFont = nullptr;
	anItem->mAlwaysHot = 0;
	List_Append(theList, anItem);
}

// 0x4151b0
void MenuItem_Insert(List<MenuItem>* theList)
{
	MenuItem* anItem = (MenuItem*)::operator new(sizeof(MenuItem));
	Obj_Init(anItem);
	anItem->mAutoClick = 0;
	anItem->mImage = nullptr;
	anItem->mBox = 0;
	anItem->mShowOnlyHot = 0;
	anItem->mHotFont = nullptr;
	anItem->mAlwaysHot = 0;
	List_InsertBefore(theList, anItem);
}

// A requester's button: one of the art's animations at (x, y)
void Button_Setup(MenuItem* theItem, const char* theAnim, float x, float y)
{
	theItem->mDirty = 1;
	theItem->mMidHandle = 1;
	theItem->mDrawKind = 1;
	theItem->mAnim = Animation_Find(theAnim, &theItem->mFrameCount, &theItem->mFrameTime);
	theItem->mImage = Animation_Frame(theItem->mAnim, 0);
	theItem->mX = x;
	theItem->mY = y;
	theItem->mDepth = 0x15e;
}

// A requester's next text, one of the level's
Text* Requester_AddText(Menu* theRequester)
{
	Text_Append(&gLevels.mCurrent->mTexts);
	theRequester->mTexts[theRequester->mTextCount] = gLevels.mCurrent->mTexts.mCurrent;
	return theRequester->mTexts[theRequester->mTextCount];
}

// 0x413d10: an item's attributes; its type is its kind
void MenuItem_FromXml(MenuItem* theItem, const char* p, const XmlTag* theTag)
{
	// (the original leaves the type unset when there is none)
	char aType[0x20] = "";
	Obj_FromXml(theItem, p, theTag);
	// (the full version's type, "fv_type", is the one: the title's BUY NOW item is its Help)
	if (Xml_GetString(theTag, "fv_type") == nullptr)
	{
		if (Xml_GetString(theTag, "type") != nullptr)
			strcpy(aType, Xml_GetString(theTag, "type"));
	}
	else
		strcpy(aType, Xml_GetString(theTag, "fv_type"));
	if (strcmp(aType, "help") == 0)
		theItem->mKind = ITEM_HELP;
	else if (strcmp(aType, "help2") == 0)
		theItem->mKind = ITEM_HELP2;
	else if (strcmp(aType, "start") == 0)
		theItem->mKind = ITEM_START;
	else if (strcmp(aType, "quit") == 0)
		theItem->mKind = ITEM_QUIT;
	else if (strcmp(aType, "credits") == 0)
		theItem->mKind = ITEM_CREDITS;
	else if (strcmp(aType, "back") == 0)
		theItem->mKind = ITEM_BACK;
	else if (strcmp(aType, "options") == 0)
		theItem->mKind = ITEM_OPTIONS;
	else if (strcmp(aType, "music") == 0)
		theItem->mKind = ITEM_MUSIC;
	else if (strcmp(aType, "sound") == 0)
		theItem->mKind = ITEM_SOUND;
	else if (strcmp(aType, "video") == 0)
		theItem->mKind = ITEM_VIDEO;
	else if (strcmp(aType, "lcd") == 0)
		theItem->mKind = ITEM_LCD;
	else if (strcmp(aType, "difficulty") == 0)
		theItem->mKind = ITEM_DIFFICULTY;
	else if (strcmp(aType, "high_scores") == 0)
		theItem->mKind = ITEM_HIGH_SCORES;
	else if (strcmp(aType, "hs_entered") == 0)
		theItem->mKind = ITEM_HS_ENTERED;
	else if (strcmp(aType, "select_level") == 0)
		theItem->mKind = ITEM_SELECT_LEVEL;
	else if (strcmp(aType, "select_up") == 0)
		theItem->mKind = ITEM_SELECT_UP;
	else if (strcmp(aType, "select_down") == 0)
		theItem->mKind = ITEM_SELECT_DOWN;
	else if (strcmp(aType, "select_slide") == 0)
		theItem->mKind = ITEM_SELECT_SLIDE;
	else if (strcmp(aType, "shooter_mode") == 0)
		theItem->mKind = ITEM_SHOOTER_MODE;
	else if (strcmp(aType, "next_level") == 0)
		theItem->mKind = ITEM_NEXT_LEVEL;
	else if (strcmp(aType, "closing_nag") == 0)
		theItem->mKind = ITEM_QUIT;  // (the title's QUIT: it quits)
	else if (strcmp(aType, "hs_stuff") == 0)
		theItem->mKind = ITEM_HS_STUFF;
	else if (strcmp(aType, "select_world") == 0)
		theItem->mKind = ITEM_SELECT_WORLD;
	else if (strcmp(aType, "change_user") == 0)
		theItem->mKind = ITEM_CHANGE_USER;
	else if (strcmp(aType, "user_slot") == 0)
		theItem->mKind = ITEM_USER_SLOT;
	else if (strcmp(aType, "user_rename") == 0)
		theItem->mKind = ITEM_USER_RENAME;
	else if (strcmp(aType, "user_delete") == 0)
		theItem->mKind = ITEM_USER_DELETE;
	else if (strcmp(aType, "user_rename") == 0)
		theItem->mKind = ITEM_USER_RENAME;
	else if (strcmp(aType, "confirm_quit") == 0)
		theItem->mKind = ITEM_CONFIRM_QUIT;
	theItem->mShowOnlyHot = Xml_GetInt(theTag, "show_only_hot");
	theItem->mHeightAdder = Xml_GetInt(theTag, "height_adder");
	theItem->mBox = Xml_GetInt(theTag, "box");
	theItem->mBoxSize = Xml_GetInt(theTag, "box_size");
	if (Xml_GetString(theTag, "fv_name") == nullptr)
		strcpy(theItem->mName, Localize(Xml_GetString(theTag, "name")));
	else
		strcpy(theItem->mName, Localize(Xml_GetString(theTag, "fv_name")));
	strcpy(theItem->mShown, theItem->mName);
	theItem->mFont = Font_Find(Xml_GetString(theTag, "font"));
	if (Xml_GetString(theTag, "hot_font") != nullptr)
		theItem->mHotFont = Font_Find(Xml_GetString(theTag, "hot_font"));
	if (theItem->mX == 0.0f && theItem->mY == 0.0f)
	{
		// under the item before it
		theItem->mX = theItem->mPrev->mX;
		theItem->mY = (float)((Font_Height(theItem->mFont) * 0xb) / 5) + theItem->mPrev->mY;
	}
	theItem->mFirstParticle = Bank_IndexOf(&gArtBank, "particle_0");
	theItem->mLastParticle = Bank_IndexOf(&gArtBank, "particle_0");
}

// 0x414330: the select-level slider follows the mouse while the button is held (from a click on it), from 200 to
// 600 across the level set, no further than two levels past the furthest one reached; it chooses the level
void MenuItem_Slide(MenuItem* theItem)
{
	Image* anImage = theItem->mImage;
	float aHalfW = (float)(anImage->mW / 2);
	int aLeft = (int)((double)theItem->mX - (double)aHalfW);
	int aRight = (int)((double)aHalfW + (double)theItem->mX);
	float aHalfH = (float)(anImage->mH / 2);
	int aTop = (int)((double)theItem->mY - (double)aHalfH);
	int aBottom = (int)((double)aHalfH + (double)theItem->mY);
	int aHeld = Mouse_State(1);
	if (aHeld == 0)
	{
		gSliding = aHeld;
		return;
	}
	if (gLevels.mCurrent->mClicked == 0 || gMouseX <= aLeft || aRight <= gMouseX || gMouseY <= aTop ||
		aBottom <= gMouseY)
	{
		if (gSliding == 0)
			return;
	}
	else
		gSliding = 1;
	int anEnd = gLevelSets.mCurrent->mEnd;
	int aLast = gProfiles[gProfile].mLevelReached + 2;
	if (anEnd <= gProfiles[gProfile].mLevelReached + 2)
		aLast = anEnd;
	int aX = ((aLast - gLevelSets.mCurrent->mStart) * 400) / (anEnd - gLevelSets.mCurrent->mStart) + 200;
	if (gMouseX <= aX)
		aX = gMouseX;
	float x = (float)aX;
	theItem->mX = x;
	if (x < 200.0f)
		x = 200.0f;
	theItem->mX = x;
	LevelSet* aSet = gLevelSets.mCurrent;
	int aStart = aSet->mStart;
	int aLevel = (((int)x - 200) * (aSet->mEnd - aStart)) / 400 - 1 + aStart;
	if (aSet->mEnd - 1 < aLevel)
		aLevel = aSet->mEnd - 1;
	gLevelInSet = aLevel < 0 ? 0 : aLevel;
}

// 0x4144e0
void MenuItem_Draw(MenuItem* theItem)
{
	int aHot = theItem->mHot;
	if (aHot == 0 && theItem->mShowOnlyHot == 1)
		return;
	if (gProfiles[gProfile].mLevelReached + 1 < gLevels.mCurrent->mMenus.mCurrent->mLockedLevel)
		return;
	if (theItem->mAnim != nullptr)
	{
		if (theItem->mFrameCount != 2)
		{
			theItem->mTint = aHot != 0 ? 2 : 5;
			if (theItem->mKind != ITEM_SELECT_WORLD)
				theItem->mShadow = 1;
			Obj_Draw(theItem);
			return;
		}
		// two frames: the second while hot
		theItem->mFrameTimer = 0;
		if (aHot != 0)
		{
			theItem->mFrame = 1;
			Obj_Draw(theItem);
			return;
		}
		theItem->mFrame = 0;
		Obj_Draw(theItem);
		return;
	}
	char aShown[0x100];
	char aDrawn[0xff];
	sprintf(aShown, "%s", theItem->mName);
	sprintf(aDrawn, theItem->mHot == 0 ? "^1%s" : "^4%s", theItem->mName);
	int aKind = theItem->mKind;
	if (aKind == ITEM_SHOOTER_MODE)
		AddValue(aShown, aDrawn, theItem->mHot, gAutoFire == 0 ? GT(0x459350) : GT(0x459360));
	else if (aKind == ITEM_MUSIC)
		AddValue(aShown, aDrawn, theItem->mHot, Music_On() == 0 ? GT(0x459348) : GT(0x45934c));
	else if (aKind == ITEM_SOUND)
		AddValue(aShown, aDrawn, theItem->mHot, Sound_On() == 0 ? GT(0x459348) : GT(0x45934c));
	else if (aKind == ITEM_VIDEO)
		AddValue(aShown, aDrawn, theItem->mHot, Screen_Fullscreen() == 0 ? GT(0x459334) : GT(0x45933c));
	else if (aKind == ITEM_LCD)
		AddValue(aShown, aDrawn, theItem->mHot, Screen_Centered() == 0 ? GT(0x45931c) : GT(0x459328));
	else if (aKind == ITEM_DIFFICULTY)
	{
		if (gDifficulty == -1)
		{
			// (the original's format for the easiest has "5s" for "%s", so the drawn text shows "5s" there)
			AppendTo(aDrawn, "%s^%d5s", (2 - theItem->mHot) * 2, Localize(GT(0x459314)));
			strcat(aShown, Localize(GT(0x459314)));
		}
		else if (gDifficulty == 0)
			AddValue(aShown, aDrawn, theItem->mHot, GT(0x459304));
		else
			AddValue(aShown, aDrawn, theItem->mHot, GT(0x4592fc));
	}
	else if (aKind == ITEM_CHANGE_USER)
	{
		// the name in place of its marker, all in the font's first colour
		if (strstr(theItem->mName, Localize(GT(0x4592f0))) != nullptr)
		{
			char* aMark = strstr(aShown, Localize(GT(0x4592f0)));
			strcpy(aMark, gProfiles[gProfile].mName);
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
	else if (aKind == ITEM_USER_SLOT)
	{
		// a player's slot: the player's name, on a light bar when it is the current player
		int aSlot = atol(theItem->mName);
		sprintf(aDrawn, "^%d%s", theItem->mHot + 1, gProfiles[aSlot].mName);
		sprintf(aShown, "%s", gProfiles[aSlot].mName);
		if (aSlot == gProfile)
		{
			Draw_FillBlendTarget((int)((double)theItem->mX - 92.0), (int)((double)theItem->mY - 4.0),
				(int)((double)theItem->mX + 91.0), (int)((double)theItem->mY + 20.0), 0xff, 0xff, 0xff, 100);
			Dirty_AddClippedShown(&gDirty, (int)((double)theItem->mX - 92.0), (int)((double)theItem->mY - 4.0), 0xb8,
				0x19, SCREEN_CLIP);
		}
	}
	strcpy(theItem->mShown, aShown);
	Font* aFont;
	if (theItem->mHot == 0 || (aFont = theItem->mHotFont) == nullptr)
		aFont = theItem->mFont;
	Font_SetEscapes(aFont, 1);
	Font_SetTarget(aFont, gScreenBits, gScreenPitch, &gBackBuffer.mFmt, gBackBuffer.mW, gBackBuffer.mH);
	Font_SetPalette(aFont, nullptr);
	aFont->mAlpha = 0xff;
	aFont->mAlign = 2;
	// a shadow a sixth of the font's height down and right
	uint32_t aShadowAlpha = 0x4b0 / Font_Height(theItem->mFont);
	int aShadowY = (int)((double)(Font_Height(theItem->mFont) / 6) + (double)theItem->mY);
	int aShadowX = (int)((double)(Font_Height(theItem->mFont) / 6) + (double)theItem->mX);
	Font_DrawFill(aFont, aShown, aShadowX, aShadowY, 2, 0, 0, 0, aShadowAlpha);
	int aRect[4];
	Font_DrawOutlined(aFont, aRect, (int)theItem->mX, (int)theItem->mY, aDrawn);
	double aX = (double)theItem->mX - 1.0;
	int aLeft = (int)((aX - (double)(Font_StringWidth(aFont, aShown) / 2)) - 5.0);
	int aTop = (int)(((double)theItem->mY - 1.0) - 5.0);
	int aRight = Font_StringWidth(aFont, aShown) + aLeft + 0xc;
	int aBottom = Font_StringHeight(aFont, aShown) + aTop + 0xc;
	int aW = aRight - aLeft;
	int aH = aBottom - aTop;
	if (theItem->mBox != 0)
	{
		int aSize = theItem->mBoxSize;
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
			double x = theItem->mX;
			double y = theItem->mY;
			double aS = (float)aSize;
			int aLeftX = (int)(x - aS);
			Draw_Line(aLeftX, (int)(y - (double)(aSize / 2)), aLeftX, (int)(aS + y), 0xff, 0xff, 0xff, 0x80);
			aSize = theItem->mBoxSize;
			aS = (float)aSize;
			int aRightX = (int)(aS + x);
			Draw_Line(aRightX, (int)(y - (double)(aSize / 2)), aRightX, (int)(aS + y), 0xff, 0xff, 0xff, 0x80);
			aSize = theItem->mBoxSize;
			int aTopY = (int)(y - (double)(aSize / 2));
			aS = (float)aSize;
			Draw_Line((int)(x - aS), aTopY, (int)(aS + x), aTopY, 0xff, 0xff, 0xff, 0x80);
			aS = (float)theItem->mBoxSize;
			int aBottomY = (int)(y + aS);
			Draw_Line((int)(x - aS), aBottomY, (int)(aS + x), aBottomY, 0xff, 0xff, 0xff, 0x80);
			aSize = theItem->mBoxSize;
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
	Dirty_AddClippedShown(&gDirty, aLeft, aTop, aW + 3, aH + 3, SCREEN_CLIP);
}

// 0x415580: an on-screen key: its character is added to the name being typed ('<' rubs one out)
void MenuItem_TypeKey(MenuItem* theItem)
{
	const char* aKey = theItem->mName;
	if (theItem->mName[0] == '<')
	{
		Key_Push(8);
		return;
	}
	List_Each(&gLevels.mCurrent->mTexts, [&](Text* theText) {
		if (theText->mState == TEXT_NAME_ENTRY)
		{
			TextInput* anInput = &theText->mInput;
			char aName[0x100];
			aName[0] = 0;
			if (TextInput_GetText(anInput) != nullptr)
				strcpy(aName, TextInput_GetText(anInput));
			strcat(aName, aKey);
			// (the original's buffer holds 16 characters; the byte after it is cleared, here four: the string is the
			// same)
			memset(aName + 16, 0, 4);
			TextInput_SetText(anInput, aName);
		}
	});
}

// 0x416d70: the quit requester (the board's texts go first)
void MenuItem_ConfirmQuit(MenuItem* theItem)
{
	bool aMore;
	do
	{
		aMore = List_RemoveCurrent(&gLevels.mCurrent->mTexts);
	} while (aMore);
	int aProfile = gProfile;
	strcpy(gSavedName, gProfiles[gProfile].mName);
	gSavedProfile = aProfile;
	const char* aSecond = Localize(GT(0x4594d8));
	Menu_ShowRequester(gLevels.mCurrent->mMenus.mCurrent, REQUESTER_QUIT, Localize(GT(0x4594bc)), aSecond);
	gLevels.mCurrent->mState = LEVEL_PAUSED;
	theItem->mHot = 0;
}

// 0x416e10: hot under the mouse (or always), and its kind's thing when clicked; 0 when the menu's screen is done
int MenuItem_Update(MenuItem* theItem)
{
	int aLeft = 0;
	int aRight = 0;
	int aTop = 0;
	int aResult = 1;
	int aBottom = 0;
	if (theItem->mAnim == nullptr)
	{
		// round its name
		float x = theItem->mX;
		const char* aShown = theItem->mShown;
		aLeft = (int)(((double)x - 1.0) - (double)(Font_StringWidth(theItem->mFont, aShown) / 2));
		aRight = (int)((double)(Font_StringWidth(theItem->mFont, aShown) / 2) + ((double)theItem->mX + 1.0));
		float anAdder = (float)theItem->mHeightAdder;
		int aHeight = Font_StringHeight(theItem->mFont, aShown);
		aBottom = (int)((((double)aHeight + (double)theItem->mY) + 1.0) + (double)anAdder);
		aTop = (int)(((double)theItem->mY - (double)anAdder) + 1.0);
	}
	else
	{
		Obj_Update(theItem);
		Image* anImage = theItem->mImage;
		if (anImage != nullptr)
		{
			float aHalfW = (float)(anImage->mW / 2);
			aLeft = (int)((double)theItem->mX - (double)aHalfW);
			aRight = (int)((double)aHalfW + (double)theItem->mX);
			float aHalfH = (float)(anImage->mH / 2);
			aTop = (int)((double)theItem->mY - (double)aHalfH);
			aBottom = (int)((double)aHalfH + (double)theItem->mY);
		}
	}
	if (theItem->mAlwaysHot == 0 && theItem->mAutoClick == 0 &&
		(gMouseX <= aLeft || aRight <= gMouseX || gMouseY <= aTop || aBottom <= gMouseY))
		theItem->mHot = 0;
	else
	{
		theItem->mHot = 1;
		if ((gLevels.mCurrent->mClicked != 0 &&
				gLevels.mCurrent->mMenus.mCurrent->mLockedLevel <= gProfiles[gProfile].mLevelReached + 1) ||
			theItem->mAutoClick != 0)
		{
			Sound_Play(Sound_Find("audio\\click.wav"));
			Level* aLevel = gLevels.mCurrent;
			int aProfile = gProfile;
			switch (theItem->mKind)
			{
			case ITEM_START:
				gGoTo = LEVEL_BOARD;
				aResult = 0;
				if (gLevels.mCurrent->mKind == LEVEL_SELECT_LEVEL2)
				{
					gProfiles[gProfile].mSavedLevel = 0;
					gProfiles[gProfile].mSavedLives = 0;
					gProfiles[gProfile].mSavedScore = 0;
				}
				SaveProfiles();
				break;
			case ITEM_OPTIONS:
				aResult = 0;
				gGoTo = LEVEL_OPTIONS;
				break;
			case ITEM_QUIT:
				SaveProfiles();
				aResult = 0;
				gQuit = 1;
				break;
			case ITEM_HELP:
				aResult = 0;
				gGoTo = LEVEL_HELP;
				break;
			case ITEM_HELP2:
				aResult = 0;
				gGoTo = LEVEL_HELP2;
				break;
			case ITEM_HIGH_SCORES:
				aResult = 0;
				if (gLevels.mCurrent->mKind == LEVEL_MAIN_MENU)
					gScore = 0;
				gGoTo = (gScore <= gHighScores[7].mScore ? 1 : 0) + 2;
				break;
			case ITEM_NEXT_LEVEL:
				aResult = 0;
				gGoTo = LEVEL_SAVED_GAME;
				break;
			case ITEM_HS_ENTERED:
				aResult = 0;
				gGoTo = LEVEL_HIGH_SCORES;
				SortHighScores();
				break;
			case ITEM_HS_STUFF:
				MenuItem_TypeKey(theItem);
				break;
			case ITEM_CREDITS:
				aResult = 0;
				gGoTo = LEVEL_CREDITS;
				break;
			case ITEM_VIDEO:
				Screen_SetFullscreen(1 - Screen_Fullscreen());
				break;
			case ITEM_LCD:
				Screen_SetCentered(1 - Screen_Centered());
				break;
			case ITEM_SOUND:
				Sound_SetOn(1 - Sound_On());
				break;
			case ITEM_MUSIC:
				Music_SetOn(1 - Music_On());
				break;
			case ITEM_DIFFICULTY:
				gDifficulty = gDifficulty + 1;
				if (1 < gDifficulty)
					gDifficulty = -1;
				break;
			case ITEM_SHOOTER_MODE:
				gAutoFire = 1 - gAutoFire;
				SaveProfiles();
				break;
			case ITEM_SELECT_LEVEL:
				gLives = 6;
				aResult = 0;
				gScore = 0;
				if (gProfiles[gProfile].mLevelReached == 0)
					gGoTo = LEVEL_HELP2;
				else if (gProfiles[gProfile].mSavedLives < 1)
					gGoTo = LEVEL_SELECT_LEVEL;
				else
					Menu_ShowRequester(gLevels.mCurrent->mMenus.mTail, REQUESTER_RESUME, GT(0x459594), GT(0x4595b0));
				break;
			case ITEM_SELECT_UP:
			{
				// up a level, no further than the furthest reached and the set's last
				int aReached = gProfiles[gProfile].mLevelReached;
				int aNext = gLevelInSet + 1;
				int aLevel = aNext;
				if (aReached <= aNext)
					aLevel = aReached;
				gLevelInSet = gLevelSets.mCurrent->mEnd - 1;
				if (aLevel <= gLevelInSet)
				{
					gLevelInSet = aReached;
					if (aNext < aReached)
						gLevelInSet = aNext;
				}
				int aFirst = gLevelSets.mCurrent->mStart - 1;
				if (gLevelInSet < aFirst)
					gLevelInSet = aFirst;
				Level_UpdateProgressStars();
				break;
			}
			case ITEM_SELECT_DOWN:
			{
				if (gLevelSets.mCurrent->mEnd < gLevelInSet)
					gLevelInSet = gLevelSets.mCurrent->mEnd;
				int aLevel = gLevelSets.mCurrent->mStart - 1;
				if (aLevel <= gLevelInSet - 1)
					aLevel = gLevelInSet - 1;
				gLevelInSet = aLevel < 0 ? 0 : aLevel;
				Level_UpdateProgressStars();
				break;
			}
			case ITEM_SELECT_WORLD:
			{
				// a world: the level set holding this menu's locked level, from the furthest level reached in it
				aResult = 0;
				if (gLevelSets.mCurrent != nullptr)
				{
					gLevelSets.mCurrent = gLevelSets.mHead;
					do
					{
						int aLocked = aLevel->mMenus.mCurrent->mLockedLevel;
						if (gLevelSets.mCurrent->mStart <= aLocked && aLocked <= gLevelSets.mCurrent->mEnd)
						{
							gWorldChanged = 1;
							break;
						}
					} while (List_Next(&gLevelSets));
				}
				int aReached = gProfiles[gProfile].mLevelReached;
				gLevelInSet = aReached;
				if (gLevelSets.mCurrent->mEnd < aReached + 1)
				{
					int aStart = gLevelSets.mCurrent->mStart;
					if (aStart < 1)
						aStart = 1;
					gLevelInSet = aStart - 1;
				}
				if (aReached == 0 ||
					(gLevelInSet < gLevelSets.mCurrent->mStart && aReached + 1 <= gLevelSets.mCurrent->mEnd))
				{
					gProfiles[aProfile].mSavedLevel = 0;
					gProfiles[aProfile].mSavedLives = 0;
					gGoTo = LEVEL_BOARD;
					gProfiles[aProfile].mSavedScore = 0;
				}
				else
					gGoTo = LEVEL_SELECT_LEVEL2;
				break;
			}
			case ITEM_BACK:
				aResult = 0;
				gGoTo = LEVEL_MAIN_MENU;
				break;
			case ITEM_CHANGE_USER:
				aResult = 0;
				gGoTo = LEVEL_CHANGE_USER;
				break;
			case ITEM_USER_SLOT:
				// a player's slot: an empty one asks for a name, the current one goes back, another is chosen
				strcpy(gSavedName, gProfiles[gProfile].mName);
				gSavedProfile = aProfile;
				if (strcmp(gProfiles[atol(theItem->mName)].mName, Localize(GT(TEXT_EMPTY_PROFILE))) == 0)
				{
					theItem->mHot = 0;
					gProfile = atol(theItem->mName);
					Menu_ShowRequester(gLevels.mCurrent->mMenus.mCurrent, REQUESTER_NAME, Localize(GT(0x45954c)),
						nullptr);
#ifdef __EMSCRIPTEN__
					// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with it
					SiteFillName(&gLevels.mCurrent->mMenus.mCurrent->mRequesters.mCurrent->mEntry->mInput);
#endif
				}
				else if (gProfile == atol(theItem->mName))
				{
					aResult = 0;
					gGoTo = LEVEL_MAIN_MENU;
				}
				else
				{
					gProfile = atol(theItem->mName);
					gAutoFire = gProfiles[gProfile].mAutoFire;
				}
				break;
			case ITEM_USER_RENAME:
				// rename: the name entry starts empty
				theItem->mHot = 0;
				gSavedProfile = gProfile;
				strcpy(gSavedName, gProfiles[gProfile].mName);
				gProfiles[gProfile].mName[0] = 0;
				Menu_ShowRequester(gLevels.mCurrent->mMenus.mCurrent, REQUESTER_NAME, Localize(GT(0x459534)), nullptr);
				TextInput_SetText(&gLevels.mCurrent->mMenus.mCurrent->mRequesters.mCurrent->mEntry->mInput,
					gProfiles[gProfile].mName);
#ifdef __EMSCRIPTEN__
				SiteFillName(&gLevels.mCurrent->mMenus.mCurrent->mRequesters.mCurrent->mEntry->mInput);	// as above
#endif
				break;
			case ITEM_NAME_OKAY:
				Level_SetUserName(
					TextInput_GetText(&gLevels.mCurrent->mMenus.mCurrent->mRequesters.mCurrent->mEntry->mInput));
				break;
			case ITEM_USER_DELETE:
			{
				// delete, unless it is the only player
				int aPlayers = 0;
				for (Profile* aSlot = gProfiles; aSlot < gProfiles + 10; aSlot++)
				{
					if (strcmp(aSlot->mName, Localize(GT(TEXT_EMPTY_PROFILE))) != 0)
						aPlayers = aPlayers + 1;
				}
				theItem->mHot = 0;
				gSavedProfile = gProfile;
				if (aPlayers < 2)
				{
					strcpy(gSavedName, gProfiles[gProfile].mName);
					const char* aSecond = Localize(GT(0x45950c));
					Menu_ShowRequester(gLevels.mCurrent->mMenus.mCurrent, REQUESTER_MESSAGE, Localize(GT(0x4594f4)),
						aSecond);
				}
				else
				{
					strcpy(gSavedName, gProfiles[gProfile].mName);
					Menu_ShowRequester(gLevels.mCurrent->mMenus.mCurrent, REQUESTER_DELETE_PLAYER, GT(0x459528),
						nullptr);
				}
				break;
			}
			case ITEM_DELETE_OKAY:
				// the delete's okay: the slot is emptied and the first player left is chosen
				gProfiles[gProfile].mAutoFire = 1;
				gProfiles[gProfile].mLevelReached = 0;
				strcpy(gProfiles[gProfile].mName, Localize(GT(TEXT_EMPTY_PROFILE)));
				for (int i = 0; i < 10; i++)
				{
					if (strcmp(gProfiles[i].mName, Localize(GT(TEXT_EMPTY_PROFILE))) != 0)
					{
						gProfile = i;
						i = 10;
					}
				}
				SaveProfiles();
				aResult = 0;
				break;
			case ITEM_CANCEL:
				// cancel: back to the player and name before
				if (gLevels.mCurrent->mKind == LEVEL_CHANGE_USER)
				{
					gProfile = gSavedProfile;
					strcpy(gProfiles[gSavedProfile].mName, gSavedName);
				}
				aLevel->mState = LEVEL_PLAYING;
				aResult = 0;
				break;
			case ITEM_CONFIRM_QUIT:
				MenuItem_ConfirmQuit(theItem);
				break;
			case ITEM_RESUME_YES:
			{
				// resume the saved game
				int aSaved = gProfiles[gProfile].mSavedLevel;
				gScore = gProfiles[gProfile].mSavedScore;
				gLives = gProfiles[gProfile].mSavedLives;
				aResult = 0;
				gGoTo = LEVEL_SAVED_GAME;
				gLevelInSet = aSaved;
				if (gLevelSets.mCurrent != nullptr)
				{
					gLevelSets.mCurrent = gLevelSets.mHead;
					do
					{
						if (gLevelSets.mCurrent->mStart <= aSaved && aSaved <= gLevelSets.mCurrent->mEnd)
							break;
					} while (List_Next(&gLevelSets));
				}
				break;
			}
			case ITEM_RESUME_NO:
				aResult = 0;
				gLives = 6;
				gScore = 0;
				gGoTo = (gProfiles[gProfile].mLevelReached == 0 ? 1 : 0) * 8 + 5;
				break;
			}
		}
		if (gLevels.mCurrent->mMenus.mCurrent->mLockedLevel <= gProfiles[gProfile].mLevelReached + 1 &&
			RandRange(1, 0x1e) == 1)
		{
			// a sparkle at the mouse
			ParticleSystem_New();
			int aLifetime = RandRange(400, 500);
			int aLast = theItem->mLastParticle;
			int aFirst = theItem->mFirstParticle;
			float aVy = (float)((double)RandRange(-0x32, 0x32) / 200.0);
			float aVx = (float)((double)RandRange(-0x32, 0x32) / 200.0);
			ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, (float)gMouseX, (float)gMouseY, aVx, aVy, aFirst,
				aLast, 0, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 400, 10, aLifetime, 0.998f, 0.0003f, 5);
		}
	}
	if (theItem->mKind == ITEM_SELECT_SLIDE)
	{
		if (gWorldChanged != 0)
		{
			gWorldChanged = 0;
			theItem->mAge = 0;
		}
		MenuItem_Slide(theItem);
		if ((double)theItem->mAge < 5.0)
			Level_UpdateProgressStars();
	}
	return aResult;
}

} // namespace

void HighScore_Entered(const char*)
{
	// the high-score screen's buttons go on by themselves
	List_Each(&gLevels.mCurrent->mMenus, [](Menu* theMenu) {
		List_Each(&theMenu->mItems, [](MenuItem* theItem) {
			int aKind = theItem->mKind;
			if (aKind == ITEM_HS_ENTERED || aKind == ITEM_START || aKind == ITEM_BACK)
				theItem->mAutoClick = 1;
		});
	});
}

void Menu_FromXml(Menu* theMenu, const char* theCursor, const XmlTag* theTag)
{
	Obj_FromXml(theMenu, theCursor, theTag);
	theMenu->mLogo = nullptr;
	theMenu->mLockedLevel = Xml_GetInt(theTag, "locked_level");
	if (Xml_GetString(theTag, "locked_animation") != nullptr)
		strcpy(theMenu->mLockedAnim, Xml_GetString(theTag, "locked_animation"));
	if (theMenu->mAnim != nullptr && gProfiles[gProfile].mLevelReached + 1 < theMenu->mLockedLevel)
	{
		theMenu->mAnim = Animation_Find(theMenu->mLockedAnim, &theMenu->mFrameCount, &theMenu->mFrameTime);
		theMenu->mImage = Animation_Frame(theMenu->mAnim, 0);
	}
	if (Xml_GetString(theTag, "logo_animation") != nullptr)
	{
		int aCount;
		int aFrameTime;
		theMenu->mLogo = Animation_Frame(Animation_Find(Xml_GetString(theTag, "logo_animation"), &aCount, &aFrameTime),
			0);
		theMenu->mLogoX = (Xml_GetInt(theTag, "logo_x") * SCREEN_WIDTH) / SCREEN_WIDTH;
		theMenu->mLogoY = (Xml_GetInt(theTag, "logo_y") * SCREEN_HEIGHT) / SCREEN_HEIGHT;
	}
	Menu_DrawToBackground(theMenu);
	XmlTag* aTag = const_cast<XmlTag*>(theTag);
	while (strcmp(aTag->mName, "/menu") != 0)
	{
		if (strcmp(aTag->mName, "menu_item") == 0)
		{
			MenuItem_Append(&theMenu->mItems);
			MenuItem_FromXml(theMenu->mItems.mCurrent, theCursor, aTag);
		}
		theCursor = Xml_NextTag(theCursor, aTag);
	}
}

void Menu_Draw(Menu* theMenu)
{
	// the menu, then its newest requester, and that one's, ...
	while (true)
	{
		if (theMenu->mKind == 1)
		{
			float anAlpha = theMenu->mAge <= 255.0f ? theMenu->mAge : 255.0f;
			theMenu->mAlpha = (int)anAlpha;
			Obj_Draw(theMenu);
		}
		List_Each(&theMenu->mItems, [](MenuItem* theItem) { MenuItem_Draw(theItem); });
		if (theMenu->mRequesters.mCurrent == nullptr)
			return;
		theMenu->mRequesters.mCurrent = theMenu->mRequesters.mHead;
		theMenu = theMenu->mRequesters.mHead;
	}
}

Menu* Menu_Init(Menu* theMenu)
{
	Obj_Init(theMenu);
	List_Init(&theMenu->mItems);
	List_Init(&theMenu->mRequesters);
	theMenu->mTextCount = 0;
	theMenu->mEntry = nullptr;
	theMenu->mImage = nullptr;
	theMenu->mLogo = nullptr;
	theMenu->mLockedLevel = 0;
	theMenu->mLockedAnim[0] = 0;
	return theMenu;
}

void Menu_Destroy(Menu* theMenu)
{
	// a requester's texts leave the level's
	Level* aLevel = gLevels.mCurrent;
	for (int i = 0; i < theMenu->mTextCount; i++)
	{
		Text* aText = theMenu->mTexts[i];
		List_Each(&aLevel->mTexts, [&](Text* theCurrent) {
			if (aText == theCurrent)
				List_RemoveCurrent(&gLevels.mCurrent->mTexts);
		});
		aLevel = gLevels.mCurrent;
	}
	if (theMenu->mEntry != nullptr)
	{
		Text* anEntry = theMenu->mEntry;
		List_Each(&aLevel->mTexts, [&](Text* theCurrent) {
			if (anEntry == theCurrent)
				List_RemoveCurrent(&gLevels.mCurrent->mTexts);
		});
	}
	List_Clear(&theMenu->mRequesters);
	List_Clear(&theMenu->mItems);
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
	List<Menu>* aList = &theMenu->mRequesters;
	Requester_Push(aList);
	Menu* aRequester = aList->mCurrent;
	aRequester->mKind = 1;
	aRequester->mState = theKind;
	aRequester->mAnim = Animation_Find("requester", &aRequester->mFrameCount, &aRequester->mFrameTime);
	aRequester->mImage = Animation_Frame(aRequester->mAnim, 0);
	aRequester->mMidHandle = 1;
	aRequester->mDirty = 1;
	aRequester->mDrawKind = 1;
	aRequester->mX = (float)SCREEN_CENTRE_X;
	aRequester->mY = (float)SCREEN_CENTRE_Y;
	aRequester->mDepth = 300;
	Text_Append(&gLevels.mCurrent->mTexts);
	aList->mCurrent->mTexts[0] = gLevels.mCurrent->mTexts.mCurrent;
	aList->mCurrent->mTextCount = 1;
	MenuItem* anItem;
	switch (theKind)
	{
	case REQUESTER_DELETE_PLAYER:
		// deleting the player
		sprintf(aBuf, "%s%s,", Localize(GT(0x4594a4)), gProfiles[gProfile].mName);
		Text_Make(aList->mCurrent->mTexts[0], aBuf, Font_Find("bonus_font"), SCREEN_CENTRE_X, 0xfa, 0, 100,
			TEXT_IDLE_FOREVER, 500, 0, 0);
		Requester_AddText(aList->mCurrent);
		sprintf(aBuf, "%s", Localize(GT(0x45948c)));
		Text_Make(aList->mCurrent->mTexts[aList->mCurrent->mTextCount], aBuf, Font_Find("bonus_font"), SCREEN_CENTRE_X,
			0x10c, 0, 100, TEXT_IDLE_FOREVER, 500, 0, 0);
		aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
		MenuItem_Insert(&aList->mCurrent->mItems);
		anItem = aList->mCurrent->mItems.mCurrent;
		Button_Setup(anItem, "okay", 340.0f, 330.0f);
		anItem->mMidHandle = 1;
		anItem->mKind = ITEM_DELETE_OKAY;
		break;
	case REQUESTER_MESSAGE:
		// a message to close
		Text_Make(aList->mCurrent->mTexts[0], theText, Font_Find("bonus_font"), SCREEN_CENTRE_X, 0xfa, 0, 0,
			TEXT_IDLE_FOREVER, 500, 0, 0);
		if (theSecond != nullptr)
		{
			Text* aText = Requester_AddText(aList->mCurrent);
			Text_Make(aText, theSecond, Font_Find("bonus_font"), SCREEN_CENTRE_X, 0x10c, 0, 100, TEXT_IDLE_FOREVER, 500,
				0, 0);
			aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
		}
		MenuItem_Append(&aList->mCurrent->mItems);
		anItem = aList->mCurrent->mItems.mCurrent;
		Button_Setup(anItem, "okay", (float)SCREEN_CENTRE_X, 330.0f);
		anItem->mMidHandle = 1;
		anItem->mKind = ITEM_CANCEL;
		strcpy(anItem->mName, GT(0x459460));
		return;
	case REQUESTER_NAME:
	{
		// a name to type
		Text_Append(&gLevels.mCurrent->mTexts);
		aList->mCurrent->mEntry = gLevels.mCurrent->mTexts.mCurrent;
		Text* anEntry = aList->mCurrent->mEntry;
		anEntry->mFont = Font_Find("bonus_font");
		TextInput* anInput = &anEntry->mInput;
		TextInput_Init(anInput, Font_Find("bonus_font"), 0xc, 4, 0x113, 0x113, 0x20d, 300);
		TextInput_SetFocus(anInput, 1);
		TextInput_SetCaseMode(anInput, 1);
		TextInput_SetFillColour(anInput, 100, 100, 100, 100);
		TextInput_SetOutline(anInput, 0);
		TextInput_SetOnEnter(anInput, Level_SetUserName);
		anEntry->mState = TEXT_NAME_ENTRY;
		Text_Make(aList->mCurrent->mTexts[0], theText, Font_Find("bonus_font"), SCREEN_CENTRE_X, 0xfa, 0, 0,
			TEXT_IDLE_FOREVER, 500, 0, 0);
		MenuItem_Insert(&aList->mCurrent->mItems);
		anItem = aList->mCurrent->mItems.mCurrent;
		Button_Setup(anItem, "okay", 340.0f, 330.0f);
		anItem->mMidHandle = 1;
		anItem->mKind = ITEM_NAME_OKAY;
		strcpy(anItem->mName, GT(0x459474));
		MenuItem_Append(&aList->mCurrent->mItems);
		anItem = aList->mCurrent->mItems.mCurrent;
		Button_Setup(anItem, "cancel", 460.0f, 330.0f);
		anItem->mMidHandle = 1;
		anItem->mKind = ITEM_CANCEL;
		strcpy(anItem->mName, GT(0x459460));
		return;
	}
	case REQUESTER_QUIT:
		// leaving the game
		Text_Make(aList->mCurrent->mTexts[0], theText, Font_Find("bonus_font"), SCREEN_CENTRE_X, 0xfa, 0, 100,
			TEXT_IDLE_FOREVER, 500, 0, 0);
		if (theSecond != nullptr)
		{
			Text* aText = Requester_AddText(aList->mCurrent);
			Text_Make(aText, theSecond, Font_Find("bonus_font"), SCREEN_CENTRE_X, 0x10c, 0, 100, TEXT_IDLE_FOREVER, 500,
				0, 0);
			aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
		}
		MenuItem_Insert(&aList->mCurrent->mItems);
		anItem = aList->mCurrent->mItems.mCurrent;
		Button_Setup(anItem, "okay", 340.0f, 330.0f);
		anItem->mMidHandle = 1;
		anItem->mKind = ITEM_BACK;
		break;
	case REQUESTER_BOARD_END:
	{
		// a board's end: its fill and shots against the records, and the bonus
		aRequester = aList->mCurrent;
		aRequester->mAnim = Animation_Find("end_level_panel", &aRequester->mFrameCount, &aRequester->mFrameTime);
		aRequester->mAlpha = 0;
		aRequester->mDrawKind = 4;
		aRequester->mY = 260.0f;
		Text_Make(aRequester->mTexts[0], theText, Font_Find("bonus_font"), SCREEN_CENTRE_X, 0x5c, 0, 100,
			TEXT_IDLE_FOREVER, 500, 0, 0);
		if (theSecond != nullptr)
		{
			Level* aLevel = gLevels.mCurrent;
			LevelRecord* aRecord;
			Text* aText = Requester_AddText(aList->mCurrent);
			Text_Make(aText, theSecond, Font_Find("bonus_font2"), SCREEN_CENTRE_X, 0x87, 0, 0, TEXT_IDLE_FOREVER, 500,
				0, 0);
			aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
			sprintf(aBuf, "%d%s", (aLevel->mFilledCells * 100) / aLevel->mTotalCells, "%");
			aText = Requester_AddText(aList->mCurrent);
			Text_Make(aText, aBuf, Font_Find("bonus_font"), SCREEN_CENTRE_X, 0x98, 300, 0x32, TEXT_IDLE_FOREVER, 500, 0,
				0);
			aList->mCurrent->mTexts[aList->mCurrent->mTextCount]->mAppearSound = Sound_Find("audio\\pop.wav");
			aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
			aText = Requester_AddText(aList->mCurrent);
			Text_Make(aText, GT(0x459410), Font_Find("bonus_font2"), SCREEN_CENTRE_X, 0xb4, 0, 0, TEXT_IDLE_FOREVER,
				500, 0, 0);
			aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
			aRecord = &gLevelRecords[gLevels.mCurrent->mNumber];
			sprintf(aBuf, "%s, %d%s", aRecord->mFillName, aRecord->mBestFill, "%");
			aText = Requester_AddText(aList->mCurrent);
			Text_Make(aText, aBuf, Font_Find("bonus_font"), SCREEN_CENTRE_X, 0xc5, 600, 0x32, TEXT_IDLE_FOREVER, 500, 0,
				0);
			aList->mCurrent->mTexts[aList->mCurrent->mTextCount]->mAppearSound = Sound_Find("audio\\pop.wav");
			aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
			aText = Requester_AddText(aList->mCurrent);
			Text_Make(aText, GT(0x4593f0), Font_Find("bonus_font2"), SCREEN_CENTRE_X, 0xeb, 0, 0, TEXT_IDLE_FOREVER,
				500, 0, 0);
			aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
			sprintf(aBuf, "%d", gLevels.mCurrent->mShots);
			aText = Requester_AddText(aList->mCurrent);
			Text_Make(aText, aBuf, Font_Find("bonus_font"), SCREEN_CENTRE_X, 0xfc, 900, 0x32, TEXT_IDLE_FOREVER, 500, 0,
				0);
			aList->mCurrent->mTexts[aList->mCurrent->mTextCount]->mAppearSound = Sound_Find("audio\\pop.wav");
			aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
			aText = Requester_AddText(aList->mCurrent);
			Text_Make(aText, GT(0x4593e0), Font_Find("bonus_font2"), SCREEN_CENTRE_X, 0x118, 0, 0, TEXT_IDLE_FOREVER,
				500, 0, 0);
			aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
			aRecord = &gLevelRecords[gLevels.mCurrent->mNumber];
			sprintf(aBuf, "%s, %d", aRecord->mShotsName, aRecord->mFewestShots);
			aText = Requester_AddText(aList->mCurrent);
			Text_Make(aText, aBuf, Font_Find("bonus_font"), SCREEN_CENTRE_X, 0x129, 0x4b0, 0x32, TEXT_IDLE_FOREVER, 500,
				0, 0);
			aList->mCurrent->mTexts[aList->mCurrent->mTextCount]->mAppearSound = Sound_Find("audio\\pop.wav");
			aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
			aText = Requester_AddText(aList->mCurrent);
			Text_Make(aText, GT(0x4593c8), Font_Find("bonus_font2"), SCREEN_CENTRE_X, 0x14f, 0, 0, TEXT_IDLE_FOREVER,
				500, 0, 0);
			aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
			sprintf(aBuf, "%d", gLevels.mCurrent->mBonus);
			aText = Requester_AddText(aList->mCurrent);
			Text_Make(aText, aBuf, Font_Find("bonus_font"), SCREEN_CENTRE_X, 0x160, 0x5dc, 0x32, TEXT_IDLE_FOREVER, 500,
				0, 0);
			aList->mCurrent->mTexts[aList->mCurrent->mTextCount]->mAppearSound = Sound_Find("audio\\pop.wav");
			aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
		}
		// a click anywhere goes on
		MenuItem_Append(&aList->mCurrent->mItems);
		anItem = aList->mCurrent->mItems.mCurrent;
		anItem->mDirty = 1;
		anItem->mMidHandle = 1;
		anItem->mDrawKind = 1;
		anItem->mFont = Font_Find("bonus_font");
		anItem->mHotFont = Font_Find("bonus_font");
		anItem->mX = (float)SCREEN_CENTRE_X;
		anItem->mY = 416.0f;
		anItem->mDepth = 0x15e;
		anItem->mMidHandle = 1;
		anItem->mKind = ITEM_NEXT_LEVEL;
		anItem->mName[0] = ' ';
		anItem->mName[1] = 0;
		anItem->mAlwaysHot = 1;
		anItem->mShowOnlyHot = 0;
		{
			Text* aText = Requester_AddText(aList->mCurrent);
			Text_Make(aText, GT(0x4590fc), Font_Find("bonus_font"), SCREEN_CENTRE_X, 400, 0, 300, 300, 300, 1, 0);
			aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
		}
		return;
	}
	case REQUESTER_RESUME:
		// yes or no
		Text_Make(aList->mCurrent->mTexts[0], theText, Font_Find("bonus_font"), SCREEN_CENTRE_X, 0xfa, 0, 100,
			TEXT_IDLE_FOREVER, 500, 0, 0);
		if (theSecond != nullptr)
		{
			Text* aText = Requester_AddText(aList->mCurrent);
			Text_Make(aText, theSecond, Font_Find("bonus_font"), SCREEN_CENTRE_X, 0x10c, 0, 100, TEXT_IDLE_FOREVER, 500,
				0, 0);
			aList->mCurrent->mTextCount = aList->mCurrent->mTextCount + 1;
		}
		MenuItem_Insert(&aList->mCurrent->mItems);
		anItem = aList->mCurrent->mItems.mCurrent;
		Button_Setup(anItem, "yes", 340.0f, 330.0f);
		anItem->mMidHandle = 1;
		anItem->mKind = ITEM_RESUME_YES;
		strcpy(anItem->mName, GT(0x416435));
		MenuItem_Append(&aList->mCurrent->mItems);
		anItem = aList->mCurrent->mItems.mCurrent;
		Button_Setup(anItem, "no", 460.0f, 330.0f);
		anItem->mMidHandle = 1;
		anItem->mKind = ITEM_RESUME_NO;
		strcpy(anItem->mName, GT(0x459454));
		return;
	default:
		return;
	}
	// REQUESTER_DELETE_PLAYER and REQUESTER_QUIT: okay and cancel
	strcpy(aList->mCurrent->mItems.mCurrent->mName, GT(0x459474));
	MenuItem_Append(&aList->mCurrent->mItems);
	anItem = aList->mCurrent->mItems.mCurrent;
	Button_Setup(anItem, "cancel", 460.0f, 330.0f);
	anItem->mMidHandle = 1;
	anItem->mKind = ITEM_CANCEL;
	strcpy(anItem->mName, GT(0x459460));
}

int Menu_Update(Menu* theMenu)
{
	int aResult = 1;
	Obj_Update(theMenu);
	List<Menu>* aRequesters = &theMenu->mRequesters;
	if (theMenu->mRequesters.mCurrent == nullptr)
	{
		List_Each(&theMenu->mItems, [&](MenuItem* theItem) { aResult = aResult & MenuItem_Update(theItem); });
		return aResult;
	}
	aRequesters->mCurrent = theMenu->mRequesters.mHead;
	if (Menu_Update(theMenu->mRequesters.mHead) != 0)
	{
		// a requester asking for a name stays up until it has one
		const char* aName = gProfiles[gProfile].mName;
		if (strcmp(aName, Localize(GT(TEXT_EMPTY_PROFILE))) == 0)
			return 1;
		if (aName[0] == 0)
			return 1;
		if (aRequesters->mCurrent->mState != REQUESTER_NAME)
			return 1;
	}
	List_RemoveCurrent(aRequesters);
	if (gGoTo != LEVEL_MAIN_MENU && gGoTo != LEVEL_SELECT_LEVEL && gGoTo != LEVEL_HELP2)
	{
		if (gGoTo != LEVEL_SAVED_GAME)
			return 1;
		gGoTo = LEVEL_BOARD;
	}
	return 0;
}

} // namespace cb
