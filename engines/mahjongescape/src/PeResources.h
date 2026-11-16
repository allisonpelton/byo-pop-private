#ifndef __MAHJONG_PERESOURCES_H__
#define __MAHJONG_PERESOURCES_H__

// The port's reader for the resources of a PE file (no original counterpart: the original asked Windows, with
// FindResourceA, LoadResource and LoadStringA on its own exe and on Resource.dll, which it loads as a module).
// The game's art, layouts, fonts, sounds, music and texts are resources of the user's own copy of the game's exe and
// Resource.dll; this reads them from those files (the PE resource directory is Microsoft's documented format) and
// never runs either of them.

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace mahjong
{

class PeResources
{
public:
	// A resource type or name: an integer id, or a string (compared without case, as Windows does)
	struct Key
	{
		bool mIsName;
		uint32_t mId;
		std::string mName;  // upper case

		Key(uint32_t theId) : mIsName(false), mId(theId) {}
		Key(const char* theName);
		bool operator<(const Key& theOther) const;
	};

	// The file's bytes: a view of them, wherever they are held (mapped or read in)
	struct Bytes
	{
		const uint8_t* mBase = nullptr;
		size_t mSize = 0;

		const uint8_t* data() const { return mBase; }
		size_t size() const { return mSize; }
		bool empty() const { return mSize == 0; }
		const uint8_t& operator[](size_t theIndex) const { return mBase[theIndex]; }
	};

	PeResources() {}
	~PeResources() { Close(); }
	PeResources(const PeResources&) = delete;
	PeResources& operator=(const PeResources&) = delete;
	// Reads the file and its resource directory; false when it is missing or not a PE file. On Windows the file is mapped
	// read-only, as LoadLibrary mapped Resource.dll for the original: the system holds its pages as the file's own and
	// reads them in as they are touched, so the file is no part of the process's private bytes (11.7 MB of Resource.dll
	// in Ancient Japan). Where it cannot be mapped, and in a browser, it is read in.
	bool Open(const std::string& thePath);
	bool IsOpen() const { return !mData.empty(); }
	// port: the bytes the file holds in the process's private memory (the memory report, platform/MemReport.h): none while it
	// is mapped
	size_t FileBytes() const { return mRead.capacity(); }
	// port: the file's size, mapped or read
	size_t FileSize() const { return mData.size(); }

	// The bytes of a resource (the first language of it), or nullptr
	const uint8_t* Find(const Key& theType, const Key& theName, uint32_t* theSize) const;
	bool Load(const Key& theType, const Key& theName, std::vector<uint8_t>& theOut) const;
	// LoadStringA: the string of a string table, converted from UTF-16 to the ANSI code page 1252 the original
	// used; false when there is none (LoadStringA's 0)
	bool LoadString(uint32_t theId, std::string& theOut) const;
	// Every name of a type, in the directory's order
	std::vector<Key> Names(const Key& theType) const;

private:
	Bytes mData;
	std::vector<uint8_t> mRead;       // the file read in, when it is not mapped
	void* mFileHandle = nullptr;      // the mapped file's handles (Windows)
	void* mMapHandle = nullptr;
	struct Entry { uint32_t mOffset, mSize; };
	std::map<Key, std::map<Key, Entry>> mDir;
	std::vector<std::pair<Key, std::vector<Key>>> mOrder;

	uint32_t RvaToOffset(uint32_t theRva) const;
	bool MapFile(const std::string& thePath);
	bool ReadIn(const std::string& thePath);
	void Close();
};

} // namespace mahjong

#endif
