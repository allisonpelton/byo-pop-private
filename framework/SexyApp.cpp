#include "SexyApp.h"

using namespace Sexy;

SexyApp* Sexy::gSexyApp = NULL;

SexyApp::SexyApp()
{
	gSexyApp = this;
	mInternetManager = NULL;
	mBetaSupport = NULL;
	mDownloadId = 0;
	mLastVerCheckQueryTime = 0;
	mSkipAd = true;
	mDontUpdate = true;
	mBuildNum = 0;
	mIsRegistered = true;
	mBuildUnlocked = true;
	mTimesPlayed = 0;
	mTimesExecuted = 0;
	mTimedOut = false;
}

SexyApp::~SexyApp()
{
}

bool SexyApp::Validate(const std::string&, const std::string&)
{
	return true;
}

void SexyApp::UpdateFrames()
{
	SexyAppBase::UpdateFrames();
}

void SexyApp::WriteToRegistry()
{
	SexyAppBase::WriteToRegistry();
	RegistryWriteInteger("TimesPlayed", mTimesPlayed);
	RegistryWriteInteger("TimesExecuted", mTimesExecuted);
}

void SexyApp::ReadFromRegistry()
{
	SexyAppBase::ReadFromRegistry();
	RegistryReadInteger("TimesPlayed", &mTimesPlayed);
	RegistryReadInteger("TimesExecuted", &mTimesExecuted);
	// port: always registered (Steam build / no online registration)
	mIsRegistered = true;
}

bool SexyApp::CheckSignature(const Buffer&, const std::string&)
{
	return true;
}

bool SexyApp::ShouldCheckForUpdate()
{
	return false;
}

void SexyApp::UpdateCheckQueried()
{
}

void SexyApp::URLOpenSucceeded(const std::string& theURL)
{
	SexyAppBase::URLOpenSucceeded(theURL);
}

bool SexyApp::OpenRegisterPage(DefinesMap)
{
	return false;
}

bool SexyApp::OpenRegisterPage()
{
	return false;
}

void SexyApp::PreDisplayHook()
{
}

// The distributor's settings (0x48caf0). Steam's partner.xml sets NoReg, DefaultWindowed=false, the
// registry key (read by SexyAppBase::ReadFromRegistry) and the Options dialog's "more games" link.
// port: no signature check (CheckSignature accepts everything) and no BetaSupport.
void SexyApp::InitPropertiesHook()
{
	LoadProperties("properties\\partner.xml", false, false);

	if (GetBoolean("NoReg", false))
	{
		mIsRegistered = true;
		mBuildUnlocked = true;
	}

	mProdName = SexyStringToString(GetString("ProdName", StringToSexyString(mProdName)));
	mIsWindowed = GetBoolean("DefaultWindowed", mIsWindowed);

	SexyString aTitle = GetString("Title", _S(""));
	if (!aTitle.empty())
		mTitle = aTitle + _S(" ") + StringToSexyString(mProductVersion);
}

void SexyApp::Init()
{
	SexyAppBase::Init();
	mTimesExecuted++;
}

void SexyApp::PreTerminate()
{
}

bool SexyApp::OpenHTMLTemplate(const std::string&, const DefinesMap&)
{
	return false;
}

void SexyApp::OpenUpdateURL()
{
}

void SexyApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	SexyAppBase::HandleCmdLineParam(theParamName, theParamValue);
}

std::string SexyApp::GetGameSEHInfo()
{
	return SexyAppBase::GetGameSEHInfo();
}
