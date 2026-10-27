#include "PuzzleListWidget.h"
#include "BinFile.h"
#include "FossilizedApp.h"
#include "Graphics.h"
#include "IntroScreen.h"
#include "ScrollbarWidget.h"
#include "Sprite.h"
#include "platform/VFS.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>

using namespace Sexy;

// 0x4318f0
PuzzleListWidget::PuzzleListWidget(FossilizedApp* theApp)
{
	mApp = theApp;
	mScrollbar = NULL;
	mNothingToPlay = true;
	mNeedsScrollbar = false;
	mScrollY = 0;
	LoadPacks();
	mSelected = mApp->mField1090;
	if (mPacks.mCount - 1 < mSelected)
		mSelected = 0;
	PuzzlePack* aPack = mPacks.Get(mSelected);
	if (aPack != NULL)
	{
		strcpy(mApp->mPackFile, aPack->mFile);
		mApp->mPuzzleSkills = aPack->mSkills;
		mPackSkills = aPack->mSkills;
	}
	mOver = -1;
}

// 0x4311f0
PuzzleListWidget::~PuzzleListWidget()
{
	mPacks.ResetIter();
	PuzzlePack* aPack = mPacks.Next();
	while (aPack != NULL)
	{
		mPacks.Remove(aPack);
		delete aPack->mPic;
		delete aPack;
		aPack = mPacks.Next();
	}
	mApp->mField1090 = mSelected;
}

// A pack's file: its name, the three skills' puzzle offsets (-1 none) and its picture. The original has this
// twice, inline, for DEFAULT.EGG and for the STOMPED folder's packs.
static bool ReadPack(FossilizedApp* theApp, const char* theFile, PuzzlePack* thePack, PtrList<PuzzlePack>& thePacks)
{
	// port: the original news the file and never deletes it
	BinFile aFile;
	aFile.Open(theFile, true);
	aFile.ReadString(thePack->mName);
	int aNormal = aFile.ReadInt();
	int aHard = aFile.ReadInt();
	int anExpert = aFile.ReadInt();
	bool added;
	if ((aNormal == -1) && (aHard == -1) && (anExpert == -1))
	{
		delete thePack;
		added = false;
	}
	else
	{
		unsigned int* aBits = (unsigned int*) malloc(0xb2e0);
		aFile.Read(aBits, 0xb2e0);
		thePack->mPic = new Pic(theApp, aBits, 0x6c, 0x6a);
		thePack->mSkills = 0;
		if (aNormal != -1)
			thePack->mSkills = 1;
		if (aHard != -1)
			thePack->mSkills |= 2;
		if (anExpert != -1)
			thePack->mSkills |= 4;
		free(aBits);
		thePacks.Add(thePack);
		added = true;
	}
	aFile.Close();
	return added;
}

// 0x4312b0: DEFAULT.EGG first, then the STOMPED folder's packs; the first one found is the app's pack
void PuzzleListWidget::LoadPacks()
{
	char aName[256];
	mPacks.Clear();
	bool first = true;
	if (Platform::VFS::Get().Exists("DEFAULT.EGG"))
	{
		sprintf(aName, "DEFAULT.EGG");
		// port: the pack is zeroed; the original hands the app its skills before they are read (the
		// constructor sets them again from the chosen pack)
		PuzzlePack* aPack = new PuzzlePack();
		strcpy(aPack->mFile, aName);
		strcpy(mApp->mPackFile, aName);
		mApp->mPuzzleSkills = aPack->mSkills;
		mPackSkills = aPack->mSkills;
		first = false;
		if (ReadPack(mApp, aName, aPack, mPacks))
			mNothingToPlay = false;
	}

	// port: FindFirstFile("STOMPED\\*.EGG") gives NTFS's order, so the names are sorted here
	std::vector<std::string> aFiles;
	std::string aDir = Platform::VFS::FindCaseInsensitive(Platform::VFS::Get().GetBaseDir(), "STOMPED");
	if (!aDir.empty())
	{
		for (const std::string& aFile : Platform::ListDirectory(aDir))
		{
			if ((aFile.size() > 4) && (strcasecmp(aFile.c_str() + aFile.size() - 4, ".EGG") == 0))
				aFiles.push_back(aFile);
		}
		std::sort(aFiles.begin(), aFiles.end(), [](const std::string& a, const std::string& b)
			{ return strcasecmp(a.c_str(), b.c_str()) < 0; });
	}
	size_t i = 0;
	if (!aFiles.empty())
	{
		do
		{
			sprintf(aName, "STOMPED\\%s", aFiles[i].c_str());
			PuzzlePack* aPack = new PuzzlePack();
			strcpy(aPack->mFile, aName);
			if (first)
			{
				strcpy(mApp->mPackFile, aName);
				mApp->mPuzzleSkills = aPack->mSkills;
				mPackSkills = aPack->mSkills;
				first = false;
			}
			if (ReadPack(mApp, aName, aPack, mPacks))
				mNothingToPlay = false;
			i++;
		} while (i < aFiles.size());
	}
	MarkDirty();
}

