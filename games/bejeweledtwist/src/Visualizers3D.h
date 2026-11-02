// The animated visualizers the VizManager (VizManager.h) shows with Direct3D and the options' "visualizers" box: each is a
// Visualizer (vtable 0x7d804c) with its own images, made by the maker the VizManager's constructor registers by name
// (0x7597cc, FUN_0075978d). Ported here: RedBars, WindCloud, ContainmentField, UVWarp, Raptis_Pulse, BlobDrift and
// ParametricFractal (Visualizers3D.cpp), and FluffyStars, PlasmaClouds, SpaceThreads, VizWarpBars and VizKaleidoCat
// (Visualizers3D2.cpp); the other 3D ones are not ported yet and MakeVisualizer3D answers null for them, so the
// VizManager shows Viz2D in their place.
//
// Random draws: the original's visualizers draw from the game's Rand (0x45c110 and its Rand(n) 0x45c120 and Rand(float)
// 0x45c130) in BlobDrift and Raptis_Pulse, and from the C runtime's rand (_rand, CrtRand here) in WindCloud and
// ContainmentField. Each is kept in its original order and place; the listing is in each class's comment.
#pragma once

#include "Common.h"
#include "TriVertex.h"
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


// 0x75727e (FluffyStars, PlasmaClouds and 0x69041e): theImage drawn as a theGrid by theGrid mesh (at most 10),
// its size scaled by theScaleX and theScaleY, turned by theRot (radians) about its middle at theX, theY; every vertex is
// g's colour, and the linear filter is set on g.
void DrawImageMeshRotated(Graphics* g, Image* theImage, float theX, float theY, double theRot, float theScaleX,
	float theScaleY, int theGrid);

// Sexy::FluffyStars (vtable 0x7d7900, constructor 0x6722d0): up to 50 soft IMAGE_FLUFFYSTARS growing and fading on black.
// Random draws, all of the game's Rand (RandSigned, 0x756e97), in Update 0x67237d, when the timer has run out and a
// place is free: angle, x, y, and for the blue kind a fourth (its growth).
class FluffyStars : public Visualizer {
public:
	struct Star {  // 0x2c bytes
		float mX = 0.0f;       // +0x00
		float mY = 0.0f;       // +0x04
		float mScale = 0.0f;   // +0x08
		float mGrowth = 0.0f;  // +0x0c
		float mAngle = 0.0f;   // +0x10
		float mAlpha = 0.0f;   // +0x14
		Color mColor;          // +0x18
		float mMult = 0.0f;    // +0x28
	};
	Star* mStars[50];          // +0x24 (an array of mCount pointers)
	float mTimer = 0.0f;       // +0x28
	Image* mImage = nullptr;   // +0x2c
	int mCount = 0x32;         // +0x30
	explicit FluffyStars(WinTwistApp* theApp);  // 0x6722d0
	~FluffyStars() override;                    // 0x6727c2
	void Init() override;                       // 0x672358: 1000 updates
	void Update() override;                     // 0x67237d
	void Draw(Graphics* g) override;            // 0x67262a
};

// Sexy::PlasmaClouds (vtable 0x7d7c7c, constructor 0x67759d): twenty clouds of IMAGE_CLOUD3 drifting out from the middle
// on black, coloured by where they are round it. Random draws: all of the game's Rand(), in Update 0x677677 where a cloud
// is made (nine for each, until there are twenty); Init (0x676c1d) runs 30 updates.
class PlasmaClouds : public Visualizer {
public:
	struct Cloud {  // 0x40 bytes
		Image* mImage = nullptr;  // +0x00
		float mX = 0.0f;          // +0x04
		float mY = 0.0f;          // +0x08
		float mVX = 0.0f;         // +0x0c
		float mVY = 0.0f;         // +0x10
		float mScaleX = 0.0f;     // +0x14
		float mScaleY = 0.0f;     // +0x18
		Color mColor;             // +0x1c (its alpha is +0x28)
		float mAlpha = 0.0f;      // +0x2c
		int mFade = 0;            // +0x30: updates it still fades in
		bool mDone = false;       // +0x34
		float mAngle = 0.0f;      // +0x38
		float mSpin = 0.0f;       // +0x3c
	};
	Image* mCloud1 = nullptr;     // +0x24
	Image* mCloud2 = nullptr;     // +0x28
	Image* mCloud3 = nullptr;     // +0x2c
	std::vector<Cloud*> mClouds;  // +0x30
	float mAngle = 0.0f;          // +0x40: where round the middle the next cloud starts
	int mNextImage = 0;           // +0x44
	explicit PlasmaClouds(WinTwistApp* theApp);  // 0x67759d
	~PlasmaClouds() override;                    // 0x677245
	void Init() override;                        // 0x676c1d
	void Update() override;                      // 0x677677
	void Draw(Graphics* g) override;             // 0x676e10
};

