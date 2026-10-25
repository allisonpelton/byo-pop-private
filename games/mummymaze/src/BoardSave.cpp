// The classic game in progress: WinMM.exe 0x410b70 writes data\saves\classic.sav, 0x41c600 reads it, 0x410dd0 deletes it.
//
// The original's file is version 2: the random generators' states, the high-score data buffer, then the level, the maze,
// the file, the score, whether the level was done, the time played and the game mode. The port writes the numbers in the
// same order into the user folder's classic.sav (the generators' states are not kept: nothing draws from them).

#include "Board.h"
#include "GameText.h"
#include "ImageFont.h"
#include "Screens.h"
#include "MummyApp.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <cstring>
#include <string>

using namespace Sexy;

static std::string ClassicSavePath()
{
	return "classic.sav";
}

// 0x410b70: not written for an adventure game or when the level is lost
bool Board::SaveClassic()
{
	if (mShowingSolution || mApp->mGameMode == MODE_ADVENTURE)
		return false;
	std::string aData;
	auto aInt = [&aData](int v) { aData.append((const char*) &v, 4); };
	aInt(2);
	aInt(mApp->mClassicChamber);
	aInt(mApp->mClassicPyramid);
	aInt(mMaze);
	aInt(mFile);
	aInt(mApp->mScore);
	aData.push_back(mState == STATE_DONE ? 1 : 0);
	aInt((int) (mUpdateCnt - mTimeBase));
	aInt(mApp->mGameMode);
	return Platform::WriteUserFileAtomic(ClassicSavePath(), aData);
}

// 0x410dd0
bool Board::DeleteClassicSave()
{
	return Platform::RemoveUserFile(ClassicSavePath());
}

bool MummyApp::ClassicSaveExists()
{
	std::string aData;
	return Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + ClassicSavePath(), &aData);
}

// 0x41c600: takes the saved game's numbers over; the next board loads that maze
bool MummyApp::LoadClassic()
{
	std::string aData;
	if (!Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + ClassicSavePath(), &aData) || aData.size() < 4 * 8 + 1)
		return false;
	const char* p = aData.data();
	auto aInt = [&p]() { int v; memcpy(&v, p, 4); p += 4; return v; };
	if (aInt() != 2)
		return false;
	mClassicChamber = aInt();
	mClassicPyramid = aInt();
	mRestoreMaze = aInt();
	mRestoreFile = aInt();
	mScore = aInt();
	p++;
	mElapsedBase = (DWORD) aInt() * 10;
	mGameMode = aInt();
	mRestoring = true;
	return true;
}
