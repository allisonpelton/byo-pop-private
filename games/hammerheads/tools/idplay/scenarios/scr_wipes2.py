"""The zoom wipes of a game's levels, for the screens pass (SCREENS.md): the bot's game of classic_nd5 (from level 5: the wipe to the
next level, the wipe to the shop and the wipe back), with a shot every 15 updates through each of them."""
import idplay

_BASE = idplay.load_scenario('classic_nd5')
INPUTS = _BASE.INPUTS
SEED = getattr(_BASE, 'SEED', 1)
JUMP_LEVEL = getattr(_BASE, 'JUMP_LEVEL', None)
PATCHES = getattr(_BASE, 'PATCHES', [])
END = 20500

NAMES = {}
for label, first, last in (('next_level', 14160, 14460), ('to_shop', 19440, 19740), ('from_shop', 19800, 20100)):
    for i, u in enumerate(range(first, last + 1, 15), 1):
        NAMES[u] = 'wipe_%s_%02d' % (label, i)
SHOTS = sorted(NAMES)
