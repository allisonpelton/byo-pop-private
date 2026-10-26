// The scripts' GameApp class (registered by GameApp::Init, 0x445a50): 73 methods. The ones the port has not got
// to yet log their name the first time a script calls them and return nothing.
#include <SDL.h>

#include <set>
#include <string>

#include "CurtainTransition.h"
#include "GameApp.h"
#include "LuaBind.h"
#include "ResourceManager.h"
#include "lauxlib.h"

using namespace Sexy;
using namespace LuaLib;

namespace {

// The methods not ported yet, by name
const char* const kNotPorted[] = {
	"DoErrorDialog", "DoGenericDialog", "DoYesNoDialog", "MakeTooltip", "TurnOffTooltips", "EnableTooltips",
	"RemoveTooltip", "ReturnToChapterSelect", "ReturnToMMFromMiniGame", "StartBattle", "StartMiniGame",
	"PlayDelayedSample", "DoArenaWinDialog", "SetArenaWinStats", "CanShowTilePips", "PlayCutScene", "DoEndGame",
	"EnableBoardForTMCheckpointContinuation", "DoCurtainTransition", "DoFullCurtainTransition",
	"DoTimedModeTSTransition", "DoCSToGameTransition", "DoCSToMMTransition", "DoGameToMMTransition", "LoopSample",
	"DoAfterHydraConvPanel", "StopSample", "DoHighScoreScreenFromMG", "FadeOutSample", "ShowingConversationPanel",
	"HideBookManager", "GetWordWrappedHeight", "CloseCutScene", "PlayPitchShiftedSample",
	"UnlockedMiniGameComplete", "PlaySong", "ShouldDoHydraLockout", "SetSampleVolume", "IncSampleVolume",
	"PlayCachedSample", "ReleaseCachedSample", "PlaySampleMinTime", "PlaySongWhenDone", "PlayChainedSample",
	"AddStatsBook", "AddBonusText", "RemoveAllBonusText", "DrawBonusText", "UpdateBonusText", "BreakupString",
	"WriteStat", "StartLoadChapterResources", "MakeSepia", "MakeGrayscale", "LuaAntiAliasImage",
};

template <int N>
int NotPorted(GameApp*, lua_State*)
{
	static bool sLogged = false;
	if (!sLogged) {
		sLogged = true;
		SDL_Log("GameApp:%s: not ported yet", kNotPorted[N]);
	}
	return 0;
}

template <int... N>
void RegisterNotPorted(Class<GameApp>& theClass, std::integer_sequence<int, N...>)
{
	(theClass.Raw<&NotPorted<N>>(kNotPorted[N]), ...);
}

// ---- the ported methods ----

int GetUpdateCount(GameApp* app) { return app->mUpdateCount; }        // 0x442e60
void SetUpdateCount(GameApp* app, int n) { app->mUpdateCount = n; }   // 0x442e50
int GetWidth(GameApp* app) { return app->mWidth; }                    // 0x442e80
void SetWidth(GameApp* app, int n) { app->mWidth = n; }               // 0x442e70
int GetHeight(GameApp* app) { return app->mHeight; }                  // 0x442ea0
void SetHeight(GameApp* app, int n) { app->mHeight = n; }             // 0x442e90
bool HasDialogs(GameApp* app) { return app->GetDialogCount() > 0; }   // 0x402b80
bool HasPlayedGameBefore(GameApp* app) { return app->HasPlayedGameBefore(); }
void ClearUpdateBacklog(GameApp* app) { app->ClearUpdateBacklog(false); }  // 0x442eb0
// 0x442ec0: the original's two vsync flags; the port's display has neither
bool SmoothScrolling(GameApp* app) { return false; }
// 0x429cc0 (+0x91d) and 0x416380 (the stats book, +0x82c): neither exists in the port yet
bool QueuedHydraLockout(GameApp* app) { return false; }
bool ShowingStatsBook(GameApp* app) { return false; }

// 0x44f540: a curtain that has not finished
bool DoingCurtainTransition(GameApp* app)
{
	CurtainTransition* aCurtain = app->mCurtain;
	if (aCurtain == nullptr)
		return false;
	return !(aCurtain->mDone && aCurtain->mDrawn);
}

bool IsResGroupLoaded(GameApp* app, std::string theGroup)  // 0x44cb30
{
	return app->mResourceManager->IsGroupLoaded(theGroup);
}

bool LoadResGroup(GameApp* app, std::string theGroup)  // 0x44cb60
{
	bool aLoaded = app->mResourceManager->LoadResources(theGroup);
	app->ClearUpdateBacklog(false);
	return aLoaded;
}

std::string GetResGroupError(GameApp* app)  // 0x44cc70
{
	return app->mResourceManager->GetErrorText();
}

void UnloadResGroup(GameApp* app, std::string theGroup)  // 0x44cca0
{
	app->mResourceManager->DeleteResources(theGroup);
}

}  // namespace

void GameApp::RegisterGameAppMethods()
{
	Class<GameApp> aClass("GameApp", false);
	aClass.Base<LuaApp>()
		.Function<&::GetUpdateCount>("GetUpdateCount")
		.Function<&::SetUpdateCount>("SetUpdateCount")
		.Function<&::GetWidth>("GetWidth")
		.Function<&::SetWidth>("SetWidth")
		.Function<&::GetHeight>("GetHeight")
		.Function<&::SetHeight>("SetHeight")
		.Function<&::HasDialogs>("HasDialogs")
		.Function<&::HasPlayedGameBefore>("HasPlayedGameBefore")
		.Function<&::ClearUpdateBacklog>("ClearUpdateBacklog")
		.Function<&::SmoothScrolling>("SmoothScrolling")
		.Function<&::QueuedHydraLockout>("QueuedHydraLockout")
		.Function<&::ShowingStatsBook>("ShowingStatsBook")
		.Function<&::DoingCurtainTransition>("DoingCurtainTransition")
		.Function<&::IsResGroupLoaded>("IsResGroupLoaded")
		.Function<&::LoadResGroup>("LoadResGroup")
		.Function<&::GetResGroupError>("GetResGroupError")
		.Function<&::UnloadResGroup>("UnloadResGroup");
	RegisterNotPorted(aClass, std::make_integer_sequence<int, (int)(sizeof(kNotPorted) / sizeof(kNotPorted[0]))>{});
}
