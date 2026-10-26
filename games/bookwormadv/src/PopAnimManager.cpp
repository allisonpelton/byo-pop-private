#include "PopAnimManager.h"

#include "Graphics.h"
#include "LuaInterface.h"
#include "lauxlib.h"

using namespace Sexy;

PopAnimManager::~PopAnimManager()
{
	for (auto& anEntry : mAnims)
		delete anEntry.second.mAnim;
}

void PopAnimManager::Add(int theId, PopAnim* theAnim, const std::string& theCallback, bool theClone)
{
	Entry& anEntry = mAnims[theId];
	anEntry.mAnim = theAnim;
	anEntry.mCallback = theCallback;
	if (theClone)
		mClones.push_back(theId);
}

PopAnim* PopAnimManager::Get(int theId) const
{
	auto it = mAnims.find(theId);
	return it == mAnims.end() ? nullptr : it->second.mAnim;
}

void PopAnimManager::Update(lua_State* L, int theId)
{
	auto it = mAnims.find(theId);
	if (it == mAnims.end())
		luaL_error(L, "UPDATE LUA POP ANIM ERROR: id = %d: not found", theId);
	if (it->second.mAnim->IsPlaying())
		it->second.mAnim->UpdateF(1.0f);
}

void PopAnimManager::Draw(lua_State* L, Graphics* g, int theId, int theX, int theY)
{
	g->Translate(theX, theY);
	auto it = mAnims.find(theId);
	if (it == mAnims.end()) {
		g->Translate(-theX, -theY);
		luaL_error(L, "DRAW LUA POP ANIM ERROR: id = %d, x = %d, y = %d: not found", theId, theX, theY);
	}
	it->second.mAnim->Draw(g);
	g->Translate(-theX, -theY);
}

void PopAnimManager::ChangeCallback(lua_State* L, int theId, const std::string& theCallback)
{
	auto it = mAnims.find(theId);
	if (it == mAnims.end())
		luaL_error(L, "CHANGE LUA POP ANIM CALLBACK ERROR: id = %d: not found", theId);
	it->second.mCallback = theCallback;
}

void PopAnimManager::Delete(int theId)
{
	lua_pushint(gLuaInterface->L, theId);
	gLuaInterface->Call("PAMAnimator.AnimDeleted", 1, 0);
	auto it = mAnims.find(theId);
	if (it == mAnims.end())
		return;
	delete it->second.mAnim;
	mAnims.erase(it);
}

void PopAnimManager::DeleteClones()
{
	for (int anId : mClones)
		Delete(anId);
	mClones.clear();
}

void PopAnimManager::PopAnimStopped(int theId)
{
	lua_State* L = gLuaInterface->L;
	lua_pushint(L, theId);
	lua_pushstring(L, "STOPPED");
	lua_pushstring(L, "_INTERNAL_MESSAGE_");
	gLuaInterface->Call(mAnims[theId].mCallback, 3, 0);
}

void PopAnimManager::PopAnimCommand(int theId, const std::string& theCommand, const std::string& theParam)
{
	lua_State* L = gLuaInterface->L;
	lua_pushint(L, theId);
	lua_pushstring(L, theCommand.c_str());
	lua_pushstring(L, theParam.c_str());
	gLuaInterface->Call(mAnims[theId].mCallback, 3, 0);
}
