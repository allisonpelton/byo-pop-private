#include "Mandala.h"

#include "Board.h"
#include "Buffer.h"
#include "EffectsManager.h"
#include "Font.h"
#include "GameText.h"
#include "GoodTube.h"
#include "Graphics.h"
#include "Hyperspace.h"
#include "Image.h"
#include "Menu.h"
#include "ModeButton.h"
#include "PlayButton.h"
#include "Profile.h"
#include "Res.h"
#include "TwistButton.h"
#include "TwistDialog.h"
#include "SexyMatrix.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

static_assert(IMAGE_PLAYMENU_CLASSICBUTTON_ID == 0x122 && IMAGE_PLAYMENU_ZENBUB_ID == 0x130,
	"the mode table's resource ids are the exe's");

namespace {

// 0x85d8d8: the four buttons, made once (the texts through the string table)
struct ModeEntry {
	SexyString mDescription;
	int mMode;
	int mButtonId;
	int mPictureId;
	int mTagId;
};
ModeEntry gModeEntries[4];
bool gModeEntriesMade = false;  // 0x85d928

// 0x84f604: the voice each button says
const int kModeVoices[4] = {SOUND_VOICE_CLASSIC_ID, SOUND_VOICE_CHALLENGE_ID, SOUND_VOICE_BLITZ_ID, SOUND_VOICE_ZEN_ID};

// 0xe0b69c: the modes' names, which Init (0x7652cc) reads from the string table once
// port: read when they are needed
SexyString GetModeName(int theMode)
{
	switch (theMode) {
	case 0: return gApp->GetString(GTS(0x7f4104), GT(0x7f40f4));
	case 1: return gApp->GetString(GTS(0x7f40ec), GT(0x7f40dc));
	case 2: return gApp->GetString(GTS(0x7f40d0), GT(0x7f40bc));
	case 3: return gApp->GetString(GTS(0x7f40b4), GT(0x7f40a8));
	default: return gApp->GetString(GTS(0x7f40a4), GT(0x7f409c));
	}
}

// 0x6a1984: CurvedVal::GetColor, white at the curve's opacity
Color CurveColor(CurvedVal& theCurve)
{
	return Color(255, 255, 255, Ftol(theCurve.GetOutVal() * 255.0));
}

// 0x6d8d19: the share of the challenges done, in percent
int GetChallengePercent()
{
	return gApp->mChallengeInfo->GetPercentDone(gApp->mProfile);
}

}  // namespace

