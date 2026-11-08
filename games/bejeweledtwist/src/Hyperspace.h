// Sexy::Hyperspace (0x15e0 bytes, a TwistWidget; 0x70e341-0x71f10e): the space behind every screen and the jump
// between levels. With Direct3D it flies a camera through the level's world (planets, a saucer, the sidebar's tube);
// in 2D it draws the still picture of space and, for the jumps, flashes and a tunnel of its own.
//
// port: both modes. Hyperspace.cpp holds the 2D paths and the shared head; with gD3DInterface set SetTransition, Update
// and Draw go to HyperspaceD3D.cpp (the world's curves, the state machine, the camera, the ship, the draws). Not ported
// there yet (NotPorted): the tunnel's fixed-function and CPU-transformed paths (DrawTunnel3D
// 0x711b65), and the replay pod.
#pragma once

#include "CurvedVal.h"
#include "Math3D.h"
#include "TwistWidget.h"

#include <list>
#include <vector>

namespace Sexy {

class D3DObject;
class SoundInstance;
class WorldGraphicsListener;

// WorldOffCenterPerspectiveCamera (0x68 bytes, vtable 0x7e4200; ctor 0x70e361): a perspective camera whose frustum is
// off to one side, +0x58 to +0x64. Its projection (slot 0, 0x70e44b) differs from the plain camera's; the screen
// position (slot 3, 0x70e4b0) is the same as PerspectiveCamera::Project's.
struct OffCenterCamera : PerspectiveCamera {
	float mLeft = 0;    // +0x58
	float mRight = 0;   // +0x5c
	float mTop = 0;     // +0x60
	float mBottom = 0;  // +0x64

	OffCenterCamera() : PerspectiveCamera(45.0f, 1.0f, 1.0f, 10000.0f) {}
	// 0x70e381: theCenterX and theCenterY move the frustum; the rest as PerspectiveCamera::SetFov
	void SetFrustum(float theFovTimesAspect, float theAspect, float theCenterX, float theCenterY, float theNear,
		float theFar);
};

// A crystal flying about the level's world (0x48 bytes; the list at +0xc68, made by 0x717dd6)
struct WorldCrystal {
	int mType = 0;            // +0x00: which of the app's crystal models
	Transform3D mTransform;   // +0x04
	float mUnknown40 = 0;     // +0x40: set by 0x717dd6, read only by the code that moves them
	float mUnknown44 = 0;     // +0x44
};

// A puff of the ship's thrust (0x4c bytes; the list at +0x14b4, made by 0x714ea9, reset by 0x70e527). Type 0 is a
// glow or a puff, 1 a trail drawn as strips between the engine's last two
struct ThrustParticle {
	int mType = 0;                 // +0x00
	Vec3 mPos{0, 0, 0};            // +0x04
	Vec3 mVel{0, 0, 0};            // +0x10
	Vec3 mDir{0, 0, 0};            // +0x1c: its own drift, damped by mDamp
	float mDamp = 0;               // +0x28
	float mAlpha = 1.0f;           // +0x2c: gone when it is 0 or less
	float mAlphaRate = 0;          // +0x30
	float mSize = 1.0f;            // +0x34: half the quad's side, gone when it is 0 or less
	float mSizeRate = 0;           // +0x38
	float mAngle = 0;              // +0x3c: the quad's turn about z
	int mEngine = -1;              // +0x40: which of the nine engines made it
	Image* mImage = nullptr;       // +0x44
	bool mDead = false;            // +0x48
};

// A crystal circling the planet (0x94 bytes; the list at +0xc78, made by 0x717fcc)
struct RingCrystal {
	int mType = 0;            // +0x00: which of the app's crystal models
	Transform3D mTransform;   // +0x04
	Mat3 mUnknown40;          // +0x40: its own turn
	Mat3 mUnknown64;          // +0x64: turned by the planet's
	Vec3 mUnknown88;          // +0x88
};

class Hyperspace : public TwistWidget {
public:
	int mLevel = 0;              // +0x148: the board's level - 1
	int mState = 0;              // +0x14c: 0 idle, 6 and 7 after a jump, 10 done, ...
	int mTransition = 0;         // +0x150: the jump SetTransition last set up
	bool mFlag155 = false;       // +0x155
	bool mIs3D = false;          // +0x156: drawn with Direct3D
	bool mFlag157 = false;       // +0x157: set by the mode select's new game (Mandala::StartMode)
	bool mFlag158 = false;       // +0x158
	bool mFlag159 = true;        // +0x159
	bool mFlag15A = false;       // +0x15a: set when a board takes it (0x6a5fa3)
	bool mFlag15B = true;        // +0x15b: the jump's first half is still to come (transition 8)
	bool mFlag15C = false;       // +0x15c: set while Update skipped the jump
	bool mFlag15E = false;       // +0x15e: holds the jump (set from outside, not yet known)
	CurvedVal mUnknown1E8;       // +0x1e8: the VizManager's speed while the camera runs (SetTransition(12) sets it
	                             // to 1; in 2D nothing else sets it, so it reads 0)
	CurvedVal mUnknown258;       // +0x258: the board's brightness through a jump (Board 0x6a6a07)
	// (+0xc58: the trial's limit after transition 14, registration code, left out)
	CurvedVal mUnknown12B0;      // +0x12b0: the board's brightness while state 11 (transition 12) runs
	CurvedVal mUnknown1570;      // +0x1570: the jump's length
	int mWorld = 4;              // +0xc5c: the level's world (0x7181b3 picks it by level)
	CurvedVal mFlash;            // +0x1320: the white flash of a jump
	double mWarmUp = 0;          // +0x1078: rises to 1 while idle
	Image* mUnknown160 = nullptr;  // +0x160: the old level's picture during a 2D jump (the VizManager's)
	int mUnknown164 = -1;        // +0x164: its id
	bool mOwnsUnknown160 = false;  // port: mUnknown160 is the screen-sized picture MakeOldPicture made, not a level's
	// +0x1548: the 3D camera's turn; the challenge menu's Update (0x6e3f41) sets it to the menu's camera (+0x800) by mCameraTurn,
	// the 3D drawing reads it (0x71726b)
	Mat3 mUnknown1548;

