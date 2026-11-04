# Token economy

What the sessions on this project spend their usage on, measured from the transcripts, and the rules
that follow from it. The short form of the rules is in `CLAUDE.md`.

## How a call is paid for

Every API call sends the whole conversation again. Claude Code caches it, so each call pays for:
- **cache reads**: the part of the context that was already cached;
- **cache writes**: new context (the last tool results), or all of it when the cache had expired;
- **output**: the reply, the tool calls and the thinking.

API prices per million tokens, the best public guide to how these weigh against each other (the plans'
own limits have no published formula):

| | Uncached input | Cache read | Cache write, 5-minute entry | Cache write, 1-hour entry | Output |
|---|---|---|---|---|---|
| Opus 5.5 | $4 | $0.20 | $5 | $8 | $20 |
| Sonnet 5.5 | $2 | $0.20 | $2.50 | $4 | $10 |

On Opus 5.5 a token that misses the cache costs as much as 25 (5-minute) or 40 (1-hour) tokens that hit
it. Main sessions get the 1-hour cache; subagents and workflow agents mostly the 5-minute one.

## The week of 2026-10-18

The weekly limit of a 20x plan ran out in about 36 hours (2026-10-18 and 19, UTC). From
`tools/token_report.py`:
- 22,433 API calls, all Opus 5.5, with an average context of 300K tokens.
- Weighed at API prices: cache reads 62%, cache writes 35%, output (with thinking) 3%.
- Who made the calls:

| | Calls | Cache reads | Cache writes | Share of the cost |
|---|---|---|---|---|
| Workflow agents (86 workflows) | 15,079 | 3.58B | 98M | about 57% |
| Subagents (70 agents) | 4,506 | 1.53B | 25M | about 20% |
| Main sessions | 2,848 | 1.49B | 15M | about 22% |

## What was wasted

1. **Workflows by default.** With ultracode on, every task became a workflow. Its agents ran 150 to
   275 calls each, at contexts of 400 to 530K tokens, and many of them read the same large design
   documents and sources.
2. **Expired caches.** 145 calls found their cache expired and wrote their whole context again: 61M
   tokens, about 14% of the cost. 123 of them were workflow agents, whose 5-minute cache ran out while
   they waited on a long command: the oracle, `until` loops waiting for another oracle, loops of game
   runs, builds. A rewrite of a 400K context costs as much as about 25 normal calls.
3. **Large contexts.** Sessions grew to 967K tokens before they compacted, and every call re-reads all
   of it. Auto-compact now triggers at 400K (2026-10-24).
4. **The fixed prompt.** Every call of every agent starts at about 55K tokens before any work, 18% of all
   cache reads. Most of it is tool definitions: the Artifact tool alone is about 15K tokens, the
   PowerShell tool (which does what Bash does here) about 5K, then Workflow, ScheduleWakeup and the
   app's session tools.
5. **Reading whole things.** Among tool results, the most context came from `sed -n` ranges, whole
   Markdown documents (design documents of 15 to 20K tokens, read by agent after agent), `cat`, grep
   output and 860 image reads. A result stays in the context for every later call until a compaction.
6. **Polling.** About 420 calls were `sleep`/`until` loops or Monitor checks. Each re-reads the whole
   context.
7. **Not worth chasing:** the model's own text and thinking (3%), and messages between sessions.

## Rules

- No workflows or parallel agents unless the user asks for them. For mechanical work (running tests,
  extraction, bulk edits) use one agent on Sonnet.
- A subagent never runs anything that takes more than about 4 minutes: the oracle, whole test suites,
  full builds, wait loops. The session that owns the work runs those itself, in the background.
- Read narrowly: grep for the place, then read that range. Do not `cat` or Read whole large files, and
  read the section of a document that matters, not the document.
- Test with `tools/check.sh`: one summary line per suite, logs in `build/logs/check/`. Open a log only
  when its suite fails. `tools/check.sh quick` (the native build and unit tests) takes seconds; the oracle
  and the game bots belong to the session that owns the work, in the background.
