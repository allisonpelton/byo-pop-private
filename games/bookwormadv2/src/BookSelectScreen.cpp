#include "BookSelectScreen.h"

#include "CurtainTransition.h"
#include "FadeButton.h"
#include "Font.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "LuaBind.h"
#include "LuaSupport.h"
#include "Profile.h"
#include "Res.h"
#include "WidgetManager.h"

using namespace Sexy;
using namespace LuaLib;

BookSelectScreen::BookSelectScreen(GameApp* theApp)
	: mApp(theApp), mBeginButton(nullptr), mBackButton(nullptr), mOptionsButton(nullptr)
{
	RegisterLua();
	Resize(0, 0, mApp->mWidth, mApp->mHeight);
	mHeader = GT(0x6f8dbc);
	mText = GT(0x6f8dd0);

	// the three books of Volume 2: where the mouse picks one, its picture and where the picture goes (a few pixels
	// off the rectangle's corner, up by the picture's lean), and what shows for it
	Item aBook4;
	aBook4.mTitle = GT(0x6f8e14);
	aBook4.mText = GT(0x6f8e30);
	aBook4.mRect = Rect(205, 139, 350, 75);
	aBook4.mX = aBook4.mRect.mX + 8;
	aBook4.mY = aBook4.mRect.mY - 15;
	aBook4.mId = 4;
	aBook4.mImage = IMAGE_BS_BOOK4;
	mItems.push_back(aBook4);

	Item aBook5;
	aBook5.mTitle = GT(0x6f8e74);
	aBook5.mText = GT(0x6f8e88);
	aBook5.mRect = Rect(218, 221, 340, 72);
	aBook5.mX = aBook5.mRect.mX + 4;
	aBook5.mY = aBook5.mRect.mY - 10;
	aBook5.mId = 5;
	aBook5.mImage = IMAGE_BS_BOOK5;
	mItems.push_back(aBook5);

	Item aBook6;
	aBook6.mTitle = GT(0x6f8ed4);
	aBook6.mText = GT(0x6f8ee8);
	aBook6.mRect = Rect(208, 305, 353, 85);
	aBook6.mX = aBook6.mRect.mX + 8;
	aBook6.mY = aBook6.mRect.mY - 8;
	aBook6.mId = 6;
	aBook6.mImage = IMAGE_BS_BOOK6;
	mItems.push_back(aBook6);

	// the welcome message comes once (when the key is missing, it is still to come)
	mNeedsReplayDialog = true;
	if (mApp->mProfile->KeyExists("NeedsReplayDialog"))
		mNeedsReplayDialog = mApp->mProfile->GetBool("NeedsReplayDialog");
}

BookSelectScreen::~BookSelectScreen()
{
	RemoveAllWidgets(true, true);
}

// The original registered the class in the constructor (0x425630), every time
void BookSelectScreen::RegisterLua()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	Class<BookSelectScreen>("BookSelectScreen", false).Base<LuaWidget>();
}

void BookSelectScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	// back (the book page's back button's place), options, and begin under the page, which waits for a book
	mBackButton = new FadeButton(BUTTON_BACK, mApp);
	mBackButton->SetupImage(IMAGE_UI_BOOK_BACK_BUTTON);
	AddWidget(mBackButton);
	mBackButton->Move(36, 489);
	mBackButton->SetVisible(false);

	mOptionsButton = new FadeButton(BUTTON_OPTIONS, mApp);
	mOptionsButton->SetupImage(IMAGE_UI_BOOK_OPTIONS_BUTTON);
	AddWidget(mOptionsButton);
	mOptionsButton->Move(664, 489);
	mOptionsButton->SetVisible(false);

	mBeginButton = new FadeButton(BUTTON_BEGIN, mApp);
	mBeginButton->SetupImage(IMAGE_UI_BOOK_ENTER_BUTTON);
	AddWidget(mBeginButton);
	mBeginButton->SetDisabled(true);
	mBeginButton->Move(289, 555);
	mBeginButton->SetVisible(false);
}

