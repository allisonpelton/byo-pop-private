# a coverage run (docs/COVERAGE.md): the tutorial (tutorial.py), with clicks on its "DON'T SHOW THIS AGAIN" check box (at 73, 568, only
# while an instruction is up; it only sets a flag of the profile, so unlike the options' two check boxes it needs no sandbox)
import os
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'tutorial.py')).read())
for frame in (700, 1100, 1500, 1900, 2300, 2700, 3100, 3500):
    INPUTS.extend([(frame, 'move 73 568', 'move 73 568'), (frame + 20, 'down 73 568', 'down 73 568'), (frame + 22, 'up 73 568', 'up 73 568')])
