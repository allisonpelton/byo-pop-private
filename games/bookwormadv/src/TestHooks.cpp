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
//
// The drivers' commands (docs/TEST_COMMANDS.md): each logs "OK <command>" or "ERROR <command>: <reason>". A command
// that waits for its screen (a curtain, the title) tries again every 10 updates, 300 times at most.
//   state           STATE {json}: screen, mode, level (book.chapter), health, maxhealth, enemy, enemyhealth, and
//                   the board: its letters row by row ("." for none), "+" after a letter that can be picked
//   goto <where>    menu (once the title has gone on to it); adventure, arena, minigames, tome, mediaplayer,
//                   options, highscores (the main menu's items); a mini-game by its class (wordmaster, letterrip,
//                   linknspell: the mini-games' menu's buttons, opened first from the main menu)
//   unlock all      the arena, the mini-games and the tome, as the end of a book unlocks them (the arena still
//                   wants the first book's sixth chapter)
//   set health|enemy <n>
//                   Lex's or the enemy's health (the scripts' _LexSetHealth, _EnemySetHealth)
//   win             the battle's enemy dies (the scripts' _KillEnemy, only while the battle waits for a word)
//   lose            Lex near death (the scripts' _lex_near_death: a quarter of a heart), so that the enemy's next
//                   attack ends the battle (the battle decides a death only then)
//   move            MOVE {json} the move the drivers' bot makes next, or MOVE null (MoveForTests): "what" (dialog, talk,
//                   tutorial, lostturn, wait, hint, overlay, word, scramble, guess, treasure, continue, enter), "word" for a word or
//                   a guess, and "input", the real input that makes it; it changes nothing
#include <SDL.h>

#include <cstdio>
#include <algorithm>
#include <cctype>
#include <list>
#include <sstream>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "ArrowHint.h"
#include "BattleEngine.h"
#include "Board.h"
#include "FadeButton.h"
#include "GameApp.h"
#include "DialogButton.h"
#include "GameDialog.h"
#include "GameText.h"
#include "Lex.h"
#include "SongPlayer.h"
#include "LuaSupport.h"
#include "MainMenu.h"
#include "MinigameMenu.h"
#include "CurtainTransition.h"
#include "Profile.h"
#include "TreasureScreen.h"
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

