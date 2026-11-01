#include "Board.h"

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iterator>
#include <set>

#include "BattleEngine.h"
#include "BookManager.h"
#include "CurtainTransition.h"
#include "CurvedValue.h"
#include "FadeButton.h"
#include "Font.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Lex.h"
#include "LuaBind.h"
#include "LuaInterface.h"
#include "LuaWidgetTpl.h"
#include "Profile.h"
#include "Res.h"
#include "SexyAppBase.h"
#include "Tile.h"
#include "TileEngine.h"
#include "TooltipWidget.h"
#include "WidgetManager.h"
#include "lauxlib.h"
#include "lua.h"

using namespace Sexy;
using namespace LuaLib;

Board* Sexy::gBoard = nullptr;

static lua_State* LS()
{
	return gLuaSupport->L;
}

// a method of the battle engine's Lua object, with no arguments
static LuaClassRet BattleCall(const char* theMethod)
{
	return gBattleEngine->CallLinkedMethod(theMethod, 0);
}

// ---- the AstroPop flash ----

void Board::Flash::Start()
{
	mActive = false;
	mDone = false;
	mAlphaDelta = 16;
	mAlpha = 0;
	mY = gSexyAppBase->mHeight;
	mHue = 0;
}

void Board::Flash::Update()
{
	if (mDone || !mActive)
		return;
	mY -= 4;
	mHue = (mHue + 4) & 0xff;
	mAlpha += mAlphaDelta;
	if (mAlpha >= 256 || mAlpha < 0) {
		mAlpha = mAlpha >= 256 ? 255 : 0;
		mAlphaDelta = -mAlphaDelta;
	}
	// done once the ships and their jets are above the screen
	if (mY <= -(IMAGE_GS_SHIP1_WARPJET->GetHeight() + IMAGE_GS_SHIP1->GetHeight())) {
		mDone = true;
		mActive = false;
	}
}

