// The site's logo drawn inline (docs/WEB_PLAYER.md, "The site's logo and icons"). Each page's markup has the logo as an
// image, logo.svg; once the page is read this puts its own drawing in its place (its "byo" drawn in --bp-logo-ink), so that
// its opener, the hyphen, can move: on mouseover (or the focus of its link) the opener nudges toward "pop" and levers twice, as
// if to open the cap, which stays on (the user, 2026-11-03: "animate the logo so on mouseover the bottle opener moves as if
// to open the cap (without actually opening it)"). A player who asked for less motion sees it stay still. Where logo.svg
// cannot be read, the images stay.
(function () {
  "use strict";
  var STYLE =
    ":root { --bp-logo-ink: #12304A; }\n" +
    ".bp-logo { display: block; max-width: 100%; height: auto; overflow: visible; }\n" +
    // the opener turns about its head, the end toward "pop"
    ".bp-logo .bp-lever { transform-box: fill-box; transform-origin: 100% 50%; }\n" +
    ".bp-logo.lever .bp-lever { animation: bp-lever 0.9s ease-in-out; }\n" +
    "@keyframes bp-lever { 0% { transform: none; } 22% { transform: translateX(3px); }" +
    " 40% { transform: translateX(3px) rotate(16deg); } 56% { transform: translateX(3px) rotate(3deg); }" +
    " 72% { transform: translateX(3px) rotate(16deg); } 88% { transform: translateX(3px); } 100% { transform: none; } }\n" +
    "@media (prefers-reduced-motion: reduce) { .bp-logo.lever .bp-lever { animation: none; } }\n";

  function inline(img, text) {
    var doc = new DOMParser().parseFromString(text, "image/svg+xml");
    var svg = doc.documentElement;
    if (!svg || svg.nodeName !== "svg" || !svg.querySelector(".bp-lever")) return;
    svg = document.importNode(svg, true);
    var title = svg.querySelector("title");
    if (title) svg.removeChild(title);
    svg.setAttribute("class", "bp-logo");
    svg.setAttribute("width", img.getAttribute("width"));
    svg.setAttribute("height", img.getAttribute("height"));
    svg.setAttribute("role", "img");
    svg.setAttribute("aria-label", img.alt);
    svg.setAttribute("focusable", "false");
    var ink = svg.querySelector(".bp-ink");
    if (ink) ink.style.stroke = "var(--bp-logo-ink)";
    var parent = img.parentNode;
    parent.replaceChild(svg, img);
    // once per mouseover: a lever that is still moving finishes first
    function lever() { svg.classList.add("lever"); }
    var host = parent.closest ? parent.closest("a, .logo-drop") || parent : parent;
    host.addEventListener("mouseenter", lever);
    if (host.nodeName === "A") host.addEventListener("focus", lever);
    svg.addEventListener("animationend", function () { svg.classList.remove("lever"); });
  }

  function init() {
    var imgs = document.querySelectorAll("img.logo-light");
    if (!imgs.length || !window.fetch || !window.DOMParser) return;
    var style = document.createElement("style");
    style.textContent = STYLE;
    document.head.appendChild(style);
    fetch(imgs[0].getAttribute("src")).then(function (r) { return r.ok ? r.text() : Promise.reject(r.status); }).then(function (text) {
      Array.prototype.forEach.call(imgs, function (img) { try { inline(img, text); } catch (e) {} });
    }).catch(function () {});
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init); else init();
})();
