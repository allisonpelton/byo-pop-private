#ifndef __MUMMYMAZE_DIALOGS_H__
#define __MUMMYMAZE_DIALOGS_H__

// The dialogs of Mummy Maze Deluxe that are more than a header, some lines and buttons (see Dialogs.cpp).

#include "Compat2004.h"
#include "EditWidget.h"
#include "EditListener.h"

namespace Sexy
{

class MummyApp;

// 0x427dc0: the name for a high score
class RecordScoreDialog : public Dialog2004, public EditListener
{
public:
	MummyApp*			mApp;                   // +0xc4
	EditWidget*			mNameEdit;              // +0xc8

public:
	RecordScoreDialog(MummyApp* theApp, const SexyString& theLines);
	virtual ~RecordScoreDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;           // 0x428090
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;       // 0x4280c0
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x4280f0
	virtual int			GetPreferredHeight(int theWidth) override;                          // 0x428080
	virtual void		Draw(Graphics* g) override;                                         // 0x428140
	virtual void		EditWidgetText(int theId, const SexyString& theString) override;    // 0x428450
	using Widget::Resize;
};

}

#endif