void Board::Flash::Draw(Graphics* g)
{
	if (mDone || !mActive)
		return;
	// the screen in a colour that cycles, then the four ships of AstroPop flying up with their jets
	Color aColor((int)(gApp->HSLToRGB(mHue, 255, 128) | 0xff000000));
	aColor.mAlpha = 128;
	g->SetColor(aColor);
	g->FillRect(0, 0, gApp->mWidth, gApp->mHeight);
	if (gBoard->mZooming)
		return;
	g->DrawImage(IMAGE_GS_GO, (gApp->mWidth - IMAGE_GS_GO->GetWidth()) / 2,
	             (gApp->mHeight - IMAGE_GS_GO->GetHeight()) / 2);
	static const int kShipX[4] = {243, 343, 443, 543};
	Image* const aShips[4] = {IMAGE_GS_SHIP1, IMAGE_GS_SHIP2, IMAGE_GS_SHIP3, IMAGE_GS_SHIP4};
	for (int i = 0; i < 4; i++)
		g->DrawImageCel(aShips[i], kShipX[i], mY, 2);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (int i = 0; i < 4; i++)
		g->DrawImageCel(IMAGE_GS_SHIP1_WARPJET, kShipX[i] - 5, mY - 55, 8);
	g->SetColorizeImages(true);
	g->SetColor(Color(mAlpha, mAlpha, mAlpha));
	for (int i = 0; i < 4; i++)
		g->DrawImageCel(IMAGE_GS_SHIP1_WARPJET, kShipX[i] - 5, mY - 55, 8);
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// ---- construction ----

Board::Board(GameApp* theApp, bool theIsMiniGame) : mIsMiniGame(theIsMiniGame), mApp(theApp)
{
	// port: the original sets gBoard before its members are made
	RegisterLua();
	gBoard = this;
	mFlash = new Flash();
	mFlash->mY = gSexyAppBase->mHeight;
	if (!theIsMiniGame)
		new BattleEngine(this, theApp);  // gBattleEngine
	else
		gBattleEngine = nullptr;
	delete gTileEngine;
	gTileEngine = nullptr;
	new TileEngine();  // gTileEngine
	CreateButtons();
	mGridCells.assign(mGridHeight, std::vector<bool>(mGridWidth, false));
	mTooltip = new TooltipWidget();
}

Board::~Board()
{
	CallLinkedMethod("Destructor", 0);
	delete mFlash;
	for (int i = 0; i < mMuteCount; i++)
		mApp->Unmute(true);
	RemoveAllWidgets(true, true);
	mButtons.clear();
	std::set<Tile*> aTiles(mTiles.begin(), mTiles.end());
	aTiles.insert(mNewTiles.begin(), mNewTiles.end());
	aTiles.insert(mClones.begin(), mClones.end());
	for (Tile* aTile : aTiles)
		gTileEngine->DeleteTile(aTile, true, true);
	mTiles.clear();
	mNewTiles.clear();
	mClones.clear();
	delete gBattleEngine;  // sets gBattleEngine to null
	gTileEngine->mBoard = nullptr;
	gBattleEngine = nullptr;
	delete mTooltip;
	gBoard = nullptr;
	for (const char* aName : {"gBoard", "gLuaBoard", "gBattleEngine"}) {
		lua_pushstring(LS(), aName);
		lua_pushnil(LS());
		lua_settable(LS(), LUA_GLOBALSINDEX);
	}
}

static void SetGlobalInt(const char* theName, int theValue)
{
	lua_pushstring(LS(), theName);
	lua_pushint(LS(), theValue);
	lua_settable(LS(), LUA_GLOBALSINDEX);
}

void Board::Init()
{
	mGridWidth = 4;
	mGridHeight = 4;
	SetGlobalInt("gTilesWide", mGridWidth);
	SetGlobalInt("gTilesHigh", mGridHeight);
	mIsMiniGame = false;
	mGridCells.assign(mGridHeight, std::vector<bool>(mGridWidth, false));

	gTileEngine->Init(this);
	gTileEngine->CreateLinkedObject(gTileEngine);
	gTileEngine->CallLinkedMethod("Setup", 0);
	gBattleEngine->Init(this);
	gBattleEngine->CreateLinkedObject(gBattleEngine);
	gBattleEngine->CallLinkedMethod("Setup", 0);
	mStarting = true;

	std::string aDistro;
	if (mApp->mBookManager != nullptr)
		aDistro = mApp->mBookManager->CallLinkedMethod("GetInitialChapterLetterDistro", 0).String();

	// the first game's grid: the tutorial's letters
	static const unsigned int kFirstGrid[16] = {
	    GTA(0x7aa92c), GTA(0x7aa928), GTA(0x7aa924), GTA(0x7aa920), GTA(0x7aa91c), GTA(0x7aa928),
	    GTA(0x7aa918), GTA(0x7aa914), GTA(0x7aa910), GTA(0x7aa90c), GTA(0x7aa908), GTA(0x7aa904),
	    GTA(0x7aa900), GTA(0x7aa8fc), GTA(0x7aa8f8), GTA(0x7aa92c)};
	for (int aRow = 0; aRow < mGridHeight; aRow++) {
		for (int aCol = 0; aCol < mGridWidth; aCol++) {
			Tile* aTile = AddTile(gTileEngine->CreateTile(false, true, true), aCol, aRow);
			if (!mApp->HasPlayedGameBefore())
				aTile->SetLetter(GT(kFirstGrid[aCol + aRow * 4]));
			else if (!aDistro.empty())
				aTile->SetLetter(StrFormat("%c", aDistro[mGridWidth * aRow + aCol]));
		}
	}

	mRestoringGrid = true;
	if (!aDistro.empty()) {
		lua_pushboolean(LS(), true);
		gTileEngine->CallLinkedMethod("SaveGridState", 1);
	} else {
		// (the original asks first whether a game is in progress, and restores the grid either way)
		if (mApp->mProfile->KeyExists(mApp->mTimedMode ? "TM_GridState" : "GridState"))
			gTileEngine->CallLinkedMethod("RestoreGridState", 0);
	}
	mRestoringGrid = false;
	if (!mApp->mTimedMode && mApp->mProfile->KeyExists("MiniGameGemAwards"))
		gTileEngine->CallLinkedMethod("AwardMiniGameGems", 0);
	mApp->ClearUpdateBacklog(false);
}

void Board::InitMiniGame(const std::string& theName, int x, int y, int theXGap, int theYGap, int theCols, int theRows,
                         const std::string& theLetters, bool theNoTilesWhilePaused)
{
	mGridHeight = theRows;
	mGridX = x;
	mIsMiniGame = true;
	mGridWidth = theCols;
	mGridY = y;
	mNoTilesWhilePaused = theNoTilesWhilePaused;
	SetGlobalInt("gTilesWide", theCols);
	SetGlobalInt("gTilesHigh", theRows);
	mGridCells.assign(mGridHeight, std::vector<bool>(mGridWidth, false));
	gTileEngine->Init(this);
	gTileEngine->CreateLinkedObject(gTileEngine);
	gTileEngine->CallLinkedMethod("Setup", 0);
	for (int aRow = 0; aRow < mGridHeight; aRow++) {
		for (int aCol = 0; aCol < mGridWidth; aCol++) {
			Tile* aTile = AddTile(gTileEngine->CreateTile(false, true, theLetters.empty()), aCol, aRow, theXGap, theYGap);
			if (!theLetters.empty())
				aTile->SetLetter(StrFormat("%c", theLetters[mGridWidth * aRow + aCol]));
		}
	}
	// 0x426230: the scripts' Board:InitMiniGame(theName) makes the mini-game (gMiniGame) and loads its resources
	lua_pushstring(LS(), theName.c_str());
	CallLinkedMethod("InitMiniGame", 1);
	mApp->ClearUpdateBacklog(false);
}

void Board::LoadScripts()
{
	gLuaInterface->DoFile("scripts\\main.lua");
	CallLua("main.LoadRequiredScripts", 0);
	CallLua("main.LoadResources", 0);
}

// ---- the grid ----

bool Board::IsGridCell(int theCol, int theRow) const
{
	if (theCol < 0 || theCol >= mGridWidth || theRow < 0 || theRow >= mGridHeight)
		return false;
	return mGridCells[theRow][theCol];
}

Tile* Board::GridGetTile(int theCol, int theRow)
{
	if (!IsGridCell(theCol, theRow))
		return nullptr;
	for (Tile* aTile : mTiles)
		if (aTile->mCol == theCol && aTile->mRow == theRow)
			return aTile;
	for (Tile* aTile : mNewTiles)
		if (aTile->mCol == theCol && aTile->mRow == theRow)
			return aTile;
	return nullptr;
}

bool Board::GetAdjacentTiles(int theCol, int theRow, std::list<Tile*>& theTiles)
{
	theTiles.clear();
	for (int aCol = theCol - 1; aCol <= theCol + 1; aCol++) {
		for (int aRow = theRow - 1; aRow <= theRow + 1; aRow++) {
			if (aCol == theCol && aRow == theRow)
				continue;
			if (Tile* aTile = GridGetTile(aCol, aRow))
				theTiles.push_back(aTile);
		}
	}
	return !theTiles.empty();
}

Tile* Board::AddTile(Tile* theTile, int theCol, int theRow, int theXGap, int theYGap)
{
	mGridCells[theRow][theCol] = true;
	theTile->mCol = theCol;
	theTile->mRow = theRow;
	int x = (theXGap + 50) * theCol + mGridX;
	int aRowsBelow = mGridHeight - theRow;
	int y = mIsMiniGame ? theRow * 50 + mGridY : mGridY - aRowsBelow * 50 - theRow * 6;
	y += theRow * theYGap;
	theTile->mX = x;
	if (!mIsMiniGame)
		theTile->TileXChanged(x);
	theTile->mY = y;
	if (!mIsMiniGame)
		theTile->TileYChanged(y);
	if (!mIsMiniGame) {
		// it falls in from above the grid, the lower rows first
		theTile->mFallDistance = aRowsBelow * 50 + theRow * 57;
		theTile->mFalling = true;
		theTile->mFallDelay = aRowsBelow * 3 - 3 + theCol * 2;
		mNewTiles.push_back(theTile);
	} else {
		mTiles.push_back(theTile);
	}
	return theTile;
}

void Board::MoveTile(Tile* theTile, int theOldCol, int theOldRow, int theNewCol, int theNewRow)
{
	mGridCells[theNewRow][theNewCol] = true;
	theTile->mCol = theNewCol;
	theTile->mRow = theNewRow;
	theTile->mFalling = true;
	theTile->mFallDistance = (theNewRow - theOldRow) * 51;
	theTile->mFallDelay = (mGridHeight - theNewRow - 1) * 3 + theNewCol * 2;
	mGridCells[theOldRow][theOldCol] = false;
}

void Board::RemoveTile(int theCol, int theRow)
{
	Tile* aTile = GridGetTile(theCol, theRow);
	mGridCells[theRow][theCol] = false;
	mTiles.remove(aTile);
	gTileEngine->RemoveTile(aTile);  // 0x4990e0
}

bool Board::IsClonedTile(Tile* theTile)
{
	for (Tile* aTile : mClones)
		if (aTile == theTile)
			return true;
	return false;
}

void Board::TileDeleted(Tile* theTile)
{
	if (theTile == mHiliteTile)
		mHiliteTile = nullptr;
	for (auto it = mClones.begin(); it != mClones.end(); ++it) {
		if (*it == theTile) {
			mClones.erase(it);
			break;
		}
	}
}

// the topmost tile of theTiles under (x, y) (0x418d20)
static Tile* FindTileAt(std::list<Tile*>& theTiles, int x, int y)
{
	for (auto it = theTiles.rbegin(); it != theTiles.rend(); ++it)
		if ((*it)->Contains(x, y))
			return *it;
	return nullptr;
}

static void TileLua(Tile* theTile, const char* theMethod, bool theArg)
{
	lua_pushboolean(LS(), theArg);
	theTile->CallLinkedMethod(theMethod, 1);
}

void Board::UpdateHilite(int x, int y)
{
	Tile* aTile = FindTileAt(mTiles, x, y);
	if (aTile == nullptr && mIsMiniGame)
		aTile = FindTileAt(mClones, x, y);
	if (aTile == nullptr) {
		if (mHiliteTile == nullptr)
			return;
		mHiliteTile->mHilited = false;
		TileLua(mHiliteTile, "Hilited", false);
		mHiliteTile = nullptr;
		mApp->SetCursor(CURSOR_POINTER);
		return;
	}
	if (aTile == mHiliteTile && (mHiliteTile == nullptr || mHiliteTile->mHilited)) {
		// the same tile: the hand while it can be picked (or unpicked)
		if ((!mIsMiniGame && mHiliteTile->mSelected) || mHiliteTile->CanSelect(x, y))
			mApp->SetCursor(CURSOR_HAND);
		return;
	}
	if (mHiliteTile != nullptr) {
		mHiliteTile->mHilited = false;
		TileLua(mHiliteTile, "Hilited", false);
	}
	mHiliteTile = aTile;
	if ((!mIsMiniGame && aTile->mSelected) || aTile->CanSelect(x, y)) {
		if (!mScrambling && mHiliteTile != nullptr) {
			mHiliteTile->Hilite(true);
			return;
		}
	}
	mApp->SetCursor(CURSOR_POINTER);
}

// ---- the word ----

std::string Board::GetWord(const std::list<Tile*>& theTiles)
{
	std::string aWord;
	for (Tile* aTile : theTiles) {
		char c = aTile->GetLetter();
		if (c == 'Q')
			aWord.append(GT(0x7aacdc));  // its tile is "Qu"
		else
			aWord.append(1, c);
	}
	return aWord;
}

bool Board::IsValidWord(const std::string& theWord, bool theNoEgg)
{
	if (theWord.size() <= 2 || !mApp->mLex->IsWord(theWord))
		return false;
	if (theWord == GT(0x7aad5c) && !mApp->mMiniGameFromMenu && !mFlash->mActive && !theNoEgg) {
		mApp->PlaySample(SOUND_SLOWDOWN);
		BattleCall("SpelledAstroPop");
		mFlash->Start();
		mFlash->mActive = true;
	}
	return true;
}

// the word's state after a change: the attack button lit for a word the enemy is not immune to (the tail of
// 0x4195f0, 0x4199a0 and 0x419840)
static int EnemyImmunity()
{
	return BattleCall("GetEnemyWordLengthImmunity").Int();
}

void Board::CheckIfWordStillValid()
{
	std::string aWord = GetWord(mSelected);
	if (!IsValidWord(aWord, false)) {
		gTileEngine->ValidWordSpelled(mSelected, false);
		SetAttackDisabled(true, false);
		mButtons[BUTTON_ATTACK]->mGlow = false;
	} else if ((int)aWord.size() > EnemyImmunity()) {
		SetAttackDisabled(false, false);
		gTileEngine->ValidWordSpelled(mSelected, true);
	} else {
		ShowImmuneAttack();
	}
}

bool Board::SelectTile(Tile* theTile)
{
	if (!theTile->CanSelect(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY) || theTile->mMoving)
		return false;
	theTile->mSelected = true;
	TileLua(theTile, "Selected", true);
	if (!mIsMiniGame)
		mSelected.push_back(theTile);
	mApp->PlaySample(SOUND_TILE_SELECT);
	LayoutWord(theTile, true);
	if (!mIsMiniGame) {
		mString130.clear();
		CheckIfWordStillValid();
	}
	return true;
}

void Board::DeselectTile(Tile* theTile)
{
	auto it = mSelected.begin();
	while (it != mSelected.end() && *it != theTile)
		++it;
	if (!mIsMiniGame) {
		// it and the tiles after it go back to their cells
		while (it != mSelected.end()) {
			Tile* aTile = *it;
			aTile->mSelected = false;
			TileLua(aTile, "Selected", false);
			lua_pushnumber(LS(), 1.0);
			aTile->CallLinkedMethod("SetScaleTarget", 1);
			aTile->mSlideToX = aTile->mCol * 50 + mGridX;
			aTile->mSlideToY = aTile->mRow * 51 + mGridY;
			aTile->mSlideFromX = aTile->mX;
			aTile->mSlideFromY = aTile->mY;
			aTile->mSlideSpeed = 0.05f;
			aTile->mSlideCurve->mT = 0;
			aTile->mSlideCurve->Step();
			gTileEngine->StartMoving(aTile, true);
			it = mSelected.erase(it);
		}
	} else {
		theTile->mSelected = false;
		TileLua(theTile, "Selected", false);
		if (theTile == mHiliteTile)
			mHiliteTile = nullptr;
	}
	LayoutWord(theTile, false);
	if (!mIsMiniGame) {
		mString130.clear();
		std::string aWord = GetWord(mSelected);
		if (!IsValidWord(aWord, false)) {
			gTileEngine->ValidWordSpelled(mSelected, false);
			SetAttackDisabled(true, false);
			mButtons[BUTTON_ATTACK]->mGlow = false;
		} else if ((int)aWord.size() > EnemyImmunity()) {
			gTileEngine->ValidWordSpelled(mSelected, true);
			SetAttackDisabled(false, false);
		} else {
			ShowImmuneAttack();
			lua_pushstring(LS(), aWord.c_str());
			gBattleEngine->CallLinkedMethod("SpelledImmuneWord", 1);
		}
	}
}

void Board::DeselectAllTiles()
{
	if (mSelected.empty())
		return;
	for (Tile* aTile : mSelected) {
		aTile->mSelected = false;
		TileLua(aTile, "Selected", false);
		aTile->mSlideToX = aTile->mCol * 50 + mGridX;
		aTile->mSlideToY = aTile->mRow * 51 + mGridY;
		aTile->mSlideFromX = aTile->mX;
		aTile->mSlideFromY = aTile->mY;
		aTile->mSlideSpeed = 0.05f;
		aTile->mSlideCurve->mT = 0;
		aTile->mSlideCurve->Step();
		gTileEngine->StartMoving(aTile, true);
		if (!mIsMiniGame) {
			lua_pushnumber(LS(), 1.0);
			aTile->CallLinkedMethod("SetScaleTarget", 1);
		}
	}
	mSelected.clear();
	mString130.clear();
	gTileEngine->ValidWordSpelled(mSelected, false);
	gTileEngine->CallLinkedMethod("DeselectedAllTiles", 0);
	if (!mIsMiniGame && !BattleCall("GridHasOverlay").Bool())
		SetAttackDisabled(true, false);
}

void Board::LayoutWord(Tile* theTile, bool theSelected)
{
	if (mIsMiniGame) {
		gLuaSupport->PushObject(ClassOf<Tile>::info, theTile);
		lua_pushboolean(LS(), theSelected);
		lua_pushboolean(LS(), IsClonedTile(theTile));
		CallLua("gMiniGame:TileSelected", 3);
		return;
	}
	// the word above the grid, centred on x 400, its tiles smaller past 8 letters
	int aCount = (int)mSelected.size();
	float aScale = aCount > 8 ? (400.0f / aCount) / 50.0f : 1.0f;
	float aTileSize = std::fabs(aScale - 1.0f) <= 0.05 ? 50.0f : aScale * 50.0f;
	float x = (float)(int)(400.0 - aCount * aTileSize * 0.5);
	int aStep;
	switch (aCount) {
	case 9: aStep = 42; break;
	case 10: aStep = 38; break;
	case 11: aStep = 34; break;
	case 12: aStep = 31; break;
	case 13: aStep = 30; break;
	case 14: aStep = 27; break;
	case 15:
	case 16: aStep = 25; break;
	default: aStep = 50; break;
	}
	int i = 0;
	for (Tile* aTile : mSelected) {
		if ((float)aTile->mX != x || (double)aTile->mY != 125.0) {
			bool aLast = theSelected && i == aCount - 1;
			float aSpeed = aCount <= 8 ? 0.05f : 0.07f;
			aTile->mSlideToX = (int)x;
			aTile->mSlideToY = 125.0;
			aTile->mSlideFromX = aTile->mX;
			aTile->mSlideFromY = aTile->mY;
			aTile->mSlideSpeed = aSpeed;
			aTile->mSlideCurve->mT = 0;
			aTile->mSlideCurve->Step();
			gTileEngine->StartMoving(aTile, aLast);
		}
		lua_pushnumber(LS(), aScale);
		aTile->CallLinkedMethod("SetScaleTarget", 1);
		i++;
		x += (float)aStep;
	}
}

// ---- the buttons ----

void Board::CreateButtons()
{
	LuaWidgetTplBase* aListener = static_cast<LuaWidgetTplBase*>(this);
	if (!mIsMiniGame) {
		FadeButton* aButton = new FadeButton(BUTTON_SCRAMBLE, aListener);
		aButton->mDrawnByOwner = true;
		aButton->SetupImage(IMAGE_BUTTON_SCRAMBLE);
		aButton->Resize(IMAGE_BUTTON_SCRAMBLE->GetCelRect(0));
		mButtons[BUTTON_SCRAMBLE] = aButton;
	}
	FadeButton* aMenu = new FadeButton(BUTTON_MENU, aListener);
	aMenu->mDrawnByOwner = true;
	aMenu->Resize(IMAGE_BUTTON_MENU->GetCelRect(0));
	aMenu->SetupImage(IMAGE_BUTTON_MENU);
	mButtons[BUTTON_MENU] = aMenu;

	FadeButton* anAttack = new FadeButton(BUTTON_ATTACK, aListener);
	anAttack->mDrawnByOwner = true;
	anAttack->mHideMouseWhenDisabled = true;
	anAttack->SetupImage(IMAGE_BUTTON_ATTACK);
	anAttack->Resize(IMAGE_BUTTON_ATTACK->GetCelRect(0));
	// the glow that plays over it while a word can attack
	anAttack->SetEffect("images/pam/fx_attackbutton.pam", -20, "EFFECT");
	anAttack->mWidgetFlagsMod.mRemoveFlags &= WIDGETFLAGS_CLIP;
	anAttack->mEffectRect = Rect(0, 0, 308, 90);
	anAttack->mEffectOff = true;
	mButtons[BUTTON_ATTACK] = anAttack;
}

void Board::LayoutButtons()
{
	for (auto& anEntry : mButtons) {
		switch (anEntry.first) {
		case BUTTON_MENU:
			anEntry.second->Layout(LAY_SetLeft | LAY_SetTop, this, 571, mIsMiniGame ? 566 : 563);
			break;
		case BUTTON_ATTACK:
			anEntry.second->Layout(LAY_SetLeft | LAY_SetTop, this, 279, 540);
			break;
		case BUTTON_SCRAMBLE:
			anEntry.second->Layout(LAY_SetLeft | LAY_SetTop, this, 64, 563);
			break;
		}
	}
}

void Board::SetAttackDisabled(bool isDisabled, bool keepLook)
{
	FadeButton* aButton = mButtons[BUTTON_ATTACK];
	bool wasDisabled = aButton->mDisabled;
	aButton->mMouseVisible = !isDisabled;
	aButton->SetDisabled(isDisabled);
	if (keepLook)
		return;
	if (!mIsMiniGame && gBattleEngine->LostTurn())
		return;
	bool wasImmune = mImmuneLook;
	mImmuneLook = false;
	int w = IMAGE_BUTTON_ATTACK->GetCelWidth();
	int h = IMAGE_BUTTON_ATTACK->GetCelHeight();
	aButton->mNormalRect = Rect(0, 0, w, h);
	aButton->mOverRect = Rect(w, 0, w, h);
	aButton->mDownRect = Rect(w * 2, 0, w, h);
	aButton->mDisabledRect = Rect(w * 3, 0, w, h);
	if ((isDisabled && !wasDisabled) || mTMEnemyAttacking) {
		aButton->mEffectOff = true;
		return;
	}
	if (!isDisabled && (wasDisabled || wasImmune))
		aButton->EnableEffect(true, true);
}

void Board::ShowImmuneAttack()
{
	if (!mIsMiniGame && gBattleEngine->LostTurn())
		return;
	// the attack button's fifth cel: the enemy is immune to the word
	mImmuneLook = true;
	FadeButton* aButton = mButtons[BUTTON_ATTACK];
	aButton->mEffectOff = true;
	if (mTMEnemyAttacking)
		return;
	aButton->SetDisabled(false);
	int w = IMAGE_BUTTON_ATTACK->GetCelWidth();
	int h = IMAGE_BUTTON_ATTACK->GetCelHeight();
	Rect aRect(w * 4, 0, w, h);
	aButton->mNormalRect = aRect;
	aButton->mOverRect = aRect;
	aButton->mDownRect = aRect;
	aButton->mDisabledRect = aRect;
}

void Board::SetAttackBtnPos(int x, int y)
{
	mButtons[BUTTON_ATTACK]->Move(x, y);
}

void Board::SetScrambleDisabled(bool isDisabled)
{
	auto it = mButtons.find(BUTTON_SCRAMBLE);
	if (it == mButtons.end())
		return;
	FadeButton* aButton = it->second;
	aButton->SetDisabled(isDisabled);
	aButton->mMouseVisible = !isDisabled;
	if (!isDisabled)
		aButton->SetVisible(true);
}

void Board::SetScrambleVisible(bool isVisible)
{
	auto it = mButtons.find(BUTTON_SCRAMBLE);
	if (it != mButtons.end())
		it->second->SetVisible(isVisible);
}

void Board::ScrambleUseNormalForDisabled(bool on)
{
	// the disabled scramble button looks enabled (its disabled cel kept to restore)
	auto it = mButtons.find(BUTTON_SCRAMBLE);
	if (it == mButtons.end())
		return;
	FadeButton* aButton = it->second;
	if (on) {
		aButton->mSavedDisabledRect = aButton->mDisabledRect;
		aButton->mDisabledRect = aButton->mNormalRect;
		aButton->mMouseVisible = false;
	} else {
		aButton->mDisabledRect = aButton->mSavedDisabledRect;
		aButton->mMouseVisible = true;
	}
}

bool Board::Scramble()
{
	if ((unsigned)(mGridHeight * mGridWidth) <= mTiles.size() && (mIsMiniGame || gBattleEngine->CanScrambleTiles())) {
		if (!mScrambling) {
			mWidgetManager->SetFocus(this);
			DeselectAllTiles();
			mScrambling = true;
			mScrambleStep = 1;
			gTileEngine->StartRound(mApp->mTimedMode);
			if (!mIsMiniGame) {
				BattleCall("Scramble");
				mApp->PlaySample(SOUND_SCRAMBLE);
			}
			return true;
		}
	}
	if (mApp->mTimedMode)
		mApp->PlaySample(SOUND_SUBMIT_FAILED);
	return false;
}

// ---- pause ----

void Board::Pause(bool thePause, bool theNoBanner)
{
	if (!thePause) {
		mPauseCount--;
		if (theNoBanner)
			mPauseNoBannerCount--;
		if (mPauseCount < 1) {
			mUnpauseUpdateCnt = mUpdateCnt;
			mPauseCount = 0;
			mPauseFade = 0;
			if (mWidgetManager != nullptr && mWidgetManager->mMouseIn)
				MouseMove(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY);
			auto it = mButtons.find(BUTTON_SCRAMBLE);
			if (it != mButtons.end() && it->second != nullptr && mScrambleWasEnabled) {
				mScrambleWasEnabled = false;
				mButtons[BUTTON_SCRAMBLE]->SetDisabled(false);
			}
			it = mButtons.find(BUTTON_MENU);
			if (it != mButtons.end() && it->second != nullptr)
				mButtons[BUTTON_MENU]->SetDisabled(false);
		}
		if (mApp->GetDialog(GameApp::DIALOG_ARENA_WIN) == nullptr) {
			mApp->Unmute(true);
			mMuteCount--;
		}
		mButtons[BUTTON_ATTACK]->SetEffectPlaying(true);
		mButtons[BUTTON_ATTACK]->mMouseVisible = true;
	} else {
		mApp->SetCursor(CURSOR_POINTER);
		mPauseCount++;
		if (theNoBanner)
			mPauseNoBannerCount++;
		if (mApp->GetDialog(GameApp::DIALOG_ARENA_WIN) == nullptr) {
			mApp->Mute(true);
			mMuteCount++;
		}
		mButtons[BUTTON_ATTACK]->SetEffectPlaying(false);
		mButtons[BUTTON_ATTACK]->mMouseVisible = false;
		auto it = mButtons.find(BUTTON_SCRAMBLE);
		if (it != mButtons.end() && it->second != nullptr) {
			mScrambleWasEnabled = mScrambleWasEnabled || !it->second->mDisabled;
			it->second->SetDisabled(true);
		}
		it = mButtons.find(BUTTON_MENU);
		if (it != mButtons.end() && it->second != nullptr)
			it->second->SetDisabled(true);
	}
	MarkDirty();
}

void Board::GotFocus()
{
	CallLinkedMethod("GotFocus", 0);
	Pause(false, false);
	mButtons[BUTTON_ATTACK]->SetEffectPlaying(true);
}

void Board::LostFocus()
{
	CallLinkedMethod("LostFocus", 0);
	Pause(true, false);
	mButtons[BUTTON_ATTACK]->SetEffectPlaying(false);
}

// ---- the widget ----

void Board::Resize(int x, int y, int w, int h)
{
	Widget::Resize(x, y, w, h);
	LayoutButtons();
}

void Board::OrderInManagerChanged()
{
	if (mApp->mArrowHintManager != nullptr)
		mWidgetManager->BringToFront(mApp->mArrowHintManager);
}

void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	LuaWidget::RemovedFromManager(theWidgetManager);
	lua_pushstring(LS(), "BattleEngine");
	lua_pushnil(LS());
	lua_settable(LS(), LUA_GLOBALSINDEX);
}

