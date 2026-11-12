// port: a game's improvements as options (platform/GameOptions.h).

#include "GameOptions.h"
#include <map>

namespace Platform
{
static OptionSource gOptionSource = nullptr;
static bool gOriginal = false;										// -original
static std::map<std::string, bool> gOptionParams[NUM_OPTION_GROUPS];	// -fix=<id>[=on|off], -lite=<id>[=on|off]

void SetOptionSource(OptionSource theSource)
{
	gOptionSource = theSource;
}

bool OptionOn(OptionGroup theGroup, const char* theId)
{
	std::map<std::string, bool>::const_iterator aParam = gOptionParams[theGroup].find(theId);
	if (aParam != gOptionParams[theGroup].end())
		return aParam->second;
	if (gOriginal)
		return false;
	bool aValue = false;
	if ((gOptionSource != nullptr) && gOptionSource(theGroup, theId, &aValue))
		return aValue;
	return false;
}

bool HandleOptionParam(const std::string& theName, const std::string& theValue)
{
	if (theName == "-original")
	{
		gOriginal = true;
		return true;
	}
	OptionGroup aGroup;
	if (theName == "-fix")
		aGroup = OPTION_FIX;
	else if (theName == "-lite")
		aGroup = OPTION_LITE;
	else
		return false;
	// <id>, or <id>=on|off (also 1|0, true|false)
	std::string anId = theValue;
	bool isOn = true;
	size_t anEquals = theValue.find('=');
	if (anEquals != std::string::npos)
	{
		anId = theValue.substr(0, anEquals);
		std::string aWord = theValue.substr(anEquals + 1);
		isOn = !((aWord == "off") || (aWord == "0") || (aWord == "false"));
	}
	if (!anId.empty())
		gOptionParams[aGroup][anId] = isOn;
	return true;
}

void ResetOptionParams()
{
	gOriginal = false;
	for (int i = 0; i < NUM_OPTION_GROUPS; i++)
		gOptionParams[i].clear();
}
}
