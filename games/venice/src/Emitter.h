// The emitter: a scene object that makes particles, a new child now and then, each with a picture, a lifetime and a path
// that the emitter's stages (one for each kind of particle in the order it changes) give.

#ifndef VENICE_EMITTER_H
#define VENICE_EMITTER_H

#include "CursorList.h"
#include "SceneObject.h"

// 100 bytes in the original; one stage of a particle's life
class EmitterStage
{
public:
	EmitterStage* mNext = nullptr;
	EmitterStage* mPrev = nullptr;
	float mMinDuration = 0; // in milliseconds
	float mMaxDuration = 0;
	float mMinStartAlpha = 0;
	float mMaxStartAlpha = 0;
	float mMinEndAlpha = 0;
	float mMaxEndAlpha = 0;
	float mMinStartScale = 0;
	float mMaxStartScale = 0;
	float mMinEndScale = 0;
	float mMaxEndScale = 0;
	float mMinStartRotation = 0;
	float mMaxStartRotation = 0;
	float mMinEndRotation = 0;
	float mMaxEndRotation = 0;
	float mMinXSpeed = 0;
	float mMaxXSpeed = 0;
	float mMinYSpeed = 0;
	float mMaxYSpeed = 0;
	float mDecay = 0; // what the speeds are multiplied by each update
	float mGravity = 0; // what the y speed grows by each update
	int mFilterType = -1; // the type of the filter on all attributes that comes with the first stage (no data file sets it)
	ImageRecord* mImage = nullptr;
};

// 0x174 bytes in the original
class Emitter : public SceneObject
{
public:
	float mFrequency = 0; // 0x148: the updates between particles (about that often at random when mRandom is set)
	float mRandom = 0; // 0x14c
	float mMinX = 0; // 0x150: where a particle starts, relative to the emitter
	float mMaxX = 0; // 0x154
	float mMinY = 0; // 0x158
	float mMaxY = 0; // 0x15c
	int mParticlesPerEmit = 1; // 0x160
	int mKeepLocal = 0; // 0x164: the particle stays a child of the emitter (it goes to the sparkle layer otherwise)
	CursorList<EmitterStage> mEmitterStages; // 0x168

	Emitter(); // 0x421ca0
	~Emitter() override; // 0x421d70
	void Load(const char* theText, SceneTag* theTag) override; // 0x421ea0
	bool Update() override; // 0x422250

	EmitterStage* AddEmitterStage(); // 0x421e10
	void LoadStageTag(const SceneTag* theTag); // 0x422080
};

#endif
