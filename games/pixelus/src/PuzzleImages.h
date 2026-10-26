#ifndef __PIXELUS_PUZZLEIMAGES_H__
#define __PIXELUS_PUZZLEIMAGES_H__

// PuzzleImages.cpp: the picture a puzzle makes and the little pictures of it (the second half of Grid 0x426910 and
// the mini Grid's Draw 0x422ab0 with 6x6 cells, which the temple screen and the puzzle-completed screen use).
// Grid itself is free of the framework, so these two live here.

namespace Sexy
{

class PixelusApp;
class Grid;
class MemoryImage;

// 0x426910: reads images/levels/<name>.gif (one pixel per cell) into Grid::mPicture, saturation and brightness
// raised by a third and a tenth. False when the puzzle has no picture.
bool LoadPuzzlePicture(PixelusApp* theApp, Grid* theGrid);

// 96x96: the puzzle as little cells. Without the picture the cells that want a tile show as a pale shape, with it
// they are the picture's colours.
MemoryImage* MakeMiniImage(PixelusApp* theApp, const Grid& theGrid, bool thePicture);

// the finished puzzle at 20 pixels a cell: a gray tile of the picture's colour on every cell that wanted one (what the
// puzzle-completed screen shows in its frame; 0x4344d0 draws the grid into a new image)
MemoryImage* MakeSolvedImage(PixelusApp* theApp, const Grid& theGrid);

}

#endif
