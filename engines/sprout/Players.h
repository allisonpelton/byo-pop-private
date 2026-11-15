#ifndef __SPROUT_PLAYERS_H__
#define __SPROUT_PLAYERS_H__

// The players of the game (0x28 bytes, constructor 0x46a5a0 (pizzafrenzy), made on first use by
// 0x46a700 (pizzafrenzy) and kept in a global, 0x532f6c (pizzafrenzy)): the registry key
// Software\<publisher>\<game>\users
// holds a key for each player, with the player's parameters as its values, and its default value names the player
// who played last. The players are kept by name in order, whatever the case. The signal (the exe's signal with no
// arguments, vtable 0x5012b0 (pizzafrenzy)) says that the current player changed.
//
// Left out: adding, removing and choosing a player (0x469eb0 (pizzafrenzy), 0x46a190 (pizzafrenzy),
// 0x46a600 (pizzafrenzy)), which no state of the loading reaches, and the number of players (0x469df0 (pizzafrenzy)).

#include "Object.h"
#include "PersistKey.h"
#include "Signal.h"
#include "UserInfo.h"

#include <string>
#include <vector>

namespace engine
{

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

private:
	bool load();                                // 0x46aae0 (pizzafrenzy)
	bool loadUser(const std::string& theName);  // 0x46a770 (pizzafrenzy)

	Ref<PersistKey> mKey;                // +0x10
	std::vector<Ref<UserInfo>> mUsers;   // +0x18
	int mCurrentIndex;                   // +0x24
};

} // namespace engine

#endif
