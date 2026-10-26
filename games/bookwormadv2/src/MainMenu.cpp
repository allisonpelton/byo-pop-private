#include "MainMenu.h"

#include <SDL.h>

#include "BookManager.h"
#include "CurtainTransition.h"
#include "Font.h"
#include "GameApp.h"
#include "GameDialog.h"
#include "GameText.h"
#include "Graphics.h"
#include "HyperlinkWidget.h"
#include "JeffLib.h"
#include "MemoryImage.h"
#include "PopLua.h"
#include "Profile.h"
#include "Res.h"
#include "ResourceManager.h"
#include "SoundInstance.h"
#include "SoundPlayer.h"
#include "UserSelectionDialog.h"
#include "WidgetManager.h"
#include "lua.h"

using namespace Sexy;

namespace {

const char* const kIdleLabel = "idle";

// the secret words (and the names of the animations Lex does for them)
const unsigned int kSecretWords[] = {GTA(0x7ae700), GTA(0x7ae6f0), GTA(0x7ae6e8), GTA(0x7ae6e0),
                                     GTA(0x7ae6d4), GTA(0x7ae6cc), GTA(0x7ae6c4)};
const int kNumSecretWords = (int)(sizeof(kSecretWords) / sizeof(kSecretWords[0]));

// The things of the menu, by Item: where the lit picture goes, where the mouse is over it (and how much smaller
// than the picture that is), from the constructor's tables
struct ItemPlace {
	int mX, mY;
	int mHitX, mHitY;
	int mHitDX, mHitDY;
};
const ItemPlace kPlaces[MainMenu::NUM_ITEMS] = {
	{26, 477, 26, 477, 0, 0},       // options
	{628, 478, 628, 487, 0, -9},    // quit
	{133, 228, 150, 228, -32, -20}, // adventure
	{484, 228, 484, 228, 0, -25},   // arena
	{581, 356, 581, 366, 0, -10},   // hall of fame
	{45, 313, 45, 313, -34, 0},     // mini-games
	{320, 445, 320, 485, 0, 0},     // tome
	{490, 492, 490, 492, 0, 0},     // film reel
};

bool InRect(const Rect& theRect, int x, int y)
{
	return x >= theRect.mX && x < theRect.mX + theRect.mWidth && y >= theRect.mY && y < theRect.mY + theRect.mHeight;
}

bool ProfileBool(GameApp* theApp, const char* theKey)
{
	return theApp->mProfile != nullptr && theApp->mProfile->KeyExists(theKey) && theApp->mProfile->GetBool(theKey);
}

}  // namespace

// ---- Tooltip ----

MainMenu::Tooltip::Tooltip(Image* theTitle, const std::string& theText, int theTitleY)
{
	mY = 0;
	mBaseY = 0;
	mClosedBottomY = mBottomRollY = IMAGE_MM_SCROLL->mHeight - 12;
	mBottomY = IMAGE_MM_SCROLLBACK->mHeight + 13;
	mHeight = 0;
	mX = 0;
	mTextX = 26;
	mTitleY = theTitleY;
	mShowText = false;
	int aWidth = IMAGE_MM_SCROLLBACK->GetCelWidth();
	int aHeight = IMAGE_MM_SCROLLBACK->GetCelHeight();
	mImage = new MemoryImage(gApp);
	mImage->Create(aWidth, aHeight);
	{
		Graphics g(mImage);
		g.SetColor(Color(255, 255, 255, 0));
		g.FillRect(0, 0, aWidth, aHeight);
		g.DrawImage(IMAGE_MM_SCROLLBACK, 0, 0);
		g.DrawImage(theTitle, (aWidth - theTitle->GetCelWidth()) / 2, mTitleY + 21);
		g.SetColor(Color::White);
		g.SetFont(FONT_SMALL);
		int aTextHeight = g.GetWordWrappedHeight(191, theText, -1);
		g.WriteWordWrapped(Rect(12, (44 - aTextHeight) / 2 + 52, 191, 400), theText, -1, 0);
	}
	mFullHeight = aHeight;
	mState = STATE_HIDDEN;
}

MainMenu::Tooltip::~Tooltip()
{
	delete mImage;
}

void MainMenu::Tooltip::SetPosition(int theX, int theY)
{
	int aScrollWidth = IMAGE_MM_SCROLL->mWidth;
	int x = theX - aScrollWidth / 2;
	if (x < 0)
		x = 0;
	if (x + aScrollWidth > gApp->mWidth)
		x = gApp->mWidth - aScrollWidth;
	int y = theY;
	if (y < 0)
		y = 0;
	if (y + 7 + IMAGE_MM_SCROLL->mHeight + mImage->mHeight > gApp->mHeight)
		y = gApp->mHeight - IMAGE_MM_SCROLL->mHeight - mImage->mHeight - 7;
	int aDelta = y - mY;
	mBaseY += aDelta;
	mClosedBottomY += aDelta;
	mBottomRollY += aDelta;
	mBottomY += aDelta;
	mX = x;
	mY += aDelta;
}

void MainMenu::Tooltip::Update()
{
	int aCount = gApp->mUpdateCount;
	if (mState == STATE_OPENING) {
		if (mBottomRollY < mBottomY) {
			mBottomRollY += 3;
			if (mBottomRollY > mBottomY)
				mBottomRollY = mBottomY;
		}
		if (mHeight < mFullHeight) {
			mHeight += 3;
			if (mHeight > mFullHeight)
				mHeight = mFullHeight;
		}
		if (mBottomRollY == mBottomY && mHeight == mFullHeight)
			mState = STATE_OPEN;
		if (mY < mBaseY + 10 && aCount % 3 == 0)
			mY++;
	} else if (mState == STATE_CLOSING) {
		if (mHeight > 0) {
			mHeight -= 3;
			if (mHeight < 0)
				mHeight = 0;
		}
		mBottomRollY = mClosedBottomY + mHeight;
		if (mHeight < 1) {
			mState = STATE_HIDDEN;
			mShowText = false;
		}
		if (mY > mBaseY && aCount % 3 == 0)
			mY--;
	}
}

