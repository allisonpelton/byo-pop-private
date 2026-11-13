#ifndef __HAMMERHEADS_ZOOMWIPEWIDGET_H__
#define __HAMMERHEADS_ZOOMWIPEWIDGET_H__

// The wipe between two screens: a picture in the middle of the screen grows until it covers the screen, the colour it is made of
// fills the screen, the new screen is made (the callback), and a second picture, a hole, grows from the middle to show it.
// Application::DoZoomWipe makes it, picks the two pictures and the colour, and Application::EndZoomWipe takes it down.
// Original: class Sexy::ZoomWipeWidget : public Widget (0xb0 bytes; the constructor is 0x462d00, the destructor 0x462da0).

#include "Color.h"
#include "Rect.h"
#include "Widget.h"

namespace Sexy
{

class Image;

class ZoomWipeWidget : public Widget
{
public:
	enum
	{
		STATE_WAITING = 0,			// not set up
		STATE_CLOSING = 1,			// the first picture grows, fading in at first, over the old screen
		STATE_COVERED = 2,			// the screen is the colour (for the update that ends it)
		STATE_OPENING = 3			// the second picture grows over the new screen
	};

	Image*			mFrom;			// +0x84: the picture that grows over the old screen
	Image*			mTo;			// +0x88: the hole that grows to show the new one
	Color			mColor;			// +0x8c
	int				mState;			// +0x9c
	double			mScale;			// +0xa0: from 60 down to 1; the picture is eight times its size over this
	void			(*mCallback)();	// +0xac: runs while the screen is covered

public:
	ZoomWipeWidget();																// 0x462d00

	// 0x462eb0: the wipe starts
	void			Setup(Image* theFrom, Image* theTo, const Color& theColor, void (*theCallback)());

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x462e80: it covers the application's window
	virtual void	UpdateF(float theFrac) override;								// 0x462db0
	virtual void	Draw(Graphics* g) override;										// 0x463060

private:
	// 0x462f30: the picture is drawn in the middle of the screen, eight times its size over the scale; where it is is the result
	Rect			DrawZoomedImage(Graphics* g, Image* theImage);
};

}

#endif
