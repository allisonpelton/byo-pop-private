#ifndef __SPROUT2_GENERICUSERSELECTSCREEN_H__
#define __SPROUT2_GENERICUSERSELECTSCREEN_H__

// The players' list (GenericUserSelectScreen, 0x1b4 bytes, vtable 0x55dc8c (feedingfrenzy2) with 103 slots; its
// constructor 0x49fc30 (feedingfrenzy2)): a screen whose list box "userList" shows the players' names, the current one
// selected. Its owner picks the selected player or deletes them through it; buttons named "new", "delete" and "select",
// where the screen's file has them, are greyed out when they cannot be used.

#include "GenericScreen.h"
#include "ListBox.h"

namespace engine
{

class GenericUserSelectScreen : public GenericScreen
{
public:
	GenericUserSelectScreen() : mUserList(nullptr) {}

	// 0x90 (0x48e170 (feedingfrenzy2)): the layout's enable, then the current player selected and the buttons brought
	// up to date
	void enable() override;

	// 0x180 (0x48aac0 (feedingfrenzy2)): the list found and given a fresh model of the players, the current player
	// selected, and the buttons brought up to date. (The original takes an argument it does not read, and connects
	// its own actions afresh each time to a slot that does nothing, 0x4c1fe0 (feedingfrenzy2) calling the empty slot
	// 0x194; the port leaves both out)
	virtual void init();
	// 0x184 (0x493bb0 (feedingfrenzy2)): the selected player made the current one
	virtual void selectUser();
	// 0x188 (0x53f2b6 (feedingfrenzy2)): the selected player deleted, the current one selected, and the buttons
	// brought up to date
	virtual void deleteUser();
	// 0x18c (0x532ad7 (feedingfrenzy2)): as many players as the list shows rows (nothing the export shows calls it)
	virtual bool isFull();
	// 0x190 (0x4a3ef0 (feedingfrenzy2)): the rows the list shows
	virtual int getRows();
	// 0x198 (0x40da50 (feedingfrenzy2)): "new" usable while there are fewer players than rows, "delete" and "select"
	// while there are any
	virtual void updateButtons();

private:
	// The greyed look of a button that cannot be used (inline in 0x40da50 (feedingfrenzy2))
	static void setUsable(Component* theButton, bool theUsable);

	ListBox* mUserList;  // +0x1a4: "userList", the layout's child
	// +0x1a8: the list's rows, kept by init and read nowhere (getRows asks the list)
};

} // namespace engine

#endif
