// Sexy::GPUInterface's OpenGL ES 3.0 / WebGL2 implementation (GPUInterface.h): Direct3D 9's device state carried out
// through GL, with every convention that differs converted here (the list at the top of GPUInterface.h).
#include "GPUInterface.h"
#include "MemoryImage.h"
#include "platform/GLContext.h"
#include "platform/GLFuncs.h"

#include <SDL.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <sstream>

using namespace Sexy;

namespace
{

const int kStages = 4;
const int kLights = 8;
const int kVSConstants = 64;
const int kPSConstants = 32;

// The attribute slots, the same in every program
enum { ATTR_POSITION, ATTR_NORMAL, ATTR_DIFFUSE, ATTR_SPECULAR, ATTR_TEX0 };

// An FVF's layout in bytes
struct VertexLayout
{
	bool mRHW = false;
	int mPosition = -1, mNormal = -1, mDiffuse = -1, mSpecular = -1;
	int mTex[kStages] = {-1, -1, -1, -1};
	int mTexSize[kStages] = {0, 0, 0, 0};
	int mNumTex = 0;
	int mStride = 0;
};

VertexLayout LayoutOf(unsigned long theFVF)
{
	VertexLayout aLayout;
	int anOffset = 0;
	if ((theFVF & 0xe) == GPUFVF_XYZRHW) {
		aLayout.mRHW = true;
		aLayout.mPosition = anOffset;
		anOffset += 16;
	} else {
		aLayout.mPosition = anOffset;
		anOffset += 12;
	}
	if (theFVF & GPUFVF_NORMAL) {
		aLayout.mNormal = anOffset;
		anOffset += 12;
	}
	if (theFVF & GPUFVF_DIFFUSE) {
		aLayout.mDiffuse = anOffset;
		anOffset += 4;
	}
	if (theFVF & GPUFVF_SPECULAR) {
		aLayout.mSpecular = anOffset;
		anOffset += 4;
	}
	aLayout.mNumTex = (int)((theFVF >> 8) & 0xf);
	for (int i = 0; i < aLayout.mNumTex && i < kStages; i++) {
		static const int kSizes[4] = {2, 3, 4, 1};  // D3DFVF_TEXCOORDSIZEn
		aLayout.mTex[i] = anOffset;
		aLayout.mTexSize[i] = kSizes[(theFVF >> (16 + 2 * i)) & 3];
		anOffset += 4 * aLayout.mTexSize[i];
	}
	aLayout.mStride = anOffset;
	return aLayout;
}

std::string JoinInts(const std::vector<long long>& theValues)
{
	std::string aText;
	for (size_t i = 0; i < theValues.size(); i++) {
		if (i > 0)
			aText += ",";
		aText += std::to_string(theValues[i]);
	}
	return aText;
}

std::vector<long long> SplitInts(const std::string& theText)
{
	std::vector<long long> aValues;
	std::stringstream aStream(theText);
	std::string aPart;
	while (std::getline(aStream, aPart, ','))
		aValues.push_back(std::strtoll(aPart.c_str(), nullptr, 10));
	return aValues;
}

// The vertex stage a draw needs: generated fixed function, pre-transformed, or a game shader
struct VSKey
{
	enum { FIXED, PRETRANSFORMED, GAME } mKind = FIXED;
	std::string mGame;
	long long mFVF = 0;
	long long mLighting = 0, mColorVertex = 0, mNormalize = 0, mLocalViewer = 0, mSpecular = 0;
	long long mDiffuseSource = 0, mSpecularSource = 0, mAmbientSource = 0, mEmissiveSource = 0;
	std::vector<long long> mLightTypes;
	long long mTexCoordIndex[kStages] = {0, 1, 2, 3};
	long long mTextureTransform[kStages] = {0, 0, 0, 0};

	std::string Text() const
	{
		if (mKind == GAME)
			return "VG" + mGame + ";" + std::to_string(mFVF);
		std::vector<long long> aValues = {mKind, mFVF, mLighting, mColorVertex, mNormalize, mLocalViewer, mSpecular,
			mDiffuseSource, mSpecularSource, mAmbientSource, mEmissiveSource};
		for (int i = 0; i < kStages; i++) {
			aValues.push_back(mTexCoordIndex[i]);
			aValues.push_back(mTextureTransform[i]);
		}
		aValues.push_back((long long)mLightTypes.size());
		for (long long aType : mLightTypes)
			aValues.push_back(aType);
		return "V" + JoinInts(aValues);
	}

	static bool Parse(const std::string& theText, VSKey* theKey)
	{
		if (theText.compare(0, 2, "VG") == 0) {
			size_t aSemi = theText.find(';');
			if (aSemi == std::string::npos)
				return false;
			theKey->mKind = GAME;
			theKey->mGame = theText.substr(2, aSemi - 2);
			theKey->mFVF = std::strtoll(theText.c_str() + aSemi + 1, nullptr, 10);
			return true;
		}
		if (theText.empty() || theText[0] != 'V')
			return false;
		std::vector<long long> aValues = SplitInts(theText.substr(1));
		if (aValues.size() < 20)
			return false;
		size_t i = 0;
		theKey->mKind = aValues[i++] == PRETRANSFORMED ? PRETRANSFORMED : FIXED;
		theKey->mFVF = aValues[i++];
		theKey->mLighting = aValues[i++];
		theKey->mColorVertex = aValues[i++];
		theKey->mNormalize = aValues[i++];
		theKey->mLocalViewer = aValues[i++];
		theKey->mSpecular = aValues[i++];
		theKey->mDiffuseSource = aValues[i++];
		theKey->mSpecularSource = aValues[i++];
		theKey->mAmbientSource = aValues[i++];
		theKey->mEmissiveSource = aValues[i++];
		for (int s = 0; s < kStages; s++) {
			theKey->mTexCoordIndex[s] = aValues[i++];
			theKey->mTextureTransform[s] = aValues[i++];
		}
		long long aCount = aValues[i++];
		theKey->mLightTypes.clear();
		for (long long n = 0; n < aCount && i < aValues.size(); n++)
			theKey->mLightTypes.push_back(aValues[i++]);
		return true;
	}
};

// The pixel stage a draw needs: generated texture stage combiners, or a game shader
struct PSKey
{
	enum { COMBINERS, GAME } mKind = COMBINERS;
	std::string mGame;
	// per stage, while enabled: COLOROP, COLORARG1, COLORARG2, COLORARG0, ALPHAOP, ALPHAARG1, ALPHAARG2, ALPHAARG0,
	// RESULTARG, whether a texture is bound
	std::vector<std::vector<long long>> mStages;
	long long mSpecularAdd = 0;

	std::string Text() const
	{
		if (mKind == GAME)
			return "PG" + mGame;
		std::vector<long long> aValues = {mSpecularAdd, (long long)mStages.size()};
		for (const std::vector<long long>& aStage : mStages)
			aValues.insert(aValues.end(), aStage.begin(), aStage.end());
		return "P" + JoinInts(aValues);
	}

