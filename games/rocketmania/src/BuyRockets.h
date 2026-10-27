// The buy-rockets screen of Rocket Mania Deluxe: the widget that the board puts over itself between two levels (and at the
// end of a game). See docs/GAME_NOTES.md.

#ifndef __ROCKETMANIA_BUYROCKETS_H__
#define __ROCKETMANIA_BUYROCKETS_H__

#include "Widget.h"
#include <list>

namespace Sexy
{

class RocketApp;

// 0x401010, 0x74 bytes: not a dialog, a widget with the board's size over the board. Between two levels the coins that were
// collected buy upgrades for the rockets: for every five coins a launcher's meter goes up by one (the launchers are taken in
// an order, the lowest meters first), and each purchase is five coins that fly from the box of the coins to the launcher,
// one after the other. When every meter is full the coins turn into 500 points each instead, and so they do at the end of a
// game (the arguments of the constructor: the game is over). With no coins to spend it shows the help for the coins, for
// the first levels, and waits 250 updates; then (and when the last coin has landed) it goes on to the next level (0x428bc0).
// It only runs while the board has the focus (the board pauses the screen when a dialog is up).
class BuyRocketsWidget : public Widget
{
public:
	// 0x30 bytes: one coin on its way
	struct Item
	{
		int			mTime;          // +0x00 counts up from a negative number (the delay); the coin flies from 0 to 0x37, lands
		                            //       at 0x37 and the lit rocket stays until 0x55
		int			mStartX;        // +0x04 the box of the coins
		int			mStartY;        // +0x08
		int			mEndX;          // +0x0c the launcher (or the score)
		int			mEndY;          // +0x10
		                            // (+0x18, +0x1c: a number that the first constructor sets and nothing reads)
		int			mLauncher;      // +0x20 the launcher that the coin pays for the upgrade of (the fifth coin), else -1
		int			mImage;         // +0x24 the picture of the rocket that is lit when the coin lands
		int			mPoints;        // +0x28 the points that the landing adds
		Item(int theTime) : mTime(theTime), mStartX(0), mStartY(0), mEndX(0), mEndY(0), mLauncher(-1), mImage(0), mPoints(0) {}
	};

	int					mStartTick;     // +0x58 mUpdateCnt when it was made
	RocketApp*			mApp;           // +0x5c
	std::list<Item*>	mItems;         // +0x64 (0x68 the size)
	int					mUpgrades;      // +0x6c the number of upgrades that the coins buy
	bool				mGameOver;      // +0x70 the game is over: coins turn into points
	bool				mAllFull;       // +0x71 every launcher's meter is full
	bool				mSkipWait;      // +0x72 there is nothing to show and nothing to buy: go on at once

public:
	BuyRocketsWidget(RocketApp* theApp, bool theGameOver);
	virtual ~BuyRocketsWidget();

	virtual void		Update() override;              // 0x402070
	virtual void		Draw(Graphics* g) override;     // 0x401530

private:
	void				DrawCentered(Graphics* g, const SexyString& theText, int theY);   // x = the middle of the fourth column (Board::CellX(3))
};

}

#endif
