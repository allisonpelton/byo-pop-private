#include "SaveImport.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace Platform
{

// "-importsaves" or "-importsaves=<value>" (one or two dashes); the last one wins
static bool FindImportOption(int argc, char** argv, std::string* theValue)
{
	bool found = false;
	for (int i = 1; i < argc; i++)
	{
		const char* a = argv[i];
		if (a == nullptr)
			continue;
		while (*a == '-')
			a++;
		if (strncmp(a, "importsaves", 11) != 0 || (a[11] != '=' && a[11] != 0))
			continue;
		found = true;
		*theValue = (a[11] == '=') ? std::string(a + 12) : std::string();
	}
	return found;
}

}

#ifndef _WIN32

namespace Platform
{

bool RunSaveImportIfRequested(int argc, char** argv, const char*, const std::string&, const std::string&, int* theExitCode)
{
	std::string aValue;
	if (!FindImportOption(argc, argv, &aValue))
		return false;
	fprintf(stderr, "-importsaves is not available in this build: it reads the registry and folders of a Windows install.\n");
	*theExitCode = 2;
	return true;
}

}

#else // _WIN32

#include "Json.h"
#include "SaveImportTable.h"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <ctime>
#include <map>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Platform
{

namespace
{

// ---------- text ----------
std::string Fmt(const char* theFormat, ...)
{
	char aBuf[2048];
	va_list anArgs;
	va_start(anArgs, theFormat);
	vsnprintf(aBuf, sizeof(aBuf), theFormat, anArgs);
	va_end(anArgs);
	return aBuf;
}

std::string Lower(std::string s)
{
	for (size_t i = 0; i < s.size(); i++)
		if (s[i] >= 'A' && s[i] <= 'Z')
			s[i] = (char) (s[i] + 32);
	return s;
}

std::wstring Wide(const std::string& s)
{
	if (s.empty())
		return std::wstring();
	int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), (int) s.size(), nullptr, 0);
	UINT aPage = CP_UTF8;
	if (n <= 0)
	{
		aPage = CP_ACP;
		n = MultiByteToWideChar(aPage, 0, s.data(), (int) s.size(), nullptr, 0);
	}
	std::wstring w((size_t) (n > 0 ? n : 0), L'\0');
	if (n > 0)
		MultiByteToWideChar(aPage, aPage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, s.data(), (int) s.size(), &w[0], n);
	return w;
}

std::string Narrow(const std::wstring& w)
{
	if (w.empty())
		return std::string();
	int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int) w.size(), nullptr, 0, nullptr, nullptr);
	std::string s((size_t) (n > 0 ? n : 0), '\0');
	if (n > 0)
		WideCharToMultiByte(CP_UTF8, 0, w.data(), (int) w.size(), &s[0], n, nullptr, nullptr);
	return s;
}

std::string Backslashes(std::string s)
{
	std::replace(s.begin(), s.end(), '/', '\\');
	return s;
}

std::string Hex(const std::string& theBytes)
{
	static const char aDigits[] = "0123456789abcdef";
	std::string aResult;
	for (size_t i = 0; i < theBytes.size(); i++)
	{
		unsigned char b = (unsigned char) theBytes[i];
		aResult += aDigits[b >> 4];
		aResult += aDigits[b & 15];
	}
	return aResult;
}

// ---------- the table's patterns ----------
// '*' is any characters inside one part, '#' digits (one or more); without regard to case. The captures are given in order.
bool MatchPattern(const char* p, const char* t, char theSep, std::vector<std::string>& theCaps)
{
	if (*p == 0)
		return *t == 0;
	if (*p == '*')
	{
		size_t aMax = 0;
		while (t[aMax] != 0 && t[aMax] != theSep)
			aMax++;
		for (size_t n = aMax + 1; n-- > 0;)
		{
			theCaps.push_back(std::string(t, n));
			if (MatchPattern(p + 1, t + n, theSep, theCaps))
				return true;
			theCaps.pop_back();
		}
		return false;
	}
	if (*p == '#')
	{
		size_t aMax = 0;
		while (isdigit((unsigned char) t[aMax]))
			aMax++;
		for (size_t n = aMax; n >= 1; n--)
		{
			theCaps.push_back(std::string(t, n));
			if (MatchPattern(p + 1, t + n, theSep, theCaps))
				return true;
			theCaps.pop_back();
		}
		return false;
	}
	if (*t == 0 || tolower((unsigned char) *p) != tolower((unsigned char) *t))
		return false;
	return MatchPattern(p + 1, t + 1, theSep, theCaps);
}

bool Matches(const std::string& thePattern, const std::string& theText, char theSep, std::vector<std::string>* theCaps = nullptr)
{
	std::vector<std::string> aCaps;
	bool ok = MatchPattern(thePattern.c_str(), theText.c_str(), theSep, aCaps);
	if (ok && theCaps != nullptr)
		*theCaps = aCaps;
	return ok;
}

