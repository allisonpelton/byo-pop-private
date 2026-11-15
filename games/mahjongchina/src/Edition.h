// What differs in Mahjong Escape: Ancient China from Ancient Japan: the names, the counts, the places and sizes on the
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
#define EDITION_GAME_FOLDER "mahjongchina"
#define EDITION_EXE_NAME "MahjongEscapeAC"  // the original exe's name
#define EDITION_EXE_FILE "MahjongEscapeAC.exe"  // the original exe, which holds the menus' texts
#define EDITION_GAME_TITLE "Mahjong Escape: Ancient China"  // what the game is called in the dialogs of the host
// the window's title: the exe's own (China's calls itself only Mahjong Escape)
#define EDITION_APP_TITLE "Mahjong Escape"
#define EDITION_APP_PRODUCT "Mahjong Escape"  // the product's name, which makes the registry key
#define EDITION_INSTALL_FOLDER "Mahjong Escape Ancient China"  // the game's folder in the PopCap Game Collection
#define EDITION_WEB_FOLDER "mcpersist"  // the web page's persistent folder

// The features only one edition has, for #if: 1 there and 0 in the other
#define EDITION_DRAGON_CORNERS 1  // the dialog frame's corner of kind 2, with a dragon (China)
#define EDITION_RESTART_BOX 0  // the box that asks whether to start the escape again once it is won (Japan)
// the map's last stages have six and seven levels, and a stage's end is a table's (Japan)
#define EDITION_UNEVEN_STAGES 0
#define EDITION_ESCAPE_TITLE_FONT 1  // the panel has its own font for the escape's title (China)
#define EDITION_TIPS_OFF 0  // no tips on a level that has a best score (Japan)
#define EDITION_MENU_RESTART 1  // the in-game menu has a restart (China)