- Do not poll. Start a long command in the background and wait for its notification.
- Prefer page text to screenshots; when an image is needed, use a small scale or a zoomed region.
- Brief an agent with the files and sections it needs, so it does not read the design documents to
  find out.

## Measuring

```bash
python tools/token_usage.py ~/.claude/projects/<project folder>
```

prints the calls and tokens per transcript or folder, and

```bash
python -I tools/token_report.py 2026-10-17T00:00 2026-10-25T00:00 ~/.claude/projects/<project folders>
```

prints the full report: totals by model and day, the transcripts that cost the most, the context
charged to each kind of tool result, the cache rewrites and their causes, and the weighting at API
prices.

## The Sonnet trial: Noah's Ark Deluxe (2026-10-25)

The first game after the push ran on Sonnet 5.5 in one session, with no subagents or workflows (CLAUDE.md, "Sonnet
trial"). Up to the same milestone for each game, playing natively and in the browser, `tools/token_report.py` over
each game's own project folders gives the numbers below. The ends are the commits that reached it: Big Money's
0205e69 and BookWorm's 6ce3e42, both from before the token rules, and Noah's Ark's message of 13:50 UTC. The cost
column weighs every model at Opus 5.5's API prices, as the report does, so it overstates Sonnet's share.

| Port phase | Model | Effort (survey) | Time | Calls | Cache reads | Cache writes | Output | At Opus prices |
|---|---|---|---|---|---|---|---|---|
| Big Money Deluxe | Opus, 5 subagents | ~2x | 1 h 52 | 1,201 | 534M | 6.1M | 0.3M | $145 |
| BookWorm Deluxe | Opus, subagents | ~3-3.5x | 3 h 54 | 3,107 | 1.08B | 14.9M | 0.5M | $304 |
| Noah's Ark Deluxe | Sonnet, none | ~1-1.5x | 1 h 23 | 330 | 134M | 1.1M | 0.5M | $45 |

Per unit of the survey's effort estimate, Noah's Ark used about 107M cache reads, against 267M for Big Money and about
330M for BookWorm: between a third and two fifths as many, before Sonnet's lower prices. Part of that is the game, which
is the smallest and reused BookWorm's pipeline, and part is the single session, whose calls carried a large context
(409K on average, 967K at most, one compaction) but made a tenth as many calls. Output was a larger share (21% of the
weighted cost, against 3-4%), so Sonnet wrote and thought more per call. Its own account of where it struggled
(`games/noahsark/docs/README.md`, "Where the work was hard"): a misread goto tree in a Draw function gave an
out-of-range sprite index that looked like heap corruption, and it audited the game logic for a long time before an
AddressSanitizer build found the fault in one run.

The mods came next, in the same session: four of them (undo, a move helper, tide speed, a daily board), each with a
test in the game's check suite, in about 10 minutes and 64 calls (19.7M cache reads, 0.1M output, about $6 at Opus
prices). The session as a whole, to the rebase that made its branch mergeable, took 1 h 36 and 394 calls, about $50
at Opus prices, and every suite passed on the merged trunk (18bb77e). Its mods are fewer and smaller than Big Money's
and BookWorm's, so that phase does not compare one to one. One stumble was the app's, not the model's: the worktree
was cut from the publish snapshot instead of `main`, and the branch had to be rebased before it could merge.

How faithful the port is was checked afterwards by hand, on a sample of eleven functions compared with the Ghidra
decompilation: the move rules (0x407600 connecting a pair, 0x406aa0 and 0x406cd0 looking for a move and a partner,
0x4036f0 the path search along straight runs, 0x4026d0 the reach search), 0x4067e0 the abandon penalty, 0x402260 the
puzzle matcher, 0x401580 the level set-up, 0x403a90 the board rise, 0x411ea0 the high-score loader, and the gravity,
settle and swap-back steps of Board::Update (0x403d60). Seven are faithful. The path search follows the original line
for line; the others reach the same result by another route: a queue and a seen array for the original's two linked
lists, an early return where the original sets a flag and finishes the loop, a pointer swap where it clones a piece and
frees the old one. The puzzle matcher returns whether it found a run, which the Ghidra export had dropped; the
disassembly confirms the port's reading.

