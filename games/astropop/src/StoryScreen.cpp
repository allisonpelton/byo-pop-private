// StoryScreen: the story between two levels (see StoryScreen.h).

#include "StoryScreen.h"
#include "AstroButton.h"
#include "AstroPopApp.h"
#include "Board.h"
#include "DDImage.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Res.h"
#include "ResourceManager.h"
#include "UserProfile.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// the fonts of the screen (the ids of the resource table)
const int kPartFontId = 0xb0;
const int kTextFontId = 0xb1;
const int kTitleFontId = 0xb2;

// the names of the stories, by pilot (0x59dd58, 0x59df38, 0x59e018 and 0x59de58 in the original, filled in by start-up
// code; the addresses of the texts in the game's image are in StoryText.inc)
struct TitleSpec
{
	int				mPilot;
	int				mNumber;
	unsigned int	mAddress;
};

const TitleSpec kTitles[] =
{
#define STORYLINE(pilot, story, page, va)
#define STORYTITLE(pilot, number, va) { pilot, number, va },
#define FINALETEXT(scene, text, va)
#include "StoryText.inc"
#undef STORYLINE
#undef STORYTITLE
#undef FINALETEXT
	{ -1, 0, 0 }
};

struct FinaleSpec
{
	int				mScene;
	int				mText;
	unsigned int	mAddress;
};

const FinaleSpec kFinaleTexts[] =
{
#define STORYLINE(pilot, story, page, va)
#define STORYTITLE(pilot, number, va)
#define FINALETEXT(scene, text, va) { scene, text, va },
#include "StoryText.inc"
#undef STORYLINE
#undef STORYTITLE
#undef FINALETEXT
	{ 0, 0, 0 }
};

// the name of a story (empty when there is none)
std::string StoryTitle(int thePilot, int theNumber)
{
	for (size_t i = 0; kTitles[i].mPilot >= 0; i++)
		if (kTitles[i].mPilot == thePilot && kTitles[i].mNumber == theNumber)
			return GT(kTitles[i].mAddress);
	return "";
}

// one of the three texts of a scene of the finale
std::string FinaleText(int theScene, int theText)
{
	for (size_t i = 0; kFinaleTexts[i].mScene != 0; i++)
		if (kFinaleTexts[i].mScene == theScene && kFinaleTexts[i].mText == theText)
			return GT(kFinaleTexts[i].mAddress);
	return "";
}

// 0x4af710: the width of one cel of a picture
int CelWidth(Image* theImage)
{
	return theImage->mWidth / theImage->mNumCols;
}

// Where a page's picture and text go on the screen (the code of 0x447400 works this out for the upper page and the
// lower page alike): the picture is on one side, the text on the other; a page without a picture has its text across the
// screen
struct PageLayout
{
	int		mPictureHeight;     // the height of the first picture
	int		mPictureWidth;      // the width of a cel of it
	int		mPictureX;
	int		mTextX;
	int		mTextWidth;
};

PageLayout LayOutPage(const StoryPage* thePage, int theScreenWidth, int theOldTextX)
{
	PageLayout aLayout;
	aLayout.mPictureHeight = thePage->mImages.empty() ? 0 : thePage->mImages[0]->mHeight;
	aLayout.mPictureWidth = thePage->mImages.empty() ? 0 : CelWidth(thePage->mImages[0]);
	aLayout.mTextX = theOldTextX;
	if (aLayout.mPictureWidth == 0)
	{
		aLayout.mTextWidth = theScreenWidth - 100;
		aLayout.mPictureX = 0;
	}
	else
	{
		aLayout.mTextWidth = (theScreenWidth - aLayout.mPictureWidth) - 0x28;
		if (thePage->mSide < 0)
		{
			aLayout.mTextX = aLayout.mPictureWidth + 0x46;
			aLayout.mPictureX = 0x32;
		}
		else
		{
			aLayout.mPictureX = (theScreenWidth - aLayout.mPictureWidth) - 0x32;
			aLayout.mTextX = 0x32;
		}
	}
	return aLayout;
}

// the picture of a page and what is laid over it
void DrawPagePicture(Graphics* g, const StoryPage* thePage, int theX, int theY, const FrameAnim& theStatic,
					 const FrameAnim& theGauge)
{
	int aCel = thePage->mAnim.GetCel();
	if (thePage->mImages.size() == 1 && thePage->mAnim.mNumFrames > 1)
		DrawCel(g, thePage->mImages[0], theX, theY, aCel);
	else
		g->DrawImage(thePage->mImages[aCel], theX, theY);

	if (thePage->mGauge)
		DrawCel(g, IMAGE_STORY_GAUGE_OVERLAY, theX, theY, theGauge.GetCel());
}

