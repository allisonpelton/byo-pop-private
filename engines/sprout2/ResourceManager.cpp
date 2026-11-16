#include "ResourceManager.h"
#include "Engine.h"

#include <algorithm>

namespace engine
{

namespace
{

std::string withSlashes(const std::string& thePath)
{
	std::string aPath = thePath;
	std::replace(aPath.begin(), aPath.end(), '\\', '/');
	return aPath;
}

} // namespace

void ResourceManager::addArchive(const std::string& thePath)
{
	Ref<ArchiveLoader> anArchive = new ArchiveLoader();
	anArchive->open(thePath);
	mArchives.push_back(anArchive);
}

bool ResourceManager::exists(const std::string& thePath)
{
	std::string aPath = withSlashes(thePath);
	if (mUseLooseFiles)
		return getEngine()->diskFileExists(aPath);
	for (const Ref<ArchiveLoader>& anArchive : mArchives)
	{
		if (anArchive->contains(aPath))
			return true;
	}
	return false;
}

InputStream* ResourceManager::openArchivedFile(const std::string& thePath)
{
	std::string aPath = withSlashes(thePath);
	for (const Ref<ArchiveLoader>& anArchive : mArchives)
	{
		if (anArchive->contains(aPath))
			return anArchive->getInputStream(aPath, mCheckDigests);
	}
	getEngine()->log("Unable to find file in archive: %s\n", aPath.c_str());
	return nullptr;
}

} // namespace engine
