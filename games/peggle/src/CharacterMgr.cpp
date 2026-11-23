// CharacterMgr.cpp: reading characters/characters.cfg and cutting the masters' pictures into frames; see CharacterMgr.h.

#include "CharacterMgr.h"
#include "NotTranslated.h"

#include "Color.h"
#include "ConfigParser.h"
#include "DDImage.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"

#include <algorithm>
#include <set>

using namespace Sexy;

static const char* const CHARACTERS_FILE = "characters/characters.cfg";
static const char* const CHARACTERS_FOLDER = "characters/";

// where the first master stands when a game has none to read (0x4747b0)
static const float DEFAULT_CHARACTER_X = 320;
static const float DEFAULT_CHARACTER_Y = 40;

// the powers' names in characters.cfg, in the order of PowerupType (0x6496a8)
static const char* const POWERUP_NAMES[NUM_POWERUPS] = {
	"None",		   "Guide",	  "Flippers", "Multiball",	   "Pyramid",  "SpaceBlast", "SpookyBall",
	"ZenBall",	   "ShotExtender", "FlowerPower", "Nudge", "TimeBomb", "Fireball", "LuckySpin"};

// A picture's pixels with a mask's transparency: where the mask is not clear, the picture's colours take the mask's
// alpha, or when the picture itself is part clear, the product of both (0x404ed0).
static void CopyPixelsWithMask(MemoryImage* theSource, MemoryImage* theMask, const Rect& theSourceRect, MemoryImage* theDest, int theMaskX,
							   int theMaskY, int theDestX, int theDestY)
{
	int aSourceWidth = theSource->GetWidth();
	int aMaskWidth = theMask->GetWidth();
	int aDestWidth = theDest->GetWidth();
	ulong* aSourceRow = theSource->GetBits() + theSourceRect.mY * aSourceWidth + theSourceRect.mX;
	ulong* aMaskRow = theMask->GetBits() + theMaskY * aMaskWidth + theMaskX;
	ulong* aDestRow = theDest->GetBits() + theDestY * aDestWidth + theDestX;
	for (int aRow = 0; aRow < theSourceRect.mHeight; aRow++)
	{
		for (int aCol = 0; aCol < theSourceRect.mWidth; aCol++)
		{
			unsigned int aMaskAlpha = aMaskRow[aCol] & 0xff000000;
			if (aMaskAlpha == 0)
				continue;
			unsigned int aPixel = aSourceRow[aCol];
			if ((aPixel & 0xff000000) < 0xff000000)
			{
				if ((aPixel & 0xff000000) != 0)
				{
					unsigned int aProduct = (aPixel >> 24) * (aMaskAlpha >> 8);
					aDestRow[aCol] = ((aProduct ^ aPixel) & 0xffffff) ^ aProduct;
				}
			}
			else
				aDestRow[aCol] = (aPixel & 0xffffff) | aMaskAlpha;
		}
		aSourceRow += aSourceWidth;
		aMaskRow += aMaskWidth;
		aDestRow += aDestWidth;
	}
}

// The smallest rectangle of the rows that hold what a mask leaves visible (a pixel that is not fully opaque); the first
// run of such rows only (0x4078c0). A width or height below one comes back for a rectangle without any.
static Rect FindVisibleRect(MemoryImage* theMask, const Rect& theRect)
{
	int aWidth = theMask->GetWidth();
	ulong* aRow = theMask->GetBits() + theRect.mY * aWidth + theRect.mX;
	int aMinX = theRect.mWidth;
	int aMinY = theRect.mHeight;
	int aMaxX = 0;
	int aLastRow = 0;
	for (int aRowIndex = 0; aRowIndex < theRect.mHeight; aRowIndex++)
	{
		bool aFound = false;
		for (int aCol = 0; aCol < theRect.mWidth; aCol++)
		{
			if ((aRow[aCol] & 0xff000000) != 0xff000000)
			{
				if (aFound)
				{
					if (aMaxX < aCol)
						aMaxX = aCol;
				}
				else
				{
					if (aCol < aMinX)
						aMinX = aCol;
					aFound = true;
				}
			}
		}
		if (!aFound)
		{
			if (aMinY != theRect.mHeight)
				break;
		}
		else
		{
			if (aMinY == theRect.mHeight)
				aMinY = aRowIndex;
			aLastRow = aRowIndex;
		}
		aRow += aWidth;
	}
	return Rect(theRect.mX + aMinX, theRect.mY + aMinY, aMaxX - aMinX + 1, aLastRow - aMinY + 1);
}

