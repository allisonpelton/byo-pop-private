#ifndef __HAMMERHEADS_READYGODIALOG_H__
#define __HAMMERHEADS_READYGODIALOG_H__

// "Ready?": the box that holds a level back until the player clicks "Go!" (dialog 0xd). The power-ups bought in the shop start
// when it is answered, and the game goes on once it has left.
// Original: class Sexy::ReadyGoDialog : public NcMovingDialog (0x168 bytes; the constructor is 0x45ccb0, the destructor 0x45cab0).

#include "NcMovingDialog.h"

namespace Sexy
{

class ReadyGoDialog : public NcMovingDialog
{
public:
	ReadyGoDialog(const std::string& theHeader, const std::string& theLines);	// 0x45ccb0
	virtual ~ReadyGoDialog();												// 0x45cab0

	virtual int		GetPreferredHeight(int theWidth) override;				// 0x45cc80
	virtual void	ButtonDepress(int theId) override;						// 0x45caf0
	virtual void	Closed() override;										// 0x45cc10
};

// 0x45cde0: the box is made, sized (taller when it has lines) and added as dialog 0xd
ReadyGoDialog* MakeReadyGoDialog(const std::string& theHeader, const std::string& theLines);

}

#endif