Mandala::Mandala(int theMode)
{
	if (!gModeEntriesMade) {
		gModeEntriesMade = true;
		gModeEntries[0] = {gApp->GetString(GTS(0x7e7bdc), GT(0x7e7b90)), 1, IMAGE_PLAYMENU_CLASSICBUTTON_ID,
			IMAGE_PLAYMENU_CLASSICBUB_ID, IMAGE_PLAYMENU_LVLTAGCLASSIC_ID};
		gModeEntries[1] = {gApp->GetString(GTS(0x7e7b4c), GT(0x7e7ad0)), 2, IMAGE_PLAYMENU_CHALLENGEBUTTON_ID,
			IMAGE_PLAYMENU_CHALLENGEBUB_ID, -1};
		gModeEntries[2] = {gApp->GetString(GTS(0x7e7a98), GT(0x7e7a30)), 3, IMAGE_PLAYMENU_BLITZBUTTON_ID,
			IMAGE_PLAYMENU_BLITZBUB_ID, IMAGE_PLAYMENU_LVLTAGZEN_ID};
		gModeEntries[3] = {gApp->GetString(GTS(0x7e79e8), GT(0x7e7960)), 4, IMAGE_PLAYMENU_ZENBUTTON_ID,
			IMAGE_PLAYMENU_ZENBUB_ID, IMAGE_PLAYMENU_LVLTAGZEN_ID};
	}
	if (gApp->mRegistered && !gApp->mUnknown759)
		gApp->mProfile->UnlockMode(3);
	mOver = -1;
	mTextBright = 0;
	mIdle = 0;
	mMode = theMode;
	mClip = false;
	mState = 0;

	for (int i = 0; i < 4; i++) {
		const ModeEntry& anEntry = gModeEntries[i];
		ModeButton* aButton = new ModeButton(i, GetImageById(anEntry.mButtonId), this);
		aButton->mButtonId = anEntry.mButtonId;
		aButton->mHidden = true;
		aButton->mPictureId = anEntry.mPictureId;
		aButton->mMode = anEntry.mMode;
		aButton->mTagId = anEntry.mTagId;
		aButton->mDescription = anEntry.mDescription;
		aButton->mMouseVisible = false;
		mButtons.push_back(aButton);

		int aMode = aButton->mMode;
		if ((unsigned)(aMode - 1) < 5 && mMode == 0) {
			if (!gApp->mProfile->IsModeUnlocked(aMode)) {
				aButton->mDoFinger = false;
			} else if (aMode == 2) {
				int aPercent = GetChallengePercent();
				if (aPercent == 100) {
					aButton->mPictureId = IMAGE_PLAYMENU_CHALLENGEBUB100_ID;
					aButton->mTagText = GT(0x80e8a4);
				} else {
					aButton->mTagText = StrFormat(GTC(0x7e7950), aPercent);
				}
			} else if (aMode == 3) {
				aButton->mTagText = GT(0x80d9a4);
			} else if (gApp->SaveExists(aMode, 0)) {
				// the saved game's level under the picture; a save that does not read is deleted
				Buffer aBuffer;
				std::string aName = gApp->GetSaveName(aMode, 0);
				gApp->ReadBufferFromFile(aName, &aBuffer, true);
				int aVersion;
				int aSaveMode;
				int aFlags;
				int anUnknown154;
				int aLevel;
				if (!Board::ReadSaveHeader(aBuffer, &aVersion, &aSaveMode, &aFlags, &anUnknown154, &aLevel))
					gApp->EraseFile(aName);
				else
					aButton->mTagText = StrFormat(GTC(0x80da2c), aLevel);
			}
		}
	}

	mZoom.SetConstant(1.0);
	mSaveMode = -1;
	mDelay = 0;
	mOffsetY = 0;
	mUnknown1A8 = 0;
	mPlayVoice = true;
	mEgg = false;
	mFXManager = new EffectsManager();
	mPortal = -1;
	mFade = 1.0f;
	mGlow = 0;
	mDelay = 0;
	mShowPortal = true;
	for (size_t i = 0; i < mButtons.size(); i++) {
		ModeButton* aButton = mButtons[i];
		AddWidget(aButton);
		int aMode = aButton->mMode;
		if ((aMode == 3 && !gApp->mRegistered) || !gApp->mProfile->IsModeUnlocked(aMode))
			aButton->SetDoorOpen(false, true);
		else if ((gApp->mProfile->mUnknown218 & (1u << (aMode & 31))) != 0)
			aButton->SetDoorOpen(true, true);  // opened before: open at once
	}

	mMainMenuButton = new TwistButton(-1, IMAGE_SIZEABLEBUTTON, this);
	mMainMenuButton->SetFont(FONT_DIALOG_BUTTONS);
	mMainMenuButton->mLabel = gApp->GetString(GTS(0x7e0cb0), GT(0x7e0c9c));
	mMainMenuButton->mLabelOffsetY = Ftol((double)gApp->mScale * 0.0);
	mMainMenuButton->mIcon = IMAGE_PLAYMENU_MMGRAPHIC;
	mMainMenuButton->mIconOffsetX = Ftol(0.0 * (double)gApp->mScale);
	mMainMenuButton->mIconOffsetY = Ftol((double)gApp->mScale * 6.0);
	if (mPlayVoice) {
		gApp->PlaySample(SOUND_VOICE_CHOOSEYOURGAME);
		mPlayVoice = false;
	}

	PlayButton* aPlayButton = gApp->mMainMenu->mPlayButton;
	if (mMode == 0) {
		mOpen.SetCurve(GTS(0x7e7680));
		mConnect.SetCurve(GTS(0x7e7658), &mOpen);
		mSeamGlow.SetCurve(GTS(0x7e7620));
	} else {
		// the zen jump: the other mode's button says what it does, the challenge and blitz buttons and the main menu
		// button are hidden, and the menu shows behind
		if (mMode == 1) {
			mButtons[0]->mDescription = gApp->GetString(GTS(0x7e7920), GT(0x7e78c0));
			mButtons[3]->mDescription = gApp->GetString(GTS(0x7e7884), GT(0x7e7810));
		} else {
			mButtons[0]->mDescription = gApp->GetString(GTS(0x7e77c8), GT(0x7e7740));
			mButtons[3]->mDescription = gApp->GetString(GTS(0x7e7710), GT(0x7e76b8));
		}
		mButtons[1]->SetVisible(false);
		mButtons[2]->SetVisible(false);
		mMainMenuButton->SetVisible(false);
		gApp->mMainMenu->SetVisible(true);
		if (gApp->Is3DAccelerated()) {
			aPlayButton->mAppear.SetCurve(GTS(0x7dbc28));
			aPlayButton->mAlpha.SetCurve(GTS(0x7dbc28));
		} else {
			aPlayButton->mAppear.SetConstant(1.0);
			aPlayButton->mAlpha.SetConstant(1.0);
		}
		aPlayButton->mImageAppear.SetConstant(0.0);
		mZoom.SetCurve(GTS(0x7dbc28));
	}
	AddWidget(mFXManager);
	Layout();
	mEggCount = 0;
	mEggNext = 0;
	mRegistered = gApp->mRegistered;
}

Mandala::~Mandala()
{
	for (size_t i = 0; i < mButtons.size(); i++) {
		RemoveWidget(mButtons[i]);
		delete mButtons[i];
	}
	mButtons.clear();
	RemoveWidget(mMainMenuButton);
	delete mMainMenuButton;
	// port: the original deletes the effects manager while it is still a child; the port takes it out first
	RemoveWidget(mFXManager);
	delete mFXManager;
}

void Mandala::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	Layout();
	AddWidget(mMainMenuButton);
	gApp->PlaySample(SOUND_INNERLOCK1);
}

void Mandala::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	RemoveAllWidgets(false, true);
}

void Mandala::MouseMove(int x, int y)
{
	mMouseX = x;
	mMouseY = y;
}

