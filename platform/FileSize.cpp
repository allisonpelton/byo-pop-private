#include "FileSize.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/stat.h>
#endif

namespace Platform
{

FileSizeWatch::FileSizeWatch()
#ifdef _WIN32
	: mFileHandle(NULL), mDirChanges(NULL), mLooks(0)
#endif
{
}

FileSizeWatch::~FileSizeWatch()
{
	Close();
}

#ifdef _WIN32

// Every this many calls the file is looked at by name whatever the directory's notification says, for a change it
// cannot tell of (a parent directory renamed, a file system that sends none)
static const unsigned int kLookByNameEvery = 256;

// theFile's directory with its last separator (a drive's root keeps its backslash), "." when theFile has none. The
// name is walked a character at a time, so the second byte of a double-byte character is never taken for a separator.
static std::string DirectoryOf(const std::string& theFile)
{
	const char* aLast = NULL;
	for (const char* aChar = theFile.c_str(); *aChar != 0; aChar = CharNextA(aChar))
	{
		if ((*aChar == '\\') || (*aChar == '/') || (*aChar == ':'))
			aLast = aChar;
	}
	if (aLast == NULL)
		return ".";
	return theFile.substr(0, (size_t) (aLast - theFile.c_str()) + 1);
}

// theFile opened for its size: read-attributes access with every sharing allowed, so the file's writers (appending,
// truncating, deleting and recreating it) are not held up. NULL when it cannot be opened (it is not there).
static void* OpenForSize(const std::string& theFile)
{
	HANDLE aFile = CreateFileA(theFile.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	return (aFile == INVALID_HANDLE_VALUE) ? NULL : aFile;
}

// The file's size from its open handle; false when the handle cannot be asked or its file was deleted
static bool SizeFromHandle(void* theHandle, long long* theSize)
{
	FILE_STANDARD_INFO anInfo;
	if (!GetFileInformationByHandleEx((HANDLE) theHandle, FileStandardInfo, &anInfo, sizeof(anInfo)))
		return false;
	if (anInfo.DeletePending)
		return false;
	*theSize = (long long) anInfo.EndOfFile.QuadPart;
	return true;
}

void FileSizeWatch::Close()
{
	if (mFileHandle != NULL)
		CloseHandle((HANDLE) mFileHandle);
	mFileHandle = NULL;
	if (mDirChanges != NULL)
		FindCloseChangeNotification((HANDLE) mDirChanges);
	mDirChanges = NULL;
}

bool FileSizeWatch::Query(const std::string& theFile, long long* theSize)
{
	if (theFile != mFile)
	{
		Close();
		mFile = theFile;
		mLooks = 0;
	}

	// The first call and every 256th look by name; so does one after a change in the directory, and each call when
	// the directory cannot be watched
	bool aLookByName = false;
	if ((mLooks % kLookByNameEvery) == 0)
	{
		aLookByName = true;
		if (mDirChanges == NULL)
		{
			// watched before the look by name, so a change after it is not missed
			HANDLE aWatch = FindFirstChangeNotificationA(DirectoryOf(mFile).c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
			if (aWatch != INVALID_HANDLE_VALUE)
				mDirChanges = aWatch;
		}
	}
	mLooks++;
	if (mDirChanges == NULL)
		aLookByName = true;
	else if (WaitForSingleObject((HANDLE) mDirChanges, 0) == WAIT_OBJECT_0)
	{
		if (!FindNextChangeNotification((HANDLE) mDirChanges))
		{
			FindCloseChangeNotification((HANDLE) mDirChanges);
			mDirChanges = NULL;
		}
		aLookByName = true;
	}

	if (aLookByName)
	{
		if (mFileHandle != NULL)
			CloseHandle((HANDLE) mFileHandle);
		mFileHandle = OpenForSize(mFile);
	}

	if (mFileHandle == NULL)
		return false;
	if (SizeFromHandle(mFileHandle, theSize))
		return true;

	// The open file was deleted (its change not told of yet) or cannot be asked: once more by name
	CloseHandle((HANDLE) mFileHandle);
	mFileHandle = NULL;
	if (aLookByName)
		return false;
	mFileHandle = OpenForSize(mFile);
	if (mFileHandle == NULL)
		return false;
	if (SizeFromHandle(mFileHandle, theSize))
		return true;
	CloseHandle((HANDLE) mFileHandle);
	mFileHandle = NULL;
	return false;
}

#else

void FileSizeWatch::Close()
{
}

bool FileSizeWatch::Query(const std::string& theFile, long long* theSize)
{
	struct stat aStat;
	if (stat(theFile.c_str(), &aStat) != 0)
		return false;
	*theSize = (long long) aStat.st_size;
	return true;
}

#endif

}