void Board::KeyDown(KeyCode theKey)
{
}

// whether the app is between a chapter's loading screen and its battle (no updates, draws or input then)
#define LOADING_CHAPTER() (mApp->mLoadingChapter && mApp->mChapterLoadScreen != nullptr)

void Board::Update()
{
	if (LOADING_CHAPTER() || gTileEngine == nullptr || mDisabled)
		return;
	if (!mApp->mHasFocus && mPauseCount <= 0)
		Pause(true, false);
	if (mPauseCount > 0 || mApp->IsCurtainMoving()) {
		if (mApp->IsCurtainMoving())
			MarkDirty();
		if (mPauseCount > 0 && mPauseFade < 140) {
			MarkDirty();
			mPauseFade = std::min(mPauseFade + 6, 140);
		}
		return;
	}
	if (mFlash->mActive)
		mFlash->Update();
	gTileEngine->DeleteAllTiles();
	if (!mIsMiniGame) {
		if (!mLevelupOverlay && !mEndingChapter)
			mApp->mBattleUpdates++;
	} else if (!mApp->mMiniGameFromMenu) {
		mApp->mMiniGameUpdates++;
	}
	LuaWidget::Update();
	if (mIsMiniGame)
		CallLua("gMiniGame:Update", 0);
	gTileEngine->Update(mTiles, true);
	gTileEngine->Update(mNewTiles, false);
	if (!mClones.empty())
		gTileEngine->Update(mClones, false);
	if (mScrambling && mUpdateCnt % mScrambleStep == 0) {
		// the letters change, slower and slower
		gTileEngine->ChangeLetters(mTiles);
		if (++mScrambleStep >= 14) {
			mScrambling = false;
			BattleCall("ScrambleComplete");
		}
	}
	if (!mNewTiles.empty()) {
		bool allIn = true;
		for (Tile* aTile : mNewTiles)
			if (aTile->mFalling)
				allIn = false;
		if (allIn) {
			for (Tile* aTile : mNewTiles)
				mTiles.push_back(aTile);
			mNewTiles.clear();
			if (mStarting && !mIsMiniGame) {
				mStarting = false;
				if (!mApp->IsCurtainMoving())
					CallLua("gBattleEngine.mPlayerPtr:ChapterInit", 0);
				else
					mApp->mChapterInitQueued = true;
			}
		}
	}
	bool aMoving = gTileEngine->mMovingCount > 0;
	// the zoom effect on a 3D card updates the battle every 8th update while it holds
	bool aZoomSkip = mZooming && mApp->Is3DMode() &&
	                 (mUpdateCnt % 8 != 0 || (mZoomingIn && mZoomHoldCount <= 0));
	if (!mIsMiniGame) {
		if (!aZoomSkip)
			BattleCall("Update");
		if (mLevelupOverlay)
			BattleCall("UpdateLevelupOverlay");
	}
	if (aMoving)
		UpdateHilite(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY);
	if (!mApp->mVSyncUpdates) {
		if (!mIsMiniGame) {
			if (!aZoomSkip) {
				lua_pushnumber(LS(), 1.0);
				gBattleEngine->CallLinkedMethod("UpdateF", 1);
			}
		} else {
			lua_pushnumber(LS(), 1.0);
			CallLua("gMiniGame:UpdateF", 1);
		}
	}
	if (mZoomingIn) {
		mZoom = (float)(mZoom + 0.03);
		if (mZoom >= 1.0f) {
			if (mZoomHoldCount == 0)
				BattleCall("ZoomInComplete");
			mZoomHoldCount++;
			mZoom = 1.0f;
			if (mZoomHoldCount >= 300) {
				mZoomingIn = false;
				MarkDirty();
				return;
			}
		}
	} else if (mZooming) {
		mZoom = (float)(mZoom - 0.04);
		if (mZoom <= 0.0f) {
			BattleCall("ZoomEffectComplete");
			DoZoom(false);
		}
	}
	MarkDirty();
}