void Mandala::Layout()
{
	float aScale = gApp->mScale;
	if (mMode == 0) {
		// the panels slide apart while mOpen falls: the left two buttons to the left, the right two to the right
		for (int i = 0; i < (int)mButtons.size(); i++) {
			ModeButton* aButton = mButtons[i];
			int anId = aButton->mButtonId;
			int aSlide = Ftol(mOpen.GetOutVal() * (double)gApp->mScale);
			if (i < 2)
				aSlide = -aSlide;
			int aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(anId));
			int anX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(anId));
			aButton->Resize(anX + aSlide, aY, aButton->mButtonImage->GetCelWidth(),
				aButton->mButtonImage->GetCelHeight());
		}
		int anOffset = Ftol((double)gApp->mScale * -0.0);
		mMainMenuButton->mX = (-anOffset - mMainMenuButton->mWidth / 2) + mWidth / 2;
		mMainMenuButton->mY = Ftol(mOpen.GetOutVal() * (double)gApp->mScale * 0.25 + (double)gApp->mHeight *
			0.8999999761581421);
	} else if (mMode == 1) {
		// over a classic game: classic on the right, zen on the left
		mButtons[0]->Move(Ftol((double)aScale * 1319.0), Ftol((double)aScale * 514.0));
		aScale = gApp->mScale;
		mButtons[3]->Move(Ftol((double)aScale * 292.0), Ftol((double)aScale * 514.0));
	} else {
		mButtons[0]->Move(Ftol((double)aScale * 292.0), Ftol((double)aScale * 514.0));
		aScale = gApp->mScale;
		mButtons[3]->Move(Ftol((double)aScale * 1319.0), Ftol((double)aScale * 514.0));
	}
}

void Mandala::Update()
{
	if (mEgg) {
		mEgg = false;
		// port: 0x75fd40 puts up a 0x288-byte widget of its own (app +0xf94); it is not ported yet
		NotPorted("Mandala: the app's 0x75fd40");
		return;
	}
	TwistWidget::Update();
	if (mRegistered != gApp->mRegistered && !gApp->mUnknown759 && mMode == 0) {
		// the game was registered while the mode select was up: blitz opens
		gApp->mProfile->UnlockMode(3);
		mButtons[2]->SetDoorOpen(true, false);
		gApp->mProfile->mUnknown218 |= 8;
		mButtons[2]->mDoFinger = true;
	}

	if (0.0f < mGlow && mPortal == 0) {
		// sparks round the classic portal
		int i = 0;
		while (i < Ftol((double)RandFloat() * 10.0 * (double)mGlow)) {
			Effect* anEffect = mFXManager->AllocEffect(6);
			float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
			float aCos = cosf(anAngle);
			float aRadius = (float)((double)gApp->mScale * 220.0);
			anEffect->mX = aRadius * aCos + (float)(mWidth / 2);
			float aSin = sinf(anAngle);
			aRadius = (float)((double)gApp->mScale * 220.0);
			anEffect->mY = aRadius * aSin + (float)(mHeight / 2);
			anAngle = (float)((double)anAngle - 0.699999988079071);
			aCos = cosf(anAngle);
			float aSpeed = (float)((double)gApp->mScale * -1.7999999523162842);
			anEffect->mDX = aSpeed * aCos;
			aSin = sinf(anAngle);
			aSpeed = (float)((double)gApp->mScale * -1.7999999523162842);
			anEffect->mDY = aSpeed * aSin;
			anEffect->mAlpha = 0;
			anEffect->mDAlpha = (float)((double)RandSigned() * 0.001500000013038516 + 0.007499999832361937);
			if (gApp->Is3DAccelerated()) {
				anEffect->mScale = (float)((double)RandSigned() * 0.03500000014901161 + 0.5);
				anEffect->mDScale = (float)((double)RandSigned() * 0.004999999888241291 + 0.013000000268220901);
			}
			mFXManager->AddEffect(anEffect);
			i++;
		}
	}
	if (mOver != 0) {
		// away from the classic button its sparks fade
		for (auto& aPair : mFXManager->mEffects) {
			Effect* anEffect = aPair.second;
			if (anEffect->mType == 6) {
				int anAlpha = anEffect->mColor.mAlpha - 16;
				if (anAlpha < 0)
					anAlpha = 0;
				anEffect->mColor.mAlpha = anAlpha;
			}
		}
	}

	mVisible = 0 < mUpdateCnt;
	if (mUpdateCnt < 1)
		return;

	float aGlow;
	if (mOver == -1) {
		mIdle = (float)((double)mIdle + 0.009999999776482582);
		aGlow = (float)((double)mGlow - 0.02500000037252903);
		if (aGlow < 0.0f)
			aGlow = 0.0f;
	} else {
		mIdle = 0;
		aGlow = (float)((double)mGlow + 0.019999999552965164);
		if (1.0f < aGlow)
			aGlow = 1.0f;
	}
	mGlow = aGlow;
	if (mOpen.GetOutVal() != 0.0) {
		double aLess = (double)mGlow - 0.10000000149011612;
		if (aLess < 0.0)
			aLess = 0.0;
		mGlow = (float)aLess;
	}
	if (0.0f < mDelay) {
		double aLess = (double)mDelay - 0.009999999776482582;
		if (aLess < 0.0)
			aLess = 0.0;
		mDelay = (float)aLess;
	}

	if (mState == 0 || mState == 2) {
		if (mDelay <= 0.0f) {
			mSeamGlow.IncInVal();
			if (!mOpen.IncInVal()) {
				if (mState == 0) {
					// the panels are open: the unlocked doors open
					bool anyOpened = false;
					for (size_t i = 0; i < mButtons.size(); i++) {
						ModeButton* aButton = mButtons[i];
						int aMode = aButton->mMode;
						aButton->mHidden = false;
						aButton->mMouseVisible = true;
						bool isOpen = !((aMode == 3 && !gApp->mRegistered) || !gApp->mProfile->IsModeUnlocked(aMode));
						if (-1 < aMode && aButton->mDoorState == 0 && isOpen) {
							aButton->SetDoorOpen(true, false);
							anyOpened = true;
							gApp->mProfile->mUnknown218 |= 1u << (aMode & 31);
						}
					}
					if (anyOpened)
						gApp->PlaySample(SOUND_DOORLOCK2);
					mState = 1;
					if (-1 < gApp->mAutoPlayMode) {
						// -autoplay picks its mode's button
						for (size_t i = 0; i < mButtons.size(); i++) {
							if (mButtons[i]->mMode == gApp->mAutoPlayMode) {
								ButtonDepress(mButtons[i]->mId);
								break;
							}
						}
					}
				} else {
					// closed: it goes
					mClicked = true;
					mParent->RemoveWidget(this);
					gApp->SafeDeleteWidget(this);
					gApp->mUnknownF24 = nullptr;
				}
			}
		}
	} else if (mState == 3) {
		if (mZoom.GetOutVal() == 0.0) {
			mClicked = true;
			mParent->RemoveWidget(this);
			gApp->SafeDeleteWidget(this);
			gApp->mUnknownF24 = nullptr;
			if (gBoard->mUnknown2208 == this)
				gBoard->mUnknown2208 = nullptr;
			gApp->mMainMenu->mPlayButton->mAppear.SetConstant(0.0);
			gApp->mMainMenu->mPlayButton->mAlpha.SetConstant(0.0);
		}
	} else if (mState == 4 && !mDarken.IncInVal()) {
		// the zen jump's dark screen is complete: the other mode's game starts
		if (gBoard->mUnknown2208 == this)
			gBoard->mUnknown2208 = nullptr;
		StartMode(mSelected, false);
		mState = 3;
		mZoom.SetConstant(0.0);
		gApp->mMainMenu->mPlayButton->mAlpha.SetConstant(0.0);
		gBoard->mHyperspace->mFlash.SetCurve(GTS(0x7e6934));
		gBoard->mHyperspace->mUnknown12B0.SetCurve(GTS(0x7e7520));
		gBoard->mUnknown2578 = 400;
	}

	if (0.0f < mTextBright) {
		float aLess = (float)((double)mTextBright - 0.029999999329447746);
		if (aLess < 0.0f)
			aLess = 0.0f;
		mTextBright = aLess;
	}
	Layout();
}

