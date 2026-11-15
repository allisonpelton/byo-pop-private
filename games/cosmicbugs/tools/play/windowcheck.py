#!/usr/bin/env python3
"""The window checks of a done game, through real input (docs/REAL_INPUT.md): the window resized (to the game's size, to 3x it,
to sizes that are no whole multiple, maximised and restored), minimised and restored with the focus lost and gained, and full
screen and back with Alt+Enter, each in the window the display's two modes give: the window's own surface (a drawable exactly
1x to 4x the game's size) and the renderer the display falls back to (full screen, any other size).

    build/native/rundesk.exe python games/cosmicbugs/tools/play/windowcheck.py <out dir> [--exe E] [--only resizes,fullscreen,surface_away]

Every run has SURFACE_VERIFY=1 (the display compares the window's surface with the game's screen after every present and aborts
at a difference), so a check passes only while the process is alive at its end. After each step the driver clicks the title's
Options button and Back with the pointer at the game's point, mapped as the window shows it (a click that lands elsewhere finds
no Options page), takes a screenshot of the game's screen, and checks that the game's updates run (and stand still while the focus
is away). The display's own log lines of each step go to the run's check.log, which the notes quote.
"""
import json, os, subprocess, sys, time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import drive
from drive import Game, Failed, TITLE, OPTIONS


def log_lines(g):
    with open(os.path.join(g.out, 'game.log'), errors='replace') as f:
        return f.read().splitlines()


class Check:
    def __init__(self, out, exe, start):
        self.g = Game(os.path.join(out, start), exe, False, start)
        self.name = start
        self.seen = 0
        self.out = self.g.out
        self.chk = open(os.path.join(self.out, 'check.log'), 'w')
        self.failures = []

    def say(self, text):
        print(text, file=self.chk, flush=True)
        self.g.say(text)

    def display(self, step):
        """the display lines the step made (the SDL log lines of SurfaceDisplay and the framework's display line)"""
        lines = log_lines(self.g)
        new = [l for l in lines[self.seen:] if 'SurfaceDisplay' in l or 'display:' in l]
        self.seen = len(lines)
        for l in new:
            self.say('  [%s] %s' % (step, l.strip()))
        return new

    def ready(self):
        """the title, with the first answer waited for as long as a loaded machine needs to load the game"""
        end = time.time() + 240
        while True:
            try:
                self.g.until('the title', lambda s: s['screen'] == 'title', 90)
                break
            except Failed:
                if time.time() > end or self.g.proc.poll() is not None:
                    raise
        self.g.wait(30)

    def started(self, expect):
        new = self.display('start')
        if not any(expect in l for l in new):
            self.fail('start: no display line has "%s"' % expect)

    def fail(self, text):
        self.failures.append(text)
        self.say('FAILED: ' + text)

    def alive(self, step):
        if self.g.proc.poll() is not None:
            self.fail('%s: the game exited with %s (SURFACE_VERIFY aborts at a difference)' % (step, self.g.proc.returncode))
            raise Failed('the game exited')

    def step(self, label, *commands, expect=None, forbid=None, shot=True):
        """Sends the commands, lets the game run, checks the display's lines against `expect` (a substring one of them has)
        and `forbid` (none has), then that the pointer reaches the title's Options button and Back, and that the updates run"""
        g = self.g
        self.say('== %s' % label)
        for c in commands:
            g.ask(c)
            g.wait(15)
        g.wait(40)
        new = self.display(label)
        self.alive(label)
        if expect and not any(expect in l for l in new):
            self.fail('%s: no display line has "%s"' % (label, expect))
        if forbid and any(forbid in l for l in new):
            self.fail('%s: a display line has "%s"' % (label, forbid))
        if shot:
            g.shot(label.replace(' ', '_').replace('/', '_'))
        st = g.state()
        if st['screen'] != 'title':
            self.fail('%s: the game is on %s, not the title' % (label, st['screen']))
            return
        g.click(*TITLE['options'])
        try:
            g.until('the options page after %s' % label, lambda s: s['screen'] == 'options', 10)
        except Failed as e:
            self.fail('%s: a click at the Options button did not open it (%s)' % (label, e))
            return
        g.wait(10)
        g.click(*OPTIONS['back'])
        try:
            g.until('the title after %s' % label, lambda s: s['screen'] == 'title', 10)
        except Failed as e:
            self.fail('%s: Back did not return to the title (%s)' % (label, e))
        u = g.state()['updates']
        time.sleep(0.3)
        if g.state()['updates'] <= u:
            self.fail('%s: the game\'s updates do not run' % label)
        self.display(label + ' after clicks')
        self.alive(label)

    def away(self, label):
        """minimise with the focus lost first (a hand's minimize), the updates stand still, restore, the focus back, they run"""
        g = self.g
        self.say('== %s' % label)
        g.ask('focus lost')
        g.ask('minimize')
        time.sleep(0.3)  # no g.wait: the updates stand still while the window is away
        u0 = g.state()['updates']
        time.sleep(0.5)
        u1 = g.state()['updates']
        time.sleep(0.5)
        u2 = g.state()['updates']
        self.say('  updates while away: %d, %d, %d' % (u0, u1, u2))
        if u2 - u0 > 12:
            self.fail('%s: the updates ran while the window was minimised and unfocused (%d, %d, %d)' % (label, u0, u1, u2))
        g.ask('restore')
        g.ask('focus gained')
        time.sleep(0.5)
        u3 = g.state()['updates']
        time.sleep(0.5)
        u4 = g.state()['updates']
        self.say('  updates after the restore: %d, %d' % (u3, u4))
        if u4 <= u3:
            self.fail('%s: the updates did not run again after the restore (%d, %d)' % (label, u3, u4))
        self.display(label)
        self.alive(label)

    def full_screen(self, on):
        g = self.g
        g.ask('keydown Left Alt')
        g.ask('key Return')
        g.ask('keyup Left Alt')
        g.until('full screen %s' % ('on' if on else 'off'), lambda s: s['fullscreen'] == on)
        g.wait(30)

    def finish(self):
        self.g.close()
        lines = log_lines(self.g)
        tail = [l for l in lines if 'SurfaceDisplay:' in l and ('frames through' in l or 'compared with the screen' in l)]
        for l in tail:
            self.say('  [end] ' + l.strip())
        if self.g.proc.returncode not in (0, None):
            self.fail('the exit code was %s' % self.g.proc.returncode)
        self.say('RESULT ' + ('ok' if not self.failures else 'FAILED: ' + '; '.join(self.failures)))
        print('%s: RESULT %s' % (self.name, 'ok' if not self.failures else 'FAILED (%d)' % len(self.failures)), flush=True)
        return not self.failures


