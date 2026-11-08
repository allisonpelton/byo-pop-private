#include "TempleScreen.h"
#include "Character.h"
#include "Temples.h"
#include "PixelusApp.h"
#include "PuzzleImages.h"
#include "GameDialog.h"
#include "GameText.h"
#include "LevelData.h"
#include "ProgressManager.h"
#include "UserDirectory.h"
#include "Grid.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "DialogButton.h"
#include "WidgetManager.h"
#include "Buffer.h"
#include "Compat2004.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

using namespace Sexy;

// the hints the cave shows for each of the tutorial's nine puzzles (0x53d25c; the board's sidebar teaches with the
// same texts)
static const unsigned int gTutorialHints[9] = { 0x50d760, 0x50d724, 0x50d6f0, 0x50d688, 0x50d628, 0x50d5e8, 0x50d5a8, 0x50d578, 0x50d530 };

// 0x42e1f0: what each god is made of. The arms are drawn where the original puts them, turned about their joint by
// a sine of the time (mode 0: 0.01 per update, 1: slowly changing, 2: 1/300, 3: 1/130)
struct GodArm
{
	int		mImage;
	float	mX, mY;			// the joint's place; mY is added to the god's height when mRelative
	bool	mRelative;
	double	mAmplitude;
	int		mMode;
	float	mPivotX, mPivotY;
	bool	mBack;			// behind the body
};

struct GodInfo
{
	int		mBody;
	float	mX, mY;			// where body and head go
	double	mBob;			// how far the whole god rises and sinks
	double	mBobSpeed;
	GodArm	mArms[3];
};

static const GodInfo gGods[NUM_TEMPLES] =
{
	{ IMAGE_CYCLOP_BODY, 40, 221, 2, 0.005, {
		{ IMAGE_CYCLOP_ARM1, 140, 280, false, 0.032, 0, 7, 84, true },
		{ IMAGE_CYCLOP_ARM2, -16, 244, false, -0.032, 0, 118, 117, true }, { 0 } } },
	{ IMAGE_FLORA_BODY, 10, 170, 0, 0, {
		{ IMAGE_FLORA_ARM1, 68, 286, false, 0.08, 0, 0, 22, true },
		{ IMAGE_FLORA_ARM2, 20, 302, false, 0.075, 2, 35, 4, false }, { 0 } } },
	{ IMAGE_CUPID_BODY, 10, 187, 7, 0.01, { { 0 } } },
	{ IMAGE_NEPTUNE_BODY, 0, 187, 7, 0.01, {
		{ IMAGE_NEPTUNE_ARM, 0, 65, true, 0.08, 0, 77, 98, false }, { 0 } } },
	{ IMAGE_VENUS_BODY, 10, 221, 2, 0.0125, {
		{ IMAGE_VENUS_SHELL, 10, 0, true, 0, 0, 0, 0, true },
		{ IMAGE_VENUS_ARM2, 24, 77, true, 0.075, 1, 34, 52, true },
		{ IMAGE_VENUS_ARM1, 65, 121, true, 0.08, 0, 3, 18, true } } },
	{ IMAGE_BACCHUS_BODY, 5, 180, 0, 0, {
		{ IMAGE_BACCHUS_ARM2, 20, 242, false, 0.075, 1, 85, 90, true },
		{ IMAGE_BACCHUS_ARM1, 5, 180, false, 0.08, 0, 120, 165, true }, { 0 } } },
	{ IMAGE_VULCAN_BODY, 10, 225, 2, 0.005, {
		{ IMAGE_VULCAN_ARM, 10, 365, false, 0.032, 0, 100, 0, false }, { 0 } } },
	{ IMAGE_DIANE_BODY, 10, 205, 0, 0, {
		{ IMAGE_DIANE_ARM2, 38, 152, false, 0.075, 1, 53, 172, true },
		{ IMAGE_DIANE_ARM1, 101, 325, false, 0.08, 0, 3, 18, true }, { 0 } } },
	{ IMAGE_PROSERPINE_BODY, 0, 184, 0, 0, {
		{ IMAGE_PROSERPINE_ARM1, 0, 184, false, 0.06, 3, 96, 176, false }, { 0 } } },
	{ IMAGE_JUPITER_BODY, 0, 173, 4, 0.005, {
		{ IMAGE_JUPITER_ARM1, 123, 127, true, 0.08, 0, 8, 23, true },
		{ IMAGE_JUPITER_ARM2, 2, 19, true, 0.1, 2, 97, 127, true }, { 0 } } }
};

