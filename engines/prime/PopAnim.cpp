#include "PopAnim.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

#include "Buffer.h"
#include "Common.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "PIEffect.h"
#include "PIEffectMath.h"
#include "PopAnimModParser.h"
#include "PopAnimNames.h"
#include "PrimeResourceManager.h"
#include "X86Math.h"
#include "SexyAppBase.h"

using namespace Sexy;

// ---- the data (PopAnimData.h) ----

// port: the exe constructs these in place
PAObjectPos::PAObjectPos() : mColor(Color::White)
{
	mTransform.LoadIdentity();
}

// port: the exe constructs these in place
PAImage::PAImage()
{
	mTransform.LoadIdentity();
}

// port: the exe constructs these in place (0x44acd0 (bejeweled3) for a vector of them)
PASlot::PASlot() : mPreviousColor(Color::White), mColor(Color::White)
{
	mTransform.LoadIdentity();
	mPreviousTransform.LoadIdentity();
}

// 0x440ea0 (bejeweled3)
PASpriteInst::PASpriteInst() : mColor(Color::White)
{
	mTransform.LoadIdentity();
}

// port: the exe's virtual deleting destructor
PASpriteInst::~PASpriteInst()
{
	for (size_t i = 0; i < mEffects.size(); i++)
		delete mEffects[i].mEffect;
	for (size_t i = 0; i < mSlots.size(); i++)
		delete mSlots[i].mChild;
}

// ---- transforms ----

// 0x583230 (bejeweled3): theLeft * theRight as a 3x3 matrix product. Each sum is rounded to a float as it grows,
// which is what the x87 code does and not what SexyMatrix3's operator* does.
static SexyMatrix3 MultiplyMatrices(const SexyMatrix3& theLeft, const SexyMatrix3& theRight)
{
	SexyMatrix3 r;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			float aSum = (float)((double)theLeft.m[i][0] * theRight.m[0][j]);
			aSum = (float)(aSum + (double)theLeft.m[i][1] * theRight.m[1][j]);
			r.m[i][j] = (float)(aSum + (double)theLeft.m[i][2] * theRight.m[2][j]);
		}
	}
	return r;
}

// 0x43f150 (bejeweled3): theParent * theChild, for the affine matrices of an animation (the last row stays 0 0 1).
// The x87 of the original adds in double precision.
static SexyMatrix3 MultiplyTransforms(const SexyMatrix3& theParent, const SexyMatrix3& theChild)
{
	const SexyMatrix3& c = theParent;
	const SexyMatrix3& e = theChild;
	SexyMatrix3 r;
	r.LoadIdentity();
	r.m00 = (float)((double)e.m00 * c.m00 + (double)e.m10 * c.m01);
	r.m01 = (float)((double)e.m11 * c.m01 + (double)c.m00 * e.m01);
	r.m10 = (float)((double)c.m11 * e.m10 + (double)c.m10 * e.m00);
	r.m11 = (float)((double)c.m10 * e.m01 + (double)e.m11 * c.m11);
	r.m02 = (float)(((double)e.m02 * c.m00 + c.m02) + (double)e.m12 * c.m01);
	r.m12 = (float)(((double)c.m10 * e.m02 + c.m12) + (double)e.m12 * c.m11);
	return r;
}

// 0x43f1c0 (bejeweled3): the transform a fraction theFrac of the way from theFrom to theTo
static SexyMatrix3 InterpolateTransforms(const SexyMatrix3& theFrom, const SexyMatrix3& theTo, float theFrac)
{
	double aFromWeight = 1.0 - (double)theFrac;
	SexyMatrix3 r;
	r.LoadIdentity();
	r.m00 = (float)((double)theTo.m00 * theFrac + (double)theFrom.m00 * aFromWeight);
	r.m01 = (float)((double)theTo.m01 * theFrac + (double)theFrom.m01 * aFromWeight);
	r.m10 = (float)((double)theTo.m10 * theFrac + (double)theFrom.m10 * aFromWeight);
	r.m11 = (float)((double)theTo.m11 * theFrac + (double)theFrom.m11 * aFromWeight);
	r.m02 = (float)((double)theTo.m02 * theFrac + (double)theFrom.m02 * aFromWeight);
	r.m12 = (float)((double)theTo.m12 * theFrac + (double)theFrom.m12 * aFromWeight);
	return r;
}

// The rotation by an angle that the file gives in thousandths of a radian, in the form the matrices have
// (x' = cos x - sin y, y' = sin x + cos y). The angle goes through a float, as in the exe.
// port: helper (inlined in 0x441500 (bejeweled3) and 0x442590 (bejeweled3))
static void RotationFromThousandths(int theThousandths, float* theSin, float* theCos)
{
	float anAngle = (float)(theThousandths / 1000.0);
	*theSin = (float)sin((double)anAngle);
	*theCos = (float)cos((double)anAngle);
}

// ---- the PopAnim ----

// 0x440750 (bejeweled3)
PopAnim::PopAnim(int theId, PopAnimListener* theListener) : mAnimColor(Color::White)
{
	mId = theId;
	mListener = theListener;
	mImageFolders.push_back("images\\");
	mImageFolders.push_back("");
	mVersion = 0;
	mDrawScale = 1.0f;
	mImageScale = 1.0f;
	mLoaded = false;
	mFrameRate = 0;
	mPamX = mPamY = mPamWidth = mPamHeight = 0;
	mMainInst = new PASpriteInst();
	mData = nullptr;
	mDataUsers = 0;
	mTransitionTime = 0;
	mTransitionElapsed = 0;
	mStartDelay = 0;
	mRandSeeded = false;
	mEffectOffsetX = 0.0;
	mEffectOffsetY = 0.0;
	mTransform.LoadIdentity();
	mDrawAdditive = false;
	mTransformsStale = true;
	mPlaying = false;
	mMirror = false;
	mInterpolate = true;
	mImageIsNew = false;
	mPaused = false;
}

// 0x4410a0 (bejeweled3)
PopAnim::~PopAnim()
{
	Reset();
}

// 0x441270 (bejeweled3): the animation is gone, but for what the game set up (the id, the listener, the image folders,
// the scales and whether to interpolate)
void PopAnim::Reset()
{
	mMirror = false;
	mAnimColor = Color::White;
	mLoaded = false;
	mRandSeeded = false;
	mPlaying = false;
	mFrameRate = 0;
	mError.clear();
	mPamFile.clear();
	mImages.clear();
	mRemaps.clear();
	if (mData != nullptr) {
		if (mDataUsers <= 1)
			delete mData;
		else
			mDataUsers--;
	}
	mData = nullptr;
	mDataUsers = 0;
	mTransformsStale = true;
	delete mMainInst;
	mMainInst = nullptr;
}

// 0x4413f0 (bejeweled3): what a load that failed says; the callers return the false it stands for
void PopAnim::SetError(const std::string& theError)
{
	mError = theError;
}

// 0x440bf0 (bejeweled3), 0x440f10 (bejeweled3) and 0x441020 (bejeweled3). What is copied is the state: the images are shared by reference,
// the definitions in mData by pointer, and the instances are the caller's to make.
void PopAnim::CopyFrom(const PopAnim* theOther)
{
	mX = theOther->mX;
	mY = theOther->mY;
	mWidth = theOther->mWidth;
	mHeight = theOther->mHeight;
	mHasAlpha = theOther->mHasAlpha;
	mClip = theOther->mClip;
	mPriority = theOther->mPriority;
	mZOrder = theOther->mZOrder;
	mVisible = theOther->mVisible;
	mMouseVisible = theOther->mMouseVisible;
	mDisabled = theOther->mDisabled;
	mHasFocus = theOther->mHasFocus;
	mIsDown = theOther->mIsDown;
	mIsOver = theOther->mIsOver;
	mHasTransparencies = theOther->mHasTransparencies;
	mColors = theOther->mColors;
	mMouseInsets = theOther->mMouseInsets;
	mDoFinger = theOther->mDoFinger;
	mWantsFocus = theOther->mWantsFocus;

	mId = theOther->mId;
	mListener = theOther->mListener;
	mImageFolders = theOther->mImageFolders;
	mVersion = theOther->mVersion;
	mRemaps = theOther->mRemaps;
	mDrawScale = theOther->mDrawScale;
	mImageScale = theOther->mImageScale;
	mLoaded = theOther->mLoaded;
	mPamFile = theOther->mPamFile;
	mPamFileName = theOther->mPamFileName;
	mFrameRate = theOther->mFrameRate;
	mPamX = theOther->mPamX;
	mPamY = theOther->mPamY;
	mPamWidth = theOther->mPamWidth;
	mPamHeight = theOther->mPamHeight;
	mError = theOther->mError;
	mLabel = theOther->mLabel;
	mImages = theOther->mImages;
	mData = theOther->mData;
	mDataUsers = theOther->mData != nullptr ? theOther->mDataUsers + 1 : 0;
	mEffectOffsetX = theOther->mEffectOffsetX;
	mEffectOffsetY = theOther->mEffectOffsetY;
	mTransform = theOther->mTransform;
	mAnimColor = theOther->mAnimColor;
	mDrawAdditive = theOther->mDrawAdditive;
	mPlaying = theOther->mPlaying;
	mMirror = theOther->mMirror;
	mInterpolate = theOther->mInterpolate;
	mImageIsNew = theOther->mImageIsNew;
	mPaused = theOther->mPaused;
}

// 0x440a30 (bejeweled3)
PopAnim* PopAnim::Clone() const
{
	PopAnim* aClone = new PopAnim(mId, mListener);
	aClone->CopyFrom(this);
	// the users of the data are counted in each of them
	const_cast<PopAnim*>(this)->mDataUsers = aClone->mDataUsers;
	// its own place in the animation: SetupMainSprite builds it
	return aClone;
}

// 0x441410 (bejeweled3): what the mod file's Remap commands make of a name from the .pam; the first that matches decides
std::string PopAnim::RemapName(const std::string& theName) const
{
	std::string aResult;
	for (std::list<std::pair<std::string, std::string> >::const_iterator anItr = mRemaps.begin(); anItr != mRemaps.end();
	     ++anItr) {
		if (PopAnimRemapName(theName, anItr->first, anItr->second, &aResult))
			return aResult;
	}
	return theName;
}

// ---- loading ----