// the static over a picture is added to it
void DrawPageStatic(Graphics* g, const StoryPage* thePage, int theX, int theY, const FrameAnim& theStatic)
{
	if (thePage->mStatic)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		DrawCel(g, IMAGE_STORY_STATIC, theX, theY, theStatic.GetCel());
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}

}

// 0x44f580
StoryPage::StoryPage() :
	mAlpha(0),
	mSide(-1),
	mLevel(0),
	mBorder(2),
	mStatic(false),
	mGauge(false)
{
}

// 0x44b9d0: the lines go with the page (the pictures belong to the resources)
StoryPage::~StoryPage()
{
	for (size_t i = 0; i < mLines.size(); i++)
		delete mLines[i];
}

// 0x4567d0. The page counter calls of the original are left out (PopCap's ad statistics), and the window that tells that
// the pictures of the finale could not be loaded.
StoryScreen::StoryScreen(AstroPopApp* theApp, int theNumber, int theMode, bool theSeen, bool theFinaleAnyway) :
	mButton(NULL),
	mApp(theApp),
	mBoard(theApp->mBoard),
	mPageA(NULL),
	mPageB(NULL),
	mNumber(theNumber),
	mMode(theMode),
	mTotal(0),
	mTopBar(0),
	mBottomBar(0),
	mFadeOut(0),
	mScene(0),
	mSceneAlpha(0xff),
	mTimer(100),
	mSeen(theSeen),
	mDoneA(false),
	mDoneB(false),
	mClosing(false),
	mFinale(false),
	mBracket(NULL),
	mBracketFlipped(NULL)
{
	switch (theMode)
	{
	case 0:
		mTotal = 9;
		break;
	case 1:
	case 2:
	case 3:
		mTotal = 8;
		break;
	}

	// the finale comes after the last story of a pilot when all the pilots have been through every one of theirs
	int aProgress = 0;
	for (int i = 0; i < 4; i++)
	{
		int aStories = 0;
		mApp->mUser->GetInt(i + 0x4b, &aStories);
		aProgress += aStories;
	}
	if ((aProgress < 0x21 || theNumber != mTotal) && !theFinaleAnyway)
	{
		mFinale = false;
	}
	else
	{
		mApp->mResourceManager->StartLoadResources("EndScene");
		while (mApp->mResourceManager->LoadNextResource())
		{
		}
		if (mApp->mResourceManager->HadError() || !ExtractEndSceneResources(mApp->mResourceManager))
		{
			mApp->ShowResourceError(false);
			mApp->Shutdown();
			return;
		}
		mFinale = true;
		// the pictures of the pilots in the last scene: three frames each, the first three not at the same frame
		static const int kStartFrame[4] = { 1, 2, 1, 0 };
		for (int i = 0; i < 4; i++)
		{
			FrameAnim& anAnim = mActorAnim[i];
			anAnim.SetNumFrames(3);
			anAnim.SetDelay(0x14);
			anAnim.mHoldLast = false;
			anAnim.SetStopped(false);
			if (i < 3)
				anAnim.SetFrame(kStartFrame[i]);
		}
	}

	// the button is made like the buttons of the dialogs
	mButton = new AstroButton(BUTTON_ID_NEXT, this);
	MemoryImage* anInset = NULL;
	MemoryImage* aStrip = NULL;
	mApp->MakeDialogImages(Color(150, 170, 0, 255), mApp->GetDialogColor(2), &anInset, &aStrip, false);
	delete anInset;
	mButton->mButtonImage = mApp->CopyImage(aStrip, Rect(0, 0, 0x78, 0x21));
	mButton->mOverImage = mApp->CopyImage(aStrip, Rect(0x78, 0, 0x78, 0x21));
	mButton->mDownImage = mApp->CopyImage(aStrip, Rect(0xf0, 0, 0x78, 0x21));
	// (the original never frees the strip: a memory leak)
	delete aStrip;
	mButton->mLabel = mFinale ? "Next..." : "Continue";
	mButton->SetFont(GetFontById(kTextFontId));
	mButton->SetColor(0, Color(0xff, 0xff, 0));
	mButton->SetColor(1, Color(0xff, 0xff, 0xff));
	mButton->SetVisible(false);

	mStaticAnim.SetNumFrames(IMAGE_STORY_STATIC->mNumCols);
	mStaticAnim.SetDelay(0x14);
	mStaticAnim.mHoldLast = false;
	mStaticAnim.SetStopped(true);

	mGaugeAnim.SetNumFrames(IMAGE_STORY_GAUGE_OVERLAY->mNumCols);
	mGaugeAnim.SetDelay(10);
	mGaugeAnim.mHoldLast = false;
	mGaugeAnim.mPingPong = true;
	mGaugeAnim.SetStopped(true);

	mBracket = IMAGE_STORY_BRACKET;
	mBracketFlipped = mApp->CopyImage(IMAGE_STORY_BRACKET);
	mApp->FlipImage(mBracketFlipped);
}

