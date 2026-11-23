#ifndef __SPROUT2_USERMANAGER_H__
#define __SPROUT2_USERMANAGER_H__

// The players (0x28 bytes, its constructor 0x49d7b0 (feedingfrenzy2); made on first use by 0x40a630 (feedingfrenzy2)
// and kept in a global, 0x5ac62c (feedingfrenzy2), which 0x4a4700 (feedingfrenzy2) deletes at the end; the original
// gives the class no name, and Talismania's AtlantisUserManager suggests this one). The registry key
// Software\<vendor>\<appName>\users of HKEY_CURRENT_USER holds a key for each player, with the player's parameters as
// its string values, and its value "activeUser" names the current player. The players are kept in the order of their
// names, whatever the case. The signal says that the current player changed.

#include "Object.h"
#include "PersistKey.h"
#include "UserInfo.h"
#include "sigslot.h"

#include <string>
#include <vector>

namespace engine
{

class UserManager
{
public:
	UserManager();  // 0x49d7b0 (feedingfrenzy2)

	// 0x40a630 (feedingfrenzy2)
	static UserManager* instance();
	// 0x4a4700 (feedingfrenzy2)
	static void destroy();

	// 0x4921e0 (feedingfrenzy2): the key opened, and the players read from it when it is
	void open(const std::string& theAppName, const std::string& theVendor);
	// The current player, null when there is none (0x481990 (feedingfrenzy2) with the current index)
	UserInfo* getCurrentUser() { return getUser(mCurrentIndex); }
	// 0x481990 (feedingfrenzy2): the player at the index, null for an index out of range
	UserInfo* getUser(int theIndex);
	int getCurrentIndex() const { return mCurrentIndex; }
	// 0x409cd0 (feedingfrenzy2)
	int getUserCount() const { return (int)mUsers.size(); }
	// 0x492390 (feedingfrenzy2): the index of the player of that name, whatever the case, -1 for none
	int findUser(const std::string& theName);
	// 0x4910d0 (feedingfrenzy2): the player at the index made the current one, named as the last player in the key,
	// and the signal's slots told; nothing for an index out of range
	void setCurrentUser(int theIndex);
	// 0x498ae0 (feedingfrenzy2): a player of a name the players do not have is put in order and saved; saved, the
	// player becomes the current one. False for a name the players have, or a player that could not be saved
	bool addUser(UserInfo* theUser);
	// 0x473c50 (feedingfrenzy2): the player's parameters written to the player's key, stopping at the first that
	// cannot be; false then
	bool saveUser(UserInfo* theUser);
	// 0x53ca4f (feedingfrenzy2): the player at the index taken out of the key and the list; the current index kept on
	// the same player, or on the last when the current one was last; the signal's slots told. False for an index out
	// of range or a key that could not be deleted
	bool removeUser(int theIndex);

	sigslot::signal0 mCurrentUserSignal;  // +0x00

private:
	// 0x48eb60 (feedingfrenzy2): each key below the players' is a player; the value "activeUser" names the current one
	void load();
	// 0x47cc70 (feedingfrenzy2): the player of the key of that name, put in the order of the names and made the
	// current one; a name the players have already is not added
	void loadUser(const std::string& theName);
	// The first player whose name is not before the name, and the first whose name is after it (std::lower_bound,
	// 0x4a0840 (feedingfrenzy2), and std::upper_bound, 0x4a07f0 (feedingfrenzy2), by 0x49e130 (feedingfrenzy2))
	size_t lowerBound(const std::string& theName);
	size_t upperBound(const std::string& theName);

	Ref<PersistKey> mKey;               // +0x10
	std::vector<Ref<UserInfo>> mUsers;  // +0x14
	int mCurrentIndex;                  // +0x24
};

} // namespace engine

#endif
