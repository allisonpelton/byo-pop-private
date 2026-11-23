# Skins: the plan (approved by the user on 2026-11-23, less NT 4.0)

The site has one look, 95. This plan shows how other looks will be added without redoing it. It covers what a skin
holds, how the 95 look is wired today, what each new look needs, how to check them all, and the costs. The layer's first
batch (2026-11-23) is in: `<html data-look>` and `?skin=`, the sizes and the band as variables, and the drawings' colours
through `uriRules` (docs/WEB_PLAYER.md, "Desktop 95", "The skins"). The 98 skin followed the same day: web/skin-98.css, the
Skin list, and each skin's own style sheet loaded only where it is drawn. Then 2000's first step: the Standard line's
colours as 2000's own standard scheme, its frames and both title gradients, its banner, and menus that fade.

The scope is the user's (2026-11-23): "This is the first I'm learning of Classic. I see it was present up through Windows
7. I'd like all of those as skins to start; then the unique ones with XP up through 8 later", and "Oh also Windows Me".
Their rulings on the proposal, the same day: "As recommended, but skip NT 4.0, and the theme names should drop the word
desktop. Just the number/word. Ok to get installers and use Hyper-V if needed to examine, but they must be put on the E
drive".

The first skins are the classic family, seven in all:
- 95, as the site is now;
- 98 and Me;
- 2000;
- XP's Windows Classic style;
- Vista's and 7's Windows Classic.

NT 4.0 is skipped altogether: no skin, no variant and no banner of its own.

Later come the looks of their own: XP's Luna, Vista's Aero and Basic, 7's Aero and 8's.

Each skin follows the user's rule for the 95 look in its own era: the more authentic design of that version, in our own
drawings made in its manner.

## Where the facts come from

Every detail below was checked against the real versions, not taken from memory. Where no source settled a detail, it is
marked **to check**, and nothing is drawn from it until it is checked. The sources:
- **Pixel reads** in the browser of real screenshots: each image's colour counts, plus row and column samples. They were:
  - Wikipedia's screenshots of 98, Me, 2000 and XP with the Windows Classic style. The 98, Me and XP ones are scaled
    down, so edges blend, but flat areas are exact.
  - Wikimedia Commons' public-domain and CC0 shots at full size, where every pixel is exact: a Windows 98 message box,
    Vista's About box in its classic look, and 7's error box in Windows Classic, active and inactive.
  - The Start menu banners themselves, as the bitmap resources shown in Adam Demasi's article on the Start menu's banners
    (adamdemasi.com, 2024-07-24). 98's is shown at four times its size, so every pixel is exact; 2000's at a little over
    twice, read row by row as the share of each colour.
  - Commons' public-domain message boxes of Windows 2000, taken in VirtualBox: their frames row by row. They were taken in
    a low colour mode (solid titles, the face #d7d0c8, black for the dark edge), so they give the frame's rows but not its
    colours. 7's Windows Classic box, the same Standard scheme at full colour, gives the dark edge: #404040.
- **Text facts** from Wikipedia's articles:
  - the versions: Windows 98, Windows Me, Windows 2000;
  - the parts: Start menu, Taskbar;
  - the looks: Windows XP visual styles, List of features removed in Windows 7.
- **The two settings** below come from Microsoft's own pages:
  - Raymond Chen's "Why does Windows hide keyboard accelerators and focus rectangles by default?" (The Old New Thing,
    2006-10-05);
  - the Windows 2000 Resource Kit's "Disable personalized menus" (Microsoft Learn);
  - "Access Keys" (Microsoft Learn, previous versions).

## (a) What a skin holds

A skin is everything the 95 look draws that a version of Windows drew differently. The table says what each part holds
and whether the site draws it today.

