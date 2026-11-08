// Board: the input. The keys and the mouse move the pointer along the bottom of the field (the column it stands in is
// m198c), take bricks off the stack (PickUp), catch the bricks that fall or fly past it, and shoot the bricks in hand up
// the column (FirePiece). The widgets of the screen come and go with the manager (AddedToManager and RemovedFromManager).
//
// Frame: these are the object's own functions, so an offset in the original's code is the Board's offset itself.

#include "Board.h"
#include "AstroButton.h"
#include "AstroPopApp.h"
#include "Brick.h"
#include "Checkbox.h"
#include "BrickKinds.h"
#include "Font.h"
#include "GameText.h"
#include "HyperlinkWidget.h"
#include "Image.h"
#include "Res.h"
#include "SexyAppBase.h"
#include "UserProfile.h"
#include "WidgetManager.h"

using namespace Sexy;

// The rectangles of the screen that the pointer is tested against (static data of the original at 0x59da64 and
// 0x59da74), as left, top, right and bottom, both edges included
static const int kStartRect[4] = {18, 225, 126, 253};       // the button that starts the play
static const int kPauseRect[4] = {-4, 194, 148, 296};       // the box of the pause hint (x, y, 152 wide, 102 high)

// the rectangle of the button of the field (0x59da84): x, y, width, height
static const int kFieldButtonRect[1][4] = {{29, 437, 118, 43}};

// 0x4328a0: is the point inside the rectangle (the edges count)
static bool PointInRect(int x, int y, const int theRect[4])
{
	return x >= theRect[0] && x <= theRect[2] && y >= theRect[1] && y <= theRect[3];
}

// the kinds that are used up in one piece (they are never taken in a stack of bricks)
static bool IsToolKind(int theKind)
{
	return theKind == 6 || theKind == 8 || theKind == 10 || theKind == 0xe || theKind == 0xb || theKind == 0xd;
}

//////////////////////////////////////////////////////////////////////////
// The widgets

// 0x459820: the Board has been added to the manager: its buttons and the overlay come with it
void Board::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);

	int i = 0;
	do
	{
		AstroButton* aButton = new AstroButton(0xfa1 + i, mApp);
		aButton->mButtonImage = NULL;
		if (i == 0)
		{
			aButton->mOverImage = IMAGE_MM_OVER_UI;
			aButton->mDownImage = IMAGE_MM_DOWN_UI;
		}
		aButton->Resize(Rect(kFieldButtonRect[i][0], kFieldButtonRect[i][1], kFieldButtonRect[i][2], kFieldButtonRect[i][3]));
		theManager->AddWidget(aButton);
		theManager->BringToFront(aButton);
		m1c0[i] = aButton;
		i++;
	} while (i < 1);

	AstroButton* aHelp = new AstroButton(0xfa6, mApp);
	m2ec = aHelp;
	aHelp->mButtonImage = NULL;
	aHelp->mOverImage = IMAGE_UI_INGAME_HELP_OVER;
	aHelp->mDownImage = IMAGE_UI_INGAME_HELP_DOWN;
	aHelp->Resize(0, 0x1b5, 0x1d, 0x2b);
	theManager->AddWidget(m2ec);
	theManager->BringToFront(m2ec);

	theManager->AddWidget(m74c);
	m74c->SetVisible(false);
	theManager->AddWidget(m750);
	m750->SetVisible(false);
	theManager->AddWidget(&mOverlay);
	theManager->BringToFront(&mOverlay);
	SetOverlayHidden(true);
	theManager->SetFocus(this);

	if (m8dc)
	{
		// the demo invites the player to start a game
		HyperlinkWidget* aLink = new HyperlinkWidget(0xfa7, mApp);
		m2e0 = aLink;
		aLink->mUnderlineSize = 1;
		aLink->SetFont(GetFontById(0xb1));
		aLink->mColor = Color(0xff, 0xff, 0xff);
		aLink->mOverColor = Color(200, 200, 200);
		aLink->mLabel = GT(0x005654ec);
		int aWidth = aLink->mFont->StringWidth(aLink->mLabel);
		int aHeight = aLink->mFont->GetHeight();
		aLink->Resize(0x182 - aWidth / 2, 0x14c, aWidth + 10, aHeight + 10);
		theManager->AddWidget(aLink);
		aLink->mDoFinger = true;
	}

	mApp->SetCursor(0);
}

// 0x43a5d0: the Board has been taken out of the manager, and its widgets with it
void Board::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);

	// (the widget at +0x754 is never assigned)
	if (!m1c0.empty() && m1c0[0] != NULL)
		theManager->RemoveWidget(m1c0[0]);
	if (m2e0 != NULL)
		theManager->RemoveWidget(m2e0);
	if (m2ec != NULL)
		theManager->RemoveWidget(m2ec);
	if (m2e8 != NULL)
		theManager->RemoveWidget(m2e8);
	if (m2e4 != NULL)
		theManager->RemoveWidget(m2e4);
	if (m74c != NULL)
		theManager->RemoveWidget(m74c);
	if (m750 != NULL)
		theManager->RemoveWidget(m750);
	theManager->RemoveWidget(&mOverlay);
}

//////////////////////////////////////////////////////////////////////////
// The first move

// (inlined in the Return key, the middle button and the click on the start button) the player's first move: it starts
// the play when nothing is falling or flying, or tells the player that it cannot be done now
void Board::StartByHand()
{
	if (!m910 && CountMovingBricks(false) == 0 && m94.empty() && m7c.empty() && m1990 == 0)
	{
		if (m1c08 && !m917)
			mApp->ShowPowerDialog();
		else
			BeginPlay();
	}
	else if (!m90b)
		mApp->PlaySample(SOUND_CANT_CLICK);
}

