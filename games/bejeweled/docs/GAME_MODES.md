# Game modes and the fun lab

The user asked for combinations of the mods' features and settings that make fun, balanced game modes, tested with a
bot, and for ways to put a number on "fun" in a game of Bejeweled. This is the method, what it found, and the five
modes it chose. The modes are curated mods (CLAUDE.md, "Mods"): `forge`, `prism`, `halo`, `goldsmith` and
`arrowhead` in `games/bejeweled/mods/`. Each starts off; turn one on at a time in the mod menu, as each description
says.

## 1. The lab

`bejeweled_verify funlab` (src/headless/FunLab.cpp, native only) plays bot games on any set of mods' rules with the
game's own Board code and no window, sound or files: several hundred games a minute on one core. The rules merge as the
game merges them (ModManager's order, the settings resolved as the mod menu would), so a mod's RulesId in the lab is
the game's. Each game prints one line of measures; `games/bejeweled/tools/funlab/run.py` plays a file of candidate
modes (each a list of mods and their settings) and `report.py` sums them up and marks each measure against its band.

    build/native/rundesk.exe python games/bejeweled/tools/funlab/run.py modes.json --seeds 1..20 --jobs 3

Two bots play every seed in both Normal and Timed, each looking at the board for a second (100 updates) before a
move, so "minutes" are game time at about a human pace.

The **skilled** bot plays as an attentive player who cannot see the future. It tries every move on a copy of the
board whose random generator it has reseeded itself (twice, with different guesses), so it knows exactly what the
move itself matches but only guesses the gems that fall in after. Of the moves least likely to end the game (a board
left with few moves, an arrow left at its edge), it takes the one that scores most, or gilds most on a gold board.
It looks one move ahead, like a player who checks that a move does not leave them stuck.

The **novice** plays any move it can see, at random: a player who has just learned the rules. It is a floor, not a
person; real newcomers do better. Its games show how forgiving a mode is, and the gap between the two bots' scores
shows how much skill counts.

## 2. Measuring fun

No single number is fun. The lab measures the parts that the study of games ties to it, and reads them together:

**Length and pace.** The skilled bot's game minutes (median) and the moves each level takes. A game that ends in a
minute or two leaves no room to get absorbed, and one that never ends has no stakes. Isaksen, Gopstein and Nealen
(*Exploring Game Space Using Survival Analysis*, FDG 2015) read a game's difficulty from how long bots survive in it;
this is the same reading.

**Choice.** The moves the board offers at each decision (median), and the share of decisions with two or fewer. A
game, in Sid Meier's phrase, is a series of interesting decisions; a board with one move left is no decision at all.

**Rewards that surprise.** The share of moves that set off a cascade (a second round of matches from the gems that
fell in), and "big moments", moves that score four times the median of the level's earlier moves. Both are rewards
the player did not plan and cannot predict, the variable-ratio schedule that John Hopson (*Behavioral Game Design*,
Gamasutra 2001) showed keeps players most engaged. Events per minute count the power gems made and arrows cleared:
new things happening.

**Tension.** The share of decisions made in danger: two or fewer moves left in Normal, the Timed bar or a gold
level's clock below a quarter, an arrow one gem or less from its edge. Close calls count the dangers a game came
out of. Csikszentmihalyi's flow sits between boredom (no danger) and anxiety (danger all the time), and Browne and
Maire's measured "drama" (*Evolutionary Game Design*, IEEE TCIAIG 2010) is the same kind of comeback.

**Fairness.** The share of game overs that came with a warning, with danger at one of the last three decisions. An
ending the player saw coming feels earned; one out of nowhere feels cheap. Bejeweled's own Normal mode is known for
the sudden "no more moves", and it shows: about one game over in six came without warning.

**Skill.** The skilled bot's median score over the novice's. When better play wins more, the choices mean
something (Nielsen, Barros, Togelius and Nelson, *General Video Game Evaluation Using Relative Algorithm Performance
Profiles*, EvoApplications 2015, rank games by whether better algorithms do better).

The bands (report.py) take the rewards from the original game, since they are what Bejeweled feels like: a cascade on
25 to 55 % of moves (the original: 42 %) and 4 to 14 big moments in 100 (the original: 9). The rest are design
targets: 8 to 45 minutes in Normal and 4 to 15 in Timed; 15 to 60 moves a level in Normal, 6 to 30 in Timed; at
least 4 moves to choose from and at most 10 % of decisions forced; danger at 2 to 35 % of decisions; a close call a
game in Normal (half one in Timed); 60 % of game overs warned; a skilled score at least one and a half times the
novice's; and a novice's game at least 3 minutes in Normal and 2 in Timed.

## 3. What the rounds found

Four rounds tried about 50 candidates, 80 games each (seeds 1 to 20, both modes, both bots). A curated mode never
copies another game's mode, though, and five of the first six leaned on one (Bejeweled 2's power gems, Twist's
twists, Jewel Quest's gold, Bejeweled 3's Butterflies, Zen with Twilight), so three more rounds tried about 30
candidates that give each an idea of its own. The coordinator's review of those six found four still too close to
another game's mode (Twist's board, Jewel Quest's gold, Butterflies, Zen), so three more rounds tried about 40
candidates on new rules: power gems that only diagonal runs make, gold that only blasts gild, and arrows that twists
turn, with any edge but the top a shot. Still Water was dropped: a calm endless mode needs a mechanic of its own. The
five modes were then played again on seeds 101 to 150, which no tuning had seen. Some findings are worth more than the
modes:

- **Blasts alone gild slowly.** When only power gems gild, no 7 x 7 or 8 x 8 board got past level 1 in four minutes
  with Gem Forge's 3 x 3 flames. A 5 x 5 flame, and power gems from runs of 4 and 5 as well as from shapes, clear about
  a level every five minutes, and the clock, 10 seconds shorter each level, ends the game.
- **Diagonal power wants diagonals of four.** When every diagonal run of three forged a flame, flames came 28 times a
  minute and more than half of all moves cascaded. Diagonals of four or more, on a 10 x 10 board with 8 colours,
  brought every measure but danger into band.
- **Steering and a small bonus tame the arrows.** With twists turning the arrows and three edges of four counting as
  shots, a bonus of 100 an arrow made the levels fly (7 moves a level); at 20 the skilled bot's Normal score is about
  130 times the novice's, where the first Arrowhead's was 434 times.
- **Seven colours is the sweet spot, and the lab finds it.** With 6 kinds of gem, nine games in ten were still going
  at the 30-minute cap; with 8, a game lasted 3 minutes. The original's 7 gives 8 to 11 minutes with every measure in
  band. More colours need a bigger board or more ways to match: 8 colours on a 10 x 10 board, or 10 colours with
  diagonal runs on 9 x 9, come back into band.
- **Power gems lengthen games and add big moments** without changing how often a move cascades.
- **Twilight's alternating gravity** sets off a cascade on half of all moves and makes Normal games last over 20
  minutes; on its own it is a relaxed mode.
- **Arrows feed on their own bonus.** The bonus grows with every arrow cleared, it fills the level bar, and a new
  level starts with no arrows, so the bonus is also the way out: a smaller bonus made games shorter, not longer.
  Arrows pointing every way end most games in 3 to 4 minutes; arrows pointing up last two to three times as long,
  since an arrow falls back with the gems when the gems below it clear. Whatever the settings, a careless player loses in about
  a minute: arrows are a mode for attention.
- **Gold levels take long.** On the full board a gold level takes 100 moves or more and drags; on 7 x 7 with power
  gems it takes about 60, and the level's clock gives the mode the most tension of all (13 % of decisions in danger,
  22 close calls a game).
