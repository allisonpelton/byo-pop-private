# phase two, focus: the window loses the focus and gets it back, on the main menu, at a Flood level's explanation and in play, with a shot
# in the lost state (the game's "paused" sign) and after each regain. The original does no update while it has no focus, so a loss is a
# FOCUS entry (idplay.py: its script waits in game time and shoots, the port counts frames meanwhile) and the later frames are the
# original's. The inputs are those of `flood` (a player with every mode unlocked: Play Now, Flood and its new game are clicked); the level
# then waits for the click that starts it (no bot).
PROFILE_COMMANDS = ['unlock all', 'goto menu']
INPUTS = [
    (100, 'move 400 232', 'move 400 232'),
    (150, 'down 400 232', 'down 400 232'),
    (152, 'up 400 232', 'up 400 232'),
    (700, 'move 401 270', 'move 401 270'),
    (750, 'down 401 270', 'down 401 270'),
    (752, 'up 401 270', 'up 401 270'),
    (1300, 'move 470 330', 'move 470 330'),
    (1350, 'down 470 330', 'down 470 330'),
    (1352, 'up 470 330', 'up 470 330'),
    # the click that gives the focus back is not the player's click: the first one after it is dropped
    (2400, 'move 400 300', 'move 400 300'),
    (2450, 'down 400 300', 'down 400 300'),
    (2452, 'up 400 300', 'up 400 300'),
    (2600, 'move 400 300', 'move 400 300'),
    (2650, 'down 400 300', 'down 400 300'),
    (2652, 'up 400 300', 'up 400 300'),
]
FOCUS = [
    {'frame': 40, 'steps': [(120, 'menu_lost_a'), (240, 'menu_lost_b')], 'wait': 120},
    {'frame': 1900, 'steps': [(120, 'level_lost_a'), (240, 'level_lost_b')], 'wait': 120},
    {'frame': 3600, 'steps': [(120, 'play_lost_a'), (240, 'play_lost_b')], 'wait': 120},
]
SHOTS = [(30, 'menu_before'), (46, 'menu_regained'), (110, 'menu_after'), (1800, 'level_explained'), (1906, 'level_regained'),
         (2500, 'level_after_click1'), (2800, 'level_after_click2'), (3500, 'play_before'), (3606, 'play_regained'),
         (4000, 'play_after')]
MOUSE_AT = (790, 300)
END = 4100