// 0x4440a0 (bejeweled3). The exe returns nothing: whether it worked is mLoaded (and mError, when it says why not).
bool PopAnim::LoadAnimation(const std::string& theFileName)
{
	Reset();
	if (mMainInst == nullptr)
		mMainInst = new PASpriteInst();
	mMirror = false;
	// the extension starts at the first dot of the name
	std::string anExt;
	size_t aDot = theFileName.find('.');
	if (aDot != std::string::npos)
		anExt = StringToLower(theFileName.substr(aDot));

	if (anExt == ".pam") {
		LoadPam(theFileName);
	}
	else if (anExt == ".txt") {
		if (!LoadMod(theFileName) && mError.empty())
			SetError("Mod file loading error");
	}
	else if (anExt.empty()) {
		if (!LoadPam(theFileName + ".pam"))
			LoadMod(theFileName + ".txt");
	}
	return mLoaded;
}

// 0x443770 (bejeweled3): reads the mod file for the .pam and the renames, loads the .pam, reads the mod file again
// for what changes the loaded animation
bool PopAnim::LoadMod(const std::string& theFileName)
{
	PopAnimModParser aParser(this, theFileName);
	aParser.mMode = 1;
	if (!aParser.LoadDescriptor(theFileName))
		return false;
	if (mPamFile.empty()) {
		SetError("The mod file does not name a .pam (SetPamFile)");
		return false;
	}
	// the .pam is named relative to the mod file
	if (!LoadPam(GetPathFrom(mPamFile, GetFileDir(theFileName))))
		return false;
	aParser.mMode = 2;
	return aParser.LoadDescriptor(theFileName);
}

// 0x443ac0 (bejeweled3), slot 88: the name is tried in each image folder, beside the .pam; the app's shared images
// find the file (and, for a name with a "|" and an id after it, the resource manager's picture, which the game's
// app does in its GetSharedImage)
SharedImageRef PopAnim::GetImageHook(const std::string& thePamDir, const std::string& thePamFile,
                                     const std::string& theImageName)
{
	if (theImageName.empty()) {
		SetError("No image file name specified");
		return SharedImageRef();
	}
	for (size_t i = 0; i < mImageFolders.size(); i++) {
		std::string aPath = GetPathFrom(mImageFolders[i], thePamDir);
		if (!aPath.empty() && aPath[aPath.size() - 1] != '\\' && aPath[aPath.size() - 1] != '/')
			aPath += "\\";
		aPath += theImageName;
		SharedImageRef anImage = gSexyAppBase->GetSharedImage(aPath, mMirror ? "MIRRORED" : "", &mImageIsNew);
		MemoryImage* aMemoryImage = anImage;
		if (aMemoryImage != nullptr) {
			aMemoryImage->Palletize();
			return anImage;
		}
	}
	SetError("Unable to load image: " + theImageName + " (" + thePamFile + ")");
	return SharedImageRef();
}

// port: the exe's slot 90 is a bare return
// slot 90: a bare return in the exe
void PopAnim::ImageLoaded(PAImage& /*theImage*/)
{
}

// 0x443f30 (bejeweled3), slot 89: one of an image entry's pictures. False when the picture is missing.
bool PopAnim::GetImage(const std::string& thePamDir, const std::string& thePamFile, PAImage* theEntry,
                       const std::string& theImageName)
{
	SharedImageRef anImage = GetImageHook(thePamDir, thePamFile, theImageName);
	MemoryImage* aMemoryImage = anImage;
	if (aMemoryImage == nullptr)
		return false;
	aMemoryImage->mNumCols = theEntry->mNumCols;
	aMemoryImage->mNumRows = theEntry->mNumRows;
	// the first picture of a file with a grid: the position gives the room the grid leaves around the cells
	if (theEntry->mImages.empty() && theEntry->mGridWidth != -1 && theEntry->mGridHeight != -1) {
		theEntry->mTransform.m02 = (float)(theEntry->mTransform.m02 -
			(aMemoryImage->mWidth - theEntry->mGridWidth * mImageScale) / (double)(aMemoryImage->mNumCols + 1));
		theEntry->mTransform.m12 = (float)(theEntry->mTransform.m12 -
			(aMemoryImage->mHeight - theEntry->mGridHeight * mImageScale) / (double)(aMemoryImage->mNumRows + 1));
	}
	theEntry->mImages.push_back(anImage);
	return true;
}

// 0x442590 (bejeweled3), slot 86. Fails, with mError, for a file that is missing, damaged or of a version this does
// not know (0 to 5).
bool PopAnim::LoadPam(const std::string& thePath)
{
	mPamFileName = thePath;
	if (mData != nullptr)
		return false;  // there must be no animation yet
	if (mMainInst == nullptr)
		mMainInst = new PASpriteInst();
	mMainInst->mParent = nullptr;
	mMainInst->mDef = nullptr;
	mData = new PAData();
	mDataUsers = 1;

	Buffer aBuffer;
	if (!gSexyAppBase->ReadBufferFromFile(thePath, &aBuffer, true)) {
		SetError("Unable to open file: " + thePath);
		return false;
	}
	if ((unsigned int)aBuffer.ReadLong() != 0xBAF01954) {
		SetError("Invalid header");
		return false;
	}
	mVersion = (int)aBuffer.ReadLong();
	if ((unsigned int)mVersion >= 6) {
		SetError("Invalid version");
		return false;
	}
	mFrameRate = aBuffer.ReadByte();
	// the bounds are in twentieths of a pixel
	mPamX = aBuffer.ReadShort() / 20;
	mPamY = aBuffer.ReadShort() / 20;
	mPamWidth = (aBuffer.ReadShort() & 0xffff) / 20;
	mPamHeight = (aBuffer.ReadShort() & 0xffff) / 20;

	int aNumImages = aBuffer.ReadShort();
	mImages.clear();
	mImages.resize(aNumImages > 0 ? aNumImages : 0);
	std::string aPamDir = GetFileDir(thePath);
	for (int i = 0; i < aNumImages; i++) {
		PAImage& anEntry = mImages[i];
		anEntry.mAdditive = false;
		std::string aName = RemapName(aBuffer.ReadString());

		// The name may carry options: in parentheses anywhere in it, or before a $ ("add$name" draws the image
		// additively); neither is part of the file's name
		std::string anOptions;
		size_t aLeft = aName.find('(');
		size_t aRight = aName.find(')');
		if (aLeft == std::string::npos || aRight == std::string::npos || aRight <= aLeft) {
			size_t aDollar = aName.find('$');
			if (aDollar != std::string::npos) {
				anOptions = StringToLower(aName.substr(0, aDollar));
				aName = Trim(aName.substr(aDollar + 1));
			}
		}
		else {
			anOptions = StringToLower(aName.substr(aLeft + 1, aRight - aLeft - 1));
			aName = Trim(aName.substr(0, aLeft) + aName.substr(aRight + 1));
		}

		// a grid of cels: "[columns,rows]"
		anEntry.mNumCols = 1;
		anEntry.mNumRows = 1;
		size_t aBracketLeft = aName.find('[');
		size_t aBracketRight = aName.find(']');
		if (aBracketLeft != std::string::npos && aBracketRight != std::string::npos && aBracketLeft < aBracketRight) {
			std::string aGrid = StringToLower(aName.substr(aBracketLeft + 1, aBracketRight - aBracketLeft - 1));
			aName = Trim(aName.substr(0, aBracketLeft) + aName.substr(aBracketRight + 1));
			size_t aComma = aGrid.find(',');
			if (aComma != std::string::npos) {
				anEntry.mNumCols = atoi(aGrid.substr(0, aComma).c_str());
				anEntry.mNumRows = atoi(aGrid.substr(aComma + 1).c_str());
			}
		}
		if (anOptions.find("add") != std::string::npos)
			anEntry.mAdditive = true;

		if (mVersion < 4) {
			anEntry.mGridWidth = -1;
			anEntry.mGridHeight = -1;
		}
		else {
			anEntry.mGridWidth = aBuffer.ReadShort();
			anEntry.mGridHeight = aBuffer.ReadShort();
		}

		// where the image sits in its sprite: a rotation and a position in the first version, a matrix and a
		// position in the others
		SexyMatrix3& anImageMatrix = anEntry.mTransform;
		anImageMatrix.LoadIdentity();
		if (mVersion == 1) {
			float aSin, aCos;
			RotationFromThousandths(aBuffer.ReadShort(), &aSin, &aCos);
			anImageMatrix.m00 = aCos;
			anImageMatrix.m01 = -aSin;
			anImageMatrix.m10 = aSin;
			anImageMatrix.m11 = aCos;
			anImageMatrix.m02 = (float)(aBuffer.ReadShort() / 20.0);
			anImageMatrix.m12 = (float)(aBuffer.ReadShort() / 20.0);
		}
		else {
			anImageMatrix.m00 = (float)((int)aBuffer.ReadLong() / 1310720.0);
			anImageMatrix.m01 = (float)((int)aBuffer.ReadLong() / 1310720.0);
			anImageMatrix.m10 = (float)((int)aBuffer.ReadLong() / 1310720.0);
			anImageMatrix.m11 = (float)((int)aBuffer.ReadLong() / 1310720.0);
			anImageMatrix.m02 = (float)(aBuffer.ReadShort() / 20.0);
			anImageMatrix.m12 = (float)(aBuffer.ReadShort() / 20.0);
		}
		anEntry.mName = aName;

		if (aName.empty()) {
			// a picture that is one white pixel
			bool aWhitePixelIsNew = false;
			SharedImageRef aWhitePixel = gSexyAppBase->GetSharedImage("!whitepixel", "", &aWhitePixelIsNew);
			MemoryImage* aWhiteImage = aWhitePixel;
			if (aWhiteImage != nullptr && aWhitePixelIsNew) {
				aWhiteImage->Create(1, 1);
				aWhiteImage->SetImageMode(false, false);
				aWhiteImage->GetBits()[0] = 0xFFFFFFFF;
				aWhiteImage->BitsChanged();
			}
			anEntry.mImages.push_back(aWhitePixel);
		}
		else {
			// the name may be several, which the objects pick from by their image frame
			size_t aStart = 0;
			while (aStart < aName.size()) {
				size_t aComma = aName.find(',', aStart);
				std::string aPart = aComma == std::string::npos ? aName.substr(aStart) : aName.substr(aStart, aComma - aStart);
				GetImage(aPamDir, thePath, &anEntry, aPart);
				if (aComma == std::string::npos)
					break;
				aStart = aComma + 1;
			}
		}
		if (!mError.empty())
			return false;
		ImageLoaded(anEntry);
	}

	int aNumSprites = aBuffer.ReadShort();
	mData->mSprites.resize(aNumSprites > 0 ? aNumSprites : 0);
	for (int i = 0; i < aNumSprites; i++) {
		if (!LoadSprite(aBuffer, &mData->mSprites[i]))
			return false;
	}
	if (mVersion < 4 || aBuffer.ReadBoolean()) {
		mData->mMain = new PASpriteDef();
		if (!LoadSprite(aBuffer, mData->mMain))
			return false;
	}
	mLoaded = true;
	mRandSeeded = false;
	return true;
}

