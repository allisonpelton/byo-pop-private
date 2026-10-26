#ifndef __TIPTOP_GAME_H__
#define __TIPTOP_GAME_H__

// The original's WinMain (0x42d020) and game main (0x411fff), cut at its main loop so that the host can run one
// pass of the loop per update.

namespace tiptop
{

// 0x411fff up to its main loop: the configuration, the registry, the screen and the first screen of the game.
// False if the game could not start (it has shown why).
bool GameInit();
// One pass of the main loop: input events, the frame (update and draw) and LOWOS_EndFrame. False once the game
// has quit.
bool GameFrame();
// The end of 0x411fff after its loop
void GameShutdown();

} // namespace tiptop

#endif
