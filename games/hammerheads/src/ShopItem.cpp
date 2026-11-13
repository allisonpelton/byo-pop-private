#include "ShopItem.h"

#include <cstdio>

#include "Application.h"
#include "Color.h"
#include "DrawUtil.h"
#include "Floater.h"
#include "Font.h"
#include "GameScreen.h"
#include "Graphics.h"
#include "HammerDisplay.h"
#include "Hearts.h"
#include "Image.h"
#include "Res.h"
#include "Score.h"
#include "ShopScreen.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "Stats.h"
#include "ValueList.h"

using namespace Sexy;

// 0x441250
bool Sexy::CanBuy(int theKind, int thePrice, bool theAnnounce)
{
	int aRefusal = 0;
	bool aRefused = false;

	// the hammers: the one the player has, or a better one, cannot be bought again
	if (theKind == 0x20)
	{
		int aLevel = GetStats()->GetInt(STAT_HAMMER_LEVEL);
		if (aLevel == 1)
		{
			aRefused = true;
			aRefusal = REFUSE_HAVE_HAMMER;
		}
		else if (aLevel > 1)
		{
			aRefused = true;
			aRefusal = REFUSE_HAVE_BETTER_HAMMER;
		}
	}
	else if (theKind == 0x21)
	{
		int aLevel = GetStats()->GetInt(STAT_HAMMER_LEVEL);
		if (aLevel == 2)
		{
			aRefused = true;
			aRefusal = REFUSE_HAVE_HAMMER;
		}
		else if (aLevel > 2)
		{
			aRefused = true;
			aRefusal = REFUSE_HAVE_BETTER_HAMMER;
		}
	}
	else if (theKind == 0x22)
	{
		int aLevel = GetStats()->GetInt(STAT_HAMMER_LEVEL);
		if (aLevel == 3)
		{
			aRefused = true;
			aRefusal = REFUSE_HAVE_HAMMER;
		}
		else if (aLevel >= 4)
		{
			aRefused = true;
			aRefusal = REFUSE_HAVE_BETTER_HAMMER;
		}
	}

	if (!aRefused && 0 < thePrice && GetStats()->GetInt(STAT_COIN_BALANCE) < thePrice)
	{
		aRefused = true;
		aRefusal = REFUSE_NO_COINS;
	}

	if (!aRefused && (theKind == 0x1c || theKind == 0x1d))
	{
		GetHearts();
		if (GetPlayerHealth() == GetHealthCapacity())
		{
			aRefused = true;
			aRefusal = REFUSE_HEALTH_FULL;
		}
	}

	if (!aRefused)
	{
		if (theKind == 0x1f)
		{
			if (GetStats()->GetInt(STAT_HAMMER_ON_FIRE) == 1)
			{
				aRefused = true;
				aRefusal = REFUSE_HAVE_FLAME;
			}
		}
		else if (theKind == 0x1e)
		{
			if (GetStats()->GetInt(STAT_HEALTH_CAPACITY) == 7)
			{
				aRefused = true;
				aRefusal = REFUSE_HEARTS_AT_MAXIMUM;
			}
		}
		else if (theKind == 0x23)
		{
			if (GetStats()->GetInt(STAT_HAS_ICE_CREAM) != 0)
			{
				aRefused = true;
				aRefusal = REFUSE_HAVE_ICE_CREAM;
			}
		}
		else if (theKind == 0x24)
		{
			if (GetStats()->GetInt(STAT_HAS_CLOVER) != 0)
			{
				aRefused = true;
				aRefusal = REFUSE_HAVE_CLOVER;
			}
		}
		else if (theKind == 0x25)
		{
			if (GetStats()->GetInt(STAT_HAS_STRAWBERRY) != 0)
			{
				aRefused = true;
				aRefusal = REFUSE_HAVE_STRAWBERRY;
			}
		}
	}

	if (aRefused)
	{
		if (theAnnounce)
			gApp->mShopScreen->ShowRefusal(aRefusal);
		return false;
	}
	return true;
}

