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
//   statedump       logs the battle engine's state and the board's selected tiles (the F9 key in a test run)
//   widgets         logs the widget manager's widgets: class, rectangle, visibility, priority, focus
//   dismiss         presses Escape when a dialog is up, which the game's dialogs answer with their yes or ok; when the same
//                   dialog is still there at the next dismiss, clicks its first button (logs "script: dismiss: <what it did>")
//   lua <chunk>     runs the rest of the line as Lua (logs "script: lua: <error>" when it fails)
//   spectrum        logs how many cells of the media player's spectrum are lit ("script: spectrum <n> lit")
//
// The drivers' commands (docs/TEST_COMMANDS.md): each answers "OK <command>", "ERROR <command>: <reason>", "STATE {json}"
// or "MOVE {json}|null" on the log. A command that waits for its screen (a curtain, the title) tries again every 10
// updates, 300 times at most. A command never given changes nothing.
//   state           STATE {json}: screen, mode, level (book.chapter), loaded (the title: its startup load over),
//                   health, maxhealth, enemy, enemyhealth, and the board: its letters row by row ("." for none), "+" after
//                   a letter that can be picked, paused, busy; curtain, dialogs, talk
//   goto <where>    menu (once the title has gone on to it), credits; adventure, arena, minigames, tome, options,
//                   highscores (the main menu's items); a mini-game by its class (wordmaster, letterrip, linknspell,
//                   wordup, mutawords, goldencoins: the mini-games' menu's buttons, opened first from the main menu)
//   unlock all      the arena, the mini-games and the tome, as the end of a book unlocks them
//   set health|enemy <n>   Lex's or the enemy's health (the scripts' _LexSetHealth, _EnemySetHealth)
//   win             the battle's enemy dies (the scripts' _KillEnemy, only while the battle waits for a word)
//   lose            Lex near death (the scripts' _lex_near_death), so that the enemy's next attack ends the battle
//   move            MOVE {json} the move the drivers' bot makes next, or MOVE null (MoveForTests); it changes nothing
//   level, seed     ERROR: the map's chapters are the scripts' (goto adventure); the game's seed is -seed=<n>
#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/heap.h>
#endif

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <list>
#include <sstream>
#include <cstdlib>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

#include "BattleEngine.h"
#include "ArrowHint.h"
#include "Board.h"
#include "CurtainTransition.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "WidgetManager.h"
#include "FadeButton.h"
#include "GameApp.h"
#include "GameDialog.h"
#include "GameText.h"
#include "Lex.h"
#include "SongPlayer.h"
#include "LuaSupport.h"
#include "MainMenu.h"
#include "MinigameMenu.h"
#include "TitleScreen.h"
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

namespace {

// runs theChunk; its first result, as Lua's tostring gives it, in theResult; false with the error in theResult
bool RunLua(const std::string& theChunk, std::string& theResult)
{
	lua_State* L = LuaLib::gLuaSupport->L;
	int aTop = lua_gettop(L);
	bool anOk = luaL_loadbuffer(L, theChunk.data(), theChunk.size(), "=test") == 0 && lua_pcall(L, 0, 1, 0) == 0;
	const char* aText = lua_tostring(L, -1);
	theResult = aText != nullptr ? aText : "";
	lua_settop(L, aTop);
	return anOk;
}

std::string JsonString(const std::string& theText)
{
	std::string aJson = "\"";
	for (char c : theText) {
		if (c == '"' || c == '\\')
			aJson += '\\';
		if ((unsigned char)c >= 0x20)
			aJson += c;
	}
	return aJson + "\"";
}

std::string Squash(const std::string& theText)  // lower case, letters and digits only
{
	std::string aSquashed;
	for (char c : theText)
		if (isalnum((unsigned char)c))
			aSquashed += (char)tolower((unsigned char)c);
	return aSquashed;
}

}  // namespace

// port: the screen showing, for the test command state
std::string GameApp::ScreenForTests()
{
	return mCutScene != nullptr ? "cutscene" : mCredits != nullptr ? "credits" : mHallOfFame != nullptr ? "highscores"
	       : mBookScreen != nullptr ? "tome" : mMiniGameMenu != nullptr ? "minigames"
	       : mTreasureScreen != nullptr && mTreasureScreen->mVisible ? "treasure"
	       : mBoard != nullptr && mBoard->mVisible ? (mBoard->IsMiniGame() ? "minigame" : "battle")
	       : mChapterLoadScreen != nullptr ? "loading" : mBookManager != nullptr ? "map"
	       : mMainMenu != nullptr ? "menu" : mTitleScreen != nullptr ? "title" : "none";
}

// port: is the battle's board busy (a dialog, a conversation, a pause, tiles moving, falling or already in the word)
bool GameApp::BoardBusyForTests()
{
	bool aBusy = !mDialogMap.empty() || ShowingConversationPanel() || mBoard->IsPaused() ||
	             !mBoard->mSelected.empty() || !mBoard->mNewTiles.empty();
	for (Tile* aTile : mBoard->mTiles)
		aBusy = aBusy || aTile->IsMoving() || aTile->IsFalling() || aTile->mAlphaState != 1;
	return aBusy;
}

