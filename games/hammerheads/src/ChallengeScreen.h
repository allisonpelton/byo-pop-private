#ifndef __HAMMERHEADS_CHALLENGESCREEN_H__
#define __HAMMERHEADS_CHALLENGESCREEN_H__

// The challenge (0x44c170-0x44e110): the bonus round a challenge gnome opens. Sixteen cubbies in a slanted grid send up a coin
// gnome each, one after the other, and the player hits them all before the round's time is out; the screen pauses the level
// behind it (its objects are deactivated, the round's own are made), counts "N Bonus Round!", "Ready?" and "Go!", and ends in
// a box that pays three coins for each gnome and fifty-two more for a round without a miss.
// Original: class Sexy::ChallengeScreen : public Widget (sizeof 0xb8, made by 0x41e8b0).

#include <string>
#include <vector>

#include "ButtonListener.h"
#include "CubbyContainer.h"
#include "Widget.h"

namespace Sexy
{

class Dialog;
class Image;
class NcButton;

class ChallengeScreen : public Widget, public ButtonListener, public CubbyContainer
{
public:
	enum
	{
		NUM_COLUMNS = 4,
		NUM_ROWS = 4
	};

	// the sixteen cubbies are the container's list (the container's part starts at +0x84)
	double			mFade;					// +0x98: how far the screen is dimmed (0 to 1; it is also what the application hands to the dialogs)
	bool			mFinished;				// +0xa0: every cubby has had its gnome; the closing box has been made
	bool			mReadySaid;				// +0xa1: the "Ready" voice has played
	bool			mGoSaid;				// +0xa2: the "Go" voice has played
	bool			mRunning;				// +0xa3: not paused (a lost focus pauses it until the next click)
	double			mCountdown;				// +0xa8: the seconds-ish before the round starts, from 5 down to 0
	bool			mClicksCleared;			// +0xb0: the hammer's waiting click was dropped when the round began
	NcButton*		mMenuButton;			// +0xb4

public:
	ChallengeScreen();									// 0x44d800
	virtual ~ChallengeScreen();							// 0x44d290

	// theLoaded: the screen is made again from a saved game (no "Bonus Round!" box)
	void			Opened(bool theLoaded);				// vfd8 (0x44d360; 0x41f180 calls it right after making the screen)
	void			Closing();							// vfdc (0x44ccf0; 0x421d70 calls it before the screen is deleted)
	void			StartChallenge();					// 0x44cd80: the bonus round dialog was answered (or the pause was clicked away)
	int				GetCreatureKind();					// vfe0 (0x44cd40): the kind of every gnome the round sends up (a big coin)
	std::string		SaveExtra();						// vfe4 (0x44d5d0): what a saved game keeps of the screen (a ValueList's text)
	void			LoadExtra(const std::string& theData);	// vfe8 (0x44d6e0)

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x44cc40
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x44cc70
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x44cca0: the menu button stays in the top right corner
	virtual void	UpdateF(float theFrac) override;				// 0x44ce90
	virtual void	Draw(Graphics* g) override;						// 0x44d950
	virtual void	LostFocus() override;							// 0x44cdc0 (the original does not override GotFocus)
	virtual void	MouseEnter() override;							// 0x44cbb0
	virtual void	MouseLeave() override;							// 0x44cc00
	virtual void	MouseDown(int x, int y, int theClickCount) override;	// 0x44ce10
	using Widget::MouseDown;
	virtual void	ButtonDepress(int theId) override;				// 0x44cd50 (the ButtonListener part)

	virtual Image*	GetBackdrop() override;							// 0x44cd30 (the container's slot 0): the round's own backdrop
};

// 0x44c9e0 (also what Application's 0x41f0f0 forwards to): the box that closes the round. It pays the coins into the
// stats, so it is made once for each round (and once more for a saved game that was in it)
Dialog* MakeChallengeFinishedDialog();

}

#endif