// 0x42f270
void Sexy::GetShopItemAnchor(double theAnchor[2], int theKind)
{
	switch (theKind)
	{
	case 0x1c: theAnchor[0] = 20.0; theAnchor[1] = 33.0; return;
	case 0x1d:
	case 0x1e: theAnchor[0] = 32.0; theAnchor[1] = 52.0; return;
	case 0x1f: theAnchor[0] = 26.0; theAnchor[1] = 86.0; return;
	case 0x20: theAnchor[0] = 60.0; theAnchor[1] = 66.0; return;
	case 0x21: theAnchor[0] = 61.0; theAnchor[1] = 68.0; return;
	case 0x22: theAnchor[0] = 50.0; theAnchor[1] = 78.0; return;
	default: theAnchor[0] = 0.0; theAnchor[1] = 0.0; return;
	}
}

// 0x442570: the table of the exe holds the pictures by their numbers (0x134 the heart, 0x135 the big heart, 0x136 the extra
// heart, 0x137 the flame, 0x11a to 0x11c the hammers); the gift and the other kinds show the heart
Image* Sexy::GetShopItemImage(int theKind)
{
	switch (theKind)
	{
	default: return IMAGE_SHOP_HEART;
	case 0x1d: return IMAGE_SHOP_HEART_BIG;
	case 0x1e: return IMAGE_SHOP_HEART_EXTRA;
	case 0x1f: return IMAGE_SHOP_FLAME;
	case 0x20: return IMAGE_PLAYER_HAMMER_2;
	case 0x21: return IMAGE_PLAYER_HAMMER_3;
	case 0x22: return IMAGE_PLAYER_HAMMER_4;
	}
}

// 0x441540: the cell of the sheet that a kind is drawn from (the hammers are the sixth cell of the first row)
static void GetShopItemCell(double theCell[2], int theKind)
{
	if (0x1f < theKind && theKind < 0x23)
	{
		theCell[0] = 0.0;
		theCell[1] = 6.0;
		return;
	}
	theCell[0] = 0.0;
	theCell[1] = 0.0;
}

// 0x442610: a prize of the gift: the flame, a big heart, an extra heart or a hammer (the one above the player's, if the
// player has a hammer already)
static int RollGiftPrize()
{
	int aPrizes[4] = { 0x1f, 0x1d, 0x1e, 0x20 };
	int aKind = aPrizes[Rand() % 4];
	if (aKind == 0x20)
	{
		int aLevel = GetStats()->GetInt(STAT_HAMMER_LEVEL);
		if (aLevel == 1)
			return 0x21;
		if (aLevel == 2)
			return 0x22;
	}
	return aKind;
}

// 0x442910: a prize that is neither of the two kinds given
static int RollOtherPrize(int theKind, int theOtherKind)
{
	int aKind = theKind;
	while (aKind == theKind || aKind == theOtherKind)
		aKind = RollGiftPrize();
	return aKind;
}

// 0x441b40 (the kinds' prices, effects and texts are set in the switch)
ShopItem::ShopItem(int theKind, double theCubbyY) :
	Creature(theKind),
	mPosX(0.0),
	mPosY(0.0),
	mUnusedD0(1.0),
	mPrice(0),
	mHeartAmount(0.0),
	mOffsetY(0),
	mPurchaseTimer(0.0)
{
	switch (theKind)
	{
	case 0x1c:
		mHeartAmount = 0.5;
		mPrice = 5;
		mDescription = "Restore some health with this affordable heart.";
		mThanks = "A good choice!";
		break;
	case 0x1d:
		mHeartAmount = 2.5;
		mPrice = 20;
		mDescription = "Great value for money! Will restore much of your health.";
		mThanks = "I use them myself you know!";
		break;
	case 0x1e:
		mPrice = 700;
		mDescription = "Increase your heart capacity. Well worth the expense.";
		mThanks = "Awesome! I also sell heart power to fill it up.";
		break;
	case 0x1f:
		mPrice = 100;
		mDescription = "This will make your hammer lightning fast,  until you upgrade it!";
		mThanks = "They look alive, these flames. So lovely...";
		break;
	case 0x20:
		mPrice = 250;
		mDescription = "Twice the power of your default hammer!";
		mThanks = "You will be the envy of your friends!";
		break;
	case 0x21:
		mPrice = 1200;
		mDescription = "Careful with that! It's three times as powerful as your default hammer!";
		mThanks = "You will be pleased, I guarantee it.";
		break;
	case 0x22:
		mPrice = 2250;
		mDescription = "The best there is. No less than SIX times the power of a standard hammer!";
		mThanks = "It is a hammer worthy of Thor himself.";
		break;
	case 0x23:
		mPrice = 250;
		mDescription = "This ice-cream will freeze the gnomes for a short while.";
		mThanks = "Enjoy! The freezing will start as soon as you leave the shop.";
		break;
	case 0x24:
		mPrice = 250;
		mDescription = "Lots of health and wealth will come your way with one of these.";
		mThanks = "Great. When it runs out, just buy another one!";
		break;
	case 0x25:
		mPrice = 250;
		mDescription = "Missing a lot? This magic strawberry will make it less painful!";
		mThanks = "Click away!";
		break;
	case 0x26:
		mPrice = 500;
		mDescription = "Feeling lucky? Will you get something useless, or something great?";
		mThanks = "Happy?";
		break;
	default:
		break;
	}

	// the lower the cubby, the less the item stands above its line
	mOffsetY = ToInt((double)IMAGE_SHOP_PEDESTAL->mHeight * (600.0 - theCubbyY) * 0.001);
	mHeight = (double)mOffsetY;
	mGravity = 0.0;
}

