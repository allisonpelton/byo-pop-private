#ifndef __FEEDINGFRENZY_HIGHSCORELAYOUT_H__
#define __FEEDINGFRENZY_HIGHSCORELAYOUT_H__

// The screen with the best scores (HighScoreLayout, 0x168 bytes, constructor 0x434b90; the file
// config\HighScore.xml): the lists of the normal and of the timed game, a page of ten at a time.

#include "ScoreList.h"
#include "ScreenLayout.h"

namespace feedingfrenzy
{

class HighScoreLayout : public engine::ScreenLayout
{
public:
	HighScoreLayout();  // 0x434b90

	// slot 0xfc: the lists the screen shows (the controller keeps them)
	virtual void setLists(ScoreList* theNormalList, ScoreList* theTimedList);
	// slot 0x100: the page's names and scores are put on the screen
	virtual void refresh();
	// slot 0x104: the list to show: the game's mode
	virtual void setMode(int theMode);
	// slot 0x108 and 0x10c: the next page and the one before
	virtual void nextPage();
	virtual void prevPage();
};

} // namespace feedingfrenzy

#endif
