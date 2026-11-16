// ThunderCommon.cpp: see ThunderCommon.h. The tweakable numbers of the original (its ModVal calls) are named constants, each
// with the line of ThunderCommon.cpp its call names.

#include "ThunderCommon.h"

#include "Color.h"
#include "Font.h"
#include "Dialog.h"
#include "Insets.h"
#include "ListWidget.h"
#include "NotTranslated.h"
#include "Res.h"
#include "SexyAppBase.h"
#include "ThunderButton.h"
#include "ThunderballApp.h"

using namespace Sexy;

static const int BUTTON_TEXT_OFFSET_Y = 5;			// line 131: the label of a button sits this far below the middle
static const int PURPLE_BUTTON_TEXT_OFFSET_X = 0;	// line 132
static const int PURPLE_BUTTON_TEXT_OFFSET_Y = 5;	// line 133
static const int PURPLE_BUTTON_FONT_COLOR = 10;
static const int BROWN_BUTTON_FONT_COLOR = 0;		// the brown buttons use the first colour set
static const int DIALOG_BUTTON_FONT_COLOR = 3;		// the others the fourth
static const int LIST_ITEM_HEIGHT = 16;				// line 130

// where dialogs go (PositionDialog; the lines are of ThunderCommon.cpp)
static const int DIALOG_HEADER_PADDING = 20;		// added to the header's width and the insets for the least width
static const int DIALOG_DEFAULT_TOP = 150;			// the top edge is this less the background's top inset
static const int DIALOG_BOTTOM_LIMIT = 580;			// the bottom edge goes no lower than this
static const int DIALOG_LEAST_TOP = 10;				// and the top edge no higher than this when it was pushed up
static const int DIALOG_CENTER_X = 400;				// the middle of the screen
static const int MENU_DIALOG_CENTER_X = 290;		// the middle of the part of the main menu to the right of the mascot (line 145)
static const int MENU_DIALOG_TALL = 300;			// from this height up it does not fit there (line 142)
static const int MENU_DIALOG_RIGHT_EDGE = 550;		// and goes against this edge instead (line 143)
static const int MENU_DIALOG_TALL_TOP = 50;			// at this height (line 144)
static const int NEW_USER_DIALOG_ID = 10;			// the dialog of the first profile; others open below it, lower by
static const int NEW_USER_DIALOG_OFFSET_Y = 140;	// this much when it can be closed (line 140; line 141 is 0)

// the colours of the list widget, by its indices (lines 125 to 129)
static const int LIST_BACKGROUND_COLOR = 0x50;
static const int LIST_OUTLINE_COLOR = 0x800000;
static const int LIST_TEXT_COLOR = 0xffff50;
static const int LIST_HILITE_COLOR = 0xffffff;
static const int LIST_SELECT_COLOR = 0xf45d02;

// 0x407c40
void Sexy::SetThreeStateButton(ThunderButton* theButton, bool theSideBySide)
{
	if (theButton == nullptr)
		return;

	int aWidth = theButton->mComponentImage->mWidth;
	int aHeight = theButton->mComponentImage->mHeight;
	if (!theSideBySide)
	{
		aHeight = aHeight / 3;
		theButton->mNormalRect = Rect(0, 0, aWidth, aHeight);
		theButton->mOverRect = Rect(0, aHeight, aWidth, aHeight);
		theButton->mDownRect = Rect(0, aHeight * 2, aWidth, aHeight);
	}
	else
	{
		aWidth = aWidth / 3;
		theButton->mNormalRect = Rect(0, 0, aWidth, aHeight);
		theButton->mOverRect = Rect(aWidth, 0, aWidth, aHeight);
		theButton->mDownRect = Rect(aWidth * 2, 0, aWidth, aHeight);
	}

	theButton->SetFont(FONT_BUTTON);
	theButton->mTextOffsetY = BUTTON_TEXT_OFFSET_Y;
	theButton->SetColor(ButtonWidget::COLOR_LABEL, Color(0xffffff));
	theButton->SetColor(ButtonWidget::COLOR_LABEL_HILITE, Color(0xffffff));
}

