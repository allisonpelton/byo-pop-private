/* The music player's unused patterns (web/music.js; docs/WEB_PLAYER.md, "The music player"; the user, 2026-11-15): one
   pattern that no order plays, played once on its own. libopenmpt plays only what its order list names and its C interface
   cannot change that list, so this reaches the module's sound file and puts the order list aside for one that names the
   pattern alone, then puts it back. web/music/openmpt_web.c's bm_solo calls it; libxmp-lite's side is there. Built with
   libopenmpt's own include folders and LIBOPENMPT_BUILD (cmake/ByopopWeb.cmake), as its sources are. */
/* (in the order libopenmpt_impl.cpp has them) */
#include "common/stdafx.h"
#include "libopenmpt_internal.h"
#include "libopenmpt.hpp"
#include "libopenmpt.h"
#include "libopenmpt_impl.hpp"
#include "soundlib/Sndfile.h"

#include <vector>

/* the C handle's layout (libopenmpt/libopenmpt_c.cpp, word for word) */
extern "C" {
struct openmpt_module {
	openmpt_log_func logfunc;
	void * loguser;
	openmpt_error_func errfunc;
	void * erruser;
	int error;
	const char * error_message;
	openmpt::module_impl * impl;
};
}

namespace {
/* module_impl keeps its sound file to itself and those made from it (m_sndFile is protected): a pointer to that member,
   taken from a class made from it, reads it on any module_impl */
struct SoundFileOf : openmpt::module_impl {
	static OpenMPT::CSoundFile & of( openmpt::module_impl * m ) { return *( m->*( &SoundFileOf::m_sndFile ) ); }
};
}

/* pattern >= 0: the module's order list becomes that pattern alone (the list kept at *saved the first time), the player at
   its first row; returns the pattern's rows, 0 when it is no pattern. pattern < 0: the kept list put back. */
extern "C" int bm_mpt_solo( openmpt_module * mod, int pattern, void ** saved ) {
	if ( !mod || !mod->impl ) return 0;
	OpenMPT::CSoundFile & sf = SoundFileOf::of( mod->impl );
	/* (the sequence as the list it is: ModSequence's own assign takes orders, not a list) */
	std::vector<OpenMPT::PATTERNINDEX> & order = sf.Order();
	std::vector<OpenMPT::PATTERNINDEX> * kept = static_cast<std::vector<OpenMPT::PATTERNINDEX> *>( *saved );
	if ( pattern < 0 ) {
		if ( kept ) {
			order.assign( kept->begin(), kept->end() );
			delete kept;
			*saved = nullptr;
		}
		return 0;
	}
	if ( !sf.Patterns.IsValidIndex( static_cast<OpenMPT::PATTERNINDEX>( pattern ) ) ) return 0;
	if ( !kept ) *saved = new std::vector<OpenMPT::PATTERNINDEX>( order.begin(), order.end() );
	order.assign( 1, static_cast<OpenMPT::PATTERNINDEX>( pattern ) );
	openmpt_module_set_position_order_row( mod, 0, 0 );
	return static_cast<int>( sf.Patterns[static_cast<OpenMPT::PATTERNINDEX>( pattern )].GetNumRows() );
}
