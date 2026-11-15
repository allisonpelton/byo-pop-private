#ifndef __SPROUT_RESOURCEMANAGER_H__
#define __SPROUT_RESOURCEMANAGER_H__

// The archive loader (0x43f1f0 (wordharmony)-0x4417c0 (wordharmony)) and the resource manager (0x20 bytes, constructor
// 0x43eae0 (wordharmony), vtable 0x484e08 (wordharmony)). The application forwards its slots 0x18-0x34 to the resource
// manager (its field +0x70).

#include "Stream.h"
#include <map>
#include <string>
#include <vector>

namespace engine
{

// A Sprout archive (.saf): the header "FFAS", the version 1 and the offset of the table of contents; the table holds
// its version, a keyed MD5 of the rest of the table, the count and the entries (offset, size, keyed MD5 of the data,
// the name's length with its terminating zero, and the name). Names are looked up in lower case.
class ArchiveLoader : public Object
{
public:
	ArchiveLoader();  // 0x43f520 (wordharmony)

	// 0x43fa00 (wordharmony): opens an archive of the game's folder; throws on an error
	void openArchive(const std::string& thePath);
	// 0x43f690 (wordharmony)
	bool contains(const std::string& theName) const;
	// 0x43f900 (wordharmony): throws when the file is not in the archive
	InputStream* getInputStream(const std::string& theName);
	// The names of the files (the port's, for its tools)
	std::vector<std::string> getNames() const;

private:
	struct Entry
	{
		uint32_t mOffset;
		uint32_t mSize;
	};

	// 0x43f710 (wordharmony) (with the table's reader 0x441120 (wordharmony)): the MD5 digests are not checked
	void readToc();

	SharedData mData;
	std::map<std::string, Entry> mEntries;
};

class ResourceManager : public Object
{
public:
	ResourceManager();  // 0x43eae0 (wordharmony)

	// 0x43ebb0 (wordharmony) (application slot 0x18)
	void addArchive(const char* thePath);
	// 0x43e4f0 (wordharmony) (slot 0x1c): in an archive, or on disk when loose files are used
	bool exists(const char* theName);
	// 0x43e400 (wordharmony) (slot 0x20): a loose file
	bool fileExists(const char* theName);
	// 0x43e310 (wordharmony) (slot 0x24): read loose files instead of the archives (a developer setting)
	void setUseFiles(bool theUseFiles) { mUseFiles = theUseFiles; }
	// 0x43e860 (wordharmony) (slot 0x28)
	InputStream* openRawStream(const char* theName);
	// 0x43e880 (wordharmony) (slot 0x2c): the same, buffered
	InputStream* openStream(const char* theName);
	// 0x43e320 (wordharmony) (slot 0x30): a loose file; throws when it is missing
	InputStream* openFile(const char* theName);
	// 0x43e390 (wordharmony) (slot 0x34): the same, buffered
	InputStream* openBufferedFile(const char* theName);
	// 0x43e640 (wordharmony): throws when no archive has the file
	InputStream* getArchiveStream(const char* theName);
	// The port's, for its tools
	const std::vector<Ref<ArchiveLoader>>& getArchives() const { return mArchives; }

private:
	std::vector<Ref<ArchiveLoader>> mArchives;  // +0x10
	bool mUseFiles;                             // +0x1c
};

} // namespace engine

#endif