void Mandala::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	if (mMode != 0) {
		mWidgetManager->FlushDeferredOverlayWidgets(0x7fffffff);
		g->SetColor(Color(0, 0, 0, Ftol(mZoom.GetOutVal() * 200.0)));
		g->FillRect(0, 0, mWidth, mHeight);
	}
	if (mZoom.GetOutVal() != 1.0) {
		// the original builds a zoom by mZoom about the screen's middle (mOffsetY lower), which only Direct3D applies
		mZoom.GetOutVal();
		mZoom.GetOutVal();
	}
	Widget::DrawAll(theFlags, g);
	mZoom.GetOutVal();
}

void Mandala::Draw(Graphics* g)
{
	Widget::Draw(g);
	DeferOverlay(0);
	float aScale = gApp->mScale;
	if (mMode == 0) {
		// the panels and their connectors, apart by mOpen
		int aY = Ftol((double)aScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_CONNECTORLEFT_ID));
		int anX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_PLAYMENU_CONNECTORLEFT_ID));
		double anOpen = mOpen.GetOutVal() * (double)gApp->mScale;
		int aConnect = Ftol(mConnect.GetOutVal() * (double)gApp->mScale);
		g->DrawImage(IMAGE_PLAYMENU_CONNECTORLEFT, (Ftol(-anOpen) - aConnect) + anX, aY);
		aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_CONNECTORRIGHT_ID));
		anX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_PLAYMENU_CONNECTORRIGHT_ID));
		anOpen = mOpen.GetOutVal() * (double)gApp->mScale;
		aConnect = Ftol(mConnect.GetOutVal() * (double)gApp->mScale);
		g->DrawImage(IMAGE_PLAYMENU_CONNECTORRIGHT, Ftol(anOpen) + aConnect + anX, aY);
		aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_DIALOGLEFT_ID));
		anX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_PLAYMENU_DIALOGLEFT_ID));
		int aSlide = Ftol(mOpen.GetOutVal() * (double)gApp->mScale);
		g->DrawImage(IMAGE_PLAYMENU_DIALOGLEFT, anX - aSlide, aY);
		aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_DIALOGRIGHT_ID));
		anX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_PLAYMENU_DIALOGRIGHT_ID));
		aSlide = Ftol(mOpen.GetOutVal() * (double)gApp->mScale);
		g->DrawImage(IMAGE_PLAYMENU_DIALOGRIGHT, aSlide + anX, aY);
		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(CurveColor(mSeamGlow));
		aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_SEAMGLOW_ID));
		anX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_PLAYMENU_SEAMGLOW_ID));
		g->DrawImage(IMAGE_PLAYMENU_SEAMGLOW, anX, aY);
		aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_SEAMGLOW2_ID));
		anX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_PLAYMENU_SEAMGLOW2_ID));
		g->DrawImage(IMAGE_PLAYMENU_SEAMGLOW2, anX, aY);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	} else {
		int aY = Ftol((double)aScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_ZENJUMPDIALOG_ID));
		int anX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_PLAYMENU_ZENJUMPDIALOG_ID));
		g->DrawImage(IMAGE_PLAYMENU_ZENJUMPDIALOG, anX, aY);
	}

	if (mShowPortal) {
		g->SetColor(Color(255, 255, 255, Ftol((double)mFade * 255.0 * (double)mGlow)));
		if (mPortal == 1) {
			// the challenge portal's gears turn
			const struct { Image* mImage; int mId; double mTurn; } kGears[3] = {
				{IMAGE_PLAYMENU_CHALLENGEGEAR, IMAGE_PLAYMENU_CHALLENGEGEAR_ID, -0.0020000000949949026},
				{IMAGE_PLAYMENU_CHALLENGEGEAR2, IMAGE_PLAYMENU_CHALLENGEGEAR2_ID, 0.0020000000949949026},
				{IMAGE_PLAYMENU_CHALLENGEGEAR3, IMAGE_PLAYMENU_CHALLENGEGEAR3_ID, -0.0020000000949949026},
			};
			for (const auto& aGear : kGears) {
				int aY = Ftol((double)GetImageOffsetY(aGear.mId) * (double)gApp->mScale);
				int aDown = Ftol((double)gApp->mScale * 8.0);
				int anOffsetX = GetImageOffsetX(aGear.mId);
				double aTurn = (double)gApp->mUpdateCount * aGear.mTurn;
				aY = aDown + aY;
				int anX = Ftol((double)gApp->mScale * (double)anOffsetX);
				g->DrawImageRotated(aGear.mImage, anX, aY, aTurn, nullptr);
			}
		}
	}
}