static double ArmSine(int theMode, int theTicks)
{
	switch (theMode)
	{
	case 1:		// the slowly changing speed of the second arms
		return sin(theTicks / (sin(0.00125 * theTicks) * 400.0 + 1200.0));
	case 2:
		return sin(theTicks * 0.0033333333333333335);
	case 3:
		return sin(theTicks * 0.007692307692307693);
	default:
		return sin(theTicks * 0.01);
	}
}

// 0x42e110: the temple screen gives the figure a speech bubble, with the picture of the map behind it where the bubble's
// mode goes (which the bubble does not understand: -1)
GodFigure::GodFigure(PixelusApp* theApp, int theGod, Widget* theOwner)
{
	mApp = theApp;
	mGod = theGod;
	mCharacter = new Character(theApp, theGod, theOwner, -1);
}

GodFigure::~GodFigure()
{
	delete mCharacter;
}

// 0x42df40
void GodFigure::Update(Widget* theOwner)
{
	mCharacter->Update(theOwner);
}

// 0x42de40
void GodFigure::Poke()
{
	mCharacter->Poke();
}

void GodFigure::Draw(Graphics* g)
{
	if (mGod < 0 || mGod >= NUM_TEMPLES)
		return;
	const GodInfo& anInfo = gGods[mGod];
	float aX = anInfo.mX;
	int aTicks = mCharacter->mTick;
	float aY = anInfo.mY + (float) (anInfo.mBob * sin(aTicks * anInfo.mBobSpeed));

	for (int aPass = 0; aPass < 2; aPass++)
	{
		if (aPass == 1)
		{
			g->DrawImage(mApp->mImages[anInfo.mBody], (int) aX, (int) aY);
			if (mGod == 2)	// Cupid's wings flap: three frames
			{
				Image* aWings = mApp->mImages[IMAGE_CUPID_WINGS];
				int aFrameHeight = aWings->GetHeight() / 3;
				g->DrawImage(aWings, (int) aX, (int) aY + 104, Rect(0, aFrameHeight * ((aTicks / 12) % 3), aWings->GetWidth(), aFrameHeight));
			}
		}
		for (int i = 0; i < 3; i++)
		{
			const GodArm& anArm = anInfo.mArms[i];
			if (anArm.mImage == 0 || anArm.mBack != (aPass == 0))
				continue;
			float anArmY = anArm.mRelative ? aY + anArm.mY : anArm.mY;
			Image* anImage = mApp->mImages[anArm.mImage];
			if (anArm.mAmplitude == 0)
				g->DrawImage(anImage, (int) anArm.mX, (int) anArmY);
			else
				g->DrawImageRotatedF(anImage, anArm.mX, anArmY, anArm.mAmplitude * ArmSine(anArm.mMode, aTicks), anArm.mPivotX, anArm.mPivotY);
		}
	}

	// 0x42de50: the head swings on its spring; a new face comes in over the one before
	const Character& aCharacter = *mCharacter;
	if (aCharacter.mFade < 0xff)
	{
		MemoryImage* aPrevious = mApp->mHeads[mGod][aCharacter.mPrevPose];
		if (aPrevious != NULL)
			g->DrawImageRotatedF(aPrevious, aX, aY, aCharacter.mAngle, aPrevious->GetWidth() / 2, aPrevious->GetHeight() / 2);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, aCharacter.mFade));
	}
	MemoryImage* aHead = mApp->mHeads[mGod][aCharacter.mPose];
	if (aHead != NULL)
		g->DrawImageRotatedF(aHead, aX, aY, aCharacter.mAngle, aHead->GetWidth() / 2, aHead->GetHeight() / 2);
	if (aCharacter.mFade < 0xff)
		g->SetColorizeImages(false);
}

