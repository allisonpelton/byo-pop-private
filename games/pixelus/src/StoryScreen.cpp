#include "StoryScreen.h"
#include "Character.h"
#include "TempleScreen.h"
#include "PixelusApp.h"
#include "Temples.h"
#include "GameDialog.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "DialogButton.h"
#include "WidgetManager.h"
#include "Buffer.h"
#include <ctype.h>
#include <string.h>
#include <algorithm>

using namespace Sexy;

namespace
{

std::string LowerCase(const std::string& theString)
{
	std::string aResult = theString;
	for (size_t i = 0; i < aResult.size(); i++)
		aResult[i] = (char) tolower((unsigned char) aResult[i]);
	return aResult;
}

// each speech takes this much room, and the next one comes in this long after it
const int SPEECH_SPACING = 0x8e;
const double SPEECH_GAP = 2.0;

}

// 0x41cf50: mode 0 the introduction, 1 a temple has opened (from the temple the player is in), 2 the story of the
// current temple again, 3 the ending
StoryScreen::StoryScreen(PixelusApp* theApp, bool theFromMenu, int theFrom, int theTo, int theMode)
{
	mApp = theApp;
	mTime = 0;
	mMusicDelay = 1;
	mAfter = theFromMenu ? AFTER_MENU : AFTER_ISLAND;
	if (theMode == 3)
		mAfter = AFTER_ENDING;

	mContinueButton = new SoundButton(mApp, mApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS], ID_CONTINUE, this);
	mContinueButton->mLabel = GT(0x50c9a0);
	StyleButton(mApp, mContinueButton, 3);

	Load(LowerCase(std::string(gTemples[theFrom].mName) + gTemples[theTo].mName));
}

StoryScreen::~StoryScreen()
{
	delete mContinueButton;
	for (size_t i = 0; i < mSpeeches.size(); i++)
	{
		delete mSpeeches[i].mCharacter;
		delete mSpeeches[i].mPortrait;
		delete mSpeeches[i].mFramed;
	}
}

// 0x42db50: the script. A line ending in a colon starts a speech, the lines after it are what is said; the first
// empty line ends it. When there are two speeches only, they are moved down to the middle of the sky
void StoryScreen::Load(const std::string& theName)
{
	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile("data/story/" + theName + ".txt", &aBuffer))
		return;
	std::string aText((const char*) aBuffer.GetDataPtr(), aBuffer.GetDataLen());

	int aY = 0x10;
	double aStart = 0.5;
	size_t aPos = 0;
	while (aPos < aText.size())
	{
		size_t anEnd = aText.find('\n', aPos);
		if (anEnd == std::string::npos)
			anEnd = aText.size();
		std::string aLine = aText.substr(aPos, anEnd - aPos);
		aPos = anEnd + 1;
		if (!aLine.empty() && aLine[aLine.size() - 1] == '\r')
			aLine.erase(aLine.size() - 1);
		if (aLine.empty())
			break;

		if (aLine[aLine.size() - 1] == ':')
		{
			aLine.erase(aLine.size() - 1);
			int aSpeaker = CLAUDIUS;
			for (int i = 0; i < NUM_TEMPLES; i++)
				if (LowerCase(gTemples[i].mName) == LowerCase(aLine))
				{
					aSpeaker = i;
					break;
				}
			Speech aSpeech;
			aSpeech.mSpeaker = aSpeaker;
			aSpeech.mY = aY;
			aSpeech.mStart = aStart;
			aSpeech.mCharacter = new Character(mApp, aSpeaker, NULL, 2);
			// 0x42d810: the picture the speaker is drawn on, and a copy of the frame the picture is put in
			aSpeech.mPortrait = new MemoryImage(mApp);
			aSpeech.mPortrait->Create(0xa0, 0xa0);
			aSpeech.mPortrait->SetImageMode(true, true);
			Image* aFrameImage = mApp->mImages[IMAGE_STORYFRAME];
			aSpeech.mFramed = new MemoryImage(mApp);
			aSpeech.mFramed->Create(aFrameImage->GetWidth(), aFrameImage->GetHeight());
			aSpeech.mFramed->SetImageMode(true, true);
			{
				Graphics aCopy(aSpeech.mFramed);
				aCopy.DrawImage(aFrameImage, 0, 0);
			}
			mSpeeches.push_back(aSpeech);
			aStart += SPEECH_GAP;
			aY += SPEECH_SPACING;
		}
		else if (!mSpeeches.empty())
			mSpeeches.back().mLines.push_back(aLine);
	}

	if (mSpeeches.size() == 2)
		for (size_t i = 0; i < mSpeeches.size(); i++)
			mSpeeches[i].mY += 0x46;
}

void StoryScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mContinueButton);
}

void StoryScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mContinueButton);
}

// 0x41cdc0: the button sits at the lower right
void StoryScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aHeight = mApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS]->GetHeight();
	mContinueButton->Resize(theX + 0x1e5, theY + theHeight - aHeight - 0xc, 0x96, aHeight);
}