void Mandala::DrawOverlay(Graphics* g)
{
	g->SetFont(FONT_HUGE_OUTLINE);
	SetFontLayerColor(g->GetFont(), 0, Color::White);
	SetFontLayerColor(g->GetFont(), 1, Color::Black);
	g->SetColor(Color(255, 255, 255, Ftol(mZoom.GetOutVal() * 255.0)));
	if (mMode == 1 || mMode == 2) {
		SexyString aTitle = mMode == 1 ? gApp->GetString(GTS(0x7e70d8), GT(0x7e7098)) :
			gApp->GetString(GTS(0x7e706c), GT(0x7e7018));
		WriteCenteredLine(g, Ftol((double)gApp->mScale * 200.0), aTitle);
	}
	if (mZoom.GetOutVal() != 1.0) {
		// (Direct3D's zoom, as in DrawAll)
		mZoom.GetOutVal();
		mZoom.GetOutVal();
	}
	float anArt = (float)gApp->mArtRes / 1200.0f;
	g->SetColorizeImages(true);

	if (mShowPortal) {
		g->SetColor(Color(255, 255, 255, Ftol((double)mFade * 255.0 * (double)mGlow)));
		switch (mPortal) {
		case 0: {
			// the classic portal turns about the middle
			Transform aTransform;
			aTransform.RotateRad((float)((double)gApp->mUpdateCount * -0.009999999776482582));
			g->DrawImageTransform(IMAGE_PLAYMENU_CLASSICPORTAL, aTransform, (float)(mWidth / 2), (float)(mHeight / 2));
			break;
		}
		case 1: {
			g->SetColor(Color(255, 255, 255, Ftol((double)mFade * 180.0 * (double)mGlow)));
			int aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_CHALLENGEPORTAL_ID));
			int anOffsetX = GetImageOffsetX(IMAGE_PLAYMENU_CHALLENGEPORTAL_ID);
			double aTurn = (double)gApp->mUpdateCount * -0.007499999832361937;
			int anX = Ftol((double)gApp->mScale * (double)anOffsetX);
			g->DrawImageRotated(IMAGE_PLAYMENU_CHALLENGEPORTAL, anX, aY, aTurn, nullptr);
			break;
		}
		case 2:
			if (!gApp->mRegistered) {
				g->SetColor(Color(255, 255, 255, Ftol((double)mFade * 255.0 * (double)mGlow)));
				DrawImageScaledCentered(g, IMAGE_PLAYMENU_LOCKED, (float)(mWidth / 2), (float)(mHeight / 2),
					ALIGN_CENTER, 1.0f, 1.0f);
			} else {
				// the blitz portal turns and the clock's hands go round
				g->SetColor(Color(255, 255, 255, Ftol((double)mFade * 255.0 * (double)mGlow)));
				int aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_BLITZPORTAL_ID));
				int anOffsetX = GetImageOffsetX(IMAGE_PLAYMENU_BLITZPORTAL_ID);
				double aTurn = (double)gApp->mUpdateCount * -0.004999999888241291;
				int anX = Ftol((double)gApp->mScale * (double)anOffsetX);
				g->DrawImageRotated(IMAGE_PLAYMENU_BLITZPORTAL, anX, aY, aTurn, nullptr);
				aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_BLITZCLOCKTICKS_ID));
				anX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_PLAYMENU_BLITZCLOCKTICKS_ID));
				g->DrawImage(IMAGE_PLAYMENU_BLITZCLOCKTICKS, anX, aY);
				g->SetColor(Color(255, 255, 255, Ftol((double)mFade * 64.0 * (double)mGlow)));
				float aCentre = (float)((double)gApp->mScale * 37.5);
				aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_BLITZLONGARM_ID));
				anOffsetX = GetImageOffsetX(IMAGE_PLAYMENU_BLITZLONGARM_ID);
				int aCentreY = Ftol((double)aCentre);
				int aCentreX = Ftol((double)aCentre);
				aTurn = (double)gApp->mUpdateCount * -0.05000000074505806;
				anX = Ftol((double)gApp->mScale * (double)anOffsetX);
				g->DrawImageRotated(IMAGE_PLAYMENU_BLITZLONGARM, anX, aY, aTurn, aCentreX, aCentreY, nullptr);
				aCentre = (float)((double)gApp->mScale * 106.0);
				float aStart = (float)((double)gApp->mScale * 0.1379999965429306);
				aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_BLITZSHORTARM_ID));
				anOffsetX = GetImageOffsetX(IMAGE_PLAYMENU_BLITZSHORTARM_ID);
				aCentreY = Ftol((double)aCentre);
				aCentreX = Ftol((double)aCentre);
				aTurn = (double)aStart - (double)gApp->mUpdateCount * 0.05000000074505806 / 12.0;
				anX = Ftol((double)gApp->mScale * (double)anOffsetX);
				g->DrawImageRotated(IMAGE_PLAYMENU_BLITZSHORTARM, anX, aY, aTurn, aCentreX, aCentreY, nullptr);
			}
			break;
		case 3: {
			// the zen portal, its yin turning in and out
			int aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_ZENPORTAL_ID));
			int anX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_PLAYMENU_ZENPORTAL_ID));
			g->DrawImage(IMAGE_PLAYMENU_ZENPORTAL, anX, aY);
			float aPulse = cosf((float)((double)gApp->mUpdateCount * 0.02500000037252903));
			aPulse = (float)(((double)aPulse * 0.5 + 0.5) * 0.5 + 0.5);
			g->SetColor(Color(255, 255, 255, Ftol((double)aPulse * 255.0 * (double)mFade * (double)mGlow)));
			aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_ZENMOON_ID));
			anX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_PLAYMENU_ZENMOON_ID));
			g->DrawImage(IMAGE_PLAYMENU_ZENMOON, anX, aY);
			aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_PLAYMENU_ZENYIN_ID));
			anX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_PLAYMENU_ZENYIN_ID));
			g->DrawImage(IMAGE_PLAYMENU_ZENYIN, anX, aY);
			break;
		}
		default:
			break;
		}
	}

	g->SetColor(Color(255, 255, 255, Ftol((double)mFade * 255.0)));
	SexyString aHint;
	Font* aFont = FONT_DIALOG_TEXT_GLOW;
	g->SetFont(aFont);
	if (-1 < mOver) {
		// the description of the button under the mouse, in the middle
		int aWidth = Ftol((double)gApp->mScale * 410.0);
		int aHeight = g->GetWordWrappedHeight(aWidth, mText, -1);
		int aRight = Ftol((double)gApp->mScale * -10.0);
		Rect aRect;
		aRect.mX = (mWidth / 2 - aRight) - aWidth / 2;
		aRect.mY = Ftol((double)(mHeight / 2 - aHeight / 2) - (double)anArt * 35.0);
		aRect.mWidth = aWidth;
		aRect.mHeight = aHeight;
		int aGrey = Ftol((double)mTextBright * 255.0);
		SetFontLayerColor(aFont, GTS(0x7dd6b0), Color(aGrey, aGrey, aGrey, 255));
		g->WriteWordWrapped(aRect, mText, -1, 0);
		aRect.mX += Ftol((double)gApp->mScale * -2.0);
		SetFontLayerColor(aFont, GTS(0x7dd6b0), Color(0, 0, 0, 0));
		SetFontLayerColor(aFont, GTS(0x80dfb4), Color(255, 255, 255, 255));
		g->SetColor(Color(255, 255, 255, Ftol((double)mFade * 255.0)));
		g->WriteWordWrapped(aRect, mText, -1, 0);
	} else if (mOpen.GetOutVal() == 0.0 && 0.25f < mIdle) {
		// after a while with no button under the mouse, a hint
		double anAlpha = ((double)mIdle - 0.25) * 255.0;
		if (200.0 < anAlpha)
			anAlpha = 200.0;
		g->SetColor(Color(255, 255, 255, Ftol(mZoom.GetOutVal() * ((double)mFade * anAlpha))));
		int aY = mHeight / 2 - Ftol((double)gApp->mScale * 36.0);
		if (mMode == 0) {
			aHint = gApp->GetString(GTS(0x7e6fe8), GT(0x7e6fd0));
		} else {
			if (mMode == 1)
				aHint = gApp->GetString(GTS(0x7e6fa0), GT(0x7e6f40));
			else
				aHint = gApp->GetString(GTS(0x7e6f0c), GT(0x7e6eb0));
			aY += Ftol((double)gApp->mScale * -64.0);
		}
		int aWidth = Ftol((double)gApp->mScale * 800.0);
		Rect aRect;
		aRect.mX = mWidth / 2 - Ftol((double)gApp->mScale * 400.0);
		aRect.mY = aY;
		aRect.mWidth = aWidth;
		aRect.mHeight = 0;
		SetFontLayerColor(aFont, GTS(0x7e029c), Color(0, 0, 0, 0));
		SetFontLayerColor(aFont, GTS(0x80dfb4), Color(255, 255, 255, 255));
		SetFontLayerColor(aFont, GTS(0x7dd6b0), Color(0, 0, 0, 180));
		g->WriteWordWrapped(aRect, aHint, Ftol((double)gApp->mScale * 68.0), 0);
	}

	mZoom.GetOutVal();
	if (mDarken.GetOutVal() != 0.0) {
		g->SetColor(CurveColor(mDarken));
		g->FillRect(0, 0, mWidth, mHeight);
	}
}

