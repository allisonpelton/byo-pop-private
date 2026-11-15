#ifndef __HAMMERHEADS_USERMANAGER_H__
#define __HAMMERHEADS_USERMANAGER_H__

// The player roster and what each player has earned (0x469300-0x473710): the trophies, the unlocked modes, the saved game of each
// mode and the restart points. Every player has a file, users\<id>.xml; users\directory.txt lists the numbers of the files, one a
// line, and is written whenever the roster is (the original's UserManager::Save()). The pieces that make up a saved game are
// in SavedGames.h.

#include <map>
#include <string>
#include <vector>

#include "SavedGames.h"

namespace Sexy
{

class XmlNode;

// 0x592950: the "version" of the player files that the game writes and takes
extern const char* const PROFILE_VERSION;

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
	SavedGame		mSavedGames[NUM_SAVED_GAMES];	// +0x000, +0x140, +0x280: the game of each mode that "Continue" picks up
	std::string		mName;						// +0x3c0
	std::string		mVersion;					// +0x3dc: the file's "version"; a file of another version is not taken (0x46a3c0)
	bool			mIsCurrent;					// +0x3f8: the file's "iscurrent": the player the game was left with
	bool			mShowHowToPlay;				// +0x3f9: the next game starts with the how-to-play movie (0x469b90, 0x469ba0)
	bool			mToughUnlocked;				// +0x3fa: Tough Cookie is open (0x469bb0, 0x469bc0): the file's "hascompletedclassic"
	bool			mToughAnnounced;			// +0x3fb: the player has been told so (0x469bd0, 0x469be0): the file's "cookieexplanationshown"
	int				mId;						// +0x3fc: the number of the player's file (users\%d.xml)
	bool			mTrophies[NUM_TROPHIES];	// +0x400
	std::vector<RestartPoint> mClassicRestartPoints;	// +0x410: one for each level started in the classic mode
	std::vector<RestartPoint> mToughRestartPoints;		// +0x420: the same for Tough Cookie

public:
	User(const std::string& theName, int theId);	// 0x46d500: a new player
	explicit User(int theId);						// 0x471380: a player whose file is users\<theId>.xml

	bool			IsCurrentVersion() const;								// 0x46a3c0: the file's version is the one the game writes

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
	void			SaveProfile();											// 0x46b1a0: writes the player's file

	// The levels a game can be taken back to ("Resurrect"): one for each level started, kept for the classic and tough modes
	int				GetRestartPointCount(int theMode) const;				// 0x469d10
	const RestartPoint* GetRestartPoint(int theIndex) const;				// 0x469d40: of the mode being played; null in marathon
	void			AddRestartPoint();										// 0x469cd0: from the game on screen
	void			ResumeFromRestartPoint(int theIndex);					// 0x469d90

private:
	SavedGame&		GetSavedGame(int theMode);								// the slot of the mode (the classic one for a number that is none)
	const SavedGame& GetSavedGame(int theMode) const;
	std::vector<RestartPoint>* GetRestartPoints(int theMode);				// null for marathon
	void			SetUpRestartPoints();									// 0x469a10 twice: each list holds the starting point
	void			Load();													// 0x46e500: reads the file
	void			ReadSavedGame(const XmlNode* theElement);
	void			ReadRestartPoints(const XmlNode* theElement, std::vector<RestartPoint>& theList);
};

class UserManager
{
public:
	UserManager();															// 0x473710
	~UserManager();															// 0x473030: writes the roster, then lets the players go

	User*			GetCurrentUser();										// 0x471450: null when there is none
	User*			FindUser(const std::string& theName);					// 0x471820
	User*			CreateUser(const std::string& theName);					// 0x473600: null when the name is taken; the new player becomes the current one
	bool			RenameUser(const std::string& theOldName, const std::string& theNewName);	// 0x4733c0: false when the new name is taken by another player
	void			SetCurrentUser(int theId);								// 0x4716b0
	void			DeleteUser(int theId);									// 0x472170
	void			SaveRoster();											// 0x4720d0: every player's file, then the directory

	const std::vector<User*>& GetUsers() const { return mUsers; }			// in the order they were made or read

private:
	std::map<std::string, User*> mByName;			// +0x00
	std::map<int, User*> mById;						// +0x0c
	User*			mCurrentUser;					// +0x18
	std::vector<int> mIds;							// +0x1c: the numbers of the files, in the order of users\directory.txt
	std::vector<User*> mUsers;						// +0x2c

	void			ReadDirectory(const std::string& theFolder);			// 0x472e50
	bool			ScanFolder(const std::string& theFolder);				// 0x472d20
	void			WriteDirectory(const std::string& theFileName);			// 0x4715e0
	void			ClearCurrentUser();										// 0x4716f0
	int				FindFreeId();											// 0x471790
	User*			LoadUser(int theId);									// 0x473660: null when the file is not of this version
	void			AddUser(User* theUser);									// 0x473460
	User*			MakeUser(const std::string& theName, int theId);		// 0x473530
};

}

#endif