namespace mahjong
{
namespace edition
{

// The player's lists (Players.cpp)
constexpr int PLAYER_BOX_SELECTION = 0x81060;  // the colour of the selected name
constexpr int PLAYER_BOX_FRAME_1 = 0x304838;  // the box frame's first colour
constexpr int PLAYER_BOX_FRAME_2 = 0x90b8a8;  // the box frame's second colour

// The player's records (User.h, User.cpp)
constexpr int NUM_LAYOUT_RECORDS = 150;  // the slots of the layouts' best times and scores
constexpr int NUM_LEVELS = 52;  // the escape's levels
constexpr int DEFAULT_QUAD_PERCENT = 0x19;  // the share of fours in a new player's deals

// The main menu (MainMenu.cpp)
constexpr int MENU_HINT_X = 0;  // the caption of a button: its offset from the middle
constexpr int MENU_HINT_Y = 0x5b;  // the caption of the two big buttons
constexpr int MENU_PLAYER_HINT_Y = 0x56;  // the caption of the player's button
constexpr int MENU_WELCOME_Y = 0x1e2;  // the welcome line
constexpr int MENU_LANTERN_LEFT_X = 0x95;
constexpr int MENU_LANTERN_RIGHT_X = 0x274;
constexpr int MENU_LANTERN_Y = 0x146;
constexpr int MENU_LANTERN_WIDTH = 0x22;  // the width of a lantern's frame
constexpr int MENU_ADVENTURE_X = 0xef;
constexpr int MENU_ADVENTURE_Y = 0xdf;
constexpr int MENU_ADVENTURE_LABEL_X = 10;
constexpr int MENU_ADVENTURE_LABEL_RIGHT = 0x140;
constexpr int MENU_ADVENTURE_LABEL_Y = 0x2f;
constexpr int MENU_CLASSIC_X = 0x134;
constexpr int MENU_CLASSIC_Y = 0x144;
constexpr int MENU_CLASSIC_LABEL_X = 10;
constexpr int MENU_CLASSIC_LABEL_RIGHT = 0xaa;
constexpr int MENU_SMALL_Y = 0x209;  // the options, instructions and exit buttons
constexpr int MENU_SMALL_LABEL_Y = 0x1c;
constexpr int MENU_OPTIONS_X = 0xae;
constexpr int MENU_OPTIONS_LABEL_X = 8;
constexpr int MENU_OPTIONS_LABEL_RIGHT = 0x6e;
constexpr int MENU_INSTRUCTIONS_X = 0x13c;
constexpr int MENU_INSTRUCTIONS_LABEL_X = 0xd;
constexpr int MENU_INSTRUCTIONS_LABEL_RIGHT = 0xaa;
constexpr int MENU_EXIT_X = 0x20f;
constexpr int MENU_EXIT_LABEL_X = 0xd;
constexpr int MENU_EXIT_LABEL_RIGHT = 0x5a;
constexpr int MENU_PLAYER_BUTTON_Y = 0x1f6;
constexpr int MENU_SMALL_FONT = FONT_MENU_SMALL_BUTTON;  // the font of the small buttons' labels
constexpr int MENU_SMALL_FONT_MAP = FONT_MENU_SMALL_BUTTON_MAP;

// The classic game's layout list (LayoutSelect.cpp)
constexpr int NUM_LISTED_LAYOUTS = 0x7e;  // the layouts of the list
constexpr int LIST_FIRST_RANGE = 126;  // the list's layouts from id 437 on; Japan's others are from id 600 on
constexpr int BACKGROUNDS_AT_START = 4;  // the backgrounds a new player can choose, before the stages add theirs
constexpr int MAX_FIRST_BACKGROUND = 9;  // the first of the last three backgrounds of the list's background page
constexpr int LAYOUTS_TITLE_Y = 0x12;
constexpr int LAYOUTS_PREV_PAGE_X = 0xa3;  // the page captions
constexpr int LAYOUTS_NEXT_PAGE_X = 0x279;
constexpr int LAYOUTS_PAGE_Y = 0x22c;
constexpr int LAYOUTS_PLAYED_Y = 0x2e;
constexpr int LAYOUTS_PLAYED_FONT = FONT_TEXT_ALT;
constexpr int LAYOUTS_PLAYED_FONT_MAP = FONT_TEXT_ALT_MAP;
constexpr int SLOT_WIDTH = 0xcb;  // a layout's slot of the page
constexpr int SLOT_HEIGHT = 0x99;
constexpr int SLOT_NAME_Y = 0x12;
constexpr int SLOT_INFO_Y = 0x92;
constexpr int SLOT_RIBBON_X = 0x83;  // the ribbon of a layout won
constexpr int SLOT_RIBBON_Y = 2;
constexpr int SLOT_NAME_FONT = FONT_LAYOUT_SLOT;
constexpr int SLOT_NAME_FONT_MAP = FONT_LAYOUT_SLOT_MAP;
constexpr int SLOT_INFO_FONT = FONT_LAYOUT_INFO;  // (Japan's info line has the name's font)
constexpr int SLOT_INFO_FONT_MAP = FONT_LAYOUT_INFO_MAP;
constexpr int LAYOUTS_PREV_BUTTON_X = 0x68;
constexpr int LAYOUTS_NEXT_BUTTON_X = 0x27c;
constexpr int LAYOUTS_PAGE_BUTTONS_Y = 0x212;
constexpr int LAYOUTS_MENU_BUTTON_X = 0xf5;
constexpr int LAYOUTS_PLAY_BUTTON_X = 0x1a3;
constexpr int LAYOUTS_BUTTONS_Y = 0x227;  // the menu and play buttons
constexpr int LAYOUTS_MENU_LABEL_RIGHT = 0x90;
constexpr int LAYOUTS_PLAY_LABEL_RIGHT = 0x90;
constexpr int LAYOUTS_LABEL_Y = 0x18;
constexpr int LAYOUTS_MUSIC = MUSIC_LAYOUTS_3;  // the music of the turns after the first two
// the places of the list's slots: the three columns' x, then the three rows' y
constexpr int SLOT_PLACES[6] = {0x3e, 0x12b, 0x218, 0x39, 0xd9, 0x179};

// The ending (Ending.cpp)
constexpr int NUM_SHOWCASE_LINES = 11;  // the names the showcase shows, one by one
constexpr int ENDING_TITLE_Y = 0x2d;
constexpr int ENDING_TITLE_LINE_GAP = -5;  // added to the title font's line height
constexpr int ENDING_SHOWCASE_X = 0x44;
constexpr int ENDING_SHOWCASE_Y = 0x77;
constexpr int ENDING_TITLE_FONT = FONT_ESCAPE_TITLE;
constexpr int ENDING_TITLE_FONT_MAP = FONT_ESCAPE_TITLE_MAP;
constexpr int ENDING_LINE_FONT = FONT_RESULT;  // the font of the showcase's names
constexpr int ENDING_LINE_FONT_MAP = FONT_RESULT_MAP;
constexpr int ENDING_MAP_BUTTON_X = 0xe;
constexpr int ENDING_MENU_BUTTON_X = 0x295;
constexpr int ENDING_BUTTONS_Y = 0x221;
constexpr int ENDING_MAP_LABEL_RIGHT = 0x90;
constexpr int ENDING_MENU_LABEL_RIGHT = 0x7c;
constexpr int ENDING_LABEL_Y = 0x18;
// 0x51c608 (mahjongchina): the showcase's names' places (each is centred on its x)
constexpr int SHOWCASE_LINE_POS[NUM_SHOWCASE_LINES][2] = {
	{409, 115}, {538, 136}, {646, 203}, {633, 347}, {524, 390}, {403, 433},
	{261, 418}, {145, 334}, {146, 181}, {254, 120}, {383, 231}};

// A showcase name: Japan's starts at its place, China's is centred on it
inline void DrawShowcaseLine(jb::Object* theObject, jb::Font* theFont, int theX, int theY, const char* theText,
	int theAlpha)
{
	theFont->DrawLinesCentred(theObject, theX, theY, theText, 0, theFont->GetLineHeight() - 7, 100, 100, theAlpha);
}

// The tips (GameDialogs.cpp)
constexpr int TIP_ARROW_X = 0;  // the bobbing arrow's x on a tip's picture
// 0x51bab8 (mahjongchina): the places of the tips
constexpr int TIP_POS[9][2] = {{0xa0, 0x55}, {5, 0xc3}, {10, 0x8c}, {0x8c, 0x50}, {0xca, 0xaa},
	{0xe6, 0x78}, {0x82, 0x118}, {0x15e, 0x50}, {0xa0, 0x55}};

// The dialog frame (Dialog.cpp)
constexpr int DIALOG_BUTTON_LABEL_X = 0;  // the label of a dialog button drawn from pieces
constexpr int DIALOG_BUTTON_LABEL_Y = 0x18;
// the label of a dialog button with a picture: its right edge from the frame's width
constexpr int DIALOG_PICTURE_LABEL_RIGHT = 0;
constexpr int DIALOG_PICTURE_LABEL_Y = 0x17;
constexpr int DIALOG_TEXT_LINE_GAP = -1;  // added to the font's line height between the lines of a dialog's text

// The escape's map (MapLayer.cpp)
constexpr int NUM_STAGES = 11;  // the stages of the escape, each with a picture on the map
constexpr int MAP_RING_SHIFT = -7;  // added to where the ring of the chosen level is drawn
constexpr int MAP_ENDS_TEXT_Y = 0x3c;  // the y of the texts at the ends of the map's bar
constexpr int MAP_BAR_HEIGHT = 0x18;  // the progress bar's picture
constexpr int MAP_BAR_X = 0x13;
constexpr int MAP_BAR_Y = 0x17;
constexpr int MAP_BAR_LAST = 0x2e3;  // the widest the bar's marker goes
constexpr int MAP_MARKER_SHIFT_X = 6;  // added to the bar's width for the marker's x
constexpr int MAP_MARKER_Y = 8;
constexpr int MAP_END_GLOW_X = 0x2ce;  // where the glow shows once the escape is won
constexpr int MAP_END_GLOW_Y = 0xb0;
constexpr int MAP_GLOW_SHIFT = 0x1c;  // the glow's centre, taken off its place
constexpr int MAP_FIRST_GLOW_X = 0x34;  // where the glow starts for the first move
constexpr int MAP_FIRST_GLOW_Y = 0x31;
constexpr int MAP_MENU_Y = 0x221;  // the three buttons under the map
constexpr int MAP_MENU_LABEL_X = 0x7e;
constexpr int MAP_MENU_LABEL_Y = 0x18;
constexpr int MAP_PLAY_X = 0x299;
constexpr int MAP_PLAY_Y = 0x21f;
constexpr int MAP_PLAY_LABEL_X = 0x80;
constexpr int MAP_SHOWCASE_X = 0x158;
constexpr int MAP_SHOWCASE_Y = 0x21f;
constexpr int MAP_SHOWCASE_LABEL_X = 0x78;
constexpr int MUSIC_LOOP_EVEN = 0xd;  // the game music's loop at an even level
constexpr bool MAP_ENDS_TEXT_FIRST = true;  // the texts at the bar's ends are drawn before the stages' pictures
constexpr bool MAP_MOVE_SOUND_FIRST = true;  // the map's move sound is loaded before the new stage's
// 0x51be48 (mahjongchina): the progress at which each stage's picture shows
constexpr int STAGE_LEVELS[NUM_STAGES] = {3, 7, 12, 17, 22, 27, 32, 37, 42, 47, 52};
// 0x51be78 (mahjongchina): the pictures' rects (the left and top place them; a click inside picks the stage's first
// level)
constexpr int STAGE_RECTS[NUM_STAGES][4] = {
	{42, 83, 152, 165}, {120, 202, 213, 302}, {190, 338, 272, 450}, {265, 489, 352, 571},
	{313, 349, 446, 447}, {352, 191, 488, 294}, {428, 49, 560, 133}, {500, 269, 627, 341},
	{586, 450, 659, 562}, {662, 294, 747, 402}, {651, 61, 788, 205}};
// 0x51bf28 (mahjongchina)
constexpr int STAGE_IMAGES[NUM_STAGES] = {244, 240, 235, 238, 236, 242, 243, 241, 245, 237, 239};
// 0x51bf58 (mahjongchina): the fifty-two levels' places on the path, then (0x51c0f8 (mahjongchina)) the progress bar's
// width at each progress. One table, as in the exe: the map reads the place of level 52 (the bar's first widths) while
// the last move ends, and the bar's width before progress 0 (the last level's y) while the first move runs.
constexpr int MAP_PATH[157] = {
	72, 69, 94, 76, 114, 88, 148, 125, 161, 145,
	173, 167, 184, 191, 208, 253, 214, 275, 219, 297,
	223, 319, 227, 342, 237, 451, 239, 468, 243, 486,
	250, 503, 262, 517, 350, 522, 365, 507, 374, 486,
	380, 466, 383, 447, 391, 368, 393, 349, 395, 329,
	397, 311, 399, 293, 419, 186, 424, 167, 432, 150,
	442, 134, 455, 120, 547, 137, 555, 166, 560, 197,
	562, 227, 562, 257, 561, 349, 562, 377, 566, 406,
	572, 434, 581, 461, 660, 490, 679, 473, 690, 452,
	699, 429, 706, 405, 724, 319, 729, 291, 735, 263,
	740, 234, 746, 204, 22, 44, 66, 87, 103, 120,
	136, 153, 166, 179, 192, 205, 218, 231, 244, 257,
	270, 283, 296, 309, 322, 335, 348, 361, 374, 387,
	400, 413, 426, 439, 452, 465, 479, 492, 505, 518,
	531, 544, 557, 570, 583, 596, 609, 622, 635, 648,
	661, 674, 687, 700, 713, 726, 761};
// 0x51c228 (mahjongchina): each stage's two figures for the play layer
constexpr int STAGE_STATS[NUM_STAGES][2] = {
	{-2000, -1500}, {-1500, -1045}, {-1045, -221}, {-221, -207}, {-206, 220}, {589, 617},
	{618, 907}, {960, 1279}, {1279, 1368}, {1368, 1644}, {1644, 1912}};
// 0x51c280 (mahjongchina): each level's layout
constexpr int LEVEL_BACKGROUNDS[NUM_LEVELS] = {
	385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397,
	398, 399, 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410,
	411, 412, 413, 414, 415, 416, 417, 418, 419, 420, 421, 422, 423,
	424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435, 436};
// 0x5240d0 (mahjongchina): the stages' names in the exe
constexpr uint32_t STAGE_NAMES[NUM_STAGES] = {
	GTA(0x524140), GTA(0x524138), GTA(0x524130), GTA(0x52412c), GTA(0x524128), GTA(0x524124),
	GTA(0x52411c), GTA(0x524114), GTA(0x52410c), GTA(0x524104), GTA(0x5240fc)};

// The play layer (PlayLayer.cpp, Board.cpp)
constexpr int PANEL_BUTTON_TEXT_X = 0xc;  // the two lines of text on a panel's button: the x of both, the second's y
constexpr int PANEL_BUTTON_LINE2_Y = 0x2d;
constexpr int PANEL_UNDO_LABEL_X = 0x50;  // the labels of the panel's buttons
constexpr int PANEL_UNDO_LABEL_Y = 0x18;
constexpr int PANEL_HINT_LABEL_X = 0x52;
constexpr int PANEL_HINT_LABEL_Y = 0x17;
constexpr int PANEL_SHUFFLE_LABEL_X = 100;
constexpr int PANEL_SHUFFLE_LABEL_Y = 0x17;
constexpr int PANEL_MENU_LABEL_X = 0x56;
constexpr int PANEL_MENU_LABEL_Y = 0x15;
constexpr int PANEL_PAUSE_LABEL_X = 0x56;
constexpr int PANEL_PAUSE_LABEL_Y = 0x16;
constexpr int TRAY_LEFT_X = 0x16c;  // where the tray's left and right tiles are (the faces and the flights go to them)
constexpr int TRAY_RIGHT_X = 0x1a0;
constexpr int BOARD_OFFSET_Y_ADJUST = 0;  // added to the board's y once it is centred in its area
constexpr int MUSIC_LOOP_KEYS = 7;  // the game music's loops while a key is in play and after a gold pair
constexpr int MUSIC_LOOP_GOLD = 0xc;
// the panel preloads its small font before the panel's pictures (China: after the clock's)
constexpr bool PANEL_SMALL_FONT_FIRST = false;
constexpr bool BOARD_JOIN_SOUNDS_FIRST = true;  // the board loads the join and deal sounds before the undo's (China)
// where a button of the panel goes: alike in the classic game (0) and the escape (1)
struct ButtonSpot
{
	int x;
	int y;
};
constexpr ButtonSpot SHADE_SPOT[2] = {{0xd3, 0x223}, {0xd3, 0x223}};
constexpr ButtonSpot UNDO_SPOT[2] = {{0x117, 0x222}, {0x117, 0x222}};
constexpr ButtonSpot HINT_SPOT[2] = {{0x1db, 0x222}, {0x1db, 0x222}};
constexpr ButtonSpot SHUFFLE_SPOT[2] = {{0x230, 0x222}, {0x230, 0x222}};
constexpr ButtonSpot MENU_SPOT[2] = {{0x2b8, 0x212}, {0x2b8, 0x212}};
constexpr ButtonSpot PAUSE_SPOT[2] = {{0x2b8, 0x235}, {0x2b8, 0x235}};
// the levels at which the tips 2, 3 and 4 are shown
constexpr int TIP_LEVELS[3] = {3, 7, 0xc};

// The backgrounds (Globals.cpp)
constexpr int NUM_BACKGROUNDS = 15;  // the backgrounds: the classic game's, then the escape's
// 0x51b8a4 (mahjongchina): the backgrounds' images (the escape's from [BACKGROUNDS_AT_START])
constexpr int BACKGROUNDS[NUM_BACKGROUNDS] = {0xe8, 0xe9, 0xea, 0x100, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1,
	0xe2, 0xe3, 0xda, 0xe7};

} // namespace edition
} // namespace mahjong

#endif
