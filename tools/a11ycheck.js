#!/usr/bin/env node
// A small accessibility checker for the web player's built pages (docs/WEB_PLAYER.md, "Accessibility"). It needs no
// browser and installs nothing: a forgiving HTML reader of its own, a simplified accessible-name computation and the
// rules below, which are the faults that matter most for keyboard and screen reader players.
//
//   node tools/a11ycheck.js [dir | file.html ...]     default: build/web (BYOPOP_WEB_DIR names another build folder)
//
// With a folder it checks the site's own pages: index.html, the game pages that index.html has a tile for (the built list of
// BYOPOP_WEB_FINISHED, cmake/ByopopWeb.cmake, read from the tiles' links: `<a class="tile" href="<game>.html">`) and the
// copyright page that each of those game pages links to. Other .html files in the folder (fwtest.html and smoke.html, the
// test pages of a build) are not site pages and are not checked; name a file to check it. It prints one line per page
// (PASS or FAIL, and the faults under a FAIL), then "a11ycheck: N pages, E errors, W warnings", and exits 1 on any error.
//
// The checks, each with the rule's short name:
//   html-lang, title, viewport     the page's language, a title, and a viewport that can be zoomed
//   h1, heading-order, empty-heading   one <h1>, no skipped levels, no empty headings
//   landmarks, landmark-name       one <main>, at most one banner and one contentinfo, names for <aside> and the like
//   duplicate-id, idref            ids are unique; every id that aria-labelledby, -describedby, -controls and <label for> name exists
//   name                           buttons, links, fields, dialogs, the canvas, progress bars... have an accessible name
//   label-in-name                  a button or link named by aria-label still contains its visible words
//   click-handler, tabindex        no clickable <div>/<span>; no tabindex above 0
//   aria-hidden-focusable          nothing focusable inside aria-hidden
//   aria-value, aria-role          aria-expanded, -pressed... have legal values; roles exist; progress bars have a value
//   dialog, list, fieldset, table  dialogs are named and sit in a card; lists hold only items; fieldsets start with a legend
//   new-tab                        a link that opens a new tab says so
//   skip-link                      a game page's first focusable element is a skip link to something that exists
//   live-region                    the status line is a live region
//   outline                        no rule removes the focus ring without another one
//   motion                         a page with transitions or animations has a prefers-reduced-motion rule
//   contrast                       the page's colour tokens, in both themes, against WCAG AA (4.5:1 text, 3:1 large text and
//                                  the focus ring)
// In a browser the same checks run on the live page, which includes the markup the page's scripts built:
//   A11yCheck.checkHtml(document.documentElement.outerHTML, { name: "live" })
"use strict";

// ---------- the HTML reader ----------
var VOID = { area: 1, base: 1, br: 1, col: 1, embed: 1, hr: 1, img: 1, input: 1, link: 1, meta: 1, param: 1, source: 1, track: 1, wbr: 1 };
var RAW = { script: 1, style: 1, textarea: 1, title: 1 };
var BLOCK = { address: 1, article: 1, aside: 1, blockquote: 1, details: 1, div: 1, dl: 1, fieldset: 1, figure: 1, footer: 1, form: 1, h1: 1, h2: 1, h3: 1, h4: 1, h5: 1, h6: 1,
  header: 1, hr: 1, main: 1, nav: 1, ol: 1, p: 1, pre: 1, section: 1, table: 1, ul: 1 };
var ENTITIES = { amp: "&", lt: "<", gt: ">", quot: '"', apos: "'", nbsp: " ", hellip: "…", mdash: "—", ndash: "–", rsquo: "’", lsquo: "‘", ldquo: "“", rdquo: "”", middot: "·", copy: "©" };

function decode(t) {
  return t.replace(/&(#x[0-9a-f]+|#[0-9]+|[a-z]+);/gi, function (m, e) {
    if (e.charAt(0) === "#") {
      var n = e.charAt(1).toLowerCase() === "x" ? parseInt(e.slice(2), 16) : parseInt(e.slice(1), 10);
      return isFinite(n) ? String.fromCodePoint(n) : m;
    }
    return Object.prototype.hasOwnProperty.call(ENTITIES, e.toLowerCase()) ? ENTITIES[e.toLowerCase()] : m;
  });
}

