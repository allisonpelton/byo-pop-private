// Sexy::HintDialog, Sexy::HelpDialog and Sexy::QuestHelpDialog. See HintDialog.h.
#include "HintDialog.h"

#include "Bej3App.h"
#include "Board.h"
#include "CrystalBall.h"
#include "DialogButton.h"
#include "Font.h"
#include "Graphics.h"
#include "Profile.h"
#include "Res.h"

namespace Sexy {

namespace {

// The exe's measures are in 1200ths of the art size in use
int Scaled(int theMeasure)
{
	return gApp->mArtRes * theMeasure / 0x4b0;
}

// 0x475e30: the text of a numbered string in default.xml. The texts are read at run time; the defaults are empty
std::string TextById(int theId, const std::string& theDefault)
{
	return gApp->GetString(std::to_string(theId), theDefault);
}

// the strings 0x109 (OK), 0x10b (Disable Hints), 0x106 (Don't show hints) and 0xef (HOW TO PLAY)
const int TEXT_OK = 0x109;
const int TEXT_DISABLE_HINTS = 0x10b;
const int TEXT_DONT_SHOW_HINTS = 0x106;
const int TEXT_HOW_TO_PLAY = 0xef;

// the box of each bit of Board::HINT_*: the header's string and the text's (0x765d50 picks them by the bit)
struct HintTexts {
	int mHintId;
	int mHeader;
	int mLines;
};
const HintTexts HINT_TEXTS[] = {
	{Board::HINT_FLAME, 0x60, 0x61},
	{Board::HINT_STAR, 0x62, 0x63},
	{Board::HINT_HYPERCUBE, 0x64, 0x65},
	{Board::HINT_MULTIPLIER, 0x68, 0x69},
	{Board::HINT_SUPERNOVA, 0x66, 0x67},
	{Board::HINT_COIN, 0x6a, 0x6b},
	{Board::HINT_TIME_GEM, 0x6c, 0x6d},
	{Board::HINT_VERTICAL_MATCH, 0x6e, 0x6f},
	{Board::HINT_DARK_ROCK, 0x70, 0x71},
};

const HintTexts* FindHintTexts(int theHintId)
{
	for (const HintTexts& aTexts : HINT_TEXTS) {
		if (aTexts.mHintId == theHintId)
			return &aTexts;
	}
	return nullptr;
}

// the pages of each help (0x694900 reads them): a title (0 for none) and up to three page texts
struct HelpTexts {
	int mHelpId;
	int mTitle;
	int mPages[3];
};
const HelpTexts HELP_TEXTS[] = {
	{0x15, 0xf3, {0xf4, 0xf5, 0xf6}},       // Classic
	{0, 0, {0xf4, 0xf5, 0xf6}},             // Zen
	{10, 0, {0xf7, 0xf8, 0xf9}},            // Lightning
	{0x11, 0, {0xfa, 0xfb, 0}},             // Butterflies
	{0x12, 0, {0xfc, 0xfd, 0xfe}},          // Poker
	{0x14, 0xff, {0x100, 0x101, 0x102}},    // Ice Storm
	{0x16, 0, {0x103, 0x104, 0x105}},       // Diamond Mine
};

// 0x696100: the button's label (strings 0xf0 to 0xf2): START GAME when this help is the only request waiting and the game is held for it, else CLOSE
std::string HelpButtonLabel(int theQueued)
{
	if (gApp->mBoard == nullptr || gApp->mBoard->mHelpHold < 1)
		return TextById(0xf2, "");
	return theQueued < 2 ? TextById(0xf1, "") : TextById(0xf0, "");
}

}  // namespace

HintDialog::HintDialog(int theHintId) :
	Bej3Dialog(Board::HINT_DIALOG_ID, true, "", "", TextById(TEXT_OK, "OK"), BUTTONS_FOOTER)
{
	const HintTexts* aTexts = FindHintTexts(theHintId);
	if (aTexts != nullptr) {
		mDialogHeader = TextById(aTexts->mHeader, "");
		mDialogLines = TextById(aTexts->mLines, "");
	}
	mCheckbox = new Checkbox(IMAGE_DIALOG_CHECKBOX_BLANK, IMAGE_DIALOG_CHECKBOX_CHECKED, 0, this);
	mCheckbox->Resize(0, 0, IMAGE_DIALOG_CHECKBOX_BLANK->GetWidth(), IMAGE_DIALOG_CHECKBOX_BLANK->GetHeight());
	AddWidget(mCheckbox);
}

HintDialog::~HintDialog()
{
	RemoveWidget(mCheckbox);
	delete mCheckbox;
}

// port: the exe's box gives room under its text for the "Disable Hints" box (its height is not read; 60 of the 1200ths is the port's measure)
int HintDialog::GetPreferredHeight(int theWidth)
{
	return Bej3Dialog::GetPreferredHeight(theWidth) + Scaled(60);
}

// the checkbox stands left of its label, which 0x6829c0 draws at the dialog's middle less 110 (1200ths) and 160 above the bottom
void HintDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Bej3Dialog::Resize(theX, theY, theWidth, theHeight);
	if (mCheckbox != nullptr)
		mCheckbox->Move(mWidth / 2 - Scaled(110), mHeight - Scaled(160) - mCheckbox->mHeight / 2);
}

