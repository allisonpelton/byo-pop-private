#ifndef __PLATFORM_FILESIZE_H__
#define __PLATFORM_FILESIZE_H__

// The size of one file, asked for on every update of the game: SexyAppBase::PollLiveCommands looks at the -commands
// file that often, so a look costs what the update does not have to spare. On Windows a look by name (the C
// runtime's stat) goes through the file system's whole open path and cost 33,000 to 58,000 cycles a call in
// 2026-11 (Windows Defender's filter included). Elsewhere a look is stat().

#include <string>

namespace Platform
{

class FileSizeWatch
{
public:
	FileSizeWatch();
	~FileSizeWatch();

	// Looks at theFile (the same name each call, the way a path is read by the C runtime): true with its size in
	// bytes at this moment, false when the file is not there. The size a writer has flushed shows at once, as with
	// stat; so does a file started again (truncated), deleted, recreated or moved away.
	//
	// Windows: the file stays open between the calls (read-attributes access, every sharing allowed) and its size
	// comes from that handle, 2,500 cycles a call. A change of the directory's file names (a file created, deleted
	// or renamed in it) is waited on, with no cost while nothing happens, and the file is opened by name again when
	// one comes, so a file deleted and made again is the new one. Every 256th call looks by name regardless. If
	// the directory cannot be watched, each call looks by name. One thing the open handle changes for the file's
	// writers: another process cannot rename a file over it (MoveFileEx with MOVEFILE_REPLACE_EXISTING fails with
	// access denied), nor rename or move the folder it is in; appending, truncating, deleting and recreating it work, and
	// so does deleting the folder after the file. A file deleted while it is open keeps its name until the handle closes
	// on Windows without POSIX delete semantics: the next Query (it sees the file is delete-pending) closes it.
	bool Query(const std::string& theFile, long long* theSize);

private:
	FileSizeWatch(const FileSizeWatch&) = delete;
	FileSizeWatch& operator=(const FileSizeWatch&) = delete;

#ifdef _WIN32
	std::string mFile;     // the file being watched
	void* mFileHandle;     // the open file, or NULL
	void* mDirChanges;     // the change notification of the file's directory, or NULL
	unsigned int mLooks;   // the calls so far for mFile
#endif

	void Close();
};

}

#endif
