// The application's player handling: the list of the players (0x422870, 0x40bcc0), the choice of one (0x430f00), the
// deletion of one (0x431310) and the name typed in the new-player box (0x430500).

#include "AstroPopApp.h"
#include "AstroDialog.h"
#include "EditWidget.h"
#include "ListDialog.h"
#include "ListWidget.h"
#include "MainMenu.h"
#include "GameText.h"
#include "NewUserDialog.h"
#include "UserProfile.h"
#include "WidgetManager.h"
#include <SDL.h>

using namespace Sexy;

// 0x410c90: the spaces, tabs and line ends at both ends of a name go
static std::string TrimName(const std::string& theName)
{
	size_t aStart = 0;
	size_t anEnd = theName.size();
	while (aStart < anEnd)
	{
		char c = theName[aStart];
		if (c != ' ' && c != '\n' && c != '\r' && c != '\t')
			break;
		aStart++;
	}
	while (anEnd > aStart)
	{
		char c = theName[anEnd - 1];
		if (c != ' ' && c != '\n' && c != '\r' && c != '\t')
			break;
		anEnd--;
	}
	return theName.substr(aStart, anEnd - aStart);
}

// 0x410b80: the letters a .. z become capitals (the names are compared that way)
static std::string UpperName(const std::string& theName)
{
	std::string aResult = theName;
	for (size_t i = 0; i < aResult.size(); i++)
	{
		if (aResult[i] > '`' && aResult[i] < '{')
			aResult[i] = (char) (aResult[i] - 0x20);
	}
	return aResult;
}

// A box with the word OK that stands a little below the middle (so that the box under it, the name entry or the player
// list, stays in sight), made where the original makes it with 0x411ba0 and a resize
AstroDialog* AstroPopApp::ShowLowMessageDialog(const std::string& theHeader, const std::string& theText)
{
	AstroDialog* aDialog = MakeDialog(1, false, 0x6c, -1, -1, 3, theHeader, theText, "OK", std::string(), true);
	int aWidth = aDialog->mWidth;
	int aHeight = aDialog->mHeight;
	aDialog->Resize(0x140 - aWidth / 2, 0x11d - aHeight / 2, aWidth, aHeight);
	return aDialog;
}

// 0x430500: the OK of the new-player box. A name is good when it is not empty after the spaces at its ends are cut off, is
// not one of the two entries of the player list, and does not match (capitals alike) a name already there. The new player is
// made, saved and becomes the current player; the box (and the player list under it, when it is open) goes.
void AstroPopApp::NewUserOk()
{
	NewUserDialog* aNameDialog = (NewUserDialog*) GetDialog(0x6b);
	std::string aName = TrimName(aNameDialog->mEdit->mString);
	std::string anUpper = UpperName(aName);
	std::string aCreateEntry = UpperName(GT(0x55dce0));
	std::string aDeleteEntry = UpperName(GT(0x55dcd0));

	bool anExists = false;
	for (size_t i = 0; i < mUserNames.size(); i++)
	{
		if (UpperName(mUserNames[i]) == anUpper)
		{
			anExists = true;
			break;
		}
	}

	Dialog* aListDialog = GetDialog(0x6d);

	if (!anUpper.empty() && anUpper != aCreateEntry && anUpper != aDeleteEntry && !anExists)
	{
		if (mUser != NULL)
		{
			mUser->Save();
			delete mUser;
		}
		mUser = new UserProfile();
		mUser->InitNew(aName, -1);
		if (!mUser->Save())
			ShowLowMessageDialog("ERROR", GT(0x563678));
		ValidateUser();
		ListUsers();

		if (aListDialog != NULL)
		{
			// the player list (when it is open under the box) takes the new name and selects it; both boxes go right after
			ListDialog* aList = (ListDialog*) aListDialog;
			aList->RebuildList();
			aList->MarkDirty();
			aList->mList->MarkDirty();
			aList->SelectByName(aName);
		}
		mOpenDialogs--;
		KillDialog(0x6b);
		if (GetDialog(0x6d) != NULL)
		{
			KillDialog(0x6d);
			mOpenDialogs--;
		}
		return;
	}

	ShowLowMessageDialog(GT(0x005635e8), GT(0x5635f8));
}

