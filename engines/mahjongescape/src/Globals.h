#ifndef __MAHJONG_GLOBALS_H__
#define __MAHJONG_GLOBALS_H__

// The game's globals, by their addresses in the exe. Their meaning is noted where it is known.

namespace mahjong
{

// The deal (0x413120 (mahjongchina), 0x4136a0 (mahjongjapan)). Tile kinds are 1-42: 34 suit and honour kinds (1-0x22),
// four seasons (0x23-0x26) and four flowers (0x27-0x2a), the seasons matching one another and so do the flowers.
// gKindCounts counts the tiles of each kind 1-34 at [kind-1], all seasons at [34] and all flowers at [35] ([36-41] are
// the layout's own bonus counts when it has them, 0x414af0 (mahjongchina), 0x415070 (mahjongjapan)); gBonusCounts
// splits [34] and [35] over the four seasons and the four flowers.
extern int gKindCounts[42];     // 0x5329b4 (mahjongchina), 0x53365c (mahjongjapan)
extern int gBonusCounts[8];     // 0x532994 (mahjongchina), 0x53363c (mahjongjapan)
extern int gQuadKinds;          // 0x53298c (mahjongchina), 0x533634 (mahjongjapan) the kinds dealt four at a time
extern int gPairKinds;          // 0x532990 (mahjongchina), 0x533638 (mahjongjapan) the kinds dealt two at a time

extern int gLayoutCount;        // 0x532048 (mahjongchina), 0x532e28 (mahjongjapan) the layouts of the list (0xa2)
// 0x5240bc (mahjongchina), 0x5250bc (mahjongjapan) the background shown in turn (-1 before the first)
extern int gBgCycle;
// 0x53204c (mahjongchina), 0x532e2c (mahjongjapan) the list's music, in turn (0x150 at 1, else 0x151)
extern int gMusicTurn;
// 0x5240c0 (mahjongchina), 0x5250c0 (mahjongjapan) where the next game's tiles fly in from (1-6 in turn)
extern int gFlyIn;


} // namespace mahjong

#endif
