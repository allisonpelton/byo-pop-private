// What differs in Mahjong Escape: Ancient Japan from Ancient China: the names, the counts, the places and sizes on the
// screens, the switches of the features only one edition has, and the tables that the code shared by both
// (engines/mahjongescape) reads. Each game has its own Edition.h and Edition.cpp with the same names; a value is
// the exe's own, found by comparing the two.

#ifndef __MAHJONG_EDITION_H__
#define __MAHJONG_EDITION_H__

#include "Res.h"
#include "Font.h"
#include "Object.h"
#include "GameText.h"
#include <cstdint>

// The game's names; macros, so that they join the literals around them
// the game's folder under assets/, the mods folder's name and the name of its saves
#define EDITION_GAME_FOLDER "mahjongjapan"
#define EDITION_EXE_NAME "MahjongEscapeAJ"  // the original exe's name
#define EDITION_EXE_FILE "MahjongEscapeAJ.exe"  // the original exe, which holds the menus' texts
#define EDITION_GAME_TITLE "Mahjong Escape: Ancient Japan"  // what the game is called in the dialogs of the host
// the window's title: the exe's own (China's calls itself only Mahjong Escape)
#define EDITION_APP_TITLE "Mahjong Escape: Ancient Japan"
#define EDITION_APP_PRODUCT "Mahjong Escape Ancient Japan"  // the product's name, which makes the registry key
#define EDITION_INSTALL_FOLDER "Mahjong Escape Ancient Japan"  // the game's folder in the PopCap Game Collection
#define EDITION_WEB_FOLDER "mjpersist"  // the web page's persistent folder

// The features only one edition has, for #if: 1 there and 0 in the other
#define EDITION_DRAGON_CORNERS 0  // the dialog frame's corner of kind 2, with a dragon (China)
#define EDITION_RESTART_BOX 1  // the box that asks whether to start the escape again once it is won (Japan)
// the map's last stages have six and seven levels, and a stage's end is a table's (Japan)
#define EDITION_UNEVEN_STAGES 1
#define EDITION_ESCAPE_TITLE_FONT 0  // the panel has its own font for the escape's title (China)
#define EDITION_TIPS_OFF 1  // no tips on a level that has a best score (Japan)
#define EDITION_MENU_RESTART 0  // the in-game menu has a restart (China)

