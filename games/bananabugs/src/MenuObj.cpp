#include "MenuObj.h"

#include "Anim.h"
#include "MenuAnim.h"
#include "ObjString.h"
#include "Pending.h"

#include <cstdlib>
#include <cstring>

using namespace Sexy;

// A line's text before it is set: this many of the letter A, a width of the panel's own
const int LINE_PLACEHOLDER_SIZE = 0x84;
const char LINE_PLACEHOLDER_LETTER = 'A';
const int LINE_PLACEHOLDER_DIVISOR = 0x18;
const int LINE_SPACING = 32;

// 0x446ac0
MenuObj::MenuObj()
{
	mHasLines = false;
	mPanel = nullptr;
	ObjBase::Deactivate(this);
}

// 0x446af0 (body 0x446b10): the panel is this object's own picture
MenuObj::~MenuObj()
{
	if (mAnim != nullptr) {
		delete mAnim;
		mAnim = nullptr;
	}
}

// 0x446b90 (EDI the columns, EBX the rows): a panel of at least two by two tiles; with less the original deletes the
// window and ends the program
void MenuObj::BuildPanel(int theColumns, int theRows, bool openRight)
{
	if (theColumns < 2 || theRows < 2) {
		Delete();
		std::_Exit(-1);
	}
	MenuAnim* aPanel = new MenuAnim(theRows, theColumns, openRight);
	mPanel = aPanel;
	mAnim = aPanel;
	Activate(this, false);
}

// 0x446c30: a column of lines down the panel, 32 design pixels apart, each of letter As to fill the panel's width and
// inactive (SetLineText sets the text and activates the line). Left-aligned lines start at the panel's left side.
void MenuObj::BuildLines(BBFont* theFont, bool leftAligned)
{
	if (leftAligned)
		mLineX = (float)(64.0 - (double)GetPanelHalfWidth());
	else
		mLineX = 0.0f;
	float aY = (float)((60.0 - (double)GetPanelHalfHeight()) + 16.0);
	mLineTop = aY;
	int aLineCount = (mPanel->mHeight - 100) / LINE_SPACING;
	int aTextLength = mPanel->mWidth / LINE_PLACEHOLDER_DIVISOR;
	char aText[LINE_PLACEHOLDER_SIZE];
	memset(aText, 0, sizeof(aText));
	if (aTextLength > 0)
		memset(aText, LINE_PLACEHOLDER_LETTER, aTextLength);
	if (aLineCount > 0) {
		ObjString** aSlot = mLines;
		for (int aLeft = aLineCount; aLeft != 0; aLeft--) {
			ObjString* aLine = new ObjString();
			*aSlot = aLine;
			aLine->SetParent(this, nullptr);
			aLine->SetFont(theFont);
			aLine->Create(aText, leftAligned, 0);
			aLine->SetState(STATE_X, mLineX);
			aLine->SetState(STATE_Y, aY);
			aLine->SetState(STATE_Z, 1.0f);
			aLine->SetState(STATE_SCALE_X, 0.5f);
			aLine->SetState(STATE_SCALE_Y, 0.5f);
			aLine->SetState(STATE_RED, 1.0f);
			aLine->SetState(STATE_GREEN, 1.0f);
			aLine->SetState(STATE_BLUE, 1.0f);
			aLine->Deactivate(aLine);
			aY = (float)((double)aY + 32.0);
			aSlot++;
		}
	}
	mLineCount = aLineCount;
	mHasLines = true;
}

// 0x446e40 (EAX the window, ECX the line)
void MenuObj::SetLineColor(int theLine, float theRed, float theGreen, float theBlue, float theAlpha)
{
	if (mHasLines && theLine < mLineCount) {
		ObjString* aLine = mLines[theLine];
		aLine->SetState(STATE_RED, theRed);
		aLine->SetState(STATE_GREEN, theGreen);
		aLine->SetState(STATE_BLUE, theBlue);
		aLine->SetState(STATE_ALPHA, theAlpha);
	}
}

// 0x446eb0 (EDX the window, ECX the first line): moves the line down, and every line after it with throughLast
void MenuObj::MoveLines(int theFirst, float theDeltaY, bool throughLast)
{
	if (mHasLines && theFirst < mLineCount) {
		int aEnd = mLineCount;
		if (!throughLast)
			aEnd = theFirst + 1;
		for (int i = theFirst; i < aEnd; i++) {
			ObjString* aLine = mLines[i];
			aLine->SetState(STATE_Y, aLine->mBase[STATE_Y] + theDeltaY);
		}
	}
}

// 0x446f10 (EAX the window, ECX the line)
void MenuObj::SetLineScale(int theLine, float theScaleX, float theScaleY)
{
	if (mHasLines && theLine < mLineCount) {
		ObjString* aLine = mLines[theLine];
		aLine->SetState(STATE_SCALE_X, theScaleX);
		aLine->SetState(STATE_SCALE_Y, theScaleY);
	}
}

// 0x446f50 (EAX the window, ECX the line): the line's text, and the line active
void MenuObj::SetLineText(int theLine, const char* theText)
{
	if (mHasLines && theLine < mLineCount) {
		ObjString* aLine = mLines[theLine];
		aLine->SetText(theText);
		aLine->Activate(aLine, true);
	}
}

// 0x446f90
float MenuObj::GetPanelHalfWidth()
{
	return (float)((double)mPanel->mWidth * 0.5);
}

// 0x446fb0
float MenuObj::GetPanelHalfHeight()
{
	return (float)((double)mPanel->mHeight * 0.5);
}

// 0x446fd0: a whole number from theMin to theMax, as a float
float MenuObj::RandomWhole(float theMin, float theMax)
{
	return (float)GameRandomRange((int)theMax, (int)theMin);
}
