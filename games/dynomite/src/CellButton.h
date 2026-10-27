#ifndef __CELLBUTTON_H__
#define __CELLBUTTON_H__

// The game's button (vtable 0x4fcca0, 0x94 bytes; 0x4373c0-0x437a80): the framework's ButtonWidget of 2003 with the
// game's Cells for its pictures. It derives from Widget and has the ButtonWidget's fields and slot 43 (SetFont).

#include "Widget.h"

namespace Sexy
{

class ButtonListener;
class Cell;
class Font;

class CellButton : public Widget
{
public:
	int					mId;				// +0x58
	SexyString			mLabel;				// +0x5c
	Font*				mFont;				// +0x78
	Cell*				mButtonCell;		// +0x7c
	Cell*				mOverCell;			// +0x80
	Cell*				mDownCell;			// +0x84
	Cell*				mDisabledCell;		// +0x88
	bool				mInverted;			// +0x8c
	bool				mBtnNoDraw;			// +0x8d
	bool				mFrameNoDraw;		// +0x8e
	ButtonListener*		mButtonListener;	// +0x90

	CellButton(int theId, ButtonListener* theButtonListener);	// 0x437520
	virtual ~CellButton();										// 0x4375c0

	virtual void		SetFont(Font* theFont);					// 0x4373c0 (slot 43)
	virtual void		SetDisabled(bool isDisabled) override;	// 0x4373e0
	virtual void		Draw(Graphics* g) override;				// 0x437670
	virtual void		Update() override;						// 0x4374f0
	virtual void		MouseEnter() override;					// 0x437410
	virtual void		MouseLeave() override;					// 0x437440
	virtual void		MouseDown(int x, int y, int theClickCount) override;	// 0x437470
	virtual void		MouseUp(int x, int y) override;			// 0x4374b0
};

}

#endif