	static bool Parse(const std::string& theText, PSKey* theKey)
	{
		if (theText.compare(0, 2, "PG") == 0) {
			theKey->mKind = GAME;
			theKey->mGame = theText.substr(2);
			return true;
		}
		if (theText.empty() || theText[0] != 'P')
			return false;
		std::vector<long long> aValues = SplitInts(theText.substr(1));
		if (aValues.size() < 2)
			return false;
		theKey->mKind = COMBINERS;
		theKey->mSpecularAdd = aValues[0];
		size_t aCount = (size_t)aValues[1];
		if (aValues.size() != 2 + aCount * 10)
			return false;
		theKey->mStages.clear();
		for (size_t s = 0; s < aCount; s++)
			theKey->mStages.push_back(std::vector<long long>(aValues.begin() + 2 + s * 10, aValues.begin() + 12 + s * 10));
		return true;
	}
};

const char* kVSPrelude =
	"in vec4 aPosition;\n"
	"in vec3 aNormal;\n"
	"in vec4 aDiffuseBGRA;\n"
	"in vec4 aSpecularBGRA;\n"
	"in vec4 aTex0;\nin vec4 aTex1;\nin vec4 aTex2;\nin vec4 aTex3;\n"
	"out vec4 vDiffuse;\nout vec4 vSpecular;\n"
	"out vec4 vTex0;\nout vec4 vTex1;\nout vec4 vTex2;\nout vec4 vTex3;\n"
	"uniform vec4 uViewport;\n"
	"uniform vec4 uVS[64];\n"
	"vec4 aDiffuse;\nvec4 aSpecular;\nvec4 oPos;\n"
	// D3D9's pixel centres are on integers, GL's on half-integers; the targets are stored top row first, so GL's
	// window y is D3D's; GL clips z at -w, D3D at 0
	"vec4 FixPosition(vec4 p) {\n"
	"  return vec4(p.x + p.w / uViewport.z, -p.y + p.w / uViewport.w, 2.0 * p.z - p.w, p.w);\n"
	"}\n";

const char* kPSPrelude =
	"in vec4 vDiffuse;\nin vec4 vSpecular;\n"
	"in vec4 vTex0;\nin vec4 vTex1;\nin vec4 vTex2;\nin vec4 vTex3;\n"
	"uniform sampler2D uSampler0;\nuniform sampler2D uSampler1;\nuniform sampler2D uSampler2;\n"
	"uniform sampler2D uSampler3;\n"
	"uniform vec4 uPS[32];\n"
	"uniform vec4 uTFactor;\n"
	"uniform vec4 uAlphaTest;\n"  // x: D3DCMPFUNC (8, always, when alpha test is off), y: ALPHAREF
	"out vec4 fragColor;\n"
	"vec4 oC0;\n"
	"bool AlphaPasses(float a) {\n"
	"  float v = floor(clamp(a, 0.0, 1.0) * 255.0 + 0.5);\n"
	"  int f = int(uAlphaTest.x);\n"
	"  float r = uAlphaTest.y;\n"
	"  if (f == 1) return false;\n"
	"  if (f == 2) return v < r;\n"
	"  if (f == 3) return v == r;\n"
	"  if (f == 4) return v <= r;\n"
	"  if (f == 5) return v > r;\n"
	"  if (f == 6) return v != r;\n"
	"  if (f == 7) return v >= r;\n"
	"  return true;\n"
	"}\n";

std::string TexAttr(int theIndex)
{
	return "aTex" + std::to_string(theIndex);
}

std::string GenerateFixedVS(const VSKey& theKey)
{
	VertexLayout aLayout = LayoutOf((unsigned long)theKey.mFVF);
	std::string s;
	s += "uniform mat4 uWorldView;\nuniform mat4 uWorldViewProj;\nuniform mat4 uNormalMatrix;\n";
	s += "uniform mat4 uTexMatrix[4];\n";
	s += "uniform vec4 uLightPos[8];\nuniform vec4 uLightDir[8];\nuniform vec4 uLightDiffuse[8];\n";
	s += "uniform vec4 uLightSpecular[8];\nuniform vec4 uLightAmbient[8];\nuniform vec4 uLightAtt[8];\n";
	s += "uniform vec4 uMatDiffuse;\nuniform vec4 uMatAmbient;\nuniform vec4 uMatSpecular;\nuniform vec4 uMatEmissive;\n";
	s += "uniform float uMatPower;\nuniform vec4 uAmbient;\n";
	s += "void main() {\n";
	s += aLayout.mDiffuse >= 0 ? "  vec4 diffuse = aDiffuseBGRA.zyxw;\n" : "  vec4 diffuse = vec4(1.0);\n";
	s += aLayout.mSpecular >= 0 ? "  vec4 specular = aSpecularBGRA.zyxw;\n" : "  vec4 specular = vec4(0.0);\n";
	// Texture coordinate sets, expanded as D3D expands them for a texture matrix: (u, v, 1, 0) for two components
	for (int i = 0; i < aLayout.mNumTex && i < kStages; i++) {
		std::string anAttr = TexAttr(i);
		std::string aSet = "set" + std::to_string(i);
		switch (aLayout.mTexSize[i]) {
		case 1: s += "  vec4 " + aSet + " = vec4(" + anAttr + ".x, 1.0, 0.0, 0.0);\n"; break;
		case 2: s += "  vec4 " + aSet + " = vec4(" + anAttr + ".xy, 1.0, 0.0);\n"; break;
		case 3: s += "  vec4 " + aSet + " = vec4(" + anAttr + ".xyz, 1.0);\n"; break;
		default: s += "  vec4 " + aSet + " = " + anAttr + ";\n"; break;
		}
	}
	if (theKey.mKind == VSKey::PRETRANSFORMED) {
		// Screen coordinates with D3D's pixel centres, w = 1/rhw for perspective-correct texturing
		s += "  float w = aPosition.w != 0.0 ? 1.0 / aPosition.w : 1.0;\n";
		s += "  vec2 sp = aPosition.xy + 0.5 - uViewport.xy;\n";
		s += "  gl_Position = vec4((sp.x * 2.0 / uViewport.z - 1.0) * w, (sp.y * 2.0 / uViewport.w - 1.0) * w,";
		s += " (2.0 * aPosition.z - 1.0) * w, w);\n";
		s += "  vDiffuse = diffuse;\n  vSpecular = specular;\n";
		for (int st = 0; st < kStages; st++) {
			int anIndex = (int)(theKey.mTexCoordIndex[st] & 0xffff);
			std::string anOut = "vTex" + std::to_string(st);
			if (anIndex < aLayout.mNumTex)
				s += "  " + anOut + " = set" + std::to_string(anIndex) + ";\n";
			else
				s += "  " + anOut + " = vec4(0.0, 0.0, 0.0, 1.0);\n";
		}
		s += "}\n";
		return s;
	}
	s += "  vec4 P = uWorldView * vec4(aPosition.xyz, 1.0);\n";
	s += aLayout.mNormal >= 0 ? "  vec3 N = (uNormalMatrix * vec4(aNormal, 0.0)).xyz;\n" : "  vec3 N = vec3(0.0);\n";
	if (theKey.mNormalize)
		s += "  if (dot(N, N) > 0.0) N = normalize(N);\n";
	s += "  gl_Position = FixPosition(uWorldViewProj * vec4(aPosition.xyz, 1.0));\n";
	if (theKey.mLighting) {
		// D3D9's lighting in camera space; the material's colours come from the vertex where COLORVERTEX says so
		auto aSource = [&](long long theSource, const char* theMaterial) {
			if (theKey.mColorVertex && theSource == 1 && aLayout.mDiffuse >= 0)
				return std::string("diffuse");
			if (theKey.mColorVertex && theSource == 2 && aLayout.mSpecular >= 0)
				return std::string("specular");
			return std::string(theMaterial);
		};
		s += "  vec4 Md = " + aSource(theKey.mDiffuseSource, "uMatDiffuse") + ";\n";
		s += "  vec4 Ms = " + aSource(theKey.mSpecularSource, "uMatSpecular") + ";\n";
		s += "  vec4 Ma = " + aSource(theKey.mAmbientSource, "uMatAmbient") + ";\n";
		s += "  vec4 Me = " + aSource(theKey.mEmissiveSource, "uMatEmissive") + ";\n";
		s += "  vec3 sumDiffuse = vec3(0.0);\n  vec3 sumAmbient = vec3(0.0);\n  vec3 sumSpecular = vec3(0.0);\n";
		s += theKey.mLocalViewer ? "  vec3 V = -normalize(P.xyz);\n" : "  vec3 V = vec3(0.0, 0.0, -1.0);\n";
		for (size_t i = 0; i < theKey.mLightTypes.size(); i++) {
			std::string n = std::to_string(i);
			s += "  {\n    vec3 L;\n    float att = 1.0;\n";
			if (theKey.mLightTypes[i] == 3) {
				s += "    L = -uLightDir[" + n + "].xyz;\n";
			} else {
				// point (and spot, without its cone)
				s += "    vec3 d = uLightPos[" + n + "].xyz - P.xyz;\n    float dist = length(d);\n";
				s += "    L = dist > 0.0 ? d / dist : vec3(0.0);\n";
				s += "    vec4 a = uLightAtt[" + n + "];\n";
				s += "    att = dist > a.w ? 0.0 : 1.0 / (a.x + a.y * dist + a.z * dist * dist);\n";
			}
			s += "    float nl = max(dot(N, L), 0.0);\n";
			s += "    sumDiffuse += att * nl * uLightDiffuse[" + n + "].rgb;\n";
			s += "    sumAmbient += att * uLightAmbient[" + n + "].rgb;\n";
			if (theKey.mSpecular)
				s += "    if (nl > 0.0) sumSpecular += att * pow(max(dot(N, normalize(L + V)), 0.0), uMatPower) * uLightSpecular["
					+ n + "].rgb;\n";
			s += "  }\n";
		}
		s += "  vec3 lit = Me.rgb + Ma.rgb * (uAmbient.rgb + sumAmbient) + Md.rgb * sumDiffuse;\n";
		s += "  vDiffuse = clamp(vec4(lit, Md.a), 0.0, 1.0);\n";
		s += "  vSpecular = clamp(vec4(Ms.rgb * sumSpecular, 0.0), 0.0, 1.0);\n";
	} else {
		s += "  vDiffuse = diffuse;\n  vSpecular = specular;\n";
	}
	for (int st = 0; st < kStages; st++) {
		long long aTCI = theKey.mTexCoordIndex[st];
		int anIndex = (int)(aTCI & 0xffff);
		int aGeneration = (int)((aTCI >> 16) & 0xf);
		std::string aSource;
		if (aGeneration == 1)
			aSource = "vec4(N, 1.0)";
		else if (aGeneration == 2)
			aSource = "vec4(P.xyz, 1.0)";
		else if (aGeneration == 3)
			aSource = theKey.mLocalViewer ? "vec4(reflect(normalize(P.xyz), N), 1.0)" : "vec4(reflect(vec3(0.0, 0.0, 1.0), N), 1.0)";
		else if (anIndex < aLayout.mNumTex)
			aSource = "set" + std::to_string(anIndex);
		else
			aSource = "vec4(0.0, 0.0, 0.0, 1.0)";
		std::string anOut = "vTex" + std::to_string(st);
		long long aFlags = theKey.mTextureTransform[st];
		int aCount = (int)(aFlags & 0xff);
		if (aCount == 0) {
			s += "  " + anOut + " = " + aSource + ";\n";
		} else {
			s += "  " + anOut + " = uTexMatrix[" + std::to_string(st) + "] * " + aSource + ";\n";
			if (aFlags & 0x100) {
				static const char* kComponents = "xyzw";
				s += "  " + anOut + " /= " + anOut + "." + kComponents[(aCount - 1) & 3] + ";\n";
			}
		}
	}
	s += "}\n";
	return s;
}

std::string GenerateGameVS(const std::string& theSource)
{
	return "#define main vsMain\n" + theSource + "\n#undef main\n"
		"void main() {\n"
		"  aDiffuse = aDiffuseBGRA.zyxw;\n  aSpecular = aSpecularBGRA.zyxw;\n"
		"  vDiffuse = vec4(1.0);\n  vSpecular = vec4(0.0);\n"
		"  vTex0 = vec4(0.0);\n  vTex1 = vec4(0.0);\n  vTex2 = vec4(0.0);\n  vTex3 = vec4(0.0);\n"
		"  oPos = vec4(0.0, 0.0, 0.0, 1.0);\n"
		"  vsMain();\n"
		"  gl_Position = FixPosition(oPos);\n"
		"}\n";
}

// A D3DTA argument of a stage, as an expression
std::string StageArg(long long theArg, int theStage, bool theAlpha)
{
	std::string aValue;
	switch (theArg & 0xf) {
	case 0: aValue = "vDiffuse"; break;
	case 1: aValue = "current"; break;
	case 2: aValue = "tex" + std::to_string(theStage); break;
	case 3: aValue = "uTFactor"; break;
	case 4: aValue = "vSpecular"; break;
	case 5: aValue = "temp"; break;
	default: aValue = "vec4(1.0)"; break;
	}
	if (theArg & 0x20)
		aValue = aValue + ".aaaa";  // ALPHAREPLICATE
	if (theArg & 0x10)
		aValue = "(vec4(1.0) - " + aValue + ")";  // COMPLEMENT
	return theAlpha ? "(" + aValue + ").a" : "(" + aValue + ").rgb";
}

// A D3DTOP operation on its three arguments; theType is "vec3" or "float"
std::string StageOp(long long theOp, const std::string& a1, const std::string& a2, const std::string& a0, int theStage,
	const std::string& theType)
{
	std::string t = "tex" + std::to_string(theStage);
	std::string one = theType + "(1.0)";
	switch (theOp) {
	case 2: return a1;
	case 3: return a2;
	case 4: return a1 + " * " + a2;
	case 5: return "2.0 * " + a1 + " * " + a2;
	case 6: return "4.0 * " + a1 + " * " + a2;
	case 7: return a1 + " + " + a2;
	case 8: return a1 + " + " + a2 + " - " + theType + "(0.5)";
	case 9: return "2.0 * (" + a1 + " + " + a2 + " - " + theType + "(0.5))";
	case 10: return a1 + " - " + a2;
	case 11: return a1 + " + " + a2 + " * (" + one + " - " + a1 + ")";
	case 12: return "mix(" + a2 + ", " + a1 + ", vDiffuse.a)";
	case 13: return "mix(" + a2 + ", " + a1 + ", " + t + ".a)";
	case 14: return "mix(" + a2 + ", " + a1 + ", uTFactor.a)";
	case 15: return a1 + " + " + a2 + " * (1.0 - " + t + ".a)";
	case 16: return "mix(" + a2 + ", " + a1 + ", current.a)";
	case 24: return theType == "float" ? "clamp(4.0 * dot(vec3(" + a1 + ") - 0.5, vec3(" + a2 + ") - 0.5), 0.0, 1.0)"
		: theType + "(clamp(4.0 * dot(" + a1 + " - 0.5, " + a2 + " - 0.5), 0.0, 1.0))";
	case 25: return a0 + " + " + a1 + " * " + a2;
	case 26: return "mix(" + a2 + ", " + a1 + ", " + a0 + ")";
	default: return a1;  // the bump and premodulate operations are logged by the caller and taken as SELECTARG1
	}
}

std::string GenerateCombinersPS(const PSKey& theKey)
{
	std::string s = "void main() {\n  vec4 current = vDiffuse;\n  vec4 temp = vec4(0.0);\n";
	for (size_t st = 0; st < theKey.mStages.size(); st++) {
		const std::vector<long long>& v = theKey.mStages[st];
		std::string n = std::to_string(st);
		s += "  vec4 tex" + n + " = " + (v[9] ? "texture(uSampler" + n + ", vTex" + n + ".xy)" : std::string("vec4(1.0)")) + ";\n";
		std::string aColor = StageOp(v[0], StageArg(v[1], (int)st, false), StageArg(v[2], (int)st, false),
			StageArg(v[3], (int)st, false), (int)st, "vec3");
		std::string anAlpha = v[4] == 1 ? std::string("current.a")
			: StageOp(v[4], StageArg(v[5], (int)st, true), StageArg(v[6], (int)st, true), StageArg(v[7], (int)st, true),
				(int)st, "float");
		std::string aResult = v[8] == 5 ? "temp" : "current";
		s += "  {\n    vec3 c = clamp(" + aColor + ", 0.0, 1.0);\n    float a = clamp(" + anAlpha + ", 0.0, 1.0);\n";
		s += "    " + aResult + " = vec4(c, a);\n  }\n";
	}
	if (theKey.mSpecularAdd)
		s += "  current.rgb = clamp(current.rgb + vSpecular.rgb, 0.0, 1.0);\n";
	s += "  if (!AlphaPasses(current.a)) discard;\n  fragColor = current;\n}\n";
	return s;
}

std::string GenerateGamePS(const std::string& theSource)
{
	return "#define main psMain\n" + theSource + "\n#undef main\n"
		"void main() {\n"
		"  oC0 = vec4(0.0);\n"
		"  psMain();\n"
		"  if (!AlphaPasses(oC0.a)) discard;\n"
		"  fragColor = oC0;\n"
		"}\n";
}

GLenum BlendFactor(unsigned long theBlend, bool theSource)
{
	switch (theBlend) {
	case 1: return GL_ZERO;
	case 2: return GL_ONE;
	case 3: return GL_SRC_COLOR;
	case 4: return GL_ONE_MINUS_SRC_COLOR;
	case 5: return GL_SRC_ALPHA;
	case 6: return GL_ONE_MINUS_SRC_ALPHA;
	case 7: return GL_DST_ALPHA;
	case 8: return GL_ONE_MINUS_DST_ALPHA;
	case 9: return GL_DST_COLOR;
	case 10: return GL_ONE_MINUS_DST_COLOR;
	case 11: return GL_SRC_ALPHA_SATURATE;
	case 12: return theSource ? GL_SRC_ALPHA : GL_ONE_MINUS_SRC_ALPHA;  // BOTHSRCALPHA
	case 13: return theSource ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA;  // BOTHINVSRCALPHA
	default: return GL_ONE;
	}
}

GLenum CompareFunc(unsigned long theFunc)
{
	static const GLenum kFuncs[9] = {GL_ALWAYS, GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL,
		GL_GEQUAL, GL_ALWAYS};
	return theFunc < 9 ? kFuncs[theFunc] : GL_ALWAYS;
}

GLenum PrimitiveMode(GPUPrimitiveType theType)
{
	if (theType == GPUPT_TRIANGLESTRIP)
		return GL_TRIANGLE_STRIP;
	if (theType == GPUPT_TRIANGLEFAN)
		return GL_TRIANGLE_FAN;
	return GL_TRIANGLES;
}

int VertexCount(GPUPrimitiveType theType, int thePrimitives)
{
	return theType == GPUPT_TRIANGLELIST ? thePrimitives * 3 : thePrimitives + 2;
}

// The inverse transpose of a matrix's upper 3x3, in D3D's row-vector layout (normals are transformed by it)
GPUMatrix NormalMatrix(const GPUMatrix& M)
{
	const float (*m)[4] = M.m;
	float aDet = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
		m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
	GPUMatrix aResult = GPUMatrix::Identity();
	if (aDet == 0.0f)
		return aResult;
	float d = 1.0f / aDet;
	// inverse(A)^T = cofactor(A) / det
	aResult.m[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * d;
	aResult.m[0][1] = -(m[1][0] * m[2][2] - m[1][2] * m[2][0]) * d;
	aResult.m[0][2] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * d;
	aResult.m[1][0] = -(m[0][1] * m[2][2] - m[0][2] * m[2][1]) * d;
	aResult.m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * d;
	aResult.m[1][2] = -(m[0][0] * m[2][1] - m[0][1] * m[2][0]) * d;
	aResult.m[2][0] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * d;
	aResult.m[2][1] = -(m[0][0] * m[1][2] - m[0][2] * m[1][0]) * d;
	aResult.m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * d;
	// D3D transforms n by (A^-1)^T as a row vector: n * (A^-1)^T; the cofactor matrix computed here is (A^-1)^T * det
	// laid out with rows as D3D rows, so it is used as is
	return aResult;
}

void TransformRow(const float v[4], const GPUMatrix& M, float out[4])
{
	for (int c = 0; c < 4; c++)
		out[c] = v[0] * M.m[0][c] + v[1] * M.m[1][c] + v[2] * M.m[2][c] + v[3] * M.m[3][c];
}

struct Program
{
	GLuint mProgram = 0;
	GLint mViewport = -1, mVS = -1, mPS = -1, mTFactor = -1, mAlphaTest = -1;
	GLint mWorldView = -1, mWorldViewProj = -1, mNormalMatrix = -1, mTexMatrix = -1;
	GLint mLightPos = -1, mLightDir = -1, mLightDiffuse = -1, mLightSpecular = -1, mLightAmbient = -1, mLightAtt = -1;
	GLint mMatDiffuse = -1, mMatAmbient = -1, mMatSpecular = -1, mMatEmissive = -1, mMatPower = -1, mAmbient = -1;
	GLint mSamplers[kStages] = {-1, -1, -1, -1};
	unsigned long long mVSSerial = 0, mPSSerial = 0;
};

struct Shader
{
	std::string mName;
	std::string mSource;
	bool mVertex = false;
};

// An image's texture: uploaded from a MemoryImage's bits, or the colour buffer of a render target
struct TextureData
{
	GLuint mTexture = 0;
	int mWidth = 0, mHeight = 0;
	int mBitsChangedCount = -1;
	bool mTarget = false;
	GPUFormat mFormat = GPUFMT_A8R8G8B8;
	GLuint mFramebuffer = 0;
};

struct BufferData
{
	GLuint mBuffer = 0;
	bool mIndex = false;
	unsigned long mFVF = 0;
	std::vector<unsigned char> mData;
};

class GPUInterfaceGL : public GPUInterface
{
public:
	GPUInterfaceGL(GLContext* theContext, const GPUPolicy& thePolicy, int theWidth, int theHeight);
	~GPUInterfaceGL() override;

	bool Init();

	bool IsNull() override { return false; }
	const GPUPolicy& GetPolicy() override { return mPolicy; }
	void Resize(int theWidth, int theHeight) override;
	void ContextLost() override;
	void ContextRestored() override;

	bool BeginSceneIfNeeded() override;
	void EndScene() override;
	void SetSceneAllowed(bool theAllowed) override { mSceneAllowed = theAllowed; }
	int GetSceneCount() override { return mSceneCount; }

	bool CreateRenderTarget(Image* theImage, GPUFormat theFormat) override;
	bool SetRenderTarget(Image* theImage) override;
	Image* GetRenderTarget() override { return mTarget; }
	void StretchRect(Image* theSource, const Rect& theSrcRect, Image* theDest, const Rect& theDestRect,
		GPUTextureFilter theFilter) override;
	void Clear(unsigned long theFlags, unsigned long theColor, float theZ) override;
	void SetViewport(const Rect& theRect, float theMinZ, float theMaxZ) override;
	void SetScissorRect(const Rect& theRect) override { mScissor = theRect; }

	bool ReadBack(Image* theImage, MemoryImage* theDest) override;
	bool RecoverBits(MemoryImage* theImage) override;
	unsigned long GetCaps(int theWhich) override { return theWhich == 1 ? mPolicy.mMaxTextureBlendStages : mPolicy.mCaps; }
	void BitsChanged(Image* theImage) override;
	void ImageDeleted(Image* theImage) override;

	void SetRenderState(int theState, unsigned long theValue) override;
	void SetTextureStageState(int theStage, int theType, unsigned long theValue) override;
	void SetSamplerState(int theStage, int theType, unsigned long theValue) override;
	void SetTexture(int theStage, Image* theImage) override;
	void SetTransform(int theType, const GPUMatrix& theMatrix) override;
	void SetLight(int theIndex, const GPULight& theLight) override;
	void LightEnable(int theIndex, bool theEnable) override;
	void SetMaterial(const GPUMaterial& theMaterial) override { mMaterial = theMaterial; }

	GPUVertexShader CreateVertexShader(const std::string& theName, const char* theSource) override;
	GPUPixelShader CreatePixelShader(const std::string& theName, const char* theSource) override;
	void SetVertexShader(GPUVertexShader theShader) override;
	void SetPixelShader(GPUPixelShader theShader) override;
	GPUVertexShader GetVertexShader() override { return mVertexShader; }
	GPUPixelShader GetPixelShader() override { return mPixelShader; }
	void SetVertexShaderConstantF(int theRegister, const float* theValues, int theCount) override;
	void SetPixelShaderConstantF(int theRegister, const float* theValues, int theCount) override;
	std::vector<std::string> GetUsedProgramKeys() override;
	void Precompile(const std::vector<std::string>& theKeys) override;

	void DrawPrimitiveUP(GPUPrimitiveType theType, int thePrimitiveCount, unsigned long theFVF, const void* theVertices,
		int theStride) override;
	GPUBuffer CreateVertexBuffer(unsigned long theFVF, const void* theVertices, int theCount, int theStride) override;
	GPUBuffer CreateIndexBuffer(const unsigned short* theIndices, int theCount) override;
	void DeleteBuffer(GPUBuffer theBuffer) override;
	void DrawIndexedPrimitive(GPUPrimitiveType theType, GPUBuffer theVertices, GPUBuffer theIndices, int theStartIndex,
		int thePrimitiveCount) override;

private:
	void LogOnce(const std::string& theText);
	void SetDefaultStates();
	void MakeDeviceObjects();
	void MakeScreen();
	void EnsureDepth(int theWidth, int theHeight);
	void BindTarget(Image* theImage);
	int TargetWidth();
	int TargetHeight();
	TextureData* GetTexture(Image* theImage);
	void UploadTexture(MemoryImage* theImage, TextureData* theData);
	void RefreshTarget(MemoryImage* theImage, TextureData* theData);
	bool ReadFramebuffer(GLuint theFramebuffer, int theWidth, int theHeight, unsigned long* theBits);
	GLuint CompileShader(GLenum theType, const std::string& theSource, const std::string& theKey);
	Program* GetProgram(const VSKey& theVS, const PSKey& thePS);
	Program* BuildProgram(const std::string& theKey, const VSKey& theVS, const PSKey& thePS);
	const Shader* FindShader(const std::string& theName, bool theVertex);
	VSKey MakeVSKey(unsigned long theFVF);
	PSKey MakePSKey();
	void ApplyState();
	void ApplyUniforms(Program* theProgram, const VSKey& theVS);
	void BindTextures();
	void SetVertexAttributes(unsigned long theFVF, const unsigned char* theBase, int theStride);
	bool PrepareDraw(unsigned long theFVF, Program** theProgram);

	GLContext* mContext;
	GPUPolicy mPolicy;
	int mWidth, mHeight;
	bool mLost = false;

	bool mSceneAllowed = true;
	bool mInScene = false;
	int mSceneCount = 0;

	unsigned long mRenderStates[256];
	unsigned long mStageStates[kStages][33];
	unsigned long mSamplerStates[kStages][14];
	Image* mTextures[kStages] = {nullptr, nullptr, nullptr, nullptr};
	GPUMatrix mWorld, mView, mProjection, mTextureMatrices[kStages];
	GPULight mLights[kLights];
	bool mLightEnabled[kLights] = {};
	GPUMaterial mMaterial;
	Rect mViewport;
	float mMinZ = 0.0f, mMaxZ = 1.0f;
	Rect mScissor;

	std::vector<Shader> mShaders;
	GPUVertexShader mVertexShader = 0;
	GPUPixelShader mPixelShader = 0;
	float mVSConstants[kVSConstants][4] = {};
	float mPSConstants[kPSConstants][4] = {};
	unsigned long long mVSSerial = 1, mPSSerial = 1;
	std::map<std::string, Program> mPrograms;
	std::vector<std::string> mUsedKeys;

	std::map<Image*, TextureData> mImages;
	Image* mTarget = nullptr;
	GLuint mScreenFramebuffer = 0, mScreenColor = 0;
	GLuint mDepth = 0;
	int mDepthWidth = 0, mDepthHeight = 0;
	GLuint mVertexArray = 0, mStreamBuffer = 0, mStreamIndexBuffer = 0;
	GLuint mSamplers[kStages] = {};
	std::map<int, BufferData> mBuffers;
	int mNextBuffer = 1;

	std::set<std::string> mLogged;
};

GPUInterfaceGL::GPUInterfaceGL(GLContext* theContext, const GPUPolicy& thePolicy, int theWidth, int theHeight)
	: mContext(theContext), mPolicy(thePolicy), mWidth(theWidth), mHeight(theHeight)
{
	mWorld = mView = mProjection = GPUMatrix::Identity();
	for (GPUMatrix& aMatrix : mTextureMatrices)
		aMatrix = GPUMatrix::Identity();
	std::memset(mLights, 0, sizeof(mLights));
	std::memset(&mMaterial, 0, sizeof(mMaterial));
	SetDefaultStates();
}

GPUInterfaceGL::~GPUInterfaceGL()
{
	if (mLost)
		return;
	for (auto& anEntry : mImages) {
		gGL.glDeleteTextures(1, &anEntry.second.mTexture);
		if (anEntry.second.mFramebuffer != 0)
			gGL.glDeleteFramebuffers(1, &anEntry.second.mFramebuffer);
	}
	for (auto& anEntry : mPrograms)
		gGL.glDeleteProgram(anEntry.second.mProgram);
	for (auto& anEntry : mBuffers)
		gGL.glDeleteBuffers(1, &anEntry.second.mBuffer);
	gGL.glDeleteFramebuffers(1, &mScreenFramebuffer);
	gGL.glDeleteRenderbuffers(1, &mScreenColor);
	gGL.glDeleteRenderbuffers(1, &mDepth);
	gGL.glDeleteBuffers(1, &mStreamBuffer);
	gGL.glDeleteBuffers(1, &mStreamIndexBuffer);
	gGL.glDeleteVertexArrays(1, &mVertexArray);
	gGL.glDeleteSamplers(kStages, mSamplers);
}

void GPUInterfaceGL::LogOnce(const std::string& theText)
{
	if (mLogged.insert(theText).second)
		SDL_Log("GPU: %s", theText.c_str());
}

// Direct3D 9's defaults for a new device
void GPUInterfaceGL::SetDefaultStates()
{
	std::memset(mRenderStates, 0, sizeof(mRenderStates));
	mRenderStates[GPURS_ZENABLE] = 1;
	mRenderStates[GPURS_FILLMODE] = 3;
	mRenderStates[GPURS_ZWRITEENABLE] = 1;
	mRenderStates[GPURS_SRCBLEND] = 2;
	mRenderStates[GPURS_DESTBLEND] = 1;
	mRenderStates[GPURS_CULLMODE] = 3;
	mRenderStates[GPURS_ZFUNC] = 4;
	mRenderStates[GPURS_ALPHAFUNC] = 8;
	mRenderStates[GPURS_TEXTUREFACTOR] = 0xffffffff;
	mRenderStates[GPURS_LIGHTING] = 1;
	mRenderStates[GPURS_COLORVERTEX] = 1;
	mRenderStates[GPURS_LOCALVIEWER] = 1;
	mRenderStates[GPURS_DIFFUSEMATERIALSOURCE] = 1;
	mRenderStates[GPURS_SPECULARMATERIALSOURCE] = 2;
	std::memset(mStageStates, 0, sizeof(mStageStates));
	for (int s = 0; s < kStages; s++) {
		mStageStates[s][GPUTSS_COLOROP] = s == 0 ? 4 : 1;
		mStageStates[s][GPUTSS_COLORARG1] = 2;
		mStageStates[s][GPUTSS_COLORARG2] = 1;
		mStageStates[s][GPUTSS_ALPHAOP] = s == 0 ? 2 : 1;
		mStageStates[s][GPUTSS_ALPHAARG1] = 2;
		mStageStates[s][GPUTSS_ALPHAARG2] = 1;
		mStageStates[s][GPUTSS_TEXCOORDINDEX] = s;
		mStageStates[s][GPUTSS_RESULTARG] = 1;
	}
	std::memset(mSamplerStates, 0, sizeof(mSamplerStates));
	for (int s = 0; s < kStages; s++) {
		mSamplerStates[s][GPUSAMP_ADDRESSU] = 1;
		mSamplerStates[s][GPUSAMP_ADDRESSV] = 1;
		mSamplerStates[s][GPUSAMP_MAGFILTER] = 1;
		mSamplerStates[s][GPUSAMP_MINFILTER] = 1;
	}
	mViewport = Rect(0, 0, mWidth, mHeight);
}

bool GPUInterfaceGL::Init()
{
	MakeDeviceObjects();
	return gGL.glGetError() == GL_NO_ERROR;
}

void GPUInterfaceGL::MakeDeviceObjects()
{
	gGL.glGenVertexArrays(1, &mVertexArray);
	gGL.glBindVertexArray(mVertexArray);
	gGL.glGenBuffers(1, &mStreamBuffer);
	gGL.glGenBuffers(1, &mStreamIndexBuffer);
	gGL.glGenSamplers(kStages, mSamplers);
	gGL.glDisable(GL_DITHER);
	gGL.glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	gGL.glPixelStorei(GL_PACK_ALIGNMENT, 4);
	MakeScreen();
	BindTarget(nullptr);
}

void GPUInterfaceGL::MakeScreen()
{
	// The screen is X8R8G8B8: an RGB target reads alpha 1 when copied or read back
	gGL.glGenRenderbuffers(1, &mScreenColor);
	gGL.glBindRenderbuffer(GL_RENDERBUFFER, mScreenColor);
	gGL.glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, mWidth, mHeight);
	gGL.glGenFramebuffers(1, &mScreenFramebuffer);
	gGL.glBindFramebuffer(GL_FRAMEBUFFER, mScreenFramebuffer);
	gGL.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, mScreenColor);
	EnsureDepth(mWidth, mHeight);
}

