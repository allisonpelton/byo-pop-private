#ifndef __UNDOORB_H__
#define __UNDOORB_H__

// port(variant): the undo hotspot (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.12, 8). Not part of the original
// game. The decorative red orb left of the timer bar is a button (BUTTON_UNDO) on a board whose rules enable
// undo: it glows while an undo is possible, brighter under the mouse, and shows the undos left when undo.max
// limits them. On every other board it draws nothing and takes no mouse, so the screen stays the original's.
// WinDMApp::TitleScreenIsFinished makes it with the side-bar buttons (not in a BEJ_NO_VARIANTS build).

#include "ButtonWidget.h"

namespace Sexy
{

class WinDMApp;
class MemoryImage;

class UndoOrb : public ButtonWidget
{
public:
	// The orb in the background art, left of the timer bar (237, 444); the level number sits in its twin on the right.
	// Its red ball has its centre at (209, 456) and a radius of about 13 px inside the silver ring (screenshots).
	enum { ORB_X = 193, ORB_Y = 440, ORB_WIDTH = 34, ORB_HEIGHT = 33, DIGIT_Y = 465, BALL_X = 209, BALL_Y = 456, BALL_RADIUS = 13 };

	UndoOrb(WinDMApp* theApp, int theId);
	virtual ~UndoOrb();

	virtual void	Update() override;
	virtual void	Draw(Graphics* g) override;

private:
	WinDMApp*		mApp;
	int				mDrawnState;    // what Draw showed last (Update redraws on a change): -1 nothing
	MemoryImage*	mGlow;          // a white disc over the ball with a soft edge, drawn additively; made at the first Draw
};

}

#endif