// 0x473c30
CharacterMgr::CharacterMgr()
	: mBlankImage(nullptr), mCurCharacter(0), mEyeLinesLeft(0), mNumFrames(0), mSlowMoFrames(0), mUnknown30(0), mUnknown34(0),
	  mUnknown38(0), mUnknown3C(0), mUnknown40(0), mUnknown45(false), mUnknown48(0), mPosOffsetX(400), mPosOffsetY(70)
{
}

CharacterMgr::~CharacterMgr()
{
	Clear(true);
	delete mBlankImage;
}

// 0x473d90 (the pictures the frames hold are deleted here; the original leaks them)
void CharacterMgr::Clear(bool theClearCharacters)
{
	mUnknown48 = 0;
	mUnknown34 = 0;
	mUnknown30 = 0;
	mUnknown3C = -2;
	mUnknown38 = -2;
	mUnknown45 = false;
	if (theClearCharacters)
	{
		mImageMgr.Clear();
		std::set<Image*> aPictures;
		for (size_t i = 0; i < mCharacters.size(); i++)
		{
			for (size_t j = 0; j < mCharacters[i].mFrames.size(); j++)
			{
				const CharacterFrame& aFrame = mCharacters[i].mFrames[j];
				aPictures.insert(aFrame.mSourceImage);
				aPictures.insert(aFrame.mImage);
				aPictures.insert(aFrame.mMaskImage);
			}
		}
		aPictures.erase(nullptr);
		for (std::set<Image*>::iterator anItr = aPictures.begin(); anItr != aPictures.end(); ++anItr)
			delete *anItr;
		mCharacters.clear();
	}
}

// 0x43e3b0
int CharacterMgr::GetCharacterIndex(const std::string& theName)
{
	for (int i = 0; i < (int)mCharacters.size(); i++)
	{
		if (sexy_stricmp(mCharacters[i].mName.c_str(), theName.c_str()) == 0)
			return i;
	}
	return -1;
}

// 0x43e320: makes a master the one shown (the number wraps around)
int CharacterMgr::SetCurCharacter(int theCharacter)
{
	int aCount = (int)mCharacters.size();
	if (aCount == 0)
		return 0;
	mUnknown40 = 0;
	int aPrevious = mCurCharacter;
	int aQuotient;
	if (theCharacter < 0)
	{
		aQuotient = -theCharacter / aCount;
		int aRemainder = -theCharacter % aCount;
		theCharacter = (aRemainder != 0) ? aCount - aRemainder : 0;
	}
	else
	{
		aQuotient = theCharacter / aCount;
		theCharacter = theCharacter % aCount;
	}
	mCurCharacter = theCharacter;
	// the eyes are placed again when the master changes (CalcEyePos, 0x438a10, with the Board)
	(void)aPrevious;
	return aQuotient;
}

// 0x446490
int CharacterMgr::ReadPowerupType(ConfigParser& theParser, const std::string& theName)
{
	for (int i = 0; i < NUM_POWERUPS; i++)
	{
		if (sexy_stricmp(theName.c_str(), POWERUP_NAMES[i]) == 0)
			return i;
	}
	theParser.Error("Unknown powerup type");
	return 0;
}

// 0x442bd0: the frame the last "Eyes" line belongs to (the last frame when the frames are not being counted)
CharacterFrame* CharacterMgr::GetEyesFrame(Character& theCharacter)
{
	int aCount = (int)theCharacter.mFrames.size();
	if (aCount == 0)
		return nullptr;
	if (mEyeLinesLeft < 1)
		return &theCharacter.mFrames.back();
	int anIndex = aCount - mEyeLinesLeft;
	if (anIndex >= 0 && anIndex < aCount)
		return &theCharacter.mFrames[anIndex];
	return nullptr;
}

