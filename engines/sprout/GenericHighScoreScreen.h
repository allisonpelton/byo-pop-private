#ifndef __SPROUT_GENERICHIGHSCORESCREEN_H__
#define __SPROUT_GENERICHIGHSCORESCREEN_H__

// The signal build's screen for a scoreboard (Pizza Frenzy's; 0x1f0 bytes, constructor 0x46b880 (pizzafrenzy)): the
// scoreboard's rows in the table "scoreTable", which two buttons scroll, as long as they are held down.

#include "GenericScreen.h"
#include "ScoreboardTableModel.h"
#include "Table.h"

namespace engine
{

class GenericHighScoreScreen : public GenericScreen
{
public:
	GenericHighScoreScreen();  // 0x46b880 (pizzafrenzy)

	// 0x17c: 0x46bbd0 (pizzafrenzy): the table shows the scoreboard
	virtual void setScoreboard(Scoreboard* theBoard);

	// 0x94: 0x46b9f0 (pizzafrenzy): a held scroll button scrolls again each tenth of a second
	void update(UpdateInfo& theInfo) override;
	// The button listener (+0x144, vtable 0x4f95c0 (pizzafrenzy)): 0x46ba50 (pizzafrenzy), a scroll button pressed
	// scrolls at once and starts the wait before it repeats
	void buttonDown(std::string theName) override;
	// 0x46bb20 (pizzafrenzy)
	void buttonUp(std::string theName) override;

protected:
	// 0x46b9a0 (pizzafrenzy): the table's top row moves by the direction, when that leaves its rows on the board
	void scroll();

	int mScrollDirection;            // +0x1d8: -1 up, 1 down, 0 none
	float mRepeatTimer;              // +0x1dc: the seconds until the next scroll
	Ref<Scoreboard> mScoreboard;     // +0x1e0
	Ref<ScoreboardTableModel> mTableModel;  // +0x1e4
	Ref<Table> mTable;               // +0x1e8: the node "scoreTable"
};

} // namespace engine

#endif