// 0x44cd50 (body 0x447190): the turned-over frame goes, and the button with its pictures if it has not been pressed
// for the last time
StoryScreen::~StoryScreen()
{
	delete mBracketFlipped;
	if (mButton != NULL)
	{
		delete mButton->mButtonImage;
		delete mButton->mOverImage;
		delete mButton->mDownImage;
		delete mButton;
	}
}

// 0x435b60: the upper page comes
void StoryScreen::SetPageA(StoryPage* thePage)
{
	mPageA = thePage;
	mDoneA = false;
	if (mPageA->mStatic)
		mStaticAnim.SetStopped(false);
	if (mPageA->mGauge)
		mGaugeAnim.SetStopped(false);
}

// 0x435bb0: the lower page comes (without one the screen has nothing more to wait for)
void StoryScreen::SetPageB(StoryPage* thePage)
{
	mDoneB = thePage == NULL;
	mPageB = thePage;
	if (thePage != NULL && thePage->mStatic)
		mStaticAnim.SetStopped(false);
	if (mPageB->mGauge)
		mGaugeAnim.SetStopped(false);
}

// 0x435c00
void StoryScreen::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	theManager->AddWidget(mButton);
	theManager->BringToFront(mButton);
}

// 0x435c30
void StoryScreen::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	if (mButton != NULL)
		theManager->RemoveWidget(mButton);
}

// 0x435c60: the button is in the lower right corner
void StoryScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mButton->mButtonImage->mWidth;
	int aHeight = mButton->mButtonImage->mHeight;
	mButton->Resize((mWidth - aWidth) - 10, (mHeight - aHeight / 2) - 0x17, aWidth, aHeight);
}

// 0x43cdb0: a click brings everything in at once
void StoryScreen::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);
	mBottomBar = 0x2e;
	mTopBar = (mScene < 1) ? 0x2b : 6;
	mPageA->mAlpha = 0xff;
	for (size_t i = 0; i < mPageA->mLines.size(); i++)
		mPageA->mLines[i]->mAlpha = 0xff;
	if (mPageB != NULL)
	{
		mPageB->mAlpha = 0xff;
		for (size_t i = 0; i < mPageB->mLines.size(); i++)
			mPageB->mLines[i]->mAlpha = 0xff;
	}
	mTimer = 0;
	if (mButton != NULL)
		mButton->SetVisible(true);
	mSceneAlpha = 0xff;
}

// 0x435cc0
void StoryScreen::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTON_DOWN);
}

// the button goes with its pictures (the application's widget manager no longer has it)
void StoryScreen::CloseButton()
{
	mClosing = true;
	mButton->SetVisible(false);
	mApp->mWidgetManager->RemoveWidget(mButton);
	delete mButton->mButtonImage;
	delete mButton->mOverImage;
	delete mButton->mDownImage;
	mApp->SafeDeleteWidget(mButton);
	mButton = NULL;
}

