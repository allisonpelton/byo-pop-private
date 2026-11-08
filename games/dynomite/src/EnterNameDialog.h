#ifndef __ENTERNAMEDIALOG_H__
#define __ENTERNAMEDIALOG_H__

// Dialog 6, the name for a high score (vtable 0x4fa298, 0x128 bytes; 0x413d40-0x4145c0; made by the app's
// 0x418df0): an edit box with the name last entered, OK (button 100) and Cancel (101), whose presses go to the
// app (FossilizedApp::ButtonDepress adds the score). Return in the edit box presses OK.

#include "EditListener.h"
#include "compat2004/Compat2004.h"

namespace Sexy
{

class ButtonWidget;
class EditWidget;
class FossilizedApp;
class HighScores;

class EnterNameDialog : public Dialog2004, public EditListener
{
public:
	FossilizedApp*		mApp;				// +0x114
	HighScores*			mHighScores;		// +0x118
	EditWidget*			mNameEdit;			// +0x11c
	ButtonWidget*		mOkButton;			// +0x120 (100) a plain button (0x47f3b0) with the dialog button's picture whole
	ButtonWidget*		mCancelButton;		// +0x124 (101)

	EnterNameDialog(FossilizedApp* theApp, HighScores* theHighScores);	// 0x413f50
	virtual ~EnterNameDialog();											// 0x413d40

	virtual int			GetPreferredHeight(int theWidth) override;		// 0x413dd0: 200
	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;		// 0x413de0
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x413e30
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x413e70
	virtual void		Draw(Graphics* g) override;						// 0x4145c0
	virtual void		EditWidgetText(int theId, const SexyString& theString) override;	// 0x413f00: OK

	using Dialog2004::Resize;
};

}

#endif
