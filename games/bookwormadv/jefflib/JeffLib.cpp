#include "JeffLib.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

#include "platform/VFS.h"

namespace JeffLib {

// 0x6a8e10: the last of the digits from i on (i - 1 when there is none)
static int LastDigit(const std::string& s, int i)
{
	while (i < (int)s.size() && isdigit((unsigned char)s[i]))
		i++;
	return i - 1;
}

bool StringLessNatural(const std::string& a, const std::string& b)
{
	int aLen = (int)a.size();
	int bLen = (int)b.size();
	int i = 0;
	int j = 0;
	while (i < aLen && j < bLen) {
		char ca = (char)tolower((unsigned char)a[i]);
		char cb = (char)tolower((unsigned char)b[j]);
		bool aDigit = isdigit((unsigned char)ca) != 0;
		bool bDigit = isdigit((unsigned char)cb) != 0;
		int aStart = i;
		int bStart = j;
		if (aDigit)
			i = LastDigit(a, i);
		if (bDigit)
			j = LastDigit(b, j);
		if (aDigit) {
			if (!bDigit)
				return ca < cb;
			long aNum = atol(a.substr(aStart, i - aStart + 1).c_str());
			long bNum = atol(b.substr(bStart, j - bStart + 1).c_str());
			if (aNum != bNum)
				return aNum < bNum;
		} else {
			if (bDigit || ca != cb)
				return ca < cb;
		}
		i++;
		j++;
	}
	return aLen < bLen;
}

// The * and ? of a FindFirstFile pattern, ignoring case
static bool WildMatch(const char* p, const char* s)
{
	for (; *p; p++, s++) {
		if (*p == '*') {
			for (const char* t = s;; t++) {
				if (WildMatch(p + 1, t))
					return true;
				if (!*t)
					return false;
			}
		}
		if (!*s || (*p != '?' && tolower((unsigned char)*p) != tolower((unsigned char)*s)))
			return false;
	}
	return !*s;
}

// A folder under a real folder, its name in any case; "" when there is none. (VFS::FindCaseInsensitive finds
// files only.)
static std::string FindDirectory(const std::string& theRoot, const std::string& theRelPath)
{
	std::string aPath = theRoot;
	size_t aStart = 0;
	while (aStart < theRelPath.size()) {
		size_t anEnd = theRelPath.find('/', aStart);
		if (anEnd == std::string::npos)
			anEnd = theRelPath.size();
		std::string aPart = theRelPath.substr(aStart, anEnd - aStart);
		aStart = anEnd + 1;
		if (aPart.empty() || aPart == ".")
			continue;
		std::string anExact = aPath + "/" + aPart;
		if (Platform::IsDirectory(anExact)) {
			aPath = anExact;
			continue;
		}
		std::string aFound;
		for (const std::string& aName : Platform::ListDirectory(aPath))
			if (aName.size() == aPart.size() && WildMatch(aPart.c_str(), aName.c_str()) &&
			    Platform::IsDirectory(aPath + "/" + aName))
				aFound = aName;
		if (aFound.empty())
			return "";
		aPath += "/" + aFound;
	}
	return aPath;
}

std::vector<std::string> GetFilesInDirectory(const std::string& thePattern)
{
	std::string aPattern = thePattern;
	std::replace(aPattern.begin(), aPattern.end(), '\\', '/');
	size_t aSlash = aPattern.find_last_of('/');
	std::string aDir = aSlash == std::string::npos ? "" : aPattern.substr(0, aSlash);
	std::string aName = aSlash == std::string::npos ? aPattern : aPattern.substr(aSlash + 1);

	std::vector<std::string> aFiles;
	Platform::VFS& aVFS = Platform::VFS::Get();
	for (const std::string& aRoot : {aVFS.GetUserDir(), aVFS.GetBaseDir()}) {
		std::string aRealDir = aDir.empty() ? aRoot : FindDirectory(aRoot, aDir);
		if (aRealDir.empty() || !Platform::IsDirectory(aRealDir))
			continue;
		for (const std::string& aFile : Platform::ListDirectory(aRealDir))
			if (WildMatch(aName.c_str(), aFile.c_str()))
				aFiles.push_back(aFile);
		break;
	}
	std::sort(aFiles.begin(), aFiles.end());
	return aFiles;
}

}  // namespace JeffLib
