#include "ModRules.h"
#include "Board.h"

#include "platform/ModManager.h"

#include <SDL.h>
#include <algorithm>

using namespace Sexy;

BoardTunables::BoardTunables()
	: mGroupPoints(gGroupBaseScores, gGroupBaseScores + 14)
{
}

int BoardTunables::GroupPoints(int theCount) const
{
	int anIndex = std::max(theCount - 2, 0);
	int aLast = (int) mGroupPoints.size() - 1;
	if (anIndex <= aLast)
		return mGroupPoints[anIndex];
	return mGroupPoints[aLast] + (anIndex - aLast) * mGroupPointsStep;
}

double Sexy::ModTunable(const char* theKey, double theDefault, double theMin, double theMax)
{
	Platform::ModManager& aMods = Platform::ModManager::Get();
	if (!aMods.HasTunable(theKey))
		return theDefault;
	double aValue = std::min(std::max(aMods.GetTunable(theKey, theDefault), theMin), theMax);
	SDL_Log("tunable %s = %g", theKey, aValue);
	return aValue;
}

int Sexy::ModTunableInt(const char* theKey, int theDefault, int theMin, int theMax)
{
	Platform::ModManager& aMods = Platform::ModManager::Get();
	if (!aMods.HasTunable(theKey))
		return theDefault;
	// clamped as a double first, so a huge value converts to int defined (GetTunableInt would not)
	int aValue = (int) std::min(std::max(aMods.GetTunable(theKey, theDefault), (double) theMin), (double) theMax);
	SDL_Log("tunable %s = %d", theKey, aValue);
	return aValue;
}

bool Sexy::ModTunableBool(const char* theKey, bool theDefault)
{
	Platform::ModManager& aMods = Platform::ModManager::Get();
	if (!aMods.HasTunable(theKey))
		return theDefault;
	bool aValue = aMods.GetTunableBool(theKey, theDefault);
	SDL_Log("tunable %s = %s", theKey, aValue ? "true" : "false");
	return aValue;
}

// theFileName with theSuffix before its extension
static std::string WithSuffix(const std::string& theFileName, const std::string& theSuffix)
{
	if (theSuffix.empty())
		return theFileName;
	size_t aDot = theFileName.find_last_of('.');
	size_t aSlash = theFileName.find_last_of("/\\");
	if ((aDot == std::string::npos) || ((aSlash != std::string::npos) && (aDot < aSlash)))
		return theFileName + theSuffix;
	return theFileName.substr(0, aDot) + theSuffix + theFileName.substr(aDot);
}

std::string Sexy::ScopedUserFileName(const std::string& theFileName, bool theSeeded)
{
	const std::string& aKey = Platform::ModManager::Get().GetScoreKey();
	return WithSuffix(theFileName, (aKey.empty() ? std::string() : "-" + aKey) + (theSeeded ? "-seeded" : ""));
}
