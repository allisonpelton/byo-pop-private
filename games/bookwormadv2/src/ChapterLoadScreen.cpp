#include "ChapterLoadScreen.h"

#include <algorithm>

#include "Color.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

ChapterLoadScreen::ChapterLoadScreen(GameApp* theApp) : mApp(theApp)
{
	Resize(225, 200, 350, 200);
	mHasAlpha = true;
	mHasTransparencies = true;
	// the hints (the constructor's list, 0x425c60: 88 of them)
	mMessages = {
		GT(0x6f929c), GT(0x6f92b0), GT(0x6f92c8), GT(0x6f92d8), GT(0x6f92ec), GT(0x6f9300),
		GT(0x6f9318), GT(0x6f9330), GT(0x6f9354), GT(0x6f936c), GT(0x6f9388), GT(0x6f93ac),
		GT(0x6f93d4), GT(0x6f93f0), GT(0x6f940c), GT(0x6f9420), GT(0x6f9434), GT(0x6f9448),
		GT(0x6f9460), GT(0x6f9480), GT(0x6f9494), GT(0x6f94b8), GT(0x6f94dc), GT(0x6f94f4),
		GT(0x6f950c), GT(0x6f9524), GT(0x6f9540), GT(0x6f9558), GT(0x6f956c), GT(0x6f9580),
		GT(0x6f9594), GT(0x6f95ac), GT(0x6f95bc), GT(0x6f95d8), GT(0x6f95ec), GT(0x6f9600),
		GT(0x6f9614), GT(0x6f9628), GT(0x6f963c), GT(0x6f9654), GT(0x6f9664), GT(0x6f9678),
		GT(0x6f9690), GT(0x6f96a4), GT(0x6f96c4), GT(0x6f96dc), GT(0x6f96f0), GT(0x6f9704),
		GT(0x6f9718), GT(0x6f9730), GT(0x6f9748), GT(0x6f9760), GT(0x6f9774), GT(0x6f9790),
		GT(0x6f97a8), GT(0x6f97bc), GT(0x6f97d0), GT(0x6f97e4), GT(0x6f9800), GT(0x6f9818),
		GT(0x6f9834), GT(0x6f9848), GT(0x6f9858), GT(0x6f9870), GT(0x6f9890), GT(0x6f98a4),
		GT(0x6f98b8), GT(0x6f98d0), GT(0x6f98ec), GT(0x6f98fc), GT(0x6f991c), GT(0x6f9934),
		GT(0x6f9948), GT(0x6f9960), GT(0x6f9974), GT(0x6f9990), GT(0x6f99a0), GT(0x6f99b4),
		GT(0x6f99cc), GT(0x6f99e8), GT(0x6f99fc), GT(0x6f9a10), GT(0x6f9a24), GT(0x6f9a34),
		GT(0x6f9a48), GT(0x6f9a58), GT(0x6f9a70), GT(0x6f9a88),
	};
	mCurrent = (unsigned int)Rand() % mMessages.size();
	mShown.push_back(mCurrent);
}

void ChapterLoadScreen::Update()
{
	// (redrawn when the app has loaded a resource, which marks the box dirty)
	if (--mTimer >= 1)
		return;
	mTimer = 50;
	// A hint not shown yet, while there are any; then any hint
	std::vector<int> aCandidates;
	for (int i = 0; i < (int)mMessages.size(); i++)
		if (std::find(mShown.begin(), mShown.end(), i) == mShown.end())
			aCandidates.push_back(i);
	if (!aCandidates.empty()) {
		mCurrent = aCandidates[(unsigned int)Rand() % aCandidates.size()];
		mShown.push_back(mCurrent);
	} else {
		mCurrent = (unsigned int)Rand() % mMessages.size();
	}
}

void ChapterLoadScreen::Draw(Graphics* g)
{
	g->DrawImageBox(Rect(0, 0, mWidth, mHeight), IMAGE_DIALOG);
	g->SetFont(FONT_LARGE_OUTLINE);
	g->SetColor(Color::White);
	g->DrawImage(IMAGE_DIALOG_BANNER_LOADING, (mWidth - IMAGE_DIALOG_BANNER_LOADING->GetWidth()) / 2, 17);
	g->SetFont(FONT_SMALL);
	g->SetColor(Color::Black);
	g->WriteString(mMessages[mCurrent], 0, 100, mWidth - 10, 0, true, 0, -1, -1);
	g->SetColor(Color::White);
	g->FillRect(70, 110, mWidth - 140, 20);
	g->SetColor(Color(255, 0, 0));
	if (mApp->mChapterLoadTotal > 0)
		g->FillRect(70, 110, (int)((double)mApp->mChapterLoadCount / mApp->mChapterLoadTotal * (mWidth - 140)), 20);
}