// 0x41c610: the island as it was, over the temple screen and fading out (0x41c700: three of 255 each update); its widget
// goes at the end of the screen's children, so it covers the buttons as well. It does not take the mouse
namespace Sexy
{

class TempleFade : public Widget
{
public:
	MemoryImage*	mImage;		// +0x5c, the screen's
	int				mAlpha;		// +0x64

	TempleFade(MemoryImage* theImage) : mImage(theImage), mAlpha(0xff) { mMouseVisible = false; }

	virtual void Update() override
	{
		Widget::Update();
		if (mAlpha > 0)
		{
			mAlpha -= 3;
			if (mAlpha < 1)
				mAlpha = 0;
			MarkDirty();
		}
	}

	virtual void Draw(Graphics* g) override
	{
		if (mAlpha > 0 && mImage != NULL)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, mAlpha));
			g->DrawImage(mImage, 0, 0);
			g->SetColorizeImages(false);
		}
	}
};

}

// 0x413ce0
TempleScreen::TempleScreen(PixelusApp* theApp, MemoryImage* theBackdrop)
{
	mApp = theApp;
	mBackdrop = theBackdrop;
	mTemple = theApp->mCurrentTemple;
	mFirst = 0;
	mFirstUnsolved = -1;
	mTicks = 0;
	mTextAlpha = 0;
	mFade = theBackdrop != NULL ? new TempleFade(theBackdrop) : NULL;
	mDragging = false;
	mDragOffset = 0;
	mPanel = NULL;
	mGod = new GodFigure(theApp, mTemple, this);

	bool aTutorial = mTemple == 0;
	mBackButton = MakeStripButton(mApp, this, ID_BACK, IMAGE_BACKTOISLAND, 6, -3);
	mMoreButton = MakeStripButton(mApp, this, ID_MORE, IMAGE_MOREPUZZLESBUTTON, FRAME_X + (FRAME_W - 292) / 2, 432);
	mSkipButton = MakeStripButton(mApp, this, ID_SKIP, IMAGE_SKIPTUTORIAL, 298, 436);

	BuildThumbs();

	// the puzzle the cursor starts on: the first that is not solved, or the one the player came back from
	LevelData* aLevelData = mApp->mLevelData;
	mFirstUnsolved = FindFirstUnsolved();
	int aCurrent = aLevelData->mCurrentPuzzle;
	if (aCurrent == -1 || aCurrent == 0 || aCurrent < mFirstUnsolved)
		aLevelData->mCurrentPuzzle = (mFirstUnsolved == -1) ? 0 : mFirstUnsolved;
	if (aLevelData->mCurrentPuzzle >= (int) mThumbs.size())
		aLevelData->mCurrentPuzzle = 0;
	while (ROWS * COLUMNS + mFirst <= aLevelData->mCurrentPuzzle)
		mFirst += COLUMNS;
	UpdateButtons();

	// 0x410900: when all the others are solved the mystery puzzle's chains break
	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	if (!aTutorial && aUser != NULL && !mThumbs.empty())
	{
		int& aFlags = aUser->mTempleFlags[gTemples[mTemple].mName];
		int aSolved = aUser->CountSolved(gTemples[mTemple].mName);
		if (aSolved >= (int) mThumbs.size() - 1 && !(aFlags & 1))
		{
			aFlags |= 1;
			mThumbs.back().mMystery = false;
			mApp->PlaySample(SOUND_CHAIN_BREAKING_01);
		}
	}

	// the panel of the temples that are not the cave: the backdrop in the god's colour, the board over it
	if (!aTutorial)
	{
		mPanel = new MemoryImage(mApp);
		Image* aBackdrop = mApp->mImages[IMAGE_SELECTPUZZLEBDROP];
		Image* aBoard = mApp->mImages[IMAGE_SELECTPUZZLEBOARD];
		mPanel->Create(aBackdrop->GetWidth(), aBackdrop->GetHeight());
		mPanel->SetImageMode(true, true);
		Graphics g(mPanel);
		g.SetColorizeImages(true);
		g.SetColor(Color(gTemples[mTemple].mRed, gTemples[mTemple].mGreen, gTemples[mTemple].mBlue));
		g.DrawImage(aBackdrop, 0, 0);
		g.SetColorizeImages(false);
		g.DrawImage(aBoard, aBackdrop->GetWidth() - aBoard->GetWidth(), 0);
	}
	mApp->PlaySample(SOUND_PIXELUS_INSIDE_TEMPLE_V01);
}

