// The texts on the screen (0x41f0e0..0x41fe90): a string in a font that waits, fades in, stays, fades out and is
// gone (or fades in again after a while), refreshing what it shows each update when it is a high score, the level
// or one of the player's numbers; the high-score name entry is a text too. 0x410a80 destroys one.

#include "CBLevel.h"
#include "R64Sys.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif

namespace cb
{

namespace
{

// The font set up for the screen as Text_Draw sets it (0x421f60, 0x421e90, 0x421ef0)
void SetScreenTarget(Font* theFont)
{
	Font_SetEscapes(theFont, 1);
	Font_SetTarget(theFont, gScreenBits, gScreenPitch, &gBackBuffer.mFmt, gBackBuffer.mW, gBackBuffer.mH);
	Font_SetPalette(theFont, nullptr);
}

} // namespace

Text* Text_Init(Text* theText)
{
	Obj_Init(theText);
	new (&theText->mInput) TextInput();
	return theText;
}

void Text_Destroy(Text* theText)
{
	theText->mInput.~TextInput();
	Obj_Destroy(theText);
}

void List_DeleteNode(Text* theText)
{
	Text_Destroy(theText);
	::operator delete(theText);
}

// 0x41f0e0
void Text_Make(Text* theText, const char* theString, Font* theFont, int theX, int theY, int theDormant, int theFadeIn,
	int theIdle, int theFadeOut, int theLoopTime, int theBlink)
{
	if (theString == nullptr)
		theText->mText[0] = 0;
	else
	{
		strcpy(theText->mText, theString);
		// (_strupr)
		for (char* c = theText->mText; *c != 0; c++)
			*c = (char)toupper((unsigned char)*c);
	}
	theText->mX = (float)theX;
	theText->mFont = theFont;
	theText->mFadeIn = theFadeIn;
	theText->mY = (float)theY;
	theText->mFadeOut = theFadeOut;
	theText->mIdle = theIdle;
	theText->mDormant = theDormant;
	theText->mLifetime = (float)theDormant;
	theText->mAlpha = 0;
	theText->mDepth = 0xb;
	theText->mLoopTime = theLoopTime;
	theText->mState = TEXT_DORMANT;
	theText->mAlign = 2;
	theText->mBlink = theBlink;
	theText->mFirstParticle = Bank_IndexOf(&gArtBank, "bubble02");
	theText->mLastParticle = Bank_IndexOf(&gArtBank, "bubble02");
	theText->mAppearSound = 0;
}

int Text_Update(Text* theText)
{
	int aResult = 1;
	if (theText->mState == TEXT_NAME_ENTRY)
	{
		TextInput* anInput = &theText->mInput;
		TextInput_Update(anInput, (double)0.001f);
		HighScore* anEntry = &gHighScores[8];
		anEntry->mScore = gScore;
		if (TextInput_GetText(anInput)[0] == 0)
			strcpy(anEntry->mName, GT(0x459dc0));
		else
		{
			strcpy(anEntry->mName, TextInput_GetText(anInput));
			if (0x1e0 < Font_StringWidth(theText->mFont, anEntry->mName))
			{
				do
				{
					anEntry->mName[strlen(anEntry->mName) - 1] = 0;
					TextInput_SetText(anInput, anEntry->mName);
				} while (0x1e0 < Font_StringWidth(theText->mFont, anEntry->mName));
				return 1;
			}
		}
		return 1;
	}
	int aKind = theText->mKind;
	if (aKind == TEXT_HIGH_SCORE_NAME)
	{
		sprintf(theText->mText, "^%d%s", theText->mRow + 2, gHighScores[theText->mRow].mName);
		if (gHighScores[theText->mRow].mScore == gScore)
			theText->mBlink = 0x2ee;
	}
	else if (aKind == TEXT_HIGH_SCORE)
		sprintf(theText->mText, "^%d%d", theText->mRow + 2, gHighScores[theText->mRow].mScore);
	else if (aKind == TEXT_LEVEL)
		sprintf(theText->mText, "^4%d", gLevelInSet + 1);
	else
	{
		char* aMark;
		int aValue;
		if (aKind == TEXT_LIVES_LOST)
		{
			aMark = strstr(theText->mText, "LL");
			aValue = gProfiles[gProfile].mRiderHits;
		}
		else if (aKind == TEXT_SHOTS)
		{
			aMark = strstr(theText->mText, "SF");
			aValue = gProfiles[gProfile].mBeamsFired;
		}
		else if (aKind == TEXT_BUGS_KILLED)
		{
			aMark = strstr(theText->mText, "BK");
			aValue = gProfiles[gProfile].mKills;
		}
		else if (aKind == TEXT_POWERUPS_CAUGHT)
		{
			aMark = strstr(theText->mText, "PC");
			aValue = gProfiles[gProfile].mPowerupsCaught;
		}
		else
			aMark = nullptr;
		if (aMark != nullptr)
			sprintf(aMark, "%d", aValue);
	}
	Obj_Update(theText);
	if (theText->mLifetime < theText->mAge)
	{
		theText->mState = theText->mState + 1;
		int aState = theText->mState;
		theText->mAge = 0;
		if (aState == TEXT_FADING_IN)
		{
			theText->mLifetime = (float)theText->mFadeIn;
			if (theText->mAppearSound != 0)
			{
				Sound_Play(theText->mAppearSound);
				const char* aString = theText->mText;
				int aCount = 0x28;
				do
				{
					ParticleSystem_New();
					float aX = theText->mX;
					int aLifetime = RandRange(400, 500);
					int aFirst = theText->mFirstParticle;
					int aLast = theText->mLastParticle;
					float aVy = (float)((double)RandRange(-0xa0, -0x32) / 1000.0);
					float aVx = (float)((double)RandRange(-0x32, 0x32) / 600.0);
					int aHeight = Font_StringHeight(theText->mFont, aString);
					int aBottom = (int)((double)aHeight + (double)theText->mY);
					float y = (float)RandRange((int)theText->mY, aBottom);
					int aRight = (int)((double)theText->mX + 1.0 + (double)(Font_StringWidth(theText->mFont, aString) / 2));
					int aLeft = (int)(((double)aX - 1.0) - (double)(Font_StringWidth(theText->mFont, aString) / 2));
					float x = (float)RandRange(aLeft, aRight);
					ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, x, y, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff, 0x154, 10, aLifetime, 1.0f, 0.0004f, 5);
					aCount = aCount - 1;
					gLevels.mCurrent->mSystems.mCurrent->mDepth = 500;
					gLevels.mCurrent->mSystems.mCurrent->mParticles.mCurrent->mDepth = 500;
				} while (aCount != 0);
			}
		}
		else if (aState == TEXT_SHOWN)
			theText->mLifetime = (float)theText->mIdle;
		else if (aState == TEXT_FADING_OUT)
			theText->mLifetime = (float)theText->mFadeOut;
		else if (aState == TEXT_GONE)
			theText->mLifetime = (float)theText->mLoopTime;
	}
	if (TEXT_GONE < theText->mState)
	{
		if (theText->mLoopTime != 0)
		{
			theText->mState = TEXT_FADING_IN;
			theText->mAlpha = 0xff;
			theText->mLifetime = (float)theText->mFadeIn;
			theText->mAge = 0;
			return 1;
		}
		aResult = 0;
	}
	return aResult;
}

