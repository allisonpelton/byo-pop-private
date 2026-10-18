#include "WebPersist.h"
#include "VFS.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS_DEPS(webpersist, "$UTF8ToString");

// Queues a file for the next webpersist_backup (copied now: the C++ buffer goes away).
EM_JS(void, webpersist_stage, (const char* thePath, const unsigned char* theData, int theLen), {
	var staged = Module['sexyBackupStaged'] || (Module['sexyBackupStaged'] = []);
	staged.push({ path: UTF8ToString(thePath), bytes: HEAPU8.slice(theData, theData + theLen) });
});

// The backup is one localStorage item per user dir:
//   { v: 1, time, files: { "<path relative to the user dir>": { m: <mtime ms>, d: <base64> } } }
// Files over 1 MB are listed without data (d), so that they are never taken for deleted ones.
// (EM_JS bodies go through the C preprocessor: no empty '' strings.)
EM_JS(void, webpersist_backup, (const char* theDir), {
	var dir = UTF8ToString(theDir);
	var staged = Module['sexyBackupStaged'] || [];
	Module['sexyBackupStaged'] = [];
	var toBase64 = function(bytes) {
		var bin = "";
		for (var i = 0; i < bytes.length; i += 0x8000)
			bin += String.fromCharCode.apply(null, bytes.subarray(i, i + 0x8000));
		return btoa(bin);
	};
	var files = {};
	var walk = function(path, rel) {
		var names;
		try { names = FS.readdir(path); } catch (e) { return; }
		names.forEach(function(name) {
			if ((name === '.') || (name === '..'))
				return;
			var p = path + '/' + name;
			var r = rel ? rel + '/' + name : name;
			var st;
			try { st = FS.stat(p); } catch (e) { return; }
			if (FS.isDir(st.mode)) {
				walk(p, r);
				return;
			}
			var entry = { m: st.mtime.getTime() };
			if (st.size <= 1048576) {
				try { entry.d = toBase64(FS.readFile(p)); } catch (e) {}
			}
			files[r] = entry;
		});
	};
	walk(dir, "");

	var now = Date.now();
	staged.forEach(function(s) {
		var real = null;
		try { real = FS.readFile(dir + '/' + s.path); } catch (e) {}
		var same = (real !== null) && (real.length === s.bytes.length);
		for (var i = 0; same && (i < real.length); i++)
			same = real[i] === s.bytes[i];
		if (!same)
			files[s.path] = { m: now, d: toBase64(s.bytes) };
	});

	var key = 'sexy-userdata-backup:' + dir;
	try {
		localStorage.setItem(key, JSON.stringify({ v: 1, time: now, files: files }));
	} catch (e) {
		// Unavailable, or full: an older backup would now be out of date
		try { localStorage.removeItem(key); } catch (e2) {}
	}
});

// Returns the number of files written or removed, or -1 if there was no backup.
EM_JS(int, webpersist_restore, (const char* theDir), {
	var dir = UTF8ToString(theDir);
	var key = 'sexy-userdata-backup:' + dir;
	var raw = null;
	try { raw = localStorage.getItem(key); } catch (e) { return -1; }
	if (!raw)
		return -1;
	var backup = null;
	try { backup = JSON.parse(raw); } catch (e) {}
	if (!backup || (backup.v !== 1) || (typeof backup.files !== 'object') || (typeof backup.time !== 'number')) {
		try { localStorage.removeItem(key); } catch (e) {}
		return -1;
	}

	var has = function(rel) { return Object.prototype.hasOwnProperty.call(backup.files, rel); };
	var mtimeOf = function(p) {
		try { return FS.stat(p).mtime.getTime(); } catch (e) { return -1; }
	};
	var changed = 0;

	// Files written after the last sync that completed
	Object.keys(backup.files).forEach(function(rel) {
		var f = backup.files[rel];
		if ((typeof f.d !== 'string') || (typeof f.m !== 'number'))
			return;
		var p = dir + '/' + rel;
		if (f.m <= mtimeOf(p))
			return;
		try {
			var bin = atob(f.d);
			var bytes = new Uint8Array(bin.length);
			for (var i = 0; i < bin.length; i++)
				bytes[i] = bin.charCodeAt(i);
			FS.mkdirTree(p.substring(0, p.lastIndexOf('/')));
			FS.writeFile(p, bytes);
			FS.utime(p, f.m, f.m);
			changed++;
		} catch (e) {
			console.warn('Restoring ' + p + ' failed:', e);
		}
	});

	// Files deleted before the backup was taken, and not written again since
	var walk = function(path, rel) {
		var names;
		try { names = FS.readdir(path); } catch (e) { return; }
		names.forEach(function(name) {
			if ((name === '.') || (name === '..'))
				return;
			var p = path + '/' + name;
			var r = rel ? rel + '/' + name : name;
			var st;
			try { st = FS.stat(p); } catch (e) { return; }
			if (FS.isDir(st.mode))
				walk(p, r);
			else if ((!has(r)) && (st.mtime.getTime() < backup.time)) {
				try { FS.unlink(p); changed++; } catch (e) {}
			}
		});
	};
	walk(dir, "");
	return changed;
});

EM_JS(void, webpersist_sync, (), {
	if ((typeof FS === 'undefined') || !FS.syncfs)
		return;
	if (Module['sexySyncBusy']) {
		Module['sexySyncPending'] = true;
		return;
	}
	var sync = function() {
		Module['sexySyncBusy'] = true;
		Module['sexySyncPending'] = false;
		FS.syncfs(false, function(err) {
			Module['sexySyncBusy'] = false;
			if (err)
				console.warn('Saving user data failed:', err);
			if (Module['sexySyncPending'])
				sync();
		});
	};
	sync();
});

#endif

void Platform::RestoreUserDataBackup(const std::string& theUserDir)
{
#ifdef __EMSCRIPTEN__
	// The backup itself stays: the game replaces it with its own once it runs
	if (webpersist_restore(theUserDir.c_str()) > 0)
		webpersist_sync();
#else
	(void) theUserDir;
#endif
}

void Platform::BackupUserData(const std::vector<BackupFile>& theFiles)
{
#ifdef __EMSCRIPTEN__
	for (size_t i = 0; i < theFiles.size(); i++)
		webpersist_stage(theFiles[i].mRelPath.c_str(), (const unsigned char*) theFiles[i].mData.data(), (int) theFiles[i].mData.size());
	webpersist_backup(VFS::Get().GetUserDir().c_str());
#else
	(void) theFiles;
#endif
}

void Platform::SyncUserData()
{
#ifdef __EMSCRIPTEN__
	webpersist_sync();
#endif
}