One is a real bug. The abandon penalty's partner search stops at cell 79, while the original's loop tests its bound
before the increment and so reaches cell 80: a partner in the bottom-right cell is never abandoned. Three others
behave differently in rare cases. The high-score loader drops the whole file at the first record whose check fails,
where the original keeps that name, cuts the score to its remainder modulo 256 and reads on; the port's comment
describes the wrong behaviour too. When the board rises (0x403a90) or a column slides over (in Update), the port
leaves each piece's screen position with the cell instead of moving it with the piece, so an animal flying off to the
ark at that moment jumps. The rest is small: a guard in 0x407600 that returns when one cell is linked where the
original returns only when none is, a comment that says five rows where the code makes four, dead writes with a
garbled expression in the board rise, and a swap-back that would take a lone flagged piece as both ends, which play
cannot reach.

So the Sonnet port is close but not exact. Its errors are the kind a translator makes while restructuring control
flow (an off-by-one from turning a test-then-increment loop into a for loop, a misread branch), not invented logic.
Its own tests could not catch them: its bot games compare the port only with itself, and nothing in them runs the
original exe for comparison. Whether Opus would have made fewer such errors is not known, since Big Money's and
BookWorm's ports have not been sampled the same way.

The decision that followed (2026-10-25, CLAUDE.md "Model selection"): Sonnet did well in most of what was measured,
so games whose port can largely copy another game's run on Sonnet, and games that need an engine or framework layer
no port has yet run on Opus. Opus covers Sonnet's deficits by review: the coordinator samples each Sonnet port's
translated functions against the decompilation before merging it, as above. Each later game is measured the same way
(tokens and time per unit of the survey's estimate, and the review's findings), so the rule can move a kind of game
back to Opus if Sonnet does badly on it.

## The next Sonnet port: Seven Seas Deluxe (2026-10-25)

