// LiveSplit auto splitter for Bejeweled Deluxe on byo-pop, the decompiled port of Bejeweled Deluxe 1.87
// (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15). The port fills a tagged, read-only block in its .data
// while its "livesplit" mod is on; this script finds the block and starts, splits and resets the timer:
//   start  at GO (board tick 150) of a new game, not a resumed save; the category's mode must match
//   split  segment names that are numbers of at least 1000 ("10000", "25,000", "25K") split when the score
//          on screen reaches them; any other segment name is a level and splits at the wormhole
//   reset  when the run's game ends: no moves, time up, New Game, a mode switch, quitting (each switchable)
// Real time only: there is no game time or load removal, as speedrun.com times the game.
// Written for LiveSplit 1.8.37's ScriptableAutoSplit (C# 5: the .NET Framework compiler).
// SPDX-License-Identifier: CC0-1.0. To the extent possible under law, the authors have waived all copyright
// and related rights to this script: https://creativecommons.org/publicdomain/zero/1.0/

state("bejeweled") {}

startup
{
	refreshRate = 100;

	settings.Add("modeGuard", true, "Start only on a board of the category's mode");
	settings.SetToolTip("modeGuard", "A category name containing \"Timetrial\" starts only on a Timetrial game, one containing \"Normal\" only on a Normal game.");
	settings.Add("vanillaOnly", false, "Start only without gameplay mods and without a chosen seed");
	settings.SetToolTip("vanillaOnly", "Off: seeded and modded games start too (practice). The game shows mods and seeds on screen anyway.");
	settings.Add("resets", true, "Reset when the run's game ends");
	settings.SetToolTip("resets", "Never resets a finished run: that stays until you reset it.");
	settings.Add("resetNoMoves", true, "No moves left", "resets");
	settings.Add("resetTimeUp", true, "Time up", "resets");
	settings.Add("resetNewGame", true, "New Game", "resets");
	settings.Add("resetReplaced", true, "Another game replaces the run's game (mode switch)", "resets");
	settings.Add("resetQuit", true, "The game quits", "resets");
	settings.Add("log", false, "Log each event and its lag to BejeweledDeluxe-byopop.log in LiveSplit's folder");

	// The block, layout 1 (games/bejeweled/src/LiveSplitBlock.h): "name offset type" per field.
	// games/bejeweled/tools/livesplit/check_asl_offsets.py compares this table and the magic with the game's.
	vars.Magic = new byte[] { 0x42, 0x59, 0x4F, 0x50, 0x4F, 0x50, 0x2D, 0x4C, 0x49, 0x56, 0x45, 0x53, 0x50, 0x4C, 0x49, 0x54, 0xF0, 0xF1, 0xF2, 0xF3 };
	vars.GameId = 0x58444A42L;
	vars.LayoutVersion = 1;
	vars.BlockSize = 0xB0;
	vars.Fields = new string[] {
		"gameId 0x14 u32", "layoutVersion 0x18 u16", "size 0x1A u16", "self 0x1C u32", "seq 0x20 u32", "active 0x24 u32",
		"updates 0x28 u32", "flags 0x2C u32", "boardSerial 0x30 u32", "goCount 0x34 u32", "goSerial 0x38 u32",
		"levelClears 0x3C u32", "levelClearSerial 0x40 u32", "gameOvers 0x44 u32", "gameOverSerial 0x48 u32",
		"deathsNoMoves 0x4C u32", "deathsTimeUp 0x50 u32", "deathsNewGame 0x54 u32", "deathsOther 0x58 u32",
		"tick 0x5C i32", "state 0x60 i32", "level 0x64 i32", "displayLevel 0x68 i32", "displayScore 0x6C i32",
		"score 0x70 i32", "boardLevelClears 0x74 i32", "goTick 0x78 i32", "displayScoreMax 0x7C i32",
		"qpc 0xA0 u64", "qpcFrequency 0xA8 u64" };
	vars.FlagTimed = 0x01L;
	vars.FlagVanilla = 0x04L;
	vars.FlagReplay = 0x20L;
	vars.FlagGameOver = 0x40L;

	vars.Mode = "none";
	vars.Target = new long[0];
	vars.IsScore = new bool[0];
	vars.RunSerial = 0L;
	vars.PendingReset = "";
	vars.TimerModel = new TimerModel { CurrentState = timer };
	// The clock for the rescan and the message below (a test harness may replace it)
	vars.Now = (Func<int>)(() => Environment.TickCount);

	// The block's address in the game (0: not found). The main module's PE header from the game's memory,
	// then each writable section (.data, .tls) in one read, at 4-byte steps: the magic, the game id, a
	// layout and size this script reads, and self either 0 (not active yet) or the hit's own address.
	vars.FindBlock = (Func<Process, long>)(p => {
		byte[] magic = (byte[])vars.Magic;
		var module = p.MainModuleWow64Safe();
		long moduleBase = (long)module.BaseAddress;
		int moduleSize = module.ModuleMemorySize;
		if (moduleSize <= 0)
			return 0L;
		byte[] header = p.ReadBytes(module.BaseAddress, Math.Min(0x1000, moduleSize));
		if (header == null || header.Length < 0x40 || header[0] != 0x4D || header[1] != 0x5A)
			return 0L;
		int pe = BitConverter.ToInt32(header, 0x3C);
		if (pe <= 0 || pe + 24 > header.Length || BitConverter.ToUInt32(header, pe) != 0x00004550)
			return 0L;
		int sections = BitConverter.ToUInt16(header, pe + 6);
		int table = pe + 24 + BitConverter.ToUInt16(header, pe + 20);
		for (int i = 0; i < sections; i++)
		{
			int entry = table + i * 40;
			if (entry + 40 > header.Length)
				break;
			int virtualSize = BitConverter.ToInt32(header, entry + 8);
			int virtualAddress = BitConverter.ToInt32(header, entry + 12);
			uint characteristics = BitConverter.ToUInt32(header, entry + 36);
			if ((characteristics & 0x80000000u) == 0 || virtualSize <= 0 || virtualAddress <= 0 ||
				(long)virtualAddress + virtualSize > moduleSize)
				continue;
			byte[] data = p.ReadBytes(new IntPtr(moduleBase + virtualAddress), virtualSize);
			if (data == null)
				continue;
			for (int at = 0; at + (int)vars.BlockSize <= data.Length; at += 4)
			{
				if (data[at] != magic[0])
					continue;
				bool same = true;
				for (int m = 1; m < magic.Length && same; m++)
					same = data[at + m] == magic[m];
				if (!same || BitConverter.ToUInt32(data, at + 0x14) != (uint)(long)vars.GameId ||
					BitConverter.ToUInt16(data, at + 0x18) < (int)vars.LayoutVersion || BitConverter.ToUInt16(data, at + 0x1A) < (int)vars.BlockSize)
					continue;
				long address = moduleBase + virtualAddress + at;
				uint self = BitConverter.ToUInt32(data, at + 0x1C);
				if (self != 0 && (long)self != address)
					continue;
				return address;
			}
		}
		return 0L;
	});

	// One consistent copy of the block, or null: seq, the block, seq again; it counts when both seqs are
	// equal and even and the copy's own seq is the same (the game makes seq odd while it writes). Up to 3
	// retries, then the cycle is skipped.
	vars.ReadBlock = (Func<Process, long, Dictionary<string, long>>)((p, address) => {
		string[] fields = (string[])vars.Fields;
		int size = (int)vars.BlockSize;
		for (int attempt = 0; attempt < 4; attempt++)
		{
			byte[] seq1 = p.ReadBytes(new IntPtr(address + 0x20), 4);
			byte[] block = p.ReadBytes(new IntPtr(address), size);
			byte[] seq2 = p.ReadBytes(new IntPtr(address + 0x20), 4);
			if (seq1 == null || block == null || seq2 == null || block.Length < size)
				return null;
			uint s1 = BitConverter.ToUInt32(seq1, 0);
			if (s1 != BitConverter.ToUInt32(seq2, 0) || (s1 & 1) != 0 || BitConverter.ToUInt32(block, 0x20) != s1)
				continue;
			var snapshot = new Dictionary<string, long>();
			foreach (string field in fields)
			{
				string[] parts = field.Split(' ');
				int offset = Convert.ToInt32(parts[1], 16);
				long value;
				if (parts[2] == "u16")
					value = BitConverter.ToUInt16(block, offset);
				else if (parts[2] == "i32")
					value = BitConverter.ToInt32(block, offset);
				else if (parts[2] == "u64")
					value = (long)BitConverter.ToUInt64(block, offset);
				else
					value = BitConverter.ToUInt32(block, offset);
				snapshot[parts[0]] = value;
			}
			return snapshot;
		}
		return null;
	});

	// The block's fields into the ASL state (current), each a long
	vars.CopyFields = (Action<dynamic, Dictionary<string, long>>)((state, s) => {
		state.self = s["self"]; state.seq = s["seq"]; state.active = s["active"]; state.updates = s["updates"];
		state.flags = s["flags"]; state.boardSerial = s["boardSerial"]; state.goCount = s["goCount"];
		state.goSerial = s["goSerial"]; state.levelClears = s["levelClears"]; state.levelClearSerial = s["levelClearSerial"];
		state.gameOvers = s["gameOvers"]; state.gameOverSerial = s["gameOverSerial"]; state.deathsNoMoves = s["deathsNoMoves"];
		state.deathsTimeUp = s["deathsTimeUp"]; state.deathsNewGame = s["deathsNewGame"]; state.deathsOther = s["deathsOther"];
		state.tick = s["tick"]; state.state = s["state"]; state.level = s["level"]; state.displayLevel = s["displayLevel"];
		state.displayScore = s["displayScore"]; state.score = s["score"]; state.boardLevelClears = s["boardLevelClears"];
		state.goTick = s["goTick"]; state.displayScoreMax = s["displayScoreMax"]; state.qpc = s["qpc"];
		state.qpcFrequency = s["qpcFrequency"];
	});
	var zero = new Dictionary<string, long>();
	foreach (string field in (string[])vars.Fields)
		zero[field.Split(' ')[0]] = 0L;
	vars.Zero = zero;

	// The segments: a "-" (subsplit) prefix or a section prefix in braces is dropped; a number of at least
	// 1000 ("10000", "25,000", "25K") is a score split (vars.IsScore, vars.Target = the score), anything else
	// the next level split (vars.Target = how many level clears it needs)
	vars.ParseSplits = (Action<LiveSplitState>)(t => {
		// The brace characters as char codes: ScriptableAutoSplit ends an action at the brace that balances
		// its opening one, counting every brace character in the text, in strings, chars and comments too
		char sectionOpen = (char)0x7B, sectionClose = (char)0x7D;
		int count = t.Run.Count;
		long[] target = new long[count];
		bool[] isScore = new bool[count];
		int levels = 0;
		for (int i = 0; i < count; i++)
		{
			string name = (t.Run[i].Name ?? "").Trim();
			if (name.StartsWith("-"))
				name = name.Substring(1).Trim();
			if (name.Length > 0 && name[0] == sectionOpen && name.IndexOf(sectionClose) > 0)
				name = name.Substring(name.IndexOf(sectionClose) + 1).Trim();
			string digits = name.Replace(",", "");
			long scale = 1;
			if (digits.EndsWith("K") || digits.EndsWith("k"))
			{
				scale = 1000;
				digits = digits.Substring(0, digits.Length - 1);
			}
			bool isNumber = digits.Length > 0 && digits.Length <= 12;
			long value = 0;
			for (int c = 0; c < digits.Length && isNumber; c++)
			{
				isNumber = digits[c] >= '0' && digits[c] <= '9';
				value = value * 10 + (digits[c] - '0');
			}
			if (isNumber && value * scale >= 1000)
			{
				isScore[i] = true;
				target[i] = value * scale;
			}
			else
				target[i] = ++levels;
		}
		vars.Target = target;
		vars.IsScore = isScore;
	});

	// Whether the run's current segment is reached in this snapshot of the run's board: a score segment by
	// the highest score the HUD showed (displayScoreMax), the k-th level segment by the board's k-th level
	// clear. split splits on it; reset holds a game over back for it. Both fields are read in every call (see
	// the "log" setting below): a run whose segments mix scores and levels meets no first read when it splits
	vars.Reached = (Func<dynamic, LiveSplitState, bool>)((state, t) => {
		if ((long)vars.RunSerial == 0 || (long)state.boardSerial != (long)vars.RunSerial)
			return false;
		long scoreMax = (long)state.displayScoreMax, levelClears = (long)state.boardLevelClears;
		if (t.Run.Count != ((long[])vars.Target).Length)
			vars.ParseSplits(t);
		long[] target = (long[])vars.Target;
		bool[] isScore = (bool[])vars.IsScore;
		int index = t.CurrentSplitIndex;
		if (index < 0 || index >= target.Length)
			return false;
		return isScore[index] ? scoreMax >= target[index] : levelClears >= target[index];
	});

	// The "log" setting: an event, the board, its tick and the lag bounds. The event happened after the
	// previous copy's write and at or before this copy's (both stamped with the game's QPC, the same clock
	// as Stopwatch). Nothing between a decision and LiveSplit's timer may run for the first time: the C#
	// binder compiles a dynamic call site the first time it runs (about a millisecond each), and a first
	// DateTime.Now or file write takes milliseconds too. So start, split and reset (whose cycle may split)
	// read the block's fields, the vars, the settings and LogEvent in every cycle; LogEvent takes typed
	// values, only stamps the decision and keeps the line, and runs once here; WriteLog adds the time and
	// appends the line in onStart, onSplit or onReset, after LiveSplit took the event's time. update drops a
	// line no timer event took.
	string[] logLine = new string[1];
	DateTime[] logTime = new DateTime[1];
	Action<string, long, long, long, long, long> logEvent = (what, board, tick, qpc, frequency, beforeQpc) => {
		long stamp = Stopwatch.GetTimestamp();
		logTime[0] = DateTime.UtcNow;
		string lag = "";
		if (frequency > 0 && qpc > 0)
		{
			double newest = (stamp - qpc) * 1000.0 / frequency;
			double oldest = beforeQpc > 0 ? (stamp - beforeQpc) * 1000.0 / frequency : newest;
			lag = " lag_ms=" + newest.ToString("F1", System.Globalization.CultureInfo.InvariantCulture) + ".." +
				oldest.ToString("F1", System.Globalization.CultureInfo.InvariantCulture);
		}
		logLine[0] = what + " board=" + board + " tick=" + tick + lag;
	};
	logEvent("warm-up", 0, 0, 1, 1, 1);
	logLine[0] = null;
	vars.LogEvent = logEvent;
	vars.DropLog = (Action)(() => logLine[0] = null);
	vars.WriteLog = (Action)(() => {
		string line = logLine[0];
		logLine[0] = null;
		if (line != null)
		{
			try
			{
				File.AppendAllText(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "BejeweledDeluxe-byopop.log"),
					logTime[0].ToLocalTime().ToString("yyyy-MM-dd HH:mm:ss.fff", System.Globalization.CultureInfo.InvariantCulture) +
					" " + line + Environment.NewLine);
			}
			catch (Exception)
			{
			}
		}
	});
	vars.ExitReset = false;
}

