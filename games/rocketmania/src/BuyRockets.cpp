// The buy-rockets screen (BuyRocketsWidget, 0x401010-0x402350). See BuyRockets.h and docs/GAME_NOTES.md.

#include "BuyRockets.h"
#include "Board.h"
#include "RocketApp.h"
#include "MusicInterface.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "WidgetManager.h"

using namespace Sexy;

// the board keeps the coins that were collected at +0xa8
#define BOARD_COINS(theBoard) ((theBoard)->m8c[7])

// the coins fly for 0x37 updates, the lit rocket stays until 0x55
#define COIN_FLIGHT 0x37
#define ITEM_END 0x55

// 0x401010
BuyRocketsWidget::BuyRocketsWidget(RocketApp* theApp, bool theGameOver) :
	mStartTick(0),
	mApp(theApp),
	mUpgrades(0),
	mGameOver(theGameOver),
	mAllFull(false),
	mSkipWait(false)
{
	Board* aBoard = mApp->mBoard;
	mStartTick = mUpdateCnt;
	if (!mGameOver)
	{
		// (the talking head goes to rest, 0x416520 with 0: the head is not ported yet)
		aBoard->mEffects->Clear();      // 0x41aae0
	}

	PieceGenerator* aGen = aBoard->mGen;
	mAllFull = aGen->AllMetersFull();
	if (!mAllFull && !mGameOver)
	{
		// every five coins buy an upgrade of a launcher; its coins fly one after the other, 50 updates apart
		mUpgrades = aGen->Upgrades(BOARD_COINS(aBoard), 0, -1);
		for (int anUpgrade = 0; anUpgrade < mUpgrades; anUpgrade++)
		{
			int aLauncher = aGen->Upgrades(BOARD_COINS(aBoard), 1, anUpgrade);
			int anEndX = Board::CellX(aBoard->mCols);
			int anEndY = Board::CellY(aLauncher);
			int aCoin = 0;
			do
			{
				Item* anItem = new Item(aCoin * -0x32);
				aCoin++;
				anItem->mStartX = 0x8c;
				anItem->mStartY = 0x18d;
				anItem->mEndX = anEndX + 10;
				anItem->mEndY = anEndY - 10;
				anItem->mImage = (int) aGen->Fuse(aLauncher) + 0x36;
				if (aCoin == 5)
					anItem->mLauncher = aLauncher;
				else
					anItem->mLauncher = -1;
				anItem->mPoints = 0;
				mItems.push_back(anItem);
			} while (aCoin < 5);
		}
	}
	else
	{
		// the coins turn into points: up to 15 fly to the score, 14 updates apart; the first carries all the points
		mUpgrades = 0;
		int aCoin = 0;
		if (0 < BOARD_COINS(aBoard))
		{
			do
			{
				if (14 < aCoin)
					break;
				Item* anItem = new Item(aCoin * -0xe);
				anItem->mStartX = 0x8c;
				anItem->mStartY = 0x18d;
				anItem->mEndX = 0x3c;
				anItem->mEndY = 0x2d;
				anItem->mImage = 0;
				anItem->mLauncher = -1;
				if (aCoin == 0)
					anItem->mPoints = BOARD_COINS(aBoard) * 500;
				else
					anItem->mPoints = 0;
				mItems.push_back(anItem);
				aCoin++;
			} while (aCoin < BOARD_COINS(aBoard));
		}
	}

	// m100: 1 while the screen waits for the coins (or for its help to be read), 2 at the end of a game
	if ((mUpgrades == 0) && !mAllFull)
	{
		if (!mGameOver)
		{
			aBoard->m100 = 1;
			// the help for the coins shows in the first levels when the hints are on; otherwise there is nothing to wait for
			if ((aBoard->mLevel != 0) && !mApp->mNoHints && (aBoard->mLevel < 6))
				return;
			mSkipWait = true;
			return;
		}
	}
	else if (!mGameOver)
	{
		aBoard->m100 = 1;
		return;
	}
	aBoard->m100 = 2;
}

// 0x401400: when it goes away between two levels the level-up song gives way to the game's
BuyRocketsWidget::~BuyRocketsWidget()
{
	if (!mGameOver)
	{
		mApp->mMusicInterface->FadeOut(SONG_LEVELUP, true, 0.02);
		mApp->mMusicInterface->FadeIn(SONG_GAME, -1, 0.333, false);
	}
	for (std::list<Item*>::iterator anItr = mItems.begin(); anItr != mItems.end(); ++anItr)
		delete *anItr;
}