// 0x41bc70
ThunderButton* Sexy::MakeButton(int theId, ButtonListener* theListener, const std::string& theLabel, Image* theImage)
{
	if (theImage == nullptr)
		theImage = IMAGE_DLG_BUTTON3;

	ThunderButton* aButton = new ThunderButton(theImage, theId, theListener);
	aButton->mLabel = theLabel;
	aButton->mWidth = aButton->mComponentImage->mWidth;
	aButton->mHeight = aButton->mComponentImage->mHeight / 3;
	if (theImage == IMAGE_DLG_BROWNBUTTON1 || theImage == IMAGE_DLG_BROWNBUTTON2)
		aButton->mFontColor = BROWN_BUTTON_FONT_COLOR;
	else
		aButton->mFontColor = DIALOG_BUTTON_FONT_COLOR;
	aButton->mDownSound = SOUND_BUTTON1;
	SetThreeStateButton(aButton, false);
	return aButton;
}

// 0x41bd60
ThunderButton* Sexy::MakePurpleButton(int theId, ButtonListener* theListener, const std::string& theLabel)
{
	ThunderButton* aButton = MakeButton(theId, theListener, theLabel, IMAGE_DLG_PURPLEBUTTON);
	aButton->mFontColor = PURPLE_BUTTON_FONT_COLOR;
	aButton->mTextOffsetX = PURPLE_BUTTON_TEXT_OFFSET_X;
	aButton->mTextOffsetY = PURPLE_BUTTON_TEXT_OFFSET_Y;
	return aButton;
}

// 0x41bdc0: the picture holds two states (normal and over/pressed); nothing is drawn while the mouse is not over it
ThunderButton* Sexy::MakeBottomButton(int theId, ButtonListener* theListener, const std::string& theLabel)
{
	ThunderButton* aButton = MakeButton(theId, theListener, theLabel, nullptr);
	Image* aImage = IMAGE_DLG_PLAYBUTTON1;
	aButton->mComponentImage = aImage;
	aButton->mWidth = aImage->mWidth;
	aButton->mHeight = aImage->mHeight / 2;
	int aHalf = aImage->mHeight / 2;
	aButton->mNormalRect.mWidth = 1;
	aButton->mNormalRect.mHeight = 0;
	aButton->mOverRect = Rect(0, 0, aImage->mWidth, aHalf);
	aButton->mDownRect = Rect(0, aHalf, aImage->mWidth, aHalf);
	return aButton;
}

// 0x407d90
ThunderButton* Sexy::MakeEmbeddedButton(int theId, ButtonListener* theListener, Image* theImage, bool theSinglePicture)
{
	// the picture holds the over and the pressed states, side by side if it has columns, otherwise one above the other
	int aWidth = theImage->mWidth;
	int aHeight = theImage->mHeight;
	if (theImage->mNumCols < 2)
		aHeight = aHeight / 2;
	else
		aWidth = aWidth / 2;
	if (theSinglePicture && theImage->mNumRows == 1 && theImage->mNumCols == 1)
	{
		aHeight = theImage->mHeight;
		aWidth = theImage->mWidth;
	}

	ThunderButton* aButton = new ThunderButton(theImage, theId, theListener);
	aButton->mWidth = aWidth;
	aButton->mHeight = aHeight;
	aButton->mFontColor = BUTTON_FONT_COLOR_NONE;
	aButton->mDownSound = SOUND_BUTTON1;
	aButton->mNormalRect.mWidth = 1; // nothing of the picture is drawn until the mouse is over it
	aButton->mNormalRect.mHeight = 0;
	aButton->mOverRect = Rect(0, 0, aWidth, aHeight);
	if (aHeight == theImage->mHeight)
		aButton->mDownRect = Rect(aWidth, 0, aWidth, aHeight);
	else
		aButton->mDownRect = Rect(0, aHeight, aWidth, aHeight);
	if (theSinglePicture)
	{
		aButton->mDownRect = aButton->mOverRect;
		aButton->mDarkenWhenPressed = true;
	}
	return aButton;
}

