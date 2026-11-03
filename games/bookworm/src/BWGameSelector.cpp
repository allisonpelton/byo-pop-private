#include "BWGameSelector.h"
#include "BookWormApp.h"
#include "Res.h"
#include "BigLex.h"
#include "UserManager.h"
#include "NewUserDialog.h"
#include "GameText.h"
#include "ButtonWidget.h"
#include "HyperlinkWidget.h"
#include "EditWidget.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "DDImage.h"
#include "Image.h"

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif

using namespace Sexy;

// 0x401600
BWGameSelector::BWGameSelector(BookWormApp* theApp)
{
	mBubbleX = 0;
	mBubbleY = 0;
	mApp = theApp;
	mHasAlpha = true;

	mBigLex = new BigLex(theApp);
	mBigLex->Resize(70, 231, 133, 123);

	mBubbleX = 14;
	mGameMode = -1;
	mGameOption = -1;
	mBubbleY = 114;
	mClassicButton = NULL;
	mActionButton = NULL;
	mHallButton = NULL;
	mQuitButton = NULL;
	mNotYouLink = NULL;
	SetGameMode(mApp->mGameMode);
	SetGameOption(mApp->mGameOption);
	mUnused8C = 0; // port: initialised (never used)
	mUnused90 = 0.0;
	mBackdropY = 0;
	mClosing = false;
	mOpening = false;
	mSnapshot = NULL;
	// port: initialised (the original sets them only in the transitions, before Draw reads them)
	mBandHeight = 0;
	mStripY = 0;
}

// 0x401000
BWGameSelector::~BWGameSelector()
{
	delete mBigLex;
	delete mClassicButton;
	delete mActionButton;
	delete mHallButton;
	delete mQuitButton;
	delete mNotYouLink;
	mNotYouLink = NULL;
	delete mSnapshot;
	mSnapshot = NULL;
}

// 0x402430: the buttons and the link are top-level widgets above the selector, made on the first add
void BWGameSelector::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	if (mBigLex != NULL)
		mApp->mWidgetManager->AddWidget(mBigLex);

	// Image-only buttons (no label, no font): the over image appears at once and fades out (0.08 per update)
	struct ButtonDef
	{
		ButtonWidget**	mButton;
		int				mId;
		int				mImage;
		int				mOverImage;
		int				mX;
		int				mY;
	};
	const ButtonDef aButtonDefs[4] =
	{
		{ &mClassicButton, CLASSIC_BUTTON_ID, IMAGE_GAMESELECT_CLASSIC, IMAGE_GAMESELECT_CLASSIC_OVER, 360, 50 },
		{ &mActionButton, ACTION_BUTTON_ID, IMAGE_GAMESELECT_ACTION, IMAGE_GAMESELECT_ACTION_OVER, 340, 145 },
		{ &mHallButton, HALL_OF_FAME_BUTTON_ID, IMAGE_GAMESELECT_HALL, IMAGE_GAMESELECT_HALL_OVER, 376, 240 },
		{ &mQuitButton, QUIT_BUTTON_ID, IMAGE_GAMESELECT_QUIT, IMAGE_GAMESELECT_QUIT_OVER, 352, 350 }
	};

	for (int i = 0; i < 4; i++)
	{
		const ButtonDef& aDef = aButtonDefs[i];
		if (*aDef.mButton == NULL)
		{
			Image* anImage = mApp->mImages[aDef.mImage];
			int aWidth = anImage->GetWidth();
			int aHeight = anImage->GetHeight();
			ButtonWidget* aButton = new ButtonWidget(aDef.mId, this);
			*aDef.mButton = aButton;
			aButton->Resize(Rect(aDef.mX, aDef.mY, aWidth, aHeight));
			aButton->mDoFinger = true;
			aButton->mButtonImage = anImage;
			aButton->mOverImage = mApp->mImages[aDef.mOverImage];
			aButton->mOverAlphaSpeed = 0.08;
			aButton->mHasAlpha = true;
		}
		mApp->mWidgetManager->AddWidget(*aDef.mButton);
	}

	if (mNotYouLink == NULL)
	{
		mNotYouLink = new HyperlinkWidget(NOT_YOU_LINK_ID, this);
		mNotYouLink->mLabel = GameText(GT_SELECTOR_NOT_YOU);
		mNotYouLink->mDoFinger = true;
		mNotYouLink->mUnderlineSize = 1;
		mNotYouLink->SetVisible(false);
		ImageFont* aFont = mApp->mFonts[FONT_MYRIADWEB10];
		mNotYouLink->SetFont(aFont);
		mNotYouLink->mColor = Color(32, 255, 32);
		mNotYouLink->mOverColor = Color(216, 216, 0);

		// Centred in the bubble, three and a half lines above its bottom
		int aTextWidth = aFont->StringWidth(mNotYouLink->mLabel);
		Image* aBubble = mApp->mImages[IMAGE_GAMESELECT_BUBBLE];
		int aBubbleWidth = aBubble->GetWidth();
		double aBubbleBottom = (double)(aBubble->GetHeight() + mBubbleY);
		int aY = (int)(aBubbleBottom - aFont->GetHeight() * 3.5);
		mNotYouLink->Resize((aBubbleWidth - aTextWidth) / 2 + mBubbleX, aY, aTextWidth, aFont->GetHeight());
	}
	mApp->mWidgetManager->AddWidget(mNotYouLink);
}