// port: the longest word the board's tiles spell (at most theMax letters), with its tiles in order; empty if none.
// With theLongest, also the theCount longest words they spell, longest first. (Volume 2 has no Astro Pop egg word to
// leave out: Board::IsValidWord)
std::string GameApp::BestWordForTests(size_t theMax, std::vector<Tile*>& theOrder, std::vector<std::string>* theLongest,
                                      size_t theCount)
{
	TileLetters aTiles;
	for (Tile* aTile : mBoard->mTiles)
		if (!aTile->mSelected && aTile->CanSelect(aTile->mX + 25, aTile->mY + 25))
			aTiles.push_back(std::make_pair(Board::GetWord(std::list<Tile*>(1, aTile)), aTile));
	std::string aBest;
	std::vector<Tile*> aBestOrder;
	std::vector<Tile*> anOrder;
	std::vector<std::string> aWords;
	for (const std::pair<const std::string, int>& anEntry : mLex->mWords) {
		const std::string& aWord = anEntry.first;
		if ((theLongest != nullptr || aWord.size() > aBest.size()) && aWord.size() <= theMax && Spell(aWord, aTiles, anOrder)) {
			if (aWord.size() > aBest.size()) {
				aBest = aWord;
				aBestOrder = anOrder;
			}
			if (theLongest != nullptr)
				aWords.push_back(aWord);
		}
	}
	if (theLongest != nullptr) {
		std::stable_sort(aWords.begin(), aWords.end(),
		                 [](const std::string& a, const std::string& b) { return a.size() > b.size(); });
		if (aWords.size() > theCount)
			aWords.resize(theCount);
		*theLongest = aWords;
	}
	theOrder = aBestOrder;
	return aBest;
}

// port: Word Master's rows (theRows: "<line>|" then, for each row up to the current one, five "<letter><status>",
// status 2 here, 1 elsewhere, 0 not in the word, - not answered; empty when there are none), each row's ten characters
std::vector<std::string> GameApp::WordMasterRowsForTests(std::string& theRows)
{
	std::vector<std::string> aRow;
	if (!RunLua("local g = gMiniGame if g == nil or g.mTiles == nil or g.mLine == nil then return '' end "
	            "local out = tostring(g.mLine) "
	            "for r = 0, g.mLine do local row = g.mTiles[r] out = out .. '|' "
	            "for i = 0, 4 do local t = row and row[i] "
	            "local l = t and t.mTable and t.mTable.mLetter or '.' if l == '&' then l = 'Q' end "
	            "out = out .. l .. (t and t.mStatus and tostring(t.mStatus) or '-') end end return out",
	            theRows))
		theRows.clear();
	for (size_t aPos = theRows.find('|'); aPos != std::string::npos;) {
		size_t aNext = theRows.find('|', aPos + 1);
		aRow.push_back(theRows.substr(aPos + 1, aNext == std::string::npos ? std::string::npos : aNext - aPos - 1));
		aPos = aNext;
	}
	return aRow;
}

// port: Word Master's guess for the rows it shows (each "<letter><status>" five times; the last the row being typed):
// "<GUESS|-> x,y ... submit x,y", the keys that type it and Submit
std::string GameApp::WordMasterGuessForTests(const std::vector<std::string>& theRow)
{
	// the colours a guess would get against theWord: right place first, then elsewhere while the word has letters left
	auto Score = [](const std::string& theGuess, const std::string& theWord) {
		std::string aScore(5, '0');
		int aLeft[26] = {0};
		for (int i = 0; i < 5; i++) {
			if (theGuess[i] == theWord[i])
				aScore[i] = '2';
			else
				aLeft[theWord[i] - 'A']++;
		}
		for (int i = 0; i < 5; i++)
			if (aScore[i] != '2' && aLeft[theGuess[i] - 'A'] > 0) {
				aScore[i] = '1';
				aLeft[theGuess[i] - 'A']--;
			}
		return aScore;
	};
	std::string aGuess;
	for (const std::pair<const std::string, int>& anEntry : mLex->mWords) {
		const std::string& aWord = anEntry.first;
		if (aWord.size() != 5 || aWord.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos)
			continue;
		bool aFits = true;
		for (size_t r = 0; r < theRow.size(); r++) {
			const std::string& aCells = theRow[r];
			if (aCells.size() != 10)
				continue;
			std::string aLetters, aStatus;
			for (int i = 0; i < 5; i++) {
				aLetters += aCells[i * 2];
				aStatus += aCells[i * 2 + 1];
			}
			if (r + 1 < theRow.size()) {
				aFits = aFits && Score(aLetters, aWord) == aStatus;
			} else {
				// the row being typed: the letters it already shows (the first, given) stay
				for (int i = 0; i < 5; i++)
					if (aLetters[i] != '.')
						aFits = aFits && aWord[i] == aLetters[i];
			}
		}
		if (aFits) {
			aGuess = aWord;
			break;
		}
	}
	// the keys: the board's letter tiles; the row's letters already shown are not typed again
	std::string aLine = aGuess.empty() ? "-" : aGuess;
	const std::string& aCurrent = theRow.empty() ? std::string() : theRow.back();
	for (size_t i = 0; i < aGuess.size(); i++) {
		if (aCurrent.size() == 10 && aCurrent[i * 2] != '.')
			continue;
		for (Tile* aTile : mBoard->mTiles)
			if (Board::GetWord(std::list<Tile*>(1, aTile)) == aGuess.substr(i, 1) ||
			    (aGuess[i] == 'Q' && (Board::GetWord(std::list<Tile*>(1, aTile)) == "QU" ||
			                          Board::GetWord(std::list<Tile*>(1, aTile)) == "&"))) {
				aLine += StrFormat(" %d,%d", aTile->mX + 25, aTile->mY + 25);
				break;
			}
	}
	auto anIt = mBoard->mButtons.find(Board::BUTTON_ATTACK);
	if (anIt != mBoard->mButtons.end() && anIt->second != nullptr)
		aLine += StrFormat(" submit %d,%d", mBoard->mX + anIt->second->mX + anIt->second->mWidth / 2,
		                   mBoard->mY + anIt->second->mY + anIt->second->mHeight / 2);
	return aLine;
}