init
{
	// ASL keeps the state object across reconnects: every field starts at 0 for this process
	vars.CopyFields(current, vars.Zero);
	vars.Mode = "port";
	vars.Block = 0L;
	vars.AttachTick = vars.Now();
	vars.LastScan = vars.Now();
	vars.Baseline = true;
	vars.SeenActive = false;
	vars.Warned = false;
	try
	{
		vars.Block = vars.FindBlock(game);
	}
	catch (Exception e)
	{
		print("Bejeweled Deluxe (byo-pop): the block is not readable yet: " + e.Message);
	}
}

update
{
	// A log line whose timer event did not come (its start, split or reset box unticked) is dropped
	vars.DropLog();
	if (vars.Mode != "port")
		return false;
	try
	{
		// Until the block is found (the game may still be starting), look again once a second
		if (vars.Block == 0)
		{
			if (unchecked((int)vars.Now() - (int)vars.LastScan) < 1000)
				return false;
			vars.LastScan = vars.Now();
			vars.Block = vars.FindBlock(game);
			if (vars.Block == 0)
				return false;
		}
		Dictionary<string, long> s = vars.ReadBlock(game, (long)vars.Block);
		if (s == null)
			return false;
		if (s["active"] == 0)
		{
			// Not active: the mod is off (once per attach, after the game had 10 s to start), or the game quits
			if (!vars.SeenActive && !vars.Warned && unchecked((int)vars.Now() - (int)vars.AttachTick) > 10000)
			{
				vars.Warned = true;
				MessageBox.Show("Bejeweled Deluxe (byo-pop) runs without its LiveSplit block. Turn on the game's \"livesplit\" mod and restart the game; until then the auto splitter cannot start, split or reset.",
					"LiveSplit auto splitter", MessageBoxButtons.OK, MessageBoxIcon.Information);
			}
			vars.Baseline = true;
			return false;
		}
		vars.SeenActive = true;
		vars.CopyFields(current, s);
		// The first copy after attaching or activation is only a baseline: a game in progress is never a run
		if (vars.Baseline)
		{
			vars.Baseline = false;
			return false;
		}
		return true;
	}
	catch (Exception e)
	{
		print("Bejeweled Deluxe (byo-pop): " + e.Message);
		vars.Block = 0L;
		vars.Baseline = true;
		return false;
	}
}

