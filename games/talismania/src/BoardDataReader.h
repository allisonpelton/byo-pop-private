#ifndef __TALISMANIA_BOARDDATAREADER_H__
#define __TALISMANIA_BOARDDATAREADER_H__

// The reader of the board elements of a data file (the loader 0x5b0e00 and the readers of its children); the data
// handler offers every element to it before the level reader.

#include "AtlantisData.h"
#include "Params.h"

class BoardDataReader
{
public:
	explicit BoardDataReader(AtlantisData& theData) : mData(theData) {}

	// True when the element is one of the board's and has been read
	bool startElement(const std::string& theName, const engine::Params& p);
	bool endElement(const std::string& theName);

private:
	PotClass readPot(const engine::Params& p) const;

	AtlantisData& mData;
	BoardClass mBoard;
	bool mInBoard = false;
	bool mInLayout = false;
	std::string mColumnTemplate;  // the <vspanclass> being read, empty outside one
};

#endif
