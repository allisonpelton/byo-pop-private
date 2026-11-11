// The controls of each game, for a game page's Controls panel and for the page's key remapping, gamepad and on-screen touch
// keys, by the game's id (the id: of its games/<id>/web/page.js). An entry is one of:
//   {does, how}    a mouse action, or anything that is not a single key
//   {does, keys}   keys: KeyboardEvent.code values (e.g. "KeyP", "Space", "Escape") that do that one thing in the game
// The words are our own. Only keys the game's own code handles are listed: not the framework's (Alt+Enter, the mod menu's F2),
// no debug or cheat keys, no keys that only type into a name box, and no keys of a mod. The comment above each game names the
// source it was read from (a path in games/<id>/).
var ByopopControlData = {

  // src/Board.cpp Board::KeyChar (any key pauses), Board::MouseDown (a right click, theClickCount < 0, uses the forge)
  alchemy: [
    { does: "Place the piece", how: "Move the mouse, then click a square" },
    { does: "Use the forge", how: "Right click" },
    { does: "Pause or resume", how: "Press any key" }
  ],

  // src/BoardInput.cpp Board::KeyChar (Space, Z and X), Board::KeyDown (arrows, Return, Escape), Board::MouseDown (left, right and
  // middle buttons), Board::MouseMove (the ship follows the mouse)
  astropop: [
    { does: "Move the ship", how: "Move the mouse" },
    { does: "Take the bricks at the ship", how: "Left click" },
    { does: "Shoot a brick", how: "Right click" },
    { does: "Start the play at once", how: "Middle click" },
    { does: "Start the play at once with the keyboard", keys: ["Enter"] },
    { does: "Move the ship left", keys: ["ArrowLeft", "KeyZ"] },
    { does: "Move the ship right", keys: ["ArrowRight", "KeyX"] },
    { does: "Take the bricks", keys: ["ArrowDown"] },
    { does: "Shoot a brick", keys: ["ArrowUp"] },
    { does: "Pause or resume", keys: ["Space"] },
    { does: "Open the menu", keys: ["Escape"] }
  ],

  // src/Grid.cpp Grid::KeyDown (Space), Grid::MouseDown (left, double and right clicks)
  atomica: [
    { does: "Pick up an atom", how: "Click it" },
    { does: "Move the atom", how: "Click where it should go" },
    { does: "Set the atom down at once", how: "Double click" },
    { does: "Drop the atom you hold", how: "Right click" },
    { does: "Pause or resume", keys: ["Space"] }
  ],

  // src/BoardRules.cpp Board::KeyChar (any key pauses; the undo key there is a variant mod), Board::MouseDown / MouseDrag
  bejeweled: [
    { does: "Swap two gems", how: "Click a gem, then one next to it" },
    { does: "Swap by dragging", how: "Drag a gem onto a neighbour" },
    { does: "Pause or resume", how: "Press any key" }
  ],

  // src/BoardInput.cpp Board::MouseDown (a click, a right click, a click while the game is paused), Board::MouseDrag; the board's
  // Board::KeyChar (src/BoardCheats.cpp) takes only typed cheat words
  bejeweled2: [
    { does: "Swap two gems", how: "Click a gem, then one next to it" },
    { does: "Swap by dragging", how: "Drag a gem onto a neighbour" },
    { does: "Let go of the gem you picked", how: "Right click" },
    { does: "Resume a paused game", how: "Click" }
  ],

  // src/BoardTwist.cpp Board::KeyDown (Pause/Escape, Return/Space, arrows), Board::MouseDown
  bejeweledtwist: [
    { does: "Twist four gems", how: "Click between four gems" },
    { does: "Move the cursor up", keys: ["ArrowUp"] },
    { does: "Move the cursor down", keys: ["ArrowDown"] },
    { does: "Move the cursor left", keys: ["ArrowLeft"] },
    { does: "Move the cursor right", keys: ["ArrowRight"] },
    { does: "Twist at the cursor", keys: ["Enter", "Space"] },
    { does: "Pause or resume", keys: ["Escape", "Pause"] }
  ],

  // src/Board.cpp Board::KeyChar (Space; the H, U, P, R and digit keys there are mods), Board::MouseDown
  bigmoney: [
    { does: "Take a group of money", how: "Click a group of matching pieces" },
    { does: "Pause or resume", keys: ["Space"] }
  ],

  // game/GameScreen.cpp event type 8 (Space, Pause, Escape, F1), mouse play through Game::
  bonnies: [
    { does: "Spell a word", how: "Click or drag across neighbouring letters" },
    { does: "Pause or resume", keys: ["Space", "Pause"] },
    { does: "Open the menu, or resume", keys: ["Escape"] },
    { does: "Help", keys: ["F1"] }
  ],

  // src/Board.cpp Board::KeyChar (Space; digits and Tab there are mods), Board::MouseDown / MouseDrag / MouseUp
  bookworm: [
    { does: "Spell a word", how: "Click or drag across neighbouring tiles" },
    { does: "Submit the word", how: "Click the last tile again, or drag and let go" },
    { does: "Clear the word", how: "Right click" },
    { does: "Pause or resume", keys: ["Space"] }
  ],

  // src/Board.cpp Board::KeyChar (1 to 3, Return, 4, Space), Board::MouseDown
  bookwormadv: [
    { does: "Spell a word", how: "Click or drag across neighbouring tiles" },
    { does: "Attack with the word", keys: ["Enter", "Digit4"] },
    { does: "Use the first potion", keys: ["Digit1"] },
    { does: "Use the second potion", keys: ["Digit2"] },
    { does: "Use the third potion", keys: ["Digit3"] },
    { does: "Pause or resume", keys: ["Space"] }
  ],

  // src/Board.cpp Board::KeyChar (same as the first Adventures)
  bookwormadv2: [
    { does: "Spell a word", how: "Click or drag across neighbouring tiles" },
    { does: "Attack with the word", keys: ["Enter", "Digit4"] },
    { does: "Use the first potion", keys: ["Digit1"] },
    { does: "Use the second potion", keys: ["Digit2"] },
    { does: "Use the third potion", keys: ["Digit3"] },
    { does: "Pause or resume", keys: ["Space"] }
  ],

  // src/CBLevel.cpp Level_Update (Mouse_Pressed 1 and 2, Key_Pressed 0x20, gEscape, 0xd), src/CBShooter.cpp Shooter_Control
  cosmicbugs: [
    { does: "Move the ship", how: "Move the mouse" },
    { does: "Fire", how: "Click or hold the left button" },
    { does: "Turn the ship around", how: "Right click, or press Space" },
    { does: "Pause", keys: ["Escape"] },
    { does: "Resume", keys: ["Enter"] }
  ],

  // src/Dynomite.cpp Dynomite::KeyDown, src/DynomiteFossil.cpp DynomiteFossil::KeyDown, Dynomite::MouseDrag / MouseDown
  dynomite: [
    { does: "Aim the launcher", how: "Move the mouse" },
    { does: "Fire an egg", how: "Click" },
    { does: "Pause", keys: ["Space", "Escape", "F1", "KeyP"] },
    { does: "Show or hide the museum", keys: ["KeyM", "NumpadSubtract"] }
  ],

  // src/MahjongApp.cpp MahjongWidget::KeyChar / KeyDown / MouseDown, src/Board.cpp Board::OnChar (Return/Space skip the ending),
  // Board::OnLButtonDown / OnRButtonDown
  mahjongchina: [
    { does: "Remove a pair", how: "Click two matching free tiles" },
    { does: "Skip the ending", keys: ["Enter", "Space"] }
  ],

  // same as mahjongchina: src/Board.cpp Board::OnChar, Board::OnLButtonDown
  mahjongjapan: [
    { does: "Remove a pair", how: "Click two matching free tiles" },
    { does: "Skip the ending", keys: ["Enter", "Space"] }
  ],

  // src/BoardLogic.cpp KEY_* macros (mKeyDown for Space and the arrows), src/BoardTurn.cpp Board::MouseDown
  mummymaze: [
    { does: "Step up", keys: ["ArrowUp"] },
    { does: "Step down", keys: ["ArrowDown"] },
    { does: "Step left", keys: ["ArrowLeft"] },
    { does: "Step right", keys: ["ArrowRight"] },
    { does: "Wait a turn", keys: ["Space"] },
    { does: "Step or wait with the mouse", how: "Click a square beside you, or your own" }
  ],

  // src/NingPo.cpp NingPo::handleKey (s, f, h, u, Space), onLeftClick / onRightClick (the Backspace, q cheat is left out)
  ningpo: [
    { does: "Remove a pair", how: "Click two matching free tiles" },
    { does: "Drop your selection", how: "Right click" },
    { does: "Hint", keys: ["KeyH", "KeyF"] },
    { does: "Undo", keys: ["KeyU"] },
    { does: "Shuffle", keys: ["KeyS"] },
    { does: "Show the moves (hold)", keys: ["Space"] }
  ],

  // src/BoardInput.cpp Board::MouseDownImpl (a right click puts the animal back), Board::MouseDrag; keys there are mods only
  noahsark: [
    { does: "Link animals", how: "Drag from one animal to the next" },
    { does: "Put the animal back", how: "Right click" }
  ],

  // src/Board.cpp Board::KeyDown (Space), Board::MouseMove / MouseDown
  pixelus: [
    { does: "Throw or pull a block", how: "Point at a square and click" },
    { does: "Pause or resume", keys: ["Space"] }
  ],

  // src/BoardInput.cpp Board::KeyChar (Space), Board::MouseDown / MouseUp / MouseDrag
  rocketmania: [
    { does: "Turn a tile", how: "Click it" },
    { does: "Turn a tile the other way", how: "Right click it" },
    { does: "Move a tile sideways (arcade)", how: "Drag it to the next column" },
    { does: "Pause or resume", keys: ["Space"] }
  ],

  // src/BoardUpdate.cpp Board::Update (GetKeys: arrows, numpad, Space), src/BoardInput.cpp Board::MouseDown
  sevenseas: [
    { does: "Steer left", keys: ["ArrowLeft", "Numpad4"] },
    { does: "Steer right", keys: ["ArrowRight", "Numpad6"] },
    { does: "Steer up", keys: ["ArrowUp", "Numpad8"] },
    { does: "Steer down", keys: ["ArrowDown", "Numpad2"] },
    { does: "Fire the cannons", keys: ["Space", "Numpad5", "Numpad0", "Insert"] },
    { does: "Move with the mouse", how: "Click a square next to your ship" }
  ],

  // src/BoardKeys.cpp Board::KeyChar (letters, Return, Space), src/MapScreen.cpp MapScreen::KeyChar (letters, Space),
  // src/HintDialog.cpp HintDialog::KeyChar (Return, Space, Escape)
  typershark: [
    { does: "Kill a creature", how: "Type the letters of its word" },
    { does: "Use the Shark Zapper when it is charged", keys: ["Enter"] },
    { does: "Pause or resume (when the player's space bar setting is on)", keys: ["Space"] },
    { does: "Go back to the map at the end of a level", keys: ["Space"] },
    { does: "Choose a level on the map", how: "Click its chest, or type its letter" },
    { does: "Start the level chosen on the map", keys: ["Space"] },
    { does: "Go on from a hint box", keys: ["Enter", "Space", "Escape"] }
  ],

  // src/App.cpp ColumnCursorProc MSG_KEY (arrows, numpad 4 and 6, Space), MSG_MOUSEDOWN
  tiptop: [
    { does: "Move along the columns", how: "Move the mouse" },
    { does: "Drop a block", how: "Click" },
    { does: "Move one column left", keys: ["ArrowLeft", "Numpad4"] },
    { does: "Move one column right", keys: ["ArrowRight", "Numpad6"] },
    { does: "Drop a block", keys: ["Space"] }
  ],

  // src/WBLevel.cpp Level_Update (Mouse_Pressed 1 and 2, Key_Pressed 0x20, gEscape asks to quit), src/WBShooter.cpp
  waterbugs: [
    { does: "Move the ship", how: "Move the mouse" },
    { does: "Fire", how: "Click or hold the left button" },
    { does: "Turn the ship around", how: "Right click, or press Space" },
    { does: "Quit the board", keys: ["Escape"] }
  ],

  // game/WordBallSystem.cpp WordBallSystem::keyDown (P, H, F, Escape; digits and letters there are the level editor)
  wordharmony: [
    { does: "Move a letter ball", how: "Hold and drag it, then let go" },
    { does: "Pause or resume", keys: ["KeyP"] },
    { does: "Help", keys: ["KeyH"] },
    { does: "Toggle full screen", keys: ["KeyF", "Escape"] }
  ]
};