void MainMenu::Tooltip::Draw(Graphics* g)
{
	if (mState == STATE_HIDDEN)
		return;
	if (mShowText && mHeight > 0)
		g->DrawImage(mImage, mX + mTextX, mY + 10, Rect(0, 0, mImage->GetCelWidth(), mHeight));
	g->DrawImage(IMAGE_MM_SCROLL, mX, mBottomRollY);
	g->DrawImage(IMAGE_MM_SCROLL, mX, mY);
}

// ---- MenuItem ----

bool MainMenu::MenuItem::HitTest(int theX, int theY) const
{
	int x = theX - mRect.mX;
	int y = theY - mRect.mY;
	if (x < 0 || y < 0 || x > mWidth || y > mHeight)
		return false;
	if (mBits == nullptr)
		return true;
	// port: the original's bounds let x and y be the width and height, reading past the picture at its corner
	int anIndex = y * mWidth + x;
	if (anIndex >= mWidth * mHeight)
		return false;
	return (mBits[anIndex] & 0xff000000) != 0;
}

// ---- MainMenu ----

MainMenu::MainMenu(GameApp* theApp, bool theFromTitle)
{
	mApp = theApp;
	Resize(0, 0, mApp->mWidth, mApp->mHeight);
	mLogoX = (mWidth - IMAGE_MM_LOGO->mWidth) / 2;
	mLogoY = theFromTitle ? (mHeight - IMAGE_MM_LOGO->mHeight) / 2 - 40 : 0;
	mLogoTimerDelta = -1;
	mLogoTimer = theFromTitle ? 255 : 0;
	mFromTitle = theFromTitle;
	mReallyQuit = false;
	mLex = new PopAnim(-2, this);
	if (!mLex->LoadAnimation("images/pam/mmlex.pam"))
		SDL_Log("MainMenu: images/pam/mmlex.pam: %s", mLex->GetError().c_str());

	Profile* aProfile = mApp->mProfile;
	mAdventureAvailable = aProfile == nullptr || !aProfile->KeyExists("CurrentBook") ||
	                      aProfile->GetInt("CurrentBook") <= mApp->mTotalBooks;
	mArenaUnlocked = ProfileBool(mApp, "UnlockedArena");
	mMiniGamesUnlocked = ProfileBool(mApp, "UnlockedMiniGames");
	mTomeUnlocked = ProfileBool(mApp, "UnlockedBestiary");
	mExpansionAvailable = mAdventureAvailable && mApp->mTotalBooks > 3;

	Image* const anOverImages[NUM_ITEMS] = {IMAGE_MM_OPTIONS_OVER, IMAGE_MM_QUIT_OVER, IMAGE_MM_ADVENTURE_OVER,
	                                        IMAGE_MM_ARENA_OVER, IMAGE_MM_HS_OVER, IMAGE_MM_MINI_OVER,
	                                        IMAGE_MM_TOME_OVER, IMAGE_MM_FILM_REEL_OVER};
	Image* const aNormalImages[NUM_ITEMS] = {IMAGE_MM_OPTIONS, IMAGE_MM_QUITGAME, nullptr, nullptr,
	                                         IMAGE_MM_HIGHSCORES, nullptr, nullptr, nullptr};
	for (int i = 0; i < NUM_ITEMS; i++) {
		MenuItem* anItem = new MenuItem();
		Image* anOver = anOverImages[i];
		const ItemPlace& aPlace = kPlaces[i];
		anItem->mOverImage = anOver;
		if (i == ITEM_FILM_REEL) {
			anItem->mReelImage = IMAGE_FILM_REEL;
			anItem->mReelDownImage = IMAGE_MM_FILM_REEL_DOWN;
			anItem->mVisible = aProfile != nullptr && aProfile->KeyExists("MediaPlayer");
		}
		anItem->mRect = Rect(aPlace.mX, aPlace.mY, anOver->mWidth, anOver->mHeight);
		anItem->mHitRect = Rect(aPlace.mHitX, aPlace.mHitY, anOver->mWidth + aPlace.mHitDX,
		                        anOver->mHeight + aPlace.mHitDY);
		anItem->mSrcRect = Rect(0, 0, anOver->mWidth, anOver->mHeight);
		anItem->mNormalImage = aNormalImages[i];
		if (MemoryImage* aMemoryImage = dynamic_cast<MemoryImage*>(anOver))
			anItem->mBits = aMemoryImage->GetBits();
		anItem->mWidth = anOver->mWidth;
		anItem->mHeight = anOver->mHeight;
		mHoverUpdate[i] = -200;
		mItems.push_back(anItem);
	}
	MakeTooltips(GT(0x7ae8c8));

	// "(Click here to change users)" under the name
	mChangeUserLink = new HyperlinkWidget(1, this);
	mChangeUserLink->mUnderlineSize = 1;
	mChangeUserLink->SetFont(FONT_MEDIUM);
	mChangeUserLink->mColor = Color(200, 0, 0);
	mChangeUserLink->mOverColor = Color(255, 205, 90);
	mChangeUserLink->mLabel = GT(0x7ae9a0);
	int aLinkWidth = FONT_MEDIUM->StringWidth(mChangeUserLink->mLabel) + 10;
	mChangeUserLink->Resize(279 + (245 - aLinkWidth) / 2, 198, aLinkWidth, FONT_MEDIUM->GetHeight());
	AddWidget(mChangeUserLink);

	// Lex's words: a sample and his animation's label for each thing
	mReactions[ITEM_ADVENTURE] = {SOUND_MM_ADVENTURE, "ADVENTURE"};
	mReactions[ITEM_ARENA] = {SOUND_MM_ARENA, "ARENA"};
	mReactions[ITEM_HALL_OF_FAME] = {SOUND_MM_HS, "HALLOFFAME"};
	mReactions[ITEM_MINI_GAMES] = {SOUND_MM_MINIGAMES, "MINIGAMES"};
	mReactions[ITEM_OPTIONS] = {SOUND_MM_OPTIONS, "OPTIONS"};
	mReactions[ITEM_QUIT] = {SOUND_MM_QUIT, "QUIT"};
	mReactions[ITEM_TOME] = {SOUND_MM_TOK, "TOME"};
	mReactions[ITEM_FILM_REEL] = {SOUND_MM_CNG, "CNG"};
	mReactions[REACTION_WELCOME] = {SOUND_WELCOME, "WELCOME"};

	mLex->PlayLabel(kIdleLabel);
}

