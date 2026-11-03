#include "PopAnim.h"

#include <math.h>
#include <stdlib.h>
#include <algorithm>

#include "Buffer.h"
#include "Common.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "PopAnimModParser.h"
#include "PopAnimNames.h"
#include "SexyAppBase.h"

using namespace Sexy;

// ---- the data (PopAnimData.h) ----

PAObjectPos::PAObjectPos() : mColor(Color::White)
{
	mTransform.LoadIdentity();
}

PAImage::PAImage()
{
	mTransform.LoadIdentity();
}

// 0x529e10, 0x529d60
PASpriteInst::~PASpriteInst()
{
	for (size_t i = 0; i < mChildren.size(); i++)
		delete mChildren[i];
}

// ---- transforms ----

// 0x560650: theLeft * theRight as a 3x3 matrix product. Each sum is rounded to a float as it grows, which is what
// the x87 code does and not what SexyMatrix3's operator* does.
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

// 0x529ba0: theParent * theChild, for the affine matrices of an animation (the last row stays 0 0 1). The x87
// of the original adds in extended precision, so this adds in double.
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

// 0x529c10: the transform a fraction theFrac of the way from theFrom to theTo
static SexyMatrix3 InterpolateTransforms(const SexyMatrix3& theFrom, const SexyMatrix3& theTo, float theFrac)
{
	double aFrom = 1.0 - (double)theFrac;
	SexyMatrix3 r;
	r.LoadIdentity();
	r.m00 = (float)((double)theTo.m00 * theFrac + (double)theFrom.m00 * aFrom);
	r.m01 = (float)((double)theTo.m01 * theFrac + (double)theFrom.m01 * aFrom);
	r.m10 = (float)((double)theTo.m10 * theFrac + (double)theFrom.m10 * aFrom);
	r.m11 = (float)((double)theTo.m11 * theFrac + (double)theFrom.m11 * aFrom);
	r.m02 = (float)((double)theTo.m02 * theFrac + (double)theFrom.m02 * aFrom);
	r.m12 = (float)((double)theTo.m12 * theFrac + (double)theFrom.m12 * aFrom);
	return r;
}

// The rotation by an angle that the file gives in thousandths of a radian, in the form the matrices have
// (x' = cos x - sin y, y' = sin x + cos y). The angle goes through a float, as in the exe.
static void RotationFromThousandths(int theThousandths, float* theSin, float* theCos)
{
	float anAngle = (float)(theThousandths / 1000.0);
	*theSin = (float)sin((double)anAngle);
	*theCos = (float)cos((double)anAngle);
}

// ---- the PopAnim ----

// 0x52ac30
PopAnim::PopAnim(int theId, PopAnimListener* theListener) : mAnimColor(Color::White)
{
	mId = theId;
	mListener = theListener;
	mImageFolders.push_back("");
	mImageFolders.push_back("images\\");
	mVersion = 0;
	mLoaded = false;
	mFraction = 0;
	mFrameRate = 0;
	mPamX = mPamY = mPamWidth = mPamHeight = 0;
	mRoot = NULL;
	mPlaying = false;
	mMirror = false;
	mInterpolate = true;
	mImageIsNew = false;
}

// 0x52ae50
PopAnim::~PopAnim()
{
	Reset();
}

// 0x52b000: the animation is gone, but for what the game set up (the id, the listener, the image folders and
// whether to interpolate)
void PopAnim::Reset()
{
	mMirror = false;
	mAnimColor = Color::White;
	mFraction = 0;
	mLoaded = false;
	mPlaying = false;
	mFrameRate = 0;
	mError.clear();
	mImages.clear();
	mPamFile.clear();
	mRemaps.clear();
	mMissingImages.clear();
	mData.reset();
	delete mRoot;
	mRoot = NULL;
}

// 0x52b500: what a load that failed says; the callers return the false it stands for
void PopAnim::SetError(const std::string& theError)
{
	mError = theError;
}

