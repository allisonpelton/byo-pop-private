// bwa2_luarun: PopLua headless, on the staged game files.
//
//     bwa2_luarun [-gamedir=<dir>] [-load] [-runall] [-e <source>] [-call <function>] [-compile <in.lua> <out.luc> [chunkname]]
//                [script ...]
//
// -load undumps every scripts/**/*.luc and lists the ones that fail; -runall also runs the main chunk of each in a
// fresh state whose missing globals are a stub that answers every index, call and operator with itself (the chunks
// define classes and functions; the game's C++ side is not here); -e runs a chunk of source; -call calls a
// global function by its dotted name (main.LoadRequiredScripts, as the game does after main.lua); -compile
// compiles a source file with the port's compiler and writes the precompiled chunk (the source name recorded in it
// is chunkname, "@<in.lua>" by default; the game's own chunks have "@scripts\\Name.lua"); each script
// (a path as the game names it, like scripts\main.lua) is run with dofile's rules. The scripts' paths are
// read as the game's: backslashes or slashes, any case (the staged files are lower case), under the game
// folder (default assets/bookwormadv2). Errors are printed with a traceback; the exit code is the number of
// failures.
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "lauxlib.h"
#include "lcore.h"

static std::string gGameDir = "assets/bookwormadv2";

static std::string HostPath(const char* path)
{
	std::string p = path;
	for (char& c : p)
		c = c == '\\' ? '/' : (char)tolower((unsigned char)c);
	while (p.compare(0, 2, "./") == 0)
		p.erase(0, 2);
	return gGameDir + "/" + p;
}

static int ReadFile(const char* path, char** data, size_t* size)
{
	FILE* f = fopen(HostPath(path).c_str(), "rb");
	if (f == NULL)
		return 0;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	*data = (char*)malloc(n > 0 ? (size_t)n : 1);
	*size = n > 0 ? fread(*data, 1, (size_t)n, f) : 0;
	fclose(f);
	return 1;
}

static int FileExists(const char* path)
{
	std::error_code ec;
	return std::filesystem::is_regular_file(HostPath(path), ec) ? 1 : 0;
}

static int Traceback(lua_State* L)
{
	lua_pushliteral(L, "_TRACEBACK");
	lua_rawget(L, LUA_GLOBALSINDEX);
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 1);
		return 1;
	}
	lua_pushvalue(L, 1);
	lua_call(L, 1, 1);
	return 1;
}

static int Report(lua_State* L, int status)
{
	if (status != 0) {
		const char* msg = lua_tostring(L, -1);
		fprintf(stdout, "error: %s\n", msg ? msg : "(not a string)");
		lua_pop(L, 1);
	}
	return status;
}

static int Run(lua_State* L, int status)
{
	if (status == 0) {
		int base = lua_gettop(L);
		lua_pushcfunction(L, Traceback);
		lua_insert(L, base);
		status = lua_pcall(L, 0, 0, base);
		lua_remove(L, base);
	}
	return Report(L, status);
}

static int ChunkWriter(lua_State*, const void* p, size_t sz, void* ud)
{
	return fwrite(p, 1, sz, (FILE*)ud) == sz ? 0 : 1;
}

// Compiles a source file to a precompiled chunk file
static int Compile(lua_State* L, const char* in, const char* out, const char* name)
{
	FILE* f = fopen(in, "rb");
	std::string src, chunkname = name ? name : std::string("@") + in;
	char buf[4096];
	size_t n;
	if (f == NULL) {
		printf("error: cannot open %s\n", in);
		return 1;
	}
	while ((n = fread(buf, 1, sizeof buf, f)) > 0)
		src.append(buf, n);
	fclose(f);
	if (Report(L, luaL_loadbuffer(L, src.data(), src.size(), chunkname.c_str())) != 0)
		return 1;
	f = fopen(out, "wb");
	if (f == NULL) {
		printf("error: cannot write %s\n", out);
		return 1;
	}
	n = (size_t)lua_dump(L, ChunkWriter, f);
	fclose(f);
	return n != 0;
}

static lua_State* NewState()
{
	lua_State* L = lua_open();
	lua_CFunction libs[] = {luaopen_base, luaopen_table, luaopen_io, luaopen_string, luaopen_math, luaopen_debug,
		luaopen_support};
	for (lua_CFunction f : libs) {
		f(L);
		lua_settop(L, 0);
	}
	return L;
}

