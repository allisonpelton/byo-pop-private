// The texts on the screen (0x41f5d0..0x420370): a string in a font that waits, fades in, stays, fades out and is
// gone (or fades in again after a while), refreshing what it shows each update when it is a high score, the level
// or one of the player's numbers; the high-score name entry is a text too. 0x4109d0 destroys one.

#include "WBLevel.h"
#include "R64Sys.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

namespace wb
{

namespace
{

// The font set up for the screen as Text_Draw sets it (0x422440, 0x422370, 0x4223d0)
void SetScreenTarget(Font* theFont)
{
	Font_SetEscapes(theFont, 1);
	Font_SetTarget(theFont, gScreenBits, gScreenPitch, &gBackBuffer.fmt, gBackBuffer.w, gBackBuffer.h);
	Font_SetPalette(theFont, nullptr);
}

} // namespace

Text* Text_Init(Text* theText)
{
	Obj_Init(theText);
	new (&theText->input) TextInput();
	return theText;
}

void Text_Destroy(Text* theText)
{
	theText->input.~TextInput();
	Obj_Destroy(theText);
}

void List_DeleteNode(Text* theText)
{
	Text_Destroy(theText);
	::operator delete(theText);
}

// 0x41f5d0 (Cosmic Bugs' version upper-cased the text)
void Text_Make(Text* theText, const char* theString, Font* theFont, int theX, int theY, int theDormant, int theFadeIn,
	int theIdle, int theFadeOut, int theLoopTime, int theBlink)
{
	if (theString == nullptr)
		theText->text[0] = 0;
	else
	{
		strcpy(theText->text, theString);
	}
	theText->x = (float)theX;
	theText->font = theFont;
	theText->fadeIn = theFadeIn;
	theText->y = (float)theY;
	theText->fadeOut = theFadeOut;
	theText->idle = theIdle;
	theText->dormant = theDormant;
	theText->lifetime = (float)theDormant;
	theText->alpha = 0;
	theText->depth = 0xb;
	theText->loopTime = theLoopTime;
	theText->state = 4;
	theText->align = 2;
	theText->blink = theBlink;
	theText->firstParticle = Bank_IndexOf(&gArtBank, "bubble02");
	theText->lastParticle = Bank_IndexOf(&gArtBank, "bubble02");
	theText->appearSound = 0;
}

int Text_Update(Text* theText)
{
	int aResult = 1;
	if (theText->state == 3)
	{
		TextInput* anInput = &theText->input;
		TextInput_Update(anInput, (double)0.001f);
		HighScore* anEntry = &gHighScores[8];
		anEntry->score = gScore;
		if (TextInput_GetText(anInput)[0] == 0)
			strcpy(anEntry->name, GT(0x459f50));
		else
		{
			strcpy(anEntry->name, TextInput_GetText(anInput));
			if (0x1e0 < Font_StringWidth(theText->font, anEntry->name))
			{
				do
				{
					anEntry->name[strlen(anEntry->name) - 1] = 0;
					TextInput_SetText(anInput, anEntry->name);
				} while (0x1e0 < Font_StringWidth(theText->font, anEntry->name));
				return 1;
			}
		}
		return 1;
	}
	int aKind = theText->kind;
	if (aKind == 1)
	{
		sprintf(theText->text, "^%d%s", theText->row + 2, gHighScores[theText->row].name);
		if (gHighScores[theText->row].score == gScore)
			theText->blink = 0x2ee;
	}
	else if (aKind == 2)
		sprintf(theText->text, "^%d%d", theText->row + 2, gHighScores[theText->row].score);
	else if (aKind == 3)
		sprintf(theText->text, "^4%d", gLevelInSet + 1);
	else
	{
		char* aMark;
		int aValue;
		if (aKind == 6)
		{
			aMark = strstr(theText->text, "LL");
			aValue = gProfiles[gProfile].riderHits;
		}
		else if (aKind == 5)
		{
			aMark = strstr(theText->text, "SF");
			aValue = gProfiles[gProfile].beamsFired;
		}
		else if (aKind == 7)
		{
			aMark = strstr(theText->text, "BK");
			aValue = gProfiles[gProfile].kills;
		}
		else if (aKind == 8)
		{
			aMark = strstr(theText->text, "PC");
			aValue = gProfiles[gProfile].powerupsCaught;
		}
		else
			aMark = nullptr;
		if (aMark != nullptr)
			sprintf(aMark, "%d", aValue);
	}
	Obj_Update(theText);
	if (theText->lifetime < theText->age)
	{
		theText->state = theText->state + 1;
		int aState = theText->state;
		theText->age = 0;
		if (aState == 5)
		{
			theText->lifetime = (float)theText->fadeIn;
			if (theText->appearSound != 0)
			{
				Sound_Play(theText->appearSound);
				const char* aString = theText->text;
				int aCount = 0x28;
				do
				{
					ParticleSystem_New();
					float aX = theText->x;
					int aLifetime = RandRange(400, 500);
					int aFirst = theText->firstParticle;
					int aLast = theText->lastParticle;
					float aVy = (float)((double)RandRange(-0xa0, -0x32) / 1000.0);
					float aVx = (float)((double)RandRange(-0x32, 0x32) / 600.0);
					int aHeight = Font_StringHeight(theText->font, aString);
					int aBottom = (int)((double)aHeight + (double)theText->y);
					float y = (float)RandRange((int)theText->y, aBottom);
					int aRight = (int)((double)theText->x + 1.0 + (double)(Font_StringWidth(theText->font, aString) / 2));
					int aLeft = (int)(((double)aX - 1.0) - (double)(Font_StringWidth(theText->font, aString) / 2));
					float x = (float)RandRange(aLeft, aRight);
					ParticleSystem_Add(gLevels.current->systems.current, x, y, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff, 0x154, 10, aLifetime, 1.0f, 0.0004f, 5);
					aCount = aCount - 1;
					gLevels.current->systems.current->depth = 500;
					gLevels.current->systems.current->particles.current->depth = 500;
				} while (aCount != 0);
			}
		}
		else if (aState == 6)
			theText->lifetime = (float)theText->idle;
		else if (aState == 7)
			theText->lifetime = (float)theText->fadeOut;
		else if (aState == 8)
			theText->lifetime = (float)theText->loopTime;
	}
	if (8 < theText->state)
	{
		if (theText->loopTime != 0)
		{
			theText->state = 5;
			theText->alpha = 0xff;
			theText->lifetime = (float)theText->fadeIn;
			theText->age = 0;
			return 1;
		}
		aResult = 0;
	}
	return aResult;
}

void Text_Draw(Text* theText)
{
	int aState = theText->state;
	float t = theText->age / theText->lifetime;
	if (aState == 3)
	{
		TextInput_Draw(&theText->input, gScreenBits, gScreenPitch, Sys_ScreenFormat(), &gDirty);
		return;
	}
	if (aState == 6)
	{
		int aBlink = theText->blink;
		t = 1.0f;
		if (aBlink != 0 && aBlink / 2 < (int)theText->age % aBlink)
			return;
	}
	else if (aState == 7)
		t = 1.0f - t;
	if (aState == 4 || aState == 8)
		return;
	theText->alpha = (int)((double)t * 255.0);
	if (theText->anim != nullptr)
	{
		theText->shadow = 1;
		theText->drawKind = 4;
		Obj_Draw(theText);
		return;
	}
	SetScreenTarget(theText->font);
	theText->font->alpha = theText->alpha;
	theText->font->align = theText->align;
	const char* aString = theText->text;
	int aRect[4];
	Font_DrawOutlined(theText->font, aRect, (int)theText->x, (int)theText->y, aString);
	int aLeft, aTop, aW, aH;
	if (theText->align == 0)
	{
		aH = Font_StringHeight(theText->font, aString) + 2;
		aW = Font_StringWidth(theText->font, aString) + 2;
		aTop = (int)((double)theText->y - 1.0);
		aLeft = (int)((double)theText->x - 1.0);
	}
	else
	{
		double aX = (double)theText->x - 1.0;
		aH = Font_StringHeight(theText->font, aString) + 2;
		aW = Font_StringWidth(theText->font, aString) + 2;
		aTop = (int)((double)theText->y - 1.0);
		if (theText->align == 1)
			aLeft = (int)(aX - (double)Font_StringWidth(theText->font, aString));
		else
			aLeft = (int)(aX - (double)(Font_StringWidth(theText->font, aString) / 2));
	}
	Dirty_AddClippedShown(&gDirty, aLeft, aTop, aW, aH, kScreenClip);
}

int Text_FromXml(Text* theText, const char* p, const XmlTag* theTag)
{
	char aBuf[0x80];
	if (Xml_GetString(theTag, "type") != nullptr)
	{
		const char* aType = Xml_GetString(theTag, "type");
		if (strcmp(aType, "hs") == 0)
		{
			// a high score's row: its rank, then this text its name, then its score
			strcpy(aBuf, Xml_GetString(theTag, "text"));
			int aRow = atol(aBuf + 8);
			theText->row = aRow;
			sprintf(aBuf, "^%d%d.", aRow + 2, aRow + 1);
			Text_Append(&gLevels.current->texts);
			Text_Make(gLevels.current->texts.current, aBuf, Font_Find(Xml_GetString(theTag, "font")), 0x1e,
				(Xml_GetInt(theTag, "y") * 600) / 600, Xml_GetInt(theTag, "dormant_time"),
				Xml_GetInt(theTag, "fade_in_time"), Xml_GetInt(theTag, "idle_time"), Xml_GetInt(theTag, "fade_out_time"),
				0, 0);
			gLevels.current->texts.current->align = 0;
			gLevels.current->texts.current = theText;
			sprintf(aBuf, "^%d%s", theText->row + 2, gHighScores[theText->row].name);
			Text_Make(theText, aBuf, Font_Find(Xml_GetString(theTag, "font")), 100, (Xml_GetInt(theTag, "y") * 600) / 600,
				Xml_GetInt(theTag, "dormant_time"), Xml_GetInt(theTag, "fade_in_time"), Xml_GetInt(theTag, "idle_time"),
				Xml_GetInt(theTag, "fade_out_time"), 0, 0);
			theText->align = 0;
			theText->kind = 1;
			sprintf(aBuf, "^%d%d", theText->row + 2, gHighScores[theText->row].score);
			Text_Append(&gLevels.current->texts);
			Text_Make(gLevels.current->texts.current, aBuf, Font_Find(Xml_GetString(theTag, "font")), 0x302,
				(Xml_GetInt(theTag, "y") * 600) / 600, Xml_GetInt(theTag, "dormant_time"),
				Xml_GetInt(theTag, "fade_in_time"), Xml_GetInt(theTag, "idle_time"), Xml_GetInt(theTag, "fade_out_time"),
				0, 0);
			gLevels.current->texts.current->kind = 2;
			gLevels.current->texts.current->row = theText->row;
			gLevels.current->texts.current->align = 1;
			gLevels.current->texts.current = theText;
			return 1;
		}
		if (strcmp(Xml_GetString(theTag, "type"), "hs_entry") == 0)
		{
			theText->font = Font_Find(Xml_GetString(theTag, "font"));
			int aX = Xml_GetInt(theTag, "x");
			int aWidth = Xml_GetInt(theTag, "width");
			int aY = (Xml_GetInt(theTag, "y") * 600) / 600;
			TextInput* anInput = &theText->input;
			TextInput_Init(anInput, nullptr, nullptr, theText->font, 0x10, 4, (aX * 800) / 800 - aWidth / 2, aY,
				aWidth / 2 + (aX * 800) / 800, aY + 0x1e);
			TextInput_SetFocus(anInput, 1);
			TextInput_SetCaseMode(anInput, 1);
			TextInput_SetFillColour(anInput, 100, 100, 100, 100);
			TextInput_SetOutline(anInput, 0);
			TextInput_SetOnEnter(anInput, HighScore_Entered);
			theText->state = 3;
			return 1;
		}
		if (strcmp(Xml_GetString(theTag, "type"), "level_select") == 0)
			theText->kind = 3;
		else if (strcmp(Xml_GetString(theTag, "type"), "shots_fired") == 0)
			theText->kind = 5;
		else if (strcmp(Xml_GetString(theTag, "type"), "lives_lost") == 0)
			theText->kind = 6;
		else if (strcmp(Xml_GetString(theTag, "type"), "bugs_killed") == 0)
			theText->kind = 7;
		else if (strcmp(Xml_GetString(theTag, "type"), "powerups_caught") == 0)
			theText->kind = 8;
		else
			return 1;
		Text_Make(theText, Xml_GetString(theTag, "text"), Font_Find(Xml_GetString(theTag, "font")),
			(Xml_GetInt(theTag, "x") * 800) / 800, (Xml_GetInt(theTag, "y") * 600) / 600,
			Xml_GetInt(theTag, "dormant_time"), Xml_GetInt(theTag, "fade_in_time"), Xml_GetInt(theTag, "idle_time"),
			Xml_GetInt(theTag, "fade_out_time"), 0, 0);
		return 1;
	}
	if (Obj_FromXml(theText, p, theTag) == 0)
		return 0;
	Text_Make(theText, Xml_GetString(theTag, "text"), Font_Find(Xml_GetString(theTag, "font")), (int)theText->x,
		(int)theText->y, Xml_GetInt(theTag, "dormant_time"), Xml_GetInt(theTag, "fade_in_time"),
		Xml_GetInt(theTag, "idle_time"), Xml_GetInt(theTag, "fade_out_time"), Xml_GetInt(theTag, "loop_time"), 0);
	if (Xml_GetString(theTag, "justify") != nullptr)
	{
		if (strcmp(Xml_GetString(theTag, "justify"), "left") == 0)
		{
			theText->align = 0;
			return 1;
		}
		if (strcmp(Xml_GetString(theTag, "justify"), "right") == 0)
			theText->align = 1;
	}
	return 1;
}

} // namespace wb