	// The 3D mode's state (gD3DInterface; the constructor 0x714f98 makes them, 0x715c21 and 0x7181b3 move them)
	CurvedVal mUnknown2C8, mUnknown338, mUnknown3A8, mUnknown488, mUnknown4F8;  // +0x2c8, +0x338, +0x3a8, +0x488, +0x4f8
	CurvedVal mUnknown5D8, mUnknown648, mUnknown6B8;   // +0x5d8 (the tunnel shows while it is under 1), +0x648, +0x6b8
	CurvedVal mUnknown798, mUnknown808, mUnknown878;   // +0x798, +0x808, +0x878
	double mUnknown8E8[3] = {0.0, 0.0, -50.0};         // +0x8e8
	CurvedVal mUnknown900;                             // +0x900
	CurvedVal mUnknown978, mUnknown9E8, mUnknownA58;   // +0x978, +0x9e8, +0xa58: x, y and z of the crystals' orbit
	Transform3D mSaucer;                               // +0xac8: the saucer's place
	CurvedVal mUnknownDD8, mUnknownE48, mUnknownEB8;   // +0xdd8, +0xe48, +0xeb8: the sidebar camera's turns about x, z and y
	CurvedVal mUnknownF28, mUnknownF98, mUnknown1008;  // +0xf28, +0xf98, +0x1008: the three backdrops' fields of view
	CurvedVal mUnknown1080;                            // +0x1080: the cone's alpha
	CurvedVal mUnknown1160;                            // +0x1160: the sidebar engine's glow
	// The rest of the world's curves (0x7181b3 sets them, 0x715c21 steps them); what each one drives where it is known
	CurvedVal mUnknown178;                             // +0x178: how far the ship's sidebar is moved aside (0x715480)
	CurvedVal mUnknown418;                             // +0x418: how far behind the camera the sidebar flies (0x716fdb)
	CurvedVal mUnknown568;                             // +0x568
	CurvedVal mUnknown10F0;                            // +0x10f0
	CurvedVal mUnknown11D0;                            // +0x11d0
	CurvedVal mUnknown1240;                            // +0x1240: the sidebar's slide into place (0x715480)
	CurvedVal mUnknownB08, mUnknownB78, mUnknownBE8;   // +0xb08, +0xb78, +0xbe8: x, y and z of the saucer's path (0x7158f8)
	CurvedVal mUnknownC88, mUnknownCF8, mUnknownD68;   // +0xc88, +0xcf8, +0xd68: x, y and z of the camera's path
	Vec3 mUnknown1520;                                 // +0x1520: which way the ship's path goes (0x715480)
	Vec3 mUnknown153C;                                 // +0x153c: the camera path's start, taken when a jump starts (0x7181b3)
	Mat3 mUnknown14FC;                                 // +0x14fc: the ship's turn (0x715480, 0x7158f8)
	SoundInstance* mSpaceSound = nullptr;              // +0x970: the jump's space noise, looping
	std::vector<WorldCrystal> mCrystals;               // +0xc68
	std::vector<RingCrystal> mRingCrystals;            // +0xc78
	int mPlanetObject = 0;                             // +0xc60: which of the app's planet models (0x717fcc)
	Image* mPlanetImage = nullptr;                     // +0xc64: its surface (0x717fcc)
	float mUnknown1390[3] = {0, 0, 0};                 // +0x1390: where the world's camera sits (0x716f83)
	PerspectiveCamera mCamera139C{45.0f, 1.0f, 1.0f, 10000.0f};  // +0x139c: the world's camera
	PerspectiveCamera mCamera13F4{45.0f, 1.0f, 1.0f, 10000.0f};  // +0x13f4: the backdrops' camera
	OffCenterCamera mCamera144C;                       // +0x144c: the sidebar's camera (its transform is +0x1450)
	std::list<ThrustParticle> mParticles;              // +0x14b4: the ship's thrust (0x714ea9 adds, 0x715c21 steps)
	Transform3D mUnknown14C0;                          // +0x14c0: the ship's (the sidebar's) place on the camera's path (0x715480)