| Part | What it holds | The site today |
|---|---|---|
| Colours and scheme | The window and button face, the 3D edges, the title bars (active and inactive, one colour or a gradient), the desktop, the selection | Drawn, from variables; 9 schemes of our own |
| Frames and title bars | Frame widths and colours row by row, title height, the caption buttons | Drawn |
| Controls and metrics | Push buttons (75x23 in dialogs), check boxes, option buttons, scroll bars, sliders, list boxes, fields, tabs | Drawn |
| The focus and defaults | The dotted rectangle, when it shows, the default button's frame | Drawn (item 10) |
| The task bar and the Pop menu | Their layout, the Pop button, the band, the tray, the clock | Drawn |
| Fonts | The interface font at its sizes | Drawn: Byopop Sans, our pixel font in the manner of MS Sans Serif |
| Icons | The apps' icons, the Settings sections' icons, the message box signs | Drawn: our own, 17 apps and 11 sections at 32 and 16 px |
| Cursors | The pointer and its kinds | One of our own: an hourglass while the desk starts and a game loads (skins.css, a drawing in a data address). Otherwise the browser's pointers; a game's own cursors stay the game's |
| System sounds | Opening, closing, errors | The games' own sounds through the desktop themes; not part of a skin |
| The default wallpaper | The desktop's colour or picture | Drawn: the desktop colour, the patterns and our wallpapers |
| Behaviours | Menus that slide or fade, hidden keyboard cues, personalized menus, menu shadows, task grouping | None yet; 95 had none of these |

## (b) How the 95 look is wired today, and what moves

- **web/skins.css** (1,367 lines) holds the look. It has 185 `:root[data-skin="d95"]` rules and 450 `html.d95-desk`
  rules. Its colours are mostly variables already (`--panel`, `--sk-hi`, `--sk-lo`, `--sk-dk`, `--sk-mid`, `--sk-title`,
  `--sk-band`). Its 32 drawings are SVG in data addresses: caption buttons, scroll arrows, check marks and the box signs.
- **web/schemes.js** (316 lines) holds the schemes. It sets those variables and rewrites the SVG drawings with a scheme's
  colours (`uriRules`), since a data address cannot read a variable. Since item 10 each scheme keeps two title colours;
  the 95 look draws the first, and a gradient look would draw both.
- **web/skins.js** (2,612 lines) is the window manager and builds the desk's markup: windows, the task bar, the Pop menu
  and its band, icons, message boxes. Its markup uses look-free classes. A few details in it are the 95 look's own, such
  as the band's two-colour name.
- **The icons** are web/appicons.js and web/settingsicons.js. tools/appicons.py and tools/settingsicons.py draw them in
  16-colour pixel art.
- **The font**, web/fonts/byopop-sans.woff, is made by tools/web/pixelfont.py.
- **Hard-coded 95 colours** remain outside skins.css, in addgames.js, appicons.js (22), controls.js, fontview.js,
  library.js, music.js, schemes.js, settings.js, settingsicons.js (16), skins.js, sysinfo.js and index.html.

What moves behind the skin layer:
- **Colours** that are still hard-coded become variables.
- **Metrics** become variables: title height, frame rows, caption button size, dialog button size, menu item height and
  the task bar's height.
- **The drawings** become one description per drawing, written out per skin with that skin's colours. This is the
  `uriRules` idea made general, and a tool can generate them.
- **The band** becomes data: its colours and their steps, its width, and how its text is lettered.
- **The font** becomes one `@font-face` per skin.
- **The hourglass cursor** becomes one drawing per skin, each from that version's own (to check per version).
- **The icons** become one set per manner, from the same icon tools given the manner as a parameter.
- **Behaviours** become flags a skin sets, read by skins.js and the CSS, each with the setting that turns it off where the
  version had one (below).

The `d95-` class prefix and `data-skin="d95"` stay as the desk's names. Renaming 600 or more selectors would cost churn
and buy nothing.

## (c) What stays shared

