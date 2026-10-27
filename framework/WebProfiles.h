#ifndef __WEBPROFILES_H__
#define __WEBPROFILES_H__

// port: what a game asks the web page about the site's players (docs/WEB_PLAYER.md, "Player profiles"; web/profiles.js).
// Web only: natively this header is empty and framework/WebProfiles.cpp compiles to nothing, so the games' hooks stand in
// #ifdef __EMSCRIPTEN__ blocks and the native programs do not change.
//
// The page cannot push to a game: a game's new-player dialog opens at start, before any message from the page would arrive.
// So the game pulls, at its own decision points. A game's hook (its web-only file, usually src/SiteProfiles.cpp) does three
// things, all through this header:
//   1. at its decision point at start (where it would show its new-player dialog), SiteStart(): it applies the operations
//      the Players section queued (rename, delete), has the site's player when the mode is "create", and reports its players;
//   2. wherever its name boxes (a new player's, a high score's) are made, SiteType() or SiteNameFor(): under "fill" and
//      "create" the box starts with the site player's name, typed in as the player would, so the game's own name rules keep
//      or drop each character;
//   3. wherever its list of players changes, SiteChanged(): it reports the list again.
// Under the mode "game" (the default) nothing here changes anything: the hooks return at once and the game plays as the
// original does.

#ifdef __EMSCRIPTEN__

#include "EditListener.h"

#include <string>
#include <vector>

namespace Sexy
{
class EditWidget;
class Font;
class SexyAppBase;
}

namespace Byopop
{

enum SiteMode { SITE_GAME = 0, SITE_FILL = 1, SITE_CREATE = 2 };

// An operation from the Players section, waiting for the game
struct SiteOp
{
	int			mId;
	std::string	mOp;			// "rename" or "delete"
	std::string	mFrom;			// rename: the game's player (as the game named it in its report)
	std::string	mTo;			// rename: the new name
	std::string	mName;			// delete: the game's player
};

// What the page says for this game: the mode of the setting "profile", the current site player's name in the game's text
// (Latin-1; empty with none, or when nothing of it can be written), the site players' names and the queued operations
struct SiteInfo
{
	SiteMode				mMode;
	std::string				mName;
	std::vector<std::string>	mProfiles;
	std::vector<SiteOp>		mOps;
	SiteInfo() : mMode(SITE_GAME) {}
};

// A game's players, in the game's own words. The game's hook implements it with the functions its dialogs use, so a
// player made, renamed, deleted or chosen here is the same as one made in the dialog. Names are the game's text (Latin-1).
class SiteProfiles
{
public:
	virtual ~SiteProfiles() {}

	// The game's players in its order, and the current one ("" with none)
	virtual std::vector<std::string> Names() = 0;
	virtual std::string Current() = 0;
	// The game's own name for the player it counts as the one called theName (it compares as its dialogs do: most ignore
	// case), "" when it has none
	virtual std::string Find(const std::string& theName) = 0;
	// theName as the game's name box would keep it when it is typed in (its characters, length, width, trimming); "" when
	// nothing is left of it. Used for every name that goes to the game.
	virtual std::string Fit(const std::string& theName) = 0;
	// Makes the player theName (already fitted, not in the game) and selects it, as the new-player dialog's OK does. False
	// with a reason (the game has room for no more players)
	virtual bool Create(const std::string& theName, std::string* theWhy) = 0;
	virtual bool Select(const std::string& theName) = 0;
	// Renames theFrom (a name of Names()) to theTo (fitted, and not the name of another player). The player keeps everything.
	virtual bool CanRename() { return false; }
	virtual bool Rename(const std::string& theFrom, const std::string& theTo, std::string* theWhy)
		{ *theWhy = "This game cannot rename a player."; return false; }
	// Deletes the player theName and its progress, as the game's own delete does
	virtual bool CanDelete() { return false; }
	virtual bool Delete(const std::string& theName, std::string* theWhy)
		{ *theWhy = "This game cannot delete a player."; return false; }
	// The game's name rules in words, for the Players section ("Letters and spaces, up to 22 characters")
	virtual const char* Rules() = 0;
	// What the game's own delete removes, for the Players section's confirmation ("the player's options; saved games stay")
	virtual const char* DeleteNote() { return "the player and the progress that goes with it"; }
	// The most players the game keeps, 0 for no limit
	virtual int MaxPlayers() { return 0; }
	// A game that has no players and only asks for a name at each high score: Names() is the last name it remembers
	// (empty when none), Create and Select are not used, and "create" does what "fill" does
	virtual bool IsScores() { return false; }
};

// The players of a SexyAppFramework game that has none and only asks for a name at each high score, in a box with limits
// (RecordScoreDialog and its like), remembering the last name in the registry value "LastScoreName" and in the app's
// string theLast. Names() is that name, Rename sets it, and Fit types a name into a box with the game's font and limits, so a
// game's whole hook is the construction of one of these.
class SiteScoreNames : public SiteProfiles
{
public:
	// theFont: the box's font (NULL before it is loaded), theMaxChars and theMaxPixels as the box has them (-1: none),
	// theListener the box's own EditListener when it filters characters, theRules the rules in words. theLast NULL for a game
	// that does not remember the name (nothing to rename).
	SiteScoreNames(Sexy::SexyAppBase* theApp, std::string* theLast, Sexy::Font* theFont, int theMaxChars, int theMaxPixels,
		const char* theRules, Sexy::EditListener* theListener = NULL);

