// HCvarManager (see HCvar.h).

#include "HCvar.h"

#include <cstdlib>

namespace pg
{

// 0x413a9c
void HCvarManager::registerVariable(const char* theName, const char* theValue, unsigned int theFlags)
{
	auto it = mVars.find(theName);
	if (it != mVars.end() && (it->second.mFlags & 1) != 0)
		return;
	Cvar& aVar = mVars[theName];
	aVar.mValue = theValue;
	aVar.mFlags = theFlags;
}

// 0x413b81
int HCvarManager::getVariableInt(const char* theName)
{
	const char* aValue = getVariableString(theName);
	if (aValue == nullptr)
		return 0;
	return atoi(aValue);
}

// 0x413bab
const char* HCvarManager::getVariableString(const char* theName)
{
	auto it = mVars.find(theName);
	if (it == mVars.end())
		return nullptr;
	return it->second.mValue.c_str();
}

} // namespace pg
