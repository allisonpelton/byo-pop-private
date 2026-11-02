// The animated visualizers the VizManager (VizManager.h) shows with Direct3D and the options' "visualizers" box: each is a
// Visualizer (vtable 0x7d804c) with its own images, made by the maker the VizManager's constructor registers by name
// (0x7597cc, FUN_0075978d). Ported here: RedBars, WindCloud, ContainmentField, UVWarp, Raptis_Pulse, BlobDrift and
// ParametricFractal; the other 3D ones are not ported yet and MakeVisualizer3D answers null for them, so the VizManager
// shows Viz2D in their place.
//
// Random draws: the original's visualizers draw from the game's Rand (0x45c110 and its Rand(n) 0x45c120 and Rand(float)
// 0x45c130) in BlobDrift and Raptis_Pulse, and from the C runtime's rand (_rand, CrtRand here) in WindCloud and
// ContainmentField. Each is kept in its original order and place; the listing is in each class's comment.
#pragma once

#include "VizManager.h"

#include <vector>

namespace Sexy {

class Image;

// 0x757e6b and its fellows: the maker of the visualizer called theName, null when it is not ported (the caller then
// uses Viz2D)
Visualizer* MakeVisualizer3D(const char* theName, WinTwistApp* theApp);

// Sexy::RedBars (vtable 0x7d7e44, maker 0x757e6b, constructor 0x67a186): IMAGE_FIRESTAR scrolling over the whole screen as
// one quad, by the visualizer's own update count. No random draws.
class RedBars : public Visualizer {
public:
	Image* mImage = nullptr;  // +0x24
	explicit RedBars(WinTwistApp* theApp);  // 0x67a186
	~RedBars() override;                    // 0x67a351
	void Draw(Graphics* g) override;        // 0x67a206
};

// Sexy::BlobDrift (vtable 0x7d7710, maker 0x757b56, constructor 0x6700aa): 250 blobs pushing each other about, the
// screen's background under them. Random draws, all of the game's Rand: Init 0x66fbb7 (Rand(width), Rand(height),
// Rand(2.0f), Rand(2.0f), Rand(7), Rand(0.1f) for each blob) and Update 0x66fd2b (Rand(2.0f), Rand(0.1f) for each blob
// that falls off the top).
class BlobDrift : public Visualizer {
public:
	struct Blob {  // 0x28 bytes
		float mX = 0.0f;       // +0x04
		float mY = 0.0f;       // +0x08
		float mVX = 0.0f;      // +0x0c
		float mVY = 0.0f;      // +0x10
		Color mColor;          // +0x14
		float mSize = 0.0f;    // +0x24
	};
	float mFactor = 1.0f;      // +0x24
	Image* mBlobImage = nullptr;  // +0x28
	Image* mBackground = nullptr; // +0x2c
	Blob mBlobs[250];             // +0x30

