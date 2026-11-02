// The animated visualizers the VizManager (VizManager.h) shows with Direct3D and the options' "visualizers" box: each is a
// Visualizer (vtable 0x7d804c) with its own images, made by the maker the VizManager's constructor registers by name
// (0x7597cc, FUN_0075978d). Ported here: RedBars, WindCloud, ContainmentField, UVWarp, Raptis_Pulse, BlobDrift and
// ParametricFractal (Visualizers3D.cpp), and FluffyStars, PlasmaClouds, SpaceThreads, VizWarpBars and VizKaleidoCat
// (Visualizers3D2.cpp), and VizRainbowGlyphs, SubAtomic, Cubes and RainDrops (Visualizers3D3.cpp), and Gears, Slipstream (Visualizers3D4.cpp), GravityField, VizBuckyShards and VizMoleculeRedux (Visualizers3D5.cpp); the other 3D ones are not ported yet and MakeVisualizer3D answers null for them, so the
// VizManager shows Viz2D in their place.
//
// Random draws: the original's visualizers draw from the game's Rand (0x45c110 and its Rand(n) 0x45c120 and Rand(float)
// 0x45c130) in BlobDrift and Raptis_Pulse, and from the C runtime's rand (_rand, CrtRand here) in WindCloud and
// ContainmentField. Each is kept in its original order and place; the listing is in each class's comment.
#pragma once

#include "Common.h"
#include "SexyMatrix.h"
#include "TriVertex.h"
#include "VizManager.h"

#include <vector>

