// The music of each game, for the listing page's music player (web/music.js; docs/WEB_PLAYER.md, "The music player"), by the
// game's id. Paths are in the game's folder as the import writes it, matched without regard to case, where * stands for any part
// of a file name.
//   files   the files that are tunes: a module's subsongs are its tunes, an Ogg Vorbis or WAV file is one tune
//   parts   for a module the game plays in ranges of its orders: {"<file name>": [[first order, last order], ...]}, its tunes
//           instead of its subsongs
//   joined  tunes the game joins from pieces by a playlist: [{playlist: "<path>", piece: "<path with {} for the playlist's word>"}],
//           or from files in order: [{id: "<name>", pieces: ["<path>", ...], loopFrom: <the piece its loop goes back to>}]
//   cuts    {"<file name>#<n>": [order, ...]}: a subsong split where the game's code starts a different tune partway
//           through, or where one song of a medley ends and the next begins; each later piece is "<file name>#<n>@<order>"
//           (an order the subsong never reaches: from it to the end)
//   starts  {"<file name>#<n>": [order, ...]}: orders the game also starts a subsong at, where the tune before plays on
//           through them in the game (no seam: a long game's music runs into the next part); each is a piece
//           "<file name>#<n>@<order>" from there to the end, and the subsong stays whole
//   chains  {"<file name>#<n>": ["<file name>#<m>", ...]}: parts the game only plays as jumps inside one tune, one track
//   skip    ["<file name>#<n>", ...]: tunes that are not tunes (n counts from 0 in the file's subsongs or parts)
//   names   {"<file name>#<n>" or "<playlist name>": "<name>"}: plain names, never invented titles and never the game's own text
//           at length; without one a tune is named after its file
//   titles  {"<file name>#<n>": <title>}: a tune's own title, shown in quotes beside its name: "<title>" by the user's word
//           (cited at the entry), or one the module's own text holds, read from the game's file as it plays and never written
//           here: {own: true} the module's title, {line: "<order>"} what its message says beside that order (as the message
//           writes it), up to a comma or a bracket, {line: "<order>", inner: true} the words in that line's brackets;
//           {luc: "<script>", list: "<string>", data: <n>} a string of the game's Lua script (Bookworm Adventures' music player
//           list): in the constants of the function holding the string list, the one just before the whole number data, or with
//           after: <k> the k-th string after it
//   alone   ["<file name>#<n>", ...]: subsongs that are one pattern running on into the module's silence: listed with the unused
//           patterns as that pattern, played alone (its when still its tooltip)
//   inside  tunes kept inside a game's file: [{file: "<path>", magic: "<first bytes>"}] its resources (a Windows module's)
//           that start with those bytes, the magic naming their format, or [{file: "<path>", zip: ["<entry>", ...]}] those
//           entries of a zip archive, or [{file: "<path>", tiptop: ["<entry>", ...]}] those of TipTop's archive; each is then a
//           file as above
//   bank    for a game whose music is MIDI: the General MIDI bank its port plays it with, served beside the pages (a .mid file
//           is one tune, rendered by the game's own sequencer into TinySoundFont: web/music/midi_web.c)
//   where   {"<file name>#<n>", "<file name>" (an Ogg or WAV file) or "<playlist name>": "<place>"}: where in the game its code plays
//           the tune, in the style of a game music rip ("Main Menu", "Danger 2": Title Case, one place, at most 20 characters,
//           tunes of one place numbered as the game reaches them), from the game's code (its session's answer) or, for an Ogg
//           game, its file's name; shown beside the tune and in the display
//   unused  ["<file name>#<n>", ...]: tunes the game's code never plays (left in its files), shown as such
//   shuffle {out: [ids], groups: [[ids in order], ...]}: the player's shuffle (the user, 2026-11-19): out the tunes that make no
//           sense on shuffle (an intro, a loading or logo tune) beyond what it leaves out by itself (unused tunes and patterns,
//           and tunes under 20 seconds: stings, fanfares, jingles), groups the tunes the game only ever plays one straight
//           after the other, drawn as one and played in that order
//   order   ["<file name>#<n>" ...]: the tunes in the order a player hears them, from launch through a game to its end (modes,
//           levels and books in the order the game reaches them), as the list shows them (the user, 2026-11-13: "more closely
//           follow what you might hear instead of having the parts in order"); tunes it leaves out follow, then the unused ones
//   when    {"<file name>#<n>" ...: "<when>"}: exactly when the game's code plays it, as numbers (levels, orders, updates), the
//           place's tooltip
//   composer  who wrote the music, by the game's own credits (its credits screen, text or exe; a company where they name no
//           person), or where they name nobody by the user's word (2026-11-19, cited at the entry); none shown as Unknown. One
//           form for a person across the site: "Peter Hajba (Skaven)", "Philippe Charron". composers {"<id>": "<name>"} where
//           the music is split by tune (Heavy Weapon's love theme); an id without "#<n>" takes every tune of that file
//   voices  true for a game that plays its tunes as sounds, not as the mixer's songs (Venice): Music follows the one playing
//           among its voices (web/music.js, voiceTune)
//   level   the level the game plays its modules at, where it is not the renderer's own (platform/MusicLevels.h):
//           {bassAmplify: n} the BASS amplify it sets (0 to 100), {fmod3XM: true} FMOD 3's XM pre-amp. The renderer is the
//           game's own (libopenmpt for MO3 and OXM, libxmp-lite for the rest: web/music/openmpt_web.c)
// What the game does at a tune's end, for the player's Rip Style, Themes Only and series (the user, 2026-11-22; docs/WEB_PLAYER.md,
// "Rip style, themes and series"), from the game's code and notes, never from the sound alone. Every tune the list shows (not
// unused, not one of the patterns played alone) is in exactly one of loops, once and next (tools/musicfieldstest.js):
//   loops   ["<id>", ...]: the game goes round it, by the module's own loop or from its start, until something else starts
//   once    ["<id>", ...]: the game plays it through once and then nothing, until something else starts music
//   next    {"<id>": "<id>"}: the game goes straight on into the other tune when this one ends (an intro into its loop, a piece
//           into the next piece, a fanfare into the tune it runs on into)
//   stings  ["<id>", ...]: the one-offs, game-over tunes, level-complete stings, fanfares and results, which Themes Only leaves
//           out unless it has a next (then it plays with what it leads into)
//   gameOver {"<id>": {to: "<id>", how: "cut" | "fade" | "cross", ms: <n>}}: where the game itself goes from this looping tune to
//           its game-over tune, and how: the tune stopped and the other started (cut), faded out over ms and then the other
//           (fade), or the other faded in as this one fades out, both over ms (cross: the framework's FadeOut and FadeIn); ms
//           from the game's own timing (its fade's step per update, an update 10 ms, Heavy Weapon's 5 ms)
//   series  {"<name>": ["<id>", ...]}: tunes the game plays in turn as one set (a mode's pieces, one after the other; a level's
//           tune stepping through a fixed list), in the order it plays them, the first again after the last; the name in the
//           style of where's places
// Tunes under two seconds are left out (the silences and stings between tunes). Every game's tunes were listed from the user's
// copies on 2026-11-07.
var ByopopSoundtracks = {
  // Places and unused tunes from each port's code (the coordinator's reading, 2026-11-11); a place only inferred is left blank
  alchemy: {
    // the user, 2026-11-19: "Seven Seas credit goes to Peter Hajba as well as the original Bejeweled and Alchemy" (Alchemy's
    // In-Game tune, "He Has No Face", is one of his own works from before the game: the user, 2026-11-16)
    composer: "Peter Hajba (Skaven)",
    files: ["music/*.s3m"],
    where: { "alch_intro#0": "Title Screen", "1985-hehasnoface#0": "In-Game" },
    // (each module's own title, read from the file as it plays)
    titles: { "alch_intro#0": { own: true }, "1985-hehasnoface#0": { own: true } },
    order: ["alch_intro#0", "1985-hehasnoface#0"],
    when: {
      "alch_intro#0": "Order 0 from launch, over the loading title screen; fades out (0.004) when loading completes",
      "1985-hehasnoface#0": "Fades in (0.002) when loading completes; loops through the menus and every game; no other tune"
    },
    unused: ["1985-hehasnoface#1"],
    loops: ["alch_intro#0", "1985-hehasnoface#0"],
  },
  // AstroPop's one module holds every tune, which the game starts at an order of it (PlaySong); its subsongs stand for them,
  // each starting at one of the orders the game plays (0 loading, 5 the menus, 9 a level, 35 the danger siren, 39 the game's
  // end, 41 a level's tally and level-up: games/astropop/src, checked 2026-11-11 once its music played)
  astropop: {
    // its credits spell it "Phillipe Charon"; the site has one form for him, the user's (2026-11-19, Twist's entry)
    composer: "Philippe Charron",
    files: ["music/*.mo3"],
    shuffle: { out: ["music#0"] },   // (the loading tune)
    where: { "music#0": "Loading", "music#1": "Main Menu", "music#2": "In-Game", "music#3": "Danger", "music#4": "Game Over",
      "music#5": "Level Complete" },
    order: ["music#0", "music#1", "music#2", "music#3", "music#5", "music#4"],
    when: {
      "music#0": "Order 0; at launch, during loading",
      "music#1": "Order 5; main menu, pilot choice screen, and a level's story page (cross-fade 0.05)",
      "music#2": "Order 9 (10 if 35 or 41 plays); a board's start, after a story page, when the siren stops",
      "music#3": "Order 35; when a brick reaches the danger line (siren starts); back to 9 when the siren ends",
      "music#4": "Order 39; when the game ends (BoardEnd)",
      "music#5": "Order 41; a level is completed, from the level-up text through the tally"
    },
    loops: ["music#0", "music#1", "music#2", "music#3", "music#4", "music#5"],
    stings: ["music#5", "music#4"],
    gameOver: { "music#2": { to: "music#4", how: "cross", ms: 200 }, "music#3": { to: "music#4", how: "cross", ms: 200 } },
  },
  atomica: {
    composer: "Hannu Salonen",
    files: ["music/*.it"],
    shuffle: { out: ["atomica#2"] },   // (the title screen's intro, which its message names one)
    where: { "atomica#2": "Title Screen", "atomica#3": "Level Theme 1", "atomica#5": "Level Theme 2", "atomica#7": "Main Menu", "atomica#9": "Level Theme 3",
      "atomica#8": "Game Over 1", "atomica#10": "Game Over 2" },
    order: ["atomica#2", "atomica#7", "atomica#3", "atomica#5", "atomica#9", "atomica#8", "atomica#10"],
    when: {
      "atomica#2": "Order 6; title screen, until it closes",
      "atomica#3": "Order 12; Strategy mode's board start, and levels with level%5==1 (not level 1)",
      "atomica#5": "Order 24; levels with level%5==0",
      "atomica#7": "Order 31; main menu (not after a game), and the Play button (button 6) pressed in it",
      "atomica#9": "Order 44; Action mode's board start",
      "atomica#8": "Order 41; the main menu shown after a game over",
      "atomica#10": "Order 66; 300 updates into the board's game-over sequence"
    },
    unused: ["atomica#1", "atomica#4", "atomica#6"],
    loops: ["atomica#2", "atomica#7", "atomica#3", "atomica#5", "atomica#9", "atomica#8", "atomica#10"],
    stings: ["atomica#8", "atomica#10"],
    gameOver: { "atomica#3": { to: "atomica#10", how: "cut" }, "atomica#5": { to: "atomica#10", how: "cut" },
      "atomica#9": { to: "atomica#10", how: "cut" } },
  },
  bejeweled: {
    // the user, 2026-11-16: "Bejeweled's composer is listed as unknown. It is rather famously Skaven."; and 2026-11-19
    composer: "Peter Hajba (Skaven)",
    files: ["music/*.s3m"],
    where: { "1980-Autonomus#0": "Title Screen", "1984-Network#0": "Normal Mode", "1981-DataJack#0": "Timed Mode" },
    // (each module's own title, read from the file as it plays)
    titles: { "1980-Autonomus#0": { own: true }, "1984-Network#0": { own: true }, "1981-DataJack#0": { own: true } },
    order: ["1980-Autonomus#0", "1984-Network#0", "1981-DataJack#0"],
    when: {
      "1980-Autonomus#0": "From launch on the title screen; fades out (0.004) when it finishes",
      "1984-Network#0": "A new game in Normal mode; loops; fades the Timed tune out, fades in at 0.002",
      "1981-DataJack#0": "A new game in Timed mode; loops; fades the Normal tune out, fades in at 0.002"
    },
    unused: ["1981-DataJack#1", "1981-DataJack#2", "1981-DataJack#3"],
    loops: ["1980-Autonomus#0", "1984-Network#0", "1981-DataJack#0"],
  },
  // one module, BeyondNetwork.mo3: the game plays it from a starting order for each screen (properties/music.xml), so its
  // subsongs are the tunes. A game's music is subsong 1, a medley of fourteen songs that plays straight through and back to
  // its start (order 181 jumps to order 2): the board starts it at order 2 (MUSICOFFSET_MAIN) unless the mode is an Action or
  // a Puzzle one (modes 1, 2, 5, 6 and 7: Action, Puzzle, Hyper, Cognito and Finity), which start at order 122
  // (MUSICOFFSET_ACTION; BoardUpdate.cpp at board update 2, BoardCtor.cpp's mode flags), and nothing else moves it, at a
  // level or anywhere. It is cut at the composer's own markers between its songs (the "+++" orders 10, 21, 29, ... 168,
  // which play skips), so a piece ends where its song does. The titles are the user's (2026-11-19), in their order, and the
  // composer's album, which the user pointed to (2026-11-22: "I'd look up the track names on Skaven's Bandcamp"), names
  // the first piece, before the marker at order 10, Beyond the Network: the album's lengths agree with the pieces' (The
  // Journey Begins 1:44 there and 1:45 from order 11 here, and nine more within a second; the rest differ by 3 to 18
  // seconds, Beyond the Network by 7). Both of the first two are Classic Mode 1, the start of a game. They are not a
  // shuffle group: each is a song of its own on the album, ended at a marker as every other is, and only the second
  // intro, which runs into its song, is grouped. Subsongs 3 and 4 are one pattern each (127 at order 199, 128 at
  // 201, music.xml's GET_READY and LEVEL_CLEAR, which the code never reads), then the module's silent pattern 0 looping:
  // listed with the unused patterns (the user: "really, they're only single patterns that we could just file with the others")
  bejeweled2: {
    composer: "Peter Hajba (Skaven)",
    files: ["music/*.mo3"],
    cuts: { "BeyondNetwork#1": [11, 22, 30, 42, 54, 70, 91, 122, 126, 141, 151, 160, 169] },
    alone: ["BeyondNetwork#3", "BeyondNetwork#4"],
    // (the second intro leads straight into the song after it)
    shuffle: { groups: [["BeyondNetwork#1@122", "BeyondNetwork#1@126"]] },
    titles: {
      "BeyondNetwork#1": "Beyond the Network", "BeyondNetwork#1@11": "The Journey Begins", "BeyondNetwork#1@22": "Rain of Lights", "BeyondNetwork#1@30": "LightStorm",
      "BeyondNetwork#1@42": "Sea of Amorphity", "BeyondNetwork#1@54": "Masked Intentions", "BeyondNetwork#1@70": "Routinoid",
      "BeyondNetwork#1@91": "Tunnel Society V2", "BeyondNetwork#1@122": "A New Beginning (Intro 2)", "BeyondNetwork#1@126": "Silent Conquest",
      "BeyondNetwork#1@141": "Schein", "BeyondNetwork#1@151": "Choose and Contemplate", "BeyondNetwork#1@160": "Breathing Love",
      "BeyondNetwork#1@169": "Jewels of Denial", "BeyondNetwork#2": "Bejeweled 2 Theme", "BeyondNetwork#5": "Autonomus V2",
      "BeyondNetwork#6": "Final Destination"
    },
    where: {
      "BeyondNetwork#1": "Classic Mode 1", "BeyondNetwork#1@11": "Classic Mode 1", "BeyondNetwork#1@22": "Classic Mode 2", "BeyondNetwork#1@30": "Classic Mode 3",
      "BeyondNetwork#1@42": "Classic Mode 4", "BeyondNetwork#1@54": "Classic Mode 5", "BeyondNetwork#1@70": "Classic Mode 6",
      "BeyondNetwork#1@91": "Classic Mode 7", "BeyondNetwork#1@122": "Action Mode 1", "BeyondNetwork#1@126": "Action Mode 2",
      "BeyondNetwork#1@141": "Action Mode 3", "BeyondNetwork#1@151": "Action Mode 4", "BeyondNetwork#1@160": "Action Mode 5",
      "BeyondNetwork#1@169": "Action Mode 6", "BeyondNetwork#2": "Main Menu", "BeyondNetwork#5": "Title Screen", "BeyondNetwork#6": "Finale"
    },
    order: ["BeyondNetwork#5", "BeyondNetwork#2", "BeyondNetwork#1", "BeyondNetwork#1@11", "BeyondNetwork#1@22", "BeyondNetwork#1@30", "BeyondNetwork#1@42",
      "BeyondNetwork#1@54", "BeyondNetwork#1@70", "BeyondNetwork#1@91", "BeyondNetwork#1@122", "BeyondNetwork#1@126", "BeyondNetwork#1@141",
      "BeyondNetwork#1@151", "BeyondNetwork#1@160", "BeyondNetwork#1@169", "BeyondNetwork#6"],
    when: {
      "BeyondNetwork#1": "Order 2; a game's start (board update 2) in Classic, Endless, Twilight or the hidden Classic mode; the songs then play on in turn, whatever the level, and after the last one back to this (12:31 into an Action or Puzzle game)",
      "BeyondNetwork#1@11": "Order 11; played on into, 1:05 after a Classic, Endless or Twilight game's start",
      "BeyondNetwork#1@22": "Order 22; played on into, 2:50 after a Classic, Endless or Twilight game's start",
      "BeyondNetwork#1@30": "Order 30; played on into, 4:18 after a Classic, Endless or Twilight game's start",
      "BeyondNetwork#1@42": "Order 42; played on into, 6:22 after a Classic, Endless or Twilight game's start",
      "BeyondNetwork#1@54": "Order 54; played on into, 8:46 after a Classic, Endless or Twilight game's start",
      "BeyondNetwork#1@70": "Order 70; played on into, 11:54 after a Classic, Endless or Twilight game's start",
      "BeyondNetwork#1@91": "Order 91; played on into, 14:23 after a Classic, Endless or Twilight game's start",
      "BeyondNetwork#1@122": "Order 122; a game's start (board update 2) in Action, Puzzle, Hyper, Cognito or Finity; played on into 18:23 after a Classic game's start",
      "BeyondNetwork#1@126": "Order 126; played on into, 0:31 after an Action or Puzzle game's start (18:54 after a Classic one's)",
      "BeyondNetwork#1@141": "Order 141; played on into, 3:06 after an Action or Puzzle game's start (21:29 after a Classic one's)",
      "BeyondNetwork#1@151": "Order 151; played on into, 5:21 after an Action or Puzzle game's start (23:43 after a Classic one's)",
      "BeyondNetwork#1@160": "Order 160; played on into, 7:21 after an Action or Puzzle game's start (25:44 after a Classic one's)",
      "BeyondNetwork#1@169": "Order 169; played on into, 10:01 after an Action or Puzzle game's start (28:24 after a Classic one's); then back to order 2",
      "BeyondNetwork#2": "Order 184; main menu: after the title show, after a game over, quitting from options, high scores",
      "BeyondNetwork#3": "Order 199, music.xml's GET_READY, which the game's code never reads",
      "BeyondNetwork#4": "Order 201, music.xml's LEVEL_CLEAR, which the game's code never reads",
      "BeyondNetwork#5": "Order 203; at launch, on the loading title screen until it completes",
      "BeyondNetwork#6": "Order 217; 500 updates after the hyperspace jump to the last planet of a non-secret Puzzle galaxy"
    },
    unused: ["BeyondNetwork#7"],
    // the pieces run on from one to the next (next), and each mode's last piece runs on into the other mode's first, so a
    // game goes round both modes' pieces; a Classic, Endless or Twilight game starts at Classic Mode 1's first, any other at
    // Action Mode 1 (series: each mode's own, the user's "for each of its modes")
    loops: ["BeyondNetwork#5", "BeyondNetwork#2", "BeyondNetwork#6"],
    next: { "BeyondNetwork#1": "BeyondNetwork#1@11", "BeyondNetwork#1@11": "BeyondNetwork#1@22",
      "BeyondNetwork#1@22": "BeyondNetwork#1@30", "BeyondNetwork#1@30": "BeyondNetwork#1@42",
      "BeyondNetwork#1@42": "BeyondNetwork#1@54", "BeyondNetwork#1@54": "BeyondNetwork#1@70",
      "BeyondNetwork#1@70": "BeyondNetwork#1@91", "BeyondNetwork#1@91": "BeyondNetwork#1@122",
      "BeyondNetwork#1@122": "BeyondNetwork#1@126", "BeyondNetwork#1@126": "BeyondNetwork#1@141",
      "BeyondNetwork#1@141": "BeyondNetwork#1@151", "BeyondNetwork#1@151": "BeyondNetwork#1@160",
      "BeyondNetwork#1@160": "BeyondNetwork#1@169", "BeyondNetwork#1@169": "BeyondNetwork#1" },
    series: {
      "Classic Mode": ["BeyondNetwork#1", "BeyondNetwork#1@11", "BeyondNetwork#1@22", "BeyondNetwork#1@30", "BeyondNetwork#1@42",
        "BeyondNetwork#1@54", "BeyondNetwork#1@70", "BeyondNetwork#1@91"],
      "Action Mode": ["BeyondNetwork#1@122", "BeyondNetwork#1@126", "BeyondNetwork#1@141", "BeyondNetwork#1@151",
        "BeyondNetwork#1@160", "BeyondNetwork#1@169"]
    },
  },
  bejeweledtwist: {
    // the user, 2026-11-19: "bejewel twist 06.10 is the part composed by Philippe Charron, while BejeweledTwist24 is by Peter
    // Hajba; update the composers accordingly"
    composer: "Philippe Charron & Peter Hajba (Skaven)",
    // (its unused 1981-DataJack is Bejeweled's Timed tune, his too)
    composers: { "bejewel twist 06.10": "Philippe Charron", "bejeweledtwist24": "Peter Hajba (Skaven)", "1981-DataJack": "Peter Hajba (Skaven)" },
    files: ["music/*.mo3"],
    cuts: { "bejewel twist 06.10#3": [72], "bejewel twist 06.10#7": [73], "bejeweledtwist24#0": [24] },
    skip: ["bejewel twist 06.10#3@72"],
    shuffle: { out: ["bejewel twist 06.10#7"] },   // (the loading tune)
    // The challenge tunes are named from the game's data (the user, 2026-11-22: "I think a 3-minute challenge refers to Eclipse
    // mode after beating all seven levels on a planet", then "I'll leave naming them to the session then"). Order 24 plays for
    // a challenge with a 3-minute limit, and in properties/challenges.xml those are each planet's eighth, its eclipse challenge
    // (night mode, open once its seven are done), on 11 of the 13 planets; Stratamax's and Survivor's count moves instead and
    // play #5. The split is the game's (Board::PlayChallengeMusic): a time limit plays Blitz's tune, or order 24's at 3 minutes;
    // without one, a challenge with a Moves rule plays #5 and any other #6, the one-move challenges among them. #5 keeps the
    // user's first name, which the game's own properties use for its tempo (MUSIC_CHALLENGE_TURNBASED_MIN_TEMPO and
    // _MAX_TEMPO) and the module's message for order 52; #6 is the message's untimed one
    where: { "bejewel twist 06.10#0": "In-Game 1", "bejewel twist 06.10#1": "In-Game 2", "bejewel twist 06.10#2": "In-Game 3",
      "bejewel twist 06.10#6": "In-Game 4", "bejewel twist 06.10#7@73": "In-Game 5", "bejewel twist 06.10#8": "In-Game 6", "bejewel twist 06.10#7": "Loading",
      "bejewel twist 06.10#3": "Game Over", "bejewel twist 06.10#4": "Victory", "bejewel twist 06.10#5": "Main Menu",
      "bejeweledtwist24#0": "Blitz", "bejeweledtwist24#0@24": "Eclipse Challenge", "bejeweledtwist24#1": "Blitz Results 1", "bejeweledtwist24#2": "Blitz Results 2",
      "bejeweledtwist24#3": "Blitz Results 3", "bejeweledtwist24#4": "Challenge Menu", "bejeweledtwist24#5": "Turn-Based Challenge",
      "bejeweledtwist24#6": "Untimed Challenge", "bejeweledtwist24#7": "Bonus" },
    // the composer's own names for its parts, by their orders in the module's message (read from the file as it plays; order
    // 63's, an untimed challenge, is the place of #6 as well)
    titles: { "bejeweledtwist24#0@24": { line: "24" }, "bejeweledtwist24#4": { line: "48" }, "bejeweledtwist24#7": { line: "79" } },
    order: ["bejewel twist 06.10#7", "bejewel twist 06.10#5", "bejewel twist 06.10#0", "bejewel twist 06.10#1",
      "bejewel twist 06.10#2", "bejewel twist 06.10#6", "bejewel twist 06.10#7@73", "bejewel twist 06.10#8",
      "bejewel twist 06.10#4", "bejewel twist 06.10#3", "bejeweledtwist24#0", "bejeweledtwist24#7", "bejeweledtwist24#1",
      "bejeweledtwist24#2", "bejeweledtwist24#3", "bejeweledtwist24#4", "bejeweledtwist24#0@24", "bejeweledtwist24#5",
      "bejeweledtwist24#6"],
    when: {
      "bejewel twist 06.10#0": "Order 0; in-game track 1 of 6; a new level steps the track by 1 mod 6 (Classic-type modes, Zen)",
      "bejewel twist 06.10#1": "Order 13; in-game track 2 of 6 (the first level-up's)",
      "bejewel twist 06.10#2": "Order 39; in-game track 3 of 6 (the second level-up's)",
      "bejewel twist 06.10#6": "Order 56; in-game track 4 of 6 (the third level-up's)",
      "bejewel twist 06.10#7@73": "Order 73; in-game track 5 of 6 (the fourth level-up's)",
      "bejewel twist 06.10#8": "Order 87; in-game track 6 of 6 (the fifth level-up's); the sixth wraps to order 0",
      "bejewel twist 06.10#7": "Order 70; the loading screen at launch (no SexyCache)",
      "bejewel twist 06.10#3": "Order 52; a non-Blitz game ends by the nuke",
      "bejewel twist 06.10#4": "Order 53; a non-Blitz game ends, not by the nuke (EndGame)",
      "bejewel twist 06.10#5": "Order 54; main menu, and back from the Challenge menu",
      "bejeweledtwist24#0": "Order 0; Blitz: at each level's Go, and after the bonus fruit dance; a challenge with a 5-minute limit (Bonanza's, Firestorm's and Voltage's seven, Chain Reaction's last four)",
      "bejeweledtwist24#0@24": "Order 24; a challenge with a 3-minute limit, at its start: a planet's eclipse challenge, its eighth, open once its seven are done (11 of the 13 planets: all but Stratamax and Survivor)",
      "bejeweledtwist24#1": "Order 42; Blitz ends with a score under 40000, after the board clears out",
      "bejeweledtwist24#2": "Order 44; Blitz ends with a score of 40000-79999",
      "bejeweledtwist24#3": "Order 46; Blitz ends with a score of 80000 or more",
      "bejeweledtwist24#4": "Order 48; the Challenge menu opens",
      "bejeweledtwist24#5": "Order 52; a challenge that counts moves, at its Go: Survivor's eight (last so many moves) and Stratamax's eclipse (50 moves); its layers and tempo follow the moves made",
      "bejeweledtwist24#6": "Order 63; a challenge with neither a time limit nor a move count (one-move challenges included), at its Go: the seven of Arsenal, Coal Mine, Detonator, Enigma, Gem Fall, Preserver, Spectrum and Stratamax, and Chain Reaction's first three",
      "bejeweledtwist24#7": "Order 79; the bonus fruit dance, until it ends after 2200 updates"
    },
    unused: ["bejewel twist 06.10#9", "bejewel twist 06.10#10", "bejewel twist 06.10#11", "bejewel twist 06.10#12",
      "bejewel twist 06.10#13", "bejeweledtwist24#8"],
    loops: ["bejewel twist 06.10#5", "bejewel twist 06.10#0", "bejewel twist 06.10#1", "bejewel twist 06.10#2",
      "bejewel twist 06.10#6", "bejewel twist 06.10#7@73", "bejewel twist 06.10#8", "bejewel twist 06.10#4",
      "bejewel twist 06.10#3", "bejeweledtwist24#0", "bejeweledtwist24#7", "bejeweledtwist24#1", "bejeweledtwist24#2",
      "bejeweledtwist24#3", "bejeweledtwist24#4", "bejeweledtwist24#0@24", "bejeweledtwist24#5", "bejeweledtwist24#6"],
    once: ["bejewel twist 06.10#7"],
    stings: ["bejewel twist 06.10#4", "bejewel twist 06.10#3", "bejeweledtwist24#1", "bejeweledtwist24#2", "bejeweledtwist24#3"],
    gameOver: { "bejewel twist 06.10#0": { to: "bejewel twist 06.10#3", how: "cut" },
      "bejewel twist 06.10#1": { to: "bejewel twist 06.10#3", how: "cut" },
      "bejewel twist 06.10#2": { to: "bejewel twist 06.10#3", how: "cut" },
      "bejewel twist 06.10#6": { to: "bejewel twist 06.10#3", how: "cut" },
      "bejewel twist 06.10#7@73": { to: "bejewel twist 06.10#3", how: "cut" },
      "bejewel twist 06.10#8": { to: "bejewel twist 06.10#3", how: "cut" } },
    series: {
      "In-Game": ["bejewel twist 06.10#0", "bejewel twist 06.10#1", "bejewel twist 06.10#2", "bejewel twist 06.10#6",
        "bejewel twist 06.10#7@73", "bejewel twist 06.10#8"]
    },
  },
  bigmoney: {
    composer: "Peter Hajba (Skaven)",
    files: ["music/*.it"],
    cuts: { "QualityTime_BigMoney#1": [24], "QualityTime_BigMoney#3": [41], "QualityTime_BigMoney#5": [64] },
    // (each level-up fanfare runs straight on into its mode's pause tune)
    shuffle: { groups: [["QualityTime_BigMoney#1", "QualityTime_BigMoney#1@24"], ["QualityTime_BigMoney#3", "QualityTime_BigMoney#3@41"],
      ["QualityTime_BigMoney#5", "QualityTime_BigMoney#5@64"]] },
    where: { "QualityTime_BigMoney#0": "Puzzle Mode", "QualityTime_BigMoney#1": "Puzzle Level Up", "QualityTime_BigMoney#1@24": "Puzzle Pause",
      "QualityTime_BigMoney#2": "Action Mode", "QualityTime_BigMoney#3": "Action Level Up", "QualityTime_BigMoney#3@41": "Action Pause",
      "QualityTime_BigMoney#4": "Strategy Mode", "QualityTime_BigMoney#5": "Strategy Level Up", "QualityTime_BigMoney#5@64": "Strategy Pause",
      "QualityTime_BigMoney#6": "Game Over", "QualityTime_BigMoney#7": "Title Screen" },
    order: ["QualityTime_BigMoney#7", "QualityTime_BigMoney#0", "QualityTime_BigMoney#1", "QualityTime_BigMoney#1@24",
      "QualityTime_BigMoney#2", "QualityTime_BigMoney#3", "QualityTime_BigMoney#3@41", "QualityTime_BigMoney#4",
      "QualityTime_BigMoney#5", "QualityTime_BigMoney#5@64", "QualityTime_BigMoney#6"],
    when: {
      "QualityTime_BigMoney#0": "Order 0; a Puzzle game starts; fades back in after a pause or Continue",
      "QualityTime_BigMoney#1": "Order 23; a Puzzle level is completed; until Continue",
      "QualityTime_BigMoney#1@24": "Order 24; the Puzzle board pauses (pause button, help) while the window has focus",
      "QualityTime_BigMoney#2": "Order 26; an Action game starts; fades back in after a pause or Continue",
      "QualityTime_BigMoney#3": "Order 40; an Action level is completed; until Continue",
      "QualityTime_BigMoney#3@41": "Order 41; the Action board pauses while the window has focus",
      "QualityTime_BigMoney#4": "Order 44; a Strategy game starts; fades back in after a pause or Continue",
      "QualityTime_BigMoney#5": "Order 63; a Strategy level is completed; until Continue",
      "QualityTime_BigMoney#5@64": "Order 64; the Strategy board pauses while the window has focus",
      "QualityTime_BigMoney#6": "Order 67; the game ends (Board::GameOver); the game's tune fades out at 0.02",
      "QualityTime_BigMoney#7": "Order 87; at launch from the loading title screen; plays on into the game selector"
    },
    // (each level-up fanfare runs on into its mode's pause tune: next)
    loops: ["QualityTime_BigMoney#7", "QualityTime_BigMoney#0", "QualityTime_BigMoney#1@24", "QualityTime_BigMoney#2",
      "QualityTime_BigMoney#3@41", "QualityTime_BigMoney#4", "QualityTime_BigMoney#5@64", "QualityTime_BigMoney#6"],
    next: { "QualityTime_BigMoney#1": "QualityTime_BigMoney#1@24", "QualityTime_BigMoney#3": "QualityTime_BigMoney#3@41",
      "QualityTime_BigMoney#5": "QualityTime_BigMoney#5@64" },
    stings: ["QualityTime_BigMoney#1", "QualityTime_BigMoney#3", "QualityTime_BigMoney#5", "QualityTime_BigMoney#6"],
    gameOver: { "QualityTime_BigMoney#0": { to: "QualityTime_BigMoney#6", how: "cross", ms: 500 },
      "QualityTime_BigMoney#2": { to: "QualityTime_BigMoney#6", how: "cross", ms: 500 },
      "QualityTime_BigMoney#4": { to: "QualityTime_BigMoney#6", how: "cross", ms: 500 } },
  },
  bookworm: {
    composer: "Peter Hajba (Skaven)",
    files: ["music/*.mo3"],
    // (the song the module's message names in brackets for the Hall of Fame's orders, read from the file as it plays)
    titles: { "bookworm1#4": { line: "5/9", inner: true } },
    where: { "bookworm1#0": "Classic Mode", "bookworm1#1": "Title Screen", "bookworm1#2": "Action Mode",
      "bookworm1#3": "Danger 1", "bookworm1#4": "Hall of Fame", "bookworm1#5": "Game Over 2", "bookworm1#6": "Game Over 1",
      "bookworm1#7": "Danger 2" },
    order: ["bookworm1#1", "bookworm1#0", "bookworm1#3", "bookworm1#6", "bookworm1#2", "bookworm1#7", "bookworm1#5",
      "bookworm1#4"],
    when: {
      "bookworm1#0": "Order 0; a Classic game starts; loops",
      "bookworm1#1": "Order 27; launch, the game selector, and back to it from a game or the Hall of Fame",
      "bookworm1#2": "Order 37; an Action game starts; loops",
      "bookworm1#3": "Order 51; Classic: while a fire tile is on the bottom row, or Lex sweats in a resumed save",
      "bookworm1#4": "Order 54; the Hall of Fame opens (menu tune fades out at 0.008)",
      "bookworm1#5": "Order 59; an Action game ends; once, no loop",
      "bookworm1#6": "Order 61; a Classic game ends; once, no loop",
      "bookworm1#7": "Order 65; Action: while a fire tile is on the bottom row, or Lex sweats in a resumed save"
    },
    loops: ["bookworm1#1", "bookworm1#0", "bookworm1#3", "bookworm1#2", "bookworm1#7", "bookworm1#4"],
    once: ["bookworm1#6", "bookworm1#5"],
    stings: ["bookworm1#6", "bookworm1#5"],
    gameOver: { "bookworm1#0": { to: "bookworm1#6", how: "cut" }, "bookworm1#3": { to: "bookworm1#6", how: "cut" },
      "bookworm1#2": { to: "bookworm1#5", how: "cut" }, "bookworm1#7": { to: "bookworm1#5", how: "cut" } },
  },
  // Bookworm Adventures sets its music's amplify to 12.0, which BASS holds at 100
  bookwormadv: {
    composer: "Reflexive Audio",
    files: ["sounds/music.mo3"],
    level: { bassAmplify: 100 },
    where: { "music#0": "Greek Battle", "music#1": "Greek Map", "music#2": "Greek Boss", "music#3": "Level Complete",
      "music#4": "Boss Defeated", "music#5": "Arabian Battle", "music#6": "Arabian Map", "music#7": "Arabian Boss",
      "music#8": "Credits", "music#9": "Monster Battle", "music#10": "Monster Map", "music#11": "Monster Boss",
      "music#12": "Main Menu", "music#14": "Mini-Games", "music#15": "Game Over" },
    // the titles the game's own music player gives its tunes (the arena's list, scripts/battleengine.luc): each one's title
    // stands just before its order in that list, and Book 1's battle tune (order 0, a number the list does not write again) is
    // the second string after the order before it (Bookworm Adventures' session, 2026-11-22)
    titles: {
      "music#1": { luc: "scripts/battleengine.luc", list: "NeedsMPDialog", data: 27 }, "music#0": { luc: "scripts/battleengine.luc", list: "NeedsMPDialog", data: 27, after: 2 },
      "music#2": { luc: "scripts/battleengine.luc", list: "NeedsMPDialog", data: 32 }, "music#6": { luc: "scripts/battleengine.luc", list: "NeedsMPDialog", data: 67 },
      "music#5": { luc: "scripts/battleengine.luc", list: "NeedsMPDialog", data: 42 }, "music#7": { luc: "scripts/battleengine.luc", list: "NeedsMPDialog", data: 72 },
      "music#10": { luc: "scripts/battleengine.luc", list: "NeedsMPDialog", data: 128 }, "music#9": { luc: "scripts/battleengine.luc", list: "NeedsMPDialog", data: 102 },
      "music#11": { luc: "scripts/battleengine.luc", list: "NeedsMPDialog", data: 133 }, "music#8": { luc: "scripts/battleengine.luc", list: "NeedsMPDialog", data: 78 }
    },
    order: ["music#12", "music#1", "music#0", "music#3", "music#2", "music#4", "music#6", "music#5", "music#7", "music#10",
      "music#9", "music#11", "music#8", "music#14", "music#15"],
    when: {
      "music#0": "Order 0; a battle in Book 1, and in timed mode, which uses Book 1's tunes",
      "music#1": "Order 27; Book 1's map between battles, and timed mode's",
      "music#2": "Order 32; a book's final boss fight in Book 1, and timed mode's",
      "music#3": "Order 39; a battle is won",
      "music#4": "Order 40; a boss fight is won",
      "music#5": "Order 42; a battle in Book 2",
      "music#6": "Order 67; Book 2's map between battles",
      "music#7": "Order 72; Book 2's final boss fight",
      "music#9": "Order 102; a battle in Book 3",
      "music#10": "Order 128; Book 3's map between battles",
      "music#11": "Order 133; Book 3's final boss fight",
      "music#15": "Order 158; Lex is defeated",
      "music#8": "Order 78; the credits, looping",
      "music#12": "Order 140; main menu, back from the credits, leaving the stats book (loops, fade 0.005)",
      "music#14": "Order 150; a mini-game starts; loops"
    },
    unused: ["music#13"],
    // (Level Complete leads on into the book's map tune, which depends on the book: once)
    loops: ["music#12", "music#1", "music#0", "music#2", "music#6", "music#5", "music#7", "music#10", "music#9", "music#11",
      "music#8", "music#14"],
    once: ["music#3", "music#4", "music#15"],
    stings: ["music#3", "music#4", "music#15"],
    gameOver: { "music#0": { to: "music#15", how: "cross", ms: 2000 }, "music#2": { to: "music#15", how: "cross", ms: 2000 },
      "music#5": { to: "music#15", how: "cross", ms: 2000 }, "music#7": { to: "music#15", how: "cross", ms: 2000 },
      "music#9": { to: "music#15", how: "cross", ms: 2000 }, "music#11": { to: "music#15", how: "cross", ms: 2000 } },
  },
  // as Water Bugs (its session's reading of CBLevel.cpp, CBMain.cpp and bugs.xml, 2026-11-11): cosmo1 through the logos,
  // title and menus; a board the song of its level set, 0-9 cosmo4, 10-19 cosmo3, 20-49 cosmo2, 50-99 cosmo5, then again by
  // hundreds from cosmo4 at 100; a new song only at a set's first level, and cosmo1 again on leaving a board
  cosmicbugs: {
    composer: "Staffan Melin",
    files: ["audio/*.mo3"],
    where: { "cosmo1#0": "Main Menu", "cosmo4#0": "Level Theme 1", "cosmo3#0": "Level Theme 2",
      "cosmo2#0": "Level Theme 3", "cosmo5#0": "Level Theme 4" },
    order: ["cosmo1#0", "cosmo4#0", "cosmo3#0", "cosmo2#0", "cosmo5#0"],
    when: {
      "cosmo1#0": "The logos, title and menus; again on leaving a board",
      "cosmo4#0": "Levels 0-9, 100-199, 500-577; starts at a set's first level, loops",
      "cosmo3#0": "Levels 10-19, 200-299; starts at a set's first level, loops",
      "cosmo2#0": "Levels 20-49, 300-399; starts at a set's first level, loops",
      "cosmo5#0": "Levels 50-99, 400-499; starts at a set's first level, loops"
    },
    loops: ["cosmo1#0", "cosmo4#0", "cosmo3#0", "cosmo2#0", "cosmo5#0"],
  },
  // the menus play cosmo1; each set of levels its own song, the sets' songs repeating (its session's reading, 2026-11-11):
  // 0-9 cosmo4, 10-19 cosmo3, 20-49 cosmo2, 50-99 cosmo5, then again from cosmo4 at 100 (Level_SetBackground's own song
  // table is never played)
  waterbugs: {
    composer: "Staffan Melin",
    files: ["audio/*.mo3"],
    where: { "cosmo1#0": "Main Menu", "cosmo4#0": "Level Theme 1", "cosmo3#0": "Level Theme 2",
      "cosmo2#0": "Level Theme 3", "cosmo5#0": "Level Theme 4" },
    order: ["cosmo1#0", "cosmo4#0", "cosmo3#0", "cosmo2#0", "cosmo5#0"],
    when: {
      "cosmo1#0": "The logos, title and menus; again on leaving a board",
      "cosmo4#0": "Levels 0-9, 100-199, 500-599; starts at a set's first level, loops",
      "cosmo3#0": "Levels 10-19, 200-299; starts at a set's first level, loops",
      "cosmo2#0": "Levels 20-49, 300-399; starts at a set's first level, loops",
      "cosmo5#0": "Levels 50-99, 400-499; starts at a set's first level, loops"
    },
    loops: ["cosmo1#0", "cosmo4#0", "cosmo3#0", "cosmo2#0", "cosmo5#0"],
  },
  dynomite: {
    composer: "Peter Hajba (Skaven)",
    files: ["music/*.it"],
    cuts: { "amazonas#2": [64] },
    shuffle: { out: ["amazonas#5"] },   // (the game's get-ready start)
    // the places from the Dynomite session (2026-11-11): #2@64 is where the won challenge starts, and the game over (#2) and
    // Time Trial's clear (#4) jump into it; #7 is Endless (the port's DynomitePanic); #9, two seconds, nothing starts or jumps to
    where: { "amazonas#0": "Fossil Challenge", "amazonas#1": "Title Screen", "amazonas#2": "Game Over", "amazonas#4": "Time Trial Clear",
      "amazonas#5": "Game Start", "amazonas#6": "Time Trial", "amazonas#2@64": "Challenge Won", "amazonas#7": "Endless", "amazonas#8": "Museum",
      "omniphilia#0": "Stomped" },
    // (the song the module's message names at orders 00-20, and Omniphilia's own title, read from the files as they play)
    titles: { "amazonas#0": { line: "00-20" }, "omniphilia#0": { own: true } },
    order: ["amazonas#1", "amazonas#5", "amazonas#0", "amazonas#8", "amazonas#2@64", "amazonas#6", "amazonas#4",
      "amazonas#7", "omniphilia#0", "amazonas#2"],
    when: {
      "amazonas#0": "Order 0; Fossil Challenge: at a board's first shot (not in the tutorial)",
      "amazonas#1": "Order 22; at launch, and on every return to the intro screen",
      "amazonas#2": "Order 35; the egg line reaches the floor in Fossil Challenge, Endless or Stomped (not in the tutorial)",
      "amazonas#4": "Order 39; Time Trial: every egg is cleared",
      "amazonas#5": "Order 41; a game starts from the intro screen (cases 2 and 6)",
      "amazonas#6": "Order 43; Time Trial: at a board's first shot",
      "amazonas#2@64": "Order 64; Fossil Challenge: the last round of level 15 is won",
      "amazonas#7": "Order 71; Endless: at a board's first shot",
      "amazonas#8": "Order 84; the Museum opens between rounds (started muted at the first shot, faded in)",
      "omniphilia#0": "Order 0 of its own module; Stomped: at a board's first shot"
    },
    unused: ["amazonas#3", "amazonas#9"],
    loops: ["amazonas#1", "amazonas#5", "amazonas#0", "amazonas#8", "amazonas#2@64", "amazonas#6", "amazonas#4", "amazonas#7",
      "omniphilia#0", "amazonas#2"],
    stings: ["amazonas#5", "amazonas#2@64", "amazonas#4", "amazonas#2"],
    gameOver: { "amazonas#0": { to: "amazonas#2", how: "cut" }, "amazonas#7": { to: "amazonas#2", how: "cut" },
      "omniphilia#0": { to: "amazonas#2", how: "cut" } },
  },
  sevenseas: {
    composer: "Peter Hajba (Skaven)",  // the user, 2026-11-19 (Alchemy's entry)
    files: ["music/*.it"],
    where: { "game#0": "In-Game", "game#1": "Level Complete", "game#2": "Game Over", "game#3": "Treasure Bonus",
      "game#4": "Bonus End", "game#5": "Tutorial", "game#6": "Title Screen", "game#7": "Intro", "game#8": "High Scores",
      "game#9": "Bonus Level" },
    order: ["game#6", "game#7", "game#5", "game#0", "game#1", "game#9", "game#3", "game#4", "game#2", "game#8"],
    when: {
      "game#0": "Orders 0-10: (level-1) mod 11 at a level's start, for levels not a multiple of 10 (fade-in)",
      "game#1": "Order 12; a level is completed, with the next-level dialog",
      "game#2": "Order 14; the game is over (Board::GameOver)",
      "game#3": "Order 16; a treasure square is opened in a bonus level",
      "game#4": "Order 18; 150 updates after a bonus level's second state starts",
      "game#5": "Order 20; the tutorial's board",
      "game#6": "Order 22; at launch, until the first level's music",
      "game#7": "Order 25; the pirate's introduction talk (fade-in)",
      "game#8": "Order 28; the high-score list opens after a game (fade-in)",
      "game#9": "Orders 31-34; levels 10, 20, 30...: order 31 + (level/10) mod 4"
    },
    unused: ["game#10"],
    loops: ["game#6", "game#7", "game#5", "game#0", "game#1", "game#9", "game#3", "game#4", "game#2", "game#8"],
    stings: ["game#1", "game#3", "game#4", "game#2"],
    gameOver: { "game#0": { to: "game#2", how: "cut" }, "game#9": { to: "game#2", how: "cut" } },
  },
  // Typer Shark's one module (wordshar.mo3) holds every tune, which the game starts at an order of it (ChangeMusic); its subsongs stand for them
  typershark: {
    composer: "Jonne Valtonen",
    files: ["music/*.mo3"],
    cuts: { "wordshar#1": [20] },
    where: { "wordshar#0": "In-Game", "wordshar#1": "Main Menu", "wordshar#1@20": "Map", "wordshar#2": "Game Over", "wordshar#3": "Level Complete",
      "wordshar#4": "Danger 1", "wordshar#6": "Danger 2", "wordshar#7": "Treasure", "wordshar#8": "Boss Fight" },
    order: ["wordshar#1", "wordshar#1@20", "wordshar#0", "wordshar#4", "wordshar#6", "wordshar#7", "wordshar#8",
      "wordshar#3", "wordshar#2"],
    when: {
      "wordshar#0": "Order 0; a level's board starts (Play from the map); back after danger or an EMP",
      "wordshar#1": "Order 17; launch, a reset game, the post-level Continue, and a bonus level's treasure scene",
      "wordshar#1@20": "Order 20; the map screen opens",
      "wordshar#2": "Order 30; the game-over dialog opens",
      "wordshar#3": "Order 34; a level is cleared, once the rating has shown",
      "wordshar#4": "Order 37; an enemy comes within 160 pixels of the left edge (player not diving)",
      "wordshar#6": "Order 43; an enemy comes within 15 pixels of the left edge",
      "wordshar#7": "Order 46; the diver goes for treasure (player state 6 or 11); fade 0.015",
      "wordshar#8": "Order 48; boss event 10 (the boss fight)"
    },
    unused: ["wordshar#5"],
    loops: ["wordshar#1", "wordshar#1@20", "wordshar#0", "wordshar#4", "wordshar#8", "wordshar#3", "wordshar#2"],
    once: ["wordshar#6", "wordshar#7"],
    stings: ["wordshar#6", "wordshar#7", "wordshar#3", "wordshar#2"],
    gameOver: { "wordshar#0": { to: "wordshar#2", how: "cross", ms: 2000 },
      "wordshar#4": { to: "wordshar#2", how: "cross", ms: 2000 }, "wordshar#8": { to: "wordshar#2", how: "cross", ms: 2000 } },
  },
  mummymaze: {
    composer: "Hannu Salonen",
    files: ["music/*.it"],
    cuts: { "game#9": [67], "game#10": [77] },
    // (the name the module's message gives the victory tune, read from the file as it plays)
    titles: { "game#7": { line: "55" } },
    where: { "game#0": "Main Menu", "game#1": "Tutorial", "game#2": "In-Game", "game#3": "Stage Clear",
      "game#5": "Chamber End 1", "game#6": "Chamber End 2", "game#7": "Victory", "game#9@67": "Map", "game#10@77": "High Scores" },
    order: ["game#0", "game#1", "game#9@67", "game#2", "game#5", "game#6", "game#3", "game#7", "game#10@77"],
    when: {
      "game#0": "Order 0 at launch; order 1 (fade) on the intro screen, and back from the map to it",
      "game#1": "Order 14; the tutorial's first map loads",
      "game#2": "Orders 23-38; a chamber starts: 23 + chamber mod 16 (Adventure or Classic chamber)",
      "game#3": "Order 40; the next-chamber dialog opens after a chamber",
      "game#5": "Order 44; a chamber's ending after its fade, an even chance with order 46",
      "game#6": "Order 46; a chamber's ending after its fade, an even chance with order 44",
      "game#7": "Order 55; the victory (beach) screen",
      "game#9@67": "Order 67; the adventure map opens (fade)",
      "game#10@77": "Order 76; the high scores open (fade)"
    },
    unused: ["game#9", "game#10"],
    loops: ["game#0", "game#1", "game#9@67", "game#2", "game#5", "game#6", "game#3", "game#7", "game#10@77"],
    stings: ["game#5", "game#6", "game#3"],
  },
  // tunes inside the games' own files: the Mahjong Escapes keep their MO3 modules among Resource.dll's resources (the game's
  // module, 339, starts at one of two orders by the player's progress and jumps to others while gold tiles are in play), Ning
  // Po its one music stream and three jingles in its Python archive; the layout select's tune turns with each visit (LayoutSelect's
  // gMusicTurn): China 338, 336, 337, Japan 337, 336, 337, numbered as the game reaches them
  mahjongchina: {
    composer: "Lin Gardiner",  // the user, 2026-11-19: "Lin Gardiner was the composer for Mahjong Escape"
    inside: [{ file: "resource.dll", magic: "MO3" }],
    chains: { "Resource 339#0": ["Resource 339#1", "Resource 339#2", "Resource 339#3"] },
    where: { "Resource 335#0": "Main Menu", "Resource 338#0": "Layout Select 1", "Resource 336#0": "Layout Select 2",
      "Resource 337#0": "Layout Select 3", "Resource 339#0": "In-Game" },
    order: ["Resource 335#0", "Resource 338#0", "Resource 336#0", "Resource 337#0", "Resource 339#0"],
    when: {
      "Resource 335#0": "The main menu opens",
      "Resource 338#0": "Layout select: visit 1 of every 3 (gMusicTurn 0)",
      "Resource 336#0": "Layout select: visit 2 of every 3 (gMusicTurn 1)",
      "Resource 337#0": "Layout select: visit 3 of every 3 (gMusicTurn 2)",
      "Resource 339#0": "A game's map opens; loop piece 0 at odd progress, 13 at even; 7 with a key in play, 12 after a gold pair"
    },
    loops: ["Resource 335#0", "Resource 338#0", "Resource 336#0", "Resource 337#0", "Resource 339#0"],
    series: {
      "Layout Select": ["Resource 338#0", "Resource 336#0", "Resource 337#0"]
    },
  },
  mahjongjapan: {
    composer: "Lin Gardiner",  // the user, 2026-11-19: "Lin Gardiner was the composer for Mahjong Escape"
    inside: [{ file: "resource.dll", magic: "MO3" }],
    chains: { "Resource 339#0": ["Resource 339#1", "Resource 339#2", "Resource 339#3"] },
    where: { "Resource 335#0": "Main Menu", "Resource 337#0": "Layout Select 1", "Resource 336#0": "Layout Select 2",
      "Resource 339#0": "In-Game" },
    order: ["Resource 335#0", "Resource 337#0", "Resource 336#0", "Resource 339#0"],
    when: {
      "Resource 335#0": "The main menu opens",
      "Resource 337#0": "Layout select: visits 1 and 3 of every 3 (gMusicTurn 0 and 2)",
      "Resource 336#0": "Layout select: visit 2 of every 3 (gMusicTurn 1)",
      "Resource 339#0": "A game's map opens; loop piece 0 at odd progress, 10 at even; 4 with a key in play, 8 after a gold pair"
    },
    // (the game's round of the layout screen's tunes plays Layout Select 1 again after 2: 1, 2, 1)
    loops: ["Resource 335#0", "Resource 337#0", "Resource 336#0", "Resource 339#0"],
    series: {
      "Layout Select": ["Resource 337#0", "Resource 336#0"]
    },
  },
  // Word Harmony's music is made as it plays, from WAV clips: the menu's loop, and in the rising-water mode one drum track
  // stepping through the intense pieces as the water rises, intense-between its bridge from each to the next (which piece goes
  // with which height is not known), here as one tune, each piece once; its other modes pick single instrument samples at
  // random, which are not tunes and are left out, as is intense-levelend (a sound); intenseEnd is the game over's jingle (Game::gameOver)
  wordharmony: {
    composer: "Guy Whitmore",
    files: ["res/music/menuMusic.wav", "res/music/intenseEnd.wav"],
    joined: [{ id: "intense", pieces: ["res/music/intense0.wav", "res/music/intense-between.wav", "res/music/intense1.wav",
      "res/music/intense-between.wav", "res/music/intense1b.wav", "res/music/intense-between.wav", "res/music/intense2.wav",
      "res/music/intense-between.wav", "res/music/intense3.wav", "res/music/intense-between.wav", "res/music/intense4.wav",
      "res/music/intense-between.wav", "res/music/intense5.wav", "res/music/intense-between.wav", "res/music/intense6.wav"] }],
    where: { menuMusic: "Main Menu", intense: "Rising Water", intenseEnd: "Game Over" },
    order: ["menuMusic", "intense", "intenseEnd"],
    when: {
      "menuMusic": "The main menu opens (also on return from a game)",
      "intense": "The game screen opens in mode 2 (drums.xml); modes 0 and 1 use other clips",
      "intenseEnd": "The game-over results show (or the congratulations screen after mode 0's last level)"
    },
    // (intenseEnd is the drums layer's last clip, and the game's own stop fades the layer out before its last clip, so the
    // game may never play it: the game over starts game-over-music.xml instead; kept as the table's game-over tune)
    loops: ["menuMusic", "intense"],
    once: ["intenseEnd"],
    stings: ["intenseEnd"],
  },
  ningpo: {
    shuffle: { out: ["nm_splash"] },   // (the splash screen's, before the assets load)
    // the user, 2026-11-19: "Ian Livingstone was the composer for Ning Po, Noah's Ark, Rocket Mania, Pixelus, and Hammer Heads"
    composer: "Ian Livingstone",
    inside: [{ file: "ningpo.pyz", zip: ["theme_np.ogg", "nm_splash.ogg", "nm_gameover.ogg", "nm_youwin.ogg"] }],
    where: { theme_np: "In-Game", nm_splash: "Splash Screen", nm_gameover: "Game Over", nm_youwin: "Victory" },
    order: ["nm_splash", "theme_np", "nm_youwin", "nm_gameover"],
    when: {
      "theme_np": "A game starts; loops; off while paused, and at game over or win",
      "nm_splash": "Once at launch, before the assets load",
      "nm_gameover": "No moves left; the game ends",
      "nm_youwin": "The board is won"
    },
    loops: ["theme_np"],
    once: ["nm_splash", "nm_youwin", "nm_gameover"],
    stings: ["nm_splash", "nm_youwin", "nm_gameover"],
    gameOver: { "theme_np": { to: "nm_gameover", how: "cut" } },
  },
  // TipTop's one tune is a MIDI file in its archive, played as the port plays it: TipTop's sequencer into TinySoundFont with
  // the port's bank (games/tiptop/src/Music.cpp and MusicSynth.cpp). It starts at the main menu's first showing, after the
  // delay the game's config sets (MusicStarterProc), and is never stopped; the game holds it while paused (MusicPauseProc)
  // and while its window is inactive
  tiptop: { composer: "PCB Productions",
    inside: [{ file: "TipTop.dat", tiptop: ["sounds/bkgd.mid"] }],
    bank: "soundfonts/tiptop.sf2",
    where: { bkgd: "In-Game" },
    order: ["bkgd"],
    when: {
      "bkgd": "From the main menu's first showing, after the config's delay; loops through every screen and game; held while the game is paused or its window inactive"
    },
    loops: ["bkgd"],
  },
  // the Ogg games
  bookwormadv2: {
    composer: "Staffan Melin",
    files: ["music/*.ogg"],
    where: { chinese_map: "Chinese Map", chinese_level: "Chinese Levels", chinese_boss: "Chinese Boss",
      fairy_tale_map: "Fairy Tale Map", fairy_tale_level: "Fairy Tale Levels", fairy_tale_boss: "Fairy Tale Boss",
      scifi_map: "Sci-Fi Map", scifi_level: "Sci-Fi Levels", scifi_boss: "Sci-Fi Boss", dance_battle_level: "Dance Battle" },
    order: ["fairy_tale_level", "fairy_tale_map", "fairy_tale_boss", "chinese_map", "chinese_level", "chinese_boss",
      "scifi_map", "scifi_level", "scifi_boss", "dance_battle_level"],
    when: {
      "fairy_tale_map": "Book 4's map (and Book 0's): on entering it, and again after a battle or boss is won; also Letter Rip, Link 'n Spell, Word Master and Word Up",
      "chinese_map": "Book 5's map: on entering it, and again after a battle or boss is won; also the Golden Coins mini-game",
      "scifi_map": "Book 6's map: on entering it, and again after a battle or boss is won; also the Mutawords mini-game",
      "fairy_tale_boss": "Book 4's boss fights, chapters 1 to 10 (the Caterpillar's among them)",
      "chinese_boss": "Book 5's boss fights, chapters 1 to 10 (the Monkey King's among them)",
      "scifi_boss": "Book 6's boss fights, chapters 1 to 6 and 8 to 10 (The Machine's among them)",
      "dance_battle_level": "Book 6, chapter 7: the dance battle, for its ordinary fights and its boss alike (it has no boss tune)",
      "fairy_tale_level": "Book 4's ordinary battles (chapters 1 to 10); also the main menu while the profile's current book is 4 or less",
      "chinese_level": "Book 5's ordinary battles (chapters 1 to 10); also the main menu while the profile's current book is 5",
      "scifi_level": "Book 6's ordinary battles (chapters 1 to 6 and 8 to 10); also the main menu while the profile's current book is above 5"
    },
    loops: ["fairy_tale_level", "fairy_tale_map", "fairy_tale_boss", "chinese_map", "chinese_level", "chinese_boss", "scifi_map",
      "scifi_level", "scifi_boss", "dance_battle_level"],
  },
  // Noah's Ark: mainintro plays once as play starts and runs on into mainloop, which loops: one tune, repeating from the loop
  noahsark: {
    composer: "Ian Livingstone",  // the user, 2026-11-19 (Ning Po's entry)
    files: ["music/loading.ogg", "music/gameover.ogg"],
    joined: [{ id: "mainloop", pieces: ["music/mainintro.ogg", "music/mainloop.ogg"], loopFrom: 1 }],
    where: { mainloop: "In-Game", loading: "Title Screen", gameover: "Game Over" },
    order: ["loading", "mainloop", "gameover"],
    when: {
      "mainloop": "A game starts: mainintro plays once, then mainloop loops",
      "loading": "At launch (fade 0.002); again on the main menu; loops",
      "gameover": "The game-over high-score list opens (fade 0.002); loops"
    },
    loops: ["loading", "mainloop", "gameover"],
    stings: ["gameover"],
    gameOver: { "mainloop": { to: "gameover", how: "cross", ms: 2500 } },
  },
  // Venice: each scene names its music (levels/*.xml: main_music, and win_music for a level won), which the scene starts
  // silent and raises over 1000 updates (4 seconds at 250 a second) from intro_time / 4 - 1000 updates after it opens, and
  // which starts again with every level (games/venice/src/Level.cpp: CrossFade). A background's levels share one tune, its
  // place named by their title; Flood and Journey (config.xml: level_file_game, level_file_relaxed) both go through the
  // backgrounds 1, 3, 4, 5, 6, 2, 7, 8, nine levels each (eight for the last), and Trick Shot (level_file_trick) through
  // regions in that order. ingame_2_intro is only in config.xml's list of sounds to load; ingame_1_intro is the world map's,
  // and an intro before a level's tune only in blank.xml, which nothing opens. victory.ogg is a sound (the Sounds window's)
  venice: {
    composer: "Guillaume Bastard & Staffan Melin",
    files: ["audio/title_music.ogg", "audio/ingame_*.ogg", "audio/final_victory.ogg"],
    // its music is a looped sound (games/venice/src/VeniceSound.cpp), not a song
    voices: true,
    where: { title_music: "Main Menu", ingame_1_intro: "World Map", ingame_1_body: "Welcome to Venice",
      ingame_1_win: "Level Complete", ingame_3_body: "Lunar Lanes", ingame_4_body: "Cupid's Cove",
      ingame_5_body: "Venetian Villa", ingame_2_body: "San Marco Square", final_victory: "Final Victory" },
    order: ["title_music", "ingame_4_body", "ingame_1_intro", "ingame_1_body", "ingame_1_win", "ingame_3_body",
      "ingame_5_body", "ingame_2_body", "final_victory", "ingame_2_intro"],
    when: {
      "title_music": "The main menu opens: after loading, and from its start on every return to it; loops",
      "ingame_1_intro": "The world map opens: a new Flood or Journey game's start (after the tutorial when it shows), then before levels 4, 7, 10 and on; loops",
      "ingame_1_body": "Flood and Journey levels 1 to 9 (Welcome to Venice) and 37 to 45 (Grotto Ghetto), Trick Shot's first and fifth regions, and Survival; loops",
      "ingame_1_win": "A level other than a trick shot is won: starts 2 updates after its last plate goes and rises over 80 (0.32 seconds) as the level's tune drops to a fifth and plays on under it; once",
      "ingame_3_body": "Flood and Journey levels 10 to 18 (Lunar Lanes) and 64 to 71 (Lavender Lair), Trick Shot's second and last regions; loops",
      "ingame_4_body": "Flood and Journey levels 19 to 27 (Cupid's Cove), Trick Shot's third region; also the tutorial level, a new player's first game before the map, and the high scores; loops",
      "ingame_5_body": "Flood and Journey levels 28 to 36 (Venetian Villa), Trick Shot's fourth region; loops",
      "ingame_2_body": "Flood and Journey levels 46 to 63 (San Marco Square, then Vesta's Vista), Trick Shot's sixth and seventh regions; also the credits; loops",
      "final_victory": "The win scene: after Flood's or Journey's level 71, or once every trick shot is made; loops, until the high scores"
    },
    unused: ["ingame_2_intro"],
    loops: ["title_music", "ingame_4_body", "ingame_1_intro", "ingame_1_body", "ingame_3_body", "ingame_5_body", "ingame_2_body",
      "final_victory"],
    once: ["ingame_1_win"],
    stings: ["ingame_1_win"],
  },
  // Heavy Weapon: one module, atomictank.mo3, which the game starts at an order of it for each screen (games/heavyweapon/src:
  // its song ids are named by those orders), and one Ogg, lovetheme.ogg; 200 updates a second, fades 0.002 an update in
  // (2.5 seconds) and 0.004 out. Its subsongs are its tunes: 0 from order 0, 2 from 0x2a, 4 from 0x3b, 6 from 0x49, 8 from
  // 0x4f; 1 starts at order 41, which the game never starts (its orders 82 to 84 are reached from nowhere else), and 3, 5, 7
  // and 9 are a stop pattern (0.2 seconds). The original's BASS calls a song of the module still playing once it has played
  // (HeavyWeaponApp.cpp, IsPlaying), so only the first main menu fades its tune in and the armory never changes the music.
  // The credits give the music to Yannis Brown of Groovy Audio and the love theme to MegaLazer (data/credits.xml)
  heavyweapon: {
    shuffle: { out: ["atomictank#8"] },   // (the intro under the PopCap and title pictures)
    composer: "Yannis Brown",
    composers: { lovetheme: "MegaLazer" },
    files: ["music/*.mo3", "music/lovetheme.ogg"],
    where: { "atomictank#8": "Intro", lovetheme: "Title Screen", "atomictank#4": "Main Menu", "atomictank#0": "In-Game",
      "atomictank#2": "Boss Fight", "atomictank#6": "Level Complete" },
    order: ["atomictank#8", "lovetheme", "atomictank#4", "atomictank#0", "atomictank#2", "atomictank#6", "atomictank#1"],
    when: {
      "atomictank#8": "Order 0x4f; at launch, once, under the PopCap and title pictures, fading out over 250 updates (1.25 seconds) as they end; faded in over 500 updates for the cutscene after level 9 and for the game over (no tank left), looping, on through the high scores and menus after it",
      "lovetheme": "The intro's pictures end: once, as the game loads and the title waits for Play",
      "atomictank#4": "Order 0x3b; the first main menu after launch, faded in over 500 updates (2.5 seconds), and the high scores and armory from it; also the game-over dialog's Continue, on into the armory; loops",
      "atomictank#0": "Order 0; a level starts, Campaign or Survival: faded in over 500 updates (2.5 seconds) as the music before fades out over 250; in Campaign it fades out over 250 as the boss warning comes; loops",
      "atomictank#2": "Order 0x2a; at once after the boss warning's fifth flash (127 updates each); fades out over 250 updates (1.25 seconds) as the beaten boss's last blast goes; loops",
      "atomictank#6": "Order 0x49; at once as the results come up, 500 updates (2.5 seconds) after the boss's last blast; loops, on through the armory and the final cutscene"
    },
    unused: ["atomictank#1"],
    // (the Intro plays once at launch, but goes round in the cutscene and after a game ends through it: loops)
    loops: ["atomictank#8", "atomictank#4", "atomictank#0", "atomictank#2", "atomictank#6"],
    once: ["lovetheme"],
    stings: ["atomictank#6"],
    gameOver: { "atomictank#0": { to: "atomictank#8", how: "cross", ms: 1250 },
      "atomictank#2": { to: "atomictank#8", how: "cross", ms: 1250 } },
  },
  // Bonnie's Bookstore plays its one module as tracks, each a range of orders (its track table, games/bonnies/game/Music.cpp:
  // the first for the menu, the rest in a game), and a logo tune
  bonnies: {
    shuffle: { out: ["logo"] },   // (the logo's)
    composer: "Barry Leitch",
    files: ["sound/logo.ogg", "sound/music.oxm"],
    level: { fmod3XM: true },
    parts: { music: [[1, 18], [20, 52], [54, 82], [84, 99], [101, 126], [128, 150], [152, 168], [170, 196], [198, 220]] },
    where: { logo: "Logo", "music#0": "Main Menu", "music#1": "In-Game 1", "music#2": "In-Game 2", "music#3": "In-Game 3",
      "music#4": "In-Game 4", "music#5": "In-Game 5", "music#6": "In-Game 6", "music#7": "In-Game 7", "music#8": "In-Game 8" },
    order: ["logo", "music#0", "music#1", "music#2", "music#3", "music#4", "music#5", "music#6", "music#7", "music#8"],
    when: {
      "music#0": "The menu screen opens (kind 2); a new pick 700 ms after the tune ends",
      "music#1": "A game screen opens (kind 3): the least-played of its eight tunes, random among ties, never twice in a row",
      "music#2": "Same pick as In-Game 1; the next pick starts 700 ms after a tune ends",
      "music#3": "Same pick as In-Game 1",
      "music#4": "Same pick as In-Game 1",
      "music#5": "Same pick as In-Game 1",
      "music#6": "Same pick as In-Game 1",
      "music#7": "Same pick as In-Game 1",
      "music#8": "Same pick as In-Game 1"
    },
    // (each In-Game tune plays once, and the game picks another at random 0.7 s after it ends)
    loops: ["music#0"],
    once: ["Logo", "music#1", "music#2", "music#3", "music#4", "music#5", "music#6", "music#7", "music#8"],
    stings: ["Logo"],
  },
  // Pixelus and Rocket Mania join their in-game (and Rocket Mania its menu) music from pieces by a playlist, as the games do
  pixelus: {
    composer: "Ian Livingstone",  // the user, 2026-11-19 (Ning Po's entry)
    files: ["music/pixelus *.ogg"],
    joined: [{ playlist: "music/pixelus in-game playlist.txt", piece: "music/{}.ogg" }],
    where: { "pixelus main menu loop v02": "Main Menu", "pixelus story screen loop v01": "Story Screen",
      "pixelus temple screen loop v01": "Temple Screen", "pixelus in-game": "In-Game" },
    order: ["pixelus main menu loop v02", "pixelus story screen loop v01", "pixelus in-game",
      "pixelus temple screen loop v01"],
    when: {
      "pixelus main menu loop v02": "Launch, and every return to the main menu; all music fades out for the island map, a story and a puzzle",
      "pixelus story screen loop v01": "1 update after a story screen opens",
      "pixelus temple screen loop v01": "The temple screen opens",
      "pixelus in-game": "150 updates after a puzzle shows (no help over it); fades out at the last tile; pauses with the game"
    },
    loops: ["pixelus main menu loop v02", "pixelus story screen loop v01", "pixelus in-game", "pixelus temple screen loop v01"],
  },
  rocketmania: {
    composer: "Ian Livingstone",  // the user, 2026-11-19 (Ning Po's entry)
    files: ["music/* loop *.ogg"],
    joined: [{ playlist: "music/tubetricks menu one v02 playlist.txt", piece: "music/tubetricks menu one v02 {}.ogg" },
      { playlist: "music/tubetricks in-game one v02 playlist.txt", piece: "music/tubetricks in-game one v02 {}.ogg" }],
    where: { "tubetricks menu one v02": "Main Menu", "tubetricks in-game one v02": "In-Game",
      "tubetricks level up loop one v01": "Level Complete", "tubetricks game over loop one v01": "Game Over" },
    order: ["tubetricks menu one v02", "tubetricks in-game one v02", "tubetricks level up loop one v01",
      "tubetricks game over loop one v01"],
    when: {
      "tubetricks menu one v02": "Launch, and the game selector (fade 0.2); other tunes stop",
      "tubetricks in-game one v02": "A game's start (fade 0.666); after a pause, and after the buy-rockets screen between levels",
      "tubetricks level up loop one v01": "A level ends; until the buy-rockets screen closes",
      "tubetricks game over loop one v01": "The night runs out, the top line fills, or strategy mode runs out of moves"
    },
    loops: ["tubetricks menu one v02", "tubetricks in-game one v02", "tubetricks level up loop one v01",
      "tubetricks game over loop one v01"],
    stings: ["tubetricks level up loop one v01", "tubetricks game over loop one v01"],
    gameOver: { "tubetricks in-game one v02": { to: "tubetricks game over loop one v01", how: "cross", ms: 500 } },
  },
  // Hammer Heads' four tracks, one a screen (Application::StartMusic, games/hammerheads/src/Application.cpp: a track fades in
  // over the others, and one already playing goes on where it was). Its credits name its developers and nobody for the music;
  // the user named its composer
  hammerheads: {
    composer: "Ian Livingstone",  // the user, 2026-11-19 (Ning Po's entry)
    files: ["sounds/Music - *.ogg"],
    where: { "Music - Loading and Menu": "Main Menu", "Music - In-Game": "In-Game", "Music - Bonus Game": "Bonus Round",
      "Music - High Score": "Trophy Room" },
    order: ["Music - Loading and Menu", "Music - In-Game", "Music - Bonus Game", "Music - High Score"],
    when: {
      "Music - Loading and Menu": "Fades in slowly over the loading screen and plays on through the main menu, and again whenever the main menu opens",
      "Music - In-Game": "A game opens (new, continued or from a restart point), and again after each wave's result, a bonus round or the trophy room; fades out at each wave's result, the game over and the win",
      "Music - Bonus Game": "A bonus round opens",
      "Music - High Score": "The trophy room opens, from the main menu or from a game when trophies are won; also the high score table from the main menu"
    },
    loops: ["Music - Loading and Menu", "Music - In-Game", "Music - Bonus Game", "Music - High Score"],
  }
};