std::string FillTarget(const std::string& theTo, const std::vector<std::string>& theCaps)
{
	std::string aResult;
	size_t n = 0;
	for (size_t i = 0; i < theTo.size(); i++)
	{
		if (theTo[i] == '*' || theTo[i] == '#')
		{
			if (n < theCaps.size())
				aResult += theCaps[n];
			n++;
		}
		else
			aResult += theTo[i];
	}
	return aResult;
}

// ---------- the registry, read only ----------
struct RegValue
{
	std::string mPath;      // below the game's key, with backslashes
	std::string mKind;      // sz, expand, dword, hex, multi, qword, other
	std::string mText;      // sz, expand: the ANSI text
	int mNum = 0;           // dword
	std::string mBytes;     // hex, multi, qword, other
};
typedef std::map<std::string, RegValue> RegMap;   // lower-case path -> value

void ReadKeyTree(HKEY theKey, REGSAM theView, const std::string& thePrefix, RegMap& theMap, int theDepth)
{
	DWORD aNumSubKeys = 0, aMaxSubKey = 0, aNumValues = 0, aMaxName = 0, aMaxData = 0;
	if (RegQueryInfoKeyA(theKey, nullptr, nullptr, nullptr, &aNumSubKeys, &aMaxSubKey, nullptr, &aNumValues, &aMaxName, &aMaxData, nullptr, nullptr) != ERROR_SUCCESS)
		return;
	std::vector<char> aName(aMaxName + 2);
	std::vector<BYTE> aData(aMaxData + 4);
	for (DWORD i = 0; i < aNumValues; i++)
	{
		DWORD aNameLen = aMaxName + 1, aType = 0, aDataLen = aMaxData + 2;
		if (RegEnumValueA(theKey, i, &aName[0], &aNameLen, nullptr, &aType, &aData[0], &aDataLen) != ERROR_SUCCESS)
			continue;
		RegValue v;
		std::string aValueName(&aName[0], aNameLen);
		v.mPath = thePrefix.empty() ? aValueName : thePrefix + "\\" + aValueName;
		std::string aRaw((const char*) &aData[0], aDataLen);
		switch (aType)
		{
		case REG_SZ:
		case REG_EXPAND_SZ:
			v.mKind = (aType == REG_SZ) ? "sz" : "expand";
			v.mText = aRaw.substr(0, aRaw.find('\0'));
			break;
		case REG_DWORD:
			v.mKind = "dword";
			if (aRaw.size() >= 4)
			{
				DWORD d;
				memcpy(&d, aRaw.data(), 4);
				v.mNum = (int) d;
			}
			break;
		case REG_BINARY:
			v.mKind = "hex";
			v.mBytes = aRaw;
			break;
		case REG_MULTI_SZ:
			v.mKind = "multi";
			v.mBytes = aRaw;
			break;
		case REG_QWORD:
			v.mKind = "qword";
			v.mBytes = aRaw;
			break;
		default:
			v.mKind = "other";
			v.mBytes = aRaw;
			break;
		}
		theMap[Lower(v.mPath)] = v;
	}
	if (theDepth >= 8)
		return;
	std::vector<char> aSub(aMaxSubKey + 2);
	for (DWORD i = 0; i < aNumSubKeys; i++)
	{
		DWORD aSubLen = aMaxSubKey + 1;
		if (RegEnumKeyExA(theKey, i, &aSub[0], &aSubLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
			continue;
		std::string aSubName(&aSub[0], aSubLen);
		HKEY aChild = nullptr;
		if (RegOpenKeyExA(theKey, aSubName.c_str(), 0, KEY_READ | theView, &aChild) != ERROR_SUCCESS)
			continue;
		ReadKeyTree(aChild, theView, thePrefix.empty() ? aSubName : thePrefix + "\\" + aSubName, theMap, theDepth + 1);
		RegCloseKey(aChild);
	}
}

// One place of the game's key; false when the key is not there. Only reads: KEY_READ.
bool ReadPlace(const std::string& thePlace, const std::string& theRegKey, RegMap& theMap, std::string* theWhere)
{
	HKEY aKey = nullptr;
	REGSAM aView = 0;
	std::string aPath;
	if (thePlace == "hklm")
	{
		// the 32-bit view, where a 32-bit original is: HKLM\SOFTWARE\WOW6432Node\<key> on a 64-bit Windows
		aView = KEY_WOW64_32KEY;
		aPath = "SOFTWARE\\" + theRegKey;
		*theWhere = "HKLM\\SOFTWARE\\WOW6432Node\\" + theRegKey;
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, aPath.c_str(), 0, KEY_READ | aView, &aKey) != ERROR_SUCCESS)
			return false;
	}
	else if (thePlace == "hkcu")
	{
		aPath = "Software\\" + theRegKey;
		*theWhere = "HKCU\\" + aPath;
		if (RegOpenKeyExA(HKEY_CURRENT_USER, aPath.c_str(), 0, KEY_READ, &aKey) != ERROR_SUCCESS)
			return false;
	}
	else
	{
		// where Windows redirects the writes of a game that was not run as administrator
		aView = KEY_WOW64_64KEY;
		aPath = "Software\\Classes\\VirtualStore\\MACHINE\\SOFTWARE\\WOW6432Node\\" + theRegKey;
		*theWhere = "HKCU\\" + aPath;
		if (RegOpenKeyExA(HKEY_CURRENT_USER, aPath.c_str(), 0, KEY_READ | aView, &aKey) != ERROR_SUCCESS)
		{
			aPath = "Software\\Classes\\VirtualStore\\MACHINE\\SOFTWARE\\" + theRegKey;
			*theWhere = "HKCU\\" + aPath;
			if (RegOpenKeyExA(HKEY_CURRENT_USER, aPath.c_str(), 0, KEY_READ | aView, &aKey) != ERROR_SUCCESS)
				return false;
		}
	}
	ReadKeyTree(aKey, aView, std::string(), theMap, 0);
	RegCloseKey(aKey);
	return true;
}

