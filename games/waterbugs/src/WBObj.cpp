// The game objects' base (0x40ecb0..0x40f2c0)

#include "WBObj.h"
#include "WBLevel.h"
#include "R64Xml.h"

#include <cstring>
#include <new>

namespace wb
{

uint8_t* gScreenBits = nullptr;
int gScreenPitch = 0;
Dirty gDirty;
const int kScreenClip[4] = {0, 0, 0x31f, 0x257};

// The original leaves the links, the colour and a few others as they were; here the base starts all 0
void Obj_Init(Obj* theObj)
{
	std::memset((void*)theObj, 0, sizeof(Obj));
	List_Init(&theObj->emitters);
	List_Init(&theObj->children);
	theObj->x = 0.0f;
	theObj->y = 0.0f;
	theObj->oldX = 0.0f;
	theObj->image = nullptr;
	theObj->oldY = 0.0f;
	theObj->kind = 0;
	theObj->vx = 0.0f;
	theObj->state = 0;
	theObj->vy = 0.0f;
	theObj->depth = 0;
	theObj->age = 0.0f;
	theObj->tint = 0;
	theObj->lifetime = 0.0f;
	theObj->drawKind = 0;
	theObj->dirty = 0;
	theObj->fade = 0.0f;
	theObj->anim = nullptr;
	theObj->frameCount = 0;
	theObj->frameTime = 0;
	theObj->frame = 0;
	theObj->frameTimer = 0;
	theObj->midHandle = 0;
	theObj->oneShot = 0;
	theObj->shadow = 0;
	theObj->loop = 1;
	theObj->frameStep = 1;
	theObj->exploSound = -1;
}

// 0x419c00 (the same destructor again, for the nodes of another list)
void Obj_Destroy(Obj* theObj)
{
	List_Clear(&theObj->children);
	List_Clear(&theObj->emitters);
}

void Obj_Delete(Obj* theObj)
{
	Obj_Destroy(theObj);
	::operator delete((void*)theObj);
}

int Obj_Update(Obj* theObj)
{
	int n = theObj->frameCount;
	theObj->oldX = theObj->x;
	theObj->oldY = theObj->y;
	int aResult = 1;
	theObj->x = theObj->vx + theObj->x;
	theObj->y = theObj->vy + theObj->y;
	if (0 < n && (theObj->frameTimer = theObj->frameTimer + 1, theObj->frameTime <= theObj->frameTimer))
	{
		theObj->frame = theObj->frame + theObj->frameStep;
		int f = theObj->frame;
		theObj->frameTimer = 0;
		if (f < n)
		{
			if (-1 < f)
				goto stepped;
			if (theObj->oneShot == 0)
				f = n + -1;
			else
			{
				f = f + 1;
				aResult = 0;
			}
		}
		else
		{
			if (theObj->oneShot == 0)
			{
				if (theObj->loop == 0)
				{
					theObj->frame = n + -1;
					theObj->state = 2;
				}
				else
					theObj->frame = 0;
				goto stepped;
			}
			f = f + -1;
			aResult = 0;
		}
		theObj->frame = f;
	}
stepped:
	if (n != 0)
	{
		Image* anImage = Animation_Frame(theObj->anim, theObj->frame);
		theObj->image = anImage;
		if (theObj->midHandle != 0)
			Image_SetMidHandle(anImage);
	}
	if (theObj->state == 2)
	{
		theObj->drawKind = 4;
		theObj->fade = (float)((double)theObj->fade + 1.0);
		double v = (double)theObj->fade * 0.5;
		if (v < 0.0)
			v = 0.0;
		else if (255.0 < v)
			v = 255.0;
		theObj->alpha = (int)(255.0 - v);
	}
	theObj->age = (float)(1.0 + (double)theObj->age);
	if (theObj->lifetime != 0.0f && theObj->lifetime <= theObj->age)
		aResult = 0;
	List_Each(&theObj->children, [&](Obj* aChild) {
		if (Obj_Update(aChild) == 0)
			List_RemoveCurrent(&theObj->children);
	});
	return aResult;
}

void Obj_DrawTo(Obj* theObj, uint8_t* theDst, int theDPitch)
{
	Image* anImage = theObj->image;
	if (anImage != nullptr)
	{
		Palette* aPalette;
		if (theObj->tint == 0)
			aPalette = theObj->anim != nullptr ? &theObj->anim->palette : nullptr;
		else if (theObj->tint == -1 || theObj->anim == nullptr)
			aPalette = nullptr;
		else
			aPalette = &theObj->anim->tints[theObj->tint];
		uint32_t aSeq = (uint32_t)(uintptr_t)theObj;
		DrawList_Add(anImage, aSeq, (int)theObj->x, (int)theObj->y, theObj->depth, theObj->drawKind, theDst,
			theDPitch, theObj->r, theObj->g, theObj->b, theObj->alpha, aPalette, nullptr);
		if (theObj->dirty != 0)
			Dirty_AddClippedImage(&gDirty, (int)theObj->x, (int)theObj->y, theObj->image, kScreenClip);
		if (theObj->shadow != 0)
		{
			int anAlpha = (theObj->alpha * 0x82) / 0xff;
			int sy = (int)((double)theObj->y + 5.0);
			int sx = (int)(5.0 + (double)theObj->x);
			DrawList_Add(theObj->image, aSeq, sx, sy, theObj->depth + -1, 5, theDst, theDPitch, 10, 10, 10, anAlpha,
				nullptr, nullptr);
			if (theObj->dirty != 0)
			{
				sy = (int)((double)theObj->y + 5.0);
				sx = (int)(5.0 + (double)theObj->x);
				Dirty_AddClippedImage(&gDirty, sx, sy, theObj->image, kScreenClip);
			}
		}
	}
	List_Each(&theObj->children, [](Obj* aChild) { Obj_Draw(aChild); });
}

void Obj_Draw(Obj* theObj)
{
	Obj_DrawTo(theObj, gScreenBits, gScreenPitch);
}

void Obj_UpdateEmitters(Obj* theObj)
{
	List_Each(&theObj->emitters, [](Obj* anEmitter) { Emitter_Update(anEmitter); });
}

void Obj_Copy(Obj* theObj, const Obj* theFrom)
{
	theObj->kind = theFrom->kind;
	theObj->state = theFrom->state;
	theObj->x = theFrom->x;
	theObj->y = theFrom->y;
	theObj->depth = theFrom->depth;
	theObj->oldX = theFrom->oldX;
	theObj->oldY = theFrom->oldY;
	theObj->vx = theFrom->vx;
	theObj->vy = theFrom->vy;
	theObj->speed = theFrom->speed;
	theObj->r = theFrom->r;
	theObj->g = theFrom->g;
	theObj->b = theFrom->b;
	theObj->alpha = theFrom->alpha;
	theObj->image = theFrom->image;
	theObj->drawKind = theFrom->drawKind;
	theObj->dirty = theFrom->dirty;
	theObj->lifetime = theFrom->lifetime;
	theObj->anim = theFrom->anim;
	theObj->frame = theFrom->frame;
	theObj->frameCount = theFrom->frameCount;
	theObj->frameTime = theFrom->frameTime;
	theObj->frameTimer = theFrom->frameTimer;
	theObj->fade = theFrom->fade;
	theObj->shadow = theFrom->shadow;
	theObj->dieAnim = theFrom->dieAnim;
	theObj->dieFrameCount = theFrom->dieFrameCount;
	theObj->dieFrameTime = theFrom->dieFrameTime;
	theObj->oneShot = theFrom->oneShot;
	theObj->exploSound = theFrom->exploSound;
	theObj->appearSound = theFrom->appearSound;
	theObj->midHandle = theFrom->midHandle;
}


// The attributes every object of the game file takes. Positions are in 800 by 600 pixels (the original scales by
// 800.0 / 800.0 and 600.0 / 600.0); "left", "right", "top" and "bottom" put an object with an image at an edge.
int Obj_FromXml(Obj* theObj, const char* p, const XmlTag* theTag)
{
	(void)p;
	if (Xml_GetString(theTag, "animation") != nullptr)
	{
		theObj->anim = Animation_Find(Xml_GetString(theTag, "animation"), &theObj->frameCount, &theObj->frameTime);
		if (theObj->anim == nullptr)
			return 0;
	}
	if (Xml_GetString(theTag, "explo_animation") != nullptr)
		theObj->dieAnim = Animation_Find(Xml_GetString(theTag, "explo_animation"), &theObj->dieFrameCount,
			&theObj->dieFrameTime);
	if (theObj->anim != nullptr)
		theObj->image = Animation_Frame(theObj->anim, 0);
	if (Xml_GetString(theTag, "mid_handle") != nullptr)
		theObj->midHandle = 1;
	theObj->y = 0.0f;
	theObj->x = 0.0f;
	if (Xml_GetString(theTag, "x") != nullptr)
	{
		if (strcmp(Xml_GetString(theTag, "x"), "left") == 0 && theObj->image != nullptr)
			theObj->x = 0.0f;
		else if (strcmp(Xml_GetString(theTag, "x"), "right") == 0 && theObj->image != nullptr)
			theObj->x = (float)(800 - theObj->image->w);
		else
			theObj->x = (float)(Xml_GetFloat(theTag, "x") * 800.0 / 800.0);
	}
	if (Xml_GetString(theTag, "y") != nullptr)
	{
		if (strcmp(Xml_GetString(theTag, "y"), "top") == 0 && theObj->image != nullptr)
			theObj->y = 0.0f;
		else if (strcmp(Xml_GetString(theTag, "y"), "bottom") == 0 && theObj->image != nullptr)
			theObj->y = (float)(600 - theObj->image->h);
		else
			theObj->y = (float)(Xml_GetFloat(theTag, "y") * 600.0 / 600.0);
	}
	if (Xml_GetFloat(theTag, "z") != 0.0)
		theObj->depth = (int)Xml_GetFloat(theTag, "z");
	theObj->vx = (float)(Xml_GetFloat(theTag, "x_speed") * 800.0 / 800.0);
	theObj->vy = (float)(Xml_GetFloat(theTag, "y_speed") * 800.0 / 800.0);
	theObj->speed = (float)(Xml_GetFloat(theTag, "velocity") * 800.0 / 800.0);
	theObj->dirty = Xml_GetInt(theTag, "buffered");
	if (Xml_GetString(theTag, "explo_sound") != nullptr)
		theObj->exploSound = Sound_Load(Xml_GetString(theTag, "explo_sound"));
	if (Xml_GetString(theTag, "position") != nullptr)
	{
		// a key of the level's map: the middle of its cell
		const char* aKey = Xml_GetString(theTag, "position");
		int aCol = 0;
		int aRow = 0;
		Level_FindKey(gLevels.current, *aKey, &aCol, &aRow);
		aCol = aCol * 0x10 + 8;
		aRow = aRow * 0x10 + 8;
		theObj->x = (float)aCol;
		theObj->y = (float)aRow;
	}
	if (Xml_GetString(theTag, "terminate_on_end") != nullptr)
		theObj->oneShot = Xml_GetInt(theTag, "terminate_on_end");
	if (Xml_GetString(theTag, "loop") != nullptr)
		theObj->loop = Xml_GetInt(theTag, "loop");
	if (Xml_GetString(theTag, "color") != nullptr)
	{
		// a tint by name, else the draw list's colour and alpha as "rrggbbaa"
		char aName[256];
		strcpy(aName, Xml_GetString(theTag, "color"));
		if (strcmp(aName, "black") == 0)
			theObj->tint = 0;
		else if (strcmp(aName, "white") == 0)
			theObj->tint = 1;
		else if (strcmp(aName, "red") == 0)
			theObj->tint = 2;
		else if (strcmp(aName, "orange") == 0)
			theObj->tint = 3;
		else if (strcmp(aName, "yellow") == 0)
			theObj->tint = 4;
		else if (strcmp(aName, "green") == 0)
			theObj->tint = 5;
		else if (strcmp(aName, "cyan") == 0)
			theObj->tint = 6;
		else if (strcmp(aName, "blue") == 0)
			theObj->tint = 7;
		else if (strcmp(aName, "purple") == 0)
			theObj->tint = 8;
		else if (strcmp(aName, "magenta") == 0)
			theObj->tint = 9;
		else
			ParseHex4(Xml_GetString(theTag, "color"), &theObj->r, &theObj->g, &theObj->b, &theObj->alpha);
	}
	else
		theObj->tint = 5; // (the original shares green's store: an object without a colour gets tint 5)
	if (Xml_GetString(theTag, "blit_type") != nullptr)
	{
		if (strcmp(Xml_GetString(theTag, "blit_type"), "alpha_edge") == 0)
			theObj->drawKind = 1;
		if (strcmp(Xml_GetString(theTag, "blit_type"), "blend") == 0)
		{
			theObj->drawKind = 4;
			return 1;
		}
		if (strcmp(Xml_GetString(theTag, "blit_type"), "alpha") == 0)
		{
			theObj->drawKind = 2;
			return 1;
		}
		if (strcmp(Xml_GetString(theTag, "blit_type"), "solid") == 0)
		{
			theObj->drawKind = 3;
			return 1;
		}
		if (strcmp(Xml_GetString(theTag, "blit_type"), "block") == 0)
		{
			theObj->drawKind = 0;
			return 1;
		}
		if (strcmp(Xml_GetString(theTag, "blit_type"), "add") == 0)
			theObj->drawKind = 6;
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
		float aY = (float)RandRange(-10, 10) + theObj->y;
		float aX = (float)RandRange(-10, 10) + theObj->x;
		ParticleSystem_Add(gLevels.current->systems.current, aX, aY, aVx, aVy, aFirst, aLast, aFrameTime, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0x2e4, 10, aLifetime, 0.9999f, 0.0002f, 4);
	}
}

// The death animation, as a one-shot drawn above the rest, with a ball of fire and shrapnel thrown up and along
// the object's way
void Obj_Explode(Obj* theObj, float theVx, float theVy)
{
	theObj->anim = theObj->dieAnim;
	theObj->frame = 0;
	theObj->frameCount = theObj->dieFrameCount;
	theObj->oneShot = 1;
	theObj->frameTime = theObj->dieFrameTime;
	theObj->frameTimer = 0;
	theObj->state = 1;
	theObj->depth = 9;
	theObj->shadow = 0;
	for (int i = 0; i < 0x18; i++)
	{
		ParticleSystem_New();
		int aLifetime = RandRange(700, 0x44c);
		int aLast = Bank_IndexOf(&gArtBank, "exp_4");
		int aFirst = Bank_IndexOf(&gArtBank, "exp_0");
		aFirst = aFirst + RandRange(0, 3);
		float aVy = (float)((double)RandRange(-100, 100) / 1000.0);
		float aVx = (float)((double)RandRange(-100, 100) / 1000.0);
		float aY = (float)RandRange(-10, 10) + theObj->y;
		float aX = (float)RandRange(-10, 10) + theObj->x;
		ParticleSystem_Add(gLevels.current->systems.current, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff, 0xff,
			0xe7, 0xa6, 0x39, 0x21c, 10, aLifetime, 1.0f, 0.0f, 5);
	}
	Explode_Shrapnel(theObj, theVx, theVy, "shrapnel01_0001", "shrapnel01_0016");
	if (-1 < theObj->exploSound)
	{
		// (the original works out a pan from x for a play call that takes none)
		Sound_Play(theObj->exploSound);
	}
	Explode_Shrapnel(theObj, theVx, theVy, "shrapnel03_0001", "shrapnel03_0016");
	Explode_Shrapnel(theObj, theVx, theVy, "shrapnel04_0001", "shrapnel04_0016");
}

} // namespace wb