	virtual std::vector<std::string> Names();
	virtual std::string Current() { return ""; }
	virtual std::string Find(const std::string& theName);
	virtual std::string Fit(const std::string& theName);
	virtual bool Create(const std::string& theName, std::string* theWhy) { return false; }
	virtual bool Select(const std::string& theName) { return false; }
	virtual bool CanRename() { return mLast != NULL; }
	virtual bool Rename(const std::string& theFrom, const std::string& theTo, std::string* theWhy);
	virtual const char* Rules() { return mRules; }
	virtual bool IsScores() { return true; }

private:
	Sexy::SexyAppBase*		mApp;
	std::string*			mLast;
	Sexy::Font*				mFont;
	int						mMaxChars;
	int						mMaxPixels;
	const char*				mRules;
	Sexy::EditListener*		mListener;
};

// The framework calls this when a SexyAppFramework game has finished loading (SexyAppBase::DoUpdateFrames, right after the
// game's LoadingThreadCompleted, so its fonts and images are there and its registry has been read): the decision point at
// start of every such game. A game's web-only file sets it (a static initializer) to its start function, so that the game's own
// sources need no line for it. Without one, nothing happens.
typedef void (*SiteLoadedFn)(Sexy::SexyAppBase* theApp);
extern SiteLoadedFn gSiteLoaded;
void SiteLoaded(Sexy::SexyAppBase* theApp);

// Asks the page about the game with this id (the id of its page.js, "pixelus"): its mode, the site player and the queued
// operations. With no page (a Node run) or no answer: the mode "game".
SiteInfo SiteGet(const char* theGameId);

// The game's decision point at start. Applies the queued operations (the page is told what became of each), then, for the
// mode "create" and a site player, selects the game's player of that name or makes it, and reports the game's players.
// Call it once, before the game decides whether to ask for a new player, at a point where the game can make and
// select a player and its fonts are ready.
void SiteStart(const char* theGameId, SiteProfiles& theGame);

// The game's list of players changed (a player was made, deleted, renamed or chosen): report it to the page
void SiteChanged(const char* theGameId, SiteProfiles& theGame);

// The site player's name for a name box: "" unless the mode is "fill" or "create" and there is a site player
std::string SiteNameFor(const char* theGameId);

// Gives a SexyAppFramework edit box the site player's name when the mode is "fill" or "create", in place of what the game put
// in: the name is typed in (EditWidget::KeyChar), so the box applies its own rules, and ends as the game's own SetText
// leaves a box (the text selected, theLeftPosToZero as the game passes it). Call it right after the game's own SetText, with
// the box's font, limits and listener set. Returns whether the box was filled.
bool SiteType(const char* theGameId, Sexy::EditWidget* theEdit, bool theLeftPosToZero = true);

// theName as a box with this font, limits and listener keeps it when typed in (the font may be NULL before it is loaded: then
// only the characters and the count are applied, not the width). Used by the games' Fit.
std::string SiteFitTyped(const std::string& theName, Sexy::Font* theFont, int theMaxChars, int theMaxPixels,
	Sexy::EditListener* theListener = NULL);

}

#endif // __EMSCRIPTEN__

#endif
