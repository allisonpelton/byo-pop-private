// Sexy::LuaResourceManager (constructor 0x65d3c0, vtable of 19 slots): a ResourceManager whose resources the
// scripts see as globals named by their ids: images as LuaImage handles, sounds as their ids, fonts as LuaFont
// handles (borrowed). The resources loaded since the last export are kept (ResourceLoadedHook 0x65d930) until
// ExportToLua (0x65d950) sets their globals; deleting a group sets its globals to nil again (0x65d9b0).
#pragma once

#include <set>
#include <string>

#include "ResourceManager.h"

namespace Sexy {

class PopLua;

class LuaResourceManager : public ResourceManager {
public:
	PopLua* mLua;  // +0xcc

	LuaResourceManager(SexyAppBase* theApp, PopLua* theLua);
	~LuaResourceManager() override;

	void ResourceLoadedHook(BaseRes* theRes) override;
	void DeleteResources(const std::string& theGroup) override;

	// Sets the globals of the resources loaded since the last call
	void ExportToLua();

	// The resource of an id, for the scripts' code in C++ (0x65d500, 0x65d580, 0x65d5f0)
	SharedImageRef LuaGetImage(const std::string& theId) { return GetImage(theId); }
	int LuaGetSound(const std::string& theId) { return GetSound(theId); }

protected:
	std::set<BaseRes*> mLoaded;  // +0xd0

	void PushResource(BaseRes* theRes);  // 0x65d660
};

extern LuaResourceManager* gLuaResourceManager;  // 0x825cdc

}  // namespace Sexy
