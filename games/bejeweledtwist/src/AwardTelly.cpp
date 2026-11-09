#include "AwardTelly.h"

#include "Board.h"
#include "Challenge.h"
#include "EffectsManager.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Profile.h"
#include "RankBonusWidget.h"
#include "Ranks.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "TwistButton.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

AwardTelly::AwardTelly()
{
	mFXManager = new EffectsManager();
	mFXManager2 = new EffectsManager();
	mFadeIn = 0;
	mKind = 0;
	mState = 0;
	mButton = nullptr;
	mClip = false;
	mPressed = false;
	mCounter = 0;
	mRankBonus = nullptr;
	mTrophy = nullptr;
	Resize(0, 0, gApp->mWidth, gApp->mHeight);
}

AwardTelly::~AwardTelly()
{
	delete mFXManager;
	delete mFXManager2;
	RemoveAllWidgets(true, false);
}

void AwardTelly::Resize(int theX, int theY, int theWidth, int theHeight)
{
	(void)theX; (void)theY; (void)theWidth; (void)theHeight;
	Widget::Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mFXManager->Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mFXManager2->Resize(0, 0, gApp->mWidth, gApp->mHeight);
}

void AwardTelly::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	if (mKind != 1 && mKind != 2) {
		mButton = new TwistButton(0, IMAGE_RANKBAR_CONTINUEBUTTON, this);
		mButton->mDoFinger = true;
		int aWidth = IMAGE_RANKBAR_CONTINUEBUTTON->GetCelWidth();
		int aHeight = IMAGE_RANKBAR_CONTINUEBUTTON->GetCelHeight();
		int aY = Ftol((double)gApp->mScale * GetImageOffsetY(IMAGE_RANKBAR_CONTINUEBUTTON_ID));
		mButton->Resize(mWidth / 2 - aWidth / 2, aY, aWidth, aHeight);
		AddWidget(mButton);
	}
	AddWidget(mFXManager2);
}

void AwardTelly::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	mClicked = false;
	if (mRankBonus != nullptr) {
		mRankBonus->mParent->RemoveWidget(mRankBonus);
		delete mRankBonus;
		mRankBonus = nullptr;
	}
	if (mButton != nullptr) {
		RemoveWidget(mButton);
		delete mButton;
		mButton = nullptr;
	}
	RemoveWidget(mFXManager2);
}

void AwardTelly::MouseDown(int x, int y, int theClickCount)
{
	(void)x; (void)y; (void)theClickCount;
	if (mButton != nullptr && !mButton->mDisabled)
		mPressed = true;
}

void AwardTelly::ButtonPress(int theId)
{
	(void)theId;
	gApp->PlaySample(SOUND_CLICK);
}

void AwardTelly::ButtonDepress(int theId)
{
	(void)theId;
	mPressed = true;
}

void AwardTelly::ShowRank(int theRank)
{
	if (mKind == 0) {
		mKind = 3;
		if (mState == 0) {
			mState = 1;
			mShow.SetCurve(GTS(0x7dbcd0));
		}
	} else {
		gApp->PlaySample(SOUND_TROPHY_AWARDED);
	}
	mRank = theRank;
	if (98 < theRank)
		theRank = 100;
	mBadge = theRank / 5;
	mOldBadge = (theRank - 1) / 5;
	mMark = theRank % 10;
	mOldMark = (theRank - 1) % 10;
	mRankUpAlpha.SetCurve(GTS(0x7dbca4));
	mOldTextAlpha.SetCurve(GTS(0x7dbc74));
	mNewTextAlpha.SetCurve(GTS(0x7dbc48));
	mAlpha.SetCurve(GTS(0x7dbc28));
	mChange.SetConstant(1.0);
	mOldMarkScale.SetConstant(1.0);
	mOldBadgeAlpha.SetConstant(1.0);
}

void AwardTelly::ShowTrophy(Challenge* theChallenge, bool theFlag)
{
	if (mKind == 0) {
		mKind = 3;
		if (mState == 0) {
			mState = 1;
			mShow.SetCurve(GTS(0x7dbc0c));
		}
	} else {
		gApp->PlaySample(SOUND_TROPHY_AWARDED);
	}
	gApp->PlaySample(SOUND_TROPHY_AWARDED);
	// the bonus: 2 more than three times the challenge's number; a night challenge's by its score between the low
	// and the best, up to 15
	int aBonus = theChallenge->mNumber * 3 + 2;
	if ((theChallenge->mFlags & Challenge::FLAG_NIGHTMODE) != 0 && theChallenge->mBestScore != 0) {
		int aScore = gBoard->mChallengeBest;
		if (gBoard->mChallengeBest < gBoard->mChallengeCount)
			aScore = gBoard->mChallengeCount;
		float aFrac = (float)((double)(aScore - theChallenge->mLowScore) /
			((double)theChallenge->mBestScore - (double)theChallenge->mLowScore));
		if (0.0f < aFrac)
			aFrac = (float)std::pow((double)aFrac, (double)0.8f);  // 0x670a49
		double aScaled = (double)aFrac * 15.0;
		if (aScaled < 0.0)
			aScaled = 0.0;
		aBonus = Ftol(aScaled);
	}
	int aY = Ftol((double)mHeight * 0.2750000059604645);
	mRankBonus = new RankBonusWidget(gApp->mProfile->mRankPoints, aBonus, mWidth / 2, aY);
	mRankBonus->Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mRankBonus->mZOrder = mZOrder + 1;
	gApp->mWidgetManager->AddWidget(mRankBonus);
	mTrophy = theChallenge;
	mIsNightTrophy = theFlag;
	mState = 4;
	mKind = 1;
	mTrophyIn.SetCurve(GTS(0x7dbbd4));
	mTrophyScale.SetCurve(GTS(0x7dbba4));
}

