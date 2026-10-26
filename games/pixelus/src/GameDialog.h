#ifndef __PIXELUS_GAMEDIALOG_H__
#define __PIXELUS_GAMEDIALOG_H__

// GameDialog.cpp: the base of Pixelus' dialogs (Pixelus.exe 0x41c260-0x41c750) and the helpers that style a
// button from the game's button strips (0x431380, 0x4314e0) and drive its music (0x4311f0-0x431250).
// Original: class GameDialog : public Dialog (2004), vftable 0x50e9a8 (53 slots), ButtonListener part 0x50b3b4;
// own data +0x114 mDecoration, +0x118 mApp. The three dialog slots it overrides are Move (14), Draw (16)
// and MouseDown (27), and ButtonPress in the listener part.

#include "Compat2004.h"

namespace Sexy
{

class DialogButton;
class ButtonListener;
class Dialog;
class Widget;
class Graphics;
class Image;
class PixelusApp;

class GameDialog : public Dialog2004
{
public:
	Image*			mDecoration;        // +0x114 optional picture over the top edge (the temple-heads dialogs), or NULL
	PixelusApp*		mApp;               // +0x118

public:
	GameDialog(PixelusApp* theApp, Image* theComponentImage, Image* theButtonComponentImage, int theId,
		bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines,
		const SexyString& theDialogFooter, int theButtonMode, Image* theDecoration);   // 0x41c260

	// 0x41c380 / 0x41c3a0 / 0x41c3c0: the content area in screen coordinates (the subclasses lay their widgets out in it)
	int				GetContentLeft();
	int				GetContentTop();
	int				GetContentWidth();

	virtual void	Move(int theNewX, int theNewY) override;                        // 0x41c470 slot 14
	virtual void	Draw(Graphics* g) override;                                     // 0x41c4d0 slot 16
	virtual void	MouseDown(int x, int y, int theClickCount) override;            // 0x41c3e0 slot 27
	virtual void	ButtonPress(int theId) override;                                // 0x41c440 (listener part)

	using Dialog2004::MouseDown;
};

// 0x431380: cut a three-frame button strip (normal, over, down) out of the button's component image,
// give it the button font and the golden hover colour, and size an unsized button to its label
void StyleButton(PixelusApp* theApp, DialogButton* theButton, int theFrames);
// 0x4314e0: place a dialog (width, content height from the dialog itself) and style its two buttons
void SizeDialog(PixelusApp* theApp, Dialog* theDialog, int theMinWidth);
// an image button cut from a three-frame strip (normal, over, down) at a place (the original does it with 0x431670)
DialogButton* MakeStripButton(PixelusApp* theApp, ButtonListener* theListener, int theId, int theImage, int theX, int theY);
// 0x431160: the colours of the game's dialogs
void SetDialogColors(Widget* theWidget);

// 0x431230 / 0x431250 / 0x4311f0: music helpers
void FadeOutAllMusic(PixelusApp* theApp);
void StartMusic(PixelusApp* theApp, int theSongId, bool theSlow);
void FadeOutMusicIfPlaying(PixelusApp* theApp, int theSongId);

}

#endif
