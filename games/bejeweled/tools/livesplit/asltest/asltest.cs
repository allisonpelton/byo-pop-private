// asltest: compiles games/bejeweled/tools/livesplit/BejeweledDeluxe-byopop.asl the way LiveSplit 1.8.37's
// ScriptableAutoSplit compiles a script (CSharpCodeProvider with CompilerVersion v4.0, which runs the .NET
// Framework's own csc.exe, a C# 5 compiler; each action in a class of its own with a fixed using list),
// against stubs of the few LiveSplit members the script uses, and replays a poll file through the compiled
// actions with ASL's phase rules (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15, test 5). The poll file
// comes from lsdecide.py, which runs the same scenario through its own re-implementation of the script;
// both must print the same events.
//
// Build (in-box .NET Framework 4 compiler, C# 5):
//   C:/Windows/Microsoft.NET/Framework/v4.0.30319/csc.exe /nologo /out:asltest.exe /r:Microsoft.CSharp.dll
//       /r:System.Windows.Forms.dll asltest.cs
// Run:
//   asltest <script.asl> -compileonly
//   asltest <script.asl> <polls.txt> [-category=<name>] [-segment=<name>]... [-set=<id>=<0|1>]...
//       [-attach=<poll>] [-manualstart=<poll>]
//
// Poll file, one line per poll of the replayed game (from its first poll; -attach skips the ones before):
//   ok <t ms> <the 0xB0 block bytes as hex>   a consistent read of the block (self 1 = "the block's address")
//   torn <t ms>                               no consistent read (the block's seq is odd for every try)
// The fake game: a PE image at 0x400000 whose writable .data section holds the block; the script finds it
// with its own FindBlock. vars.Now (the script's clock) is replaced by the poll times.
//
// Output, one line per event: "start <poll> board=<b>", "split <index> <poll> board=<b>",
// "reset <cause> <poll> board=<b>" (the cause the script's "log" setting writes in onReset, which asltest
// turns on; "exit" for the reset the exit action makes), "manualstart <poll> board=<b>", "messagebox <poll>".
//
// Each action ends where LiveSplit's parser ends it: ScriptableAutoSplit's grammar counts every brace
// character in an action (in strings, chars and comments too) and ends the action where they balance, so
// asltest fails when that is not the action's closing brace.
//
// What it shows: the script compiles as C# 5 with ScriptableAutoSplit's usings, every action runs without
// an exception, and its decisions on the replayed polls. What it does not: that LiveSplit's real members
// behave like these stubs (ReadBytes, MainModuleWow64Safe, the settings, TimerModel), real timing, or the
// Activate flow; the manual LiveSplit test (4.15 test 7) covers those.

using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Diagnostics;
using System.Dynamic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using Microsoft.CSharp;
using LiveSplit.ComponentUtil;
using LiveSplit.Model;

namespace LiveSplit.Model
{
	public enum TimerPhase { NotRunning, Running, Ended, Paused }

	public interface ISegment
	{
		string Name { get; set; }
	}

	public class Segment : ISegment
	{
		public Segment(string name) { Name = name; }
		public string Name { get; set; }
	}

	public interface IRun : IList<ISegment>
	{
		string GameName { get; set; }
		string CategoryName { get; set; }
	}

	public class Run : List<ISegment>, IRun
	{
		public string GameName { get; set; }
		public string CategoryName { get; set; }
	}

	public class LiveSplitState
	{
		public LiveSplitState() { CurrentSplitIndex = -1; }
		public IRun Run { get; set; }
		public TimerPhase CurrentPhase { get; set; }
		public int CurrentSplitIndex { get; set; }
		public event EventHandler OnStart;
		public event EventHandler OnSplit;
		public event EventHandler OnReset;
		internal void Raise(string what)
		{
			EventHandler handler = what == "start" ? OnStart : what == "split" ? OnSplit : OnReset;
			if (handler != null)
				handler(this, EventArgs.Empty);
		}
	}

	public class TimerModel
	{
		public LiveSplitState CurrentState { get; set; }

		public void Start()
		{
			if (CurrentState.CurrentPhase != TimerPhase.NotRunning)
				return;
			CurrentState.CurrentPhase = TimerPhase.Running;
			CurrentState.CurrentSplitIndex = 0;
			CurrentState.Raise("start");
		}