//////////////////////////////////////////////////////////////////////////
// The keys

// 0x45c850: the space bar pauses (and unpauses), Z and X move the pointer one column to the left and to the right
void Board::KeyChar(SexyChar theChar)
{
	if (mApp->mHelpScreen != NULL)
		return;

	if (theChar == ' ')
	{
		bool aFlag = false;
		if (!m230.empty() && m230[0]->m1c && !m8ea && !(m7e8 > -m7ec))
		{
			aFlag = true;
			if (m1924 > 0 && !m919)
				return;
		}

		if (!mOverlay.mFlag78 && (aFlag || !m919 || m1924 < 1))
			Pause(m1924 == 0, true);
	}
	else if (theChar == 'z' || theChar == 'Z')
	{
		if (m8dc)
			return;
		int aCol = (m198c - 0xa6) / 0x2c;
		if (aCol >= 1 && aCol <= 9)
			MouseMove(0xa6 + (aCol - 1) * 0x2c, 0x1e0);
	}
	else if (theChar == 'x' || theChar == 'X')
	{
		if (m8dc)
			return;
		int aCol = (m198c - 0xa6) / 0x2c;
		if (aCol >= 0 && aCol <= 8)
			MouseMove(0xa6 + (aCol + 1) * 0x2c, 0x1e0);
	}
}

// 0x473870: the arrows move the pointer (left and right) and take or shoot bricks (down and up), Escape presses the
// button of the field, Return starts the play
void Board::KeyDown(KeyCode theKey)
{
	if (m8eb || m8ed || m8ef || m8f4 || m8ec || mFlag8E9 || mFlag8E8 || m1924 != 0 || m8dc)
		return;

	if (theKey == 0x25)
	{
		int aCol = (m198c - 0xa6) / 0x2c;
		if (aCol - 1 >= 0)
			MouseMove(aCol * 0x2c + 0x7a, m195c);
	}
	else if (theKey == 0x27)
	{
		int aCol = (m198c - 0xa6) / 0x2c;
		if (aCol + 1 < 10)
			MouseMove(aCol * 0x2c + 0xd2, m195c);
	}
	else if (theKey == 0x28)
	{
		m8e5 = true;
		PickUp();
	}
	else if (theKey == 0x26)
		FirePiece();
	else if (theKey == 0x1b)
		mApp->ButtonDepress(0xfa1);
	else if (theKey == 0xd && !mApp->mWidgetManager->mKeyDown[0x12] && !m8e4 && m1bd8 == m1bdc && m1a34 == 0)
		StartByHand();
}

//////////////////////////////////////////////////////////////////////////
// The mouse

// 0x432b20: a drag moves the pointer like a move
void Board::MouseDrag(int x, int y)
{
	Widget::MouseDrag(x, y);
	MouseMove(x, y);
}

// 0x432dc0: the button goes up: it is no longer held (m974 counts the buttons held), and the pointer may take again
void Board::MouseUp(int x, int y, int theClickCount)
{
	Widget::MouseUp(x, y, theClickCount);
	m1958 = x;
	m195c = y;

	if (theClickCount == 1)
		m974 -= 1;
	else if (theClickCount == -1)
		m974 -= 2;
	if (m974 < 0)
		m974 = 0;

	if (!mFlag8E8 && !m90f && (!mFlag8E9 || m8ef) && m1af4 != mUpdateCnt && !m8ec && !m8f4 && (!m8eb || m8ef) && !m8ea &&
		m1924 == 0)
		m8e5 = false;
}

// 0x458170: the middle button starts the play, as the Return key does
void Board::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	(void) x;
	(void) y;
	(void) theClickCount;
	if (mApp->mHelpScreen == NULL && theBtnNum == 2 && (!m8dc || m8dd) && !m8eb && !m8ed && !m8ef && !m8f4 && !m8ec &&
		!mFlag8E9 && !mFlag8E8 && m1924 == 0 && !m8e4 && m1bd8 == m1bdc && m1a34 == 0)
		StartByHand();
}

