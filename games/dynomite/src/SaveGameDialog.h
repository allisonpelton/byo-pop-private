#ifndef __SAVEGAMEDIALOG_H__
#define __SAVEGAMEDIALOG_H__

// Dialog 7, the save dialog (vtable 0x4fca40, 0x2d0 bytes; 0x433420-0x434690; made by the app's SaveGame, 0x419130):
// eight slots, each a button with the picture SLOTn.SAV keeps (60 x 60) or an empty one, under its mode and name.
// A slot's press goes to the app (FossilizedApp::ButtonPress picks it); Save (500) and Cancel (501) too.

#include "EditListener.h"
#include "compat2004/Compat2004.h"

namespace Sexy
{

class Cell;
class CellButton;
class ButtonWidget;
class FossilizedApp;
class Pic;

class SaveGameDialog : public Dialog2004, public EditListener
{
public:
	FossilizedApp*		mApp;				// +0x114
	ButtonWidget*		mSaveButton;		// +0x118 (500)
	ButtonWidget*		mCancelButton;		// +0x11c (501)
	CellButton*			mSlotButtons[8];	// +0x120 (600 + the slot)
	Pic*				mSlotPics[8];		// +0x140 the saved games' pictures
	Cell*				mSlotCells[8];		// +0x160
	Pic*				mEmptyPic;			// +0x184 an empty slot's picture, all black
	Cell*				mEmptyCell;			// +0x188
	char				mModes[8][0x14];	// +0x18c the saved games' modes, "Empty" for an empty slot
	char				mNames[8][0x14];	// +0x22c their names
	int					mChosenSlot;		// +0x2cc -1 none

	SaveGameDialog(FossilizedApp* theApp);	// 0x433910
	virtual ~SaveGameDialog();				// 0x433420

	void				ReadSlots();										// 0x433670
	virtual int			GetPreferredHeight(int theWidth) override;		// 0x4334e0: 450
	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;		// 0x4334f0
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x433540
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x433590
	virtual void		Draw(Graphics* g) override;						// 0x433fe0

	using Dialog2004::Resize;
};

}

#endif