		public void Split()
		{
			if (CurrentState.CurrentPhase != TimerPhase.Running)
				return;
			CurrentState.CurrentSplitIndex++;
			if (CurrentState.CurrentSplitIndex >= CurrentState.Run.Count)
				CurrentState.CurrentPhase = TimerPhase.Ended;
			CurrentState.Raise("split");
		}

		public void Reset()
		{
			if (CurrentState.CurrentPhase == TimerPhase.NotRunning)
				return;
			CurrentState.CurrentPhase = TimerPhase.NotRunning;
			CurrentState.CurrentSplitIndex = -1;
			CurrentState.Raise("reset");
		}
	}
}

namespace LiveSplit.Options
{
	public static class Log
	{
		public static void Info(string message) { AslTest.Program.Note("print: " + message); }
		public static void Error(Exception e) { AslTest.Program.Note("error: " + e.Message); }
	}
}

namespace LiveSplit.ComponentUtil
{
	public class ProcessModuleWow64Safe
	{
		public IntPtr BaseAddress { get; set; }
		public int ModuleMemorySize { get; set; }
		public string ModuleName { get; set; }
		public string FileName { get; set; }
	}

	public static class ExtensionMethods
	{
		public static ProcessModuleWow64Safe MainModuleWow64Safe(this Process process) { return AslTest.FakeGame.Module; }
		public static ProcessModuleWow64Safe[] ModulesWow64Safe(this Process process) { return new ProcessModuleWow64Safe[] { AslTest.FakeGame.Module }; }
		public static byte[] ReadBytes(this Process process, IntPtr address, int count) { return AslTest.FakeGame.Read((long)address, count); }
	}
}

namespace LiveSplit.ASL
{
	public class ASLSetting
	{
		public string Id;
		public bool Value;
		public string Parent;
	}

	public class ASLSettingsBuilder
	{
		public readonly Dictionary<string, ASLSetting> Settings = new Dictionary<string, ASLSetting>();
		public string CurrentDefaultParent { get; set; }

		public void Add(string id, bool default_value = true, string description = null, string parent = null)
		{
			if (Settings.ContainsKey(id))
				throw new ArgumentException("Setting '" + id + "' was already added");
			if (parent == null)
				parent = CurrentDefaultParent;
			if (parent != null && !Settings.ContainsKey(parent))
				throw new ArgumentException("Parent for setting '" + id + "' is not a setting: " + parent);
			Settings[id] = new ASLSetting { Id = id, Value = default_value, Parent = parent };
		}

		public void SetToolTip(string id, string text)
		{
			if (!Settings.ContainsKey(id))
				throw new ArgumentException("Can't set tooltip, '" + id + "' is not a setting");
		}
	}

	public class ASLSettingsReader
	{
		private readonly ASLSettingsBuilder _settings;
		public ASLSettingsReader(ASLSettingsBuilder settings) { _settings = settings; }

		// A setting counts only while its parents are on too
		public dynamic this[string id]
		{
			get
			{
				ASLSetting setting;
				if (!_settings.Settings.TryGetValue(id, out setting))
					return false;
				bool value = setting.Value;
				while (value && setting.Parent != null)
				{
					setting = _settings.Settings[setting.Parent];
					value = setting.Value;
				}
				return value;
			}
		}

		public bool ContainsKey(string id) { return _settings.Settings.ContainsKey(id); }
		public bool StartEnabled { get { return true; } }
		public bool SplitEnabled { get { return true; } }
		public bool ResetEnabled { get { return true; } }
	}
}

namespace AslTest
{
	using LiveSplit.ASL;

	public static class FakeGame
	{
		public const long Base = 0x400000;
		public const int ImageSize = 0x4000;
		public const int DataRva = 0x2000;
		public const long BlockAddress = Base + DataRva + 0x40;
		public static readonly byte[] Image = new byte[ImageSize];
		public static readonly ProcessModuleWow64Safe Module = new ProcessModuleWow64Safe {
			BaseAddress = new IntPtr(Base), ModuleMemorySize = ImageSize, ModuleName = "bejeweled.exe", FileName = "bejeweled.exe" };

		static void Put32(int offset, uint value) { BitConverter.GetBytes(value).CopyTo(Image, offset); }
		static void Put16(int offset, ushort value) { BitConverter.GetBytes(value).CopyTo(Image, offset); }

