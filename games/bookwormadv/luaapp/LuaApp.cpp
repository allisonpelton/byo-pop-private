#include "LuaApp.h"

#include <cstring>

#include "BuiltinFont.h"
#include "LuaImage.h"
#include "LuaWidgets.h"
#include "PopLua.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "WidgetManager.h"

using namespace Sexy;
using namespace LuaLib;

LuaApp* Sexy::gLuaApp = nullptr;

// 0x65ed00
LuaApp::LuaApp()
{
	gLuaApp = this;
}

// 0x65edc0. The original also stopped its debugger thread (+0x764, started for -wait) and logged the time.
LuaApp::~LuaApp()
{
	gLuaApp = nullptr;
	while (!mDialogMap.empty())
		KillDialog(mDialogMap.begin()->second);
	DeleteLinkedObject();
	delete mLua;
	mLua = nullptr;
	delete mListFont;
}

void LuaApp::RegisterClasses()
{
	if (mLua == nullptr) {
		mLua = new PopLua();
		mLua->OpenPopLua();
	}
	LuaImage::RegisterLua();
	LuaFont::RegisterLua();
	RegisterLuaWidgets();

	Class<LuaApp>("LuaApp", false)
		.Method<static_cast<ulong (SexyAppBase::*)(int, int, int)>(&SexyAppBase::HSLToRGB)>("HSLToRGB")
		.Method<static_cast<ulong (SexyAppBase::*)(int, int, int)>(&SexyAppBase::RGBToHSL)>("RGBToHSL")
		.Method<&SexyAppBase::UpdateApp>("UpdateApp")
		.Method<&SexyAppBase::RegistryEraseKey>("RegistryEraseKey")
		.Method<&SexyAppBase::RegistryEraseValue>("RegistryEraseValue")
		.Method<&SexyAppBase::SetCursor>("SetCursor")
		.Method<&LuaApp::Is3DMode>("Is3DAccelerated")
		.Method<&SexyAppBase::CleanSharedImages>("CleanSharedImages")
		.Raw<&LuaApp::SafeDeleteWidget_>("SafeDeleteWidget")
		.Raw<&LuaApp::GetWidgetManager_>("GetWidgetManager")
		.Method<&LuaApp::LuaAddDialog>("AddDialog")
		.Method<&LuaApp::LuaGetDialog>("GetDialog")
		.Method<static_cast<bool (SexyAppBase::*)(int)>(&SexyAppBase::KillDialog)>("KillDialog")
		.Method<&LuaApp::LuaGetImage>("GetImage")
		.Method<&LuaApp::LuaTryGetImage>("TryGetImage")
		.Method<&LuaApp::LuaGetFont>("GetFont")
		.Method<&LuaApp::LuaTryGetFont>("TryGetFont")
		.Method<&LuaApp::LuaColorizeImage>("ColorizeImage")
		.Method<&LuaApp::LuaMirrorImage>("MirrorImage")
		.Method<&LuaApp::LuaFlipImage>("FlipImage")
		.Method<&LuaApp::LuaRotateImageHue>("RotateImageHue")
		.Method<&LuaApp::LuaUpdateAppStep>("UpdateAppStep")
		.Method<&LuaApp::LuaRegistryRead>("RegistryRead")
		.Method<&LuaApp::LuaRegistryWrite>("RegistryWrite")
		.Method<&LuaApp::LuaRegistryGetSubKeys>("RegistryGetSubKeys")
		.Method<&LuaApp::LuaDebugKeyDown>("DebugKeyDown")
		.Method<&LuaApp::LuaLoadSound>("LoadSound")
		.Method<&LuaApp::LuaPlaySample>("PlaySample")
		.Method<&LuaApp::LuaPlaySampleShifted>("PlaySampleShifted")
		.Method<&LuaApp::LuaReleaseSound>("ReleaseSound")
		.Method<&LuaApp::CreateLuaButtonWidget>("CreateLuaButtonWidget")
		.Method<&LuaApp::CreateLuaDialog>("CreateLuaDialog")
		.Method<&LuaApp::CreateLuaScrollbarWidget>("CreateLuaScrollbarWidget")
		.Method<&LuaApp::CreateLuaEditWidget>("CreateLuaEditWidget")
		.Method<&LuaApp::CreateLuaListWidget>("CreateLuaListWidget")
		.Method<&LuaApp::CreateLuaCheckboxWidget>("CreateLuaCheckboxWidget")
		.Method<&LuaApp::CreateLuaWidget>("CreateLuaWidget")
		.Method<&LuaApp::CreateLuaHyperlinkWidget>("CreateLuaHyperlinkWidget")
		.Method<&LuaApp::CreateLuaTextWidget>("CreateLuaTextWidget")
		.Method<&LuaApp::LuaCreateSavePoint>("CreateSavePoint")
		.Method<&LuaApp::LuaDeferReload>("DeferReload")
		.Method<&LuaApp::LuaPopSavePoint>("PopSavePoint")
		.Method<&LuaApp::GetHasFocus>("GetHasFocus")
		.Method<&LuaApp::SetHasFocus>("SetHasFocus")
		.Method<&LuaApp::GetWidth>("GetWidth")
		.Method<&LuaApp::SetWidth>("SetWidth")
		.Method<&LuaApp::GetHeight>("GetHeight")
		.Method<&LuaApp::SetHeight>("SetHeight");
}

