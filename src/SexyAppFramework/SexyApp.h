#ifndef __SEXYAPP_H__
#define __SEXYAPP_H__

// Portable SexyApp: PopCap's registration / update-check / ad layer between SexyAppBase and the game.
// The port has no online services, so the game always runs as a registered, up-to-date copy (which is
// how the Steam build behaves: partner.xml sets NoReg and DontUpdate). Members and virtual hooks the
// game reads or overrides are kept.

#include "SexyAppBase.h"

namespace Sexy
{

class SexyApp : public SexyAppBase
{
public:
	void* mInternetManager;  // not ported (update checks / ads)
	void* mBetaSupport;      // not ported
	std::string mReferId;
	std::string mVariation;
	ulong mDownloadId;
	std::string mRegSource;
	ulong mLastVerCheckQueryTime;
	bool mSkipAd;
	bool mDontUpdate;
	int mBuildNum;
	std::string mBuildDate;
	std::string mUserName;
	std::string mRegUserName;
	std::string mRegCode;
	bool mIsRegistered;
	bool mBuildUnlocked;
	int mTimesPlayed;
	int mTimesExecuted;
	bool mTimedOut;

public:
	virtual void UpdateFrames();
	virtual void WriteToRegistry();
	virtual void ReadFromRegistry();
	virtual bool CheckSignature(const Buffer& theBuffer, const std::string& theFileName);
	virtual bool ShouldCheckForUpdate();
	virtual void UpdateCheckQueried();
	virtual void URLOpenSucceeded(const std::string& theURL);

public:
	SexyApp();
	virtual ~SexyApp();

	bool Validate(const std::string& theUserName, const std::string& theRegCode);
	virtual bool OpenRegisterPage(DefinesMap theDefinesMap);
	virtual bool OpenRegisterPage();
	virtual void PreDisplayHook();
	virtual void InitPropertiesHook();
	virtual void Init();
	virtual void PreTerminate();
	virtual bool OpenHTMLTemplate(const std::string& theTemplateFile, const DefinesMap& theDefinesMap);
	virtual void OpenUpdateURL();
	virtual void HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue);
	virtual std::string GetGameSEHInfo();
};

extern SexyApp* gSexyApp;

}

#endif