// port: the move the drivers' bot makes next (the standard "move" command, docs/TEST_COMMANDS.md), as JSON with its real
// input, or "null" while there is none (the board busy, a curtain moving). A dialog's first button, a conversation, the
// tutorial's tile or Attack, a lost turn's grid, an arrow's hot spot, then the board's longest word with Attack (or
// Scramble), Word Master's guess, the treasure room's treasures and Continue, the map's Enter. It reads the game and
// changes nothing
std::string GameApp::MoveForTests()
{
	auto Click = [](int x, int y) { return StrFormat("{\"click\":[%d,%d]}", x, y); };
	auto Move = [](const std::string& theWhat, const std::string& theInput, const std::string& theMore = "") {
		return "{\"what\":\"" + theWhat + "\"" + theMore + ",\"input\":[" + theInput + "]}";
	};
	auto Centre = [](Widget* theWidget, int theX, int theY) {
		return StrFormat("%d,%d", theX + theWidget->mWidth / 2, theY + theWidget->mHeight / 2);
	};
	if (mCurtain != nullptr && !(mCurtain->mDone && mCurtain->mDrawn))
		return "null";
	// a dialog's first button, once it takes clicks
	if (!mDialogMap.empty()) {
		Dialog* aDialog = mDialogMap.rbegin()->second;
		Widget* aButton = aDialog->mYesButton;
		if (GameDialog* aGameDialog = dynamic_cast<GameDialog*>(aDialog))
			aButton = aGameDialog->mButton1;
		if (aButton == nullptr || aDialog->mUpdateCnt <= 30)
			return "null";
		Point aPos = aButton->GetAbsPos();
		return Move("dialog", Click(aPos.mX + aButton->mWidth / 2, aPos.mY + aButton->mHeight / 2),
		            StrFormat(",\"id\":%d", aDialog->mId));
	}
	if (ShowingConversationPanel())
		return Move("talk", Click(400, 150));
	std::string aScreen = ScreenForTests();
	bool aBattle = mBoard != nullptr && mBoard->mVisible && !mBoard->IsMiniGame();
	// the first battle's tutorial: one tile at a time, then Attack
	std::string aState;
	if (aBattle &&
	    RunLua("if IntroTutorial == nil or IntroTutorial.mDone or IntroTutorial.mState == nil then return '' end "
	           "if IntroTutorial.mState < 0 then return '' end return tostring(IntroTutorial.mState)",
	           aState) &&
	    !aState.empty()) {
		Tile* aNext = nullptr;
		int aCount = 0;
		for (Tile* aTile : mBoard->mTiles) {
			if (aTile->IsMoving() || aTile->IsFalling() || aTile->mAlphaState != 1)
				return "null";
			if (!aTile->mSelected && aTile->CanSelect(aTile->mX + 25, aTile->mY + 25)) {
				aNext = aTile;
				aCount++;
			}
		}
		if (aCount == 1)
			return Move("tutorial", Click(aNext->mX + 25, aNext->mY + 25));
		auto anAttack = mBoard->mButtons.find(Board::BUTTON_ATTACK);
		if (aCount == 0 && !mBoard->mSelected.empty() && anAttack != mBoard->mButtons.end() && anAttack->second != nullptr)
			return Move("tutorial", Click(mBoard->mX + anAttack->second->mX + anAttack->second->mWidth / 2,
			                              mBoard->mY + anAttack->second->mY + anAttack->second->mHeight / 2));
	}
	// a lost turn (Lex stunned), once two questions 30 updates apart or more have seen it (a turn changing shows one for
	// a moment): a click on the grid ends its round
	static int sLostSince = -1;
	bool aLostTurn = aBattle && gBattleEngine != nullptr && !mBoard->IsPaused() && gBattleEngine->LostTurn() &&
	                 gBattleEngine->mState == BattleEngine::BE_IDLE &&
	                 gBattleEngine->CallLinkedMethod("CanEndLostTurnRound", 0).Bool() &&
	                 gBattleEngine->CallLinkedMethod("AttackerIsPlayer", 0).Bool() && !mBoard->mTiles.empty();
	if (!aLostTurn) {
		sLostSince = -1;
	} else {
		if (sLostSince < 0)
			sLostSince = mUpdateCount;
		if (mUpdateCount - sLostSince < 30)
			return Move("wait", "{\"wait\":10}");
		int aX = 0, aY = 0;
		for (Tile* aTile : mBoard->mTiles) {
			aX += aTile->mX + 25;
			aY += aTile->mY + 25;
		}
		return Move("lostturn", Click(aX / (int)mBoard->mTiles.size(), aY / (int)mBoard->mTiles.size()));
	}
	// an arrow waiting for a click on its hot spot (one that a click 60 updates ago left standing is not asked again:
	// the map's own hints wait for a chapter's click)
	static ArrowHint* sHint = nullptr;
	static int sHintId = -1, sHintAt = -1;
	if (mArrowHintManager != nullptr)
		for (ArrowHint* anArrow : mArrowHintManager->mHints)
			if (anArrow->mActive && anArrow->mState == ArrowHint::STATE_SHOWING && !anArrow->mHotSpots.empty()) {
				if (anArrow != sHint || anArrow->mId != sHintId) {
					sHint = anArrow;
					sHintId = anArrow->mId;
					sHintAt = mUpdateCount;
				} else if (mUpdateCount - sHintAt > 60) {
					continue;
				}
				const Rect& aSpot = anArrow->mHotSpots.front();
				return Move("hint", Click(aSpot.mX + aSpot.mWidth / 2, aSpot.mY + aSpot.mHeight / 2),
				            StrFormat(",\"id\":%d", anArrow->mId));
			}
	// an overlay over the battle's board that a click takes away (Level Up's "Click to continue", the scripts' own):
	// a click above the board, where the drivers click conversations on
	if (aBattle && (mBoard->mLevelupOverlay || mBoard->HasOverlays()) && !mBoard->IsPaused())
		return Move("overlay", Click(400, 150));
	// a mini-game over ("Click anywhere to continue"): a click
	std::string anEnd;
	if (mBoard != nullptr && mBoard->IsMiniGame() &&
	    RunLua("if gMiniGame == nil or gMiniGame.mEndGameState == nil then return '' end "
	           "return tostring(gMiniGame.mEndGameState)", anEnd) &&
	    !anEnd.empty() && anEnd != "0")
		return Move("gameover", Click(400, 150));
	// Word Master: the guess's keys, then Submit
	if (mBoard != nullptr && mLex != nullptr && mBoard->IsMiniGame() && mMiniGameName == "WordMaster") {
		std::string aRows;
		std::vector<std::string> aRow = WordMasterRowsForTests(aRows);
		if (aRow.empty() || BoardBusyForTests())
			return "null";
		std::string aLine = WordMasterGuessForTests(aRow);
		if (aLine[0] == '-' || aLine.find(" submit ") == std::string::npos)
			return "null";
		std::string aGuess = aLine.substr(0, aLine.find(' ')), anInput;
		std::istringstream aStream(aLine.substr(aGuess.size()));
		std::vector<std::pair<int, int>> aPressed;
		for (std::string aToken; aStream >> aToken;) {
			int x = 0, y = 0;
			if (sscanf(aToken.c_str(), "%d,%d", &x, &y) != 2)
				continue;
			// a key pressed again (a letter the guess has twice): 60 updates after the first, since the key drops a press
			// that comes sooner, and with a press on the empty curtain below the keys between, since two presses on one spot
			// within the double-click time (milliseconds of real time, which a fast run's updates do not take) make a double
			// click, which the key ignores
			bool anAgain = std::find(aPressed.begin(), aPressed.end(), std::make_pair(x, y)) != aPressed.end();
			if (!anInput.empty())
				anInput += anAgain ? ",{\"wait\":8}," + Click(30, 560) + ",{\"wait\":52}," : std::string(",{\"wait\":8},");
			anInput += Click(x, y);
			aPressed.push_back(std::make_pair(x, y));
		}
		return Move("guess", anInput, ",\"word\":" + JsonString(aGuess));
	}
	// a board (a battle's, a mini-game's): its longest word, then Attack; Scramble when it has none
	if (mBoard != nullptr && mBoard->mVisible && mLex != nullptr) {
		// a word left in the bar (a Submit that did not take, so that the next word's letters piled onto it): once it
		// has stood for 100 updates, Submit when it is a word, else its last letter given back, as a player does
		static int sSelectedSince = -1;
		bool aStill = !mBoard->mSelected.empty() && mDialogMap.empty() && !ShowingConversationPanel() &&
		              !mBoard->IsPaused();
		for (Tile* aTile : mBoard->mTiles)
			aStill = aStill && !aTile->IsMoving() && !aTile->IsFalling();
		if (!aStill)
			sSelectedSince = -1;
		else if (sSelectedSince < 0 || sSelectedSince > mUpdateCount)
			sSelectedSince = mUpdateCount;
		else if (mUpdateCount - sSelectedSince >= 100) {
			sSelectedSince = mUpdateCount;
			auto anAttack = mBoard->mButtons.find(Board::BUTTON_ATTACK);
			if (mBoard->IsValidWord(Board::GetWord(mBoard->mSelected), true) && anAttack != mBoard->mButtons.end() &&
			    anAttack->second != nullptr && !anAttack->second->mDisabled) {
				Widget* aButton = anAttack->second;
				std::string aCentre = Centre(aButton, mBoard->mX + aButton->mX, mBoard->mY + aButton->mY);
				return Move("unstick", "{\"move\":[" + aCentre + "]},{\"wait\":20},{\"click\":[" + aCentre + "]}");
			}
			Tile* aLast = mBoard->mSelected.back();
			return Move("unstick", Click(aLast->mX + 25, aLast->mY + 25));
		}
		if (BoardBusyForTests())
			return "null";
		bool aPickable = false;
		for (Tile* aTile : mBoard->mTiles)
			aPickable = aPickable || aTile->CanSelect(aTile->mX + 25, aTile->mY + 25);
		if (!aPickable)
			return "null";  // the enemy's turn
		std::vector<Tile*> anOrder;
		std::string aWord = BestWordForTests(std::string::npos, anOrder);
		if (mBoard->IsMiniGame()) {
			// a mini-game wants words it has not had yet (Letter Rip deals one word to take others from): the longest
			// that this mini-game's bot has not offered before
			static Board* sMiniBoard = nullptr;
			static std::vector<std::string> sOffered;
			if (sMiniBoard != mBoard) {
				sMiniBoard = mBoard;
				sOffered.clear();
			}
			std::vector<std::string> aLongest;
			BestWordForTests(std::string::npos, anOrder, &aLongest, 200);
			aWord.clear();
			anOrder.clear();
			for (const std::string& aCandidate : aLongest)
				if (std::find(sOffered.begin(), sOffered.end(), aCandidate) == sOffered.end()) {
					aWord = aCandidate;
					break;
				}
			if (!aWord.empty()) {
				TileLetters aTiles;
				for (Tile* aTile : mBoard->mTiles)
					if (!aTile->mSelected && aTile->CanSelect(aTile->mX + 25, aTile->mY + 25))
						aTiles.push_back(std::make_pair(Board::GetWord(std::list<Tile*>(1, aTile)), aTile));
				Spell(aWord, aTiles, anOrder);
				sOffered.push_back(aWord);
			}
		}
		auto anAttack = mBoard->mButtons.find(Board::BUTTON_ATTACK);
		auto aScramble = mBoard->mButtons.find(Board::BUTTON_SCRAMBLE);
		if (aWord.empty()) {
			if (aScramble == mBoard->mButtons.end() || aScramble->second == nullptr)
				return "null";
			Widget* aButton = aScramble->second;
			std::string aCentre = Centre(aButton, mBoard->mX + aButton->mX, mBoard->mY + aButton->mY);
			return Move("scramble", "{\"move\":[" + aCentre + "]},{\"wait\":20},{\"click\":[" + aCentre + "]}");
		}
		std::string anInput;
		for (Tile* aTile : anOrder)
			anInput += (anInput.empty() ? "" : ",{\"wait\":8},") + Click(aTile->mX + 25, aTile->mY + 25);
		if (anAttack != mBoard->mButtons.end() && anAttack->second != nullptr) {
			// the button lights up for a mouse that moved over it, before the click
			Widget* aButton = anAttack->second;
			std::string aCentre = Centre(aButton, mBoard->mX + aButton->mX, mBoard->mY + aButton->mY);
			anInput += ",{\"wait\":8},{\"move\":[" + aCentre + "]},{\"wait\":20},{\"click\":[" + aCentre + "]}";
		}
		return Move("word", anInput, ",\"word\":" + JsonString(aWord));
	}
	// the treasure room: Continue once it is enabled, else the next treasure (the companion first, as the soak picks)
	if (aScreen == "treasure") {
		auto anIt = mTreasureScreen->mButtons.find(TreasureScreen::BUTTON_CONTINUE);
		if (anIt == mTreasureScreen->mButtons.end() || anIt->second == nullptr || !anIt->second->mVisible)
			return "null";
		if (!anIt->second->mDisabled) {
			Point aPos = anIt->second->GetAbsPos();
			return Move("continue", Click(aPos.mX + anIt->second->mWidth / 2, aPos.mY + anIt->second->mHeight / 2));
		}
		// the first unselected treasure of the kind wanted (Volume 2's table holds each treasure's hot spot, mHX and mHY)
		std::string anAt;
		RunLua("local comp, cands = false, {} "
		       "for k, v in pairs(gTreasureScreen.mTreasures) do "
		       "if v.mSelected then if v.mIsCompanion then comp = true end else table.insert(cands, v) end end "
		       "for i = 1, table.getn(cands) do "
		       "if (cands[i].mIsCompanion and true or false) == (not comp) then "
		       "return (cands[i].mHX + 10) .. ' ' .. (cands[i].mHY + 10) end end return ''",
		       anAt);
		int x = 0, y = 0;
		if (sscanf(anAt.c_str(), "%d %d", &x, &y) != 2)
			return "null";
		return Move("treasure", Click(x, y));
	}
	if (aScreen == "map")
		return Move("enter", Click(400, 575));
	return "null";
}

