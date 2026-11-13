#ifndef __HAMMERHEADS_BOARDFILE_H__
#define __HAMMERHEADS_BOARDFILE_H__

// The board files of a level (boards/board001.xml and the others, 0x40d9e0, the editor widget's loader). A file is a stream
// of chunks, each a little-endian 16 bit length and that many bytes of text; the text is XML with one <object> element for
// each cubby and each sticker. The original reads it with its own TinyXML; the port reads only what the game uses: the
// class, the box (the X, Y, Width and Height attributes) and a sticker's picture (the "index" of its <choice name="Image">).

#include <string>
#include <vector>

namespace Sexy
{

enum BoardObjectKind
{
	BOARD_OBJECT_CUBBY,
	BOARD_OBJECT_STICKER
};

struct BoardObject
{
	int				mKind;			// BoardObjectKind
	int				mX;				// the box's corner
	int				mY;
	int				mWidth;
	int				mHeight;
	int				mChoice;		// a sticker's picture; 0 for a cubby
};

// Reads the objects of a board file, in the order of the file; false when the file is missing or holds no <Level>
bool LoadBoardFile(const std::string& theFileName, std::vector<BoardObject>* theObjects);

}

#endif
