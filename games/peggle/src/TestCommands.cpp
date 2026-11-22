// TestCommands.cpp: the test commands of Peggle (docs/TEST_COMMANDS.md), handled through ThunderballApp::HandleScriptCommand. A
// command that is never given changes nothing. Each answers on stdout with one line: STATE {...}, OK <command> or
// ERROR <command>: <reason>. They act through the game's own code.

#include "Board.h"
#include "Gun.h"
#include "LevelFile.h"
#include "LogicMgr.h"
#include "NotTranslated.h"
#include "MainMenu.h"
#include "PhysObj.h"
#include "PlayerInfo.h"
#include "ThunderballApp.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <vector>
#include <string>

using namespace Sexy;

static void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

static std::string Words(const ScriptCommand& theCommand)
{
	return theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);
}

bool ThunderballApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand == "profile")
	{
		// makes the profile of that name if there is none and takes it as the one in use (the test's way round the dialog
		// that asks a new player for a name)
		std::string aName = theCommand.mArgs;
		if (aName.empty())
		{
			Reply("ERROR " + Words(theCommand) + ": needs a name");
			return true;
		}
		PlayerInfo* aProfile = mProfileMgr.GetProfile(aName);
		if (aProfile == nullptr)
		{
			aProfile = mProfileMgr.AddProfile(aName);
			mProfileMgr.Save();
		}
		mCurrentPlayer = aProfile;
		if (mMainMenu != nullptr)
			mMainMenu->SyncPlayerInfo();
		Reply("OK " + Words(theCommand));
		return true;
	}

	if (theCommand.mCommand == "checklevels")
	{
		// checklevels <folder>: reads every level file of the folder with the game's own reader and writes it back out with
		// the same code; each file must be used up to its last byte and must come out as it went in. One line a file.
		std::string aFolder = theCommand.mArgs;
		int aFiles = 0, aBad = 0;
		std::vector<std::filesystem::path> aPaths;
		std::error_code anError;
		for (const std::filesystem::directory_entry& anEntry : std::filesystem::directory_iterator(aFolder, anError))
			if (anEntry.path().extension() == ".dat")
				aPaths.push_back(anEntry.path());
		if (anError)
		{
			Reply("ERROR " + Words(theCommand) + ": " + anError.message());
			return true;
		}
		std::sort(aPaths.begin(), aPaths.end());
		for (const std::filesystem::path& aPath : aPaths)
		{
			std::ifstream aFile(aPath, std::ios::binary);
			std::vector<uint8_t> aBytes((std::istreambuf_iterator<char>(aFile)), std::istreambuf_iterator<char>());
			aFiles++;
			std::string aName = aPath.filename().string();
			try
			{
				PieceList aPieces;
				int aVersion = 0;
				bool aFlag = false;
				size_t aUsed = 0;
				std::vector<size_t> aReadStarts, aWriteStarts;
				ReadLevelData(aBytes.data(), aBytes.size(), aPieces, &aVersion, &aFlag, &aUsed, &aReadStarts);
				// the reader replaces the saved rotation of a piece that has a mover with the mover's own, so that is the one
				// thing a written file cannot keep: the file is read again with it kept and that is the one written back
				PieceList aKeptPieces;
				gSyncKeepSavedRotation = true;
				ReadLevelData(aBytes.data(), aBytes.size(), aKeptPieces);
				gSyncKeepSavedRotation = false;
				int aClassCounts[9] = {0};
				for (PieceList::iterator anItr = aPieces.begin(); anItr != aPieces.end(); ++anItr)
				{
					int aClass = (*anItr)->GetClass();
					aClassCounts[aClass >= 1 && aClass <= 8 ? aClass : 0]++;
				}
				std::vector<uint8_t> aWritten = WriteLevelData(aVersion, aFlag, aKeptPieces, &aWriteStarts);
				std::string aResult = "OK";
				if (aUsed != aBytes.size())
					aResult = "UNUSED BYTES " + std::to_string(aBytes.size() - aUsed);
				else if (aWritten != aBytes)
				{
					size_t aFirst = 0;
					while (aFirst < aWritten.size() && aFirst < aBytes.size() && aWritten[aFirst] == aBytes[aFirst])
						aFirst++;
					aResult = "ROUND TRIP DIFFERS at " + std::to_string(aFirst) + " (wrote " + std::to_string(aWritten.size()) +
							  " of " + std::to_string(aBytes.size()) + ")";
					std::string aHex;
					char aDigits[8];
					for (size_t b = (aFirst > 6 ? aFirst - 6 : 0); b < aFirst + 14; b++)
					{
						snprintf(aDigits, sizeof(aDigits), "%s%02x/%s", b == aFirst ? "|" : "", b < aBytes.size() ? aBytes[b] : 0, "");
						aHex += aDigits;
						snprintf(aDigits, sizeof(aDigits), "%02x ", b < aWritten.size() ? aWritten[b] : 0);
						aHex += aDigits;
					}
					size_t aPiece = 0;
					while (aPiece + 1 < aReadStarts.size() && aReadStarts[aPiece + 1] <= aFirst)
						aPiece++;
					PieceList::iterator aPieceItr = aKeptPieces.begin();
					std::advance(aPieceItr, aPiece);
					Reply("DIFF " + aName + " piece " + std::to_string(aPiece) + " of class " + std::to_string((*aPieceItr)->GetClass()) +
						  " starts at " + std::to_string(aReadStarts[aPiece]) + " (written at " + std::to_string(aWriteStarts[aPiece]) +
						  ") (file/written) " + aHex);
					for (int aSide = 0; aSide < 2; aSide++)
					{
						const std::vector<uint8_t>& aData = aSide == 0 ? aBytes : aWritten;
						const std::vector<size_t>& aStarts = aSide == 0 ? aReadStarts : aWriteStarts;
						size_t aEnd = aPiece + 1 < aStarts.size() ? aStarts[aPiece + 1] : aData.size();
						std::string aDump;
						for (size_t b = aStarts[aPiece]; b < aEnd && b < aData.size(); b++)
						{
							snprintf(aDigits, sizeof(aDigits), "%02x ", aData[b]);
							aDump += aDigits;
						}
						Reply(std::string(aSide == 0 ? "FILE   " : "WRITTEN") + " piece " + aDump);
					}
				}
				if (aResult != "OK")
					aBad++;
				std::string aLine = aName + " v" + std::to_string(aVersion) + (aFlag ? "f" : "") + " " + std::to_string(aBytes.size()) + "B " +
									std::to_string(aPieces.size()) + " pieces [";
				for (int i = 1; i <= 8; i++)
					aLine += (i > 1 ? " " : "") + std::to_string(aClassCounts[i]);
				Reply("LEVEL " + aLine + "] " + aResult);
			}
			catch (std::exception& anException)
			{
				aBad++;
				Reply("LEVEL " + aName + " EXCEPTION " + anException.what());
			}
		}
		Reply((aBad == 0 ? "OK " : "ERROR ") + Words(theCommand) + ": " + std::to_string(aFiles) + " files, " + std::to_string(aBad) + " bad");
		return true;
	}

	if (theCommand.mCommand == "loadlevel" || theCommand.mCommand == "resetlevel")
	{
		// loadlevel <seed> <path>: puts a level on a board the way Board::Reset would (the reset count is 1) with the generator
		// seeded, and tells what the rules made of it: the pegs by their serial number (kinds: 1 blue, 2 orange, 3 purple, 4 green),
		// the cannon's aim and how many draws the level start took. Drawing the original does and the port has not reached is skipped.
		// resetlevel <seed> <path> is the same through Board::Reset (the board's mode is the app's, the music is not played
		// when the board is hidden: "hidden" as a third word hides it).
		size_t aSpace = theCommand.mArgs.find(' ');
		if (aSpace == std::string::npos)
		{
			Reply("ERROR " + Words(theCommand) + ": needs a seed and a path");
			return true;
		}
		unsigned long aSeed = std::stoul(theCommand.mArgs.substr(0, aSpace));
		std::string aPath = theCommand.mArgs.substr(aSpace + 1);
		gSkipUntranslatedDrawing = true;
		if (mBoard == nullptr)
			mBoard = new Board(this);
		Board* aBoard = static_cast<Board*>(mBoard);
		aBoard->mResetCount = 1;
		SRand(aSeed);
		uint aDrawsBefore = GetRandCount();
		bool aLoaded = true;
		if (theCommand.mCommand == "resetlevel")
		{
			size_t aSecondSpace = aPath.find(' ');
			if (aSecondSpace != std::string::npos)
			{
				aBoard->mVisible = aPath.substr(aSecondSpace + 1) != "hidden";
				aPath = aPath.substr(0, aSecondSpace);
			}
			aBoard->mLevelName = aPath;
			aBoard->Reset();
		}
		else
			aLoaded = aBoard->LoadLevel(aPath);
		LogicMgr* aLogic = aBoard->mLogicMgr.get();
		std::string aState = "STATE {\"loaded\":" + std::string(aLoaded ? "true" : "false") + ",\"objects\":" +
							 std::to_string(aBoard->mObjects.size()) + ",\"pegs\":" + std::to_string(aLogic->mPegs.size()) + ",\"goal\":" +
							 std::to_string(aLogic->mGoalPegs.size()) + ",\"other\":" + std::to_string(aLogic->mOtherPegs.size()) +
							 ",\"draws\":" + std::to_string(GetRandCount() - aDrawsBefore) + ",\"scoreMult\":" +
							 std::to_string(aLogic->mScoreMult) + ",\"music\":" + std::to_string(aLogic->mMusicIntensity) + ",\"balls\":" +
							 std::to_string(aLogic->mNumBalls[aLogic->mCurPlayer]);
		char aDigits[32];
		snprintf(aDigits, sizeof(aDigits), "%.9g", (double)aLogic->mGunAngle);
		aState += ",\"gunAngle\":" + std::string(aDigits) + ",\"kinds\":\"";
		for (std::list<SharedPtr<PhysObj> >::iterator anIter = aLogic->mPegs.begin(); anIter != aLogic->mPegs.end(); ++anIter)
			aState += (char)('0' + (*anIter)->mPegInfo->mKind);
		aState += "\"}";
		Reply(aState);
		Reply((aLoaded ? "OK " : "ERROR ") + Words(theCommand));
		return true;
	}

	return false;
}