// port: a test command tried again in 10 updates. In a -script run it goes into the script; on the live channel
// (-commands, the web page's call) into a queue of its own, since a command in the script would make the framework drop
// every real event from then on (SexyAppBase::HandleEvent)
void GameApp::RetryForTests(const std::string& theCommand, const std::string& theArgs)
{
	if (!mScriptFileName.empty()) {
		AddScriptCommand(mUpdateCount + 10, theCommand, theArgs);
		return;
	}
	ScriptCommand aCommand;
	aCommand.mUpdateCount = mUpdateCount + 10;
	aCommand.mCommand = theCommand;
	aCommand.mArgs = theArgs;
	aCommand.mLine = 0;
	mRetriesForTests.push_back(aCommand);
}

// port: the retries that are due (UpdateFrames)
void GameApp::RunRetriesForTests()
{
	std::vector<ScriptCommand> aDue;
	for (size_t i = 0; i < mRetriesForTests.size();) {
		if (mRetriesForTests[i].mUpdateCount <= mUpdateCount) {
			aDue.push_back(mRetriesForTests[i]);
			mRetriesForTests.erase(mRetriesForTests.begin() + (long)i);
		} else {
			i++;
		}
	}
	for (const ScriptCommand& aCommand : aDue)
		HandleScriptCommand(aCommand);
}