		// MZ, PE\0\0, a read-only .text and a writable .data
		public static void Build()
		{
			Image[0] = 0x4D;
			Image[1] = 0x5A;
			Put32(0x3C, 0x80);
			Put32(0x80, 0x00004550);
			Put16(0x80 + 6, 2);
			Put16(0x80 + 20, 0xE0);
			int table = 0x80 + 24 + 0xE0;
			Encoding.ASCII.GetBytes(".text").CopyTo(Image, table);
			Put32(table + 8, 0x1000);
			Put32(table + 12, 0x1000);
			Put32(table + 36, 0x60000020);
			Encoding.ASCII.GetBytes(".data").CopyTo(Image, table + 40);
			Put32(table + 40 + 8, 0x1000);
			Put32(table + 40 + 12, DataRva);
			Put32(table + 40 + 36, 0xC0000040);
		}

		// The block as one poll left it: its bytes, self (1 in the poll file: the block's address), and for a
		// torn poll an odd seq
		public static void SetBlock(byte[] block, bool torn)
		{
			byte[] copy = (byte[])block.Clone();
			if (BitConverter.ToUInt32(copy, 0x1C) != 0)
				BitConverter.GetBytes((uint)BlockAddress).CopyTo(copy, 0x1C);
			if (torn)
				BitConverter.GetBytes(BitConverter.ToUInt32(copy, 0x20) | 1).CopyTo(copy, 0x20);
			copy.CopyTo(Image, (int)(BlockAddress - Base));
		}

		public static byte[] Read(long address, int count)
		{
			if (count < 0 || address < Base || address + count > Base + ImageSize)
				return null;
			byte[] result = new byte[count];
			Array.Copy(Image, (int)(address - Base), result, 0, count);
			return result;
		}
	}

	public class AslAction
	{
		public string Name;
		public int Line;      // of the body's first line in the .asl
		public object Instance;
		public MethodInfo Execute;
		public FieldInfo RefreshRate;
	}

	public static class Program
	{
		static readonly List<string> Notes = new List<string>();
		public static void Note(string text) { Notes.Add(text); }

		// The using list and wrapper of ScriptableAutoSplit's ASLMethod (9c1e207, bundled with LiveSplit
		// 1.8.37), as the design read it; MessageBox is redirected to a recorder
		const string Prefix =
			"using System;\n" +
			"using System.Collections.Generic;\n" +
			"using System.Diagnostics;\n" +
			"using System.Dynamic;\n" +
			"using System.IO;\n" +
			"using System.Linq;\n" +
			"using System.Reflection;\n" +
			"using System.Text;\n" +
			"using System.Threading;\n" +
			"using System.Threading.Tasks;\n" +
			"using System.Windows.Forms;\n" +
			"using LiveSplit.ComponentUtil;\n" +
			"using LiveSplit.Model;\n" +
			"using LiveSplit.Options;\n" +
			"public class CompiledScript\n" +
			"{\n" +
			"    public string version;\n" +
			"    public double refreshRate;\n" +
			"    void print(string s) { Log.Info(s); }\n" +
			"    static class MessageBox { public static DialogResult Show(string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon) { AslTest.Program.MessageBoxShown(text); return DialogResult.OK; } }\n" +
			"    public dynamic Execute(LiveSplitState timer, dynamic old, dynamic current, dynamic vars, Process game, dynamic settings)\n" +
			"    {\n" +
			"        var memory = game;\n" +
			"        var modules = game != null ? game.ModulesWow64Safe() : null;\n";
		const int PrefixLines = 24;
		const string Suffix = "\n        return null;\n    }\n}\n";

		static int sMessageBoxes;
		public static void MessageBoxShown(string text) { sMessageBoxes++; }

