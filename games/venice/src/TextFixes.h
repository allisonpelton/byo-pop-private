#ifndef VENICE_TEXTFIXES_H
#define VENICE_TEXTFIXES_H

// The fixes mod's "typos" option (games/venice/mods/fixes/mod.json): the texts of the original's data files that are
// misspelt, as they should read. Each is read from a level or screen file and shown as it is there, so the one place that
// takes a text from its file (a level's title and messages, a text object's string) asks for its corrected version.
// Returns theText itself when the option is off, or when the text is not one of the misspelt ones.
const char* CorrectedText(const char* theText);

#endif