// 0x4657c0: one "Key: value" line of a master
void CharacterMgr::ReadCharacterConfig(ConfigParser& theParser, Character& theCharacter)
{
	StringParser& aValue = theParser.GetValParser();

	if (theParser.KeyIs("image"))
	{
		// "Image: name[,frames]": the file with the frames one above the other, its mask and optionally a second mask
		std::string aName = std::string(CHARACTERS_FOLDER) + aValue.ReadString(true, true);
		std::string aMaskName = aName + "mask";
		std::string aMask2Name = aName + "mask2";
		MemoryImage* anImage = gSexyAppBase->GetImage(aName, true);
		MemoryImage* aMask = gSexyAppBase->GetImage(aMaskName, true);
		MemoryImage* aMask2 = gSexyAppBase->GetImage(aMask2Name, true);
		if (anImage == nullptr)
			theParser.Error("Image not found: " + aName);
		else if (aMask == nullptr)
			theParser.Error("Image not found: " + aMaskName);
		if (aMask->GetWidth() != anImage->GetWidth() || aMask->GetHeight() != anImage->GetHeight())
			theParser.Error("Mask size must be the same as image size");
		if (aMask2 != nullptr && (aMask2->GetWidth() != anImage->GetWidth() || aMask2->GetHeight() != anImage->GetHeight()))
			theParser.Error("Mask2 size must be the same as image size");

		int aNumFrames = 1;
		mEyeLinesLeft = 0;
		if (aValue.CheckNextChar(','))
		{
			aNumFrames = aValue.ReadInt();
			if (aNumFrames < 1)
				theParser.Error("Invalid number of frames");
			else if (anImage->GetHeight() % aNumFrames != 0)
				theParser.Error("Image height not divisble by number of frames");
			if (aNumFrames >= 2)
			{
				mEyeLinesLeft = aNumFrames + 1;
				mNumFrames = aNumFrames;
			}
		}

		gSexyAppBase->AddMemoryImage(anImage);
		int aFrameHeight = anImage->GetHeight() / aNumFrames;
		int aPackedWidth = 0;
		int aPackedHeight = 0;
		for (int aFrameIndex = 0; aFrameIndex < aNumFrames; aFrameIndex++)
		{
			int aTop = aFrameIndex * aFrameHeight;
			theCharacter.mFrames.push_back(CharacterFrame());
			CharacterFrame& aFrame = theCharacter.mFrames.back();
			aFrame.mSourceImage = anImage;
			aFrame.mSourceRect = Rect(0, aTop, anImage->GetWidth(), aFrameHeight);

			// what the mask leaves visible, and a pixel more around it
			Rect aTrimmed = FindVisibleRect(aMask, aFrame.mSourceRect);
			if (aTrimmed.mWidth < 1 || aTrimmed.mHeight < 1)
				theParser.Error(aMaskName + " messed.");
			aFrame.mTrimmedRect = aTrimmed;
			aFrame.mTrimmedOffsetX = aTrimmed.mX;
			aFrame.mTrimmedOffsetY = aTrimmed.mY - aTop;
			Rect aWithBorder = aTrimmed;
			if (aWithBorder.mX > 0)
			{
				aWithBorder.mWidth++;
				aWithBorder.mX--;
			}
			if (aWithBorder.mWidth + aWithBorder.mX < aFrame.mSourceRect.mWidth + aFrame.mSourceRect.mX)
				aWithBorder.mWidth++;
			if (aWithBorder.mY > 0)
			{
				aWithBorder.mHeight++;
				aWithBorder.mY--;
			}
			if (aWithBorder.mHeight + aWithBorder.mY < aFrame.mSourceRect.mHeight + aTop)
				aWithBorder.mHeight++;

			if (aMask2 != nullptr)
			{
				Rect aTrimmed2 = FindVisibleRect(aMask2, aFrame.mSourceRect);
				if (aTrimmed2.mWidth < 1 || aTrimmed2.mHeight < 1)
					theParser.Error(aMask2Name + " messed.");
				Image* aMaskCopy = gSexyAppBase->CopyImage(aMask2, aTrimmed2);
				aFrame.mMaskImage = aMaskCopy;
				aFrame.mMaskOffsetX = aTrimmed2.mX;
				aFrame.mMaskOffsetY = aTrimmed2.mY - aTop;
				aFrame.mMaskRect = Rect(0, 0, aTrimmed2.mWidth, aTrimmed2.mHeight);
				gSexyAppBase->AddMemoryImage(dynamic_cast<MemoryImage*>(aMaskCopy));
			}
			aPackedWidth = std::max(aPackedWidth, aWithBorder.mWidth);
			aPackedHeight = std::max(aPackedHeight, aWithBorder.mHeight);
			aFrame.mImageRect = aWithBorder;
		}

		// the frames' pixels go under each other in one new picture
		MemoryImage* aPacked = new MemoryImage();
		aPacked->Create(aPackedWidth, aPackedHeight * aNumFrames);
		aPacked->mFilePath = "prog_char_" + theCharacter.mName;
		gSexyAppBase->AddMemoryImage(aPacked);
		int aFirstNewFrame = (int)theCharacter.mFrames.size() - aNumFrames;
		for (int aFrameIndex = 0; aFrameIndex < aNumFrames; aFrameIndex++)
		{
			CharacterFrame& aFrame = theCharacter.mFrames[aFirstNewFrame + aFrameIndex];
			int aPackedTop = aFrameIndex * aPackedHeight;
			CopyPixelsWithMask(anImage, aMask, aFrame.mImageRect, aPacked, aFrame.mImageRect.mX, aFrame.mImageRect.mY, 0, aPackedTop);
			int aX = aFrame.mTrimmedRect.mX - aFrame.mImageRect.mX;
			int aY = (aFrame.mTrimmedRect.mY - aFrame.mImageRect.mY) + aPackedTop;
			aFrame.mImageRect = Rect(aX, aY, aFrame.mTrimmedRect.mWidth, aFrame.mTrimmedRect.mHeight);
			aFrame.mImage = aPacked;
			if (aMask2 == nullptr)
			{
				aFrame.mMaskRect = aFrame.mImageRect;
				aFrame.mMaskImage = aPacked;
				aFrame.mMaskOffsetX = aFrame.mTrimmedOffsetX;
				aFrame.mMaskOffsetY = aFrame.mTrimmedOffsetY;
			}
		}
		delete aMask2;
		delete aMask;
	}
	else if (theParser.KeyIs("eyeimage"))
	{
		// "EyeImage: name[,mask]"
		std::string aName = std::string(CHARACTERS_FOLDER) + aValue.ReadString(true, true);
		std::string aMaskName = aName;
		if (aValue.CheckNextChar(','))
			aMaskName = std::string(CHARACTERS_FOLDER) + aValue.ReadString(true, true);
		theCharacter.mEyeImage = mImageMgr.GetImage(aName, nullptr, true);
		theCharacter.mEyeMaskImage = mImageMgr.GetImage(aMaskName, nullptr, true);
		if (theCharacter.mEyeImage == nullptr)
			theParser.Error("Image not found: " + aName);
		else if (theCharacter.mEyeMaskImage == nullptr)
			theParser.Error("Image not found: " + aMaskName);
	}
	else if (theParser.KeyIs("eyes"))
	{
		// "Eyes: x1,y1,x2,y2": the two eyes' places, in the whole picture
		if (mEyeLinesLeft > 0)
			mEyeLinesLeft--;
		CharacterFrame* aFrame = GetEyesFrame(theCharacter);
		if (aFrame == nullptr)
			theParser.Error("You need to specify an image first.");
		aFrame->mEyes[0] = (float)aValue.ReadInt();
		aValue.EnsureNextChar(',');
		aFrame->mEyes[1] = (float)aValue.ReadInt();
		aValue.EnsureNextChar(',');
		aFrame->mEyes[2] = (float)aValue.ReadInt();
		aValue.EnsureNextChar(',');
		aFrame->mEyes[3] = (float)aValue.ReadInt();
		if (mEyeLinesLeft > 0)
		{
			float anOffset = (float)((mNumFrames - mEyeLinesLeft) * aFrame->mSourceRect.mHeight);
			aFrame->mEyes[1] -= anOffset;
			aFrame->mEyes[3] -= anOffset;
		}
	}
	else if (theParser.KeyIs("pos"))
	{
		theCharacter.mPosX = (float)aValue.ReadInt();
		aValue.EnsureNextChar(',');
		theCharacter.mPosY = (float)aValue.ReadInt();
	}
	else if (theParser.KeyIs("charoff"))
	{
		CharacterFrame* aFrame = GetEyesFrame(theCharacter);
		if (aFrame == nullptr)
			theParser.Error("You need to specify an image first.");
		aFrame->mCharOffsetX = aValue.ReadInt();
		aValue.EnsureNextChar(',');
		aFrame->mCharOffsetY = aValue.ReadInt();
	}
	else if (theParser.KeyIs("deleteframe"))
	{
		int anIndex = aValue.ReadInt();
		if (anIndex >= 0 && anIndex < (int)theCharacter.mFrames.size())
			theCharacter.mFrames.erase(theCharacter.mFrames.begin() + anIndex);
	}
	else if (theParser.KeyIs("Powerup"))
		theCharacter.mPowerup = ReadPowerupType(theParser, aValue.ReadString(true, true));
	else if (theParser.KeyIs("tweenspeed"))
		theCharacter.mTweenSpeed = aValue.ReadInt();
	else if (theParser.KeyIs("turnline"))
		theCharacter.mTurnLine = aValue.ReadInt();
	else if (theParser.KeyIs("desc"))
		aValue.ReadString(theCharacter.mDesc, false, true);
	else if (theParser.KeyIs("tip"))
	{
		theCharacter.mTips.push_back(std::string());
		aValue.ReadString(theCharacter.mTips.back(), false, true);
	}
	else
		theParser.ErrorUnexpectedKey();
}