// Sexy::SpaceThreads (vtable 0x7d7f58, constructor 0x67c1a2): twenty threads of 45 points in perspective, drawn as
// quads of IMAGE_SPACE_THREAD over IMAGE_SPACETHREADS_BG. Random draws: Init 0x67bb86, Rand() once for the count and four
// RandSigned for each of the 900 points; none in Update or Draw. Init also runs 1200 updates.
class SpaceThreads : public Visualizer {
public:
	struct Point {  // 0x14 bytes
		float mA = 0.0f;  // +0x00 (the original's +0x24 on)
		float mB = 0.0f;  // +0x04
		float mC = 0.0f;  // +0x08: its depth
		float mD = 0.0f;  // +0x0c
		float mE = 0.0f;  // +0x10
	};
	Point mPoints[20][45];         // +0x24
	int mTriCount = 0;             // +0x2e974
	int mCount = 0;                // +0x2e978
	int mCursor = 0;               // +0x2e97c
	Image* mThread = nullptr;      // +0x2e980
	Image* mBackground = nullptr;  // +0x2e984
	TriVertex mTriangles[1800][3];  // +0x4674 (the original's vertices are 32 bytes)
	explicit SpaceThreads(WinTwistApp* theApp);  // 0x67c1a2
	~SpaceThreads() override;                    // 0x67c4c1
	void Init() override;                        // 0x67bb86
	void Update() override;                      // 0x67bc06
	void Draw(Graphics* g) override;             // 0x67c242
	// 0x67be27: the quad from point thePoint to the next of thread theThread, in theColor
	void AddQuad(const Color& theColor, int theThread, int thePoint);
};

// Sexy::VizWarpBars (vtable 0x7d83c4, constructor 0x68f2b0): 400 bars of IMAGE_COREWARP_BLUR flying out of the middle,
// over IMAGE_COREWARP_BG (IMAGE_VIRTUTORIAL while the profile's flag 14 is not set). Random draws, all of the C runtime's
// rand: four or more (until the bar is 80 or more from the middle) each time a bar starts, 0x68ed45: Init (0x68f458, 400
// bars) and Update 0x68eeb1 (a bar that leaves the screen).
class VizWarpBars : public Visualizer {
public:
	struct Bar {  // 0x10 bytes
		float mX = 0.0f;      // +0x00
		float mY = 0.0f;      // +0x04
		float mZ = 0.0f;      // +0x08
		float mSpeed = 0.0f;  // +0x0c
		void Start(WinTwistApp* theApp);  // 0x68ed45
	};
	int mCount = 0;                 // +0x24
	std::vector<Bar*> mBars;        // +0x28 (a std::list)
	Image* mBlur = nullptr;         // +0x34
	Image* mBackground = nullptr;   // +0x38
	bool mTutorial = false;         // +0x3c
	explicit VizWarpBars(WinTwistApp* theApp);  // 0x68f2b0
	~VizWarpBars() override;                    // 0x68f352
	void Init() override;                       // 0x68f458
	void Update() override;                     // 0x68eeb1
	void Draw(Graphics* g) override;            // 0x68f004
};

// Sexy::VizKaleidoCat (vtable 0x7d8240, constructor 0x68aeee, maker as the others): little IMAGE_SNACKERS stamped by
// branching "cats" that walk programs of steps and turns, painted into an offscreen image kept from update to update,
// and shown round the screen's middle as sixteen turning wedges of it. It draws from no game random generator: its
// own (0x689985, the C runtime's rand's formula on a seed shared by all of them: 0x12345678, and the time once an
// instance has run 1000 updates).
class VizKaleidoCat : public Visualizer {
public:
	struct Node {  // 0x70 bytes: a cat, in a ring with the others (the Cats' own, mHead, is the ring's start)
		Node* mNext = this;       // +0x00
		Node* mPrev = this;       // +0x04
		float mX = 0.0f;          // +0x08
		float mY = 0.0f;          // +0x0c
		float mDX = 0.0f;         // +0x10: its direction
		float mDY = 0.0f;         // +0x14
		float mScale = 0.0f;      // +0x18
		int mColor = 0;           // +0x1c: the hue
		unsigned char mProgram[0x40] = {};  // +0x20: 1 a step, 2 and 3 a turn, 0 the end
		float mTurn = 0.0f;       // +0x60
		float mStep = 0.0f;       // +0x64
		int mPC = 0;              // +0x68
		float mTimer = 0.0f;      // +0x6c
	};
	struct Particle {  // 0x20 bytes: a stamp on its way
		float mX = 0.0f;       // +0x00
		float mY = 0.0f;       // +0x04
		float mSize = 0.0f;    // +0x08
		Color mColor;          // +0x0c
		float mAge = 0.0f;     // +0x1c
	};
	struct Cats {  // 0x88 bytes (0x68aed8)
		std::vector<Particle> mParticles;  // +0x00
		Node mHead;                        // +0x10
		int mWidth = 0;                    // +0x80
		int mHeight = 0;                   // +0x84
		~Cats();                           // 0x68ab53
		void SetSize(int theWidth, int theHeight);  // 0x68a96f
		void Spawn();                      // 0x68a8f6 (false)
		void StepAll();                    // 0x68aa74
	};
	Cats* mCats = nullptr;          // +0x24
	bool mSeeded = false;           // +0x28
	DDImage* mCanvas = nullptr;     // +0x2c: where Draw puts the stamps so far and the stamps on their way
	DDImage* mKept = nullptr;       // +0x30: the stamps that have landed
	Image* mSnackers = nullptr;     // +0x34
	int mCount = 0;                 // +0x38
	int mCountBase = 0;             // +0x3c
	explicit VizKaleidoCat(WinTwistApp* theApp);  // 0x68aeee
	~VizKaleidoCat() override;                    // 0x68ac1b
	void Update() override;                       // 0x68aff4
	void Draw(Graphics* g) override;              // 0x689ed4
	void Slot7() override { mCountBase = mCount; }  // 0x689cb9
	// 0x689b0d and 0x689be3: the image made when it is missing, and made the target
	bool EnsureImage(DDImage*& theImage);
};

}  // namespace Sexy