MainMenu::~MainMenu()
{
	delete mLex;
	mLex = nullptr;
	// the cached samples of Lex's words go
	JeffLib::SoundPlayer* aSoundPlayer = mApp->mSoundPlayer;
	for (auto& aReaction : mReactions) {
		int anId = aReaction.second.mSound;
		if (anId >= 0 && anId < JeffLib::SoundPlayer::MAX_SAMPLES && aSoundPlayer->mCached[anId] != nullptr) {
			aSoundPlayer->mCached[anId]->Release();
			aSoundPlayer->mCached[anId] = nullptr;
		}
	}
	RemoveAllWidgets(true, true);
	for (MenuItem* anItem : mItems) {
		delete anItem->mTooltip;
		delete anItem;
	}
}

void MainMenu::MakeTooltips(const std::string& theTomeText)
{
	for (int i = ITEM_ADVENTURE; i < NUM_ITEMS; i++) {
		MenuItem* anItem = mItems[i];
		delete anItem->mTooltip;
		anItem->mTooltip = nullptr;
		switch (i) {
		case ITEM_ADVENTURE:
			anItem->mPulse = mAdventureAvailable ? 1 : 0;
			// "Help Lex solve the mystery ...", or that adventure mode is completed
			anItem->mTooltip = new Tooltip(IMAGE_MM_ADVENTURE, mAdventureAvailable ? GT(0x7ae740) : GT(0x7ae774), 0);
			break;
		case ITEM_ARENA:
			anItem->mTooltip = new Tooltip(IMAGE_MM_ARENA, mArenaUnlocked ? GT(0x7ae7b8) : GT(0x7ae804), 0);
			break;
		case ITEM_MINI_GAMES:
			anItem->mTooltip = new Tooltip(IMAGE_MM_MINIGAMES, mMiniGamesUnlocked ? GT(0x7ae840) : GT(0x7ae88c), 0);
			break;
		case ITEM_TOME:
			anItem->mTooltip = new Tooltip(IMAGE_MM_TOME_TEXT, mTomeUnlocked ? theTomeText : GT(0x7ae910), -12);
			break;
		case ITEM_FILM_REEL:
			if (mApp->mProfile != nullptr && mApp->mProfile->KeyExists("MediaPlayer"))
				anItem->mTooltip = new Tooltip(IMAGE_MM_CNG_TITLE, GT(0x7ae958), -22);
			break;
		}
	}
}

void MainMenu::RefreshUnlocks()
{
	mMiniGamesUnlocked = ProfileBool(mApp, "UnlockedMiniGames");
	mArenaUnlocked = ProfileBool(mApp, "UnlockedArena");
	mTomeUnlocked = ProfileBool(mApp, "UnlockedBestiary");
	Profile* aProfile = mApp->mProfile;
	mAdventureAvailable = aProfile == nullptr || !aProfile->KeyExists("CurrentBook") ||
	                      aProfile->GetInt("CurrentBook") <= mApp->mTotalBooks;
	mItems[ITEM_FILM_REEL]->mVisible = aProfile != nullptr && aProfile->KeyExists("MediaPlayer");
	mExpansionAvailable = mAdventureAvailable && mApp->mTotalBooks > 3;
	// (the tome's text is not the constructor's here, as in the original)
	MakeTooltips(GT(0x7aea10));
}

void MainMenu::PlayReaction(int theItem)
{
	if (mApp->mSfxVolume <= 0.0 || mLex == nullptr || SDL_strcasecmp(mLex->mLabel.c_str(), kIdleLabel) != 0)
		return;
	if (theItem == -1)
		theItem = REACTION_WELCOME;
	else if (theItem != REACTION_WELCOME && mApp->IsCurtainMoving())
		return;
	JeffLib::SoundPlayer* aSoundPlayer = mApp->mSoundPlayer;
	if (aSoundPlayer->IsCachedSamplePlaying(SOUND_WELCOME))
		return;
	for (auto& aReaction : mReactions)
		if (aSoundPlayer->IsCachedSamplePlaying(aReaction.second.mSound))
			return;
	auto anIt = mReactions.find(theItem);
	if (anIt == mReactions.end())
		return;
	aSoundPlayer->PlayCachedSample(anIt->second.mSound, 0, 0);
	if (!anIt->second.mLabel.empty())
		mLex->PlayLabel(anIt->second.mLabel);
}