// part of 0x456d90: the three pictures of a scene of the finale. Scene 5 (all the pilots in one picture) needs none.
void StoryScreen::StartScene(int theScene)
{
	for (int i = 0; i < 3; i++)
		mScenePicture[i].mImages.clear();

	switch (theScene)
	{
	case 1:
		mScenePicture[0].mImages.push_back(IMAGE_ES_SPROCKET_WORRY1);
		mScenePicture[0].mImages.push_back(IMAGE_ES_SPROCKET_WORRY2);
		mScenePicture[1].mImages.push_back(IMAGE_ES_VECTOR_WORRY1);
		mScenePicture[1].mImages.push_back(IMAGE_ES_VECTOR_WORRY2);
		mScenePicture[2].mImages.push_back(IMAGE_ES_TURBOT_BLINK);
		mScenePicture[2].mImages.push_back(IMAGE_ES_TURBOT_FORWARD);
		break;
	case 2:
		mScenePicture[0].mImages.push_back(IMAGE_ES_TURBOT_BLINK);
		mScenePicture[0].mImages.push_back(IMAGE_ES_TURBOT_FORWARD);
		mScenePicture[1].mImages.push_back(IMAGE_ES_VECTOR_SPROCKET_CONFUSED);
		mScenePicture[2].mImages.push_back(IMAGE_ES_VIXX_BLINK);
		mScenePicture[2].mImages.push_back(IMAGE_ES_VIXX_FORWARD);
		break;
	case 3:
		mScenePicture[0].mImages.push_back(IMAGE_ES_TURBOT_BLINK);
		mScenePicture[0].mImages.push_back(IMAGE_ES_TURBOT_FORWARD);
		mScenePicture[1].mImages.push_back(IMAGE_ES_VECTOR_SPROCKET_HUG);
		mScenePicture[2].mImages.push_back(IMAGE_ES_TURBOT_HAPPY1);
		mScenePicture[2].mImages.push_back(IMAGE_ES_TURBOT_HAPPY2);
		mScenePicture[2].mImages.push_back(IMAGE_ES_TURBOT_HAPPY3);
		break;
	case 4:
		mScenePicture[0].mImages.push_back(IMAGE_ES_VIXX_BLINK);
		mScenePicture[0].mImages.push_back(IMAGE_ES_VIXX_FORWARD);
		mScenePicture[1].mImages.push_back(IMAGE_ES_VECTOR_HAPPY1);
		mScenePicture[1].mImages.push_back(IMAGE_ES_VECTOR_HAPPY2);
		mScenePicture[1].mImages.push_back(IMAGE_ES_VECTOR_HAPPY3);
		mScenePicture[2].mImages.push_back(IMAGE_ES_VECTOR_VIXX_KISS);
		break;
	default:
		return;
	}

	for (int i = 0; i < 3; i++)
	{
		ScenePicture& aPicture = mScenePicture[i];
		aPicture.mAnim.SetNumFrames((int) aPicture.mImages.size());
		aPicture.mAnim.SetDelay(0x19);
		aPicture.mAnim.mHoldLast = false;
		aPicture.mAnim.SetStopped(false);
		aPicture.mSide = (i == 1) ? 1 : -1;
	}
}

// 0x456d90: the button takes the story away, or goes on to the next scene of the finale
void StoryScreen::ButtonDepress(int theId)
{
	if (theId != BUTTON_ID_NEXT)
		return;

	if (!mFinale)
	{
		CloseButton();
		return;
	}

	mScene++;
	mTopBar = 6;
	mSceneAlpha = 0;
	if (mScene > 5)
	{
		CloseButton();
		return;
	}
	if (mScene == 5)
		mButton->mLabel = "Done";
	else if (mScene == 4)
		mButton->mLabel = "Next...";
	StartScene(mScene);
}

