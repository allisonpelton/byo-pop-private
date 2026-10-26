// The scripts' GameApp class (registered by GameApp::Init, 0x445a50): 73 methods. The ones the port has not got
// to yet log their name the first time a script calls them and return nothing.
#include <SDL.h>

#include <set>
#include <string>

#include "CurtainTransition.h"
#include "GameApp.h"
#include "LuaBind.h"
#include "Profile.h"
#include "ResourceManager.h"
#include "SongPlayer.h"
#include "SoundInstance.h"
#include "SoundPlayer.h"
#include "lauxlib.h"

using namespace Sexy;
using namespace LuaLib;

namespace {

// The methods not ported yet, by name. PlaySongWhenDone (0x44f440, the music interface's vf 0x58: the song to
// play when this one is done, the queue SongPlayer does not have yet) is among them.
const char* const kNotPorted[] = {
	"DoErrorDialog", "DoGenericDialog", "DoYesNoDialog", "MakeTooltip", "TurnOffTooltips", "EnableTooltips",
	"RemoveTooltip", "ReturnToChapterSelect", "ReturnToMMFromMiniGame", "StartBattle", "StartMiniGame",
	"DoArenaWinDialog", "SetArenaWinStats", "PlayCutScene", "DoEndGame",
	"EnableBoardForTMCheckpointContinuation", "DoCurtainTransition", "DoFullCurtainTransition",
	"DoTimedModeTSTransition", "DoCSToGameTransition", "DoCSToMMTransition", "DoGameToMMTransition",
	"DoAfterHydraConvPanel", "DoHighScoreScreenFromMG",
	"HideBookManager", "GetWordWrappedHeight", "CloseCutScene",
	"UnlockedMiniGameComplete", "ShouldDoHydraLockout",
	"PlaySongWhenDone",
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
bool ShowingConversationPanel(GameApp* app) { return app->ShowingConversationPanel(); }  // 0x450460

// 0x44f880: the profile's EnablePips when it has one, else the app's own flag
bool CanShowTilePips(GameApp* app)
{
	if (app->mProfile != nullptr && app->mProfile->KeyExists("EnablePips"))
		return app->mProfile->GetBool("EnablePips");
	return app->mShowTilePips;
}

// ---- the sounds ----

void PlayDelayedSample(GameApp* app, int theId, int theDelay, int thePitch)  // 0x44f580
{
	app->mSoundPlayer->PlayDelayedSample(theId, theDelay, thePitch);
}

void PlaySampleMinTime(GameApp* app, int theId, int theMinUpdates)  // 0x44f5e0
{
	app->mSoundPlayer->PlaySampleMinTime(theId, theMinUpdates);
}

void PlayChainedSample(GameApp* app, int theId, int theNextId, int theDelay, int thePitch)  // 0x44f620
{
	app->mSoundPlayer->PlayChainedSample(theId, theNextId, theDelay, thePitch);
}

void LoopSample(GameApp* app, int theId) { app->mSoundPlayer->LoopSample(theId); }  // 0x44f650
void StopSample(GameApp* app, int theId) { app->mSoundPlayer->StopSample(theId); }  // 0x44f660

void FadeOutSample(GameApp* app, int theId, float theFade)  // 0x44f670
{
	app->mSoundPlayer->FadeOutSample(theId, theFade);
}

// 0x44f690: not within the last 5 updates
void PlayPitchShiftedSample(GameApp* app, int theId, int thePitch)
{
	app->mSoundPlayer->PlayPitchShiftedSample(theId, thePitch, 5);
}

// 0x44f6b0. port: an id outside the cached samples does nothing (the original indexed past the array)
void SetSampleVolume(GameApp* app, int theId, double theVolume)
{
	if (theId < 0 || theId >= JeffLib::SoundPlayer::MAX_SAMPLES)
		return;
	if (SoundInstance* anInstance = app->mSoundPlayer->mCached[theId])
		anInstance->SetVolume(theVolume);
}

// 0x44f6e0. port: an id outside the cached samples does nothing
void IncSampleVolume(GameApp* app, int theId, double theVolume)
{
	if (theId < 0 || theId >= JeffLib::SoundPlayer::MAX_SAMPLES)
		return;
	if (SoundInstance* anInstance = app->mSoundPlayer->mCached[theId])
		anInstance->SetVolume(anInstance->GetVolume() + theVolume);
}

void PlayCachedSample(GameApp* app, int theId, int thePan, int theMinUpdates)  // 0x44f720
{
	app->mSoundPlayer->PlayCachedSample(theId, thePan, theMinUpdates);
}

// 0x44f740. port: an id outside the cached samples does nothing
void ReleaseCachedSample(GameApp* app, int theId)
{
	if (theId < 0 || theId >= JeffLib::SoundPlayer::MAX_SAMPLES)
		return;
	if (SoundInstance*& anInstance = app->mSoundPlayer->mCached[theId]) {
		anInstance->Release();
		anInstance = nullptr;
	}
}

// 0x44f410: the music interface's vf 0x54. port: the song player does what the original's interface added
void PlaySong(GameApp* app, int theOrder, float theSpeed, bool theLoop)
{
	if (app->mSongPlayer != nullptr)
		app->mSongPlayer->PlaySong(theOrder, theSpeed, theLoop);
}

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
		.Function<&::ShowingConversationPanel>("ShowingConversationPanel")
		.Function<&::CanShowTilePips>("CanShowTilePips")
		.Function<&::PlayDelayedSample>("PlayDelayedSample")
		.Function<&::PlaySampleMinTime>("PlaySampleMinTime")
		.Function<&::PlayChainedSample>("PlayChainedSample")
		.Function<&::LoopSample>("LoopSample")
		.Function<&::StopSample>("StopSample")
		.Function<&::FadeOutSample>("FadeOutSample")
		.Function<&::PlayPitchShiftedSample>("PlayPitchShiftedSample")
		.Function<&::SetSampleVolume>("SetSampleVolume")
		.Function<&::IncSampleVolume>("IncSampleVolume")
		.Function<&::PlayCachedSample>("PlayCachedSample")
		.Function<&::ReleaseCachedSample>("ReleaseCachedSample")
		.Function<&::PlaySong>("PlaySong")
		.Function<&::DoingCurtainTransition>("DoingCurtainTransition")
		.Function<&::IsResGroupLoaded>("IsResGroupLoaded")
		.Function<&::LoadResGroup>("LoadResGroup")
		.Function<&::GetResGroupError>("GetResGroupError")
		.Function<&::UnloadResGroup>("UnloadResGroup");
	RegisterNotPorted(aClass, std::make_integer_sequence<int, (int)(sizeof(kNotPorted) / sizeof(kNotPorted[0]))>{});
}
