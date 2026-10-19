#include "UserFiles.h"
#include "VFS.h"
#include "WebPersist.h"

#include <cstdio>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Platform
{

static void (*gUserDataChanged)() = nullptr;

#ifdef _WIN32
// UTF-8 to UTF-16 for the wide file API; empty if the text is not UTF-8
static std::wstring Widen(const std::string& theText)
{
	if (theText.empty())
		return std::wstring();
	int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, theText.data(), (int) theText.size(), nullptr, 0);
	if (n <= 0)
		return std::wstring();
	std::wstring out((size_t) n, L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, theText.data(), (int) theText.size(), &out[0], n);
	return out;
}
#endif

static FILE* OpenHostFile(const std::string& thePath, const char* theMode)
{
#ifdef _WIN32
	std::wstring aPath = Widen(thePath);
	if (!aPath.empty())
	{
		std::wstring aMode(theMode, theMode + strlen(theMode));
		return _wfopen(aPath.c_str(), aMode.c_str());
	}
#endif
	return fopen(thePath.c_str(), theMode);
}

static bool ReplaceHostFile(const std::string& theFrom, const std::string& theTo)
{
#ifdef _WIN32
	std::wstring aFrom = Widen(theFrom), aTo = Widen(theTo);
	if (!aFrom.empty() && !aTo.empty())
		return MoveFileExW(aFrom.c_str(), aTo.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
	return MoveFileExA(theFrom.c_str(), theTo.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
	return rename(theFrom.c_str(), theTo.c_str()) == 0;
#endif
}

static bool RemoveHostFile(const std::string& thePath)
{
#ifdef _WIN32
	std::wstring aPath = Widen(thePath);
	if (!aPath.empty())
		return DeleteFileW(aPath.c_str()) != 0;
#endif
	return remove(thePath.c_str()) == 0;
}

bool ReadHostFile(const std::string& thePath, std::string* theData)
{
	FILE* f = OpenHostFile(thePath, "rb");
	if (!f)
		return false;
	char buf[4096];
	size_t n;
	theData->clear();
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
		theData->append(buf, n);
	bool ok = !ferror(f);
	fclose(f);
	return ok;
}

static std::string UserPath(const std::string& theRelPath)
{
	return VFS::Get().GetUserDir() + "/" + theRelPath;
}

bool WriteUserFileAtomic(const std::string& theRelPath, const std::string& theData)
{
	MakeDirs(VFS::Get().GetUserDir());
	std::string aPath = UserPath(theRelPath);
	std::string aTemp = aPath + ".tmp";
	FILE* f = OpenHostFile(aTemp, "wb");
	if (!f)
		return false;
	bool ok = fwrite(theData.data(), 1, theData.size(), f) == theData.size();
	ok = (fflush(f) == 0) && ok;
	ok = (fclose(f) == 0) && ok;
	if (ok)
		ok = ReplaceHostFile(aTemp, aPath);
	if (!ok)
		RemoveHostFile(aTemp);
	UserDataChanged();
	return ok;
}

bool RenameUserFile(const std::string& theFrom, const std::string& theTo)
{
	bool ok = ReplaceHostFile(UserPath(theFrom), UserPath(theTo));
	if (ok)
		UserDataChanged();
	return ok;
}

bool RemoveUserFile(const std::string& theRelPath)
{
	std::string aPath = UserPath(theRelPath);
	if (!IsFile(aPath) || !RemoveHostFile(aPath))
		return false;
	UserDataChanged();
	return true;
}

void SetUserDataChangedCallback(void (*theCallback)())
{
	gUserDataChanged = theCallback;
}

void UserDataChanged()
{
	SyncUserData();
	if (gUserDataChanged)
		gUserDataChanged();
#ifdef __EMSCRIPTEN__
	else
		BackupUserData(std::vector<BackupFile>());
#endif
}

}
