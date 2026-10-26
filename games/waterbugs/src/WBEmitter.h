#ifndef __WATERBUGS_WBEMITTER_H__
#define __WATERBUGS_WBEMITTER_H__

// Particle emitters and particles (0x40ad60..0x40c2c0). An emitter is an object in another object's emitter list
// (an enemy's, from the game file's emitter tags); every few updates it makes a particle system from a static pool,
// appends it to the level's list, and gives it one particle from another static pool. The pools are used round
// and round without checking that a slot is free, and their nodes are unlinked, never deleted.

#include "WBObj.h"

namespace wb
{

struct ParticleSystem;

// 0xe4 bytes (0x407ce0 allocates one; 0x40b660 sets it up)
struct Emitter : Obj
{
	// kind +0x08: 0 angular (turns with the parent's animation), 1 scattered, 2 trailing (the game file makes 0
	// and 2); oldX/oldY +0x1c: the offset from the parent's position; vx/vy: the particles' speed (per update);
	// anim, frameCount, frameTime: the particles' animation
	int frequency;             // +0xb0  updates between particles
	int rotation;              // +0xb4  angular: degrees over the parent's whole animation
	Obj* parent;               // +0xb8
	int startAlpha;            // +0xbc
	int endAlpha;              // +0xc0
	int time;                  // +0xc4  the particles' lifetime
	int startR;                // +0xc8
	int startG;                // +0xcc
	int startB;                // +0xd0
	int endR;                  // +0xd4
	int endG;                  // +0xd8
	int endB;                  // +0xdc
	int variance;              // +0xe0  each colour moves by up to this much
};
static_assert(sizeof(Emitter) == 0xe4, "Emitter");

// 0xe0 bytes, from the pool at 0x478628
struct Particle : Obj
{
	float drag;                // +0xb0  the speed is multiplied by it each update
	float gravity;             // +0xb4
	int firstImage;            // +0xb8  art.bbk's index of the first frame, when there is no animation
	int startAlpha;            // +0xbc  colour and alpha go from start to end over the lifetime
	int endAlpha;              // +0xc0
	int startR;                // +0xc4
	int endR;                  // +0xc8
	int startG;                // +0xcc
	int endG;                  // +0xd0
	int startB;                // +0xd4
	int endB;                  // +0xd8
	ParticleSystem* owner;     // +0xdc
};
static_assert(sizeof(Particle) == 0xe0, "Particle");

// 0xbc bytes, from the pool at 0x62de20; its position and speed follow its first particle
struct ParticleSystem : Obj
{
	List<Particle> particles;  // +0xb0
};
static_assert(sizeof(ParticleSystem) == 0xbc, "ParticleSystem");

// The CRT's construction of the pools: 0x40c2c0 on each system, the base on each particle
void Particles_Init();

int Particle_Update(Particle* theParticle);                      // 0x40ad60: 0 when done
void Particle_Draw(Particle* theParticle);                       // 0x40ae40
// 0x40b5d0: Particle_Update with drag and gravity, keeping the system with the particle (its result is the
// update's, left in eax)
int Particle_Follow(Particle* theParticle);

void Emitter_Append(List<Obj>* theList);                         // 0x407ce0: a new one, current
void Emitter_Setup(Emitter* theEmitter, Obj* theParent, int theX, int theY, float theXSpeed, float theYSpeed,
	int theFrequency, int theRotation, const char* theAnim, int theKind, float theStartAlpha, float theEndAlpha,
	float theTime);                                              // 0x40afa0
void Emitter_CopyPrev(Emitter* theEmitter, int theAngle);       // 0x40b0c0: the previous one's, turned
void Emitter_Copy(Emitter* theEmitter, const Emitter* theFrom);  // 0x40b210
// 0x40b660: an emitter tag of the game file, on the level's current enemy (else the game file's)
void Emitter_FromXml(Emitter* theEmitter, const XmlTag* theTag);

void ParticleSystem_New();                                       // 0x40b2d0: appended to the level's, current
void ParticleSystem_Add(ParticleSystem* theSystem, float theX, float theY, float theVx, float theVy,
	int theFirstImage, int theLastImage, int theFrameTime, int theStartR, int theStartG, int theStartB, int theEndR,
	int theEndG, int theEndB, int theStartAlpha, int theEndAlpha, int theLifetime, float theDrag, float theGravity,
	int theDrawKind);                                            // 0x40b360
void ParticleSystem_Draw(ParticleSystem* theSystem);             // 0x40b5b0: its first particle
int ParticleSystem_Update(ParticleSystem* theSystem);            // 0x40c280: 0 when it has no particles left
ParticleSystem* ParticleSystem_Init(ParticleSystem* theSystem);  // 0x40c2c0

} // namespace wb

#endif
