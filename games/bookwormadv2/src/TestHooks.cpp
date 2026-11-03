// port: the test scripts' own commands (-script, see SexyAppBase.h); the original had none.
//
//   word [attack] [ruby|preferruby]  spells the longest word the battle's grid allows: clicks on its tiles, 8 updates apart
//                   (then a move to the attack button and a click on it when asked; with ruby, only a word that uses a
//                   ruby tile, which the first battle's tutorial asks for; preferruby: such a word if there is one). Logs "script: word <WORD>", or "no word". While
//                   the board is busy (a dialog, a conversation, a pause, tiles moving, falling or already in
//                   the word) it tries again every 10 updates, and gives up after 200 tries ("busy").
//   soak <update>   plays by itself until the update, a step each 100 updates: dismisses a dialog, clicks through a
//                   conversation, spells the longest word and attacks (potions only while the first battle's tutorial asks
//                   for one), clicks the middle of the screen (treasure and win screens), picks two treasures and the
//                   companion in the treasure room and goes on, and clicks the chapter map's Enter button (when the
//                   board is hidden)
//   nowords <0|1>  1: the soak spells no words (the rest of it goes on), so a battle is lost to the enemy; 0: it does again
//   wordmax <n>    the words the soak and the word command spell are at most n letters (0: no limit), so the enemies hit harder than Lex's words
//   battle <book> [<stage>]  from the main menu (it waits for it) starts a new adventure of the book (4, 5 or 6: the profile's
//                   CurrentBook is set, AdventureComplete, InGame and the tile help off), goes on to the stage
//                   (the enemy of the chapter, 1 for the first, one more than the enemies is the boss) once the board shows,
//                   clicking the chapter map's Enter until it does; logs "script: battle: ..." at each step. A soak after it plays the battle
//                   (wordmax 3 lets the enemy attack)
//   pam <id>        logs where a script's animation is (playing, label, frame)
//   state           logs the battle engine's state and the board's selected tiles
//   widgets         logs the widget manager's widgets: class, rectangle, visibility, priority, focus
//   dismiss         presses Escape when a dialog is up, which the game's dialogs answer with their yes or ok; when the same
//                   dialog is still there at the next dismiss, clicks its first button (logs "script: dismiss: <what it did>")
//   lua <chunk>     runs the rest of the line as Lua (logs "script: lua: <error>" when it fails)
//   spectrum        logs how many cells of the media player's spectrum are lit ("script: spectrum <n> lit")
#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/heap.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

#include "BattleEngine.h"
#include "Board.h"
#include "Dialog.h"
#include "WidgetManager.h"
#include "FadeButton.h"
#include "GameApp.h"
#include "GameDialog.h"
#include "GameText.h"
#include "Lex.h"
#include "SongPlayer.h"
#include "LuaSupport.h"
#include "PopAnimManager.h"
#include "Profile.h"
#include "Tile.h"
#include "TreasureScreen.h"

using namespace Sexy;

namespace {

typedef std::vector<std::pair<std::string, Tile*>> TileLetters;  // a tile's letters ("QU" for its Qu tile)

// theWord from theTiles' letters, the first tile that fits at each place: the tiles in order, or false
bool Spell(const std::string& theWord, const TileLetters& theTiles, std::vector<Tile*>& theOrder)
{
	std::vector<bool> aUsed(theTiles.size(), false);
	theOrder.clear();
	for (size_t i = 0; i < theWord.size();) {
		size_t aFound = theTiles.size();
		for (size_t t = 0; t < theTiles.size() && aFound == theTiles.size(); t++) {
			const std::string& aLetters = theTiles[t].first;
			if (!aUsed[t] && !aLetters.empty() && theWord.compare(i, aLetters.size(), aLetters) == 0)
				aFound = t;
		}
		if (aFound == theTiles.size())
			return false;
		aUsed[aFound] = true;
		theOrder.push_back(theTiles[aFound].second);
		i += theTiles[aFound].first.size();
	}
	return true;
}

// whether the tile is a ruby: gTileTable[tile]:HasAttribute(RubyTile) in the scripts
bool IsRuby(Tile* theTile)
{
	lua_State* L = LuaLib::gLuaSupport->L;
	int aTop = lua_gettop(L);
	bool aResult = false;
	lua_getglobal(L, "gTileTable");
	Tile::PushTile(theTile);
	lua_gettable(L, -2);
	if (lua_istable(L, -1)) {
		lua_pushstring(L, "HasAttribute");
		lua_gettable(L, -2);
		lua_pushvalue(L, -2);
		lua_getglobal(L, "RubyTile");
		if (lua_pcall(L, 2, 1, 0) == 0)
			aResult = lua_toboolean(L, -1) != 0;
	}
	lua_settop(L, aTop);
	return aResult;
}

}  // namespace

