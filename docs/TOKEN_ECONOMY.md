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
330M for BookWorm: two and a half to three times fewer, before Sonnet's lower prices. Part of that is the game, which
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

Not yet measured: how faithful the port is. Its tests are its own bot games, which show that it plays and is
deterministic, not that it plays like the original; no one has yet compared its translated functions with the
decompiled originals.