// 0x41ce50
void StoryScreen::Update()
{
	Widget::Update();
	if (mMusicDelay > 0 && --mMusicDelay == 0)
		StartMusic(mApp, MUSIC_PIXELUS_STORY_SCREEN_LOOP_V01, true);
	mTime += 0.01;
	// 0x42c9a0
	for (size_t i = 0; i < mSpeeches.size(); i++)
		if (mSpeeches[i].mStart + 0.5 < mTime)
			mSpeeches[i].mCharacter->Update(NULL);
	MarkDirty();
}

void StoryScreen::DrawSpeech(Graphics* g, const Speech& theSpeech, int theOrdinal)
{
	double anAge = mTime - theSpeech.mStart;
	if (anAge < 0)
		return;
	int anAlpha = (int) std::min(255.0, anAge * 512.0);
	bool aRight = theSpeech.mSpeaker == CLAUDIUS;	// Claudius is on the right, the gods on the left

	Image* aBubble = mApp->mImages[aRight ? IMAGE_BUBBLELEFT : IMAGE_BUBBLERIGHT];
	int aBubbleX = aRight ? 15 : 125;
	int aFrameX = aRight ? 514 : 10;
	int aY = theSpeech.mY;

	g->SetColorizeImages(true);
	// 0x42ccf0: the bubble is never more than three quarters opaque
	g->SetColor(Color(255, 255, 255, std::min(anAlpha, 0xc0)));
	g->DrawImage(aBubble, aBubbleX, aY);
	g->SetColor(Color(255, 255, 255, anAlpha));

	// 0x42ccf0: the speaker in a frame. The backdrop is stretched over the 160 by 160 picture and the figure drawn on it
	// (moved by the speaker's offset, 0x53d660), the picture is shrunk into the copy of the frame and the frame laid over it
	{
		static const int OFFSETS[11][2] = { { -5, -50 }, { 0, -160 }, { -15, -180 }, { 10, -210 }, { -5, -210 }, { -40, -190 },
			{ -15, -220 }, { -35, -190 }, { -45, -220 }, { -50, -180 }, { 15, 5 } };
		Image* aBackdrop = mApp->mImages[IMAGE_STORYFRAMEBDROP];
		Graphics aPicture(theSpeech.mPortrait);
		aPicture.DrawImage(aBackdrop, Rect(0, 0, 0xa0, 0xa0), Rect(0, 0, aBackdrop->GetWidth(), aBackdrop->GetHeight()));
		aPicture.Translate(OFFSETS[theSpeech.mSpeaker][0], OFFSETS[theSpeech.mSpeaker][1]);
		DrawCharacter(mApp, theSpeech.mCharacter, &aPicture);

		Image* aFrame = mApp->mImages[IMAGE_STORYFRAME];
		Graphics aFramed(theSpeech.mFramed);
		aFramed.DrawImage(theSpeech.mPortrait, Rect(1, 1, theSpeech.mFramed->GetWidth() - 2, theSpeech.mFramed->GetHeight() - 2),
			Rect(0, 0, 0xa0, 0xa0));
		aFramed.DrawImage(aFrame, 0, 0);
		g->DrawImage(theSpeech.mFramed, aFrameX, aY - 3);
	}

	// 0x42ccf0: what is said, a line at a time, each 0.5 s after the one before, in black on the bubble
	if (anAge > 0.5)
	{
		Font* aFont = mApp->mFonts[FONT_DIOGENES15];
		g->SetFont(aFont);
		int aLines = (int) theSpeech.mLines.size();
		int aY0 = aY + (aLines == 2 ? 0x30 : aLines == 1 ? 0x3f : 0x21);
		int aCenter = aRight ? 0xf0 : 0x18c;
		double aLeft = anAge;
		for (int i = 0; i < aLines; i++)
		{
			aLeft -= 0.5;
			if (aLeft <= 0)
				break;
			g->SetColor(Color(0, 0, 0, std::min(255, (int) (aLeft * 512.0))));
			g->DrawString(theSpeech.mLines[i], aCenter - aFont->StringWidth(theSpeech.mLines[i]) / 2, aY0 + i * 0x1e);
		}
	}
	g->SetColorizeImages(false);
}

// 0x41d290
void StoryScreen::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_STORYBACKDROP], 0, 0);
	for (size_t i = 0; i < mSpeeches.size(); i++)
		DrawSpeech(g, mSpeeches[i], (int) i);
}

// 0x41ce10: where the story leads: the island (or the menu it was asked for from)
void StoryScreen::ButtonDepress(int theId)
{
	if (theId != ID_CONTINUE)
		return;
	// the ending (0x433d00) goes to the main menu
	if (mAfter == AFTER_MENU || mAfter == AFTER_ENDING)
		mApp->EndStory(true);
	else
		mApp->EndStory(false);
}
