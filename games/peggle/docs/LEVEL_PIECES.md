# The level pieces: what the exe's classes hold (decoded from our export)

The pieces of a level (the original's PhysObj and its kin) are read from `levels/*.dat` by `Board::LoadLevel2` (0x429d10), which
makes them with `SyncState` of each class. This file keeps the layouts while they are being worked out; the headers in
`src/` carry them once a class is translated. Offsets are the original's. Names come from the level editor's own property
names (`tools/edfields.py`), from `SyncState` and from the users of a field. `tools/vtable.py <Class>` prints a class's vtable.

## Classes and their order

`EditValObj` (the editor's base: vftable +0, a reference count +4, +8 a pointer, +0xc a byte; the port keeps only the count)
 - `PhysObj` (size 0xe4 or so; the level piece with a box, flags, a mover, a peg and an image)
    - `Poly` (0x148: points, lines, rotation, scale; the background picture of a level is a Poly with an image)
       - `Brick` (0x184 or more: a curved or straight brick)
    - `Ball`, `Hole`, `Line`?, `Gun`?: to be placed from their vtables.
 - `PegInfo` (0x28: what makes a piece a peg: kind, hit, flash and the glow timers), a reference-counted object at PhysObj +0xd0.
 - `Mover` (0x68: how a piece moves: a shape with radii, phases and pauses; reached from PhysObj +0x48).

Pointers to a Mover or a PegInfo are shared and counted (the original's `SmartPtr<T>`: `InterlockedIncrement(p + 4)`); a Mover
is saved with `FUN_00406860`, which writes 0 for none, 1 and the mover for a new one, or the number of one already written.

## PhysObj (the fields found so far)

| offset | what | from |
|---|---|---|
| +0x04 | reference count | `FUN_00402d00` |
| +0x10 | the kind of piece (the Poly constructor sets 5; `GetTypeFlags` tests 2 and 3) | ctor, `GetTypeFlags` |
| +0x14, +0x18, +0x1c, +0x20 | the box: left, top, right, bottom (floats) | `GetCenter`, `Translate` |
| +0x24 | collidable (bool) | `physobj_collidable` |
| +0x25 | visible | `physobj_visible` |
| +0x26 | moving | `physobj_moving` |
| +0x27 | background | `physobj_background` |
| +0x28 | foreground | `physobj_foreground` |
| +0x29 | foreground 2 | `physobj_foreground2` |
| +0x2a | base object | `physobj_baseobj` |
| +0x2b | the ball's stop resets | `physobj_ballstopreset` |
| +0x2c | draw sorted | `physobj_drawsort` |
| +0x2d | draw floating | `physobj_drawfloat` |
| +0x2e | (copy of +0x2d; set to 1 for kinds 8, 9, 13 when read) | `SyncState` |
| +0x30 | a byte saved in the version 15 group | `SyncState` |
| +0x34 | bounciness (float) | `physobj_bouncy` |
| +0x38 | rolliness (float) | `physobj_rolly` |
| +0x3c | the largest bounce speed (float) | `physobj_maxbouncevel` |
| +0x40 | solid colour (int) | `physobj_solidcolor` |
| +0x44 | outline colour (int, 0xffffff when none) | `physobj_outlinecolor` |
| +0x48 | the Mover (shared pointer) | `Update`, `SyncState` |
| +0x4c | the mover's time (counts while the piece moves) | `Update` |
| +0x50 | updates until the piece stops moving | `Update` |
| +0x54 | sound (byte) | `physobj_sound` |
| +0x5c | id (string) | `physobj_id` |
| +0x78 | logic (string) | `physobj_logic` |
| +0x94 | image name (string) | `physobj_image` |
| +0xb0 | sub id | `physobj_subid` |
| +0xb4 | the image (owned when +0xc4 is set) | `DeleteImage` |
| +0xb8, +0xbc | image offset x, y | `physobj_imagedx`, `physobj_imagedy` |
| +0xc0 | image rotation (float) | `SyncState` |
| +0xc4 | owns the image | `DeleteImage` |
| +0xc8 | updates until the piece is made inactive | `Update` |
| +0xcc | flipper flags (byte) | `physobj_flipperflags` |
| +0xd0 | the PegInfo (shared pointer), null for a piece that is not a peg | `GetTypeFlags` |
| +0xdc, +0xe0 | saved by version 2 files only | `SyncState` |
| +0xe1 | a flag read by `GetTypeFlags` | `GetTypeFlags` |

Virtual slots of PhysObj (`tools/vtable.py PhysObj`): 0 destructor, 1 EditGetSetValHook, 2 SyncState, 3 GetClass, 4 GetTypeFlags,
5 EditDrawMovement, 6 EditDrawOutline, 8 EditContains, 9 EditIntersects, 12 EditGetDragMode, 15 LoadImage, 16 EditReflect, 17 SetPos
(+0x44), 18 Translate (+0x48), 19 SetVelocity (+0x4c), 21 DeleteImage, 22 Update (+0x58), 23 UpdateSlowMo, 24 DrawShadow, 25 Draw,
26 DrawLight, 28 NotifyCollision, 29 GetCenter, 30 GetXPos, 31 GetYPos, 32 IsCollidableWith, 33 SetActiveWithGrowAnim. The Edit
slots belong to the level editor and are not translated.

## Poly (0x148): fields from the constructor 0x481670 and the editor names

+0x10 is set to 5; +0xe8 .. +0x120 words are zeroed; +0x12d and +0x12e are 1. Editor names: `poly_imagemirror` +0x130, `poly_growtype`
+0x13c; the rest by use. `Brick` adds `brick_leftangle` +0x150, `brick_rightangle` +0x154, `brick_thickness` +0x158, `brick_rotation`
+0x15c, `brick_subdiv` +0x168, `brick_type` +0x16c, `brick_texflip` +0x180. `Hole`: `hole_circular` +0x10c, `hole_score` +0xf0,
`hole_outdelay` +0xe8. `Mover`: `mover_offset` +0x14, `mover_radius2` +0x28, `mover_pause1` +0x2c, `mover_phase1` +0x30,
`mover_pause2` +0x34, `mover_phase2` +0x38; the current place is at +0x54, +0x58.
