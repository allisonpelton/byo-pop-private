#ifndef __BONNIES_BOARD_H__
#define __BONNIES_BOARD_H__

// The board's size and the other counts that the game's tables and loops share.

namespace bonnies
{

constexpr int BOARD_COLUMNS = 9;                         // the columns of cells
constexpr int BOARD_ROWS = 9;                            // the rows (a cell is column * BOARD_ROWS + row)
constexpr int BOARD_CELLS = BOARD_COLUMNS * BOARD_ROWS;  // 81
constexpr int PATH_LETTERS = 20;                         // the most tiles on the path, and in a hint's word
constexpr int LEVEL_COUNT = 50;                          // the levels, a book chapter each

} // namespace bonnies

#endif