	Hyperspace();                // 0x714f98
	~Hyperspace() override;      // 0x714e6e

	void Update() override;                  // 0x715c21
	void Draw(Graphics* g) override;         // 0x713e2d
	void DrawOverlay(Graphics* g) override;  // 0x713d82
	using Widget::DrawOverlay;

	// 0x71c18f: the jump from a picked challenge's planet into the game (state 3, transition 11). The planet's place
	// (the three floats it gets) only aims the 3D camera at it
	void StartChallenge();
	// 0x7181b3: sets up the jump theTransition (0 none) and the level's world
	void SetTransition(int theTransition);
	// 0x714883: the end of a game, space coming back over the board (transition 13, state 12)
	void StartEndGame();
	// 0x70e623: the VizManager's picture (and 3D visualizer) for the board's game and level
	void ChooseViz(bool theDraw);
	// 0x711b65: the tunnel of a jump; in 2D, while the VizManager is hidden, the level's picture (mUnknown160 for the
	// first half) faded in by mUnknown1570 and centred at the art's width
	void DrawTunnel(Graphics* g);
	// 0x70e567: in 2D a DDImage the size of the screen for mUnknown160, unless it has one (with Direct3D a render
	// target, "VizImage")
	void MakeOldPicture();
	// 0x70f836: the old level's picture (mUnknown160) at mUnknown12B0's alpha, at the end of a jump
	void DrawOldPicture(Graphics* g);
	// 0x716e2f on (the end of Update): the planets and the camera (Direct3D only) and, while the board holds this
	// hyperspace, the ship's thrust and its random numbers
	void UpdateSpace();

	// The 3D mode (gD3DInterface set); the original's Draw 0x713e2d and what it calls (HyperspaceD3D.cpp)
	void Draw3D(Graphics* g);                 // 0x713e2d's 3D half
	void DrawCrystals();                      // 0x71042e: the crystals flying about
	void DrawSaucer();                        // 0x710c13
	void DrawPlanet(D3DObject* thePlanet);    // 0x7106f6
	void Draw3DSidebar();                     // 0x711529
	void DrawCone();                          // 0x71019e
	void DrawTunnel3D(Graphics* g);           // 0x711b65's 3D half
	void UpdateThrust();                      // 0x715c21's tail: the thrust particles stepped and made (0x717...)
	void DrawParticles();                     // 0x71378b: the ship's thrust (UpdateThrust steps and makes the puffs)
	void DrawTravellers(Graphics* g);         // 0x710f5b: the souls of the last level's special gems, put on the screen
	void MakeCrystals();                      // 0x717dd6
	void SetTransition3D(int theTransition);  // 0x7181b3's gD3DInterface branches
	void SetWorldCurves(int theWorld);        // 0x718d8e's cases
	void Update3D();                          // 0x715c21's gD3DInterface branches
	void UpdateWorld3D();                     // 0x716e2f: crystals, camera, ship and saucer
	void PlaceShip();                         // 0x715480
	void PlaceSaucer();                       // 0x7158f8
	void MakeRingCrystals(int theCount);      // 0x717fcc
};

}  // namespace Sexy
