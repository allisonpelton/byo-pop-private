// Sexy::EncodingParser (vtable 0x872478 (bejeweled3), 9 slots, 0x28 bytes): this framework generation's reader of text
// files, which gives the characters one by one as 16-bit values. It looks at the first bytes of the file for a byte
// order mark: UTF-16 in either byte order, UTF-8, else every byte is one character.
#pragma once

#include <string>
#include <vector>

namespace Sexy {

class EncodingParser {
public:
	// What GetChar answers (0x47eff0): 0 a character, 1 and 2 the end of the file, 3 no output given
	enum { GET_CHAR_OK = 0, GET_CHAR_END_OF_FILE = 2 };

	// 0x47ec60 (slot 2): opens the file through the app (the user's folder first, then the game's) and finds its
	// encoding; false when there is no such file
	bool OpenFile(const std::string& theFileName);
	// 0x47eff0 (slot 6): the next character into *theChar (a character put back comes first)
	int GetChar(wchar_t* theChar);
	// 0x47ef20 (slot 4): nothing put back and no byte left
	bool EndOfFile() const;
	// 0x47eed0 (slot 3)
	bool CloseFile();

private:
	enum Encoding { ENCODING_BYTES, ENCODING_UTF8, ENCODING_UTF16_LITTLE_ENDIAN, ENCODING_UTF16_BIG_ENDIAN };

	std::vector<unsigned char> mBytes;
	size_t mPosition = 0;
	bool mOpen = false;
	Encoding mEncoding = ENCODING_BYTES;
	std::vector<wchar_t> mPutBack;
};

// port: the characters a file gives, as UTF-8 (the text the port keeps its game strings in)
std::string AppendUTF8(const std::string& theText, wchar_t theChar);

}  // namespace Sexy
