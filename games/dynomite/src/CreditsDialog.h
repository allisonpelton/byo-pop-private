#ifndef __CREDITSDIALOG_H__
#define __CREDITSDIALOG_H__

// The options dialog's credits (dialog 22; vtable 0x4fc3e8, ButtonListener 0x4fc3d4 at +0x58; 0x11c bytes;
// constructor 0x42fed0, destructor 0x42ea10): the roles and names; two of the names are links to their sites.

#include "Compat2004.h"

namespace Sexy
{

class FossilizedApp;
class Graphics;
class HyperlinkWidget;
class WidgetManager;

class CreditsDialog : public Dialog2004
{
public:
	FossilizedApp*		mApp;				// +0x110
	HyperlinkWidget*	mDesignLink;		// +0x114 id 0, the game design's
	HyperlinkWidget*	mMusicLink;			// +0x118 id 1, the music's

	CreditsDialog(FossilizedApp* theApp);	// 0x42fed0
	virtual ~CreditsDialog();				// 0x42ea10

	virtual int			GetPreferredHeight(int theWidth) override;	// 0x42eaa0: 350
	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;		// 0x42eab0
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4410b0
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x42eae0
	// 0x4302b0: each role right-aligned to x 175 in pale yellow, its names in white from x 185
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;			// 0x430b90: the links' sites
};

}

#endif