// Shows a book's name and description in place of the ones shown, which fade out
void BookSelectScreen::Update()
{
	CallLinkedMethod("Update", 0);
	if (mNeedsReplayDialog) {
		mNeedsReplayDialog = false;
		mApp->mProfile->SetBool("NeedsReplayDialog", false);
		mApp->DoOkDialog(GT(0x6f9004), GT(0x6f8f60), "", nullptr);
	}
	// the buttons come when no message is up
	if (mApp->GetDialog(GameApp::DIALOG_OK) == nullptr) {
		mBackButton->SetVisible(true);
		mOptionsButton->SetVisible(true);
		mBeginButton->SetVisible(true);
	}

	bool aPointing = false;
	for (size_t i = 0; i < mItems.size(); i++) {
		Item& anItem = mItems[i];
		if (mMouseX >= anItem.mRect.mX && mMouseX < anItem.mRect.mWidth + anItem.mRect.mX &&
		    mMouseY >= anItem.mRect.mY && mMouseY < anItem.mRect.mHeight + anItem.mRect.mY) {
			if (mHover != anItem.mId) {
				mHover = anItem.mId;
				mPrevHeader = mHeader;
				mPrevText = mText;
				mPrevAlpha = mAlpha;
				mHeader = anItem.mTitle;
				mText = anItem.mText;
				mAlpha = 0;
				if (mHeader == mPrevHeader) {
					mPrevAlpha = 0;
					mAlpha = 255;
				}
			}
			aPointing = true;
		}
	}

	// the hand over a book and over the begin button
	if (aPointing || mBeginButton->mIsOver)
		mApp->SetCursor(CURSOR_HAND);
	else
		mApp->SetCursor(CURSOR_POINTER);

	if (!aPointing && mHover != 0) {
		// the mouse left the books: the book picked shows again, or the instruction when there is none
		bool aHadSelection = mSelected > 0;
		mHover = 0;
		if (!aHadSelection) {
			mPrevHeader = mHeader;
			mPrevText = mText;
			mPrevAlpha = mAlpha;
			mHeader = GT(0x6f8dbc);
			mText = GT(0x6f8dd0);
			mAlpha = 0;
		} else {
			for (size_t i = 0; i < mItems.size(); i++) {
				Item& anItem = mItems[i];
				if (mSelected == anItem.mId) {
					mPrevHeader = mHeader;
					mPrevText = mText;
					mPrevAlpha = mAlpha;
					mHeader = anItem.mTitle;
					mText = anItem.mText;
					mAlpha = 0;
					if (mHeader == mPrevHeader) {
						mPrevAlpha = 0;
						mAlpha = 255;
					}
				}
			}
		}
	}

	if (mPrevAlpha > 0) {
		mPrevAlpha -= 12;
		if (mPrevAlpha < 0)
			mPrevAlpha = 0;
	}
	if (mAlpha < 255) {
		mAlpha += 12;
		if (mAlpha > 255)
			mAlpha = 255;
	}
	MarkDirty();
}

