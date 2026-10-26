// The game objects' base (0x40eac0..0x40f0f0)

#include "CBObj.h"

#include <cstring>
#include <new>

namespace cb
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
	theObj->unka4 = -1;
}

void Obj_Destroy(Obj* theObj)
{
	List_Clear(&theObj->children);
	List_Clear(&theObj->emitters);
}

void List_DeleteNode(Obj* theObj)
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

void Obj_ClearChildren(List<Obj>* theList)
{
	List_Clear(theList);
}

} // namespace cb
