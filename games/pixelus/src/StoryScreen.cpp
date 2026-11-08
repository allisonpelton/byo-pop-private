#include "StoryScreen.h"
#include "Character.h"
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
		delete mSpeeches[i].mCharacter;
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
	int anAlpha = (int) std::min(255.0, anAge / 0.3 * 255.0);
	bool aRight = theSpeech.mSpeaker == CLAUDIUS;	// Claudius is on the right, the gods on the left

	Image* aBubble = mApp->mImages[aRight ? IMAGE_BUBBLELEFT : IMAGE_BUBBLERIGHT];
	int aBubbleX = aRight ? 15 : 125;
	int aFrameX = aRight ? 520 : 8;
	int aY = theSpeech.mY;

	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, anAlpha));
	g->DrawImage(aBubble, aBubbleX, aY);

	// the speaker in a frame
	Image* aFrame = mApp->mImages[IMAGE_STORYFRAME];
	int aSize = aFrame->GetWidth();
	g->DrawImage(mApp->mImages[IMAGE_STORYFRAMEBDROP], aFrameX, aY);
	if (aRight)
	{
		Image* aFace = mApp->mImages[IMAGE_STORY_CLAUDIUS];
		g->DrawImage(aFace, Rect(aFrameX + 4, aY + 4, aSize - 8, aSize - 8), Rect(0, 0, aFace->GetWidth(), aFace->GetHeight()));
	}
	else
	{
		MemoryImage* aHead = mApp->mHeads[theSpeech.mSpeaker][2];
		if (aHead != NULL)
		{
			Graphics aClip(*g);
			aClip.ClipRect(aFrameX + 3, aY + 3, aSize - 6, aSize - 6);
			double aScale = std::min((aSize - 6.0) / aHead->GetWidth(), (aSize - 6.0) / aHead->GetHeight());
			int aWidth = (int) (aHead->GetWidth() * aScale), aHeight = (int) (aHead->GetHeight() * aScale);
			aClip.DrawImage(aHead, Rect(aFrameX + (aSize - aWidth) / 2, aY + (aSize - aHeight) / 2, aWidth, aHeight),
				Rect(0, 0, aHead->GetWidth(), aHead->GetHeight()));
		}
	}
	g->DrawImage(aFrame, aFrameX, aY);

	// what is said, in the middle of the bubble's body
	Font* aFont = mApp->mFonts[FONT_GARAMOND14];
	g->SetFont(aFont);
	g->SetColor(Color(0x3c, 0x28, 0x14, anAlpha));
	int aLines = (int) theSpeech.mLines.size();
	int aSpacing = aFont->GetLineSpacing();
	int aTop = aY + 58 - aLines * aSpacing / 2 + aFont->GetAscent() - 4;
	int aCenter = aBubbleX + (aRight ? 235 : 265);
	for (int i = 0; i < aLines; i++)
		g->DrawString(theSpeech.mLines[i], aCenter - aFont->StringWidth(theSpeech.mLines[i]) / 2, aTop + i * aSpacing);
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
