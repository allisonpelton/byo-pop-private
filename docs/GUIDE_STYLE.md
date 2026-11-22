# Game guides: the style guide

The user, 2026-11-18: "I've decided I'm actually ok with AI-generated guides for the games that actually benefit from it on
the site, as long as they are clearly marked as AI-generated", and "eventually we'll work on a style guide to ensure they are
comprehensible to players and provide genuinely useful tips, not just basic gameplay info". This is that style guide, a draft
for the user to read and change before more guides are written. One guide follows it so far, Mummy Maze Deluxe's, as the
example to judge it by. The guides are the one place on the site where our own prose appears; everything else keeps the rule
of no AI prose (docs/WEB_PLAYER.md, "Values, not sentences").

## What a guide is for

A guide tells a player what the game does not: the exact rule behind something they can see, the numbers the game keeps to
itself, what a choice costs, and how to use the rules to their advantage. The measure is a player who has done the tutorial and
played for an hour: every tip should teach them something. What the game's own help, tutorial or tips already say is left out,
unless the guide adds the exact rule behind it.

## Which games get one

A game benefits when its rules run deeper than its help shows: hidden scores and bonuses, enemies that follow fixed rules a
player can learn, items and upgrades whose worth is not shown, things to unlock, modes that change the rules. A game whose depth
is all on the screen gets no guide. Which games come next is the user's choice. The ones to look at first, each needing its code
read before anyone decides, are the two Bookworm Adventures, Heavy Weapon, Venice, Bejeweled 2 and Twist, Typer Shark, AstroPop,
Hammer Heads and Big Money.

## Where the facts come from

Only from our own work: the port's code and the game's notes (`games/<game>/src` and `docs`), which follow our own export of
the game, and the port itself. Never another site's guide, FAQ, wiki, forum post or video, and no web search for a guide's
content, under the rule of no references and the rule on copyright both. Every fact is checked in the code before it is
written, and the guide's source names the function it comes from in a note the player does not see. A number is written as the
player meets it (pyramids counted from 1, seconds, points), with the code's own form in the note. What has not been checked is
left out: a guide never guesses, and never says "probably".

## Marking

The window draws an "AI-generated guide" banner at the head of every guide, with the date the guide was last checked against
the port's code, and its status line says AI-generated too. The banner belongs to the window, not to each guide's text, so no
guide can be shown without it.

## How a guide reads

Plain English in the site's spelling (colour, centre), in the second person and the present tense. Short sentences, one idea
each. The game's own names for its things, as the game shows them (the Ankh, the key stone, Abandon Hope), but never its help or
tip texts quoted. Numbers as digits; where a formula matters it is given once in words and worked out for an example. Nothing to
fill space ("In this guide", "Good luck"), no praise of the game, no exclamation marks, no emoji, no "simply" or "just", no
hedging, and no mention of the port, the code, the site or AI inside the text, which the banner covers.

## Its shape

Three to seven sections, each a heading and two to six tips. A tip opens with a short lead of two to six words, shown in bold
and ending in a colon, and goes on for one to three sentences: the rule first, then what to do with it. The sections follow the
order a player meets things: how a turn or a round works, then the pieces and enemies, then scoring, then the modes. A guide runs
to between 300 and 900 words.

## A tip that earns its place

Each tip passes three tests: the code says it is true, the game does not already say it, and a player can act on it. "Plan your
moves before you make them" fails the second and third. "A white mummy in your row with a wall between you does not move at all,
however long you stay" passes all three: it is in the code that chooses a mummy's step, the game never says it, and it tells the
player where they can stand safely.

## Spoilers

No solutions to particular levels, no endings, and nothing from `docs/HIDDEN_CONTENT.md`, unless the user asks for them.

## The format

The guides are in `web/guidetexts.js`, one entry for each game: the listing's id, the date the guide was last checked against
the code, and the text in a small markup. A line that starts `## ` is a section's heading; one that starts `- ` is a tip, its
lead the words before the first colon; any other line is a paragraph, and lines next to each other join; a line that starts
`// ` is a note on where the facts come from, not shown; a blank line ends a paragraph or tip. The window (`web/guides.js`)
shows the text as text only, so no guide can hold a link or a picture. `tools/guidestest.js` (unit-web) checks each guide's
shape: its sections and tips within the counts above, its length, a source note in every section, every lead ending in a colon,
and none of the words this guide rules out.

## Before a guide is merged

The user reads each new guide, and the merge request names it, so that nothing written by a session reaches the page unread.
