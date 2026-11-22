// ThunderDialog.cpp: see ThunderDialog.h. The original keeps its tweakable numbers in ModVal calls (each returns the number it
// was compiled with); they are named constants here, each with the line of ThunderDialog.cpp its call names. A few parts of
// the original are dead: the ModVal call at line 851 is 0, which turns off the chain's swinging in Update; line 846 is 0, which
// turns off a bounce at the end of a slide; and line 845 is 0, the bounce's height. They are left out.

#include "ThunderDialog.h"

#include "Color.h"
#include "Font.h"
#include "Graphics.h"
#include "Insets.h"
#include "Res.h"
#include "SexyAppBase.h"
#include "ThunderCommon.h"
#include "ThunderButton.h"
#include "WidgetManager.h"

using namespace Sexy;

static const int CONTENT_INSET_LEFT = 35;			// lines 832 to 835
static const int CONTENT_INSET_TOP = 20;
static const int CONTENT_INSET_RIGHT = 35;
static const int CONTENT_INSET_BOTTOM = 26;
static const int SPACE_AFTER_HEADER = 32;			// line 842
static const int CHAIN_SPACING = 35;				// line 843
static const int HEADER_COLOR = 0x63f7ff;			// line 841
static const int TEXT_COLOR = 0xffffff;				// lines 838 to 840
static const int DEFAULT_REST_Y = 100;				// line 844: where a dialog that was made off the screen comes to rest
static const int SCREEN_HEIGHT = 600;				// a dialog that comes up from below starts here
static const int SCROLL_SPEED_DIVISOR = 5;			// line 858: a slide covers this share of what is left in an update
static const int SCROLL_SPEED_MIN = 10;				// line 859: pixels an update
static const int SCROLL_SPEED_MAX = 40;				// line 860
static const int SCROLL_OFF_SPEED = 40;				// line 861
static const int LEFT_EDGE_ADJUST = 0;				// line 848
static const int TOP_EDGE_ADJUST = 12;				// line 849
static const int WIDTH_ADJUST = 0;					// line 850
static const int CONNECTOR_BOTTOM = 6;				// line 862: the chain's connector ends this far below the top of the screen
static const int FIRST_LINK_GAP = 30;				// line 863
static const int LINK_OFFSET_X = 0;					// lines 864, 865
static const int CONNECTOR_OFFSET_X = 0;
static const int FLAG_ALLOW_MOUSE_AND_FOCUS = 0x30;	// the bits of the widget flags a dialog sliding out gives up

// 0x497280
ThunderDialog::ThunderDialog(int theId, bool isModal, const std::string& theHeader, const std::string& theLines,
							 const std::string& theFooter, int theButtonMode)
	: Dialog(IMAGE_DLG_DIALOGBOX, IMAGE_DLG_BUTTON3, theId, isModal, theHeader, theLines, theFooter, theButtonMode)
{
	mClip = false;
	mContentInsets = Insets(CONTENT_INSET_LEFT, CONTENT_INSET_TOP, CONTENT_INSET_RIGHT, CONTENT_INSET_BOTTOM);
	SetHeaderFont(FONT_OVERLOAD24GRAD1); // (the original names the fonts by id: 0x2f, 0x31)
	SetLinesFont(FONT_PLAINOUTLINE);
	mButtonHeight = IMAGE_DLG_BUTTON3->mHeight / 3;

	// the framework's buttons are replaced by the game's, of the same label, id, listener and place
	for (DialogButton** aButtonPointer : {&mYesButton, &mNoButton})
	{
		DialogButton* aOld = *aButtonPointer;
		if (aOld == nullptr)
			continue;
		ThunderButton* aNew = MakeButton(aOld->mId, aOld->mButtonListener, aOld->mLabel, nullptr);
		aNew->Resize(aOld->mX, aOld->mY, aOld->mWidth, aOld->mHeight);
		delete aOld;
		SetThreeStateButton(aNew, false);
		*aButtonPointer = aNew;
	}

	SetColor(COLOR_BUTTON_TEXT, Color(TEXT_COLOR));
	SetColor(COLOR_BUTTON_TEXT_HILITE, Color(TEXT_COLOR));
	SetColor(COLOR_LINES, Color(TEXT_COLOR));
	SetColor(COLOR_HEADER, Color(HEADER_COLOR));

	mScrollTargetY = SCROLL_NONE;
	mScrollSpeedX = 0;
	mScrollDirection = -1;
	mScrollingOff = false;
	mKillWhenScrolledOff = false;
	mHangsOnChain = false;
	mFastScrollOff = true;
	mSpaceAfterHeader = SPACE_AFTER_HEADER;
	mChainSpacing = CHAIN_SPACING;
	mAlwaysDirty = false;
	mDrawHook = nullptr;
}

// 0x497550
ThunderDialog::~ThunderDialog()
{
}

// 0x497570
bool ThunderDialog::IsScrolling() const
{
	return mScrollTargetY != SCROLL_NONE;
}

