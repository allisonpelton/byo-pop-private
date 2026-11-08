#pragma once

// Test hooks shared by testhooks/Autoplay.cpp and testhooks/TestCommands.cpp

#include "Grid.h"

namespace Sexy
{

class Board;

// the pixels where real input makes theMove: the click, and a cell to move the mouse over first when the tile maker has to
// turn to the move's side (theHoverX is -1 when none is needed). False when no cell does it
bool TestFindCell(Board* theBoard, const Grid::Move& theMove, int* theHoverX, int* theHoverY, int* theX, int* theY);

}