size_t UsedVirtualMemory();  // CrashLog.cpp

static bool sSoakNoWords = false;  // the nowords command
static int sSoakMaxWordLen = 0;    // the wordmax command: 0 is no limit

// The treasure room's next click, "x y" (empty when there is none): a companion first, then the treasures, one
// of the unselected each time round (one that does not take the click does not block the others)
static std::string PickTreasureClick()
{
	static const char kPick[] =
	    "local comp, cands = false, {}\n"
	    "for k, v in pairs(gTreasureScreen.mTreasures) do\n"
	    "  if v.mSelected then\n"
	    "    if v.mIsCompanion then comp = true end\n"
	    "  else table.insert(cands, v) end\n"
	    "end\n"
	    "local want = {}\n"
	    "for i = 1, table.getn(cands) do\n"
	    "  if (cands[i].mIsCompanion and true or false) == (not comp) then table.insert(want, cands[i]) end\n"
	    "end\n"
	    "if table.getn(want) == 0 then return '' end\n"
	    "gSoakPick = (gSoakPick or 0) + 1\n"
	    "local v = want[math.mod(gSoakPick, table.getn(want)) + 1]\n"
	    "return (v.mHX + 10) .. ' ' .. (v.mHY + 10)\n";
	lua_State* L = LuaLib::gLuaSupport->L;
	std::string aResult;
	if (luaL_loadbuffer(L, kPick, sizeof(kPick) - 1, "=soakpick") != 0 || lua_pcall(L, 0, 1, 0) != 0)
		SDL_Log("script: soak: treasure pick: %s", lua_tostring(L, -1));
	else if (lua_isstring(L, -1))
		aResult = lua_tostring(L, -1);
	lua_pop(L, 1);
	return aResult;
}

