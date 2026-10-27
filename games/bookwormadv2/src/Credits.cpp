#include "Credits.h"

#include <SDL.h>

#include <algorithm>
#include <cstdlib>

#include "Color.h"
#include "Common.h"
#include "DDImage.h"
#include "Font.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Rect.h"
#include "Res.h"
#include "ResourceManager.h"
#include "SongPlayer.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace {

// 0x82592d: a flag of the exe's data that nothing sets. Set, the credits would load every animation when the file is
// read and not as each credit begins.
const bool kLoadAllAtOnce = false;

// 0x42c090: whether an attribute's key (or an animation's label) is the name, in any case
bool NameIs(const std::string& theString, const char* theName)
{
	return sexystricmp(theString.c_str(), theName) == 0;
}

// V1:0x434ed0: the entries' order is that of their priorities
bool ByPriority(const std::pair<int, int>& theA, const std::pair<int, int>& theB)
{
	return theA.first < theB.first;
}

// V1:0x438f50: a tile of the thanks: where it is, between which y it bobs (the first is the highest), how fast it
// moves, and its letter (the address of the original's text)
struct TileDef {
	float mX, mY, mTop, mBottom, mSpeed;
	unsigned int mLetter;
};

const TileDef kTileDefs[] = {
	{230, 240, 220, 240, -0.5f, GTA(0x6f7d2c)},   {282, 220, 220, 240, 0.45f, GTA(0x6f820c)},
	{334, 240, 220, 240, -0.6f, GTA(0x6f7d50)},   {386, 220, 220, 240, 0.55f, GTA(0x6f7d40)},
	{438, 240, 220, 240, -0.4f, GTA(0x6f8200)},   {490, 220, 220, 240, 0.52f, GTA(0x6f7d58)},
	{570, 240, 220, 240, -0.35f, GTA(0x6f7d54)},  {622, 220, 220, 240, 0.5f, GTA(0x6f821c)},
	{674, 240, 220, 240, -0.45f, GTA(0x6f7d3c)},  {260, 320, 300, 320, -0.6f, GTA(0x6f7d48)},
	{312, 300, 300, 320, 0.4f, GTA(0x6f7d28)},    {364, 320, 300, 320, -0.55f, GTA(0x6f7d50)},
	{416, 300, 300, 320, 0.35f, GTA(0x6f7d30)},   {468, 320, 300, 320, -0.48f, GTA(0x6f7d24)},
	{520, 300, 300, 320, 0.58f, GTA(0x6f7d40)},   {572, 320, 300, 320, -0.37f, GTA(0x6f8218)},
	{652, 300, 300, 320, 0.53f, GTA(0x6f9f7c)},
};

}  // namespace

// 0x42c160: the tags the file has, the picture and the fonts of the screen, then the file; the band that shows the
// credits opens from its middle (y 302)
Credits::Credits()
	: mCurSection(nullptr), mLex(nullptr), mSectionIndex(0), mNextAnimId(0), mFadeAlpha(0), mFadeStep(3),
	  mClipY(302), mClipHeight(0), mState(STATE_NONE), mLexX(-120), mLexY(-150), mTileAlpha(0), mFinaleTime(0),
	  mFlashAlpha(0), mFlash(false), mFading(false), mOpening(true), mEndOfGame(false), mReloadSection(0)
{
	mRoot.mName = "Main";
	Node* aCredit = AddGroup(nullptr, "Credit", [this]() { StartCredit(); }, [this]() { EndCredit(); });
	AddElement(aCredit, "Settings", [this](const AttributeList& theAttributes) { ReadSettings(theAttributes); });
	AddElement(aCredit, "Entry", [this](const AttributeList& theAttributes) { ReadEntry(theAttributes); });
	Node* aColumn = AddGroup(aCredit, "Column", [this]() { StartColumn(); }, [this]() { EndColumn(); });
	AddElement(aColumn, "Entry", [this](const AttributeList& theAttributes) { ReadColumnEntry(theAttributes); });

	Resize(0, 0, gApp->mWidth, gApp->mHeight);
	FreeAnims();
	if (!gApp->mResourceManager->LoadResources("Credits"))
		gApp->ShowResourceError(false);
	if (!ExtractCreditsResources(gApp->mResourceManager))
		gApp->ShowResourceError(false);
	mPriority = 3;
	Load("data/credits.xml");
	// port: a file that is missing or has no credit would have crashed the original; this shows one empty credit
	if (mSections.empty()) {
		mSections.push_back(Section());
		mCurSection = &mSections[0];
	}
	mFadeAlpha = 0;
	mFadeStep = 3;
	mFading = false;
	mSectionIndex = 0;
	mOpening = true;
	mClipY = 302;
	mClipHeight = 0;
	mEndOfGame = false;
	mFinaleTime = 0;
	mTileAlpha = 0;
	mFlash = false;
	mFlashAlpha = 0;
	mLex = nullptr;
	mLexX = -120;
	mLexY = -150;
	mState = STATE_NONE;
	PlaySectionLabels();
	gApp->SetCursor(CURSOR_POINTER);
	MakeTiles();
}

