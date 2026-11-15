#ifndef __SPROUT_GENERICUSERSELECTSCREEN_H__
#define __SPROUT_GENERICUSERSELECTSCREEN_H__

// The signal build's screen for choosing a player (Pizza Frenzy's; 0x1e0 bytes, constructor 0x4732a0 (pizzafrenzy)):
// a list of the players (the node "userList"), and the buttons "new", "delete" and "ok", which are on while they can
// be used.

#include "GenericScreen.h"
#include "ListBox.h"

namespace engine
{

class GenericUserSelectScreen : public GenericScreen
{
public:
	// 0x88: 0x473190 (pizzafrenzy): the list shows the current player picked
	void connect() override;

	// 0x17c: 0x4738a0 (pizzafrenzy): the list gets the players as its model
	virtual void setup();
	// 0x180: 0x4731c0 (pizzafrenzy): the player picked in the list is the current one
	virtual void choosePlayer();
	// 0x184: 0x4731e0 (pizzafrenzy): the player picked in the list is deleted
	virtual void removePlayer();
	// 0x188: 0x473220 (pizzafrenzy): the list has no room for another player
	virtual bool isListFull();
	// 0x190: 0x473210 (pizzafrenzy): a row of the list was picked
	virtual void onRowPicked(int theRow) { refreshButtons(); }
	// 0x194: 0x473460 (pizzafrenzy): each button is on or off, by what the list holds and has picked
	virtual void refreshButtons();
	// 0x4736e0 (pizzafrenzy): the name of the player picked, "" when none
	std::string getPickedName();

protected:
	ListBox* mUserList;  // +0x1d8
	int mVisibleRows;    // +0x1dc: how many rows the list shows
};

} // namespace engine

#endif