// 0x4415c0
ShopItem::~ShopItem()
{
}

// 0x441580
void ShopItem::GetSheetCell(double theCell[2]) const
{
	GetShopItemCell(theCell, mKind);
}

// 0x4411d0
void ShopItem::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mPosX = thePos[0];
	mPosY = thePos[1];
	if (mSpent && 0.0 <= mPurchaseTimer)
	{
		double aTimer = mPurchaseTimer - 0.01;
		if (aTimer <= 0.0)
			aTimer = 0.0;
		mPurchaseTimer = aTimer;
		if (aTimer <= 0.0)
			mFinished = true;
	}
}

// 0x441640: the pedestal, the item with its shadow, and its price under it; an item that cannot be bought now is drawn grey
void ShopItem::Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime)
{
	bool aCanBuy = CanBuy(mKind, mPrice, false);
	if (!aCanBuy)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0xa0, 0xa0, 0xa0));
	}

	DrawImageLit(g, IMAGE_SHOP_PEDESTAL, ToInt(theX) - IMAGE_CUBBY_PLATFORM->mWidth / 2,
		(ToInt(theY) - IMAGE_CUBBY_PLATFORM->mHeight / 2) - mOffsetY, theHover != 0);

	if (!mSpent && mGlowImage != nullptr)
	{
		g->SetColorizeImages(true);
		double aShadowAlpha = 100.0 - (mHeight - (double)mOffsetY) * 6.0;
		if (!(0.0 <= aShadowAlpha))
			aShadowAlpha = 0.0;
		g->SetColor(Color(255, 255, 255, ToInt(aShadowAlpha)));
		g->DrawImage(IMAGE_SHADOW_GNOME, ToInt(theX - (double)IMAGE_SHADOW_GNOME->mWidth * 0.5),
			ToInt((theY - (double)mOffsetY) - (double)IMAGE_SHADOW_GNOME->mHeight * 0.5));
		g->SetColorizeImages(false);
		Creature::Draw(g, theX, theY, theHover, theFlashTime);
	}

	// the price, centred under the item
	int aTextBaseY = ToInt((theY - mHeight) + 55.0);
	char aBuffer[32];
	sprintf(aBuffer, "%d", mPrice);
	std::string aText = aBuffer;
	Font* aFont = FONT_PTBANANASPLIT22;
	int aWidth = aFont->StringWidth(aText);
	int aHeight = aFont->GetHeight();
	int aTextY = ToInt((double)aTextBaseY - (double)aFont->GetHeight() * 0.5);
	int aTextX = ToInt(theX - (double)aWidth * 0.5);
	Rect aClip(aTextX, aTextY, aWidth, aHeight);
	Graphics aG(*g);
	Color aColor = aCanBuy ? Color(255, 255, 255, 255) : Color(0xa0, 0xa0, 0xa0);
	aFont->DrawString(&aG, aTextX, aTextY, aText, aColor, aClip);
}

