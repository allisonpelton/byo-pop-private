// The text translator: the strings of the data files that name something the game decides (a title, a score, the next level or
// the next screen to show) are replaced by what the player's profile and the level just played say. Translating some of them
// also changes the profile (the bonus ids add to the score), so a translation happens once, when a text object is set.

#ifndef VENICE_GAMETEXT_H
#define VENICE_GAMETEXT_H

// 0x405e10: the text for this id, or the id itself when the translator does not know it. The result is in a buffer that the
// next call writes over; a null id gives null.
const char* TranslateText(const char* theId);

#endif
