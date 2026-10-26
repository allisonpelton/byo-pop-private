#include "LuaInterface.h"

#include <SDL.h>

#include "Color.h"
#include "ConversationPanel.h"
#include "GameApp.h"
#include "Graphics.h"
#include "JeffLib.h"
#include "LuaBind.h"
#include "LuaImage.h"
#include "MemoryImage.h"
#include "PopAnimManager.h"
#include "Profile.h"
#include "SexyAppBase.h"
#include "lauxlib.h"

using namespace Sexy;
using namespace LuaLib;

LuaInterface* Sexy::gLuaInterface = nullptr;

// The functions read their arguments from the stack themselves, as the original's do

// sexy.PrintConsole(text), 0x4c4540: to the debug console
static int PrintConsole(lua_State* L)
{
	const char* aText = lua_tostring(L, -1);
	if (aText == nullptr)
		luaL_typerror(L, -1, "string");
	SDL_Log("%s", aText);
	return 1;
}

// sexy.NextRand(), 0x4c4640: the framework's random number
static int NextRand(lua_State* L)
{
	lua_pushint(L, Rand());
	return 1;
}

// sexy.PlaySample(id[, pan]), 0x4c4670
static int PlaySample(lua_State* L)
{
	if (lua_gettop(L) == 1)
		gSexyAppBase->PlaySample(lua_toint(L, 1));
	else
		gSexyAppBase->PlaySample(lua_toint(L, 1), lua_toint(L, 2));
	return 0;
}

// sexy.DebugDialog, sexy.DebugMsgBox (0x4c4700, 0x4c4970): the developers' message boxes, used by cheat.lua only.
// sexy.PrintWarning, sexy.PrintError (0x4c4b00, 0x4c4c10): coloured lines on the debug console.
static int DebugMessage(lua_State* L)
{
	const char* aText = lua_tostring(L, 1);
	SDL_Log("script: %s", aText != nullptr ? aText : "");
	return 0;
}

// sexy.GetOffsetRectFromImage(image), 0x4c4d50: the box around the image's white pixels (any alpha), as x, y,
// width, height; the whole image when it has none
static int GetOffsetRectFromImage(lua_State* L)
{
	if (lua_gettop(L) != 1)
		luaL_error(L, "1 parameter expected");
	LuaImage* anImage = (LuaImage*)gLuaSupport->CheckObject(1, ClassOf<LuaImage>::info);
	if (anImage == nullptr)
		luaL_typerror(L, 1, "LuaImage");
	MemoryImage* aMemImage = (MemoryImage*)anImage->mImage;
	if (aMemImage == nullptr)
		luaL_error(L, "Image is not a memory image");
	const ulong* aBits = aMemImage->GetBits();
	int aWidth = aMemImage->GetWidth();
	int aMinX = -1, aMaxX = -1, aMinY = -1, aMaxY = -1;
	int aRow = 0;
	int aRowStart = 0;
	for (int i = 0; i < aMemImage->GetHeight() * aWidth; i++) {
		if (i % aWidth == 0 && i > 0) {
			aRow++;
			aRowStart += aWidth;
		}
		int x = i - aRowStart;
		if ((aBits[i] & 0xffffff) == 0xffffff) {
			if (aMinX == -1 || x < aMinX)
				aMinX = x;
			if (x > aMaxX)
				aMaxX = x;
			if (aMinY == -1 || aRow < aMinY)
				aMinY = aRow;
			if (aRow > aMaxY)
				aMaxY = aRow;
		}
	}
	int aX = aMinX != -1 ? aMinX : 0;
	int aY = aMinY != -1 ? aMinY : 0;
	lua_pushint(L, aX);
	lua_pushint(L, aY);
	lua_pushint(L, aMaxX == -1 ? aWidth : aMaxX - aX);
	lua_pushint(L, aMaxY == -1 ? aMemImage->GetHeight() : aMaxY - aY);
	return 4;
}

// sexy.DoHandCursor(), sexy.DoPointerCursor() (0x4c4d30, 0x4c4d40)
static int DoHandCursor(lua_State* L)
{
	gSexyAppBase->SetCursor(CURSOR_HAND);
	return 0;
}

