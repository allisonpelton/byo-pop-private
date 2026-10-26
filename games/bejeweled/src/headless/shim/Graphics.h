#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

// port(variant): headless shim of framework/Graphics.h for the verifier (games/bejeweled/docs/CODE_MODS_DESIGN.md
// 6.9). Not part of the original game. Drawing does nothing: no draw writes a field play reads (Board::Draw,
// SetupLevel's background and ColorizeText only write image bits), so a headless board plays as a drawn one.

#include "Common.h"
#include "Rect.h"
#include "Color.h"
#include "Image.h"

namespace Sexy
{

class Font;

class Graphics
{
public:
	enum
	{
		DRAWMODE_NORMAL,
		DRAWMODE_ADDITIVE
	};

	Font*				mFont;

	Graphics(Image* theDestImage = NULL) : mFont(NULL) {}
	Graphics(const Graphics& theGraphics) : mFont(theGraphics.mFont) {}

	void				SetFont(Font* theFont) { mFont = theFont; }
	Font*				GetFont() { return mFont; }
	void				SetColor(const Color& theColor) {}
	void				SetDrawMode(int theDrawMode) {}
	void				SetColorizeImages(bool colorizeImages) {}
	void				ClipRect(int theX, int theY, int theWidth, int theHeight) {}
	void				DrawString(const SexyString& theString, int theX, int theY) {}
	void				DrawImage(Image* theImage, int theX, int theY) {}
	void				DrawImage(Image* theImage, int theX, int theY, const Rect& theSrcRect) {}
	void				DrawImage(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect) {}
	void				DrawImageRotated(Image* theImage, int theX, int theY, double theRot, int theRotCenterX, int theRotCenterY, const Rect* theSrcRect = NULL) {}
	void				DrawImageMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, bool mirror = true) {}
	void				DrawImageMirror(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, bool mirror = true) {}
	void				SetScale(float theScaleX, float theScaleY, float theOrigX, float theOrigY) {}
};

}

#endif
