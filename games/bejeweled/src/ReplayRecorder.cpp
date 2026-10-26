// port(variant): the replay recorder (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.2-6.5); see
// ReplayRecorder.h. Not part of the original game.

#include "ReplayRecorder.h"
#include "Board.h"
#include "WinDMApp.h"
#include "GameConfig.h"
#include "StateCodec.h"
#include "HighScoreWidget.h"
#include "ImageFont.h"
#include "MTRand.h"
#include "Common.h"
#include "platform/Json.h"
#include "platform/ModManager.h"
#include "platform/VFS.h"
#include "platform/WebPersist.h"

#include <SDL.h>
#include <sys/stat.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <utility>

using namespace Sexy;

// The last finished replay written and the board whose game it holds (FinishedReplay)
static uint32_t gFinishedBoard = 0;
static std::string gFinishedPath;

ReplayRecorder::ReplayRecorder(WinDMApp* theApp, Board* theBoard)
{
	mApp = theApp;
	mBoard = theBoard;
	mHover = gGameConfig.mFeatures.mReplayHover;
	mEnded = false;
	mNewGame = false;
	mEndReason = Replay::END_ABANDONED;
	mSaving = false;
	mSaved = false;
	mLoaded = false;
	mContinued = false;
	mWroteSidecar = false;
	mLastState = theBoard->mState;
	mMoves = 0;
	mLastHoverX = theBoard->mMouseX;
	mLastHoverY = theBoard->mMouseY;
	mMirror = NULL;
	mMirrorDraws = 0;

	mFile.mFlags = mHover ? Replay::FLAG_HOVER : 0;
	mFile.mHasRules = true;
	mFile.mRules = theBoard->mRules.Canonical();
	mFile.mHasRulesId = true;
	theBoard->mRules.RulesId(mFile.mRulesId);

	mSegment.mMode = theBoard->mTimedMode ? 1 : 0;
	mSegment.mRng = theBoard->mRngStart;
	mSegment.mUserSeeded = theBoard->mUserSeeded;
	mSegment.mShowInstructions = theApp->mShowInstructions;
	mSegment.mTimesPlayed = theApp->mTimesPlayed;
	mSegment.mPointsAscent = (theApp->mAlphaDigitsFont != NULL) ? theApp->mAlphaDigitsFont->GetAscent() : 0;

	// The constructor's draws are the board's own: the stream started that many draws ago
	mSession = (theBoard->mOwnRng == NULL);
	mRandBase = GetRandCount() - theBoard->mBoardDraws;
	mDrawBase = 0;
	mXrandLogged = 0;
	if (mSession)
	{
		mMirror = new MTRand(1);
		if (!mMirror->SetState(theBoard->mRngStart.mMt, theBoard->mRngStart.mMti))
		{
			delete mMirror;
			mMirror = NULL;
		}
	}
}

ReplayRecorder::~ReplayRecorder()
{
	delete mMirror;
}

void ReplayRecorder::Add(int theOp, int64_t theArg0, int64_t theArg1)
{
	Replay::Event anEvent;
	anEvent.mTick = (uint32_t) mBoard->mUpdateCnt;
	anEvent.mOp = theOp;
	anEvent.mArgs[0] = theArg0;
	anEvent.mArgs[1] = theArg1;
	mSegment.mEvents.push_back(anEvent);
	mSaved = (theOp == Replay::OP_SAVE);
}

// SELECT and TARGET name a cell: where the piece is in the grid (-1 -1 for none)
void ReplayRecorder::AddCell(int theOp, Piece* thePiece)
{
	int aCol = -1;
	int aRow = -1;
	for (int r = 0; (r < mBoard->mRows) && (thePiece != NULL) && (aCol < 0); r++)
	{
		for (int c = 0; c < mBoard->mCols; c++)
		{
			if (mBoard->mBoard[r][c] == thePiece)
			{
				aCol = c;
				aRow = r;
				break;
			}
		}
	}
	Add(theOp, aCol, aRow);
	if ((theOp == Replay::OP_TARGET) && (aCol >= 0))
		mMoves++;
}

