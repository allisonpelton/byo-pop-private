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
| Fable 5.2 | $8 | $0.20 | $10 | $16 | $40 |
| Opus 5.5 | $4 | $0.20 | $5 | $8 | $20 |
| Sonnet 5.5 | $2 | $0.20 | $2.50 | $4 | $10 |

On Opus 5.5 a token that misses the cache costs as much as 25 (5-minute) or 40 (1-hour) tokens that hit
it. Main sessions get the 1-hour cache; subagents and workflow agents mostly the 5-minute one.

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
came in over: Atomica, whose session ran to the full context window before Sonnet got the 400K auto-compact,
and Cosmic Bugs, whose engine the survey did not know and guessed at ~2x. Sequels on a shared engine were estimated
well: Volume 2 (4-6x after a 10-15x first game) cost 36% of Bookworm Adventures' port, and Water Bugs 36% of Cosmic
Bugs'.

**The process mattered more than the model.** The three ports from before the rules, with subagents and many short
calls, cost $114-251 per unit; Bejeweled's includes setting up the repository and the framework. Every port since has
cost between a sixth and a third as much per unit. Sonnet ports cost about the same per unit as Opus ports in API
dollars: cache reads are most of every session's cost and are priced the same for both models, so Sonnet saves only on
output and cache writes, and its sessions carried larger contexts. Whether it weighs less against the plan's limits is
not known, since those have no published formula. Its ports also need the Opus review (COORDINATION.md, "Model
selection").

**What the estimates leave out.** They are for a first playable port. Since 2026-10-27 a game is done only when it plays
identically to its original (CLAUDE.md, "Done means identical"), and the survey has nothing for that pass. On the
games' side its first costs are small (from 2026-10-26 to 2026-11-01: Bookworm Adventures $86, Seven Seas $20, Water
Bugs $11), while its tools (origrun, shotdiff, the originals' 3D mode) count as shared. A rule for it waits until more
games have finished the pass.

**A large game's remaining work** is estimated from this rule and the survey, at the week's dollars per percentage
point of the limit, and never from a session's own guess in days. Bejeweled Twist's session put its work left at 14 to
22 days on the evening of 2026-11-01; it took about a day and a half. Per unit of the survey's 25x, Twist's port, its 3D
mode and both phases of its identical-play pass together cost about $43, what Bookworm Adventures paid per unit for its
port alone.

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

A call now costs about 40% less, and the extra compactions (about $0.40-0.60 each) cost a small part of that saving.
About 87K tokens of every call are fixed (tools, the app's MCP tools, the system prompt, CLAUDE.md, skills), and a
summary adds about 20K, so a session restarts at about 110K. Compacting at about 250K would save roughly another 10% per
call, before any reading again, which grows as compactions come more often. 400K stays unless usage gets tight.

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