// One depth buffer for the screen and every target, as D3D9 keeps its one depth surface bound across SetRenderTarget.
// ES 3.0 and WebGL2 take attachments of different sizes (drawing covers their intersection).
void GPUInterfaceGL::EnsureDepth(int theWidth, int theHeight)
{
	if (mDepth != 0 && theWidth <= mDepthWidth && theHeight <= mDepthHeight)
		return;
	mDepthWidth = std::max(mDepthWidth, theWidth);
	mDepthHeight = std::max(mDepthHeight, theHeight);
	if (mDepth != 0)
		gGL.glDeleteRenderbuffers(1, &mDepth);
	gGL.glGenRenderbuffers(1, &mDepth);
	gGL.glBindRenderbuffer(GL_RENDERBUFFER, mDepth);
	gGL.glRenderbufferStorage(GL_RENDERBUFFER, mPolicy.mDepthBits <= 16 ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24,
		mDepthWidth, mDepthHeight);
}

int GPUInterfaceGL::TargetWidth()
{
	return mTarget == nullptr ? mWidth : mTarget->mWidth;
}

int GPUInterfaceGL::TargetHeight()
{
	return mTarget == nullptr ? mHeight : mTarget->mHeight;
}

void GPUInterfaceGL::BindTarget(Image* theImage)
{
	GLuint aFramebuffer = mScreenFramebuffer;
	int aWidth = mWidth, aHeight = mHeight;
	if (theImage != nullptr) {
		TextureData& aData = mImages[theImage];
		aFramebuffer = aData.mFramebuffer;
		aWidth = aData.mWidth;
		aHeight = aData.mHeight;
	}
	EnsureDepth(aWidth, aHeight);
	gGL.glBindFramebuffer(GL_FRAMEBUFFER, aFramebuffer);
	gGL.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepth);
	if (gGL.glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		LogOnce("a render target's framebuffer is not complete");
	mTarget = theImage;
	// D3D9's SetRenderTarget resets the viewport to the whole target
	SetViewport(Rect(0, 0, aWidth, aHeight), 0.0f, 1.0f);
}