// 0x42c6a0
Credits::~Credits()
{
	gApp->mResourceManager->DeleteResources("Credits");
	delete mLex;
	FreeAnims();
}

// 0x42c810: every animation goes (the credits too, which are read again)
void Credits::FreeAnims()
{
	size_t i = 0;
	while (i < mSections.size()) {
		delete mSections[i].mAnim;
		delete mSections[i].mMidAnim;
		size_t j = 0;
		while (j < mSections[i].mEntries.size()) {
			delete mSections[i].mEntries[j].mAnim;
			j++;
		}
		delete mSections[i].mBgImage;  // port: the original never freed the pictures
		i++;
	}
	mSections.clear();
	mNextAnimId = 0;
	mCurSection = nullptr;
	gApp->ClearUpdateBacklog(false);
}

// 0x42cee0: the credit that was showing is remembered
void Credits::BeforeReload()
{
	mReloadSection = mSectionIndex;
	FreeAnims();
}

// 0x42c030 (0x42c9f0): the credit that was showing again, loaded, and the credits begin to fade in
void Credits::AfterReload()
{
	if ((size_t)mSectionIndex < mSections.size()) {
		// port: the original did not check that the credit is still there
		mSectionIndex = (size_t)mReloadSection < mSections.size() ? mReloadSection : 0;
		mCurSection = &mSections[mSectionIndex];
	} else if (!mSections.empty()) {
		mSectionIndex = 0;
		mCurSection = &mSections[0];
	}
	if (!kLoadAllAtOnce)
		LoadSectionAnims();
	PlaySectionLabels();
	mFadeAlpha = 0;
	mFading = false;
	mOpening = true;
	mClipY = 302;
	mClipHeight = 0;
	gApp->ClearUpdateBacklog(false);
}

// The credit's animations play its frame (the background's and the midground's)
void Credits::PlaySectionLabels()
{
	if (mCurSection->mAnim != nullptr)
		mCurSection->mAnim->PlayLabel(mCurSection->mFrame);
	if (mCurSection->mMidAnim != nullptr)
		mCurSection->mMidAnim->PlayLabel(mCurSection->mFrame);
}

// port: the credit's animations loaded as it begins. The original had this in AfterReload (V1:0x435876) and in Update
// (V1:0x437834).
void Credits::LoadSectionAnims()
{
	if (!mCurSection->mPamFile.empty() && mCurSection->mAnim != nullptr) {
		if (!mCurSection->mAnim->LoadAnimation(mCurSection->mPamFile)) {
			gApp->MsgBox(GT(0x6f9dd4) + mCurSection->mPamFile, GT(0x6f701c), 0);
			delete mCurSection->mAnim;
			mCurSection->mAnim = nullptr;
		}
	}
	if (!mCurSection->mMidPamFile.empty() && mCurSection->mMidAnim != nullptr) {
		if (!mCurSection->mMidAnim->LoadAnimation(mCurSection->mMidPamFile)) {
			gApp->MsgBox(GT(0x6f9dd4) + mCurSection->mMidPamFile, GT(0x6f701c), 0);
			delete mCurSection->mMidAnim;
			mCurSection->mMidAnim = nullptr;
		}
	}
	size_t i = 0;
	while (i < mCurSection->mEntries.size()) {
		Entry& anEntry = mCurSection->mEntries[i];
		if (!anEntry.mPamFile.empty() && anEntry.mAnim != nullptr) {
			if (!anEntry.mAnim->LoadAnimation(anEntry.mPamFile)) {
				gApp->MsgBox(GT(0x6f9df4) + anEntry.mPamFile, GT(0x6f701c), 0);
				delete anEntry.mAnim;
				anEntry.mAnim = nullptr;
			}
		}
		i++;
	}
}

// V1:0x435b60: a new credit; the one that shows is the first
void Credits::StartCredit()
{
	mPriorities.clear();
	mSections.push_back(Section());
	mCurSection = &mSections[0];
	mSectionIndex = 0;
}

// V1:0x435d10: the entries drawn in the order of their priorities (those that are the same in the order they came), and
// the columns centred
void Credits::EndCredit()
{
	Section& aSection = mSections.back();
	// port: the original sorted with std::sort, which is an insertion sort for so few and keeps that order
	std::stable_sort(mPriorities.begin(), mPriorities.end(), ByPriority);
	size_t i = 0;
	while (i < mPriorities.size()) {
		aSection.mDrawOrder.push_back(mPriorities[i].second);
		i++;
	}
	mPriorities.clear();
	if (!aSection.mColumns.empty()) {
		int aTotalWidth = 0;
		size_t aCol = 0;
		while (aCol < aSection.mColumns.size()) {
			aTotalWidth += aSection.mColumns[aCol].mWidth + 20;
			aCol++;
		}
		aSection.mColumnsX = (mWidth - aTotalWidth + 20) / 2;
	}
}

