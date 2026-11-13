#ifndef __HEAVYWEAPON_CREDITSSCREEN_H__
#define __HEAVYWEAPON_CREDITSSCREEN_H__

// CreditsScreen: the scrolling credits over the main menu's picture. The lines of data\credits.xml scroll up through a
// wavy black ribbon; the first click closes it. When it closes the main menu comes back with a new lamp, and an options
// dialog that was open is made again. Original: a Widget with a ButtonListener at +0x58, constructor 0x41ec80, vtable
// 0x52cd78 (listener 0x52cd58), destructor body 0x41e7b0 (deleting 0x41e940).

#include "Widget.h"
#include "ButtonListener.h"
#include "Point.h"
#include <string>
#include <vector>

namespace Sexy
{

class HeavyWeaponApp;

// One line of the credits (0x20 bytes: its text and its kind)
struct CreditLine
{
	std::string	mText;		// +0x00
	int			mType;		// +0x1c a CREDIT_ kind
};

class CreditsScreen : public Widget, public ButtonListener
{
public:
	enum
	{
		NUM_RIBBON_POINTS = 16,
		// The kinds of line (the credits file's tags)
		CREDIT_HEADING = 0,		// h: centred, in the large font
		CREDIT_ROLE = 1,		// t: on the left, in yellow, level with the name that follows
		CREDIT_NAME = 2,		// l: in the right-hand column
		CREDIT_BLANK = 3		// b: a gap
	};

	HeavyWeaponApp*			mApp;							// +0x5c
	double					mRibbonPhase;					// +0x60 where the ribbon's waves are, 0..2 pi
	double					mScroll;						// +0x68 how far the text has scrolled: 510 at the start, 0.25 less each update
	Point					mRibbon[NUM_RIBBON_POINTS];		// +0x70 the ribbon's outline: the top edge left to right, then the bottom edge back
	std::vector<CreditLine>	mLines;							// +0xf0

public:
	CreditsScreen(HeavyWeaponApp* theApp);					// 0x41ec80
	virtual ~CreditsScreen();								// 0x41e940 (body 0x41e7b0)

	virtual void	Draw(Graphics* g) override;				// 0x41ddd0 slot 16
	virtual void	Update() override;						// 0x41dce0 slot 17
	virtual void	GotFocus() override;					// 0x42ea90 slot 19
	virtual void	MouseDown(int x, int y, int theClickCount) override;	// 0x41dcb0 slot 28

	using Widget::MouseDown;

private:
	void			LoadLines();							// 0x41e970
};

}

#endif
