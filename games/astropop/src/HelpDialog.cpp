// HelpDialog: the "how to play" box (see HelpDialog.h).

#include "HelpDialog.h"
#include "AstroPopApp.h"
#include "Board.h"
#include "DialogButton.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Res.h"

using namespace Sexy;

// 0x48ab10: the box is of the colour set it is given (the game's mode + 2, or 1 from the selector). The lines of the first
// page are those of the game that is on: none (from the selector: all the rules), the classic game (m1a60 is 1) or the other.
HelpDialog::HelpDialog(int theColourSet, AstroPopApp* theApp) :
	Text2004<Dialog>(IMAGE_DIALOG, IMAGE_DIALOG_BTN, 0x79, true, GT(0x0056b554), std::string(), std::string(), 1)
{
	mApp = theApp;
	mLinesTopOffset = 0;
	mTextIndent = IMAGE_FUELCELL->mWidth + 10;
	mPage = 0;

	mYesButton->mLabel = "CLOSE";
	mNoButton->mLabel = "MORE...";

	if (theApp->mBoard == NULL)
	{
		mLines.push_back(GT(0x56be30));
		mLines.push_back(GT(0x56bde8));
		mLines.push_back(GT(0x56bdb4));
		mLines.push_back(GT(0x56c134));
		mLines.push_back(GT(0x56bd38));
		mLines.push_back(GT(0x56bcc0));
	}
	else if (theApp->mBoard->m1a60 == 1)
	{
		mLines.push_back(GT(0x56c19c));
		mLines.push_back(GT(0x56c16c));
		mLines.push_back(GT(0x56c134));
		mLines.push_back(GT(0x56c0c8));
		mLines.push_back(GT(0x56c060));
	}
	else
	{
		mLines.push_back(GT(0x56bff8));
		mLines.push_back(GT(0x56c134));
		mLines.push_back(GT(0x56bf68));
		mLines.push_back(GT(0x56bef0));
		mLines.push_back(GT(0x56be88));
	}

	Color anInsetColor;
	Color aButtonColor;
	if (theColourSet >= 0 && theColourSet <= 5)
	{
		anInsetColor = theApp->GetDialogInsetColor(theColourSet);
		aButtonColor = theApp->GetDialogColor(theColourSet);
	}
	MemoryImage* anInsetImage = NULL;
	MemoryImage* aButtonImage = NULL;
	theApp->MakeDialogImages(anInsetColor, aButtonColor, &anInsetImage, &aButtonImage, true);
	mComponentImage = anInsetImage;

	// the 2004 Dialog reads the button height from the button picture at each use (GetPreferredHeight 0x4e91d0 and Resize); the
	// port keeps the height it took at construction, so it follows the picture that replaces it
	mYesButton->mComponentImage = aButtonImage;
	mButtonHeight = aButtonImage->mHeight;
	mYesButton->mNormalRect = Rect(0, 0, 0x78, 0x21);
	mYesButton->mOverRect = Rect(0x78, 0, 0x78, 0x21);
	mYesButton->mDownRect = Rect(0xf0, 0, 0x78, 0x21);
	if (mNoButton != NULL)
	{
		mNoButton->mNormalRect = Rect(0, 0, 0x78, 0x21);
		mNoButton->mOverRect = Rect(0x78, 0, 0x78, 0x21);
		mNoButton->mDownRect = Rect(0xf0, 0, 0x78, 0x21);
		mNoButton->mComponentImage = aButtonImage;
	}

	AddItem(IMAGE_FUELCELL, GT(0x56bc70));
	AddItem(IMAGE_BRICK_RADIAL, GT(0x56bc38));
	AddItem(IMAGE_BRICK_STONE, GT(0x56bbf0));
	AddItem(IMAGE_BRICK_STEEL_1, GT(0x56bbbc));
	AddItem(IMAGE_BRICK_ROW_NUKE, GT(0x56bb8c));
	AddItem(IMAGE_BRICK_MUTATOR, GT(0x56bb40));
	AddItem(IMAGE_BRICK_FREEZE, GT(0x56bb20));
	AddItem(IMAGE_BRICK_KILLALL_PINK, GT(0x56baec));
	AddItem(IMAGE_BRICK_COL_NUKE, GT(0x56bac8));
	AddItem(IMAGE_BRICK_BOOSTER, GT(0x56ba70));
}

