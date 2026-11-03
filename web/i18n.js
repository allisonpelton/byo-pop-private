// How the pages' scripts read the site's text (docs/WEB_PLAYER.md, "The site's text"). The build writes strings.js, every
// page's first script: the strings of web/strings/en.txt as BYOPOP_STRINGS, then this file. The pages' own markup is filled
// at the build ({{key}} markers); text that a script writes goes through ByopopStrings.t(key, vars):
//   - {name} in a string is filled from vars (t("import.title", {title: "Bejeweled"}));
//   - a string with forms for a count, "{n} mod active | {n} mods active", picks one by vars.n with the page language's
//     plural rules: the forms are the language's categories in the order zero, one, two, few, many, other, leaving out
//     those it does not have (English: one, other);
//   - a key it does not know comes back as the key itself, and is noted in the console, so a missing string shows.
// ByopopStrings.html(key, vars) is the same for a string that goes into innerHTML: its vars are HTML-escaped (its own
// markup, in a key ending in "_html", is not).
var ByopopStrings = (function () {
  "use strict";
  var table = (typeof BYOPOP_STRINGS === "object" && BYOPOP_STRINGS) || {};
  var ORDER = ["zero", "one", "two", "few", "many", "other"];
  var lang = "en", categories = ["one", "other"], rules = null;
  try {
    lang = (typeof document !== "undefined" && document.documentElement.lang) || "en";
    rules = new Intl.PluralRules(lang);
    var have = rules.resolvedOptions().pluralCategories;
    categories = ORDER.filter(function (c) { return have.indexOf(c) >= 0; });
  } catch (e) { rules = null; }
  var warned = {};

  function form(s, n) {
    var forms = s.split(" | ");
    if (forms.length < 2) return s;
    var cat = rules ? rules.select(n) : (n === 1 ? "one" : "other");
    var i = categories.indexOf(cat);
    return forms[i >= 0 && i < forms.length ? i : forms.length - 1];
  }
  function fill(s, vars, escape) {
    // a count string asked for without its number: the form for any other number, never the "a | b" source
    if (!vars || typeof vars.n !== "number") { if (typeof s === "string" && s.indexOf(" | ") >= 0) s = s.split(" | ").pop(); if (!vars) return s; }
    else s = form(s, vars.n);
    return s.replace(/\{([A-Za-z0-9_]+)\}/g, function (m, k) {
      if (!Object.prototype.hasOwnProperty.call(vars, k)) return m;
      return escape ? escapeHtml(String(vars[k])) : String(vars[k]);
    });
  }
  function escapeHtml(s) {
    return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
  }
  function get(key) {
    var s = table[key];
    if (typeof s === "string") return s;
    if (!warned[key]) { warned[key] = true; try { console.warn("byo-pop: no string \"" + key + "\" in web/strings/en.txt"); } catch (e) {} }
    return key;
  }
  function t(key, vars) { return fill(get(key), vars, false); }
  function html(key, vars) {
    var s = get(key);
    return fill(/_html$/.test(key) ? s : escapeHtml(s), vars, true);
  }
  function has(key) { return typeof table[key] === "string"; }
  return { t: t, html: html, has: has, lang: lang };
})();
