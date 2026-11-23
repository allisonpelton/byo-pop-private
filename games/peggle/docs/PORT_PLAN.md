# Peggle: what is translated and what is left

Counted on 2026-11-22 from the export's function list (0x400000 to 0x4a0000, the game's own code) against the addresses the
sources name; a function counts as translated when a comment gives its address, so the numbers are close, not exact.

| Class | Functions | Translated | Bytes of code left |
| --- | ---: | ---: | ---: |
| LogicMgr (the rules: shots, turns, scoring, power-ups) | 99 | 19 | 49,000 |
| Effect (the effects' own classes) | 47 | 0 | 28,000 |
| Board | 83 | 22 | 21,000 |
| StoryScreen, UpsellScreen, AdventureScreen | 35 | 0 | 37,000 |
| InterfaceMgr | 41 | 11 | 11,000 |
| EffectMgr | 16 | 4 | 7,500 |
| AIMgr, AIBall (the bot) | 17 | 3 | 10,700 |
| CharacterMgr, Gun, PhysObj, Brick, Ball, Poly, PegInfo | | | about 25,000 |

About 228,000 of 403,000 bytes are left in that range. The order that keeps every step testable by state:

1. LogicMgr's turn (`Update`, `UpdateGun`, `MouseDown`/`MouseUp`/`MouseMove`, `BeginTurn`, the shot states) with the ball's
   physics (`Ball::Update`, `CollisionMgr`, `Poly`/`Line` collisions): the logic phase's centre. A `shoot <angle>` test command
   then plays a shot and prints the board's state each update; the original's own states (origrun) are the check.
2. The scoring and the power-ups (`LogicMgr` hit and bonus functions, `PegInfo`), then the level's end (`DoLevelDone`,
   `EndLevelDialog`, the high scores).
3. The managers' updates (`EffectMgr`/`Effect`, `SoundMgr`, `FloatingTextMgr`, `InterfaceMgr`, `CharacterMgr`) as the shot
   states reach them: each is declared and stops at `NOT_TRANSLATED` until then.
4. The screens (story, upsell, adventure) and the drawing (`Board::Draw`, the managers' draws), the looks phase.
