// Sexy::CurtainTransition (0xcc bytes, constructor 0x440e90, vtable 0x7ac134): the theatre curtain that closes
// over the screen and opens again between the game's screens. Its four quarters are cels of IMAGE_CURTAIN_ANIM:
// the strip as it is for the top left, mirrored for the right, flipped for the bottom. When it is done it calls
// the app's CurtainDone (0x44d360), which goes on with the transition the app is in.
#pragma once

#include <string>

#include "Widget.h"

namespace Sexy {

class GameApp;
class Graphics;
class Image;

class CurtainTransition : public Widget {
public:
	int mFrame;                // +0x88, the cel (0 closed, the last one open)
	int mTopY;                 // +0x8c, the valance's y (0 down, minus its height up)
	int mCordX;                // +0x90, the cord's x (moved, never drawn)
	bool mDrawn;               // +0x94, drawn at its last frame
	bool mClosing;             // +0x95
	bool mDone;                // +0x96
	bool mTopMoving;           // +0x97
	bool mShowLoadingBox;      // +0x98, a box with mLoadingText once closed
	bool mLoadingBoxShown;     // +0x99
	std::string mLoadingText;  // +0x9c
	Image* mImage;             // +0xb8, IMAGE_CURTAIN_ANIM
	Image* mMirroredImage;     // +0xbc
	Image* mFlippedImage;      // +0xc0
	Image* mMirroredFlippedImage;  // +0xc4
	GameApp* mApp;             // +0xc8

	explicit CurtainTransition(GameApp* theApp);
	~CurtainTransition() override;  // 0x441020

	void StartClosing();  // 0x4410e0
	void StartOpening();  // 0x441140

	void Update() override;              // 0x4411d0
	void Draw(Graphics* g) override;     // 0x4413b0
};

}  // namespace Sexy
