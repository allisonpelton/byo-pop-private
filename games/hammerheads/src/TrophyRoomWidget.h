#ifndef __HAMMERHEADS_TROPHYROOMWIDGET_H__
#define __HAMMERHEADS_TROPHYROOMWIDGET_H__

// The trophy room: the twenty-one trophies on a shelf (a trophy the player has not won is a dark shape), a line of text under them
// that names the one the mouse is over, and a button back to the menu or the game. A click on a trophy that has been won shows
// it close up. When it opens after a game (mode 1), the trophies just won are announced first, one page for each, and the shelf shows
// them flashing.
// Original: class Sexy::TrophyRoomWidget : public Widget, public ButtonListener (0xd0 bytes; the constructor is 0x460c00, the
// destructor 0x460600).

#include <vector>

#include "ButtonListener.h"
#include "Widget.h"

namespace Sexy
{

class NcButton;
class TrophyAppearButton;
class TrophyButton;
class TrophyCloseUpButton;

class TrophyRoomWidget : public Widget, public ButtonListener
{
public:
	enum
	{
		MODE_MENU = 0,				// from the main menu: back to it
		MODE_GAME = 1				// at the end of a game: the trophies won are announced, and back to the game
	};

	enum
	{
		BUTTON_BACK = 0,
		BUTTON_CLOSE_UP = 1,
		BUTTON_APPEAR = 2,
		BUTTON_FIRST_TROPHY = 3		// the trophies' buttons count up from here
	};

	std::vector<TrophyButton*> mTrophyButtons;	// +0x84: one for each trophy, by the trophy's number
	NcButton*		mBackButton;			// +0x94: "Back to Main Menu" or "Back to Game"
	TrophyCloseUpButton* mCloseUp;			// +0x98
	TrophyAppearButton* mAppear;			// +0x9c
	int				mOverTrophy;			// +0xa4: the trophy the mouse is over (-1 for none)
	int				mMode;					// +0xac
	TrophyButton*	mSelected;				// +0xb8: the trophy that is shown close up (its button is hidden meanwhile)
	std::vector<int> mNewTrophies;			// +0xbc: the trophies just won, in the order they are announced
	int				mNewIndex;				// +0xcc: the one being announced (-1 for none)

public:
	// theNewTrophies: the ones just won, which the player is given here (MODE_GAME)
	TrophyRoomWidget(int theMode, const std::vector<int>& theNewTrophies);			// 0x460c00
	virtual ~TrophyRoomWidget();													// 0x460600

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x4608d0
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x460940
	virtual void	Update() override;											// 0x460b40
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x460760
	virtual void	Draw(Graphics* g) override;									// 0x461290
	virtual void	ButtonDepress(int theId) override;							// 0x4609b0 (the ButtonListener part)

private:
	void			ShowShelf(bool theShow);										// 0x4606f0: the trophies and the back button, and the page behind them
	void			ShowCloseUp(bool theShow);										// 0x4605a0
	void			AnnounceNext();													// 0x460730
	void			DrawTrophyText(Graphics* g, int theTrophy, int theY);			// 0x4610a0
};

}

#endif