// 0x474870
void CharacterMgr::ReadConfig(const std::string& theFileName)
{
	Clear(true);

	// Lines before the first "Character" group set what every master starts with.
	Character aPrototype;
	ConfigParser aParser(true);
	aParser.Open(theFileName, true);
	bool inCharacter = false;
	while (aParser.ReadNext())
	{
		if (aParser.mEvent == ConfigParser::EVENT_GROUP_BEGIN)
		{
			if (!aParser.GroupIs("Character", 1))
			{
				aParser.ErrorUnexpectedGroupType();
				continue;
			}
			mEyeLinesLeft = -1;
			inCharacter = true;
			const std::string& aName = aParser.mGroups.back().mName;
			if (aName.empty())
				aParser.Error("Need name for character");
			mCharacters.push_back(aPrototype);
			Character& aCharacter = mCharacters.back();
			aCharacter.mName = aName;
			aCharacter.mPosX += (float)mPosOffsetX;
			aCharacter.mPosY += (float)mPosOffsetY;
			aCharacter.mEyeImage = mBlankImage;
			aCharacter.mEyeMaskImage = mBlankImage;
		}
		else if (aParser.mEvent == ConfigParser::EVENT_KEY)
			ReadCharacterConfig(aParser, inCharacter ? mCharacters.back() : aPrototype);
		else
			inCharacter = false;
	}
}

