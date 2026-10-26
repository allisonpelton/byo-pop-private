# TinySoundFont (SoundFont synthesizer and MIDI loader)

TinySoundFont by Bernhard Schelling, `tsf.h` (the SoundFont 2 synthesizer) and `tml.h` (the MIDI file loader), from
`https://github.com/schellingb/TinySoundFont` at commit `853a0a171759f1ddba0de1442133a75912bbeffa` (2026-07-19),
downloaded on 2026-10-25 with the user's approval for TipTop's music. MIT (`LICENSE`).

| File | Bytes | SHA-256 |
|---|---|---|
| `tsf.h` | 92,719 | `70d55963c98f60ebb81518eaa1f25d46888d5180eb5f5289fd6b74ffc177d197` |
| `tml.h` | 20,463 | `93257db259e0efb2ea2037d7157841bec8cb4a2d7986286e43c8090705326546` |
| `LICENSE` | 1,155 | `ef10cb839de73fb6d5d32ea7151979e87809a6df70dc7e7ee8457ae1c9e7aa7e` |
| `README.md` | 1,711 | `a04b6331e826ef6f3ee893fadc04a2d145ab62e7839e8b3056619ea1178a9ed0` |

Copied unchanged (git stores them with LF line ends). Left out: the examples, `sfotool` and the project files. Both
are single-header libraries: `third_party/CMakeLists.txt` makes them the interface target `tinysoundfont` (the include
path only), and the one source file of a game that uses them defines `TSF_IMPLEMENTATION` and `TML_IMPLEMENTATION`
before including them. Its General MIDI sound bank is GeneralUser GS (`third_party/generaluser-gs/BYO-POP.md`).
Web pages that use it list `LICENSES tinysoundfont` (`web/licenses/tinysoundfont.txt`).
