// Sexy::PIEffect: PopCap's particle effect player (the .ppf file of the "PPF1" format that PopCap's particle editor
// writes). This file holds the loaded definition (PIEffectDef.h), the effect's own random generator and the loader.
//
// Only the new file format is read (the file's name has ".ppf" in it): the original also reads an older format with
// 4-byte booleans, class-name tags and images embedded in the file, which no file of Bejeweled 3 uses.
//
// Not translated (the update and draw of the original, listed with their addresses in the report of the pass that
// made this file): Update 0x573290 (bejeweled3), UpdateParticleDef 0x5707d0 (bejeweled3), UpdateParticleGroup 0x571ad0 (bejeweled3), CalcParticleTransform
// 0x56fe50 (bejeweled3), GetGeomPos 0x569f80 (bejeweled3), GetEmitterPos 0x56fda0 (bejeweled3), DrawParticleGroup 0x574060 (bejeweled3), DrawLayer 0x574700 (bejeweled3), Draw 0x575650 (bejeweled3),
// ResetAnim 0x56f730 (bejeweled3), Clear 0x56fab0 (bejeweled3) and the saved state (LoadState 0x56e4f0 (bejeweled3), SaveState 0x56eed0 (bejeweled3)).
#pragma once

#include "MTRand.h"
#include "PIEffectDef.h"

#include <cstddef>
#include <string>

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

	// 0x5679b0 (bejeweled3): finds the image of a texture. The original asks the app's shared image store; here a
	// game (or a test that loads without images) says what to do. Returns false when the image is missing
	virtual bool LoadTextureImage(PITexture& theTexture);

	// How many bytes of the last Load's file were used (the whole file when it is read exactly)
	size_t mBytesRead;
	// How many bytes of a cached state the file ends with (0 when it has none)
	int mCachedStateSize;

	// What PopAnim calls (not translated yet, empty here: games/bejeweled3/docs/NOTES.md, "Particles")
	// 0x573290 (bejeweled3): one step of the effect
	void Update();
	// 0x575650 (bejeweled3): draws the effect
	void Draw(Graphics* g);
	// 0x56f730 (bejeweled3): back to the start
	void ResetAnim();
	// 0x575790 (bejeweled3): whether the effect has particles or emitters left
	bool IsActive();

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
