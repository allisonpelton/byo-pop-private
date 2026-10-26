# The 2003 framework as Atomica uses it, and what the port does

Atomica Deluxe (2003-03-28) was built on the SexyAppFramework of its day: one class `SexyApp : ButtonListener` with 55 virtual
slots, flat widgets (0x58 bytes, 43 slots: the 2003 widget has no children, a screen that has buttons adds them to the
widget manager itself), a non-virtual `Graphics` and a `ButtonWidget` that draws itself. The shared port is built on the 2004
framework; these are the places where Atomica's code needed something different. Noah's Ark's `FRAMEWORK_2002.md` has the
differences the two games share.

- **Mouse buttons.** The widget's `MouseDown(x, y, click)` of 2003 takes 1 for a left click, 2 for a double click and -1 for a
  right click; the port's handler gets `(x, y, button, clickCount)` and maps it (`Grid::MouseDown`).
- **Widget flags.** The bytes at +0x1e and +0x1f of a 2003 widget are `mIsDown` and `mIsOver`; the field deselects an atom on a
  release when the mouse is not over it, and the board's buttons are invisible `ButtonWidget`s (`mBtnNoDraw`) whose pictures the
  board draws by `mIsDown` and `mIsOver`.
- **Child buttons.** A screen with buttons (the board, the main menu, the title screen) lists them and adds and removes them in
  `AddedToManager` and `RemovedFromManager`; the screen must not be brought to the front afterwards, or it covers its buttons.
- **Drawing order.** The board draws the Grid's picture inside its own (`Grid::DrawField`), so the Grid widget itself draws nothing.
- **The loading.** `LoadingThreadProc` runs in the main thread after the first frame (the port has no thread); it draws the
  title screen's bar itself now and then (`PumpLoading`).
- **Sound and music.** `Sound::Play` is `PlaySample` on the id the sound got when it was loaded; the music is one module,
  and `PlayMusic(0, order, false)` starts it at an order (0xc strategy, 0x2c action, 0x18 bonus levels, 0x42 the game over).
- **Registry and files.** The saved game and the high scores are files in the user's folder (`SAVEGAME.DAT`, `HS.CFG`); the
  port reads and writes them through the framework's buffer functions so the web build keeps them in its persistent storage.
