// port: the test scripts' own commands (-script, see SexyAppBase.h); the original had none.
//
//   word [attack] [ruby]  spells the longest word the battle's grid allows: clicks on its tiles, 8 updates apart
//                   (then a move to the attack button and a click on it when asked; with ruby, only a word that uses a
//                   ruby tile, which the first battle's tutorial asks for). Logs "script: word <WORD>", or "no word". While
//                   the board is busy (a dialog, a conversation, a pause, tiles moving, falling or already in
//                   the word) it tries again every 10 updates, and gives up after 200 tries ("busy").
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

#include "Board.h"
#include "Dialog.h"
#include "WidgetManager.h"
#include "FadeButton.h"
#include "GameApp.h"
#include "GameText.h"
#include "Lex.h"
#include "SongPlayer.h"
#include "LuaSupport.h"
#include "Tile.h"

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
	if (theCommand.mCommand == "widgets") {
		for (Widget* aWidget : mWidgetManager->mWidgets)
			SDL_Log("script: widget %s at %d,%d %dx%d visible %d disabled %d priority %d z %d%s%s",
			        typeid(*aWidget).name(), aWidget->mX, aWidget->mY, aWidget->mWidth, aWidget->mHeight,
			        (int)aWidget->mVisible, (int)aWidget->mDisabled, aWidget->mPriority, aWidget->mZOrder,
			        aWidget == mWidgetManager->mFocusWidget ? " FOCUS" : "",
			        aWidget == mWidgetManager->mBaseModalWidget ? " MODAL" : "");
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
	if (aBusy) {
		int aTries = 0;
		size_t aPos = theCommand.mArgs.find("try=");
		if (aPos != std::string::npos)
			aTries = atoi(theCommand.mArgs.c_str() + aPos + 4);
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
	for (const std::pair<const std::string, int>& aEntry : mLex->mWords) {
		const std::string& aWord = aEntry.first;
		if (aWord.size() > aBest.size() && aWord != anEgg && Spell(aWord, aTiles, anOrder)) {
			if (aWantRuby && std::find_if(anOrder.begin(), anOrder.end(), IsRuby) == anOrder.end())
				continue;
			aBest = aWord;
			aBestOrder = anOrder;
		}
	}
	if (aBest.empty()) {
		SDL_Log("script: word: no word");
		return true;
	}
	SDL_Log("script: word %s", aBest.c_str());

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