void MainMenu::CloseTooltips()
{
	for (MenuItem* anItem : mItems)
		if (anItem->mTooltip != nullptr && anItem->mTooltip->mState != Tooltip::STATE_HIDDEN)
			anItem->mTooltip->mState = Tooltip::STATE_CLOSING;
}

void MainMenu::Update()
{
	Widget::Update();
	if (!mApp->mVSyncUpdates)
		UpdateF(1.0f);
	if (mLogoY > 0) {
		mLogoY -= 6;
		if (mLogoY < 0)
			mLogoY = 0;  // (the original then set a flag of the widget's when the app was inactive)
	} else if (mLogoTimer > 0) {
		mLogoTimer += mLogoTimerDelta;
		if (mLogoTimer < 0)
			mLogoTimer = 0;
	}
	for (MenuItem* anItem : mItems) {
		if (anItem->mTooltip != nullptr)
			anItem->mTooltip->Update();
		int aDelta = anItem->mFadeDelta;
		if (aDelta != 0) {
			anItem->mFade += aDelta;
			if ((aDelta < 0 && anItem->mFade <= anItem->mFadeTarget) ||
			    (aDelta > 0 && anItem->mFade >= anItem->mFadeTarget)) {
				anItem->mFade = anItem->mFadeTarget;
				anItem->mFadeDelta = 0;
			}
		} else if (anItem->mPulse != 0) {
			// glows up to 200 and down to 0, 4 an update
			anItem->mFade += anItem->mPulse * 4;
			if (anItem->mPulse < 0 && anItem->mFade < 1) {
				anItem->mFade = 0;
				anItem->mPulse = -anItem->mPulse;
			} else if (anItem->mPulse > 0 && anItem->mFade > 199) {
				anItem->mFade = 200;
				anItem->mPulse = -anItem->mPulse;
			}
		}
	}
	MarkDirty();
}

void MainMenu::UpdateF(float theFrac)
{
	if (mApp->IsCurtainMoving())
		return;
	mLex->UpdateF(theFrac);
}

void MainMenu::Draw(Graphics* g)
{
	g->DrawImage(IMAGE_MM_BG, 0, 0);
	g->SetFont(FONT_MEDIUM);
	g->SetColor(Color(106, 37, 0));
	std::string aName = mApp->mProfile != nullptr ? mApp->mProfile->mName : "";
	g->WriteString(aName, 306, g->GetFont()->GetAscent() + 180, 189, 0);  // centred on the plaque
	g->Translate(290, 270);
	mLex->Draw(g);
	g->Translate(-290, -270);
	g->DrawImage(IMAGE_MM_TOME, 320, 445);

	for (int i = 0; i < (int)mItems.size(); i++) {
		MenuItem* anItem = mItems[i];
		if (!anItem->mVisible)
			continue;
		if (anItem->mReelImage != nullptr)
			g->DrawImage(anItem->mReelImage, anItem->mRect, anItem->mSrcRect);
		if (anItem->mFade > 0) {
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, anItem->mFade));
			g->DrawImage(anItem->mOverImage, anItem->mRect, anItem->mSrcRect);
			if (Image* aNormal = anItem->mNormalImage) {
				g->SetColor(Color(255, 255, 255, std::min(anItem->mFade * 2, 255)));
				g->DrawImage(aNormal, (anItem->mRect.mWidth - aNormal->mWidth) / 2 + anItem->mRect.mX,
				             (anItem->mRect.mHeight - aNormal->mHeight) / 2 + 15 + anItem->mRect.mY);
			}
			g->SetColorizeImages(false);
		}
		if (i == ITEM_ADVENTURE) {
			// the adventure completed, or an expansion's books: a badge on the door. (The original's x subtracts
			// the door's height from the badge's width, or the other way round: kept.)
			Image* aBadge = !mAdventureAvailable ? IMAGE_MM_ADV_COMPLETE : mExpansionAvailable ? IMAGE_MM_EXP_PACK_AVAILABLE
			                                                                                   : nullptr;
			if (aBadge != nullptr) {
				int aBadgeWidth = aBadge->mWidth, aBadgeHeight = aBadge->mHeight;
				int aDoorWidth = anItem->mOverImage->mWidth, aDoorHeight = anItem->mOverImage->mHeight;
				int dx = aBadgeWidth > aDoorWidth ? aBadgeWidth - aDoorHeight : aDoorHeight - aBadgeWidth;
				int dy = aBadgeHeight > aDoorHeight ? aBadgeHeight - aDoorHeight : aDoorHeight - aBadgeHeight;
				g->DrawImage(aBadge, anItem->mRect.mX + dx / 2 + (mAdventureAvailable ? 15 : -21),
				             anItem->mRect.mY + dy / 2);
			}
		}
	}
	if (!mMiniGamesUnlocked)
		g->DrawImage(IMAGE_MM_LOCKED_LEFT, 90, 380);
	if (!mArenaUnlocked)
		g->DrawImage(IMAGE_MM_LOCKED_RIGHT, 503, 278);
	if (!mTomeUnlocked)
		g->DrawImage(IMAGE_MM_LOCKED_LEFT, 380, 521);
	if (!mFromTitle && !mReallyQuit)
		g->DrawImage(IMAGE_MM_LOGO, mLogoX, mLogoY);
	else
		DeferOverlay(1);  // over the curtain, or under the question whether to quit
	for (MenuItem* anItem : mItems)
		if (anItem->mTooltip != nullptr)
			anItem->mTooltip->Draw(g);
}