// 0x478cd0: a button goes down (1 the left, -1 the right). The left one takes bricks, or presses the start button, or
// speeds up the count of a cleared stage; the right one shoots; on the warp screen the two panels are chosen with it.
void Board::MouseDown(int x, int y, int theClickCount)
{
	if (mApp->mHelpScreen != NULL)
		return;

	if (m1924 == 1 && !m919 && (m230.empty() || !m230[0]->m1c))
	{
		// the game is paused: a click goes on with it
		if (mOverlay.mFlag78)
		{
			if (!PointInRect(x, y, kPauseRect))
				return;
			mOverlay.SetFlag78(false);
			mApp->mUser->SetInt(0x2e, 0);
			mApp->mUser->Save();
			m8df = true;
		}
		Pause(false, false);
		return;
	}

	if ((m8dc && !m8dd) || m90f)
		return;

	Widget::MouseDown(x, y, theClickCount);
	m1958 = x;
	m195c = y;
	if (theClickCount == 1)
		m974 = 1;
	else if (theClickCount == -1)
		m974 = 0;

	if (!(!mFlag8E8 && (!mFlag8E9 || m8ef) && !m8ec && !m8f4 && (!m8eb || m8ef) && !m8ea &&
		(m8eb || m8ed || m8ef || m7ec + m7e8 <= 0) && m1924 == 0 && !(mMode == 1 && m1a34 == 1) &&
		!(mMode == 2 && m1a34 == 1) && m1af4 != mUpdateCnt))
		return;

	if (!m913 && !m912 && m910)
	{
		m913 = true;
		Fire(0x28, 0);
		return;
	}

	m8e5 = false;
	m1988 = 0;

	if (!m8ef)
	{
		if (theClickCount == 1)
		{
			if (!m916)
			{
				if (!m8eb && !m8ed && !mFlag8E9 && !m8e4 && m1bd8 == m1bdc && PointInRect(x, y, kStartRect) && m1a34 == 0)
				{
					StartByHand();
					return;
				}
				m8e5 = true;
				PickUp();
				return;
			}

			// a click while the stage is counted speeds the count up
			if (m1b7c < 1 && m1bb0 < 6)
			{
				m1bb0 = 6;
				m1b98 = m1b94;
				int aLeft = (m960 - m1b9c) + m1ba4;
				m1b88 = 0x140;
				m1b8c = 0xff;
				m1ba0 = m1b9c;
				m1ba8 = m1ba4;
				m1b90 = 0x14;
				if (aLeft < 50000)
					m1bb4 = 500;
				else if (aLeft < 100000)
					m1bb4 = 1000;
				else
					m1bb4 = (aLeft >= 200000) ? 4000 : 2000;
			}
		}
		else if (theClickCount == -1 && !m916)
		{
			mApp->PlaySample(SOUND_RIGHT_CLICK);
			FirePiece();
		}
		return;
	}

	if (m1a60 != 1)
		return;

	// the warp screen: one of the two panels picks the stage the game goes on from
	int aGateHeight = IMAGE_EASY_WARP_GATE->GetCelHeight();
	if (!(x < m1a70 || m1a70 + 0x78 < x || m195c < 0xe1 || aGateHeight + 0xf5 < m195c))
	{
		mApp->PlaySample(SOUND_BUTTON_DOWN);
		if (mField193C < m1a04)
		{
			int aCount = m1a04 - mField193C;
			do
			{
				float aSum = 0.125f + m940;
				mField193C++;
				m940 = aSum;
				if (aSum > 6.0f)
					m940 = 6.0f;
				AdvanceLevel(true);
				aCount--;
			} while (aCount != 0);
		}
		m1940 = mField193C / 2 + 1;
		m3d0.mAlpha = 0;
		m3d0.mRed = 0;
		m8f4 = true;
		m8ef = false;
		mOverlay.SetFill(0, m3d0.mGreen, m3d0.mBlue, 0);
	}
	else
	{
		if (m1a08 < 1 || x < m1a74 || m1a74 + 0x78 < x || m195c < 0xe1 || aGateHeight + 0xf5 < m195c)
			return;
		m1b28 = Fn433130(m1a08 / 2 + 1);
		mApp->PlaySample(SOUND_BUTTON_DOWN);
		if (mField193C < m1a08)
		{
			int aCount = m1a08 - mField193C;
			do
			{
				float aSum = 0.5625f + m940;
				mField193C++;
				m940 = aSum;
				if (aSum > 6.0f)
					m940 = 6.0f;
				AdvanceLevel(true);
				aCount--;
			} while (aCount != 0);
		}
		m1940 = mField193C / 2 + 1;
		MarkHintsSeen(m1940);
		m3d0.mAlpha = 0;
		m3d0.mRed = 0;
		m8f4 = true;
		m8ef = false;
		mOverlay.SetFill(0, m3d0.mGreen, m3d0.mBlue, 0);
	}

	SetOverlayHidden(false);
	m928 = 0.98f;
	m92c = 0.98f;
	if (m1a60 == 1)
		m1978 = mScore;
	m1954 = GetImageById(mMode + 0x17b)->mNumCols - 1;
}

