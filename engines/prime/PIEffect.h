// Sexy::PIEffect: PopCap's particle effect player (the .ppf file of the "PPF1" format that PopCap's particle editor
// writes). This file holds the loaded definition (PIEffectDef.h), the effect's own random generator and the loader.
//
// Only the new file format is read (the file's name has ".ppf" in it): the original also reads an older format with
// 4-byte booleans, class-name tags and images embedded in the file, which no file of Bejeweled 3 uses.
//
// What a running effect keeps is in PIEffectRuntime.h. Update and its helpers are in PIEffectUpdate.cpp, the drawing in
// PIEffectDraw.cpp (which says what it leaves out).
//
// Not translated: SaveState 0x56eed0 (bejeweled3) (the file's cached starting state is read by LoadState; nothing in
// the game saves a state).
#pragma once

#include "MTRand.h"
#include "PIEffectDef.h"
#include "PIEffectRuntime.h"
#include "SexyMatrix.h"

#include <cstddef>
#include <string>
#include <vector>

namespace Sexy {

class Graphics;

class PIEffect {
public:
	PIEffectDef mDef;                    // +0xaf0 (a reference-counted pointer in the original, shared by duplicates)
	std::string mHeaderText;             // +0xa70: the text the file's header carries
	std::string mError;                  // what went wrong when Load returned false
	int mWidth;                          // +0xaa8
	int mHeight;                         // +0xaac
	Color mBackgroundColor;              // +0xab0
	int mFramerate;                      // +0xac0
	int mFirstFrameNum;                  // +0xac4
	int mLastFrameNum;                   // +0xac8
	int mVersion;                        // +0x3c: the file's version
	MTRand mRand;                        // +0x78

	PIEffect();
	virtual ~PIEffect();

	// 0x56aac0 (bejeweled3): reads a .ppf file's bytes. The name decides the format: only names with ".ppf" in them
	// are read. Returns false, with mError set, when the file is not valid or an image cannot be found
	bool Load(const unsigned char* theData, size_t theSize, const std::string& theFileName);

	// 0x56aac0 (bejeweled3): reads a .ppf file by its path through the game's files (PakInterface); mError says what
	// went wrong (empty when it loaded)
	void LoadEffect(const std::string& theFileName);

	// 0x5679b0 (bejeweled3), slot 0: finds the image a texture names: the app's shared image store, looked up by the
	// folder of the effect's file and the name. A game may override it to find the images some other way. Without an app
	// (a test that loads without images) nothing is found and Load goes on without the image
	virtual SharedImageRef LoadTextureImage(const std::string& theImageName);
	// 0x5ad6c0 (bejeweled3), slot 1: called with each image Load has found; does nothing here
	virtual void TextureImageLoaded(Image* theImage);

	// +0x40: the name the effect was loaded from
	std::string mFileName;

	// How many bytes of the last Load's file were used (the whole file when it is read exactly)
	size_t mBytesRead;
	// How many bytes of a cached state the file ends with (0 when it has none)
	int mCachedStateSize;

	// What PopAnim calls
	// 0x573290 (bejeweled3): one step of the effect
	void Update();
	// 0x575650 (bejeweled3): draws the effect (PIEffectDraw.cpp)
	void Draw(Graphics* g);
	// 0x56f730 (bejeweled3): back to the start
	void ResetAnim();
	// 0x575790 (bejeweled3): whether the effect has particles or emitters left
	bool IsActive();
	// 0x56fab0 (bejeweled3): back to nothing loaded
	void Clear();

