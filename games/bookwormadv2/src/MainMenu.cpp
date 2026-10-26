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
const unsigned int kSecretWords[] = {GTA(0x6fc908), GTA(0x6fc8fc), GTA(0x6fc8f4), GTA(0x6fc8ec),
                                     GTA(0x6fc8e0), GTA(0x6fc8d8), GTA(0x6fc8d0)};
const int kNumSecretWords = (int)(sizeof(kSecretWords) / sizeof(kSecretWords[0]));

// The words that can be typed on the menu (0x46deb0, in the order of its switch) and what each does: the animation Lex
// plays for it (none for some), the text the scripts keep it by (main.SecretUnlocked) and the hat it puts on Lex
// (main.SetHatMode; 0 for none). ELITE, the eighth, switches the 1337 tiles instead.
struct TypedWord {
	unsigned int mWord;
	const char* mLabel;
	unsigned int mText;
	int mHat;
};
const TypedWord kTypedWords[] = {
    {GTA(0x6fc908), "seattle", GTA(0x6fd0fc), 5},   {GTA(0x6fc8fc), nullptr, GTA(0x6fd148), 4},
    {GTA(0x6fc8f4), "lovely", GTA(0x6fd17c), 0},    {GTA(0x6fc8ec), "gobble", GTA(0x6fd1ac), 0},
    {GTA(0x6fc8e0), "celebrate", GTA(0x6fd1e0), 6}, {GTA(0x6fc8d8), "pirate", GTA(0x6fd214), 0},
    {GTA(0x6fc8d0), "winter", GTA(0x6fd244), 0},    {GTA(0x6f8634), nullptr, GTA(0x6fd26c), 0},
    {GTA(0x6fc8c4), nullptr, GTA(0x6fd29c), 7},     {GTA(0x6fc8bc), nullptr, GTA(0x6fd2c8), 3}};
const int kNumTypedWords = (int)(sizeof(kTypedWords) / sizeof(kTypedWords[0]));
const int kEliteWord = 7;

// The things of the menu, by Item: where the picture is drawn and where the mouse is over it, from the
// constructor's tables (Volume 2 0x466420). The tome is centred on x = 420. The adventure, the arena and the
// mini-games are animations: the mouse is over them within kPopRects (their pictures' size is also the hit
// rectangle's).
struct ItemPlace {
	int mX, mY;
	int mHitX, mHitY;
};
const ItemPlace kPlaces[MainMenu::NUM_ITEMS] = {
	{60, 476, 60, 476},    // options
	{657, 439, 657, 439},  // quit
	{133, 228, 150, 228},  // adventure
	{484, 228, 484, 228},  // arena
	{616, 344, 616, 344},  // hall of fame
	{45, 313, 45, 313},    // mini-games
	{0, 490, 0, 490},      // tome
};
const Rect kPopRects[MainMenu::NUM_ITEMS] = {
	Rect(), Rect(), Rect(202, 226, 121, 208), Rect(492, 252, 119, 154), Rect(), Rect(71, 264, 105, 207), Rect(),
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
	if (!mHasMask)
		return true;
	int x = theX - mRect.mX;
	int y = theY - mRect.mY;
	if (mWidth < x || mHeight < y || x < 0 || y < 0)
		return false;
	if (mBits == nullptr)
		return true;
	// port: the original's bounds let x and y be the width and height, reading past the picture at its corner
	int anIndex = y * mWidth + x;
	if (anIndex >= mWidth * mHeight)
		return false;
	return mBits[anIndex] == 0xffffffffu;
}