// ---------- the port's registry file ----------
struct IniEntry
{
	std::string mFull;
	std::string mValue;
};
typedef std::map<std::string, IniEntry> IniMap;   // lower-case full path -> entry

IniMap ParseIni(const std::string& theText)
{
	IniMap aMap;
	std::string aSection;
	bool hasSection = false;
	size_t aPos = 0;
	while (aPos <= theText.size())
	{
		size_t anEnd = theText.find('\n', aPos);
		if (anEnd == std::string::npos)
			anEnd = theText.size();
		std::string aLine = theText.substr(aPos, anEnd - aPos);
		aPos = anEnd + 1;
		if (!aLine.empty() && aLine[aLine.size() - 1] == '\r')
			aLine.resize(aLine.size() - 1);
		if (aLine.empty() || aLine[0] == ';')
			continue;
		if (aLine[0] == '[')
		{
			size_t aClose = aLine.rfind(']');
			aSection = aLine.substr(1, aClose == std::string::npos ? std::string::npos : aClose - 1);
			hasSection = true;
			continue;
		}
		size_t anEq = aLine.find('=');
		if (anEq != std::string::npos && hasSection)
		{
			IniEntry e;
			e.mFull = aSection + "\\" + aLine.substr(0, anEq);
			e.mValue = aLine.substr(anEq + 1);
			aMap[Lower(e.mFull)] = e;
		}
	}
	return aMap;
}

// SexyAppBase::GetRegistryText: the entries grouped by key, in the order of its std::map
std::string WriteIni(const IniMap& theMap)
{
	std::vector<const IniEntry*> anEntries;
	for (IniMap::const_iterator i = theMap.begin(); i != theMap.end(); ++i)
		anEntries.push_back(&i->second);
	std::sort(anEntries.begin(), anEntries.end(), [](const IniEntry* a, const IniEntry* b) { return Lower(a->mFull) < Lower(b->mFull); });
	std::map<std::string, std::pair<std::string, std::string> > aKeys;   // lower-case key -> (key, lines)
	for (size_t i = 0; i < anEntries.size(); i++)
	{
		size_t anAt = anEntries[i]->mFull.rfind('\\');
		if (anAt == std::string::npos)
			continue;
		std::string aSection = anEntries[i]->mFull.substr(0, anAt);
		std::pair<std::string, std::string>& k = aKeys[Lower(aSection)];
		k.first = aSection;
		k.second += anEntries[i]->mFull.substr(anAt + 1) + "=" + anEntries[i]->mValue + "\n";
	}
	std::string aText = "; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n";
	for (std::map<std::string, std::pair<std::string, std::string> >::const_iterator i = aKeys.begin(); i != aKeys.end(); ++i)
		aText += "\n[" + i->second.first + "]\n" + i->second.second;
	return aText;
}

std::string EscapeSz(const std::string& s)
{
	std::string aResult;
	for (size_t i = 0; i < s.size(); i++)
	{
		if (s[i] == '\\')
			aResult += "\\\\";
		else if (s[i] == '\n')
			aResult += "\\n";
		else if (s[i] == '\r')
			aResult += "\\r";
		else
			aResult += s[i];
	}
	return aResult;
}

// ---------- the plan ----------
struct Rule
{
	std::string mName, mType;
};
struct SkipRule
{
	std::string mPattern, mWhy;
};
struct RegItem
{
	std::string mPath, mFull, mPlace, mValue, mShown, mStatus, mOld;
};
struct RegSkipped
{
	std::string mPath, mPlace, mWhy;
};
struct FileItem
{
	std::string mTo, mFrom, mWhat, mSource, mStatus;
	int mRoot = 0;
	unsigned long mSize = 0;
};
struct FileSkipped
{
	std::string mRel, mWhy;
};

std::string KindName(const std::string& k)
{
	if (k == "sz" || k == "expand")
		return "text";
	if (k == "dword")
		return "a number";
	if (k == "hex" || k == "multi")
		return "binary data";
	if (k == "qword")
		return "a 64-bit number";
	return "a kind of value the port does not read";
}