// 0x48a6b0: the box's picture and the buttons' picture are its own
HelpDialog::~HelpDialog()
{
	delete mComponentImage;
	mComponentImage = NULL;
	if (mYesButton != NULL)
		delete mYesButton->mComponentImage;
}

// 0x48aa50
void HelpDialog::AddItem(Image* theImage, const std::string& theText)
{
	Item anItem;
	anItem.mImage = theImage;
	anItem.mText = theText;
	mItems.push_back(anItem);
}

// 0x4837e0: the box is taller from the selector, where the first page has more to say
int HelpDialog::GetPreferredHeight(int theWidth)
{
	(void) theWidth;
	return mApp->mBoard == NULL ? 0x186 : 0x168;
}

// 0x4882d0: the first page, a white square and a text for each rule and, at the foot, the hint to the other pages; or, on
// the other pages, five of the ten bricks, each with its picture and a text beside it
void HelpDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);
	Rect aBox(mBackgroundInsets.mLeft, mBackgroundInsets.mTop, (mWidth - mBackgroundInsets.mRight) - mBackgroundInsets.mLeft,
			  (mHeight - mBackgroundInsets.mBottom) - mBackgroundInsets.mTop);
	g->DrawImageBox(aBox, IMAGE_DIALOG);
	g->SetFont(mLinesFont);

	if (mPage == 0)
	{
		int aUsed = 0;
		for (size_t i = 0; i < mLines.size(); i++)
		{
			int aTop = mLinesTopOffset + mLinesFont->GetHeight() * 2 + mSpaceAfterHeader + mContentInsets.mTop + aUsed;
			g->SetColor(Color(0xff, 0xff, 0xff));
			g->FillRect(mContentInsets.mLeft, aTop, 8, 8);
			Rect aRect(mContentInsets.mLeft + 0x10, aTop - 4, ((mWidth - mContentInsets.mRight) - mContentInsets.mLeft) - 0x10,
					   mHeight);
			int aHeight = WriteWordWrapped(g, aRect, mLines[i], -1, -1);
			aUsed += aHeight + 0xf;
		}

		std::string aMore = GT(0x56b50c);
		Rect aRect(mContentInsets.mLeft,
				   mLinesTopOffset - 10 + mLinesFont->GetHeight() * 2 + mSpaceAfterHeader + mContentInsets.mTop + aUsed,
				   ((mWidth - mContentInsets.mRight) - mContentInsets.mLeft) - 0x10, mHeight);
		WriteWordWrapped(g, aRect, aMore, -1, 0);
	}
	else
	{
		// the first page of bricks has the first half of them, the second the rest
		unsigned int aCount = (unsigned int) mItems.size();
		unsigned int anEnd = (mPage == 1) ? (aCount >> 1) : aCount;
		unsigned int aStart = ((unsigned int) ((mPage - 1) * (int) aCount)) >> 1;

		int aUsed = 0;
		for (unsigned int i = aStart; i < anEnd; i++)
		{
			Image* anImage = mItems[i].mImage;
			int aTop = mLinesTopOffset + mLinesFont->GetHeight() * 2 + mSpaceAfterHeader + mContentInsets.mTop + aUsed;
			g->DrawImageCel(anImage, mContentInsets.mLeft, aTop, 0);
			Rect aRect(mTextIndent + mContentInsets.mLeft, aTop,
					   ((mWidth - mTextIndent) - mContentInsets.mLeft) - mContentInsets.mRight, 0);
			int aHeight = WriteWordWrapped(g, aRect, mItems[i].mText, -1, -1) + 8;
			if (aHeight < anImage->mHeight)
				aHeight = anImage->mHeight + 8;
			aUsed += 10 + aHeight;
		}
	}
}

// 0x4886a0: CLOSE is for the application (which takes the box away); MORE... turns to the next page, and back to the first
// after the last, and the heading says which kind of page it is
void HelpDialog::ButtonDepress(int theId)
{
	if (theId == ID_YES)
	{
		Dialog::ButtonDepress(theId);
		return;
	}

	if (mPage == 0)
	{
		mDialogHeader = GT(0x0056b544);
		mPage = 1;
	}
	else if (mPage == 1)
	{
		mDialogHeader = GT(0x0056b544);
		mPage = 2;
	}
	else if (mPage == 2)
	{
		mPage = 0;
		mDialogHeader = GT(0x0056b554);
	}
}
