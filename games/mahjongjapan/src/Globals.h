#ifndef __MAHJONGJAPAN_GLOBALS_H__
#define __MAHJONGJAPAN_GLOBALS_H__

// The game's globals, by their addresses in the exe. Their meaning is noted where it is known.

namespace mahjongjapan
{

// The deal (0x4136a0). Tile kinds are 1-42: 34 suit and honour kinds (1-0x22), four seasons (0x23-0x26) and four
// flowers (0x27-0x2a), the seasons matching one another and so do the flowers. gKindCounts counts the tiles of each
// kind 1-34 at [kind-1], all seasons at [34] and all flowers at [35] ([36-41] are the layout's own bonus counts when it
// has them, 0x415070); gBonusCounts splits [34] and [35] over the four seasons and the four flowers.
extern int gKindCounts[42];     // 0x53365c
extern int gBonusCounts[8];     // 0x53363c
extern int gQuadKinds;          // 0x533634 the kinds dealt four at a time
extern int gPairKinds;          // 0x533638 the kinds dealt two at a time

extern int gLayoutCount;        // 0x532e28 the layouts of the list (0xa2)
extern int gBgCycle;            // 0x5250bc the background shown in turn (-1 before the first)
extern int gMusicTurn;          // 0x532e2c the list's music, in turn (0x150 at 1, else 0x151)
extern int gFlyIn;              // 0x5250c0 where the next game's tiles fly in from (1-6 in turn)

// 0x51c90c: the backgrounds of the list's games (21) and the escape's (from [9])
extern const int gBackgrounds[21];

} // namespace mahjongjapan

#endif