// the value in the port's form, or false with why
bool Convert(const RegValue& v, const std::string& theType, std::string* theValue, std::string* theShown, std::string* theWhy)
{
	if (theType == "sz")
	{
		if (v.mKind != "sz" && v.mKind != "expand")
		{
			*theWhy = "stored by the original as " + KindName(v.mKind) + ", where the port reads text";
			return false;
		}
		*theValue = "sz:" + EscapeSz(v.mText);
		*theShown = "\"" + (v.mText.size() > 40 ? v.mText.substr(0, 40) + "..." : v.mText) + "\"";
		return true;
	}
	if (theType == "dword")
	{
		if (v.mKind != "dword")
		{
			*theWhy = "stored by the original as " + KindName(v.mKind) + ", where the port reads a number";
			return false;
		}
		*theValue = Fmt("dword:%d", v.mNum);
		*theShown = Fmt("%d", v.mNum);
		return true;
	}
	if (theType == "hex")
	{
		if (v.mKind != "hex" && v.mKind != "multi")
		{
			*theWhy = "stored by the original as " + KindName(v.mKind) + ", where the port reads binary data";
			return false;
		}
		*theValue = "hex:" + Hex(v.mBytes);
		*theShown = Fmt("binary data, %d bytes", (int) v.mBytes.size());
		return true;
	}
	*theWhy = "unknown type in the table";
	return false;
}

// ---------- files ----------
bool ReadFileBytes(const std::string& thePath, std::string* theData)
{
	FILE* f = _wfopen(Wide(thePath).c_str(), L"rb");
	if (f == nullptr)
		return false;
	char aBuf[8192];
	size_t n;
	theData->clear();
	while ((n = fread(aBuf, 1, sizeof(aBuf), f)) > 0)
		theData->append(aBuf, n);
	bool ok = !ferror(f);
	fclose(f);
	return ok;
}