// 0x461010: the pointer moves. The column it stands in follows it (but a column that is full up to the ship stops it),
// the cells under it light up, and the boxes of the hints show what the brick under it does.
void Board::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	m1958 = x;
	m195c = y;

	if (m8dc && !m8dd)
		return;
	if (mFlag8E8)
		return;
	if (m901)
		return;
	if (m8ec)
		return;
	if (m916)
		return;
	if (m8f4)
		return;
	if (m8eb && !m8ef)
		return;
	if (m8ea)
		return;
	if (!m8eb && !m8ed && !m8ef && m7ec + m7e8 > 0)
		return;
	if (m1924 != 0 && !mOverlay.mFlag78)
		return;

	if (!m8ef && !mOverlay.mFlag78)
	{
		int aX = x;
		if (aX < 0xa6)
			aX = 0xa6;
		else if (0x25d < aX)
			aX = 0x25d;
		if (0xa5 < m1958 && m1958 < 0x25e)
		{
			m96c = y;
			m968 = m1958;
		}
		int aNewCol = (aX - 0xa6) / 0x2c;
		unsigned int aNewRow = (y < 0x22) ? 0 : (y - 0x22) / 0x19 + 1;
		if (mMode == 3 && m1a34 == 1 && 0xa5 < m1958 && m1958 < 0x25e)
			HoverCell(aNewCol, aNewRow, false);

		int anOldX = m198c;
		if (anOldX + 0x2c <= aX || aX < anOldX)
		{
			// a column that reaches the ship stops the pointer
			int aStopCol = (anOldX - 0xa6) / 0x2c;
			bool aBlocked = false;
			if (aX < anOldX)
			{
				int aTry = anOldX - 1;
				if (aX <= aTry)
				{
					int anOffset = anOldX - 0xa7;
					do
					{
						if (ColumnReachesShip(aTry))
						{
							if ((m198c - 0xa6) / 0x2c != aStopCol)
								m194c = 0;
							aBlocked = true;
							break;
						}
						aStopCol = anOffset / 0x2c;
						aTry -= 0x2c;
						anOffset -= 0x2c;
					} while (aX <= aTry);
				}
				if (!aBlocked)
					m194c = 0;
			}
			else
			{
				int aTry = anOldX;
				int anOffset = anOldX - 0xa6;
				do
				{
					if (ColumnReachesShip(aTry))
					{
						if ((m198c - 0xa6) / 0x2c != aStopCol)
							m194c = 1;
						aBlocked = true;
						break;
					}
					aStopCol = anOffset / 0x2c;
					aTry += 0x2c;
					anOffset += 0x2c;
				} while (aTry <= aX);
				if (!aBlocked)
					m194c = 1;
			}

			if (aBlocked)
			{
				m198c = aStopCol * 0x2c + 0xa6;
				if (!m8eb && !m8ed && !m8ef && !m8f4 && !m8ec && !mFlag8E9 && !mFlag8E8 && m1924 == 0 &&
					PointInRect(m1958, m195c, kStartRect) && m1a34 == 0 && !m910)
				{
					if (m1bdc <= m1bd8 && !m8e4)
						mApp->SetCursor(1);
					m2b0[0]->SetShown(true);
					m2b0[0]->mX = m1958 + 10;
					m2b0[0]->mY = m195c;
					m2b0[1]->SetShown(false);
					m2b0[2]->SetShown(false);
					return;
				}
				mApp->SetCursor(0);
				return;
			}

			// a column that shakes (its stack reaches the top) stops when the pointer leaves it
			int aCol = (m198c - 0xa6) / 0x2c;
			Brick* aTop = m314[0][aCol];
			if (aTop != NULL && aTop->mShake && ColumnDepth(aCol) + 2 < 0x12 && !m917 && !m8ec)
			{
				for (int aRow = 0; aRow < 20; aRow++)
				{
					Brick* aBrick = m314[aRow][aCol];
					if (aBrick == NULL)
						break;
					aBrick->StopShake();
				}
			}

			if (m8db)
			{
				m948 = (unsigned char) (0xc - aNewCol);
				m94c = 0;
				RecordStep(false);
			}
			m198c = aNewCol * 0x2c + 0xa6;
			UpdateKlaxon();
			if (!m912)
			{
				for (size_t i = 0; i < mTargeted.size(); i++)
					mTargeted[i]->StopFlash();
				mTargeted.clear();
				HighlightTargets();
			}
			if (m354 != NULL)
				m354->SetParticleX((float) m198c);
			CheckShipColumn();
		}
	}

	int aGateHeight = IMAGE_EASY_WARP_GATE->GetCelHeight();
	bool aWarp = m8ef;
	if (aWarp)
	{
		// the warp screen: the pointer turns to a hand over the two panels
		if (m1a70 <= m1958 && m1958 <= m1a70 + 0x78 && 0xe0 < m195c && m195c <= aGateHeight + 0xf5)
		{
			m1ab0 = 2;
			m8f1 = false;
			m8f2 = true;
			m8f3 = false;
			mApp->SetCursor(1);
			return;
		}
		if (3 < m1a0c && 0 < m1a74 && m1a74 <= m1958 && m1958 <= m1a74 + 0x78 && 0xe0 < m195c &&
			m195c <= aGateHeight + 0xf5)
		{
			m1ab0 = 3;
			m8f1 = false;
			m8f2 = false;
			m8f3 = true;
			mApp->SetCursor(1);
			return;
		}
	}

	if (mOverlay.mFlag78)
	{
		// the pause hint: the pointer is a hand over its box
		bool anInside = PointInRect(m1958, m195c, kPauseRect);
		int aCursor = mApp->mCursorNum;
		if (anInside)
		{
			if (aCursor == 1)
				return;
			aCursor = 1;
		}
		else
		{
			if (aCursor == 0)
				return;
			aCursor = 0;
		}
		mApp->SetCursor(aCursor);
		MarkDirtyFull();
		return;
	}

	m1ab0 = 0;
	m8f3 = false;
	m8f2 = false;
	m8f1 = false;
	if (!m8eb && !m8ed && !aWarp && !m8f4 && !m8ec && !mFlag8E9 && !mFlag8E8 && m1924 == 0)
	{
		if (PointInRect(m1958, m195c, kStartRect) && m1a34 == 0 && !m910)
		{
			if (m1bdc <= m1bd8 && !m8e4)
				mApp->SetCursor(1);
			m2b0[0]->SetShown(true);
			m2b0[0]->mX = m1958 + 10;
			m2b0[0]->mY = m195c;
			m2b0[1]->SetShown(false);
			m2b0[2]->SetShown(false);
			return;
		}
	}

	if (!m8dc)
		mApp->SetCursor(0);

	if (599 < m1958)
	{
		if (!m910)
		{
			// beyond the field: the box of the stats follows the pointer
			m2b0[1]->SetShown(true);
			m2b0[1]->mX = (m1958 - m2b0[1]->ComputeWidth()) - 4;
			int aBoxY = m195c;
			if (0x1dc < m2b0[1]->ComputeHeight() + aBoxY)
				aBoxY = 0x1dc - m2b0[1]->ComputeHeight();
			m2b0[1]->mY = aBoxY;
			m2b0[0]->SetShown(false);
			m2b0[2]->SetShown(false);
			return;
		}
		if (599 < m1958)
			return;
	}

	m2b0[1]->SetShown(false);
	m2b0[0]->SetShown(false);

	// over the field: the box of the brick under the pointer
	if (m1958 < 0xa6 || 0x25d < m1958)
		return;
	if (m8eb || m8ed || aWarp || m8f4 || m8ec || mFlag8E9 || mFlag8E8 || m1924 != 0)
		return;
	if (m910 && m195c < 0x22)
		return;

	int aCol = (m1958 - 0xa6) / 0x2c;
	unsigned int aRow = (m195c < 0x22) ? 0 : (m195c - 0x22) / 0x19 + 1;
	if (m314.size() <= aRow)
		return;
	if (9 < aCol)
		return;

	HintBox* aBox = m2b0[2];
	Brick* aBrick = m314[aRow][aCol];
	if (aBrick == NULL)
	{
		aBox->SetShown(false);
		return;
	}

	int aKind = aBrick->GetEffectiveType();
	if (aKind < 0 || aKind > 5 || aBrick->IsJammed())
	{
		aBox->mLines.clear();
		aBox->m60 = aCol;
		aBox->m64 = (int) aRow;
		if (aBrick->IsJammed())
		{
			aBox->AddLine(GT(0x00565a70));
			aBox->AddLine(GT(0x00565a60));
		}
		else
		{
			switch (aKind)
			{
			case 6:
				aBox->AddLine(GT(0x00565a44));
				break;
			case 7:
				aBox->AddLine(GT(0x005659b8));
				break;
			case 8:
				aBox->AddLine(GT(0x00565940));
				aBox->AddLine(GT(0x0056592c));
				break;
			case 9:
				if (((SpecialBrick*) aBrick)->mStage != 2)
				{
					aBox->AddLine(GT(0x005659b8));
					break;
				}
				aBox->AddLine(GT(0x005659a0));
				aBox->AddLine(GT(0x0056598c));
				break;
			case 10:
				aBox->AddLine(GT(0x0056590c));
				break;
			case 0xb:
				aBox->AddLine(GT(0x00565a18));
				break;
			case 0xc:
				aBox->AddLine(GT(0x005659fc));
				aBox->AddLine(GT(0x005659dc));
				break;
			case 0xd:
				aBox->AddLine(GT(0x00565a2c));
				break;
			case 0xe:
				aBox->AddLine(GT(0x00565970));
				aBox->AddLine(GT(0x00565958));
				break;
			default:
				break;
			}
		}
	}
	else
	{
		// a plain brick has a box only when a piece of its kind is waiting to drop
		if (!HasMarkOfType(aKind))
		{
			aBox->SetShown(false);
			return;
		}
		aBox->mLines.clear();
		aBox->AddLine(GT(0x005658f4));
	}

	aBox->SetShown(true);
	int aBoxX = m1958;
	if (0x275 < aBox->ComputeWidth() + 10 + aBoxX)
		aBoxX = 0x276 - aBox->ComputeWidth();
	else
		aBoxX += 10;
	aBox->mX = aBoxX;
	aBox->mY = m195c + 0xd;
}