// 0x46c950 (every update): the mask's pixels are asked again
void MainMenu::MenuItem::UpdateMask()
{
	if (!mHasMask)
		return;
	if (MemoryImage* aMemoryImage = dynamic_cast<MemoryImage*>(mMask))
		mBits = aMemoryImage->GetBits();
	mWidth = mMask->GetCelWidth();
	mHeight = mMask->GetCelHeight();
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
	// the tome opens with the fifth book: the constructor looks at nothing else (RefreshUnlocks also wants the
	// profile's UnlockedBestiary)
	mTomeUnlocked = aProfile != nullptr && aProfile->KeyExists("CurrentBook") && aProfile->GetInt("CurrentBook") > 4;
	mExpansionAvailable = mAdventureAvailable && mApp->mTotalBooks > 3;

	// the pictures (two cels: the second is lit); the three animations' stand for their size
	Image* const anImages[NUM_ITEMS] = {IMAGE_MM_OPTIONS,    IMAGE_MM_QUITGAME,    IMAGE_MM_ADVENTURE_OVER, IMAGE_MM_ARENA_OVER,
	                                    IMAGE_MM_HIGHSCORES, IMAGE_MM_MINI_OVER, IMAGE_MM_TOME};
	Image* const aMasks[NUM_ITEMS] = {nullptr, IMAGE_MM_QUITGAME_MASK, nullptr, nullptr, IMAGE_MM_HIGHSCORES_MASK, nullptr, nullptr};
	for (int i = 0; i < NUM_ITEMS; i++) {
		MenuItem* anItem = new MenuItem();
		Image* anImage = anImages[i];
		const ItemPlace& aPlace = kPlaces[i];
		int aWidth = anImage->GetCelWidth();
		int aHeight = anImage->mHeight;
		int anX = aPlace.mX;
		int aHitX = aPlace.mHitX;
		if (i == ITEM_TOME)
			anX = aHitX = 420 - aWidth / 2;
		anItem->mImage = anImage;
		anItem->mPopRect = kPopRects[i];
		anItem->mRect = Rect(anX, aPlace.mY, aWidth, aHeight);
		anItem->mSrcRect = Rect(0, 0, aWidth, aHeight);
		anItem->mHitRect = Rect(aHitX, aPlace.mHitY, aWidth, aHeight);
		anItem->mMask = aMasks[i];
		anItem->mHasMask = aMasks[i] != nullptr;
		anItem->UpdateMask();
		mHoverUpdate[i] = -200;
		mItems.push_back(anItem);
	}
	LoadPopAnims(true);
	MakeTooltips(GT(0x6fcb40));

	// "(Click here to change users)" under the name
	mChangeUserLink = new HyperlinkWidget(1, this);
	mChangeUserLink->mUnderlineSize = 1;
	mChangeUserLink->SetFont(FONT_MEDIUM);
	mChangeUserLink->mColor = Color(200, 0, 0);
	mChangeUserLink->mOverColor = Color(255, 205, 90);
	mChangeUserLink->mLabel = GT(0x6fcbcc);
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
		delete anItem->mPopAnim;
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
		case ITEM_ADVENTURE: {
			anItem->mPulse = mAdventureAvailable ? 1 : 0;
			// "Help Lex solve the mystery ...", or that adventure mode is completed; the replay of a completed one
			// says to go for a high score
			std::string aText = mAdventureAvailable ? GT(0x6fc97c) : GT(0x6fc9b0);
			if (ProfileBool(mApp, "AdventureComplete"))
				aText = GT(0x6fc9f0);
			anItem->mTooltip = new Tooltip(IMAGE_MM_ADVENTURE, aText, 0);
			break;
		}
		case ITEM_ARENA:
			anItem->mTooltip = new Tooltip(IMAGE_MM_ARENA, mArenaUnlocked ? GT(0x6fca30) : GT(0x6fca7c), 0);
			break;
		case ITEM_MINI_GAMES:
			anItem->mTooltip = new Tooltip(IMAGE_MM_MINIGAMES, mMiniGamesUnlocked ? GT(0x6fcab8) : GT(0x6fcb04), 0);
			break;
		case ITEM_TOME:
			// (the profile's key, not the flag: that one wants the fifth book)
			anItem->mTooltip = new Tooltip(IMAGE_MM_TOME_TEXT, ProfileBool(mApp, "UnlockedBestiary") ? theTomeText : GT(0x6fcb88), -12);
			break;
		}
	}
}

// The label of an animation for the book now played: up to the fourth book "book4", then "book5" and "book6"
static std::string BookLabel(int theBook, const char* theSuffix)
{
	return std::string(theBook < 5 ? "book4" : theBook == 5 ? "book5" : "book6") + theSuffix;
}

