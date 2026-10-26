#ifndef __WORDHARMONY_GAMETEXT_H__
#define __WORDHARMONY_GAMETEXT_H__

// The texts Word Harmony shows that are hard-coded in its exe (the HUD's hint button, the bonus word line, the
// default high score names...). By the user's decision (2026-10-19) no game text is in the repository: the port
// reads each one at start-up from the player's own WordHarmony.exe by its address in the original, and code names a
// text by that address: GT(0x482e84).

#include <string>

// Reads the texts from the game folder's WordHarmony.exe (through the engine's host; GameTextTable.inc, made by
// tools/make_gametext.py, gives each one's address, length and hash). False when the exe is missing or nothing matched.
bool loadGameTexts();
// The text the original keeps at theVA; "" for an address that is not in the table or did not match
const std::string& gameText(unsigned int theVA);

#define GT(theVA) (::gameText(theVA))

#endif