// 0x413a90
TempleScreen::~TempleScreen()
{
	for (size_t i = 0; i < mThumbs.size(); i++)
	{
		delete mThumbs[i].mShape;
		delete mThumbs[i].mPicture;
	}
	delete mBackButton;
	delete mMoreButton;
	delete mSkipButton;
	delete mGod;
	delete mPanel;
	delete mFade;
	delete mBackdrop;
}

void TempleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mBackButton);
	theWidgetManager->AddWidget(mMoreButton);
	theWidgetManager->AddWidget(mSkipButton);
	if (mFade != NULL)
	{
		mFade->Resize(0, 0, mWidth, mHeight);
		theWidgetManager->AddWidget(mFade);
	}
	StartMusic(mApp, MUSIC_PIXELUS_TEMPLE_SCREEN_LOOP_V01, true);
}

void TempleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mBackButton);
	theWidgetManager->RemoveWidget(mMoreButton);
	theWidgetManager->RemoveWidget(mSkipButton);
	if (mFade != NULL)
		theWidgetManager->RemoveWidget(mFade);
}

// 0x4138d0 and 0x413420: the pictures of every puzzle of the temple
void TempleScreen::BuildThumbs()
{
	for (size_t i = 0; i < mThumbs.size(); i++)
	{
		delete mThumbs[i].mShape;
		delete mThumbs[i].mPicture;
	}
	mThumbs.clear();

	const LevelData::Level* aLevel = mApp->mLevelData->GetLevel(gTemples[mTemple].mName);
	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	if (aLevel == NULL)
		return;

	UserProfile* aProfile = aUser;
	bool aMysteryLocked = mTemple != 0 && aProfile != NULL && !(aProfile->mTempleFlags[gTemples[mTemple].mName] & 1);
	Grid* aGrid = new Grid(16, 16);
	for (size_t i = 0; i < aLevel->mPuzzles.size(); i++)
	{
		Thumb aThumb;
		aThumb.mFile = aLevel->mPuzzles[i].mFile;
		aThumb.mUuid = aLevel->mPuzzles[i].mUuid;

		Buffer aBuffer;
		if (mApp->ReadBufferFromFile("levels/" + aThumb.mFile, &aBuffer))
		{
			std::string aText((const char*) aBuffer.GetDataPtr(), aBuffer.GetDataLen());
			if (aGrid->LoadFromText(aText))
			{
				LoadPuzzlePicture(mApp, aGrid);
				aThumb.mShape = MakeMiniImage(mApp, *aGrid, false);
				aThumb.mPicture = MakeMiniImage(mApp, *aGrid, true);
			}
		}

		// 0x413420: the medal says how close to the best known solution the player came
		const PuzzleRecord* aRecord = aProfile != NULL ? aProfile->GetRecord(aThumb.mUuid) : NULL;
		int aBest = mApp->mProgress->GetBestMoves(aThumb.mUuid);
		if (aRecord != NULL)
		{
			aThumb.mSolved = true;
			aThumb.mFade = 255;
			// no medal while no best solution is known
			if (aBest >= 1)
			{
				if (aProfile->IsMaxed(*aRecord, aThumb.mUuid))
					aThumb.mMedal = 3;
				else if (aRecord->mMoves <= aBest + 5)
					aThumb.mMedal = 2;
				else if (aRecord->mMoves <= aBest + 10)
					aThumb.mMedal = 1;
			}
		}
		aThumb.mMystery = aMysteryLocked && (i + 1 == aLevel->mPuzzles.size());
		if (aThumb.mSolved && aThumb.mUuid == mApp->mLastSolvedUuid)
		{
			aThumb.mFade = 1;
			mApp->mLastSolvedUuid.clear();
		}
		mThumbs.push_back(aThumb);
	}
	delete aGrid;
}

