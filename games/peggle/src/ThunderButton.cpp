// ThunderButton.cpp: see ThunderButton.h.

#include "ThunderButton.h"

#include "DDImage.h"
#include "ImageFont.h"
#include "Res.h"
#include "SexyAppBase.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "ThunderballApp.h"
#include "WidgetMover.h"

#include <algorithm>

using namespace Sexy;

// The colours of the button font's three layers for each ButtonFontColor (the table the original builds at 0x40b570 the
// first time a colour is set): red, orange, yellow, ..., then the check boxes'
static const int FONT_COLOR_SETS[NUM_BUTTON_FONT_COLORS][3][4] = {
	{ {86, 7, 0, 204}, {255, 186, 2, 255}, {252, 94, 0, 255} },
	{ {76, 0, 12, 204}, {250, 200, 65, 255}, {255, 175, 0, 255} },
	{ {196, 20, 0, 204}, {246, 253, 66, 255}, {244, 178, 20, 255} },
	{ {130, 28, 1, 255}, {255, 243, 163, 255}, {255, 204, 0, 255} },
	{ {179, 39, 2, 255}, {255, 250, 218, 255}, {255, 228, 0, 255} },
	{ {229, 94, 4, 255}, {255, 255, 255, 255}, {255, 254, 172, 255} },
	{ {15, 94, 20, 204}, {211, 246, 92, 255}, {10, 221, 40, 255} },
	{ {15, 94, 20, 204}, {211, 246, 92, 255}, {10, 221, 40, 255} },
	{ {81, 12, 114, 204}, {243, 177, 253, 255}, {221, 84, 255, 255} },
	{ {84, 15, 81, 204}, {245, 179, 255, 255}, {209, 81, 234, 255} },
	{ {91, 30, 84, 255}, {251, 84, 234, 255}, {204, 43, 183, 255} },
	{ {96, 2, 86, 255}, {254, 148, 231, 255}, {224, 140, 239, 255} },
	{ {229, 51, 204, 255}, {253, 182, 247, 255}, {255, 152, 255, 255} },
	{ {19, 51, 216, 255}, {101, 243, 248, 255}, {76, 196, 247, 255} },
	{ {26, 59, 214, 255}, {255, 255, 255, 255}, {210, 247, 255, 255} },
	{ {10, 93, 204, 255}, {194, 246, 255, 255}, {127, 250, 255, 255} },
	{ {0, 106, 41, 255}, {255, 253, 228, 255}, {246, 255, 0, 255} },
};

// 0x40b570
void Sexy::SetButtonFontColor(int theColor)
{
	ImageFont* aFont = static_cast<ImageFont*>(FONT_BUTTON);
	int aLayerIndex = 0;
	for (FontLayerList::iterator aLayer = aFont->mFontData->mFontLayerList.begin(); aLayer != aFont->mFontData->mFontLayerList.end();
		 ++aLayer, aLayerIndex++)
	{
		const int* aSet = FONT_COLOR_SETS[theColor][aLayerIndex < 3 ? aLayerIndex : 2];
		if (aLayerIndex == 2)
			aLayer->mColorAdd = Color(aSet[0], aSet[1], aSet[2], aSet[3]);
		else
			aLayer->mColorMult = Color(aSet[0], aSet[1], aSet[2], aSet[3]);
	}
}

// 0x405020
void Sexy::DrawScreenFrame(Graphics* g)
{
	g->DrawImage(IMAGE_SCREENFRAMELEFT, 0, 0);
	g->DrawImage(IMAGE_SCREENFRAMETOP, IMAGE_SCREENFRAMELEFT->GetWidth(), 0);
	g->DrawImage(IMAGE_SCREENFRAMEBOTTOM, IMAGE_SCREENFRAMELEFT->GetWidth(),
				 IMAGE_SCREENFRAMELEFT->GetHeight() - IMAGE_SCREENFRAMEBOTTOM->GetHeight());
	g->DrawImage(IMAGE_SCREENFRAMERIGHT, IMAGE_SCREENFRAMETOP->GetWidth() + IMAGE_SCREENFRAMELEFT->GetWidth(), 0);
}