		// state("name") { ... } and "name { ... }" blocks of the .asl, with the line of each body
		static List<KeyValuePair<string, KeyValuePair<int, string>>> ParseAsl(string text)
		{
			var blocks = new List<KeyValuePair<string, KeyValuePair<int, string>>>();
			int i = 0;
			int line = 1;
			while (i < text.Length)
			{
				char c = text[i];
				if (c == '\n') { line++; i++; continue; }
				if (char.IsWhiteSpace(c)) { i++; continue; }
				if (c == '/' && i + 1 < text.Length && text[i + 1] == '/')
				{
					while (i < text.Length && text[i] != '\n') i++;
					continue;
				}
				if (c == '/' && i + 1 < text.Length && text[i + 1] == '*')
				{
					int end = text.IndexOf("*/", i + 2);
					end = end < 0 ? text.Length : end + 2;
					line += text.Substring(i, end - i).Count(ch => ch == '\n');
					i = end;
					continue;
				}
				int open = text.IndexOf('{', i);
				if (open < 0)
					throw new Exception("line " + line + ": expected a block");
				string head = text.Substring(i, open - i).Trim();
				line += text.Substring(i, open - i).Count(ch => ch == '\n');
				int depth = 0;
				int j = open;
				bool inString = false, inChar = false, inLineComment = false, inBlockComment = false, verbatim = false;
				for (; j < text.Length; j++)
				{
					char d = text[j];
					char next = j + 1 < text.Length ? text[j + 1] : '\0';
					if (inLineComment) { if (d == '\n') inLineComment = false; continue; }
					if (inBlockComment) { if (d == '*' && next == '/') { inBlockComment = false; j++; } continue; }
					if (inString)
					{
						if (verbatim) { if (d == '"' && next == '"') j++; else if (d == '"') inString = false; }
						else if (d == '\\') j++;
						else if (d == '"') inString = false;
						continue;
					}
					if (inChar) { if (d == '\\') j++; else if (d == '\'') inChar = false; continue; }
					if (d == '/' && next == '/') { inLineComment = true; continue; }
					if (d == '/' && next == '*') { inBlockComment = true; continue; }
					if (d == '"') { inString = true; verbatim = j > 0 && text[j - 1] == '@'; continue; }
					if (d == '\'') { inChar = true; continue; }
					if (d == '{') depth++;
					else if (d == '}' && --depth == 0) break;
				}
				if (j >= text.Length)
					throw new Exception("line " + line + ": unclosed block " + head);
				// ScriptableAutoSplit's grammar reads an action's code with a matcher (ASLGrammar's
				// MatchCodeTerminal) that counts every brace character from the opening one, in strings, chars
				// and comments too, and ends the action where they balance: that must be this closing brace
				if (!head.StartsWith("state"))
				{
					int raw = open + 1;
					int stack = 1;
					while (stack > 0)
					{
						int close = text.IndexOf('}', raw);
						if (close < 0)
						{
							raw = -1;
							break;
						}
						for (int k = raw; k < close; k++)
						{
							if (text[k] == '{')
								stack++;
						}
						stack--;
						raw = close + 1;
					}
					if (raw - 1 != j)
					{
						string where = raw < 0 ? "the end of the file" :
							"line " + (line + text.Substring(open, raw - 1 - open).Count(ch => ch == '\n'));
						throw new Exception("line " + line + ": LiveSplit would end " + head + " at " + where + ", not at its closing brace on line " +
							(line + text.Substring(open, j - open).Count(ch => ch == '\n')) + ": its parser counts the brace characters in strings, " +
							"chars and comments too (write them as (char)0x7B and (char)0x7D, keep those in comments balanced)");
					}
				}
				string body = text.Substring(open + 1, j - open - 1);
				blocks.Add(new KeyValuePair<string, KeyValuePair<int, string>>(head, new KeyValuePair<int, string>(line, body)));
				line += text.Substring(open, j - open).Count(ch => ch == '\n');
				i = j + 1;
			}
			return blocks;
		}

		static AslAction Compile(string name, int line, string body, string selfPath)
		{
			var options = new Dictionary<string, string>();
			options["CompilerVersion"] = "v4.0";
			using (var provider = new CSharpCodeProvider(options))
			{
				var parameters = new CompilerParameters { GenerateInMemory = true, CompilerOptions = "/optimize /d:TRACE" };
				foreach (string reference in new[] { "System.dll", "System.Core.dll", "System.Data.dll", "System.Data.DataSetExtensions.dll",
					"System.Drawing.dll", "System.Windows.Forms.dll", "System.Xml.dll", "System.Xml.Linq.dll", "Microsoft.CSharp.dll", selfPath })
					parameters.ReferencedAssemblies.Add(reference);
				CompilerResults results = provider.CompileAssemblyFromSource(parameters, Prefix + body + Suffix);
				var errors = results.Errors.Cast<CompilerError>().Where(e => !e.IsWarning).ToList();
				foreach (CompilerError e in results.Errors.Cast<CompilerError>().Where(e => e.IsWarning))
					Console.Error.WriteLine("asltest: {0} line {1}: warning {2}: {3}", name, line + e.Line - PrefixLines - 1, e.ErrorNumber, e.ErrorText);
				if (errors.Count > 0)
				{
					foreach (CompilerError e in errors)
						Console.Error.WriteLine("asltest: {0} line {1}: error {2}: {3}", name, line + e.Line - PrefixLines - 1, e.ErrorNumber, e.ErrorText);
					return null;
				}
				Type type = results.CompiledAssembly.GetType("CompiledScript");
				return new AslAction { Name = name, Line = line, Instance = Activator.CreateInstance(type), Execute = type.GetMethod("Execute"),
					RefreshRate = type.GetField("refreshRate") };
			}
		}

