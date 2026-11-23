// siteshots.js: shots of the site's own pages and windows for a before-and-after comparison with tools/shotdiff
// (docs/WEB_PLAYER.md, "Site shots"): Desktop 95 as it starts, with each of its windows open alone, its Pop menu, its own
// context menu, a message box, and the pages that open as windows (Jungle Juice's, a game's copyright page, a game without its
// files). At a desktop's size, a phone's width, and each zoomed to 200% (the device scale 2 of a browser at 200%); in the
// standard scheme and in grape (Desktop 95's colour schemes, web/schemes.js); and the pages in frames in the browser's dark
// scheme too, which only they follow. Headless Edge over the DevTools protocol with Node's own WebSocket, as
// tools/webtest/webmem.js; nothing is installed.
//
//   build/native/rundesk.exe node tools/web/siteshots.js --out <abs dir> [--only <regex>] [--port 18797] [--root build/web]
//        [--browser <msedge.exe>] [--profile <abs dir>] [--audit <abs file.jsonl>] [--census <abs file.json>] [--list]
//        [--skin <id>]
// --skin <id> opens every page in that skin (?skin=<id>, docs/SKINS.md) and takes the skin's own set of shots (SKIN_SET, below;
// --only narrows or widens it), for a baseline of that skin that shotdiff compares as it does 95's.
//
// Each shot is <out>/<scene>.png. The shots compare run to run: every page starts in a fresh browser profile (wiped at the
// start, and each page in a browser context of its own), on a fixed clock (Date) and a seeded Math.random, with reduced motion, the page told it has the focus, no text
// caret, and the software renderer. Compare two runs with
//   python tools/shotdiff/shotdiff.py --game site --orig <before dir> --port <after dir> --run <id>
// The browser counts as one game process (python tools/gamecount.py --limit first). It opens no link and plays nothing.
//
// --check is the guard of the site's elements (tools/check.sh's unit-web; docs/WEB_PLAYER.md, "Site shots"): the desktop at a
// desktop's size, each window, the menus, a message box and the pages that open as windows, as the shots have them but with no
// shot taken, each read by the audit below. It fails on an element that takes clicks or keys and is not a control, not inside
// one, and not a container with a role (a menu, a grid, a dialog); on a field with no name; on a role that a native element
// would give or that repeats its element's own; on a focusable element with no role; and on a picture a screen reader would
// read. Each exception is listed below (EXCEPTIONS) with its reason. It prints "semanticcheck: ..." and exits 1 on a fault.
//   build/native/rundesk.exe node tools/web/siteshots.js --check [--root build/web] [--port 0]
"use strict";
const fs = require("fs");
const path = require("path");
const { spawn } = require("child_process");

const ROOT = path.join(__dirname, "..", "..");
function arg(name, def) { const i = process.argv.indexOf("--" + name); return i > 0 ? process.argv[i + 1] : def; }
const OUT = arg("out", "");
// a skin other than 95 is checked by ten shots (docs/SKINS.md, (f)): the desktop with an inactive window (System Information
// behind the Library), the Pop menu, the desktop's menu, a message box with its default, Setup, Settings and its Appearance, a
// game page's start card, and the desktop at 2x and narrow
const SKIN = arg("skin", "");
const SKIN_SET = /^(desk-(start|popmenu|deskmenu|msgbox|setup|tab-settings|set-look|app-game)|desk2x-start|narrow-start)$/;
const ONLY = new RegExp(arg("only", SKIN ? SKIN_SET.source : "."));
const CHECK = process.argv.includes("--check");
let PORT = +arg("port", CHECK ? "0" : "18797");
const WEB = path.resolve(arg("root", path.join(ROOT, "build", "web")));
const BROWSER = arg("browser", "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe");
const PROFILE = path.resolve(arg("profile", path.join(ROOT, "build", CHECK ? "semanticcheck-profile" : "siteshots-profile")));
const LIST = process.argv.includes("--list");
if (!LIST && !CHECK && (!OUT || !path.isAbsolute(OUT))) { console.error("siteshots: --out <absolute folder> is needed"); process.exit(2); }

// the sizes: a desktop, the same zoomed to 200% (a 1920 x 1080 screen's browser at 200%: 960 x 540 CSS pixels at scale 2),
// a phone's width, and a 1280 x 800 window at 200% (640 x 400 at scale 2, narrower than the desktop's 760)
const SIZES = { desk: [1024, 768, 1], desk2x: [960, 540, 2], narrow: [375, 812, 1], narrow2x: [640, 400, 2] };
let BASE = "http://127.0.0.1:" + PORT + "/";

