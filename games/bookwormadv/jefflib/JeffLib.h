// JeffLib (0x6a0000-0x6c9a00): the helper library of the PopCap Lua games, free of game code. This holds its free
// functions; its classes (states, data-driven classes, dialogs) have their own files.
#pragma once

#include <string>
#include <vector>

namespace JeffLib {

// 0x6a8e50: a < b ignoring case, a run of digits counting as its number ("map2" < "map10"). With equal parts
// the shorter string is less.
bool StringLessNatural(const std::string& a, const std::string& b);

// 0x6a8a90: the names (without their folder) of what matches a pattern such as "scripts/books/*.luc", sorted. The
// folder is looked up in the user's folder, then in the game's (port: the original asked Windows for it
// relative to the current folder).
std::vector<std::string> GetFilesInDirectory(const std::string& thePattern);

}  // namespace JeffLib
