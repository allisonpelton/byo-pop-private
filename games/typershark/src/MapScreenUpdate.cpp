// MapScreenUpdate.cpp: MapScreen::Update (0x45ad20). See MapScreen.cpp for the rest of the class and the quirks.

#include "MapScreen.h"
#include "WinWSApp.h"
#include "UserProfile.h"
#include "UserProfileMgr.h"
#include "WinGameDialog.h"
#include "XWidget.h"
#include "TooltipWidget.h"
#include "WSTimer.h"
#include "HyperlinkWidget.h"
#include "WidgetManager.h"
#include "GameText.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// 0x45ad20 slot 17. Nothing runs while the screen is disabled. The final scroll (SetFinalLevel): the map's targets count down
// by one a tick, first x and then y, until both are at the end; then the profile's "games completed" goes up (once per game)
// and the win dialog opens. The prompt slides in from the right (2 pixels a tick) unless a message box is up; the ship
// frame turns every 11 ticks.
//
// Then, unless the map is on the final panel, the ship's site is looked at: the number of its links that lead to unvisited
// sites is counted. With exactly one (or a link of -1) the ship simply takes it: a chest A is put on that site, the route
// to it is added, the "continue" link shows and the level is chosen at once (mLevel). With none or several, a chest (the
// letters A, B, C from the left to the right) is put on every unvisited site, the dots of the ways are made for the screen to
// show, and the screen waits for a click or a letter. The hint box is shown once for a player who has not seen it.
// Last the ship's timer: every 500 ms the ship moves one dot along the route (a sound each), and on the last dot the arrival
// bell rings (up to three times); Gumbo blinks every mGumboPeriod ticks.
void MapScreen::Update()
{
	if (mDisabled != false)
		return;

	Widget::Update();
	int anEnd = mApp->mWidth - mApp->mImages[IMAGE_MAPBG]->mWidth;
	if (mScrolling != false && mScrollX >= anEnd && mScrollY > anEnd)
	{
		mFinalScrollStarted = true;
		mContinueLink->SetVisible(false);
		mContinueLink->SetDisabled(true);
		int aScrollX = mScrollX - 1;
		mScrollX = aScrollX;
		if (aScrollX <= anEnd)
		{
			int aScrollY = mScrollY - 1;
			mScrollX = anEnd;
			mScrollY = aScrollY;
			if (aScrollY <= anEnd)
			{
				mScrollY = anEnd;
				if (mApp->mUser->mCompletionPending != false)
				{
					mApp->mUser->mGamesCompleted = mApp->mUser->mGamesCompleted + 1;
					mApp->mUser->mCompletionPending = false;
					mApp->mProfileMgr->WriteUserProfileToRegistry(mApp->mUser, false, false);		// 0x41f1d0
				}
				WinGameDialog* aDialog = new WinGameDialog(mApp);		// 0x43a4f0
				aDialog->mTime = mApp->mTypingTimeSoFar;
				aDialog->mMistakes = mApp->mLettersWrongSoFar;
				aDialog->mCharsTyped = mApp->mLettersRightSoFar;
				aDialog->Resize(0x5a, 0, 0x1cc, aDialog->GetPreferredHeight(4));
				mApp->AddDialog(DIALOG_WIN_GAME, aDialog);
				mScrolling = false;
			}
		}
	}

	if (mPromptX > mPromptEndX && mApp->GetDialog(DIALOG_MESSAGE) == NULL)
	{
		MarkDirty();
		mPromptX = mPromptX - 2;
	}

	if (mUpdateCnt % 11 == 0)
	{
		MarkDirty();
		mShipFrame = mShipFrame + 1;
		if (mShipFrame >= 6)
			mShipFrame = 0;
	}

	if (mFinal == false)
	{
		// 0x45af60: count the links of the ship's site that lead to unvisited sites. A link of -1 (the whirlpool's) ends the
		// count and takes the single-way path.
		int anUnvisited = 0;
		unsigned int aLink = 0;
		bool aSingleWay = false;
		while (true)
		{
			MapNode* aLast = mRouteNodes.back();
			if (aLink >= aLast->mLinks.size())
				break;
			if (aLast->mLinks[aLink] == -1)
			{
				aSingleWay = true;
				break;
			}
			if (mNodesById[mRouteNodes.back()->mLinks[aLink]]->mVisited == false)
				anUnvisited = anUnvisited + 1;
			aLink++;
		}

		if (aSingleWay != false || anUnvisited == 1)
		{
			// 0x45b00f: one way on
			if (mChoicePending != false)
			{
				mLevelChosen = true;
				MapNode* aLast = mRouteNodes.back();
				mDotsDone = mRouteDots.size();
				MapAddRoutePoints(aLast, mNodesById[aLast->mLinks[0]], mRouteDots);		// 0x4537b0
				mRouteNodes.push_back(mNodesById[aLast->mLinks[0]]);
				mLevel = mRouteNodes.back()->mId;
				mChosenLetter = 'A';
				mTimer->SetDisabled(false);
				mChoicePending = false;

				MapNode* aNode = mRouteNodes.back();
				XWidget* aChest;
				if (mApp->mUser->mVisitedLevels[aNode->mId] != 0)
					aChest = new XWidget(mApp, this, mApp->mImages[IMAGE_MAPCHEST_A_OPEN], 'A');
				else
					aChest = new XWidget(mApp, this, mApp->mImages[IMAGE_MAPCHEST_A], 'A');
				MapSetChestTip(aChest, aNode->mText);		// 0x4506a0
				aChest->mLevel = aNode->mId;
				Image* aImage = mApp->mImages[IMAGE_MAPCHEST_A];
				aChest->Resize(aNode->mX - aImage->mWidth / 2, aNode->mY - aImage->mHeight / 2, aImage->mWidth, aImage->mHeight);
				mApp->mWidgetManager->AddWidget(aChest);
				aChest->SetVisible(true);
				mChests.push_back(aChest);
				unsigned int i = 0;
				while (i < mChests.size())
				{
					mApp->mWidgetManager->BringToFront(mChests[i]->mTooltip);
					i++;
				}
				mApp->mWidgetManager->BringToFront(aChest);
				mHeadingOn = true;
				if (mHintPending != false)
				{
					mHintPending = false;
					mApp->mUser->mOpenChestHintSeen = true;
					mApp->mProfileMgr->WriteUserProfileToRegistry(mApp->mUser, false, true);		// 0x41f1d0
					mApp->ShowMessageDialog(GT(0x00531464), GT(0x00531470), GT(0x0052b4d0), true);		// 0x41bd80
				}
			}
		}
		else
		{
			// 0x45b372: no way or several: a chest on each unvisited site
			if (mChoicePending != false)
			{
				mLevelChosen = false;
				mDotsDone = mRouteDots.size();
				unsigned int aWay = 0;
				while (true)
				{
					MapNode* aLast = mRouteNodes.back();
					if (aWay >= aLast->mLinks.size())
						break;
					if (mNodesById[mRouteNodes.back()->mLinks[aWay]]->mVisited == false)
						MapAddRoutePoints(mRouteNodes.back(), mNodesById[mRouteNodes.back()->mLinks[aWay]], mForkRouteDots);		// 0x4537b0
					aWay++;
				}

				aWay = 0;
				while (true)
				{
					MapNode* aLast = mRouteNodes.back();
					if (aWay >= aLast->mLinks.size())
						break;
					if (mNodesById[mRouteNodes.back()->mLinks[aWay]]->mVisited == false)
					{
						MapNode* aNode = mNodesById[mRouteNodes.back()->mLinks[aWay]];
						XWidget* aChest;
						if (mApp->mUser->mVisitedLevels[aNode->mId] != 0)
							aChest = new XWidget(mApp, this, mApp->mImages[IMAGE_MAPCHEST_A_OPEN + aWay], (char)(aWay + 'A'));
						else
							aChest = new XWidget(mApp, this, mApp->mImages[IMAGE_MAPCHEST_A + aWay], (char)(aWay + 'A'));
						MapSetChestTip(aChest, aNode->mText);		// 0x4506a0
						aChest->mLevel = aNode->mId;
						aChest->mAlpha = 0x80;
						Image* aImage = mApp->mImages[IMAGE_MAPCHEST_A];
						aChest->Resize(aNode->mX - aImage->mWidth / 2, aNode->mY - aImage->mHeight / 2, aImage->mWidth, aImage->mHeight);
						mApp->mWidgetManager->AddWidget(aChest);
						aChest->SetVisible(true);
						mChests.push_back(aChest);
						mApp->mWidgetManager->BringToFront(aChest);
					}
					aWay++;
				}

				// 0x45b72d: with two or more chests, the letters and pictures are given from the left to the right by a
				// selection of the three smallest x (the original's way; more than three would not be ordered)
				if (mChests.size() >= 2)
				{
					int aBestX = 9999;
					int aSlots[3];
					aSlots[0] = 0;
					aSlots[1] = 1;
					aSlots[2] = 2;
					int aSecondX = 9999;
					int aSecond = 1;
					unsigned int aChestIndex = 0;
					while (true)
					{
						if (aChestIndex >= mChests.size())
							break;
						int aX = mChests[aChestIndex]->mX;
						if (aX < aBestX)
						{
							aSlots[2] = aSecond;
							aSecond = aSlots[0];
							aSecondX = aBestX;
							aSlots[0] = aChestIndex;
							aBestX = aX;
							aChestIndex++;
						}
						else if (aX < aSecondX)
						{
							aSlots[2] = aSecond;
							aSecond = aChestIndex;
							aSecondX = aX;
							aChestIndex++;
						}
						else
						{
							aSlots[2] = aChestIndex;
							aChestIndex++;
						}
					}
					aSlots[1] = aSecond;

					unsigned int aRank = 0;
					while (true)
					{
						if (aRank >= mChests.size())
							break;
						XWidget* aChest = mChests[aSlots[aRank]];
						if (mApp->mUser->mVisitedLevels[aChest->mLevel] != 0)
							aChest->mImage = mApp->mImages[IMAGE_MAPCHEST_A_OPEN + aRank];
						else
							aChest->mImage = mApp->mImages[IMAGE_MAPCHEST_A + aRank];
						mChests[aSlots[aRank]]->mLetter = (char)(aRank + 'A');
						aRank++;
					}
				}

				unsigned int i = 0;
				while (i < mChests.size())
				{
					mApp->mWidgetManager->BringToFront(mChests[i]->mTooltip);
					i++;
				}
				mHeadingOn = false;
				mChoicePending = false;
				mContinueLink->SetVisible(false);
				if (mHintPending != false)
				{
					mHintPending = false;
					mApp->mUser->mOpenChestHintSeen = true;
					mApp->mProfileMgr->WriteUserProfileToRegistry(mApp->mUser, false, true);		// 0x41f1d0
					mApp->ShowMessageDialog(GT(0x00531464), GT(0x005313b8), "", true);		// 0x41bd80 (the button text is the empty string at 0x5297a0)
				}
			}
		}
	}

	// 0x45b99d: the ship's step
	if (mTimer->Elapsed() >= 500)
	{
		mDotIndex = mDotIndex + 1;
		if (mDotIndex < (int)mRouteDots.size())
		{
			MarkDirty();
			mTimer->Reset();
			mApp->PlaySample(SOUND_SHIPMOVE);
		}
		else
		{
			mDotIndex = mRouteDots.size();
			if (mLevel != -1)
				mShipArrived = true;
			mArrivals = mArrivals + 1;
			if (mArrivals <= 3)
			{
				mApp->PlaySample(SOUND_DIVEBELL1);
				mTimer->Reset();
			}
		}
	}

	if (mUpdateCnt % mGumboPeriod == 0)
		mGumboFrame = ((mGumboFrame != 0) - 1) & 6;
}
