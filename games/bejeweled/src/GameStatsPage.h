#ifndef __GAMESTATSPAGE_H__
#define __GAMESTATSPAGE_H__

// port(variant): the game-over screen's Stats page (features.gameStats, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.19).
// Not part of the original game. The STATS link of HighScoreWidget, and the one in the REPLAY END dialog of a watched
// replay (WinDMApp::OnWatchEnded), open it over the board window (191,9 435x423); a click anywhere on it calls its
// owner's close, and the screen is as it was. It draws the board's GameStats with the
// game's fonts and gem sheet only, in the game-over panel's colours, and the board frame (alphaback1) over its edge as
// the game-over screen does: play and matches in two columns (rows only for what the rules play or the game did), the
// gems cleared per colour, the levels as bars and the score as a line in the height left, and the board's rules. A bar
// under the mouse shows its level's points, time and moves.

#include "Widget.h"

#include <functional>

namespace Sexy
{

class WinDMApp;
class Graphics;
class GameStats;

class GameStatsPage : public Widget
{
public:
	enum
	{
		PAGE_X = 191,
		PAGE_Y = 9,
		PAGE_WIDTH = 435,
		PAGE_HEIGHT = 423
	};

	GameStatsPage(WinDMApp* theApp, std::function<void()> theOnClose); // theOnClose removes and deletes the page

	virtual void	Draw(Graphics* g) override;
	virtual void	MouseDown(int x, int y, int theClickCount) override;
	virtual void	MouseMove(int x, int y) override;
	virtual void	MouseLeave() override;

private:
	WinDMApp*		mApp;
	std::function<void()> mOnClose;
	int				mHoverLevel;        // the bar under the mouse, -1 for none
	int				mChartTop;          // the charts' top and bottom, as the last Draw laid them out
	int				mChartBottom;

	int				DrawGems(Graphics* g, const GameStats& theStats, int theY);
	void			DrawLevels(Graphics* g, const GameStats& theStats, int theY);
	void			DrawBoard(Graphics* g);
	int				BarAt(int x, int y, const GameStats& theStats);
};

}

#endif