// The board's stream starts again (the constructor's, or the one a LOAD leaves it on)
void ReplayRecorder::StartStream()
{
	mSession = (mBoard->mOwnRng == NULL);
	mRandBase = GetRandCount();
	mDrawBase = mBoard->mBoardDraws;
	mXrandLogged = 0;
	delete mMirror;
	mMirror = NULL;
	mMirrorDraws = 0;
	if (mSession)
	{
		uint32_t aState[MTRAND_N];
		int anIndex = 0;
		GetRandMTState(aState, &anIndex);
		mMirror = new MTRand(1);
		if (!mMirror->SetState(aState, anIndex))
		{
			delete mMirror;
			mMirror = NULL;
		}
	}
}

const MTRand* ReplayRecorder::HashRng()
{
	if (!mSession || (mMirror == NULL))
		return NULL;
	uint32_t aTarget = mXrandLogged + (mBoard->mBoardDraws - mDrawBase);
	while (mMirrorDraws < aTarget)
	{
		mMirror->Next();
		mMirrorDraws++;
	}
	return mMirror;
}

void ReplayRecorder::SyncExternalRand()
{
	// ENV: the app's values the tick-150 block of Board::Update reads, as they are at the start of the update
	// that runs it. The constructor's may be out of date by then: the board this one replaced is deleted after
	// it was made, and a game it had in progress counts as played (~Board).
	if (!mEnded && (mBoard->mUpdateCnt == 149) && (mBoard->mPauseCount == 0) && (mBoard->mUserPauseCount == 0))
	{
		mSegment.mShowInstructions = mApp->mShowInstructions;
		mSegment.mTimesPlayed = mApp->mTimesPlayed;
	}
	if (mEnded || !mSession)
		return;
	uint32_t aSeen = mXrandLogged + (mBoard->mBoardDraws - mDrawBase);
	uint32_t aTotal = GetRandCount() - mRandBase;
	if ((int32_t) (aTotal - aSeen) <= 0)
		return;
	uint32_t anExternal = aTotal - aSeen;
	mXrandLogged += anExternal;
	// One XRAND per tick: a paused board is entered every frame at the same tick
	if (!mSegment.mEvents.empty() && (mSegment.mEvents.back().mOp == Replay::OP_XRAND) &&
		(mSegment.mEvents.back().mTick == (uint32_t) mBoard->mUpdateCnt))
		mSegment.mEvents.back().mArgs[0] += anExternal;
	else
		Add(Replay::OP_XRAND, anExternal);
}

void ReplayRecorder::AfterUpdate()
{
	if (mEnded)
		return;
	int aState = mBoard->mState;
	if ((aState == STATE_PLAYING) && (mLastState != STATE_PLAYING))
		Add(Replay::OP_CHECK, (int64_t) StateCodec::LogicHash(mBoard, HashRng()));
	mLastState = aState;

	if (aState == STATE_GAME_OVER_DONE)
	{
		// ShowHighScoreWidget has just made the game-over screen: a score its table takes keeps the replay
		bool isPinned = false;
		HighScoreWidget* aWidget = mApp->mHighScoreWidget;
		if (aWidget != NULL)
		{
			int aMode = mApp->mTimedMode ? 1 : 0;
			isPinned = (mBoard->mScore > aWidget->mTodayScores[aMode]) || (aWidget->GetRank(aMode, mBoard->mScore) > 0);
		}
		Finish(mEndReason, isPinned);
	}
}

void ReplayRecorder::OnSelect(Piece* thePiece)
{
	if (!mEnded)
		AddCell(Replay::OP_SELECT, thePiece);
}

void ReplayRecorder::OnDeselect()
{
	if (!mEnded)
		Add(Replay::OP_DESELECT);
}