// 0x413a00: the first puzzle the player has no record for
int TempleScreen::FindFirstUnsolved()
{
	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	for (size_t i = 0; i < mThumbs.size(); i++)
		if (aUser == NULL || aUser->GetRecord(mThumbs[i].mUuid) == NULL)
			return (int) i;
	return -1;
}

int TempleScreen::GetRows() const
{
	return ((int) mThumbs.size() + COLUMNS - 1) / COLUMNS;
}

// 0x4102f0: the puzzle in the 96 pixel square under the point, among the nine in view
int TempleScreen::HitTest(int x, int y) const
{
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLUMNS; aCol++)
		{
			int aX = LEFT + PITCH * aCol;
			int aY = TOP + PITCH * aRow;
			if (x >= aX && x < aX + CELL && y >= aY && y < aY + CELL)
			{
				int anIndex = mFirst + aRow * COLUMNS + aCol;
				return anIndex < (int) mThumbs.size() ? anIndex : -1;
			}
		}
	return -1;
}

void TempleScreen::ScrollTo(int theFirst)
{
	int aMax = std::max(0, GetRows() - ROWS) * COLUMNS;
	theFirst = std::max(0, std::min(aMax, theFirst));
	if (theFirst != mFirst)
	{
		mFirst = theFirst;
		UpdateButtons();
		MarkDirty();
	}
}

// 0x410210 and 0x40fca0: the plaque shows only while there are more puzzles below; the skip button only in the cave
void TempleScreen::UpdateButtons()
{
	mMoreButton->SetVisible((int) mThumbs.size() > ROWS * COLUMNS && mFirst + ROWS * COLUMNS < (int) mThumbs.size());
	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	mSkipButton->SetVisible(mTemple == 0 && aUser != NULL && !aUser->mFlag79);
}

bool TempleScreen::SliderVisible() const
{
	return (int) mThumbs.size() > ROWS * COLUMNS;
}

void TempleScreen::Update()
{
	Widget::Update();
	mTicks++;
	mGod->Update(this);
	if (mTextAlpha < 240)
		mTextAlpha = std::min(240, mTextAlpha + 8);
	for (size_t i = 0; i < mThumbs.size(); i++)
		if (mThumbs[i].mSolved && mThumbs[i].mFade > 0 && mThumbs[i].mFade < 255)
			mThumbs[i].mFade = std::min(255, mThumbs[i].mFade + 3);
	MarkDirty();
}