// Calls a global function by its dotted name (main.LoadRequiredScripts), with no arguments
static int Call(lua_State* L, const char* name)
{
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	for (const char* p = name;;) {
		const char* dot = strchr(p, '.');
		size_t n = dot ? (size_t)(dot - p) : strlen(p);
		lua_pushlstring(L, p, n);
		lua_gettable(L, -2);
		lua_remove(L, -2);
		if (!dot)
			break;
		p = dot + 1;
	}
	if (!lua_isfunction(L, -1)) {
		printf("error: %s is not a function\n", name);
		lua_pop(L, 1);
		return 1;
	}
	return Run(L, 0) != 0;
}

// Undumps every compiled script; prints the failures and a count
static int LoadAll(lua_State* L)
{
	std::vector<std::string> files;
	std::error_code ec;
	std::filesystem::path root = std::filesystem::path(gGameDir) / "scripts";
	for (auto it = std::filesystem::recursive_directory_iterator(root, ec);
		 !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
		if (it->is_regular_file() && it->path().extension() == ".luc")
			files.push_back(std::filesystem::relative(it->path(), gGameDir).generic_string());
	}
	std::sort(files.begin(), files.end());
	int failed = 0;
	for (const std::string& f : files) {
		int status = luaL_loadfile(L, f.c_str());
		if (status != 0) {
			printf("%s: ", f.c_str());
			Report(L, status);
			failed++;
		}
		lua_settop(L, 0);
	}
	printf("loaded %d of %d scripts\n", (int)files.size() - failed, (int)files.size());
	return failed;
}

// What -runall gives each script's state: every global the script does not define is U, a table that answers an
// index, a call and the operators with itself, so that the chunk can define its classes without the game's C++ side
static const char kStubEnvironment[] =
	"U = {}\n"
	"local u = function() return U end\n"
	"setmetatable(U, {__index = u, __call = u, __add = u, __sub = u, __mul = u, __div = u,\n"
	"  __pow = u, __unm = u, __concat = u, __lt = function() return false end, __le = function() return false end})\n"
	"setmetatable(_G, {__index = u})\n";

// Runs the main chunk of every compiled script in a state of its own; prints the failures and a count
static int RunAll(lua_State*)
{
	std::vector<std::string> files;
	std::error_code ec;
	std::filesystem::path root = std::filesystem::path(gGameDir) / "scripts";
	for (auto it = std::filesystem::recursive_directory_iterator(root, ec);
		 !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
		if (it->is_regular_file() && it->path().extension() == ".luc")
			files.push_back(std::filesystem::relative(it->path(), gGameDir).generic_string());
	}
	std::sort(files.begin(), files.end());
	int failed = 0;
	for (const std::string& f : files) {
		lua_State* L = NewState();
		int status = luaL_loadbuffer(L, kStubEnvironment, sizeof kStubEnvironment - 1, "=(stub environment)");
		if (status == 0)
			status = lua_pcall(L, 0, 0, 0);
		if (status == 0)
			status = luaL_loadfile(L, f.c_str());
		if (status == 0) {
			int base = lua_gettop(L);
			lua_pushcfunction(L, Traceback);
			lua_insert(L, base);
			status = lua_pcall(L, 0, 0, base);
			lua_remove(L, base);
		}
		if (status != 0) {
			printf("%s: ", f.c_str());
			Report(L, status);
			failed++;
		}
		lua_close(L);
	}
	printf("ran %d of %d scripts\n", (int)files.size() - failed, (int)files.size());
	return failed;
}

int main(int argc, char** argv)
{
	poplua_setreader(ReadFile);
	poplua_setexists(FileExists);
	lua_State* L = NewState();
	int failed = 0;
	for (int i = 1; i < argc; i++) {
		const char* a = argv[i];
		if (strncmp(a, "-gamedir=", 9) == 0)
			gGameDir = a + 9;
		else if (strcmp(a, "-load") == 0)
			failed += LoadAll(L);
		else if (strcmp(a, "-runall") == 0)
			failed += RunAll(L);
		else if (strcmp(a, "-call") == 0 && i + 1 < argc)
			failed += Call(L, argv[++i]);
		else if (strcmp(a, "-compile") == 0 && i + 2 < argc) {
			const char* in = argv[++i];
			const char* out = argv[++i];
			const char* name = i + 1 < argc && argv[i + 1][0] != '-' ? argv[++i] : NULL;
			failed += Compile(L, in, out, name);
		}
		else if (strcmp(a, "-e") == 0 && i + 1 < argc) {
			const char* src = argv[++i];
			failed += Run(L, luaL_loadbuffer(L, src, strlen(src), "=(command line)")) != 0;
		}
		else
			failed += Run(L, luaL_loadfile(L, a)) != 0;
		lua_settop(L, 0);
	}
	lua_close(L);
	return failed;
}