void ReplayRecorder::OnTarget(Piece* thePiece)
{
	if (!mEnded)
		AddCell(Replay::OP_TARGET, thePiece);
}

void ReplayRecorder::OnTwist(int theCol, int theRow, int theDir)
{
	if (mEnded)
		return;
	Add(Replay::OP_TWIST, theCol, theRow);
	mSegment.mEvents.back().mArgs[2] = theDir;
	mMoves++;
}

void ReplayRecorder::OnSlide(int theAxis, int theLine, int theStart, int theK, int theOffset)
{
	if (mEnded)
		return;
	Add(Replay::OP_SLIDE, theAxis, theLine);
	int64_t* anArgs = mSegment.mEvents.back().mArgs;
	anArgs[2] = theStart;
	anArgs[3] = theK;
	anArgs[4] = theOffset;
	mMoves++;
}

void ReplayRecorder::OnHint()
{
	if (!mEnded)
		Add(Replay::OP_HINT);
}

void ReplayRecorder::OnUndo()
{
	if (!mEnded)
		Add(Replay::OP_UNDO);
}

void ReplayRecorder::OnPause(bool theUserPause)
{
	if (!mEnded && !mSaving)
		Add(Replay::OP_PAUSE, theUserPause ? 1 : 0);
}

void ReplayRecorder::OnUnpause(bool theUserPause)
{
	if (!mEnded && !mSaving)
		Add(Replay::OP_UNPAUSE, theUserPause ? 1 : 0);
}

void ReplayRecorder::OnNewGame()
{
	if (mEnded)
		return;
	Add(Replay::OP_NEW_GAME);
	mNewGame = true;
}

void ReplayRecorder::OnHover(int theX, int theY)
{
	if (mEnded || !mHover || ((theX == mLastHoverX) && (theY == mLastHoverY)))
		return;
	mLastHoverX = theX;
	mLastHoverY = theY;
	// The last position of a tick is the one that counts
	if (!mSegment.mEvents.empty() && (mSegment.mEvents.back().mOp == Replay::OP_HOVER) &&
		(mSegment.mEvents.back().mTick == (uint32_t) mBoard->mUpdateCnt))
	{
		mSegment.mEvents.back().mArgs[0] = theX;
		mSegment.mEvents.back().mArgs[1] = theY;
	}
	else
		Add(Replay::OP_HOVER, theX, theY);
}

// Called by GameOver before it changes the state: no moves (from UpdateFalling), time up (from Update's
// level check, in STATE_PLAYING), or the game over that New Game starts
void ReplayRecorder::OnGameOver()
{
	if (mEnded)
		return;
	if (mNewGame)
		mEndReason = Replay::END_NEW_GAME;
	else
		mEndReason = (mBoard->mState == STATE_FALLING) ? Replay::END_NO_MOVES : (mBoard->mState == STATE_ARROW_STEPPING) ? Replay::END_ARROW_ESCAPED : // port(variant): an arrow escaped (stage 18)
			mBoard->mVariant.mGold ? Replay::END_GOLD_TIME_UP : Replay::END_TIME_UP; // port(variant): a gold level's time (stage 12)
}

void ReplayRecorder::BeginSave()
{
	mSaving = true;
}

std::string ReplayRecorder::EndSave(const std::string& theSave)
{
	mSaving = false;
	if (mEnded)
		return "";
	Add(Replay::OP_SAVE, Replay::SAVE_GAME, Replay::SaveHash(theSave.data(), theSave.size()));
	return Sidecar(theSave, Replay::SAVE_GAME, false);
}

std::string ReplayRecorder::SnapshotSidecar(const std::string& theSave)
{
	if (mEnded)
		return "";
	return Sidecar(theSave, Replay::SAVE_SNAPSHOT, true);
}