// V1:0x435ee0
void Credits::ReadSettings(const AttributeList& theAttributes)
{
	Section& aSection = mSections.back();
	size_t i = 0;
	while (i < theAttributes.size()) {
		const std::string& aKey = theAttributes[i].first;
		const std::string& aValue = theAttributes[i].second;
		if (NameIs(aKey, "title")) {
			aSection.mTitle = aValue;
		} else if (NameIs(aKey, "bgpam")) {
			aSection.mAnim = new PopAnim(mNextAnimId++, this);
			aSection.mPamFile = "images/pam/" + aValue + ".pam";
			// only the first credit is loaded now
			if ((kLoadAllAtOnce || mSections.size() == 1) && !aSection.mAnim->LoadAnimation(aSection.mPamFile)) {
				gApp->MsgBox(GT(0x6f9dd4) + aSection.mPamFile, GT(0x6f701c), 0);
				delete aSection.mAnim;
				aSection.mAnim = nullptr;
			}
		} else if (NameIs(aKey, "mgpam")) {
			aSection.mMidAnim = new PopAnim(mNextAnimId++, this);
			aSection.mMidPamFile = "images/pam/" + aValue + ".pam";
			if ((kLoadAllAtOnce || mSections.size() == 1) && !aSection.mMidAnim->LoadAnimation(aSection.mMidPamFile)) {
				gApp->MsgBox(GT(0x6f9dd4) + aSection.mMidPamFile, GT(0x6f701c), 0);
				delete aSection.mMidAnim;
				aSection.mMidAnim = nullptr;
			}
		} else if (NameIs(aKey, "idlecount")) {
			aSection.mIdleCount = atol(aValue.c_str());
		} else if (NameIs(aKey, "fadein")) {
			aSection.mFadeIn = atol(aValue.c_str());
		} else if (NameIs(aKey, "nextalpha")) {
			aSection.mNextAlpha = atol(aValue.c_str());
		} else if (NameIs(aKey, "frame")) {
			aSection.mFrame = aValue;
		} else if (NameIs(aKey, "y")) {
			aSection.mY = atol(aValue.c_str());
		} else if (NameIs(aKey, "bgimage")) {
			aSection.mBgImage = gApp->GetImage(aValue, true);
			if (aSection.mBgImage == nullptr)
				gApp->MsgBox(GT(0x6f9e7c) + aValue, GT(0x6f9e74), 0);
		}
		i++;
	}
}

// V1:0x436900: a name with its creature, in the credit being read (the order it is drawn in comes at EndCredit)
void Credits::ReadEntry(const AttributeList& theAttributes)
{
	Section& aSection = mSections.back();
	Entry anEntry;
	int aPriority = 0;
	size_t i = 0;
	while (i < theAttributes.size()) {
		const std::string& aKey = theAttributes[i].first;
		const std::string& aValue = theAttributes[i].second;
		if (NameIs(aKey, "name")) {
			anEntry.mName = aValue;
		} else if (NameIs(aKey, "fadein")) {
			anEntry.mFadeIn = atol(aValue.c_str());
		} else if (NameIs(aKey, "nextalpha")) {
			anEntry.mNextAlpha = atol(aValue.c_str());
		} else if (NameIs(aKey, "x")) {
			anEntry.mX = atol(aValue.c_str());
		} else if (NameIs(aKey, "y")) {
			anEntry.mY = atol(aValue.c_str());
		} else if (NameIs(aKey, "attack")) {
			anEntry.mAttack = aValue;
		} else if (NameIs(aKey, "targetx")) {
			anEntry.mTargetX = atol(aValue.c_str());
		} else if (NameIs(aKey, "pam")) {
			anEntry.mAnim = new PopAnim(mNextAnimId++, this);
			anEntry.mPamFile = "images/pam/" + aValue + ".pam";
			if ((kLoadAllAtOnce || mSections.size() == 1) && !anEntry.mAnim->LoadAnimation(anEntry.mPamFile)) {
				gApp->MsgBox(GT(0x6f9dd4) + anEntry.mPamFile, GT(0x6f701c), 0);
				delete anEntry.mAnim;
				anEntry.mAnim = nullptr;
			}
		} else if (NameIs(aKey, "priority")) {
			aPriority = atol(aValue.c_str());
		}
		i++;
	}
	aSection.mEntries.push_back(anEntry);
	mPriorities.push_back(std::make_pair(aPriority, (int)aSection.mEntries.size() - 1));
}

// V1:0x436650: a column of the credit being read
void Credits::StartColumn()
{
	mSections.back().mColumns.push_back(Column());
}