// 0x469140
void StoryScreen::Update()
{
	Widget::Update();

	if (mScene == 0)
	{
		if (mTopBar < 0x2b || mBottomBar < 0x2e)
		{
			// the bars grow a pixel every second update
			if ((mUpdateCnt & 1) == 0)
			{
				mTopBar++;
				mBottomBar++;
			}
			int aDone = 0;
			if (mTopBar > 0x2a)
			{
				mTopBar = 0x2b;
				aDone = 1;
			}
			if (mBottomBar > 0x2d)
			{
				aDone++;
				mBottomBar = 0x2e;
			}
			if (aDone == 2 && mSeen)
				mButton->SetVisible(true);
		}
		else if (!mDoneA)
		{
			// the upper page: the picture fades in, then the lines one after another
			if (mPageA->mImages.empty() || mPageA->mAlpha > 0xfe)
			{
				for (size_t i = 0; i < mPageA->mLines.size(); i++)
				{
					StoryLine* aLine = mPageA->mLines[i];
					aLine->mAlpha += 2;
					if (aLine->mAlpha > 0xfe)
						mPageA->mLines[i]->mAlpha = 0xff;
					if (mPageA->mLines[i]->mAlpha < 0xff)
						break;
					if (i == mPageA->mLines.size() - 1 && mPageA->mLines[i]->mAlpha == 0xff)
						mDoneA = true;
				}
			}
			else
			{
				mPageA->mAlpha += 4;
				if (mPageA->mAlpha > 0xfe)
					mPageA->mAlpha = 0xff;
			}
		}
		else if (!mDoneB)
		{
			// the lower page comes after the timer has run out
			mTimer--;
			if (mTimer < 1)
			{
				mTimer = 0;
				if (mPageB->mImages.empty() || mPageB->mAlpha > 0xfe)
				{
					for (size_t i = 0; i < mPageB->mLines.size(); i++)
					{
						StoryLine* aLine = mPageB->mLines[i];
						aLine->mAlpha += 2;
						if (aLine->mAlpha > 0xfe)
							mPageB->mLines[i]->mAlpha = 0xff;
						// (the stories whose lower page the player must watch fully before the rest come in: the two
						// last pilots' always, the first pilot's before the third and the second pilot's before the
						// fourth)
						if (mPageB->mLines[i]->mAlpha < 0xff &&
							((mMode == 2 || mMode == 3) || (mMode == 0 && mNumber != 3) || (mMode == 1 && mNumber != 4)))
							break;
						if (i == mPageB->mLines.size() - 1 && mPageB->mLines[i]->mAlpha == 0xff)
							mDoneB = true;
					}
				}
				else
				{
					mPageB->mAlpha += 4;
					if (mPageB->mAlpha > 0xfe)
						mPageB->mAlpha = 0xff;
				}
			}
		}
		else if (mButton != NULL && mTimer < 1)
		{
			mButton->SetVisible(true);
		}

		if (!mPageA->mImages.empty())
		{
			mPageA->mAlpha = 0xff;
			mPageA->mAnim.Update();
		}
		if (mPageB != NULL && !mPageB->mImages.empty() && mPageB->mAlpha == 0xff)
			mPageB->mAnim.Update();
	}
	else
	{
		if (mScene < 5)
		{
			for (int i = 0; i < 3; i++)
				mScenePicture[i].mAnim.Update();
			MarkDirty();
		}
		if (mSceneAlpha < 0xff)
		{
			mSceneAlpha += 2;
			if (mSceneAlpha > 0xfe)
				mSceneAlpha = 0xff;
		}
	}

	if (!mStaticAnim.mStopped)
	{
		MarkDirty();
		mStaticAnim.Update();
	}
	if (!mGaugeAnim.mStopped)
	{
		MarkDirty();
		mGaugeAnim.Update();
	}

	if (mClosing)
	{
		mFadeOut += 6;
		if (mFadeOut > 0xff)
		{
			// (the original tells its page counter that the page "EndScene" is done: PopCap's ad statistics, left out)
			mFadeOut = 0xff;
			mClosing = false;
			mBoard->CloseStory();
			return;
		}
	}
	if (mFinale)
	{
		mActorAnim[3].Update();
		mActorAnim[0].Update();
		mActorAnim[2].Update();
		mActorAnim[1].Update();
	}

	// with nothing left to move the screen is not drawn again
	if (mDoneA && !mClosing && mDoneB)
	{
		if (mPageA->mImages.empty())
		{
			if (mPageB == NULL)
				return;
			if (mPageB->mImages.empty())
				return;
		}
	}
	MarkDirty();
}