// port: the drivers' commands (see the top of this file); false for any other command
bool GameApp::HandleDriverCommand(const ScriptCommand& theCommand)
{
	const std::string& aCommand = theCommand.mCommand;
	if (aCommand != "state" && aCommand != "goto" && aCommand != "level" && aCommand != "unlock" &&
	    aCommand != "set" && aCommand != "win" && aCommand != "lose" && aCommand != "seed" &&
	    aCommand != "move")
		return false;
	// the arguments, without a "try=" a retry added
	std::string anArgs = theCommand.mArgs;
	int aTries = 0;
	size_t aTryPos = anArgs.find(" try=");
	if (aTryPos != std::string::npos) {
		aTries = atoi(anArgs.c_str() + aTryPos + 5);
		anArgs.erase(aTryPos);
	}
	std::string aFull = anArgs.empty() ? aCommand : aCommand + " " + anArgs;
	auto Ok = [&]() { SDL_Log("OK %s", aFull.c_str()); return true; };
	auto Error = [&](const std::string& theReason) { SDL_Log("ERROR %s: %s", aFull.c_str(), theReason.c_str()); return true; };
	auto Later = [&](const std::string& theWhy) {
		if (aTries >= 300)
			return Error(theWhy);
		RetryForTests(aCommand, StrFormat("%s try=%d", anArgs.c_str(), aTries + 1));
		return true;
	};
	bool aCurtainBusy = mCurtain != nullptr && !(mCurtain->mDone && mCurtain->mDrawn);
	bool aBattle = mBoard != nullptr && !mBoard->IsMiniGame();

	if (aCommand == "state") {
		std::string aScreen = ScreenForTests();
		std::string aJson = "{\"screen\":" + JsonString(aScreen);
		if (mBoard != nullptr && !aBattle)
			aJson += ",\"mode\":" + JsonString(mMiniGameName);
		else if (mProfile != nullptr && mProfile->KeyExists("CurrentBook"))
			aJson += StrFormat(",\"mode\":\"%s\",\"level\":\"%d.%d\"", mTimedMode ? "arena" : "adventure",
			                   mProfile->GetInt("CurrentBook"), mProfile->GetInt("CurrentChapter"));
		aJson += StrFormat(",\"curtain\":%s,\"dialogs\":%d,\"talk\":%s", aCurtainBusy ? "true" : "false",
		                   (int)mDialogMap.size(), ShowingConversationPanel() ? "true" : "false");
		if (aScreen == "title" && mTitleScreen != nullptr)  // the startup load over: the title takes its click
			aJson += StrFormat(",\"loaded\":%s", mTitleScreen->mLoaded ? "true" : "false");
		std::string aValue;
		if (aBattle && gBattleEngine != nullptr &&
		    RunLua("local e = gBattleEngine local p, n = e.mPlayerPtr, e.mEnemyPtr "
		           "return p.mHealth .. ',' .. p.mMaxHealth .. ',' .. (n and n.mHealth or '') .. ',' .. "
		           "(n and n.mMaxHealth or '') .. ',' .. (n and n.mName or '')", aValue)) {
			double aHealth = 0, aMax = 0, anEnemy = 0, anEnemyMax = 0;  // in hearts, by quarters
			sscanf(aValue.c_str(), "%lf,%lf,%lf,%lf", &aHealth, &aMax, &anEnemy, &anEnemyMax);
			aJson += StrFormat(",\"health\":%g,\"maxhealth\":%g,\"enemyhealth\":%g,\"enemymaxhealth\":%g", aHealth, aMax,
			                   anEnemy, anEnemyMax);
			size_t aComma = aValue.rfind(',');
			aJson += ",\"enemy\":" + JsonString(aComma == std::string::npos ? "" : aValue.substr(aComma + 1));
		}
		if (mBoard != nullptr) {
			// the letters by row and column
			int aRows = 0, aCols = 0;
			for (Tile* aTile : mBoard->mTiles) {
				aRows = std::max(aRows, aTile->mRow + 1);
				aCols = std::max(aCols, aTile->mCol + 1);
			}
			std::vector<std::string> aCells((size_t)(aRows * aCols), ".");
			for (Tile* aTile : mBoard->mTiles)
				if (aTile->mRow >= 0 && aTile->mCol >= 0)
					aCells[(size_t)(aTile->mRow * aCols + aTile->mCol)] =
					    Board::GetWord(std::list<Tile*>(1, aTile)) +
					    (aTile->CanSelect(aTile->mX + 25, aTile->mY + 25) ? "+" : "");
			aJson += ",\"board\":[";
			for (int r = 0; r < aRows; r++) {
				std::string aRow;
				for (int c = 0; c < aCols; c++)
					aRow += (c > 0 ? " " : "") + aCells[(size_t)(r * aCols + c)];
				aJson += (r > 0 ? "," : "") + JsonString(aRow);
			}
			aJson += StrFormat("],\"paused\":%s,\"busy\":%s", mBoard->IsPaused() ? "true" : "false",
			                   BoardBusyForTests() ? "true" : "false");
		}
		SDL_Log("STATE %s}", aJson.c_str());
		return true;
	}

	if (aCommand == "move") {
		SDL_Log("MOVE %s", MoveForTests().c_str());
		return true;
	}

	if (aCommand == "goto") {
		std::string aWhere = Squash(anArgs);
		static const std::pair<const char*, int> kItems[] = {
		    {"adventure", MainMenu::ITEM_ADVENTURE}, {"arena", MainMenu::ITEM_ARENA},
		    {"minigames", MainMenu::ITEM_MINI_GAMES}, {"tome", MainMenu::ITEM_TOME},
		    {"options", MainMenu::ITEM_OPTIONS}, {"highscores", MainMenu::ITEM_HALL_OF_FAME}};
		bool aMenuReady = mMainMenu != nullptr && mTitleScreen == nullptr && !aCurtainBusy && mDialogMap.empty() &&
		                  mBookScreen == nullptr && mHallOfFame == nullptr && mMiniGameMenu == nullptr && mBoard == nullptr &&
		                  mCredits == nullptr && !mMainMenu->mDisabled;
		if (aWhere == "menu")
			return aMenuReady ? Ok() : Later("not at the main menu");
		if (aWhere == "credits") {
			if (!aMenuReady)
				return Later("not at the main menu");
			StartCredits();
			return Ok();
		}
		for (const auto& anItem : kItems)
			if (aWhere == anItem.first) {
				if (!aMenuReady)
					return Later("not at the main menu");
				if (!mMainMenu->ActivateItem(anItem.second))
					return Error("the menu's item cannot be clicked");
				return Ok();
			}
		// a mini-game: its button on the mini-games' menu, which the main menu opens first
		if (mMiniGameMenu == nullptr) {
			if (aMenuReady && aTries == 0) {
				if (mProfile == nullptr || !mProfile->KeyExists("UnlockedMiniGames") || !mProfile->GetBool("UnlockedMiniGames"))
					return Error("the mini-games are locked");
				if (!mMainMenu->ActivateItem(MainMenu::ITEM_MINI_GAMES))
					return Error("the menu's item cannot be clicked");
			}
			return Later("not at the mini-games' menu");
		}
		if (aCurtainBusy)
			return Later("the curtain is moving");
		for (size_t i = 0; i < mMiniGameMenu->mMiniGames.size(); i++)
			if (Squash(mMiniGameMenu->mMiniGames[i].mClass) == aWhere) {
				if (i >= mMiniGameMenu->mButtons.size() || mMiniGameMenu->mButtons[i]->mDisabled)
					return Error("its button is disabled");
				mMiniGameMenu->ButtonDepress((int)i);
				return Ok();
			}
		return Error("no such screen or mini-game");
	}

	if (aCommand == "unlock") {
		if (Squash(anArgs) != "all")
			return Error("only \"unlock all\"");
		if (mProfile == nullptr)
			return Error("no player");
		// as BookManager's end of a book sets them (bookmanager.lua)
		std::string anError;
		if (!RunLua("profile.Set('UnlockedArena', true) profile.Set('NeedsArenaDialog', true) "
		            "profile.Set('UnlockedMiniGames', true) profile.Set('NeedsMiniGamesDialog', true) "
		            "profile.Set('UnlockedBestiary', true) "
		            "if not profile.KeyExists('NeedsBestiaryDialog') then profile.Set('NeedsBestiaryDialog', true) end",
		            anError))
			return Error(anError);
		if (mMainMenu != nullptr)
			mMainMenu->RefreshUnlocks();
		return Ok();
	}

	if (aCommand == "set" || aCommand == "win" || aCommand == "lose") {
		if (!aBattle || gBattleEngine == nullptr)
			return Error("not in a battle");
		std::string aChunk;
		if (aCommand == "win") {
			// the scripts' cheat does nothing unless the battle waits for a word
			if (BoardBusyForTests())
				return Later("the battle is busy");
			aChunk = "if gBattleEngine.mEnemyPtr == nil or gBattleEngine.mEnemyPtr.mHealth <= 0 then return 'no enemy' end "
			         "_KillEnemy() return ''";
		} else if (aCommand == "lose")
			aChunk = "_lex_near_death() return ''";  // the battle decides a death after the enemy's attack
		else {
			char aName[32] = "";
			int aValue = 0;
			if (sscanf(anArgs.c_str(), "%31s %d", aName, &aValue) != 2)
				return Error("set health|enemy <n>");
			std::string aKey = aName;
			if (aKey == "health")
				aChunk = StrFormat("_LexSetHealth(%d) return ''", aValue);
			else if (aKey == "enemy" || aKey == "enemyhealth")
				aChunk = StrFormat("if gBattleEngine.mEnemyPtr == nil then return 'no enemy' end _EnemySetHealth(%d) return ''",
				                   aValue);
			else
				return Error("only health and enemy");
		}
		std::string aResult;
		if (!RunLua(aChunk, aResult))
			return Error(aResult);
		return aResult.empty() ? Ok() : Error(aResult);
	}

	if (aCommand == "level")
		return Error("not yet: the map's chapters are the scripts' (goto adventure, then click a chapter)");
	return Error("the game's seed is -seed=<n> when it starts");  // seed
}

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

