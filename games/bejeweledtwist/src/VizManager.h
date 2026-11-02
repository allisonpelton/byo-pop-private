// Sexy::VizManager (0x1e8 bytes, a TwistWidget; 0x7587a0-0x758f3e and 0x7591ce-0x7597cc, app +0xc6c): the
// background behind a game. It holds one Visualizer at a time and runs it at its own speed (+0x1a0 updates of the
// visualizer per update of its own). With Direct3D and the options' "visualizers" box it picks one of 24 animated
// ones by the game's mode; in 2D it always shows Viz2D, a still picture for each level.
//
// port: 2D only, so Viz2D is the only visualizer; the 3D ones (RedBars to VizBuckyShards, registered by name in
// the constructor) and their render targets are left out, and their names are kept only for the indexes.
#pragma once

#include "CurvedVal.h"
#include "TwistWidget.h"

namespace Sexy {

class Image;
class WinTwistApp;

// Visualizer (0x24 bytes; vtable 0x7d804c, 0x67fa86-0x6801b8): what a VizManager shows (slot 8, its name, is
// only for the debug text)
class Visualizer {
public:
	int mFlags4 = 1;              // +0x4: bit 0 draws only around the board (with +0x8 and +0xc, see DrawsAroundBoard)
	int mFlags8 = 0;              // +0x8
	int mFlagsC = 0;              // +0xc: set and cleared by the VizManager for each draw
	float mUnknown10 = 1.0f;      // +0x10
	WinTwistApp* mApp;            // +0x14
	int mWidth = 0;               // +0x18: the VizManager's size (0x758dcf)
	int mHeight = 0;              // +0x1c
	int mUpdateCnt = 0;           // +0x20: its updates, counted by the VizManager

	explicit Visualizer(WinTwistApp* theApp);  // 0x67fa86
	virtual ~Visualizer();                     // 0 (0x67fb8d)
	virtual void Init() {}                     // 1 (0x67a1c9)
	virtual void Update() {}                   // 2 (0x67a1ca)
	virtual void Draw(Graphics* g) { (void)g; }  // 3 (0x67fab8)
	virtual void Slot4() {}                    // 4 (0x670126)
	virtual void NextLook() {}                 // 5 (0x67012b): a cascade (VizManager 0x757ff5)
	virtual void NewLevel() {}                 // 6 (0x67012c): the board's next level (VizManager 0x75800a)
	virtual void Slot7() {}                    // 7 (0x67012d): (VizManager 0x75801f)

	// ((~+0xc & +0x4) | +0x8) & 1: the board stands still, so only the screen around it is drawn
	bool DrawsAroundBoard() const { return (((~mFlagsC & mFlags4) | mFlags8) & 1) != 0; }
	// 0x67fdb6: theImage over the whole screen, cropped to the screen's width or stretched to it, and all of it
	// but the board's rectangle while DrawsAroundBoard
	void DrawImage(Graphics* g, Image* theImage);
};

// Sexy::Viz2D (0xa8 bytes; vtable 0x7f2ca8, 0x759a72-0x759d99): the level's picture, IMAGE_ZENDROP1 to 47 by the
// VizManager's +0x194 (IMAGE_VIRTUTORIAL for -1, the first game's tutorial), faded over the old one on a new level
class Viz2D : public Visualizer {
public:
	Image* mImage = nullptr;      // +0x24
	int mImageId = -1;            // +0x28
	Image* mNextImage = nullptr;  // +0x2c: fading in over mImage
	int mNextImageId = -1;        // +0x30
	CurvedVal mFade;              // +0x38: mNextImage's alpha

	explicit Viz2D(WinTwistApp* theApp);  // 0x759a72
	~Viz2D() override;                    // 0x759b49
	void Update() override;               // 0x759bed
	void Draw(Graphics* g) override;      // 0x759afc
	void NewLevel() override;             // 0x759cff
	// 0x759c67: the picture for the VizManager's +0x194 at once
	void SetImageFromIndex();
};

class VizManager : public TwistWidget {
public:
	WinTwistApp* mApp;                // +0x19c
	int mUpdateCnt148 = 0;            // +0x148: its updates while it runs
	int mVizIndex = -1;               // +0x14c: mViz's place in the list of names
	Visualizer* mViz = nullptr;       // +0x150
	int mUnknown18C = -1;             // +0x18c: never set otherwise (Update runs while it is -1)
	int mUnknown190 = 0;              // +0x190: the 3D visualizer chosen for the game (SetVizByName, 0x7593ea)
	int mBackdrop = 0;                // +0x194: Viz2D's picture (-1 the tutorial's, else the level)
	bool m2D = true;                  // +0x198: no Direct3D or no "visualizers" box (set by the app's Init)
	double mSpeed = 1.0;              // +0x1a0: the visualizer's updates per update
	double mUpdateFrac = 0.0;         // +0x1a8
	double mUnknown1B0 = 1.0;         // +0x1b0
	bool mPaused = false;             // +0x1b8

	explicit VizManager(WinTwistApp* theApp);  // 0x7597cc
	~VizManager() override;                    // 0x75871b

	void Update() override;           // 0x758f3e
	void Draw(Graphics* g) override;  // 0x7587a0

	static int FindViz(const char* theName);   // 0x75864e: the index of theName in the list, -1 if none
	void SetViz(int theIndex, bool force);     // 0x758e62
	void SetVizByName(const char* theName, bool theNow);  // 0x758ec7
	void SetBackdrop(int theBackdrop, bool theNow);       // 0x758ef4
	void NextLook();                           // 0x757ff5
	void NewLevel();                           // 0x75800a
	void Slot7();                              // 0x75801f
};

}  // namespace Sexy
