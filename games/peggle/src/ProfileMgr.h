// ProfileMgr.h: the list of player profiles (the original's Sexy::ProfileMgr). The list is the file userdata/users.dat: each
// profile's name, when it was last used and its number; the rest of a profile is in its own file (PlayerInfo).

#ifndef __PROFILEMGR_H__
#define __PROFILEMGR_H__

#include "Common.h"
#include "PlayerInfo.h"

#include <map>
#include <string>

namespace Sexy
{

class DataSync;

class ProfileMgr
{
public:
	typedef std::map<std::string, PlayerInfo, StringLessNoCase> ProfileMap;

	ProfileMgr();

	void Load();							   // 0x473490
	void Save();							   // 0x4735f0
	void SyncState(DataSync& theSync);		   // 0x473280
	PlayerInfo* GetProfile(const std::string& theName); // 0x442e10: loads its details and marks it as the last used
	PlayerInfo* GetAnyProfile();			   // 0x441ca0: the first profile of the list, loaded the same way

	ProfileMap mProfiles;					   // 0x8
	int mNextUserId;						   // 0x10: the number the next profile made gets
	int mNextUseSeq;						   // 0x14
};

} // namespace Sexy

#endif
