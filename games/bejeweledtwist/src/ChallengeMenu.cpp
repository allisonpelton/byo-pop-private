// Sexy::ChallengeMenu and Sexy::ChallengeButton (ChallengeMenu.h).
#include "ChallengeMenu.h"

#include "AwardTelly.h"
#include "Board.h"
#include "RankBonusWidget.h"
#include "Buffer.h"
#include "Challenge.h"
#include "Common.h"
#include "DDInterface.h"
#include "EffectsManager.h"
#include "Font.h"
#include "GameOverShow.h"
#include "Image.h"
#include "MemoryImage.h"
#include "GameText.h"
#include "Graphics.h"
#include "Hyperspace.h"
#include "Menu.h"
#include "OrbitText.h"
#include "PlayButton.h"
#include "Profile.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "TwistButton.h"
#include "TriVertex.h"
#include "TwistD3D.h"
#include "TwistDialog.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"


#include <cmath>
#include <cstdio>

using namespace Sexy;

// The globals of the menu's camera and sun (ChallengeMenu.h)
CurvedVal Sexy::gSunCurve;
Transform3D Sexy::gMenuCamera;
float Sexy::gUnknown85C508 = 0;
Color Sexy::gUnknown85C50C;
Color Sexy::gSunGlowColor;
float Sexy::gAspect = 0;
float Sexy::gAspect45 = 0;