// The game so far, ending in the SAVE of theSave (appended for a snapshot only), the RNG the save was made
// with (a board with its own RNG; on the global stream the resume goes on with the stream as it is then),
// the save's hash, and END "saved". Made in mFile itself and put back after, not in a copy: the web backup
// asks for a snapshot every 250 ms (SexyAppBase::UpdateUserDataBackup), and a long recording is big.
std::string ReplayRecorder::Sidecar(const std::string& theSave, int theSaveKind, bool isSnapshot)
{
	Replay::File& aFile = mFile;
	int aFlags = aFile.mFlags;
	aFile.mSegments.push_back(Replay::Segment());
	std::swap(aFile.mSegments.back(), mSegment);
	std::vector<Replay::Event>& anEvents = aFile.mSegments.back().mEvents;
	uint32_t aSaveHash = Replay::SaveHash(theSave.data(), theSave.size());
	if (isSnapshot)
	{
		Replay::Event anEvent;
		anEvent.mTick = (uint32_t) mBoard->mUpdateCnt;
		anEvent.mOp = Replay::OP_SAVE;
		anEvent.mArgs[0] = theSaveKind;
		anEvent.mArgs[1] = aSaveHash;
		anEvents.push_back(anEvent);
	}
	if (aFile.mSegments.size() > 1)
		aFile.mFlags |= Replay::FLAG_MULTI_SEGMENT;
	if (mBoard->mOwnRng != NULL)
	{
		aFile.mHasTrailingRng = true;
		mBoard->GetRngState(&aFile.mTrailingRng);
	}
	aFile.mHasSaveHash = true;
	aFile.mSaveHash = aSaveHash;
	aFile.mEnd = mBoard->GetEndInfo(Replay::END_SAVED, HashRng());
	std::string aBytes = Replay::Write(aFile);

	if (isSnapshot)
		anEvents.pop_back();
	std::swap(aFile.mSegments.back(), mSegment);
	aFile.mSegments.pop_back();
	aFile.mFlags = aFlags;
	aFile.mHasTrailingRng = false;
	aFile.mHasSaveHash = false;
	aFile.mSaveHash = 0;
	aFile.mEnd = Replay::EndInfo();
	return aBytes;
}

size_t ReplayRecorder::EventCount() const
{
	size_t aCount = mSegment.mEvents.size();
	for (size_t i = 0; i < mFile.mSegments.size(); i++)
		aCount += mFile.mSegments[i].mEvents.size();
	return aCount;
}

void ReplayRecorder::OnLoad(const std::string& theSave, const Replay::File* theSidecar)
{
	if (mEnded)
		return;
	Add(Replay::OP_LOAD);
	mLoaded = true;

	// The sidecar continues: its segments come first, if it is a recording of the same rules by this engine, or
	// by an earlier one under rules that play alike in every engine (GameRules::PlaysAlikeInEveryEngine) (the exemption
	// Replay::CheckPlayable makes; the merged replay is this engine's), with room for one segment more (a reader
	// takes at most Replay::MAX_SEGMENTS; else a fresh replay that holds the save)
	bool isSameEngine = (theSidecar != NULL) && ((theSidecar->mEngineAbi == mFile.mEngineAbi) ||
		(mBoard->mRules.PlaysAlikeInEveryEngine() && (theSidecar->mEngineAbi >= (uint32_t) Replay::FIRST_ENGINE_ABI) &&
		(theSidecar->mEngineAbi < (uint32_t) Replay::ENGINE_ABI)));
	bool isContinued = isSameEngine && !theSidecar->mSegments.empty() && theSidecar->mHasRulesId &&
		(memcmp(theSidecar->mRulesId, mFile.mRulesId, Replay::RULES_ID_SIZE) == 0) &&
		(theSidecar->mSegments.size() < (size_t) Replay::MAX_SEGMENTS);
	mContinued = isContinued;
	if (isContinued)
	{
		mFile.mSegments = theSidecar->mSegments;
		// The hover flag says every segment has the hover channel (Replay.h): it stays only when the sidecar's
		// segments and this one's (features.replays.hover now) all have it. Without it the player checks no
		// save bytes, which hold the hover glow. This segment still records HOVER as its setting says.
		mFile.mFlags = (mFile.mFlags & ~Replay::FLAG_HOVER) | (mFile.mFlags & theSidecar->mFlags & Replay::FLAG_HOVER) |
			Replay::FLAG_MULTI_SEGMENT;
		for (size_t i = 0; i < theSidecar->mSegments.size(); i++)
			mMoves += Replay::CountMoves(theSidecar->mSegments[i]);
	}
	else
	{
		// A fresh replay that holds the save it starts from: it can be watched, not ranked (6.5)
		mFile.mSegments.clear();
		mSegment.mHasLoadSave = true;
		mSegment.mLoadSave = theSave;
	}
	// The RNG the board goes on with (Board::GameLoadedVariant has set it)
	mSegment.mHasLoadRng = true;
	mBoard->GetRngState(&mSegment.mLoadRng);
	StartStream();
}

