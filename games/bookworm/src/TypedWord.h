#ifndef __BOOKWORM_TYPEDWORD_H__
#define __BOOKWORM_TYPEDWORD_H__

// port(mod): typing a word, mod "type-word" (games/bookworm/mods/type-word, feature "typeWord"; the user's idea).
// The letters typed select tiles that spell them, as clicks would; Enter submits the selection. Each letter
// selects the best path for the letters so far, so the selection can move as the word grows; a letter that no
// path spells is refused with a buzz. Of the paths that spell the letters, the best (TypedWord::Better): one
// that does not end the game (a bottom fire tile left out, Classic), then the most fire tiles, then the fewest
// fire tiles dropped (the tiles under them eaten), the fewest landing on the bottom row, then the most points.
// The Qu tile is typed Q, U. Backspace takes back a letter, Esc clears; a click, a hint or a word played
// starts the next typed word afresh. Space still pauses, Tab still hints, Ctrl+Z still undoes.

#include "Common.h"
#include "KeyCodes.h"
#include <string>
#include <vector>

namespace Sexy
{
class Board;
class Piece;
struct SolverWord;

class TypedWord
{
public:
	Board*				mBoard;
	SexyString			mTyped;         // the letters typed, upper case
	std::vector<Piece*>	mPath;          // the tiles selected for them

public:
	TypedWord(Board* theBoard);

	bool				KeyChar(SexyChar theChar);  // true: the key was the typed word's
	bool				KeyDown(KeyCode theKey);
	void				Reset();                    // the tiles were replaced (an undo)

	// The best path over the board's tiles that spells theText (false: none does)
	bool				FindPath(const SexyString& theText, SolverWord* theBest) const;
	static bool			Better(const SolverWord& a, const SolverWord& b);

protected:
	bool				TakesKeys() const;
	bool				SelectionIsTyped() const;
	void				Select(const SexyString& theText, const SolverWord& thePath);
	void				Clear();
};

}

#endif