// what every page gets before its own scripts: the fixed clock, the seeded random numbers, no caret
const FREEZE = `(() => {
  const T = ${Date.UTC(2026, 0, 2, 15, 4, 0)}, D = Date;
  function F(...a) { return a.length ? new D(...a) : new D(T); }
  F.prototype = D.prototype; F.now = () => T; F.UTC = D.UTC; F.parse = D.parse;
  globalThis.Date = F;
  let s = 0x2545F491;
  Math.random = () => { s ^= s << 13; s >>>= 0; s ^= s >>> 17; s ^= s << 5; s >>>= 0; return s / 4294967296; };
  addEventListener("DOMContentLoaded", () => {
    const st = document.createElement("style");
    st.textContent = "*, *::before, *::after { caret-color: transparent !important; }";
    document.head.appendChild(st);
  });
})();`;

// ---- the server and the browser ----
let server = null, browser = null, ws = null, nextId = 1;
const pending = new Map(), events = [];
function sleep(ms) { return new Promise((r) => setTimeout(r, ms)); }
function send(method, params = {}, sessionId) {
  const id = nextId++;
  ws.send(JSON.stringify({ id, method, params, sessionId }));
  return new Promise((resolve, reject) => pending.set(id, { resolve, reject, method }));
}
async function evaluate(sid, expr) {
  const r = await send("Runtime.evaluate", { expression: expr, awaitPromise: true, returnByValue: true }, sid);
  if (r.exceptionDetails) throw new Error("evaluate: " + (r.exceptionDetails.exception && r.exceptionDetails.exception.description || r.exceptionDetails.text).slice(0, 300));
  return r.result.value;
}
async function until(fn, ms, what) {
  const t0 = Date.now();
  for (;;) {
    let v = null;
    try { v = await fn(); } catch (e) {}
    if (v) return v;
    if (Date.now() - t0 > ms) throw new Error("timed out waiting for " + what);
    await sleep(100);
  }
}
async function start() {
  // --port 0: a port no one is using, found by listening on one the system picks
  if (!PORT) PORT = await new Promise((resolve, reject) => { const t = require("net").createServer(); t.on("error", reject); t.listen(0, "127.0.0.1", () => { const n = t.address().port; t.close(() => resolve(n)); }); });
  BASE = "http://127.0.0.1:" + PORT + "/";
  server = spawn("python", [path.join(ROOT, "tools/webtest/serve.py"), "--port", String(PORT), "--root", WEB], { stdio: ["ignore", "pipe", CHECK ? "ignore" : "inherit"] });
  await new Promise((resolve, reject) => {
    let text = "";
    server.stdout.on("data", (d) => { text += d; if (text.includes("serving")) resolve(); });
    server.on("exit", (c) => reject(new Error("serve.py exited with " + c)));
  });
  fs.rmSync(PROFILE, { recursive: true, force: true });
  fs.mkdirSync(PROFILE, { recursive: true });
  browser = spawn(BROWSER, ["--headless", "--remote-debugging-port=0", "--user-data-dir=" + PROFILE, "--window-size=1280,900",
    "--mute-audio", "--no-first-run", "--no-default-browser-check", "--disable-extensions", "--disable-sync", "--disable-gpu",
    "--use-angle=swiftshader", "--enable-unsafe-swiftshader", "--font-render-hinting=none", "about:blank"], { stdio: "ignore" });
  const port = await until(() => fs.readFileSync(path.join(PROFILE, "DevToolsActivePort"), "utf8").split("\n"), 30000, "DevToolsActivePort");
  ws = new WebSocket("ws://127.0.0.1:" + port[0].trim() + port[1].trim());
  await new Promise((r, j) => { ws.onopen = r; ws.onerror = j; });
  ws.onmessage = (m) => {
    const d = JSON.parse(m.data);
    if (d.id && pending.has(d.id)) {
      const p = pending.get(d.id); pending.delete(d.id);
      d.error ? p.reject(new Error(p.method + ": " + d.error.message)) : p.resolve(d.result);
    } else if (d.method) events.push(d);
  };
}
async function stop() {
  if (browser && ws) {
    const ended = new Promise((r) => browser.once("exit", r));
    try { await Promise.race([send("Browser.close"), sleep(3000)]); } catch (e) {}
    await Promise.race([ended, sleep(10000)]);
  }
  try { ws && ws.close(); } catch (e) {}
  if (browser && browser.exitCode === null) browser.kill();
  if (server) { const s = server; server = null; s.removeAllListeners("exit"); await new Promise((r) => { s.on("exit", r); s.kill(); }); }
  try { fs.rmSync(PROFILE, { recursive: true, force: true }); } catch (e) {}
}

