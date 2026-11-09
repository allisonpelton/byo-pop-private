// The sound effects of each game, for Desktop 95's Sounds window (web/sounds.js; docs/WEB_PLAYER.md, "The soundboard"), by the
// game's id. A game is listed in the window only when it has an entry here and its files are in this browser. Paths are in the
// game's folder as the import writes it, matched without regard to case.
//   groups  [{name, sounds: [[path, name, when], ...]}, ...]: the window's group boxes, in order, each with its buttons in order
//           path  the file
//           name  the button's label: a plain name of our own; a voice line is its words, in quotes (a file name gives
//                 only part of a line until someone corrects it by ear); a sound the game never plays says so; a
//                 silent file is left out
//           when  the tooltip's second line: when the game plays it, found in the port's source (every PlaySample of the
//                 sound), never a passage of the game's text
// BookWorm's 45 are its sound table (games/bookworm/src/BookWormApp.cpp, gSoundNames; Res.h, SoundId), every file of
// sounds/ but blank.au, which is silent and never played. Three are never played: bad, gameovertone and up9 (the rising
// notes stop at up8). The user corrected two labels by ear on 2026-11-09: lex_welcome says "Welcome to BookWorm!" and
// bonusword is Lex saying "Bonus!".
var ByopopSoundboards = {
  bookworm: { groups: [
    { name: "Lex", sounds: [
      ["sounds/lex_welcome.ogg", "\"Welcome to BookWorm!\"", "The game menu, once a run"],
      ["sounds/lex_go.ogg", "\"Go!\"", "A Classic game starts (half the time)"],
      ["sounds/lex_letsgo.ogg", "\"Let's go!\"", "A Classic game starts (half the time)"],
      ["sounds/lex_getready.ogg", "\"Get ready!\"", "An Action game or level starts"],
      ["sounds/lex_scramble.ogg", "\"Scramble!\"", "Lex is clicked to scramble the letters"],
      ["sounds/lex_careful.ogg", "\"Careful!\"", "A fire tile reaches the bottom row (one of four)"],
      ["sounds/lex_danger.ogg", "\"Danger!\"", "A fire tile reaches the bottom row (one of four)"],
      ["sounds/lex_lookout.ogg", "\"Look out!\"", "A fire tile reaches the bottom row (one of four)"],
      ["sounds/lex_watchout.ogg", "\"Watch out!\"", "A fire tile reaches the bottom row (one of four)"],
      ["sounds/lex_levelup.ogg", "\"Level up!\"", "A level is complete"],
      ["sounds/bonusword.ogg", "\"Bonus!\"", "The bonus word is spelled"],
      ["sounds/lex_burp.ogg", "Burp", "Lex has eaten a word with two or more fire tiles"]
    ] },
    { name: "Lex's praise", sounds: [
      ["sounds/lex_verygood.ogg", "\"Very good!\"", "A gold tile is awarded (one of three)"],
      ["sounds/lex_excellent.ogg", "\"Excellent!\"", "A gold tile is awarded (one of three)"],
      ["sounds/lex_awesome.ogg", "\"Awesome!\"", "A gold tile is awarded (one of three)"],
      ["sounds/lex_fantastic.ogg", "\"Fantastic!\"", "A sapphire tile is awarded (one of two)"],
      ["sounds/lex_astonishing.ogg", "\"Astonishing!\"", "A sapphire tile is awarded (one of two)"],
      ["sounds/lex_incredible.ogg", "\"Incredible!\"", "A diamond tile is awarded (one of two)"],
      ["sounds/lex_unbelievable.ogg", "\"Unbelievable!\"", "A diamond tile is awarded (one of two)"]
    ] },
    { name: "Words", sounds: [
      ["sounds/nextselect.ogg", "Letter picked", "A tile is added to the word"],
      ["sounds/bonuscomplete.ogg", "Special word", "A tile is added that makes a word holding a special tile"],
      ["sounds/buzzer.ogg", "Buzzer", "Not a word, or a scramble refused"],
      ["sounds/wordcomplete.ogg", "Word scored", "A word is scored; an Action level's time bonus"],
      ["sounds/redtilecleared.ogg", "Fire word scored", "A word with a fire tile in it is scored"],
      ["sounds/chomp.ogg", "Chomp", "Lex eats a tile of the word"],
      ["sounds/bad.ogg", "Bad (unused)", "Never played"]
    ] },
    { name: "Word notes", sounds: [
      ["sounds/up1.ogg", "1st letter", "After a word is scored, the note for its 1st letter"],
      ["sounds/up2.ogg", "2nd letter", "After a word is scored, the note for its 2nd letter"],
      ["sounds/up3.ogg", "3rd letter", "After a word is scored, the note for its 3rd letter"],
      ["sounds/up4.ogg", "4th letter", "After a word is scored, the note for its 4th letter"],
      ["sounds/up5.ogg", "5th letter", "After a word is scored, the note for its 5th letter"],
      ["sounds/up6.ogg", "6th letter", "After a word is scored, the note for its 6th letter"],
      ["sounds/up7.ogg", "7th letter", "After a word is scored, the note for its 7th letter"],
      ["sounds/up8.ogg", "8th letter on", "After a word is scored, the note for its 8th letter and each after it"],
      ["sounds/up9.ogg", "9th note (unused)", "Never played: the notes stop at the 8th"]
    ] },
    { name: "Tiles", sounds: [
      ["sounds/piece_hit.ogg", "Tile lands", "A falling tile lands"],
      ["sounds/appear2.ogg", "Green tile", "A green tile appears"],
      ["sounds/spinsuperbonus.ogg", "Special tile", "A gold, sapphire or diamond tile appears"],
      ["sounds/flametile.ogg", "Fire tile", "Fire tiles appear"],
      ["sounds/tileburn.ogg", "Tile burns", "A fire tile burns down into the tile below it"]
    ] },
    { name: "Danger and game over", sounds: [
      ["sounds/warning.ogg", "Warning", "A fire tile on the bottom row (three beeps); the first fire tile's hint; each second of Action's countdown"],
      ["sounds/screenburngameover.ogg", "Screen burns", "Game over: the fire burns up the screen"],
      ["sounds/gameovertone.ogg", "Game over tone (unused)", "Never played"]
    ] },
    { name: "Menus", sounds: [
      ["sounds/buttonclick.ogg", "Click", "A button is pressed"],
      ["sounds/goldtile.ogg", "Volume test", "The Options' sound effects slider is let go"]
    ] }
  ] }
};
