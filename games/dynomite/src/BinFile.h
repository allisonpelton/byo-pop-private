#ifndef __BINFILE_H__
#define __BINFILE_H__

// The game's binary file (vtable 0x4fa3b4, 8 bytes; 0x414a00-0x414d00): an _open handle with reads and writes of
// bytes, ints, floats and length-prefixed strings. It reads the puzzle packs (*.EGG). Port: the file is read whole
// through the VFS, so that the web build finds it too; the writes come with the saved games. The file the original
// writes into the game's folder (STOMPED.TMP, Stomped's copy of its pack) is kept in memory instead.

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Sexy
{

class BinFile
{
public:
	std::vector<uint8_t>	mData;		// port: the file's contents (the original keeps its handle at +4)
	size_t					mPos;
	bool					mOpen;

	BinFile();												// 0x414a00
	virtual ~BinFile();										// 0x414ce0

	void					Open(const char* theName, bool readOnly);	// 0x414a50
	void					Close();								// 0x414a90
	void					Skip(long theCount);					// 0x414aa0
	bool					ReadBool();								// 0x414b70
	int						ReadInt();								// 0x414b90
	float					ReadFloat();							// 0x414bb0
	void					ReadString(void* theBuffer);			// 0x414bd0: an int length, then the bytes
	void					Read(void* theBuffer, unsigned int theSize);	// 0x414c00
	void*					ReadAlloc();							// 0x414c20: an int length, then a malloc'd block
	int						Length();								// 0x414c60
	int						Tell();									// 0x414cb0
	void					Seek(long thePos);						// 0x414cc0

	// port: the files kept in memory, which Open finds before the VFS
	static std::map<std::string, std::vector<uint8_t> >	sMemoryFiles;
	static void				SetMemoryFile(const char* theName, const std::vector<uint8_t>& theData);
};

}

#endif