namespace mahjong
{
namespace edition
{

// The player's lists (Players.cpp)
constexpr int PLAYER_BOX_SELECTION = 0x4e9bac;  // the colour of the selected name
constexpr int PLAYER_BOX_FRAME_1 = 0x2149;  // the box frame's first colour
constexpr int PLAYER_BOX_FRAME_2 = 0x5289bf;  // the box frame's second colour

// The player's records (User.h, User.cpp)
constexpr int NUM_LAYOUT_RECORDS = 175;  // the slots of the layouts' best times and scores
constexpr int NUM_LEVELS = 60;  // the escape's levels
constexpr int DEFAULT_QUAD_PERCENT = 0;  // the share of fours in a new player's deals

// The main menu (MainMenu.cpp)
constexpr int MENU_HINT_X = -6;  // the caption of a button: its offset from the middle
constexpr int MENU_HINT_Y = 0x78;  // the caption of the two big buttons
constexpr int MENU_PLAYER_HINT_Y = 0x69;  // the caption of the player's button
constexpr int MENU_WELCOME_Y = 0x1ec;  // the welcome line
constexpr int MENU_LANTERN_LEFT_X = 0x1c;
constexpr int MENU_LANTERN_RIGHT_X = 0x280;
constexpr int MENU_LANTERN_Y = 0x20;
constexpr int MENU_LANTERN_WIDTH = 0x82;  // the width of a lantern's frame
constexpr int MENU_ADVENTURE_X = 0xc0;
constexpr int MENU_ADVENTURE_Y = 0xe2;
constexpr int MENU_ADVENTURE_LABEL_X = 0;
constexpr int MENU_ADVENTURE_LABEL_RIGHT = 0x198;
constexpr int MENU_ADVENTURE_LABEL_Y = 0x34;
constexpr int MENU_CLASSIC_X = 0x115;
constexpr int MENU_CLASSIC_Y = 0x158;
constexpr int MENU_CLASSIC_LABEL_X = 0;
constexpr int MENU_CLASSIC_LABEL_RIGHT = 0xf0;
constexpr int MENU_SMALL_Y = 0x218;  // the options, instructions and exit buttons
constexpr int MENU_SMALL_LABEL_Y = 0x19;
constexpr int MENU_OPTIONS_X = 0xb2;
constexpr int MENU_OPTIONS_LABEL_X = 0;
constexpr int MENU_OPTIONS_LABEL_RIGHT = 0x72;
constexpr int MENU_INSTRUCTIONS_X = 0x140;
constexpr int MENU_INSTRUCTIONS_LABEL_X = 0;
constexpr int MENU_INSTRUCTIONS_LABEL_RIGHT = 0x9b;
constexpr int MENU_EXIT_X = 0x1fe;
constexpr int MENU_EXIT_LABEL_X = 4;
constexpr int MENU_EXIT_LABEL_RIGHT = 0x6b;
constexpr int MENU_PLAYER_BUTTON_Y = 0x200;
constexpr int MENU_SMALL_FONT = FONT_BUTTON;  // the font of the small buttons' labels
constexpr int MENU_SMALL_FONT_MAP = FONT_BUTTON_MAP;

// The classic game's layout list (LayoutSelect.cpp)
constexpr int NUM_LISTED_LAYOUTS = 0xa2;  // the layouts of the list
constexpr int LIST_FIRST_RANGE = 125;  // the list's layouts from id 437 on; Japan's others are from id 600 on
constexpr int BACKGROUNDS_AT_START = 9;  // the backgrounds a new player can choose, before the stages add theirs
constexpr int MAX_FIRST_BACKGROUND = 0xf;  // the first of the last three backgrounds of the list's background page
constexpr int LAYOUTS_TITLE_Y = 0x10;
constexpr int LAYOUTS_PREV_PAGE_X = 0x67;  // the page captions
constexpr int LAYOUTS_NEXT_PAGE_X = 0x2b5;
constexpr int LAYOUTS_PAGE_Y = 0x233;
constexpr int LAYOUTS_PLAYED_Y = 0x2a;
constexpr int LAYOUTS_PLAYED_FONT = FONT_CLOCK;
constexpr int LAYOUTS_PLAYED_FONT_MAP = FONT_CLOCK_MAP;
constexpr int SLOT_WIDTH = 0xc1;  // a layout's slot of the page
constexpr int SLOT_HEIGHT = 0x93;
constexpr int SLOT_NAME_Y = 0xb;
constexpr int SLOT_INFO_Y = 0x8f;
constexpr int SLOT_RIBBON_X = 0x7d;  // the ribbon of a layout won
constexpr int SLOT_RIBBON_Y = 0;
constexpr int SLOT_NAME_FONT = FONT_SMALL;
constexpr int SLOT_NAME_FONT_MAP = FONT_SMALL_MAP;
constexpr int SLOT_INFO_FONT = FONT_SMALL;  // (Japan's info line has the name's font)
constexpr int SLOT_INFO_FONT_MAP = FONT_SMALL_MAP;
constexpr int LAYOUTS_PREV_BUTTON_X = 0x32;
constexpr int LAYOUTS_NEXT_BUTTON_X = 0x2ba;
constexpr int LAYOUTS_PAGE_BUTTONS_Y = 0x21d;
constexpr int LAYOUTS_MENU_BUTTON_X = 0xd3;
constexpr int LAYOUTS_PLAY_BUTTON_X = 0x1b2;
constexpr int LAYOUTS_BUTTONS_Y = 0x22a;  // the menu and play buttons
constexpr int LAYOUTS_MENU_LABEL_RIGHT = 0x8c;
constexpr int LAYOUTS_PLAY_LABEL_RIGHT = 0xa4;
constexpr int LAYOUTS_LABEL_Y = 0x19;
constexpr int LAYOUTS_MUSIC = MUSIC_LAYOUTS_2;  // the music of the turns after the first two
// the places of the list's slots: the three columns' x, then the three rows' y
constexpr int SLOT_PLACES[6] = {0x43, 0x130, 0x21c, 0x3a, 0xdd, 0x181};

// The ending (Ending.cpp)
constexpr int NUM_SHOWCASE_LINES = 12;  // the names the showcase shows, one by one
constexpr int ENDING_TITLE_Y = 0x12;
constexpr int ENDING_TITLE_LINE_GAP = 10;  // added to the title font's line height
constexpr int ENDING_SHOWCASE_X = 0x72;
constexpr int ENDING_SHOWCASE_Y = 0x5a;
constexpr int ENDING_TITLE_FONT = FONT_TITLE;
constexpr int ENDING_TITLE_FONT_MAP = FONT_TITLE_MAP;
constexpr int ENDING_LINE_FONT = FONT_LABEL;  // the font of the showcase's names
constexpr int ENDING_LINE_FONT_MAP = FONT_LABEL_MAP;
constexpr int ENDING_MAP_BUTTON_X = 0x18;
constexpr int ENDING_MENU_BUTTON_X = 0x278;
constexpr int ENDING_BUTTONS_Y = 0x214;
constexpr int ENDING_MAP_LABEL_RIGHT = 0x9c;
constexpr int ENDING_MENU_LABEL_RIGHT = 0x90;
constexpr int ENDING_LABEL_Y = 0x1b;
// 0x51d718 (mahjongjapan): the showcase's names' places (Japan's start at the left; China's are centred on their x)
constexpr int SHOWCASE_LINE_POS[NUM_SHOWCASE_LINES][2] = {
	{0x1ce, 0x77}, {0x27d, 0xdf}, {0x2a4, 0x150}, {0x25f, 0x1ba}, {0x207, 0x214}, {0x187, 0x232},
	{0xe7, 0x225}, {0x65, 0x1e3}, {0x20, 0x167}, {0x58, 0x116}, {0x91, 0x84}, {0x12b, 0x77}};

// A showcase name: Japan's starts at its place, China's is centred on it
inline void DrawShowcaseLine(jb::Object* theObject, jb::Font* theFont, int theX, int theY, const char* theText,
	int theAlpha)
{
	theFont->DrawLeft(theObject, theX, theY, theText, -1, 100, 100, theAlpha);
}

// The tips (GameDialogs.cpp)
constexpr int TIP_ARROW_X = -0x11;  // the bobbing arrow's x on a tip's picture
// 0x51cb30 (mahjongjapan): the places of the tips
constexpr int TIP_POS[9][2] = {{0xa0, 0x55}, {0xdd, 0xc3}, {0xe3, 0x8c}, {0xe1, 0x8c}, {0xca, 0xaa},
	{0xe6, 0x78}, {0x82, 0x118}, {0x15e, 0x50}, {0xa0, 0x55}};

// The dialog frame (Dialog.cpp)
constexpr int DIALOG_BUTTON_LABEL_X = -4;  // the label of a dialog button drawn from pieces
constexpr int DIALOG_BUTTON_LABEL_Y = 0x16;
// the label of a dialog button with a picture: its right edge from the frame's width
constexpr int DIALOG_PICTURE_LABEL_RIGHT = -6;
constexpr int DIALOG_PICTURE_LABEL_Y = 0x15;
constexpr int DIALOG_TEXT_LINE_GAP = -3;  // added to the font's line height between the lines of a dialog's text

// The escape's map (MapLayer.cpp)
constexpr int NUM_STAGES = 12;  // the stages of the escape, each with a picture on the map
constexpr int MAP_RING_SHIFT = 0;  // added to where the ring of the chosen level is drawn
constexpr int MAP_ENDS_TEXT_Y = 0x34;  // the y of the texts at the ends of the map's bar
constexpr int MAP_BAR_HEIGHT = 0x12;  // the progress bar's picture
constexpr int MAP_BAR_X = 9;
constexpr int MAP_BAR_Y = 0x10;
constexpr int MAP_BAR_LAST = 0x2f7;  // the widest the bar's marker goes
constexpr int MAP_MARKER_SHIFT_X = -4;  // added to the bar's width for the marker's x
constexpr int MAP_MARKER_Y = 0;
constexpr int MAP_END_GLOW_X = 0x2ba;  // where the glow shows once the escape is won
constexpr int MAP_END_GLOW_Y = 0x7d;
constexpr int MAP_GLOW_SHIFT = 0x15;  // the glow's centre, taken off its place
constexpr int MAP_FIRST_GLOW_X = -6;  // where the glow starts for the first move
constexpr int MAP_FIRST_GLOW_Y = 0x1f0;
constexpr int MAP_MENU_Y = 0x228;  // the three buttons under the map
constexpr int MAP_MENU_LABEL_X = 0x8e;
constexpr int MAP_MENU_LABEL_Y = 0x1a;
constexpr int MAP_PLAY_X = 0x283;
constexpr int MAP_PLAY_Y = 0x228;
constexpr int MAP_PLAY_LABEL_X = 0x94;
constexpr int MAP_SHOWCASE_X = 0x145;
constexpr int MAP_SHOWCASE_Y = 0x228;
constexpr int MAP_SHOWCASE_LABEL_X = 0x8e;
constexpr int MUSIC_LOOP_EVEN = 10;  // the game music's loop at an even level
constexpr bool MAP_ENDS_TEXT_FIRST = false;  // the texts at the bar's ends are drawn before the stages' pictures
constexpr bool MAP_MOVE_SOUND_FIRST = false;  // the map's move sound is loaded before the new stage's
// 0x51ce98 (mahjongjapan): the progress at which each stage's picture shows
constexpr int STAGE_LEVELS[NUM_STAGES] = {3, 7, 12, 17, 22, 27, 32, 37, 42, 47, 53, 60};
// 0x51cec8 (mahjongjapan): the pictures' rects (the left and top place them; a click inside picks the stage's first
// level)
constexpr int STAGE_RECTS[NUM_STAGES][4] = {
	{0, 426, 156, 534}, {17, 302, 144, 404}, {30, 175, 157, 278}, {139, 40, 235, 160},
	{236, 100, 376, 141}, {342, 141, 437, 264}, {357, 298, 472, 427}, {414, 427, 544, 529},
	{539, 512, 653, 586}, {570, 333, 695, 479}, {614, 198, 728, 333}, {672, 53, 779, 148}};
// 0x51cf88 (mahjongjapan)
constexpr int STAGE_IMAGES[NUM_STAGES] = {571, 574, 573, 567, 576, 570, 572, 575, 568, 569, 566, 577};
// 0x51cfb8 (mahjongjapan): the sixty levels' places on the path, then (0x51d198 (mahjongjapan)) the progress bar's
// width at each progress. One table, as in the exe: the map reads the place of level 60 (the bar's first widths) while
// the last move ends, and the bar's width before progress 0 (the last level's y) while the first move runs.
constexpr int MAP_PATH[181] = {
	14, 516, 27, 504, 37, 489, 65, 434, 71, 418,
	75, 403, 80, 385, 94, 327, 98, 309, 101, 293,
	106, 276, 109, 258, 125, 200, 131, 184, 137, 169,
	146, 154, 155, 140, 200, 105, 216, 100, 234, 97,
	252, 98, 269, 104, 314, 140, 323, 155, 331, 170,
	338, 187, 345, 204, 364, 260, 369, 278, 374, 296,
	379, 314, 384, 332, 400, 390, 406, 408, 413, 425,
	420, 444, 429, 462, 463, 511, 477, 526, 495, 536,
	515, 541, 535, 542, 594, 523, 607, 508, 618, 493,
	628, 476, 636, 458, 656, 400, 661, 382, 665, 365,
	669, 348, 673, 330, 677, 313, 690, 252, 693, 234,
	698, 216, 702, 199, 707, 181, 712, 164, 719, 146,
	20, 40, 60, 81, 96, 111, 126, 142, 154, 166,
	178, 190, 203, 215, 227, 239, 251, 266, 278, 290,
	302, 314, 329, 341, 353, 365, 377, 389, 401, 413,
	425, 437, 451, 463, 475, 487, 499, 513, 525, 537,
	549, 561, 574, 586, 598, 610, 622, 635, 645, 655,
	665, 675, 685, 696, 705, 714, 723, 732, 741, 750,
	781};
// 0x51d310 (mahjongjapan): each stage's two figures for the play layer
constexpr int STAGE_STATS[NUM_STAGES][2] = {
	{-8500, -300}, {-300, 250}, {250, 538}, {538, 710}, {710, 794}, {794, 1185},
	{1185, 1333}, {1333, 1573}, {1573, 1603}, {1603, 1867}, {1867, 1912}, {1912, 1926}};
// 0x51d370 (mahjongjapan): each level's background
constexpr int LEVEL_BACKGROUNDS[NUM_LEVELS] = {
	385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399,
	400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413, 414,
	415, 416, 417, 418, 419, 420, 421, 422, 423, 424, 425, 426, 427, 428, 429,
	430, 431, 432, 433, 434, 435, 436, 579, 580, 581, 582, 583, 584, 585, 578};
// 0x5250cc (mahjongjapan): the stages' names in the exe
constexpr uint32_t STAGE_NAMES[NUM_STAGES] = {
	GTA(0x525160), GTA(0x525158), GTA(0x525150), GTA(0x525148), GTA(0x525140), GTA(0x525138),
	GTA(0x52512c), GTA(0x525120), GTA(0x525110), GTA(0x52510c), GTA(0x525104), GTA(0x5250fc)};

// The play layer (PlayLayer.cpp, Board.cpp)
constexpr int PANEL_BUTTON_TEXT_X = 0xf;  // the two lines of text on a panel's button: the x of both, the second's y
constexpr int PANEL_BUTTON_LINE2_Y = 0x2f;
constexpr int PANEL_UNDO_LABEL_X = 0x55;  // the labels of the panel's buttons
constexpr int PANEL_UNDO_LABEL_Y = 0x18;
constexpr int PANEL_HINT_LABEL_X = 0x56;
constexpr int PANEL_HINT_LABEL_Y = 0x17;
constexpr int PANEL_SHUFFLE_LABEL_X = 0x66;
constexpr int PANEL_SHUFFLE_LABEL_Y = 0x17;
constexpr int PANEL_MENU_LABEL_X = 0x5a;
constexpr int PANEL_MENU_LABEL_Y = 0x14;
constexpr int PANEL_PAUSE_LABEL_X = 0x5a;
constexpr int PANEL_PAUSE_LABEL_Y = 0x14;
constexpr int TRAY_LEFT_X = 0x15c;  // where the tray's left and right tiles are (the faces and the flights go to them)
constexpr int TRAY_RIGHT_X = 0x18e;
constexpr int BOARD_OFFSET_Y_ADJUST = -5;  // added to the board's y once it is centred in its area
constexpr int MUSIC_LOOP_KEYS = 4;  // the game music's loops while a key is in play and after a gold pair
constexpr int MUSIC_LOOP_GOLD = 8;
// the panel preloads its small font before the panel's pictures (China: after the clock's)
constexpr bool PANEL_SMALL_FONT_FIRST = true;
constexpr bool BOARD_JOIN_SOUNDS_FIRST = false;  // the board loads the join and deal sounds before the undo's (China)
// where a button of the panel goes, in the classic game (0) and in the escape (1)
struct ButtonSpot
{
	int x;
	int y;
};
constexpr ButtonSpot SHADE_SPOT[2] = {{0xbf, 0x222}, {0xc2, 0x222}};
constexpr ButtonSpot UNDO_SPOT[2] = {{0xf9, 0x222}, {0xf8, 0x21c}};
constexpr ButtonSpot HINT_SPOT[2] = {{0x1c8, 0x222}, {0x1ca, 0x21c}};
constexpr ButtonSpot SHUFFLE_SPOT[2] = {{0x225, 0x222}, {0x227, 0x21c}};
constexpr ButtonSpot MENU_SPOT[2] = {{0x2ad, 0x213}, {0x2b4, 0x20f}};
constexpr ButtonSpot PAUSE_SPOT[2] = {{0x2ad, 0x235}, {0x2b4, 0x231}};
// the levels at which the tips 2, 3 and 4 are shown
constexpr int TIP_LEVELS[3] = {2, 5, 9};

// The backgrounds (Globals.cpp)
constexpr int NUM_BACKGROUNDS = 21;  // the backgrounds: the classic game's, then the escape's
// 0x51c90c (mahjongjapan): the backgrounds' images (the escape's from [BACKGROUNDS_AT_START])
constexpr int BACKGROUNDS[NUM_BACKGROUNDS] = {0xe8, 0xe9, 0xea, 0x100, 0x24e, 0x24f, 0x250, 0x251, 0x252, 0xdb,
	0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xda, 0xe7, 0x24d};

} // namespace edition
} // namespace mahjong

#endif