def resizes(out, exe):
    """starts at the game's own size (the window's surface), goes through 3x, a size that is no whole multiple (the renderer),
    back to 1x (the display keeps the renderer it has had), maximised, restored, and 2x again"""
    c = Check(out, exe, 'win1x')
    try:
        c.ready()
        c.started("surface (800x600")
        c.step('start 1x')
        c.step('resize 2x', 'resize 1600 1200', expect="screen at 2x")
        c.step('resize 3x', 'resize 2400 1800', expect="screen at 3x")
        c.step('resize 1x', 'resize 800 600', expect="screen at 1x")
        c.away('minimise at 1x')
        c.step('after minimise 1x', shot=True)
        c.step('resize 1000x750', 'resize 1000 750', expect='renderer')
        c.step('resize 700x500', 'resize 700 500')
        c.step('resize 800x600 after the renderer', 'resize 800 600', forbid='the window\'s surface (')
        c.away('minimise in the renderer')
        c.step('maximise', 'maximize')
        c.step('restore after maximise', 'restore')
        c.step('resize 1600x1200 after the renderer', 'resize 1600 1200')
    except Failed as e:
        c.fail('stopped: %s' % e)
    return c.finish()


def fullscreen(out, exe):
    """starts in the window the framework picks (2x), Alt+Enter to full screen and back twice, a resize and a minimise
    while full, the surface again only on a new run"""
    c = Check(out, exe, 'win2x')
    try:
        c.ready()
        c.started("screen at 2x")
        c.step('start 2x')
        c.full_screen(True)
        c.step('full screen', expect='full screen')
        c.away('minimise in full screen')
        c.step('after minimise in full screen')
        c.full_screen(False)
        c.step('window again')
        c.full_screen(True)
        c.step('full screen again')
        c.full_screen(False)
        c.step('window again 2')
        c.step('resize 800x600 after the renderer', 'resize 800 600')
        c.away('minimise at 800x600 after the renderer')
        c.step('after minimise at 800x600')
    except Failed as e:
        c.fail('stopped: %s' % e)
    return c.finish()


def surface_away(out, exe):
    """minimise and restore, and a resize, in a window that has only ever had the surface: its picture is whole afterwards"""
    c = Check(out, exe, 'win2x-surface')
    # the same start as win2x (the framework's 2x window), Game keys the user folder by the name
    try:
        c.ready()
        c.started("screen at 2x")
        c.step('start 2x')
        c.away('minimise at 2x')
        c.step('after minimise 2x')
        c.step('resize 4x', 'resize 3200 2400', expect='screen at 4x')
        c.away('minimise at 4x')
        c.step('after minimise 4x')
        c.step('resize 1x', 'resize 800 600', expect='screen at 1x')
    except Failed as e:
        c.fail('stopped: %s' % e)
    return c.finish()


def main():
    args = sys.argv[1:]
    out, exe, only = None, 'build/native/cosmicbugs.exe', None
    while args:
        a = args.pop(0)
        if a == '--exe':
            exe = args.pop(0)
        elif a == '--only':
            only = args.pop(0).split(',')
        else:
            out = a
    out, exe = os.path.abspath(out), os.path.abspath(exe)
    os.environ['SURFACE_VERIFY'] = '1'
    ok = True
    for run in (resizes, fullscreen, surface_away):
        if only is None or run.__name__ in only:
            ok = run(out, exe) and ok
    print('RESULT ' + ('ok' if ok else 'FAILED'))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