// ---- ThunderButton ----

static const int NO_SOUND = -1;
static const int NEVER_HOVERED = -1000;			// the "last hover time" before the first hover sound
static const int HOVER_SOUND_GAP = 30;			// updates between two hover sounds of one button (ModValInt 0x1e)
static const int DEFAULT_BLINK_PERIOD = 60;		// updates (ModValInt 0x3c)
static const int BLINK_HOLD = 30;				// updates a fading blink stays at full (ModValInt 0x1e)
static const float BLINK_FADE_STRENGTH = 0.7f;	// how far the pressed look is faded in at most (ModValDouble 0.7)
static const int DARKEN_COLOR = 0x404040;		// added over a pressed button (ModValInt 0x404040)

// 0x49ba30
ThunderButton::ThunderButton(Image* theComponentImage, int theId, ButtonListener* theListener) :
	DialogButton(theComponentImage, theId, theListener)
{
	mOverAlphaSpeed = 0.08;
	mOverAlphaFadeInSpeed = 0.2;
	mLastHoverTime = NEVER_HOVERED;
	mDownSound = NO_SOUND;
	mHoverSound = NO_SOUND;
	mHoverSoundPitch = 0.0f; // (the original leaves it as it was in memory; every button that has a hover sound sets it)
	mDrawEnabled = true;
	mFontColor = BUTTON_FONT_COLOR_NONE;
	mRightClicked = false;
	mBlinkCount = 0;
	mBlinkFades = false;
	mBlinkPeriod = DEFAULT_BLINK_PERIOD;
	mStopBlinkOnHover = false;
	mDarkenWhenPressed = false;
	mTranslateY = 0;
	mTranslateX = 0;
}

// 0x496c00
void ThunderButton::Blink(int theTimes, bool theFlashFirst)
{
	int aCount = mBlinkPeriod * theTimes - 1;
	mBlinkCount = aCount;
	if (!theFlashFirst)
		mBlinkCount = mBlinkPeriod / 2 + aCount;
}

// 0x496c40
void ThunderButton::StopBlink()
{
	mBlinkCount = 0;
}

// 0x496c50
void ThunderButton::MouseEnter()
{
	ButtonWidget::MouseEnter();
	if (mStopBlinkOnHover)
		mBlinkCount = 0;

	ThunderballApp* anApp = gThunderballApp;
	if (anApp != nullptr && mHoverSound >= 0)
	{
		if (anApp->mUpdateCount - mLastHoverTime > HOVER_SOUND_GAP && !anApp->mWidgetMover.IsMoving())
		{
			SoundInstance* aSound = anApp->mSoundManager->GetSoundInstance(mHoverSound);
			if (aSound == nullptr)
			{
				anApp->PlaySample(mHoverSound);
			}
			else
			{
				aSound->AdjustPitch(mHoverSoundPitch);
				aSound->Play(false, true);
			}
			mLastHoverTime = anApp->mUpdateCount;
		}
	}
}

// 0x496d30
void ThunderButton::MouseDown(int x, int y, int theClickCount)
{
	if (mDownSound >= 0)
		gSexyAppBase->PlaySample(mDownSound);
	DialogButton::MouseDown(x, y, theClickCount);
}

// 0x496d70
void ThunderButton::MouseUp(int x, int y, int theClickCount)
{
	mRightClicked = theClickCount < 0;
	DialogButton::MouseUp(x, y, theClickCount);
}

// 0x496da0
void ThunderButton::Update()
{
	ButtonWidget::Update();
	if (mBlinkCount != 0 && gSexyAppBase->mActive)
	{
		mBlinkCount--;
		MarkDirty();
	}
}

