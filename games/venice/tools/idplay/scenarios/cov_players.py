# a coverage run (docs/COVERAGE.md): the player menu through every screen it has: a new player with a name typed with shift, a name
# left empty, a rename, the delete confirmation answered No and Yes, the last player that cannot be deleted, and the tenth player
# that no eleventh follows. Keys are given as the virtual-key codes of the original (the port reads the names it knows: letters,
# digits, SHIFT, RETURN) and a symbol only to the original. Places are those of the `buttons` command on each screen.
PROFILE_COMMANDS = ['unlock all', 'goto menu']
INPUTS = []


def click(frame, x, y):
    INPUTS.extend([(frame, 'move %d %d' % (x, y), 'move %d %d' % (x, y)), (frame + 50, 'down %d %d' % (x, y), 'down %d %d' % (x, y)),
                   (frame + 52, 'up %d %d' % (x, y), 'up %d %d' % (x, y))])


def key(frame, vk, name, shift=False):
    """a key press (with shift held, when asked); the port is given the key only when it has a name for it"""
    if shift:
        INPUTS.append((frame, 'keydown 0x10', 'keydown SHIFT'))
    INPUTS.append((frame + 3, 'keydown 0x%02X' % vk, ('keydown ' + name) if name else ''))
    INPUTS.append((frame + 6, 'keyup 0x%02X' % vk, ('keyup ' + name) if name else ''))
    if shift:
        INPUTS.append((frame + 9, 'keyup 0x10', 'keyup SHIFT'))


def type_name(frame, text):
    """the keys of a name: letters (a capital with shift), digits, '-' and '!'"""
    for index, character in enumerate(text):
        at = frame + 20 * index
        if character.isalpha():
            key(at, ord(character.upper()), character.upper(), character.isupper())
        elif character == '-':
            key(at, 0xBD, None)
        elif character == '!':
            key(at, 0x31, '1', True)
        else:
            key(at, ord(character), character)
    return frame + 20 * len(text)


GONDOLIERS = (99, 585)
NEW, DELETE, RENAME, ACCEPT = (270, 442), (356, 442), (443, 442), (530, 442)
OK = (400, 340)
YES, NO = (330, 330), (470, 330)

click(100, *GONDOLIERS)
click(700, *NEW)
type_name(1400, 'Zed')
click(1800, *OK)  # the second player
click(2600, *NEW)
click(3300, *OK)  # no name
click(4000, 400, 330)  # the message that it is not a name
type_name(4700, 'Ann-B!')
click(5300, *OK)  # the third player, which is the current one
click(6100, *RENAME)
type_name(6800, 'Qr')
click(7200, *OK)  # a new name for the current player
click(8000, *DELETE)
click(8700, *NO)
click(9400, *DELETE)
click(10100, *YES)  # the current player is deleted
click(10900, *DELETE)
click(11600, *YES)  # and another
click(12400, *DELETE)  # the last one stays
click(13100, 400, 330)
frame = 13900
for count in range(9):  # the list is full at ten players
    click(frame, *NEW)
    type_name(frame + 700, 'P' + chr(ord('a') + count))
    click(frame + 1100, *OK)
    frame += 1800
click(frame, *NEW)  # an eleventh does not fit
click(frame + 700, 400, 360)  # the message that the list is full
click(frame + 1400, *ACCEPT)
END = frame + 2200
