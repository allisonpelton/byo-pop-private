#ifndef __HAMMERHEADS_NCMOVINGDIALOG_H__
#define __HAMMERHEADS_NCMOVINGDIALOG_H__

// Every dialog the application makes is a NcMovingDialog (Application::NewDialog): it comes down from above the screen with a
// whoosh, stops at the middle, and goes back up when one of its buttons (or the escape key) closes it; the application hears
// of the answer once it is out of sight. It is the original's Sexy::NcMovingDialog (0x468870, 0x168 bytes).

#include <string>

#include "Dialog.h"

namespace Sexy
{

class NcMovingDialog : public Dialog
{
public:
	int				mRestY;				// +0x148: where the box stops, in the middle of the screen
	double			mSlide;				// +0x150: the speed of the slide, from very negative (above the screen) to zero (at rest); positive once it leaves
	int				mCloseResult;		// +0x158: the button that closed it (the answer the application gets)
	bool			mLeaving;			// +0x15c: it has been told to leave
	bool			mClosePending;		// +0x15d: it has not yet gone out of sight, where Closed runs
	int				mEscapeResult;		// +0x160: the answer the escape key gives; -1 when the escape key does nothing

public:
	// theHeight is the height it settles at: it starts just above the screen, a whole height up
	NcMovingDialog(int theHeight, Image* theComponentImage, Image* theButtonComponentImage, int theId, bool isModal,
		const std::string& theDialogHeader, const std::string& theDialogLines, const std::string& theDialogFooter, int theButtonMode);	// 0x468870

	virtual void	UpdateF(float theFrac) override;						// 0x468980
	virtual void	KeyDown(KeyCode theKey) override;						// 0x468b40
	virtual void	ButtonPress(int theId) override;						// 0x468bb0
	virtual void	ButtonDepress(int theId) override;						// 0x468b80
	virtual void	ButtonMouseEnter(int theId) override;					// 0x468c30
	virtual void	Closed();												// 0x44e210: it has gone out of sight; the application is told

	void			CloseWith(int theResult);								// 0x468aa0: it goes back up, answering theResult
	void			SetEscapeResult(int theResult);							// 0x468b30: what the escape key answers (-1 for nothing)
	bool			IsMoving() const;										// 0x468bf0: it is still sliding, in or out
	int				GetContentX() const;									// 0x468ac0: the page's left edge on the screen
	int				GetContentY() const;									// 0x468ae0: the top of the page's lines on the screen
	int				GetContentWidth() const;								// 0x468b00
};

// 0x41d210: a dialog the application did not make with NewDialog (or one made with it, and given more lines): the look of the
// others (fonts, colours, the buttons' cels), and a width the text fits in (at least theMinWidth)
void LayoutDialog(Dialog* theDialog, int theMinWidth);

}

#endif