// 0x401370
void BWGameSelector::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);

	if (mBigLex != NULL)
		mApp->mWidgetManager->RemoveWidget(mBigLex);
	if (mClassicButton != NULL)
		theWidgetManager->RemoveWidget(mClassicButton);
	if (mActionButton != NULL)
		theWidgetManager->RemoveWidget(mActionButton);
	if (mHallButton != NULL)
		theWidgetManager->RemoveWidget(mHallButton);
	if (mQuitButton != NULL)
		theWidgetManager->RemoveWidget(mQuitButton);
	if (mNotYouLink != NULL)
		theWidgetManager->RemoveWidget(mNotYouLink);
}

// 0x4013f0
void BWGameSelector::SetDisabled(bool isDisabled)
{
	Widget::SetDisabled(isDisabled);

	if (mClassicButton != NULL)
		mClassicButton->SetDisabled(isDisabled);
	if (mActionButton != NULL)
		mActionButton->SetDisabled(isDisabled);
	if (mHallButton != NULL)
		mHallButton->SetDisabled(isDisabled);
	if (mQuitButton != NULL)
		mQuitButton->SetDisabled(isDisabled);
	if (mNotYouLink != NULL)
		mNotYouLink->SetDisabled(isDisabled);
}

// 0x4334f0 (shared jump to Widget::Resize)
void BWGameSelector::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
}

// 0x401450 (shared with TitleScreen)
void BWGameSelector::ButtonPress(int theId, int theClickCount)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x4014c0
void BWGameSelector::ButtonDepress(int theId)
{
	switch (theId)
	{
	case NOT_YOU_LINK_ID:
		mApp->ShowSelectUserDialog();
		break;

	case CLASSIC_BUTTON_ID:
		SetGameMode(GAMEMODE_CLASSIC);
		mApp->SelectGameMode(GAMEMODE_CLASSIC);
		break;

	case ACTION_BUTTON_ID:
		SetGameMode(GAMEMODE_ACTION);
		mApp->SelectGameMode(GAMEMODE_ACTION);
		break;

	case HALL_OF_FAME_BUTTON_ID:
		mApp->ShowHallOfFame();
		break;

	case QUIT_BUTTON_ID:
		mApp->ShowQuitDialog();
		break;
	}
}

// 0x401460: called by BookWormApp::SelectGameMode (no saved game) and for the continue-game dialog's buttons
void BWGameSelector::StartGame()
{
	// Dead defaults: the app's fields start at 0 and 1
	if (mGameMode == -1)
		mGameMode = GAMEMODE_CLASSIC;
	if (mGameOption == -1)
		mGameOption = 1;

	mApp->mGameMode = mGameMode;
	mApp->mGameOption = mGameOption;
	mApp->StartGameForUser();
	SetDisabled(true);
}