// 0x660b40
void LuaApp::Init()
{
	RegisterClasses();
	SexyApp::Init();
}

void LuaApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
	RegistryReadBoolean("Is3D", &m3DMode);
}

void LuaApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
	if (mUserChanged3DSetting)
		RegistryWriteBoolean("Is3D", m3DMode);
}

// 0x661620
void LuaApp::Shutdown()
{
	if (!mShutdown && mLua != nullptr)
		mLua->AppShutdown();
	SexyApp::Shutdown();
}

// 0x660b60
void LuaApp::UpdateFrames()
{
	SexyApp::UpdateFrames();
	if (mLua != nullptr)
		mLua->UpdateState();
}

// 0x65f750
void LuaApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-wait")  // the original waited here for a debugger to attach
		return;
	SexyApp::HandleCmdLineParam(theParamName, theParamValue);
}

bool LuaApp::DebugKeyDown(int theKey)
{
	lua_pushint(mLua->L, theKey);
	return CallLinkedMethod("DebugKeyDown", 1).Bool();
}

// ---- widgets and dialogs ----

// 0x660020
int LuaApp::SafeDeleteWidget_(LuaApp* app, lua_State* L)
{
	Widget* w = (Widget*)gLuaSupport->CheckObject(1, ClassOf<Widget>::info);
	if (w == nullptr)
		return luaL_typerror(L, 1, "sexy.Widget");
	app->SafeDeleteWidget(w);
	return 0;
}

// 0x660150
int LuaApp::GetWidgetManager_(LuaApp* app, lua_State* L)
{
	gLuaSupport->PushObject(ClassOf<WidgetManager>::info, app->mWidgetManager);
	return 1;
}

// 0x660380
void LuaApp::LuaAddDialog(LuaValue theDialog)
{
	LuaDialog* d = (LuaDialog*)gLuaSupport->CheckObject(theDialog.idx, ClassOf<LuaDialog>::info);
	if (d == nullptr) {
		luaL_typerror(mLua->L, theDialog.idx, "LuaDialog");
		return;
	}
	AddDialog(d);
}

// 0x660200
LuaMultiRet LuaApp::LuaGetDialog(int theId)
{
	LuaDialog* d = dynamic_cast<LuaDialog*>(GetDialog(theId));
	if (d == nullptr)
		return {0};
	gLuaSupport->PushObject(ClassOf<LuaDialog>::info, d);
	return {1};
}

// 0x65fa90
LuaButtonWidget* LuaApp::CreateLuaButtonWidget(int theId, LuaValue theListener)
{
	return new LuaButtonWidget(theId, LuaCheckListener(mLua->L, theListener.idx));
}

// 0x65f860
LuaDialog* LuaApp::CreateLuaDialog(LuaImage* theComponentImage, LuaImage* theButtonImage, int theId, bool isModal,
                                   const char* theHeader, const char* theLines, const char* theFooter,
                                   int theButtonMode)
{
	return new LuaDialog(ImageOf(theComponentImage), ImageOf(theButtonImage), theId, isModal, theHeader, theLines,
	                     theFooter, theButtonMode);
}

