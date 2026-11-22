#include "PathDataReader.h"

#include "Engine.h"

bool PathDataReader::startElement(const std::string& theName, const engine::Params& p)
{
	if (theName == "pathclass")
		beginPathClass(p);
	else if (theName == "pathimageclass")
	{
		beginPathImageClass(p);
		mContext = IN_PATH_IMAGE_CLASS;
	}
	else if (theName == "pathidpoolclass")
	{
		mPathIdPool = PathIdPoolClass();
		mPathIdPool.name = p.getString("name", "");
		mContext = IN_PATH_ID_POOL;
		mInSection = false;
	}
	else if (theName == "cellspawningpoolclass")
	{
		mSpawningPool = CellSpawningPoolClass();
		mSpawningPool.name = p.getString("name", "");
		mContext = IN_CELL_SPAWNING_POOL;
		mInSection = false;
	}
	else if (mContext == IN_PATH_IMAGE_CLASS && theName == "endpoint")
	{
		// 0x5b87e0: the direction is taken modulo the number of directions; a picture needs an image
		int aDirection = ((p.getInt("direction", 0) % PATH_DIRECTION_COUNT) + PATH_DIRECTION_COUNT) % PATH_DIRECTION_COUNT;
		PathPicture aPicture;
		aPicture.image = p.getString("image", "");
		aPicture.fx = p.getString("fx", "");
		if (!aPicture.image.empty())
			mPathImage.endpoints[aDirection] = aPicture;
	}
	else if (mContext == IN_PATH_IMAGE_CLASS && theName == "connection")
	{
		// 0x5b8b20
		int aDirection = ((p.getInt("direction", 0) % PATH_DIRECTION_COUNT) + PATH_DIRECTION_COUNT) % PATH_DIRECTION_COUNT;
		std::string anImage = p.getString("image", "");
		if (!anImage.empty())
			mPathImage.connections[aDirection] = anImage;
	}
	else if ((mContext == IN_PATH_ID_POOL || mContext == IN_CELL_SPAWNING_POOL) &&
			 (theName == "random" || theName == "selector"))
	{
		PoolSection<PathIdItem> aPathIdSection;
		PoolSection<SpawnItem> aSpawnSection;
		aPathIdSection.kind = aSpawnSection.kind = theName == "random" ? POOL_RANDOM : POOL_SELECTOR;
		if (mContext == IN_PATH_ID_POOL)
			mPathIdPool.sections.push_back(aPathIdSection);
		else
			mSpawningPool.sections.push_back(aSpawnSection);
		mInSection = true;
	}
	else if ((mContext == IN_PATH_ID_POOL || mContext == IN_CELL_SPAWNING_POOL) && theName == "item")
		addItem(p);
	else
		return false;
	return true;
}

bool PathDataReader::endElement(const std::string& theName)
{
	if (theName == "pathimageclass")
	{
		mData.pathImageClasses[mPathImage.name] = mPathImage;
		mContext = IN_NOTHING;
	}
	else if (theName == "pathidpoolclass")
	{
		mData.pathIdPools[mPathIdPool.name] = mPathIdPool;
		mContext = IN_NOTHING;
	}
	else if (theName == "cellspawningpoolclass")
	{
		mData.cellSpawningPools[mSpawningPool.name] = mSpawningPool;
		mContext = IN_NOTHING;
	}
	else if (theName == "random" || theName == "selector")
	{
		if (mContext != IN_PATH_ID_POOL && mContext != IN_CELL_SPAWNING_POOL)
			return false;
		mInSection = false;
	}
	else if (theName == "pathclass" || ((theName == "endpoint" || theName == "connection") && mContext == IN_PATH_IMAGE_CLASS) ||
			 (theName == "item" && (mContext == IN_PATH_ID_POOL || mContext == IN_CELL_SPAWNING_POOL)))
		return true;
	else
		return false;
	return true;
}

// 0x5dcfb0 (the energizer's picture, 0x497530)
void PathDataReader::beginPathClass(const engine::Params& p)
{
	PathClass aClass;
	std::string aName = p.getString("name", "");
	std::string aBase = p.getString("baseclass", "");
	if (!aBase.empty())
	{
		std::map<std::string, PathClass>::iterator aBaseIt = mData.pathClasses.find(aBase);
		if (aBaseIt == mData.pathClasses.end())
			engine::getEngine()->log("PATH CLASS %s CANNOT FIND BASE CLASS %s\n", aName.c_str(), aBase.c_str());
		else
			aClass = aBaseIt->second;
	}
	aClass.name = aName;
	aClass.baseClass = aBase;
	aClass.type = p.getString("type", aClass.type);
	aClass.pathImageClass = p.getString("pathimageclass", aClass.pathImageClass);
	aClass.energizingPathImageClass = p.getString("energizingpathimageclass", aClass.energizingPathImageClass);
	aClass.constEnergizeRank = p.getInt("constenergizerank", 0);
	mData.pathClasses[aName] = aClass;
}

// 0x5b8030: unlike the other kinds the attributes' defaults are not the base class's values, only its pictures are kept
void PathDataReader::beginPathImageClass(const engine::Params& p)
{
	std::string aName = p.getString("name", "");
	std::string aBase = p.getString("baseclass", "");
	mPathImage = PathImageClass();
	if (!aBase.empty())
	{
		std::map<std::string, PathImageClass>::iterator aBaseIt = mData.pathImageClasses.find(aBase);
		if (aBaseIt == mData.pathImageClasses.end())
			engine::getEngine()->log("PATHIMAGECLASS %s CANNOT FIND BASE CLASS %s\n", aName.c_str(), aBase.c_str());
		else
			mPathImage = aBaseIt->second;
	}
	mPathImage.name = aName;
	mPathImage.baseClass = aBase;
	mPathImage.overlayPriority = p.getFloat("overlaypriority", 0.0f);
	mPathImage.energizeRank = (int)p.getFloat("energizerank", 0.0f);
	mPathImage.energizeColour = p.getString("energizecolour", "");
	mPathImage.popFx = p.getString("popfx", "");
	mPathImage.compatibleWith.clear();
	if (p.has("compatiblewith"))
		mPathImage.compatibleWith = splitList(p.getString("compatiblewith", ""));
	mPathImage.clashFx = p.getString("clashfx", "");
}

// An item of a pool: inside a <random> or <selector> it joins that section, outside it is a section of its own
void PathDataReader::addItem(const engine::Params& p)
{
	if (mContext == IN_PATH_ID_POOL)
	{
		PathIdItem anItem;
		anItem.pathId = p.getString("pathid", "");
		anItem.rotation = p.getString("rotation", "");
		anItem.weight = p.getFloat("weight", 0.0f);
		if (!mInSection)
			mPathIdPool.sections.push_back(PoolSection<PathIdItem>());
		mPathIdPool.sections.back().items.push_back(anItem);
	}
	else
	{
		SpawnItem anItem;
		anItem.cellClass = p.getString("cellclass", "");
		anItem.pathClass = p.getString("pathclass", "");
		anItem.pathId = p.getString("pathid", "");
		anItem.spawnTags = p.getString("spawntags", "");
		anItem.weight = p.getFloat("weight", 0.0f);
		if (!mInSection)
			mSpawningPool.sections.push_back(PoolSection<SpawnItem>());
		mSpawningPool.sections.back().items.push_back(anItem);
	}
}