// A click or a move the test hooks make, logged so that the run's input can be replayed in the original (tools/identical/toorig.py)
void GameApp::AddInput(int theUpdate, const std::string& theCommand, const std::string& theArgs)
{
	SDL_Log("script: input %d %s %s", theUpdate, theCommand.c_str(), theArgs.c_str());
	AddScriptCommand(theUpdate, theCommand, theArgs);
}

bool GameApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (HandleDriverCommand(theCommand))
		return true;
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
	if (theCommand.mCommand == "statedump") {
		// the battle's state: the engine's, the board's flags and its selected tiles
		SDL_Log("script: state: engine %d, board %p%s", gBattleEngine != nullptr ? gBattleEngine->mState : -9, (void*)mBoard,
		        mBoard != nullptr && mBoard->mVisible ? " visible" : "");
		if (mBoard != nullptr) {
			SDL_Log("script: state: paused %d, tiles %d, selected %d, new %d, ending %d", (int)mBoard->IsPaused(),
			        (int)mBoard->mTiles.size(), (int)mBoard->mSelected.size(), (int)mBoard->mNewTiles.size(),
			        (int)mBoard->mEndingChapter);
			if (FadeButton* aBtn = mBoard->mButtons[Board::BUTTON_ATTACK])
				SDL_Log("script: state: attack button at %d,%d %dx%d visible %d disabled %d image %p cols %d", aBtn->mX, aBtn->mY, aBtn->mWidth, aBtn->mHeight,
				        (int)aBtn->mVisible, (int)aBtn->mDisabled, (void*)aBtn->mButtonImage, aBtn->mButtonImage != nullptr ? aBtn->mButtonImage->mNumCols : 0);
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
				AddInput(mUpdateCount + 1, "click", aClick);
		}
		else if (mBoard == nullptr || !mBoard->mVisible) {
			if (aWordTurn)
				SDL_Log("script: soak %d: board hidden: click Enter", mUpdateCount);
			AddInput(mUpdateCount + 1, "click", "400 575");  // the chapter map's Enter button
		}
		else if (!mBoard->mIsMiniGame && gBattleEngine != nullptr && gBattleEngine->mState == BattleEngine::BE_IDLE &&
		         gBattleEngine->LostTurn() && gBattleEngine->CallLinkedMethod("CanEndLostTurnRound", 0).Bool() &&
		         gBattleEngine->CallLinkedMethod("AttackerIsPlayer", 0).Bool()) {
			// stunned: the panel on the grid ends the lost turn when clicked (Board::MouseDown)
			SDL_Log("script: soak %d: lost turn: click the grid", mUpdateCount);
			AddInput(mUpdateCount + 1, "click", "400 420");
		}
		else if (ShowingConversationPanel() || !aWordTurn)
			AddInput(mUpdateCount + 1, "click", "400 290");
		else if (mBoard != nullptr && !sSoakNoWords) {
			if (mProfile != nullptr && mProfile->GetBool("TutorialNeedsPower")) {
				// the first battle's potion tutorial locks the grid until a potion is used
				AddInput(mUpdateCount + 1, "click", "68 330");
				AddInput(mUpdateCount + 6, "click", "143 330");
				AddInput(mUpdateCount + 11, "click", "225 330");
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
			AddInput(mUpdateCount + 1, "click", "400 575");  // the chapter map's Enter button
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
	if (theCommand.mCommand == "pams") {
		// every animation the scripts hold: id, playing, label, frame of frames, rate, callback
		if (mPopAnimManager != nullptr)
			for (const auto& aPair : mPopAnimManager->mAnims) {
				PopAnim* anAnim = aPair.second.mAnim;
				if (anAnim == nullptr || anAnim->mRoot == nullptr || anAnim->mData == nullptr)
					continue;
				SDL_Log("script: pams %d: playing %d label %s frame %d of %d rate %d cb %s", aPair.first, (int)anAnim->mPlaying,
				        anAnim->mLabel.c_str(), anAnim->mRoot->mFrameNum, (int)anAnim->mData->mMain.mFrames.size(), anAnim->mFrameRate,
				        aPair.second.mCallback.c_str());
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
			AddInput(mUpdateCount + 1, "click", aClick);
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
		AddInput(aWhen, "click", StrFormat("%d %d", aTile->mX + 25, aTile->mY + 25));
		aWhen += 8;
	}
	if (theCommand.mArgs.find("attack") != std::string::npos) {
		// the button lights up for a mouse that moved over it, before the click
		Widget* anAttack = mBoard->mButtons[Board::BUTTON_ATTACK];
		std::string aCentre = StrFormat("%d %d", mBoard->mX + anAttack->mX + anAttack->mWidth / 2,
		                                mBoard->mY + anAttack->mY + anAttack->mHeight / 2);
		AddInput(aWhen, "move", aCentre);
		AddInput(aWhen + 20, "click", aCentre);
	}
	return true;
}

void GameApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-statelog")
		mStateLogFileName = theParamValue;
	else if (theParamName == "-reseedengine")
		mReseedEngine = atoi(theParamValue.c_str());
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
	aValues.push_back({"rand", StrFormat("%u", GetRandCount() - mRandOffset)});
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
