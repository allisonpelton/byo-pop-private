// port: the game's side of 3D mode, standing where the original's gD3DInterface (0xe13bc0, a D3D9Interface: vtable
// 0x8195f4, bodies 0x618000-0x625000) answers the game's calls. The framework layers below it (framework/GPUInterface.h:
// the device; framework/GPU2D.h: the 2D draws, the state cache, the draw-mode layer, the transform stack) are generic;
// this holds what only Twist has: DrawPrimitiveEx and its listener, the .p3d meshes (D3DObject) with their listener and
// texture callback, the shader table and the slots' rule for shader constants, lights and material in the game's
// argument shapes, and WorldGraphicsListener (0x4bd5d0-0x4bd8a0), the 3D group around a world's draws.
//
// gD3DInterface is null in 2D mode, as in the original, and every 3D branch of the game tests it or Is3DAccelerated.
// Each method is named after its slot (the vtable offset) and keeps the body's own order of calls.
#pragma once

#include "GPUInterface.h"
#include "Math3D.h"
#include "SharedImage.h"

#include <map>
#include <string>
#include <vector>

namespace Sexy {

class DDImage;
class GPU2D;
class Image;
class MemoryImage;
class SexyMatrix3;
class TriVertex;

// ISexyShader (0x500310; ISexyPixelShader 0x504090, ISexyVertexShader 0x5040f0): one of the twenty shaders the
// resources' init (0x73ef19) makes from the bytecode in the exe. The port's are GLSL translations (TwistShaders.cpp).
struct TwistShader {
	std::string mName;
	bool mIsVertex = false;
	int mHandle = 0;  // the device's GPUPixelShader or GPUVertexShader
};

// The shaders by the globals that hold them (DAT_0085xxxx)
enum TwistShaderId {
	TWISTSHADER_BUTTONCOVER,       // 0x85dbd8
	TWISTSHADER_CUBE_P,            // 0x85e714
	TWISTSHADER_GRAYSCALE,         // 0x85e854
	TWISTSHADER_LOGO,              // 0x85de78
	TWISTSHADER_MASK,              // 0x85e30c
	TWISTSHADER_RAINBOW_CUBE_P,    // 0x85eb94
	TWISTSHADER_REWIND,            // 0x85db10
	TWISTSHADER_SAUCER_WINDOW_P,   // 0x85e75c
	TWISTSHADER_SCREEN_DISTORT,    // 0x85e7d0
	TWISTSHADER_SIDEBAR_BUMP_P,    // 0x85e1c0
	TWISTSHADER_SIDEBAR_P,         // 0x85df6c
	TWISTSHADER_SIDEBAR_TUBE_P,    // 0x85e008
	TWISTSHADER_UNDER_DIALOG,      // 0x85df48
	TWISTSHADER_WAVE,              // 0x85e8f8
	TWISTSHADER_WHITEOUT_FINAL,    // 0x85e0d8
	TWISTSHADER_CUBE_V,            // 0x85e988
	TWISTSHADER_RAINBOW_CUBE_V,    // 0x85e388
	TWISTSHADER_COUNT
	// (height, test and vidscreen are made by the original and never used: not carried)
};
extern TwistShader* gTwistShaders[TWISTSHADER_COUNT];

// The GLSL source of each shader (TwistShaders.cpp), in the device's convention (GPUInterface.h)
const char* TwistShaderSource(TwistShaderId theId);
const char* TwistShaderName(TwistShaderId theId);

// The colours a light is given in (the argument of slots 0xe8 and 0xec): each scaled by mIntensity (0x622f90)
struct D3DLightColors {
	Color mDiffuse;   // +0x00
	Color mSpecular;  // +0x10
	Color mAmbient;   // +0x20
	float mIntensity = 1.0f;  // +0x30
};

class D3DObject;

// A mesh of a .p3d (D3D9ObjectMesh, vtable 0x819748): its name (the shader name the draw listeners key on), its
// properties, its vertices (FVF 0x152, 36 bytes) and triangles; the buffers are made at its first draw (0x623c40)
struct D3DMesh {
	std::string mFrameName;
	std::string mName;                                // +0x0c
	std::map<std::string, std::string> mProperties;   // +0x28
	SharedImageRef mTexture;                          // texture0.fileName
	SharedImageRef mBump;                             // bump.fileName
	unsigned long mFVF = 0;                           // +0x5c
	int mStride = 0;                                  // +0x60
	int mIndexCount = 0;                              // +0x64
	int mVertexCount = 0;                             // +0x68
	std::vector<unsigned char> mVertices;             // +0x6c until the first draw
	std::vector<unsigned short> mIndices;             // +0x70 until the first draw
	GPUBuffer mVertexBuffer = 0;                      // +0x74
	GPUBuffer mIndexBuffer = 0;                       // +0x78
};

// D3DObjectListener (vtable 0x7e427c): the load callbacks and the draw callbacks of a D3DObject, the same field
// (object +0x24) for both; the base does nothing (0x70f100-0x70f128)
class D3DObjectListener {
public:
	virtual ~D3DObjectListener() {}                                                           // +0x1c
	virtual void LoadBegin(D3DObject* theObject) { (void)theObject; }                         // +0x00
	virtual void LoadProperty(D3DObject* theObject, const std::string& theFrame, const std::string& theMesh,
		const std::string& theKey, const std::string& theValue)                               // +0x04
	{
		(void)theObject; (void)theFrame; (void)theMesh; (void)theKey; (void)theValue;
	}
	// +0x08: the image of a texture property; empty to let the loader find the file beside the .p3d
	virtual SharedImageRef LoadTexture(D3DObject* theObject, const std::string& theFrame, const std::string& theMesh,
		const std::string& theKey, const std::string& theFileName)
	{
		(void)theObject; (void)theFrame; (void)theMesh; (void)theKey; (void)theFileName;
		return SharedImageRef();
	}
	virtual void BeginObject(D3DObject* theObject) { (void)theObject; }                       // +0x0c
	virtual void EndObject(D3DObject* theObject) { (void)theObject; }                         // +0x10
	// +0x14: before a mesh's triangles, its textures bound (stage 1 only with a bump map)
	virtual void BeginMesh(const D3DMesh& theMesh, bool hasBump) { (void)theMesh; (void)hasBump; }
	virtual void EndMesh(D3DObject* theObject, const D3DMesh& theMesh) { (void)theObject; (void)theMesh; }  // +0x18
};

// D3DObject (0x2c bytes, 0x4bd9b0): a loaded .p3d (0x508650 through slot 0x38)
class D3DObject {
public:
	std::string mFileName;                   // +0x04
	std::vector<D3DMesh> mMeshes;            // +0x20's list, every frame's meshes in file order
	D3DObjectListener* mListener = nullptr;  // +0x24
	~D3DObject();
};

// 0x508650: a new object for theFileName (a .p3d in the game's folder) with theListener, loaded through slot 0x38
// in 3D mode; in 2D mode it stays empty. Null when the load fails.
D3DObject* LoadD3DObject(const std::string& theFileName, D3DObjectListener* theListener);

// D3DDrawPrimitiveListener (vtable 0x7e42e8): around DrawPrimitiveEx's draw
class D3DDrawPrimitiveListener {
public:
	virtual ~D3DDrawPrimitiveListener() {}
	virtual void Before() {}  // +0x00 (0x4bd570)
	virtual void After() {}   // +0x04 (0x4bd580)
};

// 0x4c1940: a transform's matrix (scale times rotation rows, then the place)
GPUMatrix WorldMatrixOf(const Transform3D& theTransform);
// 0x4c17e0: the inverse of a transform's matrix (the view of a camera at it)
GPUMatrix ViewMatrixOf(const Transform3D& theTransform);
// 0x6741c5 (the camera's slot 0): its projection
GPUMatrix ProjectionMatrixOf(const PerspectiveCamera& theCamera);

// WorldGraphicsListener (0x54 bytes, vtable 0x814290): a 3D group's states and transforms
class WorldGraphicsListener {
public:
	// 0x4bd5d0: identity world, z test and write on (LESSEQUAL), culling CCW, lighting off; with theClearDepth the depth
	// buffer is cleared at once (slot 0x124)
	WorldGraphicsListener(const PerspectiveCamera* theCamera, bool theClearDepth);
	virtual ~WorldGraphicsListener() {}  // 0x4bd670