The skin changes how things look, never what they do. These stay shared across skins:
- the window manager: windows, focus, keys, Alt+\`, dragging and sizing;
- every window's layout and logic, and the strings;
- the games' pages and the games;
- the Library's own olive look and the Music player's;
- the schemes' mechanism and the player's settings;
- the byo-pop logo.

A skin is a file of variables and drawings plus a short list of builders where a version's layout differs (below). It is
loaded only when chosen, so a page carries one skin's weight.

## (d) How a viewer picks a skin

95 stays the default.

None of 95, 98, Me or 2000 had a way to change how windows were drawn, beyond colours and sizes. So no control of the
era fits exactly. The nearest in kind is the Appearance tab's own lists, which our Appearance window already copies with
its Scheme list. The plan therefore adds a **Skin** list there, above Scheme. It is a drop-down in the chosen skin's own
drawing, listing the skins by name. The site's text already calls them skins ("Other skins list every game"). A choice
redraws the desktop at once and is kept with the other settings.

XP had a control for exactly this: its Windows Classic style is chosen as one style among XP's own (Wikipedia, "Windows XP
visual styles"). Under the XP skins, the Appearance window takes XP's form, a list of styles (Windows Classic style, and
later Luna's), while the Skin list keeps the version. The labels of XP's lists, and how 7's Personalization window
offers "Windows Classic" among its themes, are **to check**.

Tests choose a skin with `?skin=` in the address, as `?silent=1` does for sound.

## (e) The skins

Most of the family shares one drawing: the same frames, buttons and controls, differing in colours, metrics, the band and
the fonts. So the plan is **one classic renderer**, with each version's differences as data. It cannot be data where a
version's layout is a different design: XP's two-column Start menu, Vista's Start menu, 7's task bar, grouping on the task
bar, and personalized menus. Those are a builder each in skins.js, chosen by the skin.

The real colours fall into two lines, plus Vista:
- **#c0c0c0 on #008080**, the 95 line. 95 has solid titles; 98 has a gradient from #000080 to #1084d0.
- **#d4d0c8 on #3a6ea5**, the Standard line: Me, 2000, XP Classic and 7 Classic, with titles from #0a246a to #a6caf0
  active and #808080 to #c0c0c0 inactive. Wikipedia names this "Windows Standard", the default of 2000 and Me; 7 renamed it
  "Windows Classic" and dropped the others.
- **Vista's "Windows Classic"** goes back to the 98 line: its classic About box reads #c0c0c0 with #000080 to #1084d0.

Every banner is a 21-pixel bitmap in 16 colours.

What each skin shares and what is its own:

| Skin | Shares with the family | Its own |
|---|---|---|
| 95 | The classic frames, buttons and controls | The base: solid titles, the grey band, MS Sans Serif's manner, 95's icons |
| 98 | 95's scheme line and font | The gradient titles, its banner, sliding menus, its icons (to check) |
| Me | 2000's scheme | Its banner, personalized menus; font, icons and hidden cues to check |
| 2000 | The Standard scheme | Tahoma's manner, its banner, fading menus, hidden cues, personalized menus, its icons |
| XP Classic | All of 2000 | The two-column Pop menu, grouping, tray hiding, XP's icons, menu shadows (to check) |
| Vista Classic | 98's scheme | Its banner, Vista's Pop menu and task bar drawn classic, its icons, its font (to check) |
| 7 Classic | 2000's scheme | 7's Pop menu and task bar drawn classic, its icons, its font (to check) |

**The order.** The layer comes first. Then 98, as the second skin that proves the layer: it changes the fewest parts, so a
fault in the layer shows plainly against 95. Then 2000, which brings the Standard line, the new font and the behaviours.
Me, XP Classic, Vista Classic and 7 Classic build on those two, in that order.

The estimates below are in site batches, the size of item 10 (about 150 calls). At this session's size that is roughly
$20 to $40 a batch at API prices. They are estimates, not measurements.

### The layer itself, before any second skin

Move the 95 look behind the layer:
- the variables for metrics and the remaining colours;
- the drawings generated per skin;
- the band as data;
- the font and icon sets per skin;
- the Skin list;
- `?skin=`;
- a shot set per skin (f).

The 95 look must come out pixel for pixel the same, checked by shotdiff against today's shots. **3 to 4 batches, $80 to
$150.**

### 95

95 is the site as it is, after item 10. Its banner is a solid #808080 band with "Windows" in #c0c0c0 and "95" in white
(pixels of a real Start menu and of the resource).

### 98

The 95 renderer with 98's data:
- **Title bars:** the two-colour gradient, which 98 brought (Wikipedia). Real pixels give #000080 to #1084d0, 18 pixels
  tall, and the frame's rows #dfdfdf, then #ffffff, then the face #c0c0c0.
- **The desktop:** #008080.
- **The banner:** navy #000080 with royal blue #0000ff at its foot, the name in white.
- **Menus and tooltips slide** (Wikipedia).

Drawn (2026-11-23, web/skin-98.css): the gradient titles, the banner, and the sliding menus and tooltips, at a speed of our
own (0.18 s; 98's own is **to check**). 95's frame already had 98's rows. An inactive window's title bar stays 95's solid
grey until 98's is read from a real shot (**to check**).

**The banner, read row by row** from the bitmap at four times its size: 262 rows of navy, a dither up to royal blue between
rows 163 and 210, blue to row 232, and a dither a little back toward navy at the foot (about a fifth blue in its last row),
the name in white. In the Start menu the bitmap sits at the menu's foot with its top colour above it. The first draw had a
smooth gradient over the whole band; 2000's step drew it as the bitmap is, our own dither of its colours and steps
(web/skins.js, `bandRamp`).

**To check before drawing:**
- whether 98's caption buttons, scroll bars and check marks differ from 95's;
- flat toolbar buttons that rise under the pointer (no source found);
- 98's icons and how their manner differs from 95's.

Quick Launch came with Internet Explorer 4's desktop update, so some 95 copies had it too (Wikipedia, "Taskbar"). Our task
bar has nothing to launch there, so the plan leaves it out. **1 to 2 batches, $30 to $60, plus icons (below).**

### The behaviours that hide things, as each version's defaults

The user ruled that the two behaviours that hide things are drawn as the versions' defaults, each with that version's own
setting to turn it off. Both settings existed:

**Personalized menus** came with 2000's Start menu, and Me took them from 2000 (Wikipedia, "Windows 2000" and "Windows
Me"). Windows 2000 "personalizes long menus by moving recently used items to the top of the menu and by hiding items that
have not been used recently"; an arrow at the menu's foot shows the hidden ones. The setting to turn it off is the **Use
Personalized Menus** box on the General tab of the Taskbar and Start Menu Properties (the Windows 2000 Resource Kit,
"Disable personalized menus"). Me's setting has the same name (Sony's support pages for its Me computers, "Why can't I
enable the settings of [Use Personalized menus] on Windows Me?").

On the site, the Pop menu's Games list and its long submenus would move recent games up and hide those not played lately.
The setting would go where those versions put it. The task bar's own menu (Cascade, Tile, Minimize All) gains
**Properties** at its foot, as Windows' had. It opens a small "Taskbar and Pop Menu Properties" window, with the Use
Personalized Menus box under the 2000 and Me skins.

**Hidden keyboard cues** came with 2000. Raymond Chen quotes the setting as "Hide underlined letters for keyboard
navigation until I press the Alt key", on by default for most Western languages, set in the Display control panel. The
designer's own rationale, which he quotes, hides both the access keys' underlines and the **focus rectangles**. They come
back once the user shows keyboard use. So under 2000 and later skins, the dotted focus rectangle that item 10 shows from
the first click in 95 shows only after a key, as the browser's own `:focus-visible` does.

XP kept the setting, under Display, then Appearance, then Effects (Microsoft Learn, "Access Keys"). Its place in 2000's
Display control panel is **to check**, and so are its place in Vista and 7 and whether Me had it at all. On the site the
box goes in the Appearance window, where those versions put it, with the version's wording.

### Me

The Standard line:
- **Colours:** face #d4d0c8 and desktop #3a6ea5 (pixels of a real Me desktop).
- **The banner:** black at the top, then #000080, then #0000ff, with the name and "Millennium Edition" in white.
- **Personalized menus,** on by default, with Use Personalized Menus to turn them off (above).

**To check:** Me's title gradient (no window shows in the shot), whether Me took Tahoma, whether it had the hidden cues,
and whether its icons are 2000's. **1 to 2 batches, $30 to $60**, less if its font and icons are 2000's.

### 2000

The Standard line in full:
- **Colours:** face #d4d0c8, desktop #3a6ea5, titles #0a246a to #a6caf0 and #808080 to #c0c0c0, 18 pixels.
- **The frame:** rows #d4d0c8 then #ffffff.
- **The font:** Tahoma, which 2000 brought in place of MS Sans Serif (Wikipedia, "Windows XP visual styles").
- **The banner:** dithered from black at the top through #000080 to #0000ff, with the edition in white.

Its behaviours:
- **Fading menus:** "Menus support a new Fade transition effect" (Wikipedia, "Windows 2000", citing Microsoft's page on
  layered windows). Whether tooltips fade too is **to check**.
- **Hidden keyboard cues and focus rectangles,** on by default, with their setting (above).
- **Personalized menus,** on by default, with their setting (above).

The builder for personalized menus and the Properties window come with this skin. **2 to 3 batches, $50 to $110, plus the
font (below).**

**Drawn first (2026-11-23, web/skin-2000.css and its standard scheme in web/schemes.js):**
- **The colours** as 2000's own standard scheme: the face #d4d0c8 with its soft light the face too, the desktop #3a6ea5,
  the selection #0a246a and the dark edge #404040 (the sources above). A scheme of ours keeps its colours under 2000, with
  2000's dark edge.
- **The frames:** #d4d0c8 then white on the top and left, #404040 then the shadow on the bottom and right, from 2000's
  own boxes (two rows each side; 7's frame adds a black row outside, which 2000's has not). The drawings' edges take the
  dark edge too; glyphs and the focus rectangle stay black.
- **Both title bars' gradients,** active #0a246a to #a6caf0, inactive #808080 to #c0c0c0 with its text in the face colour.
- **The banner,** read row by row from 2000 Professional's bitmap: 233 rows, black for 12, a dither to navy by row 104,
  navy to row 125, a dither to royal blue by row 209, then blue, the name in white; black above it.
- **Menus fade in,** for a length of our own (0.2 s; 2000's is **to check**). Tooltips do not, until it is checked whether
  they faded.

**The hidden keyboard cues** followed (web/skins.js, `cuesSync`; the skin's behaviours are `<html data-behaviours>`). Under
2000 the focus rectangles are hidden until Alt, Tab, an arrow key or F10 is pressed, and no menu here underlines access
keys. The setting is Microsoft's own wording, "Hide keyboard navigation indicators until I use the ALT key" (the policy
that removes it says it sits in Display in Control Panel; Internet Explorer's documentation says the feature came with
2000), on by default. Its row is in Appearance and shows only in a skin that hides its cues. Which tab of Display held it
is still **to check**.

**Personalized menus** came next (web/skins.js, `fillSub`; the behaviour "personal"). Under 2000 the Pop menu's Games
list puts the games played in the last 30 days first, the last played at the top, and hides the rest behind an arrow at
the list's foot, two chevrons down; the arrow shows the whole list in its usual order until the Pop menu is opened again.
The 30 days are ours: Windows kept its own record of each program's use, which the site has no match for, and
ByopopPlay keeps each game's last day. With no game played lately, or every game, the list is whole, as a new 2000 user's
was. The task bar's menu gains **Properties** at its foot under 2000, which opens "Taskbar and Pop Menu Properties" with
its one box, Use Personalized Menus, checked by default, OK the default button (the setting `personalMenus`). 95's menus
and shots are unchanged. Only the Games list is personalized; the Settings submenu, a short fixed list, stays whole.

**Still to come for 2000:** the font in Tahoma's manner (still Byopop Sans), and its icons.

### XP's Windows Classic style

"Windows Classic style" is its official name (Wikipedia). A real XP shot in that style has 2000's colours exactly. It
differs from 2000 in these parts:
- **The Start menu:** XP's two-column menu, drawn classic. The user's name sits in a band in the active title gradient
  (pixels). XP's older one-column menu stays as an option (Wikipedia, "Start menu"), with its own blue-to-black banner.
- **The task bar:** XP brought grouping of a program's buttons, and the hiding of inactive tray icons (Wikipedia,
  "Taskbar").
- **The icons:** XP's new ones, which replaced those in use up to 2000 and Me (Wikipedia, "Windows XP visual styles").
- **Menu shadows:** no source found; **to check**.

The plan makes it the 2000 skin with those parts swapped: a builder for the two-column menu, grouping and tray hiding, and
XP's icons. **3 to 4 batches, $70 to $150, plus icons.**

### Vista's Windows Classic

Vista's "Windows Classic" scheme is 98's: face #c0c0c0, titles #000080 to #1084d0 (a real classic About box). 2000's
Standard scheme was on Vista too (Wikipedia). The one-column classic Start menu is still there (Wikipedia), with one
banner for every edition: a blue running from about #029efb at the top to #011892 at its foot, with the name in white.

**To check:**
- Vista's own Start menu and task bar drawn in Windows Classic. No real shot of either was found, and they are new
  designs, the search box included. Where to find the references is below.
- Its font in classic: Tahoma or Segoe UI.

**3 to 5 batches, $70 to $180, plus icons.** It is the riskiest, for the missing references.

### 7's Windows Classic

7's "Windows Classic" is 2000's Standard renamed; the other classic schemes were removed (Wikipedia, "List of features
removed in Windows 7"). Its real error box reads:
- face #d4d0c8;
- titles #0a246a to #a6caf0 and #808080 to #c0c0c0, 18 pixels;
- 7's own icons (the red error circle).

7 has no classic Start menu (Wikipedia). Its task bar is a new design (Wikipedia, "Taskbar"):
- large icon buttons and pinning;
- names hidden by default;
- grouping that cannot be turned off, though combining can.

**To check:** its Start menu and task bar drawn classic (no real shot found; references below) and its font. **4 to 6
batches, $90 to $220, plus icons.**

### References for Vista's and 7's Start menu and task bar

**Wikimedia Commons first.** Its file search (the API's `list=search` in namespace 6) holds whole public-domain sets of
VirtualBox screenshots of these versions. Shots of the Start menu and task bar in Windows Classic are searched there
before either skin is drawn.

**Failing that, a Hyper-V machine** runs the version to examine it, by the user's leave (2026-11-23). Its rules:
- **Nothing in Windows changes.** Hyper-V is already on.
- **Everything goes on E:, under the existing E:\byo-og** (for example E:\byo-og\vm): the installers, the virtual disks and
  the machine's own files. That means passing New-VM's -Path and -NewVHDPath, since Hyper-V's defaults are on C:. Never
  at a drive's root.
- **No network.** The machine gets no virtual switch.
- **Installers only from Microsoft's own site.** Before any download, its name, source and size go to the coordinator for
  the user's yes. A mirror or an archive copy is not trusted.
- **Shots of the machine's screen only,** never the user's desktop. Hyper-V's own thumbnail call
  (Msvm_VirtualSystemManagementService's GetVirtualSystemThumbnailImage) reads the screen with no window open, and is to be
  tried first.
- **Input through Hyper-V's own keyboard and mouse,** so the user's cursor and keyboard are never touched: the Msvm_Keyboard
  and Msvm_SyntheticMouse classes. To be tried.
- **When done,** the machine is shut down and its files kept or deleted as the user says.

Whether Microsoft still offers Vista's and 7's installers, and the product keys they would need, is **to check** before
anything is asked for. If it offers neither, those two skins wait until references are found.

### Shared drawing work

**Icons.** There are 28 icons (17 apps, 11 Settings sections), at 32 and 16 pixels, for each manner. 95's manner exists.
98's and 2000's manners are **to check** against real icons; XP's, Vista's and 7's need far more colours and shading. A
manner costs roughly **1 batch for 95's and 98's palettes, and 2 to 3 batches for XP's, Vista's and 7's**, so $200 to
$400 across the family.

**A font in Tahoma's manner, of our own.** Byopop Sans is ours, built from W95FA (SIL OFL). No open pixel font in
Tahoma's manner has been found yet. Making one is **1 to 2 batches, $30 to $80**. Vista's and 7's fonts may add one more.

**The family in total:** about 20 to 30 batches, **$600 to $1,300**. That is a few weeks of the site's share of the
window. A Hyper-V machine, if it is needed, adds about a batch per version.

### Later: the looks of their own

XP (Luna), Vista (Aero, and Basic if it proves to differ enough to count as its own), 7 (Aero) and 8 come after the
family. None of them is the classic drawing. Each needs a renderer of its own: frames, title bars and controls drawn as
images, not as rows of colour. Aero adds glass, which the browser can only imitate, and blur behind a window costs drawing
time on every frame. The layer above is built so that they fit: a skin chooses its renderer, its drawings and its
builders. No detail of them has been checked yet, so no estimate is given. Each would be priced, from real shots, when the
family is done.

## (f) The checks and shots per skin

The base skin, 95, is checked in full in every batch, as now: deskcheck, pagecheck, the shotdiff baselines and the batch's
own shots.

Each other skin gets a fixed **shot set** with its own baseline, about ten shots:
- the desktop with the Pop menu open;
- a message box with its default;
- Setup;
- Settings and Appearance;
- a game page's start card;
- the narrow desktop;
- an inactive window.

A driver opens these in the pane with `?skin=`, and shotdiff compares them.

A batch reruns the sets of the skins whose differing parts it touches. If a batch changes the shared renderer (frames,
buttons, controls), all six other sets run. If it changes only a layout every skin shares, none do: the 95 baseline covers
it. The run is unattended. Only a flagged difference costs a judgment. Per skin a set takes a few minutes of the pane, and
costs little unless something is flagged. So a typical batch adds $2 to $10, and a renderer batch adds $10 to $30.

deskcheck runs once per skin when a skin is added, since the windows' structure is shared and the skins only restyle it.

## (g) The name and logo under other skins

Each skin is named by its version alone, as the user ruled:
- 95, 98, Me and 2000;
- XP Classic, Vista Classic and 7 Classic;
- later, for the looks of their own: XP, Vista, 7 and 8.

**Where "Desktop 95" changes.** The current look's name shows in one place a player reads: the hint of Settings' "Windows
95 games only" (`settings.win95only.hint` in web/strings/en.txt), "In Desktop 95, list only the games…". It becomes "In the
95 skin, …" in the layer's batch. Everywhere else the name is in code comments, the strings file's comments and
docs/WEB_PLAYER.md's section on the look, and those follow in the same batch. The internal names (`d95`, `data-skin`)
stay.

The Pop menu's band always carries "byo-pop", drawn in that version's banner manner: its colours, its steps, and the
weight and letter shapes of its lettering. The 95 band's name is two-toned as Windows 95's was. The byo-pop logo at the
desktop's bottom right is the same under every skin.

## (h) Costs and risks, plainly

- **Money.** The family costs roughly $600 to $1,300 to build. After that, every site batch that changes something drawn
  costs more: it changes up to seven drawings and reruns their shots.
- **Missing references.** No real shot of Vista's or 7's Start menu, or of 7's task bar, in Windows Classic has been found.
  Drawing them from memory would break the rule. Commons comes first; a Hyper-V machine is the fallback, if Microsoft still
  offers the installers.
- **Fonts.** The Tahoma-era skins look wrong without a font in that manner, and it has to be our own.
- **Icons** are the largest drawing cost, above all for XP, Vista and 7.
- **Regressions in 95.** Moving the details behind the layer could change the 95 look. The shotdiff baselines, which
  must stay identical, guard it.
- **Behaviours that hide things.** Under 2000 and later, personalized menus hide games not played lately, and the hidden
  cues hide underlines and focus rectangles until a key is used. Both are authentic and make the site harder to use at
  first. Each has its version's setting to turn it off, and 95 and 98 have neither.
- **Page weight.** Only the chosen skin's file loads, so each page carries one skin.

## What is settled and what is left

The user's rulings of 2026-11-23 settle what the proposal asked:
- the plan is approved in its scope and order, less NT 4.0;
- NT 4.0 is skipped altogether;
- personalized menus and hidden cues are drawn as the versions' defaults, each with its setting;
- the names are the versions alone;
- Vista's and 7's references come from Commons, else a Hyper-V machine on E: under the rules above.

Nothing waits on the user now. The **to check** items are read from real shots before the skin they belong to is drawn.
Any download for a Hyper-V machine comes back to the user first, with its name, source and size.