static int DoPointerCursor(lua_State* L)
{
	gSexyAppBase->SetCursor(CURSOR_POINTER);
	return 0;
}

// sexy.IncResLoadedCount(), 0x4c4d20: one more task for the loading bar
static int IncResLoadedCount(lua_State* L)
{
	gSexyAppBase->mNumLoadingThreadTasks++;
	return 0;
}

// sexy.GetFilesInDirectory(pattern), 0x4c4310: the names of the matching files, as results
static int GetFilesInDirectory(lua_State* L)
{
	const char* aPattern = lua_tostring(L, 1);
	std::vector<std::string> aFiles = JeffLib::GetFilesInDirectory(aPattern != nullptr ? aPattern : "");
	for (const std::string& aFile : aFiles)
		lua_pushstring(L, aFile.c_str());
	return (int)aFiles.size();
}

// sexy.StrLess(a, b), 0x4c2e30: JeffLib's natural order
static int StrLess(lua_State* L)
{
	int aType = lua_type(L, 1);
	if (aType != LUA_TSTRING && aType != LUA_TNUMBER && aType != LUA_TINT)
		luaL_error(L, "Parameter 1 is not a string");
	aType = lua_type(L, 2);
	if (aType != LUA_TSTRING && aType != LUA_TNUMBER && aType != LUA_TINT)
		luaL_error(L, "Parameter 2 is not a string");
	std::string a = lua_tostring(L, 1);
	std::string b = lua_tostring(L, 2);
	lua_pushboolean(L, JeffLib::StringLessNatural(a, b));
	return 1;
}

// sexy.NumToCommaStr(n), 0x4c3030: "12,345" ("0" for 0; a negative number gives "")
static int NumToCommaStr(lua_State* L)
{
	int n = lua_toint(L, 1);
	std::string s;
	if (n == 0)
		s = "0";
	for (int aDigits = 0; n > 0; aDigits++, n /= 10) {
		if (aDigits != 0 && aDigits % 3 == 0)
			s.insert(s.begin(), ',');
		s.insert(s.begin(), (char)('0' + n % 10));
	}
	lua_pushstring(L, s.c_str());
	return 1;
}

// perf.Start, perf.End (0x4c2dd0, 0x4c2de0): the developers' profiler, empty in the release
static int PerfNothing(lua_State* L)
{
	return 0;
}

// ---- the scripts' animations (the app's PopAnimManager), mostly through scripts/PAMAnimator ----

// A number argument as the original read them: a number or a string that converts, truncated
static int NumberArg(lua_State* L, int theIndex)
{
	if (!lua_isnumber(L, theIndex))
		luaL_typerror(L, theIndex, "number");
	return (int)lua_tonumber(L, theIndex);
}

// sexy.LoadPAMFile(file, callback, id), 0x4c31f0
static int LoadPAMFile(lua_State* L)
{
	if (lua_gettop(L) != 3)
		luaL_error(L, "Expected 3 parameters");
	const char* aFile = lua_tostring(L, 1);
	std::string aFileName = aFile != nullptr ? aFile : "";
	int aType = lua_type(L, 2);
	if (aType != LUA_TSTRING && aType != LUA_TNUMBER && aType != LUA_TINT)
		luaL_error(L, "PAM file callback must be a string, not a function pointer");
	std::string aCallback = lua_tostring(L, 2);
	int anId = (int)lua_tonumber(L, 3);
	PopAnimManager* aManager = gApp->mPopAnimManager;
	if (aManager->Has(anId))
		SDL_Log("static int LoadPAMFile: WARNING: PAM Id already in use, intended?");
	PopAnim* anAnim = new PopAnim(anId, aManager);
	if (!anAnim->LoadAnimation(aFileName)) {
		std::string anError = anAnim->GetError();
		delete anAnim;
		luaL_error(L, "Error loading PAM file: \"%s\". Reason given: %s", aFileName.c_str(), anError.c_str());
	}
	aManager->Add(anId, anAnim, aCallback);
	return 0;
}