void Text_Draw(Text* theText)
{
	int aState = theText->mState;
	float aProgress = theText->mAge / theText->mLifetime;
	if (aState == TEXT_NAME_ENTRY)
	{
		TextInput_Draw(&theText->mInput, gScreenBits, gScreenPitch, Sys_ScreenFormat(), &gDirty);
		return;
	}
	if (aState == TEXT_SHOWN)
	{
		int aBlink = theText->mBlink;
		aProgress = 1.0f;
		if (aBlink != 0 && aBlink / 2 < (int)theText->mAge % aBlink)
			return;
	}
	else if (aState == TEXT_FADING_OUT)
		aProgress = 1.0f - aProgress;
	if (aState == TEXT_DORMANT || aState == TEXT_GONE)
		return;
	theText->mAlpha = (int)((double)aProgress * 255.0);
	if (theText->mAnim != nullptr)
	{
		theText->mShadow = 1;
		theText->mDrawKind = 4;
		Obj_Draw(theText);
		return;
	}
	SetScreenTarget(theText->mFont);
	theText->mFont->mAlpha = theText->mAlpha;
	theText->mFont->mAlign = theText->mAlign;
	const char* aString = theText->mText;
	int aRect[4];
	Font_DrawOutlined(theText->mFont, aRect, (int)theText->mX, (int)theText->mY, aString);
	int aLeft, aTop, aW, aH;
	if (theText->mAlign == 0)
	{
		aH = Font_StringHeight(theText->mFont, aString) + 2;
		aW = Font_StringWidth(theText->mFont, aString) + 2;
		aTop = (int)((double)theText->mY - 1.0);
		aLeft = (int)((double)theText->mX - 1.0);
	}
	else
	{
		double aX = (double)theText->mX - 1.0;
		aH = Font_StringHeight(theText->mFont, aString) + 2;
		aW = Font_StringWidth(theText->mFont, aString) + 2;
		aTop = (int)((double)theText->mY - 1.0);
		if (theText->mAlign == 1)
			aLeft = (int)(aX - (double)Font_StringWidth(theText->mFont, aString));
		else
			aLeft = (int)(aX - (double)(Font_StringWidth(theText->mFont, aString) / 2));
	}
	Dirty_AddClippedShown(&gDirty, aLeft, aTop, aW, aH, SCREEN_CLIP);
}