void Mandala::ButtonPress(int theId)
{
	gApp->PlaySample(SOUND_CLICK);
}

void Mandala::DialogButtonPress(int theDialogId, int theButtonId)
{
	gApp->PlaySample(SOUND_CLICK);
}

void Mandala::DialogThirdButtonDepress(int theDialogId)
{
	gApp->KillDialog(theDialogId);
}

void Mandala::ButtonMouseLeave(int theId)
{
	if (mOver == theId)
		mOver = -1;
}

void Mandala::ButtonMouseEnter(int theId)
{
	mOver = theId;
	mPortal = theId;
	mShowPortal = true;
	// The buttons gone over from the last to the first, twelve times, start the app's 0x75fd40
	if (theId == mEggNext) {
		mEggNext--;
		if (mEggNext < 0)
			mEggNext = 3;
		mEggCount++;
		if (mEggCount == 12) {
			mEgg = true;
			mEggCount = 0;
			mEggNext = 0;
		}
	} else {
		mEggCount = 0;
		mEggNext = 0;
	}
	if (5 < theId)
		return;

	SexyString aText;
	if (theId == -1) {
		aText = gApp->GetString(GTS(0x7e73b0), GT(0x7e7320));
	} else {
		ModeButton* aButton = mButtons[theId];
		if (aButton->mMode == 3 && !gApp->mRegistered) {
			aText = StrFormat(gApp->GetString(GTS(0x7e72c0), GT(0x7e7200)).c_str(), 10);
		} else if (gApp->mProfile->IsModeUnlocked(aButton->mMode)) {
			aText = aButton->mDescription;
		} else {
			if (aButton->mMode == 2)
				aText = StrFormat(GTC(0x7e7120), 3);
			else if (aButton->mMode == 3)
				aText = StrFormat(GTC(0x7e7180), 10);
			mShowPortal = false;
		}
	}
	int aWidth = Ftol((double)gApp->mScale * 410.0);
	Graphics aG;
	aG.SetFont(FONT_DIALOG_TEXT_GLOW);
	aG.GetWordWrappedHeight(aWidth, aText, -1);
	mText = aText;
	mTextBright = 1.0f;
}