//////////////////////////////////////////////////////////////////////////
// Taking and shooting bricks

// 0x437640: the lowest cell of the column that has been emptied this update (RemoveBrick keeps them in ma8)
CellRef Board::LowestEmptiedCell(int theCol)
{
	CellRef aResult = {-1, -1};
	for (size_t i = 0; i < ma8.size(); i++)
	{
		if (ma8[i].mCol == theCol && ma8[i].mRow > aResult.mRow)
		{
			aResult.mCol = theCol;
			aResult.mRow = ma8[i].mRow;
		}
	}
	return aResult;
}

// 0x4361c0: does the brick match the kind of the brick in hand (a mutator counts as kind 12)
bool Board::MatchesHand(Brick* theBrick)
{
	return TypesMatch(theBrick, m354->mMutated == 1 ? 0xc : m354->mType);
}

// (inlined three times) the brick in hand is born: a copy of the first brick that was taken, at the bottom of the field
void Board::MakeHand(int theType, float theX)
{
	m354 = new Brick(this, mBus, 0);
	m354->Retire();
	m354->SetType(theType);
	m354->SetParticleX(theX);
	m354->SetParticleY(398.0f);
}

// 0x44d8b0 and 0x44dc30, which differ in the list of groups and in one test: the groups of bricks that fly (m7c) or fall
// (m94) in the column of the pointer are caught, the bricks of a group from the last to the first, until the hand is full or
// a brick does not match the hand. theOk is false when the player tried to catch what cannot be caught. The groups of the
// falling list are caught only below the lowest emptied cell of their column (theCheckEmptied).
static bool CatchFromGroups(Board* b, std::list<std::list<Brick*> >& theGroups, bool theCheckEmptied, bool* theOk)
{
	int aCol = (b->m198c - 0xa6) / 0x2c;
	bool aCaught = false;
	bool anOk = true;

	if (!theGroups.empty())
	{
		std::list<std::list<Brick*> >::iterator aGroup = theGroups.end();
		--aGroup;
		for (;;)
		{
			std::list<Brick*>& aBricks = *aGroup;
			// the column of a group is the column of its last brick (a group is never empty)
			if (!aBricks.empty() && aCol == ((int) aBricks.back()->mX - 0xa6) / 0x2c)
			{
				std::list<Brick*>::iterator aNode = aBricks.end();
				--aNode;
				bool aLast = false;
				do
				{
					Brick* aBrick = *aNode;
					if (b->m70.size() > 5)
					{
						anOk = false;
						break;
					}
					if (b->m354 != NULL && !b->MatchesHand(aBrick))
					{
						anOk = false;
						break;
					}
					if (aBrick->mMutated == 1 || aBrick->mType == 7 || aBrick->mType == 0xc || aBrick->mType == 9)
					{
						if (!b->m90b)
							b->mApp->PlaySample(SOUND_CANT_CLICK);
						anOk = false;
						break;
					}
					if (theCheckEmptied)
					{
						CellRef anEmptied = b->LowestEmptiedCell(aCol);
						if (aBrick->mY < (float) (anEmptied.mRow * 0x19))
							break;
					}
					if (b->m354 == NULL)
						b->MakeHand(aBrick->GetEffectiveType(), (float) aCol);

					b->m70.push_back(aBrick);
					aCaught = true;
					b->m1990++;
					aBrick->mFloorY = 423.0f;
					if (aBrick->mY == aBrick->mTargetY && aBrick->mState == 1)
						b->FireTo(aBrick, 0);
					b->FireTo(aBrick, 2);

					std::list<Brick*>::iterator anErase = aNode;
					if (aNode == aBricks.begin())
						aLast = true;
					else
						--aNode;
					aBricks.erase(anErase);
				} while (!aLast);
			}

			if (aBricks.empty())
			{
				aGroup = theGroups.erase(aGroup);
				if (theGroups.empty())
					break;
			}
			if (!anOk)
				break;
			if (aGroup == theGroups.begin())
				break;
			--aGroup;
		}
	}

	*theOk = anOk;
	return aCaught;
}

