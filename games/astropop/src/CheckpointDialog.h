#ifndef __ASTROPOP_CHECKPOINTDIALOG_H__
#define __ASTROPOP_CHECKPOINTDIALOG_H__

// CheckpointDialog: "Select a starting point", the box a player with a passed checkpoint gets before a classic game
// (original class of 0x15c bytes, constructor 0x488760, destructor 0x4896a0, vftable 0x56b560). Two warp gates stand in
// it: the left one starts at the first level (the box's second answer, 3000 + its id 0x78), the right one at the
// checkpoint level (the first answer, 2000 + id). Offsets in the comments are the original's.

#include "Text2004.h"
#include "Dialog.h"
#include "FrameAnim.h"

namespace Sexy
{

class AstroPopApp;

class CheckpointDialog : public Text2004<Dialog>
{
public:
	AstroPopApp*	mApp;       // +0x110
	FrameAnim		mAnim;      // +0x118 the gates' animation (one frame: the gates stand still)
	int				mLevel;     // +0x158 the level the right gate starts at (set by ShowCheckpointDialog)

public:
	CheckpointDialog(int theColourSet, AstroPopApp* theApp);   // 0x488760
	virtual ~CheckpointDialog();                               // 0x4896a0 (body 0x488b70)

	virtual void	Draw(Graphics* g) override;                                  // 0x488c00
	virtual void	Update() override;                                           // 0x483800
	virtual void	MouseMove(int x, int y) override;                            // 0x483820 the hand over a gate
	virtual void	MouseDown(int x, int y, int theClickCount) override;         // 0x4838b0

private:
	bool			OverGateRow(int y);     // the y of both gates' row
};

}

#endif
