#ifndef __SEXYAPP_H__
#define __SEXYAPP_H__

// port(variant): headless shim of framework/SexyApp.h and SexyAppBase.h for the verifier
// (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9). Not part of the original game. The real WinDMApp.h derives
// WinDMApp from this SexyApp; games/bejeweled/src/headless/WinDMAppHeadless.cpp defines the WinDMApp members a
// board and the replay player call. Here: the members they read, sounds and dialogs as no-ops, the strings as
// their ids, files in memory (a playback board reads and writes none; a difftest board saves at its end),
// and SafeDeleteWidget deferring the delete to the end of the frame as the framework does.

#include "Common.h"
#include "Rect.h"
#include "Color.h"
#include "Buffer.h"
#include "ButtonListener.h"
#include "DialogListener.h"

#include <map>
#include <string>
#include <vector>

namespace Sexy
{

class WidgetManager;
class DDInterface;
class Image;
class DDImage;
class Widget;
class SoundManager;
class MusicInterface;
class Dialog;

typedef std::map<int, Dialog*> DialogMap;

// One line of a -script file (SexyAppBase.h): only the type, for WinDMApp::HandleScriptCommand
struct ScriptCommand
{
	int mUpdateCount;
	std::string mCommand;
	std::string mArgs;
	int mLine;
};

// A file the app writes at shutdown (SexyAppBase.h): only the type, for WinDMApp::GetShutdownSaves
struct ShutdownSave
{
	std::string mFileName;
	std::string mData;
};

// A game's message about a mod (SexyAppBase.h): only the type, for WinDMApp::GetModMessages
struct ModMessage
{
	std::string mModId;
	std::string mText;
	bool mIsError;
};

class SexyAppBase : public ButtonListener, public DialogListener
{
public:
	WidgetManager*		mWidgetManager;
	DDInterface*		mDDInterface;
	SoundManager*		mSoundManager;
	MusicInterface*		mMusicInterface;
	DialogMap			mDialogMap;
	std::string			mProdName;
	SexyString			mTitle;
	std::string			mRegKey;
	std::string			mProductVersion;
	int					mWidth;
	int					mHeight;
	bool				mHasFocus;
	bool				mShutdown;
	bool				mCustomCursorsEnabled;
	int					mUpdateCount;
	ulong				mRandSeed;
	std::vector<Widget*> mSafeDeleteList;
	std::map<std::string, std::string> mFiles;   // the "files" the app reads and writes, by name

	SexyAppBase();
	virtual ~SexyAppBase();

	virtual void		PlaySample(int theSoundNum) {}
	virtual void		PlaySample(int theSoundNum, int thePan) {}
	virtual void		SafeDeleteWidget(Widget* theWidget) { mSafeDeleteList.push_back(theWidget); }
	void				ProcessSafeDeleteList();   // the end of a frame: the widgets SafeDeleteWidget was given

	virtual bool		KillDialog(int theDialogId, bool removeWidget, bool deleteWidget) { return false; }
	virtual bool		KillDialog(int theDialogId) { return false; }
	virtual bool		KillDialog(Dialog* theDialog) { return false; }

	// The framework virtuals WinDMApp.h overrides (it marks them override), so that header compiles against this
	// shim. The verifier calls none of them.
	virtual void		Shutdown() {}
	virtual void		LoadingThreadCompleted() {}
	virtual void		URLOpenSucceeded(const std::string& theURL) {}
	virtual MusicInterface* CreateMusicInterface() { return NULL; }
	virtual Dialog*		NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) { return NULL; }
	virtual void		ModalOpen() {}
	virtual void		LostFocus() {}
	virtual void		ModalClose() {}
	virtual void		GotFocus() {}
	virtual void		UpdateFrames() {}
	virtual void		SetMusicVolume(double theVolume) {}
	virtual std::string	GetGameSEHInfo() { return ""; }
	virtual void		URLOpenFailed(const std::string& theURL) {}
	virtual bool		OpenURL(const std::string& theURL, bool shutdownOnOpen = false) { return false; }
	virtual void		WriteToRegistry() {}
	virtual void		ReadFromRegistry() {}
	virtual void		PreDisplayHook() {}
	virtual void		Init() {}
	virtual void		LoadingThreadProc() {}
	virtual void		GetShutdownSaves(std::vector<ShutdownSave>* theSaves) {}
	virtual void		WriteShutdownRegistry() {}
	virtual void		HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) {}
	virtual bool		HandleScriptCommand(const ScriptCommand& theCommand) { return false; }
	virtual bool		HasGameInProgress() { return false; }
	virtual bool		CanRestartNow(SexyString* theReason) { return true; }
	virtual void		GetRestartArgs(std::vector<std::string>* theArgs) {}
	virtual void		GetModMessages(std::vector<ModMessage>* theMessages) {}

	SexyString			GetString(const std::string& theId) { return StringToSexyStringFast(theId); }
	SexyString			GetString(const std::string& theId, const SexyString& theDefault) { return theDefault; }

	DDImage*			GetImage(const std::string& theFileName, bool commitBits = true) { return NULL; } // no files: a mod's image is never there
	DDImage*			CreateColorizedImage(Image* theImage, const Color& theColor);
	DDImage*			CreateCrossfadeImage(Image* theImage1, const Rect& theRect1, Image* theImage2, const Rect& theRect2, double theFadeFactor);
	ulong				HSLToRGB(int h, int s, int l) { return 0; }

	bool				WriteBufferToFile(const std::string& theFileName, const Buffer* theBuffer);
	bool				ReadBufferFromFile(const std::string& theFileName, Buffer* theBuffer, bool dontWriteToDemo = false);
	bool				WriteBytesToFile(const std::string& theFileName, const void* theData, unsigned long theDataLen);
	bool				FileExists(const std::string& theFileName) { return mFiles.find(theFileName) != mFiles.end(); }
	bool				EraseFile(const std::string& theFileName) { return mFiles.erase(theFileName) > 0; }
};

class SexyApp : public SexyAppBase
{
public:
	int					mTimesPlayed;
	int					mTimesExecuted;

	SexyApp() : mTimesPlayed(0), mTimesExecuted(0) {}
	virtual ~SexyApp() {}
};

}

#endif