// 0x4119e0
void TempleScreen::DrawThumbs(Graphics* g)
{
	LevelData* aLevelData = mApp->mLevelData;
	int aCurrent = aLevelData->mCurrentPuzzle;
	bool aTutorial = mTemple == 0;

	g->SetClipRect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H);
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLUMNS; aCol++)
		{
			int anIndex = mFirst + aRow * COLUMNS + aCol;
			if (anIndex >= (int) mThumbs.size())
				continue;
			const Thumb& aThumb = mThumbs[anIndex];
			int aX = LEFT + PITCH * aCol;
			int aY = TOP + PITCH * aRow;

			if (!aTutorial)
				g->DrawImage(mApp->mImages[IMAGE_PUZZLEPATTERN], aX - 10, aY - 10);

			if (aThumb.mSolved && aThumb.mFade >= 255 && aThumb.mPicture != NULL)
			{
				g->DrawImage(aThumb.mPicture, aX, aY);
				g->SetColorizeImages(true);
				g->SetColor(Color(255, 255, 255, 0x80));
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->DrawImage(mApp->mImages[IMAGE_PUZZLELIGHT], aX - 4, aY - 4);
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				g->SetColorizeImages(false);
			}
			else
			{
				if (aThumb.mShape != NULL)
					g->DrawImage(aThumb.mShape, aX, aY);
				if (aThumb.mSolved && aThumb.mFade > 0 && aThumb.mPicture != NULL)
				{
					// the picture of a puzzle that was just solved comes in over its shape
					g->SetColorizeImages(true);
					g->SetColor(Color(255, 255, 255, aThumb.mFade));
					g->DrawImage(aThumb.mPicture, aX, aY);
					g->SetColorizeImages(false);
				}
			}

			if (anIndex == aCurrent)
			{
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->DrawImage(mApp->mImages[IMAGE_PUZZLECURSOR], aX - 4, aY - 4);
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			}

			// 0x4119e0: the next puzzle of the lesson says where to click, in three lines that pulse between white and yellow
			if (aTutorial && anIndex == mFirstUnsolved)
			{
				g->SetFont(mApp->mFonts[FONT_ARIAL10BOLD]);
				g->SetColor(Color(255, 255, (int) (sin(mTicks * 0.05) * 30.0 + 223.0)));
				const char* aLines[3] = { "Click here", anIndex == 0 ? "for the first" : (anIndex == 8 ? "for the last" : "for the next"),
					"tutorial puzzle" };
				static const int aLineY[3] = { 0x26, 0x35, 0x44 };
				for (int i = 0; i < 3; i++)
					g->DrawString(aLines[i], aX + 0x30 - g->StringWidth(aLines[i]) / 2, aY + aLineY[i]);
			}

			// the medal, the chains of the mystery puzzle, the question mark of a tutorial puzzle that is not yet open
			if (aThumb.mSolved && aThumb.mMedal > 0)
			{
				static const int aMedals[4] = { 0, IMAGE_MEDAL_BRONZE, IMAGE_MEDAL_SILVER, IMAGE_MEDAL };
				g->DrawImage(mApp->mImages[aMedals[aThumb.mMedal]], aX + CELL - 44, aY + CELL - 44);
			}
			else if (aThumb.mMystery)
				g->DrawImage(mApp->mImages[IMAGE_LOCKPUZZLEOVERLAY], aX - 3, aY - 3);
			if (aTutorial && mFirstUnsolved != -1 && mFirstUnsolved < anIndex)
				g->DrawImage(mApp->mImages[IMAGE_TUTORIALPUZZLEOVERLAY], aX, aY);
		}
	g->ClearClipRect();
}

static bool SliderMouseInTrack(int x, int y)
{
	return x >= TempleScreen::SLIDER_X && x < TempleScreen::SLIDER_X + 16 && y >= TempleScreen::TOP && y < TempleScreen::TOP + TempleScreen::SLIDER_H;
}

void TempleScreen::DrawSlider(Graphics* g)
{
	if (!SliderVisible())
		return;
	g->DrawImage(mApp->mImages[IMAGE_SLIDERBDROP], SLIDER_X, TOP);
	g->DrawImage(mApp->mImages[IMAGE_SLIDERUP], SLIDER_X, TOP);
	g->DrawImage(mApp->mImages[IMAGE_SLIDERDOWN], SLIDER_X, TOP + SLIDER_H - 16);

	Image* aThumb = mApp->mImages[IMAGE_SLIDER];
	int aTrack = SLIDER_H - 32 - aThumb->GetHeight();
	int aMaxRow = std::max(1, GetRows() - ROWS);
	int aY = TOP + 16 + aTrack * (mFirst / COLUMNS) / aMaxRow;
	g->DrawImage(aThumb, SLIDER_X, aY);
}