// 0x6829c0 (bejeweled3): the Dialog's drawing, then the label of the checkbox in the tooltip's bold font, brown, at the right of the checkbox
void HintDialog::Draw(Graphics* g)
{
	Bej3Dialog::Draw(g);
	if (mCheckbox == nullptr)
		return;
	Color aOldColor;
	const bool aSet = SetLayerColor(FONT_TOOLTIP_BOLD, "Main", Color(255, 255, 255), &aOldColor);
	g->SetFont(FONT_TOOLTIP_BOLD);
	g->SetColor(Color(128, 64, 0, 128));
	g->DrawString(TextById(TEXT_DISABLE_HINTS, ""), mCheckbox->mX + mCheckbox->mWidth, mHeight - Scaled(160));
	if (aSet)
		RestoreLayerColor(FONT_TOOLTIP_BOLD, "Main", aOldColor);
}

HelpDialog::HelpDialog(int theHelpId, int theQueued) :
	Bej3Dialog(Board::HELP_DIALOG_ID, true, TextById(TEXT_HOW_TO_PLAY, "HOW TO PLAY"), "", HelpButtonLabel(theQueued), BUTTONS_FOOTER),
	mHelpId(theHelpId)
{
	mMouseDownAllowed = false;
	for (const HelpTexts& aTexts : HELP_TEXTS) {
		if (aTexts.mHelpId != theHelpId)
			continue;
		if (aTexts.mTitle != 0)
			mTitle = TextById(aTexts.mTitle, "");
		for (int aPageText : aTexts.mPages) {
			if (aPageText != 0)
				mPages.push_back(TextById(aPageText, ""));
		}
		break;
	}
	mCheckbox = new Checkbox(IMAGE_DIALOG_CHECKBOX_BLANK, IMAGE_DIALOG_CHECKBOX_CHECKED, 0, this);
	mCheckbox->Resize(Scaled(0x41a), Scaled(0x3fc), IMAGE_DIALOG_CHECKBOX_BLANK->GetWidth(), IMAGE_DIALOG_CHECKBOX_BLANK->GetHeight());
	mCheckbox->SetChecked(gApp->mProfile != nullptr && (gApp->mProfile->mHintFlags & Board::HINTS_OFF) != 0, false);
	AddWidget(mCheckbox);
	Resize(Scaled(25), Scaled(25), Scaled(0x60e), Scaled(0x47e));
}

HelpDialog::~HelpDialog()
{
	RemoveWidget(mCheckbox);
	delete mCheckbox;
}

// 0x696100: the button is 350 wide (1200ths) at the middle less 160 and 150 above the bottom
void HelpDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Bej3Dialog::Resize(theX, theY, theWidth, theHeight);
	if (mYesButton != nullptr)
		mYesButton->Resize(mX + mWidth / 2 - Scaled(160), mY + mHeight - Scaled(150), Scaled(350), mButtonHeight);
}