	// the run-time state (offsets in the exe's object)
	std::vector<PILayer*> mLayers;       // +0xaf4: one for each layer of the definition (made by Load)
	bool mLoaded;                        // +0xb28
	int mUpdateCount;                    // +0xb2c: how many Updates it has had
	float mFrameNum;                     // +0xb30: the time, in frames
	bool mFrameChanged;                  // +0xb34: the last Update moved the time past a whole frame
	int mParticleCount;                  // +0xb64: the particles alive after the last Update
	int mEmitterCount;                   // +0xb68: the free emitters alive after the last Update
	int mDrawArea;                       // +0xb6c: what the last Draw covered, in pixels (a statistic)
	float mAnimSpeed;                    // +0xb70: 1.0 is the file's speed
	Color mColor;                        // +0xb74: multiplies every particle's color
	bool mDebugDraw;                     // +0xb84: draw the deflectors, forces and outlines (the editor's view)
	bool mUnreadFlag;                       // +0xb85: set by the constructor; nothing translated reads it
	bool mLoop;                          // +0xb86: the effect starts over instead of ending at its last frame
	std::vector<int> mSeeds;             // +0xb88: seeds the first Update picks among (none in the game's files)
	bool mNeedsSeed;                     // +0xba0: the next Update seeds mRand
	SexyMatrix3 mDrawTransform;          // +0xba4: put on every particle (PopAnim's matrix for the effect)
	SexyMatrix3 mDrawTransform2;         // +0xbc8: the second one, applied to positions only (GetEmitterPos)
	std::vector<unsigned char> mCachedState;   // the bytes the file ends with (LoadState)

	// 0x56e4f0 (bejeweled3): puts the effect into the state its file ends with (false when that state does not read
	// right to its end); SaveState (0x56eed0 (bejeweled3)) is not translated
	bool LoadCachedState();

	// port: makes the layers and emitter instances from the definition (the exe makes them while it reads the file)
	void CreateLayers();

	// 0x56fda0 (bejeweled3): where the emitter is at the current time; with theUseInstance the instance's own
	// transform and offset are put on
	PIPoint GetEmitterPos(PIEmitterInstance* theInstance, bool theUseInstance);
	// port: the turn sign the shapes of GetGeomPos use
	float GetShapeTurnSign(PIEmitterInstanceDef* theDef);
	// 0x569f80 (bejeweled3): where a new particle starts (the shape of the instance); also adds to *theAngle the
	// direction it leaves in, and sets *theOutside when the shape's mask picture says the point is not to be used
	PIPoint GetGeomPos(PIEmitterInstance* theInstance, PIParticleInstance* theParticle, float* theAngle, bool* theOutside);
	// 0x56fe50 (bejeweled3): the matrix a particle is drawn with
	void CalcParticleTransform(PILayer* theLayer, PIEmitterInstance* theInstance, PIEmitter* theEmitter,
	                           PIParticleDef* theParticleDef, PIParticleGroup* theGroup, PIParticleInstance* theParticle);
	// 0x5707d0 (bejeweled3): makes the particles one definition is due
	void UpdateParticleDef(PIEmitter* theEmitter, PIEmitterInstance* theInstance, PIParticleDef* theParticleDef,
	                       PIParticleDefInstance* theDefInstance, PIParticleGroup* theGroup,
	                       PIFreeEmitterInstance* theFreeEmitter);
	// 0x571ad0 (bejeweled3): moves, ages and ends the particles of a group
	void UpdateParticleGroup(PILayer* theLayer, PIEmitterInstance* theInstance, PIParticleGroup* theGroup);

	// 0x574700 (bejeweled3): draws one layer's particles
	void DrawLayer(Graphics* g, PILayer* theLayer);
	// 0x574060 (bejeweled3): draws the particles of one group, in the first or the second pass
	void DrawParticleGroup(Graphics* g, PILayer* theLayer, PIEmitterInstance* theInstance, PIParticleGroup* theGroup,
	                       bool theFirstPass);

	// 0x567d00 (bejeweled3): -1 to 1
	float GetRandFloat();
	// 0x567d40 (bejeweled3): 0 to 1
	float GetRandFloatU();
	// 0x567d70 (bejeweled3): 1 or -1
	float GetRandSign();
	// 0x567d90 (bejeweled3): the product of two GetRandFloat draws, so small values are the usual
	float GetVariationScalar();
};

}