// 0x412990
void TempleScreen::Draw(Graphics* g)
{
	bool aTutorial = mTemple == 0;
	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();

	if (aTutorial)
		g->DrawImage(mApp->mImages[IMAGE_TUTORIALTEMPLE], 0, 0);
	else
		g->DrawImage(mPanel, 0, 0);
	g->SetColorizeImages(false);

	if (aTutorial && aUser != NULL && !aUser->mFlag79)
		g->DrawImage(mApp->mImages[IMAGE_TREE], 244, 424);

	if (!aTutorial)
	{
		g->SetColor(Color(0xe8, 0xb9, 0x19));
		g->DrawRect(FRAME_X, FRAME_Y, FRAME_W - 1, FRAME_H - 1);

		// "Temple of" in the pediment, the name on the plate
		std::string aName = gTemples[mTemple].mName;
		for (size_t i = 0; i < aName.size(); i++)
			aName[i] = (char) toupper((unsigned char) aName[i]);
		g->SetFont(mApp->mFonts[FONT_GARAMOND14]);
		int aCenter = FRAME_X + FRAME_W / 2;
		g->SetColor(Color(0x40, 0x20, 0x00));
		g->DrawString(GT(0x50dbd0), aCenter - g->StringWidth(GT(0x50dbd0)) / 2 + 1, 41);
		g->SetColor(Color(255, 255, 255));
		g->DrawString(GT(0x50dbd0), aCenter - g->StringWidth(GT(0x50dbd0)) / 2, 40);
		g->SetFont(mApp->mFonts[FONT_GARAMOND20]);
		g->SetColor(Color(0x40, 0x20, 0x00));
		g->DrawString(aName, aCenter - g->StringWidth(aName) / 2 + 1, 73);
		g->SetColor(Color(255, 255, 255));
		g->DrawString(aName, aCenter - g->StringWidth(aName) / 2, 72);
	}
	else
	{
		// the hint of the puzzle under the cursor, in the order of the lesson
		int aCurrent = mApp->mLevelData->mCurrentPuzzle;
		if (aCurrent >= 0 && aCurrent < 9 && (mFirstUnsolved == -1 || aCurrent <= mFirstUnsolved))
		{
			g->SetFont(mApp->mFonts[FONT_GARAMOND12BOLD]);
			g->SetColor(Color(200, 200, 255, std::min(mTextAlpha, 240)));
			WriteWordWrapped2004(g, Rect(225, 20, 326, 66), GT(gTutorialHints[aCurrent]), g->GetFont()->GetLineSpacing(), 0);
		}
	}

	DrawThumbs(g);
	DrawSlider(g);

	// how many puzzles are left (0x412990: not in the tutorial's cave)
	if (aUser != NULL && !aTutorial)
	{
		int aLeft = (int) mThumbs.size() - aUser->CountSolved(gTemples[mTemple].mName);
		char aBuffer[64];
		SexyString aLine;
		if (aLeft <= 0)
			aLine = GT(0x50db80);
		else if (aLeft == 1)
			aLine = GT(0x50db9c);
		else
		{
			snprintf(aBuffer, sizeof(aBuffer), GTC(0x50dbb4), aLeft);
			aLine = aBuffer;
		}
		g->SetFont(mApp->mFonts[FONT_GARAMOND12]);
		int aCenter = FRAME_X + FRAME_W / 2;
		int aY = aTutorial ? 420 : 426;
		g->SetColor(Color(0, 0, 0));
		g->DrawString(aLine, aCenter - g->StringWidth(aLine) / 2 + 1, aY + 1);
		g->SetColor(Color(255, 255, 255));
		g->DrawString(aLine, aCenter - g->StringWidth(aLine) / 2, aY);
	}

	mGod->Draw(g);
}

void TempleScreen::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	if (mDragging)
		return;
	int aHit = HitTest(x, y);
	LevelData* aLevelData = mApp->mLevelData;
	if (aHit != aLevelData->mCurrentPuzzle)
	{
		aLevelData->mCurrentPuzzle = aHit;
		if (aHit != -1)
			mApp->PlaySample(SOUND_TEMPLE_MOUSEOVER_01);
		MarkDirty();
	}
}

void TempleScreen::MouseDrag(int x, int y)
{
	Widget::MouseDrag(x, y);
	if (mDragging)
		SliderMouse(y, false);
}

void TempleScreen::MouseUp(int x, int y)
{
	Widget::MouseUp(x, y);
	mDragging = false;
}

void TempleScreen::MouseWheel(int theDelta)
{
	ScrollTo(mFirst + (theDelta > 0 ? -COLUMNS : COLUMNS));
}

