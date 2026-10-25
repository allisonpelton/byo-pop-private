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
- Tests and builds print one summary line each and write their logs to files.
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
