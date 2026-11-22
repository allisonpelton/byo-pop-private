// Sexy::CrystalBall: see the header.
#include "CrystalBall.h"

#include "Bej3App.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "X86Math.h"

#include <algorithm>
#include <cstring>

using namespace Sexy;

namespace Sexy {

// 0x58f620 (bejeweled3): the colour of the font layer with that name (the layers' own tags are the keys of the font's
// map); the old colour is returned so that 0x58f920 (RestoreLayerColor) can put it back. port: the exe keeps a stack of
// colours in the layer; one level is all this ever uses
bool SetLayerColor(Font* theFont, const char* theName, const Color& theColor, Color* theOldColor)
{
	ImageFont* aFont = dynamic_cast<ImageFont*>(theFont);
	if (aFont == nullptr)
		return false;
	for (ActiveFontLayer& anActive : aFont->mActiveLayerList) {
		for (auto& anEntry : aFont->mFontData->mFontLayerMap) {
			if (anEntry.second == anActive.mBaseFontLayer && _stricmp(anEntry.first.c_str(), theName) == 0) {
				*theOldColor = anActive.mBaseFontLayer->mColorMult;
				anActive.mBaseFontLayer->mColorMult = theColor;
				return true;
			}
		}
	}
	return false;
}

// 0x58f920 (bejeweled3): the colour the layer had before SetLayerColor
void RestoreLayerColor(Font* theFont, const char* theName, const Color& theOldColor)
{
	ImageFont* aFont = dynamic_cast<ImageFont*>(theFont);
	if (aFont == nullptr)
		return;
	for (ActiveFontLayer& anActive : aFont->mActiveLayerList) {
		for (auto& anEntry : aFont->mFontData->mFontLayerMap) {
			if (anEntry.second == anActive.mBaseFontLayer && _stricmp(anEntry.first.c_str(), theName) == 0) {
				anActive.mBaseFontLayer->mColorMult = theOldColor;
				return;
			}
		}
	}
}

// 0x583570 (bejeweled3): the matrix a PIEffect is drawn with, a scale about the origin
void SetScaleMatrix(SexyMatrix3& theMatrix, float theScaleX, float theScaleY)
{
	theMatrix.LoadIdentity();
	theMatrix.m00 = theScaleX;
	theMatrix.m11 = theScaleY;
}

}  // namespace Sexy

namespace {

// 0x8d3370 (bejeweled3): one half
const double kHalf = 0.5;

}  // namespace

// 0x78ab50 (bejeweled3)
CrystalBall::CrystalBall(int theId, ButtonListener* theListener, const std::string& theText) :
	ButtonWidget(theId, theListener), mText(theText)
{
	mBlendCurve.SetConstant(0.0);
	mScaleCurve.SetConstant(0.17);
	mClip = false;               // +0x51: the ball, its rays and its label reach past the button's rectangle
	mHasTransparencies = true;   // +0x94
	// the effects are copies of the resource manager's, made to loop
	mBallEffect = PIEFFECT_CRYSTALBALL->Duplicate();
	mBallEffect->mLoop = true;
	mRaysEffect = PIEFFECT_CRYSTALRAYS->Duplicate();
	mRaysEffect->mLoop = true;
	// port: in 3D mode (DAT_008e170a) the exe also sets the offset curves here ("b;-3,3,0.003333,1,####     $~###    }####"
	// and "b;-3,3,0.005,1,####     $~###    }####", each given a random start); they move the ball in 3D only
}

// port: the effects are the ball's own copies
CrystalBall::~CrystalBall()
{
	delete mBallEffect;
	delete mRaysEffect;
}

