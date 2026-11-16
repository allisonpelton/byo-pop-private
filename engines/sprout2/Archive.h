#ifndef __SPROUT2_ARCHIVE_H__
#define __SPROUT2_ARCHIVE_H__

// A Sprout archive (.saf), little-endian: the magic "FFAS", the version (1, 0) and the offset of the table of contents.
// The table holds its own version (1, 0), the MD5 of the rest of the table (from a keyed initial state), the count and
// the entries: the offset and size of the file's data, the MD5 of the data, the length of the name with its zero, and
// the name in lower case.
//
// port: the digests are not checked (the original compares the table's when it reads it, and a file's when a stream
// that read it from start to end closes); the archive is the game's own, so they always match.

#include "Stream.h"

#include <cstdint>
#include <map>
#include <string>

namespace engine
{

struct ArchiveEntry
{
	uint32_t mOffset;
	uint32_t mSize;
	uint8_t mDigest[16];
};

class ArchiveToc : public Object
{
public:
	// 0x450800 (feedingfrenzy2): the table read from the stream at its start
	bool read(RandomAccessInputStream* theStream);
	// 0x45d9f0 (feedingfrenzy2): null when the archive has no such file
	const ArchiveEntry* find(const std::string& theName) const;

private:
	std::map<std::string, ArchiveEntry> mEntries;
};

class ArchiveLoader : public Object
{
public:
	// 0x499d60 (feedingfrenzy2): the archive opened through the engine's disk files and its table read; false (with the
	// error logged) when it is not one
	bool open(const std::string& thePath);
	// 0x457940 (feedingfrenzy2): the name looked up in lower case
	bool contains(const std::string& theName) const;
	// 0x45fc10 (feedingfrenzy2): a stream of the file's data, null (logged) when the archive has no such file
	InputStream* getInputStream(const std::string& theName, bool theCheckDigest);

	RandomAccessInputStream* getStream() const { return mStream; }

private:
	// 0x49b260 (feedingfrenzy2)
	bool readToc();

	Ref<ArchiveToc> mToc;
	Ref<RandomAccessInputStream> mStream;
	uint32_t mTocOffset = 0;
};

// A file's data in the archive, read through the archive's own stream (seeked when another stream moved it)
class ArchiveInputStream : public RandomAccessInputStream
{
public:
	// 0x45fc70 (feedingfrenzy2)
	ArchiveInputStream(ArchiveLoader* theArchive, const ArchiveEntry& theEntry)
		: mStart(theEntry.mOffset), mEnd(theEntry.mOffset + theEntry.mSize), mPosition(theEntry.mOffset),
		  mArchive(theArchive)
	{
	}

	// 0x44abb0 (feedingfrenzy2)
	int read(void* theBuffer, int theSize) override;
	// 0x468410 (feedingfrenzy2): the archive released
	bool close() override;
	// 0x4a6b90 (feedingfrenzy2): false for a place before the file's start
	bool seek(int theOffset, int theOrigin) override;
	// 0x4a6010 (feedingfrenzy2)
	int tell() override { return (int)(mPosition - mStart); }

private:
	uint32_t mStart;
	uint32_t mEnd;
	uint32_t mPosition;
	Ref<ArchiveLoader> mArchive;
};

} // namespace engine

#endif