void MainMenu::DrawOverlay(Graphics* g)
{
	g->DrawImage(IMAGE_MM_LOGO, mLogoX, mLogoY);
	if (mReallyQuit) {
		g->SetColor(Color(0, 0, 0, 175));
		g->FillRect(0, 0, mWidth, mHeight);
		g->DrawImage(IMAGE_MM_REALLY_QUIT_WEEPY, (mWidth - IMAGE_MM_REALLY_QUIT_WEEPY->mWidth) / 2, 30);
	}
}

void MainMenu::KeyChar(SexyChar theChar)
{
	// port: the original also wanted no hall of fame (the app's +0x89c), nothing at the app's +0x8a0, +0x82c
	// and +0x948, no conversation panel and no cheat console (none of them is ported yet), and the full game's
	// marketing mode, which the port always is
	if (mApp->mHallOfFame != nullptr || mDisabled || SDL_strcasecmp(mLex->mLabel.c_str(), kIdleLabel) != 0 ||
	    mApp->GetDialogCount() != 0 || mApp->mBookScreen != nullptr)
		return;
	mTypedKeys += (char)theChar;
	mTypedKeys = StringToUpper(mTypedKeys);
	// the secret words: one more letter of one keeps them, a word makes Lex do something
	int aFound = -1;
	bool aPrefix = false;
	for (int i = 0; i < kNumSecretWords && aFound == -1; i++) {
		const std::string& aWord = GT(kSecretWords[i]);
		size_t aLength = std::min(aWord.size(), mTypedKeys.size());
		if (mTypedKeys.compare(0, aLength, aWord, 0, aLength) == 0) {
			aPrefix = true;
			if (mTypedKeys.size() == aWord.size())
				aFound = i;
		}
	}
	if (!aPrefix) {
		mTypedKeys.clear();
		return;
	}
	if (aFound == -1)
		return;
	mTypedKeys.clear();
	static const char* const kLabels[] = {"seattle", nullptr, "lovely", "gobble", "celebrate", "pirate", "winter"};
	if (aFound == 1) {
		// the cheat script's: the player gets the media player (the film reel)
		if (mApp->mLua->PushPath("_DefaultMP")) {
			lua_pop(mApp->mLua->L, 1);
			if (mApp->mLua->Call("_DefaultMP", 0, 1))
				lua_pop(mApp->mLua->L, 1);
		}
		RefreshUnlocks();
		return;
	}
	mLex->PlayLabel(kLabels[aFound]);
}

void MainMenu::AppLostFocus()
{
	// 0x47d940: redrawn; Lex idles, unless he is at one of the secret words' animations (the labels are the words)
	MarkDirtyFull();
	bool aFound = false;
	const unsigned int* aWord = kSecretWords;
	do {
		if (aFound)
			return;
		if (SDL_strcasecmp(mLex->mLabel.c_str(), GT(*aWord).c_str()) == 0)
			aFound = true;
		aWord++;
	} while (aWord < kSecretWords + kNumSecretWords);
	if (!aFound)
		mLex->PlayLabel(kIdleLabel);
}

void MainMenu::MouseMove(int x, int y)
{
	if (!mApp->mActive || mApp->IsCurtainMoving())
		return;
	bool aFound = false;
	for (int i = 0; i < (int)mItems.size(); i++) {
		MenuItem* anItem = mItems[i];
		if (anItem->mDisabled || !anItem->mVisible)
			continue;
		bool aHit = InRect(anItem->mHitRect, x, y) && anItem->HitTest(x, y);
		if (aFound && aHit)
			continue;
		if (!aHit) {
			anItem->mHovered = false;
			if (anItem->mFade < 1) {
				if (anItem->mPulse != 0)
					anItem->mFadeDelta = 0;
			} else if (anItem->mPulse == 0) {
				anItem->mFadeDelta = -3;
				anItem->mFadeTarget = 0;
			} else {
				anItem->mFadeDelta = 0;
			}
			if (anItem->mTooltip != nullptr && anItem->mTooltip->mState != Tooltip::STATE_HIDDEN)
				anItem->mTooltip->mState = Tooltip::STATE_CLOSING;
			continue;
		}
		if (!anItem->mHovered && mApp->mHallOfFame == nullptr && mApp->mCurtain == nullptr) {
			mHoverUpdate[i] = mUpdateCnt;
			PlayReaction(i);
		}
		aFound = true;
		mApp->SetCursor(CURSOR_HAND);
		if (anItem->mPulse == 0) {
			anItem->mFadeDelta = 3;
			anItem->mFadeTarget = 128;
		}
		anItem->mHovered = true;
		if (Tooltip* aTooltip = anItem->mTooltip) {
			if (aTooltip->mState != Tooltip::STATE_OPEN) {
				aTooltip->mState = Tooltip::STATE_OPENING;
				aTooltip->mShowText = true;
			}
			aTooltip->SetPosition(x, y);
		}
	}
	if (!aFound && mApp->GetCursor() == CURSOR_HAND)
		mApp->SetCursor(CURSOR_POINTER);
}

void MainMenu::MouseDown(int x, int y, int theClickCount)
{
	if (mApp->mBookScreen != nullptr)
		return;
	bool aFound = false;
	for (MenuItem* anItem : mItems) {
		if (anItem->mDisabled || !anItem->mVisible)
			continue;
		anItem->mPressed = false;
		if (aFound || !InRect(anItem->mHitRect, x, y) || !anItem->HitTest(x, y)) {
			if (anItem->mFade > 0 && anItem->mPulse == 0) {
				anItem->mFadeTarget = 0;
				anItem->mFadeDelta = -3;
			}
			continue;
		}
		anItem->mFadeTarget = 255;
		anItem->mFadeDelta = 3;
		aFound = true;
		anItem->mPressed = true;
		mApp->PlaySample(SOUND_CLICK2);
	}
}

