# Bookworm Adventures Volume 2: behaviour notes

Addresses are virtual addresses in `BookwormAdventuresVol2.exe` (v1.0.4.8465, image base 0x400000). `PORT_PLAN.md` has the
port's state, `ENGINE.md` the engine's layers.

## 1. A chapter's resources are never unloaded (the exe leaks them)

The scripts name a chapter's resource group by joining `Book`, the book's number, `Chapter` and the chapter's number
(`books/book.luc`, the chapter's resource list), so `Book0Chapter1`; the resources file spells the group `book0Chapter1`
(`properties/book0.xml`). The exe finds the group when it loads one (the game works, so that lookup must ignore case; it was not read),
but it compares with case when it deletes: `LuaResourceManager::DeleteResources` (0x61ae80, vtable slot 6) forgets the
Lua exports of the resources whose group equals the name, then runs `ResourceManager::DeleteResources` (0x536720, with the
per-map loop at 0x536650), and `DeleteExtraImageBuffers` (0x536770) does the same; every comparison is a
`std::string::compare` (0x404b20 over the byte compare at 0x402070). `UnloadResGroup("Book0Chapter1")` therefore deletes
nothing, neither the images nor their Lua exports, and the next load of the chapter finds them loaded and skips them. The
original leaks every chapter's images until the program ends; the port does the same, with both sides comparing exactly
(`framework/ResourceManager.cpp`, `luaapp/LuaResourceManager.cpp`). `GetNumResources` was not found in the export.

Cost in the browser: the soak (`soak <update>`, `TestHooks.cpp`) plays a new player from the prologue to the end of the
adventure, 485 words in 490,000 updates, in the web build under Node. The wasm heap, which only grows (the build has
`-sALLOW_MEMORY_GROWTH` and no maximum of its own, so Emscripten's default applies; Emscripten 6.0.12: 2 GB), went from 204 MB after loading to 813 MB
at the end, in steps of about 100 MB (8100: 204, 88100: 353, 208100: 509, 368100: 712, 488100: 813 MB). That is within the
default 2 GB maximum, and no run failed, so the exe's behaviour stays. It is a lot for a phone: a browser that kills tabs
near 1 GB would kill a long session. If that turns out to matter, freeing a chapter's images when it is left (comparing the
names without case) would be a deviation from the exe, to be taken as such.