void Mandala::ButtonDepress(int theId)
{
	if (mState != 1)
		return;
	if (theId == -1) {
		// MAIN MENU: the panels close
		gApp->ShowMainMenu(false, false, 1.0f);
		mState = 2;
		mOpen.SetCurve(GTS(0x7e75f4));
		mSeamGlow.SetCurve(GTS(0x7e75c8));
		gApp->PlaySample(SOUND_INNERLOCK2);
		mUnknown1A8 = 0;
	} else {
		int aMode = mButtons[theId]->mMode;
		if (!gApp->mProfile->IsModeUnlocked(aMode))
			return;
		PlayButton* aPlayButton = gApp->mMainMenu->mPlayButton;
		if (mMode == 0) {
			if (0 <= theId)
				gApp->PlaySample(GetSoundById(kModeVoices[theId]));
			if (aMode == 1 || aMode == 3) {
				gApp->mMainMenu->HideChallenges();
				mSaveMode = aMode;
				if (gApp->SaveExists(aMode, 0) && gApp->mAutoPlayMode == -1)
					ShowContinueDialog(aMode);
				else
					StartMode(aMode, true);
			} else if (aMode == 2) {
				gApp->mMainMenu->ShowChallenges(false);
				mUnknown1A8 = 0.1f;
				Close();
				aPlayButton->mAppear.SetCurve(GTS(0x7e75a4));
				aPlayButton->mAlpha.SetCurve(GTS(0x7dbdbc));
			} else if (3 < aMode && aMode < 6) {
				gApp->mMainMenu->HideChallenges();
				StartMode(aMode, false);
			}
		} else {
			// the zen jump: the board's own mode goes on, the other starts after the screen goes dark
			bool isSameMode = false;
			if (mMode == 1)
				isSameMode = aMode == 1;
			if (mMode == 2)
				isSameMode = aMode == 4;
			// port: the original also writes "CLASSIC/ZEN PORTAL: ... SELECTED, LEVEL ..., RANK ..., TOTAL PLAYTIME"
			// to its debug log (app +0x1024)
			if (isSameMode) {
				mState = 3;
				mZoom.SetCurve(GTS(0x7dbdbc));
				if (gApp->Is3DAccelerated()) {
					aPlayButton->mAppear.SetCurve(GTS(0x7dbdbc));
					aPlayButton->mAlpha.SetCurve(GTS(0x7e6934));
				}
			} else {
				gApp->PlaySample(GetSoundById(kModeVoices[theId]));
				if (gApp->Is3DAccelerated()) {
					aPlayButton->mAppear.SetCurve(GTS(0x7e7500));
					aPlayButton->mAlpha.SetCurve(GTS(0x7e74d8));
				}
				mDarken.SetCurve(GTS(0x7d8630));
				mState = 4;
				mSelected = aMode;
			}
		}
	}
	if (mState == 2) {
		for (size_t i = 0; i < mButtons.size(); i++)
			mButtons[i]->mMouseVisible = false;
	}
	gApp->PlaySample(SOUND_BUTTONPRESS2);
}