void Board::UpdateF(float theFrac)
{
	if (mPauseCount >= 1 || LOADING_CHAPTER())
		return;
	if (mIsMiniGame) {
		lua_pushnumber(LS(), theFrac);
		CallLua("gMiniGame:UpdateF", 1);
	}
	if (gBattleEngine != nullptr) {
		if (mZooming && mApp->Is3DMode()) {
			if (mUpdateCnt % 8 != 0)
				return;
			if (mZoomingIn && mZoomHoldCount < 1)
				return;
		}
		lua_pushnumber(LS(), theFrac);
		gBattleEngine->CallLinkedMethod("UpdateF", 1);
	}
}

static void DrawLua(Graphics* g, const char* thePath)
{
	PushGraphics(g);
	CallLua(thePath, 1);
}

void Board::Draw(Graphics* g)
{
	if (LOADING_CHAPTER() || !mVisible)
		return;
	if (!mIsMiniGame) {
		// port: the zoom effect (a transform of the 3D card's, 0x41b35b-0x41b442) is not drawn
		g->DrawImage(IMAGE_COMBAT_UI_BOTTOM2, 0, mHeight - IMAGE_COMBAT_UI_BOTTOM2->GetHeight());
		if (gTileEngine->mBoard != nullptr)
			gTileEngine->Draw(mNewTiles, g, false, false);
		DrawLua(g, "gBattleEngine:DrawBottomLayer");
		DrawLua(g, "gBattleEngine:DrawCreatures");
		g->DrawImage(IMAGE_COMBAT_UI_BOTTOM1, 0,
		             mHeight - IMAGE_COMBAT_UI_BOTTOM1->GetHeight() - IMAGE_COMBAT_UI_BOTTOM2->GetHeight());
		DrawLua(g, "gBattleEngine:Draw");
		if (gTileEngine->mBoard != nullptr) {
			bool aMoving = gTileEngine->mMovingCount > 0;
			gTileEngine->Draw(mTiles, g, false, !aMoving);
			if (aMoving)
				gTileEngine->Draw(mTiles, g, true, true);
		}
		DrawLua(g, "gBattleEngine:DrawTopmostLayer");
		for (int i = 0; i < (int)mButtons.size(); i++)
			mButtons[i]->DrawFade(g);
	} else {
		DrawLua(g, "gMiniGame:DrawUnderTiles");
		if (!mNoTilesWhilePaused || mPauseCount < 1) {
			if (gTileEngine->mBoard != nullptr)
				gTileEngine->Draw(mNewTiles, g, false, false);
			if (gTileEngine->mBoard != nullptr) {
				bool aMoving = gTileEngine->mMovingCount > 0;
				gTileEngine->Draw(mTiles, g, false, !aMoving);
				if (aMoving)
					gTileEngine->Draw(mTiles, g, true, true);
			}
			if (gTileEngine->mBoard != nullptr) {
				bool aMoving = gTileEngine->mMovingCount > 0;
				gTileEngine->Draw(mClones, g, false, !aMoving);
				if (aMoving)
					gTileEngine->Draw(mClones, g, true, true);
			}
		}
		DrawLua(g, "gMiniGame:DrawAboveTiles");
		for (int i = 0; i < (int)mButtons.size(); i++)
			mButtons[i]->DrawFade(g);
	}
	if ((mPauseCount > 0 || mLevelupOverlay || HasOverlays() || mFlash->mActive) && !mApp->mQuitting)
		DeferOverlay(mApp->mDialogMap.empty() ? 10000 : 0);  // 0x41b9a8: under the dialogs while any is open
	if (mShowBeatdownMultiplier) {
		float aMultiplier = 0;
		if (!mApp->mTimedMode && mApp->mProfile->KeyExists("BeatdownMultiplier"))
			aMultiplier = mApp->mProfile->GetFloat("BeatdownMultiplier");
		g->SetFont(FONT_LARGE);
		g->SetColor(Color::White);
		g->DrawString(StrFormat(GTC(0x7aae94), (double)aMultiplier), 750, 550);
	}
}

