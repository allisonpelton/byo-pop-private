#ifndef __JB_TEXTRESOURCE_H__
#define __JB_TEXTRESOURCE_H__

// A TXT resource read line by line (vtable 0x51d148 (mahjongchina), 0x51e064 (mahjongjapan)): the fonts' glyph tables
// and the layouts. A line ends at a CR or LF, and the empty lines between are skipped.

#include "JBBase.h"

namespace jb
{

class TextResource
{
public:
	const uint8_t* mData = nullptr;  // +0x04
	int mSize = 0;                   // +0x08 in characters
	int mCharSize = 1;               // +0x0c 1 ANSI, 2 UTF-16

	// 0x42b820 (mahjongchina), 0x425430 (mahjongjapan)
	TextResource() {}
	// 0x42b850 (mahjongchina), 0x425460 (mahjongjapan)
	virtual ~TextResource() {}

	// 0x42b860 (mahjongchina), 0x425470 (mahjongjapan): false when the module has no such TXT resource
	bool Load(int theId, int theCharSize, Module theModule);
	// 0x42b900 (mahjongchina), 0x425510 (mahjongjapan): the line at thePos, which moves to the next line; false at the
	// end
	bool ReadLine(int& thePos, std::string& theLine);
};

} // namespace jb

#endif
