#ifndef __SPROUT_BUILD_H__
#define __SPROUT_BUILD_H__

// Which of Sprout's two builds of the engine a game's exe has (docs/ENGINES.md). Word Harmony's exe has the listener
// build: the application drives the controller and a root window, and buttons and screens tell listener interfaces.
// Pizza Frenzy's has the signal build: the application, the scene and the screens talk through signals, and the
// controller keeps a state machine. The classes only one build has are their own (Scene, State, Screen and the rest);
// where a shared function behaves differently in the two, it reads this switch. The game sets it at start, before it
// makes any node; it is false (the listener build) by default.

namespace engine
{

extern bool gSignalBuild;

} // namespace engine

#endif
