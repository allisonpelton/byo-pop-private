#ifndef __TYPERSHARK_USERPROFILEMGR_H__
#define __TYPERSHARK_USERPROFILEMGR_H__

// UserProfileMgr.cpp: the list of players of Typer Shark Deluxe 1.02, the name of the current one and the two high score
// tables. Original: class UserManager (the traces say UserProfileMgr::...; constructor 0x427440, destructor 0x41f090),
// no vftable, sizeof 0x34c. Kept in three places (UserProfileMgr.cpp): the registry keys Users\<name>\... (one for each
// player, UserProfile.h), the registry value Users\User (the current player) and the file users\user.idx (the high score
// tables, a Buffer of strings: the version, a number, then 10 + 10 entries of score, mode, name and a fourth value, every
// number the text of "%d").
//
// The manager owns the profiles in mUsers (the destructor frees them); UserProfileMgr::SaveUser replaces one by name.
// Methods of the original that live in other files: GetDefaultUserNumber (0x4262b0) and FirstFreeUserNumber (0x413ff0) are
// in WinWSAppGame.cpp.

#include <list>
#include <string>

namespace Sexy
{

class WinWSApp;
class UserProfile;

// one line of a high score table (0x28 bytes in the original)
struct HighScoreEntry
{
	int						mScore;			// +0x00
	int						mMode;			// +0x04 the game mode of the score (0 to 4); -1 in the abyss table
	std::string				mName;			// +0x08
	int						mField24;		// +0x24 the profile's mField0c (UserProfile::SetLastGame's third value); -1 in the abyss table

	HighScoreEntry() : mScore(0), mMode(0), mField24(0) {}
};

class UserProfileMgr
{
public:
	WinWSApp*				mApp;					// +0x00
	std::list<UserProfile*>	mUsers;					// +0x04 (pad) +0x08 head, +0x0c count: the players, in the order of the registry
	std::string				mCurrentUserName;		// +0x10 the registry value Users\User
	HighScoreEntry			mScores[10];			// +0x2c the high scores, best first
	HighScoreEntry			mAbyssScores[10];		// +0x1bc the abyss mode's high scores (the score is the depth)

public:
	UserProfileMgr(WinWSApp* theApp);														// 0x427440
	~UserProfileMgr();																		// 0x41f090

	std::string				GetCurrentUserName();											// 0x4150a0
	HighScoreEntry			GetHighScore(bool theAbyss, int theIndex);						// 0x41ac10
	void					SaveHighScores();												// 0x41a1b0
	void					LoadDefaultHighScores();										// 0x41ef10
	bool					ExistsUserKey();												// 0x420db0
	std::string				LoadDefaultUserNameFromRegistry();								// 0x419fc0
	bool					RegisterDefaultUserName(const std::string& theName);			// 0x419da0
	void					DeregisterUserProfile(const std::string& theName);				// 0x420ff0
	void					SaveUser(UserProfile* theProfile);								// 0x420c60
	UserProfile*			LoadUser(const std::string& theName);							// 0x425f20
	UserProfile*			LoadUserProfileFromRegistry(const std::string& theName);		// 0x425390
	void					WriteUserProfileToRegistry(UserProfile* theProfile, bool theRecordScore, bool theRegistryOnly);	// 0x41f1d0

	bool					ReadIntString(const std::string& theValueName, int* theValue);	// 0x413ef0
	int						GetDefaultUserNumber();											// 0x4262b0 (WinWSAppGame.cpp)
	int						FirstFreeUserNumber();											// 0x413ff0 (WinWSAppGame.cpp)
};

}

#endif