// 0x4010c0: coming back from a game. The selector is drawn normally into a snapshot (the board was already
// removed from the manager), then Update unrolls it.
void BWGameSelector::StartOpenTransition()
{
	if (mOpening)
		return;

	delete mSnapshot;
	mSnapshot = new DDImage(mApp->mDDInterface);
	mSnapshot->Create(mWidth, mHeight);
	mSnapshot->SetImageMode(false, false);
	mNotYouLink->SetVisible(false);
	Graphics aSnapshotG(mSnapshot);
	mWidgetManager->DrawWidgetsTo(&aSnapshotG);
	mClassicButton->SetVisible(false);
	mActionButton->SetVisible(false);
	mHallButton->SetVisible(false);
	mQuitButton->SetVisible(false);
	mHasTransparencies = true;
	mHasAlpha = true;
	mBandHeight = 0;
	mStripY = -100;
	mOpening = true;
	// The big Lex stays visible above the squashed snapshot (NAMES.md D43).
}

// 0x4011f0: a game starts (the board is already added behind the selector); Update rolls it up and
// removes it
void BWGameSelector::StartCloseTransition()
{
	if (mClosing)
		return;

	if (mBigLex != NULL)
		mBigLex->SetVisible(false);

	delete mSnapshot;
	mSnapshot = new DDImage(mApp->mDDInterface);
	mSnapshot->Create(mWidth, mHeight);
	mSnapshot->SetImageMode(false, false);
	mNotYouLink->SetVisible(false);
	Graphics aSnapshotG(mSnapshot);
	mWidgetManager->DrawWidgetsTo(&aSnapshotG);
	mClassicButton->SetVisible(false);
	mActionButton->SetVisible(false);
	mHallButton->SetVisible(false);
	mQuitButton->SetVisible(false);
	mHasTransparencies = true;
	mHasAlpha = true;
	mMouseVisible = false;
	mBandHeight = TRANSITION_BAND_H;
	mStripY = 0;
	mClosing = true;
	mWidgetManager->BringToFront(this);
}

// 0x402190
void BWGameSelector::Update()
{
	Widget::Update();

	// Lex welcomes the player once per run
	if ((mUpdateCnt == 100) && !mApp->mWelcomePlayed)
	{
		mApp->mWelcomePlayed = true;
		mApp->PlaySample(SOUND_LEX_WELCOME);
		mBigLex->PlayAnim(0, false);
	}

	if (mClosing)
	{
		// Roll up 8 pixels per update while sliding up 2; then the app deletes the selector
		mWidgetManager->BringToFront(this);
		mStripY -= 2;
		if (mBandHeight > 0)
			mBandHeight -= 8;
		if (mBandHeight < 1)
		{
			mBandHeight = 0;
			mApp->RemoveGameSelector(); // SafeDeleteWidget: this stays valid until the frame ends
		}
		MarkDirty();
		return;
	}

	if (mOpening)
	{
		mStripY += 2;
		if (mStripY > 0)
		{
			// In place: show the link and buttons again and delete the finished board underneath
			mStripY = 0;
			mOpening = false;
			mNotYouLink->SetVisible(true);
			mClassicButton->SetVisible(true);
			mActionButton->SetVisible(true);
			mHallButton->SetVisible(true);
			mQuitButton->SetVisible(true);
			OrderInManagerChanged();
			mApp->RemoveBoard();
		}
		mBandHeight += 8;
		if (mBandHeight > TRANSITION_BAND_H)
			mBandHeight = TRANSITION_BAND_H;
		MarkDirty();
		return;
	}

	// No player yet: ask for a name (pre-filled with the Windows user name when it is only letters and spaces)
	if ((mApp->mUserManager->mUsers.size() == 0) || (mApp->mUserManager->GetCurrentUserName() == _S("")))
	{
		if (mApp->GetDialog(DIALOG_NEW_USER) == NULL)
		{
			mApp->ShowNewUserDialog(GameText(GT_SELECTOR_ENTER_NAME));
			NewUserDialog* aDialog = (NewUserDialog*)mApp->GetDialog(DIALOG_NEW_USER);
			if ((aDialog != NULL) &&
				(mApp->mUserName.find_first_not_of(" AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz") == std::string::npos))
			{
				// port: the 2004 EditWidget::SetText leaves mLeftPos alone
				aDialog->mNameEdit->SetText(mApp->mUserName, false);
			}
#ifdef __EMSCRIPTEN__
			// the web page's player (framework/WebProfiles.h): under "fill" and "create" the box starts with it, in place of the above
			if (aDialog != NULL)
				SiteFillNewUser(aDialog->mNameEdit);
#endif
		}
	}

	// The change-player link shows while any player exists
	bool haveUsers = mApp->mUserManager->mUsers.size() != 0;
	if (mNotYouLink->mVisible != haveUsers)
	{
		mNotYouLink->SetVisible(haveUsers);
		MarkDirty();
	}
}

