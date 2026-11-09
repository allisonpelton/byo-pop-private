#ifndef __ASTROPOP_NEWUSERDIALOG_H__
#define __ASTROPOP_NEWUSERDIALOG_H__

// NewUserDialog: the box where a new player types a name (original class of 0x118 bytes, constructor 0x484d80, destructor
// 0x483c20 / body 0x482140, vftable 0x56aa08, with an EditListener at +0x110, vftable 0x56a9f4). The name field is an
// EditWidget the box keeps beside itself in the widget manager. Offsets in the comments are the original's.

#include "Dialog.h"
#include "EditListener.h"

namespace Sexy
{

class EditWidget;

class NewUserDialog : public Dialog, public EditListener
{
public:
	EditWidget*		mEdit;      // +0x114 the name field

public:
	NewUserDialog();                        // 0x484d80
	virtual ~NewUserDialog();               // 0x483c20 (body 0x482140)

	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x4821f0 the field is added and gets the keyboard
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x482230
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x482260 the field sits under the text
	virtual void	Draw(Graphics* g) override;                             // 0x483c50 the frame of the field
	virtual void	GotFocus() override;                                    // 0x4822b0 the keyboard goes to the field

	// the listener of the field
	virtual void	EditWidgetText(int theId, const SexyString& theString) override;   // 0x4822d0 Return = the OK button
	virtual bool	AllowChar(int theId, SexyChar theChar) override;                   // 0x4820a0 no punctuation in a name
};

}

#endif
