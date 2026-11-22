// The gem silhouettes of the Lightning time gems' glow and bolts: the exe's table of 20 frames (the gem's turn) by 7 colours by 256 angles
// (0x83a548), each entry the distance from the gem's middle to its edge, and the function that reads it (0x71fbd0, bejeweled3).
// The table is game data: the port does not hold it. It is read from the player's own bejeweled3.exe (in the game folder) once, at
// start-up, by a small walker of the exe's section table; when that fails, one line goes to the log and every radius is a constant.
#pragma once

namespace Sexy {

namespace GemSilhouettes {

// Reads the table from the exe in the game folder (the virtual file system's bejeweled3.exe). Run once, after the folder is set.
void Load();

// 0x71fbd0 (bejeweled3): the distance, in 1200ths of the art height as the gem's art is drawn, from a gem's middle to its edge at an
// angle in radians (any, it is wrapped into 0 to 2 pi), for the given frame (0 to 19: the gem's turn) and colour (0 to 6)
float RadiusAt(float theAngle, int theFrame, int theColor);

}  // namespace GemSilhouettes

}  // namespace Sexy