// 0x52b220, with 0x52b3f0 for the Widget part. What is copied is the state: the images are shared by reference,
// the definitions in mData by pointer, and the root is the caller's to make.
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
	mLoaded = theOther->mLoaded;
	mPamFile = theOther->mPamFile;
	mPamFileName = theOther->mPamFileName;
	mRemaps = theOther->mRemaps;
	mFraction = theOther->mFraction;
	mFrameRate = theOther->mFrameRate;
	mPamX = theOther->mPamX;
	mPamY = theOther->mPamY;
	mPamWidth = theOther->mPamWidth;
	mPamHeight = theOther->mPamHeight;
	mError = theOther->mError;
	mLabel = theOther->mLabel;
	mImages = theOther->mImages;
	mMissingImages = theOther->mMissingImages;
	mData = theOther->mData;
	mAnimColor = theOther->mAnimColor;
	mPlaying = theOther->mPlaying;
	mMirror = theOther->mMirror;
	mInterpolate = theOther->mInterpolate;
	mImageIsNew = theOther->mImageIsNew;
}

// 0x52b150
PopAnim* PopAnim::Clone() const
{
	PopAnim* aClone = new PopAnim(mId, mListener);
	aClone->CopyFrom(this);
	// its own place in the animation, at the first frame
	if (aClone->mData != NULL) {
		aClone->mRoot = new PASpriteInst();
		aClone->BuildInstances(aClone->mRoot, &aClone->mData->mMain);
	}
	return aClone;
}

// 0x52c070: the instances of a sprite and, below it, of the sprites it holds. An object slot that only held
// images has no definition and gets an instance that does nothing (the exe has no such case in its files).
void PopAnim::BuildInstances(PASpriteInst* theInst, PASpriteDef* theDef)
{
	theInst->mFrameNum = 0;
	theInst->mDef = theDef;
	if (theDef == NULL)
		return;
	theInst->mChildren.assign(theDef->mObjectDefs.size(), (PASpriteInst*)NULL);
	for (size_t i = 0; i < theDef->mObjectDefs.size(); i++) {
		PASpriteInst* aChild = new PASpriteInst();
		BuildInstances(aChild, theDef->mObjectDefs[i]);
		theInst->mChildren[i] = aChild;
	}
}

// 0x52b520: what the mod file's Remap commands make of a name from the .pam; the first that matches decides
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

// 0x52d4d0
bool PopAnim::LoadAnimation(const std::string& theFileName)
{
	Reset();
	mMirror = false;
	// the extension starts at the first dot of the name
	std::string anExt;
	size_t aDot = theFileName.find('.');
	if (aDot != std::string::npos)
		anExt = StringToLower(theFileName.substr(aDot));

	if (anExt == ".pam")
		return LoadPam(theFileName);
	if (anExt == ".txt") {
		if (LoadMod(theFileName))
			return true;
		if (mError.empty())
			SetError("The mod file could not be loaded");
		return false;
	}
	if (anExt.empty()) {
		if (LoadPam(theFileName + ".pam"))
			return true;
		return LoadMod(theFileName + ".txt");
	}
	SetError("Not a .pam or .txt file: " + theFileName);
	return false;
}

// 0x52cd00: reads the mod file for the .pam and the renames, loads the .pam, reads the mod file again for what
// changes the loaded animation
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

// slot 70, 0x52d090
SharedImageRef PopAnim::LoadImage(const std::string& thePamDir, const std::string& thePamFile,
                                  const std::string& theImageName)
{
	if (theImageName.empty()) {
		SetError("The .pam gives an image no name (" + thePamFile + ")");
		return SharedImageRef();
	}
	for (size_t i = 0; i < mImageFolders.size(); i++) {
		std::string aPath = GetPathFrom(mImageFolders[i], thePamDir) + "\\" + theImageName;
		SharedImageRef anImage = gSexyAppBase->GetSharedImage(aPath, mMirror ? "MIRRORED" : "", &mImageIsNew);
		MemoryImage* aMemoryImage = anImage;
		if (aMemoryImage != NULL) {
			aMemoryImage->Palletize();
			return anImage;
		}
	}
	return SharedImageRef();
}

// slot 71, 0x52d4c0
void PopAnim::ImageLoaded(SharedImageRef /*theImage*/)
{
}

