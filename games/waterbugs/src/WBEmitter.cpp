// Particle emitters and particles (0x40ad60..0x40c2c0)

#include "WBEmitter.h"
#include "WBLevel.h"

#include <cstring>

namespace wb
{

static ParticleSystem gSystems[4000];     // 0x62de20
static int gSystemNext = 0;               // 0x478620
static Particle gParticles[8000];         // 0x478628
static int gParticleNext = 0;             // 0x478624

void Particles_Init()
{
	for (ParticleSystem& aSystem : gSystems)
		ParticleSystem_Init(&aSystem);
	for (Particle& aParticle : gParticles)
		Obj_Init(&aParticle);
}

int Particle_Update(Particle* theParticle)
{
	theParticle->oldX = theParticle->x;
	theParticle->oldY = theParticle->y;
	int aResult = 1;
	theParticle->x = theParticle->vx + theParticle->x;
	theParticle->y = theParticle->vy + theParticle->y;
	if (theParticle->frameTime != 0)
	{
		int n = theParticle->frameCount;
		if (0 < n && (theParticle->frameTimer = theParticle->frameTimer + 1,
						 theParticle->frameTime <= theParticle->frameTimer))
		{
			theParticle->frame = theParticle->frame + 1;
			theParticle->frameTimer = 0;
			if (n <= theParticle->frame)
			{
				if (theParticle->oneShot == 0)
					theParticle->frame = 0;
				else
				{
					theParticle->frame = theParticle->frame + -1;
					aResult = 0;
				}
			}
		}
		if (n != 0)
		{
			Image* anImage = Bank_ImageAt(&gArtBank, theParticle->firstImage + theParticle->frame);
			theParticle->image = anImage;
			if (theParticle->midHandle != 0)
				Image_SetMidHandle(anImage);
		}
	}
	float anAge = theParticle->age + 1.0f;
	theParticle->age = anAge;
	if (theParticle->lifetime != 0.0f && theParticle->lifetime <= anAge)
		return 0;
	return aResult;
}

void Particle_Draw(Particle* theParticle)
{
	float t = theParticle->age / theParticle->lifetime;
	double anAlpha = (double)(theParticle->endAlpha - theParticle->startAlpha) * (double)t +
		(double)theParticle->startAlpha;
	if (255.0 < anAlpha)
		anAlpha = 255.0;
	theParticle->alpha = (int)anAlpha;
	theParticle->r = (int)((double)(theParticle->endR - theParticle->startR) * (double)t + (double)theParticle->startR);
	theParticle->g = (int)((double)(theParticle->endG - theParticle->startG) * (double)t + (double)theParticle->startG);
	theParticle->b = (int)((double)(theParticle->endB - theParticle->startB) * (double)t + (double)theParticle->startB);
	if (theParticle->frameTime == 0)
	{
		Animation* anAnim = theParticle->anim;
		if (anAnim == nullptr)
		{
			// a 64-bit fistp with the rounding set to truncate
			int aFrame = (int)(int64_t)((double)t * (double)theParticle->frameCount);
			theParticle->image = Bank_ImageAt(&gArtBank, aFrame + theParticle->firstImage);
		}
		else
		{
			float aLast = (float)((double)theParticle->frameCount - 1.0);
			theParticle->image = Animation_Frame(anAnim, (int)((double)t * (double)aLast));
		}
	}
	if (theParticle->image == nullptr)
		return;
	Image_SetMidHandle(theParticle->image);
	Obj_Draw(theParticle);
}

// The port clears the members past the base, which the original leaves as the allocation found them
void Emitter_Append(List<Obj>* theList)
{
	Emitter* anEmitter = new Emitter();
	Obj_Init(anEmitter);
	List_Append(theList, (Obj*)anEmitter);
}

// 0x40afa0
void Emitter_Setup(Emitter* theEmitter, Obj* theParent, int theX, int theY, float theXSpeed, float theYSpeed,
	int theFrequency, int theRotation, const char* theAnim, int theKind, float theStartAlpha, float theEndAlpha,
	float theTime)
{
	theEmitter->parent = theParent;
	theEmitter->kind = theKind;
	theEmitter->anim = Animation_Find(theAnim, &theEmitter->frameCount, &theEmitter->frameTime);
	theEmitter->oldX = (float)(theX - Animation_Frame(theEmitter->parent->anim, 0)->w / 2);
	int aHeight = Animation_Frame(theEmitter->parent->anim, 0)->h;
	theEmitter->frequency = theFrequency;
	theEmitter->rotation = theRotation;
	theEmitter->oldY = (float)(theY - aHeight / 2);
	theEmitter->vx = (float)((double)theXSpeed / 1000.0);
	theEmitter->vy = (float)((double)theYSpeed / 1000.0);
	theEmitter->startAlpha = (int)theStartAlpha;
	theEmitter->endAlpha = (int)theEndAlpha;
	theEmitter->time = (int)theTime;
	theEmitter->drawKind = 4;
	theEmitter->startR = 0xff;
	theEmitter->startG = 0xff;
	theEmitter->startB = 0xff;
	theEmitter->endR = 0xff;
	theEmitter->endG = 0xff;
	theEmitter->endB = 0xff;
	theEmitter->variance = 0;
}

void Emitter_CopyPrev(Emitter* theEmitter, int theAngle)
{
	Emitter* aPrev = static_cast<Emitter*>(theEmitter->prev);
	theEmitter->parent = aPrev->parent;
	theEmitter->kind = aPrev->kind;
	theEmitter->anim = aPrev->anim;
	theEmitter->frameTime = aPrev->frameTime;
	theEmitter->frameCount = aPrev->frameCount;
	float anOffX = aPrev->oldX;
	float anOffY = aPrev->oldY;
	theEmitter->frequency = aPrev->frequency;
	theEmitter->rotation = aPrev->rotation;
	float aVx = aPrev->vx;
	float aVy = aPrev->vy;
	theEmitter->oldX = (float)((double)CosDeg(theAngle) * anOffX - (double)SinDeg(theAngle) * anOffY);
	theEmitter->oldY = (float)((double)SinDeg(theAngle) * anOffX + (double)CosDeg(theAngle) * anOffY);
	theEmitter->vx = (float)((double)CosDeg(theAngle) * aVx - (double)SinDeg(theAngle) * aVy);
	theEmitter->vy = (float)((double)SinDeg(theAngle) * aVx + (double)CosDeg(theAngle) * aVy);
	aPrev = static_cast<Emitter*>(theEmitter->prev);
	theEmitter->startAlpha = aPrev->startAlpha;
	theEmitter->endAlpha = aPrev->endAlpha;
	theEmitter->time = aPrev->time;
	theEmitter->drawKind = 4;
	theEmitter->startR = 0xff;
	theEmitter->startG = 0xff;
	theEmitter->startB = 0xff;
	theEmitter->endR = 0xff;
	theEmitter->endG = 0xff;
	theEmitter->endB = 0xff;
	theEmitter->variance = 0;
}

// The parent, colours and variance are not copied: the caller (0x408930) sets the parent, and the colours keep what
// the allocation left (the port's allocation clears them; they only matter to draw kinds 3 and 5, which emitters
// never use, and to how many random numbers the colour jitter draws)
void Emitter_Copy(Emitter* theEmitter, const Emitter* theFrom)
{
	Obj_Copy(theEmitter, theFrom);
	theEmitter->frequency = theFrom->frequency;
	theEmitter->rotation = theFrom->rotation;
	theEmitter->startAlpha = theFrom->startAlpha;
	theEmitter->endAlpha = theFrom->endAlpha;
	theEmitter->time = theFrom->time;
}

// The original repeats the whole call for either parent; the x and y scaling are leftovers of a screen size
void Emitter_FromXml(Emitter* theEmitter, const XmlTag* theTag)
{
	if (Xml_GetInt(theTag, "copy_prev") != 0)
	{
		Emitter_CopyPrev(theEmitter, Xml_GetInt(theTag, "rotation"));
		return;
	}
	// (a missing emit_type would crash the original; here it is not angular)
	const char* aType = Xml_GetString(theTag, "emit_type");
	int aKind = (aType != nullptr && std::strcmp(aType, "angular") == 0) ? 0 : 2;
	Obj* aParent;
	if (gLevels.current != nullptr && gLevels.current->enemies.current != nullptr)
		aParent = gLevels.current->enemies.current;
	else
		aParent = gEnemyDefs.current;
	float aTime = (float)Xml_GetFloat(theTag, "time");
	float anEndAlpha = (float)Xml_GetFloat(theTag, "end_alpha");
	float aStartAlpha = (float)Xml_GetFloat(theTag, "start_alpha");
	const char* anAnim = Xml_GetString(theTag, "animation");
	int aRotation = Xml_GetInt(theTag, "rotation");
	int aFrequency = Xml_GetInt(theTag, "frequency");
	float aYSpeed = (float)((Xml_GetInt(theTag, "y_speed") * 600) / 600);
	float aXSpeed = (float)((Xml_GetInt(theTag, "x_speed") * 800) / 800);
	int aY = (Xml_GetInt(theTag, "y") * 600) / 600;
	int aX = (Xml_GetInt(theTag, "x") * 800) / 800;
	Emitter_Setup(theEmitter, aParent, aX, aY, aXSpeed, aYSpeed, aFrequency, aRotation, anAnim, aKind, aStartAlpha,
		anEndAlpha, aTime);
}

void ParticleSystem_New()
{
	Level* aLevel = gLevels.current;
	ParticleSystem* aSystem = &gSystems[gSystemNext];
	List_Append(&aLevel->systems, aSystem);
	gSystemNext = gSystemNext + 1;
	List_Init(&aSystem->particles);
	if (3999 < gSystemNext)
		gSystemNext = 0;
}

// 0x40b360
void ParticleSystem_Add(ParticleSystem* theSystem, float theX, float theY, float theVx, float theVy,
	int theFirstImage, int theLastImage, int theFrameTime, int theStartR, int theStartG, int theStartB, int theEndR,
	int theEndG, int theEndB, int theStartAlpha, int theEndAlpha, int theLifetime, float theDrag, float theGravity,
	int theDrawKind)
{
	List_Append(&theSystem->particles, &gParticles[gParticleNext]);
	gParticleNext = gParticleNext + 1;
	if (7999 < gParticleNext)
		gParticleNext = 0;
	Particle* p = theSystem->particles.current;
	p->image = nullptr;
	p->kind = 0;
	p->state = 0;
	p->frame = 0;
	p->frameTimer = 0;
	p->midHandle = 0;
	p->age = 0.0f;
	p->anim = nullptr;
	p->x = theX;
	p->y = theY;
	p->depth = 8;
	p->oldX = theX;
	p->oldY = theY;
	p->vx = theVx;
	p->vy = theVy;
	p->firstImage = theFirstImage;
	p->frameCount = (theLastImage - theFirstImage) + 1;
	p->frameTime = theFrameTime;
	p->startR = theStartR;
	p->startG = theStartG;
	p->startB = theStartB;
	p->endR = theEndR;
	p->endG = theEndG;
	p->endB = theEndB;
	p->startAlpha = theStartAlpha;
	p->endAlpha = theEndAlpha;
	p->lifetime = (float)theLifetime;
	p->drag = theDrag;
	p->gravity = theGravity;
	p->drawKind = theDrawKind;
	p->midHandle = 1;
	p->dirty = 1;
	p->owner = theSystem;
	if (theSystem->particles.current == theSystem->particles.head)
	{
		theSystem->x = theX;
		theSystem->y = theY;
		theSystem->vx = theVx;
		theSystem->vy = theVy;
	}
}

void ParticleSystem_Draw(ParticleSystem* theSystem)
{
	if (theSystem->particles.current != nullptr)
	{
		theSystem->particles.current = theSystem->particles.head;
		Particle_Draw(theSystem->particles.head);
	}
}

int Particle_Follow(Particle* theParticle)
{
	if (theParticle->age == 0.0f)
	{
		ParticleSystem* anOwner = theParticle->owner;
		theParticle->x = anOwner->x;
		theParticle->y = anOwner->y;
		theParticle->vx = anOwner->vx;
		theParticle->vy = anOwner->vy;
	}
	theParticle->vx = theParticle->drag * theParticle->vx;
	float aVy = theParticle->drag * theParticle->vy;
	theParticle->vy = aVy + theParticle->gravity;
	int aResult = Particle_Update(theParticle);
	theParticle->owner->x = theParticle->x;
	theParticle->owner->y = theParticle->y;
	theParticle->owner->vx = theParticle->vx;
	theParticle->owner->vy = theParticle->vy;
	return aResult;
}

int ParticleSystem_Update(ParticleSystem* theSystem)
{
	List<Particle>* aList = &theSystem->particles;
	if (aList->current != nullptr)
	{
		aList->current = aList->head;
		if (Particle_Follow(aList->head) == 0)
		{
			List_Unlink(aList);
			if (aList->current == nullptr)
				return 0;
		}
	}
	return 1;
}

ParticleSystem* ParticleSystem_Init(ParticleSystem* theSystem)
{
	Obj_Init(theSystem);
	List_Init(&theSystem->particles);
	return theSystem;
}

// 0x40b940's colour jitter: the colour moved by up to the variance and kept in 0..255, where (as in the original's
// clamp) each test draws a new random number
static int JitterColour(int theColour, int theVariance)
{
	if (RandRange(-theVariance, theVariance) + theColour >= 0 &&
		RandRange(-theVariance, theVariance) + theColour > 0xff)
		return 0xff;
	if (RandRange(-theVariance, theVariance) + theColour < 0)
		return 0;
	return RandRange(-theVariance, theVariance) + theColour;
}

void Emitter_Update(Obj* theObj)
{
	Emitter* e = static_cast<Emitter*>(theObj);
	float anAge = (float)((double)e->age + 1.0);
	e->age = anAge;
	int n = (int)anAge;
	if (n % e->frequency != 0)
		return;
	Obj* aParent = e->parent;
	int aBaseX = (int)((double)aParent->x + (double)e->oldX);
	int aBaseY = (int)((double)aParent->y + (double)e->oldY);
	float aVx = e->vx;
	float aVy = e->vy;
	int aStartR = e->startR;
	int aStartG = e->startG;
	int aStartB = e->startB;
	int anEndR = e->endR;
	int anEndG = e->endG;
	int anEndB = e->endB;
	int aLast = e->frameCount - 1;
	int x;
	int y;
	if (e->kind == 0)
	{
		int anAngle = (aParent->frame * e->rotation) / aParent->anim->count;
		int rx = (int)((double)CosDeg(anAngle) * e->oldX - (double)SinDeg(anAngle) * e->oldY);
		int ry = (int)((double)SinDeg(anAngle) * e->oldX + (double)CosDeg(anAngle) * e->oldY);
		x = (int)((double)rx + (double)e->parent->x);
		y = (int)((double)ry + (double)e->parent->y);
		float sx = e->vx;
		float sy = e->vy;
		if (e->vx < 0.0f)
		{
			// a spread: both speeds drawn from the x speed's range
			int aHigh = (int)(-(double)e->vx * 1000.0);
			int aLow = (int)(1000.0 * (double)e->vx);
			sx = (float)((double)RandRange(aLow, aHigh) / 1000.0);
			aHigh = (int)(-(double)e->vx * 1000.0);
			aLow = (int)(1000.0 * (double)e->vx);
			sy = (float)((double)RandRange(aLow, aHigh) / 1000.0);
		}
		aVx = (float)((double)CosDeg(anAngle) * sx + (double)e->parent->vx - (double)SinDeg(anAngle) * sy);
		aVy = (float)((double)SinDeg(anAngle) * sx + (double)e->parent->vy + (double)CosDeg(anAngle) * sy);
	}
	else
	{
		if (e->kind == 1)
		{
			RandMod(360);
			float aVx1000 = (float)((double)aVx * 1000.0);
			float aVy1000 = (float)((double)aVy * 1000.0);
			int aHigh = (int)aVx1000;
			aVx = (float)((double)RandRange((int)-aVx1000, aHigh) / 1000.0 + (double)e->parent->vx);
			aHigh = (int)aVy1000;
			aVy = (float)((double)RandRange((int)-aVy1000, aHigh) / 1000.0 + (double)e->parent->vy);
			aHigh = (int)aVx;
			x = (int)((double)RandRange((int)-aVx, aHigh) / 1000.0 + (double)aBaseX);
			aHigh = (int)aVy;
			y = (int)((double)RandRange((int)-aVy, aHigh) / 1000.0 + (double)aBaseY);
		}
		else
		{
			aVx = aParent->vx * 0.5f;
			aVy = aParent->vy * 0.5f;
			x = (int)aParent->x;
			y = (int)aParent->y;
		}
		aStartR = JitterColour(e->startR, e->variance);
		aStartG = JitterColour(e->startG, e->variance);
		aStartB = JitterColour(e->startB, e->variance);
		anEndR = JitterColour(e->endR, e->variance);
		anEndG = JitterColour(e->endG, e->variance);
		anEndB = JitterColour(e->endB, e->variance);
	}
	ParticleSystem_New();
	ParticleSystem_Add(gLevels.current->systems.current, (float)x, (float)y, aVx, aVy, 0, aLast, 0, aStartR, aStartG,
		aStartB, anEndR, anEndG, anEndB, e->startAlpha, e->endAlpha, e->time, 0.999f, 0.0f, e->drawKind);
	gLevels.current->systems.current->particles.current->anim = e->anim;
	ParticleSystem* aSystem = gLevels.current->systems.current;
	aSystem->particles.current->depth = 0;
}

} // namespace wb
