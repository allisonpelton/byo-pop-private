// The music of each game, for the listing page's music player (web/music.js; docs/WEB_PLAYER.md, "The music player"), by the
// game's id. Paths are in the game's folder as the import writes it, matched without regard to case, where * stands for any part
// of a file name.
//   files   the files that are tunes: a module's subsongs are its tunes, an Ogg Vorbis or WAV file is one tune
//   parts   for a module the game plays in ranges of its orders: {"<file name>": [[first order, last order], ...]}, its tunes
//           instead of its subsongs
//   joined  tunes the game joins from pieces by a playlist: [{playlist: "<path>", piece: "<path with {} for the playlist's word>"}],
//           or from files in order: [{id: "<name>", pieces: ["<path>", ...], loopFrom: <the piece its loop goes back to>}]
//   cuts    {"<file name>#<n>": [order, ...]}: a subsong split where the game's code starts a different tune partway
//           through; each later piece is "<file name>#<n>@<order>" (an order the subsong never reaches: from it to the end)
//   starts  {"<file name>#<n>": [order, ...]}: orders the game also starts a subsong at, where the tune before plays on
//           through them in the game (no seam: a long game's music runs into the next part); each is a piece
//           "<file name>#<n>@<order>" from there to the end, and the subsong stays whole
//   chains  {"<file name>#<n>": ["<file name>#<m>", ...]}: parts the game only plays as jumps inside one tune, one track
//   skip    ["<file name>#<n>", ...]: tunes that are not tunes (n counts from 0 in the file's subsongs or parts)
//   names   {"<file name>#<n>" or "<playlist name>": "<name>"}: plain names, never invented titles and never the game's own text
//           at length; without one a tune is named after its file
//   inside  tunes kept inside a game's file: [{file: "<path>", magic: "<first bytes>"}] its resources (a Windows module's)
//           that start with those bytes, the magic naming their format, or [{file: "<path>", zip: ["<entry>", ...]}] those
//           entries of a zip archive; each is then a file as above
//   where   {"<file name>#<n>", "<file name>" (an Ogg or WAV file) or "<playlist name>": "<place>"}: where in the game its code plays
//           the tune, in the style of a game music rip ("Main Menu", "Danger 2": Title Case, one place, at most 20 characters,
//           tunes of one place numbered as the game reaches them), from the game's code (its session's answer) or, for an Ogg
//           game, its file's name; shown beside the tune and in the display
//   unused  ["<file name>#<n>", ...]: tunes the game's code never plays (left in its files), shown as such
//   when    {"<file name>#<n>" ...: "<when>"}: exactly when the game's code plays it, as numbers (levels, orders, updates), the
//           place's tooltip
//   composer  who wrote the music, by the game's own credits (its credits screen, text or exe; a company where they name no
//           person); none for a game whose credits name nobody for it, shown as Unknown. composers {"<id>": "<name>"} where
//           the credits split the music by tune (none does)
//   level   the level the game plays its modules at, where it is not the renderer's own (platform/MusicLevels.h):
//           {bassAmplify: n} the BASS amplify it sets (0 to 100), {fmod3XM: true} FMOD 3's XM pre-amp. The renderer is the
//           game's own (libopenmpt for MO3 and OXM, libxmp-lite for the rest: web/music/openmpt_web.c)
// Tunes under two seconds are left out (the silences and stings between tunes). Every game's tunes were listed from the user's
// copies on 2026-11-07.
var ByopopSoundtracks = {
  // Places and unused tunes from each port's code (the coordinator's reading, 2026-11-11); a place only inferred is left blank
  alchemy: {
    files: ["music/*.s3m"],
    where: { "alch_intro#0": "Title Screen", "1985-hehasnoface#0": "In-Game" },
    when: {
      "alch_intro#0": "Order 0 from launch, over the loading title screen; fades out (0.004) when loading completes",
      "1985-hehasnoface#0": "Fades in (0.002) when loading completes; loops through the menus and every game; no other tune"
    },
    unused: ["1985-hehasnoface#1"]
  },
  // AstroPop's one module holds every tune, which the game starts at an order of it (PlaySong); its subsongs stand for them,
  // each starting at one of the orders the game plays (0 loading, 5 the menus, 9 a level, 35 the danger siren, 39 the game's
  // end, 41 a level's tally and level-up: games/astropop/src, checked 2026-11-11 once its music played)
  astropop: {
    composer: "Phillipe Charon",
    files: ["music/*.mo3"],
    where: { "music#0": "Loading", "music#1": "Main Menu", "music#2": "In-Game", "music#3": "Danger", "music#4": "Game Over",
      "music#5": "Level Complete" },
    when: {
      "music#0": "Order 0; at launch, during loading",
      "music#1": "Order 5; main menu, pilot choice screen, and a level's story page (cross-fade 0.05)",
      "music#2": "Order 9 (10 if 35 or 41 plays); a board's start, after a story page, when the siren stops",
      "music#3": "Order 35; when a brick reaches the danger line (siren starts); back to 9 when the siren ends",
      "music#4": "Order 39; when the game ends (BoardEnd)",
      "music#5": "Order 41; a level is completed, from the level-up text through the tally"
    },
  },
  atomica: {
    composer: "Hannu Salonen",
    files: ["music/*.it"],
    where: { "atomica#2": "Title Screen", "atomica#3": "Level Theme 1", "atomica#5": "Level Theme 2", "atomica#7": "Main Menu", "atomica#9": "Level Theme 3",
      "atomica#8": "Game Over 1", "atomica#10": "Game Over 2" },
    when: {
      "atomica#2": "Order 6; title screen, until it closes",
      "atomica#3": "Order 12; Strategy mode's board start, and levels with level%5==1 (not level 1)",
      "atomica#5": "Order 24; levels with level%5==0",
      "atomica#7": "Order 31; main menu (not after a game), and the Play button (button 6) pressed in it",
      "atomica#9": "Order 44; Action mode's board start",
      "atomica#8": "Order 41; the main menu shown after a game over",
      "atomica#10": "Order 66; 300 updates into the board's game-over sequence"
    },
    unused: ["atomica#1", "atomica#4", "atomica#6"]
  },
  bejeweled: {
    files: ["music/*.s3m"],
    where: { "1980-Autonomus#0": "Title Screen", "1984-Network#0": "Normal Mode", "1981-DataJack#0": "Timed Mode" },
    when: {
      "1980-Autonomus#0": "From launch on the title screen; fades out (0.004) when it finishes",
      "1984-Network#0": "A new game in Normal mode; loops; fades the Timed tune out, fades in at 0.002",
      "1981-DataJack#0": "A new game in Timed mode; loops; fades the Normal tune out, fades in at 0.002"
    },
    unused: ["1981-DataJack#1", "1981-DataJack#2", "1981-DataJack#3"]
  },
  // one module, BeyondNetwork.mo3: the game plays it from a starting order for each screen (properties/music.xml), so its
  // subsongs are the tunes
  bejeweled2: {
    composer: "Skaven",
    files: ["music/*.mo3"],
    starts: { "BeyondNetwork#1": [122] },
    where: { "BeyondNetwork#1": "Classic Mode", "BeyondNetwork#1@122": "Action Mode", "BeyondNetwork#2": "Main Menu", "BeyondNetwork#5": "Title Screen",
      "BeyondNetwork#6": "Finale" },
    when: {
      "BeyondNetwork#1": "Order 2; a Classic game's start (board update 2), neither Action nor Puzzle",
      "BeyondNetwork#1@122": "Order 122; an Action or a Puzzle game's start (board update 2)",
      "BeyondNetwork#2": "Order 184; main menu: after the title show, after a game over, quitting from options, high scores",
      "BeyondNetwork#5": "Order 203; at launch, on the loading title screen until it completes",
      "BeyondNetwork#6": "Order 217; 500 updates after the hyperspace jump to the last planet of a non-secret Puzzle galaxy"
    },
    unused: ["BeyondNetwork#3", "BeyondNetwork#4", "BeyondNetwork#7"]
  },
  bejeweledtwist: {
    composer: "Phillipe Charon & Peter Hajba",
    files: ["music/*.mo3"],
    cuts: { "bejewel twist 06.10#3": [72], "bejewel twist 06.10#7": [73], "bejeweledtwist24#0": [24] },
    skip: ["bejewel twist 06.10#3@72"],
    where: { "bejewel twist 06.10#0": "In-Game 1", "bejewel twist 06.10#1": "In-Game 2", "bejewel twist 06.10#2": "In-Game 3",
      "bejewel twist 06.10#6": "In-Game 4", "bejewel twist 06.10#7@73": "In-Game 5", "bejewel twist 06.10#8": "In-Game 6", "bejewel twist 06.10#7": "Loading",
      "bejewel twist 06.10#3": "Game Over", "bejewel twist 06.10#4": "Victory", "bejewel twist 06.10#5": "Main Menu",
      "bejeweledtwist24#0": "Blitz", "bejeweledtwist24#0@24": "Blitz 3 Minutes", "bejeweledtwist24#1": "Blitz Results 1", "bejeweledtwist24#2": "Blitz Results 2",
      "bejeweledtwist24#3": "Blitz Results 3", "bejeweledtwist24#4": "Challenge Menu", "bejeweledtwist24#5": "Challenge 1",
      "bejeweledtwist24#6": "Challenge 2", "bejeweledtwist24#7": "Bonus" },
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
      "bejeweledtwist24#0": "Order 0; Blitz: at each level's Go, and after the bonus fruit dance; timed challenges not of 180 s",
      "bejeweledtwist24#0@24": "Order 24; a timed challenge with a 180 s goal, at its start",
      "bejeweledtwist24#1": "Order 42; Blitz ends with a score under 40000, after the board clears out",
      "bejeweledtwist24#2": "Order 44; Blitz ends with a score of 40000-79999",
      "bejeweledtwist24#3": "Order 46; Blitz ends with a score of 80000 or more",
      "bejeweledtwist24#4": "Order 48; the Challenge menu opens",
      "bejeweledtwist24#5": "Order 52; an untimed challenge with a Moves goal, at its Go; layers follow the moves made",
      "bejeweledtwist24#6": "Order 63; an untimed challenge without a Moves goal, at its Go",
      "bejeweledtwist24#7": "Order 79; the bonus fruit dance, until it ends after 2200 updates"
    },
    unused: ["bejewel twist 06.10#9", "bejewel twist 06.10#10", "bejewel twist 06.10#11", "bejewel twist 06.10#12",
      "bejewel twist 06.10#13", "bejeweledtwist24#8"]
  },
  bigmoney: {
    composer: "Skaven",
    files: ["music/*.it"],
    cuts: { "QualityTime_BigMoney#1": [24], "QualityTime_BigMoney#3": [41], "QualityTime_BigMoney#5": [64] },
    where: { "QualityTime_BigMoney#0": "Puzzle Mode", "QualityTime_BigMoney#1": "Puzzle Level Up", "QualityTime_BigMoney#1@24": "Puzzle Pause",
      "QualityTime_BigMoney#2": "Action Mode", "QualityTime_BigMoney#3": "Action Level Up", "QualityTime_BigMoney#3@41": "Action Pause",
      "QualityTime_BigMoney#4": "Strategy Mode", "QualityTime_BigMoney#5": "Strategy Level Up", "QualityTime_BigMoney#5@64": "Strategy Pause",
      "QualityTime_BigMoney#6": "Game Over", "QualityTime_BigMoney#7": "Title Screen" },
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
  },
  bookworm: {
    composer: "Skaven",
    files: ["music/*.mo3"],
    where: { "bookworm1#0": "Classic Mode", "bookworm1#1": "Title Screen", "bookworm1#2": "Action Mode",
      "bookworm1#3": "Danger 1", "bookworm1#4": "Hall of Fame", "bookworm1#5": "Game Over 2", "bookworm1#6": "Game Over 1",
      "bookworm1#7": "Danger 2" },
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
    unused: ["music#13"]
  },
  // as Water Bugs (its session's reading of CBLevel.cpp, CBMain.cpp and bugs.xml, 2026-11-11): cosmo1 through the logos,
  // title and menus; a board the song of its level set, 0-9 cosmo4, 10-19 cosmo3, 20-49 cosmo2, 50-99 cosmo5, then again by
  // hundreds from cosmo4 at 100; a new song only at a set's first level, and cosmo1 again on leaving a board
  cosmicbugs: {
    composer: "Staffan Melin",
    files: ["audio/*.mo3"],
    where: { "cosmo1#0": "Main Menu", "cosmo4#0": "Level Theme 1", "cosmo3#0": "Level Theme 2",
      "cosmo2#0": "Level Theme 3", "cosmo5#0": "Level Theme 4" },
    when: {
      "cosmo1#0": "The logos, title and menus; again on leaving a board",
      "cosmo4#0": "Levels 0-9, 100-199, 500-577; starts at a set's first level, loops",
      "cosmo3#0": "Levels 10-19, 200-299; starts at a set's first level, loops",
      "cosmo2#0": "Levels 20-49, 300-399; starts at a set's first level, loops",
      "cosmo5#0": "Levels 50-99, 400-499; starts at a set's first level, loops"
    },
  },
  // the menus play cosmo1; each set of levels its own song, the sets' songs repeating (its session's reading, 2026-11-11):
  // 0-9 cosmo4, 10-19 cosmo3, 20-49 cosmo2, 50-99 cosmo5, then again from cosmo4 at 100 (Level_SetBackground's own song
  // table is never played)
  waterbugs: {
    composer: "Staffan Melin",
    files: ["audio/*.mo3"],
    where: { "cosmo1#0": "Main Menu", "cosmo4#0": "Level Theme 1", "cosmo3#0": "Level Theme 2",
      "cosmo2#0": "Level Theme 3", "cosmo5#0": "Level Theme 4" },
    when: {
      "cosmo1#0": "The logos, title and menus; again on leaving a board",
      "cosmo4#0": "Levels 0-9, 100-199, 500-599; starts at a set's first level, loops",
      "cosmo3#0": "Levels 10-19, 200-299; starts at a set's first level, loops",
      "cosmo2#0": "Levels 20-49, 300-399; starts at a set's first level, loops",
      "cosmo5#0": "Levels 50-99, 400-499; starts at a set's first level, loops"
    },
  },
  dynomite: {
    composer: "Skaven",
    files: ["music/*.it"],
    cuts: { "amazonas#2": [64] },
    // the places from the Dynomite session (2026-11-11): #2@64 is where the won challenge starts, and the game over (#2) and
    // Time Trial's clear (#4) jump into it; #7 is Endless (the port's DynomitePanic); #9, two seconds, nothing starts or jumps to
    where: { "amazonas#0": "Fossil Challenge", "amazonas#1": "Title Screen", "amazonas#2": "Game Over", "amazonas#4": "Time Trial Clear",
      "amazonas#5": "Game Start", "amazonas#6": "Time Trial", "amazonas#2@64": "Challenge Won", "amazonas#7": "Endless", "amazonas#8": "Museum",
      "omniphilia#0": "Stomped" },
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
    unused: ["amazonas#3", "amazonas#9"]
  },
  sevenseas: {
    files: ["music/*.it"],
    where: { "game#0": "In-Game", "game#1": "Level Complete", "game#2": "Game Over", "game#3": "Treasure Bonus",
      "game#4": "Bonus End", "game#5": "Tutorial", "game#6": "Title Screen", "game#7": "Intro", "game#8": "High Scores",
      "game#9": "Bonus Level" },
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
    unused: ["game#10"]
  },
  // Typer Shark's one module (wordshar.mo3) holds every tune, which the game starts at an order of it (ChangeMusic); its subsongs stand for them
  typershark: {
    composer: "Jonne Valtonen",
    files: ["music/*.mo3"],
    cuts: { "wordshar#1": [20] },
    where: { "wordshar#0": "In-Game", "wordshar#1": "Main Menu", "wordshar#1@20": "Map", "wordshar#2": "Game Over", "wordshar#3": "Level Complete",
      "wordshar#4": "Danger 1", "wordshar#6": "Danger 2", "wordshar#7": "Treasure", "wordshar#8": "Boss Fight" },
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
    unused: ["wordshar#5"]
  },
  mummymaze: {
    composer: "Hannu Salonen",
    files: ["music/*.it"],
    cuts: { "game#9": [67], "game#10": [77] },
    where: { "game#0": "Main Menu", "game#1": "Tutorial", "game#2": "In-Game", "game#3": "Stage Clear",
      "game#5": "Chamber End 1", "game#6": "Chamber End 2", "game#7": "Victory", "game#9@67": "Map", "game#10@77": "High Scores" },
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
    unused: ["game#9", "game#10"]
  },
  // tunes inside the games' own files: the Mahjong Escapes keep their MO3 modules among Resource.dll's resources (the game's
  // module, 339, starts at one of two orders by the player's progress and jumps to others while gold tiles are in play), Ning
  // Po its one music stream and three jingles in its Python archive; the layout select's tune turns with each visit (LayoutSelect's
  // gMusicTurn): China 338, 336, 337, Japan 337, 336, 337, numbered as the game reaches them
  mahjongchina: {
    inside: [{ file: "resource.dll", magic: "MO3" }],
    chains: { "Resource 339#0": ["Resource 339#1", "Resource 339#2", "Resource 339#3"] },
    where: { "Resource 335#0": "Main Menu", "Resource 338#0": "Layout Select 1", "Resource 336#0": "Layout Select 2",
      "Resource 337#0": "Layout Select 3", "Resource 339#0": "In-Game" },
    when: {
      "Resource 335#0": "The main menu opens",
      "Resource 338#0": "Layout select: visit 1 of every 3 (gMusicTurn 0)",
      "Resource 336#0": "Layout select: visit 2 of every 3 (gMusicTurn 1)",
      "Resource 337#0": "Layout select: visit 3 of every 3 (gMusicTurn 2)",
      "Resource 339#0": "A game's map opens; loop piece 0 at odd progress, 13 at even; 7 with a key in play, 12 after a gold pair"
    },
  },
  mahjongjapan: {
    inside: [{ file: "resource.dll", magic: "MO3" }],
    chains: { "Resource 339#0": ["Resource 339#1", "Resource 339#2", "Resource 339#3"] },
    where: { "Resource 335#0": "Main Menu", "Resource 337#0": "Layout Select 1", "Resource 336#0": "Layout Select 2",
      "Resource 339#0": "In-Game" },
    when: {
      "Resource 335#0": "The main menu opens",
      "Resource 337#0": "Layout select: visits 1 and 3 of every 3 (gMusicTurn 0 and 2)",
      "Resource 336#0": "Layout select: visit 2 of every 3 (gMusicTurn 1)",
      "Resource 339#0": "A game's map opens; loop piece 0 at odd progress, 10 at even; 4 with a key in play, 8 after a gold pair"
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
    when: {
      "menuMusic": "The main menu opens (also on return from a game)",
      "intense": "The game screen opens in mode 2 (drums.xml); modes 0 and 1 use other clips",
      "intenseEnd": "The game-over results show (or the congratulations screen after mode 0's last level)"
    },
  },
  ningpo: { inside: [{ file: "ningpo.pyz", zip: ["theme_np.ogg", "nm_splash.ogg", "nm_gameover.ogg", "nm_youwin.ogg"] }],
    where: { theme_np: "In-Game", nm_splash: "Splash Screen", nm_gameover: "Game Over", nm_youwin: "Victory" } },
    when: {
      "theme_np": "A game starts; loops; off while paused, and at game over or win",
      "nm_splash": "Once at launch, before the assets load",
      "nm_gameover": "No moves left; the game ends",
      "nm_youwin": "The board is won"
    },
  // the Ogg games
  bookwormadv2: {
    composer: "Staffan Melin",
    files: ["music/*.ogg"],
    where: { chinese_map: "Chinese Map", chinese_level: "Chinese Levels", chinese_boss: "Chinese Boss",
      fairy_tale_map: "Fairy Tale Map", fairy_tale_level: "Fairy Tale Levels", fairy_tale_boss: "Fairy Tale Boss",
      scifi_map: "Sci-Fi Map", scifi_level: "Sci-Fi Levels", scifi_boss: "Sci-Fi Boss", dance_battle_level: "Dance Battle" },
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
  },
  // Noah's Ark: mainintro plays once as play starts and runs on into mainloop, which loops: one tune, repeating from the loop
  noahsark: {
    files: ["music/loading.ogg", "music/gameover.ogg"],
    joined: [{ id: "mainloop", pieces: ["music/mainintro.ogg", "music/mainloop.ogg"], loopFrom: 1 }],
    where: { mainloop: "In-Game", loading: "Title Screen", gameover: "Game Over" },
    when: {
      "mainloop": "A game starts: mainintro plays once, then mainloop loops",
      "loading": "At launch (fade 0.002); again on the main menu; loops",
      "gameover": "The game-over high-score list opens (fade 0.002); loops"
    },
  },
  // Bonnie's Bookstore plays its one module as tracks, each a range of orders (its track table, games/bonnies/game/Music.cpp:
  // the first for the menu, the rest in a game), and a logo tune
  bonnies: {
    composer: "Barry Leitch",
    files: ["sound/logo.ogg", "sound/music.oxm"],
    level: { fmod3XM: true },
    parts: { music: [[1, 18], [20, 52], [54, 82], [84, 99], [101, 126], [128, 150], [152, 168], [170, 196], [198, 220]] },
    where: { logo: "Logo", "music#0": "Main Menu", "music#1": "In-Game 1", "music#2": "In-Game 2", "music#3": "In-Game 3",
      "music#4": "In-Game 4", "music#5": "In-Game 5", "music#6": "In-Game 6", "music#7": "In-Game 7", "music#8": "In-Game 8" },
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
  },
  // Pixelus and Rocket Mania join their in-game (and Rocket Mania its menu) music from pieces by a playlist, as the games do
  pixelus: {
    files: ["music/pixelus *.ogg"],
    joined: [{ playlist: "music/pixelus in-game playlist.txt", piece: "music/{}.ogg" }],
    where: { "pixelus main menu loop v02": "Main Menu", "pixelus story screen loop v01": "Story Screen",
      "pixelus temple screen loop v01": "Temple Screen", "pixelus in-game": "In-Game" },
    when: {
      "pixelus main menu loop v02": "Launch, and every return to the main menu; all music fades out for the island map, a story and a puzzle",
      "pixelus story screen loop v01": "1 update after a story screen opens",
      "pixelus temple screen loop v01": "The temple screen opens",
      "pixelus in-game": "150 updates after a puzzle shows (no help over it); fades out at the last tile; pauses with the game"
    },
  },
  rocketmania: {
    files: ["music/* loop *.ogg"],
    joined: [{ playlist: "music/tubetricks menu one v02 playlist.txt", piece: "music/tubetricks menu one v02 {}.ogg" },
      { playlist: "music/tubetricks in-game one v02 playlist.txt", piece: "music/tubetricks in-game one v02 {}.ogg" }],
    where: { "tubetricks menu one v02": "Main Menu", "tubetricks in-game one v02": "In-Game",
      "tubetricks level up loop one v01": "Level Complete", "tubetricks game over loop one v01": "Game Over" },
    when: {
      "tubetricks menu one v02": "Launch, and the game selector (fade 0.2); other tunes stop",
      "tubetricks in-game one v02": "A game's start (fade 0.666); after a pause, and after the buy-rockets screen between levels",
      "tubetricks level up loop one v01": "A level ends; until the buy-rockets screen closes",
      "tubetricks game over loop one v01": "The night runs out, the top line fills, or strategy mode runs out of moves"
    },
  }
};
