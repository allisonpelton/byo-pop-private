// The application's player handling: the name typed in the new-player box (0x430500). (The player list, the choice of a
// player and the deletion of one follow with the list box.)

#include "AstroPopApp.h"
#include "AstroDialog.h"
#include "EditWidget.h"
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
			// not translated yet: the list box shows the new name (0x486570 and the box above it)
			SDL_Log("AstroPop: the player list does not show the new player yet (0x486b00)");
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

	ShowLowMessageDialog("Invalid Name", GT(0x5635f8));
}
