#ifndef __ASTROPOP_CREDITSSCREEN_H__
#define __ASTROPOP_CREDITSSCREEN_H__

// CreditsScreen: the credits scroll up the screen (original class of 0x80 bytes, constructor 0x4563a0, destructor body
// 0x43c510, vftable 0x5641b8) and start again when the last line has gone. The screen fades in from white, a button in the
// lower right corner brings the selector back (the application does that: button 0xfb6). A copy that is registered
// adds a title for the player and the registered name under the last line; the port has no such copy, so the lines are
// left out (the random number that picked the title is still drawn, so the sequence of random numbers stays the
// original's). Offsets in the comments are the original's.

#include "Widget.h"
#include <string>
#include <vector>

namespace Sexy
{

class AstroButton;
class AstroPopApp;
class Font;
class MemoryImage;

class CreditsScreen : public Widget
{
public:
	// one line of the list: its font, its text and the gap that follows it
	struct Line
	{
		Font*			mFont;
		std::string		mText;
		int				mGap;
	};

	int				mScrollY;           // +0x58 where the first line is, going up one pixel for two updates
	AstroPopApp*	mApp;               // +0x5c
	AstroButton*	mBackButton;        // +0x60
	std::vector<Line>	mLines;         // the original makes the list (as a static) the first time it draws

public:
	explicit CreditsScreen(AstroPopApp* theApp);                                // 0x4563a0
	virtual ~CreditsScreen();                                                   // 0x43d960 (body 0x43c510)

	virtual void	AddedToManager(WidgetManager* theManager) override;         // 0x435940 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;     // 0x435990 slot 2
	virtual void	Draw(Graphics* g) override;                                 // 0x446a60 slot 16
	virtual void	Update() override;                                          // 0x4359c0 slot 17

private:
	void			MakeLines();
};

}

#endif
