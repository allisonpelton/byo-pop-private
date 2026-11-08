#include "Tile.h"

#include <string>

#include "LuaBind.h"
#include "TileEngine.h"

using namespace Sexy;
using namespace LuaLib;

// Port: what a tile tells the tile engine (gTileEngine) it leaves untold when there is none; the original
// dereferences the global without looking, as it only ever has tiles with an engine.

// The Lua object's GetLetter gives a string; the scripts' cobj:GetLetter() gets the same back (the original's wrapper
// pushed the char as a string of one character, 0x488b70)
static std::string LuaGetLetter(Tile* theTile)
{
	char aLetter = theTile->GetLetter();
	return aLetter != 0 ? std::string(1, aLetter) : std::string();
}

Tile::Tile()
{
	RegisterLua();
	// Port: the original made the curves with new and freed them with free
	mSlideCurve = new CurvedValue;  // mode 0, curve 3, 0 to 1 over t 0 to 1
	mSlideCurve->mT = 0.0;
	mSlideCurve->Step();
	mAlphaCurve = new CurvedValue;
	mAlphaCurve->mStart = 255.0;
	mAlphaCurve->mEnd = 255.0;
	mAlphaCurve->mTStart = 0.0;
	mAlphaCurve->mTEnd = 1.0;
	mAlphaCurve->mT = 0.0;
	mAlphaCurve->Step();
}

Tile::~Tile()
{
	// Port: only a tile that was linked has a Lua object to tell
	if (IsLinked())
		CallLinkedMethod("Destructor", 0);
	delete mAlphaCurve;
	delete mSlideCurve;
}

void Tile::RegisterLua()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	// Port: the original listed Update twice
	Class<Tile>("Tile", false)
		.Method<&Tile::LuaGetRegisteredName>("LuaGetRegisteredName")
		.Method<&Tile::Update>("Update")
		.Function<&LuaGetLetter>("GetLetter")
		.Method<&Tile::IsVowel>("IsVowel")
		.Method<&Tile::IsHilited>("IsHilited")
		.Method<&Tile::IsSelected>("IsSelected")
		.Method<&Tile::IsBonus>("IsBonus")
		.Method<&Tile::Hilite>("Hilite")
		.Method<&Tile::Select>("Select")
		.Method<&Tile::CanSelect>("CanSelect")
		.Method<&Tile::IsMoving>("IsMoving")
		.Method<&Tile::IsFalling>("IsFalling")
		.Method<&Tile::GetX>("GetX")
		.Method<&Tile::GetY>("GetY")
		.Method<&Tile::SetXY>("SetXY")
		.Method<&Tile::LuaSetScale>("LuaSetScale")
		.Method<&Tile::GetTargetX>("GetTargetX")
		.Method<&Tile::GetTargetY>("GetTargetY")
		.Method<&Tile::GetCol>("GetCol")
		.Method<&Tile::GetRow>("GetRow")
		.Method<&Tile::StopFalling>("StopFalling")
		.Method<&Tile::GetFallTarget>("GetFallTarget")
		.Method<&Tile::MaxAlpha>("MaxAlpha");
}

void Tile::PushTile(Tile* theTile)
{
	gLuaSupport->PushObject(ClassOf<Tile>::info, theTile);
}

void Tile::CreateLinkedObject(void* theObject, bool theUnused)
{
	LuaSupport* s = gLuaSupport;
	mLinkedName = LuaGetRegisteredName();
	mLinkedObj = theObject;
	lua_pushstring(s->L, mLinkedName);
	PushLinkedObject();
	lua_pushboolean(s->L, theUnused);
	s->Call("CreateLinkedObject", 3, 0);
	// Port: no RegisterEnums here (the original did it; a tile has no constants)
}

void Tile::CopyState(const Tile* theTile)
{
	if (theTile == this)
		return;
	mX = theTile->mX;
	mY = theTile->mY;
	mCol = theTile->mCol;
	mRow = theTile->mRow;
	mAlphaState = theTile->mAlphaState;
	mFallDistance = theTile->mFallDistance;
	mUpdateCnt = theTile->mUpdateCnt;
	mFallDelay = theTile->mFallDelay;
	mSelected = theTile->mSelected;
	mHilited = theTile->mHilited;
	mFalling = theTile->mFalling;
	mMoving = theTile->mMoving;
	mSlideFromX = theTile->mSlideFromX;
	mSlideFromY = theTile->mSlideFromY;
	mSlideToX = theTile->mSlideToX;
	mSlideToY = theTile->mSlideToY;
	mSlideSpeed = theTile->mSlideSpeed;
	*mAlphaCurve = *theTile->mAlphaCurve;
	*mSlideCurve = *theTile->mSlideCurve;
}

