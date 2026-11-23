#ifndef __TALISMANIA_PATHDATAREADER_H__
#define __TALISMANIA_PATHDATAREADER_H__

// The reader of the path class, path image class and pool elements of a data file; the data handler offers every
// element to it.

#include "AtlantisData.h"
#include "Params.h"

class PathDataReader
{
public:
	explicit PathDataReader(AtlantisData& theData) : mData(theData) {}

	// True when the element is one of the paths' and pools' and has been read
	bool startElement(const std::string& theName, const engine::Params& p);
	bool endElement(const std::string& theName);

private:
	enum Context
	{
		IN_NOTHING,
		IN_PATH_IMAGE_CLASS,
		IN_PATH_ID_POOL,
		IN_CELL_SPAWNING_POOL
	};

	void beginPathClass(const engine::Params& p);
	void beginPathImageClass(const engine::Params& p);
	void addItem(const engine::Params& p);

	AtlantisData& mData;
	int mContext = IN_NOTHING;
	PathImageClass mPathImage;
	PathIdPoolClass mPathIdPool;
	CellSpawningPoolClass mSpawningPool;
	bool mInSection = false;  // inside a <random> or <selector>
};

#endif