// 0x441500 (bejeweled3): one sprite. Each frame is the changes from the frame before: the flags say which of these
// follow. The loader plays them on a table of the objects on screen, and keeps the table's contents as the frame.
bool PopAnim::LoadSprite(Buffer& theBuffer, PASpriteDef* theDef)
{
	std::map<int, PAObjectPos> anObjects;  // by object number, which is the drawing order
	if (mVersion < 4) {
		theDef->mFrameRate = (float)mFrameRate;
	}
	else {
		theDef->mName = theBuffer.ReadString();
		theDef->mFrameRate = (float)((int)theBuffer.ReadLong() / 65536.0);
	}
	int aNumFrames = theBuffer.ReadShort();
	if (mVersion < 5) {
		theDef->mWorkStart = 0;
		theDef->mWorkLength = aNumFrames - 1;
	}
	else {
		theDef->mWorkStart = theBuffer.ReadShort();
		theDef->mWorkLength = theBuffer.ReadShort();
	}
	// the work range stays inside the frames
	theDef->mWorkLength = std::min(theDef->mWorkStart + theDef->mWorkLength, aNumFrames - 1) - theDef->mWorkStart;
	theDef->mFrames.resize(aNumFrames > 0 ? aNumFrames : 0);

	for (int aFrameNum = 0; aFrameNum < aNumFrames; aFrameNum++) {
		PAFrame& aFrame = theDef->mFrames[aFrameNum];
		int aFlags = theBuffer.ReadByte();

		// 1: objects taken off the screen
		if (aFlags & 1) {
			int aCount = theBuffer.ReadByte();
			if (aCount == 0xff)
				aCount = (unsigned short)theBuffer.ReadShort();
			for (int i = 0; i < aCount; i++) {
				int anObjectNum = (unsigned short)theBuffer.ReadShort();
				if (anObjectNum > 0x7fe)
					anObjectNum = (int)theBuffer.ReadLong();
				anObjects.erase(anObjectNum);
			}
		}

		// 2: objects put on the screen; the high bit of the number says a sprite, the next that it is additive
		if (aFlags & 2) {
			int aCount = theBuffer.ReadByte();
			if (aCount == 0xff)
				aCount = (unsigned short)theBuffer.ReadShort();
			for (int i = 0; i < aCount; i++) {
				PAObjectPos aPos;
				unsigned int aNum = (unsigned short)theBuffer.ReadShort();
				aPos.mObjectNum = aNum & 0x7ff;
				if (aPos.mObjectNum == 0x7ff)
					aPos.mObjectNum = (int)theBuffer.ReadLong();
				aPos.mIsSprite = (aNum >> 15) != 0;
				aPos.mAdditive = ((aNum >> 14) & 1) != 0;
				aPos.mResNum = theBuffer.ReadByte();
				if (aNum & 0x2000)
					aPos.mPreloadFrames = theBuffer.ReadShort();
				if (aNum & 0x1000)
					aPos.mName = theBuffer.ReadString();
				if (aNum & 0x800)
					aPos.mTimeScale = (float)((int)theBuffer.ReadLong() / 65536.0);

				// the definition of the sprite in this slot (the last one put there is the one kept)
				if ((int)theDef->mObjectDefs.size() < aPos.mObjectNum + 1)
					theDef->mObjectDefs.resize(aPos.mObjectNum + 1);
				theDef->mObjectDefs[aPos.mObjectNum].mName = aPos.mName;
				if (aPos.mIsSprite) {
					if (aPos.mResNum >= (int)mData->mSprites.size()) {
						SetError("The .pam uses a sprite it does not have");
						return false;
					}
					theDef->mObjectDefs[aPos.mObjectNum].mSprite = &mData->mSprites[aPos.mResNum];
				}
				anObjects.insert(std::make_pair(aPos.mObjectNum, aPos));  // not an object that is there already
			}
		}

		// 4: objects moved. The flags: 0x1000 a matrix follows, else 0x4000 a rotation; then a position; 0x8000 part
		// of the image to draw; 0x2000 a color; 0x400 which picture. The transform is all given anew, a color only
		// when it is there.
		if (aFlags & 4) {
			int aCount = theBuffer.ReadByte();
			if (aCount == 0xff)
				aCount = (unsigned short)theBuffer.ReadShort();
			for (int i = 0; i < aCount; i++) {
				unsigned int aMoveFlags = (unsigned short)theBuffer.ReadShort();
				int anObjectNum = aMoveFlags & 0x3ff;
				if (anObjectNum == 0x3ff)
					anObjectNum = (int)theBuffer.ReadLong();
				std::map<int, PAObjectPos>::iterator anItr = anObjects.find(anObjectNum);
				PAObjectPos aMissing;
				PAObjectPos& aPos = anItr != anObjects.end() ? anItr->second : aMissing;  // a move of nothing

				SexyMatrix3 aMatrix;
				aMatrix.LoadIdentity();
				if (aMoveFlags & 0x1000) {
					aMatrix.m00 = (float)((int)theBuffer.ReadLong() * (1.0 / 65536.0));
					aMatrix.m01 = (float)((int)theBuffer.ReadLong() * (1.0 / 65536.0));
					aMatrix.m10 = (float)((int)theBuffer.ReadLong() * (1.0 / 65536.0));
					aMatrix.m11 = (float)((int)theBuffer.ReadLong() * (1.0 / 65536.0));
				}
				else if (aMoveFlags & 0x4000) {
					float aSin, aCos;
					RotationFromThousandths(theBuffer.ReadShort(), &aSin, &aCos);
					if (mVersion == 2)
						aSin = -aSin;
					aMatrix.m00 = aCos;
					aMatrix.m01 = -aSin;
					aMatrix.m10 = aSin;
					aMatrix.m11 = aCos;
				}
				SexyMatrix3 aTranslation;
				aTranslation.LoadIdentity();
				if (aMoveFlags & 0x800) {
					aTranslation.m02 = (float)((int)theBuffer.ReadLong() / 20.0);
					aTranslation.m12 = (float)((int)theBuffer.ReadLong() / 20.0);
				}
				else {
					aTranslation.m02 = (float)(theBuffer.ReadShort() / 20.0);
					aTranslation.m12 = (float)(theBuffer.ReadShort() / 20.0);
				}
				aPos.mTransform = MultiplyMatrices(aTranslation, aMatrix);

				aPos.mHasSrcRect = (aMoveFlags & 0x8000) != 0;
				if (aMoveFlags & 0x8000) {
					aPos.mSrcRect.mX = theBuffer.ReadShort() / 20;
					aPos.mSrcRect.mY = theBuffer.ReadShort() / 20;
					aPos.mSrcRect.mWidth = theBuffer.ReadShort() / 20;
					aPos.mSrcRect.mHeight = theBuffer.ReadShort() / 20;
				}
				if (aMoveFlags & 0x2000) {
					aPos.mColor.mRed = theBuffer.ReadByte();
					aPos.mColor.mGreen = theBuffer.ReadByte();
					aPos.mColor.mBlue = theBuffer.ReadByte();
					aPos.mColor.mAlpha = theBuffer.ReadByte();
				}
				if (aMoveFlags & 0x400)
					aPos.mImageFrame = theBuffer.ReadShort();
			}
		}

		// 8: a label for this frame (the first of a name stands)
		if (aFlags & 8)
			theDef->mLabels.insert(std::make_pair(RemapName(theBuffer.ReadString()), aFrameNum));

		// 0x10: the animation stops here
		if (aFlags & 0x10)
			aFrame.mHasStop = true;

		// 0x20: commands for the game
		if (aFlags & 0x20) {
			int aCount = theBuffer.ReadByte();
			aFrame.mCommands.resize(aCount);
			for (int i = 0; i < aCount; i++) {
				aFrame.mCommands[i].mCommand = RemapName(theBuffer.ReadString());
				aFrame.mCommands[i].mParam = RemapName(theBuffer.ReadString());
			}
		}

		// the objects on screen at this frame; the preloading is for the frame an object appears in only
		aFrame.mObjectPosVector.clear();
		for (std::map<int, PAObjectPos>::iterator anItr = anObjects.begin(); anItr != anObjects.end(); ++anItr) {
			aFrame.mObjectPosVector.push_back(anItr->second);
			anItr->second.mPreloadFrames = 0;
		}

		if (theBuffer.PastEnd()) {
			SetError("The .pam ends too soon");
			return false;
		}
	}
	return true;
}

// 0x4422b0 (bejeweled3): the instance of a sprite and, below it, of the sprites it holds. The slots are one per
// object slot the definition has; an instance for the main sprite also runs up to its work range.
void PopAnim::BuildInstance(PASpriteInst* theInst, PASpriteDef* theDef)
{
	theInst->mDelayFrames = 0;
	theInst->mFrameNum = 0;
	theInst->mRepeatCount = 0;
	theInst->mDef = theDef;
	theInst->mLastParentFrame = -1;
	theInst->mFrameChanged = true;
	for (size_t i = 0; i < theInst->mSlots.size(); i++)
		delete theInst->mSlots[i].mChild;
	theInst->mSlots.clear();
	theInst->mSlots.resize(theDef->mObjectDefs.size());
	for (size_t i = 0; i < theDef->mObjectDefs.size(); i++) {
		PASlot& aSlot = theInst->mSlots[i];
		aSlot.mName = theDef->mObjectDefs[i].mName;
		aSlot.mHasPrevious = false;
		if (theDef->mObjectDefs[i].mSprite != nullptr) {
			PASpriteInst* aChild = new PASpriteInst();
			aChild->mParent = theInst;
			BuildInstance(aChild, theDef->mObjectDefs[i].mSprite);
			aSlot.mChild = aChild;
		}
	}
	if (theInst == mMainInst)
		RunToWorkRange();
}