void ReplayRecorder::OnBoardDestroyed()
{
	if (mEnded)
		return;
	if (mSaved)
	{
		// The sidecar holds the game; the resume goes on with it
		mEnded = true;
		return;
	}
	Finish(mNewGame ? Replay::END_NEW_GAME : Replay::END_ABANDONED, false);
}

void ReplayRecorder::Finish(int theReason, bool isPinned)
{
	mEnded = true;
	if (mMoves == 0)
		return; // a game without a move is not kept
	Replay::File aFile = mFile;
	aFile.mSegments.push_back(mSegment);
	if (aFile.mSegments.size() > 1)
		aFile.mFlags |= Replay::FLAG_MULTI_SEGMENT;
	aFile.mHasMeta = true;
	aFile.mMeta = Meta();
	aFile.mEnd = mBoard->GetEndInfo(theReason, HashRng());
	std::string aPath;
	if (WriteFinished(aFile, mBoard->mTimedMode, isPinned, &aPath))
	{
		gFinishedBoard = mBoard->mBoardSerial;
		gFinishedPath = aPath;
	}
}

std::string ReplayRecorder::FinishedReplay(const Board* theBoard)
{
	if ((theBoard == NULL) || (gFinishedBoard != theBoard->mBoardSerial) || gFinishedPath.empty())
		return "";
	struct stat aStat;
	return (stat(gFinishedPath.c_str(), &aStat) == 0) ? gFinishedPath : "";
}

void ReplayRecorder::Renamed(const std::string& theOld, const std::string& theNew)
{
	if (gFinishedPath == theOld)
		gFinishedPath = theNew;
}

bool ReplayRecorder::FirstSidecar()
{
	bool isFirst = !mWroteSidecar;
	mWroteSidecar = true;
	return isFirst;
}

// A sidecar's game that ended by being saved, and whose save is going away (the resume was declined, or this
// board's save replaces it), becomes a finished replay, END "saved"; only when the sidecar goes with
// theSave and holds a move
void ReplayRecorder::KeepSidecarGame(const std::string& theSidecar, const std::string& theSave)
{
	Replay::File aFile;
	if (!Replay::Read(theSidecar, &aFile) || aFile.mSegments.empty() || !aFile.mHasSaveHash ||
		(aFile.mSaveHash != Replay::SaveHash(theSave.data(), theSave.size())))
		return;
	int aMoves = 0;
	for (size_t i = 0; i < aFile.mSegments.size(); i++)
		aMoves += Replay::CountMoves(aFile.mSegments[i]);
	if (aMoves == 0)
		return;
	aFile.mHasTrailingRng = false;
	aFile.mHasSaveHash = false;
	aFile.mHasMeta = true;
	aFile.mMeta = Meta();
	// Its mode: the save's (a LOAD may have changed the board's; a v1 save has it right after its version, a v2
	// save after its header), else the last segment's
	bool isTimed = false;
	if (!Replay::SaveIsTimed(theSave, &isTimed))
		isTimed = (aFile.mSegments.back().mMode != 0);
	WriteFinished(aFile, isTimed, false);
}