// sexy.ClonePAM(id, newId, callback), 0x4c34b0
static int ClonePAM(lua_State* L)
{
	int anId = NumberArg(L, 1);
	PopAnimManager* aManager = gApp->mPopAnimManager;
	PopAnim* anAnim = aManager->Get(anId);
	int aNewId = NumberArg(L, 2);
	if (aManager->Has(aNewId))
		SDL_Log("static int ClonePAMFile: WARNING: PAM Id already in use, intended?");
	const char* aCallback = lua_tostring(L, 3);
	if (anAnim == nullptr)
		luaL_error(L, "ClonePAM: id = %d: not found", anId);  // port: the original used the missing animation
	PopAnim* aClone = anAnim->Clone();
	aClone->mId = aNewId;
	aManager->Add(aNewId, aClone, aCallback != nullptr ? aCallback : "");
	return 0;
}

// sexy.UpdatePopAnim(id), 0x4c37a0
static int UpdatePopAnim(lua_State* L)
{
	gApp->mPopAnimManager->Update(L, NumberArg(L, 1));
	return 0;
}

// sexy.DrawPopAnim(g, id, x, y), 0x4c3840
static int DrawPopAnim(lua_State* L)
{
	Graphics* g = (Graphics*)gLuaSupport->CheckObject(1, ClassOf<Graphics>::info);
	int anId = NumberArg(L, 2);
	int aX = NumberArg(L, 3);
	int aY = NumberArg(L, 4);
	gApp->mPopAnimManager->Draw(L, g, anId, aX, aY);
	return 0;
}

// sexy.GetPopAnimFile(id), 0x4c3ac0
static int GetPopAnimFile(lua_State* L)
{
	PopAnim* anAnim = gApp->mPopAnimManager->Get(NumberArg(L, 1));
	lua_pushstring(L, anAnim != nullptr ? anAnim->GetPamFileName().c_str() : "");
	return 1;
}

// sexy.PlayPopAnim(frame or label, id), 0x4c3ba0: whether it plays
static int PlayPopAnim(lua_State* L)
{
	int anId = (int)lua_tonumber(L, 2);
	PopAnim* anAnim = gApp->mPopAnimManager->Get(anId);
	if (anAnim == nullptr)
		luaL_error(L, "PlayPopAnim: id = %d: not found", anId);
	bool aPlaying;
	if (lua_type(L, 1) == LUA_TNUMBER || lua_type(L, 1) == LUA_TINT) {
		aPlaying = anAnim->PlayFrame((int)lua_tonumber(L, 1));
	} else {
		const char* aLabel = lua_tostring(L, 1);
		aPlaying = anAnim->PlayLabel(aLabel != nullptr ? aLabel : "");
	}
	lua_pushboolean(L, aPlaying);
	return 1;
}

// sexy.GetPopAnimLabelFrame(label, id), 0x4c3d10: -1 for a label it does not have
static int GetPopAnimLabelFrame(lua_State* L)
{
	const char* aLabel = lua_tostring(L, 1);
	int anId = NumberArg(L, 2);
	PopAnim* anAnim = gApp->mPopAnimManager->Get(anId);
	if (anAnim == nullptr)
		luaL_error(L, "GetPopAnimLabelFrame: id = %d: not found", anId);
	lua_pushint(L, anAnim->GetLabelFrame(aLabel != nullptr ? aLabel : ""));
	return 1;
}

// sexy.ChangePopAnimCallback(id, callback), 0x4c3e00
static int ChangePopAnimCallback(lua_State* L)
{
	int anId = NumberArg(L, 1);
	const char* aCallback = lua_tostring(L, 2);
	if (aCallback == nullptr)
		luaL_typerror(L, 2, "string");
	gApp->mPopAnimManager->ChangeCallback(L, anId, aCallback);
	return 0;
}

// sexy.ResetPopAnim(id), 0x4c3f70
static int ResetPopAnim(lua_State* L)
{
	int anId = NumberArg(L, 1);
	PopAnim* anAnim = gApp->mPopAnimManager->Get(anId);
	if (anAnim == nullptr)
		luaL_error(L, "ResetPopAnim: id = %d: not found", anId);
	anAnim->ResetAnim();
	return 0;
}

