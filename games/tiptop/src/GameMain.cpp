// The game's main (0x411fff). For now: the menu's background on the screen, to check the engine and the archive.

#include "Game.h"
#include "Lowos.h"
#include "Res.h"

namespace tiptop
{

static lowos::Surface* gBackground = nullptr;
static bool gQuit = false;

static void OnActivate(char isActive)
{
	(void)isActive;
}

static void OnClose()
{
	gQuit = true;
}

static void OnSwitch(char theMode)
{
	lowos::ScreenScope(theMode);
}

bool GameInit()
{
	res::SetTable(res::GameTable());
	res::SetDataFile(0, "TipTop.dat");
	if (lowos::InitScreen(1, OnActivate, OnClose, OnSwitch, nullptr, "TipTop Deluxe", 2, 3) == 0)
		return false;
	lowos::SetColorKey(0xfefe01);
	if (res::Load(res::GROUP_IMAGES, 1, 2) != 0)
	{
		lowos::MessageBox_("Could not load the menu background.", "Error");
		return false;
	}
	lowos::Image* anImage = (lowos::Image*)res::Get(res::GROUP_IMAGES, 2);
	gBackground = lowos::RegisterSurface(anImage->mBits, anImage->mPitch, anImage->mHeight, lowos::SURF_SCREENFORMAT);
	lowos::InitKeyboard();
	lowos::InitMouse(0);
	return true;
}

bool GameFrame()
{
	lowos::BlitRegistered(640, 480, 0, 0, 0, 0, gBackground);
	char aMode = 0;
	lowos::EndFrame(&aMode);
	return !gQuit;
}

void GameShutdown()
{
	if (gBackground != nullptr)
		lowos::UnregisterSurface(gBackground);
	gBackground = nullptr;
	res::Free(res::GROUP_IMAGES, 1, 2);
}

} // namespace tiptop
