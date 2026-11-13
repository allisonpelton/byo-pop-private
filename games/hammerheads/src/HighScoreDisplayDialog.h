#ifndef __HAMMERHEADS_HIGHSCOREDISPLAYDIALOG_H__
#define __HAMMERHEADS_HIGHSCOREDISPLAYDIALOG_H__

// The high score table (dialog 0xe): two tables of ten places (classic and tough cookie games share the first, marathon has the
// second) and each table's best score of the day. It reads users\hiscores.dat when it is made and writes it when it is deleted,
// so the application makes one for every finished game to record the score, and shows it to the player who made a place.
// Original: class Sexy::HighScoreDisplayDialog : public Dialog (0x450 bytes; the constructor is 0x456080, the destructor 0x4563d0).

#include <string>

#include "Dialog.h"

namespace Sexy
{

class HighScoreDisplayDialog : public Dialog
{
public:
	enum
	{
		NUM_TABLES = 2,
		NUM_PLACES = 10
	};

	int				mTodayTable;					// +0x148: the table whose best of the day was just beaten (-1 for none)
	int				mNewTable;						// +0x14c: the table of the place just made (-1 for none)
	int				mNewIndex;						// +0x150: the place just made, from 0
	bool			mShown;							// +0x155: the table is on screen for the player (not only made to record a score)
	double			mTime;							// +0x158: runs up by 0.01 a step; the colours of the new lines move with it
	std::string		mTodaysName[NUM_TABLES];		// +0x160: who has the best score of the day
	int				mTodaysScore[NUM_TABLES];		// +0x198
	bool			mTodaysCookie;					// +0x1a0: it was a tough cookie game (the first table only)
	std::string		mSubtitle;						// +0x1a4
	std::string		mNames[NUM_TABLES][NUM_PLACES];	// +0x1c0
	int				mScores[NUM_TABLES][NUM_PLACES];	// +0x3f0
	bool			mCookies[NUM_PLACES];			// +0x440: the place was made by a tough cookie game (the first table only)

public:
	HighScoreDisplayDialog();												// 0x456080
	virtual ~HighScoreDisplayDialog();										// 0x4563d0

	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x454a50
	virtual int		GetPreferredHeight(int theWidth) override;				// 0x454c20
	virtual void	Update() override;										// 0x454b70
	virtual void	Draw(Graphics* g) override;								// 0x455550
	virtual void	ButtonPress(int theId) override;						// 0x454ba0
	virtual void	ButtonDepress(int theId) override;						// 0x454be0

	// 0x456490: when the score makes a place, the box that tells so (dialog 0x10); to be called before the score is recorded
	void			Prepare();

	// 0x455f00: the score of the game goes into its table (and is today's best when it beats it); nothing happens for no name
	void			RecordScore(const std::string& theName);

private:
	int				GetPlace(int theTable, int theScore) const;				// 0x454ab0: 1 to 10, or 0 for a score that makes none
	void			Load();													// 0x454c60
	void			Save();													// 0x455080
};

// 0x456810: the table is made, sized to the screen and added as dialog 0xe
HighScoreDisplayDialog* MakeHighScoreDisplay();

}

#endif