// ---- a page: a tab of its own at a size and in a scheme, loaded and settled ----
async function open(url, size, scheme, scheme95) {
  const [w, h, dpr] = SIZES[size];
  // a browser context of its own: storage that no earlier page wrote (a scheme, the windows' places)
  const { browserContextId } = await send("Target.createBrowserContext", {});
  const { targetId } = await send("Target.createTarget", { url: "about:blank", browserContextId });
  const { sessionId: sid } = await send("Target.attachToTarget", { targetId, flatten: true });
  await send("Page.enable", {}, sid);
  await send("Runtime.enable", {}, sid);
  await send("Emulation.setDeviceMetricsOverride", { width: w, height: h, deviceScaleFactor: dpr, mobile: false }, sid);
  await send("Emulation.setEmulatedMedia", { features: [{ name: "prefers-color-scheme", value: scheme }, { name: "prefers-reduced-motion", value: "reduce" }] }, sid);
  await send("Emulation.setFocusEmulationEnabled", { enabled: true }, sid);
  await send("Page.addScriptToEvaluateOnNewDocument", { source: FREEZE }, sid);
  if (scheme95 && scheme95 !== "standard")
    await send("Page.addScriptToEvaluateOnNewDocument", { source: "try { localStorage.setItem('byopop-settings', JSON.stringify({ scheme: " + JSON.stringify(scheme95) + " })); } catch (e) {}" }, sid);
  await send("Page.navigate", { url: BASE + url + (SKIN ? (url.indexOf("?") < 0 ? "?" : "&") + "skin=" + encodeURIComponent(SKIN) : "") }, sid);
  await settle(sid);
  return { sid, targetId, browserContextId, size };
}
async function settle(sid) {
  await until(() => evaluate(sid, "document.readyState === 'complete' && !(window.ByopopSkin && ByopopSkin.booting && ByopopSkin.booting())"), 30000, "the page");
  await evaluate(sid, "document.fonts.ready.then(() => new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r))))");
  await sleep(400);
}
async function close(p) {
  try { await send("Target.closeTarget", { targetId: p.targetId }); } catch (e) {}
  try { await send("Target.disposeBrowserContext", { browserContextId: p.browserContextId }); } catch (e) {}
}
let shots = 0;
async function shot(p, name) {
  if (!ONLY.test(name)) return;
  await evaluate(p.sid, "new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)))");
  await sleep(150);
  if (!CHECK) {
    const r = await send("Page.captureScreenshot", { format: "png" }, p.sid);
    fs.writeFileSync(path.join(OUT, name + ".png"), Buffer.from(r.data, "base64"));
  }
  shots++;
  if (AUDIT || CHECK) await audit(p, name);
  if (CENSUS) await takeCensus(p, name);
}

