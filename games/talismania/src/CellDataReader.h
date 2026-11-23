#ifndef __TALISMANIA_CELLDATAREADER_H__
#define __TALISMANIA_CELLDATAREADER_H__

// The reader of the cell class elements of a data file (the loader 0x5a9a10 and the readers of its children: the layers
// 0x5abf10 and 0x5ac1c0, the sounds 0x5ae5e0 and the energize map); the data handler offers every element to it.

#include "AtlantisData.h"
#include "Params.h"

#include <set>

class CellDataReader
{
public:
	explicit CellDataReader(AtlantisData& theData) : mData(theData) {}

	// True when the element is one of the cell class's and has been read
	bool startElement(const std::string& theName, const engine::Params& p);
	bool endElement(const std::string& theName);

private:
	void beginCell(const engine::Params& p);
	CellLayerClass readLayer(const engine::Params& p) const;

	AtlantisData& mData;
	CellClass mCell;
	bool mInCell = false;
	std::set<std::string> mReported;
};

#endif