// port: the screen showing, for the test command state and the board log
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
// With theLongest, also the theCount longest words they spell, longest first
std::string GameApp::BestWordForTests(size_t theMax, std::vector<Tile*>& theOrder, std::vector<std::string>* theLongest,
                                      size_t theCount)
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
	std::vector<std::string> aWords;
	for (const std::string& aWord : mLex->mWords)
		if ((theLongest != nullptr || aWord.size() > aBest.size()) && aWord.size() <= aMax && aWord != anEgg &&
		    Spell(aWord, aTiles, anOrder)) {
			if (aWord.size() > aBest.size()) {
				aBest = aWord;
				aBestOrder = anOrder;
			}
			if (theLongest != nullptr)
				aWords.push_back(aWord);
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
	for (const std::string& aWord : mLex->mWords) {
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

// port: with -boardlog=<file>, a run with real input writes there, each time the battle's board comes to rest with a new
// set of tiles, the word a test driver may spell there: "board: <n> <WORD> x,y ... attack x,y scramble x,y words W1,W2,... letters L@x,y;..." (tools/realrun.py's
// "word"). Only what the driver sees: its clicks go through the real input
void GameApp::LogBoardForTests()
{
	static std::string sLast;
	static int sCount = 0;
	// a dialog: "dialog: <n> <id> ok x,y", its first button (yes, ok), once for each dialog shown
	static Dialog* sLastDialog = nullptr;
	Dialog* aDialog = mDialogMap.empty() ? nullptr : mDialogMap.rbegin()->second;
	if (aDialog != sLastDialog && aDialog != nullptr) {
		Widget* aButton = aDialog->mYesButton;
		if (GameDialog* aGameDialog = dynamic_cast<GameDialog*>(aDialog))
			aButton = aGameDialog->mButton1;
		if (aButton != nullptr && aDialog->mUpdateCnt > 30) {  // once it is in place and takes clicks
			sLastDialog = aDialog;
			if (FILE* aFile = fopen(mBoardLogForTests.c_str(), "a")) {
				Point aPos = aButton->GetAbsPos();  // (a GameDialog's buttons are not its children)
				// then its header and lines, on one line, for a reader of the log
				std::string aText = aDialog->mDialogHeader + " | " + aDialog->mDialogLines;
				std::replace(aText.begin(), aText.end(), '\n', ' ');
				std::replace(aText.begin(), aText.end(), '\r', ' ');
				fprintf(aFile, "dialog: %d %d ok %d,%d | %s\n", ++sCount, aDialog->mId, aPos.mX + aButton->mWidth / 2,
				        aPos.mY + aButton->mHeight / 2, aText.c_str());
				fclose(aFile);
			}
		}
	}
	if (aDialog == nullptr)
		sLastDialog = nullptr;
	// the screen, when it changes: "screen: <n> <name>"; a conversation's panel, as it comes and goes: "talk: <n> 1|0"
	static std::string sLastScreen;
	static int sLastTalk = -1;
	std::string aScreen = ScreenForTests();
	int aTalk = ShowingConversationPanel() ? 1 : 0;
	if (aScreen != sLastScreen || aTalk != sLastTalk) {
		if (FILE* aFile = fopen(mBoardLogForTests.c_str(), "a")) {
			if (aScreen != sLastScreen)
				fprintf(aFile, "screen: %d %s\n", ++sCount, aScreen.c_str());
			if (aTalk != sLastTalk)
				fprintf(aFile, "talk: %d %d\n", ++sCount, aTalk);
			fclose(aFile);
		}
		sLastScreen = aScreen;
		sLastTalk = aTalk;
	}
	// the treasure room's Continue, as it shows and as it is enabled: "treasure: <n> continue x,y 1|0"
	static std::string sLastTreasure;
	std::string aTreasure;
	if (aScreen == "treasure") {
		auto anIt = mTreasureScreen->mButtons.find(TreasureScreen::BUTTON_CONTINUE);
		if (anIt != mTreasureScreen->mButtons.end() && anIt->second != nullptr && anIt->second->mVisible) {
			Point aPos = anIt->second->GetAbsPos();
			aTreasure = StrFormat("continue %d,%d %d", aPos.mX + anIt->second->mWidth / 2,
			                      aPos.mY + anIt->second->mHeight / 2, anIt->second->mDisabled ? 0 : 1);
		}
	}
	if (aTreasure != sLastTreasure) {
		sLastTreasure = aTreasure;
		if (!aTreasure.empty())
			if (FILE* aFile = fopen(mBoardLogForTests.c_str(), "a")) {
				fprintf(aFile, "treasure: %d %s\n", ++sCount, aTreasure.c_str());
				fclose(aFile);
			}
	}
	// the tutorial's arrows that wait for a click on a hot spot (a tile, a button): "hint: <n> <id> x,y ..." (each hot
	// spot's centre), "hint: <n> -" once none is up
	static std::string sLastHint;
	std::string aHint;
	if (mArrowHintManager != nullptr)
		for (ArrowHint* anArrow : mArrowHintManager->mHints)
			if (anArrow->mActive && !anArrow->mHotSpots.empty()) {
				aHint += StrFormat(" %d", anArrow->mId);
				for (const Rect& aSpot : anArrow->mHotSpots)
					aHint += StrFormat(" %d,%d", aSpot.mX + aSpot.mWidth / 2, aSpot.mY + aSpot.mHeight / 2);
			}
	if (aHint != sLastHint) {
		if (FILE* aFile = fopen(mBoardLogForTests.c_str(), "a")) {
			fprintf(aFile, "hint: %d%s\n", ++sCount, aHint.empty() ? " -" : aHint.c_str());
			fclose(aFile);
		}
		sLastHint = aHint;
	}
	// Word Master: a guess that fits every row it has answered, from the word list, with the keyboard's keys to type it
	// and Submit: "wordmaster: <n> <GUESS|-> x,y ... submit x,y". Only what the rows show: their letters and colours
	if (mBoard != nullptr && mLex != nullptr && mBoard->IsMiniGame() && mMiniGameName == "WordMaster") {
		static std::string sLastRows;
		std::string aRows;
		std::vector<std::string> aRow = WordMasterRowsForTests(aRows);
		if (aRows.empty())
			return;
		if (aRows == sLastRows || BoardBusyForTests())
			return;
		sLastRows = aRows;
		// only at the start of a guess: a new row (or a new word), or the row back as it began (a guess turned down).
		// The game fills in the letters a row starts with: the given first one and those already found
		static std::string sStartLine, sStartRow;
		static bool sTyped = false;
		if (aRow.empty())
			return;
		std::string aLineNum = aRows.substr(0, aRows.find('|'));
		std::string aKey = aLineNum + aRow.front().substr(0, 1);  // the line, and the word by its given letter
		if (aKey != sStartLine) {
			sStartLine = aKey;
			sStartRow = aRow.back();
			sTyped = false;
		} else if (aRow.back() != sStartRow) {
			sTyped = true;
			return;
		} else if (!sTyped) {
			return;
		} else {
			sTyped = false;
		}
		std::string aLine = WordMasterGuessForTests(aRow);
		if (FILE* aFile = fopen(mBoardLogForTests.c_str(), "a")) {
			fprintf(aFile, "wordmaster: %d %s rows %s\n", ++sCount, aLine.c_str(), aRows.c_str());
			fclose(aFile);
		}
		return;
	}
	// the first battle's tutorial (the scripts' IntroTutorial) lets one tile be picked at a time, then Attack: "tutorial:
	// <n> <its state> x,y" (that tile) or "tutorial: <n> <its state> attack x,y"
	static std::string sLastTutorial;
	std::string aTutorial, aTutorialState;
	if (mBoard != nullptr && mBoard->mVisible && !mBoard->IsMiniGame() && mDialogMap.empty() &&
	    !ShowingConversationPanel() &&
	    RunLua("if IntroTutorial == nil or IntroTutorial.mDone or IntroTutorial.mState == nil then return '' end "
	           "if IntroTutorial.mState < 0 then return '' end return tostring(IntroTutorial.mState)",
	           aTutorialState) &&
	    !aTutorialState.empty()) {
		bool aMoving = false;
		Tile* aNext = nullptr;
		int aCount = 0;
		for (Tile* aTile : mBoard->mTiles) {
			aMoving = aMoving || aTile->IsMoving() || aTile->IsFalling() || aTile->mAlphaState != 1;
			if (!aTile->mSelected && aTile->CanSelect(aTile->mX + 25, aTile->mY + 25)) {
				aNext = aTile;
				aCount++;
			}
		}
		auto anAttack = mBoard->mButtons.find(Board::BUTTON_ATTACK);
		if (!aMoving && aCount == 1)
			aTutorial = StrFormat("%s %d,%d %d", aTutorialState.c_str(), aNext->mX + 25, aNext->mY + 25,
			                      (int)mBoard->mSelected.size());
		else if (!aMoving && aCount == 0 && !mBoard->mSelected.empty() && anAttack != mBoard->mButtons.end() &&
		         anAttack->second != nullptr)
			aTutorial = StrFormat("%s attack %d,%d", aTutorialState.c_str(),
			                      mBoard->mX + anAttack->second->mX + anAttack->second->mWidth / 2,
			                      mBoard->mY + anAttack->second->mY + anAttack->second->mHeight / 2);
		else if (!aMoving)
			aTutorial = aTutorialState + " -";  // nothing to click on the board: the state alone
	}
	if (aTutorial != sLastTutorial) {
		if (!aTutorial.empty())
			if (FILE* aFile = fopen(mBoardLogForTests.c_str(), "a")) {
				fprintf(aFile, "tutorial: %d %s\n", ++sCount, aTutorial.c_str());
				fclose(aFile);
			}
		sLastTutorial = aTutorial;
	}
	// the tiles picked so far, as they change: "select: <n> <LETTERS|->"
	static std::string sLastSelect;
	std::string aSelect;
	if (mBoard != nullptr && mBoard->mVisible)
		aSelect = mBoard->mSelected.empty() ? std::string("-") : Board::GetWord(mBoard->mSelected);
	if (aSelect != sLastSelect) {
		if (!aSelect.empty())
			if (FILE* aFile = fopen(mBoardLogForTests.c_str(), "a")) {
				fprintf(aFile, "select: %d %s\n", ++sCount, aSelect.c_str());
				fclose(aFile);
			}
		sLastSelect = aSelect;
	}
	// a lost turn (Lex stunned): a click on the grid ends its round once the game takes one (Board::MouseDown's test):
	// "lostturn: <n> x,y", once a round
	static int sLostTurnFor = 0;  // updates the lost turn has lasted: it is logged once it holds (a turn changing shows it for a moment)
	bool aLostTurn = mBoard != nullptr && mBoard->mVisible && !mBoard->IsMiniGame() && gBattleEngine != nullptr &&
	                 mDialogMap.empty() && !mBoard->IsPaused() && gBattleEngine->LostTurn() &&
	                 gBattleEngine->mState == BattleEngine::BE_IDLE &&
	                 gBattleEngine->CallLinkedMethod("CanEndLostTurnRound", 0).Bool() &&
	                 gBattleEngine->CallLinkedMethod("AttackerIsPlayer", 0).Bool();
	if (aLostTurn && sLostTurnFor == 30 && !mBoard->mTiles.empty()) {
		int aX = 0, aY = 0;
		for (Tile* aTile : mBoard->mTiles) {
			aX += aTile->mX + 25;
			aY += aTile->mY + 25;
		}
		if (FILE* aFile = fopen(mBoardLogForTests.c_str(), "a")) {
			fprintf(aFile, "lostturn: %d %d,%d\n", ++sCount, aX / (int)mBoard->mTiles.size(),
			        aY / (int)mBoard->mTiles.size());
			fclose(aFile);
		}
	}
	sLostTurnFor = aLostTurn ? sLostTurnFor + 1 : 0;
	if (mBoard == nullptr || mLex == nullptr || BoardBusyForTests()) {
		sLast.clear();
		return;
	}
	// the word list is searched only when the tiles changed
	// (and none while no tile can be picked: the enemy's turn)
	std::string aKey;
	bool aPickable = false;
	for (Tile* aTile : mBoard->mTiles) {
		bool aCanSelect = aTile->CanSelect(aTile->mX + 25, aTile->mY + 25);
		aPickable = aPickable || aCanSelect;
		aKey += StrFormat("%s%d,%d%c;", Board::GetWord(std::list<Tile*>(1, aTile)).c_str(), aTile->mX, aTile->mY,
		                  aCanSelect ? '+' : '-');
	}
	if (!aPickable) {
		sLast.clear();
		return;
	}
	if (aKey == sLast)
		return;
	sLast = aKey;
	std::vector<Tile*> anOrder;
	std::vector<std::string> aLongest;
	std::string aWord = BestWordForTests(std::string::npos, anOrder, &aLongest, 40);
	std::string aLine = aWord.empty() ? "-" : aWord;
	for (Tile* aTile : anOrder)
		aLine += StrFormat(" %d,%d", aTile->mX + 25, aTile->mY + 25);
	// a mini-game's board has no Scramble (find: the map's [] would add an empty one, which the board then draws)
	for (int aButton : {(int)Board::BUTTON_ATTACK, (int)Board::BUTTON_SCRAMBLE}) {
		auto anIt = mBoard->mButtons.find(aButton);
		if (anIt == mBoard->mButtons.end() || anIt->second == nullptr)
			aLine += aButton == Board::BUTTON_ATTACK ? " attack -" : " scramble -";
		else
			aLine += StrFormat(aButton == Board::BUTTON_ATTACK ? " attack %d,%d" : " scramble %d,%d",
			                   mBoard->mX + anIt->second->mX + anIt->second->mWidth / 2,
			                   mBoard->mY + anIt->second->mY + anIt->second->mHeight / 2);
	}
	// the next longest words and the letters that can be picked, for a driver that wants another word (Letter Rip)
	aLine += " words ";
	for (size_t i = 0; i < aLongest.size(); i++)
		aLine += (i > 0 ? "," : "") + aLongest[i];
	if (aLongest.empty())
		aLine += "-";
	aLine += " letters ";
	bool aFirst = true;
	for (Tile* aTile : mBoard->mTiles)
		if (!aTile->mSelected && aTile->CanSelect(aTile->mX + 25, aTile->mY + 25)) {
			aLine += StrFormat("%s%s@%d,%d", aFirst ? "" : ";", Board::GetWord(std::list<Tile*>(1, aTile)).c_str(),
			                   aTile->mX + 25, aTile->mY + 25);
			aFirst = false;
		}
	if (aFirst)
		aLine += "-";
	if (FILE* aFile = fopen(mBoardLogForTests.c_str(), "a")) {
		fprintf(aFile, "board: %d %s\n", ++sCount, aLine.c_str());
		fclose(aFile);
	}
}

// port: the move the drivers' bot makes next (the standard "move" command, docs/TEST_COMMANDS.md), as JSON with its real
// input, or "null" while there is none (the board busy, a curtain moving). The same choices the board log offers a
// driver: a dialog's first button, a conversation, the tutorial's tile or Attack, a lost turn's grid, an arrow's hot
// spot, then the board's longest word with Attack (or Scramble), Word Master's guess, the treasure room's treasures and
// Continue, the map's Enter. It reads the game and changes nothing
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
		for (std::string aToken; aStream >> aToken;) {
			int x = 0, y = 0;
			if (sscanf(aToken.c_str(), "%d,%d", &x, &y) != 2)
				continue;
			if (!anInput.empty())
				anInput += ",{\"wait\":8},";
			anInput += Click(x, y);
		}
		return Move("guess", anInput, ",\"word\":" + JsonString(aGuess));
	}
	// a board (a battle's, a mini-game's): its longest word, then Attack; Scramble when it has none
	if (mBoard != nullptr && mBoard->mVisible && mLex != nullptr) {
		if (BoardBusyForTests())
			return "null";
		bool aPickable = false;
		for (Tile* aTile : mBoard->mTiles)
			aPickable = aPickable || aTile->CanSelect(aTile->mX + 25, aTile->mY + 25);
		if (!aPickable)
			return "null";  // the enemy's turn
		std::vector<Tile*> anOrder;
		std::string aWord = BestWordForTests(std::string::npos, anOrder);
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
	// the treasure room: Continue once it is enabled, else the next of Lex's treasures (the left column)
	if (aScreen == "treasure") {
		auto anIt = mTreasureScreen->mButtons.find(TreasureScreen::BUTTON_CONTINUE);
		if (anIt == mTreasureScreen->mButtons.end() || anIt->second == nullptr || !anIt->second->mVisible)
			return "null";
		if (!anIt->second->mDisabled) {
			Point aPos = anIt->second->GetAbsPos();
			return Move("continue", Click(aPos.mX + anIt->second->mWidth / 2, aPos.mY + anIt->second->mHeight / 2));
		}
		std::string aCount;
		RunLua("local n = 0 if gTreasureScreen ~= nil and gTreasureScreen.mSelectedTreasures ~= nil then "
		       "for _ in pairs(gTreasureScreen.mSelectedTreasures) do n = n + 1 end end return tostring(n)",
		       aCount);
		static const int kSlots[] = {113, 209, 300, 402};
		return Move("treasure", Click(188, kSlots[atoi(aCount.c_str()) % 4]));
	}
	if (aScreen == "map")
		return Move("enter", Click(400, 575));
	return "null";
}

// port: a test command tried again in 10 updates. In a -script run it goes into the script, as before; on the live
// channel (-commands, the web page's call) into a queue of its own, since a command in the script would make the
// framework drop every real event from then on (SexyAppBase::HandleEvent)
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
			aJson += StrFormat(",\"mode\":\"adventure\",\"level\":\"%d.%d\"", mProfile->GetInt("CurrentBook"),
			                   mProfile->GetInt("CurrentChapter"));
		aJson += StrFormat(",\"curtain\":%s,\"dialogs\":%d,\"talk\":%s", aCurtainBusy ? "true" : "false",
		                   (int)mDialogMap.size(), ShowingConversationPanel() ? "true" : "false");
		std::string aValue;
		if (aBattle && RunLua("local e = gBattleEngine local p, n = e.mPlayerPtr, e.mEnemyPtr "
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
		    {"mediaplayer", MainMenu::ITEM_FILM_REEL}, {"options", MainMenu::ITEM_OPTIONS},
		    {"highscores", MainMenu::ITEM_HALL_OF_FAME}};
		bool aMenuReady = mMainMenu != nullptr && mTitleScreen == nullptr && !aCurtainBusy && mDialogMap.empty() &&
		                  mBookScreen == nullptr && mHallOfFame == nullptr && mMiniGameMenu == nullptr && mBoard == nullptr &&
		                  !mMainMenu->mDisabled;
		if (aWhere == "menu")
			return aMenuReady ? Ok() : Later("not at the main menu");
		for (const auto& anItem : kItems)
			if (aWhere == anItem.first) {
				if (!aMenuReady)
					return Later("not at the main menu");
				mMainMenu->ActivateItem(anItem.second);
				return Ok();
			}
		// a mini-game: its button on the mini-games' menu, which the main menu opens first
		if (mMiniGameMenu == nullptr) {
			if (aMenuReady && aTries == 0) {
				if (mProfile == nullptr || !mProfile->KeyExists("UnlockedMiniGames") || !mProfile->GetBool("UnlockedMiniGames"))
					return Error("the mini-games are locked");
				mMainMenu->ActivateItem(MainMenu::ITEM_MINI_GAMES);
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
		if (!aBattle)
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
			RetryForTests("word", StrFormat("%s try=%d", theCommand.mArgs.substr(0, aPos).c_str(), aTries + 1));
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
