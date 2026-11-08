// port: the test scripts' own commands (-script, see SexyAppBase.h); the original had none.
//
//   word [attack] [max=N]
//                   spells the longest word the battle's grid allows (at most N letters): clicks on its tiles, 8 updates apart
//                   (then a move to the attack button and a click on it when asked). Logs "script: word <WORD>", or "no word". While
//                   the board is busy (a dialog, a conversation, a pause, tiles moving, falling or already in
//                   the word) it tries again every 10 updates, and gives up after 200 tries ("busy").
//   lua <chunk>     runs the rest of the line as Lua (logs "script: lua: <error>" when it fails)
//   spectrum        logs how many cells of the media player's spectrum are lit ("script: spectrum <n> lit")
//   stall <ms>      the game's thread waits while the sound plays on (a browser tab that is hidden stops the main loop)
//   minimize, restore
//                   the window minimizes or comes back, as SDL's MINIMIZED and RESTORED events do
//   focus lose|regain
//                   the window loses or regains the focus, as SDL's focus events do (a -script run ignores the real
//                   ones); origrun's command of the same name posts the original's WM_ACTIVATEAPP
#include <SDL.h>

#include <cstdio>
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
#include "WidgetManager.h"

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

// port: is the battle's board busy (a dialog, a conversation, a pause, tiles moving, falling or already in the word)
bool GameApp::BoardBusyForTests()
{
	bool aBusy = !mDialogMap.empty() || ShowingConversationPanel() || mBoard->IsPaused() ||
	             !mBoard->mSelected.empty() || !mBoard->mNewTiles.empty();
	for (Tile* aTile : mBoard->mTiles)
		aBusy = aBusy || aTile->IsMoving() || aTile->IsFalling() || aTile->mAlphaState != 1;
	return aBusy;
}

// port: the longest word the board's tiles spell (at most theMax letters), with its tiles in order; empty if none
std::string GameApp::BestWordForTests(size_t theMax, std::vector<Tile*>& theOrder)
{
	TileLetters aTiles;
	for (Tile* aTile : mBoard->mTiles)
		if (!aTile->mSelected && aTile->CanSelect(aTile->mX + 25, aTile->mY + 25))
			aTiles.push_back(std::make_pair(Board::GetWord(std::list<Tile*>(1, aTile)), aTile));
	std::string anEgg = GT(0x7aad5c);  // the word that starts the Astro Pop flash: left out
	size_t aMax = theMax;
	std::string aBest;
	std::vector<Tile*> aBestOrder;
	std::vector<Tile*> anOrder;
	for (const std::string& aWord : mLex->mWords)
		if (aWord.size() > aBest.size() && aWord.size() <= aMax && aWord != anEgg && Spell(aWord, aTiles, anOrder)) {
			aBest = aWord;
			aBestOrder = anOrder;
		}
	theOrder = aBestOrder;
	return aBest;
}

// port: with -boardlog=<file>, a run with real input writes there, each time the battle's board comes to rest with a new
// set of tiles, the word a test driver may spell there: "board: <n> <WORD> x,y ... attack x,y scramble x,y" (tools/realrun.py's
// "word"). Only what the driver sees: its clicks go through the real input
void GameApp::LogBoardForTests()
{
	static std::string sLast;
	static int sCount = 0;
	if (mBoard == nullptr || mLex == nullptr || BoardBusyForTests()) {
		sLast.clear();
		return;
	}
	// the word list is searched only when the tiles changed
	std::string aKey;
	for (Tile* aTile : mBoard->mTiles)
		aKey += StrFormat("%s%d,%d%c;", Board::GetWord(std::list<Tile*>(1, aTile)).c_str(), aTile->mX, aTile->mY,
		                  aTile->CanSelect(aTile->mX + 25, aTile->mY + 25) ? '+' : '-');
	if (aKey == sLast)
		return;
	sLast = aKey;
	std::vector<Tile*> anOrder;
	std::string aWord = BestWordForTests(std::string::npos, anOrder);
	std::string aLine = aWord.empty() ? "-" : aWord;
	for (Tile* aTile : anOrder)
		aLine += StrFormat(" %d,%d", aTile->mX + 25, aTile->mY + 25);
	Widget* anAttack = mBoard->mButtons[Board::BUTTON_ATTACK];
	Widget* aScramble = mBoard->mButtons[Board::BUTTON_SCRAMBLE];
	aLine += StrFormat(" attack %d,%d scramble %d,%d", mBoard->mX + anAttack->mX + anAttack->mWidth / 2,
	                   mBoard->mY + anAttack->mY + anAttack->mHeight / 2, mBoard->mX + aScramble->mX + aScramble->mWidth / 2,
	                   mBoard->mY + aScramble->mY + aScramble->mHeight / 2);
	if (FILE* aFile = fopen(mBoardLogForTests.c_str(), "a")) {
		fprintf(aFile, "board: %d %s\n", ++sCount, aLine.c_str());
		fclose(aFile);
	}
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
	if (theCommand.mCommand == "focus") {
		mActive = theCommand.mArgs.find("regain") != std::string::npos;
		RehupFocus();
		if (mActive && !mIsWindowed)
			mWidgetManager->MarkAllDirty();
		return true;
	}
	if (theCommand.mCommand == "stall") {
		// the game's thread waits while the sound goes on, as a browser tab that is hidden stops the main loop
		SDL_Delay((Uint32)atoi(theCommand.mArgs.c_str()));
		return true;
	}
	if (theCommand.mCommand == "minimize" || theCommand.mCommand == "restore") {
		// as the framework takes SDL's MINIMIZED and RESTORED (WM_SIZE), which a -script run drops: the widgets stop
		// updating while minimized, and the sound has one more mute
		bool aMinimized = theCommand.mCommand == "minimize";
		mPhysMinimized = aMinimized;
		if (aMinimized != mMinimized) {
			mMinimized = aMinimized;
			if (mMuteOnLostFocus) {
				if (mMinimized)
					Mute(true);
				else
					Unmute(true);
			}
			if (!mMinimized)
				mWidgetManager->MarkAllDirty();
		}
		RehupFocus();
		return true;
	}
	if (theCommand.mCommand != "word")
		return false;
	if (mBoard == nullptr || mLex == nullptr) {
		SDL_Log("script: line %d: word: no battle", theCommand.mLine);
		return true;
	}

	if (BoardBusyForTests()) {
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

	size_t aMax = std::string::npos;
	size_t aMaxPos = theCommand.mArgs.find("max=");
	if (aMaxPos != std::string::npos)
		aMax = (size_t)atoi(theCommand.mArgs.c_str() + aMaxPos + 4);
	std::vector<Tile*> aBestOrder;
	std::string aBest = BestWordForTests(aMax, aBestOrder);
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
