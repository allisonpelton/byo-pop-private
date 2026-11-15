#include "ShopScreen.h"

#include <cmath>
#include <cstdlib>
#include <vector>

#include "Application.h"
#include "Avatar.h"
#include "Coins.h"
#include "Common.h"
#include "Creature.h"
#include "Cubby.h"
#include "DrawUtil.h"
#include "Font.h"
#include "GameScreen.h"
#include "Graphics.h"
#include "HammerDisplay.h"
#include "Hearts.h"
#include "Image.h"
#include "NcButton.h"
#include "Res.h"
#include "Score.h"
#include "ShopItem.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "Stats.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

const double PI_HALF = 1.5707963267948966;

// How long the mole's words last (in the hundredths the timer counts down in): a refusal, a purchase's thanks, and the start
// of the first mood
const double REFUSAL_TIME = 5.0;
const double THANKS_TIME = 3.76;
const double WELCOME_TIME = 4.0;

// How long the hammer may rest on no item before the mole tells the player to look around (in the same hundredths)
const double BROWSE_HINT_TIME = 2.0;

// What the mole says while the hammer is over the arrow back to the game, by what the player is playing
const char* const SPEECH_COME_BACK_SOON = "Come back soon!";
const char* const SPEECH_SEE_YOU_IN_TWO_LEVELS = "I hope to see you again in two levels!";
const char* const SPEECH_ALWAYS_A_PLEASURE = "Always a pleasure!";

// The level from which a classic or marathon game (every tough cookie game) has each power-up in the shop
const int ICE_CREAM_FIRST_LEVEL = 8;
const int CLOVER_FIRST_LEVEL = 13;
const int STRAWBERRY_FIRST_LEVEL = 25;

// The most hearts the player can have (the health capacity at which the extra heart is no longer sold)
const int MAX_HEALTH_CAPACITY = 7;

// 0x45ee90: the numbers in a random order. A number is drawn at random from those that are left, one draw for each number
// (the last one is drawn too, from one choice).
void Shuffle(std::vector<int>& theKinds)
{
	std::vector<int> aRemaining = theKinds;
	theKinds.clear();
	while (!aRemaining.empty())
	{
		int anIndex = Rand() % (int)aRemaining.size();
		theKinds.push_back(aRemaining[anIndex]);
		aRemaining.erase(aRemaining.begin() + anIndex);
	}
}

// The kind is offered when none of its kind is on a cubby already
void AddIfNotOnShelf(std::vector<int>& theKinds, int theKind)
{
	if (CountCreaturesOfKind(theKind) == 0)
		theKinds.push_back(theKind);
}

// Whether the shop sells a power-up that is in from the level: a tough cookie game has them all, the others from the level on
bool PowerUpIsIn(int theFirstLevel)
{
	if (gApp->mGameMode == GAME_MODE_CLASSIC && gApp->mGameScreen->mLevel >= theFirstLevel)
		return true;
	if (gApp->mGameMode == GAME_MODE_MARATHON && gApp->mGameScreen->mLevel >= theFirstLevel)
		return true;
	return gApp->mGameMode == GAME_MODE_TOUGH_COOKIE;
}

}

// 0x45e760
ShopScreen::ShopScreen() :
	mMoodTimer(WELCOME_TIME),
	mMood(MOOD_WELCOME),
	mBrowseTimer(0.0),
	mWritingSound(nullptr),
	mFade(1.0)
{
	mBackButton = MakeNcButton(0, this, "", IMAGE_SHOP_BACK_TO_GAME_ARROW, 3);
	mMenuButton = MakeNcButton(1, this, "", IMAGE_INGAMEUI_MENUBUTTON, 3);
}

// 0x45e1e0
ShopScreen::~ShopScreen()
{
	gApp->SafeDeleteWidget(mBackButton);
	gApp->SafeDeleteWidget(mMenuButton);
	if (mWritingSound != nullptr)
		mWritingSound->Release();
}

// 0x45df30
Image* ShopScreen::GetBackdrop()
{
	return IMAGE_SHOP_BACKDROP;
}