// V1:0x436790: the height of the credit's columns (one line more than the names each, as the font's ascent and 5
// pixels), and how wide the column is
void Credits::EndColumn()
{
	Section& aSection = mSections.back();
	Column& aColumn = aSection.mColumns.back();
	int aWidth = 0;
	aSection.mColumnsHeight = (FONT_MEDIUM->GetAscent() + 5) * (int)aColumn.mNames.size();
	size_t i = 0;
	while (i < aColumn.mNames.size()) {
		int aNameWidth = FONT_MEDIUM->StringWidth(aColumn.mNames[i]);
		if (aNameWidth > aWidth)
			aWidth = aNameWidth;
		i++;
	}
	aColumn.mWidth = aWidth;
}

// V1:0x436520: a name of the column being read, the first value the tag has
void Credits::ReadColumnEntry(const AttributeList& theAttributes)
{
	Column& aColumn = mSections.back().mColumns.back();
	size_t i = 0;
	while (i < theAttributes.size()) {
		if (NameIs(theAttributes[i].first, "value")) {
			aColumn.mNames.push_back(theAttributes[i].second);
			return;
		}
		i++;
	}
}

// V1:0x438f50: the letters of the thanks (17 tiles in two rows)
void Credits::MakeTiles()
{
	mTiles.clear();
	size_t i = 0;
	while (i < sizeof(kTileDefs) / sizeof(kTileDefs[0])) {
		Tile aTile;
		aTile.mX = kTileDefs[i].mX;
		aTile.mY = kTileDefs[i].mY;
		aTile.mTop = kTileDefs[i].mTop;
		aTile.mBottom = kTileDefs[i].mBottom;
		aTile.mSpeed = kTileDefs[i].mSpeed;
		aTile.mLetter = GT(kTileDefs[i].mLetter);
		mTiles.push_back(aTile);
		i++;
	}
}

// V1:0x438c30: each tile moves up or down and turns round at the end of its range
void Credits::UpdateTiles()
{
	size_t i = 0;
	while (i < mTiles.size()) {
		Tile& aTile = mTiles[i];
		float aY = aTile.mSpeed + aTile.mY;
		aTile.mY = aY;
		if (aTile.mSpeed < 0) {
			if (!(aTile.mTop < aY)) {
				aTile.mY = aTile.mTop;
				aTile.mSpeed = aTile.mSpeed * -1.0f;
			}
		} else if (aTile.mSpeed > 0) {
			if (!(aTile.mBottom > aY)) {
				aTile.mY = aTile.mBottom;
				aTile.mSpeed = aTile.mSpeed * -1.0f;
			}
		}
		i++;
	}
}

// 0x4306a0: the finale of the end of the game: Lex comes down, the tiles fade in and bob for 500 updates, Lex
// attacks (his animation's "whitescreen" flashes the screen), and the curtain closes
void Credits::UpdateFinale()
{
	mLex->UpdateF(1.0f);
	if (mState == STATE_LEX_COMES) {
		mLexY += 8;
		if (mLexY >= 50) {
			mLex->PlayLabel("levelupgiggle");
			mState++;
		}
	} else if (mState == STATE_TILES) {
		if (mTileAlpha < 255) {
			mTileAlpha += 2;
			if (mTileAlpha > 255)
				mTileAlpha = 255;
		}
		UpdateTiles();
		mFinaleTime++;
		if (mFinaleTime == 500) {
			mState++;
			mLex->PlayLabel("attack6");
		}
	} else if (mState == STATE_LEX_ATTACKS) {
		UpdateTiles();
		if (mFlash) {
			mFlashAlpha--;
			if (mFlashAlpha == 0) {
				mLex->PlayLabel("levelupgiggle");
				mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_UPDATE | WIDGETFLAGS_ALLOW_MOUSE;
				mFlash = false;
				mState = STATE_ENDING;
				gApp->DoFullCurtainTransition("");
				gApp->mCurtainCallback = GameApp::CloseCreditsCallback;
				gApp->mCurtainCallbackObject = gApp;
			}
		}
	}
}

