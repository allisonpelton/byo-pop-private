#ifndef __HAMMERHEADS_PROFILEHASH_H__
#define __HAMMERHEADS_PROFILEHASH_H__

// The check that every part of a player file carries (0x46a120): the "hash" attribute of the saved game and of the two lists of
// restart points. The values of the part are written one after the other into a string, and the hash is the MD5 of that string
// read as one 128-bit number and printed in capital hexadecimal letters without leading zeros. A part whose hash does not
// match what was read is thrown away, so a file edited by hand (or damaged) loses its saved game.

#include <string>

namespace Sexy
{

std::string HashProfileText(const std::string& theText);			// 0x46a120

}

#endif