// 0x45e310 (vfd8): the level's objects are put aside, and the player's displays and the hammer are all that stay (the gnomes
// of the level are paused); nine cubbies stand on the shelf in rows of four, three and two, each sending up an item after its
// time
void ShopScreen::Opened()
{
	ObjectManager::Get()->DeactivateAll();
	if (!gApp->IsPaused())
		gApp->PauseGame(true);

	GetHearts()->mActive = true;
	GetCoins()->mActive = true;
	GetAvatar()->mActive = true;
	GetHammerDisplay()->mActive = true;
	GetScore()->mActive = true;

	// the cubbies are as far apart as the pedestal is wide, the rows centred on the middle of the screen
	int aHalfPedestal = IMAGE_SHOP_PEDESTAL->mWidth / 2;
	int aCenterLeft = 400 - aHalfPedestal;
	Cubby* aCubby;

	aCubby = AddCubby(aCenterLeft - aHalfPedestal * 3, 0xf0, MakeCubbyName(90006));
	aCubby->mTimer = 1.0;
	aCubby = AddCubby(aCenterLeft - aHalfPedestal, 0xf0, MakeCubbyName(90007));
	aCubby->mTimer = 1.3;
	aCubby = AddCubby(aCenterLeft + aHalfPedestal, 0xf0, MakeCubbyName(90008));
	aCubby->mTimer = 1.6;
	aCubby = AddCubby(aCenterLeft + aHalfPedestal * 3, 0xf0, MakeCubbyName(90009));
	aCubby->mTimer = 1.9;

	aCubby = AddCubby(aCenterLeft - aHalfPedestal * 2, 0x140, MakeCubbyName(90003));
	aCubby->mTimer = 0.5;
	aCubby = AddCubby(aCenterLeft, 0x140, MakeCubbyName(90004));
	aCubby->mTimer = 0.8;
	aCubby = AddCubby(aCenterLeft + aHalfPedestal * 2, 0x140, MakeCubbyName(90005));
	aCubby->mTimer = 1.1;

	aCubby = AddCubby(aCenterLeft - aHalfPedestal, 400, MakeCubbyName(90001));
	aCubby->mTimer = 0.0;
	aCubby = AddCubby(aCenterLeft + aHalfPedestal, 400, MakeCubbyName(90002));
	aCubby->mTimer = 0.3;
}

// 0x45ded0 (vfdc): what the shop made is taken away, the level's objects come back and the game goes on
void ShopScreen::Closing()
{
	GetHearts()->mActive = false;
	GetCoins()->mActive = false;
	GetAvatar()->mActive = false;
	GetHammerDisplay()->mActive = false;
	GetScore()->mActive = false;

	ObjectManager::Get()->RemoveByActive(true);
	ObjectManager::Get()->ActivateAll();
	gApp->PauseGame(false);
}