// 0x52c280: the path comes as the string the method is called on, the PopAnim on the stack. Fails, with mError,
// for a file that is missing, damaged or of a version this does not know (0 to 3).
bool PopAnim::LoadPam(const std::string& thePath)
{
	mPamFileName = thePath;
	if (mData != NULL)
		return false;  // there must be no animation yet
	mData.reset(new PAData());

	Buffer aBuffer;
	if (!gSexyAppBase->ReadBufferFromFile(thePath, &aBuffer, true)) {
		SetError("Cannot read " + thePath);
		return false;
	}
	if ((unsigned int)aBuffer.ReadLong() != 0xBAF01954) {
		SetError("Not a .pam file: " + thePath);
		return false;
	}
	mVersion = (int)aBuffer.ReadLong();
	if ((unsigned int)mVersion > 3) {
		SetError("Unknown .pam version " + StrFormat("%d", mVersion) + ": " + thePath);
		return false;
	}
	mFrameRate = aBuffer.ReadByte();
	// the bounds are in twentieths of a pixel
	mPamX = aBuffer.ReadShort() / 20;
	mPamY = aBuffer.ReadShort() / 20;
	mPamWidth = aBuffer.ReadShort() / 20;
	mPamHeight = aBuffer.ReadShort() / 20;

	int aNumImages = aBuffer.ReadShort();
	mImages.clear();
	mImages.resize(aNumImages > 0 ? aNumImages : 0);
	std::string aPamDir = GetFileDir(thePath);
	for (int i = 0; i < aNumImages; i++) {
		PAImage& anImage = mImages[i];
		anImage.mDrawMode = 0;
		std::string aName = RemapName(aBuffer.ReadString());
		// The name may carry options in parentheses, which are not part of the file's name: "(add)" draws the image
		// additively
		size_t aLeft = aName.find('(');
		size_t aRight = aName.find(')');
		if (aLeft != std::string::npos && aRight != std::string::npos && aLeft < aRight) {
			if (aName.substr(aLeft + 1, aRight - aLeft - 1).find("add") != std::string::npos)
				anImage.mDrawMode = 1;
			aName = aName.substr(0, aLeft) + aName.substr(aRight + 1);
		}
		anImage.mName = aName;

		anImage.mImage = LoadImage(aPamDir, thePath, aName);
		if (!mError.empty())
			return false;
		MemoryImage* aMemoryImage = anImage.mImage;
		if (aMemoryImage != NULL)
			ImageLoaded(anImage.mImage);
		else
			mMissingImages.push_back(aName);

		// where the image sits in its sprite: a rotation and a position in the first version, a matrix and a
		// position in the others
		SexyMatrix3& m = anImage.mTransform;
		m.LoadIdentity();
		if (mVersion == 1) {
			float aSin, aCos;
			RotationFromThousandths(aBuffer.ReadShort(), &aSin, &aCos);
			m.m00 = aCos;
			m.m01 = -aSin;
			m.m10 = aSin;
			m.m11 = aCos;
			m.m02 = (float)(aBuffer.ReadShort() / 20.0);
			m.m12 = (float)(aBuffer.ReadShort() / 20.0);
		}
		else {
			m.m00 = (float)((int)aBuffer.ReadLong() / 1310720.0);
			m.m01 = (float)((int)aBuffer.ReadLong() / 1310720.0);
			m.m10 = (float)((int)aBuffer.ReadLong() / 1310720.0);
			m.m11 = (float)((int)aBuffer.ReadLong() / 1310720.0);
			m.m02 = (float)(aBuffer.ReadShort() / 20.0);
			m.m12 = (float)(aBuffer.ReadShort() / 20.0);
		}
	}

	int aNumSprites = aBuffer.ReadShort();
	mData->mSprites.resize(aNumSprites > 0 ? aNumSprites : 0);
	for (int i = 0; i < aNumSprites; i++) {
		if (!LoadSprite(aBuffer, &mData->mSprites[i]))
			return false;
	}
	if (!LoadSprite(aBuffer, &mData->mMain))
		return false;

	mRoot = new PASpriteInst();
	BuildInstances(mRoot, &mData->mMain);
	mLoaded = true;
	return true;
}

