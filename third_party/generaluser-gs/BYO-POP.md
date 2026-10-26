# GeneralUser GS (General MIDI SoundFont)

GeneralUser GS v2.0.3 by S. Christian Collins, a Roland GS and General MIDI SoundFont bank, the General MIDI
instruments for the games whose music is MIDI (TipTop), played with TinySoundFont (`third_party/tinysoundfont`). Its
license (`LICENSE.txt`, License v2.0) lets software use it and lets the bank be modified and repackaged; it asks that a
website hosting it provide its own copy rather than link to the author's download files.

The full bank, `GeneralUser-GS.sf2` (32,319,396 bytes, SHA-256
`9575028c7a1f589f5770fccc8cff2734566af40cd26ed836944e9a5152688cfe`), came from
`https://github.com/mrbumpy409/GeneralUser-GS` at commit `684543d5e5efaef08d02be50dcda8d552478fa60` (2026-02-23),
downloaded on 2026-10-25 with the user's approval. It is kept outside the repository, in
`E:/retools/generaluser-gs-684543d/`, and is not committed. What the repository holds is cut down from it to the
presets a game's music uses, by a script that records which presets it kept, so the build and the web page carry a
few megabytes instead of 32. Each cut-down bank sits in this folder beside `LICENSE.txt`, and web pages that load one
list `LICENSES generaluser-gs` (`web/licenses/generaluser-gs.txt`).

`tiptop.sf2` (3,305,494 bytes, SHA-256 `f9e30575a12e81e05f3cb5bc42244d231e8a6499213fc048ac51f89ea2bbf97e`) is TipTop
Deluxe's (`games/tiptop`), made on 2026-10-25 by `python tools/sf2trim.py <GeneralUser-GS.sf2> tiptop.sf2 --midi
re/tiptop/dat/sounds/bkgd.mid --comment "TipTop Deluxe's music (games/tiptop)"` from the game's one MIDI file. It keeps
eight presets, Grand Piano (0:0), Vibraphone (0:11), Percussive Organ (0:17), Jazz Guitar (0:26), Acoustic Bass (0:32),
Trumpet (0:56), Trombone (0:57) and the Brush drum kit (128:40), and of them only the zones the song's notes reach (27
instruments, 66 samples); its INFO comment says the same. The song rendered with it and with the full bank differs by
at most one in 32767 (the test hook `musicwav`, `games/tiptop/docs/README.md`).