// 0x44a410 (bejeweled3): the main sprite is made when it is first wanted: the data's own main sprite, or the sprite
// of a file without one that has the given name (the first sprite when it has none by that name, and for an empty
// name "main"). False while there is none.
bool PopAnim::SetupMainSprite(const std::string& theName)
{
	if (mMainInst == nullptr || mData == nullptr)
		return false;
	if (mMainInst->mDef != nullptr && theName.empty())
		return true;
	if (mData->mMain != nullptr) {
		if (mMainInst->mDef == nullptr)
			BuildInstance(mMainInst, mData->mMain);
		return true;
	}
	if (mData->mSprites.empty())
		return false;
	std::string aName = theName.empty() ? "main" : theName;
	PASpriteDef* aDef = nullptr;
	for (size_t i = 0; i < mData->mSprites.size(); i++) {
		if (_stricmp(mData->mSprites[i].mName.c_str(), aName.c_str()) == 0) {
			aDef = &mData->mSprites[i];
			break;
		}
	}
	if (aDef == nullptr)
		aDef = &mData->mSprites[0];
	if (aDef != mMainInst->mDef) {
		if (mMainInst->mDef != nullptr) {
			delete mMainInst;
			mMainInst = new PASpriteInst();
		}
		BuildInstance(mMainInst, aDef);
		mTransformsStale = true;
	}
	return true;
}

// ---- playing ----

// 0x43f230 (bejeweled3)
int PopAnim::GetLabelFrame(const std::string& theLabel) const
{
	if (!mLoaded || mMainInst == nullptr || mMainInst->mDef == nullptr)
		return -1;
	std::map<std::string, int, StringLessNoCase>::const_iterator anItr = mMainInst->mDef->mLabels.find(theLabel);
	if (anItr == mMainInst->mDef->mLabels.end())
		return -1;
	return anItr->second;
}

// 0x445390 (bejeweled3) with 0x448770 (bejeweled3): playing, or holding an effect that has not finished
bool PopAnim::IsActive() const
{
	if (mPlaying)
		return true;
	// 0x448770 (bejeweled3): a sprite with an effect, or below one that has
	struct Local {
		static bool HasEffects(const PASpriteInst* theInst)
		{
			if (theInst == nullptr)
				return false;
			if (!theInst->mEffects.empty())
				return true;
			for (size_t i = 0; i < theInst->mSlots.size(); i++) {
				if (theInst->mSlots[i].mChild != nullptr && HasEffects(theInst->mSlots[i].mChild))
					return true;
			}
			return false;
		}
	};
	return Local::HasEffects(mMainInst);
}

// 0x444310 (bejeweled3): a sprite and what is below it back at the first frame, its effects restarted
void PopAnim::ResetSprite(PASpriteInst* theInst)
{
	theInst->mFrameNum = 0;
	theInst->mRepeatCount = 0;
	theInst->mDelayFrames = 0;
	theInst->mLastParentFrame = -1;
	theInst->mFrameChanged = true;
	for (size_t i = 0; i < theInst->mEffects.size(); i++)
		theInst->mEffects[i].mEffect->ResetAnim();
	for (size_t i = 0; i < theInst->mSlots.size(); i++) {
		if (theInst->mSlots[i].mChild != nullptr)
			ResetSprite(theInst->mSlots[i].mChild);
	}
	mTransformsStale = true;
}

// 0x4445f0 (bejeweled3): runs a main sprite that is before its work range up to it, an update (and a frame, for an
// app that updates by fractions) at a time with the animation playing and not paused
void PopAnim::RunToWorkRange()
{
	while (mMainInst != nullptr && mMainInst->mDef != nullptr &&
	       mMainInst->mFrameNum < (float)mMainInst->mDef->mWorkStart) {
		bool aWasPaused = mPaused;
		bool aWasPlaying = mPlaying;
		mPlaying = true;
		mPaused = false;
		Update();
		if (gSexyAppBase->mVSyncUpdates)
			UpdateF(1.0f);
		mPlaying = aWasPlaying;
		mPaused = aWasPaused;
	}
}

// 0x4445a0 (bejeweled3): back to the start, with no effects left
void PopAnim::ResetPlayback()
{
	ResetSprite(mMainInst);
	RemoveStaleEffects(mMainInst, true);
	mPlaying = false;
	RunToWorkRange();
	mTransitionTime = 0;
	mTransitionElapsed = 0;
	mStartDelay = 0;
}

// 0x444680 (bejeweled3)
bool PopAnim::PlayFrame(int theFrame, bool theReset)
{
	if (SetupMainSprite("")) {
		PASpriteDef* aDef = mMainInst->mDef;
		if (theFrame < (int)aDef->mFrames.size()) {
			if ((float)theFrame != mMainInst->mFrameNum && theReset)
				ResetPlayback();
			mPaused = false;
			mPlaying = true;
			mMainInst->mDelayFrames = 0;
			mMainInst->mFrameNum = (float)theFrame;
			mMainInst->mRepeatCount = 0;
			if (theReset)
				RemoveStaleEffects(mMainInst, true);
			if (mStartDelay == 0)
				FrameHitSprites(mMainInst, nullptr);
			return true;
		}
	}
	mPlaying = false;
	return false;
}

// 0x444800 (bejeweled3): in a file whose main sprite is chosen by name (versions 4 and 5 have none of their own) the
// label names the sprite and playing starts at its work range; otherwise it is a label of the main sprite
bool PopAnim::PlayLabel(const std::string& theLabel, bool theReset)
{
	mPlaying = false;
	int aFrame;
	if (mData == nullptr)
		return false;
	if (mData->mMain == nullptr) {
		if (!SetupMainSprite(theLabel))
			return false;
		aFrame = mMainInst->mDef->mWorkStart;
	}
	else {
		if (!SetupMainSprite("") || (aFrame = GetLabelFrame(theLabel)) == -1)
			return false;
	}
	mLabel = theLabel;
	return PlayFrame(aFrame, theReset);
}

// What a transition keeps of an object of the old animation, by its name (0x444910 (bejeweled3)): where it was drawn,
// in what color, and the particle effects of the sprite it was
struct TransitionEntry {
	SexyMatrix3 mTransform;
	Color mColor;
	std::vector<PAEffectLayer> mEffects;
	TransitionEntry() : mColor(Color::White) { mTransform.LoadIdentity(); }
};

// port: the effects a transition drops are the exe's own leaks made tidy: they are on no sprite any more
static void DeleteEffects(std::vector<PAEffectLayer>& theEffects)
{
	for (size_t i = 0; i < theEffects.size(); i++)
		delete theEffects[i].mEffect;
	theEffects.clear();
}

// 0x444910 (bejeweled3): like PlayLabel, but the objects the old animation and the new one both have (by name) move
// and fade from what they were drawn with to what the new label has, over theTransitionTime updates; an object the old
// one did not have starts at once. With a .pam that names its main sprite (versions 4 and 5) the label names the
// sprite, and the old sprite's instance is replaced. The particle effects of an old object go to the new object of
// the same name. False (with the old animation still playing) when there is no such label.
bool PopAnim::TransitionToLabel(const std::string& theLabel, int theTransitionTime)
{
	if (!SetupMainSprite(""))
		return false;
	if (mTransformsStale) {
		UpdateTransforms(mMainInst, nullptr, &mAnimColor, false);
		mTransformsStale = false;
	}

	// what each named object of the main sprite's frame is drawn with now
	std::map<std::string, TransitionEntry> aPrevious;
	PASpriteInst* anOldMain = mMainInst;
	PAFrame& anOldFrame = anOldMain->mDef->mFrames[(int)anOldMain->mFrameNum];
	for (size_t i = 0; i < anOldFrame.mObjectPosVector.size(); i++) {
		PAObjectPos& anObject = anOldFrame.mObjectPosVector[i];
		PASlot& aSlot = anOldMain->mSlots[anObject.mObjectNum];
		if (aSlot.mName.empty())
			continue;
		if (aPrevious.find(anObject.mName) != aPrevious.end())
			continue;  // a map keeps the first of a name
		TransitionEntry anEntry;
		if (!anObject.mIsSprite || aSlot.mChild == nullptr)
			BlendObject(anOldMain, (int)i, false, &anEntry.mTransform, &anEntry.mColor);
		else {
			anEntry.mTransform = aSlot.mChild->mTransform;
			anEntry.mColor = aSlot.mChild->mColor;
		}
		if (aSlot.mChild != nullptr)
			anEntry.mEffects.swap(aSlot.mChild->mEffects);
		aPrevious.insert(std::make_pair(anObject.mName, anEntry));
	}
	std::vector<PAEffectLayer> aMainEffects;
	aMainEffects.swap(anOldMain->mEffects);

	mTransitionTime = (float)theTransitionTime;
	mTransitionElapsed = 0;
	mStartDelay = 0;
	bool aFailed = false;
	if (mData->mMain == nullptr) {
		SetupMainSprite(theLabel);
		PlayFrame(mMainInst->mDef->mWorkStart, false);
	}
	else {
		int aFrame = SetupMainSprite("") ? GetLabelFrame(theLabel) : -1;
		if (aFrame == -1)
			aFailed = true;
		else {
			mLabel = theLabel;
			PlayFrame(aFrame, false);
			mTransformsStale = true;
		}
	}
	if (aFailed) {
		// the old animation goes on, with what was taken from it
		mMainInst->mEffects.swap(aMainEffects);
		for (size_t i = 0; i < anOldFrame.mObjectPosVector.size(); i++) {
			PAObjectPos& anObject = anOldFrame.mObjectPosVector[i];
			PASlot& aSlot = mMainInst->mSlots[anObject.mObjectNum];
			std::map<std::string, TransitionEntry>::iterator anItr = aPrevious.find(anObject.mName);
			if (anItr != aPrevious.end() && aSlot.mChild != nullptr) {
				aSlot.mChild->mEffects.swap(anItr->second.mEffects);
				aPrevious.erase(anItr);
			}
		}
		for (std::map<std::string, TransitionEntry>::iterator anItr = aPrevious.begin(); anItr != aPrevious.end(); ++anItr)
			DeleteEffects(anItr->second.mEffects);
		return false;
	}

	DeleteEffects(mMainInst->mEffects);
	mMainInst->mEffects.swap(aMainEffects);
	for (size_t i = 0; i < mMainInst->mSlots.size(); i++) {
		PASlot& aSlot = mMainInst->mSlots[i];
		if (aSlot.mName.empty())
			continue;
		std::map<std::string, TransitionEntry>::iterator anItr = aPrevious.find(aSlot.mName);
		if (anItr == aPrevious.end()) {
			aSlot.mHasPrevious = false;
			continue;
		}
		aSlot.mHasPrevious = true;
		aSlot.mPreviousColor = anItr->second.mColor;
		aSlot.mPreviousTransform = anItr->second.mTransform;
		if (aSlot.mChild == nullptr)
			DeleteEffects(anItr->second.mEffects);
		else {
			DeleteEffects(aSlot.mChild->mEffects);
			aSlot.mChild->mEffects.swap(anItr->second.mEffects);
		}
		aPrevious.erase(anItr);
	}
	for (std::map<std::string, TransitionEntry>::iterator anItr = aPrevious.begin(); anItr != aPrevious.end(); ++anItr)
		DeleteEffects(anItr->second.mEffects);
	return true;
}

