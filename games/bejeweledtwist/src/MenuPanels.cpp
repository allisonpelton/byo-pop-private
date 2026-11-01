// The main menu's panels (MenuPanels.h). port: not ported yet, apart from what the menu needs of them.
#include "MenuPanels.h"

#include "Image.h"
#include "Res.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <SDL.h>

#include <set>
#include <string>

using namespace Sexy;

namespace {

void NotPorted(const char* theWhat)
{
	static std::set<std::string> sSaid;
	if (sSaid.insert(theWhat).second)
		SDL_Log("%s is not ported yet", theWhat);
}

}  // namespace

MenuPanel::~MenuPanel() = default;

HighScoresPanel::HighScoresPanel(Menu*)
{
	NotPorted("HighScoresPanel");
}

void HighScoresPanel::SetCategory(int theCategory, bool)
{
	mCategory = theCategory;
	NotPorted("HighScoresPanel::SetCategory");
}

void HighScoresPanel::Unknown694BD3()
{
	NotPorted("HighScoresPanel 0x694bd3");
}

void HighScoresPanel::Unknown695966()
{
	NotPorted("HighScoresPanel 0x695966");
}

HelpPanel::HelpPanel(WinTwistApp*)
{
	NotPorted("HelpPanel");
}

void HelpPanel::SetPage(int thePage)
{
	mPage = thePage;
	NotPorted("HelpPanel::SetPage");
}

void HelpPanel::NextPage()
{
	NotPorted("HelpPanel::NextPage");
}

void HelpPanel::PrevPage()
{
	NotPorted("HelpPanel::PrevPage");
}

ProfilePanel::ProfilePanel(Menu*)
{
	// The constructor's start (0x6a077b): the panel is the dialog picture's size, at its offset
	mDialogImage = GetImageById(IMAGE_MAINMENU_PROFILEDIALOG_ID);
	int aWidth = mDialogImage->mWidth;
	int aHeight = mDialogImage->mHeight;
	float aScale = (float)gApp->mArtRes / 1200.0f;
	Resize(0, Ftol((double)GetImageOffsetY(IMAGE_MAINMENU_PROFILEDIALOG_ID) * aScale), aWidth, aHeight);
	NotPorted("ProfilePanel");
}

void ProfilePanel::Show()
{
	NotPorted("ProfilePanel::Show");
}

void ProfilePanel::Hide()
{
	NotPorted("ProfilePanel::Hide");
}

void OptionsPanel::Unknown6A006A(bool)
{
	NotPorted("OptionsPanel 0x6a006a");
}

OptionsPanel::OptionsPanel(WinTwistApp*)
{
	NotPorted("OptionsPanel");
}

ChallengeMenu::ChallengeMenu()
{
	NotPorted("ChallengeMenu");
}

void ChallengeMenu::Show(int, bool)
{
	NotPorted("ChallengeMenu::Show");
}

EndStats::EndStats(WinTwistApp*)
{
	NotPorted("EndStats");
}

int EndStats::GetPreferredWidth()
{
	return 0;
}

int EndStats::GetPreferredHeight()
{
	return 0;
}
