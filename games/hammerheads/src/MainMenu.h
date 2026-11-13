#ifndef __HAMMERHEADS_MAINMENU_H__
#define __HAMMERHEADS_MAINMENU_H__

// The main menu: the picture of the screen with the hammer swinging over it and the buttons, which slide in from the sides one
// after the other (the three ways to play, the options, the quit button, and the high scores and the trophy room in the corners).
// A gnome peeks in from below every so often; a click on it gives the next games a head start. Under the buttons the name of the
// current player is shown, with a link to change it; while the mouse is over a button, a description of it takes the place.
// Original: class Sexy::MainMenu : public Widget, public ButtonListener (0x100 bytes; the constructor is 0x459a60, the destructor 0x459890).

#include <vector>

#include "ButtonListener.h"
#include "Widget.h"

namespace Sexy
{

class HyperlinkWidget;
class Image;
class NcButton;

class MainMenu : public Widget, public ButtonListener
{
public:
	// the ids of the buttons (6 was the "More Games" button, which the port does not have)
	enum
	{
		BUTTON_PLAY_CLASSIC = 0,
		BUTTON_PLAY_TOUGH = 1,
		BUTTON_PLAY_MARATHON = 2,
		BUTTON_OPTIONS = 3,
		BUTTON_HIGH_SCORES = 4,
		BUTTON_TROPHY_ROOM = 5,
		BUTTON_QUIT = 7,
		BUTTON_USER = 8,
		BUTTON_PEEKABOO = 9
	};

	// A button and where it goes: it waits, then slides from one place to the other (the peekaboo gnome goes back and forth)
	struct ButtonSlide
	{
		Widget*			mButton;		// +0x00
		double			mDelay;			// +0x08: counts down to 0 before the slide starts
		double			mProgress;		// +0x10: 0 to 1
		int				mFromX;			// +0x18
		int				mFromY;			// +0x1c
		int				mToX;			// +0x20
		int				mToY;			// +0x24
	};

	std::vector<ButtonSlide>	mSlides;		// +0x84 (the original's own array of pointers to them)
	double			mTime;					// +0x98: runs up by the frame time while the window has the focus (the hammer swings with it)
	NcButton*		mPlayClassicButton;		// +0xac
	NcButton*		mPlayToughButton;		// +0xb0: only there once the player has unlocked it
	NcButton*		mPlayMarathonButton;	// +0xb4: locked (a different picture, and it says so when pressed) until three levels are done
	NcButton*		mOptionsButton;			// +0xb8
	NcButton*		mHighScoresButton;		// +0xbc
	NcButton*		mTrophyRoomButton;		// +0xc0
	NcButton*		mQuitButton;			// +0xc8
	NcButton*		mPeekabooButton;		// +0xcc: null when the head start has been taken this time
	HyperlinkWidget*	mUserLink;			// +0xd0: "( If this is not you, click here )"; its alpha is also that of the welcome text

public:
	MainMenu();																		// 0x459a60
	virtual ~MainMenu();															// 0x459890

	// 0x4594e0: the picture of the tough cookie button
	void			SetPlayButtonImage(Image* theImage);
	// 0x459520, 0x459540: the player has changed: the tough cookie button is there or not, the marathon button is locked or not
	void			UserChanged();

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;			// 0x459360
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;		// 0x459410
	virtual void	Update() override;												// 0x459280
	virtual void	UpdateF(float theFrac) override;								// 0x459580
	virtual void	Draw(Graphics* g) override;										// 0x45ad20
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x459830
	virtual void	KeyChar(SexyChar theChar) override;								// 0x459500
	virtual void	ButtonPress(int theId) override;								// 0x4594c0 (the ButtonListener part)
	using ButtonListener::ButtonPress;
	virtual void	ButtonDepress(int theId) override;								// 0x45aa50

private:
	void			AddSlide(Widget* theButton, int theFromX, int theFromY, int theToX, int theToY, double theDelay);
	NcButton*		GetOverButton() const;
	void			DrawButtonCaption(int theButtonId, Graphics* g, int theAlpha);	// 0x45a5e0
};

}

#endif