// 0x4419c0
std::string ShopItem::SaveExtra()
{
	ValueList aList;
	aList.Add(mPosX);
	aList.Add(mPosY);
	aList.Add(mUnusedD0);
	return aList.ToString();
}

// 0x441a70
void ShopItem::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mPosX = aList.NextDouble();
	mPosY = aList.NextDouble();
	mUnusedD0 = aList.NextDouble();
}

// 0x441500: an item that is frozen only goes; one that is not goes with a flash that fades over a few updates
void ShopItem::Remove()
{
	if (IsFrozen())
	{
		mUntouchable = true;
		mFinished = true;
		return;
	}
	mSpent = true;
	mPurchaseTimer = 0.75;
}

// 0x442350: the hammer on an item buys it, if the shop would sell it; the gift starts its roll of prizes instead
bool ShopItem::Hit()
{
	bool aCanBuy = CanBuy(mKind, mPrice, true);
	if (aCanBuy)
	{
		if (mKind != 0x26)
		{
			Purchase(mHeartAmount, mPrice, mKind);
			return aCanBuy;
		}

		int aPrize = RollGiftPrize();
		if (!CanBuy(aPrize, 0, false))
			aPrize = 0x1d;
		static_cast<ShopItemGift*>(this)->StartRoll(aPrize);
	}
	return aCanBuy;
}

// where the picture of the item is when it flies off: its middle, the item's line above the platform
static double GetFlightY(const ShopItem* theItem, double theHalfHeight)
{
	return ((theItem->mPosY - theHalfHeight) - (theItem->mAnchorY - theItem->mHeadY) * 0.5) - (double)theItem->mOffsetY;
}

// 0x441dc0
void ShopItem::Purchase(double theHealth, int thePrice, int theKind)
{
	if (theHealth != 0.0)
	{
		GetHearts()->ChangeHealth(HEALTH_GAIN, theHealth);
		double aEnd[2];
		GetHearts()->GetHealthEndPos(aEnd);
		MakeFloaterHeart(mPosX, (mPosY - (mAnchorY - mHeadY) * 0.5) - (double)mOffsetY, aEnd[0], aEnd[1], 0.0);
	}
	if (thePrice != 0)
		GetStats()->AddInt(STAT_COIN_BALANCE, -thePrice);

	if (theKind == 0x20)
	{
		MakeFloaterRotating(mPosX, GetFlightY(this, 0.0), 15.0, 45.0, IMAGE_PLAYER_HAMMER_2, 0, 5, 0.6981317202250164);
		GetHammerDisplay()->UpgradeToLevel(1);
	}
	else if (theKind == 0x21)
	{
		MakeFloaterRotating(mPosX, GetFlightY(this, 0.0), 15.0, 45.0, IMAGE_PLAYER_HAMMER_3, 0, 5, 0.6981317202250164);
		GetHammerDisplay()->UpgradeToLevel(2);
	}
	else if (theKind == 0x22)
	{
		MakeFloaterRotating(mPosX, GetFlightY(this, 0.0), 15.0, 45.0, IMAGE_PLAYER_HAMMER_4, 0, 5, 0.6981317202250164);
		GetHammerDisplay()->UpgradeToLevel(3);
	}
	else if (theKind == 0x1f)
	{
		MakeFloaterRotating(mPosX, GetFlightY(this, 0.0), 25.0, 25.0, IMAGE_SHOP_FLAME, 0, 0, 0.0);
		GetHammerDisplay()->SetOnFire();
	}
	else
	{
		// the three power-ups fly to their signs in the corner of the screen; the next level starts with them on
		Image* aImage = nullptr;
		double aEndX = 0.0;
		double aEndY = 0.0;
		if (theKind == 0x23)
		{
			GetStats()->AddInt(STAT_HAS_ICE_CREAM, 1);
			ScoreDisplayPos aPos = GetIceCreamSignPos();
			aEndX = aPos.mX + 125.0;
			aEndY = aPos.mY - 30.0;
			aImage = IMAGE_GNOME_BONUS_ICECREAM;
		}
		else if (theKind == 0x24)
		{
			GetStats()->AddInt(STAT_HAS_CLOVER, 1);
			ScoreDisplayPos aPos = GetCloverSignPos();
			aEndX = aPos.mX + 95.0;
			aEndY = aPos.mY - 30.0;
			aImage = IMAGE_LUCKY_CLOVER;
		}
		else if (theKind == 0x25)
		{
			GetStats()->AddInt(STAT_HAS_STRAWBERRY, 1);
			ScoreDisplayPos aPos = GetStrawberrySignPos();
			aEndX = aPos.mX + 110.0;
			aEndY = aPos.mY - 30.0;
			aImage = IMAGE_GNOME_BONUS_STRAWBERRY;
		}
		else if (theKind == 0x1e)
		{
			// the extra heart flies to the place in the row where it goes
			double aNewHeart[2];
			GetHearts()->GetNewHeartPos(aNewHeart);
			MakeFloaterRotating(mPosX, GetFlightY(this, 0.0), aNewHeart[0] - 10.0, aNewHeart[1], IMAGE_INGAMEUI_HEARTS, 0, 1, 0.0);
			GetHearts()->ChangeHealth(HEALTH_NEW_HEART, 0.0);
			if (GetStats()->GetInt(STAT_HEALTH_CAPACITY) == 7)
				mThanks = "Great, but that was the last one I can sell you. Now you are at full capacity!";
		}

		if (aImage != nullptr)
		{
			MakeFloaterShrinking(mPosX - (double)(aImage->mWidth / 2), GetFlightY(this, (double)(aImage->mHeight / 2)), aEndX, aEndY,
				aImage, 0, 0, 0.0, 0, 0, 0, 0);
		}
	}

	GetStats()->AddInt(STAT_SUPPLIES_BOUGHT, 1);
	GetStats()->SetInt(STAT_LEVEL_OF_LAST_PURCHASE, gApp->mGameScreen->mLevel);
	gApp->mShopScreen->ShowThanks(mThanks);
}

