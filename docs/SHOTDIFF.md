# shotdiff: comparing a port's screenshots with the original's

Before a port's final merge its session compares the game with the original screen by screen (CLAUDE.md, "Done means
identical"; `docs/ORIGRUN.md` for how the original is run and captured). Looking at whole screenshots costs many tokens, and
most of a screenshot is identical. `tools/shotdiff` pairs the original's captures with the port's, finds the pixels that
differ, groups them into boxes, and writes a small crop of each box for a session to look at, and a static page on which
the user judges what needs a person. Everything it writes goes under `build/shotreview/<game>/<run>/`, which is git-ignored:
the pictures come from the user's games and are never committed or published.

## The step in a game session

Capture the original with origrun (`shot <name>` or `waitshot`, which write `build/origrun/<run>/shots/<name>.png`) and the
port at the same points, into a folder of its own; name the port's files like the original's, or put the pairs in a mapping
file. Then run `python tools/shotdiff/shotdiff.py --game <game> --orig <original's folder> --port <port's folder> --run <id>`.
It prints one line with the counts and the page's path, and takes seconds. The run id names the comparison (default
`latest`); a new id keeps the old run beside it, and the all-games page shows the newest run of each game.

Read `build/shotreview/<game>/<run>/summary.json`, not the page. It lists every pair with its status (`match`, `differ` or
`error`), the percentage of differing pixels and its boxes, each with its position, size, pixel count and the path of its
crop. A crop is one small image: the original on the left and the port on the right, the region around the box enlarged
twice without smoothing, the box outlined in both. Look only at the crops of the boxes that are flagged, never at the
whole screenshots, and compare what you see with the decompilation. When a pair differs because the port is wrong, fix the
port and run again. A pair that matches needs no look at all.

Never wait for the user's marks. A session fixes every difference it can judge from the decompilation and the crops by itself.
A pair that truly needs a person's judgement (a matter of taste, a sound or timing you cannot tell from a still, a difference
you cannot explain) stays unmarked on the page for the user's later review, and the session lists it in its notes as
"awaiting the user's review", with the pair's name and the run. The user reviews once the page holds the screenshots of all
the games, not game by game. When marks exist, read `build/shotreview/<game>/<run>/marks.json`: each pair has `mark` (`issue`:
a real difference, fix it; `fine`: accept it; `unsure`: the user could not tell, so it stays open), a `note`, and `at`, the
time of the mark. A mark carries over to a newer run of the game for as long as the pair still differs; a mark whose
`sig` differs from the pair's current boxes was made against other differences, and the page says so.

## The comparison, and what it never hides

Each picture is first cut to the game's client area: origrun's window captures include the window frame (646x509 for a 640x480
game), and shotdiff cuts the frame away on its own when that is exactly what makes the two sizes differ; otherwise the game's
config gives the crop. A pixel differs when any colour channel differs by more than the tolerance (default 8, `--tolerance`).
The port is then lined up with the original by searching shifts of up to 8 pixels (`--max-shift`) and taking one only when it
clearly beats no shift; a pair that needed one says so on the page and in `summary.json` (`shift`), and a shift that is not
the capture offset is itself a finding. `--align DX,DY` or `"align"` in the config fixes the offset instead, with no search.
The differing pixels are dilated, labelled, and the nearby boxes merged (`--merge`, `--dilate`, `--max-boxes`); no pixel is
dropped, only grouped.

Nothing is hidden except what an ignore rectangle in the game's config covers, and the page lists the rectangles in use. A
pair of different sizes (after the crop) is reported as differing and not compared, because comparing mismatched
pictures says nothing; fix the crop. `--allow N` lets a pair with up to N differing pixels count as matching; it is off by
default, and the page says when it is on.

A game's settings go in `games/<game>/shotdiff.json` (found by the game name, or `--config`), all keys optional:

    {"client": [640, 480],
     "orig": {"crop": [3, 26, 640, 480]}, "port": {"offset": [0, 0]},
     "ignore": [[x, y, w, h], {"rect": [x, y, w, h], "pairs": "bubbles_*", "label": "Water Bugs' bubbles"}],
     "tolerance": 8, "maxShift": 8, "names": {"orig.png": "port.png"},
     "clips": {"window": 3, "drift": 2, "stallMin": 5}}

