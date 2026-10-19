#ifndef __BOOKWORM_WORDBALLOON_H__
#define __BOOKWORM_WORDBALLOON_H__

// WordBalloon.cpp: the speech / thought balloon above Lex. One instance, OverlayWidget::mBalloon, made by
// OverlayWidget::AddedToManager (IMAGE_BACKDROP_WB_TEXT, IMAGE_WB_TEXT_TALK_TAIL, Point(159, -2), then
// Resize(0, 0, 213, GetPreferredHeight(213)) and AddWidget) and shared by every Lex message: Lex::Say /
// SayDefinition give it the 9-slice box and the talk tail, Lex::Think the whole thought image. Up to three
// word-wrapped text blocks (header, text, footer). Show(n) fades it in and keeps it n updates (-1: until
// Show(0) or a click), then it fades out and hides; the OverlayWidget places it above Lex every update.
// Original: class Sexy::WordBalloon : public Widget, sizeof 0x120 (new(0x120) in
// OverlayWidget::AddedToManager 0x430d10), vftable 0x5016c4 (52 slots: Widget's 47, overrides 0 dtor,
// 16 Draw, 17 Update, 27 MouseDown; new 47 DrawPlain, 48 DrawBox, 49 Show, 50 GetTextHeight,
// 51 GetPreferredHeight). Offsets are absolute (the 2004 Widget is 0x58 bytes).
// Behaviour spec: re/bookworm/analysis/lex.md section 4.

#include "Widget.h"
#include "Insets.h"
#include "Point.h"
#include <string>

namespace Sexy
{

class BookWormApp;
class Graphics;
class Image;
class Font;

class WordBalloon : public Widget
{
public:
	std::string		mHeader;                // +0x58 "" first block (the definition's word)
	std::string		mText;                  // +0x74 "" second block (main text)
	std::string		mFooter;                // +0x90 "" third block; only ever cleared
	Insets			mImageInsets;           // +0xac from the images (box: top/bottom (int)((h + 0.5) * 0.33), bottom + tail h) or the plain image's insets
	Insets			mTextInsets;            // +0xbc padding chosen by the caller (Say 8,0,8,0; SayDefinition 12,0,12,0; Think 0; OverlayWidget -5,-10,-5,0)
	Point			mTailPos;               // +0xcc tail position, y relative to the box bottom
	BookWormApp*	mApp;                   // +0xd4 Init
	Image*			mPlainImage;            // +0xd8 thought image drawn whole at (0, 0); excludes the box images
	Image*			mBoxImage;              // +0xdc 9-slice box (speech balloon)
	Image*			mTailImage;             // +0xe0 tail under the box; its left half is drawn
	Font*			mHeaderFont;            // +0xe4 NULL
	Font*			mTextFont;              // +0xe8 NULL
	Font*			mFooterFont;            // +0xec NULL
	int				mHeaderJustify;         // +0xf0 -1 left, 0 centre, 1 right
	int				mTextJustify;           // +0xf4 -1
	int				mFooterJustify;         // +0xf8 -1
	// +0xfc pad
	double			mAlpha;                 // +0x100 0.0; current opacity
	double			mTargetAlpha;           // +0x108 0.0
	double			mFadeSpeed;             // +0x110 0.1 per update
	int				mTimer;                 // +0x118 0; updates left while > 0; -1 = until Show(0) or a click
	bool			mClicked;               // +0x11c false; dismissed by a click (ends the mouse-over hold)
	// +0x11d..+0x11f pad; sizeof 0x120

public:
	WordBalloon(BookWormApp* theApp, Image* theBoxImage, Image* theTailImage, Point theTailPos); // 0x4372a0
	virtual ~WordBalloon();                                         // 0x4373a0 (deleting dtor 0x437370) slot 0

	// Slots 1, 2, 4, 10 (0x4363b0, 0x4363c0, 0x436350, 0x436340) and 13 (0x4334f0) only jump to the
	// Widget implementation: not overridden here.

	virtual void	Draw(Graphics* g);                              // 0x4363d0 slot 16
	virtual void	Update();                                       // 0x437080 slot 17
	virtual void	MouseDown(int x, int y, int theClickCount);     // 0x436420 slot 27
	virtual void	DrawPlain(Graphics* g);                         // 0x4367a0 slot 47
	virtual void	DrawBox(Graphics* g);                           // 0x436c40 slot 48
	virtual void	Show(int theDuration);                          // 0x436360 slot 49 (0 = fade out and hide)
	virtual int		GetTextHeight(int theWidth, const SexyString& theText, Font* theFont, int theJustify,
						int theLineSpacing = -1);                   // 0x4365d0 slot 50
	virtual int		GetPreferredHeight(int theWidth);               // 0x436680 slot 51

	using Widget::MouseDown;

	void			Init(BookWormApp* theApp);                      // 0x436260
	void			SetJustification(int theTextJustify, int theHeaderJustify, int theFooterJustify); // 0x436310
	void			SetPlainImage(Image* theImage, Insets theInsets, bool doResize);    // 0x436440
	void			SetBoxImages(Image* theBoxImage, Image* theTailImage, Point theTailPos); // 0x4364d0
	void			ClearHeader();                                  // 0x437430 (text erased, font NULL)
	void			ClearFooter();                                  // 0x437480
	void			SetHeader(const SexyString& theText, Font* theFont, int theJustify); // 0x4374e0
	void			SetText(const SexyString& theText, Font* theFont, int theJustify);   // 0x437510

protected:
	// port: the three text blocks as DrawPlain and DrawBox both draw them (the original repeats the code)
	void			DrawTextBlocks(Graphics* g, Rect theRect);
};

}

#endif