	explicit BlobDrift(WinTwistApp* theApp);  // 0x6700aa
	~BlobDrift() override;                    // 0x670134
	void Init() override;                     // 0x66fbb7
	void Update() override;                   // 0x66fd2b
	void Draw(Graphics* g) override;          // 0x66ffae
};

// Sexy::UVWarp (vtable 0x7d8014, maker 0x757caf, constructor 0x67f36f): IMAGE_CRAZY_BG on a 16 by 16 mesh whose texture
// coordinates swim, drawn again mirrored and added. No random draws.
class UVWarp : public Visualizer {
public:
	int mCount = 0;           // +0x24
	Image* mImage = nullptr;  // +0x28
	explicit UVWarp(WinTwistApp* theApp);  // 0x67f36f
	~UVWarp() override;                    // 0x67f9ae
	void Update() override { mCount++; }   // 0x67f3c9
	void Draw(Graphics* g) override;       // 0x67f5bd
};

// Sexy::ContainmentField (vtable 0x7d77a0, maker 0x757cdf, constructor 0x670ab3): IMAGE_CRAZY_BG2 scrolling, darkened in
// squares by eight spreading rings. Random draws, all of the C runtime's rand: 0x670a64 (three each time a ring starts
// again; eight from Init 0x670b0f, then more from the 1000 updates it runs, and from Update 0x670b3a).
class ContainmentField : public Visualizer {
public:
	struct Ring {  // 12 bytes
		int mX = 0;    // +0x28
		int mY = 0;    // +0x2c
		int mAge = 0;  // +0x30: negative until it starts, and again when it is over 400
	};
	int mCount = 0;           // +0x24
	Ring mRings[8];           // +0x28
	Image* mImage = nullptr;  // +0x88
	explicit ContainmentField(WinTwistApp* theApp);  // 0x670ab3
	~ContainmentField() override;                    // 0x670f11
	void Init() override;                            // 0x670b0f
	void Update() override;                          // 0x670b3a
	void Draw(Graphics* g) override;                 // 0x670b60
	void StartRing(Ring& theRing);                   // 0x670a64
};

// Sexy::ParametricFractal (vtable 0x7d7bec, maker 0x757d42, constructor 0x676494): a fractal of IMAGE_FRACTAL_BLUR
// sprites over IMAGE_FRACTAL_BG, branching to depth by the scale. No random draws.
class ParametricFractal : public Visualizer {
public:
	int mCount = 0;                // +0x28
	Image* mBlur = nullptr;        // +0x2c
	Image* mBackground = nullptr;  // +0x30
	explicit ParametricFractal(WinTwistApp* theApp);  // 0x676494
	~ParametricFractal() override;                    // 0x676b2a
	void Update() override { mCount++; }              // 0x676500
	void Draw(Graphics* g) override;                  // 0x676a54
	void DrawBranch(Graphics* g, float theX, float theY, float theAngle, float theScale, float theDepth);  // 0x67665c
	void SetBranchColor(Graphics* g, float theDepth);  // 0x676504
};

// Sexy::WindCloud (vtable 0x7d8564, maker 0x757b89, constructor 0x6938dc): 500 sprites of IMAGE_AIR_STREAM blown about by
// two vortices over IMAGE_TADPOLE_BG on a swimming mesh. Random draws, all of the C runtime's rand: Init 0x693afb
// (five for each sprite from 0x6928e8, then three for each vortex) and Update 0x69335e (one for each vortex when the
// count's last two digits are 49 or 99, and two for each of the 500 sprites near a vortex, once or twice for each).
class WindCloud : public Visualizer {
public:
	struct Vortex {  // 12 bytes
		float mX = 0.0f;      // +0
		float mY = 0.0f;      // +4
		float mAngle = 0.0f;  // +8
	};
	struct Sprite {  // 0x14 bytes
		float mX = 0.0f, mY = 0.0f, mVX = 0.0f, mVY = 0.0f, mUnknown = 0.0f;
	};
	int mCount = 0;                    // +0x24
	Sprite mSprites[500];              // +0x28 (the original has a pointer to each)
	std::vector<Vortex*> mVortices;    // +0x7f8 (a std::list)
	Image* mSprite = nullptr;          // +0x804
	Image* mBackground = nullptr;      // +0x808
	explicit WindCloud(WinTwistApp* theApp);  // 0x6938dc
	~WindCloud() override;                    // 0x6939c1
	void Init() override;                     // 0x693afb
	void Update() override;                   // 0x69335e
	void Draw(Graphics* g) override;          // 0x69307b
};

// Sexy::Raptis_Pulse (vtable 0x7d7dd8, maker 0x757e05, constructor 0x67973d): pulsing rings of IMAGE_PULSERING drawn as
// triangle strips, added on black. Random draws, all of the game's Rand, in Update 0x679836: Rand(50), Rand(30), Rand(60)
// twice, Rand(2) twice, Rand(255) three times and Rand(20) when the countdown ends; Rand(360) for a new ring; and
// Rand(1000) for each pulse. Init (0x6797d3) runs 3000 updates.
class Raptis_Pulse : public Visualizer {
public:
	struct Pulse {  // 0x34 bytes
		int mX = 0;            // +0x00
		int mY = 0;            // +0x04
		float mScale = 0.0f;   // +0x08
		Color mColor;          // +0x0c (its alpha is +0x18)
		float mAngle1 = 1.0f;  // +0x1c
		float mAngle2 = 2.0f;  // +0x20
		bool mFlipped = false; // +0x24
		float mDegrees = 0.0f; // +0x28
		float mSize = 0.0f;    // +0x2c
		float mMaxSize = 64.0f; // +0x30
	};
	struct Ring {  // 12 bytes
		float mA = 0.0f;      // +0
		float mB = 2.0f;      // +4
		float mC = 0.0f;      // +8
	};
	Image* mPulseImage = nullptr;     // +0x24
	Image* mStarry = nullptr;         // +0x28
	std::vector<Pulse*> mPulses;      // +0x2c (a List)
	int mCountdown = 0;               // +0x48
	std::vector<Ring*> mRings;        // +0x4c (a List)
	Ring* mLatestRing = nullptr;      // +0x68
	float mPhase = 0.0f;              // +0x6c
	float mPhaseStep = 0.0313f;       // +0x70 (0x3d03126f)
	float mUnknown74 = 0.0f;          // +0x74
	float mUnknown78 = 0.0f;          // +0x78
	float mAngle = 0.0f;              // +0x7c
	explicit Raptis_Pulse(WinTwistApp* theApp);  // 0x67973d
	~Raptis_Pulse() override;                    // 0x67a011
	void Init() override;                        // 0x6797d3
	void Update() override;                      // 0x679836
	void Draw(Graphics* g) override;             // 0x679d51
	void DrawPulse(Graphics* g, float theX, float theY, float theRotation, float theScale);  // 0x679bb5
};

}  // namespace Sexy
