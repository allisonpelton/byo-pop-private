#ifndef __FEEDINGFRENZY_FISHFACTORY_H__
#define __FEEDINGFRENZY_FISHFACTORY_H__

// The kinds of fish a fish file's "class" names (the registry made at 0x420f50, one creator for each)

#include "FishBase.h"
#include <string>

namespace feedingfrenzy
{

// 0x41e9e0 to 0x41f5b0: a new fish of the class, with its type name set; null for a name no class has
FishBase* createFishOfClass(const std::string& theClassName);

} // namespace feedingfrenzy

#endif