// 0x42e640
void Credits::Update()
{
	MarkDirtyFull();
	if (!gApp->mVSyncUpdates)
		UpdateF(1.0f);
	if (mOpening) {
		// the band of the screen opens from its middle
		mClipY--;
		mClipHeight += 2;
		if (mClipY <= 195) {
			mClipY = 195;
			mOpening = false;
			mClipHeight = 215;
		}
	}
	if (mFading) {
		// the credit has been seen: black comes over it (after a wait), the next credit is made ready under it, and
		// the black goes
		if (!mCurSection->mWaited && mFadeStep > 0) {
			mCurSection->mWait--;
			if (mCurSection->mWait <= 0)
				mCurSection->mWaited = true;
			return;
		}
		mFadeAlpha += mFadeStep;
		if (mFadeAlpha >= 255 && mFadeStep > 0) {
			mFadeStep = -mFadeStep;
			mFadeAlpha = 255;
			delete mCurSection->mAnim;
			mCurSection->mAnim = nullptr;
			delete mCurSection->mMidAnim;
			mCurSection->mMidAnim = nullptr;
			size_t i = 0;
			while (i < mCurSection->mEntries.size()) {
				delete mCurSection->mEntries[i].mAnim;
				mCurSection->mEntries[i].mAnim = nullptr;
				i++;
			}
			mSectionIndex++;
			if (!mSections.empty() && (size_t)mSectionIndex < mSections.size()) {
				mCurSection = &mSections[mSectionIndex];
				if (!kLoadAllAtOnce)
					LoadSectionAnims();
				PlaySectionLabels();
				return;
			}
			if (mEndOfGame && mState == STATE_NONE) {
				// no credit is left: Lex comes in
				mState = STATE_LEX_COMES;
				mLex = new PopAnim(mNextAnimId++, this);
				if (!mLex->LoadAnimation("images/pam/lex.pam")) {
					gApp->MsgBox(GT(0x6f9ef8), GT(0x6f9e74), 0);
					delete mLex;
					mLex = nullptr;  // port: the original went on to play the animation it had just deleted
					mEndOfGame = false;
					mState = STATE_NONE;
					return;
				}
				mLex->PlayLabel("idle");
			}
		} else if (mFadeAlpha <= 0 && mFadeStep < 0) {
			mFadeStep = -mFadeStep;
			mFadeAlpha = 0;
			mFading = false;
		}
		return;
	}
	if (mOpening)
		return;

	// the creatures that slid away have gone: the names fly at them, one after the other, the first now
	if (mCurSection->mAllAttacked && !mCurSection->mNamesFly && mState == STATE_NONE && mCurSection->mShakeTime <= 0) {
		mCurSection->mTextOverCreatures = true;
		size_t i = 0;
		while (i < mCurSection->mEntries.size()) {
			if (mCurSection->mEntries[i].mSlideSpeed != 0) {
				mCurSection->mTextOverCreatures = false;
				break;
			}
			i++;
		}
		if (mCurSection->mTextOverCreatures) {
			mCurSection->mNamesFly = true;
			i = 0;
			while (i < mCurSection->mEntries.size()) {
				Entry& anEntry = mCurSection->mEntries[i];
				if (anEntry.mAnim != nullptr) {
					anEntry.mNameFlies = true;
					anEntry.mDone = false;
					break;
				}
				anEntry.mDone = true;
				i++;
			}
		}
	}

	// the heading fades in; when it is clear enough the first name begins
	bool anAlphaDone = true;
	if (mCurSection->mAlpha < 255 && mState == STATE_NONE) {
		mCurSection->mAlpha += mCurSection->mFadeIn;
		if (mCurSection->mAlpha > 255)
			mCurSection->mAlpha = 255;
		else
			anAlphaDone = false;
		if (mCurSection->mAlpha >= mCurSection->mNextAlpha && !mCurSection->mTextShown) {
			mCurSection->mTextShown = true;
			if (mCurSection->mEntries.empty())
				mCurSection->mNoEntries = true;
			size_t i = 0;
			while (i < mCurSection->mEntries.size()) {
				Entry& anEntry = mCurSection->mEntries[i];
				if (anEntry.mShown) {
					i++;
					continue;
				}
				anEntry.mShown = true;
				if (anEntry.mAnim != nullptr)
					anEntry.mAnim->PlayLabel("idle");
				break;
			}
		}
	}

	// the entries
	bool aNextBegins = false;
	bool aNameFarEnough = false;
	bool anAllDone = true;
	if (mState == STATE_NONE) {
		size_t i = 0;
		while (i < mCurSection->mEntries.size()) {
			Entry& anEntry = mCurSection->mEntries[i];
			if (anEntry.mShown || aNextBegins) {
				// the name before this one flew far enough: this one's turn
				if (aNameFarEnough && !anEntry.mDying) {
					if (anEntry.mAnim == nullptr) {
						anEntry.mDone = true;
					} else {
						aNameFarEnough = false;
						anEntry.mNameFlies = true;
						anEntry.mDone = false;
					}
				}
				if (mCurSection->mShaking) {
					mCurSection->mShakeTime--;
					if (mCurSection->mShakeTime <= 0)
						mCurSection->mShaking = false;
					anEntry.mNameX = Rand() % 5;
					anEntry.mNameY = Rand() % 5;
				} else if (anEntry.mNameFlies && !anEntry.mDone) {
					if (!anEntry.mDying) {
						anEntry.mNameX += 20;
						if (anEntry.mNameX >= 100)
							aNameFarEnough = true;
						if (anEntry.mNameX >= anEntry.mTargetX) {
							anEntry.mDying = true;
							anEntry.mAnim->PlayLabel("death");
						}
					} else if (anEntry.mNameX >= 0) {
						anEntry.mNameX -= 6;
						if (anEntry.mNameX <= 0) {
							if (!anEntry.mAnim->IsPlaying())
								anEntry.mDone = true;
							anEntry.mNameX = 0;
						}
					}
				}
				// the creature slides: away (a negative speed) at first, and back once it has been hit
				if (anEntry.mSlideSpeed != 0 &&
				    (anEntry.mSlideSpeed < 0 || (anEntry.mDamaged && NameIs(anEntry.mAnim->mLabel, "idle")))) {
					anEntry.mX += anEntry.mSlideSpeed;
					anEntry.mSlid += abs(anEntry.mSlideSpeed);
					if (anEntry.mSlid >= anEntry.mSlideDistance) {
						if (anEntry.mSlideSpeed < 0) {
							anEntry.mSlideSpeed = -anEntry.mSlideSpeed;
							anEntry.mSlid = 0;
						} else {
							anEntry.mSlideSpeed = 0;
						}
					}
				}
				if (aNextBegins && !anEntry.mShown) {
					aNextBegins = false;
					anEntry.mShown = true;
					if (anEntry.mAnim != nullptr)
						anEntry.mAnim->PlayLabel("idle");
				}
				if (anEntry.mAlpha < 255) {
					anEntry.mAlpha += anEntry.mFadeIn;
					if (anEntry.mAlpha > 255)
						anEntry.mAlpha = 255;
					else
						anAlphaDone = false;
					if (anEntry.mAnim != nullptr)
						anEntry.mAnim->SetAnimColor(Color(255, 255, 255, anEntry.mAlpha));
					if (anEntry.mAlpha >= anEntry.mNextAlpha && !anEntry.mNextStarted) {
						anEntry.mNextStarted = true;
						aNextBegins = true;
					}
				}
				if (!anEntry.mDone)
					anAllDone = false;
			} else {
				anAllDone = false;
			}
			i++;
		}
	}

	// everything is in and over: the next credit, or the end
	if (anAllDone && anAlphaDone && mCurSection->mIdleCount <= 0 && mState == STATE_NONE) {
		if ((size_t)mSectionIndex + 1 < mSections.size() || mEndOfGame) {
			mFading = true;
		} else if (!gApp->IsCurtainMoving()) {
			mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
			gApp->DoFullCurtainTransition("");
			gApp->mCurtainCallback = GameApp::CloseCreditsCallback;
			gApp->mCurtainCallbackObject = gApp;
		}
	} else if (mState != STATE_NONE) {
		UpdateFinale();
	}

	// the wait before the attacks: when it is over, the creatures that have an attack play it
	if (anAlphaDone && mState == STATE_NONE) {
		mCurSection->mIdleCount--;
		if (mCurSection->mIdleCount == 0) {
			size_t i = 0;
			while (i < mCurSection->mEntries.size()) {
				Entry& anEntry = mCurSection->mEntries[i];
				if (anEntry.mAnim != nullptr && !anEntry.mAttack.empty()) {
					anEntry.mAnim->PlayLabel(anEntry.mAttack + "done");
					anEntry.mAnim->PlayLabel(anEntry.mAttack);
					mCurSection->mAttackers++;
				}
				i++;
			}
		}
	}
}