// ---- the battle engine's questions ----

static bool BattleBool(const char* theMethod)
{
	return BattleCall(theMethod).Bool();
}

// a method of the battle engine's Lua object with theCount int arguments
static LuaClassRet BattleCallInts(const char* theMethod, int theCount, int a, int b = 0, int c = 0)
{
	int anArgs[3] = {a, b, c};
	for (int i = 0; i < theCount; i++)
		lua_pushint(LS(), anArgs[i]);
	return gBattleEngine->CallLinkedMethod(theMethod, theCount);
}

// whether (x, y) is over the grid, with a margin of 15 (0x41eba0's test)
static bool OverGrid(const Board* theBoard, int x, int y)
{
	return x >= theBoard->mGridX - 15 && x <= theBoard->mGridWidth * 50 + 15 + theBoard->mGridX &&
	       y >= theBoard->mGridY - 15 && y <= theBoard->mGridHeight * 50 + 15 + theBoard->mGridY;
}

// ---- the word ----

void Board::SubmitWord()
{
	bool anEndChapter = !mIsMiniGame && gBattleEngine->mState == BattleEngine::BE_END_CHAPTER;
	if (!mLevelupOverlay) {
		if (mImmuneLook && !mIsMiniGame) {
			// the enemy is immune to words this short
			mApp->DoOkDialog(GT(0x7ab038), StrFormat(GTC(0x7ab004), EnemyImmunity()), "", nullptr);
			BattleCall("ClickedImmuneButton");
			RehupMouse();
			return;
		}
		if (!mIsMiniGame && gBattleEngine->LostTurn()) {
			gTileEngine->StartRound(mApp->mTimedMode);
			BattleCall("EndLostTurnRound");
			if (!BattleBool("GridHasOverlay"))
				SetAttackDisabled(true, false);
			RehupMouse();
			return;
		}
	}
	if (mIsMiniGame) {
		ArrowHintsClick(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY, 1);
		CallLua("gMiniGame:SubmitPressed", 0);
	} else if (!mLevelupOverlay) {
		if (!anEndChapter && gBattleEngine->SubmitTiles(mSelected)) {
			ArrowHintsClick(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY, 1);
			mString14C = mString130;
			mString130.clear();
			mRandSeed = Rand();
			std::string aWord = GetWord(mSelected);
			// the word's tiles leave the grid
			for (auto it = mSelected.begin(); it != mSelected.end();) {
				RemoveTile((*it)->mCol, (*it)->mRow);
				it = mSelected.erase(it);
			}
			SetAttackDisabled(true, false);
			mApp->mProfile->SetBool("NeedValidIdleTip", false);
			lua_pushstring(LS(), aWord.c_str());
			gTileEngine->CallLinkedMethod("SubmitPressed", 1);
			Refill();
		} else if (gTileEngine->mMovingCount <= 0 && !mButtons[BUTTON_ATTACK]->mDisabled) {
			mApp->PlaySample(SOUND_SUBMIT_FAILED);
		}
	}
	RehupMouse();
}

