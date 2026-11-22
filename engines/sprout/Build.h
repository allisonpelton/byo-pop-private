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

// Feeding Frenzy's exe is an older build than either (linked 2005-07-18; Pizza Frenzy's 2006-02-21, Word Harmony's
// 2006-02-22): the signal build's application, scene and signals, with the listener build's nodes (position, scale and
// colour offset in the node, no colour mode). It sets this beside gSignalBuild, and where a shared function behaves
// differently in it, the function reads this one too; false by default.
extern bool gEarlyBuild;

} // namespace engine

#endif
