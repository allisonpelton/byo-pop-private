// Board: what the test commands read of the game (the field), and, with the bot, the move they print.
// port: nothing here is in the original; none of it changes the game.

#include "Board.h"
#include "Brick.h"

using namespace Sexy;

// The field as a JSON array, one string for each of its rows (row 0 on top), one character for each column: a dot for no
// brick, a digit or letter for the kind of a brick that is still (the colours are 0 to 4, the rest the special kinds, a
// virus brick is c), a star for one that moves.
std::string Board::DescribeField()
{
	std::string aResult = "[";
	for (size_t aRow = 0; aRow < m314.size(); aRow++)
	{
		if (aRow > 0)
			aResult += ",";
		aResult += "\"";
		for (size_t aCol = 0; aCol < m314[aRow].size(); aCol++)
		{
			Brick* aBrick = m314[aRow][aCol];
			if (aBrick == NULL)
				aResult += '.';
			else if (!aBrick->IsIdle())
				aResult += '*';
			else
			{
				int aType = aBrick->GetEffectiveType();
				if (aType >= 0 && aType <= 9)
					aResult += (char) ('0' + aType);
				else if (aType >= 10 && aType < 36)
					aResult += (char) ('a' + (aType - 10));
				else
					aResult += '?';
			}
		}
		aResult += "\"";
	}
	aResult += "]";
	return aResult;
}
