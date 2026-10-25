#ifndef __BOOKWORM_SIDEPANEL_H__
#define __BOOKWORM_SIDEPANEL_H__

// port(mod): the mod "widescreen" (feature "widescreen"). The screen is SIDE_PANEL_WIDTH wider than the game's
// 640 x 480 (BookWormApp::PreDisplayHook; every screen of the game keeps its 640 x 480 at the left) and this
// panel fills the strip on the right, in a frame cut from the board's backdrop. Top, with the mod "solver": the
// solver's list for the board (the hints Tab cycles through, with their points; a click on one selects it, as
// its hint would). Below: the words played this game, newest first, read from the board's game log (so an undo
// or a continued game shows them as they stand), each in the colour of its best special tile.

#include "Widget.h"
#include "Common.h"
#include <vector>

namespace Sexy
{
class BookWormApp;
class Board;
class Graphics;

enum
{
	GAME_SCREEN_WIDTH = 640,    // the game's own screen, at the left of a wider one
	SIDE_PANEL_WIDTH = 214      // 640 + 214 = 854 x 480, 16:9
};

class SidePanel : public Widget
{
public:
	struct PlayedWord
	{
		SexyString	mWord;
		int			mScore;     // Submit's points
		int			mBonus;     // the bonus word's points on top, 0 for none
		int			mLevel;     // 0-based
		int			mTileType;  // its best special tile (TileType), TILE_NORMAL for none
	};

	BookWormApp*			mApp;
	Board*					mLogBoard;      // the board whose log mWords was read from, and its size then
	int						mLogBits;
	std::vector<PlayedWord>	mWords;
	int						mHoverHint;     // the row under the mouse, -1 for none

public:
	SidePanel(BookWormApp* theApp);

	static bool				Enabled();      // feature "widescreen"
	static void				TellPage(int theWidth, int theHeight);  // web: the page's stage takes the screen's shape
	static Rect				HintRect(int theIdx);    // the solver's row theIdx (panel coordinates; the -autoplay bot)
	int						HintAt(int x, int y) const; // the live row there, -1 for none

	virtual void			Update() override;
	virtual void			Draw(Graphics* g) override;
	virtual void			MouseMove(int x, int y) override;
	virtual void			MouseLeave() override;
	virtual void			MouseDown(int x, int y, int theClickCount) override;

	using Widget::MouseDown;

private:
	Board*					CurrentBoard() const;
	bool					HintsLive(Board* theBoard) const;
	void					ReadLog(Board* theBoard);
	int						DrawHints(Graphics* g, Board* theBoard, int theY);
	void					DrawWords(Graphics* g, int theY);
};

}

#endif
