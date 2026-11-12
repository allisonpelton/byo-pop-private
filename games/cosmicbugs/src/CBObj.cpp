// The game objects' base (0x40eac0..0x40f0f0)

#include "CBObj.h"
#include "CBLevel.h"
#include "R64Xml.h"

#include <cstring>
#include <new>

namespace cb
{

uint8_t* gScreenBits = nullptr;
int gScreenPitch = 0;
Dirty gDirty;
const int SCREEN_CLIP[4] = {0, 0, 0x31f, 0x257};

// The original leaves the links, the colour and a few others as they were; here the base starts all 0
void Obj_Init(Obj* theObj)
{
	std::memset((void*)theObj, 0, sizeof(Obj));
	List_Init(&theObj->mEmitters);
	List_Init(&theObj->mChildren);
	theObj->mX = 0.0f;
	theObj->mY = 0.0f;
	theObj->mOldX = 0.0f;
	theObj->mImage = nullptr;
	theObj->mOldY = 0.0f;
	theObj->mKind = 0;
	theObj->mVx = 0.0f;
	theObj->mState = 0;
	theObj->mVy = 0.0f;
	theObj->mDepth = 0;
	theObj->mAge = 0.0f;
	theObj->mTint = 0;
	theObj->mLifetime = 0.0f;
	theObj->mDrawKind = 0;
	theObj->mDirty = 0;
	theObj->mFade = 0.0f;
	theObj->mAnim = nullptr;
	theObj->mFrameCount = 0;
	theObj->mFrameTime = 0;
	theObj->mFrame = 0;
	theObj->mFrameTimer = 0;
	theObj->mMidHandle = 0;
	theObj->mOneShot = 0;
	theObj->mShadow = 0;
	theObj->mLoop = 1;
	theObj->mFrameStep = 1;
	theObj->mExploSound = -1;
}

// 0x40f0f0, and 0x419780 (a jump to it, which the nodes of another list end in)
void Obj_Destroy(Obj* theObj)
{
	List_Clear(&theObj->mChildren);
	List_Clear(&theObj->mEmitters);
}

void Obj_Delete(Obj* theObj)
{
	Obj_Destroy(theObj);
	::operator delete((void*)theObj);
}

int Obj_Update(Obj* theObj)
{
	int aFrameCount = theObj->mFrameCount;
	theObj->mOldX = theObj->mX;
	theObj->mOldY = theObj->mY;
	int aResult = 1;
	theObj->mX = theObj->mVx + theObj->mX;
	theObj->mY = theObj->mVy + theObj->mY;
	if (0 < aFrameCount && (theObj->mFrameTimer = theObj->mFrameTimer + 1, theObj->mFrameTime <= theObj->mFrameTimer))
	{
		theObj->mFrame = theObj->mFrame + theObj->mFrameStep;
		int aFrame = theObj->mFrame;
		theObj->mFrameTimer = 0;
		if (aFrame < aFrameCount)
		{
			if (-1 < aFrame)
				goto stepped;
			if (theObj->mOneShot == 0)
				aFrame = aFrameCount + -1;
			else
			{
				aFrame = aFrame + 1;
				aResult = 0;
			}
		}
		else
		{
			if (theObj->mOneShot == 0)
			{
				if (theObj->mLoop == 0)
				{
					theObj->mFrame = aFrameCount + -1;
					theObj->mState = 2;
				}
				else
					theObj->mFrame = 0;
				goto stepped;
			}
			aFrame = aFrame + -1;
			aResult = 0;
		}
		theObj->mFrame = aFrame;
	}
stepped:
	if (aFrameCount != 0)
	{
		Image* anImage = Animation_Frame(theObj->mAnim, theObj->mFrame);
		theObj->mImage = anImage;
		if (theObj->mMidHandle != 0)
			Image_SetMidHandle(anImage);
	}
	if (theObj->mState == 2)
	{
		theObj->mDrawKind = 4;
		theObj->mFade = (float)((double)theObj->mFade + 1.0);
		double v = (double)theObj->mFade * 0.5;
		if (v < 0.0)
			v = 0.0;
		else if (255.0 < v)
			v = 255.0;
		theObj->mAlpha = (int)(255.0 - v);
	}
	theObj->mAge = (float)(1.0 + (double)theObj->mAge);
	if (theObj->mLifetime != 0.0f && theObj->mLifetime <= theObj->mAge)
		aResult = 0;
	List_Each(&theObj->mChildren, [&](Obj* theChild) {
		if (Obj_Update(theChild) == 0)
			List_RemoveCurrent(&theObj->mChildren);
	});
	return aResult;
}

void Obj_DrawTo(Obj* theObj, uint8_t* theDst, int theDPitch)
{
	Image* anImage = theObj->mImage;
	if (anImage != nullptr)
	{
		Palette* aPalette;
		if (theObj->mTint == 0)
			aPalette = theObj->mAnim != nullptr ? &theObj->mAnim->mPalette : nullptr;
		else if (theObj->mTint == -1 || theObj->mAnim == nullptr)
			aPalette = nullptr;
		else
			aPalette = &theObj->mAnim->mTints[theObj->mTint];
		uint32_t aSeq = (uint32_t)(uintptr_t)theObj;
		DrawList_Add(anImage, aSeq, (int)theObj->mX, (int)theObj->mY, theObj->mDepth, theObj->mDrawKind, theDst,
			theDPitch, theObj->mR, theObj->mG, theObj->mB, theObj->mAlpha, aPalette, nullptr);
		if (theObj->mDirty != 0)
			Dirty_AddClippedImage(&gDirty, (int)theObj->mX, (int)theObj->mY, theObj->mImage, SCREEN_CLIP);
		if (theObj->mShadow != 0)
		{
			int anAlpha = (theObj->mAlpha * 0x82) / 0xff;
			int aSy = (int)((double)theObj->mY + 5.0);
			int aSx = (int)(5.0 + (double)theObj->mX);
			DrawList_Add(theObj->mImage, aSeq, aSx, aSy, theObj->mDepth + -1, 5, theDst, theDPitch, 10, 10, 10, anAlpha,
				nullptr, nullptr);
			if (theObj->mDirty != 0)
			{
				aSy = (int)((double)theObj->mY + 5.0);
				aSx = (int)(5.0 + (double)theObj->mX);
				Dirty_AddClippedImage(&gDirty, aSx, aSy, theObj->mImage, SCREEN_CLIP);
			}
		}
	}
	List_Each(&theObj->mChildren, [](Obj* theChild) { Obj_Draw(theChild); });
}

void Obj_Draw(Obj* theObj)
{
	Obj_DrawTo(theObj, gScreenBits, gScreenPitch);
}

void Obj_UpdateEmitters(Obj* theObj)
{
	List_Each(&theObj->mEmitters, [](Obj* theEmitter) { Emitter_Update(theEmitter); });
}

void Obj_Copy(Obj* theObj, const Obj* theFrom)
{
	theObj->mKind = theFrom->mKind;
	theObj->mState = theFrom->mState;
	theObj->mX = theFrom->mX;
	theObj->mY = theFrom->mY;
	theObj->mDepth = theFrom->mDepth;
	theObj->mOldX = theFrom->mOldX;
	theObj->mOldY = theFrom->mOldY;
	theObj->mVx = theFrom->mVx;
	theObj->mVy = theFrom->mVy;
	theObj->mSpeed = theFrom->mSpeed;
	theObj->mR = theFrom->mR;
	theObj->mG = theFrom->mG;
	theObj->mB = theFrom->mB;
	theObj->mAlpha = theFrom->mAlpha;
	theObj->mImage = theFrom->mImage;
	theObj->mDrawKind = theFrom->mDrawKind;
	theObj->mDirty = theFrom->mDirty;
	theObj->mLifetime = theFrom->mLifetime;
	theObj->mAnim = theFrom->mAnim;
	theObj->mFrame = theFrom->mFrame;
	theObj->mFrameCount = theFrom->mFrameCount;
	theObj->mFrameTime = theFrom->mFrameTime;
	theObj->mFrameTimer = theFrom->mFrameTimer;
	theObj->mFade = theFrom->mFade;
	theObj->mShadow = theFrom->mShadow;
	theObj->mDieAnim = theFrom->mDieAnim;
	theObj->mDieFrameCount = theFrom->mDieFrameCount;
	theObj->mDieFrameTime = theFrom->mDieFrameTime;
	theObj->mOneShot = theFrom->mOneShot;
	theObj->mExploSound = theFrom->mExploSound;
	theObj->mAppearSound = theFrom->mAppearSound;
	theObj->mMidHandle = theFrom->mMidHandle;
}


// The attributes every object of the game file takes. Positions are in 800 by 600 pixels (the original scales by
// 800.0 / 800.0 and 600.0 / 600.0); "left", "right", "top" and "bottom" put an object with an image at an edge.
int Obj_FromXml(Obj* theObj, const char* p, const XmlTag* theTag)
{
	(void)p;
	if (Xml_GetString(theTag, "animation") != nullptr)
	{
		theObj->mAnim = Animation_Find(Xml_GetString(theTag, "animation"), &theObj->mFrameCount, &theObj->mFrameTime);
		if (theObj->mAnim == nullptr)
			return 0;
	}
	if (Xml_GetString(theTag, "explo_animation") != nullptr)
		theObj->mDieAnim = Animation_Find(Xml_GetString(theTag, "explo_animation"), &theObj->mDieFrameCount,
			&theObj->mDieFrameTime);
	if (theObj->mAnim != nullptr)
		theObj->mImage = Animation_Frame(theObj->mAnim, 0);
	if (Xml_GetString(theTag, "mid_handle") != nullptr)
		theObj->mMidHandle = 1;
	theObj->mY = 0.0f;
	theObj->mX = 0.0f;
	if (Xml_GetString(theTag, "x") != nullptr)
	{
		if (strcmp(Xml_GetString(theTag, "x"), "left") == 0 && theObj->mImage != nullptr)
			theObj->mX = 0.0f;
		else if (strcmp(Xml_GetString(theTag, "x"), "right") == 0 && theObj->mImage != nullptr)
			theObj->mX = (float)(800 - theObj->mImage->mW);
		else
			theObj->mX = (float)(Xml_GetFloat(theTag, "x") * 800.0 / 800.0);
	}
	if (Xml_GetString(theTag, "y") != nullptr)
	{
		if (strcmp(Xml_GetString(theTag, "y"), "top") == 0 && theObj->mImage != nullptr)
			theObj->mY = 0.0f;
		else if (strcmp(Xml_GetString(theTag, "y"), "bottom") == 0 && theObj->mImage != nullptr)
			theObj->mY = (float)(600 - theObj->mImage->mH);
		else
			theObj->mY = (float)(Xml_GetFloat(theTag, "y") * 600.0 / 600.0);
	}
	if (Xml_GetFloat(theTag, "z") != 0.0)
		theObj->mDepth = (int)Xml_GetFloat(theTag, "z");
	theObj->mVx = (float)(Xml_GetFloat(theTag, "x_speed") * 800.0 / 800.0);
	theObj->mVy = (float)(Xml_GetFloat(theTag, "y_speed") * 800.0 / 800.0);
	theObj->mSpeed = (float)(Xml_GetFloat(theTag, "velocity") * 800.0 / 800.0);
	theObj->mDirty = Xml_GetInt(theTag, "buffered");
	if (Xml_GetString(theTag, "explo_sound") != nullptr)
		theObj->mExploSound = Sound_Load(Xml_GetString(theTag, "explo_sound"));
	if (Xml_GetString(theTag, "position") != nullptr)
	{
		// a key of the level's map: the middle of its cell
		const char* aKey = Xml_GetString(theTag, "position");
		int aCol = 0;
		int aRow = 0;
		Level_FindKey(gLevels.mCurrent, *aKey, &aCol, &aRow);
		aCol = aCol * 0x10 + 8;
		aRow = aRow * 0x10 + 8;
		theObj->mX = (float)aCol;
		theObj->mY = (float)aRow;
	}
	if (Xml_GetString(theTag, "terminate_on_end") != nullptr)
		theObj->mOneShot = Xml_GetInt(theTag, "terminate_on_end");
	if (Xml_GetString(theTag, "loop") != nullptr)
		theObj->mLoop = Xml_GetInt(theTag, "loop");
	if (Xml_GetString(theTag, "color") != nullptr)
	{
		// a tint by name, else the draw list's colour and alpha as "rrggbbaa"
		char aName[256];
		strcpy(aName, Xml_GetString(theTag, "color"));
		if (strcmp(aName, "black") == 0)
			theObj->mTint = 0;
		else if (strcmp(aName, "white") == 0)
			theObj->mTint = 1;
		else if (strcmp(aName, "red") == 0)
			theObj->mTint = 2;
		else if (strcmp(aName, "orange") == 0)
			theObj->mTint = 3;
		else if (strcmp(aName, "yellow") == 0)
			theObj->mTint = 4;
		else if (strcmp(aName, "green") == 0)
			theObj->mTint = 5;
		else if (strcmp(aName, "cyan") == 0)
			theObj->mTint = 6;
		else if (strcmp(aName, "blue") == 0)
			theObj->mTint = 7;
		else if (strcmp(aName, "purple") == 0)
			theObj->mTint = 8;
		else if (strcmp(aName, "magenta") == 0)
			theObj->mTint = 9;
		else
			ParseHex4(Xml_GetString(theTag, "color"), &theObj->mR, &theObj->mG, &theObj->mB, &theObj->mAlpha);
	}
	else
		theObj->mTint = 5; // (the original shares green's store: an object without a colour gets tint 5)
	if (Xml_GetString(theTag, "blit_type") != nullptr)
	{
		if (strcmp(Xml_GetString(theTag, "blit_type"), "alpha_edge") == 0)
			theObj->mDrawKind = 1;
		if (strcmp(Xml_GetString(theTag, "blit_type"), "blend") == 0)
		{
			theObj->mDrawKind = 4;
			return 1;
		}
		if (strcmp(Xml_GetString(theTag, "blit_type"), "alpha") == 0)
		{
			theObj->mDrawKind = 2;
			return 1;
		}
		if (strcmp(Xml_GetString(theTag, "blit_type"), "solid") == 0)
		{
			theObj->mDrawKind = 3;
			return 1;
		}
		if (strcmp(Xml_GetString(theTag, "blit_type"), "block") == 0)
		{
			theObj->mDrawKind = 0;
			return 1;
		}
		if (strcmp(Xml_GetString(theTag, "blit_type"), "add") == 0)
			theObj->mDrawKind = 6;
	}
	return 1;
}

// Three pieces of shrapnel (the original writes each set out)
static void Explode_Shrapnel(Obj* theObj, float theVx, float theVy, const char* theFirst, const char* theLast)
{
	for (int i = 0; i < 3; i++)
	{
		ParticleSystem_New();
		int aLifetime = RandRange(0x4b0, 2000);
		int aFrameTime = RandRange(0x1e, 100);
		int aLast = Bank_IndexOf(&gArtBank, theLast);
		int aFirst = Bank_IndexOf(&gArtBank, theFirst);
		float aVy = (float)((double)RandRange(-180, -60) / 1000.0 + (double)theVy);
		float aVx = (float)((double)RandRange(-100, 100) / 1000.0 + (double)theVx);
		float aY = (float)RandRange(-10, 10) + theObj->mY;
		float aX = (float)RandRange(-10, 10) + theObj->mX;
		ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aX, aY, aVx, aVy, aFirst, aLast, aFrameTime, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0x2e4, 10, aLifetime, 0.9999f, 0.0002f, 4);
	}
}