void Tile::Update()
{
	mUpdateCnt++;
	if (mMoving && !SlideStep() && gTileEngine != nullptr)
		gTileEngine->StopMoving(this, true);
	if (mFalling) {
		mFallDelay--;
		if (mFallDelay <= 0) {
			int aY = mY + 7;
			mFallDistance -= 7;
			if (mFallDistance < 0) {
				aY += mFallDistance;
				mFallDistance = 0;
			}
			mY = aY;
			mFalling = mFallDistance != 0;
			if (!mFalling)
				TileYChanged(aY);
		}
	}
	if (mAlphaState == 0 || mAlphaState == 2) {
		mAlphaCurve->mT += 0.01;
		if (!mAlphaCurve->Step())
			mAlphaState = mAlphaState != 0 ? 3 : 1;
		lua_pushnumber(gLuaSupport->L, mAlphaCurve->Value());
		CallLinkedMethod("SetAlpha", 1);
	}
	CallLinkedMethod("Update", 0);
}

char Tile::GetLetter()
{
	LuaClassRet aRet = CallLinkedMethod("GetLetter", 0);
	// (a result that is no string is reported by the original and read as an empty one)
	return aRet.String().c_str()[0];
}

bool Tile::IsVowel()
{
	std::string aVowels = "AEIOUY";
	return aVowels.find(GetLetter()) != std::string::npos;
}

bool Tile::IsWildTile()
{
	return CallLinkedMethod("IsWildTile", 0).Bool();
}

bool Tile::IsBonus()
{
	return CallLinkedMethod("IsBonus", 0).Bool();
}

void Tile::Hilite(bool theHilite)
{
	mHilited = theHilite;
	lua_pushboolean(gLuaSupport->L, theHilite);
	CallLinkedMethod("Hilited", 1);
}

void Tile::Select(bool theSelect)
{
	mSelected = theSelect;
	lua_pushboolean(gLuaSupport->L, theSelect);
	CallLinkedMethod("Selected", 1);
}

bool Tile::CanSelect(int theX, int theY)
{
	if (mMoving || gTileEngine == nullptr)
		return false;
	// gTileEngine:CanSelect(tile, x, y)
	PushTile(this);
	lua_pushint(gLuaSupport->L, theX);
	lua_pushint(gLuaSupport->L, theY);
	return gTileEngine->CallLinkedMethod("CanSelect", 3).Bool();
}

void Tile::SetXY(int theX, int theY, bool theNotify)
{
	mX = theX;
	if (theNotify)
		TileXChanged(theX);
	mY = theY;
	if (theNotify)
		TileYChanged(theY);
}

int Tile::GetFallTarget()
{
	if (!mFalling)
		return mY;
	return mFallDistance + mY;
}

void Tile::MaxAlpha()
{
	mAlphaCurve->mT = mAlphaCurve->mTEnd;
	mAlphaCurve->Step();
}

bool Tile::Contains(int theX, int theY)
{
	if (mMoving)
		return false;
	// the square of the scaled size (50 pixels) about the centre of the tile, in floats as the original computed it
	float aSize = (float)((double)mScale * 50.0);
	double aLeft = (float)((double)mX + 25.0 - (double)aSize * 0.5);
	double aTop = (float)((double)mY + 25.0 - (double)aSize * 0.5);
	double aX = theX;
	double aY = theY;
	return aX >= aLeft && aX < aLeft + aSize && aY >= aTop && aY < aTop + aSize;
}

bool Tile::SlideStep()
{
	if (mSlideSpeed == 0.0f)
		return false;
	bool aSliding = false;
	int aX, aY;
	mSlideCurve->mT += mSlideSpeed;
	if (mSlideCurve->Step()) {
		double aFrac = mSlideCurve->Value();
		aX = (int)(mSlideFromX * (1.0 - aFrac) + mSlideToX * aFrac);
		aY = (int)(mSlideFromY * (1.0 - aFrac) + mSlideToY * aFrac);
		aSliding = true;
	} else {
		aX = (int)mSlideToX;
		aY = (int)mSlideToY;
	}
	mX = aX;
	TileXChanged(aX);
	mY = aY;
	TileYChanged(aY);
	return aSliding;
}

void Tile::Fall(int theDistance, int theDelay)
{
	mFalling = true;
	mFallDistance = theDistance;
	mFallDelay = theDelay;
}

void Tile::TileXChanged(int theX)
{
	if (gTileEngine == nullptr)
		return;
	PushTile(this);
	lua_pushint(gLuaSupport->L, theX);
	gTileEngine->CallLinkedMethod("TileXChanged", 2);
}

void Tile::TileYChanged(int theY)
{
	if (gTileEngine == nullptr)
		return;
	PushTile(this);
	lua_pushint(gLuaSupport->L, theY);
	gTileEngine->CallLinkedMethod("TileYChanged", 2);
}

void Tile::SetLetter(const std::string& theLetter)
{
	if (gTileEngine == nullptr)
		return;
	PushTile(this);
	lua_pushstring(gLuaSupport->L, theLetter.c_str());
	gTileEngine->CallLinkedMethod("ChangeLetter", 2);
}