namespace {

// 0x7570c1: the colour a blended towards b by theT (a's alpha kept)
Color BlendColors(const Color& a, const Color& b, float theT)
{
	Color aColor = a;
	aColor.mRed = Ftol((double)(b.mRed - a.mRed) * (double)theT + (double)aColor.mRed);
	aColor.mGreen = Ftol((double)(b.mGreen - a.mGreen) * (double)theT + (double)aColor.mGreen);
	aColor.mBlue = Ftol((double)(b.mBlue - a.mBlue) * (double)theT + (double)aColor.mBlue);
	return aColor;
}

// The camera curves are in degrees: the radians as a float, as 0x6e2318 and 0x6e3f41 make them
float CurveRadians(CurvedVal& theCurve)
{
	float aDegrees = (float)(theCurve.GetOutVal() + 0.0);
	return (float)((double)aDegrees * 3.1415927410125732 / 180.0);
}

// 0x401880: the colour of a packed 0xAARRGGBB, its alpha as it is
Color UnpackColor(unsigned int theColor)
{
	return Color((theColor >> 16) & 0xff, (theColor >> 8) & 0xff, theColor & 0xff, theColor >> 24);
}

// DrawPrimitiveEx's listener slots 0 and 1 are a WorldGraphicsListener's Apply and Remove (as in HyperspaceD3D.cpp)
class PrimitiveListener : public D3DDrawPrimitiveListener {
public:
	explicit PrimitiveListener(WorldGraphicsListener* theListener) : mListener(theListener) {}
	void Before() override { mListener->Apply(); }
	void After() override { mListener->Remove(); }
	WorldGraphicsListener* mListener;
};

// 0x4c0840: a vector made unit (left alone when it has no length)
Vec3 NormalizedVec(const Vec3& v)
{
	float aLength = (float)std::sqrt((double)(v.y * v.y + v.x * v.x) + (double)(v.z * v.z));
	if (aLength == 0.0f)
		return v;
	float anInverse = (float)(1.0 / (double)aLength);
	return Vec3{v.x * anInverse, v.y * anInverse, v.z * anInverse};
}

Vec3 CrossVec(const Vec3& a, const Vec3& b)  // 0x4c06d0: a crossed with b
{
	return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float DotVec(const Vec3& a, const Vec3& b)  // 0x4c06a0
{
	return (float)((double)(b.z * a.z) + (double)(b.x * a.x) + (double)(b.y * a.y));
}

// 0x674035 (0x67411e puts the place first): the transform turned to look from its place away from theTarget (its third row
// is the way from the target to the place), theUp above it; left alone when the way is all but nothing or all but along
// theUp. True when it turned.
bool LookFrom(Transform3D& thePlace, const Vec3& theTarget, const Vec3& theUp)
{
	Vec3 aWay{thePlace.mPos.x - theTarget.x, thePlace.mPos.y - theTarget.y, thePlace.mPos.z - theTarget.z};  // 0x673bb6
	// 0x673f7f: the way is all but nothing
	if (std::fabs(aWay.x) <= 0.0010000000474974513 && std::fabs(aWay.y) <= 0.0010000000474974513 &&
		std::fabs(aWay.z) <= 0.0010000000474974513)
		return false;
	Vec3 aDirection = NormalizedVec(aWay);
	float aDot = DotVec(theUp, aDirection);
	if (std::fabs(aDot) > 0.9989999999525025)  // 0x7d7a60
		return false;
	thePlace.mRot.m[6] = aDirection.x;
	thePlace.mRot.m[7] = aDirection.y;
	thePlace.mRot.m[8] = aDirection.z;
	Vec3 aRight = NormalizedVec(CrossVec(theUp, aDirection));
	thePlace.mRot.m[0] = aRight.x;
	thePlace.mRot.m[1] = aRight.y;
	thePlace.mRot.m[2] = aRight.z;
	Vec3 aTop = NormalizedVec(CrossVec(aDirection, aRight));
	thePlace.mRot.m[3] = aTop.x;
	thePlace.mRot.m[4] = aTop.y;
	thePlace.mRot.m[5] = aTop.z;
	return true;
}

// The vertex of the planets' spheres (0x24 bytes; FVF 0x152: position, normal, diffuse, one texture coordinate; made by
// 0x680276 and 0x6801fe)
struct SphereVertex {
	float x, y, z;     // +0x00
	float nx, ny, nz;  // +0x0c: the position made unit
	unsigned int mColor = 0xffffffff;  // +0x18
	float u, v;        // +0x1c
};

// The three sphere meshes the menu makes (0x6e5c3e): the sizes 15 by 20, 20 by 28 and 32 by 48 (0x85c5f8, 0x85c5d8,
// 0x85c5e8); the planets pick one by the size they have on the screen, the moon uses the biggest
std::vector<SphereVertex> gSphereSmall;
std::vector<SphereVertex> gSphereMiddle;
std::vector<SphereVertex> gSphereBig;

// 0x680276: the vertex at (x, y, z) with the texture coordinate (theU, theV) and the normal (0x6801fe)
SphereVertex MakeSphereVertex(float x, float y, float z, float theU, float theV)
{
	SphereVertex aVertex;
	aVertex.x = x;
	aVertex.y = y;
	aVertex.z = z;
	aVertex.mColor = 0xffffffff;
	aVertex.u = theU;
	aVertex.v = theV;
	Vec3 aNormal = NormalizedVec(Vec3{x, y, z});
	aVertex.nx = aNormal.x;
	aVertex.ny = aNormal.y;
	aVertex.nz = aNormal.z;
	return aVertex;
}

// 0x6e4fad: a sphere of theStacks bands from pole to pole by theSlices columns, as a list of triangles (the poles' bands
// one triangle a column, the others two); nothing when the list is made already
void BuildSphere(std::vector<SphereVertex>& theVertices, int theStacks, int theSlices)
{
	if (!theVertices.empty())
		return;
	int aCount = (theStacks * 2 - 2) * theSlices * 3;  // 0x6e340c, 0x6e4bbb
	theVertices.resize(aCount);
	float aStacks = (float)theStacks;
	float aStackStep = (float)(3.1415927 / (double)aStacks);
	float aSlices = (float)theSlices;
	float aSliceStep = (float)((double)6.2831855f / (double)aSlices);  // _DAT_0084ec00
	SphereVertex* aVertex = theVertices.data();
	int i = 0;
	if (0 < theStacks) {
		do {
			float aLat0 = (float)i * aStackStep;
			float aSinLat0 = sinf(aLat0);
			int aNext = i + 1;
			float aLat1 = (float)aNext * aStackStep;
			float aSinLat1 = sinf(aLat1);
			float aCosLat0 = cosf(aLat0);
			float aCosLat1 = cosf(aLat1);
			int j = 0;
			if (0 < theSlices) {
				float aV0 = (float)i / aStacks;
				float aV1 = (float)aNext / aStacks;
				do {
					float aJ = (float)j;
					float aLon0 = aJ * aSliceStep;
					int aNextJ = j + 1;
					float aSinLon0 = sinf(aLon0);
					float aLon1 = (float)aNextJ * aSliceStep;
					float aSinLon1 = sinf(aLon1);
					float aCosLon0 = cosf(aLon0);
					float aCosLon1 = cosf(aLon1);
					float aX0a = aSinLon0 * aSinLat0;  // the upper edge of the band
					float aX1a = aSinLon1 * aSinLat0;
					float aX0b = aSinLon0 * aSinLat1;  // the lower edge
					float aX1b = aSinLon1 * aSinLat1;
					float aZ0a = aCosLon0 * aSinLat0;
					float aZ1a = aCosLon1 * aSinLat0;
					float aZ0b = aCosLon0 * aSinLat1;
					float aZ1b = aCosLon1 * aSinLat1;
					float aU0 = aJ / aSlices;
					float aU1 = (float)(((double)aJ + 1.0) / (double)aSlices);
					if (i != theStacks - 1) {
						aVertex[0] = MakeSphereVertex(aX1a, aCosLat0, aZ1a, aU1, aV0);
						aVertex[1] = MakeSphereVertex(aX1b, aCosLat1, aZ1b, aU1, aV1);
						aVertex[2] = MakeSphereVertex(aX0b, aCosLat1, aZ0b, aU0, aV1);
						aVertex += 3;
					}
					if (i != 0) {
						aVertex[0] = MakeSphereVertex(aX0a, aCosLat0, aZ0a, aU0, aV0);
						aVertex[1] = MakeSphereVertex(aX1a, aCosLat0, aZ1a, aU1, aV0);
						aVertex[2] = MakeSphereVertex(aX0b, aCosLat1, aZ0b, aU0, aV1);
						aVertex += 3;
					}
					j = aNextJ;
				} while (j < theSlices);
			}
			i = aNext;
		} while (i < theStacks);
	}
}

// A sphere's vertices all given the alpha theAlpha (in the top byte of their colour)
void SetSphereAlpha(std::vector<SphereVertex>& theVertices, int theAlpha)
{
	for (size_t i = 0; i < theVertices.size(); i++)
		theVertices[i].mColor = (theVertices[i].mColor & 0xffffff) | ((unsigned int)theAlpha << 24);
}

// 0x6dda7c's vertex (FVF 0x142: position, diffuse, one texture coordinate)
struct BillboardVertex {
	float x, y, z;
	unsigned int mColor;
	float u, v;
};

// 0x6dda7c: a picture standing at a point of the menu's 3D world, its middle where gMenuCamera sees the point and as
// big as theSize planet sizes there (76 pixels by 0.00151515 over the depth for a planet's 76 by 80; the scale is
// 0.00151515 over the depth over the picture's width, times theSize times 80). It leaves the draw mode, the colour
// and colorizing set. With Direct3D (gD3DInterface) the original draws a quad of the picture's proportions facing the
// camera instead (0x6ddd52), and sets nothing of the Graphics.
void DrawBillboard(Graphics* g, Image* theImage, float theX, float theY, float theZ, unsigned int theColor, int theDrawMode,
	float theSize)
{
	PerspectiveCamera aCamera(gAspect45, gAspect, 1.0f, 10000.0f);  // 0x67417d
	aCamera.mTransform = gMenuCamera;                              // 0x673c10
	WorldGraphicsListener aListener(&aCamera, false);              // 0x4bd5d0
	aListener.SetCulling(false);                                   // 0x4bd6c0
	aListener.SetDepth(true, true);                                // 0x4bd680
	if (gD3DInterface != nullptr) {
		// The place: scaled by theSize, at the point, turned to look away from the camera with y up (0x68b341, 0x6dd0e2,
		// 0x67411e)
		Transform3D aPlace;  // 0x4bd4c0
		aPlace.mScale.x = theSize * aPlace.mScale.x;
		aPlace.mScale.y = theSize * aPlace.mScale.y;
		aPlace.mScale.z = theSize * aPlace.mScale.z;
		aPlace.mPos.x = theX + aPlace.mPos.x;
		aPlace.mPos.y = theY + aPlace.mPos.y;
		aPlace.mPos.z = theZ + aPlace.mPos.z;
		aPlace.mPos = Vec3{theX, theY, theZ};
		LookFrom(aPlace, gMenuCamera.mPos, Vec3{0.0f, 1.0f, 0.0f});
		aListener.mWorld = aPlace;  // 0x673c10
		// The picture's proportions: two triangles, the quad's corners a little left of and below the middle
		float anAspect = (float)theImage->mWidth / (float)theImage->mHeight;
		BillboardVertex aTopLeft = {(float)(-0.019999999552965164 - (double)anAspect), 0.98f, 0.0f, theColor, 0.0f, 0.0f};
		BillboardVertex aTopRight = {(float)((double)anAspect - 0.019999999552965164), 0.98f, 0.0f, theColor, 1.0f, 0.0f};
		BillboardVertex aBottomRight = {aTopRight.x, -1.02f, 0.0f, theColor, 1.0f, 1.0f};
		BillboardVertex aBottomLeft = {aTopLeft.x, -1.02f, 0.0f, theColor, 0.0f, 1.0f};
		BillboardVertex aTriangles[6] = {aTopLeft, aTopRight, aBottomRight, aBottomRight, aBottomLeft, aTopLeft};
		PrimitiveListener aPrimitiveListener(&aListener);
		gD3DInterface->SetTextureWrap(0, false);  // slot 0xcc
		gD3DInterface->SetTexture(0, theImage);   // slot 0xd4
		gD3DInterface->DrawPrimitiveEx(0x142, GPUPT_TRIANGLELIST, aTriangles, 2, Color(255, 255, 255, 255), theDrawMode,
			0.0f, 0.0f, true, &aPrimitiveListener);  // slot 0x7c
		gD3DInterface->SetTextureWrap(0, false);
		gD3DInterface->SetTexture(0, nullptr);
		return;
	}
	// The point is the place of a transform scaled by theSize (0x68b341, 0x6dd0e2, 0x67411e), plus 0 on each axis
	Vec3 aProjected = aCamera.ProjectWorld(Vec3{theX, theY, theZ});  // 0x6dd19b
	float aScreenX = (float)((double)(float)g->mDestImage->mWidth * (double)aProjected.x);
	float aScreenY = (float)((double)(float)g->mDestImage->mHeight * (double)aProjected.y);
	float aDepth = aProjected.z;
	float aScale = (float)(0.0015151499537751079 / (double)aDepth / (double)theImage->mWidth * (double)theSize * 80.0);
	if ((double)aDepth < 9.999999747378752e-05)
		return;
	g->SetDrawMode(theDrawMode);
	g->SetColor(UnpackColor(theColor));
	g->SetColorizeImages(true);
	Transform aTransform;
	aTransform.Scale(aScale, aScale);
	g->DrawImageTransformF(theImage, aTransform, aScreenX, aScreenY);  // 0x422050
}

// 0x694387: theValue to the power thePower, by squaring, as a float
float PowInt(float theValue, int thePower)
{
	unsigned int aPower = (unsigned int)thePower;
	if (thePower < 0)
		aPower = (unsigned int)-thePower;
	float aResult = 1.0f;
	while (true) {
		if ((aPower & 1) != 0)
			aResult = aResult * theValue;
		aPower = aPower >> 1;
		if (aPower == 0)
			break;
		theValue = theValue * theValue;
	}
	if (thePower < 0)
		aResult = 1.0f / aResult;
	return aResult;
}

}  // namespace

// 0x6e5337: the planet is a TwistButton on IMAGE_CHALLENGE_BUTTON whose id is its category's number. The menu's
// constructor gives it its pictures, color and name (0x6e5c3e), AddedToManager its place in the 3D world (0x6dfc49)
ChallengeButton::ChallengeButton(int theCategory, ChallengeMenu* theMenu)
	: TwistButton(theCategory, IMAGE_CHALLENGE_BUTTON, theMenu)
{
	mMenu = theMenu;
	mCategory = theCategory;
	mClip = false;
	// The idle curve (the one curve the constructor sets; Update steps it, nothing here reads it)
	mIdle.SetCurve(GTS(0x7e0b6c));
}

ChallengeButton::~ChallengeButton()
{
	delete mTip;
}

void ChallengeButton::MakeTip()
{
	Font* aFont = FONT_PLANET_TEXT;
	SetFontLayerColor(aFont, GTS(0x7e029c), Color(0, 0, 0, 0));
	SetFontLayerColor(aFont, GTS(0x80dfb4), Color::White);
	SetFontLayerColor(aFont, GTS(0x7dd6b0), Color(0, 0, 0, 255));
	SexyString aText = GT(0x7e0258);
	aText.append(StrFormat(_S("%d"), mProgress));  // 0x45dfb0
	aText += GT(0x7e0250);
	// without Direct3D in FONT_SCORE, 500 art pixels wide, centred 250 above the button's corner
	Font* aTipFont = gApp->Is3DAccelerated() ? aFont : FONT_SCORE;
	int aWidth = Ftol((double)gApp->mScale * 500.0);
	int aY = Ftol((double)gApp->mScale * -250.0);
	Rect aRect(0, aY, aWidth, 100);
	OrbitText* aTip = new OrbitText(aTipFont, aText, aRect);
	aTip->mCount = 2;
	aTip->mSpeed = (float)((double)aTip->mSpeed * 0.15000000596046448);
	// port: the original leaves an earlier tip; the port frees it
	delete mTip;
	mTip = aTip;
}

// 0x6e3b0e
void ChallengeButton::Update()
{
	if (mUnknown51D)
		return;
	if (mTip != nullptr)
		mTip->Update();
	if (!mIdle.HasBeenTriggered())
		mIdle.IncInVal();
	// The name's glow rises while the mouse is over a planet that is all the way in, and falls otherwise
	if (!mIsOver || mFade != 1.0f) {
		double aGlow = (double)mGlow - 0.004999999888241291;
		mGlow = (float)(aGlow >= 0.0 ? aGlow : 0.0);
	} else {
		double aGlow = (double)mGlow + 0.019999999552965164;
		mGlow = (float)(aGlow <= 0.5 ? aGlow : 0.5);
	}
	if (mDim.CheckInThreshold(0.8999999761581421))
		gApp->PlaySample(SOUND_VOICE_NEWCHALLENGEUNLOCKED);
	mDim.IncInVal();
	mBarFill.IncInVal();
	mUnknown398.IncInVal();
	if (mUnknown1D8.mIncRate != 0.0 && !mUnknown1D8.HasBeenTriggered() && !mUnknown1D8.IncInVal() && mUnknown5CD)
		gApp->mMainMenu->mChallengeMenu->UpdateProgress(false, false);
	mEclipseFade.IncInVal();
	mTextFade.IncInVal();
	mUnknown248.IncInVal();
	mUnknown2B8.IncInVal();
	int aPulses = mUnknown52C;
	if (0 < aPulses || 0.0f < mUnknown530) {
		double aLeft = (double)mUnknown530 - 0.03999999910593033;
		mUnknown530 = (float)(aLeft >= 0.0 ? aLeft : 0.0);
		if (mUnknown530 <= 0.0f && 0 < aPulses) {
			mUnknown530 = 1.0f;
			mUnknown52C = aPulses - 1;
		}
	}
	if (mUnknown4E8 < 1.0f) {
		double aRise = (double)mUnknown4E8 + 0.004999999888241291;
		mUnknown4E8 = (float)(aRise <= 1.0 ? aRise : 1.0);
		int aWidth = mButtonImage->GetCelWidth();
		int aHeight = mButtonImage->GetCelHeight();
		int aX = mX;
		int aY = mY;
		// Sparks about the planet while it comes in
		int aCount = Ftol((1.0 - (double)mUnknown4E8) * 3.0);
		while (0 < aCount) {
			float anAngle = RandSigned();
			Effect* anEffect = mMenu->mFXManager1B8->AllocEffect(0);
			anEffect->mImage = IMAGE_GEM_FRUIT_SPARK;
			anEffect->mIsAdditive = true;
			float aRadius = std::fabs(RandSigned());
			float aCos = std::cos((float)((double)anAngle * 3.1415927410125732));
			anEffect->mX = (float)((double)aCos * (double)(aWidth / 2) * (double)aRadius + (double)(aX + aWidth / 2));
			float aSin = std::sin((float)((double)anAngle * 3.1415927410125732));
			anEffect->mFlags |= Effect::FLAG_SCALE_BOUNCE;
			anEffect->mY = (float)((double)aSin * (double)(aHeight / 2) * (double)aRadius + (double)(aY + aHeight / 2));
			anEffect->mDY = 0;
			anEffect->mDX = 0;
			anEffect->mScale = 0.009999999776482582f;
			anEffect->mDScale = 0.025f;
			anEffect->mAngle = 0.009999999776482582f;
			anEffect->mGravity = 0;
			anEffect->mAlpha = 1.0f;
			anEffect->mDAlpha = 0;
			anEffect->mMaxScale = 1.0f;
			mMenu->mFXManager1B8->AddEffect(anEffect);
			aCount--;
		}
	}
	// The bar and the name come in while the planet shows its text, and go out when it does not
	if (!mUnknown5C4) {
		double aLeft = (double)mFade - 0.05000000074505806;
		mFade = (float)(aLeft >= 0.0 ? aLeft : 0.0);
	} else {
		double aMore = (double)mFade + 0.007499999832361937;
		mFade = (float)(1.0 < aMore ? 1.0 : aMore);
	}
	if (mEclipse) {
		// The eclipse title's flares (32 of them, each 0 or on its way from 0 to 1) run on, and one more starts every ten
		// updates
		for (int i = 0; i < 32; i++) {
			if (0.0f < mEclipseFlares[i]) {
				float aFlare = (float)((double)mEclipseFlares[i] + 0.009999999776482582);
				mEclipseFlares[i] = aFlare;
				if (1.0f <= aFlare)
					mEclipseFlares[i] = 0.0f;
			}
		}
		if (mUpdateCnt % 10 == 0) {
			int aFlare = Rand() % 32;
			if (mEclipseFlares[aFlare] == 0.0f)
				mEclipseFlares[aFlare] = 0.009999999776482582f;
		}
	}
	TwistButton::Update();
}

// 0x6dd224: the other mouse buttons (a negative click count) do nothing
void ChallengeButton::MouseDown(int x, int y, int theClickCount)
{
	if (-1 < theClickCount)
		TwistButton::MouseDown(x, y, theClickCount);
}

// 0x6dd237: a circle about the middle of the picture, whatever the planet's size on the screen
bool ChallengeButton::IsPointVisible(int x, int y)
{
	float aDX = (float)(x - mWidth / 2);
	float aDY = (float)(y - mHeight / 2);
	float aDistance = std::sqrt(aDX * aDX + aDY * aDY);
	int aRadius = Ftol((double)gApp->mScale * 80.0);
	return (double)aDistance < (double)aRadius;
}

// 0x6dd1e9: panned by where the planet is, and pitched by its number
void ChallengeButton::PlayMouseOverSound()
{
	gApp->PlaySoundAt(SOUND_MOUSEOVER2, mX, (float)((double)mId * 0.5), 0.0f);
}

// 0x6dde98. The planet's place in the 3D world goes through the camera to the screen (the button takes its place there,
// so the clicks land on the planet). With Direct3D (gD3DInterface) the planet is a sphere mesh (0x6e4fad's, the size by
// how big it looks) turned by the update count and lit by a point light at the sun, with the cloud layer as a second
// sphere a little larger, drawn first; without it a picture of the planet is made for the size it has. The ring, name,
// moon meter and glow are drawn about it.
void ChallengeButton::Draw(Graphics* g)
{
	mClip = false;
	PerspectiveCamera aCamera(gAspect45, gAspect, 1.0f, 10000.0f);  // 0x67417d
	aCamera.mTransform = gMenuCamera;                              // 0x673c10
	WorldGraphicsListener aListener(&aCamera, false);  // 0x4bd5d0
	aListener.SetCulling(true);                        // 0x4bd6c0
	aListener.SetDepth(true, true);                    // 0x4bd680
	if (gD3DInterface != nullptr) {
		gD3DInterface->SetTextureWrap(0, true);    // slot 0xcc
		gD3DInterface->SetTexture(0, mLayers[0]);  // slot 0xd4
	}
	// The planet's place: 0.75 large, at mWorldPos, turned about y by 0.2 degrees for each update and about x by mTilt
	Transform3D aPlanet;  // 0x4bd4c0
	aPlanet.mScale.x = 0.75f * aPlanet.mScale.x;  // 0x68b341
	aPlanet.mScale.y = 0.75f * aPlanet.mScale.y;
	aPlanet.mScale.z = 0.75f * aPlanet.mScale.z;
	float aSpin = (float)((double)mUpdateCnt * 0.20000000298023224);
	aPlanet.mPos.x = mWorldPos[0] + aPlanet.mPos.x;  // 0x6dd0e2
	aPlanet.mPos.y = mWorldPos[1] + aPlanet.mPos.y;
	aPlanet.mPos.z = mWorldPos[2] + aPlanet.mPos.z;
	aPlanet.mRot.RotateZ((float)(0.0 * 3.1415927410125732 / 180.0));  // 0x673fd0
	aPlanet.mRot.RotateY((float)((double)aSpin * 3.1415927410125732 / 180.0));  // 0x68fa0b
	aPlanet.mRot.RotateX((float)((double)mTilt * 3.1415927410125732 / 180.0));  // 0x680589
	aListener.mWorld = aPlanet;  // 0x673c10
	aListener.SetLighting(true);  // 0x4bd6e0
	if (gD3DInterface != nullptr) {
		gD3DInterface->SetMaterial(Color::White, Color::White, Color::White, Color::Black, 0.0f);  // slot 0xf0
		// The light: the sun's glow colour and the menu's dark blend (by the planet's dimming), both dimmer while the menu
		// is not showing this planet, and brighter by the eclipse's curve (menu +0x640)
		D3DLightColors aColors;  // 0x6802ad: white, black, black, 1
		aColors.mDiffuse = Color::White;
		aColors.mSpecular = Color::Black;
		aColors.mAmbient = Color::Black;
		aColors.mIntensity = 1.0f;
		float aDim = (float)mDim.GetOutVal();
		Color aAmbient = BlendColors(gUnknown85C50C, Color(0x20, 0x20, 0x20), aDim);   // 0x7570c1
		aDim = (float)mDim.GetOutVal();
		Color aDiffuse = BlendColors(gSunGlowColor, Color(0x20, 0x20, 0x20), aDim);
		float aBrightness = gUnknown85C508;
		int aState = mMenu->mState;
		if ((aState == 3 || aState == 5 || aState == 4) && mMenu->mButtons[mMenu->mCategory] == this)
			aBrightness = 1.0f;
		aAmbient.mRed = Ftol((double)aBrightness * (double)aAmbient.mRed);
		aAmbient.mGreen = Ftol((double)aBrightness * (double)aAmbient.mGreen);
		aAmbient.mBlue = Ftol((double)aBrightness * (double)aAmbient.mBlue);
		aDiffuse.mRed = Ftol((double)aBrightness * (double)aDiffuse.mRed);
		aDiffuse.mGreen = Ftol((double)aBrightness * (double)aDiffuse.mGreen);
		aDiffuse.mBlue = Ftol((double)aBrightness * (double)aDiffuse.mBlue);
		float anEclipse = (float)mMenu->mUnknown640.GetOutVal();
		if (0.0f < anEclipse) {
			aColors.mIntensity = (float)((double)anEclipse * 5.0 + (double)aColors.mIntensity);
			aAmbient.mRed = Ftol((double)aAmbient.mRed / (double)aColors.mIntensity);
			aAmbient.mGreen = Ftol((double)aAmbient.mGreen / (double)aColors.mIntensity);
			aAmbient.mBlue = Ftol((double)aAmbient.mBlue / (double)aColors.mIntensity);
		}
		aColors.mDiffuse = aDiffuse;
		aColors.mAmbient = aAmbient;
		gD3DInterface->SetPointLight(0, Vec3{0.0f, -12.0f, 0.0f}, aColors, 1000.0f, Vec3{0.15f, 0.25f, 0.0f});  // slot 0xe8
		gD3DInterface->LightEnable(0, true);                                                                    // slot 0xe4
		int aSunAlpha = Ftol(gSunCurve.GetOutVal() * 255.0);
		// The sphere by how big the planet looks: the length from its pole to its middle on the screen (the first vertex of the
		// smallest sphere is its pole), over 0.15 the biggest, over 0.06 the middle one
		std::vector<SphereVertex>* aSphere = &gSphereSmall;
		Vec3 aPole{(*aSphere)[0].x, (*aSphere)[0].y, (*aSphere)[0].z};
		Vec3 aPoleOnScreen = aCamera.ProjectWorld(TransformPoint(aPlanet, aPole));  // 0x68b369, 0x6dd19b
		Vec3 aMiddleOnScreen = aCamera.ProjectWorld(aPlanet.mPos);
		Vec3 aSpan{aMiddleOnScreen.x - aPoleOnScreen.x, aMiddleOnScreen.y - aPoleOnScreen.y,
			aMiddleOnScreen.z - aPoleOnScreen.z};  // 0x673bb6
		float aSpanLength = (float)std::sqrt((double)(float)((double)aSpan.x * (double)aSpan.x + (double)aSpan.y * (double)aSpan.y +
			(double)aSpan.z * (double)aSpan.z));  // 0x4c0800
		if (0.15000000596046448 < (double)aSpanLength)
			aSphere = &gSphereBig;
		else if (0.05999999865889549 < (double)aSpanLength)
			aSphere = &gSphereMiddle;
		int aTriangles = (int)aSphere->size() / 3;
		SetSphereAlpha(*aSphere, aSunAlpha);
		PrimitiveListener aPrimitiveListener(&aListener);
		gD3DInterface->DrawPrimitiveEx(0x152, GPUPT_TRIANGLELIST, aSphere->data(), aTriangles, Color(255, 255, 255, 255), 0,
			0.0f, 0.0f, true, &aPrimitiveListener);  // slot 0x7c
		if (mOpen && mLayers.size() > 1) {
			// The clouds: a second sphere a little larger that turns on its own (35 degrees an update by the planet's number,
			// less half the planet's turn), or, for an additive category, blended added
			int aCloudMode = 0;
			if (!mAdditive)
				aPlanet.mRot.RotateY((float)(((double)(float)((double)mId * 35.0 - (double)aSpin * 0.5)) * 3.1415927410125732 /
					180.0));
			else
				aCloudMode = 1;
			int aCloudAlpha = Ftol(gSunCurve.GetOutVal() * (double)aBrightness * 255.0);
			SetSphereAlpha(*aSphere, aCloudAlpha);
			aPlanet.mScale.x = 1.024999976158142f * aPlanet.mScale.x;
			aPlanet.mScale.y = 1.024999976158142f * aPlanet.mScale.y;
			aPlanet.mScale.z = 1.024999976158142f * aPlanet.mScale.z;
			aListener.mWorld = aPlanet;
			aListener.SetDepth(true, false);
			gD3DInterface->SetTexture(0, mLayers[1]);
			gD3DInterface->DrawPrimitiveEx(0x152, GPUPT_TRIANGLELIST, aSphere->data(), aTriangles, Color(255, 255, 255, 255),
				aCloudMode, 0.0f, 0.0f, true, &aPrimitiveListener);
		}
		gD3DInterface->SetTexture(0, nullptr);
		gD3DInterface->SetTextureWrap(0, false);
	}
	g->SetColorizeImages(true);
	g->SetColor(Color::White);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	// Where the planet is on the screen, and how big it is there (0.00151515 over its depth)
	Vec3 aWorld{mWorldPos[0], mWorldPos[1], mWorldPos[2]};
	Vec3 aProjected = aCamera.ProjectWorld(aWorld);
	float aScreenX = (float)((double)(float)g->mDestImage->mWidth * (double)aProjected.x);
	float aScreenY = (float)((double)(float)g->mDestImage->mHeight * (double)aProjected.y);
	mScreenPos[0] = aScreenX;
	mScreenPos[1] = aScreenY;
	mScreenPos[2] = aProjected.z;
	if (0.0f > mScreenPos[2])
		return;
	float aScale = (float)(0.0015151499537751079 / (double)mScreenPos[2]);
	mX = Ftol((double)aScreenX - (double)(mWidth / 2));
	mY = Ftol((double)aScreenY - (double)(mHeight / 2));
	if (gD3DInterface != nullptr) {
		// 0x6a6b5f: the clip takes in a button's size more on every side (the planet is drawn by the screen's own
		// coordinates, and the matrix may take it out of the clip)
		g->mClipRect.mX -= mWidth;
		g->mClipRect.mWidth += mWidth * 2;
		g->mClipRect.mY -= mHeight;
		g->mClipRect.mHeight += mHeight * 2;
	}
	// From here on in the screen's own coordinates
	float aTransX = g->mTransX;
	float aTransY = g->mTransY;
	g->Translate(Ftol(-(double)aTransX), Ftol(-(double)aTransY));

	// The software planet is drawn without Direct3D only (gD3DInterface; the 3D mode has no picture to draw in)
	if (gD3DInterface == nullptr) {
		// The planet's picture, in the menu's 512 by 512 image: for each pixel of the size it has, IMAGE_PLANET says how
		// bright it is (red, the less the brighter) and where to look in the planet's layer (blue is the column, green
		// over two the row), and the layer is read by two columns, blended by the part after the point
		static_assert(sizeof(ulong) == 4, "the planet pictures are 32 bits a pixel");
		MemoryImage* aBuffer = gApp->mMainMenu->mChallengeMenu->mPlanetImage;
		double aSize = (double)aScale * 76.0;
		int aBufferWidth = aBuffer->mWidth;
		int aBufferHeight = aBuffer->mHeight;
		int aWidth = Ftol((double)aBufferWidth <= aSize ? (double)aBufferWidth : aSize);
		int aHeight = Ftol((double)aBufferHeight <= aSize ? (double)aBufferHeight : aSize);
		if (aWidth <= aBufferWidth) {
			const uint32_t* aPlanetBits = reinterpret_cast<const uint32_t*>(static_cast<MemoryImage*>(IMAGE_PLANET)->GetBits());
			const uint32_t* aLayerBits = reinterpret_cast<const uint32_t*>(static_cast<MemoryImage*>(mLayers[0])->GetBits());
			uint32_t* aBufferBits = reinterpret_cast<uint32_t*>(aBuffer->GetBits());
			float aStepX = (float)(384.0 / (double)aWidth);
			float aStepY = (float)(384.0 / (double)aHeight);
			int aBrightness = Ftol((1.0 - mDim.GetOutVal()) * 200.0 + 55.0);
			int aDestX = Ftol((double)aScreenX - (double)(aWidth / 2));
			int aDestY = Ftol((double)aScreenY - (double)(aHeight / 2));
			Rect aDest(aDestX, aDestY, aWidth, aHeight);
			if (aDest.Intersects(g->mClipRect)) {
				// the clouds drift with the size: 0.4 of the planet's scale, in columns
				double aShift = (double)aScale * 0.4000000059604645;
				float aRowAt = 0;
				for (int aRow = 0; aRow < aHeight; aRow++) {
					uint32_t* aOut = aBufferBits + (size_t)aBufferWidth * (size_t)aRow;
					const uint32_t* aPlanetRow = aPlanetBits + Ftol((double)aRowAt) * 384;
					float aColumnAt = 0;
					for (int aColumn = 0; aColumn < aWidth; aColumn++) {
						uint32_t aPlanet = aPlanetRow[Ftol((double)aColumnAt)];
						uint32_t aLayerRow = (((aPlanet >> 8 & 0xff) / 2) << 8);
						uint32_t aLight = ((0xff - (aPlanet >> 16 & 0xff)) * (uint32_t)aBrightness) >> 8;
						float aBlue = (float)((double)(aPlanet & 0xff) / 1.5);
						float aLookup = (float)((double)aBlue + aShift);
						int aFloor = Ftol((double)aLookup);
						uint32_t aFraction = (uint32_t)Ftol(((double)aLookup - (double)aFloor) * 255.0);
						uint32_t aFirst = aLayerBits[(aFloor & 0xff) + aLayerRow];
						uint32_t aSecond = aLayerBits[((aFloor + 1) & 0xff) + aLayerRow];
						uint32_t aBlended = ((aFirst & 0xff0000) * (0x100 - aFraction) + (aSecond & 0xff0000) * aFraction) & 0xff0000ff;
						aBlended |= ((aFirst & 0xff00) * (0x100 - aFraction) + (aSecond & 0xff00) * aFraction) & 0xff0000;
						aBlended |= (aFirst & 0xff) * (0x100 - aFraction) + (aSecond & 0xff) * aFraction;
						aBlended >>= 8;
						uint32_t aLit = ((aBlended & 0xff0000) * aLight) & 0xff0000ff;
						aLit |= ((aBlended & 0xff00) * aLight) & 0xff0000;
						aLit |= ((aBlended & 0xff) * aLight) & 0xff00;
						*aOut++ = (aLit >> 8) | (aPlanet & 0xff000000);
						aColumnAt = (float)((double)aColumnAt + (double)aStepX);
					}
					aRowAt = (float)((double)aRowAt + (double)aStepY);
				}
				aBuffer->BitsChanged();
				g->SetColor(Color(255, 255, 255));
				g->DrawImage(aBuffer, aDestX, aDestY, Rect(0, 0, aWidth, aHeight));
			}
		}
	}

	// The ring, and (below) the name and moon meter, about the planet and at half its scale
	float aHalfScale = (float)((double)aScale * 0.5);
	if (gD3DInterface == nullptr) {
		g->PushState();
		g->Translate(Ftol((double)aScreenX), Ftol((double)aScreenY));
		g->SetScale(aHalfScale, aHalfScale, 0, 0);
	} else {
		// the device takes the scale and the place (0x44f860, 0x44f990, 0x44f8a0), the place not rounded
		SexyTransform2D aPlanetMatrix;
		aPlanetMatrix.Scale(aHalfScale, aHalfScale);
		aPlanetMatrix.Translate(aScreenX, aScreenY);
		gD3DInterface->PushTransform(aPlanetMatrix, true);  // slot 0x14
	}
	int aNameY = Ftol((double)gApp->mScale * -5.0);
	Color aRingColor = Color::White;
	float aRingScale;
	double aOneMinusScale = 1.0 - (double)aScale;
	double aRingAlpha;
	if (!mOpen) {
		aRingColor = Color(0, 0, 0);
		aRingScale = (float)((double)mNameScale * 1.4500000476837158 * 2.0);
		aRingAlpha = 255.0;
	} else {
		aRingScale = (float)((double)mNameScale * 1.5 * 2.0);
		aRingAlpha = 128.0;
	}
	double aRingFade = 1.0 + aOneMinusScale;
	double aRingFadeAbove = aRingFade < 0.0 ? 0.0 : aRingFade;
	if (aRingFadeAbove * aRingAlpha <= aRingAlpha)
		aRingAlpha = aRingAlpha * aRingFadeAbove;
	aRingColor.mAlpha = Ftol(aRingAlpha);
	g->SetColor(aRingColor);
	if (gD3DInterface == nullptr) {
		g->PushState();
		float aRingDrawScale = (float)((double)aRingScale * (double)aScale * 0.25);
		g->SetScale(aRingDrawScale, aRingDrawScale, 0, 0);
		DrawImageScaledCentered(g, IMAGE_CHALLENGEUI_PLANETRING, 0, 0, ALIGN_CENTER, 1.0f, 1.0f);
		g->PopState();
	} else {
		// With Direct3D the ring is stretched a little more across the way to the middle of the screen (by the planet's
		// distance from it over the planet's width) and turned to point to it
		float aDX = (float)(gApp->mWidth / 2) - aScreenX;
		float aDY = (float)(gApp->mHeight / 2) - aScreenY;
		float anAngle = (float)std::atan2((double)aDY, (double)aDX);  // 0x4c1b70
		float aDistance = (float)std::sqrt((double)(aDX * aDX + aDY * aDY));  // 0x4c0650
		float aStretch = aDistance / (float)mWidth;
		Transform aRingTransform;  // 0x44f9d0
		aRingTransform.Translate(-0.30000001192092896f, 0.30000001192092896f);
		float aMore = (float)((double)aStretch * 0.03500000014901161);
		aRingTransform.Scale(aMore + aRingScale, aRingScale - aMore);
		aRingTransform.RotateRad(-anAngle);
		aRingTransform.Translate(0.0f, 0.0f);
		g->DrawImageTransform(IMAGE_CHALLENGEUI_PLANETRING, aRingTransform, 0.0f, 0.0f);  // 0x422000
	}
	if (!mUnknown5C4) {
		if (gD3DInterface == nullptr)
			g->PopState();
		else
			gD3DInterface->PopTransform();  // slot 0x18
		g->Translate(Ftol((double)aTransX), Ftol((double)aTransY));
		return;
	}

	// The name and the moon meter (a bar of cells, one for each challenge of the category) fade in with mFade
	float aTextFade = (float)((double)mFade * (1.0 - mDim.GetOutVal()));
	if (mUnknown5C5) {
		double aShrink = aOneMinusScale * 0.550000011920929 + 1.0;
		if (aShrink < 0.0)
			aShrink = 0.0;
		aTextFade = (float)(aShrink * (double)aTextFade);
	}
	float aBarFade = (float)(mTextFade.GetOutVal() * (double)aTextFade);
	g->SetColor(Color(255, 255, 255, Ftol((double)aTextFade * 255.0)));
	if (mNameImage != nullptr) {
		float aNameScale = mNameScale + mNameScale;
		DrawImageScaledCentered(g, mNameImage, 0, (float)aNameY, aNameScale, aNameScale);
	}
	double aBarAlpha = (double)aBarFade * 255.0;
	int aBarAlphaInt = Ftol((double)aBarFade * 255.0);
	g->SetColor(Color(255, 255, 255, aBarAlphaInt));
	Image* aFillBar = IMAGE_MOONMETER_FILLBAR;
	Image* anEmptyBar = IMAGE_MOONMETER_EMPTYBAR;
	Image* aGlowNumbers = IMAGE_MOONMETER_GLOWNUMBERS;
	Image* aGreyNumbers = IMAGE_MOONMETER_GREYNUMBERS;
	Image* aBarGlow = IMAGE_MOONMETER_BARGLOW;
	int aBarY = Ftol((double)gApp->mScale * 160.0);
	int aBarX = Ftol((double)gApp->mScale * 0.0);
	int aCellCount = aGreyNumbers->mNumCols * aGreyNumbers->mNumRows;  // 0x423770
	Color aFull(mCategoryColor.mRed, mCategoryColor.mGreen, mCategoryColor.mBlue, aBarAlphaInt);
	Color aDark(Ftol((double)mCategoryColor.mRed * 0.5), Ftol((double)mCategoryColor.mGreen * 0.5),
		Ftol((double)mCategoryColor.mBlue * 0.5), aBarAlphaInt);
	float aBarLeft = (float)((double)aBarX - (double)(aFillBar->mWidth / 2));
	int aCellWidth = aFillBar->mWidth / aCellCount;
	g->SetColor(aDark);
	g->DrawImage(anEmptyBar, Ftol((double)aBarX - (double)(anEmptyBar->mWidth / 2)),
		Ftol((double)aBarY - (double)(anEmptyBar->mHeight / 2)));
	g->SetColor(aFull);
	// The bar's fill: the done cells and the part of the next that mBarFill has got to
	double aFillWidth = (double)aFillBar->mWidth;
	if ((double)(float)aCellWidth * ((double)(mStage - 1) + mBarFill.GetOutVal()) <= aFillWidth)
		aFillWidth = ((double)(mStage - 1) + mBarFill.GetOutVal()) * (double)(float)aCellWidth;
	int aFillPixels = Ftol(aFillWidth);
	if (aFillPixels != 0) {
		g->DrawImage(aFillBar, Ftol((double)aBarLeft), Ftol((double)aBarY - (double)(aFillBar->mHeight / 2)),
			Rect(0, 0, aFillPixels, aFillBar->mHeight));
	}
	float aCellLeft = aBarLeft;
	Rect aSavedClip = g->mClipRect;
	bool aTrial = !gApp->mRegistered;
	bool aLockedCategory = aTrial && mId == 3;
	double aHalfCell = (double)((float)aCellWidth * 0.5);
	for (int aCell = 0; aCell < aCellCount; aCell++) {
		Rect aSource(0, 0, 0, 0);
		int aYShift = 0;
		Image* aCellImage;
		if (aCell != mStage - 1 || aLockedCategory || (aTrial && 3 < mStage)) {
			if (aCell < mStage - 1 && !aLockedCategory) {
				// a cell done
				aCellImage = aGreyNumbers;
				aSource = aGreyNumbers->GetCelRect(aCell);
				g->SetColor(aFull);
			} else {
				// a cell still to come: the lock (red in a copy that is not registered, past the third challenge)
				if (aTrial && (2 < aCell || mId == 3))
					g->SetColor(Color(255, 0x40, 0x40, Ftol((double)aBarFade * 220.0)));
				else
					g->SetColor(Color(255, 255, 255, aBarAlphaInt));
				aCellImage = IMAGE_MOONMETER_LOCK;
				aSource = Rect(0, 0, IMAGE_MOONMETER_LOCK->mWidth, IMAGE_MOONMETER_LOCK->mHeight);
				if (aCell == mStage) {
					// the lock the new stage opens wipes away from the top
					aSource.mY = Ftol(mUnknown1D8.GetOutVal() * (double)aSource.mHeight + (double)aSource.mY);
					aSource.mHeight = aSource.mHeight - aSource.mY;
					aYShift = (aSource.mY + (aSource.mY >> 31 & 3)) >> 2;
				}
			}
		} else {
			// the cell of the challenge the planet starts: its glowing number
			aCellImage = aGlowNumbers;
			aSource = aGlowNumbers->GetCelRect(aCell);
			g->SetColor(Color(255, 255, 255, Ftol((1.0 - mUnknown248.GetOutVal()) * aBarAlpha)));
		}
		int aDrawY = Ftol(((double)aBarY - (double)(aSource.mHeight / 2)) + (double)aYShift + 0.0);
		int aDrawX = Ftol(((double)aCellLeft + aHalfCell) - (double)(aSource.mWidth / 2));
		g->DrawImage(aCellImage, aDrawX, aDrawY, aSource);
		aCellLeft = (float)((double)(float)aCellWidth + (double)aCellLeft);
	}
	if (mUnknown1D8.GetOutVal() != 0.0) {
		// the number just done fades to grey, and the next glows
		int anAlpha = Ftol((double)aBarAlphaInt * (double)aBarFade * mUnknown1D8.GetOutVal());
		g->SetColor(Color(aFull.mRed, aFull.mGreen, aFull.mBlue, anAlpha));
		Rect aSource = aGreyNumbers->GetCelRect(mStage - 1);
		int aDrawY = Ftol(((double)aBarY - (double)(aSource.mHeight / 2)) + 0.0);
		int aDrawX = Ftol(((double)(mStage - 1) * (double)(float)aCellWidth + (double)aBarLeft + aHalfCell) -
			(double)(aSource.mWidth / 2));
		g->DrawImage(aGreyNumbers, aDrawX, aDrawY, aSource);
		int aNext = mStage;
		int aCount = gApp->mChallengeInfo->GetChallengeCount(mCategory);
		if (aNext < aCount && (gApp->mRegistered || aNext < 3)) {
			g->SetColor(Color(255, 255, 255, Ftol(mUnknown1D8.GetOutVal() * aBarAlpha)));
			int aCel = aGlowNumbers->mNumCols - 1;
			if (mStage < aGlowNumbers->mNumCols - 1)
				aCel = mStage;
			aSource = aGlowNumbers->GetCelRect(aCel);
			aDrawY = Ftol(((double)aBarY - (double)(aSource.mHeight / 2)) + 0.0);
			aDrawX = Ftol(((double)mStage * (double)(float)aCellWidth + (double)aBarLeft + aHalfCell) -
				(double)(aSource.mWidth / 2));
			g->DrawImage(aGlowNumbers, aDrawX, aDrawY, aSource);
		}
	}
	// The lock the new stage opens glows, added to the picture
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(Color(255, 255, 255, Ftol(mUnknown2B8.GetOutVal() * aBarAlpha)));
	Rect aLockSource = IMAGE_MOONMETER_LOCK->GetCelRect(0);
	aLockSource.mY = Ftol(mUnknown248.GetOutVal() * (double)aLockSource.mHeight + (double)aLockSource.mY);
	aLockSource.mHeight = aLockSource.mHeight - aLockSource.mY;
	int aLockShift = (aLockSource.mY + (aLockSource.mY >> 31 & 3)) >> 2;
	int aLockY = Ftol(((double)aBarY - (double)(aLockSource.mHeight / 2)) + (double)aLockShift + 0.0);
	int aLockX = Ftol(((double)(float)aCellWidth * 0.5 + (double)mStage * (double)(float)aCellWidth + (double)aBarLeft) -
		(double)(aLockSource.mWidth / 2));
	g->DrawImage(IMAGE_MOONMETER_LOCK, aLockX, aLockY, aLockSource);
	if (mEclipse) {
		g->mClipRect = aSavedClip;
		g->SetColor(Color(255, 255, 255, Ftol(mEclipseFade.GetOutVal() * 255.0 * (double)aTextFade)));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->DrawImage(IMAGE_ECLIPSE_TITLE, Ftol((double)aBarX - (double)(IMAGE_ECLIPSE_TITLE->mWidth / 2)),
			Ftol((double)aBarY - (double)(IMAGE_ECLIPSE_TITLE->mHeight / 2)));
		if (gD3DInterface != nullptr) {
			// The title's 32 flares: for each, a fan of up to 24 triangles round the bar's middle on the glow picture,
			// each a little wider and fainter than the one before, at a 32nd of a turn from the last flare's
			Image* aGlowImage = IMAGE_ECLIPSE_TITLE_GLOW;
			float aStep = (float)(6.2831855f * 0.03125);  // _DAT_0084ec00
			float anAngle = 0.0f;
			for (int aFlare = 0; aFlare < 32; aFlare++) {
				float aNextAngle = anAngle + aStep;
				float aSineArg = (float)((double)mEclipseFlares[aFlare] * 3.1415927410125732);
				float aSine = sinf(aSineArg);
				int aFlareAlpha = Ftol(((double)aSine * 55.0 + 100.0) * mEclipseFade.GetOutVal() * (double)aTextFade);
				int aTriangleCount = 0;
				float aRadius = 1.0f;
				TriVertex aVertices[32][3];
				if (0 < aFlareAlpha) {
					do {
						if (0x17 < aTriangleCount)
							break;
						unsigned long aColor = Color(255, 255, 255, aFlareAlpha).ToInt();  // 0x401860
						aVertices[aTriangleCount][0] = TriVertex((float)aBarX, (float)aBarY, 0.5f, 0.5f, aColor);  // 0x6847ca
						float aSin0 = sinf(anAngle);
						float aCos0 = cosf(anAngle);
						float aSin0b = sinf(anAngle);
						float aCos0b = cosf(anAngle);
						float aV = (float)((double)aSin0 * 0.5 + 0.5);
						float aU = (float)((double)aCos0 * 0.5 + 0.5);
						float aCelHeight = (float)(aGlowImage->mHeight / aGlowImage->mNumRows);  // 0x423750
						float aY = (float)((double)aRadius * (double)aSin0b * (double)aCelHeight * 0.5 + (double)aBarY);
						float aCelWidth = (float)(aGlowImage->mWidth / aGlowImage->mNumCols);  // 0x423760
						float aX = (float)((double)aRadius * (double)aCos0b * (double)aCelWidth * 0.5 + (double)aBarX);
						aVertices[aTriangleCount][1] = TriVertex(aX, aY, aU, aV, aColor);
						float aSin1 = sinf(aNextAngle);
						float aCos1 = cosf(aNextAngle);
						float aSin1b = sinf(aNextAngle);
						float aCos1b = cosf(aNextAngle);
						aV = (float)((double)aSin1 * 0.5 + 0.5);
						aU = (float)((double)aCos1 * 0.5 + 0.5);
						aY = (float)((double)aRadius * (double)aSin1b * (double)aCelHeight * 0.5 + (double)aBarY);
						aX = (float)((double)aRadius * (double)aCos1b * (double)aCelWidth * 0.5 + (double)aBarX);
						aVertices[aTriangleCount][2] = TriVertex(aX, aY, aU, aV, aColor);
						aFlareAlpha = Ftol((double)aFlareAlpha - 10.0);
						aTriangleCount++;
						aRadius = (float)((double)aRadius + 0.02500000037252903);
					} while (0 < aFlareAlpha);
				}
				gD3DInterface->DrawTrianglesTex(aGlowImage, aVertices, aTriangleCount, Color::White, 1, 0.0f, 0.0f, true);  // slot 0x74
				anAngle = aNextAngle;
			}
		}
	}
	if (gD3DInterface != nullptr) {
		// the clip taller by 100 above and below
		g->mClipRect.mY -= 100;
		g->mClipRect.mHeight += 200;
	}
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(aFull);
	g->DrawImage(aBarGlow, Ftol((double)aBarX - (double)(aBarGlow->mWidth / 2)),
		Ftol((double)aBarY - (double)(aBarGlow->mHeight / 2)));
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->ClearClipRect();
	if (0.0f < mGlow) {
		// The name glows while the mouse is over the planet: its picture again, a little larger each time, added
		float aGlow = mGlow;
		float aGlowScale = mNameScale + mNameScale;
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		for (; 0.0f < aGlow; aGlow = (float)((double)aGlow - 0.009999999776482582)) {
			g->SetColor(Color(255, 255, 255, Ftol((double)aGlow * 255.0 * 0.10000000149011612 * (double)aTextFade)));
			if (mNameImage != nullptr)
				DrawImageScaledCentered(g, mNameImage, 0, (float)aNameY, aGlowScale, aGlowScale);
			aGlowScale = (float)((double)aGlowScale + 0.03999999910593033);
		}
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColor(Color::White);
	}
	if (mTip != nullptr) {
		mTip->mX = Ftol((double)mWidth * 0.0);
		mTip->mY = Ftol((double)gApp->mScale * 5.0);
		mTip->Draw(g);
	}
	g->Translate(Ftol((double)aTransX), Ftol((double)aTransY));
	if (gD3DInterface == nullptr)
		g->PopState();
	else
		gD3DInterface->PopTransform();  // slot 0x18
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x6e5c3e
ChallengeMenu::ChallengeMenu()
{
	Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mVisible = false;
	mHover = -1;
	mCategory = 0;
	mStageSel = 0;
	mStarted = 0;
	mHoverFade = 1.0f;
	mUnknown9BC = false;
	mDragging = false;
	mCameraStackIndex = 0;
	mStartDelay = 0;
	mUnknown15C = 0;
	// The camera starts at (0, -12, 0) (+0x630) and the camera stack comes from its file (0x6e5a8d)
	mCameraPos[0] = 0;
	mCameraPos[1] = -12.0f;
	mCameraPos[2] = 0;
	LoadCameraStack();
	// The sphere meshes of the 3D draw (0x6e4fad, three sizes: the smallest is 0x85c5f8, the biggest 0x85c5e8), made in both
	// modes
	BuildSphere(gSphereSmall, 0xf, 0x14);
	BuildSphere(gSphereMiddle, 0x14, 0x1c);
	BuildSphere(gSphereBig, 0x20, 0x30);
	// The 512 by 512 picture the software planets are made in (+0x9e0, not freed by the original)
	if (!gApp->Is3DAccelerated()) {
		mPlanetImage = new MemoryImage(gApp);
		mPlanetImage->SetImageMode(true, true);
		mPlanetImage->SetVolatile(true);
		mPlanetImage->Create(0x200, 0x200);
	}
	// The planets, one for each category, with their layer pictures (planet and clouds), title and color
	float aNameScale = 1.0f;
	if ((double)gApp->mWidth / (double)gApp->mHeight < 1.7777777910232544)
		aNameScale = 0.75f;
	ChallengeInfo* aChallengeInfo = gApp->mChallengeInfo;
	for (unsigned int aCategory = 0; aCategory < (unsigned int)aChallengeInfo->mCategories.size(); aCategory++) {
		ChallengeButton* aButton = new ChallengeButton((int)aCategory, this);
		ChallengeCategory* aChallengeCategory = aChallengeInfo->mCategories[aCategory];
		for (unsigned int aLayer = 0; aLayer < (unsigned int)aChallengeCategory->mLayers.size(); aLayer++) {
			SexyString aLayerName = StringToUpper(StringToSexyString("IMAGE_") + aChallengeCategory->mLayers[aLayer]);
			int anId = GetIdByName(aLayerName.c_str());
			aButton->mAdditive = aChallengeCategory->mAdditive;
			if ((int)aCategory < (int)gApp->mProfile->mInts204.size())
				aButton->mProgress = gApp->mProfile->mInts204[aCategory];
			if (anId == RESOURCE_ID_MAX)
				gApp->Popup("Error: Can't find texture '" + aLayerName + "' for challenge menu");
			else
				aButton->mLayers.push_back(GetImageById(anId));
		}
		aButton->Resize(0, 0, aButton->mButtonImage->GetCelWidth(), aButton->mButtonImage->GetCelHeight());
		aButton->mCategory = (int)aCategory;
		SexyString aTitleName = StringToUpper(StringToSexyString("IMAGE_CHALLENGEUI_TITLES_") + aChallengeInfo->GetCategoryId(aCategory));
		int aTitleId = GetIdByName(aTitleName.c_str());
		if (aTitleId == RESOURCE_ID_MAX)
			gApp->Popup("Error: Can't find texture " + aTitleName);
		else
			aButton->mNameImage = GetImageById(aTitleId);
		aButton->mCategoryColor = aChallengeCategory->mColor;
		aButton->mNameScale = aNameScale;
		mButtons.push_back(aButton);
	}
	UpdateProgress(false, false);
	// The two buttons below: back to the play menu and to the main menu (0x754172 makes them)
	mPlayMenuButton = new TwistButton(BUTTON_PLAY_MENU, IMAGE_SIZEABLEBUTTON, this);
	mPlayMenuButton->mLabel = gApp->GetString(GTS(0x7e0cd0), GT(0x7e0cbc));
	mPlayMenuButton->SetFont(FONT_DIALOG_BUTTONS);
	mMainMenuButton = new TwistButton(BUTTON_MAIN_MENU, IMAGE_SIZEABLEBUTTON, this);
	mMainMenuButton->mLabel = gApp->GetString(GTS(0x7e0cb0), GT(0x7e0c9c));
	mMainMenuButton->SetFont(FONT_DIALOG_BUTTONS);
	// Two particle managers (0x6f9f59): +0x1b8 is a widget of the menu, +0x1bc the sun's glow
	mFXManager1B8 = new EffectsManager();
	mFXManager1BC = new EffectsManager();
	TwistButton* aButtons[2] = {mPlayMenuButton, mMainMenuButton};
	for (TwistButton* aButton : aButtons) {
		// as wide as the label needs, with a tenth to spare, but not narrower than the picture
		double aLabelWidth = (double)FONT_DIALOG_BUTTONS->StringWidth(aButton->mLabel) * 1.1000000238418579;
		int aWidth = (double)aButton->mButtonImage->GetCelWidth() >= aLabelWidth ? aButton->mButtonImage->GetCelWidth()
		                                                                           : Ftol(aLabelWidth);
		aButton->Resize(0, 0, aWidth, aButton->mButtonImage->GetCelHeight());
	}
	SetState(0, false);
}

ChallengeMenu::~ChallengeMenu()
{
	for (ChallengeButton* aButton : mButtons) {
		RemoveWidget(aButton);
		delete aButton;
	}
	RemoveWidget(mPlayMenuButton);
	delete mPlayMenuButton;
	RemoveWidget(mMainMenuButton);
	delete mMainMenuButton;
	RemoveWidget(mFXManager1B8);
	delete mFXManager1B8;
	delete mFXManager1BC;
	// port: the original never frees the planet picture
	delete mPlanetImage;
	// 0x6e0c6c
	delete mTitleImage;
	delete mDescImage;
	delete mDescGlowImage;
}

// 0x6dfc49: the planets go round a circle in the 3D world about the camera's start, from a quarter turn less half of
// what the circle leaves open (a tenth) and on, an equal step apart
void ChallengeMenu::AddedToManager(WidgetManager* theWidgetManager)
{
	TwistWidget::AddedToManager(theWidgetManager);
	mMouseVisible = false;
	const double aTwoPi = 6.2831854820251465;  // the float at 0x84ec00
	float aSpan = (float)(aTwoPi * 0.8999999761581421);
	float aStart = (float)(aTwoPi * 0.25 - (aTwoPi - (double)aSpan) * 0.5);
	float aNegSpan = -aSpan;
	float aStep = (float)((double)aNegSpan / (double)((int)mButtons.size() - 1));
	mUnknown188 = (float)((double)mHeight * 0.4000000059604645);
	float anAngle = aStart;
	for (unsigned int i = 0; i < (unsigned int)mButtons.size(); i++) {
		ChallengeButton* aButton = mButtons[i];
		aButton->mUnknown524 = anAngle;
		aButton->mUnknown520 = aStart;
		aButton->mUnknown528 = mUnknown188;
		float aCos = std::cos(anAngle);
		aButton->mWorldPos[0] = (float)((double)aCos * 4.800000190734863 * 1.5 + (double)mCameraPos[0]);
		float aSin = std::sin(anAngle);
		aButton->mWorldPos[1] = (float)((double)aSin * 4.800000190734863 + (double)mCameraPos[1]);
		aButton->mWorldPos[2] = 0;
		AddWidget(aButton);
		aButton->Update();
		anAngle = (float)((double)aStep + (double)anAngle);
	}
	AddWidget(mPlayMenuButton);
	AddWidget(mMainMenuButton);
	// the two buttons wait below the screen (Update brings them up with mButtonSlide)
	mPlayMenuButton->mX = (mWidth - Ftol((double)gApp->mScale * 40.0)) - mPlayMenuButton->mWidth;
	mMainMenuButton->mX = Ftol(40.0 * (double)gApp->mScale);
	mPlayMenuButton->mY = mHeight * 2;
	mMainMenuButton->mY = mPlayMenuButton->mY;
	AddWidget(mFXManager1B8);
	mFXManager1BC->Resize(0, 0, mWidth, mHeight);
}

// 0x6dd2cd
void ChallengeMenu::RemovedFromManager(WidgetManager* theWidgetManager)
{
	TwistWidget::RemovedFromManager(theWidgetManager);
	for (ChallengeButton* aButton : mButtons)
		RemoveWidget(aButton);
	RemoveWidget(mPlayMenuButton);
	RemoveWidget(mMainMenuButton);
	RemoveWidget(mFXManager1B8);
}

// 0x6e30d5
void ChallengeMenu::Show(int theMode, bool theNow)
{
	mUnknown9BC = false;
	mFXManager1BC->Clear();
	mVisible = true;
	UpdateProgress(false, false);
	SetState(theMode, theNow);
	if (!theNow) {
		for (ChallengeButton* aButton : mButtons)
			aButton->mMouseVisible = false;
	}
}

// 0x6dd42e
bool ChallengeMenu::IsOpenCurveRunning() const
{
	return 0.0 != mOpen.mIncRate && mOpen.mInMax != mOpen.mInVal;
}

// 0x6e3142: nothing happens while the open curve runs backwards
void ChallengeMenu::Close(int theState)
{
	if (IsOpenCurveRunning() && mOpen.mIncRate < 0.0)
		return;
	mMouseVisible = false;
	if (theState == 2)
		gApp->mMainMenu->PlayMusic(0);
	mMainMenuButton->mMouseVisible = false;
	mPlayMenuButton->mMouseVisible = false;
	for (ChallengeButton* aButton : mButtons) {
		if (theState == 2)
			aButton->mUnknown5C4 = false;
		aButton->mMouseVisible = false;
	}
	SetState(theState, false);
}

// 0x6e2318: the curves of the camera, the sun, the buttons and the moon for each state. All but the first state's are
// linked to the open curve (mOpen), which is what moves them. A state 1 or 2 menu flies the camera in or out; the camera's
// start for the zoom into a picked planet (mZoomPos) comes from the picked planet.
void ChallengeMenu::SetState(int theState, bool theNow)
{
	switch (theState) {
	case 0:
		// away
		mOpen.SetConstant(0.0);
		mButtonSlide.SetConstant(0.0);
		mStartDelay = 0.0f;
		gSunCurve.SetConstant(0.0);
		break;
	case 1: {
		// shown by the play menu: the camera flies in from the side
		gApp->mMainMenu->PlayMusic(0x130);
		mOpen.SetCurve(GTS(0x7e06a8), nullptr);
		mButtonSlide.SetCurve(GTS(0x7e06a8), &mOpen);
		mStartDelay = 0.0f;
		gSunCurve.SetCurve(GTS(0x7e068c), nullptr);
		mUnknown6B0.SetCurve(GTS(0x7e07dc), nullptr);
		mUnknown640.SetCurve(GTS(0x7e0908), nullptr);
		if (theNow) {
			// at once: one step before the ends
			mOpen.mInVal = mOpen.mInMax - 0.0010000000474974513;
			mUnknown6B0.mInVal = mUnknown6B0.mInMax - 0.0010000000474974513;
			mUnknown640.mInVal = mUnknown640.mInMax - 0.0010000000474974513;
		}
		mCameraRotX.SetCurve(GTS(0x7e066c), &mOpen);
		mCameraRotY.SetCurve(GTS(0x7e0648), &mOpen);
		mCameraRotZ.SetCurve(GTS(0x7e0620), &mOpen);
		mCameraDistance.SetCurve(GTS(0x7e05fc), &mOpen);
		mCameraRight.SetCurve(GTS(0x7e0758), &mOpen);
		mCameraUp.SetCurve(GTS(0x7e0758), &mOpen);
		mCameraShiftX.SetCurve(GTS(0x7e05dc), &mOpen);
		mCameraShiftY.SetCurve(GTS(0x7e05bc), &mOpen);
		mUnknown720.SetCurve(GTS(0x7e06f0), &mOpen);
		mCameraTurn.SetCurve(GTS(0x7e06c4), &mOpen);
		// The turn the camera starts at (the three axes' curves at 0), turned 0.625 of the way to the identity, and
		// its transpose (0x4c1670 with the identity for the other, then 0x6dda5c): mCameraTarget
		Mat3 aRotation;
		aRotation.RotateZ(CurveRadians(mCameraRotZ));
		aRotation.RotateX(CurveRadians(mCameraRotX));
		aRotation.RotateY(CurveRadians(mCameraRotY));
		Mat3 anIdentity;
		mCameraTarget = SlerpMat3(aRotation, anIdentity, 0.625f, false).Transposed();
		break;
	}
	case 2:
		// leaving for a menu: the camera flies out the other way
		mOpen.SetCurve(GTS(0x7e0828), nullptr);
		mButtonSlide.SetCurve(GTS(0x7e089c), &mOpen);
		mStartDelay = 0.0f;
		gSunCurve.SetCurve(GTS(0x7e07fc), &mOpen);
		mUnknown6B0.SetCurve(GTS(0x7e07dc), &mOpen);
		mUnknown640.SetCurve(GTS(0x7e0908), &mOpen);
		mCameraRotX.SetCurve(GTS(0x7e07bc), &mOpen);
		mCameraRotY.SetCurve(GTS(0x7e0798), &mOpen);
		mCameraRotZ.SetCurve(GTS(0x7e0798), &mOpen);
		mCameraDistance.SetCurve(GTS(0x7e0778), &mOpen);
		mCameraRight.SetCurve(GTS(0x7e0758), &mOpen);
		mCameraUp.SetCurve(GTS(0x7e0758), &mOpen);
		mCameraShiftX.SetCurve(GTS(0x7e0738), &mOpen);
		mCameraShiftY.SetCurve(GTS(0x7e0718), &mOpen);
		mUnknown720.SetCurve(GTS(0x7e06f0), &mOpen);
		mCameraTurn.SetCurve(GTS(0x7e06c4), &mOpen);
		break;
	case 3: {
		// leaving for a challenge: the camera flies to the picked planet
		float aWas = (float)mOpen.mInVal;
		mOpen.SetCurve(GTS(0x7e0940), nullptr);
		mButtonSlide.SetCurve(GTS(0x7e089c), &mOpen);
		mStartDelay = 0.0f;
		gSunCurve.SetCurve(GTS(0x7e087c), &mOpen);
		mUnknown6B0.SetCurve(GTS(0x7e0858), &mOpen);
		mUnknown640.SetCurve(GTS(0x7e0908), &mOpen);
		mUnknown720.SetConstant(0.0);
		mCameraTurn.SetConstant(1.0);
		ChallengeButton* aPicked = mButtons[mCategory];
		mZoomPos[0] = aPicked->mWorldPos[0];
		mZoomPos[1] = aPicked->mWorldPos[1];
		mZoomPos[2] = aPicked->mWorldPos[2];
		mZoom.SetCurve(GTS(0x7e0940), &mOpen);
		for (unsigned int i = 0; i < (unsigned int)mButtons.size(); i++)
			mButtons[i]->mUnknown5C5 = i != (unsigned int)mCategory;
		if (mState == 4) {
			// the menu was still coming in: the way out begins where the way in stood
			float aLeft = (float)(1.0 - (double)aWas);
			if (0.00999999977648258f < aLeft)
				mOpen.mInVal = (double)aLeft;
		}
		break;
	}
	case 4: {
		// shown at once (after a game): the camera flies from the picked planet
		gApp->mMainMenu->PlayMusic(0x130);
		mVisible = true;
		mOpen.SetCurve(GTS(0x7e0940), nullptr);
		mButtonSlide.SetCurve(GTS(0x7e0940), &mOpen);
		mStartDelay = 0.0f;
		if (mState != 5) {
			gSunCurve.SetCurve(GTS(0x7e0924), &mOpen);
			mUnknown640.SetCurve(GTS(0x7e0908), nullptr);
		}
		mUnknown6B0.SetCurve(GTS(0x7e08e4), &mOpen);
		mUnknown720.SetConstant(0.0);
		mCameraTurn.SetConstant(1.0);
		ChallengeButton* aPicked = mButtons[mCategory];
		mZoomPos[0] = aPicked->mWorldPos[0];
		mZoomPos[1] = aPicked->mWorldPos[1];
		mZoomPos[2] = aPicked->mWorldPos[2];
		mZoom.SetCurve(GTS(0x7e08c0), &mOpen);
		UpdateProgress(false, false);
		break;
	}
	case 5:
		// only shown: the sun's glow all the way
		gSunCurve.SetConstant(1.0);
		mVisible = true;
		break;
	}
	mState = theState;
	MarkDirty();
}

// 0x6e5a8d: properties/camerastack.txt, two lines of three numbers each for each camera (a blank line between)
void ChallengeMenu::LoadCameraStack()
{
	mCameraStack.clear();
	Buffer aBuffer;
	if (!gApp->ReadBufferFromFile("properties/camerastack.txt", &aBuffer, true))
		return;
	// the lines (the original reads them with fgets in text mode)
	std::vector<std::string> aLines;
	std::string aLine;
	for (unsigned char aChar : aBuffer.mData) {
		if (aChar == '\n') {
			aLines.push_back(aLine);
			aLine.clear();
		} else if (aChar != '\r') {
			aLine += (char)aChar;
		}
	}
	if (!aLine.empty())
		aLines.push_back(aLine);
	for (size_t i = 0; i < aLines.size(); i++) {
		if (Trim(aLines[i]).empty())
			continue;
		CameraStackEntry anEntry = {{0, 0, 0}, {0, 0, 0}};
		std::sscanf(aLines[i].c_str(), "%f,%f,%f\n", &anEntry.mPos[0], &anEntry.mPos[1], &anEntry.mPos[2]);
		i++;
		if (i < aLines.size())
			std::sscanf(aLines[i].c_str(), "%f,%f,%f\n", &anEntry.mRot[0], &anEntry.mRot[1], &anEntry.mRot[2]);
		mCameraStack.push_back(anEntry);
	}
}

// 0x6e0ddb: each planet's stage, whether it is open and its curves, from the player's progress. Animating, a planet
// that moves on or opens starts the curves that show it (a glow, a sound) and the call returns 200 or 250; not
// animating, the curves are put at their end.
int ChallengeMenu::UpdateProgress(bool theAnimate, bool theClearTips)
{
	bool aAdvanced = false;
	bool anOpened = false;
	ChallengeInfo* anInfo = gApp->mChallengeInfo;
	Profile* aProfile = gApp->mProfile;
	for (unsigned int i = 0; i < (unsigned int)mButtons.size(); i++) {
		ChallengeButton* aButton = mButtons[i];
		int aStage = aProfile->GetChallengeStage((int)i);
		aButton->mEclipseFade.SetConstant(0.0);
		aButton->mTextFade.SetConstant(1.0);
		if (theClearTips && aButton->mTip != nullptr) {
			// the tip MakeTip made for a night challenge's record
			gApp->SafeDeleteWidget(aButton->mTip);
			aButton->mTip = nullptr;
		}
		bool aEclipseNow = aButton->mEclipse;
		if (!aEclipseNow) {
			if (aButton->mStage != aStage) {
				int aCount = anInfo->GetChallengeCount(i);
				if (aCount < aStage) {
					// every challenge done: the planet shows its last stage and the eclipse that follows
					aButton->mBarFill.SetCurve(GTS(0x7e0558));
					aButton->mOpen = true;
					aButton->mDim.SetConstant(0.0);
					aButton->mStage = anInfo->GetChallengeCount(i);
					if (theAnimate) {
						aAdvanced = true;
						aButton->mUnknown398.SetCurve(GTS(0x7e0534));
						aButton->mEclipseFade.SetCurve(GTS(0x7e050c));
						aButton->mTextFade.SetCurve(GTS(0x7e04ec));
					} else {
						aEclipseNow = true;
					}
				} else if (!theAnimate || aStage == 1) {
					aButton->mBarFill.SetConstant(0.0);
					aButton->mUnknown1D8.SetConstant(0.0);
					aButton->mUnknown248.SetConstant(0.0);
					aButton->mUnknown2B8.SetConstant(0.0);
					aButton->mStage = aStage;
				} else {
					// the planet moves on a stage: the bar fills, the new number glows and a sound plays
					aAdvanced = true;
					gApp->PlaySample(SOUND_CHALLENGE_UNLOCK);
					aButton->mBarFill.SetCurve(GTS(0x7e0558));
					aButton->mUnknown1D8.SetCurve(GTS(0x7e04c4));
					if (aStage < 4 || gApp->mRegistered) {
						aButton->mUnknown248.SetCurve(GTS(0x7e045c));
						aButton->mUnknown2B8.SetCurve(GTS(0x7e0420));
					} else {
						aButton->mUnknown2B8.SetCurve(GTS(0x7e0488));
					}
				}
			}
		} else {
			aEclipseNow = true;
		}
		if (aEclipseNow) {
			// the eclipse (also reached from the first stage past the last, not animating)
			aButton->mUnknown398.SetConstant(1.0);
			aButton->mEclipseFade.SetConstant(1.0);
			aButton->mTextFade.SetConstant(0.0);
		}
		aButton->mProgress = (int)i < (int)aProfile->mInts204.size() ? aProfile->mInts204[i] : 0;
		aButton->mEclipse = anInfo->GetChallengeCount(i) < aProfile->GetChallengeStage((int)i);
		bool anOpen = aProfile->IsChallengeCategoryOpen((int)i);
		if (!anOpen) {
			aButton->mOpen = false;
			aButton->mUnknown398.SetConstant(0.0);
			aButton->mDim.SetConstant(1.0);
			aButton->mMouseVisible = false;
		} else if (!aButton->mOpen) {
			if (!theAnimate) {
				aButton->mDim.SetConstant(0.0);
				aButton->mOpen = anOpen;
			} else {
				anOpened = true;
				aButton->mDim.SetCurve(GTS(0x7e03f4));
				aButton->mUnknown5C4 = true;
			}
			aButton->mMouseVisible = true;
		}
	}
	if (mHover >= 0)
		ButtonMouseEnter(mHover);
	if (anOpened)
		return 0xFA;
	return aAdvanced ? 200 : 0;
}

// 0x6e3f41
void ChallengeMenu::Update()
{
	TwistWidget::Update();
	gSunCurve.IncInVal();
	mUnknown6B0.IncInVal();
	mUnknown640.IncInVal();
	// 1 - mUnknown6B0, but not below 0.25
	float aFraction = 0.25f;
	if (0.25 <= 1.0 - mUnknown6B0.GetOutVal())
		aFraction = (float)(1.0 - mUnknown6B0.GetOutVal());
	gUnknown85C508 = aFraction;
	if (mHover == -1) {
		mHoverFade = (float)((double)mHoverFade - 0.03999999910593033);
		if (mHoverFade < 0.0f)
			mHoverFade = 0.0f;
		mUnknown9D0 = (float)((double)mUnknown9D0 + 0.009999999776482582);
		if (0.5f < mUnknown9D0) {
			mUnknown9D4 = (float)((double)mUnknown9D4 + 0.019999999552965164);
			if (1.0f < mUnknown9D4)
				mUnknown9D4 = 1.0f;
		}
	} else {
		mHoverFade = (float)((double)mHoverFade + 0.02500000037252903);
		if (1.0f < mHoverFade)
			mHoverFade = 1.0f;
		mUnknown9D0 = 0;
		mUnknown9D4 = 0;
	}
	// The glow colours follow mUnknown928 (0x7570c1 blends two colours, the first's alpha kept)
	mUnknown928.IncInVal();
	float aBlend = (float)mUnknown928.GetOutVal();
	gSunGlowColor = BlendColors(Color(255, 255, 255), Color(255, 128, 255), aBlend);
	aBlend = (float)mUnknown928.GetOutVal();
	gUnknown85C50C = BlendColors(Color(128, 32, 128), Color(0, 0, 128), aBlend);
	mFXManager1BC->Update();
	// The mouse drag changes the camera stack's entry mCameraStackIndex: sideways it turns about y (the right button,
	// about z), up and down about x
	WidgetManager* aWidgetManager = gApp->mWidgetManager;
	int aMouseX = aWidgetManager->mLastMouseX;
	int aMouseY = aWidgetManager->mLastMouseY;
	if ((aWidgetManager->mDownButtons & 1) == 0) {
		mDragging = false;
	} else if (!mDragging) {
		mDragStartX = aMouseX;
		mDragStartY = aMouseY;
		mDragging = true;
	} else {
		int aDeltaX = aMouseX - mLastMouseX;
		int aDeltaY = aMouseY - mLastMouseY;
		// (with no camera stack the original would stop on its bounds check; the file ships with the game)
		if ((size_t)mCameraStackIndex < mCameraStack.size()) {
			CameraStackEntry& anEntry = mCameraStack[mCameraStackIndex];
			if ((aWidgetManager->mDownButtons & 2) == 0) {
				anEntry.mRot[1] = (float)((double)aDeltaX * 0.0020000000949949026 + (double)anEntry.mRot[1]);
				anEntry.mRot[0] = (float)((double)aDeltaY * 0.0020000000949949026 + (double)anEntry.mRot[0]);
			} else {
				anEntry.mRot[2] = (float)((double)aDeltaX * 0.0020000000949949026 + (double)anEntry.mRot[2]);
			}
		}
	}
	mLastMouseX = aMouseX;
	mLastMouseY = aMouseY;
	if (IsOpenCurveRunning()) {
		if (0.0f < mStartDelay) {
			mStartDelay = (float)((double)mStartDelay - 0.009999999776482582);
		} else if (!mOpen.IncInVal()) {
			// the curve has ended
			mUnknown9D4 = 0;
			mHoverFade = 0;
			bool anIn = mOpen.GetOutVal() == 1.0;
			mMouseVisible = anIn;
			mMainMenuButton->mMouseVisible = anIn;
			mPlayMenuButton->mMouseVisible = anIn;
			if (mOpen.GetOutVal() == 1.0 && (mState == 1 || mState == 4))
				mUnknown640.SetCurve(GTS(0x7e0b40), nullptr);
			for (unsigned int i = 0; i < (unsigned int)mButtons.size(); i++) {
				if (mState == 1)
					mButtons[i]->mUnknown5C4 = mMouseVisible && mButtons[i]->mOpen;
				mButtons[i]->mMouseVisible = mMouseVisible && mButtons[i]->mOpen;
			}
		}
	}
	// The camera (a Transform3D, kept for the planets' projection): states 1 and 2 follow the camera curves, the others
	// fly from the start to the picked planet (mZoom); state 5 leaves it as it is
	if (mState != 5) {
		Transform3D aCamera;
		if (mState == 1 || mState == 2) {
			aCamera.mRot.RotateZ(CurveRadians(mCameraRotZ));
			aCamera.mRot.RotateX(CurveRadians(mCameraRotX));
			aCamera.mRot.RotateY(CurveRadians(mCameraRotY));
			float aDistance = (float)(mCameraDistance.GetOutVal() + 16.149999618530273);
			Vec3 aRow0 = aCamera.mRot.Row(0);
			Vec3 aRow1 = aCamera.mRot.Row(1);
			Vec3 aRow2 = aCamera.mRot.Row(2);
			// the start, a distance along the third axis, some along the first and the second, and the shifts
			float aX = aRow2.x * aDistance + mCameraPos[0];
			float aY = mCameraPos[1] + aRow2.y * aDistance;
			float aZ = mCameraPos[2] + aRow2.z * aDistance;
			float aRight = (float)mCameraRight.GetOutVal();
			aCamera.mPos.x = aRow0.x * aRight + aX;
			aCamera.mPos.y = aRow0.y * aRight + aY;
			aCamera.mPos.z = aRow0.z * aRight + aZ;
			float anUp = (float)mCameraUp.GetOutVal();
			aCamera.mPos.x = aRow1.x * anUp + aCamera.mPos.x;
			aCamera.mPos.y = aRow1.y * anUp + aCamera.mPos.y;
			aCamera.mPos.z = aRow1.z * anUp + aCamera.mPos.z;
			float aShiftX = (float)mCameraShiftX.GetOutVal();
			aCamera.mPos.x = aShiftX + aCamera.mPos.x;
			float aShiftY = (float)mCameraShiftY.GetOutVal();
			aCamera.mPos.y = aShiftY + aCamera.mPos.y;
		} else {
			float aFly = (float)mZoom.GetOutVal();
			float aFromX = (float)((double)mCameraPos[0] + 0.0);
			float aFromY = (float)((double)mCameraPos[1] + 0.0);
			float aFromZ = (float)((double)mCameraPos[2] + 16.149999618530273);
			float aToX = (float)((double)mZoomPos[0] + 0.0);
			float aToY = (float)((double)mZoomPos[1] + 0.15000000596046448);
			float aToZ = (float)((double)mZoomPos[2] + 7.5);
			aCamera.mPos.x = (float)(((double)aToX - (double)aFromX) * (double)aFly + (double)aFromX);
			aCamera.mPos.y = (float)(((double)aToY - (double)aFromY) * (double)aFly + (double)aFromY);
			aCamera.mPos.z = (float)(((double)aToZ - (double)aFromZ) * (double)aFly + (double)aFromZ);
		}
		gMenuCamera = aCamera;
	}
	// The hyperspace's 3D camera turns from the identity to the menu's by mCameraTurn
	if (gApp->mHyperspace != nullptr) {
		float aTurn = (float)mCameraTurn.GetOutVal();
		Mat3 anIdentity;
		gApp->mHyperspace->mUnknown1548 = SlerpMat3(anIdentity, mCameraTarget, aTurn, false);
	}
	// The buttons rise with mButtonSlide (0x34 is the buttons' y)
	mPlayMenuButton->mY = Ftol(((1.0 - mButtonSlide.GetOutVal()) * 0.5 + 0.9250000119209290) * (double)mHeight);
	mMainMenuButton->mY = Ftol(((1.0 - mButtonSlide.GetOutVal()) * 0.5 + 0.9250000119209290) * (double)mHeight);
	// The glow's particles ride down with the open curve
	double anOpen = mOpen.GetOutVal();
	int aGlowShift = Ftol((double)gApp->mScale * 1500.0);
	mFXManager1BC->mY = Ftol((double)aGlowShift * (1.0 - anOpen));
	mFXManager1BC->mAlpha = (mState == 1 || mState == 4 || mState == 5) ? (float)gSunCurve.GetOutVal() : 0.0f;
	gAspect = (float)((double)gApp->mDDInterface->mWidth / (double)gApp->mDDInterface->mHeight);
	gAspect45 = (float)((double)gAspect * 45.0);
	// The sun's glow throws particles about while the menu is all the way in
	if (mStarted == 0 && mOpen.GetOutVal() == 1.0 && (mState == 1 || mState == 4)) {
		float aRadius = (float)((double)IMAGE_ECLIPSE_SUN->mWidth * 0.30000001192092896);
		int aCount;
		if (gD3DInterface != nullptr) {
			aCount = 2;  // with Direct3D two a time, and no random draw for the number
		} else {
			aCount = (int)(Rand() % 100) / 80;
		}
		while (0 < aCount) {
			Effect* anEffect = mFXManager1BC->AllocEffect(0);
			anEffect->mImage = GetImageById((Rand() & 1) == 0 ? IMAGE_ECLIPSE_FLARE1_ID : IMAGE_ECLIPSE_FLARE2_ID);
			float aSpeed = (float)((double)gApp->mScale * 0.30000001192092896);
			aSpeed = (float)std::fabs(RandSigned() * aSpeed);
			float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
			anEffect->mX = (float)((double)(std::cos(anAngle) * aRadius) + (double)(mWidth / 2));
			anEffect->mY = (float)((double)(std::sin(anAngle) * aRadius) + (double)(mHeight / 2));
			anEffect->mDX = std::cos(anAngle) * aSpeed;
			anEffect->mDY = std::sin(anAngle) * aSpeed;
			anEffect->mAlpha = 0;
			anEffect->mDAlpha = 0.0024999999441206455f;
			anEffect->mMaxAlpha = 0.25f;
			anEffect->mFlags |= Effect::FLAG_ALPHA_BOUNCE;
			anEffect->mScale = (float)((double)RandSigned() * 0.10000000149011612 + 0.5);
			anEffect->mDScale = 0.004999999888241291f;
			anEffect->mAngle = (float)((double)RandSigned() * 3.1415927410125732);
			anEffect->mDAngle = (float)((double)RandSigned() * 3.1415927410125732 * 0.0024999999441206455);
			int aGrey = (int)(Rand() % 15) + 240;
			anEffect->mColor = Color(aGrey, aGrey, aGrey);
			anEffect->mIsAdditive = false;
			mFXManager1BC->AddEffect(anEffect);
			aCount--;
		}
	}
	if (0.0f < mUnknown15C) {
		float aLeft = (float)((double)mUnknown15C - 0.029999999329447746);
		mUnknown15C = 0.0f < aLeft ? aLeft : 0.0f;
	}
	// 0x6e4989: the end-of-challenge message at +0x9dc. At its update count 250 the rank bar's timer gains the progress
	// update's result over 100; once clicked it goes, and the menu goes from state 5 to state 4
	if (mAwardTelly != nullptr && mAwardTelly->mUpdateCnt == 250) {
		int aResult = UpdateProgress(true, false);
		mAwardTelly->mRankBonus->mTimer = (float)((double)(float)aResult / 100.0 + (double)mAwardTelly->mRankBonus->mTimer);
	}
	if (mAwardTelly != nullptr && mAwardTelly->mClicked) {
		gApp->mWidgetManager->RemoveWidget(mAwardTelly);
		gApp->SafeDeleteWidget(mAwardTelly);
		mAwardTelly = nullptr;
		if (mState == 5)
			SetState(4, false);
	}
	MarkDirty();
}

// 0x6dd322: the sun's glow particles, and a picture behind each of the two buttons (DrawAll draws the sun and the
// rest over the planets, 0x6e4a25)
void ChallengeMenu::Draw(Graphics* g)
{
	TwistWidget::Draw(g);
	mFXManager1BC->Draw(g);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColor(Color::White);
	Image* aGraphic = IMAGE_PLAYMENU_MMGRAPHIC;
	int aShift = Ftol((double)gApp->mScale * -5.0);
	int aY = ((mPlayMenuButton->mHeight / 2 - aShift) - aGraphic->mHeight / 2) + mPlayMenuButton->mY;
	g->DrawImage(aGraphic, (mPlayMenuButton->mWidth / 2 - aGraphic->mWidth / 2) + mPlayMenuButton->mX, aY);
	g->DrawImage(aGraphic, (mMainMenuButton->mWidth / 2 - aGraphic->mWidth / 2) + mMainMenuButton->mX, aY);
}

// 0x6e4a25: nothing at all while the sun's curve is 0, unless space is bringing back the old level's picture at the
// end of a game (state 12 with +0x159 clear), which is then drawn over the menu
void ChallengeMenu::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	Hyperspace* aHyperspace = gApp->mHyperspace;
	bool anOldPicture = aHyperspace != nullptr && aHyperspace->mState == 12 && !aHyperspace->mFlag159;
	if (gSunCurve.GetOutVal() == 0.0 && !anOldPicture)
		return;
	if (gD3DInterface != nullptr)
		gD3DInterface->ClearDepth();  // slot 0x124
	TwistWidget::DrawAll(theFlags, g);
	DrawSunAndTexts(g);
	if (anOldPicture)
		gApp->mHyperspace->DrawOldPicture(g);  // 0x70f836
}

// 0x6e132d
void ChallengeMenu::DrawSunAndTexts(Graphics* g)
{
	// The sun at the camera's start (0, -12, 0), greyer as mUnknown928 rises, at the sun curve's alpha
	int aGrey = Ftol(255.0 - mUnknown928.GetOutVal() * 255.0);
	int anAlpha = Ftol(gSunCurve.GetOutVal() * 255.0);
	unsigned int aColor = (unsigned int)((((anAlpha << 8 | aGrey) << 8 | aGrey) << 8) | aGrey);
	DrawBillboard(g, IMAGE_ECLIPSE_SUN, mCameraPos[0], mCameraPos[1], mCameraPos[2], aColor, Graphics::DRAWMODE_NORMAL,
		4.75f);

	// The moon: 4.1 from the sun along z, turned about the sun's y by mUnknown720 times one and a half pi
	Transform3D aSunPlace;  // 0x4bd4c0
	aSunPlace.mPos = Vec3{mCameraPos[0], mCameraPos[1], mCameraPos[2]};  // 0x6dd0e2
	Vec3 aMoon{0.0f, 0.0f, 4.0999999f};
	Mat3 aTurn;
	float aTurnValue = (float)mUnknown720.GetOutVal();
	aTurn.RotateY((float)((double)aTurnValue * 3.1415927410125732 * 1.5));
	aMoon = TimesMat3(aMoon, aTurn);           // 0x673fa4
	aMoon = TransformPoint(aSunPlace, aMoon);  // 0x68b6b6
	if (gD3DInterface != nullptr) {
		// The moon: the biggest sphere with the moon's picture, 2.43 large a little off the point (-0.04, 0.02, 0), turned about
		// y by 0.025 degrees for each update, lit by a light along the moon's own turn; its dark side is the eclipse's,
		// by mUnknown6B0. The moon's flat picture, black, is drawn over it while the eclipse is under 0.05.
		PerspectiveCamera aCamera(gAspect45, gAspect, 1.0f, 10000.0f);  // 0x67417d
		WorldGraphicsListener aListener(&aCamera, false);              // 0x4bd5d0
		aListener.SetCulling(true);
		aListener.SetDepth(false, true);
		aCamera.mTransform = gMenuCamera;  // 0x673c10
		gD3DInterface->SetTextureWrap(0, true);
		gD3DInterface->SetTexture(0, IMAGE_PLANET_MOON);
		Transform3D aMoonPlace;  // 0x4bd4c0
		aMoonPlace.mScale.x = 2.430000066757202f * aMoonPlace.mScale.x;  // 0x68b341
		aMoonPlace.mScale.y = 2.430000066757202f * aMoonPlace.mScale.y;
		aMoonPlace.mScale.z = 2.430000066757202f * aMoonPlace.mScale.z;
		float aMoonSpin = (float)((double)mUpdateCnt * 0.02500000037252903);
		float aMoveY = (float)((double)aMoon.y + 0.019999999552965164);
		float aMoveX = (float)((double)aMoon.x - 0.03999999910593033);
		aMoonPlace.mPos.x = aMoonPlace.mPos.x + aMoveX;  // 0x68b2a8
		aMoonPlace.mPos.y = aMoonPlace.mPos.y + aMoveY;
		aMoonPlace.mPos.z = aMoonPlace.mPos.z + aMoon.z;
		aMoonPlace.mRot.RotateZ((float)(0.0 * 3.1415927410125732 / 180.0));  // 0x673fd0
		aMoonPlace.mRot.RotateY((float)((double)aMoonSpin * 3.1415927410125732 / 180.0));  // 0x68fa0b
		aListener.mWorld = aMoonPlace;
		aListener.SetLighting(true);
		gD3DInterface->SetMaterial(Color::White, Color::White, Color::White, Color::Black, 0.0f);  // slot 0xf0
		D3DLightColors aColors;  // 0x6802ad
		aColors.mDiffuse = Color::White;
		aColors.mSpecular = Color::Black;
		aColors.mAmbient = Color(0, 0, 0);
		aColors.mIntensity = 2.430000066757202f;
		aColors.mAmbient.mRed = Ftol(mUnknown6B0.GetOutVal() * (double)aColors.mAmbient.mRed + 28.0);
		aColors.mAmbient.mGreen = Ftol(mUnknown6B0.GetOutVal() * (double)aColors.mAmbient.mGreen + 28.0);
		aColors.mAmbient.mBlue = Ftol(mUnknown6B0.GetOutVal() * (double)aColors.mAmbient.mBlue + 28.0);
		aColors.mDiffuse.mRed = Ftol(mUnknown6B0.GetOutVal() * (double)aColors.mDiffuse.mRed);
		aColors.mDiffuse.mGreen = Ftol(mUnknown6B0.GetOutVal() * (double)aColors.mDiffuse.mGreen);
		aColors.mDiffuse.mBlue = Ftol(mUnknown6B0.GetOutVal() * (double)aColors.mDiffuse.mBlue);
		Vec3 aLightDirection = TimesMat3(Vec3{0.0f, 0.0f, 1.0f}, aTurn);  // 0x673fa4
		gD3DInterface->SetDirectionalLight(0, aLightDirection, aColors);   // slot 0xec
		gD3DInterface->LightEnable(0, true);                               // slot 0xe4
		int aSphereAlpha = Ftol(gSunCurve.GetOutVal() * 255.0);
		SetSphereAlpha(gSphereBig, aSphereAlpha);
		PrimitiveListener aPrimitiveListener(&aListener);
		gD3DInterface->DrawPrimitiveEx(0x152, GPUPT_TRIANGLELIST, gSphereBig.data(), (int)gSphereBig.size() / 3,
			Color(255, 255, 255, 255), 0, 0.0f, 0.0f, true, &aPrimitiveListener);  // slot 0x7c
		gD3DInterface->SetTexture(0, nullptr);
		gD3DInterface->SetTextureWrap(0, false);
		if (mUnknown6B0.GetOutVal() < 0.05000000074505806) {
			int aBlackAlpha = Ftol(gSunCurve.GetOutVal() * 255.0 * (1.0 - mUnknown6B0.GetOutVal() / 0.05000000074505806));
			DrawBillboard(g, IMAGE_ECLIPSE_MOON, (float)((double)aMoon.x - 0.07999999821186066),
				(float)((double)aMoon.y + 0.07000000029802322), aMoon.z, (unsigned int)aBlackAlpha << 24,
				Graphics::DRAWMODE_NORMAL, 2.5f);
		}
		g->SetColorizeImages(true);
		g->SetColor(Color::White);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	} else {
		// dark grey (48), lighter by mUnknown6B0
		unsigned int aBright = (unsigned int)Ftol(mUnknown6B0.GetOutVal() * 255.0) + 48;
		unsigned int aMoonGrey = 255;
		if (aBright <= 255)
			aMoonGrey = aBright;
		unsigned int aMoonAlpha = (unsigned int)Ftol(gSunCurve.GetOutVal() * 255.0);
		unsigned int aMoonColor = (((aMoonAlpha << 8 | aMoonGrey) << 8 | aMoonGrey) << 8) | aMoonGrey;
		DrawBillboard(g, IMAGE_ECLIPSE_MOON, aMoon.x + -0.0f, aMoon.y + 0.0f, aMoon.z, aMoonColor, Graphics::DRAWMODE_NORMAL,
			2.7f);
	}

	g->SetFont(FONT_DIALOG_TEXT);
	SetFontLayerColor(FONT_DIALOG_TEXT, GTS(0x80dfb4), Color(255, 255, 255, 255));
	g->SetColor(Color(255, 255, 255));
	// The eclipse's gold ring, added in while the sun is fully up, by how far mUnknown6B0 is below 1
	double aDark = mUnknown6B0.GetOutVal();
	double aSun = gSunCurve.GetOutVal();
	float aRing = (float)((1.0 - aDark) * aSun * gSunCurve.GetOutVal());
	if (0.0f < aRing && gSunCurve.GetOutVal() == 1.0) {
		if (gD3DInterface != nullptr) {
			// with Direct3D smaller (2.75), a little off the moon, as the moon's own
			unsigned int aRingColor = ((unsigned int)Ftol((double)aRing * 255.0) << 24) | 0xffffff;
			DrawBillboard(g, IMAGE_ECLIPSE_RING, (float)((double)aMoon.x - 0.10000000149011612),
				(float)((double)aMoon.y + 0.07999999821186066), (float)((double)aMoon.z + 0.10000000149011612), aRingColor,
				Graphics::DRAWMODE_ADDITIVE, 2.75f);
		} else {
			unsigned int aRingColor = ((unsigned int)Ftol((double)aRing * 255.0) << 24) | 0xffffff;
			DrawBillboard(g, IMAGE_ECLIPSE_RING, aMoon.x + -0.0f, aMoon.y + 0.0f, aMoon.z + 0.0f, aRingColor,
				Graphics::DRAWMODE_ADDITIVE, 2.9f);
		}
	}

	// The two dancing glows: while mUnknown640 is up they swing round the middle of the screen, 280 from it, from 0.3
	// of a turn to none as its input runs to its end, and turn from half a turn to none
	if (0.0 < mUnknown640.GetOutVal()) {
		const double aTwoPi = 6.2831854820251465;  // the float at 0x84ec00
		float anAngleFrom = (float)(0.30000001192092896 * aTwoPi);
		float anAngleTo = (float)(0.0 * aTwoPi);
		float aTurnFrom = (float)(aTwoPi * 0.5);
		float aTurnTo = anAngleTo;
		float aDone = (float)(mUnknown640.mInVal / mUnknown640.mInMax);
		float aRadius = (float)Ftol((double)gApp->mScale * 280.0);
		float anAngle = (float)(((double)anAngleTo - (double)anAngleFrom) * (double)aDone + (double)anAngleFrom);
		float aGlowTurn = (float)(((double)aTurnTo - (double)aTurnFrom) * (double)aDone + (double)aTurnFrom);
		float aCos = std::cos(anAngle);
		int aGlowX = Ftol((double)(mWidth / 2) + (double)aCos * (double)aRadius);
		float aSin = std::sin(anAngle);
		int aGlowY = Ftol((double)(mHeight / 2) - (double)aSin * (double)aRadius);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol(mUnknown640.GetOutVal() * 255.0)));
		DrawImageCentered(g, IMAGE_ECLIPSE_DANCE2, (float)aGlowX, (float)aGlowY, (double)aGlowTurn, 1.0f, 1.0f);
		double aGlow = mUnknown640.GetOutVal();
		g->SetColor(Color(255, 255, 255, Ftol(aGlow * 255.0 * mUnknown640.GetOutVal())));
		DrawImageCentered(g, IMAGE_ECLIPSE_DANCE1, (float)(aGlowX - Ftol((double)gApp->mScale * -15.0)),
			(float)(aGlowY - Ftol((double)gApp->mScale * 55.0)), (double)aGlowTurn * 0.0, 1.0f, 1.0f);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	// With the menu all the way in: the hovered planet's texts, or the menu's title while none is hovered
	if (mOpen.GetOutVal() != 1.0 || (mState != 1 && mState != 4))
		return;
	if (mHover <= -1) {
		if (0.0f < mUnknown9D4) {
			g->SetColorizeImages(true);
			double aTitleSun = gSunCurve.GetOutVal();
			g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown9D4 * 255.0 * aTitleSun)));
			g->DrawImage(IMAGE_CHALLENGEMODETITLE, mWidth / 2 - IMAGE_CHALLENGEMODETITLE->mWidth / 2,
				Ftol((double)gApp->mScale * 516.0));
		}
		return;
	}
	ChallengeButton* aButton = mButtons[mHover];
	int aCenterX = mWidth / 2;
	int aY = mHeight / 2 - Ftol((double)gApp->mScale * -55.0);
	if (aButton->mEclipse)
		aY += Ftol((double)gApp->mScale * 10.0);
	if (!mBestText.empty()) {
		g->SetColor(Color(255, 255, 255, Ftol((double)mHoverFade * 255.0)));
		g->SetFont(FONT_DIALOG_TEXT);
		SetFontLayerColor(g->GetFont(), GTS(0x80dfb4), Color::White);
		int aTextY = Ftol((double)gApp->mScale * 100.0) + aY;
		g->DrawString(mBestText, aCenterX - g->StringWidth(mBestText) / 2, aTextY);  // 0x4223e0, 0x421180
		aY += Ftol((double)gApp->mScale * -40.0);
	}
	g->SetColor(Color::White);
	if (mDescImage != nullptr) {
		DrawImageScaledCentered(g, mDescImage, (float)aCenterX, (float)(aY - Ftol((double)gApp->mScale * 10.0)), 1.0f,
			1.0f);
	}
	g->SetColorizeImages(true);
	if (0.0f < mUnknown15C) {
		g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown15C * 255.0)));
		if (mDescGlowImage != nullptr) {
			DrawImageScaledCentered(g, mDescGlowImage, (float)aCenterX,
				(float)(aY - Ftol((double)gApp->mScale * 10.0)), 1.0f, 1.0f);
		}
	}
	if (aButton->mEclipse) {
		aY = mHeight / 2 - Ftol((double)gApp->mScale * 70.0);
		if (!mBestText.empty())
			aY += Ftol((double)gApp->mScale * -40.0);
	} else {
		aY = mHeight / 2 - Ftol((double)gApp->mScale * 60.0);
	}
	g->SetColor(Color::White);
	if (mTitleImage != nullptr) {
		// The name, then while the text is still fading in, larger and fainter copies of it (each 0.02 larger and
		// fainter by the fade squared, at least 0.03) until they are gone
		float aScale = 1.0f;
		float aFaint = 1.0f;
		float aTitleY = (float)aY;
		float aTitleX = (float)aCenterX;
		do {
			g->SetColor(Color(255, 255, 255, Ftol((double)aFaint * 255.0 * (double)mHoverFade)));
			DrawImageScaledCentered(g, mTitleImage, aTitleX, aTitleY, aScale, aScale);
			aScale = (float)((double)aScale + 0.019999999552965164);
			if (mHoverFade == 1.0f)
				break;
			float aStep = PowInt(mHoverFade, 2);
			if ((double)aStep < 0.029999999329447746)
				aStep = 0.03f;
			aFaint = aFaint - aStep;
		} while (0.01f < aFaint);
	}
	// The trial: the planets after the third and the challenges after the third are locked
	if (!gApp->mRegistered && (aButton->mId > 2 || aButton->mStage > 3)) {
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColor(Color(255, 255, 255, Ftol((double)mHoverFade * 255.0)));
		DrawImageScaledCentered(g, IMAGE_TRIALVERSION_CHALLENGE, (float)(mWidth / 2), (float)(mHeight / 2), ALIGN_CENTER,
			1.0f, 1.0f);
		// and its flash after a click on a locked one (ButtonDepress starts mUnknown8A0)
		if (mUnknown8A0.GetOutVal() != 0.0) {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColor(Color(255, 255, 255, Ftol(mUnknown8A0.GetOutVal() * 255.0)));  // 0x6a1984
			DrawImageScaledCentered(g, IMAGE_TRIALVERSION_CHALLENGE, (float)(mWidth / 2), (float)(mHeight / 2),
				ALIGN_CENTER, 1.0f, 1.0f);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}
}

