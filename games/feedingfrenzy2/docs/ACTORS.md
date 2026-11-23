# Actors and their visuals: the map

A working map of the actor classes, their visual components and their states, read from the export while the port
translates them. Offsets are the object's own (the decompiler's struct offsets for Actor are shifted: its `offset_X`
is +X+8 and its `Actor_data.offset_X` is +X+0x28).

## The spawn's pool and its actors

- NonPlayerSpawn's pool (+0x50c) is a vector of actors resized to poolSize (0x4143b0, 0x415d70) and filled by the
  class's slot 4 (a new actor) and the spawn's slot 0x54 (0x41da40: the actor's slot 0x04 with the stage, its
  creation of its visual, and its died signal at +0x0c connected to the spawn, 0x4c26c0). Taking one (0x475b30) pops
  the pool's last, or makes a new one when the pool is empty, and pushes it on the live list (+0x4fc, a vector).
- The spawn (0x474d50): take one; the spawn's slot 0x58 sets it up (0x4224a0, FishSpawn's 0x4225a0); the location
  point (+0xa0) is a Vector2<int>, made float (0x4375a0); the actor's slot 0x24 gives the entry point
  (out, stage, &location, &point), 0x28 sets the position, 0x18 adds it to the stage (stage, location).
- FishSpawn's spawn (0x474dd0): its behaviour's +0xe8 (the school's size) above 1 makes a school (SchoolOfFish, 0xa8
  bytes, 0x4101b0) of fish placed round the first.

## Actor (0xf0 bytes and its virtual base; vtable 0x548914 with 66 slots; constructor 0x4226f0)

