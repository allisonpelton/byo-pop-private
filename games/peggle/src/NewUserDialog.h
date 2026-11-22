// NewUserDialog.h: the dialog that asks for a player's name (the original's NewUserDialog.cpp). It is two dialogs in one class:
// the plain "New User" and "Rename User" dialogs of the player list, and the certificate that comes up the first time the game
// is played, where the name is written on a diploma-like picture and stamped "approved" once it is accepted.

#ifndef __NEWUSERDIALOG_H__
#define __NEWUSERDIALOG_H__

#include "EditListener.h"
#include "ThunderDialog.h"

#include <string>

namespace Sexy
{

class EditWidget;
class Graphics;
class ThunderButton;
class ThunderballApp;
class WidgetManager;

class NewUserDialog : public ThunderDialog, public EditListener
{
public:
	// theRename makes the dialog of id DIALOG_RENAME_USER and "Rename User" its header (the other is DIALOG_CREATE_USER, "New
	// User"); theCertificate makes the certificate; theCanCancel lets the certificate be closed with its No button.
	NewUserDialog(ThunderballApp* theApp, bool theRename, bool theCertificate, bool theCanCancel); // 0x4a9380
	~NewUserDialog() override;

	int GetPreferredHeight(int theWidth) override;						 // 0x4952e0
	void AddedToManager(WidgetManager* theWidgetManager) override;		 // 0x495330
	void RemovedFromManager(WidgetManager* theWidgetManager) override;	 // 0x495370
	void Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x4953a0
	void DrawOverlay(Graphics* g) override;								 // 0x495530: the stamp
	void Draw(Graphics* g) override;									 // 0x499d70
	void Update() override;												 // 0x49cb00

	void EditWidgetText(int theId, const std::string& theString) override; // 0x495610: Enter in the box is the same as OK
	bool AllowChar(int theId, char theChar) override;					   // 0x495630: letters, digits and spaces

	void SetName(const std::string& theName); // 0x499ef0: puts a name in the box (the cursor after it)
	std::string GetName() const;			  // 0x49f890: what is in the box without the spaces at its ends and the doubled ones
	void StartStamp();						  // 0x495660: the name was accepted: the OK button goes and the stamp comes down

	ThunderballApp* mApp;		 // +0x17c
	EditWidget* mEditWidget;	 // +0x180
	ThunderButton* mOkButton;	 // +0x184: the certificate's picture button (null for the plain dialog)
	bool mCertificate;			 // +0x188
	std::string mDateText;		 // +0x18c: today's date as written on the certificate
	int mStampTime;				 // +0x1a8: 0 before the stamp, then counts the updates since it came down
};

} // namespace Sexy

#endif
