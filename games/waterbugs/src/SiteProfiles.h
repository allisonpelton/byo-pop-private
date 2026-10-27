#ifndef __WB_SITEPROFILES_H__
#define __WB_SITEPROFILES_H__

// port: Water Bugs's part of the site's players on the web page (framework/WebProfiles.h; docs/WEB_PLAYER.md, "Player profiles").
// Web only: natively this header is empty and SiteProfiles.cpp compiles to nothing.

#ifdef __EMSCRIPTEN__

namespace wb
{

}
namespace r64
{
struct TextInput;
}
namespace wb
{

// The game's players are the ten slots of its players' list (gProfiles). At the end of GameInit, once LoadProfiles has read them,
// the page is told them, and what it queued for them is applied; the mode "create" has the site's player in a slot and chosen
void SiteStart();

// The slots changed (SaveProfiles, which every change ends with): the page is told again
void SiteChanged();

// A name box starts with the site's player under "fill" and "create": the text input of a player's name (12 characters; the
// slot's rename and the name of a free slot) and the high score's (16 characters, at most 480 pixels wide). Typing is the
// game's text input: the case mode 1 makes every letter a capital, and a character its font lacks is not taken
void SiteFillName(r64::TextInput* theInput);
void SiteFillHighScore(r64::TextInput* theInput, void* theFont);

// The first player a game with no saved players makes (ResetProfiles) is named after the account; under "fill" and "create" it is
// named after the site's player
void SiteFirstPlayer(char* theName, int theSize);

}

#endif

#endif