// 0x78ae40 (bejeweled3): the 2D path. port: the 3D path (see the header) is not translated; the call at the end to
// 0x584d60 with +0x20 and +0x480, and the arrow's drawing through the object at 0x8d7a80, are not either
void CrystalBall::Draw(Graphics* g)
{
	if (mWidth == 0)
		return;

	g->PushState();   // 0x432410: a copy of the Graphics' state, dropped at the end (0x432520)
	const int anArtRes = gApp->mArtRes;
	const float aWidth = (float)mWidth;
	const float aHeight = (float)mHeight;

	// the ball's middle
	g->TranslateF((float)kHalf * aWidth, aHeight * (float)kHalf);
	if (!mFlat && mMoves) {
		double aMoveY = mOffsetYCurve.GetOutVal() * anArtRes / 1200.0;
		double aBlend = mBlendCurve.GetOutVal();
		double aMoveX = mOffsetXCurve.GetOutVal() * anArtRes / 1200.0;
		float aDownBy = (float)((1.0 - aBlend) * aMoveY);
		float anAcrossBy = (float)((1.0 - mBlendCurve.GetOutVal()) * aMoveX);
		g->TranslateF(anAcrossBy, aDownBy);
	}

	// the size
	float aScale = (float)mScaleCurve.GetOutVal();
	if (0.0f < mDepth)
		aScale = (0.0025500000920146704f / mDepth) * aScale;
	double aBase = aScale * 5.300000190734863;
	float aScaleX = (float)(((float)(mBlendCurve.GetOutVal() * kHalf) + 1.0f) * aBase);
	float aScaleY = (float)(((float)(mBlendCurve.GetOutVal() * 0.20000000298023224) + 1.0f) * aBase);

	// both effects' colour: grey, going dark as the ball covers the screen (three times in the exe, with the same result)
	{
		float aGrey = (float)(1.0 - mBlendCurve.GetOutVal()) * 255.0f;
		int aLevel = Ftol(std::max(aGrey, 0.0f));
		mRaysEffect->mColor = Color(aLevel, aLevel, aLevel, 255);
		mBallEffect->mColor = Color(aLevel, aLevel, aLevel, 255);
	}

	Color aColor;
	if (!mFlat) {
		// the rays, under the ball
		SetScaleMatrix(mRaysEffect->mDrawTransform, aScaleX * anArtRes / 1200.0f, anArtRes * aScaleY / 1200.0f);
		mRaysEffect->Draw(g);
		aColor = mColor;
	}
	else
		aColor = Color(mColor.mRed, mColor.mGreen, mColor.mBlue, Ftol((double)mColor.mAlpha * kHalf));
	g->SetColor(aColor);
	if (!(aColor == Color(255, 255, 255, 255)))
		g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);

	// the ball: its shadow, then the frame of its 40-frame sheet (a new one every four updates), and again added while
	// pressed
	int aCel = mFlat ? 0 : ((mUpdateCount + ((mUpdateCount >> 31) & 3)) >> 2) % 40;
	Transform aTransform;
	aTransform.Reset();
	aTransform.Scale(aScaleX, aScaleY);
	g->DrawImageTransform(IMAGE_CRYSTALBALL_SHADOW, aTransform, 0.0f, 0.0f);
	// 0x434020: the column is the cel modulo the columns (the image's +0x38), the row the cel over them
	Rect aCelRect = IMAGE_CRYSTALBALL->GetCelRect(aCel);
	g->DrawImageTransform(IMAGE_CRYSTALBALL, aTransform, aCelRect, 0.0f, 0.0f);
	if (mIsDown) {
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImageTransform(IMAGE_CRYSTALBALL_GLOW, aTransform, 0.0f, 0.0f);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	// port: here the 3D path draws the refraction mesh over the picture behind the ball (see the header)

	// the sparkles over the ball
	SetScaleMatrix(mBallEffect->mDrawTransform, aScaleX * anArtRes / 1200.0f, anArtRes * aScaleY / 1200.0f);
	mBallEffect->Draw(g);

	// the 2D cover: the picture behind the ball shows through its rectangle, faded in by the blend curve (the menu's
	// own background, 800 wide, or a picture stretched to 960 by 600, so that it stays where it is while the ball moves)
	double aBlendNow = mBlendCurve.GetOutVal();
	if (aBlendNow != 0.0 && (Image*)mCover != nullptr) {
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol(mBlendCurve.GetOutVal() * 255.0)));
		Image* aCover = mCover;
		if (aCover->mWidth == 800)
			g->DrawImage(aCover, Ftol(-g->mTransX), Ftol(-g->mTransY));
		else
			g->DrawImage(aCover, Ftol((double)(float)(anArtRes * -160 / 1200) - g->mTransX), Ftol(-g->mTransY), 960, 600);
	}
	g->PopState();   // 0x432520: the label and the arrow are drawn from the ball's own place again

	// port: the hint drawn when +0x487 and the app's flag at +0x687 are set (0x6a0ca0's tips) is not translated

	// the label's scale: the highlight through a curve, times the property, plus the added scale, times the ball's
	CurvedVal aTextCurve;   // 0x436940: a new one on each Draw, as the exe makes it
	aTextCurve.SetCurve("b+1,1.3,0,1,####         ~~###");
	double aTextFactor = gApp->GetDouble("CRYSTALBALL_TEXT_SCALE_FACTOR", 4.0);
	float aTextScale = (float)(aTextCurve.GetOutVal((double)mHighlight) * (double)(float)aTextFactor + mTextScaleAdd) * aScale;
	float aTextAlpha = (float)((1.0 - (mBlendCurve.GetOutVal() + mBlendCurve.GetOutVal())) * mFade);

	// the menu arrow, while the label has faded: the right-hand one mirrored, else the left-hand one
	if (aTextAlpha < 1.0f) {
		g->PushState();
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol((1.0f - aTextAlpha) * 255.0f)));
		Transform anArrowTransform;
		anArrowTransform.Reset();
		int anArrowOffset = anArtRes * 200 / 1200;
		if (mArrowRightCurve.GetOutVal() > 0.0) {
			anArrowTransform.Scale(-aTextScale, aTextScale);
			float anX = (float)(mWidth / 2) + (float)((1.0 - mArrowRightCurve.GetOutVal()) * anArrowOffset);
			g->DrawImageTransform(IMAGE_MENU_ARROW, anArrowTransform, anX, (float)(mHeight / 2));
		}
		else if (mArrowLeftCurve.GetOutVal() > 0.0) {
			anArrowTransform.Scale(aTextScale, aTextScale);
			float anX = (float)(mWidth / 2) - (float)((1.0 - mArrowLeftCurve.GetOutVal()) * anArrowOffset);
			g->DrawImageTransform(IMAGE_MENU_ARROW, anArrowTransform, anX, (float)(mHeight / 2));
		}
		g->PopState();
	}

	if (!mDrawPicture) {
		// the label, with the font's layers coloured: the outline white, the bottom glow (224, 64, 160) and the main
		// layer in the ball's text colour, scaled about the ball's middle
		if (!mText.empty() && aTextAlpha > 0.0f && FONT_PLAYBUTTONS != nullptr) {
			g->SetFont(FONT_PLAYBUTTONS);
			Color anOldOutline, anOldGlow, anOldMain;
			SetLayerColor(FONT_PLAYBUTTONS, "GLOWOUTLINE", Color(255, 255, 255, 255), &anOldOutline);
			SetLayerColor(FONT_PLAYBUTTONS, "BOTTOMGLOW", Color(224, 64, 160, 255), &anOldGlow);
			SetLayerColor(FONT_PLAYBUTTONS, "MAIN", mTextColor, &anOldMain);
			g->SetColor(Color(255, 255, 255, Ftol(aTextAlpha * 255.0f)));
			g->PushState();
			g->SetScale(aTextScale, aTextScale, (float)(mWidth / 2), (float)(mHeight / 2));
			int aTextY = mHeight / 2 + anArtRes * 34 / 1200;
			g->DrawString(mText, mWidth / 2 - FONT_PLAYBUTTONS->StringWidth(mText) / 2, aTextY);
			RestoreLayerColor(FONT_PLAYBUTTONS, "GLOWOUTLINE", anOldOutline);
			RestoreLayerColor(FONT_PLAYBUTTONS, "BOTTOMGLOW", anOldGlow);
			RestoreLayerColor(FONT_PLAYBUTTONS, "MAIN", anOldMain);
			g->PopState();
		}
	}
	else {
		// a locked mode: the question mark, centred on the ball, scaled as the label would be (0x713770, 0x713850)
		g->SetColor(Color(255, 255, 255, Ftol(aTextAlpha * 255.0)));
		g->PushState();
		g->SetScale(aTextScale, aTextScale, (float)(mWidth / 2), (float)(mHeight / 2));
		g->DrawImage(IMAGE_OVERWORLD_QUESTIONMARK, mWidth / 2 - IMAGE_OVERWORLD_QUESTIONMARK->GetWidth() / 2,
			mHeight / 2 - IMAGE_OVERWORLD_QUESTIONMARK->GetHeight() / 2);
		g->PopState();
	}
}

// 0x78c730 (bejeweled3): the highlight follows the mouse by 0.05 an update
void CrystalBall::Update()
{
	Widget::Update();
	mUpdateCount++;
	mBallEffect->Update();
	mRaysEffect->Update();
	if (!mIsOver)
		mHighlight = std::max(mHighlight - 0.05f, 0.0f);
	else
		mHighlight = std::min(mHighlight + 0.05f, 1.0f);
	MarkDirty();
}
