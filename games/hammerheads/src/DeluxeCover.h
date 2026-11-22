#ifndef __HAMMERHEADS_DELUXECOVER_H__
#define __HAMMERHEADS_DELUXECOVER_H__

#include "Image.h"

// port(deviation): the word "Deluxe" is not shown on the title and main menu pictures (the user, 2026-11-18: "Remove the "Deluxe"
// text from the main screen of any game it may appear in, to avoid misleading people about whose game version they're playing").
// Each is made on the player's own picture after it is loaded, in memory: no changed picture is kept anywhere.
namespace Sexy
{
	// the loading screen's title (IMAGE_LOADING_TITLE): the word's glow and letters are made see-through, so that the backdrop shows
	void CoverDeluxeOnLoadingTitle(Image* theImage);

	// the main menu's picture (IMAGE_MAINMENU_SCREEN): the word is painted over with the green around it
	void CoverDeluxeOnMainMenu(Image* theImage);
}

#endif
