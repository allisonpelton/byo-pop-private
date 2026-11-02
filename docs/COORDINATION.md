# Coordination: the coordinator's rules

The user's rules for planning and running the work across sessions, which only the coordinator session acts on. They
moved here from CLAUDE.md on 2026-11-01, word for word, to keep CLAUDE.md to what every session needs (the user:
CLAUDE.md was "larger than I typically like"). CLAUDE.md keeps the rules every session follows and points here.

## Usage

- **Weekly cap (the user, 2026-10-26):** when the account's weekly usage (all models) reaches 85%, the coordinator
  starts no new game session and sends no "Begin"; it checks with `mcp__ccd_session_mgmt__get_usage` before each one.
  Game sessions already running carry on and do not watch the usage. The coordinator keeps merging their milestones
  and keeps the local test site ready: `build/web` built from `main`, with every finished game importing and playable.
  For the week of 2026-11-01 to 2026-11-08 the cap is 80% (the user): what remains goes to bugs that make games harder
  to test (like Seven Seas' tutorial that would not go away) and to site work. Site work runs alongside the game
  sessions (the user, 2026-11-01).
- **Effort (the user, 2026-10-27, a trial):** game sessions run at medium effort to save usage; the coordinator sets
  it on every session it starts.

## Sessions

- **Directing sessions (the user, 2026-10-24):** the user tells the game sessions about game-specific changes;
  the coordinator does not direct or check on them. A game session messages the coordinator when it needs it:
  a shared change, a milestone ready to merge, or its mods finished. The coordinator
  looks at a session's work only when the user asks, or when it breaks something for other sessions (shared
  code, the trunk, the machine's test rules). It does message the sessions when the way they are instructed
  changes (a new rule like this one), since a running session reads CLAUDE.md again only at its next start
  or compaction.
- **Model selection (the user's rule, decided after the Sonnet trial on 2026-10-25):** a game whose port can
  largely copy another game's (the same framework generation and engine: Noah's Ark's for Seven Seas and Atomica,
  BookWorm's for Pixelus and Typer Shark, Big Money's for Hammer Heads, Bejeweled's for Bejeweled 2, a sequel after
  its first game) runs on Sonnet. A game that needs an engine or framework layer no port has yet (Bookworm
  Adventures' PopLua and PopAnim, the Sprout engines, the non-SAF engines, Chuzzle, Twist, Bejeweled 3, Zuma's
  Revenge) runs on Opus. Opus checks Sonnet where it showed deficits: before merging a Sonnet port, the coordinator
  compares about ten translated functions with the decompilation, weighted to the largest and the most
  loop-heavy, and sends the fixes back. A Sonnet session translates loops in their original shape (a
  test-then-increment loop does not become a for loop), checks what a comment claims about the original before
  writing it, and builds with AddressSanitizer at the first sign of a crash or memory corruption. If a Sonnet
  game's review finds many faults, or its port runs far over its estimate, games of its kind move to Opus.

## Publishing

- Before each publish the coordinator checks the trunk for game files and game text.

## Which games, in what order

- **Order of work (the user, 2026-10-24; updated 2026-10-25):** the measured one-session experiment (Noah's Ark,
  `docs/TOKEN_ECONOMY.md`) showed an improvement, so up to three decompilation sessions run at once, besides
  Bejeweled's mods session. The order of games is the survey's decision of 2026-10-24
  (`docs/POPCAP_GAMES_SURVEY.md` section 4): the BookWorm series, then Bejeweled Twist, then the least predicted
  effort first. A slot that the next game in that order cannot use yet, because it waits on another game's
  engine, goes to the cheapest remaining game. New game sessions list the mods worth making but make none without
  the user's approval, and no further game gets mods until every PopCap game works in the browser (Noah's Ark's
  mods were the one exception, part of the Sonnet trial).
- **The merged games first (the user, 2026-10-27):** the 18 games merged by 2026-10-27 are not done until they pass
  the identical-play comparison (CLAUDE.md, "Done means identical"); from the weekly reset (2026-11-01) they come
  first, and no new port starts until all 18 are done.
- **Triage and new ports (the user, 2026-11-01):** "If we have to triage, Twist before identical gameplay on existing
  decomps. No new decomps until all existing ones play identically, except ~trivial ones (the other Mahjong game is the
  only one, I'm guessing)." So when usage runs short, Bejeweled Twist's port comes before the identical-play passes.
  The existing decompilations are every game ported so far, Bonnie's Bookstore and Mahjong Escape: Ancient Japan
  included. No new port starts until all of them play identically. The one exception is a near-trivial port: so far
  that means only Mahjong Escape: Ancient China, which is Ancient Japan's code with other resources. Ancient Japan's
  own session ports it once Ancient Japan plays identically, so it copies the finished engine instead of one that is
  still changing (the user, 2026-11-01).
- **The ports still to make (the user, 2026-11-01):** they wait until every current game is fully done, both phases of
  identical play included. Then: "anything easier than Chuzzle can run in parallel, and anything harder has to be done
  in sequence AFTER all easier games have been finished." By the survey's estimates (`docs/POPCAP_GAMES_SURVEY.md`
  section 4), the easier ones, run side by side as before, are Venice, Heavy Weapon, Hammer Heads, Bejeweled 2,
  AstroPop, Typer Shark, and Pizza Frenzy with Iggle Pop and Feeding Frenzy after it (4x to 8x). Chuzzle (~8-12x) itself
  starts the sequence, then in the survey's order Feeding Frenzy 2 (~10x), Peggle Deluxe (~12x) and Peggle Nights after
  it, Bejeweled 3 (~30x), Banana Bugs, Gyromancer, Zuma's Revenge (~35x) and Talismania (~12-16x after Feeding Frenzy
  2), one session at a time. This moves Typer Shark, which the decision of 2026-10-24 put at the end, into the easier
  group.