// sexy.SetPopAnimColor(id, {r, g, b, a}), 0x4c3fb0
static int SetPopAnimColor(lua_State* L)
{
	int anId = (int)lua_tonumber(L, 1);
	int aValues[4] = {0, 0, 0, 0};
	static const char* const kFields[4] = {"r", "g", "b", "a"};
	for (int i = 0; i < 4; i++) {
		lua_pushstring(L, kFields[i]);
		lua_gettable(L, 2);
		if (lua_isnumber(L, -1))
			aValues[i] = (int)lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	PopAnim* anAnim = gApp->mPopAnimManager->Get(anId);
	if (anAnim == nullptr)
		luaL_error(L, "SetPopAnimColor: id = %d: not found", anId);
	anAnim->SetAnimColor(Color(aValues[0], aValues[1], aValues[2], aValues[3]));
	return 0;
}

// sexy.DeletePopAnim(id), 0x4c4270
static int DeletePopAnim(lua_State* L)
{
	gApp->mPopAnimManager->Delete(NumberArg(L, 1));
	return 0;
}

LuaInterface::LuaInterface()
{
	gLuaInterface = this;
	OpenPopLua();
	RegisterSexyFunctions();
}

void LuaInterface::RegisterSexyFunctions()
{
	RegisterFunction("sexy.PrintConsole", PrintConsole);
	RegisterFunction("sexy.NextRand", NextRand);
	RegisterFunction("sexy.PlaySample", PlaySample);
	RegisterFunction("sexy.DebugDialog", DebugMessage);
	RegisterFunction("sexy.DebugMsgBox", DebugMessage);
	RegisterFunction("sexy.PrintWarning", DebugMessage);
	RegisterFunction("sexy.PrintError", DebugMessage);
	RegisterFunction("sexy.GetOffsetRectFromImage", GetOffsetRectFromImage);
	RegisterFunction("sexy.DoHandCursor", DoHandCursor);
	RegisterFunction("sexy.DoPointerCursor", DoPointerCursor);
	RegisterFunction("sexy.IncResLoadedCount", IncResLoadedCount);
	RegisterFunction("sexy.GetFilesInDirectory", GetFilesInDirectory);
	RegisterFunction("sexy.StrLess", StrLess);
	RegisterFunction("sexy.NumToCommaStr", NumToCommaStr);
	RegisterFunction("perf.Start", PerfNothing);
	RegisterFunction("perf.End", PerfNothing);
	RegisterFunction("sexy.UpdatePopAnim", UpdatePopAnim);
	RegisterFunction("sexy.ChangePopAnimCallback", ChangePopAnimCallback);
	RegisterFunction("sexy.DrawPopAnim", DrawPopAnim);
	RegisterFunction("sexy.ResetPopAnim", ResetPopAnim);
	RegisterFunction("sexy.LoadPAMFile", LoadPAMFile);
	RegisterFunction("sexy.GetPopAnimFile", GetPopAnimFile);
	RegisterFunction("sexy.ClonePAM", ClonePAM);
	RegisterFunction("sexy.PlayPopAnim", PlayPopAnim);
	RegisterFunction("sexy.GetPopAnimLabelFrame", GetPopAnimLabelFrame);
	RegisterFunction("sexy.SetPopAnimColor", SetPopAnimColor);
	RegisterFunction("sexy.DeletePopAnim", DeletePopAnim);
	RegisterProfileFunctions();
	RegisterConvPanelFunctions();
}

void LuaInterface::ReportError(const std::string& theMessage)
{
	PopLua::ReportError(theMessage);
	if (gSexyAppBase == nullptr)
		return;
	gSexyAppBase->SwitchScreenMode(true);
	// port: the original asked the player to send its lua.log to customer support
	gSexyAppBase->Popup("A script of the game failed, so the game has to end.\n\n" + theMessage);
	gSexyAppBase->Shutdown();
}