void Board::Refill()
{
	if (mTiles.size() >= (size_t)(mGridHeight * mGridWidth))
		return;
	for (int aCol = 0; aCol < mGridWidth; aCol++) {
		int aGap = 0;
		for (int aRow = mGridHeight - 1; aRow >= 0; aRow--) {
			if (!IsGridCell(aCol, aRow) || !mGridCells[aRow][aCol])
				aGap++;
			else if (aGap > 0)
				MoveTile(GridGetTile(aCol, aRow), aCol, aRow, aCol, aRow + aGap);
		}
	}

	std::vector<Tile*> aNewTiles;
	char aGrid[4][4] = {};  // the original's, at most 4 x 4
	for (int aRow = 0; aRow < mGridHeight; aRow++) {
		for (int aCol = 0; aCol < mGridWidth; aCol++) {
			if (!IsGridCell(aCol, aRow) || !mGridCells[aRow][aCol]) {
				Tile* aTile = gTileEngine->CreateTile(false, true, true);
				aNewTiles.push_back(aTile);
				AddTile(aTile, aCol, aRow, 0, 0);
				// it fades in
				aTile->mAlphaState = 0;
				CurvedValue* anAlpha = aTile->mAlphaCurve;
				anAlpha->mStart = 0.0;
				anAlpha->mEnd = 255.0;
				anAlpha->mTStart = 0.0;
				anAlpha->mTEnd = 1.0;
				anAlpha->mT = 0.0;
				anAlpha->Step();
				anAlpha->mMode = CurvedValue::MODE_ONCE;
				anAlpha->mCurve = CurvedValue::CURVE_EASE_OUT;
				mApp->PlaySample(SOUND_TILE_SELECT);
			}
			if (aRow < 4 && aCol < 4) {
				Tile* aTile = GridGetTile(aCol, aRow);
				aGrid[aRow][aCol] = aTile != nullptr ? aTile->GetLetter() : ' ';
			}
		}
	}

	if (!mApp->mTimedMode && !mIsMiniGame) {
		// the letters of the puzzle word that the grid lacks go on the new tiles
		std::string aWord = BattleCall("GetCreaturePuzzleWord").String();
		if (!aWord.empty()) {
			for (int aRow = 0; aRow < 4 && !aWord.empty(); aRow++) {
				for (int aCol = 0; aCol < 4 && !aWord.empty(); aCol++) {
					size_t aPos = aWord.find(aGrid[aRow][aCol]);
					if (aPos != std::string::npos)
						aWord.erase(aPos, 1);
				}
			}
			for (size_t i = 0; i < aWord.size() && i < aNewTiles.size(); i++) {
				if (aWord.find(aNewTiles[i]->GetLetter()) == std::string::npos)
					aNewTiles[i]->SetLetter(std::string(1, aWord[i]));
			}
			lua_pushboolean(LS(), true);
			gTileEngine->CallLinkedMethod("SaveGridState", 1);
		}
	}

	Profile* aProfile = mApp->mProfile;
	if (aProfile->GetInt(mApp->mTimedMode ? "TM_CurrentChapter" : "CurrentChapter") < 4 &&
	    aProfile->GetInt(mApp->mTimedMode ? "TM_CurrentBook" : "CurrentBook") == 1) {
		// the first chapters always have an A and an E
		int anEs = 0;
		int anAs = 0;
		for (int aRow = 0; aRow < mGridHeight; aRow++) {
			for (int aCol = 0; aCol < mGridWidth; aCol++) {
				if (!IsGridCell(aCol, aRow) || !mGridCells[aRow][aCol])
					continue;
				char aLetter = GridGetTile(aCol, aRow)->GetLetter();
				if (aLetter == 'E')
					anEs++;
				else if (aLetter == 'A')
					anAs++;
			}
		}
		for (Tile* aTile : aNewTiles) {
			char aLetter = aTile->GetLetter();
			if (aLetter == 'A' || aLetter == 'E')
				continue;
			if (anEs == 0) {
				aTile->SetLetter("E");
				anEs = 1;
			} else if (anAs == 0) {
				aTile->SetLetter("A");
				anAs = 1;
			}
		}
	}

	for (Tile* aTile : aNewTiles)
		Tile::PushTile(aTile);
	CallLua("gBattleEngine:CreateBonusTiles", (int)aNewTiles.size());
}

// ---- input ----

void Board::KeyChar(SexyChar theChar)
{
	if (LOADING_CHAPTER() || mZooming || mApp->mQuitting || mApp->mCredits != nullptr || mApp->IsCurtainMoving())
		return;
	LuaWidget::KeyChar(theChar);
	if (!mWidgetManager->mKeyDown[KEYCODE_CONTROL] && mPauseCount < 1) {
		switch (theChar) {
		case '1':
		case '2':
		case '3':
			if (!mIsMiniGame) {
				// a potion; not while the enemy of the timed mode attacks
				static const char* const kHas[3] = {"PlayerHasHP", "PlayerHasAP", "PlayerHasPP"};
				int anItem = theChar - '1';
				if (mApp->mTimedMode && !gBattleEngine->CanClickItems() && BattleBool("IsEnemyTurn") &&
				    BattleBool(kHas[anItem]))
					mApp->PlaySample(SOUND_SUBMIT_FAILED);
				BattleCallInts("UseItem", 1, anItem);
			}
			break;
		case '\r':
		case '\n':
		case '4': {
			auto it = mButtons.find(BUTTON_ATTACK);
			if (it != mButtons.end() && it->second != nullptr && !it->second->mDisabled)
				SubmitWord();
			break;
		}
		}
	}
	if (theChar == ' ' && mPauseNoBannerCount == 0) {
		// pauses and unpauses, not again within 10 updates
		int aTime = mApp->mUpdateCount * 10;
		if ((unsigned)(aTime - mField10C) > 99) {
			mField10C = aTime;
			Pause(mPauseCount == 0, false);
		}
	}
}

void Board::MouseDown(int x, int y, int theClickCount)
{
	if (theClickCount != -1 && theClickCount != 1)
		return;
	if (LOADING_CHAPTER() || mZooming || mApp->mQuitting)
		return;
	if (mPauseCount != 0) {
		Pause(false, false);
		return;
	}
	if (mUnpauseUpdateCnt == mUpdateCnt || mApp->IsCurtainMoving())
		return;
	if (mApp->mArrowHintManager != nullptr)
		mApp->mArrowHintManager->MouseDown(x, y, theClickCount);
	if (mStarting)
		return;
	if (!mLevelupOverlay && !mIsMiniGame && gBattleEngine != nullptr && gBattleEngine->LostTurn() &&
	    OverGrid(this, x, y)) {
		// a click on the grid ends a lost turn's round
		if (!mApp->mTimedMode && BattleBool("CanEndLostTurnRound") && gBattleEngine->mState == BattleEngine::BE_IDLE &&
		    BattleBool("AttackerIsPlayer")) {
			gTileEngine->StartRound(false);
			BattleCall("EndLostTurnRound");
			if (!BattleBool("GridHasOverlay")) {
				SetAttackDisabled(true, false);
				mApp->SetCursor(CURSOR_POINTER);
				UpdateHilite(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY);
			}
		} else {
			BattleCall("ClickedGridWhileInvalid");
		}
	} else if (theClickCount < 0) {
		DeselectAllTiles();
	} else if (mHiliteTile != nullptr) {
		if (mHiliteTile->mSelected)
			DeselectTile(mHiliteTile);
		else if (!SelectTile(mHiliteTile) && gBattleEngine != nullptr)
			BattleCall("ClickedGridWhileInvalid");
	} else if (gBattleEngine != nullptr) {
		BattleCall("ClickedGridWhileInvalid");
	}
	if (!mIsMiniGame) {
		if (gBattleEngine->mInitialized)
			BattleCallInts("MouseDown", 3, x, y, theClickCount);
	} else {
		lua_pushint(LS(), x);
		lua_pushint(LS(), y);
		lua_pushint(LS(), theClickCount);
		CallLua("gMiniGame:MouseDown", 3);
	}
}

