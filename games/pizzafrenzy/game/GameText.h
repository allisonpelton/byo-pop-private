#ifndef __PIZZAFRENZY_GAMETEXT_H__
#define __PIZZAFRENZY_GAMETEXT_H__

// The texts Pizza Frenzy shows that are in its exe: its string table (the modes' names and descriptions, the frenzy
// names, the credits; the application's slot 0x8c (pizzafrenzy) loads one by its id) and the texts in its code. By the
// user's decision (2026-10-19) no game text is in the repository: the port reads each one at start-up from the
// player's own PizzaFrenzy.exe, the string table's by id and the code's by address in the original: GT(0x4f9ae8).

#include <string>

// Reads the texts from the game folder's PizzaFrenzy.exe (through the engine's host; GameTextTable.inc, made by
// tools/make_gametext.py, gives each one's address, length and hash). False when the exe is missing or nothing matched.
bool loadGameTexts();
// The text the original keeps at theVA; "" for an address that is not in the table or did not match
const std::string& gameText(unsigned int theVA);

#define GT(theVA) (::gameText(theVA))

// The string table's text of an id (LoadStringA of the exe's RT_STRING resources), "" when it has none
const std::string& stringResource(unsigned int theId);

#endif