void GPUInterfaceGL::Resize(int theWidth, int theHeight)
{
	mWidth = theWidth;
	mHeight = theHeight;
	gGL.glDeleteFramebuffers(1, &mScreenFramebuffer);
	gGL.glDeleteRenderbuffers(1, &mScreenColor);
	MakeScreen();
	BindTarget(mTarget);
}

void GPUInterfaceGL::ContextLost()
{
	// The GL names died with the context: forget them without deleting
	mLost = true;
	for (auto& anEntry : mImages) {
		anEntry.second.mTexture = 0;
		anEntry.second.mFramebuffer = 0;
		anEntry.second.mBitsChangedCount = -1;
	}
	mPrograms.clear();
	for (auto& anEntry : mBuffers)
		anEntry.second.mBuffer = 0;
	mScreenFramebuffer = mScreenColor = mDepth = 0;
	mDepthWidth = mDepthHeight = 0;
	mVertexArray = mStreamBuffer = mStreamIndexBuffer = 0;
	std::memset(mSamplers, 0, sizeof(mSamplers));
}

void GPUInterfaceGL::ContextRestored()
{
	mLost = false;
	MakeDeviceObjects();
	for (auto& anEntry : mBuffers) {
		BufferData& aData = anEntry.second;
		gGL.glGenBuffers(1, &aData.mBuffer);
		GLenum aKind = aData.mIndex ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
		gGL.glBindBuffer(aKind, aData.mBuffer);
		gGL.glBufferData(aKind, (GLsizeiptr)aData.mData.size(), aData.mData.data(), GL_STATIC_DRAW);
	}
	// Targets are made again, empty
	Image* aTarget = mTarget;
	for (auto& anEntry : mImages) {
		if (anEntry.second.mTarget) {
			anEntry.second.mTarget = false;
			CreateRenderTarget(anEntry.first, anEntry.second.mFormat);
		}
	}
	BindTarget(mImages.count(aTarget) ? aTarget : nullptr);
}

bool GPUInterfaceGL::BeginSceneIfNeeded()
{
	if (!mSceneAllowed)
		return false;
	if (mInScene)
		return true;
	// PreDraw (0x61a7d0) binds the device's base target, the screen, before the scene begins
	BindTarget(nullptr);
	mInScene = true;
	mSceneCount++;
	for (const GPUStateSet& aSet : mPolicy.mSceneBegin) {
		if (aSet.mKind == GPUStateSet::RENDER)
			SetRenderState(aSet.mType, aSet.mValue);
		else if (aSet.mKind == GPUStateSet::STAGE)
			SetTextureStageState(aSet.mStage, aSet.mType, aSet.mValue);
		else
			SetSamplerState(aSet.mStage, aSet.mType, aSet.mValue);
	}
	return true;
}