// 0x49d440
void ThunderButton::Draw(Graphics* g)
{
	if (!mDrawEnabled)
		return;

	if (mFontColor != BUTTON_FONT_COLOR_NONE)
	{
		// one step on with the mouse over it or the button pressed, two with both
		int aColor = mFontColor;
		if (mIsDown && mIsOver)
			aColor += 2;
		else if (mIsDown || mIsOver)
			aColor += 1;
		SetButtonFontColor(aColor);
	}

	bool aDrawNormal = mBlinkCount == 0;
	if (!aDrawNormal && !mBlinkFades)
	{
		// the flashing blink alternates between the normal look and the one with the mouse over it
		if ((mBlinkCount / (mBlinkPeriod / 2)) % 2 == 0)
			aDrawNormal = true;
	}
	if (!aDrawNormal && (mIsDown || mIsOver))
		aDrawNormal = true;

	if (aDrawNormal)
	{
		DialogButton::Draw(g);
		if (mDarkenWhenPressed && mIsDown && mIsOver)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			g->SetColor(Color(DARKEN_COLOR));
			DialogButton::Draw(g);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g->SetColorizeImages(false);
		}
		return;
	}

	double aSavedOverAlpha = mOverAlpha;
	if (!mBlinkFades)
	{
		mOverAlpha = 0;
		mIsOver = true;
		DialogButton::Draw(g);
		mOverAlpha = aSavedOverAlpha;
		mIsOver = false;
		return;
	}

	// the fading blink fades the pressed look in and out, the way the mouse over it fades the other
	int aPeriod = mBlinkPeriod + BLINK_HOLD;
	int aPhase = mBlinkCount % aPeriod;
	int aHalf = mBlinkPeriod / 2;
	double aAlpha;
	if (aPhase >= aHalf && aPhase < aHalf + BLINK_HOLD)
	{
		aAlpha = 1.0;
	}
	else
	{
		if (aPhase >= aHalf)
			aPhase = aPeriod - aPhase;
		aAlpha = (double)aPhase / (double)aHalf;
	}
	mOverAlpha = aAlpha;
	mOverAlpha = mOverAlpha * mOverAlpha;
	mOverAlpha = (double)(BLINK_FADE_STRENGTH * (float)mOverAlpha);
	mIsOver = false;
	std::swap(mDownRect, mOverRect);
	DialogButton::Draw(g);
	std::swap(mDownRect, mOverRect);
	mOverAlpha = aSavedOverAlpha;
	mIsOver = false;
}

// ---- SimpleButton ----

static const int SIMPLE_BUTTON_COLOR = 0xffff00;
static const int SIMPLE_BUTTON_OVER_COLOR = 0x88ff88;
static const int SIMPLE_BUTTON_DOWN_COLOR = 0x88ff88;

// 0x496de0
SimpleButton::SimpleButton(Image* theImage, int theId, ButtonListener* theListener) :
	ButtonWidget(theId, theListener)
{
	mDoFinger = true;
	mImage = theImage;
	mOverColor = SIMPLE_BUTTON_OVER_COLOR;
	mDownColor = SIMPLE_BUTTON_DOWN_COLOR;
	mColor = SIMPLE_BUTTON_COLOR;
}

// 0x496e90
void SimpleButton::Draw(Graphics* g)
{
	int aX = (mWidth - mImage->GetWidth()) / 2;
	int aY = (mHeight - mImage->GetHeight()) / 2;
	if (mIsDown && mIsOver)
	{
		g->SetColor(Color(mDownColor));
		aX++;
		aY++;
	}
	else if (!mIsDown && mIsOver)
		g->SetColor(Color(mOverColor));
	else
		g->SetColor(Color(mColor));
	g->SetColorizeImages(true);
	g->DrawImage(mImage, aX, aY);
	g->SetColorizeImages(false);
}

// 0x496f60
void SimpleButton::MouseDown(int x, int y, int theClickCount)
{
	ButtonWidget::MouseDown(x, y, theClickCount);
	gSexyAppBase->PlaySample(SOUND_BUTTON1);
	MarkDirty();
}