// 0x52b5d0: one sprite. Each frame is the changes from the frame before: the flags say which of these follow.
// The loader plays them on a table of the objects on screen, and keeps the table's contents as the frame.
bool PopAnim::LoadSprite(Buffer& theBuffer, PASpriteDef* theDef)
{
	std::map<int, PAObjectPos> anObjects;  // by object number, which is the drawing order
	int aNumFrames = theBuffer.ReadShort();
	theDef->mFrames.resize(aNumFrames > 0 ? aNumFrames : 0);

	for (int aFrameNum = 0; aFrameNum < aNumFrames; aFrameNum++) {
		PAFrame& aFrame = theDef->mFrames[aFrameNum];
		int aFlags = theBuffer.ReadByte();

		// 1: objects taken off the screen
		if (aFlags & 1) {
			int aCount = theBuffer.ReadByte();
			for (int i = 0; i < aCount; i++)
				anObjects.erase((int)theBuffer.ReadShort());
		}

		// 2: objects put on the screen; the high bit of the number says a sprite, the next byte which one
		if (aFlags & 2) {
			int aCount = theBuffer.ReadByte();
			for (int i = 0; i < aCount; i++) {
				PAObjectPos aPos;
				unsigned int aNum = (unsigned short)theBuffer.ReadShort();
				aPos.mObjectNum = aNum & 0x7fff;
				aPos.mIsSprite = (aNum >> 15) != 0;
				aPos.mResNum = theBuffer.ReadByte();
				if (aPos.mIsSprite) {
					// the definition of the sprite in this slot (the last one put there is the one kept)
					if (aPos.mResNum >= (int)mData->mSprites.size()) {
						SetError("The .pam uses a sprite it does not have");
						return false;
					}
					if ((int)theDef->mObjectDefs.size() < aPos.mObjectNum + 1)
						theDef->mObjectDefs.resize(aPos.mObjectNum + 1, (PASpriteDef*)NULL);
					theDef->mObjectDefs[aPos.mObjectNum] = &mData->mSprites[aPos.mResNum];
				}
				anObjects.insert(std::make_pair(aPos.mObjectNum, aPos));  // not an object that is there already
			}
		}

		// 4: objects moved. The flags: 0x1000 a matrix follows, else 0x4000 a rotation; then a position; 0x8000 part
		// of the image to draw; 0x2000 a color. The transform is all given anew, a color only when it is there.
		if (aFlags & 4) {
			int aCount = theBuffer.ReadByte();
			for (int i = 0; i < aCount; i++) {
				unsigned int aMoveFlags = (unsigned short)theBuffer.ReadShort();
				std::map<int, PAObjectPos>::iterator anItr = anObjects.find(aMoveFlags & 0xfff);
				PAObjectPos dummy;
				PAObjectPos& aPos = anItr != anObjects.end() ? anItr->second : dummy;  // a move of nothing: the exe stops

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
				aTranslation.m02 = (float)(theBuffer.ReadShort() / 20.0);
				aTranslation.m12 = (float)(theBuffer.ReadShort() / 20.0);
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

		// the objects on screen at this frame
		aFrame.mObjectPosVector.clear();
		for (std::map<int, PAObjectPos>::iterator anItr = anObjects.begin(); anItr != anObjects.end(); ++anItr)
			aFrame.mObjectPosVector.push_back(anItr->second);

		if (theBuffer.PastEnd()) {
			SetError("The .pam ends too soon");
			return false;
		}
	}
	return true;
}

// ---- playing ----

// 0x529c80 and 0x52ea00
int PopAnim::GetLabelFrame(const std::string& theLabel) const
{
	if (!mLoaded)
		return -1;
	std::map<std::string, int, StringLessNoCase>::const_iterator anItr = mData->mMain.mLabels.find(theLabel);
	if (anItr == mData->mMain.mLabels.end())
		return -1;
	return anItr->second;
}

// 0x52d8c0: what reaching a frame tells the game. A frame that stops the animation (in any sprite) stops it
// all, and the commands go out in their order.
void PopAnim::SendFrameEvents(const PAFrame& theFrame)
{
	if (theFrame.mHasStop) {
		mPlaying = false;
		if (mListener != NULL)
			mListener->PopAnimStopped(mId);
	}
	for (size_t i = 0; i < theFrame.mCommands.size(); i++) {
		if (mListener != NULL)
			mListener->PopAnimCommand(mId, theFrame.mCommands[i].mCommand, theFrame.mCommands[i].mParam);
	}
}

// 0x52d7a0
bool PopAnim::PlayFrame(int theFrame)
{
	if (!mLoaded || theFrame < 0 || theFrame >= (int)mData->mMain.mFrames.size()) {
		mPlaying = false;
		return false;
	}
	mPlaying = true;
	mRoot->mFrameNum = theFrame;
	SendFrameEvents(mData->mMain.mFrames[mRoot->mFrameNum]);
	MarkDirty();
	return true;
}

// 0x52d850
bool PopAnim::PlayLabel(const std::string& theLabel)
{
	mPlaying = false;
	int aFrame = GetLabelFrame(theLabel);
	if (aFrame == -1)
		return false;
	mLabel = theLabel;
	return PlayFrame(aFrame);
}

// 0x52d730
void PopAnim::ResetSprite(PASpriteInst* theInst)
{
	theInst->mFrameNum = 0;
	for (size_t i = 0; i < theInst->mChildren.size(); i++) {
		if (theInst->mChildren[i] != NULL)
			ResetSprite(theInst->mChildren[i]);
	}
}

// 0x52d790
void PopAnim::ResetAnim()
{
	if (mRoot != NULL)
		ResetSprite(mRoot);
}

// 0x52d970: a sprite moves on one frame (the last frame is followed by the first), and so do the sprites in the
// frame it leaves. A sprite that was not in that frame goes back to its first.
void PopAnim::AdvanceSprite(PASpriteInst* theInst)
{
	PASpriteDef* aDef = theInst->mDef;
	if (aDef == NULL || aDef->mFrames.empty())
		return;
	const PAFrame& aOldFrame = aDef->mFrames[theInst->mFrameNum];
	theInst->mFrameNum = (theInst->mFrameNum + 1) % (int)aDef->mFrames.size();
	SendFrameEvents(aDef->mFrames[theInst->mFrameNum]);

	int aSlot = -1;
	for (size_t i = 0; i < aOldFrame.mObjectPosVector.size(); i++) {
		const PAObjectPos& aPos = aOldFrame.mObjectPosVector[i];
		if (!aPos.mIsSprite)
			continue;
		aSlot++;
		for (; aSlot < aPos.mObjectNum && aSlot < (int)theInst->mChildren.size(); aSlot++)
			theInst->mChildren[aSlot]->mFrameNum = 0;
		aSlot = aPos.mObjectNum;
		if (aSlot < (int)theInst->mChildren.size())
			AdvanceSprite(theInst->mChildren[aSlot]);
	}
	for (aSlot++; aSlot < (int)theInst->mChildren.size(); aSlot++)
		theInst->mChildren[aSlot]->mFrameNum = 0;
}

// 0x52db50
void PopAnim::NextFrame()
{
	if (!mPlaying || mRoot == NULL)
		return;
	AdvanceSprite(mRoot);
	MarkDirty();
}

// 0x52e960
void PopAnim::Update()
{
	if (!mLoaded)
		return;
	Widget::Update();
	// with updates in step with the display, the app calls UpdateF itself
	if (!gSexyAppBase->mVSyncUpdates)
		UpdateF(1.0f);
}

// 0x52e990: the frames that theFrac of an update's time (mFrameTime milliseconds) is worth add up in
// mFraction, and each whole one moves the animation on
void PopAnim::UpdateF(float theFrac)
{
	double aFrames = ((double)gSexyAppBase->mFrameTime * theFrac) / (1000.0 / (double)mFrameRate);
	mFraction = (float)(aFrames + mFraction);
	while (mFraction > 1.0f) {
		NextFrame();
		mFraction = (float)(mFraction - 1.0);
	}
}

// 0x52d890
void PopAnim::SetAnimColor(const Color& theColor)
{
	mAnimColor = theColor;
	MarkDirty();
}

// ---- drawing ----

// 0x52e900
void PopAnim::Draw(Graphics* g)
{
	if (!mLoaded || mRoot == NULL)
		return;
	DrawSprite(g, mRoot, NULL, mAnimColor);
}

// 0x52e290: a sprite at its frame. Its objects are drawn in order, each moved by its own transform on the way
// from this frame to the next (by mFraction of it, unless mInterpolate is off or the frame stops the animation),
// then by the transform and color of the sprite that holds them. theParentTransform is null for the main sprite.
void PopAnim::DrawSprite(Graphics* g, PASpriteInst* theInst, const SexyMatrix3* theParentTransform,
                         const Color& theParentColor)
{
	PASpriteDef* aDef = theInst->mDef;
	if (aDef == NULL || aDef->mFrames.empty())
		return;
	const PAFrame& aFrame = aDef->mFrames[theInst->mFrameNum];
	const PAFrame* aNextFrame = &aDef->mFrames[(theInst->mFrameNum + 1) % aDef->mFrames.size()];
	if (aFrame.mHasStop)
		aNextFrame = &aFrame;

	for (size_t i = 0; i < aFrame.mObjectPosVector.size(); i++) {
		const PAObjectPos& aPos = aFrame.mObjectPosVector[i];

		SexyMatrix3 aLocal = aPos.mTransform;
		if (mInterpolate) {
			// the same object in the next frame: most likely at the same place in its list
			const PAObjectPos* aNextPos = NULL;
			if (i < aNextFrame->mObjectPosVector.size() && aNextFrame->mObjectPosVector[i].mObjectNum == aPos.mObjectNum)
				aNextPos = &aNextFrame->mObjectPosVector[i];
			else {
				for (size_t j = 0; j < aNextFrame->mObjectPosVector.size(); j++) {
					if (aNextFrame->mObjectPosVector[j].mObjectNum == aPos.mObjectNum) {
						aNextPos = &aNextFrame->mObjectPosVector[j];
						break;
					}
				}
			}
			if (aNextPos != NULL)
				aLocal = InterpolateTransforms(aPos.mTransform, aNextPos->mTransform, mFraction);
		}

		SexyMatrix3 aWorld = theParentTransform != NULL ? MultiplyTransforms(*theParentTransform, aLocal) : aLocal;
		Color aColor((aPos.mColor.mRed * theParentColor.mRed) / 255, (aPos.mColor.mGreen * theParentColor.mGreen) / 255,
		             (aPos.mColor.mBlue * theParentColor.mBlue) / 255, (aPos.mColor.mAlpha * theParentColor.mAlpha) / 255);

		if (aPos.mIsSprite) {
			if (aPos.mObjectNum < (int)theInst->mChildren.size())
				DrawSprite(g, theInst->mChildren[aPos.mObjectNum], &aWorld, aColor);
			continue;
		}

		if (aPos.mResNum < 0 || aPos.mResNum >= (int)mImages.size())
			continue;
		PAImage& anImage = mImages[aPos.mResNum];
		MemoryImage* aMemoryImage = anImage.mImage;
		if (aMemoryImage == NULL)
			continue;  // a picture that could not be loaded

		SexyMatrix3 m = MultiplyTransforms(aWorld, anImage.mTransform);
		Rect aSrcRect = aPos.mHasSrcRect ? aPos.mSrcRect : Rect(0, 0, aMemoryImage->mWidth, aMemoryImage->mHeight);
		g->SetColorizeImages(true);
		g->SetColor(aColor);
		g->SetDrawMode(anImage.mDrawMode);

		if (m.m00 == 1.0f && m.m01 == 0.0f && m.m10 == 0.0f && m.m11 == 1.0f) {
			// only moved
			g->DrawImageF(aMemoryImage, m.m02, m.m12, aSrcRect);
		}
		else if (mVersion == 1 || (m.m00 == m.m11 && -m.m10 == m.m01 &&
		                           fabs(((double)m.m00 * m.m00 + (double)m.m01 * m.m01) - 1.0) < 0.01)) {
			// rotated: the framework turns an image about its middle, so the position is that of the top left corner
			// the image would have if it were not turned
			float anAngle = (float)atan2((double)m.m01, (double)m.m00);
			float aTurn = -anAngle;
			float aSin = (float)sin((double)aTurn);
			float aCos = (float)cos((double)aTurn);
			double aWidth = aMemoryImage->mWidth;
			double aHeight = aMemoryImage->mHeight;
			float aY = (float)(((double)m.m12 + (aWidth * aSin) * 0.5) + (aHeight * aCos) * 0.5);
			aY = (float)((double)aY - aHeight * 0.5);
			float aX = (float)(((double)m.m02 + (aWidth * aCos) * 0.5) - (aHeight * aSin) * 0.5);
			aX = (float)((double)aX - aWidth * 0.5);
			g->DrawImageRotatedF(aMemoryImage, aX, aY, (double)anAngle, &aSrcRect);
		}
		else {
			// any other transform: the framework applies a matrix about the middle of the part drawn, so the matrix
			// goes through the move to the middle
			SexyMatrix3 aCenter;
			aCenter.LoadIdentity();
			aCenter.m02 = (float)(aSrcRect.mWidth * 0.5);
			aCenter.m12 = (float)(aSrcRect.mHeight * 0.5);
			g->DrawImageMatrix(aMemoryImage, MultiplyMatrices(m, aCenter), aSrcRect, 0.0f, 0.0f);
		}
	}
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}
