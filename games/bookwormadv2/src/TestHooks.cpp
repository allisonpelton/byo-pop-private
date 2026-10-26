// port: the test scripts' own commands (-script, see SexyAppBase.h); the original had none.
//
//   word [attack]   spells the longest word the battle's grid allows: clicks on its tiles, 8 updates apart
//                   (then a move to the attack button and a click on it when asked). Logs "script: word <WORD>", or "no word". While
//                   the board is busy (a dialog, a conversation, a pause, tiles moving, falling or already in
//                   the word) it tries again every 10 updates, and gives up after 200 tries ("busy").
//   lua <chunk>     runs the rest of the line as Lua (logs "script: lua: <error>" when it fails)
//   spectrum        logs how many cells of the media player's spectrum are lit ("script: spectrum <n> lit")
#include <SDL.h>

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "Board.h"
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
		if (aTries >= 200)
			SDL_Log("script: line %d: word: busy", theCommand.mLine);
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
	for (const std::pair<const std::string, int>& aEntry : mLex->mWords) {
		const std::string& aWord = aEntry.first;
		if (aWord.size() > aBest.size() && aWord != anEgg && Spell(aWord, aTiles, anOrder)) {
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