int Text_FromXml(Text* theText, const char* theCursor, const XmlTag* theTag)
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
			theText->mRow = aRow;
			sprintf(aBuf, "^%d%d.", aRow + 2, aRow + 1);
			Text_Append(&gLevels.mCurrent->mTexts);
			Text_Make(gLevels.mCurrent->mTexts.mCurrent, aBuf, Font_Find(Xml_GetString(theTag, "font")), 0x1e,
				(Xml_GetInt(theTag, "y") * 600) / 600, Xml_GetInt(theTag, "dormant_time"),
				Xml_GetInt(theTag, "fade_in_time"), Xml_GetInt(theTag, "idle_time"), Xml_GetInt(theTag, "fade_out_time"),
				0, 0);
			gLevels.mCurrent->mTexts.mCurrent->mAlign = 0;
			gLevels.mCurrent->mTexts.mCurrent = theText;
			sprintf(aBuf, "^%d%s", theText->mRow + 2, gHighScores[theText->mRow].mName);
			Text_Make(theText, aBuf, Font_Find(Xml_GetString(theTag, "font")), 100, (Xml_GetInt(theTag, "y") * 600) / 600,
				Xml_GetInt(theTag, "dormant_time"), Xml_GetInt(theTag, "fade_in_time"), Xml_GetInt(theTag, "idle_time"),
				Xml_GetInt(theTag, "fade_out_time"), 0, 0);
			theText->mAlign = 0;
			theText->mKind = TEXT_HIGH_SCORE_NAME;
			sprintf(aBuf, "^%d%d", theText->mRow + 2, gHighScores[theText->mRow].mScore);
			Text_Append(&gLevels.mCurrent->mTexts);
			Text_Make(gLevels.mCurrent->mTexts.mCurrent, aBuf, Font_Find(Xml_GetString(theTag, "font")), 0x302,
				(Xml_GetInt(theTag, "y") * 600) / 600, Xml_GetInt(theTag, "dormant_time"),
				Xml_GetInt(theTag, "fade_in_time"), Xml_GetInt(theTag, "idle_time"), Xml_GetInt(theTag, "fade_out_time"),
				0, 0);
			gLevels.mCurrent->mTexts.mCurrent->mKind = TEXT_HIGH_SCORE;
			gLevels.mCurrent->mTexts.mCurrent->mRow = theText->mRow;
			gLevels.mCurrent->mTexts.mCurrent->mAlign = 1;
			gLevels.mCurrent->mTexts.mCurrent = theText;
			return 1;
		}
		if (strcmp(Xml_GetString(theTag, "type"), "hs_entry") == 0)
		{
			theText->mFont = Font_Find(Xml_GetString(theTag, "font"));
			int aX = Xml_GetInt(theTag, "x");
			int aWidth = Xml_GetInt(theTag, "width");
			int aY = (Xml_GetInt(theTag, "y") * 600) / 600;
			TextInput* anInput = &theText->mInput;
			TextInput_Init(anInput, theText->mFont, 0x10, 4, (aX * 800) / 800 - aWidth / 2, aY,
				aWidth / 2 + (aX * 800) / 800, aY + 0x1e);
			TextInput_SetFocus(anInput, 1);
			TextInput_SetCaseMode(anInput, 1);
#ifdef __EMSCRIPTEN__
			SiteFillHighScore(anInput, theText->mFont);	// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with it
#endif
			TextInput_SetFillColour(anInput, 100, 100, 100, 100);
			TextInput_SetOutline(anInput, 0);
			TextInput_SetOnEnter(anInput, HighScore_Entered);
			theText->mState = TEXT_NAME_ENTRY;
			return 1;
		}
		if (strcmp(Xml_GetString(theTag, "type"), "level_select") == 0)
			theText->mKind = TEXT_LEVEL;
		else if (strcmp(Xml_GetString(theTag, "type"), "shots_fired") == 0)
			theText->mKind = TEXT_SHOTS;
		else if (strcmp(Xml_GetString(theTag, "type"), "lives_lost") == 0)
			theText->mKind = TEXT_LIVES_LOST;
		else if (strcmp(Xml_GetString(theTag, "type"), "bugs_killed") == 0)
			theText->mKind = TEXT_BUGS_KILLED;
		else if (strcmp(Xml_GetString(theTag, "type"), "powerups_caught") == 0)
			theText->mKind = TEXT_POWERUPS_CAUGHT;
		else
			return 1;
		Text_Make(theText, Xml_GetString(theTag, "text"), Font_Find(Xml_GetString(theTag, "font")),
			(Xml_GetInt(theTag, "x") * 800) / 800, (Xml_GetInt(theTag, "y") * 600) / 600,
			Xml_GetInt(theTag, "dormant_time"), Xml_GetInt(theTag, "fade_in_time"), Xml_GetInt(theTag, "idle_time"),
			Xml_GetInt(theTag, "fade_out_time"), 0, 0);
		return 1;
	}
	if (Obj_FromXml(theText, theCursor, theTag) == 0)
		return 0;
	Text_Make(theText, Xml_GetString(theTag, "text"), Font_Find(Xml_GetString(theTag, "font")), (int)theText->mX,
		(int)theText->mY, Xml_GetInt(theTag, "dormant_time"), Xml_GetInt(theTag, "fade_in_time"),
		Xml_GetInt(theTag, "idle_time"), Xml_GetInt(theTag, "fade_out_time"), Xml_GetInt(theTag, "loop_time"), 0);
	if (Xml_GetString(theTag, "justify") != nullptr)
	{
		if (strcmp(Xml_GetString(theTag, "justify"), "left") == 0)
		{
			theText->mAlign = 0;
			return 1;
		}
		if (strcmp(Xml_GetString(theTag, "justify"), "right") == 0)
			theText->mAlign = 1;
	}
	return 1;
}

} // namespace cb