start
{
	// Every dynamic read runs in every cycle, none for the first time in the cycle that starts the timer
	long goCount = current.goCount, oldGoCount = old.goCount, goSerial = current.goSerial, board = current.boardSerial;
	long flags = current.flags, tick = current.tick, qpc = current.qpc, frequency = current.qpcFrequency, oldQpc = old.qpc;
	long flagReplay = vars.FlagReplay, flagVanilla = vars.FlagVanilla, flagTimed = vars.FlagTimed;
	bool vanillaOnly = settings["vanillaOnly"], modeGuard = settings["modeGuard"], log = settings["log"];
	var logEvent = (Action<string, long, long, long, long, long>)vars.LogEvent;
	if (goCount == oldGoCount || goSerial != board || (flags & flagReplay) != 0)
		return false;
	if (vanillaOnly && (flags & flagVanilla) == 0)
		return false;
	if (modeGuard)
	{
		string category = timer.Run.CategoryName ?? "";
		bool timed = (flags & flagTimed) != 0;
		if (category.IndexOf("Timetrial", StringComparison.OrdinalIgnoreCase) >= 0 && !timed)
			return false;
		if (category.IndexOf("Normal", StringComparison.OrdinalIgnoreCase) >= 0 && timed)
			return false;
	}
	if (log)
		logEvent("start", board, tick, qpc, frequency, oldQpc);
	return true;
}

