#ifndef __CREDITSDIALOG_H__
#define __CREDITSDIALOG_H__

// The options dialog's credits (dialog 22; vtable 0x4fc3e8, ButtonListener 0x4fc3d4 at +0x58; 0x11c bytes;
// constructor 0x42fed0, destructor 0x42ea10): the roles and names. In the original two of the names (the game
// design's and the music's) are links to their sites (HyperlinkWidgets at +0x114 and +0x118, ids 0 and 1, opened by
// ButtonDepress 0x430b90); the port leads out of the game nowhere (the user, 2026-11-07), so they are plain names in
// white like the others.

#include "Compat2004.h"

namespace Sexy
{

class FossilizedApp;
class Graphics;

class CreditsDialog : public Dialog2004
{
public:
	FossilizedApp*		mApp;				// +0x110

	CreditsDialog(FossilizedApp* theApp);	// 0x42fed0
	virtual ~CreditsDialog();				// 0x42ea10

	virtual int			GetPreferredHeight(int theWidth) override;	// 0x42eaa0: 350
	// 0x4302b0: each role right-aligned to x 175 in pale yellow, its names in white from x 185
	// (port: the two linked names too)
	virtual void		Draw(Graphics* g) override;
};

}

#endif
