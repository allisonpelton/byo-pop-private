// The autoplay bot's choice of a column, shared by -autoplay (Autoplay.cpp) and the script command `move`
// (Commands.cpp). It reads the board and moves nothing; its dice are the bot's own (theSeed), never the game's.
#pragma once

#include <cstdint>

namespace tiptop
{
struct Board;
struct Handler;
}

namespace Sexy
{

// One draw of the bot's generator (the one -autoplay always used): a number in [0, theRange)
int BotRand(unsigned int& theSeed, int theRange);

// The column to drop the board's next block into: the biggest group of its kind it joins (three blocks or more),
// else one of the emptiest columns; -1 if none is free. The draws are made on theSeed in the order -autoplay's logs
// depend on.
int BotChooseColumn(tiptop::Board* theBoard, unsigned int& theSeed);

// What Enter would press: a button of the top modal layer that Enter presses, or the level summary's continue item once it
// takes input; nullptr if none
tiptop::Handler* BotEnterTarget();

// The button of the top modal layer that sends theCommand (the command without its key flags); nullptr if none
tiptop::Handler* BotButton(uint32_t theCommand);

// The click that drops a block into a column, in the game's own coordinates (640 x 480)
void BotColumnToGame(int theColumn, int* theGameX, int* theGameY);

}
