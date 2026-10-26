#include "VFS.h"
#include "Text.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <mutex>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

namespace Platform
{

VFS& VFS::Get()
{
	static VFS sVFS;
	return sVFS;
}

static std::string Lower(std::string s)
{
	for (char& c : s)
		c = (char)tolower((unsigned char)c);
	return s;
}

std::string VFS::Normalize(const std::string& thePath)
{
	return NormalizePath(thePath);
}

bool IsDirectory(const std::string& thePath)
{
	struct stat st;
	return stat(thePath.c_str(), &st) == 0 && (st.st_mode & S_IFDIR);
}

bool IsFile(const std::string& thePath)
{
	struct stat st;
	return stat(thePath.c_str(), &st) == 0 && (st.st_mode & S_IFREG);
}

std::vector<std::string> ListDirectory(const std::string& thePath)
{
	std::vector<std::string> names;
#ifdef _WIN32
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((thePath + "\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE)
		return names;
	do
	{
		std::string n = fd.cFileName;
		if (n != "." && n != "..")
			names.push_back(n);
	} while (FindNextFileA(h, &fd));
	FindClose(h);
#else
	DIR* d = opendir(thePath.c_str());
	if (!d)
		return names;
	while (dirent* e = readdir(d))
	{
		std::string n = e->d_name;
		if (n != "." && n != "..")
			names.push_back(n);
	}
	closedir(d);
#endif
	return names;
}

bool MakeDirs(const std::string& thePath)
{
	std::string p = thePath;
	std::replace(p.begin(), p.end(), '\\', '/');
	for (size_t i = 1; i <= p.size(); i++)
	{
		if (i == p.size() || p[i] == '/')
		{
			std::string sub = p.substr(0, i);
			if (sub.empty() || (sub.size() == 2 && sub[1] == ':') || IsDirectory(sub))
				continue;
#ifdef _WIN32
			_mkdir(sub.c_str());
#else
			mkdir(sub.c_str(), 0755);
#endif
		}
	}
	return IsDirectory(p);
}

// Directory listings are cached per real directory; asset folders don't change while the game runs
// except through the mod manager, which calls SetModRoots() and clears the cache.
static std::mutex gCacheMutex;
static std::map<std::string, std::map<std::string, std::string>> gDirCache;

static const std::map<std::string, std::string>& DirIndex(const std::string& theDir)
{
	std::lock_guard<std::mutex> lock(gCacheMutex);
	auto it = gDirCache.find(theDir);
	if (it != gDirCache.end())
		return it->second;
	std::map<std::string, std::string>& index = gDirCache[theDir];
	for (const std::string& n : ListDirectory(theDir))
		index.emplace(Lower(n), n);
	return index;
}

std::string VFS::FindCaseInsensitive(const std::string& theRoot, const std::string& theRelPath)
{
	std::string rel = Normalize(theRelPath);
	if (rel.empty())
		return "";
	std::string cur = theRoot;
	size_t start = 0;
	while (start < rel.size())
	{
		size_t end = rel.find('/', start);
		if (end == std::string::npos)
			end = rel.size();
		std::string part = rel.substr(start, end - start);
		std::string exact = cur + "/" + part;
		bool last = end == rel.size();
		if (last ? IsFile(exact) : IsDirectory(exact))
			cur = exact;
		else
		{
			const auto& index = DirIndex(cur);
			auto it = index.find(Lower(part));
			if (it == index.end())
				return "";
			cur = cur + "/" + it->second;
			if (last && !IsFile(cur))
				return "";
		}
		start = end + 1;
	}
	return cur;
}

void VFS::SetBaseDir(const std::string& theDir)
{
	mBaseDir = theDir.empty() ? "." : theDir;
	while (mBaseDir.size() > 1 && (mBaseDir.back() == '/' || mBaseDir.back() == '\\'))
		mBaseDir.pop_back();
	std::lock_guard<std::mutex> lock(gCacheMutex);
	gDirCache.clear();
}

void VFS::SetModRoots(const std::vector<VFSRoot>& theRoots)
{
	mModRoots = theRoots;
	std::lock_guard<std::mutex> lock(gCacheMutex);
	gDirCache.clear();
}

static bool IsAbsolute(const std::string& p)
{
	return (!p.empty() && (p[0] == '/' || p[0] == '\\')) || (p.size() > 1 && p[1] == ':');
}

void VFS::SetRemaps(const std::map<std::string, VFSRemap>& theRemaps)
{
	mRemaps.clear();
	for (const auto& kv : theRemaps)
		mRemaps[Lower(Normalize(kv.first))] = kv.second;
}

const VFSRemap* VFS::GetRemap(const std::string& theGamePath) const
{
	auto it = mRemaps.find(Lower(Normalize(theGamePath)));
	return it == mRemaps.end() ? nullptr : &it->second;
}

std::string VFS::ResolveIn(const std::string& theRoot, const std::string& theGamePath) const
{
	return FindCaseInsensitive(theRoot, theGamePath);
}

std::string VFS::Resolve(const std::string& theGamePath, std::string* theModId) const
{
	if (IsAbsolute(theGamePath))
		return IsFile(theGamePath) ? theGamePath : "";
	std::string rel = Normalize(theGamePath);
	// Save data lives outside the overlay.
	std::string lowerRel = Lower(rel);
	if (lowerRel.rfind("userdata/", 0) == 0)
	{
		std::string p = FindCaseInsensitive(mUserDir, rel.substr(9));
		return p;
	}
	auto remap = mRemaps.find(lowerRel);
	if (remap != mRemaps.end() && IsFile(remap->second.mRealPath))
	{
		if (theModId)
			*theModId = remap->second.mModId;
		return remap->second.mRealPath;
	}
	for (const VFSRoot& r : mModRoots)
	{
		std::string p = FindCaseInsensitive(r.mPath, rel);
		if (!p.empty())
		{
			if (theModId)
				*theModId = r.mModId;
			return p;
		}
	}
	std::string p = FindCaseInsensitive(mBaseDir, rel);
	if (!p.empty() && theModId)
		theModId->clear();
	return p;
}

FILE* VFS::Open(const std::string& theGamePath, const char* theMode) const
{
	bool writing = strchr(theMode, 'w') || strchr(theMode, 'a') || strchr(theMode, '+');
	if (writing)
	{
		// Writes always go to the user dir (for userdata/...) or to the literal path.
		std::string rel = Normalize(theGamePath);
		std::string target;
		if (IsAbsolute(theGamePath))
			target = theGamePath;
		else if (Lower(rel).rfind("userdata/", 0) == 0)
			target = mUserDir + "/" + rel.substr(9);
		else
			target = mUserDir + "/" + rel;
		size_t slash = target.find_last_of("/\\");
		if (slash != std::string::npos)
			MakeDirs(target.substr(0, slash));
		{
			std::lock_guard<std::mutex> lock(gCacheMutex);
			gDirCache.clear();
		}
		return fopen(target.c_str(), theMode);
	}
	std::string p = Resolve(theGamePath);
	if (p.empty())
		return nullptr;
	return fopen(p.c_str(), theMode);
}

void VFS::SetGeneratedFile(const std::string& theGamePath, const std::string& theContents)
{
	std::string key = Lower(Normalize(theGamePath));
	if (theContents.empty())
		mGenerated.erase(key);
	else
		mGenerated[key] = theContents;
}

bool VFS::ReadAll(const std::string& theGamePath, std::vector<uint8_t>& theData) const
{
	if (!mGenerated.empty() && !IsAbsolute(theGamePath))
	{
		auto g = mGenerated.find(Lower(Normalize(theGamePath)));
		std::string modId;
		if ((g != mGenerated.end()) && (Resolve(theGamePath, &modId).empty() || modId.empty()))
		{
			theData.assign(g->second.begin(), g->second.end());
			return true;
		}
	}
	FILE* f = Open(theGamePath, "rb");
	if (!f)
		return false;
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	theData.resize(len > 0 ? (size_t)len : 0);
	size_t got = len > 0 ? fread(theData.data(), 1, (size_t)len, f) : 0;
	fclose(f);
	return got == theData.size();
}

}