// 0x474cb0
void CharacterMgr::Load()
{
	if (mBlankImage == nullptr)
	{
		mBlankImage = new MemoryImage();
		mBlankImage->Create(4, 4);
		Graphics aGraphics(mBlankImage);
		aGraphics.SetColor(Color(0, 0, 0));
		aGraphics.FillRect(0, 0, mBlankImage->GetWidth(), mBlankImage->GetHeight());
	}
	ReadConfig(CHARACTERS_FILE);
}

// 0x4747b0
void CharacterMgr::InitDefault()
{
	Clear(true);
	Character aCharacter;
	aCharacter.mPosX = DEFAULT_CHARACTER_X;
	aCharacter.mPosY = DEFAULT_CHARACTER_Y;
	mCharacters.push_back(aCharacter);
	// 0x45ac90: the eyes' pictures are left to the first Load
}

// 0x4410c0 and 0x43e460 (CalcEyePos 0x438a10): the eyes follow the mouse or the ball, which is part of the drawing and is
// translated with it; nothing of the rules reads where they look.
void CharacterMgr::CalcLookPos(Board* theBoard, bool theForce)
{
	NOT_TRANSLATED_DRAWING("CharacterMgr::CalcLookPos");
}

void CharacterMgr::Resync()
{
	NOT_TRANSLATED_DRAWING("CharacterMgr::Resync");
}

// 0x439830
void CharacterMgr::Update()
{
	NOT_TRANSLATED("CharacterMgr::Update");
}

// 0x439990
void CharacterMgr::UpdatePaused()
{
	NOT_TRANSLATED("CharacterMgr::UpdatePaused");
}

// 0x439a60
void CharacterMgr::UpdateSlowMo()
{
	NOT_TRANSLATED("CharacterMgr::UpdateSlowMo");
}