// 0x497580
void ThunderDialog::DoScroll(int theDirection)
{
	gSexyAppBase->PlaySample(SOUND_DIALOG_MOVE);

	// it comes to rest where it was put, or at a default place when it was put off the screen
	int aRestY = mY;
	mScrollDirection = theDirection;
	if (aRestY < 0)
		aRestY = DEFAULT_REST_Y;
	mScrollTargetY = aRestY;
	if (mScrollDirection <= 0)
		mY = SCREEN_HEIGHT;
	else
		mY = -mHeight;

	mScrollingOff = false;
	mKillWhenScrolledOff = false;
	Resize(mX, mY, mWidth, mHeight);
	mWidgetFlagsMod.mRemoveFlags = 0;
	mMouseVisible = true;
}

// 0x497690
void ThunderDialog::DoScrollOff(bool theKillWhenDone)
{
	gSexyAppBase->PlaySample(SOUND_DIALOG_MOVE);
	mKillWhenScrolledOff = theKillWhenDone;
	mScrollDirection = -1;
	mScrollTargetY = -mHeight;
	mScrollingOff = true;
	gSexyAppBase->KillDialog(mId, false, false);
	mWidgetFlagsMod.mRemoveFlags |= FLAG_ALLOW_MOUSE_AND_FOCUS;
	mMouseVisible = false;
}

// 0x497700
void ThunderDialog::FinishScroll()
{
	if (mScrollTargetY != SCROLL_NONE)
		mY = mScrollTargetY;
}

// 0x497720
void ThunderDialog::CancelScroll()
{
	mWidgetFlagsMod.mRemoveFlags = 0;
	mMouseVisible = true;
	mScrollDirection = -1;
	mScrollTargetY = SCROLL_NONE;
	mScrollSpeedX = 0;
}

// 0x497750
int ThunderDialog::GetLeft() const
{
	return mContentInsets.mLeft + mBackgroundInsets.mLeft + mX - LEFT_EDGE_ADJUST;
}

// 0x497780 (the original takes the left insets for both edges)
int ThunderDialog::GetRight() const
{
	return mWidth - mContentInsets.mLeft - mBackgroundInsets.mLeft + mX;
}

// 0x4977a0
int ThunderDialog::GetTop() const
{
	return mHeaderFont->GetHeight() + (mSpaceAfterHeader + mContentInsets.mTop + mBackgroundInsets.mTop + mY - TOP_EDGE_ADJUST);
}

// 0x4977e0
int ThunderDialog::GetWidth() const
{
	return WIDTH_ADJUST + (mWidth - mContentInsets.mRight - mContentInsets.mLeft - mBackgroundInsets.mRight - mBackgroundInsets.mLeft);
}

// 0x497820
void ThunderDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);
	if (mDrawHook != nullptr)
		mDrawHook(this, g);

	if (mHangsOnChain)
	{
		// the chain: links up from the connector until they are above the screen
		Image* aConnector = IMAGE_DLG_CONNECTOR;
		Image* aLink = IMAGE_DLG_SEGMENT;
		int aConnectorY = CONNECTOR_BOTTOM - aConnector->mHeight;
		int aLinkY = aConnectorY - FIRST_LINK_GAP;
		while (mY + aLink->mHeight + aLinkY >= 0)
		{
			g->DrawImage(aLink, LINK_OFFSET_X + (mWidth - aLink->mWidth) / 2, aLinkY);
			aLinkY -= mChainSpacing;
		}
		g->DrawImage(aConnector, CONNECTOR_OFFSET_X + (mWidth - aConnector->mWidth) / 2, aConnectorY);
	}
}

// 0x49bf20
void ThunderDialog::Update()
{
	Widget::Update();

	if (mScrollTargetY != SCROLL_NONE)
	{
		// a slide covers a share of the way left each update, at least as fast as the minimum and no faster than the maximum
		int aDistance = mY - mScrollTargetY;
		if (aDistance < 0)
			aDistance = -aDistance;
		int aSpeed = aDistance / SCROLL_SPEED_DIVISOR;
		if (aSpeed < SCROLL_SPEED_MIN)
			aSpeed = SCROLL_SPEED_MIN;
		else if (aSpeed > SCROLL_SPEED_MAX)
			aSpeed = SCROLL_SPEED_MAX;
		if (mScrollingOff && mFastScrollOff)
			aSpeed = SCROLL_OFF_SPEED;

		int aNewY;
		if (mScrollDirection < 0)
		{
			aNewY = mY - aSpeed;
			if (aNewY <= mScrollTargetY)
			{
				aNewY = mScrollTargetY;
				mScrollTargetY = SCROLL_NONE;
			}
		}
		else
		{
			aNewY = mY + aSpeed;
			if (mScrollTargetY <= aNewY)
			{
				aNewY = mScrollTargetY;
				mScrollTargetY = SCROLL_NONE;
			}
		}
		Resize(mX + mScrollSpeedX, aNewY, mWidth, mHeight);

		if (mScrollTargetY == SCROLL_NONE && mScrollingOff)
		{
			// it is out of the screen: off the manager, and gone if told to
			mWidgetManager->RemoveWidget(this);
			if (mKillWhenScrolledOff)
				gSexyAppBase->SafeDeleteWidget(this);
		}
	}

	if (mAlwaysDirty)
		MarkDirty();
}

// 0x497910
void ThunderDialog::MouseDown(int x, int y, int theClickCount)
{
	if (mScrollTargetY == SCROLL_NONE)
		Widget::MouseDown(x, y, theClickCount);
}

// 0x497930
void ThunderDialog::KeyDown(KeyCode theKey)
{
	Widget::KeyDown(theKey);
}
