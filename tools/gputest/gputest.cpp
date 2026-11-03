// GPUInterface test (framework/GPUInterface.h): one test for each Direct3D 9 convention the GL implementation converts,
// on the real GPU, with exact pixels (±1 where GL may round a blend or a lit colour the other way). Native only; it opens
// a hidden GL window, so tools/check.sh unit runs it under rundesk.
//
//   gputest [-v]
//
// Each failed check prints one "FAIL <test>: ..." line. The last line is "gpu: N tests, M failed (<GL description>)".
// Exit status 0 only when every test passed. -v also prints each test's name as it passes.

#include "framework/GPU2D.h"
#include "framework/GPUInterface.h"
#include "framework/Graphics.h"
#include "framework/MemoryImage.h"
#include "framework/SexyAppBase.h"
#include "framework/SexyMatrix.h"
#include "framework/TriVertex.h"
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

void RHWQuad(float l, float t, float r, float b, unsigned int theColor, float z = 0.5f, float theMaxUV = 1.0f)
{
	float m = theMaxUV;
	RHWVertex v[4] = {{l, t, z, 1, theColor, 0, 0}, {r, t, z, 1, theColor, m, 0}, {l, b, z, 1, theColor, 0, m},
		{r, b, z, 1, theColor, m, m}};
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
	// each texel 2 x 2 pixels, sampled off its edges; the 4x4 image is an 8x8 texture (the minimum piece), its padding
	// right and below
	RHWQuad(7.5f, 7.5f, 15.5f, 15.5f, 0xffffffff, 0.5f, 0.5f);
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

struct CubeVertex
{
	float x, y, z, u, v, w;
};
const unsigned long FVF_CUBE = GPUFVF_XYZ | GPUFVF_TEX1 | 0x10000;  // set 0 with three components (D3DFVF_TEXCOORDSIZE3(0))

// A cube map (an image with flag 0x20, 6x1 cells; 0x61ad60, 0x61b740): cell i goes to D3D face {5, 1, 4, 0, 3, 2}[i] (the
// faces +X, -X, +Y, -Y, +Z, -Z), each face's rows top row first, and a stage with one samples with its texture
// coordinate as a direction. Asserts, with point sampling: (1) each of the six axis directions gives the colour of the
// cell the table puts on that face; (2) D3D9's face orientation on the -X face, where the texture's v grows toward -Y
// (the split cell's top rows show toward +Y); (3) TEXCOORDINDEX 0x30000, the camera-space reflection vector of a ray
// along +Z (not a local viewer) off a quad's normal, picks the face of that vector's largest component.
void TestCubeMap()
{
	Begin("cube-map");
	const unsigned long kCells[6] = {0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffff00, 0xff00ffff, 0xffff00ff};
	const unsigned long kSplitTop = 0xff00ff00, kSplitBottom = 0xffff8000;  // cell 1 (the -X face) is split
	const int kFace[6] = {5, 1, 4, 0, 3, 2};
	int aCell[6];  // the cell on each face
	for (int i = 0; i < 6; i++)
		aCell[kFace[i]] = i;
	// six cells of 4 x 4 in a row
	std::vector<unsigned long> aBits(24 * 4);
	for (int y = 0; y < 4; y++)
		for (int i = 0; i < 6; i++)
			for (int x = 0; x < 4; x++)
				aBits[y * 24 + i * 4 + x] = i == 1 ? (y < 2 ? kSplitTop : kSplitBottom) : kCells[i];
	MemoryImage* anImage = MakeImage(24, 4, aBits);
	anImage->mNumCols = 6;
	anImage->mNumRows = 1;
	anImage->mD3DFlags = 0x20 | 0x8;
	anImage->BitsChanged();
	gGPU->SetTexture(0, anImage);
	gGPU->SetTextureStageState(0, GPUTSS_COLOROP, 2);
	gGPU->SetTextureStageState(0, GPUTSS_COLORARG1, 2);
	gGPU->SetTextureStageState(0, GPUTSS_ALPHAOP, 2);
	gGPU->SetTextureStageState(0, GPUTSS_ALPHAARG1, 2);
	gGPU->SetTextureStageState(0, GPUTSS_TEXCOORDINDEX, 0);
	gGPU->SetRenderState(GPURS_LOCALVIEWER, 0);
	auto X = [](float s) { return s * 2.0f / W - 1.0f; };
	auto Y = [](float s) { return 1.0f - s * 2.0f / H; };
	// (1) a direction for each face, +X, -X, +Y, -Y, +Z, -Z, in a 10 x 10 quad each
	// (the -X face's y is 0.25, off the boundary between the split cell's halves)
	const float kDirections[6][3] = {{1, 0, 0}, {-1, 0.25f, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
	for (int f = 0; f < 6; f++) {
		const float* d = kDirections[f];
		float l = X(2.0f + f * 10), r = X(12.0f + f * 10), t = Y(0), b = Y(10);
		CubeVertex v[4] = {{l, t, 0.5f, d[0], d[1], d[2]}, {r, t, 0.5f, d[0], d[1], d[2]}, {l, b, 0.5f, d[0], d[1], d[2]},
			{r, b, 0.5f, d[0], d[1], d[2]}};
		gGPU->DrawPrimitiveUP(GPUPT_TRIANGLESTRIP, 2, FVF_CUBE, v, sizeof(CubeVertex));
	}
	// (2) the -X face over a quad whose direction runs from y = +0.5 at the top to -0.5 at the bottom
	{
		float l = X(0), r = X(32), t = Y(12), b = Y(44);
		CubeVertex v[4] = {{l, t, 0.5f, -1, 0.5f, 0}, {r, t, 0.5f, -1, 0.5f, 0}, {l, b, 0.5f, -1, -0.5f, 0},
			{r, b, 0.5f, -1, -0.5f, 0}};
		gGPU->DrawPrimitiveUP(GPUPT_TRIANGLESTRIP, 2, FVF_CUBE, v, sizeof(CubeVertex));
	}
	// (3) three quads facing the camera at different normals (camera space; the ray is along +Z):
	// (0, 0, -1) reflects to -Z; (0.6, 0, -0.8) to (0.96, 0, -0.28), +X; (0, -0.6, -0.8) to (0, -0.96, -0.28), -Y
	gGPU->SetTextureStageState(0, GPUTSS_TEXCOORDINDEX, 0x30000);
	const float kNormals[3][3] = {{0, 0, -1}, {0.6f, 0, -0.8f}, {0, -0.6f, -0.8f}};
	for (int q = 0; q < 3; q++) {
		const float* n = kNormals[q];
		float l = X(34.0f + q * 10), r = X(44.0f + q * 10), t = Y(12), b = Y(22);
		NormalVertex v[4] = {{l, t, 0.5f, n[0], n[1], n[2]}, {r, t, 0.5f, n[0], n[1], n[2]}, {l, b, 0.5f, n[0], n[1], n[2]},
			{r, b, 0.5f, n[0], n[1], n[2]}};
		gGPU->DrawPrimitiveUP(GPUPT_TRIANGLESTRIP, 2, FVF_NORMAL, v, sizeof(NormalVertex));
	}
	gGPU->SetTextureStageState(0, GPUTSS_TEXCOORDINDEX, 0);
	Read();
	for (int f = 0; f < 6; f++)
		Expect(7 + f * 10, 5, f == 1 ? kSplitTop : kCells[aCell[f]]);
	Expect(16, 16, kSplitTop);
	Expect(16, 26, kSplitTop);
	Expect(16, 30, kSplitBottom);
	Expect(16, 40, kSplitBottom);
	Expect(39, 17, kCells[aCell[5]]);
	Expect(49, 17, kCells[aCell[0]]);
	Expect(59, 17, kCells[aCell[3]]);
	gGPU->SetTexture(0, nullptr);
	gGPU->ImageDeleted(anImage);
	delete anImage;
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

// Lines as SAF's DrawLine draws them: a strip from the start to the end and on to the end plus half a pixel, so both
// end pixels are lit (D3D9's diamond-exit rule, which GL's lines follow on the drivers tested)
void TestLines()
{
	Begin("line-strip");
	RHWVertex v[3] = {{4, 10, 0.5f, 1, 0xffffffff, 0, 0}, {14, 10, 0.5f, 1, 0xffffffff, 0, 0},
		{14.5f, 10.5f, 0.5f, 1, 0xffffffff, 0, 0}};
	gGPU->DrawPrimitiveUP(GPUPT_LINESTRIP, 2, FVF_RHW, v, sizeof(RHWVertex));
	RHWVertex w[2] = {{20, 2, 0.5f, 1, 0xffff0000, 0, 0}, {20, 12, 0.5f, 1, 0xffff0000, 0, 0}};
	gGPU->DrawPrimitiveUP(GPUPT_LINELIST, 1, FVF_RHW, w, sizeof(RHWVertex));
	Read();
	Expect(4, 10, 0xffffffff);
	Expect(9, 10, 0xffffffff);
	Expect(14, 10, 0xffffffff);
	Expect(3, 10, 0xff000000);
	Expect(15, 10, 0xff000000);
	Expect(9, 9, 0xff000000);
	Expect(9, 11, 0xff000000);
	Expect(20, 2, 0xffff0000);
	Expect(20, 11, 0xffff0000);
	Expect(20, 12, 0xffff0000);  // LASTPIXEL, TRUE by default
	Expect(20, 13, 0xff000000);
	End();

	Begin("line-no-last-pixel");
	gGPU->SetRenderState(GPURS_LASTPIXEL, 0);
	gGPU->DrawPrimitiveUP(GPUPT_LINELIST, 1, FVF_RHW, w, sizeof(RHWVertex));
	gGPU->SetRenderState(GPURS_LASTPIXEL, 1);
	Read();
	Expect(20, 11, 0xffff0000);
	Expect(20, 12, 0xff000000);
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
// --- GPU2D (framework/GPU2D.h): the 2D draws on the device, as Twist's D3D9Interface makes them ---

// A fresh GPU2D after Reset (its state cache then knows nothing), with Twist's stage 0 (MODULATE texture by diffuse)
// and blending on, as its scene-begin list sets them
GPU2D* Begin2D(const char* theName, const GPUPolicy& thePolicy)
{
	Begin(theName);
	GPU2D* aGPU2D = new GPU2D(gGPU, thePolicy);
	aGPU2D->SetScreenSize(W, H);
	aGPU2D->SetRenderState(GPURS_ALPHABLENDENABLE, 1);
	aGPU2D->SetTextureStageState(0, GPUTSS_COLOROP, 4);
	aGPU2D->SetTextureStageState(0, GPUTSS_COLORARG1, 2);
	aGPU2D->SetTextureStageState(0, GPUTSS_COLORARG2, 0);
	aGPU2D->SetTextureStageState(0, GPUTSS_ALPHAOP, 4);
	aGPU2D->SetTextureStageState(0, GPUTSS_ALPHAARG1, 2);
	aGPU2D->SetTextureStageState(0, GPUTSS_ALPHAARG2, 0);
	return aGPU2D;
}

void End2D(GPU2D* theGPU2D)
{
	theGPU2D->EndScene();
	delete theGPU2D;
	End();
}

// Blt: point sampled, -0.5 so each texel lands on one pixel; a half-transparent texel blends over black
void Test2DBlt(const GPUPolicy& thePolicy)
{
	GPU2D* g = Begin2D("2d-blt", thePolicy);
	std::vector<unsigned long> aBits(16);
	for (int i = 0; i < 16; i++)
		aBits[i] = 0xff000000 | (i * 16) << 16 | (255 - i * 16) << 8 | 0x40;
	aBits[5] = 0x80ff0000;
	MemoryImage* anImage = MakeImage(4, 4, aBits);
	g->Blt(anImage, 10, 20, Rect(0, 0, 4, 4), Color::White, Graphics::DRAWMODE_NORMAL);
	g->Flush();
	Read();
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			if (y * 4 + x != 5)
				Expect(10 + x, 20 + y, aBits[y * 4 + x]);
		}
	}
	Expect(11, 21, 0xff800000, 1);
	Expect(9, 20, 0xff000000);
	Expect(14, 20, 0xff000000);
	Expect(10, 19, 0xff000000);
	Expect(10, 24, 0xff000000);
	End2D(g);
	gGPU->ImageDeleted(anImage);
	delete anImage;
}

// FillRect: exactly its rectangle; additive adds
void Test2DFillRect(const GPUPolicy& thePolicy)
{
	GPU2D* g = Begin2D("2d-fillrect", thePolicy);
	g->FillRect(Rect(5, 6, 10, 4), Color(0, 255, 0), Graphics::DRAWMODE_NORMAL);
	g->FillRect(Rect(30, 30, 4, 4), Color(0, 0, 128), Graphics::DRAWMODE_NORMAL);
	g->FillRect(Rect(30, 30, 2, 2), Color(0, 0, 64), Graphics::DRAWMODE_ADDITIVE);
	g->Flush();
	Read();
	ExpectBox(5, 6, 15, 10, 0xff00ff00, 0xff000000);
	Expect(30, 30, 0xff0000c0);
	Expect(32, 32, 0xff000080);
	End2D(g);
}

// DrawLine: Twist draws two vertices and SAF 1.3 a strip of three; both light the end pixel through LASTPIXEL
void Test2DLine(const GPUPolicy& thePolicy)
{
	GPU2D* g = Begin2D("2d-line", thePolicy);
	g->DrawLine(4, 10, 14, 10, Color::White, Graphics::DRAWMODE_NORMAL);
	Read();
	Expect(4, 10, 0xffffffff);
	Expect(13, 10, 0xffffffff);
	Expect(14, 10, 0xffffffff);
	Expect(15, 10, 0xff000000);
	Expect(3, 10, 0xff000000);
	End2D(g);

	GPUPolicy aSAF = thePolicy;
	aSAF.mLineEndPixel = true;
	g = Begin2D("2d-line-saf", aSAF);
	g->DrawLine(4, 10, 14, 10, Color::White, Graphics::DRAWMODE_NORMAL);
	Read();
	Expect(14, 10, 0xffffffff);
	Expect(15, 10, 0xff000000);
	End2D(g);
}

// The batch: draws with the same states share one draw call; a changed texture or state flushes, the same value does
// not; a line flushes and draws at once
void Test2DBatch(const GPUPolicy& thePolicy)
{
	GPU2D* g = Begin2D("2d-batch", thePolicy);
	MemoryImage* a = MakeImage(2, 2, std::vector<unsigned long>(4, 0xffffffff));
	MemoryImage* b = MakeImage(2, 2, std::vector<unsigned long>(4, 0xffff0000));
	g->Blt(a, 0, 0, Rect(0, 0, 2, 2), Color::White, Graphics::DRAWMODE_NORMAL);
	g->Blt(a, 4, 0, Rect(0, 0, 2, 2), Color::White, Graphics::DRAWMODE_NORMAL);
	g->Blt(a, 8, 0, Rect(0, 0, 2, 2), Color::White, Graphics::DRAWMODE_NORMAL);
	if (g->GetDrawCallCount() != 0)
		Fail("three Blts drew %d times before a flush", g->GetDrawCallCount());
	g->Blt(b, 12, 0, Rect(0, 0, 2, 2), Color::White, Graphics::DRAWMODE_NORMAL);
	if (g->GetDrawCallCount() != 1)
		Fail("a new texture made %d draws, expected 1", g->GetDrawCallCount());
	g->SetRenderState(GPURS_ALPHABLENDENABLE, 1);
	if (g->GetDrawCallCount() != 1)
		Fail("setting a state to its value flushed");
	g->SetRenderState(GPURS_DITHERENABLE, 1);
	if (g->GetDrawCallCount() != 2)
		Fail("a changed state did not flush (%d draws)", g->GetDrawCallCount());
	g->FillRect(Rect(0, 4, 2, 2), Color::White, Graphics::DRAWMODE_NORMAL);
	g->DrawLine(0, 8, 4, 8, Color::White, Graphics::DRAWMODE_NORMAL);
	if (g->GetDrawCallCount() != 4)
		Fail("a line did not flush and draw (%d draws)", g->GetDrawCallCount());
	Read();
	Expect(8, 0, 0xffffffff);
	Expect(12, 0, 0xffff0000);
	Expect(0, 4, 0xffffffff);
	Expect(1, 8, 0xffffffff);
	End2D(g);
	gGPU->ImageDeleted(a);
	delete a;
	gGPU->ImageDeleted(b);
	delete b;
}

// BltF and the transform stack: linear, translated, and clipped in software to the clip rectangle's pixels
void Test2DTransformed(const GPUPolicy& thePolicy)
{
	GPU2D* g = Begin2D("2d-transformed", thePolicy);
	MemoryImage* anImage = MakeImage(8, 8, std::vector<unsigned long>(64, 0xffffffff));
	Rect aClip(12, 12, 4, 4);
	g->BltF(anImage, 10, 10, Rect(0, 0, 8, 8), &aClip, Color::White, Graphics::DRAWMODE_NORMAL);
	SexyTransform2D aTransform;
	aTransform.Translate(30, 2);
	g->PushTransform(aTransform);
	g->Blt(anImage, 0, 0, Rect(0, 0, 8, 8), Color(255, 0, 0), Graphics::DRAWMODE_NORMAL);
	g->PopTransform();
	g->Flush();
	Read();
	ExpectBox(12, 12, 16, 16, 0xffffffff, 0xff000000);
	ExpectBox(30, 2, 38, 10, 0xffff0000, 0xff000000);
	End2D(g);
	gGPU->ImageDeleted(anImage);
	delete anImage;
}

// Depth-write mode 1 is a depth-only pass: ZERO / ONE leaves the colour as it was
void Test2DDepthOnly(const GPUPolicy& thePolicy)
{
	GPU2D* g = Begin2D("2d-depth-only", thePolicy);
	g->SetDepthWriteMode(1);
	g->FillRect(Rect(0, 0, 8, 8), Color::White, Graphics::DRAWMODE_NORMAL);
	g->SetDepthWriteMode(0);
	g->FillRect(Rect(8, 0, 8, 8), Color::White, Graphics::DRAWMODE_NORMAL);
	g->Flush();
	Read();
	Expect(2, 2, 0xff000000);
	Expect(10, 2, 0xffffffff);
	End2D(g);
}

// DrawTrianglesTex: no 0.5, so a quad from 20 to 24 covers pixels 20 to 23; the vertex colour wins over the draw's
void Test2DTrianglesTex(const GPUPolicy& thePolicy)
{
	GPU2D* g = Begin2D("2d-triangles-tex", thePolicy);
	MemoryImage* anImage = MakeImage(4, 4, std::vector<unsigned long>(16, 0xffffffff));
	TriVertex aTris[2][3] = {
		{TriVertex(20, 20, 0, 0), TriVertex(24, 20, 1, 0), TriVertex(20, 24, 0, 1)},
		{TriVertex(24, 20, 1, 0), TriVertex(24, 24, 1, 1), TriVertex(20, 24, 0, 1)}};
	g->DrawTrianglesTex(anImage, aTris, 2, Color::White, Graphics::DRAWMODE_NORMAL, 0, 0, false);
	TriVertex aGreen[1][3] = {{TriVertex(40, 40, 0, 0, 0xff00ff00), TriVertex(48, 40, 1, 0, 0xff00ff00),
		TriVertex(40, 48, 0, 1, 0xff00ff00)}};
	g->DrawTrianglesTex(anImage, aGreen, 1, Color(255, 0, 0), Graphics::DRAWMODE_NORMAL, 0, 0, false);
	Read();
	ExpectBox(20, 20, 24, 24, 0xffffffff, 0xff000000);
	Expect(41, 41, 0xff00ff00);
	End2D(g);
	gGPU->ImageDeleted(anImage);
	delete anImage;
}

// The draw hook (this+0x10): null draws as before; false drops the primitive (and BltTransformed's whole call); a returned
// rectangle clips the draw to it. The arguments are the original's: type 4 / count 1 for a triangle, 5 / 2 for a quad
struct HookState
{
	int mCalls = 0, mType = 0, mCount = 0, mStride = 0;
	bool mResult = true;
	Rect* mClip = nullptr;  // set into the rectangle pointer when not null
};

bool TestHook(void* theContext, int thePrimType, int theCount, const GPU2DVertex* theVertices, int theStride, Rect** theClip)
{
	HookState* aState = (HookState*)theContext;
	aState->mCalls++;
	aState->mType = thePrimType;
	aState->mCount = theCount;
	aState->mStride = theStride;
	if (theVertices == nullptr)
		Fail("hook: no vertices");
	if (aState->mClip != nullptr)
		*theClip = aState->mClip;
	return aState->mResult;
}

void ExpectHookCall(const HookState& theState, int theCalls, int theType, int theCount)
{
	if (theState.mCalls != theCalls)
		Fail("hook called %d times, expected %d", theState.mCalls, theCalls);
	else if ((theCalls > 0) && ((theState.mType != theType) || (theState.mCount != theCount) || (theState.mStride != 0x20)))
		Fail("hook got type %d count %d stride %d, expected %d, %d, 32", theState.mType, theState.mCount, theState.mStride,
			theType, theCount);
}

void Test2DDrawHook(const GPUPolicy& thePolicy)
{
	MemoryImage* anImage = MakeImage(8, 8, std::vector<unsigned long>(64, 0xffffffff));
	TriVertex aTri[1][3] = {{TriVertex(4, 4, 0, 0), TriVertex(28, 4, 1, 0), TriVertex(4, 28, 0, 1)}};
	TriVertex aP1(4, 4), aP2(28, 4), aP3(4, 28);
		for (int aCase = 0; aCase < 3; aCase++) {
		// 0: no hook; 1: false; 2: a rectangle
		HookState aState;
		aState.mResult = (aCase != 1);
		Rect aHookRect(0, 0, 12, 12);
		if (aCase == 2)
			aState.mClip = &aHookRect;
		const char* aNames[3] = {"2d-hook-null", "2d-hook-drop", "2d-hook-clip"};
		GPU2D* g = Begin2D(aNames[aCase], thePolicy);
		if (aCase > 0)
			g->SetDrawHook(TestHook, &aState);

		g->DrawTriangle(aP1, aP2, aP3, Color(255, 0, 0), Graphics::DRAWMODE_NORMAL);
		ExpectHookCall(aState, aCase > 0 ? 1 : 0, 4, 1);
		g->DrawTrianglesTex(anImage, aTri, 1, Color(0, 255, 0), Graphics::DRAWMODE_NORMAL, 32, 0, false);
		ExpectHookCall(aState, aCase > 0 ? 2 : 0, 4, 1);
		SexyTransform2D aTransform;
		aTransform.Translate(0, 40);
		g->BltTransformed(anImage, nullptr, Color(0, 0, 255), Graphics::DRAWMODE_NORMAL, Rect(0, 0, 8, 8), aTransform, false,
			6, 0, false);
		ExpectHookCall(aState, aCase > 0 ? 3 : 0, 5, 2);
		g->SetDrawHook(nullptr, nullptr);
		g->Flush();
		Read();
		const unsigned long aRed = 0xffff0000, aGreen = 0xff00ff00, aBlue = 0xff0000ff, aBlack = 0xff000000;
		// the triangle: (8, 8) is inside it and inside the rectangle, (20, 6) only inside the triangle
		Expect(8, 8, aCase == 1 ? aBlack : aRed);
		Expect(20, 6, aCase == 0 ? aRed : aBlack);
		// the textured triangle at x + 32: (40, 8) and (50, 6)
		Expect(40, 8, aCase == 0 ? aGreen : aBlack);
		Expect(50, 6, aCase == 0 ? aGreen : aBlack);
		// the quad at (6, 40)..(14, 48): dropped by the false hook, and untouched by the rectangle (0, 0, 12, 12), whose
		// part of it is empty
		Expect(8, 42, aCase == 0 ? aBlue : aBlack);
		End2D(g);
	}
	// a rectangle that holds the quad's four vertices draws it whole; one that cuts it draws the part inside
	for (int aCase = 0; aCase < 2; aCase++) {
		HookState aState;
		Rect aHookRect = (aCase == 0) ? Rect(0, 36, 60, 24) : Rect(0, 36, 10, 24);
		aState.mClip = &aHookRect;
		GPU2D* g = Begin2D(aCase == 0 ? "2d-hook-quad-inside" : "2d-hook-quad-cut", thePolicy);
		g->SetDrawHook(TestHook, &aState);
		SexyTransform2D aTransform;
		aTransform.Translate(0, 40);
		Rect aArgClip(0, 0, 60, 60);
		g->BltTransformed(anImage, &aArgClip, Color(0, 0, 255), Graphics::DRAWMODE_NORMAL, Rect(0, 0, 8, 8), aTransform, false,
			6, 0, false);
		g->SetDrawHook(nullptr, nullptr);
		g->Flush();
		Read();
		Expect(8, 42, 0xff0000ff);
		Expect(12, 42, aCase == 0 ? 0xff0000ff : 0xff000000);
		End2D(g);
	}
	gGPU->ImageDeleted(anImage);
	delete anImage;
}

// Pieces through the 2D layer: a Blt across two pieces binds each and joins them seamlessly; DrawTrianglesTex scales
// u, v by max U and V for a padded one-piece image and draws nothing for a cut one
void Test2DPieces(const GPUPolicy& thePolicy)
{
	GPU2D* g = Begin2D("2d-pieces", thePolicy);
	// 70x4 with Use64By64Subdivisions: two 64x64 pieces, the second holding columns 64-69
	std::vector<unsigned long> aBits(70 * 4);
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 70; x++)
			aBits[y * 70 + x] = 0xff000000 | (x * 3) << 16 | (y * 60) << 8 | 0x20;
	}
	MemoryImage* aWide = MakeImage(70, 4, aBits);
	aWide->mD3DFlags = 2 | 8;  // and UseA8R8G8B8, so the colours are exact
	g->Blt(aWide, 10, 10, Rect(60, 0, 8, 4), Color::White, Graphics::DRAWMODE_NORMAL);
	if (g->GetDrawCallCount() != 1)
		Fail("a Blt over two pieces made %d draws before the flush, expected 1", g->GetDrawCallCount());
	g->Flush();
	Read();
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 8; x++)
			Expect(10 + x, 10 + y, aBits[y * 70 + 60 + x]);
	}
	Expect(18, 10, 0xff000000);

	// 4x2, red then green: one 8x8 piece, max U 0.5, max V 0.25; the padding repeats the last column and row
	std::vector<unsigned long> aHalves = {0xffff0000, 0xffff0000, 0xff00ff00, 0xff00ff00,
		0xffff0000, 0xffff0000, 0xff00ff00, 0xff00ff00};
	MemoryImage* aSmall = MakeImage(4, 2, aHalves);
	TriVertex aTris[2][3] = {
		{TriVertex(20, 30, 0, 0), TriVertex(28, 30, 1, 0), TriVertex(20, 38, 0, 1)},
		{TriVertex(28, 30, 1, 0), TriVertex(28, 38, 1, 1), TriVertex(20, 38, 0, 1)}};
	g->DrawTrianglesTex(aSmall, aTris, 2, Color::White, Graphics::DRAWMODE_NORMAL, 0, 0, false);
	// the cut image draws nothing
	TriVertex aCut[1][3] = {{TriVertex(40, 30, 0, 0), TriVertex(48, 30, 1, 0), TriVertex(40, 38, 0, 1)}};
	g->DrawTrianglesTex(aWide, aCut, 1, Color::White, Graphics::DRAWMODE_NORMAL, 0, 0, false);
	Read();
	Expect(21, 31, 0xffff0000);
	Expect(26, 31, 0xff00ff00);
	Expect(26, 37, 0xff00ff00);
	Expect(41, 31, 0xff000000);
	End2D(g);
	gGPU->ImageDeleted(aWide);
	delete aWide;
	gGPU->ImageDeleted(aSmall);
	delete aSmall;
}