// The curtain's callback of the hall of fame (0x450760 is the app's function that the original called with the app)
static void OpenHallOfFameCallback(void* theApp)
{
	static_cast<GameApp*>(theApp)->OpenHallOfFame();
}

// The curtain's callback of the mini-games' menu (0x44f9d0, the app's function that the original called with the app)
static void ShowMiniGameMenuCallback(void* theApp)
{
	static_cast<GameApp*>(theApp)->ShowMiniGameMenu();
}

void MainMenu::MouseUp(int x, int y, int theClickCount)
{
	if (mApp->mBookScreen != nullptr || mApp->IsCurtainMoving() || mApp->GetDialogCount() != 0)
		return;
	bool aDisableAll = false;
	bool aFound = false;
	for (int i = 0; i < (int)mItems.size(); i++) {
		MenuItem* anItem = mItems[i];
		if (anItem->mDisabled || !anItem->mVisible)
			continue;
		if (!InRect(anItem->mHitRect, x, y) || !anItem->HitTest(x, y)) {
			if (anItem->mFade > 0 && anItem->mPulse == 0) {
				anItem->mFadeTarget = 0;
				anItem->mFadeDelta = -3;
			}
			if (anItem->mTooltip != nullptr && anItem->mTooltip->mState != Tooltip::STATE_HIDDEN)
				anItem->mTooltip->mState = Tooltip::STATE_CLOSING;
			anItem->mPressed = false;
			continue;
		}
		if (aFound) {
			anItem->mPressed = false;
			continue;
		}
		aFound = true;
		mApp->SetCursor(CURSOR_HAND);
		if (anItem->mPressed) switch (i) {
		case ITEM_OPTIONS:
			mFromTitle = false;
			mApp->ShowOptionsDialog();
			break;
		case ITEM_QUIT:
			AskReallyQuit();
			break;
		case ITEM_ADVENTURE:
			// port: the trial's end (0x451c80, its dialog) is left out
			if (!mAdventureAvailable) {
				mApp->DoOkDialog(GT(0x7aebf8), GT(0x7aeb50), "", IMAGE_DIALOG_BANNER_HOWTOPLAY);
				break;
			}
			mFromTitle = false;
			if (LuaLib::CallLua("main.NeedsIntroCutscene", 0).Bool()) {
				SetDisabled(true);
				mApp->PlayMainMenuCutScene();
			} else if (!mApp->mProfile->GetBool("InMiniGame") && !mApp->mProfile->GetBool("InGame")) {
				SetDisabled(true);
				mApp->StartNewGameTransition();
			} else {
				// a game in progress: the book under the menu, then the scripts' StartGame once the curtain is closed
				SetDisabled(true);
				mApp->InitGameScreen(false);
				mApp->mWidgetManager->BringToFront(this);
				lua_pushboolean(mApp->mLua->L, 1);
				mApp->mBookManager->CallLinkedMethod("InitSavedGame", 1);
				mApp->StartSavedGameTransition();
			}
			break;
		case ITEM_ARENA:
			// 0x47bc2e: only once the arena is unlocked and the player is in the second book or the first's sixth
			// chapter
			if (mApp->mProfile->KeyExists("UnlockedArena") && mApp->mProfile->GetBool("UnlockedArena")) {
				if (mApp->mProfile->GetInt("CurrentBook") > 1 || mApp->mProfile->GetInt("CurrentChapter") >= 6) {
					SetDisabled(true);
					StartArena();
				}
				mApp->SetCursor(CURSOR_POINTER);
			}
			break;
		case ITEM_HALL_OF_FAME:
			// 0x47be8f: the menu goes dim, the whole curtain closes, and the hall of fame comes (GameApp::OpenHallOfFame)
			SetDisabled(true);
			mApp->DoFullCurtainTransition("");
			mApp->mCurtainCallback = OpenHallOfFameCallback;
			mApp->mCurtainCallbackObject = mApp;
			mApp->SetCursor(CURSOR_POINTER);
			break;
		case ITEM_MINI_GAMES:
			// 0x47b9cf: only once the mini-games are unlocked: the menu goes dim, the whole curtain closes, and the
			// mini-games' menu comes (GameApp::ShowMiniGameMenu)
			if (mApp->mProfile->KeyExists("UnlockedMiniGames") && mApp->mProfile->GetBool("UnlockedMiniGames")) {
				SetDisabled(true);
				mApp->DoFullCurtainTransition("");
				mApp->mCurtainCallback = ShowMiniGameMenuCallback;
				mApp->mCurtainCallbackObject = mApp;
				mFromTitle = false;
			}
			mApp->SetCursor(CURSOR_POINTER);
			break;
		case ITEM_TOME:
			// 0x47bd24: only once the tome is unlocked: the whole curtain closes over the menu, then the tome comes
			// (the original wrote out what AddStatsBook does for it)
			if (mApp->mProfile != nullptr && mApp->mProfile->KeyExists("UnlockedBestiary") &&
			    mApp->mProfile->GetBool("UnlockedBestiary")) {
				mApp->AddStatsBook(false, true, false);
				CloseTooltips();
			}
			mApp->SetCursor(CURSOR_POINTER);
			break;
		case ITEM_FILM_REEL:
			// 0x47bef8: the media player, the same way
			mApp->AddStatsBook(false, false, true);
			CloseTooltips();
			mApp->SetCursor(CURSOR_POINTER);
			break;
		}
		anItem->mFadeTarget = 128;
		anItem->mFadeDelta = 3;
		anItem->mPressed = false;
	}
	if (aDisableAll)
		for (MenuItem* anItem : mItems)
			anItem->mDisabled = true;
	if (!aFound && mApp->GetCursor() == CURSOR_HAND)
		mApp->SetCursor(CURSOR_POINTER);
}

