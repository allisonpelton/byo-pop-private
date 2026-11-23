# follows.py <bot state log> <replay state log>: whether a replay of the bot's recorded input is the bot's game: the
# random state's values in the same order, on the same frames. Prints one line; exits 1 when the replay leaves it.
import sys


def rand_values(path):
    values = []
    for line in open(path):
        words = line.split()
        if len(words) >= 3 and words[1] == 'rand':
            values.append((int(words[0]), words[2]))
    return values


bot = rand_values(sys.argv[1])
replay = rand_values(sys.argv[2])
count = min(len(bot), len(replay))
for i in range(count):
    if bot[i] != replay[i]:
        print('follows     the replay leaves the bot\'s game at value %d: the bot has %s at frame %d, the replay %s at '
              'frame %d' % (i, bot[i][1], bot[i][0], replay[i][1], replay[i][0]))
        sys.exit(1)
print('follows     the replay is the bot\'s game: %d random values on the same frames (bot %d, replay %d)'
      % (count, len(bot), len(replay)))