// 0x42f4b0: the animations of the credit that has begun
void Credits::UpdateF(float theFrac)
{
	if (mCurSection->mAnim != nullptr)
		mCurSection->mAnim->UpdateF(theFrac);
	if (mCurSection->mMidAnim != nullptr)
		mCurSection->mMidAnim->UpdateF(theFrac);
	size_t i = 0;
	while (i < mCurSection->mEntries.size()) {
		Entry& anEntry = mCurSection->mEntries[i];
		if (anEntry.mShown && anEntry.mAnim != nullptr && anEntry.mAnim->IsPlaying())
			anEntry.mAnim->UpdateF(theFrac);
		i++;
	}
}

// 0x430180: the creatures that have begun, in the order of their priorities
void Credits::DrawCreatures(Graphics* g)
{
	size_t i = 0;
	while (i < mCurSection->mDrawOrder.size()) {
		Entry& anEntry = mCurSection->mEntries[mCurSection->mDrawOrder[i]];
		if (anEntry.mShown && anEntry.mAnim != nullptr) {
			g->PushState();
			g->Translate(anEntry.mX, anEntry.mY);
			anEntry.mAnim->Draw(g);
			g->PopState();
		}
		i++;
	}
}

// 0x430260: the heading (over the width of the screen when the credit has columns, else on the left, above the
// names, one under the other), and the columns
void Credits::DrawText(Graphics* g)
{
	g->SetFont(FONT_HUGE);
	g->SetColor(Color(254, 232, 0, mCurSection->mAlpha));
	int aNamesY = 0;
	if (mCurSection->mColumns.empty()) {
		g->WriteWordWrapped(Rect(0, 245, 355, 300), mCurSection->mTitle, -1, 0, nullptr, -1);
	} else {
		int aY = (235 - mCurSection->mColumnsHeight) / 2 - FONT_HUGE->GetAscent() + 195;
		g->WriteWordWrapped(Rect(0, aY, mWidth, 300), mCurSection->mTitle, -1, 0, nullptr, -1);
		aNamesY = aY + 50;
	}
	size_t i = 0;
	while (i < mCurSection->mEntries.size()) {
		Entry& anEntry = mCurSection->mEntries[i];
		if (anEntry.mShown) {
			g->SetColor(Color(255, 255, 255, anEntry.mAlpha));
			int aY = (FONT_HUGE->GetAscent() - 5) * (int)i + anEntry.mNameY + 290;
			g->WriteWordWrapped(Rect(anEntry.mNameX, aY, 355, 300), anEntry.mName, -1, 0, nullptr, -1);
		}
		i++;
	}
	g->SetFont(FONT_MEDIUM);
	g->SetColor(Color(255, 255, 255, mCurSection->mAlpha));
	int aX = mCurSection->mColumnsX;
	size_t aCol = 0;
	while (aCol < mCurSection->mColumns.size()) {
		Column& aColumn = mCurSection->mColumns[aCol];
		int aY = aNamesY;
		size_t aName = 0;
		while (aName < aColumn.mNames.size()) {
			g->WriteString(aColumn.mNames[aName], aX, aY, aColumn.mWidth, 0, true, 0, -1, -1);
			aY += FONT_MEDIUM->GetAscent() + 5;
			aName++;
		}
		aX += aColumn.mWidth + 20;
		aCol++;
	}
}

