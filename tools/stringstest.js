// The site's text: web/strings/en.txt against the code that uses it, and web/i18n.js's rules (docs/WEB_PLAYER.md, "The site's
// text"). Needs build/web/strings.js (build-web) for the runtime checks. Run: node tools/stringstest.js
//   - en.txt: well formed, keys lower case "area.name", none twice (lines may be any length: the user writes it);
//   - every {{key}} in web/*.html, every ByopopStrings.t/.html("key") (and T("key"), settings.js's short name for t) in
//     web/*.js and every byopop_web_string() key in cmake/ByopopWeb.cmake is a key of en.txt, and so is every other quoted
//     "area.name" whose area is one of en.txt's (a key picked by a ternary or held in a table), files like "index.html"
//     aside;
//   - every key of en.txt is used somewhere. This is approximate: any quoted literal that is a key counts as a use, a
//     prefix ("players.mode." + name) counts for every key under it, and theme.* are exempt; a key that only looks used
//     passes, a key used nowhere at all does not;
//   - no {{key}} inside a script (the build fills markup only), and a "_html" key is only used as markup;
//   - every {placeholder} a script passes is in the string, and every placeholder of a string is passed;
//   - ByopopStrings: placeholders, count forms, a missing key, HTML escaping.
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const root = path.join(__dirname, "..");
let checks = 0, failed = 0;
function check(ok, what) {
  checks++;
  if (!ok) { failed++; console.log("FAIL " + what); }
}
const read = (f) => fs.readFileSync(path.join(root, f), "utf8");

