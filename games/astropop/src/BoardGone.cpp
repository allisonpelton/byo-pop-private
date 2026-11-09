// Board: the handler of event 6, a brick has gone (OnBrickGone), and the sort of the falling groups that it uses. Every
// brick that is taken out of the field ends here: it scores, it counts for the meter of the stage, and when the last
// brick of a removal has gone the bricks above the holes are given the places they fall to and the fired groups are
// started.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "GameText.h"
#include "Font.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "SmallObjects.h"
#include "Res.h"
#include "Particle.h"

using namespace Sexy;

// 0x46cfd0: the groups of falling bricks (m94) are put in the order of the height of the first brick of each, the
// highest first: a group goes in front of the first group of those sorted so far whose brick is lower than its own, so
// groups of one height keep their order
void Board::SortFallingGroups()
{
	std::vector<std::list<Brick*> > aSorted;

	for (std::list<std::list<Brick*> >::iterator i = m94.begin(); i != m94.end(); ++i)
	{
		std::list<Brick*> aGroup = *i;
		int aHeight = (int) aGroup.front()->mY;

		size_t aPos = 0;
		while (aPos < aSorted.size())
		{
			if ((float) aHeight < aSorted[aPos].front()->mY)
				break;
			aPos++;
		}
		aSorted.insert(aSorted.begin() + aPos, aGroup);
	}

	m94.clear();
	for (size_t i = 0; i < aSorted.size(); i++)
		m94.push_back(aSorted[i]);
}