// 0x44d8b0: the bricks that were fired and fly up the column
bool Board::CatchFrom7c(bool* theOk)
{
	return CatchFromGroups(this, m7c, false, theOk);
}

// 0x44dc30: the bricks that fall in the column
bool Board::CatchFrom94(bool* theOk)
{
	return CatchFromGroups(this, m94, true, theOk);
}

// (the end of 0x472360, at 0x47324a) the sound of the click, and the checks that every pick up ends with
void Board::PickUpDone(bool theFail)
{
	if (!m90b)
		mApp->PlaySample(theFail ? SOUND_CANT_CLICK : SOUND_LEFT_CLICK);
	else if (theFail)
		mApp->PlaySample(SOUND_CANT_CLICK);
	m8e6 = true;
	CheckStackHeight();
	CheckShipColumn();
}

// (inside 0x472360, twice) the psychic mode (mode 3, second game type): the cells that are lit under the pointer are taken
// out together, when nothing moves. theHandEmpty is which of the two places of the function it was (they differ in the
// order of two assignments and in the list of the lit cells being emptied).
void Board::PopPsychic(bool theHandEmpty)
{
	if (!m7c.empty() || m1994 > 0 || CountMovingBricks(false) > 0 || m8e4 || m324.empty())
	{
		if (!m90b)
			mApp->PlaySample(SOUND_CANT_CLICK);
		return;
	}

	if (!theHandEmpty)
		m1aec = 0x19;
	m1ab8 = 7;
	m1ac0 = PickSpawnDelay();

	// the lit cells that hold a brick that is at rest and is not in the cells that are already taken
	std::vector<CellRef> aTargets;
	for (unsigned int i = 0; i < m324.size(); i++)
	{
		int aCol = m324[i].mCol;
		int aRow = m324[i].mRow;
		Brick* aBrick = m314[aRow][aCol];
		if (aBrick != NULL && aBrick->IsIdle() && !HasCell(aCol, aRow, mb8))
		{
			CellRef aCell = {aCol, aRow};
			aTargets.push_back(aCell);
		}
	}
	if (aTargets.empty())
	{
		if (!m90b)
			mApp->PlaySample(SOUND_CANT_CLICK);
		return;
	}

	if (theHandEmpty)
		m1aec = 0x19;
	for (size_t i = 0; i < aTargets.size(); i++)
	{
		Brick* aBrick = m314[aTargets[i].mRow][aTargets[i].mCol];
		if (aBrick->GetEffectiveType() == 9 && ((SpecialBrick*) aBrick)->mStage > 1)
			((SpecialBrick*) aBrick)->Crack(true);
	}

	mApp->PlaySample(SOUND_PSYCHIC);
	m91b = true;
	for (size_t i = 0; i < aTargets.size(); i++)
	{
		int aCol = aTargets[i].mCol;
		int aRow = aTargets[i].mRow;
		Brick* aBrick = m314[aRow][aCol];
		if (aBrick != NULL)
		{
			aBrick->SetParticleImages(IMAGE_MOON_PARTICLE);
			RemoveBrick(NULL, aCol, aRow, false);
		}
	}
	DropUnsupported(ma8);
	if (theHandEmpty)
		m324.clear();
	PickUpDone(false);
}