// 0x42fb30
void Credits::Draw(Graphics* g)
{
	g->SetColor(Color::Black);
	g->FillRect(0, 0, mWidth, mHeight);
	g->DrawImage(IMAGE_BANNER_CREDITS, (mWidth - IMAGE_BANNER_CREDITS->GetWidth()) / 2, 112);
	g->SetFont(FONT_MEDIUM_BOLD);
	g->SetColor(Color(89, 0, 0));
	g->WriteWordWrapped(Rect(0, 429, mWidth, 200), GT(0x6f9f58), -1, 0, nullptr, -1);

	// the band the credit shows in
	g->PushState();
	g->ClearClipRect();
	g->SetClipRect(Rect(0, mClipY, mWidth, mClipHeight));
	g->PushState();
	if (mCurSection->mAlpha != 255 && mCurSection->mBgImage != nullptr) {
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, mCurSection->mAlpha));
	}
	if (mState == STATE_NONE) {
		// the picture, the midground animation over it and the background animation over that (0x42fb30)
		if (mCurSection->mBgImage != nullptr)
			g->DrawImage(mCurSection->mBgImage, 0, mCurSection->mY);
		if (mCurSection->mMidAnim != nullptr) {
			g->Translate(0, mCurSection->mY);
			mCurSection->mMidAnim->Draw(g);
			g->Translate(0, -mCurSection->mY);
		}
		if (mCurSection->mAnim != nullptr) {
			g->Translate(0, mCurSection->mY);
			mCurSection->mAnim->Draw(g);
			g->Translate(0, -mCurSection->mY);
		}
	} else {
		g->DrawImage(IMAGE_CREDITS_GRASS, 0, 195);
	}
	g->PopState();

	if (mState == STATE_NONE) {
		if (mCurSection->mAttackers == 0 && mCurSection->mTextOverCreatures) {
			DrawCreatures(g);
			DrawText(g);
		} else {
			DrawText(g);
			DrawCreatures(g);
		}
	} else if (mState != STATE_ENDING) {
		if (mState >= STATE_TILES) {
			// the thanks
			g->SetFont(FONT_TILE);
			g->SetColor(Color(255, 255, 255, mTileAlpha));
			if (mTileAlpha != 255)
				g->SetColorizeImages(true);
			size_t i = 0;
			while (i < mTiles.size()) {
				Tile& aTile = mTiles[i];
				Rect aCel(0, 0, IMAGE_TILE_BASIC->GetCelWidth(), IMAGE_TILE_BASIC->mHeight);
				g->DrawImage(IMAGE_TILE_BASIC, (int)aTile.mX, (int)aTile.mY, aCel);
				g->WriteString(aTile.mLetter, (int)aTile.mX, (int)(aTile.mY + 37.0), IMAGE_TILE_BASIC->GetCelWidth(), 0, true, 0,
				               -1, -1);
				i++;
			}
			if (mTileAlpha != 255)
				g->SetColorizeImages(false);
		}
		g->Translate(mLexX, mLexY);
		mLex->Draw(g);
		g->Translate(-mLexX, -mLexY);
	}

	// the shadows at the band's edges
	g->SetColor(Color(0, 0, 0, 128));
	g->FillRect(0, 195, mWidth, 10);
	g->FillRect(0, 400, mWidth, 10);
	if (mFlash || mState == STATE_ENDING) {
		// the flash of white over black
		g->SetColor(Color(0, 0, 0));
		g->FillRect(0, 195, mWidth, 215);
		g->SetColor(Color(255, 255, 255, mFlashAlpha));
		g->FillRect(0, 195, mWidth, 215);
	} else if (mFading) {
		// the black the credit fades out to
		g->SetColor(Color(0, 0, 0, mFadeAlpha));
		g->FillRect(0, 195, mWidth, 215);
	}
	g->PopState();
}

