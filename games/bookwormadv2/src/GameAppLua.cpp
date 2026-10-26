// The scripts' GameApp class (registered by GameApp::Init, 0x445a50): 73 methods. The ones the port has not got
// to yet log their name the first time a script calls them and return nothing.
#include <SDL.h>

#include <cmath>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ConversationPanel.h"
#include "CurtainTransition.h"
#include "GameApp.h"
#include "GameDialog.h"
#include "GameText.h"
#include "Graphics.h"
#include "LuaBind.h"
#include "LuaImage.h"
#include "Profile.h"
#include "ResourceManager.h"
#include "SongPlayer.h"
#include "SoundInstance.h"
#include "SoundPlayer.h"
#include "TooltipWidget.h"
#include "lauxlib.h"

using namespace Sexy;
using namespace LuaLib;

namespace {

// The methods not ported yet, by name.
const char* const kNotPorted[] = {
	"LuaAntiAliasImage",
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
// 0x429cc0 (+0x91d): the trial's lockout does not exist in the port yet
bool QueuedHydraLockout(GameApp* app) { return false; }
bool ShowingStatsBook(GameApp* app) { return app->mBookScreen != nullptr; }  // 0x416380
void AddStatsBook(GameApp* app, bool theFromGame, bool theBestiary, bool theMediaPlayer)  // 0x444150
{
	app->AddStatsBook(theFromGame, theBestiary, theMediaPlayer);
}
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

// 0x44f440: the music interface's vf 0x58. port: the song player's queue
void PlaySongWhenDone(GameApp* app, int theOrder, float theSpeed, int theAtOrder, int theAtRow)
{
	if (app->mSongPlayer != nullptr)
		app->mSongPlayer->PlaySongWhenDone(theOrder, theSpeed, theAtOrder, theAtRow);
}

// 0x44f540: a curtain that has not finished
bool DoingCurtainTransition(GameApp* app)
{
	CurtainTransition* aCurtain = app->mCurtain;
	if (aCurtain == nullptr)
		return false;
	return !(aCurtain->mDone && aCurtain->mDrawn);
}

void StartBattle(GameApp* app) { app->StartBattle(); }  // 0x44b620

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

// ---- the curtain and the screens ----

void ReturnToChapterSelect(GameApp* app, bool theLexWon, bool theFromOptions, bool theNoMusic, bool theFromMainMenu)
{
	app->ReturnToChapterSelect(theLexWon, theFromOptions, theNoMusic, theFromMainMenu);
}
void PlayCutScene(GameApp* app, std::string theFile, std::string theCallback, std::string theCaption,
                  bool theKillWhenDone, bool theHide)
{
	app->PlayCutScene(theFile, theCallback, theCaption, theKillWhenDone, theHide);
}
void DoCurtainTransition(GameApp* app, bool theClose, std::string theCallback, int thePriority)
{
	app->DoCurtainTransition(theClose, theCallback, thePriority);
}
void DoFullCurtainTransition(GameApp* app, std::string theCallback) { app->DoFullCurtainTransition(theCallback); }
void DoCSToGameTransition(GameApp* app) { app->StartSavedGameTransition(); }
void DoCSToMMTransition(GameApp* app) { app->StartToMenuTransition(); }
void DoGameToMMTransition(GameApp* app) { app->StartGameToMenuTransition(); }
void HideBookManager(GameApp* app) { app->HideBookManager(); }

// The mini-games (GameAppMiniGames.cpp)
void StartMiniGame(GameApp* app, std::string theName, int x, int y, int theXGap, int theYGap, int theCols, int theRows,
                   std::string theLetters, bool theNoTilesWhilePaused)
{
	app->StartMiniGame(theName, x, y, theXGap, theYGap, theCols, theRows, theLetters, theNoTilesWhilePaused);
}
void ReturnToMMFromMiniGame(GameApp* app) { app->ReturnToMMFromMiniGame(); }
void UnlockedMiniGameComplete(GameApp* app) { app->UnlockedMiniGameComplete(); }
void DoTimedModeTSTransition(GameApp* app) { app->DoTimedModeTSTransition(); }
void DoEndGame(GameApp* app) { app->DoEndGame(); }
void DoHighScoreScreenFromMG(GameApp* app, std::string theName) { app->DoHighScoreScreenFromMG(theName); }
void EnableBoardForTMCheckpointContinuation(GameApp* app) { app->EnableBoardForTMCheckpointContinuation(); }
void DoArenaWinDialog(GameApp* app, bool theFirstTime, std::string theCallback)
{
	app->DoArenaWinDialog(theFirstTime, theCallback);
}
void SetArenaWinStats(GameApp* app, std::string theTime, std::string thePotions, std::string theWords,
                      std::string theLevel, std::string theAverageWordLength)
{
	app->SetArenaWinStats(theTime, thePotions, theWords, theLevel, theAverageWordLength);
}
void AddBonusText(GameApp* app, std::string theText, float x, float y, float theSpeed, int theFadeDelay, LuaFont* theFont)
{
	app->AddBonusText(theText, x, y, theSpeed, theFadeDelay, FontOf(theFont));
}
void UpdateBonusText(GameApp* app) { app->UpdateBonusText(); }
void RemoveAllBonusText(GameApp* app) { app->RemoveAllBonusText(); }
// 0x44f010: empty in the released game (no script calls it)
int WriteStat(GameApp*, lua_State*)
{
	return 0;
}

// 0x44fc20: DrawBonusText(g)
int DrawBonusText(GameApp* app, lua_State* L)
{
	app->DrawBonusText((Graphics*)gLuaSupport->CheckObject(1, ClassOf<Graphics>::info));
	return 0;
}

// 0x451c80: whether the trial's lockout follows the hydra's conversation (a distribution of the trial, not
// registered, past the first book's seventh chapter). port: the port plays as registered, so never
bool ShouldDoHydraLockout(GameApp* app) { return false; }
// 0x451e30: the conversation panel comes up again, and the warp-in sound plays once it is off (the lockout's
// callback 0x451eb0 instead, in the trial)
void DoAfterHydraConvPanel(GameApp* app)
{
	app->mConversationPanel->Activate(true);
	if (!ShouldDoHydraLockout(app))
		app->mConversationPanel->mInactiveCallback = "Book.PlayWarpInSound";
}

// 0x44c390: a message box with the title "ERROR"
void DoErrorDialog(GameApp* app, std::string theText)
{
	app->MsgBox(theText, GT(0x6f701c), MB_ICONERROR);
}

// 0x444b30: the confirm dialog (DIALOG_CONFIRM), with theHeader in place of its banner unless theBanner
void DoYesNoDialog(GameApp* app, std::string theHeader, std::string theLines, std::string theCallback, bool theBanner)
{
	GameDialog* aDialog = app->DoConfirmDialog(-1, theHeader, theLines, nullptr, theCallback);
	if (!theBanner) {
		aDialog->mDialogHeader = theHeader;
		aDialog->SetBanner(nullptr, 0);
	}
}

// 0x44f900: the height of theText wrapped at theWidth in theFont
int GetWordWrappedHeight(GameApp* app, std::string theText, int theWidth, int theLineSpacing, LuaFont* theFont)
{
	Graphics g;
	g.SetFont(FontOf(theFont));
	return g.GetWordWrappedHeight(theWidth, theText, theLineSpacing, nullptr);
}
void CloseCutScene(GameApp* app) { app->CloseCutScene(); }
int StartLoadChapterResources(GameApp* app, lua_State* L) { return app->StartLoadChapterResources(L); }

// ---- the images' filters ----

// A pixel's grey: 0.3 red, 0.59 green and 0.11 blue, rounded to the nearest (half to even, as the x87 did)
int PixelGrey(ulong thePixel)
{
	double aGrey = (thePixel & 0xff) * (double)0.11f + ((thePixel >> 8) & 0xff) * (double)0.59f +
	               ((thePixel >> 16) & 0xff) * (double)0.3f;
	return (int)std::nearbyint(aGrey);
}

// 0x44c730: the grey, with red +0x36 and green +0x26 (at most 255)
void SepiaFilter(MemoryImage* theImage)
{
	ulong* aBits = theImage->GetBits();
	int aCount = theImage->GetWidth() * theImage->GetHeight();
	for (int i = 0; i < aCount; i++) {
		int aGrey = PixelGrey(aBits[i]);
		ulong aRed = aGrey + 0x36 < 0xff ? aGrey + 0x36 : 0xff;
		ulong aGreen = aGrey + 0x26 < 0xff ? aGrey + 0x26 : 0xff;
		aBits[i] = (aBits[i] & 0xff000000) | (aRed << 16) | (aGreen << 8) | (ulong)aGrey;
	}
	theImage->BitsChanged();
}

// 0x44c950
void GrayscaleFilter(MemoryImage* theImage)
{
	ulong* aBits = theImage->GetBits();
	int aCount = theImage->GetWidth() * theImage->GetHeight();
	for (int i = 0; i < aCount; i++) {
		ulong aGrey = (ulong)PixelGrey(aBits[i]);
		aBits[i] = (aBits[i] & 0xff000000) | (aGrey << 16) | (aGrey << 8) | aGrey;
	}
	theImage->BitsChanged();
}

int MakeSepia(GameApp* app, lua_State* L)  // 0x44c890: MakeSepia(image)
{
	LuaImage* anImage = (LuaImage*)gLuaSupport->CheckObject(1, ClassOf<LuaImage>::info);
	if (anImage != nullptr)
		anImage->ApplyFilter(&SepiaFilter);
	return 0;
}

int MakeGrayscale(GameApp* app, lua_State* L)  // 0x44ca70: MakeGrayscale(image)
{
	LuaImage* anImage = (LuaImage*)gLuaSupport->CheckObject(1, ClassOf<LuaImage>::info);
	if (anImage != nullptr)
		anImage->ApplyFilter(&GrayscaleFilter);
	return 0;
}

// 0x4445c0: DoGenericDialog(header, lines[, callback[, banner image]]): DoOkDialog
int DoGenericDialog(GameApp* app, lua_State* L)
{
	const char* aHeader = lua_tostring(L, 1);
	const char* aLines = lua_tostring(L, 2);
	std::string aCallback;
	if (lua_gettop(L) >= 3) {
		const char* aName = lua_tostring(L, 3);
		aCallback = aName != nullptr ? aName : "";
	}
	Image* aBanner = nullptr;
	if (lua_gettop(L) >= 4)
		aBanner = ImageOf((LuaImage*)gLuaSupport->CheckObject(4, ClassOf<LuaImage>::info));
	app->DoOkDialog(aHeader != nullptr ? aHeader : "", aLines != nullptr ? aLines : "", aCallback, aBanner);
	return 0;
}

// 0x6a7810: theText's parts between the '#' marks, which are red, and outside them (empty parts left out);
// false for a '#' without its pair
bool SplitRedParts(const std::string& theText, std::vector<std::pair<std::string, bool>>& theParts)
{
	bool aRed = false;
	size_t aStart = 0;
	for (size_t i = 0; i < theText.size(); i++) {
		if (theText[i] != '#')
			continue;
		if (i > aStart)
			theParts.push_back(std::make_pair(theText.substr(aStart, i - aStart), aRed));
		aRed = !aRed;
		aStart = i + 1;
	}
	if (aRed)
		return false;
	if (theText.size() > aStart)
		theParts.push_back(std::make_pair(theText.substr(aStart), false));
	return true;
}

// 0x44f020: BreakupString(text): its parts, as a table from 0 of {mStr = part, mRed = red}
int BreakupString(GameApp* app, lua_State* L)
{
	const char* aText = lua_tostring(L, 1);
	std::vector<std::pair<std::string, bool>> aParts;
	if (!SplitRedParts(aText != nullptr ? aText : "", aParts))
		luaL_error(L, "Malformed string passed to BreakupString");
	lua_newtable(L);
	for (size_t i = 0; i < aParts.size(); i++) {
		lua_pushnumber(L, (lua_Number)i);
		lua_newtable(L);
		lua_pushstring(L, "mStr");
		lua_pushstring(L, aParts[i].first.c_str());
		lua_settable(L, -3);
		lua_pushstring(L, "mRed");
		lua_pushboolean(L, aParts[i].second);
		lua_settable(L, -3);
		lua_settable(L, -3);
	}
	return 1;
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
		.Function<&::AddStatsBook>("AddStatsBook")
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
		.Function<&::PlaySongWhenDone>("PlaySongWhenDone")
		.Function<&::DoingCurtainTransition>("DoingCurtainTransition")
		.Function<&::IsResGroupLoaded>("IsResGroupLoaded")
		.Function<&::LoadResGroup>("LoadResGroup")
		.Function<&::GetResGroupError>("GetResGroupError")
		.Function<&::UnloadResGroup>("UnloadResGroup")
		.Function<&::StartBattle>("StartBattle")
		.Function<&::ReturnToChapterSelect>("ReturnToChapterSelect")
		.Function<&::PlayCutScene>("PlayCutScene")
		.Function<&::DoCurtainTransition>("DoCurtainTransition")
		.Function<&::DoFullCurtainTransition>("DoFullCurtainTransition")
		.Function<&::DoCSToGameTransition>("DoCSToGameTransition")
		.Function<&::DoCSToMMTransition>("DoCSToMMTransition")
		.Function<&::DoGameToMMTransition>("DoGameToMMTransition")
		.Function<&::HideBookManager>("HideBookManager")
		.Function<&::StartMiniGame>("StartMiniGame")
		.Function<&::ReturnToMMFromMiniGame>("ReturnToMMFromMiniGame")
		.Function<&::UnlockedMiniGameComplete>("UnlockedMiniGameComplete")
		.Function<&::DoTimedModeTSTransition>("DoTimedModeTSTransition")
		.Function<&::DoEndGame>("DoEndGame")
		.Function<&::DoHighScoreScreenFromMG>("DoHighScoreScreenFromMG")
		.Function<&::EnableBoardForTMCheckpointContinuation>("EnableBoardForTMCheckpointContinuation")
		.Function<&::DoArenaWinDialog>("DoArenaWinDialog")
		.Function<&::SetArenaWinStats>("SetArenaWinStats")
		.Function<&::AddBonusText>("AddBonusText")
		.Function<&::UpdateBonusText>("UpdateBonusText")
		.Function<&::RemoveAllBonusText>("RemoveAllBonusText")
		.Raw<&::DrawBonusText>("DrawBonusText")
		.Raw<&::WriteStat>("WriteStat")
		.Function<&::ShouldDoHydraLockout>("ShouldDoHydraLockout")
		.Function<&::DoAfterHydraConvPanel>("DoAfterHydraConvPanel")
		.Function<&::DoErrorDialog>("DoErrorDialog")
		.Function<&::DoYesNoDialog>("DoYesNoDialog")
		.Function<&::GetWordWrappedHeight>("GetWordWrappedHeight")
		.Function<&::CloseCutScene>("CloseCutScene")
		.Raw<&::StartLoadChapterResources>("StartLoadChapterResources")
		.Raw<&::MakeSepia>("MakeSepia")
		.Raw<&::MakeGrayscale>("MakeGrayscale")
		.Raw<&::DoGenericDialog>("DoGenericDialog")
		.Raw<&::BreakupString>("BreakupString")
		.Method<&GameApp::MakeTooltip>("MakeTooltip")
		.Method<&GameApp::TurnOffTooltips>("TurnOffTooltips")
		.Method<&GameApp::EnableTooltips>("EnableTooltips")
		.Method<&GameApp::RemoveTooltip>("RemoveTooltip");
	RegisterNotPorted(aClass, std::make_integer_sequence<int, (int)(sizeof(kNotPorted) / sizeof(kNotPorted[0]))>{});
}
