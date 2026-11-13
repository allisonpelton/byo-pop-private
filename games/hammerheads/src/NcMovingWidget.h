#ifndef __HAMMERHEADS_NCMOVINGWIDGET_H__
#define __HAMMERHEADS_NCMOVINGWIDGET_H__

// A page that comes down from above the screen, stops, and goes down again (out of sight at the bottom) once it has been told to
// leave; the one that is told of the page's end is Closed. The level introduction is the only one. It slides as NcMovingDialog
// does, except that it leaves downward and does not sound.
// Original: class Sexy::NcMovingWidget : public Widget (0x98 bytes; the constructor is 0x468c70, the destructor 0x468e10).

#include "Widget.h"

namespace Sexy
{

class NcMovingWidget : public Widget
{
public:
	int				mRestY;				// +0x80: where the page stops
	double			mSlide;				// +0x88: the speed of the slide, from very negative (above the screen) to zero (at rest); positive once it leaves
	int				mCloseResult;		// +0x90: the button that closed it (the answer Closed looks at)
	bool			mLeaving;			// +0x94: it has been told to leave

public:
	// theHeight is the height it settles at: it starts one height above the screen
	explicit NcMovingWidget(int theHeight);										// 0x468c70

	virtual void	UpdateF(float theFrac) override;							// 0x468ce0

	// It has gone out of sight at the bottom (the original's vtable slot is a pure virtual)
	virtual void	Closed() = 0;												// +0xd8
};

}

#endif