// 0x44a2d0 (bejeweled3), slot 22
void PopAnim::Update()
{
	if (!mLoaded)
		return;
	Widget::Update();
	if (!SetupMainSprite(""))
		return;
	if (!gSexyAppBase->mVSyncUpdates)
		UpdateF(1.0f);
	UpdateTransforms(mMainInst, nullptr, &mAnimColor, false);
	mTransformsStale = false;
	if (!mPaused) {
		UpdateEffects(mMainInst, nullptr);
		RemoveStaleEffects(mMainInst, false);
	}
}

// 0x44a3f0 (bejeweled3), slot 24
void PopAnim::UpdateF(float theFrac)
{
	if (!mPaused)
		UpdateAnimation(theFrac);
}

// 0x448cc0 (bejeweled3): a step of theFrac of a frame of the app's
void PopAnim::UpdateAnimation(float theFrac)
{
	if (!mPlaying)
		return;
	if (0.0f < mTransitionTime) {
		mTransitionElapsed = theFrac + mTransitionElapsed;
		if (mTransitionTime < mTransitionElapsed)
			mTransitionTime = 0;
	}
	mTransformsStale = true;
	if (mStartDelay <= 0.0f) {
		AdvanceSprite(mMainInst, nullptr, theFrac);
		FrameEvents(mMainInst, nullptr);
		return;
	}
	mStartDelay = mStartDelay - theFrac;
	if (mStartDelay < 0.0f) {
		mStartDelay = 0;
		FrameHitSprites(mMainInst, nullptr);
	}
}

// 0x448830 (bejeweled3): moves a sprite on by theFrac of an update of the app's, which is the frame rate of its
// own and the time scale of the object it is in. A main sprite loops (or stops, on a last frame that stops) within
// its work range, the others loop in all their frames; a frame with a delay holds.
void PopAnim::AdvanceSprite(PASpriteInst* theInst, PAObjectPos* theObject, float theFrac)
{
	PASpriteDef* aDef = theInst->mDef;
	int anOldFrame = (int)theInst->mFrameNum;
	PAFrame& anOldFrameData = aDef->mFrames[anOldFrame];
	if (anOldFrameData.mHasStop)
		return;
	float aTimeScale = theObject == nullptr ? 1.0f : theObject->mTimeScale;
	theInst->mFrameNum = (float)(((aDef->mFrameRate / (1000.0 / (float)gSexyAppBase->mFrameTime)) * theFrac) / aTimeScale +
		theInst->mFrameNum);

	if (theInst == mMainInst) {
		if (aDef->mFrames.back().mHasStop) {
			if ((int)theInst->mFrameNum >= aDef->mWorkLength + aDef->mWorkStart) {
				theInst->mFrameChanged = true;
				theInst->mFrameNum = (float)(aDef->mWorkLength + aDef->mWorkStart);
				if (aDef->mWorkLength != 0) {
					mPlaying = false;
					if (mListener != nullptr)
						mListener->PopAnimStopped(mId);
					return;
				}
				theInst->mRepeatCount++;
			}
		}
		else if ((int)theInst->mFrameNum >= aDef->mWorkStart + aDef->mWorkLength + 1) {
			theInst->mRepeatCount++;
			theInst->mFrameNum = (float)(theInst->mFrameNum - (aDef->mWorkLength + 1));
		}
	}
	else if ((int)theInst->mFrameNum >= (int)aDef->mFrames.size()) {
		theInst->mRepeatCount++;
		theInst->mFrameNum = (float)(theInst->mFrameNum - (int)aDef->mFrames.size());
	}

	int aNewFrame = (int)theInst->mFrameNum;
	theInst->mFrameChanged = aNewFrame != anOldFrame;
	if (aNewFrame != anOldFrame && theInst->mDelayFrames > 0) {
		theInst->mFrameChanged = false;
		theInst->mDelayFrames--;
		theInst->mFrameNum = (float)anOldFrame;
		return;
	}

	// the sprites in the frame it left run on too
	for (size_t i = 0; i < anOldFrameData.mObjectPosVector.size(); i++) {
		PAObjectPos& anObject = anOldFrameData.mObjectPosVector[i];
		if (anObject.mIsSprite)
			AdvanceSprite(theInst->mSlots[anObject.mObjectNum].mChild, &anObject, theFrac / aTimeScale);
	}
}

// 0x448ae0 (bejeweled3): what reaching a frame does, for a sprite and below it: its commands when it moved to the
// frame, a sprite that was not in the parent's last frame starts again, a frame that stops stops the animation
void PopAnim::FrameEvents(PASpriteInst* theInst, PAObjectPos* theObject)
{
	PASpriteDef* aDef = theInst->mDef;
	PAFrame& aFrame = aDef->mFrames[(int)theInst->mFrameNum];
	if (theInst->mFrameChanged)
		FrameHit(theInst, aFrame, theObject);
	if (!aFrame.mHasStop) {
		for (size_t i = 0; i < aFrame.mObjectPosVector.size(); i++) {
			PAObjectPos& anObject = aFrame.mObjectPosVector[i];
			if (!anObject.mIsSprite)
				continue;
			PASpriteInst* aChild = theInst->mSlots[anObject.mObjectNum].mChild;
			if (aChild == nullptr)
				continue;
			int aParentFrames = (int)aDef->mFrames.size() * theInst->mRepeatCount + (int)theInst->mFrameNum;
			if (aChild->mLastParentFrame != aParentFrames - 1 && aChild->mLastParentFrame != aParentFrames) {
				aChild->mFrameNum = 0;
				aChild->mRepeatCount = 0;
				aChild->mDelayFrames = 0;
				aChild->mFrameChanged = true;
			}
			FrameEvents(aChild, &anObject);
			aChild->mLastParentFrame = aParentFrames;
		}
	}
	else if (theInst == mMainInst) {
		mPlaying = false;
		if (mListener != nullptr)
			mListener->PopAnimStopped(mId);
	}
}

// 0x447640 (bejeweled3): the commands of the current frame of a sprite and of what is below it
void PopAnim::FrameHitSprites(PASpriteInst* theInst, PAObjectPos* theObject)
{
	PAFrame& aFrame = theInst->mDef->mFrames[(int)theInst->mFrameNum];
	FrameHit(theInst, aFrame, theObject);
	for (size_t i = 0; i < aFrame.mObjectPosVector.size(); i++) {
		PAObjectPos& anObject = aFrame.mObjectPosVector[i];
		if (!anObject.mIsSprite)
			continue;
		PASpriteInst* aChild = theInst->mSlots[anObject.mObjectNum].mChild;
		if (aChild != nullptr)
			FrameHitSprites(aChild, &anObject);
	}
}

// Splits a command's argument at its commas
// port: helper (inlined in 0x445c00 (bejeweled3))
static void SplitParams(const std::string& theParam, std::vector<std::string>* theParts)
{
	size_t aStart = 0;
	while (true) {
		size_t aComma = theParam.find(',', aStart);
		if (aComma == std::string::npos) {
			theParts->push_back(theParam.substr(aStart));
			return;
		}
		theParts->push_back(theParam.substr(aStart, aComma - aStart));
		aStart = aComma + 1;
	}
}

// port: helper (0x44a780 (bejeweled3) in the exe takes the spaces out one by one)
static std::string WithoutSpaces(const std::string& theText)
{
	std::string aResult;
	for (size_t i = 0; i < theText.size(); i++) {
		if (theText[i] != ' ')
			aResult += theText[i];
	}
	return aResult;
}