void GPUInterfaceGL::EndScene()
{
	// 0x6225a0: only a scene in progress ends, and it unbinds both shaders
	if (!mInScene)
		return;
	mInScene = false;
	mVertexShader = 0;
	mPixelShader = 0;
}

// An image's pixels as the GL texture takes them (RGBA bytes), from its ARGB bits
static std::vector<unsigned char> RGBAOf(const unsigned long* theBits, int theCount)
{
	std::vector<unsigned char> aPixels((size_t)theCount * 4);
	for (int i = 0; i < theCount; i++) {
		unsigned long c = theBits[i];
		unsigned char* q = &aPixels[(size_t)i * 4];
		q[0] = (unsigned char)(c >> 16);
		q[1] = (unsigned char)(c >> 8);
		q[2] = (unsigned char)c;
		q[3] = (unsigned char)(c >> 24);
	}
	return aPixels;
}

bool GPUInterfaceGL::CreateRenderTarget(Image* theImage, GPUFormat theFormat)
{
	TextureData& aData = mImages[theImage];
	if (aData.mTarget)
		return true;
	if (aData.mTexture != 0)
		gGL.glDeleteTextures(1, &aData.mTexture);
	// A target's texture is exactly its size, never cut into pieces. X8R8G8B8 is RGB8: it samples and reads alpha 1.
	if (theFormat != GPUFMT_A8R8G8B8 && theFormat != GPUFMT_X8R8G8B8) {
		LogOnce("a render target in format " + std::to_string(theFormat) + " (made A8R8G8B8)");
		theFormat = GPUFMT_A8R8G8B8;
	}
	aData.mWidth = theImage->mWidth;
	aData.mHeight = theImage->mHeight;
	aData.mTarget = true;
	aData.mFormat = theFormat;
	// Its first contents: its bits for A8R8G8B8 (0x61c070's target branch copies them), else black
	std::vector<unsigned char> aPixels((size_t)aData.mWidth * aData.mHeight * 4, 0);
	MemoryImage* aMemoryImage = dynamic_cast<MemoryImage*>(theImage);
	if (aMemoryImage != nullptr) {
		aData.mBitsChangedCount = aMemoryImage->mBitsChangedCount;
		if (theFormat == GPUFMT_A8R8G8B8 && aMemoryImage->mBits != nullptr)
			aPixels = RGBAOf(aMemoryImage->mBits, aData.mWidth * aData.mHeight);
	}
	bool isOpaque = theFormat == GPUFMT_X8R8G8B8;
	gGL.glGenTextures(1, &aData.mTexture);
	gGL.glBindTexture(GL_TEXTURE_2D, aData.mTexture);
	gGL.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	gGL.glTexImage2D(GL_TEXTURE_2D, 0, isOpaque ? GL_RGB8 : GL_RGBA8, aData.mWidth, aData.mHeight, 0,
		isOpaque ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, aPixels.data());
	gGL.glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	gGL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	gGL.glGenFramebuffers(1, &aData.mFramebuffer);
	gGL.glBindFramebuffer(GL_FRAMEBUFFER, aData.mFramebuffer);
	gGL.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, aData.mTexture, 0);
	BindTarget(mTarget);
	return gGL.glGetError() == GL_NO_ERROR;
}

// 0x61c070's target branch, when a target's bits changed since its texture last took them: an A8R8G8B8 target is
// filled from its bits; an X8R8G8B8 one is not (0x61b5a0 has no case for its format), so it keeps its pixels
void GPUInterfaceGL::RefreshTarget(MemoryImage* theImage, TextureData* theData)
{
	if (theImage == nullptr || theData->mBitsChangedCount == theImage->mBitsChangedCount)
		return;
	theData->mBitsChangedCount = theImage->mBitsChangedCount;
	if (theData->mFormat != GPUFMT_A8R8G8B8) {
		LogOnce("the bits of an X8R8G8B8 render target changed (not copied, as in the original)");
		return;
	}
	if (theImage->mBits == nullptr)
		return;
	std::vector<unsigned char> aPixels = RGBAOf(theImage->mBits, theData->mWidth * theData->mHeight);
	gGL.glBindTexture(GL_TEXTURE_2D, theData->mTexture);
	gGL.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, theData->mWidth, theData->mHeight, GL_RGBA, GL_UNSIGNED_BYTE,
		aPixels.data());
}

bool GPUInterfaceGL::SetRenderTarget(Image* theImage)
{
	if (theImage == nullptr) {
		// For the screen, the answer is PreDraw's (0x621990 through 0x621890)
		if (!BeginSceneIfNeeded())
			return false;
		if (mTarget != nullptr)
			BindTarget(nullptr);
		return true;
	}
	auto anIt = mImages.find(theImage);
	if (anIt == mImages.end() || !anIt->second.mTarget) {
		LogOnce("SetRenderTarget on an image that is not a render target");
		return false;
	}
	RefreshTarget(dynamic_cast<MemoryImage*>(theImage), &anIt->second);
	// For an image, true once its texture exists, whether or not PreDraw lets the target change; the bound target set
	// again changes nothing (0x621890 returns at once)
	if (BeginSceneIfNeeded() && mTarget != theImage)
		BindTarget(theImage);
	return true;
}

void GPUInterfaceGL::SetVertexShader(GPUVertexShader theShader)
{
	// The original's SetVertexShader (0x622ea0) runs PreDraw first
	BeginSceneIfNeeded();
	mVertexShader = theShader;
}

void GPUInterfaceGL::StretchRect(Image* theSource, const Rect& theSrcRect, Image* theDest, const Rect& theDestRect,
	GPUTextureFilter theFilter)
{
	// 0x621dc0 runs PreDraw first, and copies nothing unless the source is a target
	BeginSceneIfNeeded();
	GLuint aSource = theSource == nullptr ? mScreenFramebuffer : mImages[theSource].mFramebuffer;
	GLuint aDest = theDest == nullptr ? mScreenFramebuffer : mImages[theDest].mFramebuffer;
	if (aSource == 0 || aDest == 0) {
		LogOnce("StretchRect between images that are not render targets");
		return;
	}
	gGL.glDisable(GL_SCISSOR_TEST);
	gGL.glBindFramebuffer(GL_READ_FRAMEBUFFER, aSource);
	gGL.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, aDest);
	// Both are stored top row first, so D3D's rectangles go as they are
	gGL.glBlitFramebuffer(theSrcRect.mX, theSrcRect.mY, theSrcRect.mX + theSrcRect.mWidth, theSrcRect.mY + theSrcRect.mHeight,
		theDestRect.mX, theDestRect.mY, theDestRect.mX + theDestRect.mWidth, theDestRect.mY + theDestRect.mHeight,
		GL_COLOR_BUFFER_BIT, theFilter == GPUTEXF_LINEAR ? GL_LINEAR : GL_NEAREST);
	gGL.glBindFramebuffer(GL_FRAMEBUFFER, mTarget == nullptr ? mScreenFramebuffer : mImages[mTarget].mFramebuffer);
}

void GPUInterfaceGL::Clear(unsigned long theFlags, unsigned long theColor, float theZ)
{
	// D3D9's Clear covers the viewport, within the scissor rectangle when the scissor test is on, and ignores the
	// write masks
	Rect anArea = mViewport;
	if (mRenderStates[GPURS_SCISSORTESTENABLE])
		anArea = anArea.Intersection(mScissor);
	gGL.glEnable(GL_SCISSOR_TEST);
	gGL.glScissor(anArea.mX, anArea.mY, anArea.mWidth, anArea.mHeight);
	GLbitfield aMask = 0;
	if (theFlags & GPUCLEAR_TARGET) {
		gGL.glColorMask(1, 1, 1, 1);
		gGL.glClearColor(((theColor >> 16) & 0xff) / 255.0f, ((theColor >> 8) & 0xff) / 255.0f, (theColor & 0xff) / 255.0f,
			((theColor >> 24) & 0xff) / 255.0f);
		aMask |= GL_COLOR_BUFFER_BIT;
	}
	if (theFlags & GPUCLEAR_ZBUFFER) {
		gGL.glDepthMask(1);
		gGL.glClearDepthf(theZ);
		aMask |= GL_DEPTH_BUFFER_BIT;
	}
	gGL.glClear(aMask);
}

void GPUInterfaceGL::SetViewport(const Rect& theRect, float theMinZ, float theMaxZ)
{
	mViewport = theRect;
	mMinZ = theMinZ;
	mMaxZ = theMaxZ;
	// GL's window y is D3D's: the targets are stored top row first
	gGL.glViewport(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
	gGL.glDepthRangef(theMinZ, theMaxZ);
}

bool GPUInterfaceGL::ReadBack(Image* theImage, MemoryImage* theDest)
{
	GLuint aFramebuffer = mScreenFramebuffer;
	int aWidth = mWidth, aHeight = mHeight;
	if (theImage != nullptr) {
		auto anIt = mImages.find(theImage);
		if (anIt == mImages.end() || !anIt->second.mTarget)
			return false;
		aFramebuffer = anIt->second.mFramebuffer;
		aWidth = anIt->second.mWidth;
		aHeight = anIt->second.mHeight;
	}
	SDL_Log("GPU: read back %s (%dx%d)", theImage == nullptr ? "the screen" : "a render target", aWidth, aHeight);
	theDest->Create(aWidth, aHeight);
	bool isRead = ReadFramebuffer(aFramebuffer, aWidth, aHeight, theDest->GetBits());
	theDest->BitsChanged();
	return isRead;
}

// A framebuffer's pixels into ARGB words, D3D's row 0 first
bool GPUInterfaceGL::ReadFramebuffer(GLuint theFramebuffer, int theWidth, int theHeight, unsigned long* theBits)
{
	std::vector<unsigned char> aPixels((size_t)theWidth * theHeight * 4);
	gGL.glBindFramebuffer(GL_READ_FRAMEBUFFER, theFramebuffer);
	gGL.glReadPixels(0, 0, theWidth, theHeight, GL_RGBA, GL_UNSIGNED_BYTE, aPixels.data());
	gGL.glBindFramebuffer(GL_FRAMEBUFFER, mTarget == nullptr ? mScreenFramebuffer : mImages[mTarget].mFramebuffer);
	for (int i = 0; i < theWidth * theHeight; i++) {
		const unsigned char* p = &aPixels[(size_t)i * 4];
		theBits[i] = ((unsigned long)p[3] << 24) | ((unsigned long)p[0] << 16) | ((unsigned long)p[1] << 8) | p[2];
	}
	return gGL.glGetError() == GL_NO_ERROR;
}

// The original's read-back (slot 0x48), which its GetBits uses for an image that has a texture but no bits: the
// texture's pixels as they are stored (a 16-bit texture gives its quantised colours)
bool GPUInterfaceGL::RecoverBits(MemoryImage* theImage)
{
	auto anIt = mImages.find(theImage);
	if (mLost || anIt == mImages.end() || anIt->second.mTexture == 0 || theImage->mBits == nullptr)
		return false;
	TextureData& aData = anIt->second;
	if (aData.mWidth != theImage->mWidth || aData.mHeight != theImage->mHeight)
		return false;
	SDL_Log("GPU: bits recovered from %s (%dx%d)", aData.mTarget ? "a render target" : "a texture", aData.mWidth,
		aData.mHeight);
	GLuint aFramebuffer = aData.mFramebuffer;
	if (aFramebuffer == 0) {
		gGL.glGenFramebuffers(1, &aFramebuffer);
		gGL.glBindFramebuffer(GL_FRAMEBUFFER, aFramebuffer);
		gGL.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, aData.mTexture, 0);
	}
	bool isRead = ReadFramebuffer(aFramebuffer, aData.mWidth, aData.mHeight, theImage->mBits);
	if (aFramebuffer != aData.mFramebuffer)
		gGL.glDeleteFramebuffers(1, &aFramebuffer);
	return isRead;
}