// 0x4423e0
ShopItemFlame::ShopItemFlame(int theKind, double theCubbyY) :
	ShopItem(theKind, theCubbyY),
	mFlameCounter(0.0)
{
}

// 0x442410
ShopItemFlame::~ShopItemFlame()
{
}

// 0x442420
void ShopItemFlame::Update(const double thePos[2])
{
	ShopItem::Update(thePos);
	mFlameCounter = (double)Rand() * 4.656612875245797e-10 * 0.2 + mFlameCounter + 0.2;
}

// 0x442460: the cel of the flame's sheet is the counter's whole part, round the sheet
void ShopItemFlame::GetSheetCell(double theCell[2]) const
{
	theCell[0] = 0.0;
	theCell[1] = (double)(ToInt(mFlameCounter) % mImage->mNumCols);
}

// 0x4424c0
ShopItemGift::ShopItemGift(int theKind, double theCubbyY) :
	ShopItem(theKind, theCubbyY),
	mOpenCount(0),
	mOpenTimer(0.0),
	mRollProgress(0.0),
	mPrize(-1),
	mNextPrize(-1),
	mLowestProgress(1.0),
	mGlowTurn(0.0)
{
}

// 0x442530
ShopItemGift::~ShopItemGift()
{
}

// 0x442540
void ShopItemGift::Remove()
{
	mOpenTimer = 1.0;
	mUntouchable = true;
	mOpenCount = mOpenCount + 1;
}

// 0x4425e0
bool ShopItemGift::IsRolling() const
{
	return mPrize != -1 && mRollProgress < 1.0;
}

// 0x442bb0
void ShopItemGift::StartRoll(int thePrize)
{
	mRollProgress = 0.0;
	mPrize = thePrize;
	mNextPrize = RollOtherPrize(thePrize, -1);
}