// 0x445c00 (bejeweled3): what reaching a frame does. The objects that are sprites with frames to play ahead run
// ahead; then each command goes to the listener, and one it does not take is the animation's own: "delay"
// (frames to wait, or "from,to" for a random number of them), "playsample" (the sample, then "volume=" and "pan="
// as the listener's ones), "addparticleeffect" (the effect, then "x=", "y=", "attachemitter", "once", "behind" and
// "transform").
void PopAnim::FrameHit(PASpriteInst* theInst, PAFrame& theFrame, PAObjectPos* theObject)
{
	theInst->mFrameChanged = false;
	for (size_t i = 0; i < theFrame.mObjectPosVector.size(); i++) {
		PAObjectPos& anObject = theFrame.mObjectPosVector[i];
		if (!anObject.mIsSprite)
			continue;
		PASpriteInst* aChild = theInst->mSlots[anObject.mObjectNum].mChild;
		if (aChild != nullptr && theInst->mDef->mFrameRate != 0.0f) {
			for (int aStep = 0; aStep < anObject.mPreloadFrames; aStep++) {
				float aFrac = (float)((1000.0 / (float)gSexyAppBase->mFrameTime) / theInst->mDef->mFrameRate);
				AdvanceSprite(aChild, &anObject, aFrac);
			}
		}
	}

	for (size_t aCommandNum = 0; aCommandNum < theFrame.mCommands.size(); aCommandNum++) {
		const PACommand& aCommand = theFrame.mCommands[aCommandNum];
		if (mListener != nullptr &&
		    mListener->PopAnimCommand(mId, theInst, aCommand.mCommand, aCommand.mParam))
			continue;

		std::string aName = StringToLower(aCommand.mCommand);
		if (aName == "delay") {
			std::vector<std::string> aParts;
			SplitParams(aCommand.mParam, &aParts);
			if (aParts.size() < 2) {
				theInst->mDelayFrames = atoi(Trim(aParts[0]).c_str());
			}
			else {
				int aLow = atoi(Trim(aParts[0]).c_str());
				int aHigh = atoi(Trim(aParts[1]).c_str());
				if (aHigh <= aLow)
					aHigh = aLow + 1;
				theInst->mDelayFrames = Rand() % (aHigh - aLow) + aLow;
			}
		}
		else if (aName == "playsample") {
			std::vector<std::string> aParts;
			SplitParams(aCommand.mParam, &aParts);
			std::string aSample = aParts[0];
			double aVolume = 1.0;
			double aPan = 0.0;
			for (size_t i = 1; i < aParts.size(); i++) {
				std::string aPart = WithoutSpaces(aParts[i]);
				// "steps=" is read like "volume=" by the exe
				if (_strnicmp(aPart.c_str(), "volume=", 7) == 0)
					aVolume = atof(aPart.c_str() + 7);
				else if (_strnicmp(aPart.c_str(), "pan=", 4) == 0)
					aPan = atof(aPart.c_str() + 4);
				else if (_strnicmp(aPart.c_str(), "steps=", 6) == 0)
					aVolume = atof(aPart.c_str() + 6);
			}
			if (mListener != nullptr)
				mListener->PopAnimPlaySample(mId, aSample, aVolume, aPan);
		}
		else if (aName == "addparticleeffect") {
			std::vector<std::string> aParts;
			SplitParams(aCommand.mParam, &aParts);
			std::string anEffectName = Trim(aParts[0]);
			double aX = 0, aY = 0;
			bool anAttach = false, aOnce = false, aBehind = false, aFollows = false;
			for (size_t i = 1; i < aParts.size(); i++) {
				std::string aPart = WithoutSpaces(aParts[i]);
				if (_strnicmp(aPart.c_str(), "x=", 2) == 0)
					aX = atof(aPart.c_str() + 2);
				else if (_strnicmp(aPart.c_str(), "y=", 2) == 0)
					aY = atof(aPart.c_str() + 2);
				else if (_stricmp(aPart.c_str(), "attachemitter") == 0)
					anAttach = true;
				else if (_stricmp(aPart.c_str(), "once") == 0)
					aOnce = true;
				else if (_stricmp(aPart.c_str(), "behind") == 0)
					aBehind = true;
				else if (_stricmp(aPart.c_str(), "transform") == 0)
					aFollows = true;
			}
			if (aOnce) {
				bool aHasIt = false;
				for (size_t i = 0; i < theInst->mEffects.size(); i++) {
					if (theInst->mEffects[i].mName == anEffectName)
						aHasIt = true;
				}
				if (aHasIt)
					continue;
			}
			PIEffect* anEffect = mListener != nullptr ? mListener->PopAnimCreateEffect(mId, anEffectName) : nullptr;
			if (anEffect == nullptr)
				anEffect = MakeEffect(anEffectName);
			if (anEffect != nullptr) {
				PAEffectLayer aLayer;
				aLayer.mEffect = anEffect;
				aLayer.mName = anEffectName;
				aLayer.mUpdateStamp = mUpdateCnt;
				aLayer.mBehind = aBehind;
				aLayer.mAttachToEmitter = anAttach;
				aLayer.mFollowsSprite = aFollows;
				aLayer.mX = aX;
				aLayer.mY = aY;
				AddEffect(theInst, anEffect, aLayer, theObject);
			}
		}
	}
}

// port: the part of FrameHit (0x445c00 (bejeweled3)) that makes the effect when the listener does not. The resource
// manager's effect for the path of "<name>\<name>.ppf" beside the .pam (then the same one folder up) or for the id
// "PIEFFECT_<NAME>" is duplicated (0x567930 (bejeweled3)); without one, a new effect reads that .ppf, the one a folder up,
// the ".ip3" beside the .pam and the ".ip3" in each of the image folders (PIEffect::Load reads only ".ppf" files). Null when
// there is none (the effect is not made and the command does nothing)
PIEffect* PopAnim::MakeEffect(const std::string& theEffectName)
{
	std::string aPamDir = GetFileDir(mPamFileName);
	std::string aPath = GetPathFrom(theEffectName + "\\" + theEffectName, aPamDir);
	std::string aParentPath = GetPathFrom("..\\" + theEffectName + "\\" + theEffectName, aPamDir);

	PrimeResourceManager* aResourceManager =
		gSexyAppBase != nullptr ? dynamic_cast<PrimeResourceManager*>(gSexyAppBase->mResourceManager) : nullptr;
	if (aResourceManager != nullptr) {
		PIEffect* aResourceEffect = aResourceManager->FindPIEffectByPath(aPath + ".ppf");
		if (aResourceEffect == nullptr)
			aResourceEffect = aResourceManager->FindPIEffectByPath(aParentPath + ".ppf");
		if (aResourceEffect == nullptr)
			aResourceEffect = aResourceManager->FindPIEffect("PIEFFECT_" + StringToUpper(theEffectName));
		if (aResourceEffect != nullptr)
			return aResourceEffect->Duplicate();
	}

	PIEffect* anEffect = new PIEffect();
	anEffect->LoadEffect(aPath + ".ppf");
	if (anEffect->mError.empty())
		return anEffect;
	anEffect->LoadEffect(aParentPath + ".ppf");
	if (anEffect->mError.empty())
		return anEffect;
	anEffect->LoadEffect(aPath + ".ip3");
	if (anEffect->mError.empty())
		return anEffect;
	for (size_t i = 0; i < mImageFolders.size(); i++) {
		anEffect->LoadEffect(mImageFolders[i] + theEffectName + ".ip3");
		if (anEffect->mError.empty())
			return anEffect;
	}
	delete anEffect;
	return nullptr;
}

// port: the rest of FrameHit's "addparticleeffect" (0x445c00 (bejeweled3)): the first effect seeds the animation's own
// generator from the game's, and each effect is seeded from it (one of the effect's own seeds when it has some); an
// effect for an object that plays ahead is run as many updates as that is long (the frames, over the object's time
// scale and the sprite's frame rate, as hundredths); then it is put on the sprite
void PopAnim::AddEffect(PASpriteInst* theInst, PIEffect* theEffect, const PAEffectLayer& theLayer, PAObjectPos* theObject)
{
	if (!mRandSeeded) {
		mRandSeeded = true;
		mRand.SRand((unsigned long)Rand());
	}
	unsigned long aSeed;
	if (theEffect->mSeeds.empty())
		aSeed = mRand.NextNoAssert();
	else
		aSeed = (unsigned long)theEffect->mSeeds[mRand.NextNoAssert() % (unsigned long)theEffect->mSeeds.size()];
	theEffect->mRand.SRand(aSeed);
	theEffect->mNeedsSeed = false;
	if (theObject != nullptr && theInst->mDef->mFrameRate != 0.0f) {
		int anUpdateCount = Ftol((double)theObject->mPreloadFrames / (double)theObject->mTimeScale /
		                         (double)theInst->mDef->mFrameRate * 100.0 + 0.5);
		for (; anUpdateCount > 0; anUpdateCount--)
			theEffect->Update();
	}
	theInst->mEffects.push_back(theLayer);
}

// 0x4482a0 (bejeweled3): runs the effects of a sprite and of what is below it; one that has finished is taken off.
// Before each update the effect gets its matrix: where its layer is on the object (or at the origin when the object
// is not given, or the effect is attached to an emitter), turned over across the animation's middle when mirrored, and
// scaled by the draw scale; an effect that follows its sprite also runs at the speed of the object's time scale
void PopAnim::UpdateEffects(PASpriteInst* theInst, PAObjectPos* theObject)
{
	if (theInst == nullptr)
		return;
	for (size_t i = 0; i < theInst->mEffects.size(); i++) {
		PAEffectLayer& aLayer = theInst->mEffects[i];
		PIEffect* anEffect = aLayer.mEffect;
		PIPoint aPosition = {0.0, 0.0};
		// port: the exe reads the object without looking, which fails for the main sprite; here that is the origin
		if (!aLayer.mAttachToEmitter && theObject != nullptr)
			aPosition = PIMath::TransformPoint(theObject->mTransform, (float)aLayer.mX, (float)aLayer.mY);
		anEffect->mDrawTransform.LoadIdentity();
		PIMath::Translate(anEffect->mDrawTransform, (float)aPosition.mX, (float)aPosition.mY);
		if (mMirror) {
			PIMath::Translate(anEffect->mDrawTransform, (float)-(mPamWidth / 2), 0.0f);
			PIMath::Scale(anEffect->mDrawTransform, -1.0f, 1.0f);
			PIMath::Translate(anEffect->mDrawTransform, (float)(mPamWidth / 2), 0.0f);
		}
		PIMath::Scale(anEffect->mDrawTransform, mDrawScale, mDrawScale);
		if (aLayer.mFollowsSprite && theObject != nullptr)
			anEffect->mAnimSpeed = (float)(1.0 / (double)theObject->mTimeScale);
		anEffect->Update();
		aLayer.mUpdateStamp = mUpdateCnt;
		if (!aLayer.mEffect->IsActive()) {
			delete aLayer.mEffect;
			theInst->mEffects.erase(theInst->mEffects.begin() + i);
			i--;
		}
	}
	PAFrame& aFrame = theInst->mDef->mFrames[(int)theInst->mFrameNum];
	for (size_t i = 0; i < aFrame.mObjectPosVector.size(); i++) {
		PAObjectPos& anObject = aFrame.mObjectPosVector[i];
		if (anObject.mIsSprite)
			UpdateEffects(theInst->mSlots[anObject.mObjectNum].mChild, &anObject);
	}
}

