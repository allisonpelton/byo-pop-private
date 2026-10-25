#ifndef __BOOKWORM_USERMANAGER_H__
#define __BOOKWORM_USERMANAGER_H__

// UserManager.cpp: the player profiles. They live in the registry, under SOFTWARE\<mRegKey>\Users\<name>
// (Alias, ShowHelp, ShowHints, ShowDefs, TimesPlayed), with Users\User = the last player; the players\
// folder holds only saved games. Code at 0x434800-0x435e20 (the tail of INVENTORY's TitleScreen.cpp
// range: its own source file) plus the inline getter 0x402090. Spec: re/bookworm/analysis/app.md 1.3.
// The original uses Win32 registry calls on HKEY_LOCAL_MACHINE directly; the port uses the framework's
// emulated registry with the value names "Users\<name>\<value>" and "Users\User".

#include "Common.h"
#include <list>
#include <string>

namespace Sexy
{

class BookWormApp;

// Original: no vtable, 0x38 bytes, ctor 0x434960 (copies the name into both strings)
struct User
{
	SexyString			mName;              // +0x00 registry subkey name, as typed
	SexyString			mAlias;             // +0x1c "Alias" (internet high-score alias); ctor: = name

	User(const SexyString& theName);        // 0x434960
};

// Original: no vtable, sizeof 0x2c, ctor 0x435b50 (new in BookWormApp::ReadFromRegistry), dtor body 0x434880
class UserManager
{
public:
	BookWormApp*		mApp;               // +0x00 for mRegKey and the option flags
	std::list<User*>	mUsers;             // +0x04 (head +0x08, size +0x0c) in registry enumeration order
	SexyString			mCurrentUser;       // +0x10 current player name ("" = none)

public:
	UserManager(BookWormApp* theApp);       // 0x435b50 enumerates Users\*, then mCurrentUser = GetLastUser()
	~UserManager();                         // 0x434880 deletes every User

	// 0x402090 (inline COMDAT, folded into BWGameSelector.obj): a copy of mCurrentUser
	SexyString		GetCurrentUserName() { return mCurrentUser; }

	User*			FindUser(const SexyString& theName);                // 0x434800 _stricmp on User::mName
	bool			ReadUserInt(const SexyString& theUserName, const SexyString& theValueName, int* theValue); // 0x4349e0
	bool			WriteUserString(const SexyString& theUserName, const SexyString& theValueName,
						const SexyString& theValue);                    // 0x434b70
	bool			WriteUserInt(const SexyString& theUserName, const SexyString& theValueName, int theValue); // 0x434d10
	void			SaveUser(User* theUser);                            // 0x434e90 Alias, ShowHelp, ShowHints, ShowDefs, TimesPlayed
	void			DeleteUser(const SexyString& theName);              // 0x4352e0 registry key + list entry
	User*			LoadUser(const SexyString& theName);                // 0x435470 new User with its "Alias"
	bool			SetCurrentUser(const SexyString& theName);          // 0x4356d0 writes Users\User
	bool			SetCurrentAlias(const SexyString& theAlias);        // 0x435880 (internet only)
	SexyString		GetCurrentAlias();                                  // 0x435910 the alias, else the name
	SexyString		GetLastUser();                                      // 0x4359b0 Users\User, else the first user
};

}

#endif