// ---- --audit <file.jsonl>: what each shot shows, as a screen reader and a keyboard meet it ----
// Every element shown (in the page and in its frames of the site's own pages) that takes clicks or keys, by the browser's own
// list of its listeners (DOMDebugger.getEventListeners), and whether it is a control: a button, a link, a field, a summary, an
// option, or an element with a widget role; or inside one; or, for keys, a container that only hears what its focused
// controls pass up. Also: a field with no name, a role that says what its element already is, a role a native element
// would give (a div that is a button, a list or a dialog), a focusable element with no role, a picture that a screen reader
// would read (an img without alt; an i or span drawn by a background image and not hidden). One JSON line per finding:
// {scene, kind, sig, events?, role?}. The pointer's own handlers (drags, grips) are listed as kind "pointer", not as faults.
const AUDIT = arg("audit", "");
const CLICKS = ["click", "dblclick", "contextmenu", "auxclick"], KEYS = ["keydown", "keyup", "keypress"];
const POINTER = ["mousedown", "mouseup", "pointerdown", "pointerup", "touchstart", "touchend"];
const AUDIT_PAGE = `(() => {
  const WIDGET = /^(button|link|menuitem|menuitemcheckbox|menuitemradio|option|tab|checkbox|radio|switch|slider|spinbutton|textbox|searchbox|combobox|gridcell|row|treeitem|scrollbar|separator)$/;
  const COMPOSITE = /^(menu|menubar|listbox|grid|treegrid|tree|tablist|toolbar|radiogroup|dialog|alertdialog|application)$/;
  // the role a native element gives, for a role that repeats it
  function implicit(e) {
    const t = e.tagName.toLowerCase();
    if (t === "a") return e.hasAttribute("href") ? "link" : "";
    if (t === "input") return { checkbox: "checkbox", radio: "radio", range: "slider", number: "spinbutton", button: "button", submit: "button", reset: "button", search: "searchbox" }[e.type] || "textbox";
    // a table with a grid's role makes its rows and cells the grid's; a th is its row's header when scoped so
    const grid = /^(tr|td|th)$/.test(t) && e.closest("table") && /^(grid|treegrid)$/.test(e.closest("table").getAttribute("role") || "");
    if (t === "td") return grid ? "gridcell" : "cell";
    if (t === "th") return e.getAttribute("scope") === "row" ? "rowheader" : "columnheader";
    // a header or footer inside a section, an article, an aside, a nav or the main is no landmark
    if ((t === "header" || t === "footer") && e.parentElement && e.parentElement.closest("article, aside, main, nav, section")) return "";
    return { button: "button", ul: "list", ol: "list", menu: "list", li: "listitem", nav: "navigation", main: "main", aside: "complementary", dialog: "dialog", table: "table",
      tr: "row", th: "columnheader", td: "cell", h1: "heading", h2: "heading", h3: "heading", h4: "heading", h5: "heading", h6: "heading", img: "img", hr: "separator",
      progress: "progressbar", select: e.multiple || e.size > 1 ? "listbox" : "combobox", textarea: "textbox", form: "form", fieldset: "group", option: "option",
      section: e.hasAttribute("aria-label") || e.hasAttribute("aria-labelledby") ? "region" : "", header: "banner", footer: "contentinfo", summary: "button", output: "status" }[t] || "";
  }
  // a role on a div or span that a native element gives
  const NATIVE = { button: "button", link: "a", dialog: "dialog", alertdialog: "dialog", list: "ul", listitem: "li", table: "table", row: "tr", cell: "td",
    columnheader: "th", rowheader: "th", heading: "h1-h6", navigation: "nav", region: "section", checkbox: "input checkbox", radio: "input radio",
    textbox: "input or textarea", progressbar: "progress", banner: "header", contentinfo: "footer", main: "main", complementary: "aside", form: "form",
    separator: "hr", group: "fieldset" };
  const GENERIC = /^(div|span|i|b|u|p|em|strong|small|pre|label|li|td|th|tr|img|h[1-6]|section|header|footer|aside|article|figure|ul|ol|dl|dt|dd|svg|canvas|fieldset|table|tbody|main|nav)$/;
  function role(e) { return (e.getAttribute("role") || "").trim().split(/\\s+/)[0] || implicit(e); }
  function control(e) {
    const t = e.tagName.toLowerCase(), r = role(e);
    if (/^(button|input|select|textarea|summary|option|iframe|video|audio)$/.test(t) || (t === "a" && e.hasAttribute("href"))) return true;
    // a grid's header cells take the focus and keys as its cells do
    if (/^(rowheader|columnheader)$/.test(r) && e.closest("table") && /^(grid|treegrid)$/.test(e.closest("table").getAttribute("role") || "")) return true;
    return WIDGET.test(r);
  }
  function sig(e) {
    const one = (x) => x.tagName.toLowerCase() + (x.id ? "#" + x.id : "") + (typeof x.className === "string" && x.className.trim() ? "." + x.className.trim().split(/\\s+/).filter((c) => !/^(d95-active|d95-min|d95-maxed|d95-on|d95-sel|d95-down|d95-open)$/.test(c)).slice(0, 2).join(".") : "");
    const parts = [one(e)];
    let x = e.parentElement;
    for (let i = 0; i < 2 && x && x !== x.ownerDocument.body; i++, x = x.parentElement) parts.unshift(one(x));
    const doc = e.ownerDocument;
    return (doc !== document ? "[" + doc.location.pathname.replace(/^\\//, "") + "] " : "") + parts.join(" > ");
  }
  function shown(e) { const r = e.getClientRects(); return r.length > 0 && !e.closest("[hidden]") && getComputedStyle(e).visibility !== "hidden"; }
  function named(e) {
    const d = e.ownerDocument;
    if ((e.getAttribute("aria-labelledby") || "").split(/\\s+/).some((id) => id && d.getElementById(id) && d.getElementById(id).textContent.trim())) return true;
    if ((e.getAttribute("aria-label") || "").trim() || (e.getAttribute("title") || "").trim()) return true;
    if (e.id && Array.prototype.some.call(d.querySelectorAll("label[for]"), (l) => l.htmlFor === e.id && l.textContent.trim())) return true;
    const l = e.closest("label");
    return !!(l && l.textContent.trim());
  }
  const els = [], info = [];
  (function walk(doc) {
    for (const e of doc.querySelectorAll("*")) {
      if (!shown(e)) continue;
      const t = e.tagName.toLowerCase(), r = e.getAttribute("role") || "", find = [];
      if (r && r.split(/\\s+/)[0] === implicit(e) && r !== "") find.push("role-repeats");
      if (/^(div|span|i|b|p|a)$/.test(t) && !(t === "a" && e.hasAttribute("href")) && NATIVE[r.split(/\\s+/)[0]]) find.push("role-native:" + NATIVE[r.split(/\\s+/)[0]]);
      if ((t === "input" && !/^(hidden|button|submit|reset|image)$/.test(e.type)) || t === "select" || t === "textarea") { if (!named(e)) find.push("unlabelled"); }
      if (t === "img" && !e.hasAttribute("alt") && !e.closest("[aria-hidden=true]")) find.push("img-no-alt");
      if ((t === "i" || t === "span") && !e.textContent.trim() && !e.closest("[aria-hidden=true]") && r !== "img" && r !== "presentation" && r !== "none" &&
          getComputedStyle(e).backgroundImage !== "none") find.push("picture-read");
      if (e.tabIndex >= 0 && e.getAttribute("tabindex") !== null && GENERIC.test(t) && !control(e) && !COMPOSITE.test(role(e)) && !/^(region|tabpanel|document|log|status|group|img)$/.test(role(e))) find.push("focusable-generic");
      els.push(e);
      info.push({ sig: sig(e), tag: t, role: role(e), control: control(e),
        inside: !!(e.parentElement && e.parentElement.closest("button, a[href], input, select, textarea, summary, [role=button], [role=link], [role=menuitem], [role=menuitemcheckbox], [role=menuitemradio], [role=option], [role=tab], [role=checkbox], [role=radio], [role=switch], [role=slider], [role=gridcell], [role=row], [role=treeitem]")),
        composite: COMPOSITE.test(role(e)), focusable: e.tabIndex >= 0 && (e.getAttribute("tabindex") !== null || e.isContentEditable), find });
    }
    for (const f of doc.querySelectorAll("iframe")) { try { if (f.contentDocument && shown(f)) walk(f.contentDocument); } catch (e) {} }
  })(document);
  window.__siteAudit = els;
  return info;
})()`;
// --census <file.json>: every shown div, span, p and i by where it is (the audit's signature), with what it holds (its
// children's tags, its own text, its font weight and size), gathered over every shot: the generic elements to judge by eye
const CENSUS = arg("census", ""), census = {};
const CENSUS_PAGE = `(() => {
  const out = [];
  (function walk(doc) {
    for (const e of doc.querySelectorAll("div, span, p, i")) {
      if (!e.getClientRects().length || e.closest("[hidden]")) continue;
      const one = (x) => x.tagName.toLowerCase() + (x.id ? "#" + x.id.replace(/[0-9]+/g, "N") : "") + (typeof x.className === "string" && x.className.trim() ? "." + x.className.trim().split(/\\s+/).slice(0, 2).join(".") : "");
      const parts = [one(e)];
      for (let x = e.parentElement, i = 0; i < 2 && x && x !== doc.body; i++, x = x.parentElement) parts.unshift(one(x));
      const kids = Array.from(e.children).map((c) => c.tagName.toLowerCase());
      const own = Array.from(e.childNodes).filter((n) => n.nodeType === 3).map((n) => n.textContent.trim()).join(" ").slice(0, 40);
      const cs = getComputedStyle(e);
      out.push({ sig: (doc !== document ? "[" + doc.location.pathname.replace(/^\\//, "") + "] " : "") + parts.join(" > "), kids: kids.length > 6 ? kids.slice(0, 6).join(",") + ",+" + (kids.length - 6) : kids.join(","),
        text: own, role: e.getAttribute("role") || "", hidden: e.closest("[aria-hidden=true]") ? 1 : 0, bold: +cs.fontWeight >= 600 ? 1 : 0, size: cs.fontSize });
    }
    for (const f of doc.querySelectorAll("iframe")) { try { if (f.contentDocument) walk(f.contentDocument); } catch (e) {} }
  })(document);
  return out;
})()`;
async function takeCensus(p, scene) {
  for (const r of await evaluate(p.sid, CENSUS_PAGE)) {
    const k = r.sig.replace(/(app|d95-title|bps|sy-panel|spl)-[0-9]+/g, "$1-N");
    if (!census[k]) census[k] = Object.assign({ n: 0, scenes: [] }, r);
    census[k].n++;
    if (census[k].scenes.length < 3 && !census[k].scenes.includes(scene)) census[k].scenes.push(scene);
  }
}
async function audit(p, scene) {
  const info = await evaluate(p.sid, AUDIT_PAGE);
  const arr = await send("Runtime.evaluate", { expression: "window.__siteAudit", returnByValue: false }, p.sid);
  const props = await send("Runtime.getProperties", { objectId: arr.result.objectId, ownProperties: true }, p.sid);
  const rows = [];
  for (const pr of props.result) {
    if (!/^\d+$/.test(pr.name) || !pr.value || !pr.value.objectId) continue;
    const i = +pr.name, el = info[i];
    el.find.forEach((k) => rows.push({ scene, kind: k.split(":")[0], sig: el.sig, role: el.role, note: k.split(":")[1] }));
    const { listeners } = await send("DOMDebugger.getEventListeners", { objectId: pr.value.objectId, depth: 0 }, p.sid);
    const types = Array.from(new Set(listeners.map((l) => l.type)));
    const clicks = types.filter((t) => CLICKS.includes(t)), keys = types.filter((t) => KEYS.includes(t)), ptr = types.filter((t) => POINTER.includes(t));
    if (!clicks.length && !keys.length && !ptr.length) continue;
    if (clicks.length || keys.length) tally[el.control ? "control" : el.inside ? "inside" : el.composite ? "composite" : !clicks.length && !el.focusable ? "container" : "other"].add(el.sig);
    if (el.control || el.inside) continue;
    if (clicks.length && !el.composite) rows.push({ scene, kind: "generic-clicks", sig: el.sig, role: el.role, events: clicks.concat(keys) });
    else if (keys.length && el.focusable && !el.composite) rows.push({ scene, kind: "generic-keys", sig: el.sig, role: el.role, events: keys });
    else if (!clicks.length && !keys.length) rows.push({ scene, kind: "pointer", sig: el.sig, role: el.role, events: ptr });
  }
  await send("Runtime.evaluate", { expression: "delete window.__siteAudit" }, p.sid);
  // the browser's own accessibility tree of the page (not its frames): one main landmark (the desktop's windows are in it),
  // and at most one banner and one contentinfo (a window's title bar and status bar are its section's, no landmarks)
  await send("Accessibility.enable", {}, p.sid);
  const ax = (await send("Accessibility.getFullAXTree", {}, p.sid)).nodes.filter((n) => !n.ignored && n.role);
  const count = (r) => ax.filter((n) => n.role.value === r).length;
  // (while a modal box is up the rest of the page is inert, its main with it)
  if (count("main") !== 1 && !(await evaluate(p.sid, "!!document.querySelector('dialog:modal')"))) rows.push({ scene, kind: "ax-landmarks", sig: count("main") + " main landmarks", role: "main" });
  for (const r of ["banner", "contentinfo"]) if (count(r) > 1) rows.push({ scene, kind: "ax-landmarks", sig: count(r) + " " + r + " landmarks", role: r });
  if (AUDIT) fs.appendFileSync(AUDIT, rows.map((r) => JSON.stringify(r) + "\n").join(""));
  found.push(...rows);
}
// what the audit found over every view, and the elements that take clicks or keys by what they are (each counted once)
const found = [], tally = { control: new Set(), inside: new Set(), composite: new Set(), container: new Set(), other: new Set() };