// 0x4485e0 (bejeweled3): takes off the effects that were not updated this update (all of them with theAll)
void PopAnim::RemoveStaleEffects(PASpriteInst* theInst, bool theAll)
{
	if (theInst == nullptr)
		return;
	for (size_t i = 0; i < theInst->mEffects.size(); i++) {
		PAEffectLayer& aLayer = theInst->mEffects[i];
		if (aLayer.mUpdateStamp != mUpdateCnt || theAll) {
			delete aLayer.mEffect;
			theInst->mEffects.erase(theInst->mEffects.begin() + i);
			i--;
		}
	}
	for (size_t i = 0; i < theInst->mSlots.size(); i++) {
		if (theInst->mSlots[i].mChild != nullptr)
			RemoveStaleEffects(theInst->mSlots[i].mChild, theAll);
	}
}

// 0x447770 (bejeweled3): the transform and color of an object of a sprite's current frame. With interpolation on,
// they are the frame's and the next frame's object (of the same number) blended by how far the sprite is between
// the two frames; an object that is not in the next frame stays as it is, and one that moves too far for that to
// look right (a jump, by the way its position and its neighbours' differ) goes to whichever is nearer. The
// object's slot then puts its own transform on it.
void PopAnim::BlendObject(PASpriteInst* theInst, int theObjectIndex, bool theForceStill, SexyMatrix3* theMatrix,
                          Color* theColor) const
{
	PASpriteDef* aDef = theInst->mDef;
	int aFrameCount = (int)aDef->mFrames.size();
	int aFrameNum = (int)theInst->mFrameNum;
	PAFrame& aFrame = aDef->mFrames[aFrameNum];
	PAObjectPos& anObject = aFrame.mObjectPosVector[theObjectIndex];
	const PASlot& aSlot = theInst->mSlots[anObject.mObjectNum];

	// the frames before the current one, and after it twice
	int anOffsets[3] = {aFrameCount - 1, 1, 2};
	bool aInWorkRange = theInst == mMainInst && (float)aDef->mWorkStart < theInst->mFrameNum;
	if (aInWorkRange)
		anOffsets[0] = aDef->mWorkLength - 1;

	SexyMatrix3 aMatrix;
	Color aColor;
	if (!mInterpolate || theForceStill) {
		aMatrix = anObject.mTransform;
		aColor = anObject.mColor;
	}
	else {
		const PAObjectPos* aNeighbors[3] = {nullptr, nullptr, nullptr};
		for (int aNeighborIndex = 0; aNeighborIndex < 3; aNeighborIndex++) {
			unsigned int aNeighborFrame;
			if (aInWorkRange) {
				aNeighborFrame = ((aFrameNum - aDef->mWorkStart) + anOffsets[aNeighborIndex]) % (aDef->mWorkLength + 1) +
					aDef->mWorkStart;
			}
			else {
				aNeighborFrame = (unsigned int)(aFrameNum + anOffsets[aNeighborIndex]) % (unsigned int)aFrameCount;
			}
			const PAFrame* aNeighborFrameData = &aDef->mFrames[aNeighborFrame];
			if (aFrame.mHasStop)
				aNeighborFrameData = &aFrame;
			// the same index in the frame when it is the same object, else the object of that number
			if ((int)aNeighborFrameData->mObjectPosVector.size() > theObjectIndex) {
				aNeighbors[aNeighborIndex] = &aNeighborFrameData->mObjectPosVector[theObjectIndex];
				if (aNeighbors[aNeighborIndex]->mObjectNum != anObject.mObjectNum)
					aNeighbors[aNeighborIndex] = nullptr;
			}
			if (aNeighbors[aNeighborIndex] == nullptr) {
				for (size_t i = 0; i < aNeighborFrameData->mObjectPosVector.size(); i++) {
					if (aNeighborFrameData->mObjectPosVector[i].mObjectNum == anObject.mObjectNum) {
						aNeighbors[aNeighborIndex] = &aNeighborFrameData->mObjectPosVector[i];
						break;
					}
				}
			}
		}

		const PAObjectPos* aNext = aNeighbors[1];
		if (aNext == nullptr) {
			aMatrix = anObject.mTransform;
			aColor = anObject.mColor;
		}
		else {
			float aFrac = (float)((double)theInst->mFrameNum - (double)(int)theInst->mFrameNum);
			const PAObjectPos* aPrevious = aNeighbors[0];
			const PAObjectPos* aNextButOne = aNeighbors[2];
			if (aPrevious != nullptr && aNextButOne != nullptr) {
				// where each of the four puts the origin
				float aCurX = anObject.mTransform.m02, aCurY = anObject.mTransform.m12;
				float aPrevX = aPrevious->mTransform.m02, aPrevY = aPrevious->mTransform.m12;
				float aNextX = aNext->mTransform.m02, aNextY = aNext->mTransform.m12;
				float aFarX = aNextButOne->mTransform.m02, aFarY = aNextButOne->mTransform.m12;
				float aBackStepX = (float)((double)aCurX - aPrevX), aBackStepY = (float)((double)aCurY - aPrevY);
				float aStepX = (float)((double)aNextX - aCurX), aStepY = (float)((double)aNextY - aCurY);
				float aForwardStepX = (float)((double)aFarX - aNextX), aForwardStepY = (float)((double)aFarY - aNextY);
				float aJerkX = (float)((double)aForwardStepX - aStepX), aJerkY = (float)((double)aForwardStepY - aStepY);
				float aBackLength = (float)sqrt((double)(float)((double)aBackStepY * aBackStepY + (double)aBackStepX * aBackStepX));
				float aForwardLength = (float)sqrt((double)(float)((double)aForwardStepY * aForwardStepY +
					(double)aForwardStepX * aForwardStepX));
				float aLongest = aBackLength < aForwardLength ? aForwardLength : aBackLength;
				double aJerkLength = sqrt((double)(float)((double)aJerkY * aJerkY + (double)aJerkX * aJerkX));
				float aThreshold = (float)((double)aLongest * 0.5 + (double)aBackLength * 0.25 + (double)aForwardLength * 0.25);
				if ((double)aThreshold * 4.0 < aJerkLength)
					aFrac = 0.5f <= aFrac ? 1.0f : 0.0f;
			}
			aMatrix = InterpolateTransforms(anObject.mTransform, aNext->mTransform, aFrac);
			double aFromWeight = 1.0 - (double)aFrac;
			aColor.mRed = (int)((double)aNext->mColor.mRed * aFrac + (double)anObject.mColor.mRed * aFromWeight + 0.5);
			aColor.mGreen = (int)((double)aNext->mColor.mGreen * aFrac + (double)anObject.mColor.mGreen * aFromWeight + 0.5);
			aColor.mBlue = (int)((double)aNext->mColor.mBlue * aFrac + (double)anObject.mColor.mBlue * aFromWeight + 0.5);
			aColor.mAlpha = (int)((double)aNext->mColor.mAlpha * aFrac + (double)anObject.mColor.mAlpha * aFromWeight + 0.5);
		}
	}

	// the slot's own transform on it
	*theMatrix = MultiplyMatrices(aSlot.mTransform, aMatrix);
	*theColor = aColor;

	// during a transition (TransitionToLabel), a slot of the main sprite that the old animation also had goes from
	// what it was drawn with then to this, by how much of the transition's time has passed
	if (aSlot.mHasPrevious && mTransitionTime != 0.0f && theInst == mMainInst) {
		float aFrac = mTransitionElapsed / mTransitionTime;
		*theMatrix = InterpolateTransforms(aSlot.mPreviousTransform, *theMatrix, aFrac);
		double aFromWeight = 1.0 - (double)aFrac;
		const Color& aFrom = aSlot.mPreviousColor;
		theColor->mRed = (int)((double)aFrom.mRed * aFromWeight + (double)aColor.mRed * aFrac + 0.5);
		theColor->mGreen = (int)((double)aFrom.mGreen * aFromWeight + (double)aColor.mGreen * aFrac + 0.5);
		theColor->mBlue = (int)((double)aFrom.mBlue * aFromWeight + (double)aColor.mBlue * aFrac + 0.5);
		theColor->mAlpha = (int)((double)aFrom.mAlpha * aFromWeight + (double)aColor.mAlpha * aFrac + 0.5);
	}
}

// 0x447f70 (bejeweled3): works out where each sprite of the animation is, in the main sprite's space: a sprite's
// own transform and color are those of its object in the frame its parent is at (mTransform and the animation's
// color for the main sprite). Then the matrix of each effect attached to an emitter of this sprite: its own place on
// the sprite, or the middle of the effect when it follows the sprite's transform, put through the sprite's transform
void PopAnim::UpdateTransforms(PASpriteInst* theInst, const SexyMatrix3* theParentTransform, const Color* theParentColor,
                               bool theForceStill)
{
	theInst->mTransform = theParentTransform != nullptr ? *theParentTransform : mTransform;
	theInst->mColor = *theParentColor;
	PAFrame& aFrame = theInst->mDef->mFrames[(int)theInst->mFrameNum];
	bool aStill = theForceStill || theInst->mDelayFrames > 0 || aFrame.mHasStop;
	for (size_t i = 0; i < aFrame.mObjectPosVector.size(); i++) {
		PAObjectPos& anObject = aFrame.mObjectPosVector[i];
		if (!anObject.mIsSprite)
			continue;
		SexyMatrix3 aMatrix;
		Color aColor;
		BlendObject(theInst, (int)i, aStill, &aMatrix, &aColor);
		if (theParentTransform != nullptr)
			aMatrix = MultiplyTransforms(*theParentTransform, aMatrix);
		PASpriteInst* aChild = theInst->mSlots[anObject.mObjectNum].mChild;
		if (aChild != nullptr)
			UpdateTransforms(aChild, &aMatrix, &aColor, aStill);
	}
	for (size_t i = 0; i < theInst->mEffects.size(); i++) {
		PAEffectLayer& aLayer = theInst->mEffects[i];
		if (!aLayer.mAttachToEmitter)
			continue;
		PIEffect* anEffect = aLayer.mEffect;
		SexyMatrix3 aPlace;
		aPlace.LoadIdentity();
		if (aLayer.mFollowsSprite) {
			PIMath::Translate(aPlace, (float)(0.5 * (double)anEffect->mWidth), (float)(0.5 * (double)anEffect->mHeight));
			aPlace = PIMath::Multiply(theInst->mTransform, aPlace);
		}
		else {
			PIPoint aPosition = PIMath::TransformPoint(theInst->mTransform, (float)aLayer.mX, (float)aLayer.mY);
			PIMath::Translate(aPlace, (float)aPosition.mX, (float)aPosition.mY);
		}
		anEffect->mDrawTransform2 = aPlace;
		PIMath::Translate(anEffect->mDrawTransform2, (float)mEffectOffsetX, (float)mEffectOffsetY);
	}
}

