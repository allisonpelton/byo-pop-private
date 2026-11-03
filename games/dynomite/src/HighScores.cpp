#include "HighScores.h"
#include "Font.h"
#include "Graphics.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

// port: the original opens the file in the current folder; the port keeps it in the user folder
static std::string HighScorePath(const char* theFileName)
{
	return Platform::VFS::Get().GetUserDir() + "/" + theFileName;
}

// 0x422410
HighScores::HighScores(int theMode)
{
	mMode = theMode;
	memset(mFileName, 0, sizeof(mFileName));
	switch (theMode)
	{
	case 0:
		strcpy(mFileName, "STOMPED.HS");
		break;
	case 1:
		strcpy(mFileName, "ENDLESS.HS");
		break;
	case 2:
		strcpy(mFileName, "FOSSIL.HS");
		break;
	case 3:
		strcpy(mFileName, "TIMETRIAL.HS");
		break;
	}
	FILE* aFile = fopen(HighScorePath(mFileName).c_str(), "rb");
	if (aFile == NULL)
	{
		memset(mEntries, 0, sizeof(mEntries));
		strcpy(mEntries[0].mName, "Tank");
		strcpy(mEntries[1].mName, "Steggy");
		strcpy(mEntries[2].mName, "Kroney");
		strcpy(mEntries[3].mName, "Whirley");
		strcpy(mEntries[4].mName, "Inchy");
		mEntries[0].mId = -1;
		mEntries[1].mId = -1;
		mEntries[2].mId = -1;
		mEntries[3].mId = -1;
		// port: the original leaves the last row's id unset
		mEntries[4].mId = -1;
		switch (mMode)
		{
		case 0:
			mEntries[0].mScore = 50000;
			mEntries[1].mScore = 25000;
			mEntries[2].mScore = 10000;
			mEntries[3].mScore = 8000;
			mEntries[4].mScore = 5000;
			mDirty = false;
			return;
		case 1:
			mEntries[0].mScore = 70000;
			mEntries[1].mScore = 55000;
			mEntries[2].mScore = 30000;
			mEntries[3].mScore = 20000;
			mEntries[4].mScore = 10000;
			mDirty = false;
			return;
		case 2:
			mEntries[0].mScore = 80000;
			mEntries[1].mScore = 50000;
			mEntries[2].mScore = 45000;
			mEntries[3].mScore = 20000;
			mEntries[4].mScore = 15000;
			mDirty = false;
			return;
		case 3:
			mEntries[0].mScore = -42000;
			mEntries[1].mScore = -60000;
			mEntries[2].mScore = -72000;
			mEntries[3].mScore = -120000;
			mEntries[4].mScore = -168000;
		}
		mDirty = false;
		return;
	}
	for (int i = 0; i < 5; i++)
	{
		// port: the original reads whatever the file holds; a short file leaves the rest zero
		HighScoreEntry anEntry;
		memset(&anEntry, 0, sizeof(anEntry));
		fread(&anEntry, 1, sizeof(anEntry), aFile);
		mEntries[i] = anEntry;
	}
	fclose(aFile);
	mDirty = false;
}

// 0x422820
HighScores::~HighScores()
{
	if (mDirty == true)
		Save();
}

// 0x4226d0
bool HighScores::IsHighScore(int theScore, int theId)
{
	int i = 0;
	do
	{
		if (mEntries[i].mId == theId)
			return mEntries[i].mScore < theScore;
		i++;
	} while (i < 5);
	i = 0;
	do
	{
		if (mEntries[i].mScore < theScore)
			return true;
		i++;
	} while (i < 5);
	return false;
}

// 0x422730
void HighScores::Save()
{
	FILE* aFile = fopen(HighScorePath(mFileName).c_str(), "wb");
	if (aFile != NULL)
	{
		fwrite(&mEntries[0], 1, sizeof(HighScoreEntry), aFile);
		fwrite(&mEntries[1], 1, sizeof(HighScoreEntry), aFile);
		fwrite(&mEntries[2], 1, sizeof(HighScoreEntry), aFile);
		fwrite(&mEntries[3], 1, sizeof(HighScoreEntry), aFile);
		fwrite(&mEntries[4], 1, sizeof(HighScoreEntry), aFile);
		fclose(aFile);
		// port (web): the scores reach the browser's storage
		Platform::UserDataChanged();
	}
	mDirty = false;
}

// 0x4227b0
void HighScores::Remove(int theId)
{
	int i = 0;
	do
	{
		if (mEntries[i].mId == theId)
		{
			if (i < 5)
			{
				if (i < 4)
					memmove(&mEntries[i], &mEntries[i + 1], (4 - i) * sizeof(HighScoreEntry));
				mEntries[4].mScore = 0;
				mEntries[4].mId = -1;
				mEntries[4].mName[0] = 0;
			}
			return;
		}
		i++;
	} while (i < 5);
}

// 0x422840: the rows below move down a place with their names and scores; their ids stay where they were
void HighScores::Add(int theScore, const char* theName, int theId)
{
	Remove(theId);
	int i = 0;
	do
	{
		if (mEntries[i].mScore < theScore)
			break;
		i++;
	} while (i < 5);
	if (i != 5)
	{
		if (i < 4)
		{
			int j = 4;
			int aCount = 4 - i;
			do
			{
				mEntries[j].mScore = mEntries[j - 1].mScore;
				strcpy(mEntries[j].mName, mEntries[j - 1].mName);
				j--;
				aCount--;
			} while (aCount != 0);
		}
		mEntries[i].mScore = theScore;
		strncpy(mEntries[i].mName, theName, 0xf);
		mEntries[i].mId = theId;
		mDirty = true;
	}
}

// 0x422900
void HighScores::Draw(Graphics* g, int x, int y, int theWidth, int theHeight)
{
	Color anOldColor = g->GetColor();
	g->SetColor(Color(0, 0, 0, 100));
	g->FillRect(x - 2, y - 2, theWidth + 4, theHeight + 4);
	y += 0xc;
	HighScoreEntry* anEntry = &mEntries[0];
	int aCount = 5;
	do
	{
		char aScore[24];
		if (mMode == 3)
		{
			int aTime = abs(anEntry->mScore / 100);
			sprintf(aScore, "%d:%.2d", aTime / 60, aTime % 60);
		}
		else
			sprintf(aScore, "%d", anEntry->mScore);
		int aScoreWidth = g->GetFont()->StringWidth(aScore);
		g->SetColor(Color(0, 0, 0));
		g->DrawString(anEntry->mName, x + 4, y + 2);
		g->DrawString(aScore, (x + (theWidth - aScoreWidth)) - 2, y + 2);
		g->SetColor(Color(0xff, 0xff, 0xff));
		g->DrawString(anEntry->mName, x + 2, y);
		g->DrawString(aScore, (x + (theWidth - aScoreWidth)) - 4, y);
		y = y - 2 + g->GetFont()->GetHeight();
		anEntry++;
		aCount--;
	} while (aCount != 0);
	g->SetColor(anOldColor);
}
