#ifndef __BOOKWORM_MODWORDS_H__
#define __BOOKWORM_MODWORDS_H__

// port(mod): word lists from mods, feature "words" (games/bookworm/mods/collins, rude-words). Every enabled
// mod with
//   "features": { "words": { "add": ["<file>", ...], "definitions": ["<file>", ...], "note": "<text>" } }
// adds the words of its "add" files (wordlist.txt's format: one a line, or prefix-compressed) to the app's
// dictionary, and the definitions of its "definitions" files (popdefs.txt's format) to Lex's, below the
// game's own: a word the game defines keeps its definition. A word only a mod has, and that has no
// definition, gets the mod's "note" in Lex's balloon instead, while the mod's bool setting "note" is on (the
// default). The files are the mod's, found through the mod overlay like the game's; a missing one adds
// nothing. A mod's words are those the game's own list lacks, whatever other mods add.
// "fire" (optional) gives the mod's words effects on play (ModWordGetEffects), from the mod's live settings:
// "points" (a float multiplier, default 1), "everyTile" (bool), "fireCount" ("fixed": "fire" tiles, default
// the feature's number; "length": one per "letters" letters), "bonus" ("keep", "none", "remove"). Then
// "reactions" (files, one comment a line) are what Lex says about such a word (while "note" is on), and
// "stats" (a label) counts them, in the game and in the player's all-time total.
// "lookup" (optional) makes a definition balloon a link: a click on it opens the URL, "{word}" standing for
// the word in lower case. Either one URL, or an object of them keyed by the value of the mod's choice
// setting "lookup" (a value without a URL, such as "off", is no link).

#include "Common.h"
#include <set>

namespace Sexy
{
class BookWormApp;
class Definitionator;

enum ModWordBonus
{
	MODWORD_BONUS_KEEP = 0,     // as usual
	MODWORD_BONUS_NONE,         // the word earns no special tile
	MODWORD_BONUS_REMOVE        // nor does any stay: every special tile on the board turns plain
};

struct ModWordEffects
{
	double			mPoints;        // the score multiplier
	bool			mEveryTile;     // every new tile burns
	int				mFire;          // else this many of them (0..9)
	int				mBonus;         // ModWordBonus
	bool			mComment;       // Lex comments and cries danger
};

void			LoadModWords(BookWormApp* theApp, std::set<SexyString>* theDictionary);  // after wordlist.txt
void			LoadModDefinitions(Definitionator* theDefinitionator);                 // before popdefs.txt
SexyString		ModWordNote(const SexyString& theWord);  // "" for the game's own words, or with no note
bool			ModWordGetEffects(const SexyString& theWord, ModWordEffects* theEffects); // false: none
int				ModWordScore(const SexyString& theWord, int theScore);  // with the points multiplier
SexyString		ModWordReaction(const SexyString& theWord, int theCount); // Lex's comment, "" none
bool			ModWordCounted(const SexyString& theWord);   // a word the stats count
SexyString		ModWordStatsLabel();                         // "" without such a mod
SexyString		ModWordStatsValueName();                     // the player's all-time total, by mod
std::string		ModWordLookupURL(const SexyString& theWord); // "" without a link

}

#endif