bool MakeDirsW(const std::wstring& thePath)
{
	std::wstring aPath = thePath;
	for (size_t i = 0; i < aPath.size(); i++)
		if (aPath[i] == L'/')
			aPath[i] = L'\\';
	for (size_t i = 1; i <= aPath.size(); i++)
	{
		if (i == aPath.size() || aPath[i] == L'\\')
		{
			std::wstring aPart = aPath.substr(0, i);
			if (aPart.size() == 2 && aPart[1] == L':')
				continue;
			if (!CreateDirectoryW(aPart.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
				return false;
		}
	}
	return true;
}

bool IsDirW(const std::wstring& thePath)
{
	DWORD a = GetFileAttributesW(thePath.c_str());
	return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

bool IsFileW(const std::wstring& thePath)
{
	DWORD a = GetFileAttributesW(thePath.c_str());
	return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

// <folder>\<rel> written whole through a temporary file and an atomic replace; the folders are made
bool WriteFileAtomic(const std::string& thePath, const std::string& theData)
{
	std::wstring aPath = Wide(Backslashes(thePath));
	size_t aSlash = aPath.rfind(L'\\');
	if (aSlash != std::wstring::npos)
		MakeDirsW(aPath.substr(0, aSlash));
	std::wstring aTemp = aPath + L".tmp";
	FILE* f = _wfopen(aTemp.c_str(), L"wb");
	if (f == nullptr)
		return false;
	bool ok = fwrite(theData.data(), 1, theData.size(), f) == theData.size();
	ok = (fflush(f) == 0) && ok;
	ok = (fclose(f) == 0) && ok;
	if (ok)
		ok = MoveFileExW(aTemp.c_str(), aPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
	if (!ok)
		DeleteFileW(aTemp.c_str());
	return ok;
}

struct Found
{
	std::string mRel;
	std::vector<std::string> mCaps;
	std::string mFull;
	unsigned long mSize = 0;
};

// The files of a root that a pattern of the table (parts separated by '/') names. A part with no wildcard is looked up by its name (the
// file system ignores case); one with a wildcard lists the folder.
void Expand(const std::wstring& theDir, const std::vector<std::string>& theParts, size_t theAt, const std::string& theRel, const std::vector<std::string>& theCaps,
	std::vector<Found>* theOut)
{
	if (theAt >= theParts.size() || theOut->size() > 4000)
		return;
	const std::string& aPart = theParts[theAt];
	bool isLast = theAt + 1 == theParts.size();
	if (aPart.find_first_of("*#") == std::string::npos)
	{
		std::wstring aNext = theDir + L"\\" + Wide(aPart);
		std::string aRel = theRel.empty() ? aPart : theRel + "/" + aPart;
		if (isLast)
		{
			WIN32_FILE_ATTRIBUTE_DATA d;
			if (GetFileAttributesExW(aNext.c_str(), GetFileExInfoStandard, &d) && !(d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				Found f;
				f.mRel = aRel;
				f.mCaps = theCaps;
				f.mFull = Narrow(aNext);
				f.mSize = d.nFileSizeLow;
				theOut->push_back(f);
			}
		}
		else if (IsDirW(aNext))
			Expand(aNext, theParts, theAt + 1, aRel, theCaps, theOut);
		return;
	}
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW((theDir + L"\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE)
		return;
	do
	{
		std::string aName = Narrow(fd.cFileName);
		if (aName == "." || aName == "..")
			continue;
		bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		if (isDir == isLast)
			continue;
		std::vector<std::string> aCaps;
		if (!MatchPattern(aPart.c_str(), aName.c_str(), '/', aCaps))
			continue;
		std::vector<std::string> aAll = theCaps;
		aAll.insert(aAll.end(), aCaps.begin(), aCaps.end());
		std::string aRel = theRel.empty() ? aName : theRel + "/" + aName;
		if (isLast)
		{
			Found f;
			f.mRel = aRel;
			f.mCaps = aAll;
			f.mFull = Narrow(theDir + L"\\" + fd.cFileName);
			f.mSize = fd.nFileSizeLow;
			theOut->push_back(f);
		}
		else
			Expand(theDir + L"\\" + fd.cFileName, theParts, theAt + 1, aRel, aAll, theOut);
	} while (FindNextFileW(h, &fd));
	FindClose(h);
}

std::vector<std::string> SplitParts(const std::string& thePattern)
{
	std::vector<std::string> aParts;
	size_t aPos = 0;
	while (true)
	{
		size_t aNext = thePattern.find('/', aPos);
		aParts.push_back(thePattern.substr(aPos, aNext == std::string::npos ? std::string::npos : aNext - aPos));
		if (aNext == std::string::npos)
			break;
		aPos = aNext + 1;
	}
	return aParts;
}

// ---------- the report ----------
struct Report
{
	std::string mText;
	void Line(const std::string& s = std::string()) { mText += s + "\r\n"; }
};

std::string DateStamp()
{
	time_t t = time(nullptr);
	struct tm* l = localtime(&t);
	char aBuf[64];
	strftime(aBuf, sizeof(aBuf), "%Y-%m-%d", l);
	return aBuf;
}

std::string TimeStamp()
{
	time_t t = time(nullptr);
	struct tm* l = localtime(&t);
	char aBuf[64];
	strftime(aBuf, sizeof(aBuf), "%Y-%m-%d %H:%M:%S", l);
	return aBuf;
}

const char* PlaceName(const std::string& thePlace)
{
	if (thePlace == "vs")
		return "the VirtualStore copy";
	if (thePlace == "hklm")
		return "the machine's key";
	return "the user's key";
}

std::string EnvVar(const char* theName)
{
	wchar_t aBuf[2048];
	DWORD n = GetEnvironmentVariableW(Wide(theName).c_str(), aBuf, 2048);
	if (n == 0 || n >= 2048)
		return std::string();
	return Narrow(std::wstring(aBuf, n));
}

} // namespace

bool RunSaveImportIfRequested(int argc, char** argv, const char* theGameId, const std::string& theGameDir, const std::string& theUserDir, int* theExitCode)
{
	std::string anOptionValue;
	if (!FindImportOption(argc, argv, &anOptionValue))
		return false;
	*theExitCode = 0;
	Report r;
	std::string aReportPath = theUserDir + "/import-report.txt";

	JsonValue aTable;
	std::string anError;
	if (!ParseJson(kSaveImportTable, aTable, &anError) || !aTable["games"].Has(theGameId))
	{
		fprintf(stderr, "-importsaves: the table of what can be imported has no entry for %s%s%s\n", theGameId, anError.empty() ? "" : ": ", anError.c_str());
		*theExitCode = 2;
		return true;
	}
	const JsonValue& aGame = aTable["games"][theGameId];

	std::string anInstall = anOptionValue.empty() ? theGameDir : anOptionValue;
	while (anInstall.size() > 3 && (anInstall.back() == '/' || anInstall.back() == '\\'))
		anInstall.pop_back();
	r.Line(Fmt("Import of saves from an installed %s, %s", theGameId, TimeStamp().c_str()));
	r.Line();
	r.Line("User folder: " + Backslashes(theUserDir));
	r.Line("Install folder: " + (anInstall.empty() ? std::string("(none: only the registry is read)") : Backslashes(anInstall)));
	r.Line("Only the values and files that the game's entry of the table lists are taken. The original's registry and folders are only read.");
	r.Line();

	// ----- the registry -----
	std::vector<RegItem> anItems;
	std::vector<RegSkipped> aSkipped;
	int anOverridden = 0;
	IniMap aCurrent;
	std::string aCurrentText;
	std::string aRegistryPath = theUserDir + "/registry.ini";
	if (aGame.Has("registry"))
	{
		const JsonValue& aReg = aGame["registry"];
		std::string aKey = aReg["key"].AsString();
		std::vector<std::string> aPlaces;
		for (size_t i = 0; i < aReg["hives"].Size(); i++)
			aPlaces.push_back(aReg["hives"][i].AsString());
		std::vector<Rule> aRules;
		std::vector<std::string> aSetNames;
		for (size_t i = 0; i < aReg["sets"].Size(); i++)
			aSetNames.push_back(aReg["sets"][i].AsString());
		std::vector<std::string> aSpecs;
		for (size_t s = 0; s < aSetNames.size(); s++)
		{
			const JsonValue& aSet = aTable["valueSets"][aSetNames[s]];
			for (size_t i = 0; i < aSet.Size(); i++)
				aSpecs.push_back(aSet[i].AsString());
		}
		for (size_t i = 0; i < aReg["values"].Size(); i++)
			aSpecs.push_back(aReg["values"][i].AsString());
		for (size_t i = 0; i < aSpecs.size(); i++)
		{
			size_t anAt = aSpecs[i].rfind('=');
			Rule rule;
			rule.mName = aSpecs[i].substr(0, anAt);
			rule.mType = aSpecs[i].substr(anAt + 1);
			aRules.push_back(rule);
		}
		std::vector<SkipRule> aSkipRules;
		const JsonValue* aSkipSources[2] = { &aTable["skipValues"], aReg.Has("skip") ? &aReg["skip"] : nullptr };
		for (int s = 0; s < 2; s++)
		{
			if (aSkipSources[s] == nullptr)
				continue;
			for (size_t i = 0; i < aSkipSources[s]->Members().size(); i++)
			{
				SkipRule sk;
				sk.mPattern = aSkipSources[s]->Members()[i].first;
				sk.mWhy = aSkipSources[s]->Members()[i].second.AsString();
				aSkipRules.push_back(sk);
			}
		}

		r.Line("Registry key: " + aKey);
		std::map<std::string, RegMap> aMaps;
		for (size_t i = 0; i < aPlaces.size(); i++)
		{
			std::string aWhere;
			bool found = ReadPlace(aPlaces[i], aKey, aMaps[aPlaces[i]], &aWhere);
			r.Line(Fmt("  %s (%s): %s", PlaceName(aPlaces[i]), aWhere.c_str(), found ? Fmt("found, %d values", (int) aMaps[aPlaces[i]].size()).c_str() : "not there"));
		}
		// the places in the game's order of preference, value by value
		std::map<std::string, std::pair<RegValue, std::string> > aMerged;
		for (size_t i = aPlaces.size(); i-- > 0;)
		{
			const RegMap& m = aMaps[aPlaces[i]];
			for (RegMap::const_iterator it = m.begin(); it != m.end(); ++it)
			{
				if (aMerged.find(it->first) != aMerged.end())
					anOverridden++;
				aMerged[it->first] = std::make_pair(it->second, aPlaces[i]);
			}
		}
		ReadFileBytes(aRegistryPath, &aCurrentText);
		aCurrent = ParseIni(aCurrentText);
		for (std::map<std::string, std::pair<RegValue, std::string> >::const_iterator it = aMerged.begin(); it != aMerged.end(); ++it)
		{
			const RegValue& v = it->second.first;
			const Rule* aRule = nullptr;
			for (size_t i = 0; i < aRules.size() && aRule == nullptr; i++)
				if (Matches(aRules[i].mName, v.mPath, '\\'))
					aRule = &aRules[i];
			if (aRule == nullptr)
			{
				RegSkipped sk;
				sk.mPath = v.mPath;
				sk.mPlace = it->second.second;
				sk.mWhy = "not a value the port keeps";
				for (size_t i = 0; i < aSkipRules.size(); i++)
					if (Matches(aSkipRules[i].mPattern, v.mPath, '\\'))
					{
						sk.mWhy = aSkipRules[i].mWhy;
						break;
					}
				aSkipped.push_back(sk);
				continue;
			}
			RegItem item;
			std::string aWhy;
			if (!Convert(v, aRule->mType, &item.mValue, &item.mShown, &aWhy))
			{
				RegSkipped sk;
				sk.mPath = v.mPath;
				sk.mPlace = it->second.second;
				sk.mWhy = aWhy;
				aSkipped.push_back(sk);
				continue;
			}
			item.mPath = v.mPath;
			item.mFull = "SOFTWARE\\" + aKey + "\\" + v.mPath;
			item.mPlace = it->second.second;
			IniMap::const_iterator old = aCurrent.find(Lower(item.mFull));
			if (old != aCurrent.end())
			{
				item.mFull = old->second.mFull;
				item.mOld = old->second.mValue;
				item.mStatus = (old->second.mValue == item.mValue) ? "same" : "changes";
			}
			else
				item.mStatus = "new";
			anItems.push_back(item);
		}
	}

	// ----- the files -----
	std::vector<FileItem> aFiles;
	std::vector<FileSkipped> aFileSkipped;
	if (aGame.Has("files") || aGame.Has("skipFiles"))
	{
		// the roots, the one that wins last: the install folder, C:\ProgramData\PopCap Games\<game>, and the VirtualStore copy of the install
		std::vector<std::pair<std::string, std::string> > aRoots;
		if (!anInstall.empty())
		{
			aRoots.push_back(std::make_pair(std::string("the install folder"), anInstall));
		}
		if (aGame.Has("appdata"))
		{
			std::string aProgramData = EnvVar("ProgramData");
			if (aProgramData.empty())
				aProgramData = "C:\\ProgramData";
			aRoots.push_back(std::make_pair(std::string("the ProgramData folder"), aProgramData + "\\PopCap Games\\" + aGame["appdata"].AsString()));
		}
		if (anInstall.size() > 3 && anInstall[1] == ':')
		{
			std::string aLocal = EnvVar("LOCALAPPDATA");
			if (!aLocal.empty())
				aRoots.push_back(std::make_pair(std::string("the VirtualStore copy of the install folder"), aLocal + "\\VirtualStore" + Backslashes(anInstall.substr(2))));
		}
		std::map<std::string, FileItem> aTake;
		std::map<std::string, FileSkipped> aSkipMap;
		for (size_t ri = 0; ri < aRoots.size(); ri++)
		{
			bool there = IsDirW(Wide(aRoots[ri].second));
			r.Line(Fmt("Folder: %s (%s): %s", aRoots[ri].first.c_str(), Backslashes(aRoots[ri].second).c_str(), there ? "found" : "not there"));
			if (!there)
				continue;
			const JsonValue& aList = aGame["files"];
			for (size_t i = 0; i < aList.Size(); i++)
			{
				std::vector<Found> aFound;
				Expand(Wide(aRoots[ri].second), SplitParts(aList[i]["from"].AsString()), 0, std::string(), std::vector<std::string>(), &aFound);
				for (size_t k = 0; k < aFound.size(); k++)
				{
					FileItem item;
					item.mTo = FillTarget(aList[i]["to"].AsString(), aFound[k].mCaps);
					item.mFrom = aFound[k].mRel;
					item.mWhat = aList[i]["what"].AsString();
					item.mSource = aFound[k].mFull;
					item.mRoot = (int) ri;
					item.mSize = aFound[k].mSize;
					aTake[Lower(item.mTo)] = item;
				}
			}
			if (aGame.Has("skipFiles"))
			{
				const JsonValue& aSkipList = aGame["skipFiles"];
				for (size_t i = 0; i < aSkipList.Size(); i++)
				{
					std::vector<Found> aFound;
					Expand(Wide(aRoots[ri].second), SplitParts(aSkipList[i]["from"].AsString()), 0, std::string(), std::vector<std::string>(), &aFound);
					for (size_t k = 0; k < aFound.size(); k++)
					{
						FileSkipped sk;
						sk.mRel = aFound[k].mRel;
						sk.mWhy = aSkipList[i]["why"].AsString();
						aSkipMap[Lower(sk.mRel)] = sk;
					}
				}
			}
		}
		unsigned long aTotal = 0;
		for (std::map<std::string, FileItem>::iterator it = aTake.begin(); it != aTake.end(); ++it)
		{
			FileItem& f = it->second;
			if (f.mSize > 4u * 1024u * 1024u || aTotal + f.mSize > 32u * 1024u * 1024u)
			{
				FileSkipped sk;
				sk.mRel = f.mFrom;
				sk.mWhy = "too large to be a save file";
				aFileSkipped.push_back(sk);
				continue;
			}
			aTotal += f.mSize;
			aFiles.push_back(f);
		}
		for (std::map<std::string, FileSkipped>::iterator it = aSkipMap.begin(); it != aSkipMap.end(); ++it)
			aFileSkipped.push_back(it->second);
	}

	// ----- what would change -----
	int aNewValues = 0, aChangedValues = 0, aSameValues = 0;
	for (size_t i = 0; i < anItems.size(); i++)
	{
		if (anItems[i].mStatus == "new")
			aNewValues++;
		else if (anItems[i].mStatus == "changes")
			aChangedValues++;
		else
			aSameValues++;
	}
	std::vector<std::string> aFileData;
	std::vector<FileItem> aReadable;
	int aNewFiles = 0, aChangedFiles = 0, aSameFiles = 0;
	for (size_t i = 0; i < aFiles.size(); i++)
	{
		FileItem f = aFiles[i];
		std::string aBytes, anOld;
		if (!ReadFileBytes(f.mSource, &aBytes))
		{
			FileSkipped sk;
			sk.mRel = f.mFrom;
			sk.mWhy = "could not be read";
			aFileSkipped.push_back(sk);
			continue;
		}
		bool has = ReadFileBytes(theUserDir + "/" + f.mTo, &anOld);
		f.mStatus = !has ? "new" : (anOld == aBytes) ? "same" : "changes";
		if (f.mStatus == "new")
			aNewFiles++;
		else if (f.mStatus == "changes")
			aChangedFiles++;
		else
			aSameFiles++;
		aReadable.push_back(f);
		aFileData.push_back(aBytes);
	}
	aFiles = aReadable;

	bool aWriteRegistry = aNewValues + aChangedValues > 0;
	IniMap aNew = aCurrent;
	for (size_t i = 0; i < anItems.size(); i++)
	{
		IniEntry e;
		e.mFull = anItems[i].mFull;
		e.mValue = anItems[i].mValue;
		aNew[Lower(e.mFull)] = e;
	}

	r.Line();
	r.Line(Fmt("Registry values: %d new, %d replace a value, %d the same as now.", aNewValues, aChangedValues, aSameValues));
	for (size_t i = 0; i < anItems.size(); i++)
	{
		const RegItem& it = anItems[i];
		const char* aWord = it.mStatus == "new" ? "new" : it.mStatus == "changes" ? "replaces" : "the same";
		r.Line(Fmt("  %s: %s = %s (from %s)%s", aWord, it.mPath.c_str(), it.mShown.c_str(), PlaceName(it.mPlace),
			it.mStatus == "changes" ? (", was " + it.mOld).c_str() : ""));
	}
	if (anOverridden > 0)
		r.Line(Fmt("  %d values were found in more than one place: the one the game reads first was used.", anOverridden));
	if (!aSkipped.empty())
	{
		r.Line(Fmt("Registry values left out: %d.", (int) aSkipped.size()));
		for (size_t i = 0; i < aSkipped.size(); i++)
			r.Line("  " + aSkipped[i].mPath + ": " + aSkipped[i].mWhy);
	}
	r.Line();
	r.Line(Fmt("Files: %d new, %d replace a file, %d the same as now.", aNewFiles, aChangedFiles, aSameFiles));
	for (size_t i = 0; i < aFiles.size(); i++)
	{
		const FileItem& f = aFiles[i];
		const char* aWord = f.mStatus == "new" ? "new" : f.mStatus == "changes" ? "replaces" : "the same";
		r.Line(Fmt("  %s: %s (%s, %lu bytes, from %s)", aWord, f.mTo.c_str(), f.mWhat.c_str(), f.mSize, Backslashes(f.mSource).c_str()));
	}
	if (!aFileSkipped.empty())
	{
		r.Line(Fmt("Files left out: %d.", (int) aFileSkipped.size()));
		for (size_t i = 0; i < aFileSkipped.size(); i++)
			r.Line("  " + aFileSkipped[i].mRel + ": " + aFileSkipped[i].mWhy);
	}

	// ----- the copy of what is replaced, then the writes -----
	bool aFailed = false;
	std::string aBackupDir;
	std::vector<std::string> aReplaced;   // relative paths in the user dir
	if (aWriteRegistry)
	{
		std::string aOld;
		if (ReadFileBytes(aRegistryPath, &aOld))
			aReplaced.push_back("registry.ini");
	}
	for (size_t i = 0; i < aFiles.size(); i++)
		if (aFiles[i].mStatus == "changes")
			aReplaced.push_back(aFiles[i].mTo);
	r.Line();
	if (!aReplaced.empty())
	{
		std::string aBase = theUserDir + "/before-import-" + DateStamp();
		aBackupDir = aBase;
		for (int n = 2; IsDirW(Wide(Backslashes(aBackupDir))); n++)
			aBackupDir = aBase + Fmt("-%d", n);
		for (size_t i = 0; i < aReplaced.size() && !aFailed; i++)
		{
			std::string aData;
			if (!ReadFileBytes(theUserDir + "/" + aReplaced[i], &aData) || !WriteFileAtomic(aBackupDir + "/" + aReplaced[i], aData))
			{
				r.Line("Could not copy " + aReplaced[i] + " to " + Backslashes(aBackupDir) + ": nothing was imported.");
				aFailed = true;
			}
		}
		if (!aFailed)
			r.Line(Fmt("Copied the %d file%s that %s replaced to %s", (int) aReplaced.size(), aReplaced.size() == 1 ? "" : "s", aReplaced.size() == 1 ? "is" : "are", Backslashes(aBackupDir).c_str()));
	}
	else
		r.Line("Nothing in the user folder is replaced, so no copy was made.");

	int aWroteValues = 0, aWroteFiles = 0;
	if (!aFailed && aWriteRegistry)
	{
		if (WriteFileAtomic(aRegistryPath, WriteIni(aNew)))
			aWroteValues = aNewValues + aChangedValues;
		else
		{
			r.Line("Could not write " + Backslashes(aRegistryPath));
			aFailed = true;
		}
	}
	for (size_t i = 0; i < aFiles.size() && !aFailed; i++)
	{
		if (aFiles[i].mStatus == "same")
			continue;
		if (WriteFileAtomic(theUserDir + "/" + aFiles[i].mTo, aFileData[i]))
			aWroteFiles++;
		else
		{
			r.Line("Could not write " + Backslashes(theUserDir + "/" + aFiles[i].mTo));
			aFailed = true;
		}
	}
	r.Line(aFailed ? "The import stopped at an error." : Fmt("Done: %d registry value%s and %d file%s written.", aWroteValues, aWroteValues == 1 ? "" : "s", aWroteFiles, aWroteFiles == 1 ? "" : "s"));
	if (!aFailed && aWroteValues + aWroteFiles == 0)
		r.Line("There was nothing to import: nothing of the original's saves differs from the port's, or none was found.");

	WriteFileAtomic(aReportPath, r.mText);
	printf("%s", r.mText.c_str());
	printf("The report is in %s\n", Backslashes(aReportPath).c_str());
	fflush(stdout);
	*theExitCode = aFailed ? 1 : 0;
	return true;
}

}

#endif