// 0x45ef90 (vfe0): three lists of kinds are made, each in a random order: every item but the power-ups and the gift; the ones
// of them that the player can use and that are not on a cubby (a heart when the health is not full, the extra heart below
// seven hearts, the flame, each hammer above the one the player has); and the power-ups that the level has and that are not
// on a cubby. The first of the second list is the next item, or else the first of the third, or else the first of the first.
int ShopScreen::GetCreatureKind()
{
	std::vector<int> anAllKinds;
	anAllKinds.push_back(KIND_SHOP_HEART);
	anAllKinds.push_back(KIND_SHOP_BIG_HEART);
	anAllKinds.push_back(KIND_SHOP_EXTRA_HEART);
	anAllKinds.push_back(KIND_SHOP_FLAME);
	anAllKinds.push_back(KIND_SHOP_HAMMER_2);
	anAllKinds.push_back(KIND_SHOP_HAMMER_3);
	anAllKinds.push_back(KIND_SHOP_HAMMER_4);

	std::vector<int> aUsefulKinds;
	if (GetPlayerHealth() != GetHealthCapacity())
	{
		AddIfNotOnShelf(aUsefulKinds, KIND_SHOP_HEART);
		AddIfNotOnShelf(aUsefulKinds, KIND_SHOP_BIG_HEART);
	}
	if (GetStats()->GetInt(STAT_HEALTH_CAPACITY) != MAX_HEALTH_CAPACITY)
		AddIfNotOnShelf(aUsefulKinds, KIND_SHOP_EXTRA_HEART);
	AddIfNotOnShelf(aUsefulKinds, KIND_SHOP_FLAME);
	if (GetStats()->GetInt(STAT_HAMMER_LEVEL) < 1)
		AddIfNotOnShelf(aUsefulKinds, KIND_SHOP_HAMMER_2);
	if (GetStats()->GetInt(STAT_HAMMER_LEVEL) < 2)
		AddIfNotOnShelf(aUsefulKinds, KIND_SHOP_HAMMER_3);
	if (GetStats()->GetInt(STAT_HAMMER_LEVEL) < 3)
		AddIfNotOnShelf(aUsefulKinds, KIND_SHOP_HAMMER_4);

	std::vector<int> aPowerUpKinds;
	if (PowerUpIsIn(ICE_CREAM_FIRST_LEVEL))
		AddIfNotOnShelf(aPowerUpKinds, KIND_SHOP_ICE_CREAM);
	if (PowerUpIsIn(CLOVER_FIRST_LEVEL))
		AddIfNotOnShelf(aPowerUpKinds, KIND_SHOP_CLOVER);
	if (PowerUpIsIn(STRAWBERRY_FIRST_LEVEL))
		AddIfNotOnShelf(aPowerUpKinds, KIND_SHOP_STRAWBERRY);

	Shuffle(anAllKinds);
	Shuffle(aUsefulKinds);
	Shuffle(aPowerUpKinds);

	if (!aUsefulKinds.empty())
		return aUsefulKinds[0];
	if (!aPowerUpKinds.empty())
		return aPowerUpKinds[0];
	return anAllKinds[0];
}

// 0x45df70: the mole stops writing and says why not
void ShopScreen::ShowRefusal(int theReason)
{
	mMoodTimer = REFUSAL_TIME;
	mMood = theReason;
	if (mWritingSound != nullptr)
	{
		mWritingSound->Release();
		mWritingSound = nullptr;
	}
}

// 0x45ee20: the mole writes the thanks (with the sound of the pen)
void ShopScreen::ShowThanks(const std::string& theLine)
{
	mMoodTimer = THANKS_TIME;
	mMood = MOOD_THANKS;
	mThanksLine = theLine;
	if (mWritingSound != nullptr)
		mWritingSound->Release();
	mWritingSound = gApp->mSoundManager->GetSoundInstance(SOUND_SHOP_MOLE_WRITING);
	if (mWritingSound != nullptr)
		mWritingSound->Play(false, false);
}

// 0x45ddf0
void ShopScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mBackButton);
	theWidgetManager->AddWidget(mMenuButton);
}

// 0x45de20
void ShopScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mBackButton);
	theWidgetManager->RemoveWidget(mMenuButton);
}

// 0x45de50: the arrow back to the game is at the right, low down; the menu button in the top right corner
void ShopScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mBackButton->Resize(gApp->mWidth - mBackButton->mWidth - 5, 0x140, mBackButton->mWidth, mBackButton->mHeight);
	mMenuButton->Move(gApp->mWidth - mMenuButton->mWidth - 6, theY + 6);
}

// 0x45ddd0: the player cannot leave while the gift box rolls
void ShopScreen::Update()
{
	Widget::Update();
	mBackButton->mDisabled = IsGiftReady();
}

// 0x45dd50
void ShopScreen::MouseEnter()
{
	Widget::MouseEnter();
	gApp->SetCursor(CURSOR_NONE);
	Avatar* anAvatar = GetAvatar();
	if (anAvatar != nullptr)
		anAvatar->mCrosshairVisible = true;
}

// 0x45dd90
void ShopScreen::MouseLeave()
{
	Widget::MouseLeave();
	gApp->SetCursor(CURSOR_POINTER);
	Avatar* anAvatar = GetAvatar();
	if (anAvatar != nullptr)
		anAvatar->mCrosshairVisible = false;
}

