#ifndef __ASTROPOP_HALLOFFAME_H__
#define __ASTROPOP_HALLOFFAME_H__

// HallOfFame: the screen of the best results (original class of 0x90 bytes, constructor 0x427e30, destructor body 0x45c2c0,
// vftable 0x562648, a ButtonListener at +0x58 with vftable 0x562628). A hundred stars fall past a picture with two lists on
// it. The all-time page shows the hall of fame of every player (five results of the classic game and five of the survival
// game, each with its pilot), the personal page the best results of the player who is signed in (the pilot, a gauge of how
// far that pilot has got and the score; the application's ranking page at the end of a game shows the same lists). A
// button on the left turns the page, one on the right closes the screen (the application does that: CloseHallOfFame).
// Offsets in the comments are the original's.

#include "ButtonListener.h"
#include "Color.h"
#include "Widget.h"
#include <string>
#include <vector>

namespace Sexy
{

class AstroButton;
class AstroPopApp;
class Font;

class HallOfFame : public Widget, public ButtonListener
{
public:
	enum
	{
		NUM_STARS = 100,
		BUTTON_ID_CLOSE = 0xfb2,        // the right button
		BUTTON_ID_PAGE = 0xfb3          // the left button: the other page
	};

	// a star (0x1c bytes): where it is, how many pixels it falls in an update and its colour
	struct Star
	{
		float		mX;             // +0x00
		float		mY;             // +0x04
		float		mSpeed;         // +0x08
		Color		mColor;         // +0x0c

		Star() : mX(0), mY(0), mSpeed(0) {}
	};

	std::vector<Star>	mStars;             // +0x70
	AstroPopApp*		mApp;               // +0x7c
	AstroButton*		mCloseButton;       // +0x80
	AstroButton*		mPageButton;        // +0x84
	int					mPage;              // +0x88 0 the hall of fame, 1 the player's own results
	bool				mInactive;          // +0x8c the window has lost the focus: the stars stand still

public:
	explicit HallOfFame(AstroPopApp* theApp);                                   // 0x427e30
	virtual ~HallOfFame();                                                      // 0x427eb0 (body 0x45c2c0)

	virtual void	AddedToManager(WidgetManager* theManager) override;         // 0x455c90 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;     // 0x4350e0 slot 2
	virtual void	Draw(Graphics* g) override;                                 // 0x44bf80 slot 16
	virtual void	Update() override;                                          // 0x43c340 slot 17
	virtual void	ButtonDepress(int theId) override;                          // 0x435140 (the listener's)

private:
	static std::string	FitName(Font* theFont, const std::string& theName);    // 0x446840
};

}

#endif