	const PerspectiveCamera* mCamera;  // +0x04
	Transform3D mWorld;          // +0x08
	int mZFunc = 4;              // +0x44
	int mCullMode = 3;           // +0x48
	bool mZEnable = true;        // +0x4c
	bool mZWrite = true;         // +0x4d
	bool mLighting = false;      // +0x4e
	WorldGraphicsListener* mInner = nullptr;  // +0x50: called after Apply and before Remove

	void SetCulling(bool theCull);                 // 0x4bd6c0: CCW or none
	void SetDepth(bool theTest, bool theWrite);    // 0x4bd680
	void SetLighting(bool theLighting) { mLighting = theLighting; }  // 0x4bd6e0
	void SetInner(WorldGraphicsListener* theInner) { mInner = theInner; }  // 0x4bd6f0
	virtual void Apply();   // 0x4bd700 (slot 0)
	virtual void Remove();  // 0x4bd8a0 (slot 1)
};

class TwistD3D {
public:
	TwistD3D(GPUInterface* theDevice, GPU2D* the2D);
	~TwistD3D();

	GPUInterface* mDevice;
	GPU2D* m2D;
	TwistShader* mPixelShader = nullptr;   // this[0x5e]
	TwistShader* mVertexShader = nullptr;  // this[0x5f]
	bool mDepthClearPending = false;       // +0xbe

