// GPUInterface test (framework/GPUInterface.h): one test for each Direct3D 9 convention the GL implementation converts,
// on the real GPU, with exact pixels (±1 where GL may round a blend or a lit colour the other way). Native only; it opens
// a hidden GL window, so tools/check.sh unit runs it under rundesk.
//
//   gputest [-v]
//
// Each failed check prints one "FAIL <test>: ..." line. The last line is "gpu: N tests, M failed (<GL description>)".
// Exit status 0 only when every test passed. -v also prints each test's name as it passes.

#include "framework/GPUInterface.h"
#include "framework/MemoryImage.h"
#include "framework/SexyAppBase.h"
#include "platform/GLContext.h"

#include <SDL.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace Sexy;

namespace
{

const int W = 64, H = 64;

GPUInterface* gGPU = nullptr;
bool gVerbose = false;
int gTests = 0, gFailed = 0;
std::string gTest;
bool gTestFailed = false;

struct RHWVertex
{
	float x, y, z, rhw;
	unsigned int diffuse;
	float u, v;
};
const unsigned long FVF_RHW = GPUFVF_XYZRHW | GPUFVF_DIFFUSE | GPUFVF_TEX1;

struct XYZVertex
{
	float x, y, z;
	unsigned int diffuse;
};
const unsigned long FVF_XYZ = GPUFVF_XYZ | GPUFVF_DIFFUSE;

struct NormalVertex
{
	float x, y, z, nx, ny, nz;
};
const unsigned long FVF_NORMAL = GPUFVF_XYZ | GPUFVF_NORMAL;

void Fail(const char* theFormat, ...)
{
	char aText[512];
	va_list anArgs;
	va_start(anArgs, theFormat);
	std::vsnprintf(aText, sizeof(aText), theFormat, anArgs);
	va_end(anArgs);
	std::printf("FAIL %s: %s\n", gTest.c_str(), aText);
	gTestFailed = true;
}

// D3D's state for a test: no depth, lighting, culling, blending or alpha test; stage 0 passes the diffuse colour; the
// screen as the target, identity transforms, the whole screen as the viewport, cleared to black
void Reset()
{
	gGPU->EndScene();
	gGPU->SetRenderTarget(nullptr);
	gGPU->SetVertexShader(0);
	gGPU->SetPixelShader(0);
	gGPU->SetRenderState(GPURS_ZENABLE, 0);
	gGPU->SetRenderState(GPURS_ZWRITEENABLE, 1);
	gGPU->SetRenderState(GPURS_ZFUNC, 4);
	gGPU->SetRenderState(GPURS_LIGHTING, 0);
	gGPU->SetRenderState(GPURS_CULLMODE, 1);
	gGPU->SetRenderState(GPURS_ALPHABLENDENABLE, 0);
	gGPU->SetRenderState(GPURS_ALPHATESTENABLE, 0);
	gGPU->SetRenderState(GPURS_SCISSORTESTENABLE, 0);
	gGPU->SetRenderState(GPURS_SPECULARENABLE, 0);
	gGPU->SetTextureStageState(0, GPUTSS_COLOROP, 2);
	gGPU->SetTextureStageState(0, GPUTSS_COLORARG1, 0);
	gGPU->SetTextureStageState(0, GPUTSS_ALPHAOP, 2);
	gGPU->SetTextureStageState(0, GPUTSS_ALPHAARG1, 0);
	gGPU->SetTextureStageState(1, GPUTSS_COLOROP, 1);
	gGPU->SetTextureStageState(1, GPUTSS_ALPHAOP, 1);
	gGPU->SetSamplerState(0, GPUSAMP_MINFILTER, 1);
	gGPU->SetSamplerState(0, GPUSAMP_MAGFILTER, 1);
	gGPU->SetSamplerState(0, GPUSAMP_ADDRESSU, 3);
	gGPU->SetSamplerState(0, GPUSAMP_ADDRESSV, 3);
	for (int i = 0; i < 4; i++)
		gGPU->SetTexture(i, nullptr);
	gGPU->SetTransform(GPUTS_WORLD, GPUMatrix::Identity());
	gGPU->SetTransform(GPUTS_VIEW, GPUMatrix::Identity());
	gGPU->SetTransform(GPUTS_PROJECTION, GPUMatrix::Identity());
	gGPU->SetViewport(Rect(0, 0, W, H), 0.0f, 1.0f);
	gGPU->Clear(GPUCLEAR_TARGET | GPUCLEAR_ZBUFFER, 0xff000000, 1.0f);
}

void Begin(const char* theName)
{
	gTest = theName;
	if (gVerbose)
		std::printf("%s\n", theName);
	gTestFailed = false;
	Reset();
}

void End()
{
	gTests++;
	if (gTestFailed)
		gFailed++;
	else if (gVerbose)
		std::printf("ok %s\n", gTest.c_str());
}

// The screen (or a target) as ARGB, D3D's row 0 first
MemoryImage* gRead = nullptr;  // made once the app is

void Read(Image* theTarget = nullptr)
{
	if (!gGPU->ReadBack(theTarget, gRead))
		Fail("read back failed");
}

unsigned long Pixel(int x, int y)
{
	return gRead->GetBits()[y * gRead->mWidth + x];
}

// The pixel's colour against an expected ARGB, each channel within theTolerance
void Expect(int x, int y, unsigned long theColor, int theTolerance = 0)
{
	unsigned long aPixel = Pixel(x, y);
	for (int aShift = 0; aShift < 32; aShift += 8) {
		int a = (int)((aPixel >> aShift) & 0xff), b = (int)((theColor >> aShift) & 0xff);
		if (std::abs(a - b) > theTolerance) {
			Fail("pixel (%d,%d) is %08lx, expected %08lx", x, y, aPixel, theColor);
			return;
		}
	}
}

void RHWQuad(float l, float t, float r, float b, unsigned int theColor, float z = 0.5f)
{
	RHWVertex v[4] = {{l, t, z, 1, theColor, 0, 0}, {r, t, z, 1, theColor, 1, 0}, {l, b, z, 1, theColor, 0, 1},
		{r, b, z, 1, theColor, 1, 1}};
	gGPU->DrawPrimitiveUP(GPUPT_TRIANGLESTRIP, 2, FVF_RHW, v, sizeof(RHWVertex));
}

// A quad in clip space covering D3D's screen pixels l..r, t..b (identity transforms, the whole screen as viewport)
void ClipQuad(float l, float t, float r, float b, unsigned int theColor, float z = 0.5f)
{
	auto X = [](float s) { return s * 2.0f / W - 1.0f; };
	auto Y = [](float s) { return 1.0f - s * 2.0f / H; };
	XYZVertex v[4] = {{X(l), Y(t), z, theColor}, {X(r), Y(t), z, theColor}, {X(l), Y(b), z, theColor},
		{X(r), Y(b), z, theColor}};
	gGPU->DrawPrimitiveUP(GPUPT_TRIANGLESTRIP, 2, FVF_XYZ, v, sizeof(XYZVertex));
}

// A box's edges: inside at (l,t) and (r-1,b-1), outside just beyond each edge
void ExpectBox(int l, int t, int r, int b, unsigned long theInside, unsigned long theOutside)
{
	Expect(l, t, theInside);
	Expect(r - 1, b - 1, theInside);
	if (l > 0)
		Expect(l - 1, t, theOutside);
	if (t > 0)
		Expect(l, t - 1, theOutside);
	if (r < W)
		Expect(r, b - 1, theOutside);
	if (b < H)
		Expect(r - 1, b, theOutside);
}

MemoryImage* MakeImage(int theWidth, int theHeight, const std::vector<unsigned long>& theBits)
{
	MemoryImage* anImage = new MemoryImage();
	anImage->Create(theWidth, theHeight);
	std::memcpy(anImage->GetBits(), theBits.data(), theBits.size() * sizeof(unsigned long));
	anImage->BitsChanged();
	return anImage;
}

// --- the conventions ---

// Clear fills the viewport only, and the targets are stored top row first: D3D's row 0 is read back first
void TestClearViewport()
{
	Begin("clear-viewport");
	gGPU->SetViewport(Rect(8, 4, 16, 10), 0.0f, 1.0f);
	gGPU->Clear(GPUCLEAR_TARGET, 0xff336699, 1.0f);
	Read();
	ExpectBox(8, 4, 24, 14, 0xff336699, 0xff000000);
	End();
}

// Pre-transformed vertices: D3D9's pixel centres are on integers, so a quad from 10 to 20 covers pixels 10..19; the
// diffuse colour is D3DCOLOR (B, G, R, A in memory)
void TestPretransformedCentres()
{
	Begin("rhw-pixel-centres");
	RHWQuad(10, 5, 20, 15, 0xff102030);
	Read();
	ExpectBox(10, 5, 20, 15, 0xff102030, 0xff000000);
	End();
	// Half a pixel off, the same pixels: centres 10..19 lie inside 9.6..19.6
	Begin("rhw-pixel-centres-offset");
	RHWQuad(9.6f, 4.6f, 19.6f, 14.6f, 0xffffffff);
	Read();
	ExpectBox(10, 5, 20, 15, 0xffffffff, 0xff000000);
	End();
}

// Transformed vertices: clip space as D3D maps it (y up, pixel centres on integers, the viewport's offset)
void TestTransformedCentres()
{
	Begin("clip-pixel-centres");
	ClipQuad(10, 5, 20, 15, 0xff40c080);
	Read();
	ExpectBox(10, 5, 20, 15, 0xff40c080, 0xff000000);
	End();
	Begin("clip-viewport");
	gGPU->SetViewport(Rect(16, 8, 32, 32), 0.0f, 1.0f);
	XYZVertex v[4] = {{-1, 1, 0.5f, 0xffff0000}, {0, 1, 0.5f, 0xffff0000}, {-1, 0, 0.5f, 0xffff0000},
		{0, 0, 0.5f, 0xffff0000}};
	gGPU->DrawPrimitiveUP(GPUPT_TRIANGLESTRIP, 2, FVF_XYZ, v, sizeof(XYZVertex));
	Read();
	// the viewport's top-left quarter: pixels 16..31, 8..23
	ExpectBox(16, 8, 32, 24, 0xffff0000, 0xff000000);
	End();
}

// Depth: z in [0,1], 0 near; z below 0 is clipped, 0 itself is drawn; the depth test's function
void TestDepth()
{
	Begin("depth-range");
	gGPU->SetRenderState(GPURS_ZENABLE, 1);
	ClipQuad(0, 0, 32, 32, 0xffff0000, 0.2f);
	ClipQuad(0, 0, 32, 32, 0xff00ff00, 0.8f);  // behind: fails LESSEQUAL
	ClipQuad(16, 16, 48, 48, 0xff0000ff, 0.0f);  // z 0: in front of everything, drawn
	ClipQuad(40, 0, 64, 8, 0xffffffff, -0.01f);  // in front of the near plane: clipped
	Read();
	Expect(4, 4, 0xffff0000);
	Expect(20, 20, 0xff0000ff);
	Expect(44, 4, 0xff000000);
	End();
	Begin("depth-viewport-range");
	// The viewport's MinZ and MaxZ map z: a quad at z 0.9 in a viewport of 0..0.5 is at depth 0.45
	gGPU->SetRenderState(GPURS_ZENABLE, 1);
	gGPU->SetViewport(Rect(0, 0, W, H), 0.0f, 0.5f);
	ClipQuad(0, 0, 32, 32, 0xffff0000, 0.9f);
	gGPU->SetViewport(Rect(0, 0, W, H), 0.0f, 1.0f);
	ClipQuad(0, 0, 32, 32, 0xff00ff00, 0.6f);  // depth 0.6, behind
	ClipQuad(0, 0, 16, 16, 0xff0000ff, 0.4f);  // depth 0.4, in front
	Read();
	Expect(20, 20, 0xffff0000);
	Expect(4, 4, 0xff0000ff);
	End();
}

// Culling: D3D's default CULL_CCW drops triangles that are counter-clockwise on the screen (y down); CULL_CW the others
void TestCulling()
{
	Begin("cull-winding");
	auto Tri = [](float x, bool theClockwise, unsigned int theColor) {
		RHWVertex a = {x, 4, 0.5f, 1, theColor, 0, 0}, b = {x + 12, 4, 0.5f, 1, theColor, 0, 0},
			c = {x, 16, 0.5f, 1, theColor, 0, 0};
		RHWVertex v[3] = {a, theClockwise ? b : c, theClockwise ? c : b};  // a, b, c: right, then down-left: clockwise
		gGPU->DrawPrimitiveUP(GPUPT_TRIANGLELIST, 1, FVF_RHW, v, sizeof(RHWVertex));
	};
	gGPU->SetRenderState(GPURS_CULLMODE, 3);
	Tri(0, true, 0xffff0000);
	Tri(20, false, 0xffff0000);
	gGPU->SetRenderState(GPURS_CULLMODE, 2);
	Tri(40, true, 0xff00ff00);
	Tri(40, false, 0xff0000ff);
	Read();
	Expect(2, 6, 0xffff0000);
	Expect(22, 6, 0xff000000);
	Expect(42, 6, 0xff0000ff);
	End();
}

// Matrices are D3D's row-vector ones: a translation in row 3 moves the quad, and world * view * projection compose
// in that order
void TestMatrices()
{
	Begin("matrix-rows");
	GPUMatrix aWorld = GPUMatrix::Identity();
	aWorld.m[3][0] = 0.5f;   // a quarter of the screen right
	aWorld.m[3][1] = -0.25f; // an eighth down
	GPUMatrix aView = GPUMatrix::Identity();
	aView.m[0][0] = 0.5f;    // half as wide, after the world's translation
	gGPU->SetTransform(GPUTS_WORLD, aWorld);
	gGPU->SetTransform(GPUTS_VIEW, aView);
	// the unit square at the top left of clip space, 0..32 by 0..32 in pixels before the transforms
	XYZVertex v[4] = {{-1, 1, 0.5f, 0xffffff00}, {0, 1, 0.5f, 0xffffff00}, {-1, 0, 0.5f, 0xffffff00},
		{0, 0, 0.5f, 0xffffff00}};
	gGPU->DrawPrimitiveUP(GPUPT_TRIANGLESTRIP, 2, FVF_XYZ, v, sizeof(XYZVertex));
	Read();
	// x: (-1 + 0.5) * 0.5 = -0.25 -> 24, (0 + 0.5) * 0.5 = 0.25 -> 40; y: 1 - 0.25 = 0.75 -> 8, -0.25 -> 40
	ExpectBox(24, 8, 40, 40, 0xffffff00, 0xff000000);
	End();
}

// Textures: stored top row first (v 0 is the top), ARGB swizzled, point sampling exact; opaque images as R5G6B5 when the
// policy says so
void TestTextures(bool theOpaqueAs565)
{
	Begin(theOpaqueAs565 ? "texture-565" : "texture-orientation");
	std::vector<unsigned long> aBits(16);
	for (int i = 0; i < 16; i++)
		aBits[i] = 0xff000000 | (unsigned long)(i * 16) << 16 | (unsigned long)(255 - i * 16) << 8 | 0x56;
	MemoryImage* anImage = MakeImage(4, 4, aBits);
	if (!theOpaqueAs565) {
		anImage->GetBits()[0] = 0xfe000000 | (aBits[0] & 0xffffff);  // not opaque: A8R8G8B8
		aBits[0] = anImage->GetBits()[0];
		anImage->BitsChanged();
	}
	gGPU->SetTexture(0, anImage);
	gGPU->SetTextureStageState(0, GPUTSS_COLOROP, 4);
	gGPU->SetTextureStageState(0, GPUTSS_COLORARG1, 2);
	gGPU->SetTextureStageState(0, GPUTSS_COLORARG2, 0);
	gGPU->SetTextureStageState(0, GPUTSS_ALPHAARG1, 2);
	RHWQuad(7.5f, 7.5f, 15.5f, 15.5f, 0xffffffff);  // each texel 2 x 2 pixels, sampled off its edges
	Read();
	for (int i = 0; i < 16; i++) {
		unsigned long c = aBits[i];
		if (theOpaqueAs565) {
			unsigned int r = (c >> 16) & 0xff, g = (c >> 8) & 0xff, b = c & 0xff;
			r = ((r >> 3) << 3) | (r >> 5);
			g = ((g >> 2) << 2) | (g >> 6);
			b = ((b >> 3) << 3) | (b >> 5);
			c = 0xff000000 | r << 16 | g << 8 | b;
		}
		Expect(8 + (i % 4) * 2 + 1, 8 + (i / 4) * 2 + 1, c | 0xff000000);  // the screen has no alpha
	}
	gGPU->ImageDeleted(anImage);
	delete anImage;
	End();
}

// A render target: drawn into, read back, sampled the same way up, and copied with StretchRect
void TestRenderTarget()
{
	Begin("render-target");
	MemoryImage* aTarget = new MemoryImage();
	aTarget->Create(16, 8);
	if (!gGPU->CreateRenderTarget(aTarget, GPUFMT_A8R8G8B8))
		Fail("CreateRenderTarget failed");
	gGPU->SetRenderTarget(aTarget);
	gGPU->Clear(GPUCLEAR_TARGET, 0x80102030, 1.0f);
	RHWQuad(0, 0, 8, 4, 0xff00ff00);  // its top-left quarter
	Read(aTarget);
	Expect(0, 0, 0xff00ff00);
	Expect(15, 7, 0x80102030);  // a target keeps its alpha
	gGPU->SetRenderTarget(nullptr);
	gGPU->SetTexture(0, aTarget);
	gGPU->SetTextureStageState(0, GPUTSS_COLORARG1, 2);
	gGPU->SetTextureStageState(0, GPUTSS_ALPHAARG1, 2);
	RHWQuad(31.5f, 31.5f, 47.5f, 39.5f, 0xffffffff);  // texel centres, as SAF's -0.5
	gGPU->SetTexture(0, nullptr);
	gGPU->StretchRect(aTarget, Rect(0, 0, 16, 8), nullptr, Rect(0, 48, 32, 64), GPUTEXF_POINT);
	Read();
	Expect(32, 32, 0xff00ff00);
	Expect(47, 39, 0xff102030);
	Expect(40, 32, 0xff102030);
	Expect(0, 48, 0xff00ff00);
	Expect(15, 55, 0xff00ff00);
	Expect(16, 48, 0xff102030);
	Expect(31, 63, 0xff102030);
	gGPU->ImageDeleted(aTarget);
	delete aTarget;
	End();
}

// Blending with straight alpha, SRCALPHA / INVSRCALPHA
void TestBlend()
{
	Begin("blend");
	gGPU->Clear(GPUCLEAR_TARGET, 0xff0000ff, 1.0f);
	gGPU->SetRenderState(GPURS_ALPHABLENDENABLE, 1);
	gGPU->SetRenderState(GPURS_SRCBLEND, 5);
	gGPU->SetRenderState(GPURS_DESTBLEND, 6);
	RHWQuad(0, 0, 16, 16, 0x80ff0000);
	gGPU->SetRenderState(GPURS_SRCBLEND, 2);  // additive
	gGPU->SetRenderState(GPURS_DESTBLEND, 2);
	RHWQuad(16, 0, 32, 16, 0xff00ff00);
	Read();
	Expect(4, 4, 0xff80007f, 1);
	Expect(20, 4, 0xff00ffff);
	End();
}

// Alpha test: ALPHAREF against the alpha in 0..255, by its function
void TestAlphaTest()
{
	Begin("alpha-test");
	gGPU->SetRenderState(GPURS_ALPHATESTENABLE, 1);
	gGPU->SetRenderState(GPURS_ALPHAREF, 0x80);
	gGPU->SetRenderState(GPURS_ALPHAFUNC, 5);  // GREATER
	RHWQuad(0, 0, 8, 8, 0x80ffffff);
	RHWQuad(8, 0, 16, 8, 0x81ffffff);
	gGPU->SetRenderState(GPURS_ALPHAFUNC, 7);  // GREATEREQUAL
	RHWQuad(16, 0, 24, 8, 0x80ffffff);
	Read();
	Expect(4, 4, 0xff000000);
	Expect(12, 4, 0xffffffff);
	Expect(20, 4, 0xffffffff);
	End();
}

// Texture stages: TFACTOR, MODULATE2X, a second stage, the argument modifiers
void TestStages()
{
	Begin("texture-stages");
	gGPU->SetRenderState(GPURS_TEXTUREFACTOR, 0xff204060);
	gGPU->SetTextureStageState(0, GPUTSS_COLORARG1, 3);
	RHWQuad(0, 0, 8, 8, 0xffffffff);
	// MODULATE2X of the diffuse colour and TFACTOR
	gGPU->SetTextureStageState(0, GPUTSS_COLOROP, 5);
	gGPU->SetTextureStageState(0, GPUTSS_COLORARG2, 0);
	RHWQuad(8, 0, 16, 8, 0xff808080);
	// stage 1: ADD the complement of TFACTOR to stage 0's SELECTARG1 diffuse
	gGPU->SetTextureStageState(0, GPUTSS_COLOROP, 2);
	gGPU->SetTextureStageState(0, GPUTSS_COLORARG1, 0);
	gGPU->SetTextureStageState(1, GPUTSS_COLOROP, 7);
	gGPU->SetTextureStageState(1, GPUTSS_COLORARG1, 1);
	gGPU->SetTextureStageState(1, GPUTSS_COLORARG2, 3 | 0x10);
	gGPU->SetTextureStageState(1, GPUTSS_ALPHAOP, 2);
	gGPU->SetTextureStageState(1, GPUTSS_ALPHAARG1, 1);
	RHWQuad(16, 0, 24, 8, 0xff101010);
	Read();
	Expect(4, 4, 0xff204060);
	Expect(12, 4, 0xff204060, 1);
	Expect(20, 4, 0xffefcfaf);
	End();
}

// The scissor test limits draws and clears
void TestScissor()
{
	Begin("scissor");
	gGPU->SetScissorRect(Rect(4, 8, 20, 12));
	gGPU->SetRenderState(GPURS_SCISSORTESTENABLE, 1);
	RHWQuad(0, 0, 64, 64, 0xffffffff);
	gGPU->Clear(GPUCLEAR_TARGET, 0xff00ff00, 1.0f);
	gGPU->SetRenderState(GPURS_SCISSORTESTENABLE, 0);
	RHWQuad(0, 40, 8, 48, 0xffff0000);
	Read();
	ExpectBox(4, 8, 24, 20, 0xff00ff00, 0xff000000);
	Expect(4, 44, 0xffff0000);
	End();
}

// Fixed-function lighting: a directional light, the material's diffuse colour, the ambient state
void TestLighting()
{
	Begin("lighting");
	gGPU->SetRenderState(GPURS_LIGHTING, 1);
	gGPU->SetRenderState(GPURS_AMBIENT, 0xff202020);
	GPUMaterial aMaterial = {};
	aMaterial.mDiffuse = {0.5f, 0.25f, 0.75f, 1.0f};
	aMaterial.mAmbient = {1.0f, 0.0f, 0.0f, 1.0f};
	gGPU->SetMaterial(aMaterial);
	GPULight aLight = {};
	aLight.mType = 3;
	aLight.mDiffuse = {1, 1, 1, 1};
	aLight.mDirection = {0, 0, 1};  // into the screen, D3D's left-handed z
	gGPU->SetLight(0, aLight);
	gGPU->LightEnable(0, true);
	NormalVertex v[4] = {{-1, 1, 0.5f, 0, 0, -1}, {0, 1, 0.5f, 0, 0, -1}, {-1, 0, 0.5f, 0, 0, -1}, {0, 0, 0.5f, 0, 0, -1}};
	gGPU->DrawPrimitiveUP(GPUPT_TRIANGLESTRIP, 2, FVF_NORMAL, v, sizeof(NormalVertex));
	// facing away from the light: only the ambient
	for (NormalVertex& aVertex : v) {
		aVertex.x += 1.0f;
		aVertex.nz = 1.0f;
	}
	gGPU->DrawPrimitiveUP(GPUPT_TRIANGLESTRIP, 2, FVF_NORMAL, v, sizeof(NormalVertex));
	gGPU->LightEnable(0, false);
	Read();
	// diffuse 0.5, 0.25, 0.75 plus the ambient 0x20 times the material's ambient (red only)
	Expect(8, 8, 0xff9f40bf, 1);
	Expect(40, 8, 0xff200000, 1);
	End();
}

// A game shader pair: the naming contract, the constant banks, the position fix-up on oPos
void TestGameShaders()
{
	Begin("game-shaders");
	GPUVertexShader aVS = gGPU->CreateVertexShader("test_vs",
		"void main() {\n"
		"  oPos = vec4(aPosition.xyz, 1.0);\n"
		"  vDiffuse = aDiffuse * uVS[2];\n"
		"}\n");
	GPUPixelShader aPS = gGPU->CreatePixelShader("test_ps",
		"void main() {\n"
		"  oC0 = vDiffuse + uPS[1];\n"
		"}\n");
	gGPU->SetVertexShader(aVS);
	gGPU->SetPixelShader(aPS);
	float aVSConstant[4] = {1.0f, 0.0f, 1.0f, 1.0f};
	float aPSConstant[4] = {0.0f, 0.0f, 64.0f / 255.0f, 0.0f};
	gGPU->SetVertexShaderConstantF(2, aVSConstant, 1);
	gGPU->SetPixelShaderConstantF(1, aPSConstant, 1);
	ClipQuad(10, 5, 20, 15, 0xff80ff00);
	gGPU->SetVertexShader(0);
	gGPU->SetPixelShader(0);
	Read();
	ExpectBox(10, 5, 20, 15, 0xff800040, 0xff000000);
	End();
}

// Static buffers: an indexed draw from a vertex and an index buffer, with a start index
void TestBuffers()
{
	Begin("indexed-buffers");
	XYZVertex v[4] = {{-1, 1, 0.5f, 0xff00ffff}, {0, 1, 0.5f, 0xff00ffff}, {-1, 0, 0.5f, 0xff00ffff},
		{0, 0, 0.5f, 0xff00ffff}};
	unsigned short anIndices[9] = {0, 0, 0, 0, 1, 2, 1, 3, 2};
	GPUBuffer aVertices = gGPU->CreateVertexBuffer(FVF_XYZ, v, 4, sizeof(XYZVertex));
	GPUBuffer anIndexBuffer = gGPU->CreateIndexBuffer(anIndices, 9);
	gGPU->DrawIndexedPrimitive(GPUPT_TRIANGLELIST, aVertices, anIndexBuffer, 3, 2);
	gGPU->DeleteBuffer(aVertices);
	gGPU->DeleteBuffer(anIndexBuffer);
	Read();
	ExpectBox(0, 0, 32, 32, 0xff00ffff, 0xff000000);
	End();
}

// A program key saved by one device compiles ahead on another
void TestPrecompile(GLContext* theContext, const GPUPolicy& thePolicy)
{
	Begin("precompile");
	std::vector<std::string> aKeys = gGPU->GetUsedProgramKeys();
	if (aKeys.size() < 5)
		Fail("only %d program keys were listed", (int)aKeys.size());
	GPUInterface* aSecond = GPUInterface::Create(theContext, thePolicy, W, H);
	aSecond->CreateVertexShader("test_vs", "void main() { oPos = vec4(aPosition.xyz, 1.0); }\n");
	aSecond->CreatePixelShader("test_ps", "void main() { oC0 = vDiffuse; }\n");
	aSecond->Precompile(aKeys);
	if (aSecond->GetUsedProgramKeys().size() != aKeys.size())
		Fail("%d of %d keys compiled ahead", (int)aSecond->GetUsedProgramKeys().size(), (int)aKeys.size());
	delete aSecond;
	End();
}

// An opaque target (X8R8G8B8, GL RGB8) reads back and samples alpha 1 whatever is drawn or cleared into it
void TestOpaqueTarget()
{
	Begin("opaque-target-alpha");
	MemoryImage* anOpaque = new MemoryImage();
	anOpaque->Create(8, 8);
	MemoryImage* aClear = new MemoryImage();
	aClear->Create(8, 8);
	gGPU->CreateRenderTarget(anOpaque, GPUFMT_X8R8G8B8);
	gGPU->CreateRenderTarget(aClear, GPUFMT_A8R8G8B8);
	gGPU->SetRenderTarget(anOpaque);
	gGPU->Clear(GPUCLEAR_TARGET, 0x40102030, 1.0f);
	RHWQuad(0, 0, 4, 8, 0x20ff0000);
	Read(anOpaque);
	Expect(1, 1, 0xffff0000);
	Expect(6, 6, 0xff102030);
	// sampled into an A8R8G8B8 target with the texture's alpha
	gGPU->SetRenderTarget(aClear);
	gGPU->Clear(GPUCLEAR_TARGET, 0x00000000, 1.0f);
	gGPU->SetTexture(0, anOpaque);
	gGPU->SetTextureStageState(0, GPUTSS_COLORARG1, 2);
	gGPU->SetTextureStageState(0, GPUTSS_ALPHAARG1, 2);
	RHWQuad(-0.5f, -0.5f, 7.5f, 7.5f, 0xffffffff);
	gGPU->SetTexture(0, nullptr);
	Read(aClear);
	Expect(1, 1, 0xffff0000);
	Expect(6, 6, 0xff102030);
	gGPU->SetRenderTarget(nullptr);
	gGPU->ImageDeleted(anOpaque);
	gGPU->ImageDeleted(aClear);
	delete anOpaque;
	delete aClear;
	End();
}

// A target's bits: an A8R8G8B8 target starts from them and takes them again when they change; an X8R8G8B8 one does
// not; GetBits on a purged target reads it back through the hook
void TestTargetBits()
{
	Begin("target-bits");
	MemoryImage* aTarget = new MemoryImage();
	aTarget->Create(4, 4);
	for (int i = 0; i < 16; i++)
		aTarget->GetBits()[i] = 0x80112233;
	aTarget->BitsChanged();
	MemoryImage* anOpaque = new MemoryImage();
	anOpaque->Create(4, 4);
	gGPU->CreateRenderTarget(aTarget, GPUFMT_A8R8G8B8);
	gGPU->CreateRenderTarget(anOpaque, GPUFMT_X8R8G8B8);
	Read(aTarget);
	Expect(2, 2, 0x80112233);
	// changed bits reach the A8R8G8B8 target at its next use
	aTarget->GetBits()[0] = 0xff445566;
	aTarget->BitsChanged();
	gGPU->SetRenderTarget(aTarget);
	gGPU->SetRenderTarget(nullptr);
	Read(aTarget);
	Expect(0, 0, 0xff445566);
	// and not the X8R8G8B8 one
	gGPU->SetRenderTarget(anOpaque);
	gGPU->Clear(GPUCLEAR_TARGET, 0xff00ff00, 1.0f);
	for (int i = 0; i < 16; i++)
		anOpaque->GetBits()[i] = 0xffff0000;
	anOpaque->BitsChanged();
	gGPU->SetRenderTarget(nullptr);
	gGPU->SetRenderTarget(anOpaque);
	Read(anOpaque);
	Expect(3, 3, 0xff00ff00);
	// GetBits on a purged target
	gGPU->SetRenderTarget(aTarget);
	gGPU->Clear(GPUCLEAR_TARGET, 0x7f0000ff, 1.0f);
	gGPU->SetRenderTarget(nullptr);
	delete[] aTarget->mBits;
	aTarget->mBits = nullptr;
	static GPUInterface* sGPU;
	sGPU = gGPU;
	gMemoryImageRecoverBits = [](MemoryImage* theImage) { return sGPU->RecoverBits(theImage); };
	unsigned long* aBits = aTarget->GetBits();
	gMemoryImageRecoverBits = nullptr;
	if (aBits[5] != 0x7f0000ff)
		Fail("recovered bits %08lx, expected 7f0000ff", aBits[5]);
	gGPU->ImageDeleted(aTarget);
	gGPU->ImageDeleted(anOpaque);
	delete aTarget;
	delete anOpaque;
	End();
}

// The policy's format rule, 0x61c070's order (Twist's device takes A8R8G8B8, A4R4G4B4 and R5G6B5)
void TestFormatRule(const GPUPolicy& thePolicy)
{
	gTest = "format-rule";
	gTestFailed = false;
	struct Case { bool mAlpha, mTrans; unsigned long mFlags; GPUFormat mFormat; } aCases[] = {
		{true, false, 0, GPUFMT_A8R8G8B8},
		{false, true, 0, GPUFMT_A8R8G8B8},
		{false, false, 0, GPUFMT_R5G6B5},
		{false, false, 8, GPUFMT_A8R8G8B8},     // UseA8R8G8B8
		{false, false, 0x10, GPUFMT_X8R8G8B8},  // an opaque target
		{true, false, 0x10, GPUFMT_A8R8G8B8},   // a target with alpha
		{false, false, 4, GPUFMT_R5G6B5},       // UseA4R4G4B4 on an opaque image: R5G6B5 comes first
		{true, false, 4, GPUFMT_A4R4G4B4},
		{false, false, 8 | 4, GPUFMT_A4R4G4B4},
	};
	for (const Case& aCase : aCases) {
		GPUFormat aFormat = thePolicy.TextureFormat(aCase.mAlpha, aCase.mTrans, aCase.mFlags);
		if (aFormat != aCase.mFormat)
			Fail("alpha %d trans %d flags %lx: format %d, expected %d", aCase.mAlpha, aCase.mTrans, aCase.mFlags,
				aFormat, aCase.mFormat);
	}
	End();
}

// The scene's rules, the same on the GL and the null device: only PreDraw's callers begin a scene, which rebinds the
// screen; SetRenderTarget's answers; EndScene unbinds the shaders; the caps
void TestSceneRules(GPUInterface* theGPU, const char* theName)
{
	gTest = std::string("scene-rules-") + theName;
	gTestFailed = false;
	MemoryImage a, b, c;
	a.Create(4, 4);
	b.Create(4, 4);
	c.Create(4, 4);
	theGPU->CreateRenderTarget(&a, GPUFMT_A8R8G8B8);
	theGPU->CreateRenderTarget(&b, GPUFMT_A8R8G8B8);
	theGPU->EndScene();
	theGPU->SetSceneAllowed(true);
	int aStart = theGPU->GetSceneCount();
	RHWVertex v[3] = {{0, 0, 0.5f, 1, 0xffffffff, 0, 0}, {4, 0, 0.5f, 1, 0xffffffff, 0, 0},
		{0, 4, 0.5f, 1, 0xffffffff, 0, 0}};
	theGPU->DrawPrimitiveUP(GPUPT_TRIANGLELIST, 1, FVF_RHW, v, sizeof(RHWVertex));
	theGPU->Clear(GPUCLEAR_TARGET, 0xff000000, 1.0f);
	theGPU->SetRenderState(GPURS_ZENABLE, 0);
	if (theGPU->GetSceneCount() != aStart)
		Fail("a draw, a Clear or a state began a scene");
	if (!theGPU->SetRenderTarget(&a) || theGPU->GetRenderTarget() != &a || theGPU->GetSceneCount() != aStart + 1)
		Fail("SetRenderTarget(a) did not begin a scene on a");
	GPUPixelShader aShader = theGPU->CreatePixelShader("scene_ps", "void main() { oC0 = vDiffuse; }\n");
	theGPU->SetPixelShader(aShader);
	theGPU->EndScene();
	if (theGPU->GetPixelShader() != 0)
		Fail("EndScene left the pixel shader bound");
	theGPU->SetVertexShader(0);
	if (theGPU->GetSceneCount() != aStart + 2 || theGPU->GetRenderTarget() != nullptr)
		Fail("SetVertexShader did not begin a scene on the screen");
	if (theGPU->SetRenderTarget(&c))
		Fail("SetRenderTarget succeeded on an image that is not a target");
	theGPU->SetRenderTarget(&a);
	theGPU->SetSceneAllowed(false);
	theGPU->EndScene();
	if (theGPU->SetRenderTarget(nullptr))
		Fail("SetRenderTarget(null) succeeded while minimised");
	if (!theGPU->SetRenderTarget(&b))
		Fail("SetRenderTarget(b) failed while minimised");
	if (theGPU->GetRenderTarget() != &a || theGPU->GetSceneCount() != aStart + 2)
		Fail("the target or the scene changed while minimised");
	theGPU->SetSceneAllowed(true);
	if (theGPU->GetCaps(0) != 0x7f || theGPU->GetCaps(1) != 8)
		Fail("caps %lx and %lu", theGPU->GetCaps(0), theGPU->GetCaps(1));
	theGPU->EndScene();
	theGPU->SetRenderTarget(nullptr);
	theGPU->ImageDeleted(&a);
	theGPU->ImageDeleted(&b);
	theGPU->ImageDeleted(&c);
	End();
}

}  // namespace

