// gridtest: replays the shipped best solutions (levels/maxedscores.txt: "<uuid>#<moves>#<move strings>#") on the
// Grid rules and checks that every puzzle ends solved after exactly those moves.
// usage: pixelus_gridtest <game dir> [-v]
#include "Grid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <dirent.h>

using namespace Sexy;

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("usage: %s <game dir> [-v]\n", argv[0]);
		return 2;
	}
	std::string aDir = argv[1];
	bool aVerbose = argc > 2 && strcmp(argv[2], "-v") == 0;
	std::string aLevels = aDir + "/levels/";

	// uuid -> level file
	std::map<std::string, std::string> aByUuid;
	int aFiles = 0;
	if (DIR* d = opendir(aLevels.c_str()))
	{
		while (dirent* e = readdir(d))
		{
			std::string aName = e->d_name;
			if (aName.size() < 5 || aName.substr(aName.size() - 4) != ".txt" || aName == "levels.txt" || aName == "maxedscores.txt")
				continue;
			Grid g;
			std::string anErr;
			if (!g.LoadFromFile(aLevels + aName, &anErr))
			{
				printf("LOAD FAIL %s: %s\n", aName.c_str(), anErr.c_str());
				continue;
			}
			aFiles++;
			aByUuid[g.mUuid] = aName;
		}
		closedir(d);
	}

	std::ifstream aScores((aLevels + "maxedscores.txt").c_str());
	std::string aLine;
	int aTotal = 0, aOk = 0, aMissing = 0;
	while (std::getline(aScores, aLine))
	{
		if (!aLine.empty() && aLine[aLine.size() - 1] == '\r')
			aLine.erase(aLine.size() - 1);
		size_t p1 = aLine.find('#');
		size_t p2 = p1 == std::string::npos ? p1 : aLine.find('#', p1 + 1);
		if (p2 == std::string::npos)
			continue;
		std::string aUuid = aLine.substr(0, p1);
		int aCount = atoi(aLine.substr(p1 + 1, p2 - p1 - 1).c_str());
		std::string aMoves = aLine.substr(p2 + 1);
		if (!aMoves.empty() && aMoves[aMoves.size() - 1] == '#')
			aMoves.erase(aMoves.size() - 1);
		aTotal++;

		std::map<std::string, std::string>::iterator it = aByUuid.find(aUuid);
		if (it == aByUuid.end())
		{
			aMissing++;
			continue;
		}
		Grid g;
		g.LoadFromFile(aLevels + it->second);
		int aDone = 0;
		bool aFailed = false;
		std::string aWhy;
		for (size_t i = 0; i + 6 <= aMoves.size(); i += 6)
		{
			Grid::Move aMove;
			if (!Grid::ParseMove(aMoves.substr(i, 6), &aMove))
			{
				aFailed = true;
				aWhy = "bad move " + aMoves.substr(i, 6);
				break;
			}
			if (!g.ReplayMove(aMove))
			{
				aFailed = true;
				char b[100];
				sprintf(b, "move %d (%s) refused", (int)(i / 6), aMoves.substr(i, 6).c_str());
				aWhy = b;
				break;
			}
			if (aMove.mRemove && g.mHistory.back().mPosition != aMove.mPosition)
			{
				aFailed = true;
				char b[100];
				sprintf(b, "move %d (%s): the pull ends at position %d", (int)(i / 6), aMoves.substr(i, 6).c_str(), g.mHistory.back().mPosition);
				aWhy = b;
				break;
			}
			aDone++;
			if (g.mDone && i + 6 < aMoves.size())
			{
				aFailed = true;
				aWhy = "solved early";
				break;
			}
		}
		if (!aFailed && !g.mDone)
		{
			aFailed = true;
			aWhy = "not solved at the end";
		}
		if (!aFailed && aDone != aCount)
		{
			aFailed = true;
			aWhy = "move count differs";
		}
		if (aFailed)
			printf("FAIL %s (%s): %s [empty wanted %d, supply %d]\n", it->second.c_str(), aUuid.c_str(), aWhy.c_str(), g.mEmptyWanted, g.mSupply);
		else
		{
			aOk++;
			if (aVerbose)
				printf("ok %s %d moves\n", it->second.c_str(), aDone);
		}
	}
	printf("gridtest: %d level files, %d solutions, %d solved, %d without a level file\n", aFiles, aTotal, aOk, aMissing);
	return (aOk == aTotal - aMissing && aTotal > 0) ? 0 : 1;
}