// ---- en.txt, parsed as the build does: "key = text", continuation lines indented, # notes, blank lines skipped
const table = {}, lines = {};
{
  const src = read("web/strings/en.txt").replace(/\r\n/g, "\n").split("\n");
  let cur = null;
  src.forEach((line, i) => {
    if (/^\s*#/.test(line)) return;      // a note (the build treats an indented # the same way)
    if (/^\s*$/.test(line)) { cur = null; return; }
    if (/^\s/.test(line)) {
      check(cur !== null, "en.txt:" + (i + 1) + ": a continuation line without a string");
      if (cur !== null) table[cur] += " " + line.trim();
      return;
    }
    const m = /^([^\s=]+) = (.*)$/.exec(line);
    check(!!m, "en.txt:" + (i + 1) + ": not 'key = text'");
    if (!m) { cur = null; return; }
    check(/^[a-z0-9_]+(\.[a-z0-9_]+)+$/.test(m[1]), "en.txt:" + (i + 1) + ": key '" + m[1] + "' is not lower case area.name");
    check(!(m[1] in table), "en.txt:" + (i + 1) + ": key '" + m[1] + "' twice");
    cur = m[1];
    table[cur] = m[2];
    lines[cur] = i + 1;
  });
}
const keys = Object.keys(table);
check(keys.length > 100, "en.txt holds " + keys.length + " keys");
const areas = {};
keys.forEach((k) => { areas[k.split(".")[0]] = true; });
const FILE = /\.(html|js|json|css|svg|png|jpg|ico|txt|xml|dat|exe|pak|zip|ogg|mo3|it|s3m|wav|bbk|saf|pyz|dll|egg)$/i;

// ---- uses
const used = {};            // key -> [places]
const markup = {};          // key -> true when used as {{key}}
const calls = [];           // {key, vars, file}
const prefixes = [];        // [prefix, file] of keys built from a prefix
function note(key, where) { (used[key] = used[key] || []).push(where); }

const webDir = path.join(root, "web");
for (const f of fs.readdirSync(webDir)) {
  if (!/\.(html|js)$/.test(f) || f === "i18n.js") continue;
  const text = read("web/" + f);
  if (f.endsWith(".html")) {
    // {{{ SCRIPT }}} and the like are not keys; a key sits in {{key}}
    const body = text.replace(/\{\{\{[^}]*\}\}\}/g, "");
    const scripts = [];
    body.replace(/<script\b[^>]*>([\s\S]*?)<\/script>/g, (m, s) => { scripts.push(s); return m; });
    scripts.forEach((s) => check(!/\{\{[a-z]/.test(s), f + ": a {{key}} inside a script"));
    body.replace(/\{\{([a-z0-9_.]+)\}\}/g, (m, k) => { note(k, f); markup[k] = true; return m; });
  }
  if (f.endsWith(".html")) {
    // inline scripts of a page are checked like the .js files
    text.replace(/<script\b[^>]*>([\s\S]*?)<\/script>/g, (m, s) => { scan(s, f); return m; });
  } else {
    scan(text, f);
  }
}
// the top-level keys of the object literal that starts at text[i] === "{", or null when it does not close
function objectKeys(text, i) {
  const keys = [];
  let depth = 0, str = null, prev = "{", word = "";
  for (let j = i; j < text.length; j++) {
    const c = text[j];
    if (str) { if (c === "\\") j++; else if (c === str) str = null; continue; }
    if (c === '"' || c === "'") { str = c; continue; }
    if ("([{".indexOf(c) >= 0) { depth++; if (depth > 1) word = ""; prev = c; continue; }
    if (")]}".indexOf(c) >= 0) { depth--; if (depth === 0) return keys; prev = c; continue; }
    if (depth !== 1) continue;
    if (/[A-Za-z0-9_$]/.test(c)) { word += c; continue; }
    if (c === ":" && word && (prev === "{" || prev === ",")) { keys.push(word); prev = ":"; word = ""; continue; }
    if (c === ",") { prev = ","; word = ""; continue; }
    if (!/\s/.test(c)) { word = ""; if (prev === "{" || prev === ",") prev = c; }
  }
  return null;
}
function scan(text, file) {
  // ByopopStrings.t/.html, and T (settings.js: var T = ByopopStrings.t)
  const re = /(?:ByopopStrings\.(t|html)|\bT)\(\s*"([a-z0-9_.]+)"\s*(,\s*\{)?/g;
  let m;
  while ((m = re.exec(text))) {
    if (/\.$/.test(m[2])) continue;       // a key built from a prefix, below
    const after = text.slice(m.index + m[0].length).match(/^\s*[,)]/);
    const vars = m[3] ? objectKeys(text, m.index + m[0].length - 1) : (after && after[0].indexOf(",") >= 0 ? null : []);
    calls.push({ key: m[2], vars, file, kind: m[1] || "t" });
    note(m[2], file);
  }
  // a key built from a prefix ("players.mode." + name + "_all") uses every key under that prefix
  text.replace(/"([a-z][a-z0-9_]*(?:\.[a-z0-9_]+)*\.)"\s*\+/g, (s, p) => { prefixes.push([p, file]); return s; });
  // a key held in a variable or a table: "area.name" strings that are keys, and those of a known area that are not (a
  // misspelled key picked by a ternary would otherwise show as itself); file names ("index.html") are not keys
  text.replace(/"([a-z][a-z0-9_]*(?:\.[a-z0-9_]+)+)"/g, (s, k) => {
    if (k in table) note(k, file);
    else if (areas[k.split(".")[0]] && !FILE.test(k)) note(k, file);
    return s;
  });
}
// the build's own text
{
  const cmake = read("cmake/ByopopWeb.cmake");
  cmake.replace(/byopop_web_string\(\s*\S+\s+([a-z0-9_.]+)\s*\)/g, (m, k) => { note(k, "cmake/ByopopWeb.cmake"); return m; });
  cmake.replace(/"([a-z][a-z0-9_]*(?:\.[a-z0-9_]+)+)"/g, (s, k) => { if (k in table) note(k, "cmake/ByopopWeb.cmake"); return s; });
}
// the games' pages are not converted, but a page may read a site key through the shell's text
for (const k of Object.keys(used)) check(k in table, "'" + k + "' (" + used[k][0] + ") is not in en.txt");
// theme.* are read by the theme script and the footer, which name them in other ways
for (const k of keys) {
  if (/^theme\./.test(k)) continue;
  if (!(k in used)) for (const [p, file] of prefixes) if (k.indexOf(p) === 0) note(k, file);
  check(k in used, "en.txt:" + lines[k] + ": '" + k + "' is not used");
}
for (const k of keys) {
  if (/_html$/.test(k)) {
    const asText = calls.filter((c) => c.key === k && c.kind === "t");
    check(asText.length === 0, "'" + k + "' is markup but " + asText.length + " call(s) use t()");
  }
}

// ---- placeholders against the calls that pass them
function placeholders(s) { const r = {}; s.replace(/\{([a-z_][a-z0-9_]*)\}/g, (m, n) => { r[n] = true; return m; }); return Object.keys(r); }
for (const c of calls) {
  if (!(c.key in table)) continue;
  if (c.vars === null) continue;       // the values are passed as a variable
  if (!c.vars.length && !placeholders(table[c.key]).length) continue;
  const have = placeholders(table[c.key]);
  for (const v of c.vars) check(have.indexOf(v) >= 0 || v === "n", c.file + ": " + c.key + " is passed '" + v + "', which the string lacks");
  // a count string takes n; its other placeholders must be passed
  for (const h of have) {
    if (h === "n" && table[c.key].indexOf(" | ") >= 0) { check(c.vars.indexOf("n") >= 0, c.file + ": " + c.key + " needs n"); continue; }
    check(c.vars.indexOf(h) >= 0, c.file + ": " + c.key + " needs {" + h + "}");
  }
}

// ---- the runtime
{
  const dir = process.env.BYOPOP_WEB_DIR || path.join(root, "build", "web");
  const file = path.resolve(dir, "strings.js");
  if (!fs.existsSync(file)) {
    check(false, "no " + file + ": run tools/check.sh build-web first");
  } else {
    const logs = [];
    const ctx = { console: { log() {}, warn: (...a) => logs.push(a.join(" ")), error: (...a) => logs.push(a.join(" ")) }, document: { documentElement: { lang: "en" } } };
    vm.createContext(ctx);
    vm.runInContext(fs.readFileSync(file, "utf8"), ctx, { filename: "strings.js" });
    const S = vm.runInContext("ByopopStrings", ctx);
    const built = vm.runInContext("BYOPOP_STRINGS", ctx);
    // the build's table is en.txt
    check(Object.keys(built).length === keys.length, "strings.js holds " + Object.keys(built).length + " keys, en.txt " + keys.length);
    for (const k of keys) if (built[k] !== table[k]) { check(false, "strings.js differs from en.txt at '" + k + "'"); break; }
    check(S.t("no.such_key") === "no.such_key", "a missing key comes back as itself");
    check(logs.length >= 1, "a missing key is noted in the console");
    const countKey = keys.find((k) => /^[^|]* \| /.test(table[k]) || table[k].indexOf(" | ") > 0);
    check(!!countKey, "en.txt has a count string");
    if (countKey) {
      const one = S.t(countKey, { n: 1, names: "x", count: 1 }), many = S.t(countKey, { n: 2, names: "x", count: 2 });
      check(one !== many, "'" + countKey + "' differs for 1 and 2");
      check(one.indexOf(" | ") < 0 && many.indexOf(" | ") < 0, "'" + countKey + "' leaves no | in the text");
    }
    const withVar = keys.find((k) => /\{title\}/.test(table[k]) && !/_html$/.test(k) && table[k].indexOf(" | ") < 0);
    if (withVar) check(S.t(withVar, { title: "Bejeweled" }).indexOf("Bejeweled") >= 0 && S.t(withVar, { title: "Bejeweled" }).indexOf("{title}") < 0, "'" + withVar + "' takes {title}");
    const htmlKey = keys.find((k) => /_html$/.test(k) && /\{[a-z_]+\}/.test(table[k]));
    if (htmlKey) {
      const name = placeholders(table[htmlKey])[0], vars = {};
      vars[name] = "<b>&";
      const out = S.html(htmlKey, vars);
      check(out.indexOf("&lt;b&gt;&amp;") >= 0, "html() escapes its vars ('" + htmlKey + "')");
    }
    // t() leaves a value that looks like a placeholder alone
    if (withVar) check(S.t(withVar, { title: "{title}" }).indexOf("{title}") >= 0, "a value holding {title} is not filled again");
  }
}

console.log("stringstest: " + checks + " checks, " + failed + " failed");
process.exit(failed ? 1 : 0);