// 0x45dd20: a click is the hammer's
void ShopScreen::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);
	Avatar* anAvatar = GetAvatar();
	if (anAvatar != nullptr)
		anAvatar->AddClick(x, y);
}

// 0x45df40 (the ButtonListener's slot 2): the arrow leaves the shop, the other button is the game menu
void ShopScreen::ButtonDepress(int theId)
{
	if (theId == 0)
		gApp->LeaveShop();
	else if (theId == 1)
		gApp->ShowGameMenu();
}

// 0x45e910: the shop is dimmed behind the game menu and the zoom wipe, and lights up when they are gone. When the lights are
// on the items' cubbies wait out their time and send up their item, and the mole's mood goes on: what it says follows what the
// hammer is over, or the words of the last purchase or refusal.
void ShopScreen::UpdateF(float theFrac)
{
	if (gApp->IsMenuOpen() || gApp->mZooming)
	{
		if (mFade >= 1.0)
			return;
		mFade = theFrac * 0.02 + mFade;
		if (mFade > 1.0)
			mFade = 1.0;
		return;
	}

	MarkDirty();
	if (mFade > 0.0)
	{
		mFade = mFade - theFrac * 0.02;
		if (mFade < 0.0)
			mFade = 0.0;
	}

	ObjectManager::Get()->Update(theFrac);

	// each cubby that is empty waits out its time and then sends its item up
	for (int i = 0; i < (int)mCubbies.size(); i++)
	{
		if (GetCubby(i)->mState == Cubby::STATE_EMPTY)
		{
			double aStep = theFrac * 0.01;
			double aTimer = GetCubby(i)->mTimer - aStep;
			if (aTimer <= 0.0)
				aTimer = 0.0;
			GetCubby(i)->mTimer = aTimer;
			if (aTimer == 0.0)
				GetCubby(i)->SetState(Cubby::STATE_RISING, -1);
		}
	}

	// when the mood's time is up the mole looks about, or back down, and it stays so for three to seven of the same hundredths
	double aMoodTimer = mMoodTimer - theFrac * 0.01;
	if (aMoodTimer < 0.0)
		aMoodTimer = 0.0;
	mMoodTimer = aMoodTimer;
	if (aMoodTimer == 0.0)
	{
		switch (mMood)
		{
		case MOOD_WELCOME:
		case MOOD_LOOKING:
		case MOOD_THANKS:
		case REFUSE_NO_COINS:
		case REFUSE_HAVE_HAMMER:
		case REFUSE_HAVE_BETTER_HAMMER:
		case REFUSE_HAVE_FLAME:
		case REFUSE_HEARTS_AT_MAXIMUM:
		case REFUSE_HAVE_ICE_CREAM:
		case REFUSE_HAVE_CLOVER:
		case REFUSE_HAVE_STRAWBERRY:
		case REFUSE_HEALTH_FULL:
			mMood = MOOD_BROWSING;
			mMoodTimer = (double)Rand() * 4.656612875245797e-10 * 4.0 + 3.0;
			break;
		case MOOD_BROWSING:
			mMood = MOOD_LOOKING;
			mMoodTimer = (double)Rand() * 4.656612875245797e-10 * 4.0 + 3.0;
			break;
		}
	}

	// what the mole says
	if (mBackButton->mIsOver)
	{
		if (gApp->mGameMode != GAME_MODE_CLASSIC && gApp->mGameMode != GAME_MODE_TOUGH_COOKIE)
			mSpeech = SPEECH_COME_BACK_SOON;
		else if (gApp->mGameScreen->mLevel <= 23)
			mSpeech = SPEECH_SEE_YOU_IN_TWO_LEVELS;
		else
			mSpeech = SPEECH_ALWAYS_A_PLEASURE;
		return;
	}

	switch (mMood)
	{
	case MOOD_WELCOME:
		mSpeech = "Welcome to the Shiny Shop, dear customer!";
		return;
	case MOOD_BROWSING:
	case MOOD_LOOKING:
		for (int i = 0; i < (int)mCubbies.size(); i++)
		{
			if (GetCubby(i)->mHover && GetCubby(i)->mCreature != nullptr)
			{
				// the hammer is over an item: the mole describes it
				ShopItem* anItem = dynamic_cast<ShopItem*>(GetCubby(i)->mCreature);
				mSpeech = anItem->mDescription;
				mBrowseTimer = 0.0;
				return;
			}
		}
		mBrowseTimer = theFrac * 0.01 + mBrowseTimer;
		if (mBrowseTimer > BROWSE_HINT_TIME)
			mSpeech = "Feel free to browse around!";
		return;
	case MOOD_THANKS:
		mSpeech = mThanksLine;
		return;
	case REFUSE_NO_COINS:
		mSpeech = "Eh! I am so sorry, but we do not sell on credit!";
		return;
	case REFUSE_HAVE_HAMMER:
		mSpeech = "I would love to sell it to you, but you already have one!";
		return;
	case REFUSE_HAVE_BETTER_HAMMER:
		mSpeech = "Your hammer is already better than this one!";
		return;
	case REFUSE_HAVE_FLAME:
		mSpeech = "Your hammer is already on fire!";
		return;
	case REFUSE_HEARTS_AT_MAXIMUM:
		mSpeech = "You are at full heart capacity. Any more and you would blow!";
		return;
	case REFUSE_HAVE_ICE_CREAM:
		mSpeech = "You can only have one ice-cream at a time. Brain-freeze, you know!";
		return;
	case REFUSE_HAVE_CLOVER:
		mSpeech = "Sorry, I can only sell one of these per customer. They are ever so rare!";
		return;
	case REFUSE_HAVE_STRAWBERRY:
		mSpeech = "One strawberry at a time, please! Two would cancel each other!";
		return;
	case REFUSE_HEALTH_FULL:
		mSpeech = "Save your money, you are in perfect health!";
		return;
	}
}