- **No new identical-play passes until Twist is done (the user, 2026-11-01):** asked when Twist was a little over half
  ported (they had hoped for more than 70%): "Multiple can run at once, just no more until Twist finishes." The passes
  already running carry on side by side. No other game's pass starts, even when one of them finishes, until Bejeweled
  Twist's port is done, 3D mode included: when Twist estimated 6 to 10 days for 3D, the user chose to wait for all of
  it rather than start after the 2D game ("Wait for all", 2026-11-01). The one exception is Bejeweled Deluxe, whose pass
  the user started at once ("Ok to get Bejeweled running identically now, I decided", 2026-11-01).
- **Plays identical first, for every game (the user, 2026-11-01):** identical play is two phases, "plays identical"
  and "looks identical" (CLAUDE.md). "Current games can still do both, but for remaining ones to review get all of them
  playing identical first." So the passes running on 2026-11-01 (Bejeweled, Bonnie's, Bookworm Adventures, and Twist's
  when it comes) do both phases. Every game not yet reviewed gets the first phase, and their second phases start only
  once every game plays identical. "If a series of games play out the same in the original and port then the first
  phase should be considered done." Those first phases start only when the usage allows (the user, 2026-11-01): "Only
  launch other games if Twist is predicted to finish, down to looking identical, with weekly usage under 80%." So before
  starting one, the coordinator reads the weekly usage (`get_usage`) and its rate, and starts it only if Twist's
  remaining work, both phases of its pass included, still fits under 80% in every week it needs.
- **Order of the work on the merged games (the user, 2026-11-01):** "once all issues I mentioned are fixed, move on down
  through the game list. Once all games are done, then we can do code quality/readability, and finally figure out what
  code does not need to be present in the decomps (mostly registration, but other things are likely to turn up as
  well)." So first every fault the user reported, in every game, and anything that makes a game hard to test; then the
  identical-play pass above, game by game; once every game is done, a readability pass to the project's code standards
  (`docs/CODE_STANDARDS.md`, written and approved by the user first); last, taking out code the ports do not need, such
  as all registration code (trial, nag and unlock paths: "It is misleading and adds unnecessary bloat"). The game's
  regression check proves each step leaves play unchanged. All of it comes before the website is ready. So does one more
  step once every decompilation is done (the user, 2026-11-01): each game ported only from its Steam copy is compared
  with its version on the PopCap Game Collection disc (the user's ISO in their Downloads folder), and its build must
  work with that version's files too. Planned alongside the website (the user, 2026-11-01): an installer that puts a
  native port beside its Steam original, started by a launch option, SMAPI-style (`docs/STEAM_LAUNCH.md`).
- **The site's requested work comes between Twist and the passes (the user, 2026-11-02):** "Slot in the remaining
  requested additions and changes to the site after finishing Twist and before the passes on existing decomps." Then:
  "Any site features that are easy, you can do now. Anything that requires designing an animation or SVG, or that
  requires a lot of code to get working right, save for later." The open items are in the "Site" section of
  `testnotes/TEST_NOTES.md`. Done now, in a site session: F11 for full screen in every game; play time, last day played
  and number of plays on the front page, with "Play now" and "Add your files"; a bigger logo on the front page;
  "Quit Game" back to the home page; the player, save and settings managers as tabs with far less text (names until the
  icons are designed). After Twist and before any identical-play pass starts on a game decompiled before Twist (passes
  already running carry on): adding many games' files at once with the game inferred from the folder; a game folder
  dropped on the logo or on a game's tile; bottle-cap placeholder icons; the bottle cap and opener animation; the recycle
  bin; the managers' tab icons. Ning Po Mahjong's and TipTop's high-score names were fixed (34e6da1, a6547b9). The chat
  is built as a test with made-up messages; its real version waits for funding. What the user asks for while testing is
  still done when asked.
- **Which games, and where they are published (the user, 2026-10-26):** every game may be decompiled except Plants vs.
  Zombies, Insaniquarium and Zuma Deluxe, whose fan communities we leave to their work. A game with another PC decompilation in
  progress or an active modding community is built but unpublished: its folder goes into `.publishignore` and it stays
  off the website. The "Published" column of `docs/POPCAP_GAMES_SURVEY.md` section 4 says which; "web only" (Zuma's
  Revenge, the user 2026-10-26) keeps the folder in `.publishignore` but puts the game on the website.

## Mods

- **No new mods unless requested (the user, 2026-10-30):** "no new mods for anything unless someone requests it." There
  is no mod phase that goes through a list: the 29 standard mods in `docs/MOD_ROADMAP.md` and each game's listed ideas
  become the first entries of a feedback page the website will have for each game, where players suggest mods and vote
  on them, and the user decides which to make. Fixes and suggestions for the mods that already exist are noted (the user's
  `testnotes/TEST_NOTES.md`) and wait until every decompilation is finished.

## History

- **The review page (the user, 2026-11-01):** the pilot on Bookworm Adventures settled who judges shotdiff's flagged
  differences ("Basically everything appears to be a timing difference, so this tool still isn't usable"; then "Yes,
  switch"): the session judges every one itself (CLAUDE.md, "Done means identical").
