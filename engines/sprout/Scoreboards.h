#ifndef __SPROUT_SCOREBOARDS_H__
#define __SPROUT_SCOREBOARDS_H__

// The scoreboards of the game (0x20 bytes, made on first use by 0x4691e0 (pizzafrenzy) and kept in a global,
// 0x532f68 (pizzafrenzy)): the registry key Software\<publisher>\<game>\scoreboards holds the players' best
// scores, by the game's code.
//
// Left out: the other scores' parts (0x469390 (pizzafrenzy), 0x469720 (pizzafrenzy)); and the online scoreboard's
// address (0x469110 (pizzafrenzy)): the port has no online scores and no posting.

#include "Object.h"
#include "PersistKey.h"
#include "Scoreboard.h"

#include <string>

namespace engine
{

class Scoreboards
{
public:
	// 0x4691e0 (pizzafrenzy)
	static Scoreboards* instance();

	// 0x469250 (pizzafrenzy): opens the key; false when it cannot be opened
	bool open(const std::string& thePublisher, const std::string& theGame);
	// 0x469370 (pizzafrenzy)
	void setGameCode(const std::string& theGameCode) { mGameCode = theGameCode; }

	// 0x469bf0 (pizzafrenzy): the board's key below the scoreboards' is read into the board, which is then ready
	bool loadBoard(Scoreboard* theBoard);
	// 0x469cd0 (pizzafrenzy): the board's entries are written to its key
	bool saveBoard(Scoreboard* theBoard);

private:
	void loadEntry(PersistKey* theKey, int theIndex, ScoreEntry* theEntry);   // 0x469580 (pizzafrenzy)
	void saveEntry(PersistKey* theKey, int theIndex, ScoreEntry* theEntry);   // 0x469ac0 (pizzafrenzy)

	Ref<PersistKey> mKey;   // +0x00
	std::string mGameCode;  // +0x04
};

} // namespace engine

#endif