// The death animation, as a one-shot drawn above the rest, with a ball of fire and shrapnel thrown up and along
// the object's way
void Obj_Explode(Obj* theObj, float theVx, float theVy)
{
	theObj->mAnim = theObj->mDieAnim;
	theObj->mFrame = 0;
	theObj->mFrameCount = theObj->mDieFrameCount;
	theObj->mOneShot = 1;
	theObj->mFrameTime = theObj->mDieFrameTime;
	theObj->mFrameTimer = 0;
	theObj->mState = 1;
	theObj->mDepth = 9;
	theObj->mShadow = 0;
	for (int i = 0; i < 0x18; i++)
	{
		ParticleSystem_New();
		int aLifetime = RandRange(700, 0x44c);
		int aLast = Bank_IndexOf(&gArtBank, "exp_4");
		int aFirst = Bank_IndexOf(&gArtBank, "exp_0");
		aFirst = aFirst + RandRange(0, 3);
		float aVy = (float)((double)RandRange(-100, 100) / 1000.0);
		float aVx = (float)((double)RandRange(-100, 100) / 1000.0);
		float aY = (float)RandRange(-10, 10) + theObj->mY;
		float aX = (float)RandRange(-10, 10) + theObj->mX;
		ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff, 0xff,
			0xe7, 0xa6, 0x39, 0x21c, 10, aLifetime, 1.0f, 0.0f, 5);
	}
	Explode_Shrapnel(theObj, theVx, theVy, "shrapnel01_0001", "shrapnel01_0016");
	if (-1 < theObj->mExploSound)
	{
		// (the original works out a pan from x for a play call that takes none)
		Sound_Play(theObj->mExploSound);
	}
	Explode_Shrapnel(theObj, theVx, theVy, "shrapnel03_0001", "shrapnel03_0016");
	Explode_Shrapnel(theObj, theVx, theVy, "shrapnel04_0001", "shrapnel04_0016");
}

} // namespace cb