onStart
{
	// A manual start too: the run then belongs to the board on screen (a resumed save: practice)
	vars.ParseSplits(timer);
	vars.RunSerial = 0L;
	vars.PendingReset = "";
	if (current != null && (vars.Mode == "port" || vars.Mode == "original"))
		vars.RunSerial = (long)current.boardSerial;
	vars.WriteLog();
}

split
{
	// As in start: every dynamic read in every cycle
	int index = timer.CurrentSplitIndex;
	long board = current.boardSerial, tick = current.tick, qpc = current.qpc, frequency = current.qpcFrequency, oldQpc = old.qpc;
	bool log = settings["log"];
	var logEvent = (Action<string, long, long, long, long, long>)vars.LogEvent;
	bool reached = vars.Reached(current, timer);
	if (reached && log)
		logEvent("split " + index, board, tick, qpc, frequency, oldQpc);
	return reached;
}

onSplit
{
	vars.WriteLog();
}

reset
{
	// As in start and split: every dynamic read in every cycle. A poll that sees the run's game end may also
	// split, in this cycle after this action (below)
	long runSerial = vars.RunSerial, board = current.boardSerial, oldBoard = old.boardSerial, oldFlags = old.flags;
	long gameOvers = current.gameOvers, oldGameOvers = old.gameOvers, gameOverSerial = current.gameOverSerial;
	long noMoves = current.deathsNoMoves, oldNoMoves = old.deathsNoMoves, timeUp = current.deathsTimeUp;
	long oldTimeUp = old.deathsTimeUp, newGame = current.deathsNewGame, oldNewGame = old.deathsNewGame;
	long tick = current.tick, qpc = current.qpc, frequency = current.qpcFrequency, oldQpc = old.qpc;
	long flagGameOver = vars.FlagGameOver;
	string pending = vars.PendingReset;
	bool resets = settings["resets"], splitEnabled = settings.SplitEnabled, log = settings["log"];
	var logEvent = (Action<string, long, long, long, long, long>)vars.LogEvent;
	bool reached = vars.Reached(current, timer);
	if (!resets || runSerial == 0)
		return false;
	// The run's game ended in this cycle, or in an earlier one that held its reset back (below)
	string cause = pending;
	if (gameOvers != oldGameOvers && gameOverSerial == runSerial)
	{
		if (noMoves != oldNoMoves)
			cause = "resetNoMoves";
		else if (timeUp != oldTimeUp)
			cause = "resetTimeUp";
		else if (newGame != oldNewGame)
			cause = "resetNewGame";
		else
			cause = "resets";
	}
	else if (oldBoard == runSerial && board != runSerial && (oldFlags & flagGameOver) == 0)
		cause = board == 0 ? "resetQuit" : "resetReplaced";
	// The cause's setting, read in every cycle too ("resets" when there is no cause)
	bool allowed = settings[string.IsNullOrEmpty(cause) ? "resets" : cause];
	allowed = allowed && !string.IsNullOrEmpty(cause);
	// One poll saw the game over and also the current segment reached, which happened by the game over (the
	// score was on screen, or the wormhole started) and counts: split in this cycle, reset in a later one
	bool holdBack = allowed && splitEnabled && reached;
	vars.PendingReset = holdBack ? cause : "";
	if (!allowed || holdBack)
		return false;
	if (log)
		logEvent("reset " + cause, board, tick, qpc, frequency, oldQpc);
	return true;
}

onReset
{
	// The exit action's reset: LiveSplit runs exit without old and current, onReset with the last copies
	if (vars.ExitReset && settings["log"] && current != null)
		((Action<string, long, long, long, long, long>)vars.LogEvent)("reset exit", (long)current.boardSerial,
			(long)current.tick, (long)current.qpc, (long)current.qpcFrequency, (long)current.qpc);
	vars.WriteLog();
	vars.RunSerial = 0L;
	vars.PendingReset = "";
}

exit
{
	// The game is gone (quit or crash): the run's game ended
	var phase = timer.CurrentPhase;
	if ((phase == TimerPhase.Running || phase == TimerPhase.Paused) && vars.RunSerial != 0 && settings.ResetEnabled &&
		settings["resets"] && settings["resetQuit"])
	{
		vars.ExitReset = true;
		try
		{
			vars.TimerModel.Reset();
		}
		finally
		{
			vars.ExitReset = false;
		}
	}
	vars.RunSerial = 0L;
	vars.PendingReset = "";
	vars.Mode = "none";
}