// 0x45dfb0: the shop's picture, the mole's words in the box on the counter and the mole itself, then the objects (the cubbies,
// the displays and the hammer), the tough cookie stamp, and the dimming
void ShopScreen::Draw(Graphics* g)
{
	Graphics aG(*g);
	aG.DrawImage(IMAGE_SHOP_BACKDROP, 0, 0);

	aG.SetFont(FONT_KIDPRINT22BOLD);
	aG.SetColor(Color(0, 0, 0, 255));
	Rect aSpeechRect(0x135, 0x1db, 0x124, 0x62);
	int aSpeechHeight = aG.GetWordWrappedHeight(0x124, mSpeech, -1, nullptr);
	aSpeechRect.mY += (aSpeechRect.mHeight - aSpeechHeight) / 2;
	aG.WriteWordWrapped(aSpeechRect, mSpeech, -1, 0);

	// the mole's picture is a sheet of three rows: the mole at rest and looking up, the mole writing (the pen goes to and fro
	// as the time runs out), and the mole with a face for the refusals
	int aColumn;
	int aRow;
	switch (mMood)
	{
	case MOOD_WELCOME:
	case MOOD_BROWSING:
		aColumn = 0;
		aRow = 0;
		break;
	case MOOD_LOOKING:
		aColumn = 1;
		aRow = 0;
		break;
	case MOOD_THANKS:
		aColumn = abs(ToInt(mMoodTimer * 8.0) % 6 - 3);
		aRow = 1;
		break;
	default:
		aColumn = 0;
		aRow = 2;
		break;
	}
	aG.DrawImageCel(IMAGE_MOLE_SHOP, 0, 0x136, aColumn, aRow);

	ObjectManager::Get()->Draw(&aG);

	if (gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
		aG.DrawImage(IMAGE_TOUGH_COOKIE_STAMP, mWidth - IMAGE_TOUGH_COOKIE_STAMP->mWidth, mHeight - IMAGE_TOUGH_COOKIE_STAMP->mHeight);

	if (mFade > 0.0)
	{
		int anAlpha = ToInt(sin(mFade * PI_HALF) * 128.0);
		aG.SetColor(Color(0, 0, 0, anAlpha));
		aG.FillRect(0, 0, mWidth, mHeight);
	}
}