void GPUInterfaceGL::BitsChanged(Image* theImage)
{
	// A texture is made again at its next use; a target takes its bits then as RefreshTarget says
	auto anIt = mImages.find(theImage);
	if (anIt != mImages.end())
		anIt->second.mBitsChangedCount = -1;
}

void GPUInterfaceGL::ImageDeleted(Image* theImage)
{
	auto anIt = mImages.find(theImage);
	if (anIt == mImages.end())
		return;
	for (Image*& aTexture : mTextures) {
		if (aTexture == theImage)
			aTexture = nullptr;
	}
	if (mTarget == theImage)
		BindTarget(nullptr);
	if (!mLost) {
		gGL.glDeleteTextures(1, &anIt->second.mTexture);
		if (anIt->second.mFramebuffer != 0)
			gGL.glDeleteFramebuffers(1, &anIt->second.mFramebuffer);
	}
	mImages.erase(anIt);
}

TextureData* GPUInterfaceGL::GetTexture(Image* theImage)
{
	if (theImage == nullptr)
		return nullptr;
	TextureData& aData = mImages[theImage];
	if (aData.mTarget) {
		if (theImage == mTarget)
			LogOnce("an image is sampled while it is the render target");
		RefreshTarget(dynamic_cast<MemoryImage*>(theImage), &aData);
		return &aData;
	}
	MemoryImage* aMemoryImage = dynamic_cast<MemoryImage*>(theImage);
	if (aMemoryImage == nullptr) {
		LogOnce("a texture that is not a MemoryImage");
		return nullptr;
	}
	if (aData.mTexture == 0 || aData.mBitsChangedCount != aMemoryImage->mBitsChangedCount)
		UploadTexture(aMemoryImage, &aData);
	return &aData;
}

// The image's bits as RGBA, in the format the policy's rule picks: 16-bit formats are quantised as D3D9 stores them and
// expanded back as it samples them
void GPUInterfaceGL::UploadTexture(MemoryImage* theImage, TextureData* theData)
{
	int aWidth = theImage->mWidth, aHeight = theImage->mHeight;
	theImage->CommitBits();  // mHasAlpha and mHasTrans, as SAF's texture upload takes them
	unsigned long* aBits = theImage->GetBits();
	if (aBits == nullptr)
		return;
	GPUFormat aFormat = mPolicy.TextureFormat(theImage->mHasAlpha, theImage->mHasTrans, theImage->mD3DFlags);
	std::vector<unsigned char> aPixels((size_t)aWidth * aHeight * 4);
	for (int i = 0; i < aWidth * aHeight; i++) {
		unsigned long c = aBits[i];
		unsigned int a = (c >> 24) & 0xff, r = (c >> 16) & 0xff, g = (c >> 8) & 0xff, b = c & 0xff;
		if (aFormat == GPUFMT_R5G6B5) {
			r = ((r >> 3) << 3) | (r >> 5);
			g = ((g >> 2) << 2) | (g >> 6);
			b = ((b >> 3) << 3) | (b >> 5);
			a = 255;
		} else if (aFormat == GPUFMT_X8R8G8B8) {
			a = 255;
		} else if (aFormat == GPUFMT_A4R4G4B4) {
			r = (r >> 4) * 17;
			g = (g >> 4) * 17;
			b = (b >> 4) * 17;
			a = (a >> 4) * 17;
		}
		unsigned char* p = &aPixels[(size_t)i * 4];
		p[0] = (unsigned char)r;
		p[1] = (unsigned char)g;
		p[2] = (unsigned char)b;
		p[3] = (unsigned char)a;
	}
	if (theData->mTexture == 0 || theData->mWidth != aWidth || theData->mHeight != aHeight) {
		if (theData->mTexture != 0)
			gGL.glDeleteTextures(1, &theData->mTexture);
		gGL.glGenTextures(1, &theData->mTexture);
		gGL.glBindTexture(GL_TEXTURE_2D, theData->mTexture);
		gGL.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, aWidth, aHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, aPixels.data());
		gGL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	} else {
		gGL.glBindTexture(GL_TEXTURE_2D, theData->mTexture);
		gGL.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, aWidth, aHeight, GL_RGBA, GL_UNSIGNED_BYTE, aPixels.data());
	}
	theData->mWidth = aWidth;
	theData->mHeight = aHeight;
	theData->mBitsChangedCount = theImage->mBitsChangedCount;
}

void GPUInterfaceGL::SetRenderState(int theState, unsigned long theValue)
{
	if (theState < 0 || theState >= 256)
		return;
	if (theState == GPURS_FILLMODE && theValue != 3)
		LogOnce("FILLMODE other than SOLID (drawn solid)");
	mRenderStates[theState] = theValue;
}

void GPUInterfaceGL::SetTextureStageState(int theStage, int theType, unsigned long theValue)
{
	if (theStage < 0 || theStage >= kStages || theType < 0 || theType >= 33)
		return;
	if ((theType == GPUTSS_COLOROP || theType == GPUTSS_ALPHAOP) && (theValue == 17 || (theValue >= 18 && theValue <= 23)))
		LogOnce("texture stage operation " + std::to_string(theValue) + " (taken as an approximation)");
	mStageStates[theStage][theType] = theValue;
}

void GPUInterfaceGL::SetSamplerState(int theStage, int theType, unsigned long theValue)
{
	if (theStage < 0 || theStage >= kStages || theType < 0 || theType >= 14)
		return;
	mSamplerStates[theStage][theType] = theValue;
}

void GPUInterfaceGL::SetTexture(int theStage, Image* theImage)
{
	if (theStage >= 0 && theStage < kStages)
		mTextures[theStage] = theImage;
}

void GPUInterfaceGL::SetTransform(int theType, const GPUMatrix& theMatrix)
{
	if (theType == GPUTS_WORLD)
		mWorld = theMatrix;
	else if (theType == GPUTS_VIEW)
		mView = theMatrix;
	else if (theType == GPUTS_PROJECTION)
		mProjection = theMatrix;
	else if (theType >= GPUTS_TEXTURE0 && theType < GPUTS_TEXTURE0 + kStages)
		mTextureMatrices[theType - GPUTS_TEXTURE0] = theMatrix;
}

void GPUInterfaceGL::SetLight(int theIndex, const GPULight& theLight)
{
	if (theIndex < 0 || theIndex >= kLights)
		return;
	if (theLight.mType == 2)
		LogOnce("a spot light (drawn as a point light)");
	mLights[theIndex] = theLight;
}

void GPUInterfaceGL::LightEnable(int theIndex, bool theEnable)
{
	if (theIndex >= 0 && theIndex < kLights)
		mLightEnabled[theIndex] = theEnable;
}

GPUVertexShader GPUInterfaceGL::CreateVertexShader(const std::string& theName, const char* theSource)
{
	mShaders.push_back(Shader{theName, theSource, true});
	return (GPUVertexShader)mShaders.size();
}

GPUPixelShader GPUInterfaceGL::CreatePixelShader(const std::string& theName, const char* theSource)
{
	mShaders.push_back(Shader{theName, theSource, false});
	return (GPUPixelShader)mShaders.size();
}

void GPUInterfaceGL::SetPixelShader(GPUPixelShader theShader)
{
	// The original's SetPixelShader (0x622dd0) runs PreDraw first
	BeginSceneIfNeeded();
	mPixelShader = theShader;
}

void GPUInterfaceGL::SetVertexShaderConstantF(int theRegister, const float* theValues, int theCount)
{
	for (int i = 0; i < theCount; i++) {
		if (theRegister + i >= 0 && theRegister + i < kVSConstants)
			std::memcpy(mVSConstants[theRegister + i], theValues + i * 4, sizeof(float) * 4);
	}
	mVSSerial++;
}

void GPUInterfaceGL::SetPixelShaderConstantF(int theRegister, const float* theValues, int theCount)
{
	for (int i = 0; i < theCount; i++) {
		if (theRegister + i >= 0 && theRegister + i < kPSConstants)
			std::memcpy(mPSConstants[theRegister + i], theValues + i * 4, sizeof(float) * 4);
	}
	mPSSerial++;
}

std::vector<std::string> GPUInterfaceGL::GetUsedProgramKeys()
{
	return mUsedKeys;
}

const Shader* GPUInterfaceGL::FindShader(const std::string& theName, bool theVertex)
{
	for (const Shader& aShader : mShaders) {
		if (aShader.mName == theName && aShader.mVertex == theVertex)
			return &aShader;
	}
	return nullptr;
}

void GPUInterfaceGL::Precompile(const std::vector<std::string>& theKeys)
{
	for (const std::string& aKey : theKeys) {
		size_t aBar = aKey.find('|');
		VSKey aVS;
		PSKey aPS;
		if (aBar == std::string::npos || !VSKey::Parse(aKey.substr(0, aBar), &aVS) ||
			!PSKey::Parse(aKey.substr(aBar + 1), &aPS))
			continue;
		if ((aVS.mKind == VSKey::GAME && FindShader(aVS.mGame, true) == nullptr) ||
			(aPS.mKind == PSKey::GAME && FindShader(aPS.mGame, false) == nullptr))
			continue;
		GetProgram(aVS, aPS);
	}
}

GLuint GPUInterfaceGL::CompileShader(GLenum theType, const std::string& theSource, const std::string& theKey)
{
	GLuint aShader = gGL.glCreateShader(theType);
	const GLchar* aText = theSource.c_str();
	gGL.glShaderSource(aShader, 1, &aText, nullptr);
	gGL.glCompileShader(aShader);
	GLint anOk = 0;
	gGL.glGetShaderiv(aShader, GL_COMPILE_STATUS, &anOk);
	if (!anOk) {
		char aLog[2048] = {};
		gGL.glGetShaderInfoLog(aShader, sizeof(aLog) - 1, nullptr, aLog);
		SDL_Log("GPU: a %s shader for %s does not compile:\n%s\n%s", theType == GL_VERTEX_SHADER ? "vertex" : "pixel",
			theKey.c_str(), aLog, theSource.c_str());
		gGL.glDeleteShader(aShader);
		return 0;
	}
	return aShader;
}

Program* GPUInterfaceGL::GetProgram(const VSKey& theVS, const PSKey& thePS)
{
	std::string aKey = theVS.Text() + "|" + thePS.Text();
	auto anIt = mPrograms.find(aKey);
	if (anIt != mPrograms.end())
		return anIt->second.mProgram != 0 ? &anIt->second : nullptr;
	return BuildProgram(aKey, theVS, thePS);
}