- **The diamond board** ends a Normal game in about a minute: its corners leave too few moves. A diamond of 60 cells
  (10 x 10, the board shapes example's large diamond) plays as long as the original board, with 10 of 11 measures in
  band in Normal and all 10 in Timed.
- **Diagonal arrows** (made for the diamond, whose edges are diagonal) end games fast whatever the settings. A
  diamond's diagonals are only about five gems long, so an arrow crosses in four moves. On the large diamond the
  skilled bot lasts about 2.5 minutes with a new arrow every 12 moves or every 16, so past a point the limit is
  catching the arrows, not how many come. Diagonal runs let it catch them for 4 to 7 minutes, but then nearly three
  moves in four cascade and the board plays itself. No mode came of them; the example's settings are there to try.
- **Twists alone with 7 colours** run long and end without warning (a third of game overs warned); 8 colours with
  power gems bring both into band.
- **Slides make matches too easy.** Every candidate that added slides to swaps or twists, with arrows, gold or
  power gems, ran to the 30-minute cap and cascaded on two moves in three.
- **A hole in the board** (a ring of 60 cells around a 2 x 2 hub) ends a twist game in 3 minutes, since the
  twists next to the hub turn fewer gems. Diagonal runs give the matches back: with 9 colours all 11 Normal measures
  are in band.
- **A twist carries an arrow**, a cell around, even back down. With twists as the only move arrows last longer than
  with swaps, and as with swaps a bigger bonus (100 an arrow) made games longer: 10 minutes instead of 4.

## 4. The modes

Seeds 101 to 150, 50 games a bot in each mode. Minutes and level are the skilled bot's medians; "novice" is the
novice's median minutes; "cascade" and "danger" are shares of moves and decisions; "big" is big moments in 100
moves; "skill" is the score ratio. The original rules are the first row.

Normal:

| Mode | Minutes | Level | Novice | Moves a level | Choices | Cascade % | Big | Danger % | Close calls | Warned % | Skill | In band |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| original | 10.7 | 8 | 2.7 | 34 | 7 | 42 | 8.8 | 2.3 | 4.4 | 84 | 14.2 | 10/11 |
| forge | 11.6 | 12 | 2.7 | 24 | 7 | 43 | 13.2 | 2.8 | 4.7 | 89 | 25.3 | 10/11 |
| prism | 20.6 | 18 | 6.4 | 25 | 9 | 50 | 13.2 | 0.9 | 1.9 | 81 | 17.7 | 10/11 |
| halo | 9.8 | 13 | 3.1 | 21 | 8 | 46 | 6.8 | 1.9 | 4.1 | 61 | 15.0 | 10/11 |
| goldsmith | 19.9 | 5 | 6.7 | 82 | 6 | 37 | 19.9 | 15.5 | 19.5 | 100 | 11.9 | 9/11 |
| arrowhead | 11.5 | 16.5 | 1.8 | 16 | 9 | 44 | 15.4 | 4.8 | 6.9 | 77 | 128 | 9/11 |

Timed:

| Mode | Minutes | Level | Novice | Moves a level | Choices | Cascade % | Big | Danger % | Close calls | Warned % | Skill | In band |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| original | 10.6 | 10 | 11.0 | 13 | 7 | 44 | 9.6 | 6.2 | 0.8 | 100 | 1.8 | 10/10 |
| forge | 11.0 | 17 | 10.9 | 10 | 7 | 43 | 13.1 | 6.8 | 2.4 | 100 | 2.7 | 10/10 |
| prism | 12.3 | 19.5 | 11.6 | 8 | 9 | 53 | 12.3 | 5.6 | 2.2 | 100 | 3.2 | 10/10 |
| halo | 13.5 | 24 | 13.0 | 7 | 9 | 48 | 6.3 | 5.2 | 1.6 | 100 | 2.5 | 10/10 |
| goldsmith | 11.5 | 3 | 5.0 | 67 | 6 | 38 | 20.6 | 12.1 | 0.6 | 100 | 5.6 | 8/10 |
| arrowhead | 15.6 | 38.5 | 3.4 | 8 | 10 | 47 | 12.1 | 7.4 | 8.9 | 100 | 71.0 | 9/10 |

Each mode below says what it draws on and the idea of its own that sets it apart.

**Gem Forge** (`forge`): shapes, not lines, forge the power gems. Four gems of one colour in a 2 x 2 square match
and make a flame gem; crossing runs join into an L, T or plus and make a lightning gem; a straight run only scores,
however long. Bejeweled 2 makes its power gems from long runs, so the eye looks along rows; here it looks for
corners and blocks, which the original never rewards. The lightning gem from an L or T is exactly Bejeweled 3's Star
Gem, so what is new is the square that forges a flame and the straight runs that forge nothing. The original's pace
and cascades, more big moments (13 in 100 moves against 9), and a wide skill gap: forging is something to plan. The
safest mode to recommend.

**Prism** (`prism`): the diagonals hold the power. Runs of four or more count on the diagonals, and each diagonal
run forges a flame gem; straight runs only score. Eight kinds of gem on a 10 x 10 board. Bejeweled's power gems come
from straight runs and their crossings; here the rows and columns score and the slants build. Long games (21 minutes
in Normal), every measure in band but danger (0.9 % of decisions), so a calm mode, and the novice lasts 6 minutes.

**Halo** (`halo`): twists only, on a ring of 60 cells around a 2 x 2 hub, with diagonal runs (a small bonus), nine
kinds of gem, and Gem Forge's shapes making the power gems. The first version, Carousel, made them from runs of 4
and 5, so the ring was all it added to Twist, and its name promised a board that turns. With shapes, a twist is the
tool that builds squares and crossings around the hole, which neither Twist nor Gem Forge asks for. Every Normal
measure in band but danger (1.9 %), games of 10 minutes, and the novice lasts 3.

**Goldsmith** (`goldsmith`): gild a 7 x 7 board before the level's clock runs out, but only blasts gild, so a match
leaves its cells plain. Gem Forge's shapes and runs of 4 and 5 forge the power gems, and a flame blows up a 5 x 5
square. Gilding is Jewel Quest's idea, where every match gilds; here the hunt for the last plain cells is a matter of
forging a power gem and aiming its blast. The first version, Gold Rush, gilded with matches and refilled a board with
no moves, so in Normal the skilled bot was still playing at 30 minutes in four games of five. Now the clock, 10
seconds shorter each level, ends four Normal games of five, at level 5 after 20 minutes. The tensest mode (16 % of
decisions in danger, about 20 close calls a game), every ending warned; a level takes about 80 moves, above the band,
and the blasts make big moments common (20 in 100 moves). Timed's levels have 3 minutes.

**Arrowhead** (`arrowhead`): arrows in the four straight directions cross the board one gem a move. One that leaves
by the top ends the game; one that leaves by another edge is a shot and scores the bonus, which grows with each arrow
cleared. The only move is a twist, and a twist turns the arrows it holds a quarter turn with it, so the player steers:
an arrow headed for the top can be turned aside into a shot. The first version followed Bejeweled 3's Butterflies,
whose butterflies only climb and end the game at the top; here an arrow goes where it points, and the player turns
it. Eight kinds of gem, a new arrow every 8 moves (one sooner each level, down to 5), and a bonus of 20 an arrow.
Games of 11 minutes in Normal and 16 in Timed; the novice, which never steers, is gone in 2, and the skilled bot's
score is about 130 times the novice's in Normal (the first version's was 434 times). A challenge mode, not a first
one.