// The game's narrow text (Latin-1: typed names keep only those code points, SexyAppBase) as UTF-8
static std::string Latin1ToUTF8(const std::string& theText)
{
	std::string anOut;
	for (size_t i = 0; i < theText.size(); i++)
	{
		unsigned char c = (unsigned char) theText[i];
		if (c < 0x80)
			anOut += (char) c;
		else
		{
			anOut += (char) (0xc0 | (c >> 6));
			anOut += (char) (0x80 | (c & 0x3f));
		}
	}
	return anOut;
}

// META (not verified, UTF-8): the player, when, which mods, which build
std::string ReplayRecorder::Meta()
{
	Platform::JsonValue aMeta = Platform::JsonValue::MakeObject();
	aMeta.Set("name", Platform::JsonValue(Latin1ToUTF8(mApp->mLastScoreName)));
	char aDate[32] = "";
	time_t aNow = time(NULL);
	struct tm* aTime = localtime(&aNow);
	if (aTime != NULL)
		strftime(aDate, sizeof(aDate), "%Y-%m-%dT%H:%M:%S", aTime);
	aMeta.Set("date", Platform::JsonValue(std::string(aDate)));
	Platform::JsonValue aMods = Platform::JsonValue::MakeArray();
	std::string aTitle;
	std::vector<const Platform::ModInfo*> anEnabled = Platform::ModManager::Get().GetEnabledMods();
	for (size_t i = 0; i < anEnabled.size(); i++)
	{
		aMods.Push(Platform::JsonValue(anEnabled[i]->mId + "@" + anEnabled[i]->mVersion));
		if (anEnabled[i]->mHasRules)
			aTitle += (aTitle.empty() ? "" : " + ") + anEnabled[i]->mName;
	}
	aMeta.Set("mods", aMods);
	aMeta.Set("build", Platform::JsonValue(std::string("Bejeweled Deluxe ") + mApp->mProductVersion + " (byo-pop), engineAbi " +
		std::to_string((int) Replay::ENGINE_ABI)));
#ifdef __EMSCRIPTEN__
	aMeta.Set("platform", Platform::JsonValue(std::string("web")));
#else
	aMeta.Set("platform", Platform::JsonValue(std::string("native")));
#endif
	aMeta.Set("title", Platform::JsonValue(aTitle.empty() ? std::string("Original rules") : aTitle));
	return aMeta.Dump(0);
}

// <yyyymmdd>-<hhmmss>-<normal|timed>-<score>[-<n>][-keep].bjr; *theCopy: n (2 and up for a name that was taken),
// 1 without it
bool ReplayRecorder::IsReplayName(const std::string& theName, bool* isPinned, int* theCopy)
{
	const char* p = theName.c_str();
	for (int i = 0; i < 15; i++, p++)
	{
		if ((i == 8) ? (*p != '-') : ((*p < '0') || (*p > '9')))
			return false;
	}
	if (*p++ != '-')
		return false;
	if (strncmp(p, "normal-", 7) == 0)
		p += 7;
	else if (strncmp(p, "timed-", 6) == 0)
		p += 6;
	else
		return false;
	if ((*p < '0') || (*p > '9'))
		return false;
	while ((*p >= '0') && (*p <= '9'))
		p++;
	int aCopy = 1;
	if ((p[0] == '-') && (p[1] >= '0') && (p[1] <= '9'))
	{
		p++;
		aCopy = 0;
		while ((*p >= '0') && (*p <= '9'))
		{
			if (aCopy < 100000)
				aCopy = aCopy * 10 + (*p - '0');
			p++;
		}
	}
	bool isKeep = (strncmp(p, "-keep", 5) == 0);
	if (isKeep)
		p += 5;
	if (strcmp(p, ".bjr") != 0)
		return false;
	if (isPinned != NULL)
		*isPinned = isKeep;
	if (theCopy != NULL)
		*theCopy = aCopy;
	return true;
}

