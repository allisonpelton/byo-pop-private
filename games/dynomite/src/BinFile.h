#ifndef __BINFILE_H__
#define __BINFILE_H__

// The game's binary file (vtable 0x4fa3b4, 8 bytes; 0x414a00-0x414d00): an _open handle with reads and writes of
// bytes, ints, floats and length-prefixed strings. It reads the puzzle packs (*.EGG) and reads and writes the saved
// games (SLOT0.SAV-SLOT7.SAV). Port: the file is read whole, through the VFS so that the web build finds the game's
// files too, and a file made with Create is written out whole by Close. The saved games are in the user folder
// (UserPath; the original's are beside the exe), and a file the original writes into the game's folder (STOMPED.TMP,
// Stomped's copy of its pack) is kept in memory instead.

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
	std::string				mWritePath;	// port: where Close writes a file made by Create ("" none)

	BinFile();												// 0x414a00
	virtual ~BinFile();										// 0x414ce0

	void					Create(const char* theName);			// 0x414a10: an empty file, for writing
	void					Open(const char* theName, bool readOnly);	// 0x414a50
	void					Close();								// 0x414a90
	void					Skip(long theCount);					// 0x414aa0
	void					WriteBool(bool theValue);				// 0x414ac0
	void					WriteInt(int theValue);					// 0x414ae0
	void					WriteFloat(float theValue);				// port: 0x414ae0 too, which the original gives a float's bits
	void					WriteString(const char* theString);		// 0x414b00: an int length with the NUL, then the bytes
	void					Write(const void* theBuffer, unsigned int theSize);	// 0x414b50
	bool					ReadBool();								// 0x414b70
	int						ReadInt();								// 0x414b90
	float					ReadFloat();							// 0x414bb0
	void					ReadString(void* theBuffer);			// 0x414bd0: an int length, then the bytes
	void					Read(void* theBuffer, unsigned int theSize);	// 0x414c00
	void*					ReadAlloc();							// 0x414c20: an int length, then a malloc'd block
	void					CopyFrom(BinFile* theFile, unsigned int theSize);	// 0x414c70: theSize bytes read from theFile
	int						Length();								// 0x414c60
	int						Tell();									// 0x414cb0
	void					Seek(long thePos);						// 0x414cc0

	// port: the files kept in memory, which Open finds before the VFS
	static std::map<std::string, std::vector<uint8_t> >	sMemoryFiles;
	static void				SetMemoryFile(const char* theName, const std::vector<uint8_t>& theData);
	// port: theName in the user folder, where the saved games are
	static std::string		UserPath(const char* theName);
	static bool				UserFileExists(const char* theName);	// port: _access(theName, 0) != -1 on UserPath
};

}

#endif