`crop` is x, y, width, height of the client area in that side's pictures, `offset` is only x, y. Ignore rectangles are in client
coordinates; give one only for an area that animates on its own and cannot be made still, and say why in `label`. `--map file`
(or `names`) pairs files whose names differ: lines of `original.png = port.png`, or a JSON object.

## Visible and faint

Many differing pixels are ones no one can see: the edges of sprites drawn through a transform, sampled a fraction of a pixel
differently, or art decoded by another JPEG 2000 library, a few levels apart. On the first pilot (Bookworm Adventures,
2026-11-01) they were spread over almost every screen and merged into boxes covering most of it, and the user found nothing
to mark in them. So each box is visible or faint, and only visible boxes are for review.

A pixel is visibly different when, after a 3x3 box blur of both pictures, it still differs by 32 or more in some channel. A
box is visible when it holds at least 32 such pixels; it is drawn around those pixels and grown over the differing pieces it
touches, so it outlines the whole change. Every other differing pixel goes into faint boxes. A pair with visible boxes is
"differ"; one with only faint boxes is "faint"; a clip frame that differs only faintly from its original frame counts as
matching it (and is listed as a faint frame).

The thresholds come from the pilot's 153 boxes. Under the blur, antialiased and sub-pixel-sampled edges (Lex's outline, the
credits' pillars, the map's rotating rays) left 0 to 21 pixels a box, and a changed digit of thin text 158. Some boxes sit
close to the line on purpose: a sprite's highlight sampled one pixel over (42 pixels) stays visible, because flipping between
the pictures does show it. Allowing a one-pixel shift for each pixel as well (`VISIBLE_SHIFT`) was tried and dropped: it also
forgave the changed digit (13 pixels left, against 10 or 11 for edge noise). The selftest holds the rule: a disc antialiased
two ways is faint; a changed glyph and a missing sprite are visible.