		static Dictionary<string, AslAction> sActions = new Dictionary<string, AslAction>();
		static double sRefreshRate = 100;
		static LiveSplitState sTimer;
		static ExpandoObject sVars = new ExpandoObject();
		static Process sGame;

		static dynamic Call(string name, object old, object current, object settings)
		{
			AslAction action;
			if (!sActions.TryGetValue(name, out action))
				return null;
			action.RefreshRate.SetValue(action.Instance, sRefreshRate);
			try
			{
				object result = action.Execute.Invoke(action.Instance, new object[] { sTimer, old, current, sVars, sGame, settings });
				sRefreshRate = (double)action.RefreshRate.GetValue(action.Instance);
				return result;
			}
			catch (TargetInvocationException e)
			{
				throw new Exception(name + " threw " + e.InnerException.GetType().Name + ": " + e.InnerException.Message, e.InnerException);
			}
		}

		static ExpandoObject Clone(ExpandoObject state)
		{
			var copy = new ExpandoObject();
			var to = (IDictionary<string, object>)copy;
			foreach (var pair in (IDictionary<string, object>)state)
				to[pair.Key] = pair.Value;
			return copy;
		}

		static bool IsTrue(object result) { return result != null && (bool)result; }

		static int Main(string[] args)
		{
			try
			{
				return Main2(args);
			}
			catch (Exception e)
			{
				Console.Error.WriteLine("asltest: " + e.Message);
				return 2;
			}
		}

