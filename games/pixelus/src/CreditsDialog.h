#ifndef __PIXELUS_CREDITSDIALOG_H__
#define __PIXELUS_CREDITSDIALOG_H__

// CreditsDialog.cpp: the credits (Pixelus.exe 0x402290 builds it over the application layer's CreditsDialog 0x4e5580,
// Draw 0x4e4f70; dialog id 3, shown over the options dialog). Who developed the game, and the thanks.

#include "GameDialog.h"

namespace Sexy
{

class CreditsDialog : public GameDialog
{
public:
	enum { DIALOG_ID = 3, PREFERRED_HEIGHT = 0x18c };

public:
	CreditsDialog(PixelusApp* theApp);	// 0x402290

	virtual int		GetPreferredHeight(int theWidth) override;	// 0x4e4ce0
	virtual void	Draw(Graphics* g) override;					// 0x4e4f70
};

}

#endif
