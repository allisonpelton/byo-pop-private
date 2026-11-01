// How PopAnim matches and renames names (0x529920 and 0x529540): the names in a .pam file (images, labels,
// commands and their arguments) against the patterns of a mod file. Case does not matter. A pattern is a name,
// or a name with a * at its start ("*text": the name ends with it), at its end ("text*": it starts with it) or at
// both ends ("*text*": it holds it); a lone * matches any name.
#pragma once

#include <string>

namespace Sexy {

// 0x529920: whether theName matches thePattern
bool PopAnimNameMatches(const std::string& theName, const std::string& thePattern);

// 0x529540: when theName matches thePattern, puts in theResult what theName becomes under the replacement
// theRep, and returns true. A replacement without a * is the new name; a * at its start or end stands for what
// theName has before or after the part of it that the pattern's text matched: "Remap pre* x*" turns pre_a into
// x_a, "Remap *_old *_new" turns a_old into a_new. When theName is exactly the pattern, a replacement with a *
// at its start or end is added to theName's end or start instead.
bool PopAnimRemapName(const std::string& theName, const std::string& thePattern, const std::string& theRep,
                      std::string* theResult);

}  // namespace Sexy