int main(int argc, char** argv)
{
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	for (int i = 1; i < argc; i++) {
		if (std::strcmp(argv[i], "-v") == 0)
			gVerbose = true;
	}
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::printf("gpu: SDL_Init failed: %s\n", SDL_GetError());
		return 2;
	}
	SDL_Window* aWindow = SDL_CreateWindow("gputest", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W, H,
		SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
	GLContext* aContext = aWindow != nullptr ? GLContext::Create(aWindow) : nullptr;
	if (aContext == nullptr) {
		std::printf("gpu: no GL context\n");
		return 2;
	}
	// MemoryImage registers with an app; one that is never initialised is enough
	new SexyAppBase();
	gRead = new MemoryImage();
	GPUPolicy aPolicy = GPUPolicy::Twist();
	aPolicy.mSceneBegin.clear();
	gGPU = GPUInterface::Create(aContext, aPolicy, W, H);
	if (gGPU == nullptr) {
		std::printf("gpu: GPUInterface::Create failed\n");
		return 2;
	}
	TestClearViewport();
	TestPretransformedCentres();
	TestTransformedCentres();
	TestDepth();
	TestCulling();
	TestMatrices();
	TestTextures(false);
	TestRenderTarget();
	TestOpaqueTarget();
	TestTargetBits();
	TestBlend();
	TestAlphaTest();
	TestStages();
	TestScissor();
	TestLighting();
	TestGameShaders();
	TestBuffers();
	TestPrecompile(aContext, aPolicy);
	TestSceneRules(gGPU, "gl");
	delete gGPU;
	gGPU = GPUInterface::Create(aContext, aPolicy, W, H);
	TestTextures(true);
	delete gGPU;
	GPUInterface* aNull = GPUInterface::CreateNull(aPolicy, W, H);
	TestSceneRules(aNull, "null");
	delete aNull;
	TestFormatRule(aPolicy);
	std::printf("gpu: %d tests, %d failed (%s)\n", gTests, gFailed, aContext->Description().c_str());
	delete aContext;
	SDL_DestroyWindow(aWindow);
	SDL_Quit();
	return gFailed == 0 ? 0 : 1;
}
