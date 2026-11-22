#ifndef __SPROUT2_PERSISTKEY_H__
#define __SPROUT2_PERSISTKEY_H__

// A key of the settings that last between runs (engine::PersistKey, the interface at +0x0c of the host's
// dxdriver::RegistryKey, whose slots are vtable 0x5629e8 (feedingfrenzy2)): a registry key, its keys below and its
// values. The engine opens one (Engine::openPersistKey); the users' list keeps its players under one. The interface's
// slots 0x00 and 0x04 open the key the engine has just made (below another key, or under a root), and slot 0x08 closes
// it; the port's host does those in its key's constructor and destructor.

#include "Object.h"

#include <string>
#include <vector>

namespace engine
{

class PersistKey : public Object
{
public:
	// 0x0c (0x494220 (feedingfrenzy2)): the names of the keys below this one, added to the list; true when the list
	// ran to its end (ERROR_NO_MORE_ITEMS)
	virtual bool getSubKeyNames(std::vector<std::string>& theNames) = 0;
	// 0x10 (0x5079a2 (feedingfrenzy2)): the key below this one by that name deleted (RegDeleteKey, so not one with keys
	// of its own); false when it cannot be
	virtual bool deleteSubKey(const std::string& theName) = 0;
	// 0x14 (0x4187d0 (feedingfrenzy2)): the names of the values in this key, added to the list; true when the list
	// ran to its end
	virtual bool getValueNames(std::vector<std::string>& theNames) = 0;
	// 0x18 (0x4767c0 (feedingfrenzy2)): a string value set; false (and a line in the log) when it cannot be
	virtual bool writeString(const std::string& theName, const std::string& theValue) = 0;
	// 0x1c (0x46f0a0 (feedingfrenzy2)): a string value, theDefault when there is none or it is not a string
	virtual std::string readString(const std::string& theName, const std::string& theDefault) = 0;
	// (0x20 (0x40c230) and 0x24 (0x488480) (feedingfrenzy2), a DWORD value set and read, are added with the code that
	// calls them)
};

} // namespace engine

#endif