// The piece layout (0x61b970, 0x61bd00), without the GPU
void TestPieceLayout()
{
	gTest = "piece-layout";
	gTestFailed = false;
	struct Case
	{
		const char* mPolicy;
		int mWidth, mHeight;
		unsigned long mFlags;
		int mColumns, mRows, mPieceWidth, mPieceHeight;
		int mLastWidth, mLastHeight;
	} aCases[] = {
		{"twist", 1722, 246, 0, 4, 1, 512, 246, 186, 246},  // cut at the wide limit
		{"twist", 1722, 246, 1, 1, 1, 1722, 246, 1722, 246},  // MinimizeNumSubdivisions
		{"twist", 1722, 246, 0x10, 1, 1, 1722, 246, 1722, 246},  // a target's flag
		{"twist", 1024, 600, 0, 1, 1, 1024, 600, 1024, 600},  // not low enough to cut
		{"twist", 4, 4, 0, 1, 1, 8, 8, 8, 8},  // the minimum, padded
		{"twist", 70, 4, 2, 2, 1, 64, 64, 64, 64},  // Use64By64Subdivisions
		{"saf13", 300, 200, 0, 2, 1, 256, 256, 64, 256},  // aGoodTextureSize, then a power of two at the edge
		{"saf13", 2000, 100, 0, 2, 1, 1024, 128, 1024, 128},
	};
	GPUPolicy aTwist = GPUPolicy::Twist(), aSAF = GPUPolicy::SAF13();
	for (const Case& c : aCases) {
		const GPUPolicy& aPolicy = std::strcmp(c.mPolicy, "twist") == 0 ? aTwist : aSAF;
		GPUPieceLayout aLayout = aPolicy.Pieces(c.mWidth, c.mHeight, c.mFlags);
		const GPUPiece& aLast = aLayout.mPieces.back();
		if (aLayout.mColumns != c.mColumns || aLayout.mRows != c.mRows || aLayout.mPieceWidth != c.mPieceWidth ||
			aLayout.mPieceHeight != c.mPieceHeight || aLast.mWidth != c.mLastWidth || aLast.mHeight != c.mLastHeight ||
			(int)aLayout.mPieces.size() != c.mColumns * c.mRows)
			Fail("%s %dx%d flags %lx: %dx%d pieces of %dx%d, last %dx%d", c.mPolicy, c.mWidth, c.mHeight, c.mFlags,
				aLayout.mColumns, aLayout.mRows, aLayout.mPieceWidth, aLayout.mPieceHeight, aLast.mWidth, aLast.mHeight);
		float aMaxU = (float)c.mWidth / c.mPieceWidth, aMaxV = (float)c.mHeight / c.mPieceHeight;
		if (aLayout.mMaxU != aMaxU || aLayout.mMaxV != aMaxV)
			Fail("%s %dx%d: max U, V %g, %g", c.mPolicy, c.mWidth, c.mHeight, aLayout.mMaxU, aLayout.mMaxV);
	}
	End();
}

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
	TestCubeMap();
	TestScissor();
	TestLines();
	TestLighting();
	TestGameShaders();
	TestBuffers();
	TestPrecompile(aContext, aPolicy);
	TestSceneRules(gGPU, "gl");
	Test2DBlt(aPolicy);
	Test2DFillRect(aPolicy);
	Test2DLine(aPolicy);
	Test2DBatch(aPolicy);
	Test2DTransformed(aPolicy);
	Test2DDepthOnly(aPolicy);
	Test2DTrianglesTex(aPolicy);
	Test2DPieces(aPolicy);
	Test2DDrawHook(aPolicy);
	delete gGPU;
	gGPU = GPUInterface::Create(aContext, aPolicy, W, H);
	TestTextures(true);
	delete gGPU;
	GPUInterface* aNull = GPUInterface::CreateNull(aPolicy, W, H);
	TestSceneRules(aNull, "null");
	delete aNull;
	TestFormatRule(aPolicy);
	TestPieceLayout();
	std::printf("gpu: %d tests, %d failed (%s)\n", gTests, gFailed, aContext->Description().c_str());
	delete aContext;
	SDL_DestroyWindow(aWindow);
	SDL_Quit();
	return gFailed == 0 ? 0 : 1;
}