Program* GPUInterfaceGL::BuildProgram(const std::string& theKey, const VSKey& theVS, const PSKey& thePS)
{
	Program& aProgram = mPrograms[theKey];  // a failed build stays as 0, so it is not tried again every draw
	std::string aHeader = mContext->ShaderHeader();
	std::string aVSSource = aHeader + kVSPrelude;
	if (theVS.mKind == VSKey::GAME) {
		const Shader* aShader = FindShader(theVS.mGame, true);
		if (aShader == nullptr)
			return nullptr;
		aVSSource += GenerateGameVS(aShader->mSource);
	} else {
		aVSSource += GenerateFixedVS(theVS);
	}
	std::string aPSSource = aHeader + kPSPrelude;
	if (thePS.mKind == PSKey::GAME) {
		const Shader* aShader = FindShader(thePS.mGame, false);
		if (aShader == nullptr)
			return nullptr;
		aPSSource += GenerateGamePS(aShader->mSource);
	} else {
		aPSSource += GenerateCombinersPS(thePS);
	}
	GLuint aVertex = CompileShader(GL_VERTEX_SHADER, aVSSource, theKey);
	GLuint aPixel = CompileShader(GL_FRAGMENT_SHADER, aPSSource, theKey);
	if (aVertex == 0 || aPixel == 0) {
		if (aVertex != 0)
			gGL.glDeleteShader(aVertex);
		if (aPixel != 0)
			gGL.glDeleteShader(aPixel);
		return nullptr;
	}
	GLuint aLinked = gGL.glCreateProgram();
	gGL.glAttachShader(aLinked, aVertex);
	gGL.glAttachShader(aLinked, aPixel);
	gGL.glBindAttribLocation(aLinked, ATTR_POSITION, "aPosition");
	gGL.glBindAttribLocation(aLinked, ATTR_NORMAL, "aNormal");
	gGL.glBindAttribLocation(aLinked, ATTR_DIFFUSE, "aDiffuseBGRA");
	gGL.glBindAttribLocation(aLinked, ATTR_SPECULAR, "aSpecularBGRA");
	for (int i = 0; i < kStages; i++)
		gGL.glBindAttribLocation(aLinked, ATTR_TEX0 + i, TexAttr(i).c_str());
	gGL.glLinkProgram(aLinked);
	gGL.glDeleteShader(aVertex);
	gGL.glDeleteShader(aPixel);
	GLint anOk = 0;
	gGL.glGetProgramiv(aLinked, GL_LINK_STATUS, &anOk);
	if (!anOk) {
		char aLog[2048] = {};
		gGL.glGetProgramInfoLog(aLinked, sizeof(aLog) - 1, nullptr, aLog);
		SDL_Log("GPU: the program for %s does not link: %s", theKey.c_str(), aLog);
		gGL.glDeleteProgram(aLinked);
		return nullptr;
	}
	aProgram.mProgram = aLinked;
	auto Location = [&](const char* theName) { return gGL.glGetUniformLocation(aLinked, theName); };
	aProgram.mViewport = Location("uViewport");
	aProgram.mVS = Location("uVS");
	aProgram.mPS = Location("uPS");
	aProgram.mTFactor = Location("uTFactor");
	aProgram.mAlphaTest = Location("uAlphaTest");
	aProgram.mWorldView = Location("uWorldView");
	aProgram.mWorldViewProj = Location("uWorldViewProj");
	aProgram.mNormalMatrix = Location("uNormalMatrix");
	aProgram.mTexMatrix = Location("uTexMatrix");
	aProgram.mLightPos = Location("uLightPos");
	aProgram.mLightDir = Location("uLightDir");
	aProgram.mLightDiffuse = Location("uLightDiffuse");
	aProgram.mLightSpecular = Location("uLightSpecular");
	aProgram.mLightAmbient = Location("uLightAmbient");
	aProgram.mLightAtt = Location("uLightAtt");
	aProgram.mMatDiffuse = Location("uMatDiffuse");
	aProgram.mMatAmbient = Location("uMatAmbient");
	aProgram.mMatSpecular = Location("uMatSpecular");
	aProgram.mMatEmissive = Location("uMatEmissive");
	aProgram.mMatPower = Location("uMatPower");
	aProgram.mAmbient = Location("uAmbient");
	gGL.glUseProgram(aLinked);
	for (int i = 0; i < kStages; i++) {
		aProgram.mSamplers[i] = Location(("uSampler" + std::to_string(i)).c_str());
		if (aProgram.mSamplers[i] >= 0)
			gGL.glUniform1i(aProgram.mSamplers[i], i);
	}
	mUsedKeys.push_back(theKey);
	return &aProgram;
}

VSKey GPUInterfaceGL::MakeVSKey(unsigned long theFVF)
{
	VSKey aKey;
	aKey.mFVF = theFVF;
	if (mVertexShader != 0) {
		aKey.mKind = VSKey::GAME;
		aKey.mGame = mShaders[mVertexShader - 1].mName;
		return aKey;
	}
	for (int s = 0; s < kStages; s++)
		aKey.mTexCoordIndex[s] = mStageStates[s][GPUTSS_TEXCOORDINDEX];
	if ((theFVF & 0xe) == GPUFVF_XYZRHW) {
		aKey.mKind = VSKey::PRETRANSFORMED;
		for (int s = 0; s < kStages; s++)
			aKey.mTexCoordIndex[s] &= 0xffff;
		return aKey;
	}
	aKey.mLighting = mRenderStates[GPURS_LIGHTING] != 0;
	for (int s = 0; s < kStages; s++)
		aKey.mTextureTransform[s] = mStageStates[s][GPUTSS_TEXTURETRANSFORMFLAGS];
	if (aKey.mLighting) {
		aKey.mColorVertex = mRenderStates[GPURS_COLORVERTEX] != 0;
		aKey.mNormalize = mRenderStates[GPURS_NORMALIZENORMALS] != 0;
		aKey.mLocalViewer = mRenderStates[GPURS_LOCALVIEWER] != 0;
		aKey.mSpecular = mRenderStates[GPURS_SPECULARENABLE] != 0;
		aKey.mDiffuseSource = mRenderStates[GPURS_DIFFUSEMATERIALSOURCE];
		aKey.mSpecularSource = mRenderStates[GPURS_SPECULARMATERIALSOURCE];
		aKey.mAmbientSource = mRenderStates[GPURS_AMBIENTMATERIALSOURCE];
		aKey.mEmissiveSource = mRenderStates[GPURS_EMISSIVEMATERIALSOURCE];
		for (int i = 0; i < kLights; i++) {
			if (mLightEnabled[i])
				aKey.mLightTypes.push_back(mLights[i].mType);
		}
	} else {
		aKey.mNormalize = mRenderStates[GPURS_NORMALIZENORMALS] != 0;
	}
	return aKey;
}

PSKey GPUInterfaceGL::MakePSKey()
{
	PSKey aKey;
	if (mPixelShader != 0) {
		aKey.mKind = PSKey::GAME;
		aKey.mGame = mShaders[mPixelShader - 1].mName;
		return aKey;
	}
	for (int s = 0; s < kStages; s++) {
		const unsigned long* v = mStageStates[s];
		if (v[GPUTSS_COLOROP] == 1)
			break;
		aKey.mStages.push_back({(long long)v[GPUTSS_COLOROP], (long long)v[GPUTSS_COLORARG1], (long long)v[GPUTSS_COLORARG2],
			(long long)v[GPUTSS_COLORARG0], (long long)v[GPUTSS_ALPHAOP], (long long)v[GPUTSS_ALPHAARG1],
			(long long)v[GPUTSS_ALPHAARG2], (long long)v[GPUTSS_ALPHAARG0], (long long)v[GPUTSS_RESULTARG] == 5 ? 5 : 1,
			mTextures[s] != nullptr ? 1 : 0});
	}
	aKey.mSpecularAdd = mRenderStates[GPURS_SPECULARENABLE] != 0;
	return aKey;
}

void GPUInterfaceGL::ApplyState()
{
	const unsigned long* rs = mRenderStates;
	if (rs[GPURS_ALPHABLENDENABLE]) {
		gGL.glEnable(GL_BLEND);
		gGL.glBlendFunc(BlendFactor(rs[GPURS_SRCBLEND], true), BlendFactor(rs[GPURS_DESTBLEND], false));
	} else {
		gGL.glDisable(GL_BLEND);
	}
	if (rs[GPURS_ZENABLE]) {
		gGL.glEnable(GL_DEPTH_TEST);
		gGL.glDepthFunc(CompareFunc(rs[GPURS_ZFUNC]));
	} else {
		gGL.glDisable(GL_DEPTH_TEST);
	}
	gGL.glDepthMask(rs[GPURS_ZWRITEENABLE] ? 1 : 0);
	gGL.glColorMask(1, 1, 1, 1);
	// D3D's clockwise (seen on a y-down screen) is GL's counter-clockwise in the same window coordinates
	gGL.glFrontFace(GL_CCW);
	if (rs[GPURS_CULLMODE] == 2) {
		gGL.glEnable(GL_CULL_FACE);
		gGL.glCullFace(GL_FRONT);
	} else if (rs[GPURS_CULLMODE] == 3) {
		gGL.glEnable(GL_CULL_FACE);
		gGL.glCullFace(GL_BACK);
	} else {
		gGL.glDisable(GL_CULL_FACE);
	}
	if (rs[GPURS_SCISSORTESTENABLE]) {
		gGL.glEnable(GL_SCISSOR_TEST);
		gGL.glScissor(mScissor.mX, mScissor.mY, mScissor.mWidth, mScissor.mHeight);
	} else {
		gGL.glDisable(GL_SCISSOR_TEST);
	}
}

static void ColorValue(const GPUColorValue& theValue, float theOut[4])
{
	theOut[0] = theValue.r;
	theOut[1] = theValue.g;
	theOut[2] = theValue.b;
	theOut[3] = theValue.a;
}

