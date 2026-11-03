"""mouse.py <port state log> <script> <first update> <updates per pair> <pairs> [skip]: appends to a port script the
clicks of <pairs> matches made with the mouse, one pair every <updates per pair> updates from <first update> on (the
second tile half way through). The pairs come from the last board in the state log (the port's -statelog writes it at
each deal and shuffle): each time, the first free tile in the board's order that has a free match, and that match,
as Board::isFree (0x406270) and Board::getMatches (0x406890) decide. With [skip], the first <skip> such pairs are
passed over by clicking a free tile and then another free tile of a different type first (a wrong pair, which only
moves the selection). A click lands on the middle of the tile's face (Tile::getFaceRect, 0x40c760)."""
import sys

log, script, first, step, pairs = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5])
wrong = int(sys.argv[6]) if len(sys.argv) > 6 else 0

board = {}
last = None
for line in open(log):
	f = line.split()
	if len(f) == 7 and f[1] == "board":
		if f[0] != last:
			board, last = {}, f[0]
		i, t, x, y, z = map(int, f[2:])
		board[i] = (t, x, y, z)

def at(x, y, z):
	return any((b[1], b[2], b[3]) == (x, y, z) for b in board.values())

def free(i):
	t, x, y, z = board[i]
	for dy in (-1, 0, 1):
		for dx in (-1, 0, 1):
			if at(x + dx, y + dy, z + 1):
				return False
	if not any(at(x - 2, y + dy, z) for dy in (-1, 0, 1)):
		return True
	return not any(at(x + 2, y + dy, z) for dy in (-1, 0, 1))

def centre(i):
	t, x, y, z = board[i]
	left = (x * 0x21) // 2 + 0x83 + z * 6
	top = (y * 0x30) // 2 + 0x1e - z * 7
	return left + 22, top + 24

out = open(script, "a", newline="")
u = first
for n in range(pairs):
	frees = [i for i in sorted(board) if free(i)]
	pair = next(((a, b) for a in frees for b in frees if a < b and board[a][0] == board[b][0]), None)
	if pair is None:
		sys.exit("mouse.py: no free pair left after %d" % n)
	if n < wrong:
		other = next(i for i in frees if board[i][0] != board[pair[0]][0])
		out.write("%d click %d %d\n%d click %d %d\n" % ((u,) + centre(pair[0]) + (u + step // 4,) + centre(other)))
		u += step // 2
	out.write("%d click %d %d\n%d click %d %d\n" % ((u,) + centre(pair[0]) + (u + step // 2,) + centre(pair[1])))
	del board[pair[0]], board[pair[1]]
	u += step
print(u)
