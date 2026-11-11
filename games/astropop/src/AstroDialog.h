#ifndef __ASTROPOP_ASTRODIALOG_H__
#define __ASTROPOP_ASTRODIALOG_H__

// AstroDialog: the game's dialog box (original class of 0x194 bytes, constructor 0x489c60 with the setup 0x483ee0,
// destructor 0x48a640, vftable 0x56b670, a Dialog with five more virtuals). Every box of the game is one: the question
// boxes, the message boxes, the name entry and the screens of text that type themselves out. A box slides up from the
// bottom of the screen when it opens (the opening states 8, 4 and 3 of Update) unless it is asked to open settled; the
// colours of the inset picture and of the buttons come from the colour set of the game mode it belongs to
// (AstroPopApp::GetDialogColor, MakeDialogImages). Offsets in the comments are the original's.

#include "Text2004.h"
#include "Dialog.h"
#include "Color.h"
#include <string>
#include <vector>

namespace Sexy
{

class AstroPopApp;
class MemoryImage;

class AstroDialog : public Text2004<Dialog>
{
public:
	int					mTitleEndY; // +0x110 where the title ends up (y) when the box has come in
	int					mBoxEndY; // +0x114 where the box ends up (y; 0)
	int					mTitleY; // +0x118 the title's y now
	int					mTitleX; // +0x11c the title's x (centred)
	int					mBoxX; // +0x120 the box's x (0)
	int					mBoxY; // +0x124 the box's y now (it starts at 480, below the screen)
	int					mState;         // +0x128 the opening: 8 (the box comes up), 4 (it sinks back a little), 3 (it settles)
	int					mTypedLine;     // +0x12c the line being typed out
	int					mTypedChars;    // +0x130 the characters of that line typed so far, less one
	int					mImageTop;      // +0x134 the title picture's top
	bool				mSettled;       // +0x138 the box has come in: the base class draws it
	bool				mCursorOn;      // +0x139 the cursor of the typing is shown (it blinks every 50 updates)
	bool				mTyping;        // +0x13a the box types its lines (SetTyping)
	bool				mColoured;      // +0x13b the box has its own pictures in the colours it was given
	AstroPopApp*		mApp;           // +0x13c
	Color				mInsetColor;    // +0x140
	Color				mButtonColor;   // +0x150
	MemoryImage*		mInsetImage;    // +0x160 the inset picture in the colour (the box's component image)
	MemoryImage*		mButtonImage;   // +0x164 the buttons' picture in the colour
	std::vector<Font*>	mLineFonts;     // +0x168 the font of a typed line (SetLineFont); none: the lines font
	bool				mCursorEnabled; // +0x178 the cursor of the typing may show
	int					mTypeSpeed;     // +0x17c updates per typed character (below 1: all lines at once)
	std::vector<std::string>	mTypedLines;    // +0x180 the lines to type out
	Image*				mPicture;       // +0x190 a picture that takes the place of the title (NULL: the title is text)

public:
	AstroDialog(AstroPopApp* theApp, int theId, const std::string& theHeader, int theButtonMode, bool theColoured,
				const Color& theInsetColor, const Color& theButtonColor);   // 0x489c60, setup 0x483ee0
	// The box with a picture for its title (the upgrade box), no header text: the picture's top is theImageTop
	AstroDialog(AstroPopApp* theApp, int theId, Image* thePicture, int theImageTop, int theButtonMode, bool theColoured,
				const Color& theInsetColor, const Color& theButtonColor);   // 0x489e20, setup 0x483ee0
	virtual ~AstroDialog();                                                 // 0x48a640 (body 0x48a030)

	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x483640
	virtual void	Draw(Graphics* g) override;                                         // 0x487a70
	virtual void	Update() override;                                                  // 0x484240
	virtual void	MouseDrag(int x, int y) override;                                   // 0x483750
	virtual int		GetPreferredHeight(int theWidth) override;                          // 0x4840c0

	void			SetButtonLabels(const std::string& theYes, const std::string& theNo);   // 0x4879d0
	void			SetAnimated(bool theAnimated);                                          // 0x483560 false: the box is settled at once
	void			SetTyping(bool theTyping);                                              // 0x487970
	void			SetLineFont(Font* theFont, unsigned int theLine);                       // 0x4897b0

private:
	void			Setup(AstroPopApp* theApp, bool theColoured, const Color& theInsetColor, const Color& theButtonColor);   // 0x483ee0
	int				HeaderWidth();      // the width of what stands for the title: the picture or the header text
};

// 0x40bab0: the look every box of the game has (fonts, insets, colours of the text)
void StyleDialog(Dialog* theDialog);

}

#endif