void GPUInterfaceGL::ApplyUniforms(Program* p, const VSKey& theVS)
{
	float aViewport[4] = {(float)mViewport.mX, (float)mViewport.mY, (float)mViewport.mWidth, (float)mViewport.mHeight};
	gGL.glUniform4fv(p->mViewport, 1, aViewport);
	unsigned long tf = mRenderStates[GPURS_TEXTUREFACTOR];
	float aFactor[4] = {((tf >> 16) & 0xff) / 255.0f, ((tf >> 8) & 0xff) / 255.0f, (tf & 0xff) / 255.0f,
		((tf >> 24) & 0xff) / 255.0f};
	gGL.glUniform4fv(p->mTFactor, 1, aFactor);
	float anAlphaTest[4] = {mRenderStates[GPURS_ALPHATESTENABLE] ? (float)mRenderStates[GPURS_ALPHAFUNC] : 8.0f,
		(float)(mRenderStates[GPURS_ALPHAREF] & 0xff), 0.0f, 0.0f};
	gGL.glUniform4fv(p->mAlphaTest, 1, anAlphaTest);
	// The constant banks are the device's: a program takes them again when they changed since it last drew
	if (p->mVS >= 0 && p->mVSSerial != mVSSerial) {
		gGL.glUniform4fv(p->mVS, kVSConstants, &mVSConstants[0][0]);
		p->mVSSerial = mVSSerial;
	}
	if (p->mPS >= 0 && p->mPSSerial != mPSSerial) {
		gGL.glUniform4fv(p->mPS, kPSConstants, &mPSConstants[0][0]);
		p->mPSSerial = mPSSerial;
	}
	if (theVS.mKind != VSKey::FIXED)
		return;
	GPUMatrix aWorldView = mWorld * mView;
	GPUMatrix aWorldViewProj = aWorldView * mProjection;
	GPUMatrix aNormal = NormalMatrix(aWorldView);
	// D3D's row-major matrices go up untransposed: GLSL's M * v then computes D3D's v * M
	gGL.glUniformMatrix4fv(p->mWorldView, 1, 0, &aWorldView.m[0][0]);
	gGL.glUniformMatrix4fv(p->mWorldViewProj, 1, 0, &aWorldViewProj.m[0][0]);
	gGL.glUniformMatrix4fv(p->mNormalMatrix, 1, 0, &aNormal.m[0][0]);
	gGL.glUniformMatrix4fv(p->mTexMatrix, kStages, 0, &mTextureMatrices[0].m[0][0]);
	if (!theVS.mLighting)
		return;
	float aPos[kLights][4] = {}, aDir[kLights][4] = {}, aDiffuse[kLights][4] = {}, aSpecular[kLights][4] = {},
		anAmbient[kLights][4] = {}, anAtt[kLights][4] = {};
	int n = 0;
	for (int i = 0; i < kLights; i++) {
		if (!mLightEnabled[i])
			continue;
		const GPULight& L = mLights[i];
		// Into camera space
		float aWorldPos[4] = {L.mPosition.x, L.mPosition.y, L.mPosition.z, 1.0f};
		TransformRow(aWorldPos, mView, aPos[n]);
		float aWorldDir[4] = {L.mDirection.x, L.mDirection.y, L.mDirection.z, 0.0f};
		TransformRow(aWorldDir, mView, aDir[n]);
		float aLength = std::sqrt(aDir[n][0] * aDir[n][0] + aDir[n][1] * aDir[n][1] + aDir[n][2] * aDir[n][2]);
		if (aLength > 0.0f) {
			for (int c = 0; c < 3; c++)
				aDir[n][c] /= aLength;
		}
		ColorValue(L.mDiffuse, aDiffuse[n]);
		ColorValue(L.mSpecular, aSpecular[n]);
		ColorValue(L.mAmbient, anAmbient[n]);
		anAtt[n][0] = L.mAttenuation0;
		anAtt[n][1] = L.mAttenuation1;
		anAtt[n][2] = L.mAttenuation2;
		anAtt[n][3] = L.mRange;
		n++;
	}
	gGL.glUniform4fv(p->mLightPos, kLights, &aPos[0][0]);
	gGL.glUniform4fv(p->mLightDir, kLights, &aDir[0][0]);
	gGL.glUniform4fv(p->mLightDiffuse, kLights, &aDiffuse[0][0]);
	gGL.glUniform4fv(p->mLightSpecular, kLights, &aSpecular[0][0]);
	gGL.glUniform4fv(p->mLightAmbient, kLights, &anAmbient[0][0]);
	gGL.glUniform4fv(p->mLightAtt, kLights, &anAtt[0][0]);
	float aMaterial[4];
	ColorValue(mMaterial.mDiffuse, aMaterial);
	gGL.glUniform4fv(p->mMatDiffuse, 1, aMaterial);
	ColorValue(mMaterial.mAmbient, aMaterial);
	gGL.glUniform4fv(p->mMatAmbient, 1, aMaterial);
	ColorValue(mMaterial.mSpecular, aMaterial);
	gGL.glUniform4fv(p->mMatSpecular, 1, aMaterial);
	ColorValue(mMaterial.mEmissive, aMaterial);
	gGL.glUniform4fv(p->mMatEmissive, 1, aMaterial);
	gGL.glUniform1f(p->mMatPower, mMaterial.mPower);
	unsigned long am = mRenderStates[GPURS_AMBIENT];
	float anAmbientColor[4] = {((am >> 16) & 0xff) / 255.0f, ((am >> 8) & 0xff) / 255.0f, (am & 0xff) / 255.0f,
		((am >> 24) & 0xff) / 255.0f};
	gGL.glUniform4fv(p->mAmbient, 1, anAmbientColor);
}

void GPUInterfaceGL::BindTextures()
{
	for (int s = 0; s < kStages; s++) {
		TextureData* aData = GetTexture(mTextures[s]);
		gGL.glActiveTexture(GL_TEXTURE0 + s);
		gGL.glBindTexture(GL_TEXTURE_2D, aData != nullptr ? aData->mTexture : 0);
		// Sampler state belongs to the stage, as in D3D9
		const unsigned long* v = mSamplerStates[s];
		auto Address = [](unsigned long theMode) -> GLint {
			if (theMode == 2)
				return GL_MIRRORED_REPEAT;
			if (theMode == 3 || theMode == 4)
				return GL_CLAMP_TO_EDGE;
			return GL_REPEAT;
		};
		gGL.glSamplerParameteri(mSamplers[s], GL_TEXTURE_WRAP_S, Address(v[GPUSAMP_ADDRESSU]));
		gGL.glSamplerParameteri(mSamplers[s], GL_TEXTURE_WRAP_T, Address(v[GPUSAMP_ADDRESSV]));
		gGL.glSamplerParameteri(mSamplers[s], GL_TEXTURE_MAG_FILTER, v[GPUSAMP_MAGFILTER] >= 2 ? GL_LINEAR : GL_NEAREST);
		// MIPFILTER is ignored: every texture has one level
		gGL.glSamplerParameteri(mSamplers[s], GL_TEXTURE_MIN_FILTER, v[GPUSAMP_MINFILTER] >= 2 ? GL_LINEAR : GL_NEAREST);
		gGL.glBindSampler(s, mSamplers[s]);
	}
	gGL.glActiveTexture(GL_TEXTURE0);
}

void GPUInterfaceGL::SetVertexAttributes(unsigned long theFVF, const unsigned char* theBase, int theStride)
{
	VertexLayout aLayout = LayoutOf(theFVF);
	int aStride = theStride > 0 ? theStride : aLayout.mStride;
	auto Attribute = [&](GLuint theIndex, int theOffset, GLint theSize, GLenum theType, bool theNormalized,
		float x, float y, float z, float w) {
		if (theOffset < 0) {
			gGL.glDisableVertexAttribArray(theIndex);
			gGL.glVertexAttrib4f(theIndex, x, y, z, w);
		} else {
			gGL.glEnableVertexAttribArray(theIndex);
			gGL.glVertexAttribPointer(theIndex, theSize, theType, theNormalized ? 1 : 0, aStride, theBase + theOffset);
		}
	};
	Attribute(ATTR_POSITION, aLayout.mPosition, aLayout.mRHW ? 4 : 3, GL_FLOAT, false, 0, 0, 0, 1);
	Attribute(ATTR_NORMAL, aLayout.mNormal, 3, GL_FLOAT, false, 0, 0, 0, 0);
	// D3DCOLOR is B, G, R, A in memory; the shaders swizzle it
	Attribute(ATTR_DIFFUSE, aLayout.mDiffuse, 4, GL_UNSIGNED_BYTE, true, 1, 1, 1, 1);
	Attribute(ATTR_SPECULAR, aLayout.mSpecular, 4, GL_UNSIGNED_BYTE, true, 0, 0, 0, 0);
	for (int i = 0; i < kStages; i++)
		Attribute(ATTR_TEX0 + i, aLayout.mTex[i], aLayout.mTexSize[i], GL_FLOAT, false, 0, 0, 0, 1);
}

bool GPUInterfaceGL::PrepareDraw(unsigned long theFVF, Program** theProgram)
{
	if (mLost)
		return false;
	VSKey aVS = MakeVSKey(theFVF);
	Program* aProgram = GetProgram(aVS, MakePSKey());
	if (aProgram == nullptr)
		return false;
	gGL.glUseProgram(aProgram->mProgram);
	ApplyState();
	ApplyUniforms(aProgram, aVS);
	BindTextures();
	gGL.glBindVertexArray(mVertexArray);
	*theProgram = aProgram;
	return true;
}

void GPUInterfaceGL::DrawPrimitiveUP(GPUPrimitiveType theType, int thePrimitiveCount, unsigned long theFVF,
	const void* theVertices, int theStride)
{
	Program* aProgram;
	if (thePrimitiveCount <= 0 || !PrepareDraw(theFVF, &aProgram))
		return;
	int aStride = theStride > 0 ? theStride : LayoutOf(theFVF).mStride;
	int aCount = VertexCount(theType, thePrimitiveCount);
	// Orphaned and filled again for every draw: WebGL2 has no client-side arrays
	gGL.glBindBuffer(GL_ARRAY_BUFFER, mStreamBuffer);
	gGL.glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)aStride * aCount, theVertices, GL_STREAM_DRAW);
	SetVertexAttributes(theFVF, nullptr, aStride);
	gGL.glDrawArrays(PrimitiveMode(theType), 0, aCount);
}

GPUBuffer GPUInterfaceGL::CreateVertexBuffer(unsigned long theFVF, const void* theVertices, int theCount, int theStride)
{
	BufferData& aData = mBuffers[mNextBuffer];
	int aStride = theStride > 0 ? theStride : LayoutOf(theFVF).mStride;
	aData.mFVF = theFVF;
	aData.mData.assign((const unsigned char*)theVertices, (const unsigned char*)theVertices + (size_t)aStride * theCount);
	gGL.glGenBuffers(1, &aData.mBuffer);
	gGL.glBindBuffer(GL_ARRAY_BUFFER, aData.mBuffer);
	gGL.glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)aData.mData.size(), aData.mData.data(), GL_STATIC_DRAW);
	return mNextBuffer++;
}

GPUBuffer GPUInterfaceGL::CreateIndexBuffer(const unsigned short* theIndices, int theCount)
{
	BufferData& aData = mBuffers[mNextBuffer];
	aData.mIndex = true;
	aData.mData.assign((const unsigned char*)theIndices, (const unsigned char*)(theIndices + theCount));
	gGL.glBindVertexArray(mVertexArray);
	gGL.glGenBuffers(1, &aData.mBuffer);
	gGL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, aData.mBuffer);
	gGL.glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)aData.mData.size(), aData.mData.data(), GL_STATIC_DRAW);
	return mNextBuffer++;
}

void GPUInterfaceGL::DeleteBuffer(GPUBuffer theBuffer)
{
	auto anIt = mBuffers.find(theBuffer);
	if (anIt == mBuffers.end())
		return;
	if (!mLost)
		gGL.glDeleteBuffers(1, &anIt->second.mBuffer);
	mBuffers.erase(anIt);
}

void GPUInterfaceGL::DrawIndexedPrimitive(GPUPrimitiveType theType, GPUBuffer theVertices, GPUBuffer theIndices,
	int theStartIndex, int thePrimitiveCount)
{
	auto aVertices = mBuffers.find(theVertices);
	auto anIndices = mBuffers.find(theIndices);
	if (aVertices == mBuffers.end() || anIndices == mBuffers.end() || thePrimitiveCount <= 0)
		return;
	Program* aProgram;
	if (!PrepareDraw(aVertices->second.mFVF, &aProgram))
		return;
	gGL.glBindBuffer(GL_ARRAY_BUFFER, aVertices->second.mBuffer);
	SetVertexAttributes(aVertices->second.mFVF, nullptr, 0);
	gGL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anIndices->second.mBuffer);
	gGL.glDrawElements(PrimitiveMode(theType), VertexCount(theType, thePrimitiveCount), GL_UNSIGNED_SHORT,
		(const void*)(std::size_t)(theStartIndex * 2));
}

}  // namespace

GPUInterface* GPUInterface::Create(GLContext* theContext, const GPUPolicy& thePolicy, int theWidth, int theHeight)
{
	if (theContext == nullptr)
		return nullptr;
	GPUInterfaceGL* anInterface = new GPUInterfaceGL(theContext, thePolicy, theWidth, theHeight);
	if (!anInterface->Init()) {
		SDL_Log("GPU: the device objects could not be made");
		delete anInterface;
		return nullptr;
	}
	return anInterface;
}
