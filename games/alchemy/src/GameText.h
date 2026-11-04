#ifndef __GAMETEXT_H__
#define __GAMETEXT_H__

// The one text the original keeps in its program and not in properties/default.xml is the ten default names of the
// score tables. port: none of it is in the repository. LoadGameTexts() reads each name at start-up from the player's
// own Alchemy.exe (or a game image carved from it, AlchemyGame.exe) in the game folder, by its address in the 1.6
// image, and checks its length and a hash; a name that is not there stays empty (no exe, another version), and the
// caller falls back to its own placeholder. GameText.cpp lists the addresses, lengths and hashes: no text.

#include <string>
#include <vector>

namespace Sexy
{

// Reads the names from the game folder's executable. Returns false when the executable is missing or nothing matched.
bool				LoadGameTexts();
// The text the original keeps at theVA; "" for an address that is not in the table or did not match
const std::string&	GameText(unsigned int theVA);
// A table of 32-bit values the original keeps in its program (read, like the texts, from the player's executable, with a
// length and hash check); empty for an address that is not in the list or did not match
const std::vector<int>&	GameTable(unsigned int theVA);

}

#endif
