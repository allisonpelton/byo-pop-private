#ifndef __PIXELUS_USERDIRECTORY_H__
#define __PIXELUS_USERDIRECTORY_H__

// UserDirectory.cpp: the players and what each has solved (Pixelus.exe 0x43da10 constructor, app+0x76c; the
// players 0x439fb0/0x4391c0/0x438450). users/directory.txt lists the player numbers, users/<number> holds a
// player: name, the flags the tutorial and the screens keep, and per puzzle the best result. The port writes
// its own byte layout in the same files.

#include <map>
#include <string>
#include <vector>

namespace Sexy
{

class PixelusApp;
class LevelData;

// the player's result in one puzzle (0x437f10 / 0x438f30)
struct PuzzleRecord
{
	int		mSolved;		// +4 largest of the "solved the best way" flags given
	int		mMoves2;		// +8 smallest of the second move count given
	int		mMoves;			// +0xc fewest moves: the puzzle counts as maxed if this is not above the best known
};

class UserProfile
{
public:
	enum { NUM_FLAGS = 64 };

	PixelusApp*		mApp;
	int				mId;						// +0x30
	std::string		mName;						// +0x14
	bool			mCurrent;					// +0x2c the player who is playing
	bool			mFlag2D;					// +0x2d
	bool			mFlag38;					// +0x38
	unsigned char	mFlags[NUM_FLAGS];			// +0x39 the tutorial hints the player has seen
	bool			mFlag79;					// +0x79
	bool			mFlag7A;					// +0x7a
	std::map<std::string, PuzzleRecord>	mRecords;	// by puzzle uuid
	std::map<std::string, int>			mTempleFlags;	// per temple (+0x10 visited, +0x11, +0x12): bits 1, 2, 4

public:
	UserProfile(PixelusApp* theApp);

	// the number of puzzles of a temple (or of all) solved, and solved as well as the best known solution
	int				CountSolved(const std::string& theLevel) const;		// 0x4383a0
	int				CountMaxed(const std::string& theLevel) const;		// 0x438310
	int				CountSolved() const;								// 0x439340
	bool			IsMaxed(const PuzzleRecord& theRecord, const std::string& theUuid) const;	// 0x437f50
	const PuzzleRecord*	GetRecord(const std::string& theUuid) const;	// 0x4393b0
	void			SetRecord(const std::string& theUuid, int theSolved, int theMoves, int theMoves2);	// 0x439f80

	void			Write(class Buffer* theBuffer) const;				// 0x4391c0
	bool			Read(const class Buffer* theBuffer);				// 0x439fb0
};

class UserDirectory
{
public:
	PixelusApp*					mApp;				// +0x1c
	std::vector<UserProfile*>	mUsers;				// +0x34
	std::vector<int>			mIds;				// +0x24 the numbers of directory.txt
	UserProfile*				mCurrent;			// +0x18

public:
	UserDirectory(PixelusApp* theApp);				// 0x43da10
	~UserDirectory();

	UserProfile*	GetCurrent() { return mCurrent; }
	UserProfile*	FindUser(int theId);
	UserProfile*	FindUserByName(const std::string& theName);
	void			Select(int theId);				// 0x43b6a0
	UserProfile*	AddUser(const std::string& theName);	// 0x43c7f0
	void			DeleteUser(int theId);			// 0x43c420
	bool			RenameUser(const std::string& theOld, const std::string& theNew);	// 0x43d650
	void			Save();							// 0x43c2a0
};

}

#endif
