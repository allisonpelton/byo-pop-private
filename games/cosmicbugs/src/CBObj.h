#ifndef __COSMICBUGS_CBOBJ_H__
#define __COSMICBUGS_CBOBJ_H__

// The base every game object starts with (0x40eac0..0x40f0f0), 0xb0 bytes. Objects of every kind sit in lists of
// Obj and are deleted through the base (0x407430 runs only the base's destructor), so the kinds that extend it
// add no members that need destroying.

#include "CBGame.h"

namespace cb
{

struct Obj
{
	Obj* next;                 // +0x00
	Obj* prev;                 // +0x04
	int kind;                  // +0x08
	int state;                 // +0x0c  2: fading out (Obj_Update)
	float x;                   // +0x10
	float y;                   // +0x14
	int depth;                 // +0x18  the draw list's
	float oldX;                // +0x1c  before the last move
	float oldY;                // +0x20
	float vx;                  // +0x24
	float vy;                  // +0x28
	int unk2c;                 // +0x2c
	int r;                     // +0x30  the draw list's colour and alpha
	int g;                     // +0x34
	int b;                     // +0x38
	int alpha;                 // +0x3c
	int tint;                  // +0x40  0: the animation's colours, -1 or no animation: none, else tints[tint]
	Image* image;              // +0x44
	int drawKind;              // +0x48  the draw list's (0 blit .. 6 additive)
	int dirty;                 // +0x4c  also marks the dirty grid
	float age;                 // +0x50  updates since made
	float lifetime;            // +0x54  0: for ever
	List<Obj> emitters;        // +0x58  (deleted as objects too)
	List<Obj> children;        // +0x64
	Animation* anim;           // +0x70
	int frame;                 // +0x74
	int frameCount;            // +0x78
	int frameTime;             // +0x7c  updates a frame
	int frameTimer;            // +0x80
	int frameStep;             // +0x84
	int loop;                  // +0x88  0: the last frame starts the fade out
	float fade;                // +0x8c
	int shadow;                // +0x90  a dark copy 5 pixels down and right, a depth below
	int unk94;                 // +0x94
	int unk98;                 // +0x98
	int unk9c;                 // +0x9c
	int oneShot;               // +0xa0  the animation's end (either way) finishes the object
	int unka4;                 // +0xa4
	int unka8;                 // +0xa8
	int midHandle;             // +0xac  each frame is drawn about its middle
};

void Obj_Init(Obj* theObj);                                      // 0x40f060
void Obj_Destroy(Obj* theObj);                                   // 0x40f0f0
// 0x407430's delete: the base's destructor, then the memory
void List_DeleteNode(Obj* theObj);
// 0x40ee80: 0 when the object is done (its lifetime is up, or a one-shot animation reached an end)
int Obj_Update(Obj* theObj);
// 0x40eac0: queues the object (and its shadow) and its children on the draw list
void Obj_DrawTo(Obj* theObj, uint8_t* theDst, int theDPitch);
void Obj_Draw(Obj* theObj);                                      // 0x40ec70: onto the screen
void Obj_UpdateEmitters(Obj* theObj);                            // 0x40ee40
void Obj_ClearChildren(List<Obj>* theList);                      // 0x40f040

// 0x40e3c0: the fields a copy of an object takes (not the links, lists, tint, age, frame step or loop)
void Obj_Copy(Obj* theObj, const Obj* theFrom);

// 0x40b7d0 (CBEmitter.cpp; its result, the new particle system, goes unused)
void Emitter_Update(Obj* theEmitter);

// The screen the game draws on (gScreenBits 0x46e394 and its pitch), set by the host each frame
extern uint8_t* gScreenBits;
extern int gScreenPitch;

} // namespace cb

#endif
