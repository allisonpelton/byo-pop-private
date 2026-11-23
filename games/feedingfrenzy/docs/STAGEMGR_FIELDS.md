# StageMgr and FishBase field map (working notes, delete once the classes are written)

Rule for reading the export: in a function whose first parameter is `int *param_1`, `param_1[0xNN]` and `param_1->mNN_b`
both mean the element NN, that is byte offset 4*NN (so `param_1->m4e_b` is 0x138). In a function whose parameter is
`int param_1` or `void *this`, `mNN` is the byte offset NN.

## StageMgr (constructor 0x43bcc0, vtable 0x4a7060, 131 slots; Container, KeyListener at 0x11c)

0x124 PlayGroup  0x128 player  0x12c "behind" group  0x130 "inFrontOfPlayer" group  0x134 fishGroupFg  0x138 fishGroupMg
0x13c fishGroupBg  0x140 midGroundGroup  0x144 hudOffsetGroup  0x148 overlay group (empty, added last)
0x14c load step (0..9)  0x150 load flag  0x154 state (0 start screen, 1 loading, 2 paused, 3 playing, 4 ?, 5 ending, 6 ?)
0x158 state before the pause (3)  0x15c stage-clear bubbles (particle system, 0x130 bytes)
0x160 end kind (1 win, 2 lose)  0x164 end handled  0x168 end timer (4.0)  0x170 fade mode (1 in, 2 out)  0x174 fade seconds
(0.2)  0x178 fade left  0x17c hud (gameHud.xml, 0x60 bytes)  0x180 BgLayout  0x184 interpolator (0x24 bytes)  0x188 MgLayout
0x18c FgLayout  0x190 fish list (vector at 0x190: begin 0x194, end 0x198)  0x1a4/0x1a8 schools list
0x1b0..0x1bc four screens (stage start, stage end, continue, game over)  0x1c0 screen shown  0x1c4 stage type (5 = menu)
0x1c8 flag  0x1cc,0x1d0 world size (640,480)  0x1d4,0x1d8 scroll margins (200,150)  0x1dc,0x1e0 camera (float)
0x1e4,0x1e8 view size (640,480)  0x2a0/0x2a4 spawner list  0x2ac the stage's FishSpawner for bonuses  0x298 music
0x2b0 stage time left  0x2b8 count  0x2bc time played  0x2c4 eat splat factory?  0x2c8 splat factory
0x2d0 stage number  0x2cc flag  0x328 .. 0x33c shield/spawn ring images  0x344 spawn timer  0x348 camera follow offset
0x350 start delay  0x354 cursor?  0x358 timer

## FishBase (constructor 0x408520, vtable 0x49f710, 146 slots, 0x2e8 bytes) and AIFish (0x402190, 0x388 bytes)

see FishBase.h