bool ReplayRecorder::WriteFinished(const Replay::File& theFile, bool isTimed, bool isPinned, std::string* thePath)
{
	const std::string& aDir = gGameConfig.mReplayDir;
	if (aDir.empty())
		return false;
	Platform::MakeDirs(aDir);

	char aStamp[32] = "00000000-000000";
	time_t aNow = time(NULL);
	struct tm* aTime = localtime(&aNow);
	if (aTime != NULL)
		strftime(aStamp, sizeof(aStamp), "%Y%m%d-%H%M%S", aTime);
	std::string aBase = std::string(aStamp) + (isTimed ? "-timed-" : "-normal-") + std::to_string(theFile.mEnd.mScore);
	std::string aSuffix = isPinned ? "-keep.bjr" : ".bjr";
	std::string aPath = aDir + "/" + aBase + aSuffix;
	for (int n = 2; n < 1000; n++)
	{
		FILE* anExisting = fopen(aPath.c_str(), "rb");
		if (anExisting == NULL)
			break;
		fclose(anExisting);
		aPath = aDir + "/" + aBase + "-" + std::to_string(n) + aSuffix;
	}

	std::string aBytes = Replay::Write(theFile);
	FILE* aFile = fopen(aPath.c_str(), "wb");
	bool isWritten = (aFile != NULL) && (fwrite(aBytes.data(), 1, aBytes.size(), aFile) == aBytes.size());
	if ((aFile != NULL) && (fclose(aFile) != 0))
		isWritten = false;
	if (!isWritten)
	{
		SDL_Log("replays: cannot write %s", aPath.c_str());
		return false;
	}
	Prune(aDir, gGameConfig.mFeatures.mReplayKeep);
	Platform::SyncUserData();
	if (thePath != NULL)
		*thePath = aPath;
	return true;
}

// The newest theKeep replays the game named, every "-keep" one, and every other file stay. Newest by the
// time the file was written (its mtime, UTC: no daylight-saving or time-zone change reorders them); within
// one second by the name's local time, then its copy number (a taken name's "-2" came after it), then the
// name.
void ReplayRecorder::Prune(const std::string& theDir, int theKeep)
{
	struct Aged
	{
		long long	mTime;
		std::string	mStamp;
		int			mCopy;
		std::string	mName;
		bool operator<(const Aged& o) const
		{
			if (mTime != o.mTime)
				return mTime < o.mTime;
			if (mStamp != o.mStamp)
				return mStamp < o.mStamp;
			if (mCopy != o.mCopy)
				return mCopy < o.mCopy;
			return mName < o.mName;
		}
	};
	std::vector<std::string> aNames = Platform::ListDirectory(theDir);
	std::vector<Aged> anUnpinned;
	for (size_t i = 0; i < aNames.size(); i++)
	{
		bool isPinned = false;
		int aCopy = 1;
		if (!IsReplayName(aNames[i], &isPinned, &aCopy) || isPinned)
			continue;
		struct stat aStat;
		Aged anAged;
		anAged.mTime = (stat((theDir + "/" + aNames[i]).c_str(), &aStat) == 0) ? (long long) aStat.st_mtime : 0;
		anAged.mStamp = aNames[i].substr(0, 15);
		anAged.mCopy = aCopy;
		anAged.mName = aNames[i];
		anUnpinned.push_back(anAged);
	}
	std::sort(anUnpinned.begin(), anUnpinned.end());
	for (size_t i = 0; (int) (anUnpinned.size() - i) > theKeep; i++)
		remove((theDir + "/" + anUnpinned[i].mName).c_str());
}
