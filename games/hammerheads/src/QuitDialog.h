#ifndef __HAMMERHEADS_QUITDIALOG_H__
#define __HAMMERHEADS_QUITDIALOG_H__

// The box that asks "Do you really want to quit?" (Yes and No), with a hammer that swings on the page and a gnome who waves goodbye.
// Original: class Sexy::QuitDialog : public NcMovingDialog (0x180 bytes; the constructor is 0x45c560, the destructor 0x45c3a0).

#include "NcMovingDialog.h"

namespace Sexy
{

class QuitDialog : public NcMovingDialog
{
public:
	double			mGnomeCel;			// +0x168: which cel of the gnome's row is shown (a whole number and a part)
	int				mGnomeRow;			// +0x170: the row of the gnome's picture: 0 plays through once, 1 repeats, 2 while the mouse is over "No"
	double			mSwing;				// +0x178: the time the hammer has been swinging

public:
	QuitDialog();															// 0x45c560

	virtual void	UpdateF(float theFrac) override;						// 0x45c3c0
	virtual void	Draw(Graphics* g) override;								// 0x45c7f0
	virtual int		GetPreferredHeight(int theWidth) override;				// 0x45c530
	virtual void	ButtonDepress(int theId) override;						// 0x44fe50: the code the game-over box has too
	virtual void	Closed() override;										// 0x45c4e0
};

// 0x45c730: the box is made, sized and added as dialog 1
QuitDialog* MakeQuitDialog();

}

#endif
