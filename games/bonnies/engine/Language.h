#ifndef __BONNIES_LANGUAGE_H__
#define __BONNIES_LANGUAGE_H__

// The game's texts (0x428f20-0x429560): Data/Language.txt, read once at start. Each line is an id and a quoted text,
// <id> "text", where \" is a quote, \n (or \r) a line break, \t a tab and \\ a backslash; "**" starts a comment and a
// line starting with '*' is skipped. A text is looked up by its id.

#include "Str.h"

namespace bonnies
{

// 0x429040: reads Language.txt (only the first time)
void loadLanguage();
// 0x428f20: frees the texts
void freeLanguage();
// 0x428f70: the text of theId ("" when there is none)
const char* getText(int theId);

} // namespace bonnies

#endif
