#ifndef __REPLAYOVERLAY_H__
#define __REPLAYOVERLAY_H__

// port(variant): the watch's controls (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.11, 8). Not part of the original
// game. While a replay is watched in the game (WinDMApp::OnWatchBoard) this transparent widget covers the board right
// of the side bar, whose buttons stay reachable (the undo orb is under it), and holds the focus. It draws a banner
// over the board's top frame: "REPLAY 2x", "REPLAY NEXT MOVE" while it steps there, "REPLAY PAUSED", "REPLAY END"
// under the end dialog; while paused the keys over the bottom frame (the gems stay in view). Space or a click pauses
// and plays on, 1-4 set the speed (1x, 2x, 4x, 8x), N plays to the next move and pauses there, Esc ends the watch
// (WinDMApp::EndWatch). Not in the headless verifier, which has no widgets. Mods can rename the texts: REPLAY_BANNER, REPLAY_BANNER_PAUSED, REPLAY_BANNER_NEXT_MOVE,
// REPLAY_BANNER_END, REPLAY_KEYS.

#include "Widget.h"

namespace Sexy
{

class WinDMApp;
class Graphics;

class ReplayOverlay : public Widget
{
public:
	// Right of the side bar (its buttons end at x 174, the undo orb starts at 193) to the screen's edge; the strips
	// centred over the board (Board::BOARD_CENTER_X) on its frame, above the first row (y 16) and below the last (424)
	enum { OVERLAY_X = 186, BANNER_Y = 0, KEYS_Y = 423 };

	ReplayOverlay(WinDMApp* theApp);

	virtual void	Update() override;
	virtual void	Draw(Graphics* g) override;
	virtual void	KeyChar(SexyChar theChar) override;
	virtual void	KeyDown(KeyCode theKey) override;
	virtual void	MouseDown(int x, int y, int theBtnNum, int theClickCount) override;

	using Widget::MouseDown;

private:
	WinDMApp*		mApp;
	SexyString		mDrawnText;     // the banner Draw shows: Update redraws when it changes

	SexyString		BannerText();
};

}

#endif
