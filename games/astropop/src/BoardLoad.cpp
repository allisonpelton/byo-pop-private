// Board: LoadLevel (0x4663f0), the start of a stage or of a game: the field is emptied and filled with the first
// rows, from a level file (data\level<n>.xcf) or by the rules of the stage.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "XcfFile.h"
#include "Res.h"
#include <stdlib.h>

using namespace Sexy;

// 0x4663f0 (the trace lines of the original, "Start of level" and "Exiting start next level", are left out)
void Board::LoadLevel(bool theFromRules)
{
	XcfFile aFile;

	m8da = false;
	mAnim460.SetStopped(true);
	ClearField();
	m1930 = -1;
	memset(&m984, 0, sizeof(m984));
	ResetGame();
	m8ea = true;
	m1b38 = 0;
	mApp->SetCursor(CURSOR_POINTER);
	if (!theFromRules)
	{
		for (unsigned int aRow = 0; aRow < m314.size(); aRow++)
		{
			for (int aCol = 0; aCol < 10; aCol++)
				m314[aRow][aCol] = NULL;
		}
	}
	else
	{
		std::string aFileName = StrFormat("data\\level%d.xcf", mField193C);
		if (mField193C == -100)
		{
			// a level from a file (no level file of the game is named like this; the original's path is kept)
			aFile.AddKeyName("DropTime");
			aFile.AddKeyName("LevelUp");
			for (int i = 0; i < 10; i++)
				aFile.AddKeyName(StrFormat("Brick%d", i));
			aFile.Parse(aFileName);
			aFile.EnterFirstChild();
			m197c = aFile.GetInt("\\LEVEL\\LevelUp");
			m19cc = aFile.GetInt("\\LEVEL\\DropTime") * 100;
			bool aDone = false;
			m1930 = -1;
			do
			{
				std::string aName = aFile.GetCurrentName();
				std::string aNumber = aName.size() > 3 ? aName.substr(3) : std::string();
				long aRowNumber = atol(aNumber.c_str());
				if (m1930 == -1)
					m1930 = (int) aRowNumber;
				for (int i = 0; i < 10; i++)
				{
					long aCode = aFile.GetInt(StrFormat("Brick%d", i));
					if (aRowNumber >= 0 && aRowNumber < 100)
						m984.mCode[aRowNumber][i] = (int) aCode;
				}
				if (aFile.NextSibling() == XcfFile::CURSOR_END)
					aDone = true;
			}
			while (!aDone);

			int aY = 4;
			for (int aRow = 0; aY < 0x1f8; aRow++)
			{
				int aX = 0xa6;
				for (int aCol = 0; aX < 0x25e; aCol++)
				{
					if (m1ac8 < aRow)
						m314[aRow][aCol] = NULL;
					else
					{
						int aScriptRow = 17 - aRow;
						int aKind = (aScriptRow >= 0 && aScriptRow < 100) ? m984.mCode[aScriptRow][aCol] : 0;
						switch (aKind)
						{
						case 0:
							m314[aRow][aCol] = NULL;
							break;
						case 1:
						case 2:
						case 3:
						case 4:
							m314[aRow][aCol] = CreateBrick(aKind - 1);
							break;
						case 5:
							m314[aRow][aCol] = CreateSpecialBrick(7);
							break;
						case 6:
						case 7:
						case 8:
						case 9:
							m314[aRow][aCol] = CreateKillAll(aKind - 6);
							break;
						case 10:
							m314[aRow][aCol] = CreateSpecialBrick(6);
							break;
						}
						// (the original calls through the cell, which is null for kind 0)
						Brick* aBrick = m314[aRow][aCol];
						if (aBrick != NULL)
						{
							aBrick->SetParticleX((float) aX);
							aBrick->SetParticleY((float) (aY > 4 ? aY + 5 : aY));
						}
					}
					aX += 0x2c;
				}
				aY += 0x19;
			}
			m1930 = m1930 + m1ac8 + 1;
			bool aAnyBrick = false;
			for (int i = 0; i < 10; i++)
			{
				if (m1930 >= 0 && m1930 < 100 && m984.mCode[m1930][i] != 0)
					aAnyBrick = true;
			}
			if (!aAnyBrick)
				m1930 = -1;
		}
		else
		{
			int aStageRows = m1ac8;
			m1930 = -1;
			for (int aRow = 0; aRow < 0x14; aRow++)
			{
				if (aStageRows < aRow)
				{
					for (int aCol = 0; aCol < 10; aCol++)
						m314[aRow][aCol] = NULL;
				}
				else
					FillTopRow(aRow < aStageRows - 1, aRow == aStageRows, false, m1a60 == 2, aRow);
			}
		}
	}

	// the warp stars of the last warp go
	for (unsigned int i = 0; i < m150.size(); i++)
		delete m150[i];
	std::vector<WarpStar*>().swap(m150);
	m198c = 0x182;
	m1998 = 8;
	m19b0 = 0;
	std::vector<Brick*>().swap(mTargeted);

	// the stage's picture and the first hint start from the beginning
	if ((unsigned int) mField193C < m220.size())
	{
		StageBg* aBg = m220[mField193C];
		aBg->m100 = 0xff;
		aBg->m114 = 0xff;
		aBg->m108 = 0;
		aBg->m118 = 0;
		aBg->m10c = 0;
		aBg->m110 = 0;
		aBg->m104 = 0;
		aBg->mf0 = 0;
	}
	if (!m230.empty() && m230[0]->m1c)
	{
		StageBg* aBg = m230[0]->mBg;
		aBg->m100 = 0xff;
		aBg->m114 = 0xff;
		aBg->m108 = 0;
		aBg->m118 = 0;
		aBg->m10c = 0;
		aBg->m110 = 0;
		aBg->m104 = 0;
		aBg->mf0 = 0;
	}
	if (m1a60 == 1)
	{
		m1b2c = m1b28;
		m1b28 = Fn433130(m1940 + 1);
	}
	Fire(0x2a, 1);
}