// port: the exe's HelpWindow (0x68a7f0) draws each page's picture and text; the port writes the title and the pages' texts in columns over the pages'
// rectangle (0x696100: 90, 150, the width less 180 and the height less 330, 1200ths), and the label of the checkbox
void HelpDialog::Draw(Graphics* g)
{
	Bej3Dialog::Draw(g);
	g->SetFont(FONT_DIALOG_SMALL_TEXT);
	g->SetColor(Color(0, 0, 0));
	const int aLeft = Scaled(90);
	int aTop = Scaled(150);
	const int aWidth = mWidth - Scaled(180);
	const int aHeight = mHeight - Scaled(330);
	if (!mTitle.empty()) {
		g->WriteWordWrapped(Rect(aLeft, aTop, aWidth, 0), mTitle, -1, 0);
		aTop += FONT_DIALOG_SMALL_TEXT->GetHeight() + Scaled(20);
	}
	const int aColumns = (int)mPages.size();
	for (int aColumn = 0; aColumn < aColumns; aColumn++) {
		const int aColumnWidth = aWidth / aColumns - Scaled(20);
		g->WriteWordWrapped(Rect(aLeft + aColumn * (aWidth / aColumns), aTop, aColumnWidth, aHeight), mPages[(size_t)aColumn], -1, -1);
	}
	g->DrawString(TextById(TEXT_DONT_SHOW_HINTS, ""), Scaled(0x44c), Scaled(0x41a));
}

void HelpDialog::ButtonDepress(int theId)
{
	if (theId == ID_YES && !mClosing && gApp->mProfile != nullptr) {
		gApp->mProfile->mHintFlags |= 1u << mHelpId;
		if (mHelpId == 0x15)
			gApp->mProfile->mHintFlags |= 1u;
		if (mHelpId == 0)
			gApp->mProfile->mHintFlags |= 1u << 0x15;
	}
	Bej3Dialog::ButtonDepress(theId);
}

// 0x6746e0 (bejeweled3): the box turns every hint box and help off or on, in the profile and the board
void HelpDialog::CheckboxChecked(int theId, bool theChecked)
{
	if (gApp->mBoard != nullptr)
		gApp->mBoard->SetHintFlag(Board::HINTS_OFF_BIT, theChecked);
	else if (gApp->mProfile != nullptr) {
		if (theChecked)
			gApp->mProfile->mHintFlags |= Board::HINTS_OFF;
		else
			gApp->mProfile->mHintFlags &= ~(uint32_t)Board::HINTS_OFF;
	}
}

QuestHelpDialog::QuestHelpDialog(Board* theBoard) :
	Bej3Dialog(0x28, true, "", "", TextById(0x1a2, "OK"), BUTTONS_FOOTER)
{
	auto aTitle = theBoard->mSettings.find("TitleText");
	if (aTitle != theBoard->mSettings.end())
		mDialogHeader = aTitle->second;
	auto aText = theBoard->mSettings.find("HelpText");
	if (aText != theBoard->mSettings.end())
		mDialogLines = aText->second;
	if (mDialogLines.empty())
		mDialogLines = "This Quest needs help text. Insert a HelpText parameter in the appropriate quest .XML file";
	mFadeCurve.SetCurve("b+0,1,0.025,1,####         ~~###");
	// 1000 wide (1200ths), as high as the text asks; a time quest's box has 180 more under it
	if (theBoard->GetTimeLimit() > 0)
		mContentInsets.mBottom += Scaled(180);
	const int aWidth = Scaled(1000);
	const int aHeight = GetPreferredHeight(aWidth) + Scaled(5);
	Resize(Scaled(theBoard->GetGridX() + 0x200) - aWidth / 2, theBoard->mHeight / 2 - Scaled(100) - aHeight / 2, aWidth, aHeight);
	if (mYesButton != nullptr)
		mYesButton->Resize(mX + (mWidth - Scaled(200)) / 2, mY + mHeight - mContentInsets.mBottom - mBackgroundInsets.mBottom - mButtonHeight, Scaled(200), mButtonHeight);
	gApp->AddDialog(this);
}

}  // namespace Sexy