// 0x496fa0
void SimpleButton::MouseUp(int x, int y)
{
	ButtonWidget::MouseUp(x, y);
	MarkDirty();
}

// 0x496fd0
void SimpleButton::MouseEnter()
{
	ButtonWidget::MouseEnter();
	MarkDirty();
}

// 0x496ff0
void SimpleButton::MouseLeave()
{
	ButtonWidget::MouseLeave();
	MarkDirty();
}

// ---- ThunderCheckbox ----

// 0x49e7c0
ThunderCheckbox::ThunderCheckbox(Image* theUncheckedImage, Image* theCheckedImage, int theId, CheckboxListener* theListener) :
	Checkbox(theUncheckedImage, theCheckedImage, theId, theListener)
{
	mClickSound = NO_SOUND;
	mStaysChecked = false;
	mTextOffsetX = 0;
	mTextOffsetY = 0;
	mLabelJustify = 0;
	mOverAlpha = 0;
	mOverAlphaSpeed = 0.08;
	mOverAlphaFadeInSpeed = 0.2;
	mFont = FONT_BUTTON;
}

// 0x497010
void ThunderCheckbox::MouseEnter()
{
	Widget::MouseEnter();
	if (mOverAlphaFadeInSpeed == 0.0 && mOverAlpha > 0.0)
		mOverAlpha = 0;
	MarkDirty();
}

// 0x497050
void ThunderCheckbox::MouseLeave()
{
	Widget::MouseLeave();
	if (mOverAlphaSpeed == 0.0 && mOverAlpha > 0.0)
		mOverAlpha = 0; // no fade out: the over picture goes at once
	else if (mOverAlphaSpeed > 0.0 && mOverAlpha == 0.0)
		mOverAlpha = 1.0; // it fades out from full
	MarkDirty();
}

// 0x4970b0
void ThunderCheckbox::MouseDown(int x, int y, int theClickCount)
{
	if (!mStaysChecked || !mChecked)
		Checkbox::MouseDown(x, y, theClickCount);
	if (mClickSound >= 0)
		gSexyAppBase->PlaySample(mClickSound);
}

// 0x497100
void ThunderCheckbox::MouseUp(int x, int y, int theClickCount)
{
	Checkbox::MouseUp(x, y, theClickCount);
	MarkDirty();
}

// 0x497130
void ThunderCheckbox::Update()
{
	Widget::Update();
	if (!mIsDown)
	{
		if (!mIsOver && mOverAlpha > 0.0)
		{
			// the over picture fades out
			if (mOverAlphaSpeed <= 0.0)
				mOverAlpha = 0;
			else
			{
				mOverAlpha -= mOverAlphaSpeed;
				if (mOverAlpha < 0.0)
					mOverAlpha = 0;
			}
			MarkDirty();
			return;
		}
	}
	if (!mIsOver)
		return;
	if (mOverAlphaFadeInSpeed > 0.0 && mOverAlpha < 1.0)
	{
		mOverAlpha += mOverAlphaFadeInSpeed;
		if (mOverAlpha > 1.0)
			mOverAlpha = 1.0;
		MarkDirty();
	}
}