	// 0x73ef19's part: the shader table (gTwistShaders)
	void MakeShaders();

	unsigned long GetCaps(int theWhich);                                            // 0x04 (0x61eec0)
	void PushTransform(const SexyMatrix3& theTransform, bool theConcatenate);       // 0x14
	void PopTransform();                                                            // 0x18
	bool LoadObject(D3DObject* theObject);                                          // 0x38 (0x61fd00)
	bool PrepareImage(Image* theImage);                                             // 0x44 (0x61e7d0)
	void FillRect(const Rect& theRect, const Color& theColor, int theDrawMode);     // 0x68
	void DrawTrianglesTex(Image* theTexture, const TriVertex theVertices[][3], int theNumTriangles,
		const Color& theColor, int theDrawMode, float theTX, float theTY, bool theBlend);  // 0x74
	void DrawTrianglesTexStrip(Image* theTexture, const TriVertex theVertices[], int theNumTriangles,
		const Color& theColor, int theDrawMode, float theTX, float theTY, bool theBlend);  // 0x78
	// 0x7c (0x6222f0): user vertices of theFVF; pre-transformed ones move by (tx, ty) and the 2D transform, and take the
	// draw's colour where their own is 0; theBlend is the linear filter
	void DrawPrimitiveEx(unsigned long theFVF, GPUPrimitiveType theType, const void* theVertices, int thePrimitiveCount,
		const Color& theColor, int theDrawMode, float theTX, float theTY, bool theBlend,
		D3DDrawPrimitiveListener* theListener);
	bool SetRenderTarget(Image* theImage);                                          // 0x98
	void Clear(int theRed, int theGreen, int theBlue, int theAlpha);                // 0x9c (0x621a30)
	// 0xa0 (0x621ae0): the screen's picture so far goes to *theImage (made, screen-sized and a render target, when
	// null) and drawing to the screen goes on in a cleared buffer; *theSwapped (the original's saved surface) says the
	// swap is in place, and a second call swaps back. Returns *theImage.
	Image* SwapScreen(DDImage** theImage, bool* theSwapped);
	void SetTextureStageState(int theStage, int theType, unsigned long theValue);   // 0xbc
	void SetRenderState(int theState, unsigned long theValue);                      // 0xc0
	void SetSamplerState(int theStage, int theType, unsigned long theValue);        // 0xc4
	void SetTextureLinearFilter(int theStage, bool theLinear);                      // 0xc8 (0x622b70)
	void SetTextureWrap(int theStage, bool theWrap);                                // 0xcc (0x622c10)
	void SetTexture(int theStage, Image* theImage);                                 // 0xd4
	void SetPixelShader(TwistShader* theShader);                                    // 0xd8 (0x622dd0)
	void SetVertexShader(TwistShader* theShader);                                   // 0xdc (0x622ea0)
	void SetTransform(int theType, const GPUMatrix& theMatrix);                     // 0xe0
	void LightEnable(int theIndex, bool theEnable);                                 // 0xe4
	void SetPointLight(int theIndex, const Vec3& thePosition, const D3DLightColors& theColors, float theRange,
		const Vec3& theAttenuation);                                                // 0xe8 (0x622ff0)
	void SetDirectionalLight(int theIndex, const Vec3& theDirection, const D3DLightColors& theColors);  // 0xec
	void SetMaterial(const Color& theDiffuse, const Color& theSpecular, const Color& theAmbient,
		const Color& theEmissive, float thePower);                                  // 0xf0 (0x623220)
	// 0x108 (0x623a20): a float constant, set only when theShader is the bound pixel or vertex shader
	void SetShaderConstant(TwistShader* theShader, int theRegister, const float theValue[4]);
	// 0x10c (0x6242f0): theObject's meshes under theWorld; with the2DStates the object brings its own z, culling,
	// lighting and texture states and puts 2D's back after
	void DrawObject(D3DObject* theObject, const GPUMatrix& theWorld, const Color& theColor, bool the2DStates);
	void SetDepthTestMode(int theMode);                                             // 0x114
	void SetDepthWriteMode(int theMode);                                            // 0x118
	void SetAlphaRef(int theRef);                                                   // 0x11c
	void SetDepthZ(float theWriteZ, float theOtherZ);                               // 0x120
	void ClearDepth();                                                              // 0x124 (0x624cd0)

private:
	GPUMatrix mWorld, mView, mProjection;  // the device's, for slot 0x144
	void SetVertexShaderMatrices();        // 0x144 (0x623fd0)
	void DrawMeshes(D3DObject* theObject); // 0x140 (0x623c40)
};

// The original's gD3DInterface: set while a GPUDisplay runs (WinTwistApp's 3D mode), null in 2D mode
extern TwistD3D* gD3DInterface;

}  // namespace Sexy
