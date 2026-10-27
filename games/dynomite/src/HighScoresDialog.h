#ifndef __HIGHSCORESDIALOG_H__
#define __HIGHSCORESDIALOG_H__

// The options dialog's high scores (dialog 4; vtable 0x4fb688, ButtonListener 0x4fb674 at +0x58; 0x124 bytes;
// constructor 0x421c50, destructor 0x421b40): the four modes' tables, each read from its file again, two by two.

#include "Compat2004.h"

namespace Sexy
{

class FossilizedApp;
class Graphics;
class HighScores;

class HighScoresDialog : public Dialog2004
{
public:
	FossilizedApp*		mApp;				// +0x110
	HighScores*			mTables[4];			// +0x114 Stomped, Endless, Fossil, Time Trial

	HighScoresDialog(FossilizedApp* theApp);	// 0x421c50
	virtual ~HighScoresDialog();				// 0x421b40

	virtual int			GetPreferredHeight(int theWidth) override;	// 0x421bf0: 400
	virtual void		Draw(Graphics* g) override;					// 0x421ff0: the four titles and tables
	virtual void		ButtonPress(int theId) override;			// 0x421c00: the click
	using Dialog2004::ButtonPress;
};

}

#endif
