# The entity classes: layout, conventions, what is known

Read `ENTITIES_BRIEF.md` for what the compiler did to the classes; this file is the result of reading them. The base
classes are written: `GameObject.h/.cpp` and `Enemy.h/.cpp` (every field and virtual has its original offset or slot
in a comment). The later jobs add the concrete classes and, when a field of Enemy or GameObject turns out to be used,
give it a name there (edit the header; keep the offset comment).

## The hierarchy of the port

    GameObject                 12 virtuals (the table below), position, velocity, state, frame counter
    Enemy : GameObject         3 virtuals more (GetPoints, GetSpeed, TypeLetter), the word, its colours and fonts
    Shark, Piranha, Projectile : Enemy        and the rest of RTTI's list (re/typershark/rtti.classes.json)
    BackgroundFish, Bubble, BonusText, Effect, Player : GameObject   (RTTI: only GameObject as base)

The original builds every creature from the virtual bases Enemy and GameObject. A complete Shark is: its own part at
+0 (vptr, the vbptr at +8, its own fields), GameObject at +0x30 (0xf0 bytes), Enemy at +0x120 (0xf0 bytes), total 0x210;
Piranha has the same shape. Other classes put the parts at other offsets (Torpedo: GameObject 16, Enemy 264, Projectile
part 504): read them from `re/typershark/rtti.classes.json` (`vtables[].offset`) and the constructor.

## Reading entity code with decread

`python games/typershark/tools/decread.py <addr>` now rewrites every access to a virtual base: `G.d8` is the field at
+0xd8 of the GameObject part, `E.58` the field at +0x58 of the Enemy part (`G.e4{c}` a byte, `{d}` a double, `{s}`,
`{f}` as before), `&E.0` the address of the Enemy part. This is `tools/vbase.py` and is right for the direct forms
(`*(int *)(this + *(int *)(*(int *)(this + 8) + 4) + 0xe4)`) and for locals that hold the vbptr, a vbtable entry or a
sub-object address, **in statement order and block by block**; a branch that reassigns such a local, or a comma
expression (`(iVar8 = ..., *(char *)(iVar8 + 0xa8 + param_1))`), can leave a raw form behind. Then the rule is: the
address is `start of the part + (X - A)` where A is the offset from `this` of the `*(int *)(this + A)` that read the
vbptr (+8 in a constructor of Shark/Piranha, -0x28 in their methods, whose `this` is the GameObject part), and the
part is `K = 1` for GameObject and `K = 2` for Enemy (the entry 4 or 8 of the vbtable); check the K of a class that is
not Shark or Piranha against its constructor's vtable stores.

Fields of the class's *own* part stay raw: in a constructor they are `this.10`, `this[0x20]` (the complete object, from
0), in a method whose `this` is the GameObject part they are `param_1 + -0xc` (the own offset is X + 0x30).

`FUN_00401220(&E.f0)` and the other calls with the odd argument `&E.f0` (or an `E.c`-relative pointer) are calls of
GameObject helpers written against the Enemy-anchored layout: 0x401190 `SetState`, 0x401220 `IncTick` (`G.e8++`),
0x401a90 sets `G.a0` (a double: the y), 0x402180 / 0x402230 `GetFrameWidth` (G.d0), 0x4021a0 / 0x402250
`GetFrameHeight` (G.d4), 0x401250 / 0x401260 `SetTarget` / `SetAttacker`, 0x4022d0 `ReleaseTarget`. The thunks
`vfNN_subMM` only adjust `this`; the real function is the jump target.

## GameObject: the 12 slots (all classes)

| slot | name | default | meaning |
| --- | --- | --- | --- |
| 0 | destructor | deleting | |
| 1 | Update() | `_purecall` | one tick of the creature; every concrete class overrides it |
| 2 | Draw(Graphics*) | 0x461270 | the base only records the position history; every class draws first and calls it |
| 3, 4 | GetFrameWidth/Height | G.d0, G.d4 | image width / 20 and image height of the sprite sheet |
| 5, 6 | SetX, SetY | G.98, G.a0 | doubles passed as two 32-bit halves |
| 7 | SetState | G.b8 | |
| 8 | SetVisible | G.e4, and G.d8 = 0 | draws only when set |
| 9, 10 | SetVelX, SetVelY | G.c0, G.c8 | |
| 11 | SetFrameDelay | G.dc | ticks per frame |

Slots 5, 8 and 11 are never overridden; 9 and 10 by a few classes (a different address in `entity_slots.py --slots`);
3 and 4 differ between classes only in the way the compiler reached the field, and 6 and 7 in a few classes by a real
override (28 distinct targets in slot 7).

## What is known about the creatures

- **Frames**: the sprite sheets have 20 frames side by side; `G.d0 = image width / 20`, `G.d4 = height`, the frame
  counter `G.d8` starts at a random value below `G.e0` (20) and advances every `G.dc` ticks (`G.e8 % G.dc`).
- **Random draws**: the Shark constructor draws `MTRand::NextNoAssert(DAT_00565384) % 20` for the first frame, the
  Piranha constructor `_rand() % 20` (the CRT's generator, not the MTRand). Keep each in its place.
- **Images**: `app + 0x914` (sharks) and `app + 0x920` (piranhas) are the two `ImageVariantTable`s of WinWSApp (type, then
  picture role); `app + 0x8f0` is the table of loaded images by `ImageId`. `Enemy::mImages` holds six of them by role.
- **Events**: the creature pushes small ints on `Enemy::mEvents` (1: its word was finished; 2, 5, 6, 9 are the others
  seen in Piranha::Update); the Board drains the list. Their meaning comes from the Board's side.
- **Draw order of a creature** (Piranha 0x402360, shark 0x4024e0): `if (visible && state != 2) { RecordHistory;
  DrawImage(mImage, x, y, frame * frameWidth ...); SetFont; if the board is not paused and the state is 0 or 4: draw
  the word centred over the creature }`; sharks also draw the typed part in `mTypedColor` and the rest in `mTextColor`.