// 0x46d1e0 event 6: a brick has gone. It is paid (the value of its place times +0x940, in tens), counted for the stage's
// meter (the bar fills and says so at a quarter, a half and three quarters), and, once the last of the bricks that were
// taken together has gone, the bricks that fall are given the rows they fall to, the bricks that were taken are handed to
// the list that frees them, the fired groups start to move and the field is looked at again (a cleared field, no move
// left).
void Board::OnBrickGone(Brick* theBrick)
{
	if (!m8eb && !m8ed && !m8ef && !m8f4 && !m8ec && !mFlag8E9 && !mFlag8E8 && m1924 == 0)
	{
		int aPoints = (int) ((float) theBrick->mColumn * m940);
		aPoints = aPoints - aPoints % 10;
		m19dc += aPoints;
		m196c += aPoints;
		if (!m910)
			AddScore(aPoints, false);
		if (m6e8 == -1 && m6ec == -1)
		{
			m6e8 = (int) theBrick->mX;
			m6ec = (int) theBrick->mY;
		}
	}

	m7c0++;
	if (theBrick->mMutated != 1 && theBrick->mType == 10)
		Fn4335e0();
	m1994--;

	if (!mFlag8E8)
	{
		if (!m910)
		{
			bool aMatches = true;
			if (m1a88 != -1)
				aMatches = (m1a88 == theBrick->GetEffectiveType());

			if (aMatches)
			{
				m1a14++;
				if (m1a60 == 2 && m1a14 % 300 == 0)
					UpdateSupercharge(true, -1);

				double aFraction = (double) m1a14 / (double) m1a18;
				m928 = (float) aFraction;
				if (aFraction > 1.0)
					m928 = 1.0f;
				if (m1a60 == 2)
					m928 = 0.0f;

				// at a quarter, a half and three quarters of the stage the meter says so
				std::string aText;
				if (m1a14 == m1a18 / 4)
					aText = GT(0x5661a4);
				else if (m1a14 == m1a18 / 2)
					aText = GT(0x5661a0);
				else if (m1a14 == (m1a18 * 3) / 4)
					aText = GT(0x56619c);

				if (!aText.empty() && m1a60 != 2 && (!m8eb || m8ec) && !m8ed && !m8ef && !m8f4 && !m8ec)
				{
					delete m778;

					MemoryImage* anImage = new MemoryImage(mApp);
					Font* aFont = GetFontById(0xb2);
					anImage->SetImageMode(true, true);
					std::string aTail = GT(0x566190);
					anImage->Create(aFont->StringWidth(aTail), aFont->GetHeight() * 2);

					Graphics g(anImage);
					g.SetColor(Color(0xff, 0xff, 0xff));
					g.SetFont(aFont);
					g.DrawString(aText, anImage->GetWidth() / 2 - aFont->StringWidth(aText) / 2, aFont->GetAscent());
					g.DrawString(aTail, 0, aFont->GetAscent() + aFont->GetHeight() - 10);

					m8de = true;
					m778 = new LevelUpText(anImage, 0);
					m778->m04 = true;
					mApp->PlaySample(SOUND_WORDSWOOSH);
					mAnim4e0.Reset();
					mAnim4e0.SetStopped(false);
					mApp->PlaySample(SOUND_BRICK_MILESTONE);
				}
			}
		}

		// the sounds for the last fifty and the last hundred bricks of the stage
		int aLeft = m1a18 - m1a14;
		if (aLeft <= 0x32 && !m8f6 && m1a60 != 2)
		{
			m8f6 = true;
			mApp->PlaySample(SOUND_LEVEL_UP2);
		}
		else if (!(aLeft > 100 || m8f5 || m1a60 == 2))
		{
			m8f5 = true;
			mApp->PlaySample(SOUND_LEVEL_UP1);
		}
	}

	// the topmost cell that was emptied in each column
	std::vector<CellRef> aTop(10);
	for (int i = 0; i < 10; i++)
	{
		aTop[i].mCol = 500000000;
		aTop[i].mRow = 500000000;
	}

	// the falling groups fall to the cells that were emptied in their column, one on top of the other
	if (m1994 == 0 && !m94.empty())
	{
		SortFallingGroups();
		for (unsigned int i = 0; i < ma8.size(); i++)
		{
			if (ma8[i].mRow < aTop[ma8[i].mCol].mRow)
				aTop[ma8[i].mCol] = ma8[i];
		}

		std::vector<int> aCounts(10, 0);
		for (std::list<std::list<Brick*> >::iterator g = m94.begin(); g != m94.end(); ++g)
		{
			for (std::list<Brick*>::iterator b = g->begin(); b != g->end(); ++b)
			{
				Brick* aBrick = *b;
				int aCol = CellColumn(aBrick->mX);
				int aRow = CellRow(aBrick->mY);
				int aTopRow = aTop[aCol].mRow;
				if (aTopRow < aRow)
				{
					int aCell = aCounts[aCol] + aTopRow;
					int aY;
					if (aCell > 0)
						aY = aCell * 0x19 + 9;
					else
						aY = 4;
					aBrick->mTargetY = (float) aY;
					aCounts[aCol]++;
				}
			}
		}
	}

	if (m1994 == 0 && !mFlag8E8 && !mFlag8E9)
	{
		if (mMode == 3 && m1a34 == 1 && m1958 > 0xa5 && m1958 < 0x25e)
			HoverCell((m1958 - 0xa6) / 0x2c, CellRow((float) m195c), false);

		std::vector<int> aCounts(10, 0);

		// the fired groups: each brick of a group falls to the place below the highest cell that was emptied in its
		// column (or stays where it is, when nothing was emptied there), one on top of the other
		if (!m7c.empty())
		{
			for (std::list<std::list<Brick*> >::iterator g = m7c.begin(); g != m7c.end(); ++g)
			{
				int aX = 500000000;
				int aY = 500000000;
				for (unsigned int i = 0; i < ma8.size(); i++)
				{
					int aCellX = ma8[i].mCol * 0x2c + 0xa6;
					int aCellY = ma8[i].mRow * 0x19 + 9;
					if (!g->empty() && aCellX == (int) g->front()->mX && aCellY < aY)
					{
						aX = aCellX;
						aY = aCellY;
					}
				}

				int aTarget = aY;
				if (aX != 500000000)
				{
					Brick* aLowest = LowestFallingBrick94((aX - 0xa6) / 0x2c);
					if (aLowest != NULL)
					{
						if (aLowest->mY == aLowest->mTargetY && aLowest->mState == 1)
						{
							if ((float) aY < aLowest->mY)
								aTarget = (int) aLowest->mY;
						}
						else
							aTarget = (int) aLowest->mTargetY + 0x19;
					}
				}

				for (std::list<Brick*>::iterator b = g->begin(); b != g->end(); ++b)
				{
					Brick* aBrick = *b;
					int aCol = CellColumn(aBrick->mX);
					if (aY != 500000000)
					{
						aBrick->SetTargetY(aCounts[aCol] * 0x19 + aTarget);
						aCounts[aCol]++;
					}
					else if (aBrick->mY == aBrick->mTargetY && aBrick->mState == 1)
						aBrick->SetTargetY((int) aBrick->mTargetY);
				}
			}
		}

		// (the draw of the random number is not used; the original made it here)
		if (!ma0.empty())
			AstroRand();

		// the bricks that were taken go to the list that frees them
		float aBonusX = 308.0f;
		float aBonusY = 260.0f;
		int anIndex = 0;
		for (std::list<Brick*>::iterator i = ma0.begin(); i != ma0.end(); ++i, anIndex++)
		{
			Brick* aBrick = *i;
			if (anIndex == 0)
			{
				aBonusX = aBrick->mX;
				aBonusY = aBrick->mY;
			}

			// a fuel cell that has gone: the colour of the brick above it is charged
			if (aBrick->mMutated != 1 && aBrick->mType == 8)
			{
				if (!m910)
				{
					FloatText* aFloat = new FloatText(StrFormat(GTC(0x566184)), FONT_POINTS, aBonusX, aBonusY + 50.0f, false);
					m140.push_back(aFloat);
				}

				int aCol = CellColumn(aBrick->mX);
				int aRow = CellRow(aBrick->mY);
				Brick* anAbove = NULL;
				if (aRow > 0)
					anAbove = m314[aRow - 1][aCol];

				int aColour = -1;
				if (((FuelCell*) aBrick)->GetFlag240() && aRow > 1 && anAbove != NULL)
				{
					if (anAbove->mMutated != 1 && (anAbove->mSpecial || (anAbove->mType >= 0 && anAbove->mType <= 5)))
						aColour = anAbove->mType;
				}
				UpdateSupercharge(false, aColour);
			}

			m88.push_back(aBrick);
			FireTo(aBrick, 4);
		}
		ma0.clear();
		std::vector<CellRef>().swap(ma8);

		// the groups that fall are fired: they join the fired groups, in front of those that were there
		std::list<std::list<Brick*> >::iterator aPos = m7c.begin();
		for (std::list<std::list<Brick*> >::iterator g = m94.begin(); g != m94.end(); ++g)
		{
			for (std::list<Brick*>::iterator b = g->begin(); b != g->end(); ++b)
				FireTo(*b, 1);
			m7c.insert(aPos, *g);
		}
		m94.clear();

		if (m7c.empty() && ma0.empty() && mf8.empty())
		{
			if (m1980 > 1 && m1980 > 2 && (mField193C > 1 || m1a60 == 2))
				m8ee = true;
			if (m19fc > 0x3c && !AnyMoveLeft(NULL))
			{
				if (m19fc < 0x3d)
					m19d8 = (int) ((float) m19cc + m930 * 20.0f);
				else
					m19fc = 0x3c;
			}
		}

		if (!m8e4)
			CheckStackHeight();

		// a cleared field (nothing in the first row): the bonus
		if (m7c.empty() && m94.empty() && ma0.empty() && mf8.empty() && CountMovingBricks(false) == 0)
		{
			m8fb = true;
			for (int aCol = 0; aCol < 10; aCol++)
			{
				if (m314[1][aCol] != NULL)
				{
					m8fb = false;
					break;
				}
			}
			if (m8fb && !m910)
				ClearBonus();
		}

		// what the bricks of this removal were worth, where the first of them was
		if (m19dc > 0 && !m910)
		{
			FloatText* aFloat = new FloatText(StrFormat(GTC(0x56617c), m19dc), FONT_POINTS, (float) m6e8, (float) m6ec, false);
			m140.push_back(aFloat);
			m6e8 = -1;
			m6ec = -1;
		}
		m19dc = 0;
		CheckSparseField();
	}

	if (mMode == 3 && m1a34 == 1 && m1958 > 0xa5 && m1958 < 0x25e)
		HoverCell((m1958 - 0xa6) / 0x2c, CellRow((float) m195c), false);

	if (mMode == 2 && !m907 && m1994 == 0 && m7c.empty() && CountMovingBricks(false) == 0)
		Fn4335e0();

	// a field with no move left (the out flag of AnyMoveLeft is a byte of the original's frame that nothing set before)
	if (m910 && mf8.empty() && !m912 && !m911 && m1b7c == 0 && m1994 == 0 && CountMovingBricks(false) == 0 &&
		m7c.empty() && m94.empty())
	{
		bool aFlag = false;
		if (!AnyMoveLeft(&aFlag))
		{
			if (!aFlag)
			{
				m1b7c = 400;
				BuildBanner(GT(0x56568c), m1b7c, FONT_NEURO30);
				mApp->PlaySample(SOUND_NO_MORE_MOVES);
			}
			StartClearCount();
		}
	}
}
