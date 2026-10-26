#ifndef __NINGPO_HFILESYSTEM_H__
#define __NINGPO_HFILESYSTEM_H__

// The read file system (PGApp::mountResource 0x40e350): a .pyz name opens the archive with the minizip reader
// (0x4279a0), any other a folder (0x427350). The game mounts ningpo.pyz, a plain ZIP file whose names it looks up
// without regard to case (games/ningpo/tools/ningpyz.py). port: the archive is read into memory through the host and
// inflated with stb's zlib; the original's open/read/size/close of a file comes down to readFile().

#include <cstdint>
#include <string>
#include <vector>

namespace pg
{

class HFileSystem
{
public:
	struct Entry
	{
		std::string mName;     // lower case
		uint32_t mOffset;      // of the local header
		uint32_t mCompressedSize;
		uint32_t mSize;
		uint16_t mMethod;      // 0 stored, 8 deflated
	};
	std::vector<uint8_t> mArchive;
	std::vector<Entry> mEntries;

	// Reads and indexes the archive; false when it is missing or not a ZIP file
	bool mount(const char* theArchiveName);
	bool exists(const char* theName) const;
	// The whole file; false when it is not in the archive
	bool readFile(const char* theName, std::vector<uint8_t>& theData) const;

private:
	const Entry* find(const char* theName) const;
};

} // namespace pg

#endif