A whole box moved by one pixel is another matter (the user, 2026-11-01): when moving the port by one pixel in a single
direction (right, left, down or up, the same move for the whole box) leaves fewer than 32 visibly different pixels in the
box, and at most a quarter of those it had without the move, the box is faint with `"shifted": [dx, dy]` (the port's
pixel at x+dx, y+dy is the original's at x, y). The second condition keeps a small change that a move merely cuts in half
(the selftest's 8 against 3 left 36 pixels, 30 after a move) from passing for a shift. A shifted box is still a deviation
to look into later: the faint list says "shifted 1 px" and which way, and `counts.shifted_boxes` counts them. A changed
digit stays visible under every move: the selftest checks a 5 against a 4 and against a 1, with strokes 3 and 2 pixels
wide (the hall of fame's time, 14:50 against 14:40), and a sharp striped sprite one pixel to the right, which is shifted.

## Review items

The same difference often shows in many shots: a locked book on every menu screen, the background behind every battle
shot. Each would be a box of its own pair, and the first page of boxes was the same thing over and over. So the visible boxes
of the stills are grouped into review items. Two boxes are the same item when they meet and at least half
(`SAME_OVERLAP`) of the visibly different pixels of the smaller differ visibly at the same place (or a pixel away) in the
other's pair. Where the differing pixels lie decides, not how bright they are (the user, 2026-11-01): a difference behind a
dimmed screen (a dialog, the focus lost, an intro's fade), in front of a sprite that moves, or split into two boxes in one
shot is still the same item. A box joins an item when it is the same as any box already in it. An item is shown once, by its largest box, with "seen in N shots", links
to the other shots and their crops in a collapsed list; a mark on it marks them all. Items seen in the most shots come
first, then the largest. `summary.json` has `items` (each with its `name`, `count`, the pair and box it is shown by, and
`seen`, the others), every grouped box has `item`, and the counts give `items`, `items_repeated` and `visible_boxes`. "Seen in N shots"
counts shots, not boxes. The differing stills are shown whole in a collapsed list after the review area, out of the queue,
since every box there is an item above; a link to one opens the list. Clips,
and stills whose sizes differ, are reviewed whole after the items, as before.

## The differing-pixels picture

Each box's crop has a third part and each differing still a fourth picture: the original dimmed to gray, with every
differing pixel in colour. Pixels of a visible box that differ visibly are bright magenta; every other differing pixel
(faint ones, and those of a visible box below the visible rule) is pale blue. It shows at a glance what the two pictures
leave to the eye: a solid magenta shape is a real change, a pale fringe is edge noise.

This is not a higher tolerance. Faint boxes stay in `summary.json` (`faint_boxes`, each with `pixels`, `max_delta`,
`strong_pixels` (off by 32 or more) and `visible_pixels`), the counts give `faint` and `faint_boxes`, and the page lists them
in a collapsed section at its end, outside the review queue, so a session still sees every difference.

## Clips

An animation is a folder of numbered frames on each side, at the same relative path (`orig/<clip>/0000.png ...` and
`port/<clip>/0000.png ...`), and a `clip.json` in the folder of either side that records the interval, `{"interval_ms": 100}`.
origrun's `record <prefix> <duration> <interval>` writes `<prefix>_000.png ...`; copy those into a clip folder. A folder with a
clip.json is a clip, and its frames are not compared as stills; its name in the report ends in a slash.

The original runs on the wall clock and the port by updates, so the frames do not line up exactly. For each port frame shotdiff
looks for an original frame within `window` frames either way (default 3) that agrees with it under the tolerance, nearest the
clip's overall offset first, and reports the offset that fits best overall ("port frame i shows what the original showed at
frame i+2"). A port frame that no original frame in the window agrees with is flagged and boxed like a still. A whole sequence
that drifts (its offset changes by `drift` frames or more over the clip, so the port runs ahead of or behind the original) or
stalls (one side shows the same picture for `stallMin` frames or more while the other moves) is reported in the clip's notes
and makes it differ even when every frame found a match. The summary has each clip's flagged frames with their boxes, and the
crops of the worst twelve flagged frames. On the page a clip is one animated image with the original on the left and the port
on the right, in step, then those crops, then a frame slider (and the arrow keys; `[` and `]` jump between flagged frames) over
the frames at full size, with the flagged frames marked on the slider. The animated image is scaled down for a large clip; the
frames are not. A clip takes the same marks as a still.

## The page and the marks

`python tools/shotdiff/serve.py` (the `shotreview` entry of `.claude/launch.json`) serves `build/shotreview/` at
`http://127.0.0.1:18770/` and saves the marks the pages post to `marks.json` beside each run's page. The root page lists every game with its latest run: pairs, matching, differing,
already marked, not yet marked, with the games that still have unmarked differences first; it is rebuilt each time shotdiff
writes a run (`shotdiff.py --index-only` rebuilds it alone). A run's page shows the review items (above), then the pairs
reviewed whole (stills of another size, clips); for each item the crop of its box (larger than the box, with its size and how
many of its pixels differ: original, port, differing pixels), the other shots it is seen in, then the whole pictures of the
shot it is shown by (original, port, the port with the boxes outlined, the differing pixels; a click shows one at full size,
and 1 to 4 switch between them). After them, collapsed, every differing still is shown whole, with links to its items. Buttons mark an
item Issue, Not sure or Fine, and there is a note field; marks are kept by item name (the pair, the box's place and size), and
a new run carries over the marks of items it still has. Keys: `j` and `k` move between items, `i`, `u` and `f` mark Issue,
Not sure and Fine, `n` writes a note, `z` shows the full size. Opened as a plain file, the page
keeps its marks in the browser and "Copy marks" copies a short text of them. The page works offline and in light and dark.

## Limits

The alignment search is a global shift of the whole picture; a part of the screen that moves on its own is a difference. A
capture that cannot be made at the same moment as the other (a screen with a timer, animated bubbles) needs an ignore
rectangle or, better, a state both sides can reach exactly. Captures from a game whose original draws outside GDI come out
black or blank (`docs/ORIGRUN.md`) and differ everywhere.
