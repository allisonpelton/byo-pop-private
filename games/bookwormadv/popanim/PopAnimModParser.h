// Sexy::PopAnimModParser (constructor 0x529e30, vtable 0x7b9158; commands 0x52a190): reads a PopAnim mod file, a
// short text that stands for a .pam and changes what it holds. A line is a command and its arguments:
//
//     SetPamFile "name.pam"    the .pam to load, beside the mod file (the first one stands)
//     Remap "pattern" "new"    a name the .pam uses (an image, a label, a command or its argument) becomes another
//                              (PopAnimNames.h has the patterns); the first Remap that matches decides
//     SetParam "command" "x"   after the .pam is loaded: the argument of the commands called that becomes x;
//                              "command[3]" is the third such command only
//     Colorize "image" r g b [a]  after the .pam is loaded: multiplies the pixels of the images with that name
//     HueShift "image" n          ... turns their hue by n of 256 (the images themselves change, for every user)
//
// The file is read twice, with mMode 1 (SetPamFile and Remap, before the .pam is loaded) and then 2 (the rest,
// after); a command is ignored in the other pass.
#pragma once

#include <string>

#include "DescParser.h"

namespace Sexy {

class PopAnim;

class PopAnimModParser : public DescParser {
public:
	int mMode;               // +0x50: 1 or 2
	PopAnim* mPopAnim;       // +0x54
	std::string mFileName;   // for the messages

	PopAnimModParser(PopAnim* thePopAnim, const std::string& theFileName);  // 0x529e30

	// 0x529ee0: the message, with the file and line, goes to the PopAnim's mError (the exe shows it in a box)
	bool Error(const std::string& theError) override;
	bool HandleCommand(const ListDataElement& theParams) override;  // 0x52a190
};

}  // namespace Sexy
