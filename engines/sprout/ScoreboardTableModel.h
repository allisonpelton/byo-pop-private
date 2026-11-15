#ifndef __SPROUT_SCOREBOARDTABLEMODEL_H__
#define __SPROUT_SCOREBOARDTABLEMODEL_H__

// A scoreboard as the rows of a table (the exe's model, 0x5c bytes, constructor 0x490910 (pizzafrenzy)): a row for each
// entry, a cell for each of the entry's display parameters and the rank

#include "Scoreboard.h"
#include "TableModel.h"

namespace engine
{

class ScoreboardTableModel : public TableModel
{
public:
	explicit ScoreboardTableModel(Scoreboard* theBoard);  // 0x490910 (pizzafrenzy)

	// 0x04: 0x490500 (pizzafrenzy)
	int rowCount() override { return mBoard->getCount(); }
	// 0x08: 0x490710 (pizzafrenzy): "O_o" for a column the entry has none of
	std::string getCell(int theRow, const std::string& theColumn) override;
	// 0x0c: 0x490510 (pizzafrenzy)
	bool isReady() override { return mBoard->getState() == Scoreboard::STATE_READY; }
	// 0x10: 0x490540 (pizzafrenzy): the name of the board's state
	std::string getStatus() override;
	// 0x14: 0x490530 (pizzafrenzy): the entry just added
	int getSelectedRow() override { return mBoard->getLastAddedIndex(); }

private:
	void loadRow(int theRow);  // 0x490600 (pizzafrenzy)

	Ref<Scoreboard> mBoard;  // +0x1c
	Params mParams;          // +0x30: the parameters of the row loaded
	int mLoadedRow;          // +0x50
};

} // namespace engine

#endif