// 0x4316e0
void PuzzleListWidget::ScrollPosition(int theId, double thePosition)
{
	mScrollY = -(int) thePosition;
	MarkDirty();
}

// 0x431700
void PuzzleListWidget::MouseLeave()
{
	Widget::MouseLeave();
}

// 0x431710: two packs a row, 120 pixels apart
void PuzzleListWidget::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	int anOldOver = mOver;
	int aBottom = mScrollY;
	int aLeft = 10;
	mOver = -1;
	int aColumn = -1;
	int i = 0;
	mPacks.ResetIter();
	PuzzlePack* aPack = mPacks.Next();
	if (aPack != NULL)
	{
		aBottom = aBottom + 0x6e;
		do
		{
			aColumn = aColumn + 1;
			if (1 < aColumn)
			{
				aColumn = 0;
				aLeft = 10;
				aBottom = aBottom + 0x78;
			}
			if ((aLeft <= x) && (x <= aLeft + 0x6e) && (aBottom - 0x69 <= y) && (y <= aBottom))
			{
				mOver = i;
				if (i != anOldOver)
				{
					strcpy(mOverFile, aPack->mFile);
					mOverSkills = aPack->mSkills;
				}
				break;
			}
			i = i + 1;
			aLeft = aLeft + 0x78;
			aPack = mPacks.Next();
		} while (aPack != NULL);
	}
	if (mOver != anOldOver)
	{
		if (mOver == -1)
		{
			ShowFinger(false);
			return;
		}
		ShowFinger(true);
	}
}

// 0x431830
void PuzzleListWidget::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);
	if (mOver != -1)
	{
		if (mOver == mSelected)
		{
			mApp->mIntroScreen->ButtonDepress(7);
			mApp->mSndEgglands->Play();
			return;
		}
		mApp->mSndClick->Play();
		strcpy(mApp->mPackFile, mOverFile);
		mApp->mPuzzleSkills = mOverSkills;
		mPackSkills = mOverSkills;
		mSelected = mOver;
		MarkDirty();
	}
}

// 0x431a10
void PuzzleListWidget::Draw(Graphics* g)
{
	SetDrawGraphics(g);
	g->SetColor(Color(0x28, 0x36, 0x17, 0xff));
	g->FillRect(0, 0, mWidth, mHeight);
	int y = mScrollY;
	int x = 10;
	int aColumn = -1;
	int aRowTop = 10;
	int i = 0;
	if (mNothingToPlay == true)
		mApp->mAnnounceFont1928->Draw(mApp->GetString("NO_PUZZLES").c_str(), 0x1e, mHeight / 2 - 10);
	mPacks.ResetIter();
	PuzzlePack* aPack = mPacks.Next();
	int aBottom = 10;
	if (aPack != NULL)
	{
		y = y + 0xc;
		do
		{
			aColumn = aColumn + 1;
			if (1 < aColumn)
			{
				y = y + 0x78;
				aRowTop = aRowTop + 0x78;
				aColumn = 0;
				x = 10;
			}
			if (mSelected == i)
			{
				g->SetColor(Color(0xac, 0x78, 1, 0xff));
				g->FillRect(x - 5, y - 2, 0x76, 0x76);
				g->SetColor(Color(0x56, 0x3c, 1, 0xff));
				g->FillRect(x - 3, y, 0x72, 0x72);
			}
			aPack->mPic->Draw(x, y + 3);
			g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
			int aWidth = mApp->mTinyFont->Width(aPack->mName);
			mApp->mTinyFont->Draw(aPack->mName, (x - aWidth / 2) + 0x32, y + 0x6c);
			x = x + 0x78;
			i = i + 1;
			aPack = mPacks.Next();
			aBottom = aRowTop;
		} while (aPack != NULL);
	}
	aBottom = aBottom + 0xb4;
	mScrollbar->SetMaxValue((double) (aBottom - mHeight));
	mScrollbar->SetPageSize((double) (mHeight / 5));
	if (0 < aBottom - mHeight)
		mNeedsScrollbar = true;
}