		static int Main2(string[] args)
		{
			if (args.Length < 2)
			{
				Console.Error.WriteLine("usage: asltest <script.asl> (-compileonly | <polls.txt> [-category=..] [-segment=..]... [-set=id=0|1]... [-attach=n] [-manualstart=n])");
				return 2;
			}
			string selfPath = Assembly.GetExecutingAssembly().Location;
			var blocks = ParseAsl(File.ReadAllText(args[0]));
			int compiled = 0;
			bool failed = false;
			var states = new List<string>();
			foreach (var block in blocks)
			{
				if (block.Key.StartsWith("state"))
				{
					states.Add(block.Key);
					continue;
				}
				AslAction action = Compile(block.Key, block.Value.Key, block.Value.Value, selfPath);
				if (action == null)
					failed = true;
				else
				{
					sActions[block.Key] = action;
					compiled++;
				}
			}
			if (failed)
				return 1;
			if (args[1] == "-compileonly")
			{
				Console.WriteLine("compiled {0} actions ({1}) with the .NET Framework C# compiler (CompilerVersion v4.0); states: {2}",
					compiled, string.Join(", ", sActions.Keys.ToArray()), string.Join(", ", states.ToArray()));
				return 0;
			}

			var run = new Run { GameName = "Bejeweled Deluxe", CategoryName = "" };
			var overrides = new Dictionary<string, bool>();
			overrides["log"] = true;
			int attach = 0;
			int manualStart = -1;
			foreach (string arg in args.Skip(2))
			{
				if (arg.StartsWith("-category="))
					run.CategoryName = arg.Substring(10);
				else if (arg.StartsWith("-segment="))
					run.Add(new Segment(arg.Substring(9)));
				else if (arg.StartsWith("-set="))
				{
					string[] parts = arg.Substring(5).Split('=');
					overrides[parts[0]] = parts[1] == "1";
				}
				else if (arg.StartsWith("-attach="))
					attach = int.Parse(arg.Substring(8), CultureInfo.InvariantCulture);
				else if (arg.StartsWith("-manualstart="))
					manualStart = int.Parse(arg.Substring(13), CultureInfo.InvariantCulture);
				else
					throw new Exception("unknown argument " + arg);
			}

			var polls = new List<KeyValuePair<int, byte[]>>();   // (t, block); a null block: torn
			foreach (string raw in File.ReadAllLines(args[1]))
			{
				string[] parts = raw.Trim().Split(' ');
				if (parts.Length < 2)
					continue;
				int t = int.Parse(parts[1], CultureInfo.InvariantCulture);
				if (parts[0] == "torn")
					polls.Add(new KeyValuePair<int, byte[]>(t, null));
				else
				{
					byte[] block = new byte[parts[2].Length / 2];
					for (int k = 0; k < block.Length; k++)
						block[k] = byte.Parse(parts[2].Substring(2 * k, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture);
					polls.Add(new KeyValuePair<int, byte[]>(t, block));
				}
			}

			FakeGame.Build();
			sGame = Process.GetCurrentProcess();
			sTimer = new LiveSplitState { Run = run, CurrentPhase = TimerPhase.NotRunning };
			var builder = new ASLSettingsBuilder();
			var reader = new ASLSettingsReader(builder);
			var model = new TimerModel { CurrentState = sTimer };
			string logPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "BejeweledDeluxe-byopop.log");
			if (File.Exists(logPath))
				File.Delete(logPath);
			int logLinesRead = 0;
			var output = new List<string>();
			int poll = -1;
			bool inExit = false;
			bool inManual = false;
			ExpandoObject current = new ExpandoObject();
			ExpandoObject old = null;
			// The script's log names the reset cause; the exit action resets through vars.TimerModel
			Func<string> nextCause = () => {
				if (!File.Exists(logPath))
					return "?";
				string[] lines = File.ReadAllLines(logPath);
				string cause = "?";
				while (logLinesRead < lines.Length)
				{
					string[] words = lines[logLinesRead++].Split(' ');
					if (words.Length > 3 && words[2] == "reset")
						cause = words[3];
				}
				return cause;
			};
			Func<string> board = () => "board=" + Convert.ToInt64(((IDictionary<string, object>)current)["boardSerial"]);
			sTimer.OnStart += (s, e) => {
				Call("onStart", old, current, reader);
				output.Add((inManual ? "manualstart " : "start ") + poll + " " + board());
			};
			sTimer.OnSplit += (s, e) => {
				Call("onSplit", old, current, reader);
				output.Add("split " + (sTimer.CurrentSplitIndex - 1) + " " + poll + " " + board());
			};
			// onReset writes the script's log line (and clears RunSerial): the cause is read after it. The exit
			// action's reset must log "exit"; its board is the run's
			sTimer.OnReset += (s, e) => {
				string where = poll + " " + (inExit ? "board=" + Convert.ToInt64(((dynamic)sVars).RunSerial) : board());
				Call("onReset", old, current, reader);
				output.Add("reset " + nextCause() + " " + where);
			};

			Call("startup", null, null, builder);
			foreach (var pair in overrides)
			{
				if (!builder.Settings.ContainsKey(pair.Key))
					throw new Exception("no setting " + pair.Key);
				builder.Settings[pair.Key].Value = pair.Value;
			}
			int now = 0;
			((dynamic)sVars).Now = (Func<int>)(() => now);
			if (sRefreshRate != 100)
				Console.Error.WriteLine("asltest: refreshRate {0}", sRefreshRate);

			bool initialised = false;
			for (poll = attach; poll < polls.Count; poll++)
			{
				now = polls[poll].Key;
				FakeGame.SetBlock(polls[poll].Value ?? FakeGame.Read(FakeGame.BlockAddress, 0xB0), polls[poll].Value == null);
				if (!initialised)
				{
					initialised = true;
					Call("init", null, current, reader);
				}
				old = Clone(current);
				int boxes = sMessageBoxes;
				object updated = Call("update", old, current, reader);
				if (sMessageBoxes != boxes)
					output.Add("messagebox " + poll);
				if (updated == null || IsTrue(updated))
				{
					if (sTimer.CurrentPhase == TimerPhase.Running || sTimer.CurrentPhase == TimerPhase.Paused)
					{
						if (IsTrue(Call("reset", old, current, reader)))
							model.Reset();
						else if (IsTrue(Call("split", old, current, reader)))
							model.Split();
					}
					if (sTimer.CurrentPhase == TimerPhase.NotRunning && IsTrue(Call("start", old, current, reader)))
						model.Start();
				}
				if (poll == manualStart)
				{
					inManual = true;
					model.Start();
					inManual = false;
				}
			}
			poll = polls.Count;
			inExit = true;
			// LiveSplit's ASLScript.DoExit clears its process, then runs exit without old and current
			// (RunNoProcessMethod); the onReset of exit's reset gets the last old and current (RunMethod)
			sGame = null;
			Call("exit", null, null, reader);
			foreach (string line in output)
				Console.WriteLine(line);
			foreach (string note in Notes)
				Console.Error.WriteLine("asltest: " + note);
			return 0;
		}
	}
}