+0x0c signal0 (died), +0x1c a Ref (0x424550's kind), +0x20 the visual (Ref), +0x24 five shape slots of 0x0c bytes
(constructor 0x46d9d0: the shape, its transform, the cached transformed shape), +0x60 (0x417230's), +0x78, +0x88,
+0x8d (byte), +0x90, +0x94, +0xa0, +0xa8, +0xb0 and +0xb8 (Vector2f each), +0xcc and +0xd0 (floats), +0xd4 (byte,
1), +0xd8 (0x417270's, 0x10 bytes), +0xe8 (a Ref), +0xec.

## Visual (a Container, 0xec bytes; vtable 0x553d24 with 71 slots; constructor 0x420b50)

Container's slots but for 0x00 and its own 0x118 (0x41f700, its signal's connections dropped, 0x41fc60).
+0xb8 HasSlots, +0xc8 signal0, +0xd8 the visual data (Ref), +0xdc the AnimatedImage, +0xe0 "VisualMirroredGroup",
+0xe4 "VisualBehindGroup", +0xe8 "VisualFrontGroup". Its set-up (0x469130): the behind group, the mirrored group, the
image (inside the mirrored group, its finished signal connected to the visual's 0x4c2a40), the front group, each a
Container (0xc0 bytes, 0x468fb0) added in that order.

- Play (0x461360, the visual in edi; name, rate, mode, restart): the run of the name in the data (+0xd8); the
  direction 1, or -1 for a rate below 0; the image already on that run in that direction (not 0) is left alone
  unless restart; else the run set at its first frame (its last going back, 0x494ca0 the run's length), the mode
  set, the rate's magnitude set, the image's update bit (4) set and the direction set.

## AnimatedImage (engine, 0xcc bytes; vtable 0x55b914 with 60 slots; constructor 0x424bb0)

Overrides paint 0xa0 (0x4464e0), update 0xa4 (0x4381a0), computeBounds 0xa8 (0x433970). +0x98 signal1<unsigned>
(finished, with the run), +0xa8 the AnimationData (Ref), +0xac the run, +0xb0 the frame, +0xb4 the mode (0 once,
1 back and forth, 2 round), +0xb8 the direction, +0xbc the rate (1), +0xc0 the time left on the frame.

- update: with time left at or above 0, the time less the rate times the step; else frames advanced while below 0.
- advance (0x437c70): past the run's end (or its start going back), mode 2 goes round, mode 1 turns back (to the
  frame after the end it reached, kept inside the run), mode 0 stops (0x425b70: direction 0, time 0, the update bit
  cleared, the finished signal with the run); else a frame on in the direction; the run's frame time added.
- set run (0x46f8f0; run, offset): the run's start plus the offset, its frame time.
- paint: the frame drawn through the context's slot 0x08 where the image shows (0x84); bit 1 cleared.
- bounds: the first frame's box at the position less its anchor; none without frames.

## FishVisual (0x4a8 bytes with its base; vtable 0x550ad4; constructor 0x420c50)

Overrides 0xa4 update (0x43b470) and 0xdc (0x421b60). Constructor (stage's +0x2d4, the fish, 1):
+0xec signal0, +0x100/+0x104 the last update's time (a 64-bit count; a second update in one logs "DOUBLE UPDATE"),
+0x108 the first argument, +0x10c the fish visual data, +0x110 the fish, +0x114 and +0x118 Refs, +0x128 (float
0x566b44), +0x12c (0x566924), +0x130 (0x566858), +0x134 to +0x137 (bytes), +0x138 (Vector2f), +0x154 1, +0x158 0,
+0x15c -1, +0x164 1, +0x170 1, +0x174 (0x42f6d0's), +0x1c0 and +0x1e8 (0x41db20's), +0x210 (a Ref), +0x218 four of
0x88 bytes, +0x438 to +0x444 Refs, +0x448 a list, +0x458 its state machine (FishVisualStateMgr, 0x41a510), +0x484 to
+0x4a0 eight callbacks.

States (each's +0x0c the visual): Idle plays "idle" and Swim "swim" (rate 1, mode 1 where the model's idleShuttle or
swimShuttle is 1 or more, else 2); Stunned plays "swim" and stops it at once. The strings: 0x5ac894 "idea",
0x5ac700 "glow", 0x5ac71c "idle", 0x5ac8cc "swim", 0x5ac878 "turn", 0x5ac8b0 "eat", 0x5ac8e8 "SuckIntro",
0x5ac920 "SuckOutro".

## Fish (constructor 0x481370) and AIFish (0x41d550, 0x1ac bytes)

- Fish's creation (0x4814a0) makes its FishVisual from the stage's theme; its addToStage (0x421ae0) resets the
  transform and pulse scales to 1, shows the visual, faces +1 entering from the left and -1 from the right, and sets
  the hold time to 0. setFishVisualData (0x422660) pulses from the old size over the new to 1 over 0.25 s when the
  old size is above 0; setFishBehavior (0x42aa30) draws max x, max y, the swim rate (0.0025 to 0.05), min x and min y,
  then the acceleration and drag.
- AIFish's addToStage keeps its home (the entry position), joins the stage's AI list and enters SpawnAI. Its update
  (0x4447e0) checks the layer, counts the hold time down, and leaves the state machine empty once the state says the
  fish has gone (FishState 0x4448a0: seen, then out of view). keepInWater (0x43d070) collides the body (shape 1) with
  the stage's BELOW_WATER edge underwater, ABOVE_WATER then SKY in the air (0x51e371), pushing the fish back by the
  separation. pickState (0x4288b0) draws r in [0, 1): a "flyBy" base, or flybyFreq (default 0.3) above r, gives
  FlyByAI, else WanderAI; the school and chase branches before it are not yet translated.
- Actor::move (in Actor.cpp) steers toward the desired velocity (ten times the difference, clamped to the
  acceleration) or applies drag when no desired velocity is set, clamps the force to its ellipse and the velocity
  to its maximum, stops below a speed of sqrt(0.3125), moves by the velocity times the step and keeps in the water.

## The AI states (FishState vtable 0x54fe8c; AIFishState 0x41fe60)

- SpawnAI (0x41fe00): the entry area is the fish's bounds inset by 80 at the sides; it swims at max x toward that
  area until inside, latching in play when the stage's area contains it, then picks a state.
- WanderAI (0x421c20): bounds of the home +-30 and +-40; three timetables (move, sight, state). pickMove (0x462530)
  draws among dart (0.2), center (1.2), random (0.2) and reverse (0.05), cumulative after their total; a move is
  clipped to the area's top and bottom over its duration, raised to the minimum speeds, and its rise dropped below 10.
  The "center" move aims at the area's half size, not its centre (the original's; a fix candidate).
- FlyByAI (0x41e910): heads away from the side of its home, a random rise in +-max y, re-drawn every
  wanderMoveChangeFrequency.
- The sight check (0x437ac0), run when a state's sight timetable comes due: the stage lists the actors the fish sees
  (0x43d0c0, into its +0x380 list): in the same water or air, within sightDist of the fish's mouth (the eater part's
  0x28, the visual data's mouth laid on the shapes' transform) and with the cosine to the way it faces above the
  behaviour's, nearest first. For each, nearest first, the behaviour's reactions in order (0x435180): the other is of
  the class to the fish (slot 0xe0: "predator" when it eats the fish, "prey" when the fish eats it), the fish may
  react so (0x130: run and avoid only from an eater, chase only what it eats, anything else always), and a draw in
  [0, 1) at or below reactFreq. The first found is reacted to (0x12c) and the look ends.
- FleeAI (0x415740): away from the threat at twice the top speeds, re-aimed every runMoveChangeFreq; the next state
  after runStateChangeFreq or once the threat is dead; it looks about again on its sight timetable.
- ChaseAI (0x47d020): from the mouth at the prey at the top speeds, re-aimed every chaseMoveChangeFreq; the next state
  after chaseStateChangeFreq, or once the prey is dead or eats the fish. It starts a sight timetable it never reads.
- AvoidAI (0x48f680, through AIFish's 0x13c): no enter, so the fish keeps its way; while the threat is in sight and
  the fish's wanted way (or its facing) is within a cosine of 0.85 of the line to it, either way, the wanted velocity
  turns 30 degrees from the threat's side; the next state once the threat is dead or out of sight.
- Checked on the menu over 9000 updates: the pompano chases the queen triggerfish, which runs from it and from the
  great white.

## Eating

- The stage's meeting step (0x50, 0x440660) takes each pair of fish once, in the list's order: the first ranked below
  the second's eater rank is offered to the second's eater; else the second, ranked below the first's, to the first.
  The player's meetings with the actors (the +0x340 list) come after, with the players.
- The eater's consider (0x43dee0): the mouth's shape (shape 2), the fish able to start an eat (0x43df80: a state in
  play, the visual's state allowing it, not held), the prey's state edible, its body (shape 1) meeting the mouth and
  its state allowing this eater (slot 0x58, always so far). Then the eat (0x47a2b0, after AIFish's fishEaten sound):
  the prey drawn in (EatenState) over the visual's time to its chomp, at most 0.2 s, kept in the eater's prey list
  (+0xfc), and the eater's state told (slot 0x54: an AI fish's becomes AIEatingState).
- EatingState (0x482210) connects the visual's chomp and starts its eat; at the chomp (or at its exit without one)
  the eater swallows (0x477660): each prey's state ends, so its spawn takes it back. AIEatingState moves the fish and
  picks its next state after the chomp.
- EatenState (0x480130) slides the prey from where it was to the eater's mouth and shrinks its visual's pulse scale
  to 0 over the seconds, then hides it at scale 1. It is neither in play nor edible and never goes by itself.
- Checked on the menu over 9000 updates: the pompano eats the triggerfish 16 times, each drawn in over 0.11 s and
  swallowed at the chomp.

## Schools

- A spawn whose behaviour's school size reaches 2 (0x474dd0) brings in a school (SchoolOfFish, 0x4101b0): its size
  drawn whole (0x41ce50), held to the phase's most (or the supply left beyond the live fish) less the leader. The
  leader comes in like a single spawn; the rest are placed in a box twice the separation distance on a side, behind
  the leader from the side it came in (across (-1,1) and set back by half for the left, (1,1) for the right and
  leading, (1,-1) for trailing). From any other location the original leaves the box as the stack held it; the port
  uses none (an oddity, no menu or level spawn seen to use one).
- The school (0x460400) updates once a frame, from the first of its fish to update: its centre and mean velocity,
  and a new goal when the goal's time comes or the centre is within 16 of it. A fly-by's goal is 187.5 past the far
  side, its height drawn 40 inside the top and bottom, with no time; a wanderer's is drawn 20 inside the bounds, its
  time from "schoolMoveChangeFreq".
- SchoolingAI (0x445610) works its pulls out every 0.05 to 0.1 s: away from the nearest fish within the separation
  distance, the others' mean velocity less its own, towards the centre and the goal, and away from each predator in
  sight. A predator frightens the whole school for 1.5 to 2 s (0x461d90): top speed by the flee factor, twice the
  acceleration and a fifth of the goal's pull. The sum is pushed on the fish each step with no drag. A fish whose
  school is down to itself picks its own state again.
- Checked on the menu: the six herring of FlyBySchoolMenu come in together and cross the screen as a group.

## The stage's edges

Six Edge shapes (vtable 0x555b3c, a normal and a distance) at +0xc4 (sky), +0xe4 (bottom), +0x104 (left), +0x124
(right), +0x144 (below the water) and +0x164 (above the water), set from the base layer's pan size, the water level
and the sky's ceiling as the layers are set up. getArea (0x44c6f0) gives the play area from the water down, or from
the sky to the water.