// 0x402cb0
void BWGameSelector::Draw(Graphics* g)
{
	if (!mClosing && !mOpening)
	{
		g->DrawImage(mApp->mImages[IMAGE_BACKDROP_MENU], 0, mBackdropY);
		DrawBubble(g);
		return;
	}

	// The snapshot as a top strip at mStripY, the middle band squashed to mBandHeight and the bottom strip
	// below it (like a scroll rolling up or down); what is not covered keeps the previous screen contents or
	// shows the board below.
	int aStripY = mStripY;
	g->SetFastStretch(true);
	g->DrawImage(mSnapshot, Rect(0, aStripY, mWidth, TRANSITION_TOP_H), Rect(0, 0, mWidth, TRANSITION_TOP_H));
	g->DrawImage(mSnapshot, Rect(0, aStripY + TRANSITION_TOP_H, mWidth, mBandHeight),
		Rect(0, TRANSITION_TOP_H, mWidth, TRANSITION_BAND_H));
	g->DrawImage(mSnapshot, Rect(0, mBandHeight + TRANSITION_TOP_H + aStripY, mWidth, TRANSITION_BOTTOM_H),
		Rect(0, TRANSITION_BOTTOM_SRC_Y, mWidth, TRANSITION_BOTTOM_H));
	g->SetFastStretch(false);
}

// 0x402a50: the speech bubble greeting the current player (by the profile's key name, not the alias)
void BWGameSelector::DrawBubble(Graphics* g)
{
	if (mOpening || mClosing)
		return;

	Image* aBubble = mApp->mImages[IMAGE_GAMESELECT_BUBBLE];
	int aBubbleWidth = aBubble->GetWidth();
	SexyString aName = mApp->mUserManager->GetCurrentUserName();
	if (aName.length() == 0)
		return;

	g->DrawImage(aBubble, mBubbleX, mBubbleY);

	// A player who never played (TimesPlayed 0) gets the first-time greeting
	SexyString aGreeting;
	ImageFont* aFont;
	if (mApp->mUserTimesPlayed == 0)
	{
		aGreeting = GameText(GT_SELECTOR_WELCOME_NEW);
		aFont = mApp->mFonts[FONT_VERDANA12BOLD];
	}
	else
	{
		aGreeting = GameText(GT_SELECTOR_WELCOME_BACK);
		aFont = mApp->mFonts[FONT_VERDANA14BOLD];
	}
	int aX = (aBubbleWidth - aFont->StringWidth(aGreeting)) / 2 + mBubbleX;
	int aY = mBubbleY - (int)(aFont->GetHeight() * -2.25);
	g->SetFont(aFont);
	g->SetColor(Color(255, 255, 255, 255));
	g->DrawString(aGreeting, aX, aY);

	SexyString aLine = aName + _S("!");
	aFont = mApp->mFonts[FONT_VERDANA14BOLD];
	int aLineHeight = aFont->GetHeight();
	aX = (aBubbleWidth - aFont->StringWidth(aLine)) / 2 + mBubbleX;
	g->SetFont(aFont);
	g->SetColorizeImages(true); // no effect on text
	g->SetColor(Color(255, 255, 255, 255));
	g->DrawString(aLine, aX, aY + aLineHeight);
	g->SetColorizeImages(false);
}