Seven Seas was the first game ported on Sonnet under that rule: a 2003 game of the same estimated size as Noah's Ark
(~1-1.5x), copying Noah's Ark's port of the 2002 framework. Its one session, from its start to the tip with the
review's fixes (20:27 to 22:15 UTC, 1 h 48), made 431 calls: 184.6M cache reads, 1.3M cache writes and 0.6M output,
about $59 at Opus prices. That covers the port, a second pass on the dialogs (it first guessed their sizes, then read
them from the vtables and the creators' disassembly) and the review's fixes, but no mods, so it cost somewhat more
than Noah's Ark's whole session with its mods (394 calls, 1 h 36, about $50). Per unit of the estimate that is about
148M cache reads, against 107M for Noah's Ark's port and 267M for Big Money's.

An Opus review agent compared about 25 of its functions with the decompilation and the disassembly. The game logic
is a very faithful translation: the board, the enemies, the shots, the level generation, the order of the `rand()`
calls and the original's quirks all match. It found six differences. One desynchronises the random stream: undo
restored each ship with one `rand()` draw, where the original's inlined constructor makes a second. Three are rare
behaviour differences: a continued game's side panel counted its score up from 0; the high-score file ignored the
day and dropped a record's name on a bad check, the same mistake Noah's Ark's port had made; and the game-over talk
ended one update early (`< 300` for the original's `<= 300`, another bound turned around). Two change nothing but a
sound's pan or a field that held the same value. Three comments were wrong too. The session fixed all of it in one
turn, and the `<= 300` fix moved its bot's pinned results by one update.

## Atomica Deluxe (2026-10-25)

Atomica ran on Sonnet at the same time as Seven Seas, copying Noah's Ark's port of the 2002 framework for a 2003 game
that the survey put at ~1.5x. Its one session, from its start to the tip with the review's fixes (20:27 to 23:31 UTC,
3 h 03), made 771 calls: 371.8M cache reads, 2.4M cache writes and 1.1M output, about $115 at Opus prices. It ported
everything on its Status list, the menus, dialogs, tutorial boxes, bonus molecules, element atoms and swirls included,
but no mods. Per unit of the estimate that is about 248M cache reads, close to Big Money's 267M and well above Seven
Seas' 148M.

Much of the difference is context, not work. The session carried 485K tokens of context per call on average and
reached 965K before each of its two compactions. The 400K auto-compact of 2026-10-24 was set in the user's settings for
Opus 5.5 only (`modelSettings`), so the Sonnet sessions ran to the full window: Noah's Ark averaged 409K (967K at most)
and Seven Seas 431K. Every call re-reads that context, so the cache reads grow with it. Sonnet 5.5 got the same 400K
window at the end of 2026-10-25, so the next Sonnet ports should show whether that closes the gap.

An Opus review agent compared about 30 functions with the decompilation and the disassembly. The game logic is
faithful: the molecule search, the scoring, all 41 level set-ups, the bonus levels, the path finder, the random
generator and the order of the constructors' draws match, and in several places the port fixed what Ghidra had lost.
It found seven differences, weightier than Seven Seas'. The bomb button was never disabled, so the atomic bomb worked
at any charge and on bonus levels: the original disables it through a virtual call whose argument Ghidra drops, and
the port had left the call out. Two of the original's random-number users were missing (an object on the board that
draws a number every update, and the air element's clouds), so the port's random stream parted from the original's
from a game's first update on. The biohazard's infection set the wrong flag on the new atom and drew a stand-in effect
instead of the original's travelling spark. The saved game was four bytes short, so a save from the original exe
would be misread from its radiation cells on. The rest is small: the high-score date in local time instead of UTC, one
flag written where another was meant, and `Widget::Update` called where the original marks the board dirty. Three
comments were wrong too. Most of these are omissions beside code that Ghidra renders poorly (dropped arguments,
unnamed objects), not mistranslations. The session fixed all of it in one turn, and the restored random-number users
moved its bot's pinned results.

## Estimates against costs (2026-11-01)

The survey (`docs/POPCAP_GAMES_SURVEY.md`, 3.3 "Verdicts and effort") rates each game's effort against Bejeweled
Deluxe as 1x, from the size of its game code plus the subsystems it needs, each estimate within about 50%. The table sets those estimates beside what
each port cost up to its first merge: the "Port work" column of README.md's progress table of 2026-10-26, computed by
`tools/game_costs.py` with each model at its own API price. The last three rows are the ports from before the rules
of 2026-10-24.

| Game | Model | Estimate | Port cost | Per unit |
|---|---|---|---:|---:|
| Noah's Ark | Sonnet | 1-1.5x | $62 | ~$50 |
| Seven Seas | Sonnet | 1-1.5x | $54 | ~$43 |
| Atomica | Sonnet | 1.5x | $105 | ~$70 |
| Cosmic Bugs | Opus | ~2x | $125 | ~$63 |
| TipTop | Opus | 2-3x | $106 | ~$42 |
| Ning Po | Opus | 2-3x | $66 | ~$26 |
| Pixelus | Sonnet | ~3x | $91 | ~$30 |
| Word Harmony | Opus | 3-4x | $115 | ~$33 |
| Rocket Mania | Sonnet | 3.5x | $134 | ~$38 |
| Dynomite | Opus | 4.5x | $164 | ~$36 |
| Bookworm Adventures Vol. 2 | Sonnet | 4-6x after Vol. 1 | $195 | ~$39 |
| Bookworm Adventures | Opus | 10-15x | $538 | ~$43 |
| Bejeweled | Opus | 1x | $251 | $251 |
| Big Money | Opus | 2x | $263 | ~$132 |
| BookWorm | Opus | 3-3.5x | $369 | ~$114 |

Mummy Maze ($84), Alchemy ($70) and Water Bugs ($45) were rated only "small" and are left out of what follows.

**A fixed cost per game.** The survey scales effort with code size alone, but every port also pays for work that does
not grow with the game: staging its files, the Ghidra export, its web page and store entry, a check suite, a review and
a merge. So the smallest games cost the most per unit: $43-70 for the games of 1-1.5x, $30-43 for those of 3x and
more. Every port since the token rules lands within about 40% of

    port cost = about $30 + $33 per unit of the survey's estimate

and all but Ning Po within a third. It is a budget for a port before it starts, and a warning when a running port has
gone far past its own.

**Penalties set too high.** The survey added effort for a game without RTTI and for an archive format to decode before
anything else. Dynomite, with no game RTTI, cost the same per unit as Rocket Mania, which has it. Ning Po, rated 2-3x
for its archive, cost what a 1.5x game costs, though TipTop, with the same penalty, came in on its estimate. Two games
came in over: Atomica, whose session ran to the full context window before Sonnet got the 400K auto-compact (above),
and Cosmic Bugs, whose engine the survey did not know and guessed at ~2x. Sequels on a shared engine were estimated
well: Volume 2 (4-6x after a 10-15x first game) cost 36% of Bookworm Adventures' port, and Water Bugs 36% of Cosmic
Bugs'.

**The process mattered more than the model.** The three ports from before the rules, with subagents and many short
calls, cost $114-251 per unit; Bejeweled's includes setting up the repository and the framework. Every port since has
cost between a sixth and a third as much per unit. Sonnet ports cost about the same per unit as Opus ports in API
dollars: cache reads are most of every session's cost and are priced the same for both models, so Sonnet saves only on
output and cache writes, and its sessions carried larger contexts. Whether it weighs less against the plan's limits is
not known, since those have no published formula. Its ports also need the Opus review that found the faults above.

**Mods and shared work.** By the scan of 2026-11-01, mods are $1,709 of the $5,825 spent so far, 29%; Bejeweled's mods
alone ($1,268) cost five times its port. Shared code and coordination came to $999. The scan counts $283 of work not
yet classified as shared by default, so that figure is approximate.

**What the estimates leave out.** They are for a first playable port. Since 2026-10-27 a game is done only when it plays
identically to its original (CLAUDE.md, "Done means identical"), and the survey has nothing for that pass. On the
games' side its first costs are small (from 2026-10-26 to 2026-11-01: Bookworm Adventures $86, Seven Seas $20, Water
Bugs $11), while its tools (origrun, shotdiff, the originals' 3D mode) count as shared. A rule for it waits until more
games have finished the pass.

For the ports under way the rule gives about $180 for Ancient Japan (4-5x), about $195 for Bonnie's Bookstore (4-6x)
and about $855 for Twist (25x). It gives about $1,020 for Bejeweled 3 and $1,185 for Zuma's Revenge, but those lie far
past Bookworm Adventures, the only large game it was fitted on, so they are rough.

**Twist, measured (2026-11-03).** Bejeweled Twist's one session ran 2 days 11 hours, from its start (01:06 UTC on
2026-11-01) to its last commit before its final merge (11:59 UTC on 2026-11-03). It made 14,647 calls, 3,314 of them
by Sonnet subagents: 3.18B cache reads, 32.4M cache writes and 11.3M output, about $1,076 with every model at Opus's
prices (`tools/token_report.py`, so the Sonnet share is a little overstated). That covers the port, its 3D mode,
both phases of its identical-play pass and the shared code it wrote (the GPU core, the software stretch and
transforms, the OpenJPEG low-end patch). The rule gave $855 for the port alone. Per unit of the survey's 25x it is
about $43, what Bookworm Adventures paid per unit for its port alone, so the survey's size held and the pass came
cheaply on top of it. The estimates in days were far off. On the evening of 2026-11-01 Twist put the work left at 14
to 22 days and its 3D mode at 6 to 10; they took about a day and a half, and about 16 hours from the first GPU commit
to the 3D merge. At 21:54 UTC on 2026-11-02, with the account at 61%, the coordinator built on Twist's estimate and
put the rest at about 60 percentage points of the weekly limit (45 to 80); it took about 6 (Twist's $180 of the $384
every session spent while the account went from 61% to 73%). That week a percentage point came to about $32 ($2,352
across the project's folders for 73%). So a large game's remaining work is estimated from this rule and the survey,
at the week's dollars per point, and never from a session's own guess in days.

## The 400K auto-compact (measured 2026-11-01)

Sessions compacted only near the full 1M window until 2026-10-24, when auto-compact moved to 400K for Opus 5.5; Sonnet
5.5 followed at the end of 2026-10-25. Every main-session call in the byo-pop transcripts, before and after, each API
response counted once and each model at its own API price:

| Main sessions | Calls | Context per call (average / median / 90th percentile) | Per call | Compactions per 1,000 calls |
|---|---:|---|---:|---:|
| Opus, 1M window | 31,775 | 505K / 497K / 863K | $0.141 | 2.9 |
| Opus, 400K | 26,525 | 234K / 233K / 341K | $0.083 | 6.9 |
| Sonnet, 1M window | 2,012 | 446K / 401K / 860K | $0.115 | 2.5 |
| Sonnet, 400K | 9,002 | 243K / 239K / 345K | $0.067 | 6.7 |

A call now costs about 40% less. The calls since the change cost about $2,800; at the old cost per call they would
have cost about $4,800. Sessions compact more often, about 145 more compactions so far, and each costs about
$0.40-0.60 (the summary call, and the first call after it writing the new context into the cache): about $90 against
about $2,000 saved. The other token rules came the same day, but the drop in context per call belongs to the
threshold: a session's context climbs from its floor to the threshold and starts again, so where it compacts sets the
average, and how fast it grows sets how often it compacts.

What the table cannot show is whether a task takes more calls once a session has lost what it read before a
compaction and reads it again. Per unit of the survey's estimate (above), the three Sonnet ports from before the
change (Noah's Ark, Seven Seas, Atomica) look somewhat dearer than the later ones, not 40% dearer, so the saving per
task may be smaller than the saving per call; three small games are too few to tell.

A lower threshold gives back less each time. About 87K tokens of every call are fixed (tools, the app's MCP tools, the
system prompt, CLAUDE.md, skills: a game session's own count on 2026-11-01), and a summary adds about 20K, so a
session restarts at about 110K. Compacting at about 250K would bring the average call from about 234K to about 170K:
roughly 10% per call after the extra compactions, before any reading again, which grows as compactions come more
often. 400K stays unless usage gets tight.

## Calls (measured 2026-11-01)

The game sessions' 33,775 main-session calls from 2026-10-25 to 2026-11-01: 89% made exactly one tool call and 8% two
or more. About 41% of all calls were a single read or search (a `sed -n` range, grep, `cat`, Read), and 79% of those
came right after another read (59% in runs of three or more). Edits and scripts were about 23% of calls, game and tool
runs 7%, builds and suites 5.5%, git 1.5%; calls right after a failed tool call and calls that woke a session (a
notification or a message) were about 1% each. Batching the reads a session already knows it needs costs nothing
extra, since the same text enters the context either way, and if a third to a half of the reads in runs were batched,
about 11-16% of the calls would go. CLAUDE.md asks for it from 2026-11-01, as a trial: measure the same mix again
once sessions started after that date have done a week of work.

Edits: 6,126 file edits in the same calls. Inline Python failed 8% of the time (2,202 edits), heredocs into a file 8%
(353), `sed -i` 4% (827), the Edit tool 1% (1,276) and Write none (1,468); a failure is an error result or a failure in
the output, such as an `assert count == 1` that stopped a replacement. Against the Edit tool's rates that is about 200
retries and 150 more checking reads, about 1% of the calls. What the count cannot see is a silent fault: Git Bash
heredocs collapse doubled backslashes. Sessions seldom use the Edit tool because it needs a Read of the file first and
they read with `sed -n`, and because the auto mode's own instructions suggest sed, heredocs and short scripts for small
edits. The user decided against a rule for it (2026-11-01): it would not be followed consistently.
