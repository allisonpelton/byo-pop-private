# Starting the native ports from Steam (planned)

Planned alongside the website (the user, 2026-11-01). Nothing is built yet.

A player who owns a game on Steam runs an installer script that puts our native port next to the original in the
Steam folder, much as SMAPI does for Stardew Valley. The player then points the game's launch options at the port.
Steam goes on treating it as the game, so the player keeps the overlay, play time, F12 screenshots, Steam Input and,
where the original has them, achievements. Mods then run natively with all of that too.

## The installer

The script finds the game through Steam's library files (`steamapps/libraryfolders.vdf` and the game's
`appmanifest_<appid>.acf`). It checks that the original's files are the version the port was made from, or a version the
Collection-disc step has shown to work. It copies the port's exe and its DLLs into a subfolder of the game folder (say
`byopop\`). Steam's "Verify integrity of game files" ignores files it does not own, so they stay. The script then shows
the launch option to paste into the game's Properties, `"<game folder>\byopop\<game>.exe" %command%`, and puts it on
the clipboard: a script cannot safely change launch options while Steam is running. Removing the port means deleting
the subfolder and clearing the launch option. The Bejeweled Deluxe window fix (cnc-ddraw, kept out of the repo in
`build/steam-fixes/`) is the model for finding the install and for a script players can run with a double-click.

Steam appends the original's command line after `%command%`, so each port ignores it. The port reads the original's
data where it lies; Steam's Bejeweled Deluxe keeps its folders loose. A game whose data is held inside its wrapper exe
cannot use this in public, since tools that unpack a wrapper stay private (`.publishignore`).

## Overlay and achievements

The overlay needs no work: Steam injects it into whatever it launches, and it draws over the Direct3D or OpenGL that
SDL uses.

Achievements are possible only for games that already have them on Steam. A port can unlock the ones the publisher
defined, never new ones. Among the user's PopCap games, only Bejeweled 3 and Zuma's Revenge ship `steam_api.dll`
(Plants vs. Zombies too, which we do not port); the old wrapped Deluxe games have no Steam API at all. The port loads
the game's own `steam_api.dll` at runtime and declares the few functions it calls itself, so we ship nothing of
Valve's, and Steam tells a game it launched which app it is. Each port reports an achievement at the moment its
original does, so play stays faithful. The Steam code lives in `platform/` and stays off unless Steam started the
game, like the site's own hooks.

## Open questions

Whether achievements unlock while gameplay mods are on is the user's decision; the recommendation is unmodded play
only, matching "plays unchanged with no mod on". Valve's Steamworks licence is written for Steam partners, so its terms
are read before anything that calls `steam_api.dll` is published. Whether a port keeps its own saves or shares the
original's, so a player can switch between them, is still to decide.
