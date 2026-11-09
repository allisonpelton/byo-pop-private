// Sexy::GraphicsRecorder (0x70d7ab-0x70e293; the one object is gGraphicsRecorder, made by the app's constructor): the
// game draws its images through it. It passes each draw on to the Graphics unless mSkipDraw is set, and while
// mRecording is set it also keeps the draw (of an image loaded from a file) in its list, tagged with mFrame. Only the
// board sets either flag (UpdateGame 0x6d090f): it records the frames of a move, and the replay's rewind draws them
// back, last frame first (DrawOverlay 0x6b4a39), while mSkipDraw keeps the board's own drawing off.
#pragma once

#include "Color.h"
#include "Rect.h"

#include <vector>

namespace Sexy {

class Graphics;
class Image;
class Transform;

// One recorded draw (Sexy::GraphicsOperation, 0x60 bytes; slot 0 Execute 0x70d10a)
class GraphicsOperation {
public:
	enum {
		TYPE_DRAW_F = 0,           // DrawImageF
		TYPE_DRAW_STRETCHED = 1,   // DrawImage x, y, width, height
		TYPE_DRAW_SRC = 2,         // DrawImage x, y, source rectangle
		TYPE_DRAW_DEST_SRC = 3,    // DrawImage destination and source rectangles
		TYPE_DRAW_CEL = 4,         // DrawImageCel x, y, cel
		TYPE_DRAW_CEL_DEST = 5,    // DrawImageCel destination rectangle, cel
		TYPE_DRAW_ROTATED = 6,     // DrawImageRotated x, y, rotation, source (0x70de4b; the game never records one)
		TYPE_DRAW_ROTATED_AT = 7,  // DrawImageRotated x, y, rotation, centre, source
	};

	int mFrame = 0;          // +0x08
	int mType = 0;           // +0x0c
	Image* mImage = nullptr; // +0x10
	double mX = 0;           // +0x18: with the Graphics' translation
	double mY = 0;           // +0x20
	double mWidth = 0;       // +0x28: a destination's size, or a rotation's centre
	double mHeight = 0;      // +0x30
	Rect mSrcRect;           // +0x38: the source; its first int is a cel's number, its last two a stretch's size
	Color mColor;            // +0x48: the Graphics' colour if it colorizes, else white
	int mDrawMode = 0;       // +0x58
	float mRot = 0;          // +0x5c

	void Execute(Graphics* g);  // 0x70d10a
};

class GraphicsRecorder {
public:
	int mFrame = 0;           // +0x04: the board's frame the draws belong to (0x70cf00)
	bool mSkipDraw = false;   // +0x08: record without drawing
	bool mRecording = false;  // +0x09
	std::vector<GraphicsOperation> mOperations;  // +0x0c

	int GetLastFrame();                      // 0x70d694: the frame of the last recorded draw, -1 with none
	void DrawFrame(Graphics* g, int theFrame);  // 0x70d293: the draws of the frame at the end from theFrame on
	void PopFramesFrom(int theFrame);        // 0x70d704: the draws of theFrame and later are dropped

	void DrawImage(Graphics* g, Image* theImage, int theX, int theY);                                  // 0x70db43
	void DrawImageF(Graphics* g, Image* theImage, float theX, float theY);                             // 0x70dae6
	void DrawImage(Graphics* g, Image* theImage, int theX, int theY, int theWidth, int theHeight);     // 0x70dbaf
	void DrawImage(Graphics* g, Image* theImage, int theX, int theY, const Rect& theSrcRect);          // 0x70dc33
	void DrawImage(Graphics* g, Image* theImage, const Rect& theDestRect, const Rect& theSrcRect);     // 0x70dcac
	void DrawImageCel(Graphics* g, Image* theImage, int theX, int theY, int theCel);                    // 0x70dd43
	void DrawImageCel(Graphics* g, Image* theImage, const Rect& theDestRect, int theCel);               // 0x70ddb8
	// 0x70def3
	void DrawImageRotated(Graphics* g, Image* theImage, int theX, int theY, double theRot, int theRotCenterX,
		int theRotCenterY, const Rect* theSrcRect);
	// 0x70dfad: Graphics::DrawImageTransformHelper's cases, the simple ones recorded (theUseFloat:
	// DrawImageTransformF)
	void DrawImageTransform(Graphics* g, Image* theImage, const Transform& theTransform, const Rect& theSrcRect,
		float theX, float theY, bool theUseFloat);

private:
	GraphicsOperation* AddOperation(Graphics* g, int theType);  // 0x70da92, 0x70d262: the operation record's constructor, inline here
};

extern GraphicsRecorder* gGraphicsRecorder;  // 0x85d6f8

}  // namespace Sexy