bool GameApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand == "lua") {
		lua_State* L = LuaLib::gLuaSupport->L;
		if (luaL_loadbuffer(L, theCommand.mArgs.data(), theCommand.mArgs.size(), "=script") != 0 ||
		    lua_pcall(L, 0, 0, 0) != 0) {
			SDL_Log("script: lua: %s", lua_tostring(L, -1));
			lua_pop(L, 1);
		}
		return true;
	}
	if (theCommand.mCommand == "spectrum") {
		int aLit = 0;
		if (mSongPlayer != nullptr && mSongPlayer->mSpectrum != nullptr)
			for (int i = 0; i < mSongPlayer->mSpectrumWidth * mSongPlayer->mSpectrumHeight; i++)
				if (mSongPlayer->mSpectrum[i] != 0)
					aLit++;
		SDL_Log("script: spectrum %d lit", aLit);
		return true;
	}
	if (theCommand.mCommand == "state") {
		// the battle's state: the engine's, the board's flags and its selected tiles
		SDL_Log("script: state: engine %d, board %p%s", gBattleEngine != nullptr ? gBattleEngine->mState : -9, (void*)mBoard,
		        mBoard != nullptr && mBoard->mVisible ? " visible" : "");
		if (mBoard != nullptr) {
			SDL_Log("script: state: paused %d, tiles %d, selected %d, new %d, ending %d", (int)mBoard->IsPaused(),
			        (int)mBoard->mTiles.size(), (int)mBoard->mSelected.size(), (int)mBoard->mNewTiles.size(),
			        (int)mBoard->mEndingChapter);
			for (Tile* aTile : mBoard->mTiles)
				SDL_Log("script: state: tile %c at %d,%d cell %d,%d selected %d can-select %d alpha %d moving %d falling %d",
				        aTile->GetLetter(), aTile->mX, aTile->mY, aTile->mCol, aTile->mRow, (int)aTile->mSelected,
				        (int)aTile->CanSelect(aTile->mX + 25, aTile->mY + 25), aTile->mAlphaState, (int)aTile->mMoving,
				        (int)aTile->mFalling);
			for (Tile* aTile : mBoard->mSelected)
				SDL_Log("script: state: selected tile at %d,%d cell %d,%d alpha %d moving %d falling %d fall %d", aTile->mX,
				        aTile->mY, aTile->mCol, aTile->mRow, aTile->mAlphaState, (int)aTile->mMoving, (int)aTile->mFalling,
				        aTile->mFallDistance);
		}
		return true;
	}
	if (theCommand.mCommand == "widgets") {
		for (Widget* aWidget : mWidgetManager->mWidgets)
			SDL_Log("script: widget %s at %d,%d %dx%d visible %d disabled %d priority %d z %d%s%s",
			        typeid(*aWidget).name(), aWidget->mX, aWidget->mY, aWidget->mWidth, aWidget->mHeight,
			        (int)aWidget->mVisible, (int)aWidget->mDisabled, aWidget->mPriority, aWidget->mZOrder,
			        aWidget == mWidgetManager->mFocusWidget ? " FOCUS" : "",
			        aWidget == mWidgetManager->mBaseModalWidget ? " MODAL" : "");
		return true;
	}
	if (theCommand.mCommand == "soak") {
		// soak <update> [tick]: every 100 updates a tick, in turn a word and a click on the middle of the screen
		int anEnd = atoi(theCommand.mArgs.c_str());
		size_t aSpace = theCommand.mArgs.find(' ');
		int aTick = aSpace == std::string::npos ? 0 : atoi(theCommand.mArgs.c_str() + aSpace + 1);
		if (mUpdateCount >= anEnd)
			return true;
		AddScriptCommand(mUpdateCount + 100, "soak", StrFormat("%d %d", anEnd, aTick + 1));
		bool aWordTurn = aTick % 2 == 0;
#ifdef __EMSCRIPTEN__
		if (aTick % 100 == 0)
			SDL_Log("script: soak %d: heap %zu bytes", mUpdateCount, emscripten_get_heap_size());
#endif
#ifdef _WIN32
		if (aTick % 100 == 0 && UsedVirtualMemory() != 0)
			SDL_Log("script: soak %d: address space used %zu MB", mUpdateCount, UsedVirtualMemory() / (1024 * 1024));
#endif
		if (mBoard != nullptr && !mBoard->mIsMiniGame) {
			static int sLogged = 0;
			for (Tile* aTile : mBoard->mTiles)
				if (sLogged < 6 && !aTile->mFalling && !aTile->mMoving && (aTile->mY < 290 || aTile->mY > 560)) {
					sLogged++;
					SDL_Log("script: soak %d: tile out of the grid at %d,%d cell %d,%d selected %d alpha %d engine %d", mUpdateCount,
					        aTile->mX, aTile->mY, aTile->mCol, aTile->mRow, (int)aTile->mSelected, aTile->mAlphaState,
					        gBattleEngine != nullptr ? gBattleEngine->mState : -9);
				}
		}
		if (!mDialogMap.empty())
			AddScriptCommand(mUpdateCount + 1, "dismiss", "");
		else if (mTreasureScreen != nullptr && mTreasureScreen->mVisible) {
			// the treasure room: a companion and treasures until Continue is there, then Continue (again and again)
			auto aFound = mTreasureScreen->mButtons.find(TreasureScreen::BUTTON_CONTINUE);
			FadeButton* aContinue = aFound != mTreasureScreen->mButtons.end() ? aFound->second : nullptr;
			std::string aClick = "400 570";
			if (aContinue != nullptr && aContinue->mVisible && !aContinue->mDisabled)
				SDL_Log("script: soak %d: treasure room: click %s (Continue)", mUpdateCount, aClick.c_str());
			else if (aTick % 2 == 0) {
				aClick = PickTreasureClick();
				if (!aClick.empty())
					SDL_Log("script: soak %d: treasure room: click %s", mUpdateCount, aClick.c_str());
			} else
				aClick.clear();
			if (!aClick.empty())
				AddScriptCommand(mUpdateCount + 1, "click", aClick);
		}
		else if (mBoard == nullptr || !mBoard->mVisible) {
			if (aWordTurn)
				SDL_Log("script: soak %d: board hidden: click Enter", mUpdateCount);
			AddScriptCommand(mUpdateCount + 1, "click", "400 575");  // the chapter map's Enter button
		}
		else if (!mBoard->mIsMiniGame && gBattleEngine != nullptr && gBattleEngine->mState == BattleEngine::BE_IDLE &&
		         gBattleEngine->LostTurn() && gBattleEngine->CallLinkedMethod("CanEndLostTurnRound", 0).Bool() &&
		         gBattleEngine->CallLinkedMethod("AttackerIsPlayer", 0).Bool()) {
			// stunned: the panel on the grid ends the lost turn when clicked (Board::MouseDown)
			SDL_Log("script: soak %d: lost turn: click the grid", mUpdateCount);
			AddScriptCommand(mUpdateCount + 1, "click", "400 420");
		}
		else if (ShowingConversationPanel() || !aWordTurn)
			AddScriptCommand(mUpdateCount + 1, "click", "400 290");
		else if (mBoard != nullptr && !sSoakNoWords) {
			if (mProfile != nullptr && mProfile->GetBool("TutorialNeedsPower")) {
				// the first battle's potion tutorial locks the grid until a potion is used
				AddScriptCommand(mUpdateCount + 1, "click", "68 330");
				AddScriptCommand(mUpdateCount + 6, "click", "143 330");
				AddScriptCommand(mUpdateCount + 11, "click", "225 330");
			}
			AddScriptCommand(mUpdateCount + 20, "word", "attack preferruby");
		}
		return true;
	}
	if (theCommand.mCommand == "battle") {
		// battle <book> <stage> [<phase>]: see the header
		int aBook = atoi(theCommand.mArgs.c_str());
		size_t aSpace = theCommand.mArgs.find(' ');
		int aStage = aSpace == std::string::npos ? 1 : atoi(theCommand.mArgs.c_str() + aSpace + 1);
		size_t aSpace2 = aSpace == std::string::npos ? aSpace : theCommand.mArgs.find(' ', aSpace + 1);
		int aPhase = aSpace2 == std::string::npos ? 0 : atoi(theCommand.mArgs.c_str() + aSpace2 + 1);
		std::string aNext = StrFormat("%d %d %d", aBook, aStage, aPhase);
		if (aPhase == 0) {
			if (mMainMenu == nullptr || IsCurtainMoving() || !mDialogMap.empty() || mProfile == nullptr) {
				AddScriptCommand(mUpdateCount + 20, "battle", aNext);
				return true;
			}
			mProfile->SetBool("NeedsIntroTileHelp", false);
			mProfile->SetBool("AdventureComplete", false);
			mProfile->SetBool("InGame", false);
			mProfile->SetInt("CurrentBook", aBook);
			SDL_Log("script: battle: book %d, stage %d: new game", aBook, aStage);
			StartNewGameTransition();
			AddScriptCommand(mUpdateCount + 20, "battle", StrFormat("%d %d 1", aBook, aStage));
			return true;
		}
		if (mBoard != nullptr && mBoard->mVisible) {
			SDL_Log("script: battle: the board shows");
			if (aStage != 1 && gBattleEngine != nullptr) {
				// the first enemy is in: the engine goes on to the stage's, as if the ones before had been beaten
				AddScriptCommand(mUpdateCount + 60, "lua",
				                 StrFormat("gBattleEngine.mWinner = gBattleEngine.mPlayerPtr; gBattleEngine.mLoser = gBattleEngine.mEnemyPtr; gBattleEngine.mStageNum = %d; "
				                           "gBattleEngine:SetupNextStage()",
				                           aStage - 1));
				SDL_Log("script: battle: on to stage %d", aStage);
			}
			return true;
		}
		if (aPhase > 400) {
			SDL_Log("script: battle: gave up");
			return true;
		}
		if (mMainMenu == nullptr && !IsCurtainMoving() && mDialogMap.empty() && aPhase % 5 == 2)
			AddScriptCommand(mUpdateCount + 1, "click", "400 575");  // the chapter map's Enter button
		AddScriptCommand(mUpdateCount + 20, "battle", StrFormat("%d %d %d", aBook, aStage, aPhase + 1));
		return true;
	}
	if (theCommand.mCommand == "nowords") {
		sSoakNoWords = atoi(theCommand.mArgs.c_str()) != 0;
		return true;
	}
	if (theCommand.mCommand == "wordmax") {
		sSoakMaxWordLen = atoi(theCommand.mArgs.c_str());
		return true;
	}
	if (theCommand.mCommand == "pam") {
		// where a script's animation is: pam <id> (the id is the animation's mAnimId)
		PopAnim* anAnim = mPopAnimManager != nullptr ? mPopAnimManager->Get(atoi(theCommand.mArgs.c_str())) : nullptr;
		if (anAnim == nullptr || anAnim->mRoot == nullptr)
			SDL_Log("script: pam %s: none", theCommand.mArgs.c_str());
		else
			SDL_Log("script: pam %s: playing %d label %s frame %d of %d, fraction %f, rate %d, listener %p", theCommand.mArgs.c_str(),
			        (int)anAnim->mPlaying, anAnim->mLabel.c_str(), anAnim->mRoot->mFrameNum,
			        (int)anAnim->mData->mMain.mFrames.size(), anAnim->mFraction, anAnim->mFrameRate, (void*)anAnim->mListener);
		if (anAnim != nullptr && anAnim->mData != nullptr)
			for (const auto& aLabel : anAnim->mData->mMain.mLabels)
				SDL_Log("script: pam label %s at %d", aLabel.first.c_str(), aLabel.second);
		return true;
	}
	if (theCommand.mCommand == "dismiss") {
		// the game's dialogs (GameDialog) press their yes or ok on Escape
		if (mDialogMap.empty()) {
			SDL_Log("script: dismiss: no dialog");
			return true;
		}
		// Escape goes to the focused widget, which is not always the dialog (the arena's welcome over the treasure
		// room): when the same dialog is still up at the next dismiss, click its first button
		static Dialog* sLastDialog = nullptr;
		static int sSameDialog = 0;
		Dialog* aTop = mDialogList.empty() ? mDialogMap.begin()->second : mDialogList.back();
		sSameDialog = (aTop == sLastDialog) ? sSameDialog + 1 : 0;
		sLastDialog = aTop;
		GameDialog* aGameDialog = dynamic_cast<GameDialog*>(aTop);
		if (sSameDialog >= 1 && aGameDialog != nullptr && aGameDialog->mButton1 != nullptr) {
			FadeButton* aButton = aGameDialog->mButton1;
			// (the dialog's buttons are widgets of the manager, in screen coordinates)
			std::string aClick = std::to_string(aButton->mX + aButton->mWidth / 2) + " " +
			                     std::to_string(aButton->mY + aButton->mHeight / 2);
			SDL_Log("script: dismiss: click %s (update %d)", aClick.c_str(), mUpdateCount);
			AddScriptCommand(mUpdateCount + 1, "click", aClick);
			return true;
		}
		SDL_Log("script: dismiss: Escape (update %d)", mUpdateCount);
		AddScriptCommand(mUpdateCount + 1, "key", "ESCAPE");
		return true;
	}
	if (theCommand.mCommand != "word")
		return false;
	if (mBoard == nullptr || mLex == nullptr) {
		SDL_Log("script: line %d: word: no battle", theCommand.mLine);
		return true;
	}

	bool aBusy = !mDialogMap.empty() || ShowingConversationPanel() || mBoard->IsPaused() ||
	             !mBoard->mSelected.empty() || !mBoard->mNewTiles.empty();
	for (Tile* aTile : mBoard->mTiles)
		aBusy = aBusy || aTile->IsMoving() || aTile->IsFalling() || aTile->mAlphaState != 1;
	// an attack or a defence in progress (the tiles of a word cannot be picked then)
	if (!mBoard->mIsMiniGame && gBattleEngine != nullptr &&
	    (gBattleEngine->mState == BattleEngine::BE_PRE_ATTACK || gBattleEngine->mState == BattleEngine::BE_ATTACK ||
	     gBattleEngine->mState == BattleEngine::BE_DEFEND))
		aBusy = true;
	if (aBusy) {
		int aTries = 0;
		size_t aPos = theCommand.mArgs.find("try=");
		if (aPos != std::string::npos)
			aTries = atoi(theCommand.mArgs.c_str() + aPos + 4);
		if (aTries == 30 && mDialogMap.empty() && !mBoard->mSelected.empty() && !mBoard->IsPaused() &&
		    !ShowingConversationPanel()) {
			// a tile left selected by an earlier click: let go of it
			SDL_Log("script: word: deselecting %d tiles left selected (engine %d)", (int)mBoard->mSelected.size(),
			        gBattleEngine != nullptr ? gBattleEngine->mState : -9);
			mBoard->DeselectAllTiles();
		}
		if (aTries >= 200) {
			int aMoving = 0, aFalling = 0, aFading = 0;
			for (Tile* aTile : mBoard->mTiles) {
				aMoving += aTile->IsMoving();
				aFalling += aTile->IsFalling();
				aFading += aTile->mAlphaState != 1;
			}
			for (const std::pair<const int, Dialog*>& aDialog : mDialogMap)
				SDL_Log("script: busy at %d: dialog id %d (%s) header %s visible %d at %d,%d %dx%d", mUpdateCount, aDialog.first,
				        typeid(*aDialog.second).name(), aDialog.second->mDialogHeader.c_str(),
				        (int)aDialog.second->mVisible, aDialog.second->mX, aDialog.second->mY,
				        aDialog.second->mWidth, aDialog.second->mHeight);
			SDL_Log("script: line %d: word: busy (dialogs %d, conversation %d, paused %d, selected %d, new %d, moving %d, falling %d, fading %d)",
			        theCommand.mLine, (int)mDialogMap.size(), (int)ShowingConversationPanel(), (int)mBoard->IsPaused(),
			        (int)mBoard->mSelected.size(), (int)mBoard->mNewTiles.size(), aMoving, aFalling, aFading);
		}
		else
			AddScriptCommand(mUpdateCount + 10, "word",
			                 StrFormat("%s try=%d", theCommand.mArgs.substr(0, aPos).c_str(), aTries + 1));
		return true;
	}

	TileLetters aTiles;
	for (Tile* aTile : mBoard->mTiles)
		if (!aTile->mSelected && aTile->CanSelect(aTile->mX + 25, aTile->mY + 25))
			aTiles.push_back(std::make_pair(Board::GetWord(std::list<Tile*>(1, aTile)), aTile));
	std::string anEgg = GT(0x7aad5c);  // the word that starts the Astro Pop flash: left out
	std::string aBest;
	std::vector<Tile*> aBestOrder;
	std::vector<Tile*> anOrder;
	bool aWantRuby = theCommand.mArgs.find("ruby") != std::string::npos;
	bool aPreferRuby = theCommand.mArgs.find("preferruby") != std::string::npos;
	if (aPreferRuby)
		aWantRuby = false;
	std::string aBestRuby;
	std::vector<Tile*> aBestRubyOrder;
	for (const std::pair<const std::string, int>& aEntry : mLex->mWords) {
		const std::string& aWord = aEntry.first;
		if (sSoakMaxWordLen > 0 && (int)aWord.size() > sSoakMaxWordLen)
			continue;
		if (aWord.size() > aBest.size() && aWord != anEgg && Spell(aWord, aTiles, anOrder)) {
			if (aWantRuby && std::find_if(anOrder.begin(), anOrder.end(), IsRuby) == anOrder.end())
				continue;
			aBest = aWord;
			aBestOrder = anOrder;
		}
		if (aPreferRuby && aWord.size() > aBestRuby.size() && aWord != anEgg && Spell(aWord, aTiles, anOrder) &&
		    std::find_if(anOrder.begin(), anOrder.end(), IsRuby) != anOrder.end()) {
			aBestRuby = aWord;
			aBestRubyOrder = anOrder;
		}
	}
	if (!aBestRuby.empty()) {
		aBest = aBestRuby;
		aBestOrder = aBestRubyOrder;
	}
	if (aBest.empty()) {
		SDL_Log("script: word: no word");
		return true;
	}
	SDL_Log("script: word %s (update %d, engine %d)", aBest.c_str(), mUpdateCount,
	        gBattleEngine != nullptr ? gBattleEngine->mState : -9);

	int aWhen = mUpdateCount + 1;
	for (Tile* aTile : aBestOrder) {
		AddScriptCommand(aWhen, "click", StrFormat("%d %d", aTile->mX + 25, aTile->mY + 25));
		aWhen += 8;
	}
	if (theCommand.mArgs.find("attack") != std::string::npos) {
		// the button lights up for a mouse that moved over it, before the click
		Widget* anAttack = mBoard->mButtons[Board::BUTTON_ATTACK];
		std::string aCentre = StrFormat("%d %d", mBoard->mX + anAttack->mX + anAttack->mWidth / 2,
		                                mBoard->mY + anAttack->mY + anAttack->mHeight / 2);
		AddScriptCommand(aWhen, "move", aCentre);
		AddScriptCommand(aWhen + 20, "click", aCentre);
	}
	return true;
}

void GameApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-statelog")
		mStateLogFileName = theParamValue;
	else if (theParamName == "-startloadupdates")
		mStartLoadWanted = theParamValue.empty() ? -1 : atoi(theParamValue.c_str());
	else if (theParamName == "-chapterloadupdates") {
		mChapterLoadUpdates.clear();
		size_t aPos = 0;
		while (aPos <= theParamValue.size()) {
			size_t aComma = theParamValue.find(',', aPos);
			if (aComma == std::string::npos)
				aComma = theParamValue.size();
			if (aComma > aPos)
				mChapterLoadUpdates.push_back(atoi(theParamValue.substr(aPos, aComma - aPos).c_str()));
			aPos = aComma + 1;
		}
	} else
		LuaApp::HandleCmdLineParam(theParamName, theParamValue);
}

// -statelog: the values the identical-play comparison reads, written when one changes ("-" when there is none): the
// RNG's draws (Sexy::GetRandCount), and in a battle the engine's state and the tiles of the grid and of the word
void GameApp::LogState()
{
	if (mStateLog == nullptr) {
		mStateLog = fopen(mStateLogFileName.c_str(), "w");
		if (mStateLog == nullptr) {
			mStateLogFileName.clear();
			return;
		}
	}
	std::vector<std::pair<std::string, std::string>> aValues;
	aValues.push_back({"rand", StrFormat("%u", GetRandCount())});
	aValues.push_back({"loaded", StrFormat("%d", (int)mLoadingThreadCompleted)});
	aValues.push_back({"dialogs", StrFormat("%d", GetDialogCount())});
	aValues.push_back({"title", mTitleScreen != nullptr ? "1" : "0"});
	aValues.push_back({"menu", mMainMenu != nullptr ? "1" : "0"});
	aValues.push_back({"trans", StrFormat("%d", (int)mTransitionState)});
	aValues.push_back({"chload", mChapterLoadScreen != nullptr ? "1" : "0"});
	aValues.push_back({"estate", gBattleEngine != nullptr ? StrFormat("%d", gBattleEngine->mState) : "-"});
	aValues.push_back({"tiles", mBoard != nullptr ? StrFormat("%d", (int)mBoard->mTiles.size()) : "-"});
	aValues.push_back({"selected", mBoard != nullptr ? StrFormat("%d", (int)mBoard->mSelected.size()) : "-"});
	for (auto& aValue : aValues) {
		std::string& aLast = mStateLogLast[aValue.first];
		if (aLast != aValue.second) {
			aLast = aValue.second;
			fprintf(mStateLog, "%d %s %s\n", mUpdateCount, aValue.first.c_str(), aValue.second.c_str());
		}
	}
	fflush(mStateLog);
}
