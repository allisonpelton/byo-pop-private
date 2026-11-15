#ifndef __SPROUT_PLAYERS_H__
#define __SPROUT_PLAYERS_H__

// The players of the game (0x28 bytes, constructor 0x46a5a0 (pizzafrenzy), made on first use by
// 0x46a700 (pizzafrenzy) and kept in a global, 0x532f6c (pizzafrenzy)): the registry key
// Software\<publisher>\<game>\users
// holds a key for each player, with the player's parameters as its values, and its value "activeUser"
// (0x521228 (pizzafrenzy)) names the player who played last. The players are kept by name in order, whatever the
// case. The signal (the exe's signal with no arguments, vtable 0x5012b0 (pizzafrenzy)) says that the current player
// changed.
//

#include "Object.h"
#include "PersistKey.h"
#include "Signal.h"
#include "UserInfo.h"

#include <string>
#include <vector>

namespace engine
{

// 0x469090 (pizzafrenzy): the characters a player's name may have (letters, digits, a space, a hyphen and a full stop);
// any other is 0, which a text box does not take. The exe also compares the character, as a signed byte, with 0xc0 to
// 0xff, which no byte is.
char filterPlayerNameChar(char theChar);

class Players : public Signal<>
{
public:
	Players() : mCurrentIndex(0) {}  // 0x46a5a0 (pizzafrenzy)

	// 0x46a700 (pizzafrenzy)
	static Players* instance();

	// 0x46acb0 (pizzafrenzy): opens the key and reads the players; false when the key cannot be opened
	bool open(const std::string& thePublisher, const std::string& theGame);
	// 0x469f40 (pizzafrenzy): the current player, null when there is none
	UserInfo* current();
	// 0x469f70 (pizzafrenzy): writes the player's parameters to the player's key
	bool saveUser(UserInfo* theUser);
	// 0x469dc0 (pizzafrenzy)
	int currentIndex() { return mCurrentIndex; }
	// 0x469df0 (pizzafrenzy)
	int count() { return (int)mUsers.size(); }
	// 0x469f00 (pizzafrenzy): the player at the index, null when there is none
	UserInfo* getUser(int theIndex);
	// 0x469eb0 (pizzafrenzy): makes the player at the index the current one, writes the name as the last player and
	// tells the signal's slots
	void choose(int theIndex);
	// 0x46a600 (pizzafrenzy): a player of a name the players do not have is added in order, saved and made the
	// current one
	bool add(UserInfo* theUser);
	// 0x46a190 (pizzafrenzy): deletes the player's key and the player
	bool remove(int theIndex);

private:
	bool load();                                // 0x46aae0 (pizzafrenzy)
	bool loadUser(const std::string& theName);  // 0x46a770 (pizzafrenzy)

	Ref<PersistKey> mKey;                // +0x10
	std::vector<Ref<UserInfo>> mUsers;   // +0x18
	int mCurrentIndex;                   // +0x24
};

} // namespace engine

#endif
