#ifndef __PODDIALOG_H__
#define __PODDIALOG_H__

// PodDialog.h: the dialogs and buttons of Bejeweled 2's own look (0x5cde4b-0x5cf059): Sexy::PodButton (the glowing
// round buttons of the menus), Sexy::PodDialogButton (the buttons of a dialog) and Sexy::PodDialog (a dialog with the
// glowing header). The offsets in the comments are the original's, from the start of the object.

#include "Dialog.h"
#include "DialogButton.h"
#include "ButtonWidget.h"

namespace Sexy
{

class WinDMApp;
class MemoryImage;

// 0x5cde4b
class PodButton : public ButtonWidget
{
public:
	Image*			mGlowImage;         // +0x120 drawn additively as the button hovers or is selected
	Image*			mSelectedGlowImage; // +0x124 drawn additively while the button is selected
	bool			mSelected;          // +0x128
	Color			mGlowColor;         // +0x12c the color the glow images are drawn in
	float			mSelectedGlow;      // +0x13c the glow of a selected button
	float			mSelectedHoverGlow; // +0x140 the glow of a selected button the mouse is over
	float			mPressedGlow;       // +0x144 the glow when the button is pressed
	float			mHoverGlow;         // +0x148 the glow of a button the mouse is over
	float			mGlow;              // +0x14c the glow now (hover)
	float			mSelectedGlowNow;   // +0x150 the glow now (selected)
	bool			mSounds;            // +0x154 the button plays its mouse-over and mouse-leave sounds
	bool			mBlinking;          // +0x155
	bool			mBlinkOn;           // +0x156
	int				mBlinkCount;        // +0x158 blinks left (0 for no limit)
	int				mBlinkPeriod;       // +0x160 updates per blink
	int				mBlinkTimer;        // +0x15c
	float			mBlinkGlow;         // +0x164 the glow of a button while it blinks
	float			mUnusedGlow;        // +0x168 (set, never read)
	float			mUnusedGlow2;       // +0x16c (set, never read)

public:
	PodButton(int theId, ButtonListener* theListener);
	virtual ~PodButton();

	virtual void	Update() override;
	virtual void	Draw(Graphics* g) override;
	virtual void	MouseEnter() override;
	virtual void	MouseLeave() override;
};

// 0x5cec97
class PodDialogButton : public DialogButton
{
public:
	Image*			mGlowImage;         // +0x138 the box drawn additively as the mouse is over the button
	MemoryImage*	mTextGlow;          // +0x13c the label in the glow font, made on the first Draw
	float			mGlow;              // +0x140
	float			mAlpha;             // +0x144

public:
	PodDialogButton(Image* theComponentImage, Image* theGlowImage, int theId, ButtonListener* theListener);
	virtual ~PodDialogButton();

	virtual void	Update() override;
	virtual void	Draw(Graphics* g) override;
};

// 0x5ce341
class PodDialog : public Dialog
{
public:
	WinDMApp*		mApp;               // +0x150
	SexyString		mHeaderText;        // +0x154 the header (the Dialog's own header stays empty)
	MemoryImage*	mHeaderGlow;        // +0x170 the header in the glow font
	bool			mEscaped;           // +0x174 the player closed the dialog with the Escape key (or the cancel button)
	PodDialogButton* mCancelButton;     // +0x178
	bool			mHasCancel;         // +0x17c
	int				mHeaderWidth;       // +0x180 the width of the header box, 0 for the standard width

public:
	PodDialog(WinDMApp* theApp, int theId, bool isModal, const SexyString& theHeader, const SexyString& theLines,
		const SexyString& theFooter, int theButtonMode);
	virtual ~PodDialog();

	void			SetHasCancel(bool theHasCancel);                // 0x5ce30e
	virtual int		GetPreferredHeight(int theWidth) override;      // 0x5ce341
	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x5ce378
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x5ce3a4
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x5ce3d0
	virtual void	KeyDown(KeyCode theKey) override;               // 0x5ce4db
	virtual void	ButtonPress(int theId) override;                // 0x5ce514
	virtual void	ButtonDepress(int theId) override;              // 0x5ce52b
	virtual void	Draw(Graphics* g) override;                     // 0x5ce86a

	void			GetHeaderRect(Rect& theRect);                   // 0x5ce560
	void			DrawEditBox(Graphics* g, const Rect& theRect);  // 0x5ce5f0
	void			DrawTopText(Graphics* g);                       // 0x5ce703
	void			CreateFontGlowImages();                         // 0x5ce767
	DialogButton*	MakePodButton(DialogButton* theOldButton);      // 0x5ce9aa
};

}

#endif