// part of 0x447400: the scenes of the finale. Scenes 1 to 4 show three pictures on the left and right with three texts
// between them; scene 5 shows the four pilots in one picture.
void StoryScreen::DrawFinale(Graphics* g)
{
	if (mScene > 4)
	{
		int aX = mWidth / 2 - IMAGE_ES_FINAL->mWidth / 2;
		if (mSceneAlpha < 0xff)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, mSceneAlpha));
		}

		// each pilot's picture: the cel follows the pilot's own frame counter (the second cel of Sprocket, Turbot and
		// Vixx follows the first pilot's counter, as in the original)
		const FrameAnim& aVector = mActorAnim[0];
		int aVectorCel = aVector.GetCel();
		Image* aVectorImage = IMAGE_ES_VECTOR_HAPPY1_ALPHA;
		if (aVector.mNumFrames != 1 && aVector.mFrame != 0)
		{
			aVectorImage = IMAGE_ES_VECTOR_HAPPY3_ALPHA;
			if (aVector.mNumFrames != 1 && aVector.mFrame == 1)
				aVectorImage = IMAGE_ES_VECTOR_HAPPY2_ALPHA;
		}
		(void) aVectorCel;

		Image* aSprocketImage = IMAGE_ES_SPROCKET_HAPPY1_ALPHA;
		if (mActorAnim[1].mNumFrames != 1 && mActorAnim[1].mFrame != 0)
		{
			aSprocketImage = IMAGE_ES_SPROCKET_HAPPY3_ALPHA;
			if (aVector.mNumFrames != 1 && aVector.mFrame == 1)
				aSprocketImage = IMAGE_ES_SPROCKET_HAPPY2_ALPHA;
		}

		Image* aTurbotImage = IMAGE_ES_TURBOT_HAPPY1_ALPHA;
		if (mActorAnim[2].mNumFrames != 1 && mActorAnim[2].mFrame != 0)
		{
			aTurbotImage = IMAGE_ES_TURBOT_HAPPY3_ALPHA;
			if (aVector.mNumFrames != 1 && aVector.mFrame == 1)
				aTurbotImage = IMAGE_ES_TURBOT_HAPPY2_ALPHA;
		}

		Image* aVixxImage;
		if (mActorAnim[3].mNumFrames == 1 || mActorAnim[3].mFrame == 0)
			aVixxImage = IMAGE_ES_VIXX_HAPPY1_ALPHA;
		else if (aVector.mNumFrames == 1 || aVector.mFrame != 1)
			aVixxImage = IMAGE_ES_VIXX_HAPPY3_ALPHA;
		else
			aVixxImage = IMAGE_ES_VIXX_HAPPY2_ALPHA;

		// where each picture sits (the original reads these from a table of tunable numbers, 0x4f1e40, and has none)
		g->DrawImage(aVectorImage, 0xa8 + aX, 0x2a);
		g->DrawImage(aVixxImage, 8 + aX, 0x6f + 0x2a);
		g->DrawImage(aSprocketImage, 0x142 + aX, 0x6d + 0x2a);
		g->DrawImage(aTurbotImage, 0xaa + aX, 0xe0 + 0x2a);
		g->DrawImage(IMAGE_ES_FINAL, aX, 0x2a);
		if (mSceneAlpha < 0xff)
			g->SetColorizeImages(false);
		return;
	}

	Font* aPartFont = GetFontById(kPartFontId);
	g->SetFont(aPartFont);
	g->SetColor(Color(0xff, 0xff, 0xff, mSceneAlpha));
	std::string aLabel = StrFormat(GTC(0x564f94), mScene, 5);
	g->DrawString(aLabel, 10, (mHeight - aPartFont->GetHeight() / 2) - 0x17 + aPartFont->GetAscent());

	int aPictureWidth = mScenePicture[0].mImages[0]->mWidth;
	int aPictureHeight = mScenePicture[0].mImages[0]->mHeight;

	// the three pictures, one under the other, framed, each fading in with the scene
	int aY[3] = { 0x31, aPictureHeight + 0x2f, aPictureHeight * 2 + 10 };
	int aPictureX[3];
	for (int i = 0; i < 3; i++)
	{
		const ScenePicture& aPicture = mScenePicture[i];
		aPictureX[i] = (aPicture.mSide < 0) ? 0x1e : (mWidth - aPictureWidth) - 0x1e;
		int aWidth = aPictureWidth;
		if (i == 2 && mScene == 4)
			aWidth = aPicture.mImages[0]->mWidth;
		g->SetColor(Color(0xff, 0xff, 0xff, mSceneAlpha));
		g->DrawRect(aPictureX[i] - 2, aY[i], aWidth + 4, aPictureHeight + 4);
		if (mSceneAlpha < 0xff)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, mSceneAlpha));
		}
		g->DrawImage(aPicture.mImages[aPicture.mAnim.GetCel()], aPictureX[i], aY[i] + 2);
		g->SetColorizeImages(false);
	}

	// the three texts, between the pictures' columns: the first green, the second yellow, the third white
	g->SetFont(GetFontById(kTextFontId));
	std::string aText[3];
	for (int i = 0; i < 3; i++)
		aText[i] = FinaleText(mScene, i);

	// (the heights are measured with a graphics that draws nothing)
	Graphics aTestG;
	aTestG.SetFont(g->GetFont());
	int aTextWidth = mWidth - (aPictureWidth * 2 + 0x78);
	int aHeightA = WriteWordWrapped(&aTestG, Rect(0, 0, aTextWidth, aPictureHeight), aText[0], -1, -1);
	int aJustify = (1 < mScene) ? 0 : -1;
	int aHeightB = WriteWordWrapped(&aTestG, Rect(0, 0, aTextWidth, aPictureHeight), aText[1], -1, aJustify);
	int aHeightC = WriteWordWrapped(&aTestG, Rect(0, 0, aTextWidth, aPictureHeight), aText[2], -1, -1);

	int aWidthC = aPictureWidth;
	if (mScene == 4)
		aWidthC = mScenePicture[2].mImages[0]->mWidth;
	Rect aRectA(aPictureWidth + 0x28, (aPictureHeight / 2 - aHeightA / 2) + 0x31, mWidth - (aPictureWidth * 2 + 0x46),
				aPictureHeight);
	Rect aRectB(aPictureWidth + 0x28, (aPictureHeight / 2 - aHeightB / 2) + aPictureHeight + 0x1d,
				mWidth - (aPictureWidth * 2 + 0x46), aPictureHeight);
	Rect aRectC(aWidthC + 0x28, (aWidthC / 2 - aHeightC / 2) + (aPictureHeight * 2 + 0xc),
				mWidth - (aWidthC * 2 + 0x46), aPictureHeight);
	g->SetColor(Color(0, 0xff, 0, mSceneAlpha));
	WriteWordWrapped(g, aRectA, aText[0], -1, -1);
	g->SetColor(Color(0xff, 0xff, 0, mSceneAlpha));
	WriteWordWrapped(g, aRectB, aText[1], -1, aJustify);
	g->SetColor(Color(0xff, 0xff, 0xff, mSceneAlpha));
	WriteWordWrapped(g, aRectC, aText[2], -1, -1);
}