// Returns { root, all } : root is a node {tag: "#root", children}, all the elements in document order
function parse(html) {
  var root = { type: "el", tag: "#root", attrs: {}, children: [], parent: null, line: 1 }, all = [], stack = [root], i = 0, n = html.length;
  function line(pos) { var c = 1; for (var k = 0; k < pos; k++) if (html.charCodeAt(k) === 10) c++; return c; }
  function top() { return stack[stack.length - 1]; }
  function addText(t) { if (t) top().children.push({ type: "text", text: t, parent: top() }); }
  function closeTo(tag) {
    for (var k = stack.length - 1; k > 0; k--) if (stack[k].tag === tag) { stack.length = k; return true; }
    return false;
  }
  function implied(tag) {
    var t = top().tag;
    if (tag === "li" && t === "li") stack.pop();
    else if ((tag === "dt" || tag === "dd") && (t === "dt" || t === "dd")) stack.pop();
    else if (tag === "option" && t === "option") stack.pop();
    else if ((tag === "td" || tag === "th") && (t === "td" || t === "th")) stack.pop();
    else if (tag === "tr" && (t === "td" || t === "th")) { stack.pop(); if (top().tag === "tr") stack.pop(); }
    else if (tag === "tr" && t === "tr") stack.pop();
    else if ((tag === "tbody" || tag === "thead" || tag === "tfoot") && (t === "tr" || t === "td" || t === "th")) { while (top().tag !== "table" && stack.length > 1) stack.pop(); }
    else if (BLOCK[tag] && t === "p") stack.pop();
  }
  while (i < n) {
    var lt = html.indexOf("<", i);
    if (lt < 0) { addText(decode(html.slice(i))); break; }
    if (lt > i) addText(decode(html.slice(i, lt)));
    i = lt;
    if (html.startsWith("<!--", i)) { var ce = html.indexOf("-->", i + 4); i = ce < 0 ? n : ce + 3; continue; }
    if (html[i + 1] === "!" || html[i + 1] === "?") { var de = html.indexOf(">", i); i = de < 0 ? n : de + 1; continue; }
    if (html[i + 1] === "/") {
      var ee = html.indexOf(">", i), name = html.slice(i + 2, ee < 0 ? n : ee).trim().toLowerCase();
      closeTo(name);
      i = ee < 0 ? n : ee + 1;
      continue;
    }
    var m = /^<([a-zA-Z][a-zA-Z0-9-]*)/.exec(html.slice(i, i + 40));
    if (!m) { addText("<"); i++; continue; }
    var tag = m[1].toLowerCase(), p = i + m[0].length, attrs = {}, selfClose = false;
    for (;;) {
      while (p < n && /\s/.test(html[p])) p++;
      if (p >= n) break;
      if (html[p] === ">") { p++; break; }
      if (html[p] === "/" && html[p + 1] === ">") { selfClose = true; p += 2; break; }
      if (html[p] === "/") { p++; continue; }
      var am = /^[^\s=>\/"']+/.exec(html.slice(p, p + 200));
      if (!am) { p++; continue; }
      var an = am[0].toLowerCase(), av = "";
      p += am[0].length;
      while (p < n && /\s/.test(html[p])) p++;
      if (html[p] === "=") {
        p++;
        while (p < n && /\s/.test(html[p])) p++;
        if (html[p] === '"' || html[p] === "'") {
          var q = html[p], qe = html.indexOf(q, p + 1);
          if (qe < 0) qe = n;
          av = html.slice(p + 1, qe);
          p = qe + 1;
        } else {
          var um = /^[^\s>]*/.exec(html.slice(p, p + 2000));
          av = um[0];
          p += av.length;
        }
      }
      if (!Object.prototype.hasOwnProperty.call(attrs, an)) attrs[an] = decode(av);
    }
    i = p;
    implied(tag);
    var el = { type: "el", tag: tag, attrs: attrs, children: [], parent: top(), line: line(lt) };
    top().children.push(el);
    all.push(el);
    if (VOID[tag] || selfClose) continue;
    if (RAW[tag]) {
      var re = new RegExp("</" + tag + "\\s*>", "i"), rest = html.slice(i), rm = re.exec(rest);
      var body = rm ? rest.slice(0, rm.index) : rest;
      if (body) el.children.push({ type: "text", text: tag === "title" || tag === "textarea" ? decode(body) : body, parent: el });
      i += rm ? rm.index + rm[0].length : rest.length;
      continue;
    }
    stack.push(el);
  }
  return { root: root, all: all };
}

// ---------- helpers on the tree ----------
function attr(el, name) { return Object.prototype.hasOwnProperty.call(el.attrs, name) ? el.attrs[name] : null; }
function has(el, name) { return Object.prototype.hasOwnProperty.call(el.attrs, name); }
function classes(el) { return (attr(el, "class") || "").split(/\s+/).filter(Boolean); }
function role(el) { var r = attr(el, "role"); return r ? r.trim().split(/\s+/)[0].toLowerCase() : ""; }
function ancestors(el) { var out = [], p = el.parent; while (p && p.tag !== "#root") { out.push(p); p = p.parent; } return out; }
function closest(el, tags) { for (var p = el; p && p.tag !== "#root"; p = p.parent) if (tags.indexOf(p.tag) >= 0) return p; return null; }
function descendants(el, fn) { el.children.forEach(function (c) { if (c.type === "el") { fn(c); descendants(c, fn); } }); }
function where(el) { return "<" + el.tag + (attr(el, "id") ? " id=" + attr(el, "id") : "") + "> line " + el.line; }
function norm(t) { return t.replace(/\s+/g, " ").trim(); }
function inlineHidden(el) { return /display\s*:\s*none/i.test(attr(el, "style") || ""); }
// Not exposed to assistive technology (or not shown): hidden, aria-hidden, display:none, here or in an ancestor
function isHidden(el, ignoreHiddenAttr) {
  for (var p = el; p && p.tag !== "#root"; p = p.parent) {
    if ((!ignoreHiddenAttr && has(p, "hidden")) || attr(p, "aria-hidden") === "true" || inlineHidden(p) || (p.tag === "input" && attr(p, "type") === "hidden")) return true;
  }
  return false;
}
function isAriaHidden(el) {
  for (var p = el; p && p.tag !== "#root"; p = p.parent) if (attr(p, "aria-hidden") === "true") return true;
  return false;
}
function isFocusable(el) {
  if (has(el, "hidden") || has(el, "disabled")) return false;
  var ti = attr(el, "tabindex");
  if (ti !== null && /^-?\d+$/.test(ti.trim())) return parseInt(ti, 10) >= 0;
  if (el.tag === "a" || el.tag === "area") return has(el, "href");
  if (el.tag === "input") return attr(el, "type") !== "hidden";
  return ["button", "select", "textarea", "summary"].indexOf(el.tag) >= 0;
}

// text of a node for naming: hidden descendants and aria-hidden ones are left out unless asked, an <img> counts as its alt
function textOf(node, o) {
  o = o || {};
  if (node.type === "text") return node.text;
  if (node.tag === "script" || node.tag === "style") return "";
  if (!o.keepHidden && node !== o.self && (has(node, "hidden") || attr(node, "aria-hidden") === "true" || inlineHidden(node))) return "";
  if (node.tag === "img") return attr(node, "alt") || "";
  if (node.tag === "input" || node.tag === "select" || node.tag === "textarea") return "";
  var parts = node.children.map(function (c) { return textOf(c, o); });
  return parts.join(BLOCK[node.tag] ? " " : "");
}

// The CSS of every <style> in the page
function styleText(doc) {
  return doc.byTag("style").map(function (st) { return st.children.map(function (c) { return c.text || ""; }).join(""); }).join("\n");
}
function Doc(html) {
  var p = parse(html);
  this.root = p.root;
  this.all = p.all;
  this.ids = {};
  this.dupes = [];
  var self = this;
  this.all.forEach(function (e) {
    var id = attr(e, "id");
    if (id === null || id === "") return;
    if (self.ids[id]) self.dupes.push([id, self.ids[id], e]); else self.ids[id] = e;
  });
  this.labels = this.all.filter(function (e) { return e.tag === "label"; });
}
Doc.prototype.byTag = function (tag) { return this.all.filter(function (e) { return e.tag === tag; }); };

// The accessible name of an element, simplified from the accname algorithm
var NAME_FROM_CONTENT = ["button", "link", "menuitem", "tab", "option", "checkbox", "radio", "switch", "treeitem", "heading", "cell", "columnheader", "rowheader", "tooltip"];
function implicitRole(el) {
  var r = role(el);
  if (r) return r;
  switch (el.tag) {
    case "a": return has(el, "href") ? "link" : "";
    case "button": return "button";
    case "summary": return "button";
    case "h1": case "h2": case "h3": case "h4": case "h5": case "h6": return "heading";
    case "img": return "img";
    case "select": return "combobox";
    case "textarea": return "textbox";
    case "ul": case "ol": return "list";
    case "li": return "listitem";
    case "aside": return "complementary";
    case "main": return "main";
    case "nav": return "navigation";
    case "dialog": return "dialog";
    case "progress": return "progressbar";
    case "table": return "table";
    case "fieldset": return "group";
    case "input": {
      var t = (attr(el, "type") || "text").toLowerCase();
      if (t === "checkbox") return "checkbox";
      if (t === "radio") return "radio";
      if (t === "button" || t === "submit" || t === "reset" || t === "image") return "button";
      if (t === "range") return "slider";
      if (t === "number") return "spinbutton";
      if (t === "file" || t === "hidden") return "";
      return "textbox";
    }
    case "section": return (attr(el, "aria-label") || attr(el, "aria-labelledby")) ? "region" : "";
    case "form": return (attr(el, "aria-label") || attr(el, "aria-labelledby")) ? "form" : "";
    default: return "";
  }
}
Doc.prototype.labelsFor = function (el) {
  var self = this, out = [], id = attr(el, "id");
  this.labels.forEach(function (l) {
    if (id && attr(l, "for") === id) out.push(l);
    else if (!has(l, "for") && descendantsInclude(l, el)) out.push(l);
  });
  return out;
};
function descendantsInclude(root, el) { var found = false; descendants(root, function (c) { if (c === el) found = true; }); return found; }
Doc.prototype.name = function (el, depth) {
  depth = depth || 0;
  var lb = attr(el, "aria-labelledby");
  if (lb && depth < 3) {
    var self = this, t = lb.split(/\s+/).map(function (id) { var r = self.ids[id]; return r ? norm(textOf(r, { keepHidden: true, self: r })) || self.name(r, depth + 1) : ""; }).join(" ");
    if (norm(t)) return norm(t);
  }
  var al = attr(el, "aria-label");
  if (al && norm(al)) return norm(al);
  var tag = el.tag, type = (attr(el, "type") || "").toLowerCase();
  if (tag === "img" || (tag === "input" && type === "image")) return norm(attr(el, "alt") || "") || (has(el, "alt") ? "" : norm(attr(el, "title") || ""));
  if (tag === "input" && (type === "button" || type === "submit" || type === "reset")) return norm(attr(el, "value") || (type === "submit" ? "Submit" : type === "reset" ? "Reset" : "")) || norm(attr(el, "title") || "");
  if (tag === "input" || tag === "select" || tag === "textarea") {
    var ls = this.labelsFor(el).map(function (l) { return norm(textOf(l)); }).filter(Boolean).join(" ");
    return ls || norm(attr(el, "title") || "");
  }
  if (tag === "fieldset") {
    var lg = el.children.filter(function (c) { return c.type === "el" && c.tag === "legend"; })[0];
    if (lg) return norm(textOf(lg));
  }
  if (tag === "table") {
    var cap = el.children.filter(function (c) { return c.type === "el" && c.tag === "caption"; })[0];
    if (cap) return norm(textOf(cap, { keepHidden: true }));
  }
  var r = implicitRole(el);
  if (NAME_FROM_CONTENT.indexOf(r) >= 0 || tag === "summary") {
    var tx = norm(textOf(el, { self: el }));
    if (tx) return tx;
  }
  return norm(attr(el, "title") || "");
};

// ---------- the CSS reader (custom properties and a few declarations) ----------
function stripComments(css) { return css.replace(/\/\*[\s\S]*?\*\//g, ""); }
// [{media, selector, decls}] in source order, one level of @media
function cssRules(css) {
  css = stripComments(css);
  var rules = [];
  function block(text, media) {
    var i = 0;
    while (i < text.length) {
      var open = text.indexOf("{", i);
      if (open < 0) break;
      var depth = 1, j = open + 1;
      while (j < text.length && depth > 0) { if (text[j] === "{") depth++; else if (text[j] === "}") depth--; j++; }
      var head = text.slice(i, open).trim(), body = text.slice(open + 1, j - 1);
      i = j;
      if (head.charAt(0) === "@") {
        if (/^@media/i.test(head)) block(body, head);
        continue;
      }
      var decls = {};
      body.split(";").forEach(function (d) {
        var k = d.indexOf(":");
        if (k > 0) decls[d.slice(0, k).trim().toLowerCase()] = d.slice(k + 1).trim();
      });
      head.split(",").forEach(function (sel) { rules.push({ media: media, selector: sel.trim().replace(/\s+/g, " "), decls: decls }); });
    }
  }
  block(css, null);
  return rules;
}
// The colour tokens of the two themes, by the cascade: a base :root rule, and the dark rules that beat it (their selectors are
// more specific), in source order
function themes(rules) {
  var light = {}, dark = {}, darkSet = {}, lightSet = {};
  rules.forEach(function (r) {
    var dm = r.media && /prefers-color-scheme\s*:\s*dark/i.test(r.media), lm = r.media && /prefers-color-scheme\s*:\s*light/i.test(r.media);
    var s = r.selector.replace(/["']/g, "");  // (a minified page has lost the quotes of its attribute selectors)
    Object.keys(r.decls).forEach(function (k) {
      if (k.indexOf("--") !== 0) return;
      var v = r.decls[k];
      if (s === ":root" && !r.media) { light[k] = v; if (!darkSet[k]) dark[k] = v; }
      else if (dm && (s === ":root" || s === ":root:not([data-theme=light])")) { dark[k] = v; darkSet[k] = true; }
      else if (s === ":root[data-theme=dark]") { dark[k] = v; darkSet[k] = true; }
      else if (s === ":root[data-theme=light]") { light[k] = v; lightSet[k] = true; }
      else if (lm && s === ":root") { light[k] = v; }
    });
  });
  return { light: light, dark: dark };
}

// ---------- contrast ----------
function rgb(hex) {
  hex = String(hex || "").trim();
  var m = /^#([0-9a-f]{3}|[0-9a-f]{6})$/i.exec(hex);
  if (!m) return null;
  var h = m[1].length === 3 ? m[1].replace(/./g, function (c) { return c + c; }) : m[1];
  return [0, 2, 4].map(function (k) { return parseInt(h.substr(k, 2), 16); });
}
function lum(c) {
  var v = c.map(function (x) { x /= 255; return x <= 0.03928 ? x / 12.92 : Math.pow((x + 0.055) / 1.055, 2.4); });
  return 0.2126 * v[0] + 0.7152 * v[1] + 0.0722 * v[2];
}
function ratio(a, b) {
  var x = rgb(a), y = rgb(b);
  if (!x || !y) return null;
  var l1 = lum(x), l2 = lum(y);
  return (Math.max(l1, l2) + 0.05) / (Math.min(l1, l2) + 0.05);
}

// The pairs of tokens that are text on a surface (4.5:1), and the ones that are a ring or a part of a control against what is next to it
// (3:1), for each kind of page, as its CSS uses them: the game pages' bar, cards and panels sit on --panel and --panel-2 and the body on
// --bg; the listing and copyright pages are text on --bg and --panel.
var PAIRS = {
  game: {
    text: [["--fg", "--bg"], ["--fg", "--panel"], ["--fg", "--panel-2"], ["--muted", "--bg"], ["--muted", "--panel"], ["--muted", "--panel-2"],
      ["--accent", "--panel"], ["--accent", "--panel-2"], ["--accent-fg", "--accent"],
      ["--gem-ruby", "--panel"], ["--gem-ruby", "--panel-2"], ["--ok", "--panel"], ["--ok", "--panel-2"], ["--warn", "--panel"], ["--warn", "--panel-2"]],
    ui: [["--accent", "--bg"], ["--accent", "--panel"], ["--accent", "--panel-2"]]
  },
  index: {
    text: [["--fg", "--bg"], ["--fg", "--panel"], ["--muted", "--bg"], ["--muted", "--panel"], ["--accent", "--bg"], ["--accent", "--panel"], ["--panel", "--accent"]],
    ui: [["--accent", "--bg"], ["--accent", "--panel"]]
  },
  copyright: {
    text: [["--fg", "--bg"], ["--fg", "--panel"], ["--muted", "--bg"], ["--muted", "--panel"], ["--accent", "--bg"], ["--accent", "--panel"]],
    ui: [["--accent", "--bg"], ["--accent", "--panel"]]
  }
};
// the error colours that the listing page and web/settings.js write as literals, and where
var ERROR_LITERAL = { light: "#b3261e", dark: "#ff8a80" };
var ERROR_ON = { game: ["--panel", "--panel-2"], index: ["--bg", "--panel"], copyright: [] };

function checkContrast(doc, report, kind) {
  var css = styleText(doc);
  var th = themes(cssRules(css));
  var checked = 0, set = PAIRS[kind] || PAIRS.copyright;
  ["light", "dark"].forEach(function (name) {
    var t = th[name];
    function val(k) { return k.charAt(0) === "#" ? k : (t[k] || null); }
    set.text.forEach(function (p) {
      var a = val(p[0]), b = val(p[1]);
      if (!a || !b || a.indexOf("var(") >= 0 || b.indexOf("var(") >= 0) return;
      var r = ratio(a, b);
      if (r === null) return;
      checked++;
      if (r < 4.5) report.error("contrast", name + " theme: " + p[0] + " " + a + " on " + p[1] + " " + b + " is " + r.toFixed(2) + ":1 (text needs 4.5:1)");
    });
    set.ui.forEach(function (p) {
      var a = val(p[0]), b = val(p[1]);
      if (!a || !b) return;
      var r = ratio(a, b);
      if (r === null) return;
      checked++;
      if (r < 3) report.error("contrast", name + " theme: the focus ring " + p[0] + " " + a + " against " + p[1] + " " + b + " is " + r.toFixed(2) + ":1 (needs 3:1)");
    });
    // the literal error colour on the surfaces it is written on
    if (/#b3261e/i.test(css)) {
      ERROR_ON[kind].forEach(function (s) {
        var b = val(s);
        if (!b) return;
        var r = ratio(ERROR_LITERAL[name], b);
        checked++;
        if (r < 4.5) report.error("contrast", name + " theme: the error colour " + ERROR_LITERAL[name] + " on " + s + " " + b + " is " + r.toFixed(2) + ":1");
      });
    }
  });
  // the badge colours of the listing page carry white initials (large, bold text: 3:1)
  doc.all.forEach(function (e) {
    var m = /--tile\s*:\s*(#[0-9a-f]{3,6})/i.exec(attr(e, "style") || "");
    if (!m) return;
    var r = ratio("#ffffff", m[1]);
    checked++;
    if (r !== null && r < 3) report.error("contrast", "the badge colour " + m[1] + " with white initials is " + r.toFixed(2) + ":1 (large text needs 3:1)");
  });
  report.checked += checked;
}

// ---------- the rules ----------
var ROLES = ("alert alertdialog application article banner blockquote button caption cell checkbox code columnheader combobox complementary contentinfo definition deletion dialog directory document emphasis feed figure form generic grid gridcell group heading img insertion link list listbox listitem log main marquee math menu menubar menuitem menuitemcheckbox menuitemradio meter navigation none note option paragraph presentation progressbar radio radiogroup region row rowgroup rowheader scrollbar search searchbox separator slider spinbutton status strong subscript superscript switch tab table tablist tabpanel term textbox time timer toolbar tooltip tree treegrid treeitem").split(" ");
var BOOL_ARIA = ["aria-atomic", "aria-busy", "aria-disabled", "aria-hidden", "aria-modal", "aria-multiline", "aria-multiselectable", "aria-readonly", "aria-required"];
var TRISTATE_ARIA = ["aria-checked", "aria-expanded", "aria-pressed", "aria-selected"];

function Report(name) {
  this.name = name; this.errors = []; this.warnings = []; this.checked = 0;
}
Report.prototype.error = function (rule, msg) { this.errors.push(rule + ": " + msg); };
Report.prototype.warn = function (rule, msg) { this.warnings.push(rule + ": " + msg); };

function checkDoc(doc, report, opts) {
  var html = doc.byTag("html")[0];
  var isGame = doc.all.some(function (e) { return e.tag === "canvas"; });
  var kind = isGame ? "game" : (doc.all.some(function (e) { return e.tag === "ul" && classes(e).indexOf("games") >= 0; }) ? "index" : "copyright");
  report.kind = kind;

  // page
  report.checked++;
  if (!html || !/^[a-z]{2,3}(-[A-Za-z0-9]+)*$/.test(attr(html, "lang") || "")) report.error("html-lang", "<html> needs a lang attribute");
  var title = doc.byTag("title")[0];
  if (!title || !norm(textOf(title, { keepHidden: true, self: title }))) report.error("title", "the page has no title");
  doc.byTag("meta").forEach(function (m) {
    if (attr(m, "name") !== "viewport") return;
    var c = attr(m, "content") || "";
    if (/user-scalable\s*=\s*(no|0)/i.test(c)) report.error("viewport", "the viewport forbids zooming (user-scalable)");
    var mx = /maximum-scale\s*=\s*([0-9.]+)/i.exec(c);
    if (mx && parseFloat(mx[1]) < 2) report.error("viewport", "the viewport limits zoom (maximum-scale=" + mx[1] + ")");
  });

  // headings
  var heads = doc.all.filter(function (e) { return /^h[1-6]$/.test(e.tag) || role(e) === "heading"; });
  var h1s = heads.filter(function (e) { return e.tag === "h1"; });
  report.checked++;
  if (h1s.length !== 1) report.error("h1", "the page needs exactly one <h1>, it has " + h1s.length);
  var prev = 0;
  heads.forEach(function (h) {
    report.checked++;
    var lvl = /^h[1-6]$/.test(h.tag) ? parseInt(h.tag.charAt(1), 10) : parseInt(attr(h, "aria-level") || "2", 10);
    if (!norm(textOf(h, { keepHidden: true, self: h })) && !attr(h, "aria-label")) report.error("empty-heading", where(h) + " is empty");
    if (prev && lvl > prev + 1) report.error("heading-order", where(h) + " jumps from level " + prev + " to " + lvl);
    if (!prev && lvl !== 1) report.error("heading-order", where(h) + " is the first heading and not an <h1>");
    prev = lvl;
  });

  // landmarks
  function inSectioning(e) { return !!closest(e.parent || e, ["article", "aside", "main", "nav", "section"]); }
  var mains = doc.all.filter(function (e) { return e.tag === "main" || role(e) === "main"; });
  var banners = doc.all.filter(function (e) { return (e.tag === "header" && !inSectioning(e) && !role(e)) || role(e) === "banner"; });
  var infos = doc.all.filter(function (e) { return (e.tag === "footer" && !inSectioning(e) && !role(e)) || role(e) === "contentinfo"; });
  report.checked += 3;
  if (mains.length !== 1) report.error("landmarks", "the page needs exactly one main landmark, it has " + mains.length);
  if (banners.length > 1) report.error("landmarks", "more than one banner (<header> outside any section): " + banners.map(where).join(", "));
  if (isGame || kind === "index") { if (banners.length < 1) report.error("landmarks", "the page has no banner (<header>)"); }
  if (infos.length > 1) report.error("landmarks", "more than one contentinfo (<footer> outside any section)");
  if (isGame && infos.length < 1) report.error("landmarks", "the game page has no footer");
  var landmarkNames = {};
  doc.all.forEach(function (e) {
    var r = implicitRole(e);
    if (["complementary", "navigation", "region", "form"].indexOf(r) >= 0 || e.tag === "aside" || e.tag === "nav") {
      report.checked++;
      var nm = doc.name(e);
      if (!nm && (e.tag === "aside" || e.tag === "nav" || role(e))) report.error("landmark-name", where(e) + " (" + (r || e.tag) + ") needs a name (aria-labelledby or aria-label)");
      var key = r + "|" + nm;
      if (nm && landmarkNames[key]) report.error("landmark-name", where(e) + " has the same name as another " + r + ": " + nm);
      landmarkNames[key] = true;
    }
    if (e.tag === "section" && !(attr(e, "aria-label") || attr(e, "aria-labelledby"))) report.warn("landmark-name", where(e) + " is a <section> without a name, so not a landmark");
  });

  // ids and references
  doc.dupes.forEach(function (d) { report.error("duplicate-id", "id \"" + d[0] + "\" is used by " + where(d[1]) + " and " + where(d[2])); });
  var dynamic = (opts && opts.dynamicIds) || [];
  doc.all.forEach(function (e) {
    ["aria-labelledby", "aria-describedby", "aria-controls", "aria-owns", "aria-activedescendant", "aria-details", "aria-errormessage"].forEach(function (a) {
      var v = attr(e, a);
      if (v === null) return;
      report.checked++;
      if (!norm(v)) { report.error("idref", where(e) + " has an empty " + a); return; }
      v.trim().split(/\s+/).forEach(function (id) {
        if (!doc.ids[id] && dynamic.indexOf(id) < 0) report.error("idref", where(e) + " " + a + " names \"" + id + "\", which is not on the page");
      });
    });
    if (e.tag === "label" && has(e, "for")) {
      report.checked++;
      var t = doc.ids[attr(e, "for")];
      if (!t) report.error("idref", where(e) + " is a label for \"" + attr(e, "for") + "\", which is not on the page");
      else if (["input", "select", "textarea", "button", "meter", "output", "progress"].indexOf(t.tag) < 0) report.error("idref", where(e) + " is a label for " + where(t) + ", which cannot be labelled");
    }
    if (has(e, "autofocus")) report.error("autofocus", where(e) + " has autofocus");
    if (has(e, "accesskey")) report.warn("accesskey", where(e) + " has an accesskey");
  });

  // names
  doc.all.forEach(function (e) {
    if (isAriaHidden(e) && !isFocusable(e)) return;
    var r = implicitRole(e), needs = false;
    if (e.tag === "a" && has(e, "href")) needs = true;
    else if (e.tag === "button" || e.tag === "summary" || e.tag === "select" || e.tag === "textarea") needs = true;
    else if (e.tag === "input") needs = ["hidden", "file"].indexOf((attr(e, "type") || "").toLowerCase()) < 0 || !has(e, "hidden");
    else if (e.tag === "img") needs = !has(e, "alt");
    else if (e.tag === "canvas" && role(e)) needs = true;
    else if (["dialog", "alertdialog", "progressbar", "img", "region", "slider", "spinbutton", "checkbox", "radio", "switch", "textbox", "combobox", "button", "link", "meter"].indexOf(role(e)) >= 0) needs = true;
    else if (e.tag === "progress" || e.tag === "dialog" || e.tag === "iframe") needs = true;
    if (e.tag === "input" && attr(e, "type") === "file" && has(e, "hidden")) needs = false;
    if (!needs) return;
    report.checked++;
    var nm = doc.name(e);
    if (e.tag === "img") { if (!has(e, "alt")) report.error("name", where(e) + " has no alt attribute"); return; }
    if (!nm) report.error("name", where(e) + " (" + (r || e.tag) + ") has no accessible name");
    // an aria-label that does not contain the words the control shows
    var al = attr(e, "aria-label");
    if ((e.tag === "a" || e.tag === "button") && al) {
      var vis = norm(textOf(e, { self: e })).toLowerCase().replace(/[^a-z0-9 ]/g, "");
      var lab = norm(al).toLowerCase().replace(/[^a-z0-9 ]/g, "");
      report.checked++;
      if (vis && lab.indexOf(vis) < 0) report.error("label-in-name", where(e) + " is named \"" + al + "\" but shows \"" + norm(textOf(e, { self: e })) + "\"");
    }
  });

  // click handlers on things that are not controls, tab order
  doc.all.forEach(function (e) {
    var handler = Object.keys(e.attrs).filter(function (k) { return k === "onclick" || k === "onmousedown" || k === "onmouseup"; })[0];
    if (handler && !isFocusable(e) && ["a", "button", "input", "select", "textarea", "summary", "label"].indexOf(e.tag) < 0) report.error("click-handler", where(e) + " has " + handler + " but cannot take the focus");
    var ti = attr(e, "tabindex");
    if (ti !== null) {
      report.checked++;
      if (!/^-?\d+$/.test(ti.trim())) report.error("tabindex", where(e) + " has tabindex=\"" + ti + "\"");
      else if (parseInt(ti, 10) > 0) report.error("tabindex", where(e) + " has tabindex above 0");
    }
    if (role(e) && ROLES.indexOf(role(e)) < 0) report.error("aria-role", where(e) + " has an unknown role \"" + role(e) + "\"");
    if (["button", "link", "checkbox", "switch", "tab", "menuitem"].indexOf(role(e)) >= 0 && !isFocusable(e)) report.error("aria-role", where(e) + " has role " + role(e) + " but cannot take the focus");
  });

  // aria values
  doc.all.forEach(function (e) {
    Object.keys(e.attrs).forEach(function (k) {
      if (k.indexOf("aria-") !== 0) return;
      var v = e.attrs[k].trim().toLowerCase();
      report.checked++;
      if (BOOL_ARIA.indexOf(k) >= 0 && ["true", "false"].indexOf(v) < 0 && !(k === "aria-hidden" && v === "undefined")) report.error("aria-value", where(e) + " " + k + "=\"" + e.attrs[k] + "\"");
      if (TRISTATE_ARIA.indexOf(k) >= 0 && ["true", "false", "mixed", "undefined"].indexOf(v) < 0) report.error("aria-value", where(e) + " " + k + "=\"" + e.attrs[k] + "\"");
      if (k === "aria-live" && ["off", "polite", "assertive"].indexOf(v) < 0) report.error("aria-value", where(e) + " aria-live=\"" + e.attrs[k] + "\"");
      if (k === "aria-haspopup" && ["true", "false", "menu", "listbox", "tree", "grid", "dialog"].indexOf(v) < 0) report.error("aria-value", where(e) + " aria-haspopup=\"" + e.attrs[k] + "\"");
    });
    if (has(e, "aria-pressed") && implicitRole(e) !== "button") report.error("aria-value", where(e) + " has aria-pressed but is not a button");
    if (has(e, "aria-expanded") && ["button", "link", "combobox", "checkbox", "tab", "treeitem", "row", "menuitem", "region", "heading"].indexOf(implicitRole(e)) < 0 && !role(e)) report.warn("aria-value", where(e) + " has aria-expanded on a " + e.tag);
    if (has(e, "aria-modal") && ["dialog", "alertdialog"].indexOf(implicitRole(e)) < 0) report.error("aria-value", where(e) + " has aria-modal but is not a dialog");
    if (implicitRole(e) === "progressbar" && e.tag !== "progress") {
      report.checked++;
      if (!has(e, "aria-valuenow")) report.error("aria-value", where(e) + " is a progress bar without aria-valuenow");
      else if (isNaN(parseFloat(attr(e, "aria-valuenow")))) report.error("aria-value", where(e) + " has a progress value that is not a number");
      if (!has(e, "aria-valuemin") || !has(e, "aria-valuemax")) report.error("aria-value", where(e) + " is a progress bar without aria-valuemin and aria-valuemax");
    }
  });
  // aria-hidden around things that can take the focus
  doc.all.forEach(function (e) {
    if (attr(e, "aria-hidden") !== "true") return;
    var bad = [];
    if (isFocusable(e)) bad.push(e);
    descendants(e, function (c) { if (isFocusable(c) && !closest(c, ["[hidden]"]) && !hiddenWithin(c, e)) bad.push(c); });
    report.checked++;
    bad.forEach(function (b) { report.error("aria-hidden-focusable", where(b) + " can take the focus inside aria-hidden (" + where(e) + ")"); });
  });
  function hiddenWithin(c, top) { for (var p = c; p && p !== top; p = p.parent) if (has(p, "hidden")) return true; return false; }

  // dialogs, lists, fieldsets, tables, details
  doc.all.forEach(function (e) {
    var r = implicitRole(e);
    if (r === "dialog" || r === "alertdialog") {
      report.checked++;
      if (!doc.name(e)) report.error("dialog", where(e) + " needs a name");
      if (!has(e, "aria-describedby") && r === "alertdialog") report.warn("dialog", where(e) + " has no aria-describedby");
    }
    if (e.tag === "ul" || e.tag === "ol") {
      report.checked++;
      e.children.forEach(function (c) {
        if (c.type === "el" && ["li", "script", "template"].indexOf(c.tag) < 0 && role(c) !== "listitem") report.error("list", where(c) + " is a child of " + where(e) + " and not a list item");
        if (c.type === "text" && norm(c.text)) report.error("list", where(e) + " holds text outside its items");
      });
    }
    if (e.tag === "li" && !e.parent.tag.match(/^(ul|ol|menu|#root)$/) && role(e.parent) !== "list") report.error("list", where(e) + " is not inside a list");
    if (e.tag === "fieldset") {
      report.checked++;
      var first = e.children.filter(function (c) { return c.type === "el"; })[0];
      if (!first || first.tag !== "legend") report.error("fieldset", where(e) + " does not start with a <legend>");
    }
    if (e.tag === "details") {
      var sm = e.children.filter(function (c) { return c.type === "el"; })[0];
      if (!sm || sm.tag !== "summary") report.error("details", where(e) + " does not start with a <summary>");
    }
    if (e.tag === "th") {
      report.checked++;
      if (!has(e, "scope") && !closest(e, ["thead"])) report.error("table", where(e) + " is a header cell without scope");
    }
    if (e.tag === "a" && attr(e, "target") === "_blank") {
      report.checked++;
      var nm2 = (doc.name(e) + " " + (attr(e, "title") || "")).toLowerCase();
      if (nm2.indexOf("new tab") < 0 && nm2.indexOf("new window") < 0) report.error("new-tab", where(e) + " opens a new tab and does not say so");
    }
    if (e.tag === "a" && has(e, "href")) {
      var tx = doc.name(e).toLowerCase();
      if (["click here", "here", "more", "read more", "link"].indexOf(tx) >= 0) report.error("link-purpose", where(e) + " is named \"" + tx + "\"");
    }
    if (e.tag === "button" && !has(e, "type") && !closest(e, ["form"])) report.warn("button-type", where(e) + " has no type");
  });

  // the game page
  if (isGame) {
    var focusables = doc.all.filter(function (e) { return isFocusable(e) && !isAriaHidden(e); });
    var first = focusables[0];
    report.checked++;
    if (!first || first.tag !== "a" || !/^#./.test(attr(first, "href") || "") || !doc.ids[attr(first, "href").slice(1)]) report.error("skip-link", "the first focusable element must be a link to an element on the page (\"Skip to game\")");
    else if (!/skip/i.test(doc.name(first))) report.error("skip-link", "the first link " + where(first) + " is not named as a skip link");
    var status = doc.ids["status"];
    report.checked++;
    if (!status || (role(status) !== "status" && !/polite|assertive/.test(attr(status, "aria-live") || ""))) report.error("live-region", "the status line (#status) must be a live region (role=status)");
    var cv = doc.byTag("canvas")[0];
    report.checked++;
    if (role(cv) !== "application") report.warn("canvas", "the canvas has no role=application");
    if (!has(cv, "aria-describedby")) report.error("canvas", "the canvas needs a hint on how to leave it (aria-describedby)");
    ["mods", "files", "settings"].forEach(function (n) {
      var b = doc.ids["btn-" + n];
      report.checked++;
      if (!b) report.error("panels", "no #btn-" + n + " on the bar");
      else if (attr(b, "aria-controls") === null && n !== "mods") report.error("panels", where(b) + " needs aria-controls");
    });
  } else {
    report.checked++;
  }

  // CSS: the focus ring and motion
  var css = styleText(doc);
  var rules = cssRules(css);
  var OUTLINE_OK = /(:focus|^\.panel:focus|^\.mp-panel:focus|^\.card:focus|^canvas#canvas)$/;
  rules.forEach(function (r) {
    var o = r.decls.outline, ow = r.decls["outline-width"];
    if ((o && /^(none|0)\b/.test(o)) || (ow && /^0/.test(ow))) {
      report.checked++;
      // allowed on a container that only takes the focus from a script (tabindex -1), where the ring would just frame the panel
      if (!OUTLINE_OK.test(r.selector)) report.error("outline", "\"" + r.selector + "\" removes the focus outline");
    }
  });
  if (isGame) {
    report.checked++;
    if (!rules.some(function (r) { return /:focus-visible/.test(r.selector) && /outline/.test(Object.keys(r.decls).join(" ")); })) report.error("outline", "no :focus-visible rule gives the focus a ring");
    if (!rules.some(function (r) { return /canvas#canvas:focus-visible/.test(r.selector); })) report.error("outline", "the canvas has no :focus-visible ring");
  }
  var moves = rules.some(function (r) { return r.decls.transition || r.decls.animation || r.decls["animation-name"]; });
  report.checked++;
  if (moves && !rules.some(function (r) { return r.media && /prefers-reduced-motion\s*:\s*reduce/.test(r.media); })) report.error("motion", "the page moves things but has no prefers-reduced-motion rule");
  if (!rules.some(function (r) { return r.media && /prefers-reduced-motion\s*:\s*reduce/.test(r.media); })) report.warn("motion", "no prefers-reduced-motion rule");

  checkContrast(doc, report, kind);
}

function checkHtml(html, opts) {
  opts = opts || {};
  var report = new Report(opts.name || "page");
  checkDoc(new Doc(html), report, opts);
  return report;
}

// ---------- a self-test: pages with one fault each, and a clean one ----------
function selftest() {
  var head = '<!doctype html><html lang="en"><head><title>t</title>';
  function page(body, style, lang) {
    return (lang === false ? head.replace(' lang="en"', "") : head) + "<style>" + (style || ":root{--fg:#111;--bg:#fff;--panel:#fff;--muted:#444;--accent:#00528f}") + "</style></head><body>" + body + "</body></html>";
  }
  var clean = '<header><h1 id="t">Games</h1></header><main><ul class="games"><li><a href="a.html">A</a></li></ul>' +
    '<button type="button" aria-controls="p" aria-expanded="false">Open</button><aside id="p" aria-labelledby="pt" hidden><h2 id="pt">Panel</h2></aside></main><footer><p>f</p></footer>';
  var cases = [
    ["the clean page has no error", clean, null, ""],
    ["a button without a name", clean.replace("</main>", '<button type="button"></button></main>'), null, "name:"],
    ["no lang", clean, null, "html-lang:", false],
    ["a duplicate id", clean.replace("</main>", '<p id="t">x</p></main>'), null, "duplicate-id:"],
    ["an aria reference to nothing", clean.replace('aria-labelledby="pt"', 'aria-labelledby="zz"'), null, "idref:"],
    ["a tabindex above 0", clean.replace("</main>", '<a href="b.html" tabindex="2">B</a></main>'), null, "tabindex:"],
    ["a clickable div", clean.replace("</main>", '<div onclick="go()">go</div></main>'), null, "click-handler:"],
    ["a skipped heading level", clean.replace('<h2 id="pt">', '<h4 id="pt">'), null, "heading-order:"],
    ["two h1", clean.replace("</main>", "<h1>two</h1></main>"), null, "h1:"],
    ["no main", clean.replace("<main>", "<div>").replace("</main>", "</div>"), null, "landmarks:"],
    ["an unnamed aside", clean.replace(' aria-labelledby="pt"', ""), null, "landmark-name:"],
    ["something focusable inside aria-hidden", clean.replace("</main>", '<div aria-hidden="true"><button type="button">x</button></div></main>'), null, "aria-hidden-focusable:"],
    ["a progress bar without a value", clean.replace("</main>", '<div role="progressbar" aria-label="p"></div></main>'), null, "aria-value:"],
    ["an aria-label that drops the visible words", clean.replace("</main>", '<a href="c.html" aria-label="Buy it">Buy on Steam</a></main>'), null, "label-in-name:"],
    ["a link to a new tab that does not say so", clean.replace("</main>", '<a href="d.html" target="_blank">D</a></main>'), null, "new-tab:"],
    ["a list with a stray child", clean.replace("</li></ul>", "</li><div>x</div></ul>"), null, "list:"],
    ["text that fails contrast", clean, ":root{--fg:#999;--bg:#aaa;--panel:#fff;--muted:#444;--accent:#00528f}", "contrast:"],
    ["an outline taken away", clean, ":root{--fg:#111;--bg:#fff} button{outline:none}", "outline:"],
    ["a transition without a reduced-motion rule", clean, ":root{--fg:#111;--bg:#fff} .x{transition:width 1s}", "motion:"]
  ];
  var bad = 0;
  cases.forEach(function (c) {
    var r = checkHtml(page(c[1], c[2], c[4]), { name: c[0] });
    var ok = c[3] === "" ? r.errors.length === 0 : r.errors.some(function (e) { return e.indexOf(c[3]) === 0; });
    if (!ok) { bad++; console.log("  selftest FAIL: " + c[0] + " -> " + (r.errors.join(" | ") || "no errors")); }
  });
  console.log("a11ycheck selftest: " + cases.length + " cases, " + bad + " failed");
  return bad ? 1 : 0;
}

// ---------- the command line ----------
// The site's own pages in a build folder, from the build itself: index.html, the games that index.html has a tile for (the tiles
// are made from BYOPOP_WEB_FINISHED by byopop_web_index()) and the copyright page each game page links to. Test pages are left out.
function siteFiles(dir) {
  var fs = require("fs"), path = require("path"), out = [], seen = {};
  function add(name) {
    if (seen[name] || !fs.existsSync(path.join(dir, name))) return false;
    seen[name] = true; out.push(path.join(dir, name));
    return true;
  }
  if (!add("index.html")) return [];
  var index = fs.readFileSync(path.join(dir, "index.html"), "utf8"), re = /<a\b[^>]*\bclass=["']?[^"'>]*\btile\b[^>]*>/g, m;
  while ((m = re.exec(index))) {
    var href = /\bhref=["']?([a-z0-9_-]+\.html)\b/.exec(m[0]);
    if (!href || !add(href[1])) continue;
    var game = fs.readFileSync(path.join(dir, href[1]), "utf8"), cr = /\bhref=["']?([a-z0-9_-]+-copyright\.html)\b/.exec(game);
    if (cr) add(cr[1]);
  }
  return out;
}

function main(argv) {
  if (argv.indexOf("--selftest") >= 0) return selftest();
  var fs = require("fs"), path = require("path");
  var args = argv.slice(2).filter(function (a) { return a !== "--quiet"; });
  var quiet = argv.indexOf("--quiet") >= 0;
  var files = [];
  if (!args.length) args = [process.env.BYOPOP_WEB_DIR || "build/web"];
  args.forEach(function (a) {
    if (fs.existsSync(a) && fs.statSync(a).isDirectory()) siteFiles(a).forEach(function (f) { files.push(f); });
    else files.push(a);
  });
  // index first, then game pages and copyright pages
  files.sort(function (x, y) { var a = path.basename(x), b = path.basename(y); return (a === "index.html" ? 0 : 1) - (b === "index.html" ? 0 : 1) || (a < b ? -1 : a > b ? 1 : 0); });
  if (!files.length) { console.log("a11ycheck: no pages found in " + args.join(", ")); return 1; }
  var errors = 0, warnings = 0, pages = 0;
  files.forEach(function (f) {
    var html = fs.readFileSync(f, "utf8");
    // a game page's pieces that the browser builds from scripts are not in the file; nothing else is missing
    var r = checkHtml(html, { name: path.basename(f) });
    pages++; errors += r.errors.length; warnings += r.warnings.length;
    console.log((r.errors.length ? "FAIL " : "PASS ") + path.basename(f) + " [" + r.kind + "] " + r.checked + " checks, " + r.errors.length + " errors, " + r.warnings.length + " warnings");
    r.errors.forEach(function (e) { console.log("  error: " + e); });
    if (!quiet) r.warnings.forEach(function (w) { console.log("  warning: " + w); });
  });
  console.log("a11ycheck: " + pages + " pages, " + errors + " errors, " + warnings + " warnings");
  return errors ? 1 : 0;
}

var api = { parse: parse, Doc: Doc, checkHtml: checkHtml, ratio: ratio, themes: themes, cssRules: cssRules };
if (typeof module !== "undefined" && module.exports) {
  module.exports = api;
  if (require.main === module) process.exitCode = main(process.argv);
} else if (typeof window !== "undefined") {
  window.A11yCheck = api;
}