namespace Sexy {

class Image;

// 0x757e6b and its fellows: the maker of the visualizer called theName, null when it is not ported (the caller then
// uses Viz2D)
Visualizer* MakeVisualizer3D(const char* theName, WinTwistApp* theApp);

// 0x44fc20: a Transform that takes its matrix as it is
class MatrixTransform : public Transform {
public:
	void SetMatrix(const SexyMatrix3& theMatrix)
	{
		mMatrix = theMatrix;
		mNeedCalcMatrix = false;
		mComplex = true;
	}
};

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

// Sexy::VizRainbowGlyphs (vtable 0x7d8354, constructor 0x68e969): IMAGE_AZTEC_BG cut into 32 strips in a changing rainbow,
// under glyphs (cels of IMAGE_AZTEC_STRIP) that sixteen points circling the screen shed and that drift and fade out.
// Random draws, all of the game's Rand (RandSigned, 0x756e97), in Update 0x68ea4f, each fourth update (the update runs
// only when its count is even) for each of the 16 points: four in the glyph's constructor 0x68daa6 (the +8, +0xc, +0x10
// and +0x14 fields), then one for the size, one for each of x and y, and one for each of the two speeds, nine in all; and
// two more (the point's first place) in the update whose count is 0, which never comes. Init (0x68dad4) runs 60 updates.
class VizRainbowGlyphs : public Visualizer {
public:
	struct Glyph {  // 0x30 bytes
		float mX = 0.0f;       // +0x00
		float mY = 0.0f;       // +0x04
		float mVX = 0.0f;      // +0x08
		float mVY = 0.0f;      // +0x0c
		float mRand10 = 0.0f;  // +0x10: made by the constructor, never read
		float mRand14 = 0.0f;  // +0x14: made by the constructor, never read
		int mUnknown18 = 0;    // +0x18
		float mLife = 1.0f;    // +0x1c
		int mCel = 0;          // +0x20: the cel of IMAGE_AZTEC_STRIP
		int mAlpha = 0;        // +0x2c
		Glyph();               // 0x68daa6
	};
	Image* mStrip = nullptr;       // +0x24: IMAGE_AZTEC_STRIP
	Image* mBackground = nullptr;  // +0x28: IMAGE_AZTEC_BG
	std::vector<Glyph*> mColumns[16];  // +0x2c: the glyphs of each point (a std::vector, 0x10 bytes each)
	float mPrevX[16] = {};         // +0x12c: where the point was
	float mPrevY[16] = {};         // +0x16c
	int mCount = 0;                // +0x1ac
	int mCel = 0;                  // +0x1b0: the cel the next glyph takes (of 23)
	int mDrawWidth = 0;            // +0x1b4: the app's width and height when it was made
	int mDrawHeight = 0;           // +0x1b8
	explicit VizRainbowGlyphs(WinTwistApp* theApp);  // 0x68e969
	~VizRainbowGlyphs() override;                    // 0x68e661
	void Init() override;                            // 0x68dad4
	void Update() override;                          // 0x68ea4f
	void Draw(Graphics* g) override;                 // 0x68dd18
};

// Sexy::SubAtomic (vtable 0x7d7f9c, constructor 0x67df9f): up to 25 "electrons" (0x2c bytes) circling their centres and
// splitting in two, leaving trails (0x14 bytes) of IMAGE_SUBATOMIC_BLUR sprites over IMAGE_SUBATOMIC_BG on a swimming mesh.
// Random draws, constructor of an electron (0x67c581; the game's Rand: RandSigned for the first five, then two of the C
// runtime's rand): Init 0x67e0ca three electrons; Update 0x67e121 one when there are fewer than 15, and one (then one more
// _rand) each time an electron's countdown ends while there are fewer than 25. Init also runs 60 updates.
class SubAtomic : public Visualizer {
public:
	struct Electron {  // 0x2c bytes
		float mX = 0.0f;           // +0x00
		float mY = 0.0f;           // +0x04
		int mLife = 0;             // +0x08: updates it lives
		int mAge = 0;              // +0x0c
		float mCenterX = 0.0f;     // +0x10
		float mCenterY = 0.0f;     // +0x14
		float mSpeed = 0.0f;       // +0x18
		float mTurn = 1.0f;        // +0x1c
		float mUnknown20 = 1.0f;   // +0x20
		int mCountdown = 0;        // +0x24: updates to the next split
		float mSign = 0.0f;        // +0x28
		Electron();                // 0x67c581
	};
	struct Trail {  // 0x14 bytes
		float mX = 0.0f;   // +0x00
		float mY = 0.0f;   // +0x04
		float mVX = 0.0f;  // +0x08
		float mVY = 0.0f;  // +0x0c
		int mAge = 0;      // +0x10
	};
	int mCount = 0;                       // +0x24
	std::vector<Electron*> mElectrons;    // +0x28 (a std::vector)
	std::vector<Trail*> mTrails;          // +0x38
	Image* mSprite = nullptr;             // +0x48: IMAGE_SUBATOMIC_BLUR
	Image* mBackground = nullptr;         // +0x4c: IMAGE_SUBATOMIC_BG
	explicit SubAtomic(WinTwistApp* theApp);  // 0x67df9f
	~SubAtomic() override;                    // 0x67da17
	void Init() override;                     // 0x67e0ca
	void Update() override;                   // 0x67e121
	void Draw(Graphics* g) override;          // 0x67d45a
	void DrawMesh(Graphics* g);               // 0x67cc9d
};

// Sexy::Cubes (vtable 0x7d77ec, constructor 0x670fb6): 360 turning cubes of IMAGE_BORDERED_SQUARE added on black, each drawn
// as the three of its faces that turn to it, as a pair of triangles each. No random draws. Its Init (0x671017) does nothing.
class Cubes : public Visualizer {
public:
	int mCount = 0;            // +0x24
	float mUnknown28 = 0.0f;   // +0x28: grows by 0.035 an update, never read
	float mUnknown2C = 0.0f;   // +0x2c: grows by 0.025 an update, never read
	Image* mImage = nullptr;   // +0x30: IMAGE_BORDERED_SQUARE
	explicit Cubes(WinTwistApp* theApp);  // 0x670fb6
	~Cubes() override;                    // 0x671d55
	void Update() override;               // 0x671018
	void Draw(Graphics* g) override;      // 0x671b57
	// 0x671198: one cube of half-edge theSize at theX, theY, turned by the three angles, in the hue theHue
	void DrawCube(float theX, float theY, float theSize, float theRotA, float theRotB, float theRotC, float theHue);
};

// Sexy::RainDrops (vtable 0x7d7da4, constructor 0x67932c): 300 drops falling out of a swaying sky of IMAGE_RAINDROPS_BG as
// sprites of IMAGE_POINTY to POINTY3 by their distance, and splashing as IMAGE_SPLASH or rings of it. Random draws, all of
// the C runtime's rand, three or more (until it is 100 or more from the middle) each time a drop is placed, 0x6784d8: Init
// 0x679613 (150 drops, then 150 more with one more rand each, the timer) and Update 0x678b36 (a drop's second life).
class RainDrops : public Visualizer {
public:
	struct Drop {  // 0x14 bytes
		float mX = 0.0f;   // +0x00
		float mY = 0.0f;   // +0x04
		float mZ = 0.0f;   // +0x08
		int mState = 0;    // +0x0c: 0 falling, 1 splashing
		int mTimer = 0;    // +0x10
		void Place(WinTwistApp* theApp);  // 0x6784d8
	};
	int mCount = 0;                 // +0x24
	float mSway = 0.0f;             // +0x28
	std::vector<Drop*> mDrops;      // +0x2c (a std::list that nothing is ever taken from)
	Image* mPointy = nullptr;       // +0x38: IMAGE_POINTY
	Image* mPointy2 = nullptr;      // +0x3c: IMAGE_POINTY2
	Image* mPointy3 = nullptr;      // +0x40: IMAGE_POINTY3
	Image* mSplash = nullptr;       // +0x44: IMAGE_SPLASH
	Image* mBackground = nullptr;   // +0x48: IMAGE_RAINDROPS_BG
	explicit RainDrops(WinTwistApp* theApp);  // 0x67932c
	~RainDrops() override;                    // 0x67942f
	void Init() override;                     // 0x679613
	void Update() override;                   // 0x678b36
	void Draw(Graphics* g) override;          // 0x678c3d
	// 0x678754: a ring of the splash image turned by theAngle, 24 triangles of a strip, at theX, theY
	void DrawSplashRing(Graphics* g, float theX, float theY, float theAngle, float theScale);
};


// Sexy::Gears (vtable 0x7d79ec, constructor 0x673855, 0x50 bytes): IMAGE_GEARS_BG under a gear (IMAGE_GEAR) with gears
// round it, each with gears round that, turning; 100 bars of IMAGE_GEAR_BLUR flying out of the middle on top (the bars
// of VizWarpBars, and the same start: 0x68ed45). Random draws, all of the C runtime's rand (four or more a try, until the
// bar is 100000 from the middle by x squared plus y squared): Init 0x673abf (100 bars) and Update 0x673183 (a bar that
// leaves the screen).
class Gears : public Visualizer {
public:
	using Bar = VizWarpBars::Bar;
	std::vector<Bar*> mBars;        // +0x24 (a std::list)
	int mCount = 0;                 // +0x30
	float mAngleA = 0.0f;           // +0x34: the middle gear's turn
	int mUnknown38 = 1;             // +0x38: multiplied by 0 and added in Update
	int mUnknown3C = 0;             // +0x3c: the first gear's last argument (0x672a71's), never read
	float mAngleB = 0.0f;           // +0x40: the turn of the gears round it
	Image* mGear = nullptr;         // +0x44: IMAGE_GEAR
	Image* mGearBlur = nullptr;     // +0x48: IMAGE_GEAR_BLUR
	Image* mBackground = nullptr;   // +0x4c: IMAGE_GEARS_BG
	explicit Gears(WinTwistApp* theApp);  // 0x673855
	~Gears() override;                    // 0x673949
	void Init() override;                 // 0x673abf
	void Update() override;               // 0x673183
	void Draw(Graphics* g) override;      // 0x673435
	void DrawGear(Graphics* g, float theScale, float theX, float theY, float theSpin, float theBase, int theDepth);  // 0x672a71
};

// Sexy::Slipstream (vtable 0x7d7ee4, constructor 0x67b8f4, 0x88 bytes): a trail of 300 points over IMAGE_SLIPSTREAM_BG
// drawn as a strip of IMAGE_JELLYTRAIL_TEXTURE in blue, and 200 specks of dust (IMAGE_SLIPSTREAM_POINTY) streaming
// past. Random draws, all of the C runtime's rand: a point's constructor 0x67a3d2 (three a try, until it is 100 or more
// from the middle by |x| + |y|), for each of the 300 trail points and the 200 specks of Init 0x67ba37, and two more for
// each speck (its place). Update and Draw draw none.
class Slipstream : public Visualizer {
public:
	struct Point {  // 0x3c bytes
		float mX = 0.0f;          // +0x00
		float mY = 0.0f;          // +0x04
		float mZ = 0.0f;          // +0x08
		float mUnknown0C = 0.0f;  // +0x0c
		int mUnknown10 = 0;       // +0x10: the strip's extent (see MakeStripVertices), always 0
		float mSize = 0.0f;       // +0x14: half the strip's height here
		float mUnknown18 = 0.0f;  // +0x18: made 20.0 by MakeStripVertices, never read
		float mAngle = 0.0f;      // +0x1c: a speck's turn
		unsigned char mFlag = 0;  // +0x20: the strip has no height here
		int mScreenX = 0;         // +0x24
		int mScreenY = 0;         // +0x28
		int mUnknown2C = 0;       // +0x2c: the strip's direction across, with +0x30 (0 and 1)
		int mUnknown30 = 0;       // +0x30
		int mUnknown34 = 0;       // +0x34
		int mUnknown38 = 0;       // +0x38
		Point();                  // 0x67a79a (0x67a3d2)
	};
	int mCount = 0;               // +0x24
	float mUnknown2C = 6.0f;      // +0x2c (the constructor's values for +0x2c to +0x58, mostly never read)
	int mUnknown30 = 0xf;         // +0x30
	int mUnknown34 = 0x28;        // +0x34: where the strip's extent starts to shrink it
	int mUnknown38 = 300;         // +0x38
	float mUnknown3C = 9.0f;      // +0x3c
	float mUnknown40 = 4.0f;      // +0x40
	float mUnknown44 = 20.0f;     // +0x44
	float mUnknown48 = 6.0f;      // +0x48
	float mUnknown4C = 40.0f;     // +0x4c
	float mUnknown50 = 4.0f;      // +0x50
	float mUnknown54 = 0.0f;      // +0x54
	float mUnknown58 = 0.0f;      // +0x58
	std::vector<Point*> mTrailPoints;  // +0x5c (a std::vector)
	std::vector<Point*> mDust;         // +0x6c
	Image* mPointy = nullptr;     // +0x7c: IMAGE_SLIPSTREAM_POINTY
	Image* mTrail = nullptr;      // +0x80: IMAGE_JELLYTRAIL_TEXTURE
	Image* mBackground = nullptr; // +0x84: IMAGE_SLIPSTREAM_BG
	explicit Slipstream(WinTwistApp* theApp);  // 0x67b8f4
	~Slipstream() override;                    // 0x67b561
	void Init() override;                      // 0x67ba37
	void Update() override;                    // 0x67a7ba
	void Draw(Graphics* g) override;           // 0x67b308
	void MakeStripVertices(Point* thePoint, TriVertex* theVertices);  // 0x67ad27
	void DrawTrail(Graphics* g);                                      // 0x67af9b
};

// Sexy::GravityField (vtable 0x7d7a38, constructor 0x673d0f): a chain of 125 bent segments that Draw works out afresh
// each frame from the update count; each segment is a run of quads (IMAGE_GRAVITY_SPIRALTEXTURE, MakeRibbon) and a thin
// quad across it (IMAGE_GRAVITY_EDGETEXTURE), drawn as triangle lists through DrawPrimitiveEx (FVF 0x142: position,
// colour, one texture coordinate) under a perspective camera at (0, 0, 555). Random draws: none (the constructor, Init,
// Update and Draw use no generator).
class GravityField : public Visualizer {
public:
	struct Vertex {  // 0x18 bytes, FVF 0x142
		float x, y, z;       // +0x00
		unsigned int mColor; // +0x0c
		float u, v;          // +0x10
	};
	int mCount = 0;              // +0x24: its updates, counted by Update
	Image* mSpiral = nullptr;    // +0x28: IMAGE_GRAVITY_SPIRALTEXTURE
	Image* mEdge = nullptr;      // +0x2c: IMAGE_GRAVITY_EDGETEXTURE
	explicit GravityField(WinTwistApp* theApp);  // 0x673d0f
	~GravityField() override;                    // 0x674963 (0x67475d)
	void Init() override;                        // 0x673d7e
	void Update() override;                      // 0x673d92
	void Draw(Graphics* g) override;             // 0x6755a5
	void DrawField(Graphics* g);                 // 0x674c02
	// 0x6749e5: a run of theCount + 1 quads from theFrom and theTo towards theAcross, each a theFraction of the way on
	// from the last; the texture coordinate runs on from theU by theUStep (which shrinks by 1 - theFraction a quad), the
	// colour is grey by 1 - theAlpha; each quad is two triangles pushed to theVertices
	static void MakeRibbon(std::vector<Vertex>& theVertices, const float* theFrom, const float* theTo, const float* theAcross,
		float theU, float theUStep, int theCount, float theFraction, float theAlpha);
};

// Sexy::VizBuckyShards (vtable 0x7d8088, constructor 0x6802e8): a 20-sided ball cut into 180 triangles with a smaller
// triangle pushed out of each, turning and lit by a point light, with IMAGE_CUBEMAP_6X1_VIZBUCKYSHARDS_CUBE as an
// environment map (Draw builds the mesh each frame). Random draws: none (no generator is used).
class VizBuckyShards : public Visualizer {
public:
	Image* mCube = nullptr;  // +0x24: IMAGE_CUBEMAP_6X1_VIZBUCKYSHARDS_CUBE
	int mCount = 0;          // +0x28: its updates
	int mCountBase = 0;      // +0x2c: mCount at the last Slot7 (the turn and the push are by the updates since)
	explicit VizBuckyShards(WinTwistApp* theApp);  // 0x6802e8
	~VizBuckyShards() override;                    // 0x680d15 (0x680aad)
	void Update() override;                        // 0x68034f
	void Slot7() override { mCountBase = mCount; } // 0x680347
	void Draw(Graphics* g) override;               // 0x681e98
};

class VizMoleculeRedux : public Visualizer {
public:
	Image* mImage = nullptr;       // +0x24: IMAGE_BUCKYTRIFACE
	Image* mBackground = nullptr;  // +0x28: IMAGE_MOLECULE_BG
	int mCount = 0;                // +0x2c: its updates
	explicit VizMoleculeRedux(WinTwistApp* theApp);  // 0x68c928
	~VizMoleculeRedux() override;                    // 0x68ca44 (0x68c9a0)
	void Update() override;                          // 0x68c99c
	void Draw(Graphics* g) override;                 // 0x68d641
};

}  // namespace Sexy