// 0x46a7ee (the mouse over the adventure), 0x46ada8 (off it), 0x469a0x (an animation done): the adventure's label,
// theSuffix " idle" or " mouseover". Once the adventure is completed the book is the replayed one, and without one
// the animation idles on the fourth book's.
void MainMenu::PlayAdventureLabel(const char* theSuffix)
{
	PopAnim* aPopAnim = mItems[ITEM_ADVENTURE]->mPopAnim;
	if (aPopAnim == nullptr)
		return;
	Profile* aProfile = mApp->mProfile;
	int aBook = 0;
	if (aProfile != nullptr) {
		if (!ProfileBool(mApp, "AdventureComplete"))
			aBook = aProfile->GetInt("CurrentBook");
		else if (aProfile->KeyExists("ReplayCurrentBook"))
			aBook = aProfile->GetInt("ReplayCurrentBook");
		else {
			aPopAnim->PlayLabel("book4 idle");
			return;
		}
	}
	aPopAnim->PlayLabel(BookLabel(aBook, theSuffix));
}

// The three animations (images/pam): the end of the constructor, and of RefreshUnlocks (0x468f23), which makes
// the adventure's again: the replay's once the adventure is completed
void MainMenu::LoadPopAnims(bool theFromConstructor)
{
	struct Entry {
		int mItem;
		const char* mFile;
	};
	const bool aComplete = ProfileBool(mApp, "AdventureComplete");
	const Entry anEntries[] = {{ITEM_ADVENTURE, aComplete ? "images/pam/mm_adventurereplay.pam" : "images/pam/mm_adventure.pam"},
	                           {ITEM_ARENA, "images/pam/mm_arena.pam"},
	                           {ITEM_MINI_GAMES, "images/pam/mm_minigames.pam"}};
	for (const Entry& anEntry : anEntries) {
		MenuItem* anItem = mItems[anEntry.mItem];
		if (anItem->mPopAnim != nullptr && anEntry.mItem != ITEM_ADVENTURE)
			continue;
		delete anItem->mPopAnim;
		anItem->mPopAnim = new PopAnim(anEntry.mItem, this);
		if (!anItem->mPopAnim->LoadAnimation(anEntry.mFile))
			SDL_Log("MainMenu: %s: %s", anEntry.mFile, anItem->mPopAnim->GetError().c_str());
	}
	Profile* aProfile = mApp->mProfile;
	if (theFromConstructor) {
		// the constructor looks at CurrentBook only, completed adventure or not
		int aBook = 0;
		if (aProfile != nullptr)
			aBook = aProfile->GetInt("CurrentBook");
		mItems[ITEM_ADVENTURE]->mPopAnim->PlayLabel(BookLabel(aBook, " idle"));
	} else {
		PlayAdventureLabel(" idle");
	}
	// the arena and the mini-games: the unlocking if the dialog about it is still to come, else locked or at rest
	const char* const anItemKeys[2] = {"NeedsArenaDialog", "NeedsMiniGamesDialog"};
	const bool anUnlocked[2] = {mArenaUnlocked, mMiniGamesUnlocked};
	const int anItems[2] = {ITEM_ARENA, ITEM_MINI_GAMES};
	for (int i = 0; i < 2; i++) {
		PopAnim* aPopAnim = mItems[anItems[i]]->mPopAnim;
		if (ProfileBool(mApp, anItemKeys[i]))
			aPopAnim->PlayLabel("unlocked");
		else
			aPopAnim->PlayLabel(anUnlocked[i] ? kIdleLabel : "locked");
	}
}

void MainMenu::RefreshUnlocks()
{
	mMiniGamesUnlocked = ProfileBool(mApp, "UnlockedMiniGames");
	mArenaUnlocked = ProfileBool(mApp, "UnlockedArena");
	Profile* aProfile = mApp->mProfile;
	// (here the tome wants its key, too, and the fifth book)
	mTomeUnlocked = aProfile != nullptr && aProfile->KeyExists("UnlockedBestiary") && aProfile->GetInt("CurrentBook") >= 5;
	if (aProfile != nullptr)
		mAdventureAvailable = !aProfile->KeyExists("CurrentBook") || aProfile->GetInt("CurrentBook") <= mApp->mTotalBooks;
	mExpansionAvailable = mAdventureAvailable && mApp->mTotalBooks > 3;
	// (the tome's text is not the constructor's here, as in the original)
	MakeTooltips(GT(0x6fcc70));
	LoadPopAnims(false);
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
		anItem->UpdateMask();
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
	for (MenuItem* anItem : mItems)
		if (anItem->mPopAnim != nullptr)
			anItem->mPopAnim->UpdateF(theFrac);
}