// the slider: its arrows move a row, its thumb is dragged, the track around the thumb moves a page
void TempleScreen::SliderMouse(int y, bool theStart)
{
	Image* aThumb = mApp->mImages[IMAGE_SLIDER];
	int aTrack = SLIDER_H - 32 - aThumb->GetHeight();
	int aMaxRow = std::max(1, GetRows() - ROWS);
	if (theStart)
	{
		int aThumbY = TOP + 16 + aTrack * (mFirst / COLUMNS) / aMaxRow;
		if (y < TOP + 16)
			ScrollTo(mFirst - COLUMNS);
		else if (y >= TOP + SLIDER_H - 16)
			ScrollTo(mFirst + COLUMNS);
		else if (y < aThumbY)
			ScrollTo(mFirst - ROWS * COLUMNS);
		else if (y >= aThumbY + aThumb->GetHeight())
			ScrollTo(mFirst + ROWS * COLUMNS);
		else
		{
			mDragging = true;
			mDragOffset = y - aThumbY;
		}
		return;
	}
	int aRow = (y - mDragOffset - (TOP + 16)) * aMaxRow / std::max(1, aTrack);
	ScrollTo(std::max(0, std::min(aMaxRow, aRow)) * COLUMNS);
}

// 0x411600
void TempleScreen::MouseDown(int x, int y, int theClickCount)
{
	if (theClickCount != 1)
		return;
	if (SliderVisible() && SliderMouseInTrack(x, y))
	{
		SliderMouse(y, true);
		return;
	}

	// a click on the god makes it pull a face
	if (x >= 0x11 && x <= 0x88 && y >= 0xb6 && y <= 0x191)
	{
		mGod->Poke();
		return;
	}

	LevelData* aLevelData = mApp->mLevelData;
	int aCurrent = HitTest(x, y);
	aLevelData->mCurrentPuzzle = aCurrent;
	if (aCurrent == -1)
		return;

	if (mThumbs[aCurrent].mMystery)
	{
		Dialog* aDialog = mApp->DoDialog(0x19, true, GT(0x50da88), GT(0x50da98), GT(0x50c9a0), Dialog::BUTTONS_FOOTER);
		mApp->CenterDialog(aDialog);
		return;
	}
	if (mTemple == 0 && mFirstUnsolved != -1 && mFirstUnsolved < aCurrent)
	{
		Dialog* aDialog = mApp->DoDialog(0x19, true, GT(0x50da1c), GT(0x50da28), GT(0x50c9a0), Dialog::BUTTONS_FOOTER);
		mApp->CenterDialog(aDialog);
		return;
	}

	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	// 0x427430: the puzzle's id comes from loading its file into a grid of its own, which makes the tiles (and draws their
	// pictures), and is thrown away again
	{
		Buffer aBuffer;
		if (mApp->ReadBufferFromFile("levels/" + mThumbs[aCurrent].mFile, &aBuffer))
		{
			Grid aGrid(16, 16);
			aGrid.LoadFromText(std::string((const char*) aBuffer.GetDataPtr(), aBuffer.GetDataLen()));
		}
	}
	if (aUser != NULL && mApp->mProgress->GetHistory(aUser->mId, mThumbs[aCurrent].mUuid) != NULL)
		mApp->ShowResumeQuestion();
	else
		mApp->StartPuzzle(false);
}

// 0x414510
void TempleScreen::ButtonDepress(int theId)
{
	switch (theId)
	{
	case ID_BACK:
		mApp->BackToIsland();
		break;
	case ID_SKIP:
		SkipTutorial();
		break;
	case ID_MORE:
		ScrollTo(mFirst + ROWS * COLUMNS);
		break;
	}
}

// 0x4114d0
void TempleScreen::SkipTutorial()
{
	Dialog* aDialog = mApp->DoDialog(0x1b, true, GT(0x50d95c), GT(0x50d970), "", Dialog::BUTTONS_YES_NO);
	mApp->CenterDialog(aDialog);
}
