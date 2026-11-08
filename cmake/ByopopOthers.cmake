# The games the site recognises but does not play (docs/WEB_PLAYER.md, "Games this site does not play"; the user, 2026-11-07:
# "Let's also add site code explicitly recognizing folders that seem to contain a game we haven't/won't decomp, and either
# direct the user to an existing decomp project for it or just say 'coming soon'"). When the player picks or drops a folder that
# holds none of the site's games but one of these, the import names it: with a link to another project that plays it, or as
# coming soon. byopop_web_index() writes the table to other-games.js beside the pages, for web/gameimport.js.
#
# A row is <id>|<title>|<marker>,<marker>...|<project>|<url>:
#   id       the game's target as BYOPOP_WEB_UPCOMING has it, where it is there (such a row has no project: the site's own port
#            is coming); another short name otherwise
#   markers  files only that game has, relative to its folder, matched in any case as web/gameimport.js matches a game's own
#            markers (the first may be at any depth). Each was checked in the user's own copies (Steam's folders on A: and E:,
#            C:/Program Files (x86)/PopCap Games), read only, on 2026-11-07. The exe's name alone is not trusted: a download
#            and the Steam release may name it apart, so most rows pair it with a data file, or use two data files.
#   project  the other project's name, and url its front page; both empty: "coming soon"
#
# A project is linked only when its own front page and README say it plays the game, ship none of the game's files and need
# the player's own copy, and it is hosted on a trusted site (GitHub). Only the front page and README are read, to learn what
# it is and whether it works: never its code (CLAUDE.md, "No references", binds every port). A project whose state is unclear
# is not linked: the game is coming soon. Jungle Juice (web/jungle/) will host other projects' decompilations later; once it
# does, these links can point there.
#
# Checked 2026-11-07 (the READMEs):
#   Zuma Deluxe: alula/CircleShootApp, a decompilation of 1.0.0, "fully playable" with bugs expected; the player brings the
#     data files and bass.dll from their own copy.
#   Insaniquarium Deluxe: Vindirect/WinFish, a decompilation of 1.1, "fully playable" with bugs and inaccuracies; the player
#     copies data, properties, fishsongs and bass.dll from their own copy.
#   Plants vs. Zombies: wszqkzqk/PvZ-Portable, a cross-platform reimplementation of the GOTY edition, with releases; it holds no
#     game files, and the player brings main.pak and properties from their own copy.
#   Peggle Deluxe: teampopwork/Thunderball is a decompilation still in progress, and Peggle is one of the site's own games to
#     come, so it is coming soon; Peggle Extreme and the World of Warcraft edition go with it.
# Games of the era with no copy here to check their files against (Feeding Frenzy, Bejeweled Blitz and the like) are left out
# until there is one.
set(BYOPOP_OTHER_GAMES
  "zuma|Zuma Deluxe|levels/claw/claw.dat,levels/levels.xml|CircleShootApp|https://github.com/alula/CircleShootApp"
  "insaniquarium|Insaniquarium Deluxe|fishsongs/OSusanna.txt,images/Aquarium5.jpg|WinFish|https://github.com/Vindirect/WinFish"
  "plantsvszombies|Plants vs. Zombies|PlantsVsZombies.exe,main.pak|PvZ-Portable|https://github.com/wszqkzqk/PvZ-Portable"
  "venice|Venice|Venice.exe,main.pak||"
  "heavyweapon|Heavy Weapon|data/bosses.xml,data/craft.xml||"
  "hammerheads|Hammer Heads|boards/board001.xml,gnomedata/0.txt||"
  "bejeweled2|Bejeweled 2|properties/Backdrops.xml,data/BombFont.txt||"
  "astropop|AstroPop|data/bonus-1.xcf,images/_galaxy1.gif||"
  "typershark|Typer Shark!|data/CourierFinalDraft15Bold.txt,data/12ptfontglow.txt||"
  "pizzafrenzy|Pizza Frenzy|PizzaFrenzy.saf,PizzaFrenzy.exe||"
  "igglepop|Iggle Pop|IgglePop.saf,IgglePop.exe||"
  "chuzzle|Chuzzle|data/mindbenders.txt,data/gamedata.cfg||"
  "feedingfrenzy2|Feeding Frenzy 2|FF2.saf,FeedingFrenzyTwo.exe||"
  "peggle|Peggle|Peggle.exe,main.pak||"
  "peggleextreme|Peggle Extreme|PeggleExtreme.exe,main.pak||"
  "pegglewow|Peggle: World of Warcraft Edition|PeggleWoW.exe,main.pak||"
  "pegglenights|Peggle Nights|PeggleNights.exe,main.pak||"
  "bejeweled3|Bejeweled 3|Bejeweled3.exe,main.pak||"
  "bananabugs|Banana Bugs|BananaBugs.dat,BananaBugs.exe||"
  "gyromancer|Gyromancer|Gyromancer_0.dat,Gyromancer.exe||"
  "zumasrevenge|Zuma's Revenge!|ZumasRevenge.exe,main.pak||"
  "talismania|Talismania|Talismania.saf,Talismania.exe||")