// 0x444410 (bejeweled3): draws the effects of a sprite that are behind its objects (theInFront false) or in front.
// An effect that follows its sprite is put where the sprite is (the middle of the effect on the parent's transform, or
// at the origin, offset, when it is attached to an emitter) and takes the parent's color
void PopAnim::DrawEffects(Graphics* g, PASpriteInst* theInst, const SexyMatrix3* theParentTransform,
                          const Color* theParentColor, bool theInFront)
{
	for (size_t i = 0; i < theInst->mEffects.size(); i++) {
		PAEffectLayer& aLayer = theInst->mEffects[i];
		PIEffect* anEffect = aLayer.mEffect;
		if (aLayer.mFollowsSprite) {
			if (!aLayer.mAttachToEmitter) {
				SexyMatrix3 aPlace;
				aPlace.LoadIdentity();
				PIMath::Translate(aPlace, (float)(0.5 * (double)anEffect->mWidth), (float)(0.5 * (double)anEffect->mHeight));
				// port: the exe reads the parent's matrix without looking, which fails for the main sprite
				if (theParentTransform != nullptr)
					anEffect->mDrawTransform = PIMath::Multiply(*theParentTransform, aPlace);
				else
					anEffect->mDrawTransform = aPlace;
			}
			else {
				anEffect->mDrawTransform.LoadIdentity();
				PIMath::Translate(anEffect->mDrawTransform, (float)-mEffectOffsetX, (float)-mEffectOffsetY);
			}
			anEffect->mColor = *theParentColor;
		}
		if (aLayer.mBehind == !theInFront)
			anEffect->Draw(g);
	}
}

// 0x44a180 (bejeweled3), slot 26
void PopAnim::Draw(Graphics* g)
{
	if (!mLoaded)
		return;
	if (!SetupMainSprite(""))
		return;
	if (mTransformsStale) {
		UpdateTransforms(mMainInst, nullptr, &mAnimColor, false);
		mTransformsStale = false;
	}
	// port: mMirror would draw with DrawSpriteMirrored (0x448da0 (bejeweled3)), which is not translated
	DrawSprite(g, mMainInst, nullptr, &mAnimColor, mDrawAdditive, false);
}

// 0x449600 (bejeweled3), slot 81: the objects of the sprite's frame in their order. The listener may look at each
// before and after, and at an image's matrix.
void PopAnim::DrawSprite(Graphics* g, PASpriteInst* theInst, const SexyMatrix3* theParentTransform,
                         const Color* theParentColor, bool theAdditive, bool theForceStill)
{
	DrawEffects(g, theInst, theParentTransform, theParentColor, false);
	PAFrame& aFrame = theInst->mDef->mFrames[(int)theInst->mFrameNum];
	bool aStill = theForceStill || theInst->mDelayFrames > 0 || aFrame.mHasStop;

	for (size_t anIndex = 0; anIndex < aFrame.mObjectPosVector.size(); anIndex++) {
		PAObjectPos& anObject = aFrame.mObjectPosVector[anIndex];
		PASlot& aSlot = theInst->mSlots[anObject.mObjectNum];
		if (mListener != nullptr && aSlot.mWantsPreDraw)
			aSlot.mWantsPreDraw = mListener->PopAnimPreDraw(mId, g, theInst, &aSlot, theParentTransform, theParentColor);

		SexyMatrix3 anObjectMatrix;
		Color anObjectColor;
		if (!anObject.mIsSprite) {
			BlendObject(theInst, (int)anIndex, aStill, &anObjectMatrix, &anObjectColor);
		}
		else {
			PASpriteInst* aChild = aSlot.mChild;
			anObjectColor = aChild->mColor;
			anObjectMatrix = aChild->mTransform;
		}

		// where the object is on the screen: an image's own transform is in its sprite, which is where the parent's
		// transform puts it; a sprite's was worked out from the main sprite's space, so it only needs the animation's
		SexyMatrix3 aWorld;
		SexyMatrix3 aScale;
		aScale.LoadIdentity();
		aScale.m00 = mDrawScale;
		aScale.m11 = mDrawScale;
		if (theParentTransform != nullptr && !anObject.mIsSprite) {
			aWorld = MultiplyTransforms(*theParentTransform, anObjectMatrix);
		}
		else if (theParentTransform == nullptr && mDrawScale != 1.0f) {
			aWorld = MultiplyTransforms(MultiplyMatrices(mTransform, aScale), anObjectMatrix);
		}
		else {
			aWorld = anObjectMatrix;
			if (mDrawScale != 1.0f)
				aWorld = MultiplyMatrices(aScale, aWorld);
			aWorld = MultiplyMatrices(mTransform, aWorld);
		}

		Color aColor;
		aColor.mRed = (aSlot.mColor.mRed * theParentColor->mRed * anObjectColor.mRed) / 65025;
		aColor.mGreen = (aSlot.mColor.mGreen * theParentColor->mGreen * anObjectColor.mGreen) / 65025;
		aColor.mBlue = (aSlot.mColor.mBlue * theParentColor->mBlue * anObjectColor.mBlue) / 65025;
		aColor.mAlpha = (aSlot.mColor.mAlpha * theParentColor->mAlpha * anObjectColor.mAlpha) / 65025;

		if (aColor.mAlpha != 0) {
			if (!anObject.mIsSprite) {
				PAImage& anEntry = mImages[anObject.mResNum];
				int aListenerAnswer;
				do {
					SexyMatrix3 aMatrix = MultiplyTransforms(aWorld, anEntry.mTransform);
					g->SetColorizeImages(true);
					g->SetColor(aColor);
					g->SetDrawMode((theAdditive || anObject.mAdditive || anEntry.mAdditive) ? Graphics::DRAWMODE_ADDITIVE
					                                                                          : Graphics::DRAWMODE_NORMAL);

					// which picture, and which part of it
					Image* anImage;
					Rect aSrcRect;
					if (anObject.mImageFrame == 0 || anEntry.mImages.size() == 1) {
						anImage = anEntry.mImages[0];
						aSrcRect = anImage->GetCelRect(anObject.mImageFrame);
					}
					else {
						anImage = anEntry.mImages[anObject.mImageFrame];
						aSrcRect = anImage->GetCelRect(0);
					}
					if (anObject.mHasSrcRect)
						aSrcRect = anObject.mSrcRect;

					// the pictures are made at mImageScale of the base size: this puts them at it
					if (mImageScale != 1.0f) {
						SexyMatrix3 aScale;
						aScale.LoadIdentity();
						aScale.m00 = (float)(1.0 / mImageScale);
						aScale.m11 = aScale.m00;
						float aKeepX = aMatrix.m02, aKeepY = aMatrix.m12;
						aMatrix = MultiplyTransforms(aScale, aMatrix);
						aMatrix.m02 = aKeepX;
						aMatrix.m12 = aKeepY;
					}

					aListenerAnswer = 0;
					if (mListener != nullptr && aSlot.mWantsImageDraw) {
						aListenerAnswer = mListener->PopAnimDrawImage(mId, theInst, &aSlot, &aMatrix, anImage);
						if (aListenerAnswer == 0)
							aSlot.mWantsImageDraw = false;
						if (aListenerAnswer == 2)
							break;
					}

					bool aScaledOrRotated = 0.001 <= fabs(aMatrix.m00 - 1.0) || 0.001 <= fabs(aMatrix.m01 - 0.0) ||
						0.001 <= fabs(aMatrix.m10 - 0.0) || 0.001 <= fabs(aMatrix.m11 - 1.0);
					if (aScaledOrRotated) {
						if (mVersion == 1 || (aMatrix.m00 == aMatrix.m11 && -aMatrix.m10 == aMatrix.m01 &&
						                      fabs((double)(aMatrix.m00 * aMatrix.m00 + aMatrix.m01 * aMatrix.m01) - 1.0) < 0.01)) {
							// only turned: about the middle of the part of the picture drawn
							double anAngle = atan2((double)aMatrix.m01, (double)aMatrix.m00);
							double aTurn = -anAngle;
							double aCos = cos(aTurn);
							double aSin = sin(aTurn);
							float aWidth = (float)aSrcRect.mWidth;
							float aHeight = (float)aSrcRect.mHeight;
							float aX = (float)((((double)aMatrix.m02 + aWidth * aCos * 0.5) - aHeight * aSin * 0.5) - aWidth * 0.5);
							float aY = (float)((((double)aMatrix.m12 + aWidth * aSin * 0.5) + aHeight * aCos * 0.5) - aHeight * 0.5);
							g->DrawImageRotatedF(anImage, aX, aY, (double)(float)anAngle, &aSrcRect);
						}
						else {
							SexyMatrix3 aCenter;
							aCenter.LoadIdentity();
							aCenter.m02 = (float)(aSrcRect.mWidth * 0.5);
							aCenter.m12 = (float)(aSrcRect.mHeight * 0.5);
							SexyMatrix3 aDrawMatrix = MultiplyMatrices(aMatrix, aCenter);
							g->DrawImageMatrix(anImage, aDrawMatrix, aSrcRect, 0, 0);
						}
					}
					else {
						int aWholeX = (int)aMatrix.m02;
						int aWholeY = (int)aMatrix.m12;
						if (fabs((double)aMatrix.m02 - aWholeX) < 0.001 && fabs((double)aMatrix.m12 - aWholeY) < 0.001)
							g->DrawImage(anImage, aWholeX, aWholeY, aSrcRect);
						else
							g->DrawImageF(anImage, aMatrix.m02, aMatrix.m12, aSrcRect);
					}
				} while (aListenerAnswer == 3);
			}
			else {
				DrawSprite(g, aSlot.mChild, &aWorld, &aColor, theAdditive || anObject.mAdditive, aStill);
			}
		}
		if (mListener != nullptr && aSlot.mWantsPostDraw)
			aSlot.mWantsPostDraw = mListener->PopAnimPostDraw(mId, g, theInst, &aSlot);
	}

	DrawEffects(g, theInst, theParentTransform, theParentColor, true);
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}