// 0x65fdd0
LuaScrollbarWidget* LuaApp::CreateLuaScrollbarWidget(int theId, LuaValue theListener)
{
	return new LuaScrollbarWidget(theId, LuaCheckListener(mLua->L, theListener.idx));
}

// 0x65fc00
LuaEditWidget* LuaApp::CreateLuaEditWidget(int theId, LuaValue theListener)
{
	return new LuaEditWidget(theId, LuaCheckListener(mLua->L, theListener.idx));
}

// Without a font the list gets a system font (the original's SysFont "SansSerif", 10, made again for each
// such list and never freed)
// 0x65fc90
LuaListWidget* LuaApp::CreateLuaListWidget(int theId, LuaFont* theFont, LuaValue theListener)
{
	Font* font = FontOf(theFont);
	if (font == nullptr) {
		if (mListFont == nullptr)
			mListFont = new BuiltinFont(false);
		font = mListFont;
	}
	return new LuaListWidget(theId, font, LuaCheckListener(mLua->L, theListener.idx));
}

// 0x65fe60
LuaCheckboxWidget* LuaApp::CreateLuaCheckboxWidget(LuaImage* theUnchecked, LuaImage* theChecked, int theId,
                                                   LuaValue theListener)
{
	return new LuaCheckboxWidget(ImageOf(theUnchecked), ImageOf(theChecked), theId,
	                             LuaCheckListener(mLua->L, theListener.idx));
}

// 0x65f7f0
LuaWidget* LuaApp::CreateLuaWidget()
{
	return new LuaWidget();
}

// 0x65fb10
LuaHyperlinkWidget* LuaApp::CreateLuaHyperlinkWidget(int theId, LuaValue theListener)
{
	return new LuaHyperlinkWidget(theId, LuaCheckListener(mLua->L, theListener.idx));
}

// 0x65fb90
LuaTextWidget* LuaApp::CreateLuaTextWidget()
{
	return new LuaTextWidget();
}

// ---- images and fonts ----

// 0x6608b0
LuaMultiRet LuaApp::LuaGetImage(const char* thePath)
{
	LuaMultiRet r = LuaTryGetImage(thePath);
	if (r.n == 0)
		luaL_error(mLua->L, "Error loading image: %s", thePath);
	return r;
}

// TryGetImage(path[, variant]) (0x660610, loading with 0x667db0)
LuaMultiRet LuaApp::LuaTryGetImage(const char* thePath)
{
	lua_State* L = mLua->L;
	std::string aVariant;
	if (lua_type(L, 2) == LUA_TSTRING)
		aVariant = lua_tostring(L, 2);
	bool isNew = false;
	SharedImageRef aRef = GetSharedImage(thePath, aVariant, &isNew);
	if ((MemoryImage*)aRef == nullptr)
		return {0};
	LuaImage* anImage = new LuaImage(aRef);
	anImage->mPath = thePath;
	anImage->mVariant = aVariant;
	gLuaSupport->PushObject(ClassOf<LuaImage>::info, anImage, true);
	lua_pushboolean(L, isNew);
	return {2};
}

// 0x6608f0
LuaMultiRet LuaApp::LuaGetFont(const char* theName)
{
	LuaFont* aFont = LuaFont::Load(theName);
	if (aFont == nullptr)
		luaL_error(mLua->L, "Error loading font: %s", theName);
	gLuaSupport->PushObject(ClassOf<LuaFont>::info, aFont, true);
	return {1};
}

// 0x660a20
LuaMultiRet LuaApp::LuaTryGetFont(const char* theName)
{
	LuaFont* aFont = LuaFont::Load(theName);
	if (aFont == nullptr)
		lua_pushnil(mLua->L);
	else
		gLuaSupport->PushObject(ClassOf<LuaFont>::info, aFont, true);
	return {1};
}

// 0x6604e0
void LuaApp::LuaColorizeImage(LuaImage* theImage, int theColor)
{
	ColorizeImage(ImageOf(theImage), Color(theColor));
}

// 0x660520
void LuaApp::LuaMirrorImage(LuaImage* theImage)
{
	MirrorImage(ImageOf(theImage));
}

// 0x6605c0
void LuaApp::LuaFlipImage(LuaImage* theImage)
{
	FlipImage(ImageOf(theImage));
}

// 0x660580
void LuaApp::LuaRotateImageHue(LuaImage* theImage, int theDelta)
{
	RotateImageHue(theImage->Image(), theDelta);
}

