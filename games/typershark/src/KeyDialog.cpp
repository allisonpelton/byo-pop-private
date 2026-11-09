#include "KeyDialog.h"
#include "WinWSApp.h"

using namespace Sexy;

// 0x43ec30: Dialog (0x43f3b0 in the framework) first; the escape key starts in the cancel list
KeyDialog::KeyDialog(WinWSApp* theApp, Image* theComponentImage, Image* theButtonComponentImage, int theId,
	bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines,
	const SexyString& theDialogFooter, int theButtonMode) :
	Dialog2004(theComponentImage, theButtonComponentImage, theId, isModal, theDialogHeader, theDialogLines,
		theDialogFooter, theButtonMode),
	mApp(theApp),
	mCloseOnKey(false)
{
	mCancelKeys.push_back(0x1b);
	mTextAlign = -1;
}

// 0x43a2d0 (body: the two lists are freed, then Dialog's destructor body runs); the scalar deleting destructor is 0x43ecf0
KeyDialog::~KeyDialog()
{
}

// 0x439210 slot 20. Without mCloseOnKey any key closes the dialog (WinWSApp::KillDialog, slot 58); with it a key in the
// cancel list presses the no button and a key in the ok list the yes button (the app's ButtonDepress, slot 4, called
// with the dialog id + 3000 / + 2000).
void KeyDialog::KeyChar(SexyChar theChar)
{
	if (!mCloseOnKey)
	{
		mApp->KillDialog(mId);
		return;
	}

	unsigned int i;
	for (i = 0; i < mCancelKeys.size(); i++)
	{
		if (theChar == (char)mCancelKeys[i])
		{
			mApp->ButtonDepress(mId + ID_NO_2004);
			return;
		}
	}

	i = 0;
	while (i < mOkKeys.size())
	{
		if (theChar == (char)mOkKeys[i])
		{
			mApp->ButtonDepress(mId + ID_YES_2004);
			return;
		}
		i++;
	}
}

// 0x43a3e0: takes the first entry of the key out of the cancel list
void KeyDialog::RemoveCancelKey(char theKey)
{
	unsigned int i = 0;
	while (i < mCancelKeys.size())
	{
		if ((char)mCancelKeys[i] == theKey)
		{
			mCancelKeys.erase(mCancelKeys.begin() + i);
			return;
		}
		i++;
	}
}

// 0x43ed20: adds the key to the ok list unless it is in it
void KeyDialog::AddOkKey(char theKey)
{
	unsigned int i;
	for (i = 0; i < mOkKeys.size(); i++)
	{
		if ((char)mOkKeys[i] == theKey)
			return;
	}
	mOkKeys.push_back((unsigned char)theKey);
}
