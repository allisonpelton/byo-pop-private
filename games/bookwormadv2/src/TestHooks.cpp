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
//   pam <id>        logs where a script's animation is (playing, label, frame)
//   state           logs the battle engine's state and the board's selected tiles
//   widgets         logs the widget manager's widgets: class, rectangle, visibility, priority, focus
//   dismiss         presses Escape when a dialog is up, which the game's dialogs answer with their yes or ok (logs "script: dismiss: <what it did>")
//   lua <chunk>     runs the rest of the line as Lua (logs "script: lua: <error>" when it fails)
//   spectrum        logs how many cells of the media player's spectrum are lit ("script: spectrum <n> lit")
#include <SDL.h>

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
		static int sRoomStart = -1;  // the tick that the treasure room came up at
		if (mTreasureScreen == nullptr || !mTreasureScreen->mVisible)
			sRoomStart = -1;
		if (!mDialogMap.empty())
			AddScriptCommand(mUpdateCount + 1, "dismiss", "");
		else if (mTreasureScreen != nullptr && mTreasureScreen->mVisible) {
			// the treasure room: two treasures, the companion, then Continue (again and again)
			static const char* sClicks[] = {"50 260", "110 260", "480 330", "400 570"};
			if (sRoomStart < 0)
				sRoomStart = aTick;
			int aStep = (aTick - sRoomStart) / 4;
			if ((aTick - sRoomStart) % 4 == 0) {
				const char* aClick = sClicks[aStep < 4 ? aStep : 3];
				SDL_Log("script: soak %d: treasure room: click %s", mUpdateCount, aClick);
				AddScriptCommand(mUpdateCount + 1, "click", aClick);
			}
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
		else if (mBoard != nullptr) {
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
		SDL_Log("script: dismiss: Escape");
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