// ---- the app ----

// 0x660310
LuaMultiRet LuaApp::LuaUpdateAppStep()
{
	bool updated = false;
	bool cont = UpdateAppStep(&updated);
	lua_pushboolean(mLua->L, cont);
	lua_pushboolean(mLua->L, updated);
	return {2};
}

// A DWORD is an int, a string a string, 8 bytes of data a number and 1 byte a boolean (0x660db0)
LuaMultiRet LuaApp::LuaRegistryRead(const char* theName, LuaValue theDefault)
{
	lua_State* L = mLua->L;
	int anInt;
	std::string aString;
	if (RegistryReadInteger(theName, &anInt)) {
		lua_pushint(L, anInt);
		return {1};
	}
	if (RegistryReadString(theName, &aString)) {
		lua_pushlstring(L, aString.data(), aString.size());
		return {1};
	}
	uchar aData[8];
	ulong aLength = sizeof(aData);
	if (RegistryReadData(theName, aData, &aLength)) {
		if (aLength == 8) {
			double d;
			memcpy(&d, aData, 8);
			lua_pushnumber(L, d);
			return {1};
		}
		if (aLength == 1) {
			lua_pushboolean(L, aData[0] != 0);
			return {1};
		}
	}
	lua_pushvalue(L, theDefault.idx);
	return {1};
}

// 0x661050
void LuaApp::LuaRegistryWrite(const char* theName, LuaValue theValue)
{
	lua_State* L = mLua->L;
	switch (lua_type(L, theValue.idx)) {
	case LUA_TINT:
		RegistryWriteInteger(theName, lua_toint(L, theValue.idx));
		break;
	case LUA_TSTRING:
		RegistryWriteString(theName, std::string(lua_tostring(L, theValue.idx), lua_strlen(L, theValue.idx)));
		break;
	case LUA_TNUMBER: {
		double d = lua_tonumber(L, theValue.idx);
		RegistryWriteData(theName, (const uchar*)&d, sizeof(d));
		break;
	}
	case LUA_TBOOLEAN: {
		uchar b = lua_toboolean(L, theValue.idx) ? 1 : 0;
		RegistryWriteData(theName, &b, 1);
		break;
	}
	default:
		break;
	}
}

// 0x6611e0
LuaMultiRet LuaApp::LuaRegistryGetSubKeys(const char* theKey)
{
	lua_State* L = mLua->L;
	std::vector<std::string> aKeys;
	if (!RegistryGetSubKeys(theKey, &aKeys)) {
		lua_pushnil(L);
		return {1};
	}
	lua_newtable(L);
	for (size_t i = 0; i < aKeys.size(); i++) {
		lua_pushint(L, (int)i);
		lua_pushlstring(L, aKeys[i].data(), aKeys[i].size());
		lua_settable(L, -3);
	}
	return {1};
}

// ---- sounds ----

// 0x661470
int LuaApp::LuaLoadSound(const char* thePath)
{
	return mSoundManager->LoadSound(thePath);
}

// 0x661520
void LuaApp::LuaPlaySample(int theId)
{
	PlaySample(theId, lua_toint(mLua->L, 2));
}

// 0x661580
void LuaApp::LuaPlaySampleShifted(int theId, double theSteps)
{
	if (mSoundManager == nullptr)
		return;
	SoundInstance* anInst = mSoundManager->GetSoundInstance(theId);
	if (anInst == nullptr)
		return;
	anInst->SetPan(lua_toint(mLua->L, 3));
	anInst->AdjustPitch(theSteps);
	anInst->Play(false, true);
}

// 0x661610
void LuaApp::LuaReleaseSound(int theId)
{
	mSoundManager->ReleaseSound(theId);
}

// ---- the development features ----

// 0x660b80
void LuaApp::LuaCreateSavePoint(const char* theName)
{
	mLua->CreateSavePoint(theName);
}

// 0x660c20
void LuaApp::LuaDeferReload()
{
	lua_State* L = mLua->L;
	mLua->DeferReload(lua_type(L, 1) == LUA_TSTRING ? lua_tostring(L, 1) : "");
}

// 0x660d10
void LuaApp::LuaPopSavePoint(const char* theName)
{
	mLua->PopSavePoint(theName);
}
