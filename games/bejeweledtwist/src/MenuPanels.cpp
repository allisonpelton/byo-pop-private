// The main menu's panels (MenuPanels.h). port: the profile and new player panels are ported; of the others only what
// the menu needs of them.
#include "MenuPanels.h"

#include "Dialog.h"
#include "EditListener.h"
#include "Font.h"
#include "GameText.h"
#include "Gates.h"
#include "Graphics.h"
#include "Image.h"
#include "ListWidget.h"
#include "Menu.h"
#include "MenuButtonWidget.h"
#include "Profile.h"
#include "Ranks.h"
#include "Res.h"
#include "TwistButton.h"
#include "TwistDialog.h"
#include "TwistEditWidget.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <SDL.h>

#include <cmath>
#include <cwctype>
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

// A button of a panel at its place in resources.xml, against the panel's picture
void PlaceButton(ButtonWidget* theButton, int theDialogId, float theScale)
{
	int anId = GetIdByImage(theButton->mButtonImage);
	int aHeight = theButton->mButtonImage->GetCelHeight();
	int aWidth = theButton->mButtonImage->GetCelWidth();
	int aY = Ftol((double)(GetImageOffsetY(anId) - GetImageOffsetY(theDialogId)) * theScale);
	int aX = Ftol((double)(GetImageOffsetX(anId) - GetImageOffsetX(theDialogId)) * theScale);
	theButton->Resize(aX, aY, aWidth, aHeight);
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

void HighScoresPanel::Refresh()
{
	SetCategory(1, false);
	NotPorted("HighScoresPanel 0x6941aa");
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

ProfilePanel::ProfilePanel(Menu* theMenu)
{
	mMenu = theMenu;
	mApp = theMenu->mApp;
	mUnknownB0 = IMAGE_MAINMENU_PROFILEDIALOG_ID;
	mDialogImage = GetImageById(IMAGE_MAINMENU_PROFILEDIALOG_ID);
	int aWidth = mDialogImage->mWidth;
	int aHeight = mDialogImage->mHeight;
	float aScale = (float)gApp->mArtRes / 1200.0f;
	Resize(0, Ftol((double)GetImageOffsetY(IMAGE_MAINMENU_PROFILEDIALOG_ID) * aScale), aWidth, aHeight);

	mList = new ListWidget(0, FONT_MENU_TINY, this);
	mList->mJustify = ListWidget::JUSTIFY_CENTER;
	mList->SetColor(ListWidget::COLOR_BKG, Color(0, 0, 0, 0));
	mList->SetColor(ListWidget::COLOR_OUTLINE, Color(0, 0, 0, 0));
	mList->SetColor(ListWidget::COLOR_TEXT, Color(0x6800b8));
	mList->SetColor(ListWidget::COLOR_HILITE, Color(0xffffff));
	mList->SetColor(ListWidget::COLOR_SELECT, Color(0xc332d7));
	mList->mDrawOutline = false;
	mList->mItemHeight = Ftol((double)gApp->mScale * 40.0);
	mMouseDown = false;
	mClicked = false;
	mCreateButton = new TwistButton(BUTTON_CREATE, IMAGE_MAINMENU_CREATEBUTTON, this);
	mDeleteButton = new TwistButton(BUTTON_DELETE, IMAGE_MAINMENU_DELETEBUTTON, this);
}

ProfilePanel::~ProfilePanel()
{
	RemoveWidget(mList);
	delete mList;
	RemoveWidget(mCreateButton);
	delete mCreateButton;
	RemoveWidget(mDeleteButton);
	delete mDeleteButton;
	RemoveAllWidgets(true, false);
}

void ProfilePanel::AddedToManager(WidgetManager* theWidgetManager)
{
	MenuPanel::AddedToManager(theWidgetManager);
	float aScale = (float)gApp->mArtRes / 1200.0f;
	int aHeight = Ftol((double)gApp->mScale * 400.0);
	int aWidth = (mWidth * 2) / 3;
	int aY = Ftol((double)aScale * 240.0);
	int aX = Ftol((double)aScale * 155.0);
	mList->Resize(aX, aY, aWidth, aHeight);
	AddWidget(mList);
	PlaceButton(mCreateButton, mUnknownB0, aScale);
	AddWidget(mCreateButton);
	PlaceButton(mDeleteButton, mUnknownB0, aScale);
	AddWidget(mDeleteButton);
	mRefresh = true;
}

void ProfilePanel::RemovedFromManager(WidgetManager* theWidgetManager)
{
	MenuPanel::RemovedFromManager(theWidgetManager);
	RemoveWidget(mList);
	RemoveWidget(mCreateButton);
	RemoveWidget(mDeleteButton);
}

void ProfilePanel::Update()
{
	MenuPanel::Update();
	bool noNewUser = mMenu->mNewUserPanel == nullptr;
	mMouseVisible = noNewUser;
	mList->mMouseVisible = noNewUser;
	if (mSlide.mIncRate > 0.0 || !gApp->mMainMenu->mPanning)
		mSlide.IncInVal();
	if (mRefresh) {
		Refresh();
		mRefresh = false;
	}
}

void ProfilePanel::Draw(Graphics* g)
{
	g->DrawImage(mDialogImage, 0, 0);
	SexyString aRank = mApp->mRanks->GetName(mApp->mProfile->mRank);
	SexyString aName = mApp->mProfile->mName;
	Font* aFont = FONT_DIALOG_CHECKBOX;
	SetFontLayerColor(aFont, 0, Color::White);
	g->SetFont(aFont);
	g->SetColor(Color(255, 255, 200));
	int aY = Ftol((double)gApp->mScale * 92.0);
	g->DrawString(aRank, mWidth / 2 - g->GetFont()->StringWidth(aRank) / 2, aY);
	g->SetFont(FONT_DIALOG_TEXT);
	SetFontLayerColor(g->GetFont(), "MAIN", Color::White);
	g->SetColor(Color(255, 255, 0));
	aY = Ftol((double)gApp->mScale * 143.0);
	g->DrawString(aName, mWidth / 2 - g->GetFont()->StringWidth(aName) / 2, aY);
}

void ProfilePanel::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	float aY = (float)(mSlide.GetOutVal() * (double)gApp->mHeight);
	g->Translate(0, Ftol((double)aY));
	MenuPanel::DrawAll(theFlags, g);
	g->Translate(0, Ftol(-(double)aY));
}

void ProfilePanel::KeyChar(SexyChar theChar)
{
	if (theChar != '\r' && theChar != 0x1b) {
		Widget::KeyChar(theChar);
		return;
	}
	static_cast<ButtonListener*>(gApp->mMainMenu)->ButtonDepress(1);
}

void ProfilePanel::MouseDown(int, int, int, int)
{
	mMouseDown = true;
}

void ProfilePanel::MouseUp(int, int, int)
{
	mMouseDown = false;
}

void ProfilePanel::ListClicked(int, int theIdx, int theClickCount)
{
	mList->SetSelect(theIdx);
	SexyString aName = mList->GetStringAt(theIdx);
	if (aName != mApp->mProfile->mName) {
		// port: the original first writes and clears its beta dump (0x7648d2, 0x75e95b; NOTES.md), and reads it
		// again after (0x764814)
		mApp->mProfile->Select(aName);
		mMenu->RefreshPlayer();
	}
	if (theClickCount > 1)
		static_cast<ButtonListener*>(gApp->mMainMenu)->ButtonDepress(1);
}

void ProfilePanel::DialogButtonPress(int, int)
{
	mApp->PlaySample(SOUND_CLICK);
}

void ProfilePanel::DialogButtonDepress(int theDialogId, int theButtonId)
{
	if (theDialogId != DIALOG_DELETE)
		return;
	mApp->KillDialog(DIALOG_DELETE);
	if (theButtonId == Dialog::ID_YES) {
		SexyString aName = mList->GetStringAt(mList->mSelectIdx);
		if (!aName.empty()) {
			mApp->mProfile->Delete(aName);
			mRefresh = true;
		}
	}
}

void ProfilePanel::ButtonPress(int, int)
{
	gApp->PlaySample(SOUND_CLICK);
}

void ProfilePanel::ButtonDepress(int theId)
{
	if (theId == BUTTON_CREATE) {
		ShowNewUser();
	} else if (theId == BUTTON_DELETE) {
		SexyString aFooter = _S("");
		SexyString aLines = mApp->GetString(GTS(0x7db980), GT(0x7db928));
		SexyString aHeader = mApp->GetString(GTS(0x7db914), GT(0x7db8f8));
		TwistDialog* aDialog = static_cast<TwistDialog*>(mApp->DoDialog(DIALOG_DELETE, true, aHeader, aLines, aFooter,
			TwistDialog::BUTTON_MODE_YES | TwistDialog::BUTTON_MODE_NO));
		aDialog->mTwistListener = this;
	}
}

void ProfilePanel::Show()
{
	if (mSlide.mIncRate <= 0.0)
		mSlide.SetCurve(GTS(0x7db9ac));
}

void ProfilePanel::Hide()
{
	if (mSlide.mIncRate >= 0.0 && mSlide.mIncRate != 0.0) {
		mSlide.mTriggered = false;
		mSlide.mIncRate = -mSlide.mIncRate;
	}
}

void ProfilePanel::Refresh()
{
	mList->RemoveAll();
	int aCount = (int)mApp->mProfile->mNames.size();
	int aSelect = -1;
	for (int i = 0; i < aCount; i++) {
		SexyString aName = mApp->mProfile->GetName(i);
		if (!aName.empty()) {
			mList->AddLine(aName, false);
			if (aName == mApp->mProfile->mName)
				aSelect = i;
		}
	}
	if (aSelect == -1 && mList->GetLineCount() != 0)
		aSelect = 0;
	if (aSelect != -1)
		ListClicked(0, aSelect, 1);
	else
		ShowNewUser();
	mMenu->RefreshPlayer();
	if (mCreateButton != nullptr)
		mCreateButton->mDisabled = mApp->mProfile->mNames.size() >= 8;
}

void ProfilePanel::ShowNewUser()
{
	mMenu->ShowNewUser();
	gApp->mGates->mProfileButton->SetGlowOn(false);
}

NewUserPanel::NewUserPanel(ButtonListener* theButtonListener, EditListener* theEditListener, bool canCancel)
{
	mEditListener = theEditListener;
	mUnknownB0 = IMAGE_MAINMENU_NEWUSERDIALOG_ID;
	float aScale = (float)gApp->mArtRes / 1200.0f;
	TwistButton* aButton = new TwistButton(BUTTON_OK, IMAGE_MAINMENU_OKBUTTON, theButtonListener);
	PlaceButton(aButton, mUnknownB0, aScale);
	AddWidget(aButton);
	if (!canCancel) {
		int aWidth = GetImageById(mUnknownB0)->mWidth;
		aButton->mX = aWidth / 2 - aButton->mButtonImage->GetCelWidth() / 2;
	} else {
		aButton = new TwistButton(BUTTON_CANCEL, IMAGE_MAINMENU_CANCELBUTTON, theButtonListener);
		PlaceButton(aButton, mUnknownB0, aScale);
		AddWidget(aButton);
	}
	Image* anImage = GetImageById(mUnknownB0);
	Resize(0, 0, anImage->mWidth, anImage->mHeight);
}

NewUserPanel::~NewUserPanel()
{
	RemoveAllWidgets(true, false);
}

void NewUserPanel::AddedToManager(WidgetManager* theWidgetManager)
{
	mEditWidget = new TwistEditWidget(0, mEditListener);
	mEditWidget->mTextX = Ftol((double)gApp->mScale * 12.0);
	mEditWidget->mCursorOffsetX = Ftol((double)gApp->mScale * -5.0);
	mEditWidget->mSelectionExtra = Ftol((double)gApp->mScale);
	int aHeight = Ftol((double)gApp->mScale * 50.0);
	int aWidth = Ftol((double)mWidth * 0.699999988079071);
	int aY = Ftol((double)gApp->mScale * 74.0);
	int aX = Ftol((double)gApp->mScale * 150.0);
	mEditWidget->Resize(aX, aY, aWidth, aHeight);
	mEditWidget->SetFont(FONT_MENU_TINY, nullptr);
	SetFontLayerColor(mEditWidget->mFont, 0, Color::White);
	mEditWidget->SetColor(EditWidget::COLOR_BKG, Color(0, 0, 0, 0));
	mEditWidget->SetColor(EditWidget::COLOR_TEXT, Color(0xf0f0f0));
	mEditWidget->SetColor(EditWidget::COLOR_HILITE, Color(0xc332d7));
	mEditWidget->SetColor(EditWidget::COLOR_OUTLINE, Color(0, 0, 0, 0));
	mEditWidget->mMaxChars = 11;
	AddWidget(mEditWidget);
	SetFocus(mEditWidget);
	TwistWidget::AddedToManager(theWidgetManager);
	mCapitalize = true;
	mEditWidget->SetText(_S(""), true);
	gApp->mWidgetManager->SetFocus(mEditWidget);
	mSlide.SetCurve(GTS(0x7d9e74));
}

void NewUserPanel::RemovedFromManager(WidgetManager* theWidgetManager)
{
	// The menu it is in (the original passes its parent as it is)
	gApp->mWidgetManager->SetFocus(dynamic_cast<Widget*>(mParent));
	RemoveWidget(mEditWidget);
	gApp->SafeDeleteWidget(mEditWidget);
	mEditWidget = nullptr;
	TwistWidget::RemovedFromManager(theWidgetManager);
}

void NewUserPanel::Update()
{
	if (!mSlide.IncInVal() && mSlide.mIncRate < 0.0) {
		mParent->RemoveWidget(this);
		gApp->SafeDeleteWidget(this);
	}
	if (mCapitalize && mEditWidget != nullptr && !mEditWidget->mString.empty()) {
		// The original's text is wide (iswlower and towupper); the port's is Latin-1
		SexyChar& aFirst = mEditWidget->mString[0];
		if (iswlower((unsigned char)aFirst))
			aFirst = (SexyChar)towupper((unsigned char)aFirst);
		mCapitalize = false;
	}
}

void NewUserPanel::Draw(Graphics* g)
{
	g->DrawImage(GetImageById(mUnknownB0), 0, 0);
}

void NewUserPanel::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	float aY = (float)(mSlide.GetOutVal() * (double)gApp->mHeight * 0.5);
	g->Translate(0, Ftol((double)aY));
	TwistWidget::DrawAll(theFlags, g);
	g->Translate(0, Ftol(-(double)aY));
}

void NewUserPanel::Hide()
{
	mSlide.mIncRate = -fabs(mSlide.mIncRate);
	mSlide.mTriggered = false;
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

int ChallengeMenu::Unknown6E0DDB(bool, bool)
{
	NotPorted("ChallengeMenu 0x6e0ddb");
	return 0;
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