// 0x6dfe2a: the camera's zoom
void ChallengeMenu::MouseWheel(int theDelta)
{
	(void)theDelta;
}

// 0x6e54c3
void ChallengeMenu::ButtonDepress(int theId)
{
	int aSound = SOUND_BUTTONPRESS2;
	if (theId == BUTTON_PLAY_MENU) {
		Close(2);
		gApp->ShowModeSelect(0);
		PlayButton* aPlayButton = gApp->mMainMenu->mPlayButton;
		aPlayButton->mAppear.SetCurve(GTS(0x7e0bb0));
		aPlayButton->mAlpha.SetCurve(GTS(0x7e0b88));
	} else if (theId == BUTTON_MAIN_MENU) {
		Close(2);
		gApp->ShowMainMenu(false, false, 0.0f);
	} else if (!mUnknown9BC) {
		ChallengeButton* aButton = mButtons[theId];
		mCategory = aButton->mCategory;
		mStageSel = aButton->mStage;
		if (aButton->mEclipse)
			mStageSel++;
		if (!gApp->mRegistered && (mStageSel > 3 || mCategory > 2)) {
			// The trial: the curve at +0x8a0 (0x6e54c3, whose listener-relative +0x758 it is) starts, and the click sound
			mUnknown8A0.SetCurve(GTS(0x7e0c70));
			aSound = SOUND_TRIALCLICK;
		} else if (!gApp->SaveExists(2, mCategory)) {
			Start(true);
		} else {
			TwistDialog* aDialog = new TwistDialog(IMAGE_SIZEABLEDIALOG, IMAGE_SIZEABLEBUTTON, 0, true, GT(0x80e8a4),
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
			aDialog->mX = (mWidth / 2 - aDialog->mWidth / 2) + mX;
			aDialog->mY = (mHeight / 2 - aDialog->mHeight / 2) + mY;
		}
	}
	gApp->PlaySample(aSound);
}

// 0x6e58f7: Continue goes on with the saved game, the other button gives it up
void ChallengeMenu::DialogButtonDepress(int theDialogId, int theButtonId)
{
	if (theDialogId == 0) {
		bool aNewGame = true;
		if (theButtonId == Dialog::ID_YES) {
			aNewGame = false;
		} else if (theButtonId == Dialog::ID_NO) {
			aNewGame = true;
			gApp->DeleteSave(2, mCategory);
		}
		Start(aNewGame);
	}
	gApp->KillDialog(theDialogId);
}

// 0x6e665e: the planet's texts start to fade in, and its three pictures are made: the challenge's description twice
// (FONT_PLANET_TEXT, 0.3 of the menu's width; the second time with the font's second layer too) and the category's
// name in FONT_FRUIT_AWARD in the category's colour (with the eclipse title under an eclipse planet's)
void ChallengeMenu::ButtonMouseEnter(int theId)
{
	if (!mMouseVisible || theId >= 100)
		return;
	mHoverFade = 0;
	mHover = theId;
	ChallengeButton* aButton = mButtons[theId];
	int aStage = aButton->mStage;
	delete mDescImage;
	mDescImage = nullptr;
	delete mTitleImage;
	mTitleImage = nullptr;
	SexyString aName = gApp->mChallengeInfo->GetCategoryId(theId);
	int aBest = 0;
	if (aButton->mEclipse)
		aBest = gApp->mProfile->mInts204[aButton->mCategory];  // 0x439090
	if (aButton->mEclipse && aBest != 0) {
		mBestText = StrFormat(GTC(0x7e0d8c), aBest);
	} else {
		if (!aButton->mEclipse)
			aName += StrFormat(GTC(0x7e0d80), aButton->mStage);
		mBestText = GT(0x80d9a4);
	}
	unsigned int i = 0;
	while (i < aName.size()) {
		if (aName[i] == '_')
			aName[i] = ' ';
		i++;
	}
	if (aButton->mEclipse)
		aStage++;
	Challenge* aChallenge = gApp->mChallengeInfo->GetChallenge(aButton->mCategory, aStage);
	SexyString aDesc;
	if (aChallenge != nullptr)
		aDesc = gApp->mChallengeInfo->GetText(aChallenge, false, false, nullptr);

	// The description (0x4233d0 is GetWordWrappedHeight; its last argument, 0, goes to WriteWordWrapped)
	int aDescWidth = Ftol((double)mWidth * 0.30000001192092896);
	Font* aFont = FONT_PLANET_TEXT;
	Graphics aMeasure;
	aMeasure.SetFont(aFont);
	int aDescHeight = aMeasure.GetWordWrappedHeight(aDescWidth, aDesc, -1, nullptr);
	Rect aDescRect(0, 0, aDescWidth, aDescHeight + Ftol((double)gApp->mScale * 20.0));
	delete mDescGlowImage;
	SetFontLayerColor(aFont, GTS(0x7e029c), Color(0, 0, 0, 0));
	SetFontLayerColor(aFont, GTS(0x80dfb4), Color::White);
	SetFontLayerColor(aFont, GTS(0x7dd6b0), Color::White);
	mDescGlowImage = new TextImage(aFont, aDesc, 0, Ftol((double)gApp->mScale * 10.0), 0, 0, &aDescRect);
	SetFontLayerColor(aFont, GTS(0x80dfb4), Color::White);
	SetFontLayerColor(aFont, GTS(0x7e029c), Color(0, 0, 0, 0));
	SetFontLayerColor(aFont, GTS(0x7dd6b0), Color(0, 0, 0, 0));
	mDescImage = new TextImage(aFont, aDesc, 0, Ftol((double)gApp->mScale * 10.0), 0, 0, &aDescRect);
	mDescImage->mD3DFlags |= 0x80;  // the picture's +0x44 flag 0x80: no region mesh for the Direct3D draw (set in both modes)

	// The name, in the colour of its category (0x6e6a8c: the table on the stack)
	const Color aColors[13] = {Color(255, 198, 45), Color(255, 255, 255), Color(255, 206, 133), Color(255, 84, 115),
		Color(210, 146, 255), Color(127, 242, 0), Color(255, 115, 26), Color(209, 43, 255), Color(209, 255, 58),
		Color(255, 203, 159), Color(255, 119, 78), Color(0, 192, 255), Color(255, 210, 80)};
	Font* aNameFont = FONT_FRUIT_AWARD;
	SetFontLayerColor(aNameFont, GTS(0x80dfb4), aColors[theId]);
	SetFontLayerColor(aNameFont, GTS(0x7e029c), Color(0, 0, 0));
	SetFontLayerColor(aNameFont, GTS(0x7dd6b0), Color(0, 0, 0, 0));
	Graphics aNameMeasure;
	aNameMeasure.SetFont(aNameFont);
	int aNameWidth = Ftol((double)gApp->mScale * 500.0);
	int aSpacing = Ftol((double)gApp->mScale * 50.0);
	int aNameHeight = aNameMeasure.GetWordWrappedHeight(aNameWidth, aName, aSpacing, nullptr);
	if (aButton->mEclipse)
		aNameHeight += Ftol((double)gApp->mScale * 50.0);
	MemoryImage* aTitle = new MemoryImage();  // 0x42bb20
	mTitleImage = aTitle;
	aTitle->Create(aNameWidth + 16, aNameHeight + 16);
	aTitle->Clear();
	// (the original copies a Graphics of the picture into the one it measured with, 0x6e6619)
	Graphics aTitleG(aTitle);
	aTitleG.SetColor(Color::White);
	aTitleG.SetFont(aNameFont);
	int aTitleWidth = aNameWidth + 8;
	// (0x422eb0's last argument, 0, is one this framework's WriteWordWrapped does not have)
	aTitleG.WriteWordWrapped(Rect(0, 0, aTitleWidth, aNameHeight + 8), aName, aSpacing, 0, nullptr, -1, nullptr);
	if (aButton->mEclipse) {
		Image* anEclipse = IMAGE_ECLIPSE_TITLE;
		int anEclipseWidth = Ftol((double)anEclipse->mWidth * 0.6499999761581421);
		int anEclipseHeight = Ftol(0.6499999761581421 * (double)anEclipse->mHeight);
		aTitleG.DrawImage(anEclipse, aTitleWidth / 2 - anEclipseWidth / 2, Ftol((double)gApp->mScale * 65.0),
			anEclipseWidth, anEclipseHeight);  // 0x4216f0
	}
	mUnknown15C = 1.0f;
}

// 0x6dd418
void ChallengeMenu::ButtonMouseLeave(int theId)
{
	if (mHover == theId)
		mHover = -1;
}

// 0x6e4ab6
void ChallengeMenu::Start(bool theNewGame)
{
	if (gApp->mHyperspace == nullptr)
		gApp->ShowHyperspace();
	gApp->NewGame(2, theNewGame, mCategory);
	if (theNewGame)
		gBoard->SetChallenge(gApp->mChallengeInfo->GetChallenge(mCategory, mStageSel));
	mStarted = 1;
	mMainMenuButton->mMouseVisible = false;
	mPlayMenuButton->mMouseVisible = false;
	mMouseVisible = false;
	// (the planet's place in the 3D world, at +0x5b4, aims the 3D camera: unused in 2D)
	gApp->mBoard->mHyperspace->StartChallenge();
	gApp->mBoard->SetUnknown3924(0.0f);
	gApp->mBoard->ApplyUnknown3924();
	Close(3);
}
