// CreditsScreen: the credits (see CreditsScreen.h).

#include "CreditsScreen.h"
#include "AstroButton.h"
#include "AstroPopApp.h"
#include "Common.h"
#include "DDImage.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Res.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// The list (0x5a5708, 38 lines of a font, a text and a gap): the heads are in the large font, the names under them in the
// font of the dialogs (0xb1). The texts are the game's (GameText.h), by their address.
struct LineSpec
{
	bool			mHead;
	unsigned int	mText;
	int				mGap;
};

const LineSpec kLineSpecs[] = {
	{true, 0x564eb0, 0x32},
	{true, 0x564ea4, 0},
	{false, 0x564e94, 0x23},
	{true, 0x564e80, 0},
	{false, 0x564e70, 0x23},
	{true, 0x564e58, 0},
	{false, 0x564e3c, 0x23},
	{true, 0x564e2c, 0},
	{false, 0x564e14, 0x23},
	{true, 0x564dfc, 0},
	{false, 0x564df0, 0x23},
	{true, 0x564de4, 0},
	{false, 0x564dd4, 0x23},
	{true, 0x564dcc, 0},
	{false, 0x564dbc, 0x23},
	{true, 0x564db0, 0},
	{false, 0x564da0, 0x23},
	{true, 0x564d98, 0},
	{false, 0x564d8c, 0},
	{false, 0x564d7c, 0},
	{false, 0x564d6c, 0},
	{false, 0x564d5c, 0x23},
	{true, 0x564d48, 0},
	{false, 0x564d3c, 0x23},
	{true, 0x564d30, 0},
	{false, 0x564d24, 0x23},
	{true, 0x564d10, 0},
	{false, 0x564cfc, 0x23},
	{false, 0x564ce4, 0x23},
	{false, 0x564ccc, 0x23},
	{false, 0x564cbc, 0x23},
	{false, 0x564ca8, 0x23},
	{false, 0x564c98, 0x23},
	{false, 0x564c8c, 0x23},
	{false, 0x564c7c, 0x23},
	{false, 0x564c6c, 0x23},
	{false, 0x564c60, 0x23},
	{false, 0x564c58, 0x23}
};

const int kNumLines = sizeof(kLineSpecs) / sizeof(kLineSpecs[0]);
const int kDialogFontId = 0xb1;

}

// 0x4563a0
CreditsScreen::CreditsScreen(AstroPopApp* theApp) :
	mScrollY(0x1e0),
	mApp(theApp),
	mBackButton(NULL)
{
	mBackButton = new AstroButton(0xfb6, mApp);

	// the three pictures of the button are cut from a strip that is made like the pictures of the dialogs' buttons
	Color aColor(50, 140, 120, 255);
	MemoryImage* anInset = NULL;
	MemoryImage* aStrip = NULL;
	mApp->MakeDialogImages(aColor, aColor, &anInset, &aStrip, false);
	delete anInset;
	mBackButton->mButtonImage = mApp->CopyImage(aStrip, Rect(0, 0, 0x78, 0x21));
	mBackButton->mOverImage = mApp->CopyImage(aStrip, Rect(0x78, 0, 0x78, 0x21));
	mBackButton->mDownImage = mApp->CopyImage(aStrip, Rect(0xf0, 0, 0x78, 0x21));
	int aStripHeight = aStrip->mHeight;
	// (the original never frees the strip: a memory leak)
	delete aStrip;

	mBackButton->mLabel = "Back";
	mBackButton->SetFont(GetFontById(kDialogFontId));
	mBackButton->SetColor(0, Color(0xff, 0xff, 0));
	mBackButton->SetColor(1, Color(0xff, 0xff, 0xff));
	mBackButton->Resize(0, 0, 0x78, aStripHeight);

	// the original picks the title of a registered copy here (one of four, by a number from the game's random numbers)
	Rand();
}

// 0x43c510: the button and its three pictures go with the screen (the application takes the button off the manager first)
CreditsScreen::~CreditsScreen()
{
	delete mBackButton->mButtonImage;
	delete mBackButton->mOverImage;
	delete mBackButton->mDownImage;
	delete mBackButton;
}

// 0x435940: the button goes in the lower right corner
void CreditsScreen::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	theManager->AddWidget(mBackButton);
	theManager->BringToFront(mBackButton);
	int aHeight = mBackButton->mHeight;
	mBackButton->Resize(0x1fe, 0x1e0 - aHeight, 0x78, aHeight);
}

// 0x435990
void CreditsScreen::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	theManager->RemoveWidget(mBackButton);
}

// 0x4359c0: the credits go up a pixel every second update
void CreditsScreen::Update()
{
	Widget::Update();
	if (mUpdateCnt % 2 == 0)
		mScrollY--;
	MarkDirty();
}

void CreditsScreen::MakeLines()
{
	Font* aHeadFont = FONT_NEURO30;
	Font* aNameFont = GetFontById(kDialogFontId);
	for (int i = 0; i < kNumLines; i++)
	{
		Line aLine;
		aLine.mFont = kLineSpecs[i].mHead ? aHeadFont : aNameFont;
		aLine.mText = GT(kLineSpecs[i].mText);
		aLine.mGap = kLineSpecs[i].mGap;
		mLines.push_back(aLine);
	}
}

// 0x446a60: the lines, each centred, from where the first one is; what is above or under the screen is not drawn
void CreditsScreen::Draw(Graphics* g)
{
	g->DrawImage(IMAGE_RED_NEBULA_BG, 0, 0);
	if (mLines.empty())
		MakeLines();

	g->SetColor(Color(0xff, 0xff, 0xff));
	int aY = mScrollY;
	for (size_t i = 0; i < mLines.size(); i++)
	{
		Font* aFont = mLines[i].mFont;
		if (aFont != NULL)
		{
			int aHeight = aFont->GetHeight();
			if (aY < mHeight && aHeight + aY > 0)
			{
				g->SetFont(aFont);
				g->DrawString(mLines[i].mText, 0x140 - aFont->StringWidth(mLines[i].mText) / 2, aFont->GetAscent() + aY);
			}
			aY += aHeight;
		}
		aY += mLines[i].mGap;
	}

	// when the last line is far above the screen the credits start again under it
	if (aY < -100)
		mScrollY = 0x208;

	// the screen fades in from white
	if (mUpdateCnt < 0x19)
	{
		int aLevel = 0xff - (mUpdateCnt * 0xff) / 0x19;
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(aLevel, aLevel, aLevel));
		g->FillRect(0, 0, 0x280, 0x1e0);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}
