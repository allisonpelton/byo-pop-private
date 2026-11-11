#ifndef __BOARDLAYOUT_H__
#define __BOARDLAYOUT_H__

// port(variant): the geometry of one level's board (games/bejeweled/docs/CODE_MODS_DESIGN.md 5.1, 5.3). Not
// part of the original game. Every board is simulated in logical 51-px space with the original constants;
// only drawing and hit-testing scale it into the original 406x406 window:
//
//   w = cols*51 - 2;  h = rows*51 - 2;  D = max(406, w, h)            8x8: w = h = D = 406
//   mOriginX = 203 + (D - w)/2;  mOriginY = 16 + (D - h)/2            GetColX(c) = c*51 + mOriginX
//   mScale = 406.0 / D                                                1 unless D > 406
//   ToScreen(lx, ly) = (203 + (lx - 203)*s, 16 + (ly - 16)*s)
//   ToLogical(sx, sy) = (203 + floordiv((sx - 203)*D, 406), 16 + floordiv((sy - 16)*D, 406))
//   mScreenBottom = 16 + (480 - 16)*D/406                             480 when D == 406
//   clip (screen) = ToScreen(origin) - (9, 4) .. ToScreen(origin + (w, h)) + (13, 6)
//                                                                     8x8: (194, 12)..(622, 428), the original's
//
// The default layout is exactly the original constants: 8x8, every cell playable, origin (203, 16),
// D 406, scale 1, screen bottom 480. Header only and free of framework headers, so the rules code
// (GameRules) and its tests can use it.

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace Sexy
{

class BoardLayout
{
public:
	enum
	{
		MAX_COLS = 16,
		MAX_ROWS = 16,
		MAX_DIRT_LAYERS = 9,
		PITCH = 51,                 // Board::CELL_SIZE
		VIEW_X = 203,               // Board::BOARD_X: left edge of the 406x406 board window
		VIEW_Y = 16,                // Board::BOARD_Y
		VIEW_SIZE = 406,            // the original board: 8*51 - 2
		VIEW_BOTTOM = 480           // Board::mHeight, the original fall limit
	};

	// Cell kinds (board.mask legend, games/bejeweled/docs/CODE_MODS_DESIGN.md 3.2 and 4.14)
	enum CellKind
	{
		CELL_HOLE = 0,              // '.': never holds a gem
		CELL_GEM = 1,               // '#': a playable cell
		CELL_DIRT = 2               // 'd' (1 layer) or '2'..'9' (that many layers): blocks falls until cleared
	};

	struct Segment
	{
		int mStart;
		int mLength;
	};

	int				mCols;
	int				mRows;
	unsigned char	mCell[MAX_ROWS][MAX_COLS];  // CellKind; cells outside mCols x mRows are holes
	unsigned char	mDirt[MAX_ROWS][MAX_COLS];  // dirt layers of a CELL_DIRT cell, else 0
	int				mOriginX;                   // logical GetColX(0)
	int				mOriginY;                   // logical GetRowY(0)
	int				mExtent;                    // D
	double			mScale;                     // view scale 406.0 / D
	int				mScreenBottom;              // fall limit (Board::mHeight in the original)
	// The gem layer's clip in screen space (CODE_MODS_DESIGN.md 4.16): the board's own rectangle, the original
	// ClipRect(194, 12, 428, 416) on 8x8; and its bottom edge in logical space (GetPieceAt's guard)
	int				mClipX;
	int				mClipY;
	int				mClipWidth;
	int				mClipHeight;
	int				mClipBottom;

	BoardLayout()
	{
		SetFull(8, 8);
	}

	// A full theCols x theRows board (no holes, no dirt)
	void SetFull(int theCols, int theRows)
	{
		mCols = Clamp(theCols, 1, MAX_COLS);
		mRows = Clamp(theRows, 1, MAX_ROWS);
		for (int aRow = 0; aRow < MAX_ROWS; aRow++)
		{
			for (int aCol = 0; aCol < MAX_COLS; aCol++)
			{
				mCell[aRow][aCol] = ((aRow < mRows) && (aCol < mCols)) ? CELL_GEM : CELL_HOLE;
				mDirt[aRow][aCol] = 0;
			}
		}
		ComputeGeometry();
	}

	// A board from mask strings (one per row, all the same length); false on a malformed mask, which
	// leaves the layout full theCols x theRows. An empty mask is a full board.
	bool SetMask(int theCols, int theRows, const std::vector<std::string>& theMask)
	{
		SetFull(theCols, theRows);
		if (theMask.empty())
			return true;
		if (!MaskIsWellFormed(theMask))
			return false;
		mCols = (int) theMask[0].size();
		mRows = (int) theMask.size();
		for (int aRow = 0; aRow < MAX_ROWS; aRow++)
		{
			for (int aCol = 0; aCol < MAX_COLS; aCol++)
			{
				char aChar = ((aRow < mRows) && (aCol < mCols)) ? theMask[aRow][aCol] : '.';
				int aLayers = DirtLayers(aChar);
				mCell[aRow][aCol] = (aChar == '#') ? CELL_GEM : (aLayers > 0) ? CELL_DIRT : CELL_HOLE;
				mDirt[aRow][aCol] = (unsigned char) aLayers;
			}
		}
		ComputeGeometry();
		return true;
	}

	// Mask legend: '#' cell, '.' hole, 'd' dirt with one layer, '2'..'9' dirt with that many layers
	static bool IsMaskChar(char theChar)
	{
		return (theChar == '#') || (theChar == '.') || (DirtLayers(theChar) > 0);
	}

	static int DirtLayers(char theChar)
	{
		if (theChar == 'd')
			return 1;
		if ((theChar >= '2') && (theChar <= '9'))
			return theChar - '0';
		return 0;
	}

	static bool MaskIsWellFormed(const std::vector<std::string>& theMask)
	{
		if (theMask.empty() || (theMask.size() > MAX_ROWS) || theMask[0].empty() || (theMask[0].size() > MAX_COLS))
			return false;
		for (size_t i = 0; i < theMask.size(); i++)
		{
			if (theMask[i].size() != theMask[0].size())
				return false;
			for (size_t j = 0; j < theMask[i].size(); j++)
			{
				if (!IsMaskChar(theMask[i][j]))
					return false;
			}
		}
		return true;
	}

	void ComputeGeometry()
	{
		int aWidth = mCols * PITCH - 2;
		int aHeight = mRows * PITCH - 2;
		mExtent = VIEW_SIZE;
		if (aWidth > mExtent)
			mExtent = aWidth;
		if (aHeight > mExtent)
			mExtent = aHeight;
		mOriginX = VIEW_X + (mExtent - aWidth) / 2;
		mOriginY = VIEW_Y + (mExtent - aHeight) / 2;
		mScale = (double) VIEW_SIZE / mExtent;
		mScreenBottom = VIEW_Y + (VIEW_BOTTOM - VIEW_Y) * mExtent / VIEW_SIZE;
		mClipX = (int) std::floor(ToScreenX(mOriginX) + 0.5) - 9;
		mClipY = (int) std::floor(ToScreenY(mOriginY) + 0.5) - 4;
		mClipWidth = (int) std::floor(ToScreenX(mOriginX + aWidth) + 0.5) + 13 - mClipX;
		mClipHeight = (int) std::floor(ToScreenY(mOriginY + aHeight) + 0.5) + 6 - mClipY;
		int aClipX = mClipX;
		mClipBottom = mClipY + mClipHeight;
		ToLogical(&aClipX, &mClipBottom);
	}

	bool InBounds(int theCol, int theRow) const
	{
		return (theCol >= 0) && (theCol < mCols) && (theRow >= 0) && (theRow < mRows);
	}

	// A cell that holds a gem (not a hole, not dirt)
	bool IsPlayable(int theCol, int theRow) const
	{
		return InBounds(theCol, theRow) && (mCell[theRow][theCol] == CELL_GEM);
	}

	bool IsDirt(int theCol, int theRow) const
	{
		return InBounds(theCol, theRow) && (mCell[theRow][theCol] == CELL_DIRT);
	}

	// Gem cells plus dirt cells (the "at least 9 cells" check counts dirt)
	int CountCells() const
	{
		int aCount = 0;
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				if (mCell[aRow][aCol] != CELL_HOLE)
					aCount++;
			}
		}
		return aCount;
	}

	bool HasDirt() const
	{
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				if (mCell[aRow][aCol] == CELL_DIRT)
					return true;
			}
		}
		return false;
	}

	// Dirt (CODE_MODS_DESIGN.md 4.14): one layer off a dirt cell, which becomes a gem cell when its last layer goes. False when the
	// cell is no dirt
	bool ClearDirtLayer(int theCol, int theRow)
	{
		if (!IsDirt(theCol, theRow))
			return false;
		if (--mDirt[theRow][theCol] == 0)
			mCell[theRow][theCol] = CELL_GEM;
		return true;
	}

	// A dirt cell of the mask with theLayers left (a save, a snapshot): 0 makes it a gem cell
	void SetDirtLayers(int theCol, int theRow, int theLayers)
	{
		mDirt[theRow][theCol] = (unsigned char) theLayers;
		mCell[theRow][theCol] = (theLayers > 0) ? CELL_DIRT : CELL_GEM;
	}

	// A v2 save's cell mask (gem cells, row-major) fits this layout of the level's mask: the same gem cells and holes,
	// and a dirt cell either way, since its last layer may be gone (4.14). With theLayers (its DIRT chunk's, row-major)
	// a dirt cell has at most the mask's layers, and some exactly where it is no gem cell; any other cell none
	bool FitsSaveMask(int theCols, int theRows, const std::vector<bool>& thePlayable, const std::vector<uint8_t>* theLayers = NULL) const
	{
		if ((theCols != mCols) || (theRows != mRows) || (thePlayable.size() != (size_t)(mCols * mRows)))
			return false;
		if ((theLayers != NULL) && (theLayers->size() != thePlayable.size()))
			return false;
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				int i = aRow * mCols + aCol;
				bool isDirt = (mCell[aRow][aCol] == CELL_DIRT);
				if (!isDirt && (thePlayable[i] != (mCell[aRow][aCol] == CELL_GEM)))
					return false;
				if (theLayers == NULL)
					continue;
				int aLayers = (*theLayers)[i];
				if (!isDirt ? (aLayers != 0) : ((aLayers > mDirt[aRow][aCol]) || ((aLayers > 0) == thePlayable[i])))
					return false;
			}
		}
		return true;
	}

	// Every cell of mCols x mRows is a gem cell
	bool IsFull() const
	{
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				if (mCell[aRow][aCol] != CELL_GEM)
					return false;
			}
		}
		return true;
	}

	// The original board: a full 8x8
	bool IsOriginal() const
	{
		return (mCols == 8) && (mRows == 8) && IsFull();
	}

	// Gem cell rows of a column, top to bottom; returns the count
	int PlayableRows(int theCol, int* theRows) const
	{
		int aCount = 0;
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			if (IsPlayable(theCol, aRow))
				theRows[aCount++] = aRow;
		}
		return aCount;
	}

	// Maximal runs of consecutive gem cells along a row / a column; returns the count
	int RowSegments(int theRow, Segment* theSegments) const
	{
		int aCount = 0;
		for (int aCol = 0; aCol < mCols; )
		{
			if (!IsPlayable(aCol, theRow))
			{
				aCol++;
				continue;
			}
			int aStart = aCol;
			while (IsPlayable(aCol, theRow))
				aCol++;
			theSegments[aCount].mStart = aStart;
			theSegments[aCount].mLength = aCol - aStart;
			aCount++;
		}
		return aCount;
	}

	int ColSegments(int theCol, Segment* theSegments) const
	{
		int aCount = 0;
		for (int aRow = 0; aRow < mRows; )
		{
			if (!IsPlayable(theCol, aRow))
			{
				aRow++;
				continue;
			}
			int aStart = aRow;
			while (IsPlayable(theCol, aRow))
				aRow++;
			theSegments[aCount].mStart = aStart;
			theSegments[aCount].mLength = aRow - aStart;
			aCount++;
		}
		return aCount;
	}

	// The longest straight (row or column) segment of gem cells
	int LongestLine() const
	{
		Segment aSegments[MAX_COLS];
		int aLongest = 0;
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			int aCount = RowSegments(aRow, aSegments);
			for (int i = 0; i < aCount; i++)
				aLongest = (aSegments[i].mLength > aLongest) ? aSegments[i].mLength : aLongest;
		}
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			int aCount = ColSegments(aCol, aSegments);
			for (int i = 0; i < aCount; i++)
				aLongest = (aSegments[i].mLength > aLongest) ? aSegments[i].mLength : aLongest;
		}
		return aLongest;
	}

	// A full 2x2 block of gem cells somewhere (twist moves need one)
	bool HasFullSquare() const
	{
		for (int aRow = 0; aRow + 1 < mRows; aRow++)
		{
			for (int aCol = 0; aCol + 1 < mCols; aCol++)
			{
				if (IsPlayable(aCol, aRow) && IsPlayable(aCol + 1, aRow) && IsPlayable(aCol, aRow + 1) && IsPlayable(aCol + 1, aRow + 1))
					return true;
			}
		}
		return false;
	}

	// Logical <-> screen (only scaled boards, D > 406, differ)
	double ToScreenX(double theX) const
	{
		return VIEW_X + (theX - VIEW_X) * mScale;
	}

	double ToScreenY(double theY) const
	{
		return VIEW_Y + (theY - VIEW_Y) * mScale;
	}

	void ToLogical(int* theX, int* theY) const
	{
		*theX = VIEW_X + FloorDiv((*theX - VIEW_X) * mExtent, VIEW_SIZE);
		*theY = VIEW_Y + FloorDiv((*theY - VIEW_Y) * mExtent, VIEW_SIZE);
	}

private:
	static int Clamp(int theValue, int theMin, int theMax)
	{
		return (theValue < theMin) ? theMin : (theValue > theMax) ? theMax : theValue;
	}

	static int FloorDiv(int theNum, int theDen)
	{
		int aQuot = theNum / theDen;
		if ((theNum % theDen != 0) && (theNum < 0))
			aQuot--;
		return aQuot;
	}
};

}

#endif
