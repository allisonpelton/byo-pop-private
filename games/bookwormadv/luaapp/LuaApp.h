// Sexy::LuaApp (constructor 0x65ed00, vtable 0x7c1534): a SexyApp whose game is written in Lua. It owns the
// PopLua state (+0x758), made by RegisterClasses (0x65f070) before SexyApp's Init, and is itself a linked
// object: the scripts' LuaApp object (its LuaLinkedClass part is at +0x730). Its 48 Lua methods give the
// scripts images, fonts, sounds, dialogs, the registry and the widget factories. Nothing here is specific to
// one game; a game's app derives from it.
#pragma once

#include <string>

#include "LuaBind.h"
#include "LuaLinkedClass.h"
#include "SexyApp.h"

namespace Sexy {

class PopLua;
class Font;
class LuaImage;
class LuaFont;
class LuaWidget;
class LuaTextWidget;
class LuaButtonWidget;
class LuaHyperlinkWidget;
class LuaEditWidget;
class LuaListWidget;
class LuaScrollbarWidget;
class LuaCheckboxWidget;
class LuaDialog;

class LuaApp : public SexyApp, public LuaLib::LuaLinkedClass {
public:
	PopLua* mLua = nullptr;      // +0x758
	Font* mListFont = nullptr;  // +0x760, for lists made without a font

	LuaApp();
	~LuaApp() override;

	const char* LuaGetRegisteredName() override { return "LuaApp"; }

	// The PopLua state (when there is none yet) and the classes scripts use (slot 105). A game's app makes
	// its own state (a LuaInterface) before calling this.
	virtual void RegisterClasses();
	virtual void LuaAppHook() {}  // slot 104, empty

	void Init() override;          // RegisterClasses, then SexyApp's
	void Shutdown() override;      // the state's shutdown, then SexyAppBase's
	void UpdateFrames() override;  // SexyApp's, then the state's per-frame work
	void HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;
	// A debug key goes to the app's Lua object (luaapp.lua), whose DebugKeyDown calls SexyApp's through
	// LuaDebugKeyDown. The original did that in DebugKeyDownAsync (0x65f4e0, which first gave Z to its DProps
	// editor), which the port's framework does not have; its DebugKeyDown (0x65f7b0) passed all keys but its
	// debugger's E, C, Z and W to SexyApp's.
	bool DebugKeyDown(int theKey) override;
	void ReadFromRegistry() override;  // SexyApp's, then Is3D (the original read it as it made its window, 0x548b10)
	void WriteToRegistry() override;   // SexyApp's, then Is3D (the original wrote it as the app closed, 0x53c120)

	// The game's 3D mode, what the original's DirectDraw interface held at +0x38: the registry's Is3D, or on when there is
	// none (the original then ran its 3D test, which a machine of today passes), and the Options dialog's 3D box. The
	// port's renderer draws the same either way; the game and its scripts (gCApp:Is3DAccelerated()) choose by this what
	// they draw, as the arena's midground animation, drawn only in 3D. Off on a renderer that is not accelerated.
	bool Is3DMode() { return m3DMode && Is3DAccelerated(); }
	void Set3DMode(bool on) { m3DMode = on; }

	// ---- the scripts' methods ----
	static int SafeDeleteWidget_(LuaApp* app, lua_State* L);
	static int GetWidgetManager_(LuaApp* app, lua_State* L);
	void LuaAddDialog(LuaLib::LuaValue theDialog);
	LuaLib::LuaMultiRet LuaGetDialog(int theId);
	// image, and whether it was loaded now
	LuaLib::LuaMultiRet LuaGetImage(const char* thePath);
	LuaLib::LuaMultiRet LuaTryGetImage(const char* thePath);
	LuaLib::LuaMultiRet LuaGetFont(const char* theName);
	LuaLib::LuaMultiRet LuaTryGetFont(const char* theName);
	void LuaColorizeImage(LuaImage* theImage, int theColor);
	void LuaMirrorImage(LuaImage* theImage);
	void LuaFlipImage(LuaImage* theImage);
	void LuaRotateImageHue(LuaImage* theImage, int theDelta);
	// continue, updated
	LuaLib::LuaMultiRet LuaUpdateAppStep();
	// the value, or the default when there is none
	LuaLib::LuaMultiRet LuaRegistryRead(const char* theName, LuaLib::LuaValue theDefault);
	void LuaRegistryWrite(const char* theName, LuaLib::LuaValue theValue);
	// a table of the names, or nil
	LuaLib::LuaMultiRet LuaRegistryGetSubKeys(const char* theKey);
	bool LuaDebugKeyDown(int theKey) { return SexyApp::DebugKeyDown(theKey); }
	int LuaLoadSound(const char* thePath);
	// PlaySample(id[, pan])
	void LuaPlaySample(int theId);
	// PlaySampleShifted(id, steps[, pan])
	void LuaPlaySampleShifted(int theId, double theSteps);
	void LuaReleaseSound(int theId);
	LuaButtonWidget* CreateLuaButtonWidget(int theId, LuaLib::LuaValue theListener);
	LuaDialog* CreateLuaDialog(LuaImage* theComponentImage, LuaImage* theButtonImage, int theId, bool isModal,
	                           const char* theHeader, const char* theLines, const char* theFooter, int theButtonMode);
	LuaScrollbarWidget* CreateLuaScrollbarWidget(int theId, LuaLib::LuaValue theListener);
	LuaEditWidget* CreateLuaEditWidget(int theId, LuaLib::LuaValue theListener);
	LuaListWidget* CreateLuaListWidget(int theId, LuaFont* theFont, LuaLib::LuaValue theListener);
	LuaCheckboxWidget* CreateLuaCheckboxWidget(LuaImage* theUnchecked, LuaImage* theChecked, int theId,
	                                           LuaLib::LuaValue theListener);
	LuaWidget* CreateLuaWidget();
	LuaHyperlinkWidget* CreateLuaHyperlinkWidget(int theId, LuaLib::LuaValue theListener);
	LuaTextWidget* CreateLuaTextWidget();
	void LuaCreateSavePoint(const char* theName);
	// DeferReload([path])
	void LuaDeferReload();
	void LuaPopSavePoint(const char* theName);
	bool GetHasFocus() { return mHasFocus; }
	void SetHasFocus(bool on) { mHasFocus = on; }
	int GetWidth() { return mWidth; }
	void SetWidth(int n) { mWidth = n; }
	int GetHeight() { return mHeight; }
	void SetHeight(int n) { mHeight = n; }

private:
	bool m3DMode = true;
};

extern LuaApp* gLuaApp;  // 0x825ce0

}  // namespace Sexy
