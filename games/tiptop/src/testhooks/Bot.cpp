// The autoplay bot's choice of a column (Bot.h): it drops the next block where it joins the biggest group of its
// kind; when no column makes a match it picks one of the emptiest columns at random.

#include "Bot.h"
#include "Board.h"
#include "Config.h"
#include "Gui.h"

#include <string>

namespace tiptop
{
uint32_t SummaryItemProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);  // App.cpp, 0x424e66
}

namespace Sexy
{

int BotRand(unsigned int& theSeed, int theRange)
{
	theSeed = theSeed * 1103515245u + 12345u;
	return (int)((theSeed >> 16) % (unsigned int)theRange);
}

namespace
{

// The cells of the same kind joined to theIndex (theKind placed there), counted once each
int GroupSize(tiptop::Field* theField, int theIndex, int theKind, std::string& theSeen)
{
	int aColumns = (int)theField->mColumns;
	int aCount = (int)theField->mColumns * (int)theField->mRows;
	if (theIndex < 0 || theIndex >= aCount || theSeen[theIndex] != 0)
		return 0;
	theSeen[theIndex] = 1;
	int aSize = 1;
	int aColumn = theIndex % aColumns;
	const int aNeighbors[4] = {theIndex - aColumns, theIndex + aColumns, aColumn > 0 ? theIndex - 1 : -1,
		aColumn < aColumns - 1 ? theIndex + 1 : -1};
	for (int i = 0; i < 4; i++)
	{
		int aNext = aNeighbors[i];
		if (aNext < 0 || aNext >= aCount)
			continue;
		tiptop::Piece* aPiece = theField->PieceAt((uint32_t)aNext);
		if (aPiece != nullptr && aPiece->mKind == theKind)
			aSize += GroupSize(theField, aNext, theKind, theSeen);
	}
	return aSize;
}

} // namespace

int BotChooseColumn(tiptop::Board* theBoard, unsigned int& theSeed)
{
	tiptop::Field* aField = theBoard->mField;
	int aColumns = (int)aField->mColumns;
	int aRows = (int)aField->mRows;
	int aKind = theBoard->mNextBlocks[0].mKind;
	int aBest = -1;
	int aBestSize = 0;
	int aBestCount = 0;
	int aMostRoom = -1;
	int aRoomCount = 0;
	int aRoomColumn = -1;
	for (int c = 0; c < aColumns; c++)
	{
		if (aField->CanPlace(c) == 0)
			continue;
		// the block rises from the bottom row until it meets a block
		int aRow = 0;
		while (aRow + 1 < aRows && aField->PieceAt((uint32_t)((aRow + 1) * aColumns + c)) == nullptr)
			aRow++;
		std::string aSeen((size_t)(aColumns * aRows), '\0');
		int aSize = (aKind < 10) ? GroupSize(aField, aRow * aColumns + c, aKind, aSeen) : 0;
		if (aSize > aBestSize)
		{
			aBest = c;
			aBestSize = aSize;
			aBestCount = 1;
		}
		else if (aSize == aBestSize && aBest >= 0 && BotRand(theSeed, ++aBestCount) == 0)
		{
			aBest = c;
		}
		if (aRow > aMostRoom)
		{
			aMostRoom = aRow;
			aRoomColumn = c;
			aRoomCount = 1;
		}
		else if (aRow == aMostRoom && BotRand(theSeed, ++aRoomCount) == 0)
		{
			aRoomColumn = c;
		}
	}
	if (aBestSize >= 3)
		return aBest;
	return aRoomColumn;
}

tiptop::Handler* BotEnterTarget()
{
	uint16_t aCount = 0;
	tiptop::Handler* aTable = tiptop::HandlerTable(&aCount);
	for (int i = 0; i < aCount; i++)
	{
		tiptop::Handler* h = &aTable[i];
		if (h->mProc == nullptr || tiptop::InTopModal(h->mLayer) == 0)
			continue;
		if ((h->mProc == tiptop::ButtonProc || h->mProc == tiptop::ShapedButtonProc) &&
			(h->u.button.mCommand & 0x80000000) != 0)
			return h;
		if (h->mProc == tiptop::SummaryItemProc && h->u.mSlots[0] == 4 && (h->u.mSlots[3] & 4) != 0)
			return h;
	}
	return nullptr;
}

tiptop::Handler* BotButton(uint32_t theCommand)
{
	uint16_t aCount = 0;
	tiptop::Handler* aTable = tiptop::HandlerTable(&aCount);
	for (int i = 0; i < aCount; i++)
	{
		tiptop::Handler* h = &aTable[i];
		if (h->mProc != nullptr && tiptop::InTopModal(h->mLayer) != 0 &&
			(h->mProc == tiptop::ButtonProc || h->mProc == tiptop::ShapedButtonProc) &&
			(h->u.button.mCommand & 0x3fffffff) == theCommand)
			return h;
	}
	return nullptr;
}

void BotColumnToGame(int theColumn, int* theGameX, int* theGameY)
{
	*theGameX = tiptop::gConfig.mBlockgridX + theColumn * tiptop::gConfig.mBlockWidth + tiptop::gConfig.mBlockWidth / 2;
	*theGameY = tiptop::gConfig.mBlockgridY + tiptop::gConfig.mBlockHeight * tiptop::gConfig.mNumRows / 2;
}

} // namespace Sexy