void BookSelectScreen::Draw(Graphics* g)
{
	g->DrawImage(IMAGE_BS_BG, 400 - IMAGE_BS_BG->GetWidth() / 2, 10);
	LuaWidget::Draw(g);  // the Lua object's Draw: the badges
	// the page's frame
	g->DrawImage(IMAGE_UI_TOP, 0, 0);
	g->DrawImage(IMAGE_UI_LEFT, 0, IMAGE_UI_TOP->GetHeight());
	g->DrawImage(IMAGE_UI_RIGHT, mApp->mWidth - IMAGE_UI_RIGHT->GetWidth(), IMAGE_UI_TOP->GetHeight());
	g->DrawImage(IMAGE_UI_BOTTOM, 0, IMAGE_UI_LEFT->GetHeight() + IMAGE_UI_TOP->GetHeight());
	g->DrawImage(IMAGE_BANNER_SMALL, 400 - IMAGE_BANNER_SMALL->GetWidth() / 2, 5);
	g->DrawImage(IMAGE_BS_TITLE, 400 - IMAGE_BS_TITLE->GetWidth() / 2, 15);
	g->DrawImage(IMAGE_UI_DESCRIPTION_BOX, 400 - IMAGE_UI_DESCRIPTION_BOX->GetWidth() / 2, 430);

	// the books: the picture's cel is 2 for the one picked, else 1 for the one pointed at
	for (size_t i = 0; i < mItems.size(); i++) {
		Item& anItem = mItems[i];
		int aCel = 0;
		if (mSelected == anItem.mId)
			aCel = 2;
		else if (mHover == anItem.mId)
			aCel = 1;
		g->DrawImageCel(anItem.mImage, anItem.mX, anItem.mY, aCel);
	}

	// the name and the description
	if (mAlpha > 0) {
		g->PushState();
		g->SetColor(Color(0, 0, 0, mAlpha));
		g->SetFont(FONT_LARGE);
		g->DrawString(mHeader, 400 - FONT_LARGE->StringWidth(mHeader) / 2, 480);
		g->SetColor(Color(95, 63, 2, mAlpha));
		g->SetFont(FONT_MEDIUM);
		g->WriteWordWrapped(Rect(275, 490, 250, 60), mText, -1, 0, nullptr, -1, nullptr);
	}
	if (mPrevAlpha > 0) {
		g->PushState();
		g->SetColor(Color(0, 0, 0, mPrevAlpha));
		g->SetFont(FONT_LARGE);
		g->DrawString(mPrevHeader, 400 - FONT_LARGE->StringWidth(mPrevHeader) / 2, 480);
		g->SetColor(Color(95, 63, 2, mPrevAlpha));
		g->SetFont(FONT_MEDIUM);
		g->WriteWordWrapped(Rect(275, 490, 250, 60), mPrevText, -1, 0, nullptr, -1, nullptr);
	}
	// 0x424bdd: one pop for both texts, whether or not either was drawn: in a cross-fade the state of the first text
	// (FONT_MEDIUM, its colour) is what the dim fill and the scripts' DrawToolTips start from
	g->PopState();

	// a message up: the screen goes dim under it
	if (mApp->GetDialog(GameApp::DIALOG_OK) != nullptr) {
		g->SetColor(Color(0, 0, 0, 140));
		g->FillRect(0, 0, mApp->mWidth, mApp->mHeight);
	}
	PushGraphics(g);
	CallLinkedMethod("DrawToolTips", 1);
}

void BookSelectScreen::DrawOverlay(Graphics* g)
{
	g->SetColor(Color(150, 150, 150, 125));
	g->FillRect(0, 0, mApp->mWidth, mApp->mHeight);
}

void BookSelectScreen::KeyChar(SexyChar theChar)
{
	// 0x424ea0: no credits (the app's +0x8dc), and no Enter while a chapter loads (+0x94f, mLoadingChapter, and +0x878,
	// mChapterLoadScreen, both set)
	if (mApp->mCredits != nullptr || (mApp->mLoadingChapter && mApp->mChapterLoadScreen != nullptr))
		return;
	if (!mApp->IsCurtainMoving() && mSelected != 0 && theChar == 13)
		mApp->ButtonDepress(BUTTON_BEGIN);
}

void BookSelectScreen::MouseMove(int x, int y)
{
	if (!mApp->mActive || mApp->IsCurtainMoving())
		return;
	mMouseX = x;
	mMouseY = y;
	LuaWidget::MouseMove(x, y);  // the Lua object's: the badges' tooltips
}

void BookSelectScreen::MouseUp(int x, int y, int theClickCount)
{
	if (mHover <= 0)
		return;
	mSelected = mHover;
	mBeginButton->SetDisabled(false);
	for (size_t i = 0; i < mItems.size(); i++) {
		Item& anItem = mItems[i];
		if (mSelected == anItem.mId) {
			mPrevHeader = mHeader;
			mPrevText = mText;
			mPrevAlpha = 255;
			mHeader = anItem.mTitle;
			mText = anItem.mText;
			mAlpha = 0;
		}
	}
	mApp->PlaySample(SOUND_CLICK2);
}