// ---- --check: the faults, less the exceptions ----
// The kinds that fail ("pointer", an element that takes only presses, is a note: a drag's start or a slider's own track).
const FAULTS = ["generic-clicks", "generic-keys", "unlabelled", "img-no-alt", "picture-read", "role-native", "role-repeats", "focusable-generic", "ax-landmarks"];
// Each exception: the kind, the element (the end of the audit's signature) and why it stays as it is.
const EXCEPTIONS = [
  { kind: "generic-clicks", sig: /(^| > )(header|div)\.d95-tb[^ ]*$/,
    why: "a window's title bar: a double click maximizes and restores it and a right click opens its control menu, as Windows 95's did, each a pointer's way to a button in the bar (Maximize, the control menu's); its click after a drag is eaten, and the chat's bar folds the chat" },
  { kind: "generic-clicks", sig: /(^| > )nav\.d95-taskbar$/,
    why: "the task bar: a right click on it opens its menu (Cascade, Tile, Minimize All), as Windows 95's did; the menu key on a button in its tray opens it too" },
  { kind: "role-native", sig: /(^| > )div(#import-bar)?\.(progress|d95-copy-bar)$/,
    why: "Windows 95's progress bar, drawn as blocks, its value told in words (aria-valuetext) where a progress element would give only a percentage" },
  { kind: "role-native", sig: /(^| > )div\.mu-vis$/,
    why: "Music's display, which a click turns to its next view: it holds the tracker's own scroll bar, which takes clicks of its own, and a button may hold neither blocks nor another control" }
];
function report() {
  const seen = new Map();
  for (const r of found) {
    if (!FAULTS.includes(r.kind)) continue;
    const k = r.kind + " " + r.sig.replace(/(app|d95-title|bps|sy-panel|spl)-[0-9]+/g, "$1-N");
    if (!seen.has(k)) seen.set(k, Object.assign({ views: [] }, r));
    const v = seen.get(k);
    if (v.views.length < 3 && !v.views.includes(r.scene)) v.views.push(r.scene);
  }
  const faults = [], allowed = [];
  for (const v of seen.values()) (EXCEPTIONS.some((x) => x.kind === v.kind && x.sig.test(v.sig)) ? allowed : faults).push(v);
  for (const v of faults) console.log("FAULT " + v.kind + (v.note ? " (" + v.note + ")" : "") + (v.events ? " [" + v.events.join(",") + "]" : "") + ": " + v.sig + "  (" + v.views.join(", ") + ")");
  const n = (k) => tally[k].size;
  console.log("semanticcheck: " + shots + " views, " + (n("control") + n("inside") + n("composite") + n("container") + n("other")) + " elements that take clicks or keys (" +
    n("control") + " controls, " + n("inside") + " inside one, " + n("composite") + " menus, grids and dialogs, " + n("container") + " lists that hear their controls' keys, " +
    n("other") + " none of these), " +
    faults.length + " faults, " + allowed.length + " known exceptions");
  if (faults.length) process.exitCode = 1;
}
async function mouse(p, x, y, button, count) {
  for (const type of ["mouseMoved", "mousePressed", "mouseReleased"])
    await send("Input.dispatchMouseEvent", { type, x, y, button: type === "mouseMoved" ? "none" : button, clickCount: type === "mouseMoved" ? 0 : count || 1 }, p.sid);
}
async function key(p, k) {
  const codes = { Escape: 27, Enter: 13 };
  for (const type of ["rawKeyDown", "keyUp"]) await send("Input.dispatchKeyEvent", { type, key: k, code: k, windowsVirtualKeyCode: codes[k] }, p.sid);
}
async function centre(p, sel) {
  return evaluate(p.sid, `(() => { const e = document.querySelector(${JSON.stringify(sel)}); if (!e) return null; const r = e.getBoundingClientRect(); return [r.left + r.width / 2, r.top + r.height / 2]; })()`);
}

// ---- the scenes ----
// Desktop 95: as it starts, each window alone (the chat's left closed: opening it talks to its server), the Pop menu, the
// desktop's context menu, a message box, and the pages that open as windows (Jungle Juice's, a game's copyright page, a game).
// With only "pages", the start and those pages alone (the browser's dark scheme, which only the pages in frames follow).
async function desk(size, scheme95, scheme, only) {
  const tag = size + (scheme95 !== "standard" ? "-" + scheme95 : "") + (scheme === "dark" ? "-dark" : "");
  let p = await open("index.html?silent=1", size, scheme, scheme95);
  await shot(p, tag + "-start");
  if (!(await evaluate(p.sid, "!!(window.ByopopSkin && ByopopSkin.desk.isOn())"))) { console.log("siteshots: " + tag + ": the desktop is off"); await close(p); return; }
  if (only !== "pages") {
    const wins = await evaluate(p.sid, "ByopopSkin.desk.list().map((w) => w.id)");
    const closeAll = "ByopopSkin.desk.closeMenus && ByopopSkin.desk.closeMenus(); ByopopSkin.desk.list().forEach((w) => ByopopSkin.desk.close(w.id))";
    for (const id of wins) {
      if (id === "chat" || !ONLY.test(tag + "-" + id)) continue;
      await evaluate(p.sid, closeAll + "; ByopopSkin.desk.openWin(" + JSON.stringify(id) + ")");
      await sleep(300);
      await shot(p, tag + "-" + id);
    }
    await evaluate(p.sid, closeAll);
    await sleep(200);
    const pop = await centre(p, "#d95-pop");
    if (pop) {
      await mouse(p, pop[0], pop[1], "left");
      await sleep(250);
      await shot(p, tag + "-popmenu");
      await key(p, "Escape");
      await sleep(150);
    }
    const [w, h] = SIZES[size];
    await mouse(p, w - 60, Math.round(h * 0.45), "right");
    await sleep(250);
    await shot(p, tag + "-deskmenu");
    await key(p, "Escape");
    await evaluate(p.sid, "ByopopSkin.desk.messageBox('Test', 'A message box for the shots.', 'question', [{ label: 'Yes', fn: function () {} }, { label: 'No', fn: function () {} }])");
    await sleep(250);
    await shot(p, tag + "-msgbox");
  }
  await close(p);
  // (a page of ours in a window: Jungle Juice's, a copyright page and a game's; until 2026-11-23 the first was terms.html, a
  // page left in build/web by an older build that no build writes now)
  for (const [name, url] of [["jungle", "index.html?silent=1&open=jungle/index.html"], ["copyright", "index.html?silent=1&open=noahsark-copyright.html"], ["game", "index.html?silent=1&open=noahsark.html"]]) {
    if (!ONLY.test(tag + "-app-" + name)) continue;
    p = await open(url, size, scheme, scheme95);
    // the window's page loaded, its fonts in, and on a game's page its first card up (the files card, without the files)
    await until(() => evaluate(p.sid, `(() => { const f = document.querySelector(".d95-win iframe"); const d = f && f.contentDocument;
      return !!(d && d.readyState === "complete" && d.body && d.body.childElementCount && (!d.getElementById("import-overlay") || d.querySelector(".overlay:not([hidden])"))); })()`), 20000, "the window's page");
    await evaluate(p.sid, "document.querySelector('.d95-win iframe').contentDocument.fonts.ready.then(() => true)");
    await sleep(800);
    await shot(p, tag + "-app-" + name);
    await close(p);
  }
}
const SCENES = [
  ["desk", () => desk("desk", "standard", "light")], ["desk-grape", () => desk("desk", "grape", "light")],
  ["desk2x", () => desk("desk2x", "standard", "light")], ["narrow", () => desk("narrow", "standard", "light")],
  ["narrow2x", () => desk("narrow2x", "standard", "light")], ["desk-dark", () => desk("desk", "standard", "dark", "pages")],
  ["narrow-dark", () => desk("narrow", "standard", "dark", "pages")]
];

async function main() {
  if (LIST) { console.log(SCENES.map((s) => s[0]).join("\n")); return; }
  if (CHECK && !fs.existsSync(BROWSER)) { console.log("semanticcheck: no browser at " + BROWSER + " (--browser names one)"); process.exitCode = 1; return; }
  if (!CHECK) fs.mkdirSync(OUT, { recursive: true });
  if (AUDIT) fs.writeFileSync(AUDIT, "");
  await start();
  for (const t of (await send("Target.getTargets")).targetInfos) if (t.type === "page") await send("Target.closeTarget", { targetId: t.targetId });
  const t0 = Date.now();
  for (const [name, run] of CHECK ? SCENES.slice(0, 1) : SCENES) {
    try { await run(); } catch (e) { console.log((CHECK ? "semanticcheck: " : "siteshots: ") + name + ": " + e.message); process.exitCode = 1; }
  }
  if (CENSUS) fs.writeFileSync(CENSUS, JSON.stringify(census, null, 1));
  if (CHECK) report();
  else console.log("siteshots: " + shots + " shots in " + OUT + " (" + ((Date.now() - t0) / 1000).toFixed(0) + " s)");
}
main().catch((e) => { console.error(e); process.exitCode = 1; }).finally(async () => { await stop(); setTimeout(() => process.exit(), 300); });