// 0x402070: the coins fly, land (a coin is taken from the board's count when it leaves, the upgrade or the points are given
// when it lands), and when nothing is left the next level begins.
void BuyRocketsWidget::Update()
{
	Board* aBoard = mApp->mBoard;
	if (!aBoard->mHasFocus)
		return;

	Widget::Update();
	for (std::list<Item*>::iterator anItr = mItems.begin(); anItr != mItems.end(); )
	{
		Item* anItem = *anItr;
		if (anItem->mTime == 0)
		{
			if (!mAllFull && !mGameOver)
			{
				BOARD_COINS(aBoard)--;
				mApp->PlaySample(SOUND_COIN_LEAVING_TREASURE_CHEST_05);
			}
			else
			{
				BOARD_COINS(aBoard) = 0;
				mApp->PlaySample(SOUND_COIN_LEAVING_TREASURE_CHEST_05);
			}
		}
		else if (anItem->mTime == COIN_FLIGHT)
		{
			mApp->PlaySample(SOUND_ROCKET_BEING_BOUGHT_02);
			Effect* anEffect = aBoard->mEffects->Add(0xb, true);
			anEffect->mLife = 1;
			anEffect->mc0 = 0;
			anEffect->mX = (double) anItem->mEndX;
			anEffect->mY = (double) anItem->mEndY;
			if (anItem->mLauncher != -1)
			{
				aBoard->mGen->RaiseMeter(anItem->mLauncher);
				Effect* aSparkle = aBoard->mEffects->Add(0xc, true);
				aSparkle->mLife = 3;
				aSparkle->mc0 = 0;
				aSparkle->mX = (double) (anItem->mEndX + 0x14);
				aSparkle->mY = (double) (anItem->mEndY - 10);
				mApp->PlaySample(SOUND_MAGIC_ROCKET_UPGRADED_03);
			}
			if ((mAllFull || mGameOver) && (anItem->mPoints != 0))
				aBoard->AddScore(anItem->mPoints);
		}

		anItem->mTime++;
		if (anItem->mTime == ITEM_END)
		{
			anItr = mItems.erase(anItr);
			delete anItem;
		}
		else
			++anItr;
	}

	if ((mUpgrades == 0) && !mAllFull && !mGameOver)
	{
		if (((mUpdateCnt - mStartTick) < 0xfb) && !mSkipWait)
			return;
		aBoard->m100 = 0;
	}
	else
	{
		if (!mItems.empty())
			return;
		aBoard->m100 = 0;
		if (mGameOver)
		{
			// the game's end: nothing follows (the board keeps no pointer to this screen)
			mApp->mWidgetManager->RemoveWidget(this);
			mApp->SafeDeleteWidget(this);
			return;
		}
	}
	aBoard->NextLevel();
}

// one line of the help, centred on the fourth column
void BuyRocketsWidget::DrawCentered(Graphics* g, const SexyString& theText, int theY)
{
	g->DrawString(theText, Board::CellX(3) - g->GetFont()->StringWidth(theText) / 2, theY);
}

// 0x401530: the help, or the words for what is happening, and the coins in flight and the rockets they light
void BuyRocketsWidget::Draw(Graphics* g)
{
	Board* aBoard = mApp->mBoard;
	if (!aBoard->mHasFocus)
		return;

	g->SetFont(mApp->mFonts[FONT_DOMO22_BOLD]);
	g->SetColor(Color(0xff, 0xff, 0xff));
	if (!mAllFull)
	{
		if (!mGameOver)
		{
			if (mUpgrades == 0)
			{
				if (!mSkipWait)
				{
					DrawCentered(g, GT(0x004e41c4), 0x96);
					DrawCentered(g, GT(0x004e41b4), 0xb9);
					DrawCentered(g, GT(0x004e41a8), 0xdc);
					DrawCentered(g, GT(0x004e4198), 0xff);
				}
				aBoard->mEffects->Draw(g, false);       // 0x41abb0: every way through ends with the board's effects
				return;
			}

			DrawCentered(g, GT(0x004e418c), 0x96);
			DrawCentered(g, GT(0x004e4180), 0xb9);
			// a coin flies along a path that is straight across and eased (it slows down) up or down; when it has landed
			// the rocket it upgrades glows (added to the picture) above its launcher until the item goes
			for (std::list<Item*>::iterator anItr = mItems.begin(); anItr != mItems.end(); ++anItr)
			{
				Item* anItem = *anItr;
				if ((0 <= anItem->mTime) && (anItem->mTime <= COIN_FLIGHT))
				{
					double aFraction = (double) anItem->mTime * 0.01818181818181818;
					double anEase = 1.0 - (1.0 - aFraction) * (1.0 - aFraction);
					int aY = (int) ((double) (anItem->mEndY - anItem->mStartY) * anEase) + anItem->mStartY;
					int aX = (int) ((double) (anItem->mEndX - anItem->mStartX) * aFraction) + anItem->mStartX;
					g->DrawImage(mApp->mImages[IMAGE_COINBRONZE], aX, aY);
				}
				if (COIN_FLIGHT < anItem->mTime)
				{
					g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
					g->DrawImage(mApp->mImages[anItem->mImage], anItem->mEndX - 9, anItem->mEndY - 0x43);
					g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				}
			}
			aBoard->mEffects->Draw(g, false);
			return;
		}
	}
	else if (!mGameOver)
	{
		DrawCentered(g, GT(0x004e4178), 0xb9);
		DrawCentered(g, GT(0x004e4170), 0xdc);
	}

	// the coins that turn into points (the same path, without the glow)
	for (std::list<Item*>::iterator anItr = mItems.begin(); anItr != mItems.end(); ++anItr)
	{
		Item* anItem = *anItr;
		if ((0 <= anItem->mTime) && (anItem->mTime <= COIN_FLIGHT))
		{
			double aFraction = (double) anItem->mTime * 0.01818181818181818;
			double anEase = 1.0 - (1.0 - aFraction) * (1.0 - aFraction);
			int aY = (int) ((double) (anItem->mEndY - anItem->mStartY) * anEase) + anItem->mStartY;
			int aX = (int) ((double) (anItem->mEndX - anItem->mStartX) * aFraction) + anItem->mStartX;
			g->DrawImage(mApp->mImages[IMAGE_COINBRONZE], aX, aY);
		}
	}
	aBoard->mEffects->Draw(g, false);
}