// 0x4119e0: the player of the next session
void AstroPopApp::SetLastUserName(const std::string& theName)
{
	RegistryWriteString("Users\\LastUser", theName);
}

// 0x40bfb0: a box of its own size in the middle of a rectangle. The boxes of the player choice are placed in the rectangle
// the original keeps at 0x59d50c (0, 110, 400, 305) when a game is on, and so are the two boxes that tell of a profile
// that could not be read or that there are too many.
static void CentreInRect(Dialog* theDialog, int theX, int theY, int theWidth, int theHeight)
{
	int aWidth = theDialog->mWidth;
	int aHeight = theDialog->mHeight;
	theDialog->Resize((theWidth / 2 - aWidth / 2) + theX, (theHeight / 2 - aHeight / 2) + theY, aWidth, aHeight);
}

// 0x422870: the list of the players, with the buttons to make a new one and to delete one. (The original refuses with a box
// when it cannot write the registry; the port's registry can always be written.)
void AstroPopApp::ShowUserSelect()
{
	mOpenDialogs++;
	UserSelectDialog* aDialog = new UserSelectDialog(this);
	StyleDialog(aDialog);
	int aHeight = aDialog->GetPreferredHeight(0x17c);
	int aY = 0xf0 - aHeight / 2;
	if (mBoard == NULL)
		aDialog->Resize(0x82, aY, 0x17c, aHeight);
	else
	{
		// the box's own rectangle (0x82, y, 0x17c, height) in the middle of the game's rectangle
		int aX = (400 / 2 - 0x17c / 2) + 0;
		int aTop = (305 / 2 - aHeight / 2) + 110;
		aDialog->Resize(aX, aTop, 0x17c, aHeight);
	}
	AddDialog(0x6d, aDialog);
}

// 0x40bcc0: the list of the players that one is to be deleted from
void AstroPopApp::ShowDeleteUserDialog()
{
	mOpenDialogs++;
	DeleteUserDialog* aDialog = new DeleteUserDialog(this);
	StyleDialog(aDialog);
	int aHeight = aDialog->GetPreferredHeight(0x17c);
	aDialog->Resize(0x82, 0x118 - aHeight / 2, 0x17c, aHeight);
	AddDialog(0x6e, aDialog);
}

// The box for a profile that could not be read: it was made new again (0x4298c0) and the player is told
static void ShowDataLostBox(AstroPopApp* theApp, bool theCentre)
{
	AstroDialog* aBox = theApp->MakeDialog(1, false, 0x6c, -1, -1, 3, "Error", GT(0x563470), "OK", std::string(), true);
	if (theCentre)
		CentreInRect(aBox, 0, 110, 400, 305);
}

