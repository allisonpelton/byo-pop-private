#ifndef __PLATFORM_VFS_H__
#define __PLATFORM_VFS_H__

// Virtual file system used by every asset load in the port.
//
// - Paths are game-relative ("images/gemsheet6", "data\\diff.txt"); '\\' and '/' are interchangeable.
// - Lookups are case-insensitive, because the game's code and its files disagree on case
//   ("music\\1980-autonomus.s3m" vs "1980-Autonomus.s3m") and MEMFS/Linux are case-sensitive.
// - Search roots form an overlay: enabled mods are searched first (highest priority first), then the
//   base game directory. A mod only has to contain the files it changes.

#include <map>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>

namespace Platform
{

struct VFSRoot
{
	std::string mPath;   // absolute or cwd-relative directory on the host FS (no trailing slash)
	std::string mModId;  // empty for the base game
};

// A mod-declared redirect of one game path to a file of the mod ("files" in mod.json), e.g. so
// "music/1984-network.s3m" plays a module or Ogg track with a different name and format.
struct VFSRemap
{
	std::string mRealPath;  // file on the host FS
	std::string mModId;
	int mStartOrder = -1;   // music: module order to start from (-1 = song start / caller's offset)
	double mStartTime = 0;  // music streams: playback start position in seconds
	double mLoopStart = 0;  // music streams: loop restart position in seconds
	double mVolume = 1.0;   // music: per-track gain multiplier
};

class VFS
{
public:
	static VFS& Get();

	// Base game data directory (the folder that contains images/, data/, sounds/, music/, properties/).
	void SetBaseDir(const std::string& theDir);
	const std::string& GetBaseDir() const { return mBaseDir; }

	// Mod overlay roots, highest priority first. Replaces the current list.
	void SetModRoots(const std::vector<VFSRoot>& theRoots);
	const std::vector<VFSRoot>& GetModRoots() const { return mModRoots; }

	// Explicit redirects (keys are game paths); they win over every overlay root.
	void SetRemaps(const std::map<std::string, VFSRemap>& theRemaps);
	const VFSRemap* GetRemap(const std::string& theGamePath) const;

	// Resolves a game-relative path to a real host path, searching mods then the base dir.
	// Returns an empty string if the file does not exist anywhere.
	std::string Resolve(const std::string& theGamePath, std::string* theModId = nullptr) const;

	// Resolves inside a single root only (used for "@2x" and alpha-mask lookups that must come from
	// the same mod as their colour image).
	std::string ResolveIn(const std::string& theRoot, const std::string& theGamePath) const;

	bool Exists(const std::string& theGamePath) const { return !Resolve(theGamePath).empty(); }
	FILE* Open(const std::string& theGamePath, const char* theMode) const;
	// Reads a whole file; a generated file (below) is read here too, where no mod has the path.
	bool ReadAll(const std::string& theGamePath, std::vector<uint8_t>& theData) const;

	// Generated files: contents a game makes at start-up (its code-drawn fonts' descriptors), read under a game path
	// as if the game folder held them. ReadAll, and so p_fopen, serves one ahead of the base dir and behind the mods;
	// Resolve, Open and Exists do not see it, since it has no host path. Empty contents remove the file. Set them
	// before loading starts: the loading thread reads them without a lock.
	void SetGeneratedFile(const std::string& theGamePath, const std::string& theContents);
	void ClearGeneratedFiles() { mGenerated.clear(); }

	// Normalizes separators to '/', strips "./" and leading slashes, collapses "a/../" (Platform::NormalizePath).
	static std::string Normalize(const std::string& thePath);

	// Case-insensitive lookup of a relative path under a real directory. Returns the real path or "".
	static std::string FindCaseInsensitive(const std::string& theRoot, const std::string& theRelPath);

	// Writable location for saves/settings ("userdata" in the original).
	void SetUserDir(const std::string& theDir) { mUserDir = theDir; }
	const std::string& GetUserDir() const { return mUserDir; }

private:
	std::string mBaseDir = ".";
	std::string mUserDir = "userdata";
	std::vector<VFSRoot> mModRoots;
	std::map<std::string, VFSRemap> mRemaps;  // key: lower-case normalized game path
	std::map<std::string, std::string> mGenerated;  // key: lower-case normalized game path
};

// Directory helpers that work on native hosts and Emscripten's FS.
bool IsDirectory(const std::string& thePath);
bool IsFile(const std::string& thePath);
std::vector<std::string> ListDirectory(const std::string& thePath);  // names only, no "." / ".."
bool MakeDirs(const std::string& thePath);

}

#endif
