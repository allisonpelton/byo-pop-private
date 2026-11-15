#include "ScoreboardTableModel.h"

#include <cstdio>

using namespace engine;

// 0x490910 (pizzafrenzy)
ScoreboardTableModel::ScoreboardTableModel(Scoreboard* theBoard) : mBoard(theBoard), mLoadedRow(-1)
{
}

// 0x490600 (pizzafrenzy): a row other than the loaded one is read again; its rank is shown as the number and a full
// stop
void ScoreboardTableModel::loadRow(int theRow)
{
	if (theRow != mLoadedRow)
	{
		ScoreEntry* anEntry = mBoard->getEntry(theRow);
		mParams.clear();
		anEntry->displayParams(mParams);
		char aRank[16];
		snprintf(aRank, sizeof(aRank), "%d. ", anEntry->getRank());
		mParams.setString("rank", aRank);
		mLoadedRow = theRow;
	}
}

// 0x490710 (pizzafrenzy)
std::string ScoreboardTableModel::getCell(int theRow, const std::string& theColumn)
{
	loadRow(theRow);
	return mParams.getString(theColumn, "O_o");
}

// 0x490540 (pizzafrenzy)
std::string ScoreboardTableModel::getStatus()
{
	switch (mBoard->getState())
	{
	case Scoreboard::STATE_CREATED:
		return "CREATED";
	case Scoreboard::STATE_LOADING:
		return "LOADING";
	case Scoreboard::STATE_READY:
		return "READY";
	case Scoreboard::STATE_ERROR:
		return "ERROR";
	}
	return ">_<";
}
