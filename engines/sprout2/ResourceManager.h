#ifndef __SPROUT2_RESOURCEMANAGER_H__
#define __SPROUT2_RESOURCEMANAGER_H__

// The engine's archives (the engine's +0x158; 0x2c bytes): files are looked for in each archive in the order they were
// added, by their names with '/' (a '\' is turned into one).

#include "Archive.h"

#include <string>
#include <vector>

namespace engine
{

class ResourceManager : public Object
{
public:
	// 0x49fad0 (feedingfrenzy2)
	void addArchive(const std::string& thePath);
	// 0x45aab0 (feedingfrenzy2): in an archive, or on the disk when loose files are used
	bool exists(const std::string& thePath);
	// 0x45d7e0 (feedingfrenzy2): the first archive's that has it; null (logged) when none has
	InputStream* openArchivedFile(const std::string& thePath);

	bool mUseLooseFiles = false;  // +0x1c
	bool mCheckDigests = false;   // +0x20

private:
	std::vector<Ref<ArchiveLoader>> mArchives;  // +0x0c
};

} // namespace engine

#endif
