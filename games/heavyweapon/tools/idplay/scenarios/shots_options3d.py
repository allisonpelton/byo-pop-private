# The screen pass (SCREENS.md): the options box's full screen and 3D boxes. No bot. The full screen box ticked and
# unticked again (not applied: DONE comes after the untick), then the 3D box: in the 2D runs neither game has 3D (the
# sandbox's DirectDraw has none, and the port runs on SDL's software renderer, PORT_ENVIRONMENT), so ticking it brings the
# "not supported" box; in the 3D runs (shots3d) the box is ticked already and unticking it brings the question whether to
# turn 3D off. Two clicks answer either: the question's NO (in 2D a click on the message box beside its OK) and then the
# message box's OK (in 3D a click on the options box between its hints box and its HELP and CREDITS).
END = 760
BOT_FROM = None
PORT_ENVIRONMENT = {'SDL_RENDER_DRIVER': 'software'}
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (100, 'char O'), (103, 'char p'), (106, 'char t'), (120, 'key RETURN'),   # the new player's name
    (200, 'move 505 285'), (200, 'click 505 285'),              # the options box
    (260, 'move 195 205'), (260, 'click 195 205'),              # the full screen box, ticked
    (320, 'move 195 205'), (320, 'click 195 205'),              # and unticked
    (380, 'move 195 235'), (380, 'click 195 235'),              # the 3D box: "not supported" in 2D, "turn 3D off?" in 3D
    (460, 'move 385 315'), (460, 'click 385 315'),              # the question's NO (3D)
    (520, 'move 320 318'), (520, 'click 320 318'),              # the message box's OK (2D)
    (600, 'move 320 375'), (600, 'click 320 375'),              # the options' DONE
]
SHOTS = [
    (240, 'opt_options'), (300, 'opt_fullscreen_ticked'), (360, 'opt_fullscreen_unticked'), (420, 'opt_3d_box'),
    (500, 'opt_3d_answered'), (560, 'opt_3d_answered_again'), (660, 'opt_mainmenu'),
]
