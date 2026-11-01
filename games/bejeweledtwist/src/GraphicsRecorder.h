// Sexy::GraphicsRecorder (0x70d7ab-0x70e293; the one object is gGraphicsRecorder, made by the app's constructor): the
// game draws its images through it. It passes each draw on to the Graphics unless mSkipDraw is set, and while
// mRecording is set it also keeps a list of the draws. Only the board sets either flag (0x6d090f, 0x6d6899); the
// list and its playback come with the board. Without them every method is the Graphics call it names.
#pragma once

#include "Rect.h"

namespace Sexy {

class Graphics;
class Image;
class Transform;

class GraphicsRecorder {
public:
	int mFrame = 0;           // +0x04: the board's frame the draws belong to (0x70cf00)
	bool mSkipDraw = false;   // +0x08: record without drawing
	bool mRecording = false;  // +0x09

	// 0x70d694: the frame of the last recorded draw, -1 with none
	// port: the list of draws comes with the board's playback; until then nothing is recorded
	int GetLastFrame() { return -1; }

	void DrawImage(Graphics* g, Image* theImage, int theX, int theY);                                  // 0x70db43
	void DrawImageF(Graphics* g, Image* theImage, float theX, float theY);                             // 0x70dae6
	void DrawImage(Graphics* g, Image* theImage, int theX, int theY, int theWidth, int theHeight);     // 0x70dbaf
	void DrawImage(Graphics* g, Image* theImage, int theX, int theY, const Rect& theSrcRect);          // 0x70dc33
	void DrawImage(Graphics* g, Image* theImage, const Rect& theDestRect, const Rect& theSrcRect);     // 0x70dcac
	void DrawImageCel(Graphics* g, Image* theImage, int theX, int theY, int theCel);                    // 0x70dd43
	void DrawImageCel(Graphics* g, Image* theImage, const Rect& theDestRect, int theCel);               // 0x70ddb8
	// 0x70dfad: Graphics::DrawImageTransform (theUseFloat: DrawImageTransformF)
	void DrawImageTransform(Graphics* g, Image* theImage, const Transform& theTransform, const Rect& theSrcRect,
		float theX, float theY, bool theUseFloat);
};

extern GraphicsRecorder* gGraphicsRecorder;  // 0x85d6f8

}  // namespace Sexy