void MainMenu::AskReallyQuit()
{
	mApp->PlaySample(SOUND_MM_DONTLEAVE);
	mApp->SetCursor(CURSOR_POINTER);
	mApp->DoConfirmDialog(GameApp::DIALOG_REALLY_QUIT, _S(""), GT(0x7aee60), nullptr, "");
	GameDialog* aDialog = static_cast<GameDialog*>(mApp->GetDialog(GameApp::DIALOG_REALLY_QUIT));
	aDialog->SetBanner(IMAGE_MM_REALLY_QUIT_BANNER, 0);
	aDialog->mDialogHeader = _S(" ");
	aDialog->Resize(200, 305, aDialog->mWidth + 75, aDialog->mHeight + 20);
	aDialog->SetButtonText(GT(0x7aeea0), GT(0x7aee98));
	aDialog->mPriority = 10;
	aDialog->SetAllowDrag(false);
	MarkDirty();
	mReallyQuit = true;
}

void MainMenu::StartArena()
{
	// 0x47c290
	lua_pushboolean(mApp->mLua->L, 1);
	mApp->mLua->Call("main.SetTimedMode", 1, 0);
	mApp->mTimedMode = true;
	mFromTitle = false;
	if (!mApp->mProfile->GetBool("TM_InGame")) {
		mApp->mProfile->ResetTimedMode();
		mApp->StartNewGameTransition();
	} else {
		// the question without DoConfirmDialog's banner, and with its header's words
		mApp->DoConfirmDialog(GameApp::DIALOG_CONTINUE_GAME, GT(0x7aed7c), GT(0x7aed20), nullptr, "");
		GameDialog* aDialog = static_cast<GameDialog*>(mApp->GetDialog(GameApp::DIALOG_CONTINUE_GAME));
		aDialog->SetBanner(nullptr, 0);
		aDialog->mDialogHeader = GT(0x7aed7c);
	}
	for (int i = 0; i < (int)mItems.size(); i++)
		mItems[i]->mDisabled = true;
}

void MainMenu::ContinueArena(bool theContinue)
{
	// 0x47c170
	if (theContinue) {
		mApp->InitGameScreen(true);
		BookManager* aBook = mApp->mBookManager;
		mApp->mWidgetManager->BringToFront(this);
		if (!(mApp->mProfile->KeyExists("TM_OnCheckpoint") && mApp->mProfile->GetBool("TM_OnCheckpoint"))) {
			lua_pushboolean(mApp->mLua->L, 1);
			aBook->CallLinkedMethod("InitSavedGame", 1);
			mApp->StartSavedGameTransition();
			return;
		}
	} else {
		mApp->mProfile->ResetTimedMode();
	}
	mApp->StartNewGameTransition();
}

void MainMenu::ButtonDepress(int theId)
{
	if (theId != mChangeUserLink->mId || mApp->IsCurtainMoving())
		return;
	ShowUserDialog();
}

void MainMenu::ShowUserDialog()
{
	// 0x47c770
	JeffLib::UserSelectionDialog::Resources aResources;
	aResources.mDialogImage = IMAGE_DIALOG;
	aResources.mButtonImage = IMAGE_DIALOG_BUTTON;
	aResources.mClickSound = SOUND_CLICK1;
	aResources.mFont = FONT_MEDIUM;
	aResources.mHeader = GT(0x7c5a20);
	aResources.mNewLabel = GT(0x7c5a30);
	aResources.mDeleteLabel = GT(0x7c5a34);
	mUserDialog = new JeffLib::UserSelectionDialog(aResources);
	mUserDialog->mMaxProfiles = mApp->GetInteger("MaxProfiles", 10);
	mUserDialog->mSpaceAfterHeader += 35;
	mUserDialog->mPriority = 2;
	mUserDialog->mContentInsets.mTop += 5;
	mUserDialog->mAllowDrag = false;
	mUserDialog->mScrollbar->mTrackColor = Color(89, 70, 14);
	mUserDialog->SetButtonFont(FONT_LARGE_OUTLINE);
	mUserDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(255, 255, 255));
	mUserDialog->SetColor(Dialog::COLOR_BUTTON_TEXT_HILITE, Color(255, 255, 0));
	mUserDialog->SetHeaderFont(FONT_LARGE_OUTLINE);
	mUserDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255));
	int aHeight = mUserDialog->GetPreferredHeight(400);
	mUserDialog->Resize((mWidth - 400) / 2, (mHeight - (aHeight + 40)) / 2, 400, aHeight + 40);

	// the list and its scrollbar a little taller than the dialog made them
	ListWidget* aList = mUserDialog->mList;
	int aListHeight = aList->mHeight;
	aList->Resize(aList->mX, aList->mY, aList->mWidth, aListHeight + 40);
	JeffLib::SexyScrollbarWidget* aScrollbar = mUserDialog->mScrollbar;
	aScrollbar->ResizeScrollbar(aScrollbar->mX, aScrollbar->mY, aScrollbar->mWidth, aListHeight + 40);

	mUserDialog->mDialogListener = this;
	mApp->AddDialog(mUserDialog);
	mWidgetManager->SetFocus(mUserDialog);
	FillUserList();
}