// part of 0x447400: a page of the story. (The text of the finished story, the name of the next one, shows once both pages
// are done.)
void StoryScreen::DrawStory(Graphics* g)
{
	Font* aPartFont = GetFontById(kPartFontId);
	g->SetFont(aPartFont);
	g->SetColor(Color(0xff, 0xff, 0xff));
	std::string aPart = StrFormat(GTC(0x565034), mNumber, mTotal);
	int aLabelY = (mHeight - aPartFont->GetHeight() / 2) - 0x17 + aPartFont->GetAscent();
	g->DrawString(aPart, 10, aLabelY);

	// on the last story of a pilot, once it is all there: the call to play the other pilots
	if (mNumber == mTotal && !mFinale && mDoneA && mDoneB)
	{
		int aPhase = mUpdateCnt & 0x7f;
		if (aPhase > 0x3f)
			aPhase = 0x7f - aPhase;
		g->SetColor(Color(0, 0xff, 0, aPhase * 2 + 0x80));
		WriteString(g, GT(0x565004), aPartFont->StringWidth(aPart) + 0x31, aLabelY, 300, -1, true, 0, -1);
	}

	// the upper page's picture
	PageLayout aLayoutA = LayOutPage(mPageA, mWidth, 0);
	g->SetColor(Color(0xff, 0xff, 0xff, mPageA->mAlpha));
	g->DrawRect(aLayoutA.mPictureX - mPageA->mBorder, 0x68 - mPageA->mBorder, aLayoutA.mPictureWidth + mPageA->mBorder * 2,
				aLayoutA.mPictureHeight + mPageA->mBorder * 2);
	if (mPageA->mAlpha < 0xff)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, mPageA->mAlpha));
	}
	DrawPagePicture(g, mPageA, aLayoutA.mPictureX, 0x68, mStaticAnim, mGaugeAnim);

	// the name of the story in red with the same in white over it, and the name of the next below once both pages are in
	g->SetFont(GetFontById(kTitleFontId));
	std::string aTitle = StoryTitle(mMode, mNumber);
	std::string aNext = (mNumber < mTotal) ? StoryTitle(mMode, mNumber + 1) : std::string();
	if (aNext != "")
		aNext = GT(0x564ff0) + aNext + GT(0x565000);
	if (mFinale)
		aNext = GT(0x564fcc);

	g->SetColor(Color(0xff, 0, 0, mPageA->mAlpha));
	Font* aTitleFont = g->GetFont();
	int aTitleWidth = aTitleFont->StringWidth(aTitle);
	g->DrawString(aTitle, (mWidth / 2 - aTitleWidth / 2) + 3, 9 + aTitleFont->GetAscent());
	g->SetColor(Color(0xff, 0xff, 0xff, mPageA->mAlpha));
	g->DrawString(aTitle, mWidth / 2 - aTitleWidth / 2, aTitleFont->GetAscent() + 6);

	if (aNext != "" && mDoneA && mDoneB)
	{
		g->SetColor(Color(0xff, 0xff, 0xff));
		g->SetFont(GetFontById(kTextFontId));
		g->DrawString(aNext, mWidth / 2 - g->GetFont()->StringWidth(aNext) / 2, aLabelY);
	}

	DrawPageStatic(g, mPageA, aLayoutA.mPictureX, 0x68, mStaticAnim);
	g->SetColorizeImages(false);

	// the lines of the upper page, beside the picture
	Font* aTextFont = GetFontById(kTextFontId);
	g->SetFont(aTextFont);
	int aY = 0x68;
	int aLineHeight = aTextFont->GetHeight();
	int aBlockHeight = aLineHeight * (int) mPageA->mLines.size();
	if (aBlockHeight < aLayoutA.mPictureHeight)
	{
		aBlockHeight /= 2;
		aY = (aLayoutA.mPictureHeight / 2 - aBlockHeight) + 0x68;
	}
	for (size_t i = 0; i < mPageA->mLines.size(); i++)
	{
		StoryLine* aLine = mPageA->mLines[i];
		if (aLine->mAlpha < 1)
			break;
		g->SetColor(Color(aLine->mColor.mRed, aLine->mColor.mGreen, aLine->mColor.mBlue, aLine->mAlpha));
		WriteString(g, aLine->mText, aLayoutA.mTextX, aY, aLayoutA.mTextWidth, -1, true, 0, -1);
		aY += aTextFont->GetHeight() + aLine->mGap;
	}

	// the lower page
	if (mPageB == NULL)
		return;
	PageLayout aLayoutB = LayOutPage(mPageB, mWidth, aLayoutA.mTextX);
	if (aLayoutB.mPictureHeight > 0)
	{
		g->SetColor(Color(0xff, 0xff, 0xff, mPageB->mAlpha));
		g->DrawRect(aLayoutB.mPictureX - mPageB->mBorder, 0xf4 - mPageB->mBorder,
					aLayoutB.mPictureWidth + mPageB->mBorder * 2, aLayoutB.mPictureHeight + mPageB->mBorder * 2);
		if (mPageB->mAlpha < 0xff)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, mPageB->mAlpha));
		}
		DrawPagePicture(g, mPageB, aLayoutB.mPictureX, 0xf4, mStaticAnim, mGaugeAnim);
		DrawPageStatic(g, mPageB, aLayoutB.mPictureX, 0xf4, mStaticAnim);
		g->SetColorizeImages(false);
	}

	g->SetFont(aTextFont);
	int aNumLines = (int) mPageB->mLines.size();
	aY = ((aNumLines < 7) ? -0x14 : 0) + 0x108;
	aLineHeight = aTextFont->GetHeight();
	if (aLineHeight * aNumLines < aLayoutB.mPictureHeight)
		aY += aLayoutB.mPictureHeight / 2 - (aLineHeight * aNumLines) / 2;
	for (size_t i = 0; i < mPageB->mLines.size(); i++)
	{
		StoryLine* aLine = mPageB->mLines[i];
		if (aLine->mAlpha < 1)
			break;
		g->SetColor(Color(aLine->mColor.mRed, aLine->mColor.mGreen, aLine->mColor.mBlue, aLine->mAlpha));
		WriteString(g, aLine->mText, aLayoutB.mTextX, aY, aLayoutB.mTextWidth, -1, true, 0, -1);
		aY += aTextFont->GetHeight() + aLine->mGap;
	}
}