// 0x472360: the down key and the left button. In the normal game it catches the bricks that fall or fly past the pointer, or
// takes up to six bricks of one kind from the top of the stack in the column; with a tool (the second game type) it is
// the laser (mode 0) or the psychic (mode 3) that takes the bricks out; with bricks in hand it takes more of the same kind.
void Board::PickUp()
{
	bool anOk = true;

	if (!m8e5 || m8ea || m1990 > 0 || (m70.size() >= 6 && m1a34 == 0) ||
		(m1a34 == 1 && (mMode == 1 || mMode == 2)))
	{
		if (!m90b)
			mApp->PlaySample(SOUND_CANT_CLICK);
		return;
	}

	unsigned int aStartSize = (unsigned int) m70.size();
	bool aCaught = false;
	if (m1a34 == 0)
	{
		aCaught = CatchFrom7c(&anOk);
		if (!aCaught && !anOk)
			return;
	}
	if (!aCaught && m1a34 == 0)
		aCaught = CatchFrom94(&anOk);

	if (aCaught)
	{
		if (m8db)
		{
			m948 = 2;
			m94c = (m70.front()->GetEffectiveType() * 0x100 - (int) aStartSize) + (int) m70.size();
			RecordStep(false);
		}
		CheckStackHeight();
		CheckShipColumn();
		mApp->PlaySample(SOUND_LEFT_CLICK);
		return;
	}
	if (!anOk)
		return;

	int aCol = (m198c - 0xa6) / 0x2c;
	CellRef anEmptied = LowestEmptiedCell(aCol);
	if (anEmptied.mCol != -1 || anEmptied.mRow != -1)
	{
		if (!m90b)
			mApp->PlaySample(SOUND_CANT_CLICK);
		return;
	}

	bool aFail = false;
	int aX = m198c;
	if (mMode == 3 && m1a34 == 1)
		aX = m1958;

	int aRow = FindBottomRow(aX, true);
	if (m354 == NULL)
	{
		// ---- nothing in hand
		if (aRow < 1)
		{
			if (mMode == 3 && m1a34 == 1)
				aFail = true;
			PickUpDone(aFail);
			return;
		}

		Brick* aCell = m314[aRow][aCol];
		if (m1a34 == 1 && mMode == 0)
		{
			// the laser takes the brick at the bottom of the column out
			if (!m7c.empty() || m1994 > 0)
				return;
			if (aCell->IsIdle())
			{
				m91c = true;
				m1aec = 0x19;
				int aKind = aCell->GetEffectiveType();
				if ((aKind >= 0 && aKind <= 5 && !aCell->IsJammed()) || aKind == 7 || aKind == 9 ||
					aCell->mMutated == 0 || aCell->mMutated == 1)
				{
					aCell->mFlag187 = true;
					aCell->AdvanceParticleCount();
				}
				RemoveBrick(aCell, aCol, aRow, true);
				DropUnsupported(ma8);
				m420.Reset();
				mApp->PlaySampleAt(SOUND_PLAYER1_LASER, aCol);
				m1ab8 = 7;
				m1ac0 = PickSpawnDelay();
			}
			PickUpDone(false);
			return;
		}
		if (m1a34 == 1 && mMode == 3)
		{
			PopPsychic(true);
			return;
		}

		int aCellKind = aCell->GetEffectiveType();
		if (aCell->mMutated == 1 || aCellKind == 7 || aCellKind == 0xc || aCellKind == 9)
		{
			if (!m90b)
				mApp->PlaySample(SOUND_CANT_CLICK);
			return;
		}
		MakeHand(aCellKind, (float) m198c);

		// the hints of the first stages go on a step (a picture of the stage is shown once it has been taken)
		unsigned int aStage = (unsigned int) mField193C;
		if ((aStage < m220.size() && !m220[aStage]->mf0) || m90a)
		{
			if ((aStage == 1 || m90a) && m220.size() > 1)
			{
				StageBg* aBg = m220[1];
				StageBg* aNext = aBg->mNext;
				if (aNext != NULL && aNext->mNext != NULL && !m8dc)
				{
					delete aBg;
					m220[1] = aNext;
					if (!m210.empty())
						m210[0] = aNext;
				}
			}
			else if (aStage != 1 && !m90a)
				m220[aStage]->m118 = -8;
		}
		if (mField193C < 2 || m90a)
		{
			if (m1a60 == 2 && !m8fc && m1960 == 0)
				m1960 = 0xff;
		}
		else
		{
			if (!m8fc)
			{
				Fire(0x2a, 0);
				Fire(0x28, 0);
				m1960 = 0xff;
			}
			m8fc = true;
			m922 = false;
		}

		// the bricks of the kind, up from the bottom of the stack
		int aRemaining = aRow;
		int aPickRow = aRow;
		bool aStop = false;
		for (;;)
		{
			Brick* aPicked = m314[aPickRow][aCol];
			int aKind = aPicked->GetEffectiveType();
			if (aKind == 7 || aKind == 0xc || aKind == 9)
				break;
			if (IsToolKind(aKind))
				mApp->PlaySample(SOUND_ACTIVATED);
			aPicked->mFloorY = 398.0f;
			m70.push_back(aPicked);
			m314[aPickRow][aCol] = NULL;
			m1990++;
			FireTo(aPicked, 2);
			aPickRow--;
			Brick* aNextCell = m314[aPickRow][aCol];
			aRemaining--;
			if (aRemaining < 1 || aNextCell == NULL || !MatchesHand(aNextCell) || !aNextCell->IsIdle() ||
				IsToolKind(aKind))
				aStop = true;
			if (m70.size() > 5 || aStop)
				break;
		}

		if (m8db)
		{
			m948 = 2;
			m94c = m70.front()->GetEffectiveType() * 0x100 + (int) m70.size();
			RecordStep(false);
		}
		PickUpDone(aFail);
		return;
	}

	// ---- bricks in hand
	if (aRow < 1)
	{
		if (!m90b)
			mApp->PlaySample(SOUND_CANT_CLICK);
		return;
	}

	if (m1a34 == 1 && mMode == 0)
	{
		if (!m7c.empty() || m1994 > 0)
			return;
		Brick* aCell = m314[aRow][aCol];
		if (aCell->IsIdle())
		{
			m91c = true;
			int aKind = aCell->GetEffectiveType();
			if ((aKind >= 0 && aKind <= 5 && !aCell->IsJammed()) || aKind == 7 || aKind == 9 ||
				aCell->mMutated == 0 || aCell->mMutated == 1)
			{
				aCell->mFlag187 = true;
				aCell->AdvanceParticleCount();
			}
			RemoveBrick(aCell, aCol, aRow, true);
			m420.Reset();
			mApp->PlaySampleAt(SOUND_PLAYER1_LASER, aCol);
			DropUnsupported(ma8);
			m1ab8 = 7;
			m1ac0 = PickSpawnDelay();
		}
		PickUpDone(false);
		return;
	}
	if (m1a34 == 1 && mMode == 3)
	{
		PopPsychic(false);
		return;
	}

	// more bricks of the kind in hand, up from the bottom of the stack
	int aPickRow = aRow;
	do
	{
		if (m70.size() > 5)
			break;
		Brick* aPicked = m314[aPickRow][aCol];
		if (aPicked == NULL || !MatchesHand(aPicked) || !aPicked->IsIdle() || IsToolKind(aPicked->GetEffectiveType()))
			break;
		if (mApp->mFlag658)
		{
			m1a5c++;
			if (m1a58 < 1000 && m1a5c == 2)
				m1a58 = 0;
		}
		m1990++;
		aPicked->mFloorY = 423.0f;
		m70.push_back(aPicked);
		m314[aPickRow][aCol] = NULL;
		FireTo(aPicked, 2);
		if (m70.size() > 5)
			break;
		aPickRow--;
	} while (aPickRow > 0);

	if (m8db && m70.size() != aStartSize)
	{
		m948 = 2;
		m94c = (m70.front()->GetEffectiveType() * 0x100 - (int) aStartSize) + (int) m70.size();
		RecordStep(false);
	}
	PickUpDone(false);
}