void Mandala::DialogButtonDepress(int theDialogId, int theButtonId)
{
	bool isNewGame = false;
	if (theButtonId == 1000)
		isNewGame = false;
	else if (theButtonId == 1001)
		isNewGame = true;
	StartMode(mSaveMode, isNewGame);
}

void Mandala::Close()
{
	mOpen.SetCurve(GTS(0x7e70f8));
	mState = 2;
}

void Mandala::ShowContinueDialog(int theMode)
{
	TwistDialog* aDialog = new TwistDialog(IMAGE_SIZEABLEDIALOG, IMAGE_SIZEABLEBUTTON, 10000, true, GetModeName(theMode),
		gApp->GetString(GTS(0x7e0c44), GT(0x7e0bf0)), GT(0x80d9a4), 7);
	aDialog->mYesLabel = gApp->GetString(GTS(0x80f054), GT(0x7e0bdc));
	aDialog->mNoLabel = gApp->GetString(GTS(0x7e0bd8), GT(0x7e0bd0));
	aDialog->PopUp();
	aDialog->mTwistListener = this;
	double aMinWidth = (double)gGemWidth * 6.75;
	double aWidth = (double)aDialog->GetPreferredWidth() <= aMinWidth ? aMinWidth : (double)aDialog->GetPreferredWidth();
	aDialog->mWidth = Ftol(aWidth);
	aDialog->mHeight = aDialog->GetPreferredHeight(aDialog->mWidth);
	gApp->AddDialog(aDialog);
	aDialog->mX = (mWidth / 2 + mX) - aDialog->mWidth / 2;
	aDialog->mY = (mHeight / 2 - aDialog->mHeight / 2) + mY;
}

void Mandala::StartMode(int theMode, bool theNewGame)
{
	Board* anOldBoard = gBoard;
	if (theNewGame && gApp->SaveExists(theMode, 0)) {
		// the saved game is given up: it counts as played
		gApp->mProfile->mGamesPlayed++;
		gApp->mProfile->mUnknown140[theMode]++;
		gApp->DeleteSave(theMode, 0);
	}
	if (gApp->mHyperspace == nullptr)
		gApp->ShowHyperspace();
	gApp->KillDialog(10000);
	gApp->NewGame(theMode, theNewGame, 0);
	PlayButton* aPlayButton = gApp->mMainMenu->mPlayButton;
	if (theMode == 4 || !theNewGame) {
		if (theMode == 4)
			gApp->PlaySample(SOUND_ZENSTART);
		if (gApp->Is3DAccelerated()) {
			aPlayButton->mAppear.SetCurve(GTS(0x7e7500));
			aPlayButton->mAlpha.SetCurve(GTS(0x7e74d8));
		} else {
			aPlayButton->mAppear.SetCurve(GTS(0x7e7418));
			aPlayButton->mAlpha.SetCurve(GTS(0x7e73f8));
		}
	} else if (gApp->Is3DAccelerated()) {
		aPlayButton->mAppear.SetCurve(GTS(0x7e74ac));
		aPlayButton->mAlpha.SetCurve(GTS(0x7e748c), &aPlayButton->mAppear);
	} else {
		aPlayButton->mAppear.SetCurve(GTS(0x7e7460));
		aPlayButton->mAlpha.SetCurve(GTS(0x7e7440), &aPlayButton->mAppear);
	}
	gBoard->mHyperspace->mFlag157 = true;
	if (mMode == 1) {
		// classic into zen: the new board goes on from the old one's level and score
		gBoard->mUnknown158 = anOldBoard->mUnknown158 - 1;
		gBoard->mGoodTube->SetLevel(gBoard->mUnknown158);
		gBoard->mUnknown168 = anOldBoard->mUnknown168;
		gBoard->mUnknown16C = gBoard->mUnknown168;
		gBoard->mUnknown14F8 = gBoard->mUnknown168;
		gBoard->SetLevelSteps(anOldBoard->mUnknown224);
	}
	Close();
}
