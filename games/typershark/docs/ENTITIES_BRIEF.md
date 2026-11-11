# The entity classes (what the compiler did to them), for the translation jobs that cover 0x401000-0x412330

Read `TRANSLATE_BRIEF.md` first; this adds what is special about the game's sprite objects (GameObject, Enemy, Shark,
Piranha, Projectile, the bosses...). They are the one place where the export does not read like ordinary C++.

## What the export shows

- **Virtual inheritance.** Every concrete entity class is built from *virtual bases* `Enemy` and `GameObject` (RTTI lists
  the bases of Shark as `Enemy, GameObject`; `Torpedo` has `Projectile, Enemy, GameObject`; `RedShark` has `BlackShark,
  Shark, Enemy, GameObject`; `BackgroundFish` has only `GameObject`). `re/typershark/rtti.classes.json` gives, for each
  class, `bases` and its `vtables`: `va`, `offset` (where that vtable's sub-object sits in the complete object) and the
  `slots` (function addresses). Example: Piranha has a 1-slot vtable at offset 0 (its own part, the deleting destructor),
  a 12-slot vtable at offset 48 (the `GameObject` sub-object) and a 3-slot vtable at offset 288 (the `Enemy` sub-object).
  Each class's constructor / destructor (`ctor_or_dtor_*`) stores those vptrs: `*(this + -0x30) = &vftable_Sexy_Piranha;
  *(*(*(this + -0x28) + 4) + -0x28 + this) = &vftable_..._sub48; ...`. A load of `*(this + 8)` (or `+4`) is the *vbptr*,
  `vbptr[1]` the displacement of the first virtual base, `vbptr[2]` of the second, so
  `*(int *)(*(int *)(*(int *)(p + 4) + 4) + 0xbc + p)` is "field 0xbc of the GameObject part of p".
  Which of +4 / +8 holds the vbptr, and the sizes of the parts, differ per class: read it from the constructor.
- **vtordisp thunks.** The tiny functions named `Class::vfNN_subMM` are `sub ecx, [ecx - 4]; jmp real` (some also add a
  constant first): they turn the pointer to a virtual-base sub-object into the pointer to the whole object and jump to the
  real implementation. The **jump target** is the real function; the thunk itself is not translated. Read thunks with
  `llvm-objdump` (see TRANSLATE_BRIEF.md): the target and the added constant are visible there, `decread` hides them.
  Default implementations that a class does not override appear as the `GameObject::vf05` ... `vf11` stubs (0x4010a0...)
  directly in its table, with no thunk.
- **Field offsets** in a function are relative to the sub-object that function's `this` points to: GameObject's own
  functions (`GameObject::vf02` at 0x461270, ...) use offsets inside the GameObject part (e.g. the x position is the double
  at +0x98, y at +0xa0), the 3-slot `Enemy` functions (0x403130: the points a word is worth from its letters; 0x4011f0;
  0x40a5b0: one more letter typed) use offsets inside the Enemy part (the word is the `std::string` at +0x34, its length
  at +0x48). A real implementation reached through a thunk (e.g. 0x40a6a0, 1554 bytes, Piranha's slot 1) has `this` =
  the start of the complete object, and reaches the parts through the vbptr.
- The **STL helpers** (`std::vector<T*>`, `std::string`, `_Tree`) are the compiler's instances; use the standard library.

## Tools and first findings (checked against the machine code)

- `python games/typershark/tools/entity_slots.py` prints, per class, its vtables with the thunks resolved (`real(vt)` or
  `real(vt+imm)`), `--slots` summarises the 12 GameObject slots over all classes; `python games/typershark/tools/attribute.py
  --class Piranha` lists the functions guessed to belong to a class (the unnamed `FUN_` ones too).
- GameObject's 12 slots: 0 the deleting destructor; 1 has `_purecall` (0x50939d) as its default and is overridden by every
  concrete class (probably Draw, since GameObject's slot 2 default 0x461270 is a motion update: it keeps two 6-entry ring
  buffers of position deltas, x double at +0x98, y double at +0xa0, previous x/y at +0x48/+0x90 ...); 3 and 4 are getters
  (default: the ints at +0xd0 and +0xd4; the Enemy-level override reads the sub-object at +0x1e0 / +0x1e8); 5 sets x
  (+0x98, a double passed as two 32-bit halves), 6 sets y (+0xa0), 7 sets the int at +0xb8 (probably the state), 8 sets the
  byte at +0xe4 and clears +0xd8, 9 sets the double at +0xc0 (velocity x), 10 the double at +0xc8 (velocity y), 11 the int at
  +0xdc. Check each against its uses before naming it.
- Enemy's 3 slots (its own vptr at offset 0 of a standalone Enemy): 0x403130 the points a word is worth (each letter
  scores 1 to 4 by its letter, times 15), 0x4011f0 returns the double at +0xc4 of the GameObject part, 0x40a5b0 counts one
  more typed letter and splits the word into the typed and the remaining part (strings at +0x98 / +0xb4 of the Enemy part).
- Classes with only GameObject as base (BackgroundFish, Bubble, BonusText, Effect) are ordinary: the GameObject vptr is at
  offset 0 and the 12 slots are direct (no thunks).
- The class names include `NaziEditListener` (a dialog's edit listener); the port names it `NameFilterEditListener`.

## The model for the port

The port is free to use any layout, since nothing outside the game reads the objects; it must keep every field's meaning,
every virtual's behaviour and the **order of every random draw**. Use plain single inheritance where the hierarchy allows
it, and say in the class comment how the original's parts map:

    GameObject            (virtual base, 12 virtuals, the sprite: position, velocity, state, ...)
    Enemy : GameObject    (the typing target: the word, how many letters are typed, its score, ...)
    Shark : Enemy, BlackShark : Shark, ...   Projectile : Enemy, Torpedo : Projectile, ...

If a class really needs two parts that are not in one chain (check the RTTI bases), use C++ virtual inheritance with the
same shape. Give every virtual slot a name that says what it does (not `vf05`) in `GameObject.h`; one comment line per slot
with the original's slot number and address.

## Jobs

1. **The base model** (done by the coordinator session): `ENTITIES.md`, `GameObject.h/.cpp`, `Enemy.h/.cpp`, and
   `tools/vbase.py` (decread now writes `G.xx` / `E.xx` for the virtual-base fields; ENTITIES.md says how to read it).
2. **The concrete classes**, a few per job (about 25 KB of original code each), each reading `ENTITIES.md`,
   `GameObject.h` and `Enemy.h` first: first Shark and Piranha (every Shark/Piranha method, constructors, Update,
   Draw, the thunk targets), then Projectile and the torpedoes, then the sharks' variants, bosses and the rest of
   `entity_slots.py`. A field of Enemy or GameObject that the class uses and the header names only by offset gets its
   name in the header (keep the offset comment); a field of the class itself goes in its own header.
