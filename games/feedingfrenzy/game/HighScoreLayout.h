#ifndef __FEEDINGFRENZY_HIGHSCORELAYOUT_H__
#define __FEEDINGFRENZY_HIGHSCORELAYOUT_H__

// The screen with the best scores (HighScoreLayout, 0x168 bytes, constructor 0x434b90; the file
// config\HighScore.xml): the ten best scores of the list of the normal game or of the timed game, and for the normal
// game the food bank's count and rank.

#include "ScoreList.h"
#include "ScreenLayout.h"

namespace feedingfrenzy
{

class HighScoreLayout : public engine::ScreenLayout
{
public:
	HighScoreLayout();  // 0x434b90

	// slot 0xfc: 0x4343c0: the lists the screen shows (the controller keeps them); the normal game's shows
	virtual void setLists(ScoreList* theNormalList, ScoreList* theTimedList);
	// slot 0x100: 0x434460: the ten scores of the list that shows are put on the screen, with the mode's name
	virtual void refresh();
	// slot 0x104: 0x434420: the list to show: the normal game's (mode 0) or the timed game's
	virtual void setMode(int theMode);
	// slot 0x108 and 0x10c (both 0x4343f0): the list that is not shown shows (there are two lists, so the next page
	// and the one before are the same)
	virtual void nextPage();
	virtual void prevPage();

private:
	ScoreList* mNormalList;  // +0x14c: not counted
	ScoreList* mTimedList;   // +0x150: not counted
	ScoreList* mShownList;   // +0x154: the one of the two that shows
	int mFirstRank;          // +0x15c: the place of the first score shown (from 0), which is 0 at all times
};

} // namespace feedingfrenzy

#endif