// 0x430f00: the OK of the player list. No name: the list goes. The entry for a new player opens the new-player box (up to ten
// players); the entry for deleting opens the delete list. A name of a player: that player is loaded in place of the one
// that is playing (whose profile is saved first), becomes the one of the next session, and the list goes.
void AstroPopApp::UserSelectOk()
{
	ListDialog* aDialog = (ListDialog*) GetDialog(0x6d);
	const std::string& aChoice = aDialog->mSelectedName;

	if (aChoice.empty())
	{
		KillDialog(0x6d);
		mOpenDialogs--;
	}
	else if (aChoice == GT(0x55dce0))
	{
		if (mUserNames.size() < 10)
		{
			ShowNewUserDialog();
			return;
		}
		AstroDialog* aBox = MakeDialog(1, false, 0x6c, -1, -1, 3, "Error", StrFormat(GTC(0x563748), 10), "OK", std::string(), true);
		CentreInRect(aBox, 0, 110, 400, 305);
	}
	else if (aChoice == GT(0x55dcd0))
		ShowDeleteUserDialog();
	else
	{
		std::string aCurrent;
		if (mUser != NULL)
		{
			mUser->GetString(PROFILE_NAME, &aCurrent);
			if (aCurrent != aChoice)
			{
				mUser->Save();
				delete mUser;
				mUser = NULL;
			}
		}
		if (aCurrent != aChoice)
		{
			mUser = new UserProfile();
			mUser->SetString(PROFILE_NAME, aChoice);
			if (!mUser->Load(NULL, NULL, false))
			{
				ResetFailedUserLoad();
				ShowDataLostBox(this, true);
			}
		}
		SetLastUserName(aChoice);
		ValidateUser();
		KillDialog(0x6d);
		mOpenDialogs--;
	}
	mWidgetManager->SetFocus(mMainMenu);
}

// 0x431310: DeleteUser(false) asks whether the player chosen in the delete list is to go; DeleteUser(true) (the Yes) deletes
// the player: the registry key and the data file. What is left of the players is listed again; none left: the box for a
// new player. The player that was playing goes on, unless it was the one deleted (then the first of the rest).
void AstroPopApp::DeleteUser(bool theConfirmed)
{
	if (!theConfirmed)
	{
		MakeDialog(1, false, 0x6f, -1, -1, 1, GT(0x005637d0), GT(0x5637e0), "Yes", "No", true);
		return;
	}

	std::string aDeleted = ((ListDialog*) GetDialog(0x6e))->mSelectedName;

	// the player that was playing is saved and put away; the one to delete is loaded for its number
	std::string aCurrent;
	if (mUser != NULL)
	{
		mUser->GetString(PROFILE_NAME, &aCurrent);
		mUser->Save();
		delete mUser;
	}
	mUser = new UserProfile();
	mUser->SetString(PROFILE_NAME, aDeleted);
	mUser->Load(NULL, NULL, false);

	int aNumber = -1;
	bool aKeyGone = RegistryEraseKey(StrFormat("Users\\%s", aDeleted.c_str()));
	bool aFine = aKeyGone;
	if (!mUser->GetInt(PROFILE_NUMBER, &aNumber))
		aFine = false;

	bool anError;
	if (aNumber == -1)
		anError = !aFine;
	else
	{
		bool aFileGone = EraseFile(StrFormat("users\\data%d.usr", aNumber));
		anError = !aFileGone || !aFine;
	}
	if (anError)
		MakeDialog(1, false, 0x6c, -1, -1, 3, "Error", GT(0x5637a0), "OK", std::string(), true);

	delete mUser;
	mUser = NULL;
	ListUsers();

	if (mUserNames.empty())
	{
		KillDialog(0x6e);
		mOpenDialogs--;
		KillDialog(0x6f);
		mOpenDialogs--;
		if (GetDialog(0x6d) != NULL)
		{
			KillDialog(0x6d);
			mOpenDialogs--;
		}
		ShowNewUserDialog();
	}
	else
	{
		mUser = new UserProfile();
		if (aDeleted == aCurrent)
		{
			aCurrent = mUserNames[0];
			SetLastUserName(aCurrent);
		}
		mUser->SetString(PROFILE_NAME, aCurrent);
		if (!mUser->Load(NULL, NULL, false))
		{
			ResetFailedUserLoad();
			MakeDialog(1, false, 0x6c, -1, -1, 3, "Error", GT(0x563470), "OK", std::string(), true);
		}
		ListDialog* aList = (ListDialog*) GetDialog(0x6d);
		aList->RebuildList();
		aList->MarkDirty();
		aList->mList->MarkDirty();
		aList->SelectByName(aCurrent);
		KillDialog(0x6e);
		mOpenDialogs--;
		KillDialog(0x6f);
		mOpenDialogs--;
	}
}
