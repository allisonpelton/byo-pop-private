#ifndef __GETSAVENAMEDIALOG_H__
#define __GETSAVENAMEDIALOG_H__

// Dialog 8, the saved game's name (vtable 0x4fb4f8, 0x124 bytes; 0x420dc0-0x421b40; made by the app's
// ShowSaveNameDialog, 0x418f40): the mode and the picture of the board over an edit box with the name, Save (700) and
// Cancel (701), whose presses go to the app (FossilizedApp::ButtonDepress writes SLOTn.SAV). Return in the edit box
// presses Save. It also names the mode for the save (the app's mSaveMode).

#include "EditListener.h"
#include "compat2004/Compat2004.h"

namespace Sexy
{

class DialogButton;
class EditWidget;
class FossilizedApp;

class GetSaveNameDialog : public Dialog2004, public EditListener
{
public:
	FossilizedApp*		mApp;				// +0x114
	EditWidget*			mNameEdit;			// +0x118
	DialogButton*		mSaveButton;		// +0x11c (700)
	DialogButton*		mCancelButton;		// +0x120 (701)

	GetSaveNameDialog(FossilizedApp* theApp);	// 0x420fb0
	virtual ~GetSaveNameDialog();				// 0x420dc0

	virtual int			GetPreferredHeight(int theWidth) override;		// 0x420e50: 300
	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;		// 0x420e60
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x420eb0
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x420ef0
	virtual void		Draw(Graphics* g) override;						// 0x4218f0
	virtual void		EditWidgetText(int theId, const SexyString& theString) override;	// 0x420f80: Save

	using Dialog2004::Resize;
};

}

#endif