// 0x49bb10
void ThunderCheckbox::Draw(Graphics* g)
{
	Rect aDestRect(0, 0, mWidth, mHeight);

	int aFontColor;
	if (mLabelJustify == 1)
	{
		if (mChecked || (mIsDown && mIsOver))
			aFontColor = BUTTON_FONT_COLOR_CHECKBOX_DOWN_OVER;
		else if (mIsDown || mIsOver)
			aFontColor = BUTTON_FONT_COLOR_CHECKBOX_DOWN;
		else
			aFontColor = BUTTON_FONT_COLOR_CHECKBOX;
	}
	else
		aFontColor = mIsOver ? BUTTON_FONT_COLOR_CHECKBOX_OVER : BUTTON_FONT_COLOR_CHECKBOX;
	SetButtonFontColor(aFontColor);

	const Rect* aSourceRect;
	const Rect* anOverRect;
	if (!mChecked)
	{
		aSourceRect = &mUncheckedRect;
		anOverRect = &mOverUncheckedRect;
	}
	else
	{
		aSourceRect = &mCheckedRect;
		anOverRect = &mOverCheckedRect;
	}

	if (mOverAlpha <= 0.0 || anOverRect->mWidth < 1)
	{
		if (mIsOver && anOverRect->mWidth >= 1)
			aSourceRect = anOverRect;
		g->DrawImageBox(*aSourceRect, aDestRect, mUncheckedImage);
	}
	else
	{
		if (mOverAlpha < 1.0)
			g->DrawImageBox(*aSourceRect, aDestRect, mUncheckedImage);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int)(mOverAlpha * 255)));
		g->DrawImageBox(*anOverRect, aDestRect, mUncheckedImage);
		g->SetColorizeImages(false);
	}

	g->SetFont(mFont);
	int aTextWidth = g->GetFont()->StringWidth(mLabel);
	int aTextX = mTextOffsetX;
	if (mLabelJustify == 1)
		aTextX += (mWidth - aTextWidth) / 2;
	int aTextTop = (mHeight - mFont->GetHeight()) / 2;
	int aTextY = mFont->GetAscent() + mTextOffsetY + aTextTop;
	g->SetColor(Color(0xffffff));
	g->DrawString(mLabel, aTextX, aTextY);
}

// ---- ThunderSlider ----

static const int SLIDER_X = -170;			// where the slider's picture is drawn from its top left (ModValInt 0xffffff56)
static const int SLIDER_Y = -2;
static const int SLIDER_LABEL_X = 18;		// the label's place in the picture
static const int SLIDER_LABEL_Y = 31;
static const int SLIDER_HILIGHT_Y = 9;		// where the lit part of the track is drawn

// 0x4ab510
ThunderSlider::ThunderSlider(const std::string& theLabel, int theId, SliderListener* theListener) :
	Slider(nullptr, IMAGE_DLG_SLIDERORB, theId, theListener),
	mLabel(theLabel)
{
	mClip = false;
}

// 0x49bd70
void ThunderSlider::Draw(Graphics* g)
{
	g->DrawImage(IMAGE_DLG_SLIDER, SLIDER_X, SLIDER_Y);
	g->SetFont(FONT_BUTTON);
	g->SetColor(Color(0xffffff));
	SetButtonFontColor(BUTTON_FONT_COLOR_CHECKBOX);
	g->DrawString(mLabel, SLIDER_LABEL_X + SLIDER_X, SLIDER_LABEL_Y + SLIDER_Y);

	// the part of the track the orb has passed is drawn lit
	Image* aHilight = IMAGE_DLG_SLIDERHILIGHT;
	int aHalfOrb = mThumbImage->GetWidth() / 2;
	int aLitWidth = (int)((aHilight->GetWidth() - aHalfOrb * 2) * mVal + aHalfOrb);
	if (aLitWidth < 0)
		aLitWidth = 0;
	else if (aLitWidth > aHilight->GetWidth())
		aLitWidth = aHilight->GetWidth();
	g->DrawImage(aHilight, 0, SLIDER_HILIGHT_Y, Rect(0, 0, aLitWidth, aHilight->GetHeight()));
	Slider::Draw(g);
}

// ---- ThunderFrame ----

// 0x497200
ThunderFrame::ThunderFrame()
{
	mHasTransparencies = true;
	mHasAlpha = true;
	mMouseVisible = false;
}

// 0x49beb0
void ThunderFrame::Draw(Graphics* g)
{
	g->DrawImageBox(Rect(0, 0, mWidth, mHeight), IMAGE_DLG_BLUEFRAME);
}
