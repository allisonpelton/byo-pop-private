#ifndef __NINGPO_HCVAR_H__
#define __NINGPO_HCVAR_H__

// HCvarManager (0x413a16..0x413c9e): the console variables. The original keeps them in a Lua table of
// {value, flags}; port: a map.

#include <map>
#include <string>

namespace pg
{

class HCvarManager
{
public:
	struct Cvar
	{
		std::string mValue;
		unsigned int mFlags;
	};
	std::map<std::string, Cvar> mVars;

	// 0x413a9c: flag 1 keeps a value already set (from the command line)
	void registerVariable(const char* theName, const char* theValue, unsigned int theFlags);
	// 0x413b3d
	// 0x413b81: atoi of the value, 0 for an unknown name
	int getVariableInt(const char* theName);
	// 0x413bab: nullptr for an unknown name
	const char* getVariableString(const char* theName);
};

} // namespace pg

#endif