void MainMenu::FillUserList()
{
	// 0x47ca20
	mUserDialog->ClearUsers();
	// the players' files: users/<name>.bwa
	std::vector<std::string> aFiles = JeffLib::GetFilesInDirectory("users/*.bwa");
	size_t i = 0;
	while (true) {
		bool aMore;
		if (aFiles.empty() || i >= aFiles.size())
			aMore = false;
		else
			aMore = (int)i < mApp->GetInteger("MaxProfiles", 10);
		if (!aMore)
			break;
		std::string aName = aFiles[i].substr(0, aFiles[i].find(".bwa"));
		mUserDialog->AddUser(aName);
		i++;
	}
	// the player now playing is first, and chosen
	if (mApp->mProfile != nullptr) {
		std::string aName = mApp->mProfile->mName;
		mUserDialog->mCurrentUser = aName;
		mUserDialog->RefreshList();
		mUserDialog->SelectUser(aName);
	}
}

bool MainMenu::IsUserNameFree(const std::string& theName)
{
	// 0x47d200: the names are compared in capitals
	if (theName.empty())
		return false;
	std::string aName = StringToUpper(theName);
	int aCount = mUserDialog->GetUserCount();
	if (aCount <= 0)
		return true;
	int i = 0;
	do {
		bool aTaken = StringToUpper(mUserDialog->GetUser(i)) == aName;
		if (aTaken)
			return false;
		i++;
	} while (i < aCount);
	return true;
}

bool MainMenu::TakeNewUser(const std::string& theName)
{
	// 0x47d390
	if (!theName.empty()) {
		if (!IsUserNameFree(theName)) {
			mApp->DoOkDialog(GT(0x7a9c04), StrFormat(GT(0x7aee10).c_str(), theName.c_str()), "", nullptr);
			return false;
		}
		FillUserList();
		mApp->KillDialog(GameApp::DIALOG_USER_SELECTION);
		// (the original has 0x47d510 here in line)
		StartIntroIfNew();
		mApp->ClearUpdateBacklog(false);
	}
	return true;
}

bool MainMenu::StartIntroIfNew()
{
	// 0x47d510
	if (!mApp->HasPlayedGameBefore()) {
		mFromTitle = false;
		mApp->StartIntro(1);
		return true;
	}
	return false;
}

void MainMenu::DialogButtonDepress(int theDialogId, int theButtonId)
{
	// 0x47cd70
	if (theDialogId == GameApp::DIALOG_USER_SELECTION) {
		JeffLib::UserSelectionDialog* aDialog = mUserDialog;
		if (theButtonId == aDialog->mYesButton->mId || theButtonId == Dialog::ID_YES) {
			// OK: the chosen player's profile, if it loads
			if (!mApp->SelectUser(aDialog->mSelected))
				return;
			RefreshUnlocks();
			mApp->KillDialog(GameApp::DIALOG_USER_SELECTION);
			mUserDialog = nullptr;
			mApp->ShowUnlockMessages();
			StartIntroIfNew();
			return;
		}
		if (theButtonId != aDialog->mNoButton->mId && theButtonId != Dialog::ID_NO) {
			if (theButtonId == aDialog->mNewButton->mId) {
				// a new player's name: with others to go back to, the name dialog has CANCEL
				mApp->ShowNewUserDialog(aDialog->GetUserCount() != 0 ? Dialog::BUTTONS_OK_CANCEL
				                                                     : Dialog::BUTTONS_FOOTER);
				return;
			}
			if (theButtonId != aDialog->mDeleteButton->mId)
				return;
			// DELETE: asks first; the answer comes back here, as the confirmation's dialog 18
			mApp->DoConfirmDialog(-1, GT(0x7aee00), StrFormat(GT(0x7aed98).c_str(), aDialog->mSelected.c_str()),
			                      nullptr, "");
			mApp->GetDialog(GameApp::DIALOG_CONFIRM)->mDialogListener = this;
			return;
		}
		// CANCEL
		mApp->KillDialog(GameApp::DIALOG_USER_SELECTION);
		mUserDialog = nullptr;
		StartIntroIfNew();
		return;
	}
	if (theDialogId != GameApp::DIALOG_CONFIRM)
		return;
	if (theButtonId != Dialog::ID_YES) {
		mApp->KillDialog(GameApp::DIALOG_CONFIRM);
		return;
	}
	// yes: the chosen player's profile file goes (and the profile too when it is the one now playing)
	bool aIsCurrent = false;
	bool aDeletedCurrent = false;
	if (mApp->mProfile != nullptr) {
		std::string aCurrentName = mApp->mProfile->mName;
		aIsCurrent = mUserDialog->mSelected == aCurrentName;
	}
	if (aIsCurrent) {
		delete mApp->mProfile;
		mApp->mProfile = nullptr;
		mApp->RegistryEraseValue("LastUser");
		aDeletedCurrent = true;
	}
	mApp->EraseFile(GetAppDataFolder() + StrFormat("users/%s.bwa", mUserDialog->mSelected.c_str()));
	FillUserList();
	mApp->KillDialog(GameApp::DIALOG_CONFIRM);
	if (mUserDialog->GetUserCount() == 0) {
		// no one left: a new player's name
		mApp->ShowNewUserDialog(Dialog::BUTTONS_FOOTER);
		return;
	}
	if (aDeletedCurrent) {
		// the first of the others plays
		mApp->SelectUser(mUserDialog->GetUser(0));
		mUserDialog->SelectUser(mUserDialog->GetUser(0));
	}
}

void MainMenu::PopAnimStopped(int theId)
{
	mLex->PlayLabel(kIdleLabel);
}

void MainMenu::PopAnimCommand(int theId, const std::string& theCommand, const std::string& theParam)
{
	if (theId != mLex->mId || SDL_strcasecmp(theCommand.c_str(), "playsound") != 0)
		return;
	int aSound = mApp->mResourceManager->GetSound(theParam);
	if (aSound >= 0)
		mApp->PlaySample(aSound);
}
