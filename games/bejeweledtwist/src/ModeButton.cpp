#include "ModeButton.h"
#include "MemoryImage.h"
#include "TwistD3D.h"

#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

ModeButton::ModeButton(int theId, Image* theImage, ButtonListener* theListener)
	: TwistButton(theId, theImage, theListener)
{
	mClip = false;
	mDoor = IMAGE_PLAYSUBMENU_DOOR;
}

void ModeButton::Update()
{
	int aGrey = Ftol((double)mBrightness * 191.0 + 64.0);
	mColor = Color(aGrey, aGrey, aGrey);
	if (mHidden)
		return;
	TwistButton::Update();
	int aCels = mDoor->mNumRows * mDoor->mNumCols;
	if (mDoorState == 1) {
		float anOpen = (float)((double)mDoorOpen + 0.009999999776482582);
		if (1.0f < anOpen)
			anOpen = 1.0f;
		mDoorOpen = anOpen;
		mDoorCel = Ftol((double)aCels * (double)mDoorOpen);
		if (aCels <= mDoorCel)
			mDoorState = 2;
	} else if (mDoorState == 3) {
		float anOpen = (float)((double)mDoorOpen - 0.009999999776482582);
		if (anOpen < 0.0f)
			anOpen = 0.0f;
		mDoorOpen = anOpen;
		mDoorCel = Ftol((double)aCels * (double)mDoorOpen);
		if (mDoorCel < 1) {
			mDoorState = 0;
			mDoorCel = 0;
		}
	}
}

void ModeButton::Draw(Graphics* g)
{
	float anArt = (float)gApp->mArtRes / 1200.0f;
	mColor.mAlpha = Ftol((double)mBrightness * 255.0);
	TwistButton::Draw(g);

	int aTagId = mTagId;
	int aPictureId = mPictureId;
	if (!mTagText.empty() && 0 <= aPictureId) {
		int aButtonId = GetIdByImage(mButtonImage);
		int anX = Ftol((double)(GetImageOffsetX(aPictureId) - GetImageOffsetX(aButtonId)) * (double)anArt);
		int aY = Ftol((double)(GetImageOffsetY(aPictureId) - GetImageOffsetY(aButtonId)) * (double)anArt);
		g->SetColor(mColor);
		g->SetColorizeImages(true);
		Image* aPicture = GetImageById(aPictureId);
		g->DrawImage(aPicture, anX, aY);
		g->SetFont(FONT_INGAME);
		int aTextX;
		if (aTagId < 0) {
			// the text alone, centred under the picture
			aTextX = GetImageById(aPictureId)->mWidth / 2 + Ftol((double)gApp->mScale * 0.0) + anX;
			aTextX = (Ftol((double)gApp->mScale * 0.0) - g->GetFont()->StringWidth(mTagText) / 2) + aTextX;
			aY += Ftol((double)gApp->mScale * 52.0);
		} else {
			// the tag and the text after it, centred together
			aY = Ftol((double)(GetImageOffsetY(aTagId) - GetImageOffsetY(aButtonId)) * (double)anArt);
			int aTagWidth = GetImageById(aTagId)->mWidth;
			int aGap = Ftol((double)anArt * 5.0);
			aTextX = (aPicture->mWidth / 2 - Ftol((double)gApp->mScale + (double)gApp->mScale)) + anX;
			Image* aTag = GetImageById(aTagId);
			if (mId == 5) {
				aTextX = Ftol((double)anArt * 8.0 + (double)(aTextX - g->GetFont()->StringWidth(mTagText) / 2));
			} else {
				aTextX = aTextX - (g->GetFont()->StringWidth(mTagText) + aGap + aTagWidth) / 2;
				g->DrawImage(aTag, aTextX, aY);
				aTextX = aTextX + aGap + aTagWidth;
			}
			aY = Ftol((double)aTag->mHeight * 0.9300000071525574 + (double)aY);
		}
		g->SetColor(Color(0x20, 0x20, 0x20));
		g->DrawString(mTagText, aTextX, aY);
	}

	if (mDoorState != 2) {
		g->SetColor(Color::White);
		int aCel;
		if (mDoorState == 0) {
			aCel = 0;
			mDoorOpen = 0;
		} else {
			aCel = mDoorCel;
		}
		if (mDoorOpen < 1.0f) {
			if (gD3DInterface != nullptr && (gD3DInterface->GetCaps(0) & 2) != 0 && 0.0f < mDoorOpen) {
				// with Direct3D (and the pixel shaders) the door's two halves slide apart through a shader
				Image* aTop = IMAGE_DOORS_SUBMENU_TOP;
				gD3DInterface->SetTexture(1, IMAGE_DOORS_SUBMENU_BOTTOM);  // slot 0xd4
				gD3DInterface->SetTextureWrap(0, false);                   // slot 0xcc
				gD3DInterface->SetTextureWrap(1, false);
				gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_BUTTONCOVER]);  // slot 0xd8
				float aConstant[4] = {(float)((double)mDoorOpen * 0.85), 0.0f, 0.0f, 0.0f};
				gD3DInterface->SetShaderConstant(gTwistShaders[TWISTSHADER_BUTTONCOVER], 0, aConstant);  // slot 0x108
				((MemoryImage*)aTop)->mD3DFlags |= 0x80;
				double aTopY = (double)mHeight * 0.5040000081062317;
				double aTopX = (double)mWidth * 0.5019999742507935;
				int aDoorY = Ftol(aTopY - (double)(aTop->GetCelHeight() / 2));
				int aDoorX = Ftol(aTopX - (double)(aTop->GetCelWidth() / 2));
				g->DrawImage(aTop, aDoorX, aDoorY);
				gD3DInterface->SetPixelShader(nullptr);
				return;
			}
			double aHalfHeight = (double)mHeight * 0.5;
			double aHalfWidth = (double)mWidth * 0.4950000047683716;
			int aY = Ftol(aHalfHeight - (double)(mDoor->GetCelHeight() / 2));
			int anX = Ftol(aHalfWidth - (double)(mDoor->GetCelWidth() / 2));
			g->DrawImageCel(mDoor, anX, aY, aCel);
		}
	}
}

void ModeButton::MouseDown(int x, int y, int theClickCount)
{
	if (mDoorState == 2)
		Widget::MouseDown(x, y, theClickCount);
}

void ModeButton::PlayMouseOverSound()
{
	gApp->PlaySoundAt(SOUND_MOUSEOVER3, mX, (float)((float)mId + (float)mId), 0.0f);
}

void ModeButton::SetDoorOpen(bool theOpen, bool theNow)
{
	if (!theOpen) {
		if (mDoorState != 0)
			mDoorState = theNow ? 0 : 3;
	} else if (mDoorState != 2) {
		mDoorState = theNow ? 2 : 1;
	}
}