void Board::MouseUp(int x, int y, int theLastDownButtonId)
{
	if (mApp->IsCurtainMoving() || LOADING_CHAPTER() || mZooming || mApp->mQuitting)
		return;
	if (!mIsMiniGame && theLastDownButtonId == 1) {
		if (gBattleEngine->CanClickItems()) {
			BattleCallInts("CheckIfClickedItem", 2, x, y);
		} else if (mApp->mTimedMode && BattleBool("IsEnemyTurn")) {
			// a potion clicked while the enemy attacks
			if ((BattleBool("PlayerHasHP") && BattleCallInts("CoordOverItemCPP", 3, x, y, 0).Bool()) ||
			    (BattleBool("PlayerHasAP") && BattleCallInts("CoordOverItemCPP", 3, x, y, 1).Bool()) ||
			    (BattleBool("PlayerHasPP") && BattleCallInts("CoordOverItemCPP", 3, x, y, 2).Bool()))
				mApp->PlaySample(SOUND_SUBMIT_FAILED);
		}
	}
	LuaWidget::MouseUp(x, y, -1, theLastDownButtonId);
}

void Board::MouseMove(int x, int y)
{
	if (mPauseCount > 0 || mApp->IsCurtainMoving() || LOADING_CHAPTER() || mZooming || mApp->mQuitting)
		return;
	if (mApp->mArrowHintManager != nullptr)
		mApp->mArrowHintManager->MouseMove(x, y);
	UpdateHilite(x, y);
	if (mIsMiniGame || (gBattleEngine != nullptr && gBattleEngine->mInitialized))
		LuaWidget::MouseMove(x, y);
	if (mShowTooltips)
		mApp->UpdateTooltipHover(x, y);
	if (OverGrid(this, x, y) && !mIsMiniGame && gBattleEngine != nullptr && !mApp->mTimedMode &&
	    gBattleEngine->LostTurn()) {
		mApp->SetCursor(CURSOR_HAND);
		return;
	}
	if (mApp->mCursorNum == CURSOR_HAND && mHiliteTile == nullptr)
		mApp->SetCursor(CURSOR_POINTER);
}

// ---- the buttons ----

void Board::ButtonPress(int theId, int theClickCount)
{
	if (mUpdateCnt == mUnpauseUpdateCnt) {
		// the click that unpaused
		mFlagE9 = true;
		return;
	}
	if (mZooming)
		return;
	if (theId == BUTTON_SCRAMBLE) {
		if (mTiles.size() < (size_t)(mGridWidth * mGridHeight))
			return;
		if (!mIsMiniGame && gBattleEngine != nullptr && !gBattleEngine->CanScrambleTiles())
			return;
		if (mScrambling)
			return;
	}
	mApp->PlaySample(SOUND_CLICK2);
}

void Board::ButtonDepress(int theId)
{
	if (mApp->IsCurtainMoving() || mZooming)
		return;
	if (mFlagE9) {
		mFlagE9 = false;
		return;
	}
	if (mWidgetManager->mLastDownButtonId < 0) {
		// a right click on a button drops the word
		DeselectAllTiles();
		return;
	}
	switch (theId) {
	case BUTTON_MENU:
		if (!mLevelupOverlay && mPauseCount < 1)
			mApp->ShowGameMenu(false);
		break;
	case BUTTON_ATTACK:
		SubmitWord();
		break;
	case BUTTON_SCRAMBLE:
		if (!mLevelupOverlay && mPauseCount < 1) {
			ArrowHintsClick(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY, 1);
			Scramble();
		}
		break;
	}
}

void Board::ButtonMouseEnter(int theId)
{
	// the disabled attack button says when it works
	if (theId == BUTTON_ATTACK && mButtons[BUTTON_ATTACK]->mDisabled && mTooltip != nullptr) {
		mTooltip->SetXY(mApp->mWidgetManager->mLastMouseX, mButtons[BUTTON_ATTACK]->mY);
		mTooltip->Show();
	}
}

void Board::ButtonMouseLeave(int theId)
{
	if (theId == BUTTON_ATTACK && mButtons[BUTTON_ATTACK]->mDisabled && mTooltip != nullptr) {
		mTooltip->Hide();
	}
}

void Board::EnableAttackBtnPAM(bool on)
{
	if (on && mImmuneLook)
		return;
	FadeButton* aButton = mButtons[BUTTON_ATTACK];
	if (!aButton->mDisabled || !on)
		aButton->EnableEffect(on, on);
}

void Board::DisableAttackPAM(bool isPlaying)
{
	mButtons[BUTTON_ATTACK]->SetEffectPlaying(isPlaying);
}

void Board::DoLevelUp(bool on)
{
	mLevelupOverlay = on;
	if (on)
		DisableButtons();
	else
		RestoreButtons();
}

void Board::DisableButtons()
{
	for (auto& anEntry : mButtons) {
		FadeButton* aButton = anEntry.second;
		if (aButton == nullptr || aButton->mDisabled || !aButton->mVisible)
			continue;
		if (std::find(mOverlayDisabled.begin(), mOverlayDisabled.end(), aButton) != mOverlayDisabled.end())
			continue;
		aButton->SetDisabled(true);
		aButton->mMouseVisible = false;
		mOverlayDisabled.push_back(aButton);
	}
}

void Board::RestoreButtons()
{
	if (!mOverlayCallbacks.empty())
		return;
	for (FadeButton* aButton : mOverlayDisabled) {
		aButton->SetDisabled(false);
		aButton->mMouseVisible = true;
	}
	mOverlayDisabled.clear();
}

void Board::AddOverlayCallback(const std::string& theFunction)
{
	mOverlayCallbacks.push_back(theFunction);
	DisableButtons();
}

void Board::RemoveOverlayCallback(const std::string& theFunction)
{
	auto it = std::find(mOverlayCallbacks.begin(), mOverlayCallbacks.end(), theFunction);
	if (it != mOverlayCallbacks.end())
		mOverlayCallbacks.erase(it);
	RestoreButtons();
}

void Board::RehupMouse()
{
	int x = mApp->mWidgetManager->mLastMouseX;
	int y = mApp->mWidgetManager->mLastMouseY;
	MouseMove(x, y);
	auto aFind = [this](int theId) -> FadeButton* {
		auto it = mButtons.find(theId);
		return it != mButtons.end() ? it->second : nullptr;
	};
	FadeButton* aScramble = aFind(BUTTON_SCRAMBLE);
	FadeButton* aMenu = aFind(BUTTON_MENU);
	FadeButton* anAttack = aFind(BUTTON_ATTACK);
	if (aScramble != nullptr && !aScramble->mDisabled && aScramble->Contains(x, y))
		mApp->SetCursor(CURSOR_HAND);
	if (aMenu != nullptr && !aMenu->mDisabled && aMenu->Contains(x, y))
		mApp->SetCursor(CURSOR_HAND);
	if (anAttack != nullptr && !anAttack->mDisabled && anAttack->mVisible && anAttack->Contains(x, y))
		mApp->SetCursor(CURSOR_HAND);
}

void Board::DoZoom(bool on)
{
	if (!on) {
		for (auto& anEntry : mButtons)
			anEntry.second->mDoFinger = true;
		mZooming = false;
		mZoomingIn = false;
		ArrowHintsHide(false);
		mApp->EnableTooltips(true);
	} else if (!mZooming) {
		mZoom = 0;
		mZooming = true;
		mZoomingIn = true;
		mZoomHoldCount = 0;
		mApp->EnableTooltips(false);
		ArrowHintsHide(true);
		for (auto& anEntry : mButtons)
			anEntry.second->mDoFinger = false;
	}
}

bool Board::CursorIsOverGrid()
{
	return OverGrid(this, mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY);
}

void Board::AllowTooltips(bool on)
{
	mShowTooltips = on;
	if (!on)
		mApp->TurnOffTooltips();
}

void Board::SetAttackImage(Image* theImage)
{
	FadeButton* aButton = mButtons[BUTTON_ATTACK];
	aButton->mButtonImage = theImage;
	aButton->mOverImage = theImage;
	aButton->mDownImage = theImage;
	aButton->mDisabledImage = theImage;
	int w = theImage->GetCelWidth();
	int h = theImage->GetCelHeight();
	aButton->mNormalRect = Rect(0, 0, w, h);
	aButton->mOverRect = Rect(w, 0, w, h);
	aButton->mDownRect = Rect(w * 2, 0, w, h);
	aButton->mDisabledRect = Rect(theImage->mNumCols < 4 ? 0 : w * 3, 0, w, h);
	if (mImmuneLook)
		ShowImmuneAttack();
}

void Board::DisableAndHide()
{
	mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_UPDATE | WIDGETFLAGS_DRAW;
}

int Board::GetWordTileSize()
{
	return (int)mSelected.size();
}

