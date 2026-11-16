#include "Params.h"

#include <cstdlib>

namespace engine
{

std::string Params::getString(const std::string& theName, const std::string& theDefault) const
{
	std::map<std::string, std::string>::const_iterator anIt = mValues.find(theName);
	if (anIt == mValues.end())
		return theDefault;
	return anIt->second;
}

int Params::getInt(const std::string& theName, int theDefault) const
{
	std::map<std::string, std::string>::const_iterator anIt = mValues.find(theName);
	if (anIt == mValues.end())
		return theDefault;
	return (int)atol(anIt->second.c_str());
}

} // namespace engine
