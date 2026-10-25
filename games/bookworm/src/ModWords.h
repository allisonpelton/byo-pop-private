#ifndef __BOOKWORM_MODWORDS_H__
#define __BOOKWORM_MODWORDS_H__

// port(mod): word lists from mods, feature "words" (games/bookworm/mods/collins). Every enabled mod with
//   "features": { "words": { "add": ["<file>", ...], "definitions": ["<file>", ...], "note": "<text>" } }
// adds the words of its "add" files (wordlist.txt's format: one a line, or prefix-compressed) to the app's
// dictionary, and the definitions of its "definitions" files (popdefs.txt's format) to Lex's, below the
// game's own: a word the game defines keeps its definition. A word only a mod has, and that has no
// definition, gets the mod's "note" in Lex's balloon instead, while the mod's bool setting "note" is on (the
// default). The files are the mod's, found through the mod overlay like the game's; a missing one adds
// nothing.
// "fire" (optional): the number of tiles a mod's word sets on fire, the mod's int setting "fire" taking its
// place (Board::SetModFire; mod "rude-words"). A mod's words are those the game's own list lacks, whatever
// other mods add.
// "lookup" (optional) makes a definition balloon a link: a click on it opens the URL, "{word}" standing for
// the word in lower case. Either one URL, or an object of them keyed by the value of the mod's choice
// setting "lookup" (a value without a URL, such as "off", is no link).

#include "Common.h"
#include <set>

namespace Sexy
{
class BookWormApp;
class Definitionator;

void			LoadModWords(BookWormApp* theApp, std::set<SexyString>* theDictionary);  // after wordlist.txt
void			LoadModDefinitions(Definitionator* theDefinitionator);                 // before popdefs.txt
SexyString		ModWordNote(const SexyString& theWord);  // "" for the game's own words, or with no note
int				ModWordFire(const SexyString& theWord);  // the fire tiles the word sets, 0..9
std::string		ModWordLookupURL(const SexyString& theWord); // "" without a link

}

#endif