// ---- the widget ----

void Board::AddedToManager(WidgetManager* theWidgetManager)
{
	LuaWidget::AddedToManager(theWidgetManager);
	for (auto& anEntry : mButtons)
		if (anEntry.second != nullptr)
			AddWidget(anEntry.second);
	LayoutButtons();
	SetAttackDisabled(true, false);
	mTooltip->SetFont(FONT_SMALL);
	mTooltip->SetBorderColor(Color(0, 0, 0));
	mTooltip->SetTooltipColor(Color(255, 255, 225));
	mTooltip->SetTextColor(Color(0, 0, 0));
	mTooltip->SetDisplayDelay(100);
	mTooltip->SetDisplayTime(500);
	mTooltip->AddText(GT(0x7aad78));
	mTooltip->AddText(GT(0x7aad94));
	mTooltip->SetXY(mButtons[BUTTON_ATTACK]->mX + 20, mButtons[BUTTON_ATTACK]->mY);
	mWidgetManager->SetFocus(this);
	if (mApp->mArrowHintManager != nullptr)
		theWidgetManager->BringToFront(mApp->mArrowHintManager);
}

void Board::DrawOverlay(Graphics* g)
{
	if (!mVisible)
		return;
	for (const std::string& aFunction : mOverlayCallbacks)
		DrawLua(g, aFunction.c_str());
	if (mDrawArrowHintsAboveOverlay && mApp->mArrowHintManager != nullptr)
		mApp->mArrowHintManager->DrawOverlay(g);
	if (mLevelupOverlay && !mIsMiniGame)
		DrawLua(g, "gBattleEngine:DrawLevelupOverlay");
	if (mPauseCount > 0 && !mApp->ShowingConversationPanel() && mApp->mBookScreen == nullptr && !mFlash->mActive &&
	    mApp->GetDialog(GameApp::DIALOG_ARENA_WIN) == nullptr) {
		g->SetColor(Color(0, 0, 0, 140));
		g->FillRect(0, 0, mWidth, mHeight);
		if (mPauseNoBannerCount == 0)
			g->DrawImage(IMAGE_PAUSED_BANNER, (mWidth - IMAGE_PAUSED_BANNER->GetWidth()) / 2,
			             (mHeight - IMAGE_PAUSED_BANNER->GetHeight()) / 2);
	}
	if (mFlash->mActive)
		mFlash->Draw(g);
}

// ---- the scripts' Board class ----

namespace {

// the grid's tiles in a random order (only the unselected ones when the argument is true)
int GetRandomizedTiles(Board* theBoard, lua_State* L)  // 0x41df60
{
	bool anUnselected = lua_gettop(L) == 1 && lua_toboolean(L, 1);
	std::vector<Tile*> aTiles(theBoard->mTiles.begin(), theBoard->mTiles.end());
	std::vector<int> anIndices;
	for (int i = 0; i < (int)aTiles.size(); i++)
		if (!anUnselected || !aTiles[i]->mSelected)
			anIndices.push_back(i);
	int aCount = 0;
	while (!anIndices.empty()) {
		size_t aPick = Rand() % anIndices.size();
		Tile::PushTile(aTiles[anIndices[aPick]]);
		anIndices.erase(anIndices.begin() + aPick);
		aCount++;
	}
	return aCount;
}

// the unselected tiles around a tile, in a random order; in the timed mode a selected tile's are the tiles before
// and after it in the word
int GetRandomizedAdjacentTiles(Board* theBoard, lua_State* L)  // 0x41e260
{
	if (lua_gettop(L) < 1)
		luaL_error(L, "1 parameter expected");
	Tile* aTile = (Tile*)gLuaSupport->CheckObject(1, ClassOf<Tile>::info);
	if (aTile == nullptr)
		luaL_typerror(L, 1, "Tile");
	std::list<Tile*> aTiles;
	if (theBoard->mApp->mTimedMode && aTile->mSelected) {
		std::list<Tile*>& aWord = theBoard->mSelected;
		auto it = std::find(aWord.begin(), aWord.end(), aTile);
		if (it != aWord.end()) {
			if (it != aWord.begin())
				aTiles.push_back(*std::prev(it));
			if (std::next(it) != aWord.end())
				aTiles.push_back(*std::next(it));
		}
	} else if (theBoard->GetAdjacentTiles(aTile->mCol, aTile->mRow, aTiles)) {
		std::vector<Tile*> anUnselected;
		for (Tile* anAdjacent : aTiles)
			if (!anAdjacent->mSelected)
				anUnselected.push_back(anAdjacent);
		// the original's std::random_shuffle, on the C library's rand (Visual C++'s, which Lua's math.random shares)
		for (size_t i = 1; i < anUnselected.size(); i++)
			std::swap(anUnselected[i], anUnselected[poplua_rand() % (i + 1)]);
		aTiles.assign(anUnselected.begin(), anUnselected.end());
	}
	for (Tile* anAdjacent : aTiles)
		Tile::PushTile(anAdjacent);
	return (int)aTiles.size();
}

// the attack button's picture: a LuaImage, or its own with no argument
int ChangeAttackBtnImage(Board* theBoard, lua_State* L)  // 0x41e660
{
	Image* anImage = IMAGE_BUTTON_ATTACK;
	if (lua_gettop(L) == 1)
		anImage = ImageOf((LuaImage*)gLuaSupport->CheckObject(1, ClassOf<LuaImage>::info));
	theBoard->SetAttackImage(anImage);
	return 0;
}

}  // namespace

void Board::RegisterLua()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	Class<Board>("Board", false)
		.Base<LuaWidget>()
		.Method<&Board::Pause>("Pause")
		.Method<&Board::IsPaused>("IsPaused")
		.Method<&Board::DeselectAllTiles>("DeselectAllTiles")
		.Method<&Board::GridGetTile>("GridGetTile")
		.Method<&Board::SetAttackDisabled>("SetAttackDisabled")
		.Method<&Board::SetAttackBtnPos>("SetAttackBtnPos")
		.Method<&Board::SetScrambleDisabled>("SetScrambleDisabled")
		.Method<&Board::SetScrambleVisible>("SetScrambleVisible")
		.Method<&Board::ScrambleUseNormalForDisabled>("ScrambleUseNormalForDisabled")
		.Method<&Board::DoLevelUp>("DoLevelUp")
		.Method<&Board::EnableAttackBtnPAM>("EnableAttackBtnPAM")
		.Method<&Board::DoAltarScene>("DoAltarScene")
		.Method<&Board::RehupMouse>("RehupMouse")
		.Method<&Board::CursorIsOverGrid>("CursorIsOverGrid")
		.Method<&Board::AllowTooltips>("AllowTooltips")
		.Method<&Board::AddOverlayCallback>("AddOverlayCallback")
		.Method<&Board::RemoveOverlayCallback>("RemoveOverlayCallback")
		.Method<&Board::IsClonedTile>("IsClonedTile")
		.Method<&Board::DrawArrowHintsAboveOverlay>("DrawArrowHintsAboveOverlay")
		.Method<&Board::ValidateWord>("ValidateWord")
		.Method<&Board::HasOverlays>("HasOverlays")
		.Method<&Board::DisableAndHide>("DisableAndHide")
		.Method<&Board::CheckIfWordStillValid>("CheckIfWordStillValid")
		.Method<&Board::SetTMEnemyAttacking>("SetTMEnemyAttacking")
		.Method<&Board::GetWordTileSize>("GetWordTileSize")
		.Method<&Board::Scramble>("Scramble")
		.Method<&Board::DoingZoomEffect>("DoingZoomEffect")
		.Method<&Board::DoZoom>("DoZoom")
		.Method<&Board::GetZoomValue>("GetZoomValue")
		.Method<&Board::SetZoomCoords>("SetZoomCoords")
		.Method<&Board::DisableAttackPAM>("DisableAttackPAM")
		.Raw<&ChangeAttackBtnImage>("ChangeAttackBtnImage")
		.Raw<&GetRandomizedTiles>("GetRandomizedTiles")
		.Raw<&GetRandomizedAdjacentTiles>("GetRandomizedAdjacentTiles");
}

// ---- the tutorial's arrows ----

void Board::ArrowHintsClick(int x, int y, int theClickCount)
{
	if (mApp->mArrowHintManager != nullptr)
		mApp->mArrowHintManager->Click(x, y, theClickCount);
}

void Board::ArrowHintsHide(bool isHidden)
{
	if (mApp->mArrowHintManager != nullptr)
		mApp->mArrowHintManager->DontDrawArrowHints(isHidden);
}