void AwardTelly::Update()
{
	TwistWidget::Update();
	if (-1 < gApp->mAutoPlayMode && mUpdateCnt == 500)
		static_cast<ButtonListener*>(this)->ButtonDepress(0);
	mFXManager->Update();
	if (mState < 3) {
		double aShow = (double)mFadeIn + 0.019999999552965164;
		if (1.0 < aShow)
			aShow = 1.0;
		mFadeIn = (float)aShow;
	}
	if (mState == 1) {
		double aShow = (double)mFadeIn - 0.019999999552965164;
		if (aShow < 0.0)
			aShow = 0.0;
		mFadeIn = (float)aShow;
	}
	mChange.IncInVal();
	if (mButton != nullptr && mNewTextAlpha.CheckInThreshold(0.4000999927520752))
		gApp->PlaySample(SOUND_TROPHY_AWARDED);
	switch (mState) {
	case 1:
		if (!mShow.IncInVal()) {
			// In: the old badge turns into the new
			mState = 2;
			mChange.SetCurve(GTS(0x7dbd94));
			mOldMarkScale.SetCurve(GTS(0x7dbd74), &mChange);
			mOldMarkDrop.SetCurve(GTS(0x7dbd54), &mChange);
			mNewMarkAlpha.SetCurve(GTS(0x7dbd34), &mChange);
			mNewMarkScale.SetCurve(GTS(0x7dbd14), &mChange);
			mOldBadgeAlpha.SetCurve(GTS(0x7dbd74), &mChange);
			mNewBadgeAlpha.SetCurve(GTS(0x7dbd34), &mChange);
			mBadgeGlow.SetCurve(GTS(0x7dbcf0), &mChange);
		}
		break;
	case 2:
		mCounter++;
		if (300 < mCounter)
			mButton->mHighlighted = 280 <= mCounter % 300;
		if (mButton != nullptr && mNewTextAlpha.HasBeenTriggered() && mPressed) {
			// Continue: it goes the way it came
			mButton->SetDisabled(true);
			gApp->PlaySample(SOUND_RANK_COMPLETION);
			mShow.mIncRate = -mShow.mIncRate;
			mState = 3;
			mAlpha.SetCurve(GTS(0x7dbdbc));
		}
		if (mRankBonus != nullptr && (mRankBonus->mClicked || mRankBonus->mState == 5))
			static_cast<ButtonListener*>(this)->ButtonDepress(0);
		break;
	case 3:
		if (!mShow.IncInVal()) {
			if (mRemoveWhenDone) {
				mParent->RemoveWidget(this);
				gApp->SafeDeleteWidget(this);
			}
			mClicked = true;
		}
		break;
	case 4:
		if (!mTrophyIn.IncInVal() && (mRankBonus == nullptr || mRankBonus->mState == 5)) {
			mTrophyIn.SetCurve(GTS(0x7dbe00));
			mTrophyScale.SetCurve(GTS(0x7dbddc), &mTrophyIn);
			mState = 5;
		}
		break;
	case 5:
		if (mRankBonus != nullptr && mRankBonus->mClicked) {
			mRankBonus->mParent->RemoveWidget(mRankBonus);
			delete mRankBonus;
			mRankBonus = nullptr;
		}
		if (!mTrophyIn.IncInVal() && mRankBonus == nullptr)
			mClicked = true;
		break;
	default:
		break;
	}
	if (gBoard != nullptr && mUpdateCnt == 300 && 2 <= (unsigned int)gApp->mProfile->mRank) {
		gBoard->UnlockMode(2);  // 0x6b40b5
	}
}

