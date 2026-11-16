// ThunderballApp.cpp: Peggle's application class.

#include "ThunderballApp.h"

#include "Res.h"
#include "ResourceManager.h"

using namespace Sexy;

ThunderballApp* Sexy::gThunderballApp = nullptr;

// 0x426f10
ThunderballApp::ThunderballApp()
{
	gThunderballApp = this;
	mProdName = "Peggle";
	mTitle = "Peggle Deluxe";
	mRegKey = "PopCap\\Peggle";
	mWidth = 800;
	mHeight = 600;
}

ThunderballApp::~ThunderballApp()
{
}

// 0x42ca30
void ThunderballApp::Init()
{
	SexyApp::Init();
	LoadResourceManifest();
	mResourceManager->LoadResources("Init");
	if (!ExtractInitResources(mResourceManager))
		ShowResourceError(true);
	mResourceManager->LoadResources("Load");
	if (!ExtractLoadResources(mResourceManager))
		ShowResourceError(true);
}