// 0x442940: the roll runs 1000 updates; the picture it shows changes eighty times, ever more slowly, the last time
// (and with a rising note) to the prize, and then the prize is bought
void ShopItemGift::Update(const double thePos[2])
{
	ShopItem::Update(thePos);
	mGlowTurn = (double)Rand() * 4.656612875245797e-10 * 0.2 + mGlowTurn + 0.2;

	if (0.0 < mOpenTimer)
	{
		double aTimer = mOpenTimer - 0.001;
		if (aTimer < 0.0)
			aTimer = 0.0;
		mOpenTimer = aTimer;
		if (aTimer == 0.0)
		{
			mSpent = true;
			mPurchaseTimer = 1.0;
		}
	}

	if (mPrize == -1)
		return;
	if (1.0 <= mRollProgress)
		return;

	int anOldStep = ToInt(Ease(mRollProgress, 0.0, 0.1) * 80.0);
	double aProgress = mRollProgress + 0.001;
	if (1.0 < aProgress)
		aProgress = 1.0;
	mRollProgress = aProgress;
	int aStep = ToInt(Ease(mRollProgress, 0.0, 0.1) * 80.0);

	if (aStep != anOldStep)
	{
		if (aStep >= 75)
		{
			mNextPrize = mPrize;
			if (mRollProgress < mLowestProgress)
				mLowestProgress = mRollProgress;
		}
		else if (aStep == 74)
			mNextPrize = RollOtherPrize(mNextPrize, mPrize);
		else
			mNextPrize = RollOtherPrize(mNextPrize, -1);

		if (aStep <= 75)
		{
			SoundInstance* aSound = gApp->mSoundManager->GetSoundInstance(SOUND_HINT_POPUP);
			if (aSound != nullptr)
			{
				if (aStep == 75)
					aSound->AdjustPitch(50.0);
				else
					aSound->AdjustPitch(8.0 - (double)aStep * 0.1);
				aSound->Play(false, true);
			}
		}
	}

	if (mRollProgress == 1.0)
		Purchase(0.0, 0, mPrize);
}

// 0x4426a0: the item, and while a roll runs the picture that it shows now, in a turning light that grows as the roll nears its end
void ShopItemGift::Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime)
{
	ShopItem::Draw(g, theX, theY, theHover, theFlashTime);
	if (mPrize == -1 || mSpent)
		return;

	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	int anAlpha = 255 - ToInt(mRollProgress * 1020.0);
	if (anAlpha < 0)
		anAlpha = 0;
	g->SetColor(Color(255, 255, 255, anAlpha));

	if (mLowestProgress < mRollProgress)
	{
		int aLightAlpha = ToInt((mRollProgress - mLowestProgress) / (1.0 - mLowestProgress) * 255.0);
		g->SetColor(Color(255, 255, 255, aLightAlpha));
		Image* aLight = IMAGE_DIVINELIGHT_SMALL;
		int aHalfWidth = aLight->mWidth / 2;
		int aHalfHeight = aLight->mHeight / 2;
		double aTurn = (double)mAge * 0.003;
		g->DrawImageRotated(aLight, ToInt(theX - (double)aHalfWidth), ToInt(((theY - mHeight) - 30.0) - (double)aHalfHeight), aTurn,
			aHalfWidth, aHalfHeight, nullptr);
	}
	g->SetColorizeImages(false);

	// the picture that the roll shows
	int aShown = mNextPrize;
	double anAnchor[2];
	GetShopItemAnchor(anAnchor, aShown);
	double aDrawX = theX - anAnchor[0];
	double aDrawY = (theY - anAnchor[1]) - mHeight;
	Image* anImage = GetShopItemImage(aShown);
	double aCellCol;
	double aCellRow;
	if (aShown == 0x1f)
	{
		aCellRow = 0.0;
		aCellCol = (double)(ToInt(mGlowTurn) % anImage->mNumCols);
	}
	else
	{
		double aCell[2];
		GetShopItemCell(aCell, aShown);
		aCellRow = aCell[0];
		aCellCol = aCell[1];
	}
	Rect aSrcRect = anImage->GetCelRect(ToInt(aCellCol), ToInt(aCellRow));
	g->DrawImage(anImage, ToInt(aDrawX), ToInt(aDrawY), aSrcRect);
}

// 0x442bf0
std::string ShopItemGift::SaveExtra()
{
	ValueList aList;
	aList.AddInt(mOpenCount);
	aList.Add(mOpenTimer);
	aList.Add(mRollProgress);
	aList.AddInt(mPrize);
	aList.AddInt(mNextPrize);
	aList.Add(mLowestProgress);
	return aList.ToString();
}