Two near misses are worth keeping in mind: twists with diagonal runs and eleven colours on the full board (10 of 11
in Normal, 10 of 10 in Timed), and a 10 x 10 board with 8 colours and L, T and plus bonuses (11 of 11 in Normal, 9 of
10 in Timed), which plays like a bigger original.

## 5. What the numbers cannot say

A bot is not a person. The skilled bot looks one move ahead; good players plan further, above all with arrows, so
the arrow modes may be less harsh for people than the bot finds them. The novice plays at random, so a new player
does better. A second's look per move is a guess at human pace, which matters in Timed. The measures say nothing
about the look and sound, about novelty wearing off, or about the pleasure of a mode's idea, and the bands are
judgment. They found the original's own tuning, though (seven colours, between an endless 6 and a 3-minute 8),
which is some evidence that they measure something real. The modes are a starting point to play, not a verdict.

## 6. Running it again

`run.py` takes any modes file; `--only` picks labels, `--wait` the look per move, `--mode` and `--policy` one of
each. The lab exits with status 1 when a mod's rules do not load (an error, a dropped mod or a setting it does not
know), so a typo in a modes file fails loudly. A curated mod's RulesId in the lab's first line can be compared with
the candidate's it came from: the five modes' match their candidates exactly. Section 4's modes file is
`games/bejeweled/tools/funlab/curated.json`; the candidates' files stayed in the lab's output folder and were not
kept.
