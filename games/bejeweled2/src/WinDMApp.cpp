#include "WinDMApp.h"

using namespace Sexy;

WinDMApp::WinDMApp()
{
	mProdName = "Bejeweled2";
	mRegKey = "PopCap\Bejeweled2";
}

WinDMApp::~WinDMApp()
{
}

void WinDMApp::Init()
{
	SexyApp::Init();
}