void MainMenu::Draw(Graphics* g)
{
	g->DrawImage(IMAGE_MM_BG, 0, 0);
	g->SetFont(FONT_MEDIUM);
	g->SetColor(Color(106, 37, 0));
	std::string aName = mApp->mProfile != nullptr ? mApp->mProfile->mName : "";
	g->WriteString(aName, 306, g->GetFont()->GetAscent() + 180, 189, 0);  // centred on the plaque
	// (nothing draws the scrolls of the tooltips here: Volume 2 makes and moves them, but has no Tooltip::Draw)
	for (MenuItem* anItem : mItems)
		if (anItem->mVisible && anItem->mPopAnim != nullptr)
			anItem->mPopAnim->Draw(g);
	g->Translate(290, 270);
	mLex->Draw(g);
	g->Translate(-290, -270);

	// the pictures, the second cel when the mouse is over them (the tome's only once it is open)
	for (int i = 0; i < (int)mItems.size(); i++) {
		MenuItem* anItem = mItems[i];
		if (!anItem->mVisible || anItem->mPopAnim != nullptr)
			continue;
		int aCel = anItem->mHovered ? 1 : 0;
		if (i == ITEM_TOME && !mTomeUnlocked)
			aCel = 0;
		g->DrawImageCel(anItem->mImage, anItem->mRect, aCel);
	}
	if (!mFromTitle && !mReallyQuit)
		g->DrawImage(IMAGE_MM_LOGO, mLogoX, mLogoY);
	else
		DeferOverlay(1);  // over the curtain, or under the question whether to quit
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
	// the slash shows the arena's and the mini-games' "unlocked" animations (no guard, as the original had none)
	if (theChar == '/') {
		for (int anItem : {(int)ITEM_ARENA, (int)ITEM_MINI_GAMES})
			if (mItems[anItem]->mPopAnim != nullptr)
				mItems[anItem]->mPopAnim->PlayLabel("unlocked");
	}
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
	for (int i = 0; i < kNumTypedWords && aFound == -1; i++) {
		const std::string& aWord = GT(kTypedWords[i].mWord);
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
	const TypedWord& aWord = kTypedWords[aFound];
	if (aWord.mLabel != nullptr)
		mLex->PlayLabel(aWord.mLabel);
	if (aFound == kEliteWord)
		mApp->mLeetMode = !mApp->mLeetMode;
	lua_pushstring(mApp->mLua->L, GT(aWord.mText).c_str());
	LuaLib::CallLua("main.SecretUnlocked", 1);
	if (aFound == kEliteWord) {
		// the 1337 tiles, on or off
		lua_pushboolean(mApp->mLua->L, mApp->mLeetMode ? 1 : 0);
		LuaLib::CallLua("main.SetLeetMode", 1);
		return;
	}
	if (aWord.mHat != 0) {
		lua_pushint(mApp->mLua->L, aWord.mHat);
		LuaLib::CallLua("main.SetHatMode", 1);
	}
	// (the original calls RefreshUnlocks after the second word only: it changes nothing here)
	if (aFound == 1)
		RefreshUnlocks();
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
	bool aHand = false;  // the hand cursor is up (a thing that is locked does not take it)
	for (int i = 0; i < (int)mItems.size(); i++) {
		MenuItem* anItem = mItems[i];
		if (anItem->mDisabled || !anItem->mVisible)
			continue;
		bool aHit = InRect(anItem->mHitRect, x, y) && anItem->HitTest(x, y);
		PopAnim* aPopAnim = anItem->mPopAnim;
		if (aPopAnim != nullptr)
			aHit = anItem->mPopRect.Contains(x, y);
		if (!aHit) {
			anItem->mHovered = false;
			// the animations go back to rest (an unfinished label of the arena's and the mini-games' is let finish)
			if (aPopAnim != nullptr) {
				if (i == ITEM_ADVENTURE)
					PlayAdventureLabel(" idle");
				else if (i == ITEM_ARENA && !aPopAnim->mPlaying)
					aPopAnim->PlayLabel(mArenaUnlocked ? kIdleLabel : "locked");
				else if (i == ITEM_MINI_GAMES && !aPopAnim->mPlaying)
					aPopAnim->PlayLabel(mMiniGamesUnlocked ? kIdleLabel : "locked");
			}
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
		if (aHand)
			continue;
		// a locked thing (the arena, the mini-games, the tome) takes no hand and has Lex say nothing
		bool aOpen = true;
		if (i == ITEM_ARENA)
			aOpen = mArenaUnlocked;
		else if (i == ITEM_MINI_GAMES)
			aOpen = mMiniGamesUnlocked;
		else if (i == ITEM_TOME)
			aOpen = mTomeUnlocked;
		bool aFree = mApp->mHallOfFame == nullptr && mApp->mCurtain == nullptr;
		if (!anItem->mHovered) {
			if (aOpen && aFree) {
				mHoverUpdate[i] = mUpdateCnt;
				PlayReaction(i);
				aHand = true;
			}
		} else if (!(!aOpen && aFree)) {
			aHand = true;
		}
		if (aHand)
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
		// the animations light up (at every move of the mouse, the adventure's again from its start)
		if (aPopAnim != nullptr) {
			if (i == ITEM_ADVENTURE)
				PlayAdventureLabel(" mouseover");
			else if (i == ITEM_ARENA && !aPopAnim->mPlaying)
				aPopAnim->PlayLabel(mArenaUnlocked ? "mouseover" : "locked");
			else if (i == ITEM_MINI_GAMES && !aPopAnim->mPlaying)
				aPopAnim->PlayLabel(mMiniGamesUnlocked ? "mouseover" : "locked");
		}
	}
	if (!aHand && mApp->GetCursor() == CURSOR_HAND)
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
		if (anItem->mPopAnim != nullptr && !anItem->mPopRect.Contains(x, y))
			continue;
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

// 0x46b688 (the click on the adventure, in MouseUp): how the adventure goes on. A game in progress (a mini-game
// of the adventure's, the book, or the replay's) goes on; otherwise a new one begins. A completed adventure is
// replayed, with the book to play picked: those branches (0x46b97c, the replay transition 0x4439b0 and the dialog
// "Continue your current adventure?") are not ported yet.
void MainMenu::ClickAdventure(bool& theDisableAll)
{
	if (mApp->mProfile == nullptr)
		return;
	// port: the trial's end (0x447b50, its dialog) is left out
	theDisableAll = true;
	mFromTitle = false;
	const bool aComplete = ProfileBool(mApp, "AdventureComplete");
	const bool aSaved = !aComplete && (ProfileBool(mApp, "InMiniGame") || ProfileBool(mApp, "InGame") ||
	                                   ProfileBool(mApp, "ReplayInGame"));
	if (aSaved) {
		// a game in progress: the book under the menu, then the scripts' StartGame once the curtain is closed
		SetDisabled(true);
		mApp->InitGameScreen(false);
		mApp->mWidgetManager->BringToFront(this);
		lua_pushboolean(mApp->mLua->L, 1);
		mApp->mBookManager->CallLinkedMethod("InitSavedGame", 1);
		mApp->StartSavedGameTransition();
	} else if (!aComplete) {
		SetDisabled(true);
		mApp->StartNewGameTransition();
	} else {
		SDL_Log("MainMenu: replaying a completed adventure is not ported yet");
		theDisableAll = false;
	}
}

// The curtain's callback of the hall of fame (0x446520 is the app's function that the original called with the app)
static void OpenHallOfFameCallback(void* theApp)
{
	static_cast<GameApp*>(theApp)->OpenHallOfFame();
}

// The curtain's callback of the mini-games' menu (0x4458b0, the app's function that the original called with the app)
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
		bool aHit = InRect(anItem->mHitRect, x, y) && anItem->HitTest(x, y);
		if (anItem->mPopAnim != nullptr)
			aHit = anItem->mPopRect.Contains(x, y);
		if (!aHit) {
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
			aDisableAll = false;
			break;
		case ITEM_QUIT:
			AskReallyQuit();
			break;
		case ITEM_ADVENTURE:
			ClickAdventure(aDisableAll);
			break;
		case ITEM_ARENA:
			// 0x46bf5x: only once the arena is unlocked and the player is in the second book or the first's sixth
			// chapter. (The warning that an expansion pack's installing ends the arena's games in progress,
			// TM_Invalidated, is not ported: nothing sets that key here.)
			if (ProfileBool(mApp, "UnlockedArena")) {
				if (mApp->mProfile->GetInt("CurrentBook") >= 2 || mApp->mProfile->GetInt("CurrentChapter") >= 6) {
					SetDisabled(true);
					StartArena();
				}
			}
			mApp->SetCursor(CURSOR_POINTER);
			break;
		case ITEM_HALL_OF_FAME:
			// 0x46c0xx: the menu goes dim, the whole curtain closes, and the hall of fame comes (GameApp::OpenHallOfFame)
			SetDisabled(true);
			mApp->DoFullCurtainTransition("");
			mApp->mCurtainCallback = OpenHallOfFameCallback;
			mApp->mCurtainCallbackObject = mApp;
			mFromTitle = false;
			mApp->SetCursor(CURSOR_POINTER);
			break;
		case ITEM_MINI_GAMES:
			// 0x46bcxx: only once the mini-games are unlocked: the menu goes dim, the whole curtain closes, and the
			// mini-games' menu comes (GameApp::ShowMiniGameMenu). (0x443c20 also sets a flag of the app's, +0x95a,
			// that nothing is known to read.)
			if (ProfileBool(mApp, "UnlockedMiniGames")) {
				SetDisabled(true);
				mApp->DoFullCurtainTransition("");
				mApp->mCurtainCallback = ShowMiniGameMenuCallback;
				mApp->mCurtainCallbackObject = mApp;
				mFromTitle = false;
			}
			mApp->SetCursor(CURSOR_POINTER);
			break;
		case ITEM_TOME:
			// 0x46c0xx: only once the tome is open: the whole curtain closes over the menu, then the tome comes (the
			// original wrote out what AddStatsBook does for it). A closed one answers with a dialog whose texts
			// are not known yet.
			if (mTomeUnlocked) {
				mApp->AddStatsBook(false, true, false);
				CloseTooltips();
			}
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
	mApp->DoConfirmDialog(GameApp::DIALOG_REALLY_QUIT, _S(""), GT(0x6fd0a0), nullptr, "");
	GameDialog* aDialog = static_cast<GameDialog*>(mApp->GetDialog(GameApp::DIALOG_REALLY_QUIT));
	aDialog->SetBanner(IMAGE_MM_REALLY_QUIT_BANNER, 0);
	aDialog->mDialogHeader = _S(" ");
	aDialog->Resize(200, 305, aDialog->mWidth + 75, aDialog->mHeight + 20);
	aDialog->SetButtonText(GT(0x6fd0e0), GT(0x6fd0d8));
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
		mApp->DoConfirmDialog(GameApp::DIALOG_CONTINUE_GAME, GT(0x6fcfbc), GT(0x6fcf60), nullptr, "");
		GameDialog* aDialog = static_cast<GameDialog*>(mApp->GetDialog(GameApp::DIALOG_CONTINUE_GAME));
		aDialog->SetBanner(nullptr, 0);
		aDialog->mDialogHeader = GT(0x6fcfbc);
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
	aResources.mHeader = GT(0x703fbc);
	aResources.mNewLabel = GT(0x703fcc);
	aResources.mDeleteLabel = GT(0x703fd0);
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
			mApp->DoOkDialog(GT(0x6f701c), StrFormat(GT(0x6fd050).c_str(), theName.c_str()), "", nullptr);
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
			mApp->DoConfirmDialog(-1, GT(0x6fd040), StrFormat(GT(0x6fcfd8).c_str(), aDialog->mSelected.c_str()),
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
	if (theId == -2) {
		mLex->PlayLabel(kIdleLabel);
		return;
	}
	if (theId < 0 || theId >= NUM_ITEMS || mItems[theId]->mPopAnim == nullptr)
		return;
	// an animation of the three is done: it goes on with its label for the state it is in
	MenuItem* anItem = mItems[theId];
	if (theId == ITEM_ADVENTURE)
		PlayAdventureLabel(anItem->mHovered ? " mouseover" : " idle");
	else if (theId == ITEM_ARENA && mArenaUnlocked)
		anItem->mPopAnim->PlayLabel(anItem->mHovered ? "mouseover" : kIdleLabel);
	else if (theId == ITEM_MINI_GAMES && mMiniGamesUnlocked)
		anItem->mPopAnim->PlayLabel(anItem->mHovered ? "mouseover" : kIdleLabel);
}

void MainMenu::PopAnimCommand(int theId, const std::string& theCommand, const std::string& theParam)
{
	if (theId != mLex->mId || SDL_strcasecmp(theCommand.c_str(), "playsound") != 0)
		return;
	int aSound = mApp->mResourceManager->GetSound(theParam);
	if (aSound >= 0)
		mApp->PlaySample(aSound);
}