// 0x447400
void StoryScreen::Draw(Graphics* g)
{
	g->DrawImage(IMAGE_RED_NEBULA_BG, 0, 0);

	if (mTopBar == 0 || mBottomBar == 0)
	{
		// white at first, then the picture comes through
		if (mUpdateCnt >= 100)
			return;
		int aLevel = (mUpdateCnt < 0x10) ? 0 : mUpdateCnt - 0xf;
		aLevel = 0xff - (aLevel * 0xff) / 100;
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(aLevel, aLevel, aLevel));
		g->FillRect(0, 0, 0x280, 0x1e0);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		return;
	}

	// the bars above and below, and the frames at the edges of the picture between them
	g->SetColor(Color(0, 0, 0));
	g->FillRect(0, 0, mWidth, mTopBar);
	g->FillRect(0, mHeight - mBottomBar, mWidth, mBottomBar);
	g->DrawImage(mBracket, 0, (mScene == 0) ? mTopBar : 6);
	g->DrawImage(mBracketFlipped, 0, (mHeight - mBracketFlipped->mHeight) - mBottomBar);

	if (mScene != 0)
	{
		DrawFinale(g);
	}
	else
	{
		if (mTopBar >= 0x2b && mBottomBar > 0x2d)
			DrawStory(g);

		// the screen comes out of white
		if (mUpdateCnt < 100)
		{
			int aLevel = (mUpdateCnt < 0x10) ? 0 : mUpdateCnt - 0xf;
			aLevel = 0xff - (aLevel * 0xff) / 100;
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColor(Color(aLevel, aLevel, aLevel));
			g->FillRect(0, 0, 0x280, 0x1e0);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}

	// and goes white as it closes
	if (mClosing)
	{
		g->SetColor(Color(0xff, 0xff, 0xff, mFadeOut));
		g->FillRect(0, 0, 0x280, 0x1e0);
	}
}
