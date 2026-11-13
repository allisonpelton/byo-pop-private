#ifndef __HAMMERHEADS_USERMANAGER_H__
#define __HAMMERHEADS_USERMANAGER_H__

// The player roster and what each player has earned (0x471380-0x473710): the trophies, the high scores, the unlocked modes.

#include <string>
#include <vector>

namespace Sexy
{

// The trophies, in the order of the exe's own list (0x42d0a0 awards them)
enum Trophy
{
	TROPHY_GNOME_CRUSHER = 0,			// 100 gnomes smashed
	TROPHY_ULTIMATE_GNOME_CRUSHER,		// 1000
	TROPHY_BLAST_MASTER,
	TROPHY_SUPREME_BLAST_MASTER,
	TROPHY_ACCURACY_AWARD,
	TROPHY_MASTER_MARKSMAN,
	TROPHY_NO_GNOME_ESCAPED,
	TROPHY_SUPPLY_CHALLENGE,
	TROPHY_GOLDEN_HEART,
	TROPHY_VOW_OF_POVERTY,
	TROPHY_HEART_HOG,
	TROPHY_SILVER_MARATHON_MEDAL,
	TROPHY_GOLD_MARATHON_MEDAL,
	TROPHY_CLASSIC_SCORE_RIBBON,
	TROPHY_MARATHON_SCORE_RIBBON,
	TROPHY_REGICIST,
	TROPHY_BONUS_BANKER,
	TROPHY_PULVERIZER,					// 2500 gnomes smashed
	TROPHY_MOGUL,						// 2500 coins
	TROPHY_POWER_PLAYER,
	TROPHY_TROPHY_MASTER,
	NUM_TROPHIES
};

class User
{
public:
	std::string		mName;						// +0x3c0
	int				mId;						// +0x3fc: the number of the player's file (users\%d.xml)
	bool			mShowHowToPlay;				// +0x3f9: the next game starts with the how-to-play movie (0x469b90, 0x469ba0)
	bool			mToughUnlocked;				// +0x3fa: Tough Cookie is open (0x469bb0, 0x469bc0)
	bool			mToughAnnounced;			// +0x3fb: the player has been told so (0x469bd0, 0x469be0)

public:
	bool			HasTrophy(int theTrophy) const;							// 0x469e40
	void			AwardTrophy(int theTrophy);								// 0x469e50

	// The game of the mode being played (gApp->mGameMode) that "Continue" picks up
	bool			HasSavedGame() const;									// 0x469bf0
	void			ClearSavedGame();										// 0x469c40
	int				GetSavedGameLevel() const;								// 0x469c90
	int				GetSavedGameScore() const;								// 0x469de0
	std::string		GetSavedGameTime() const;								// 0x46a350: the marathon's clock as text
	void			LoadSavedGame();										// 0x46de60: puts it into the game screen
	void			StoreGameSnapshot();									// 0x46d620: writes the game on screen into the saved game
	void			SaveProfile();											// 0x46b1a0

	// The levels a game can be taken back to ("Resurrect"): one for each level started, kept for the classic and tough modes
	int				GetRestartPointCount(int theMode) const;				// 0x469d10
	void			AddRestartPoint();										// 0x469cd0: from the game on screen
	void			ResumeFromRestartPoint(int theIndex);					// 0x469d90
};

class UserManager
{
public:
	UserManager();															// 0x473710
	~UserManager();															// 0x473030

	User*			GetCurrentUser();										// 0x471450: null when there is none
	User*			FindUser(const std::string& theName);					// 0x471820
	void			SetCurrentUser(int theId);								// 0x4716b0
	void			DeleteUser(int theId);									// 0x472170
	void			SaveRoster();											// 0x4720d0
};

}

#endif
