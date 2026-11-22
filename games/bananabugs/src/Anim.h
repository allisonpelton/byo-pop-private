// Anim: one picture cut into cels, pre-scaled to the window when it loads (0x401000..0x402640 in the original). Every
// scene object draws through one; the pictures are kept in one registry, shared by name, and rebuilt when the window
// changes size (the "images/reload" screen).
#pragma once

#include "Graphics.h"
#include "SexyMatrix.h"

#include <list>

namespace Sexy
{

class Image;
class SexyAppBase;

// How a picture is scaled: kept as it is, stretched fast, or stretched smoothly (the fonts' sheets)
enum AnimScaleMode
{
	ANIM_SCALE_NONE = 0,
	ANIM_SCALE_FAST = 1,
	ANIM_SCALE_SMOOTH = 2,
};

const int ANIM_MAX_CELS = 256;

struct AnimCel
{
	int mX;
	int mY;
	int mRight;  // the last column inside the cel
	int mBottom; // the last row inside the cel
	int mWidth;
	int mHeight;
};

class Anim
{
public:
	int mRefCount;
	Image* mImage;            // the scaled sheet (or the loaded picture itself for one unscaled cel)
	bool mNoScale;            // drawn at the picture's own size, whatever the window's
	int mCelsAcross;
	int mCelsDown;
	float mCelWidth;          // a cel's size after scaling
	float mCelHeight;
	int mNumCels;
	float mSourceCelWidth;    // a cel's size in the picture file
	float mSourceCelHeight;
	AnimCel mCels[ANIM_MAX_CELS];
	char mName[128];          // the picture's file name, the registry's key
	int mUnusedAfterName;     // set to 0 by the constructor; nothing reads it
	bool mPersistent;         // kept when the last reference goes (the constructor sets it, and nothing clears it)
	int mScaleMode;           // AnimScaleMode
	bool mUniformScale;       // scaled by the window's width both ways

	// The registry of every Anim made, and the app they load through
	static std::list<Anim*> sAnims;
	static SexyAppBase* sApp;

	Anim();
	virtual ~Anim();
	virtual void Reload();

	void AddRef();
	void Release();
	float GetCelWidth() const { return mCelWidth; }             // 0x401930
	float GetCelHeight() const { return mCelHeight; }           // 0x401940
	float GetSourceCelWidth() const { return mSourceCelWidth; } // 0x401950
	float GetSourceCelHeight() const { return mSourceCelHeight; } // 0x401960

	void Load(bool noScale, const char* theName, int theMode, int theCelWidth, int theCelHeight, int theNumCels,
		bool uniformScale);
	void LoadFontSheet(const char* theName);
	void Scale(Image* theImage, int theMode, float theScaleX, float theScaleY, int theCelWidth, int theCelHeight,
		int theNumCels);
	void DrawCel(int theCel, Graphics* g, float x, float y, const SexyMatrix3& theMatrix, float theExtraWidth,
		float theExtraHeight);
	void DrawCelAt(int theCel, Graphics* g, int x, int y, const SexyMatrix3& theMatrix, float theExtraWidth,
		float theExtraHeight);

	static Anim* Get(const char* theName, int theMode, bool noScale, int theCelWidth, int theCelHeight, int theNumCels,
		bool uniformScale);
	static void SetApp(SexyAppBase* theApp);
	static void Unregister(Anim* theAnim);
};

// The progressive rebuild of every picture after the window changes size: one Anim a frame, drawn as a bar
struct AnimReload
{
	static bool sActive;
	static float sProgress;
	static float sStep;
	static float sTotal;
	static Image* sImage; // "images/reload"
	static std::list<Anim*>::iterator sNext;

	static void Begin();
	static void Step();
	static void Draw(Graphics* g);
};

// The scale from the 1920x1200 design space to the window (0x600598, 0x60059c), and the one the pictures are made at
// (0x600530, 0x600534), which follows it
extern float gScreenScaleX;
extern float gScreenScaleY;
extern float gImageScaleX;
extern float gImageScaleY;

} // namespace Sexy