// 0x42f570: a click anywhere, but not of the middle button and not while the curtain moves, closes the screen
void Credits::MouseUp(int x, int y, int theLastDownButtonId)
{
	if (theLastDownButtonId != 3 && !gApp->IsCurtainMoving()) {
		mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
		gApp->DoFullCurtainTransition("");
		gApp->mCurtainCallback = GameApp::CloseCreditsCallback;
		gApp->mCurtainCallbackObject = gApp;
	}
}

// 0x42f650: an animation reached a frame that stops it: Lex giggles again, the background goes round, a creature
// that was fighting goes back to idle (the last of them lets the names fly), a dead creature stays dead
void Credits::PopAnimStopped(int theId)
{
	if (mState != STATE_NONE && theId == mLex->mId) {
		mLex->PlayLabel("levelupgiggle");
		return;
	}
	if (mCurSection->mAnim != nullptr && theId == mCurSection->mAnim->mId) {
		mCurSection->mAnim->PlayLabel(mCurSection->mFrame);
		return;
	}
	if (mCurSection->mMidAnim != nullptr && theId == mCurSection->mMidAnim->mId) {
		mCurSection->mMidAnim->PlayLabel(mCurSection->mFrame);
		return;
	}
	size_t i = 0;
	while (i < mCurSection->mEntries.size()) {
		Entry& anEntry = mCurSection->mEntries[i];
		if (anEntry.mAnim == nullptr || anEntry.mAnim->mId != theId) {
			i++;
			continue;
		}
		if (NameIs(anEntry.mAnim->mLabel, "idle") || NameIs(anEntry.mAnim->mLabel, "engage")) {
			anEntry.mAnim->PlayLabel("idle");
		} else if (NameIs(anEntry.mAnim->mLabel, anEntry.mAttack.c_str())) {
			anEntry.mAnim->PlayLabel("idle");
			mCurSection->mAttackers--;
			if (mCurSection->mAttackers == 0)
				mCurSection->mAllAttacked = true;
		} else if (NameIs(anEntry.mAnim->mLabel, "death")) {
			anEntry.mAnim->mPlaying = false;
		}
		return;
	}
}

// 0x42f8f0: a command of an animation. A creature that hits ("causedamage") shakes the screen and lets its name
// fight; one that "slide"s moves away by the distance and at the speed the command gives ("<distance> <speed>");
// Lex's "whitescreen" and "screenrumble" flash the screen.
void Credits::PopAnimCommand(int theId, const std::string& theCommand, const std::string& theParam)
{
	if (NameIs(theCommand, "causedamage")) {
		mCurSection->mShaking = true;
		mCurSection->mShakeTime = 200;
		size_t i = 0;
		while (i < mCurSection->mEntries.size()) {
			Entry& anEntry = mCurSection->mEntries[i];
			if (anEntry.mAnim != nullptr && anEntry.mAnim->mId == theId) {
				anEntry.mDamaged = true;
				return;
			}
			i++;
		}
	} else if (NameIs(theCommand, "slide")) {
		size_t i = 0;
		while (i < mCurSection->mEntries.size()) {
			Entry& anEntry = mCurSection->mEntries[i];
			if (anEntry.mAnim == nullptr || anEntry.mAnim->mId != theId) {
				i++;
				continue;
			}
			if (!anEntry.mSlideStarted) {
				anEntry.mSlideStarted = true;
				mCurSection->mTextOverCreatures = false;
				size_t aSpace = theParam.find(" ", 0, 1);
				int aDistance = atol(theParam.substr(0, aSpace).c_str());
				int aSpeed = atol(theParam.substr(aSpace + 1).c_str());
				anEntry.mSlideSpeed = -abs(aSpeed);
				anEntry.mSlideDistance = abs(aDistance);
				anEntry.mSlid = 0;
			}
			return;
		}
	} else if (NameIs(theCommand, "whitescreen") || NameIs(theCommand, "screenrumble")) {
		if (mEndOfGame && mLex != nullptr && theId == mLex->mId) {
			mFlashAlpha = 255;
			mFlash = true;
		}
	}
}