// 0x4732d0: the up key and the right button shoot the bricks in hand up the column. They fly as a group (m7c) from the
// bottom of the screen up to the top of the stack; a column nuke in hand is fired instead of shot.
void Board::FirePiece()
{
	if (CountMovingBricks(false) >= 1 || m70.empty() || m916)
		return;

	int aRow = FindBottomRow(m198c, true) + 1;
	bool aFull = (aRow == 0x13);
	unsigned int aCount = 0x13U - (unsigned int) aRow;
	if ((unsigned int) m70.size() <= aCount)
		aCount = (unsigned int) m70.size();

	if (mApp->mFlag658)
	{
		mApp->mUser->SetInt(0x11, 0);
		mApp->mFlag658 = false;
		mApp->mUser->Save();
	}

	int aCol = (m198c - 0xa6) / 0x2c;
	CellRef anEmptied = LowestEmptiedCell(aCol);
	if (LowestFallingBrick7c(aCol) != NULL)
		return;
	Brick* aLow94 = LowestFallingBrick94(aCol);

	// where the bricks land: below the lowest cell emptied in the column or the lowest falling brick, whichever is lower
	int aY = anEmptied.mRow * 0x19;
	if (anEmptied.mRow > 0)
		aY += 9;
	if (aLow94 != NULL && (float) aY < aLow94->mY)
		aY = (int) aLow94->mY;
	if (aY < 0)
		aY = aRow * 0x19 + 9;
	else
		aY += 0x19;

	std::list<Brick*> aGroup;
	unsigned int aSize = (unsigned int) m70.size();
	int aFrontKind = (m70.front()->mMutated == 1) ? 0xc : m70.front()->mType;
	int anIndex = (int) aSize - 1;
	int aSpriteY = 0x18e - anIndex * 0x19;
	int aFlyY = 0x1db;
	std::list<Brick*>::iterator aNode = m70.end();
	--aNode;
	bool aDone = false;
	while (!aDone)
	{
		Brick* aBrick = *aNode;
		int aKind = (aBrick->mMutated == 1) ? 0xc : aBrick->mType;

		if (aKind == 0xd)
		{
			// the column nuke is fired where it stands: it takes every brick of the column out
			if (CountMovingBricks(false) == 0 && m7c.empty() && m94.empty())
			{
				int aBottom = aRow - 1;
				m108.push_back(aBrick);
				((PowerBrick*) aBrick)->Activate();
				m70.clear();
				aBrick->SetParticleX((float) m198c);
				if (!aBrick->mFlag18C)
					m1990--;
				int aNukeCol = (m198c - 0xa6) / 0x2c;
				mApp->PlaySample(SOUND_COLUMN_KILLER);
				for (int r = aBottom; r > 0; r--)
				{
					Brick* aTarget = m314[r][aNukeCol];
					if (aTarget != NULL)
					{
						aTarget->SetParticleImages(IMAGE_PARTICLE_FIREBALL);
						RemoveBrick(aTarget, aNukeCol, r, true);
					}
				}
				DropUnsupported(ma8);
				if (m354 != NULL)
					m128.insert(m354);
				m354 = NULL;
				m8e6 = true;
				m270.clear();
			}
			return;
		}

		if (0x1a7 - (int) m70.size() * 0x19 < aY)
		{
			if (!m90b)
				mApp->PlaySample(SOUND_CANT_CLICK);
			return;
		}

		aBrick->SetParticleY((float) aSpriteY);
		if (anIndex < (int) m70.size() - 1)
			aBrick->mSupport = NULL;
		aBrick->SetParticleX((float) m198c);
		aBrick->mTargetY = (float) (aFull ? aFlyY : (aY + aFlyY - 0x1db));
		if ((int) aCount < anIndex || aFull)
			aBrick->SetVisible(false);
		aGroup.push_back(aBrick);
		if (aKind == 6 || aKind == 8 || aKind == 10 || aKind == 0xe || aKind == 0xb)
			aBrick->mFlag18A = true;
		if (aBrick->mMutated != 1 && aBrick->mType == 8)
			((FuelCell*) aBrick)->SetFlag240();
		if (aBrick->mState == 2 && !aBrick->mFlag18C)
			aBrick->Finish();
		else
			FireTo(aBrick, 1);

		if (aNode == m70.begin())
			aDone = true;
		else
			--aNode;
		aSpriteY += 0x19;
		aFlyY += 0x19;
		anIndex--;
	}

	m7c.push_back(aGroup);
	if (m8db)
	{
		m948 = 1;
		m94c = aFrontKind * 0x100 + (int) aSize;
		RecordStep(false);
	}
	// (the original puts the hand in the set even when it is NULL)
	if (m354 != NULL)
		m128.insert(m354);
	m354 = NULL;
	m70.clear();
	m8e6 = true;
	m270.clear();
}