// 0x405270
SimpleButton* Sexy::MakeSimpleButton(int theId, ButtonListener* theListener, Image* theImage, int theExtraWidth, int theExtraHeight)
{
	SimpleButton* aButton = new SimpleButton(theImage, theId, theListener);
	aButton->mWidth = theImage->mWidth + theExtraWidth;
	aButton->mHeight = theImage->mHeight + theExtraHeight;
	return aButton;
}

// 0x4050a0
ListWidget* Sexy::MakeListWidget(int theId, ListListener* theListener)
{
	ListWidget* aList = new ListWidget(theId, FONT_PLAIN, theListener); // (line 124 names the font by its id, 0x1d)
	aList->SetColor(ListWidget::COLOR_BKG, Color(LIST_BACKGROUND_COLOR));
	aList->SetColor(ListWidget::COLOR_OUTLINE, Color(LIST_OUTLINE_COLOR));
	aList->SetColor(ListWidget::COLOR_TEXT, Color(LIST_TEXT_COLOR));
	aList->SetColor(ListWidget::COLOR_HILITE, Color(LIST_HILITE_COLOR));
	aList->SetColor(ListWidget::COLOR_SELECT, Color(LIST_SELECT_COLOR));
	aList->mDrawOutline = false;
	aList->mItemHeight = LIST_ITEM_HEIGHT;
	return aList;
}

// 0x40bc20
void Sexy::PositionDialog(Dialog* theDialog, int theWidth, bool theCentered, int theY)
{
	ThunderballApp* anApp = gThunderballApp;

	// as wide as the header, the insets and a little room need
	int aLeastWidth = theDialog->mHeaderFont->StringWidth(theDialog->mDialogHeader) + theDialog->mContentInsets.mRight +
					  theDialog->mContentInsets.mLeft + theDialog->mBackgroundInsets.mRight + DIALOG_HEADER_PADDING +
					  theDialog->mBackgroundInsets.mLeft;
	if (theWidth < aLeastWidth)
		theWidth = aLeastWidth;

	// the dialog that was up first (the smallest id), if there is one
	Dialog* anOther = nullptr;
	if (anApp->GetDialogCount() != 0)
		anOther = anApp->mDialogMap.begin()->second;

	int aHeight = theDialog->GetPreferredHeight(theWidth);
	int aTop = DIALOG_DEFAULT_TOP - theDialog->mBackgroundInsets.mTop;
	if (theY >= 0)
		aTop = theY;
	if (aTop + aHeight > DIALOG_BOTTOM_LIMIT)
	{
		aTop = DIALOG_BOTTOM_LIMIT - aHeight;
		if (aTop < DIALOG_LEAST_TOP)
			aTop = DIALOG_LEAST_TOP;
	}

	int aLeft;
	if (anOther == nullptr)
	{
		int aCenterX;
		if (anApp->mMainMenu == nullptr || theCentered)
			aCenterX = DIALOG_CENTER_X;
		else if (aHeight < MENU_DIALOG_TALL)
			aCenterX = MENU_DIALOG_CENTER_X;
		else
		{
			// too tall to go beside the menu's mascot: against the right edge of the menu's area
			theDialog->Resize(MENU_DIALOG_RIGHT_EDGE - theWidth, MENU_DIALOG_TALL_TOP, theWidth, aHeight);
			return;
		}
		aLeft = aCenterX - theWidth / 2;
	}
	else
	{
		// over the other dialog: the same middle, and the same top (lower when the new-user dialog can be closed)
		if (anOther->mId == NEW_USER_DIALOG_ID)
			NOT_TRANSLATED("PositionDialog with the new-user dialog up (its +0x188 flag)");
		aTop = anOther->mY;
		aLeft = (anOther->mWidth - theWidth) / 2 + anOther->mX;
	}
	theDialog->Resize(aLeft, aTop, theWidth, aHeight);
}