void AwardTelly::Draw(Graphics* g)
{
	TwistWidget::Draw(g);
	mFXManager->Draw(g);
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	if (mKind == 1) {
		Transform aTransform;
		float aScale = (float)mTrophyScale.GetOutVal();
		aTransform.Scale((float)mTrophyScale.GetOutVal(), aScale);
		float aY = (float)(mTrophyIn.GetOutVal() * (double)gApp->mScale);
		g->DrawImageTransform(IMAGE_RANKBAR_CHALLENGECOMPLETE, aTransform, (float)(mWidth / 2), aY);
	} else if (mKind == 3) {
		g->SetColorizeImages(true);
		double aRankUp = mRankUpAlpha.GetOutVal();
		g->SetColor(Color(255, 255, 255, Ftol(aRankUp * 255.0 * mAlpha.GetOutVal())));
		int aY = Ftol((double)gApp->mScale * 400.0);
		g->DrawImage(IMAGE_RANKBAR_RANKUP, mWidth / 2 - IMAGE_RANKBAR_RANKUP->mWidth / 2, aY);
		double anOld = mOldTextAlpha.GetOutVal();
		DrawRankText(g, mRank, (float)(mAlpha.GetOutVal() * anOld));
		double aNew = mNewTextAlpha.GetOutVal();
		DrawRankText(g, mRank + 1, (float)(mAlpha.GetOutVal() * aNew));
		DrawBadges(g);
	}
}

void AwardTelly::DrawBadges(Graphics* g)
{
	float aCenterX = (float)(mWidth / 2);
	float aCenterY = (float)Ftol((double)gApp->mScale * 700.0);
	int i = 0;
	do {
		int aMark;
		int aBadge;
		if (i == 0) {
			aMark = mOldMark;
			aBadge = mOldBadge;
		} else {
			aMark = mMark;
			aBadge = mBadge;
		}
		g->GetColorizeImages();
		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		if (mBadge != mOldBadge) {
			double aFade = (i == 0 ? mOldBadgeAlpha : mNewBadgeAlpha).GetOutVal();
			g->SetColor(Color(255, 255, 255, Ftol(aFade * 255.0 * mAlpha.GetOutVal())));
		} else {
			g->SetColor(Color(255, 255, 255, Ftol(mAlpha.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
		}
		Image* aBadgeImage = GetImageById(kRankBadges[aBadge]);
		if (i == 0 || mBadge != mOldBadge) {
			int aWidth = Ftol((double)aBadgeImage->GetWidth());
			int aHeight = Ftol((double)aBadgeImage->GetHeight());
			int aY = Ftol((double)aCenterY - (double)(aHeight / 2) - 3.0);
			int aX = Ftol((double)aCenterX - (double)(aWidth / 2));
			g->DrawImage(aBadgeImage, aX, aY, aWidth, aHeight);
			if (i == 1) {
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->SetColor(Color(255, 255, 255, Ftol(mBadgeGlow.GetOutVal() * 255.0)));  // CurvedVal::GetColor
				g->DrawImage(aBadgeImage, aX, aY, aWidth, aHeight);
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			}
		}
		if (aMark != 0 && aMark != 5) {
			float aScale = (float)(i == 0 ? mOldMarkScale : mNewMarkScale).GetOutVal();
			Image* aMarkImage = GetImageById(kRankBadgeMarks[aMark]);
			int aWidth = Ftol((double)aMarkImage->mWidth * (double)aScale);
			int aHeight = Ftol((double)aMarkImage->mHeight * (double)aScale);
			int aY;
			if (i == 0) {
				g->SetColor(Color(255, 255, 255, Ftol(mChange.GetOutVal() * 255.0 * mAlpha.GetOutVal())));
				double aDrop = mOldMarkDrop.GetOutVal();
				aY = Ftol(((double)aCenterY - (double)(aHeight / 2)) + aDrop * (double)gApp->mScale);
			} else {
				g->SetColor(Color(255, 255, 255, Ftol(mNewMarkAlpha.GetOutVal() * 255.0 * mAlpha.GetOutVal())));
				aY = Ftol((double)aCenterY - (double)(aHeight / 2));
			}
			int aX = Ftol((double)aCenterX - (double)(aWidth / 2));
			g->DrawImage(aMarkImage, aX, aY, aWidth, aHeight);
		}
		i++;
	} while (i < 2);
	g->SetColorizeImages(false);
}

void AwardTelly::DrawRankText(Graphics* g, int theRank, float theAlpha)
{
	g->SetFont(FONT_HUGE);
	int anAlpha = Ftol((double)theAlpha * 255.0);
	g->SetColor(Color(255, 255, 255, anAlpha));
	SexyString aText = StrFormat(GTC(0x7dbe40), theRank);
	WriteCenteredLine(g, Ftol((double)gApp->mScale * 880.0), aText);
	g->SetColor(Color(255, 255, 0, anAlpha));
	SexyString aName = gApp->mRanks->GetName(theRank - 1);
	WriteCenteredLine(g, Ftol((double)gApp->mScale * 960.0), aName);
}
