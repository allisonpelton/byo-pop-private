// A game page's clip recording (docs/WEB_PLAYER.md, "Clips"; setting "clips", off by default): with it on, the bar has a Record
// button, which records the game's picture (the canvas's own stream) and its sound (the game's audio output node, the one that
// plays it, connected for the clip to a stream as well) with the browser's MediaRecorder, and saves the clip as a file when it
// stops: a press of the button again, the game ending, or ten minutes. Nothing leaves the player's computer. It only reads the
// game's picture and sound, and the game plays the same. The page's mute mutes the clip's sound too.
var ByopopClips = (function () {
  "use strict";
  var MAX_MS = 10 * 60 * 1000;
  var pageGame = null, button = null, rec = null, chunks = [], started = 0, tick = 0, audioDest = null, audioNode = null;

  function setting(name) { try { return ByopopSettings.get(name, pageGame || undefined); } catch (e) { return null; } }
  function T(key) { try { return ByopopStrings.t(key); } catch (e) { return key; } }
  function supported() {
    var c = document.getElementById("canvas");
    return !!(window.MediaRecorder && c && c.captureStream);
  }
  function mimeType() {
    var types = ["video/webm;codecs=vp9,opus", "video/webm;codecs=vp8,opus", "video/webm", "video/mp4"];
    for (var i = 0; i < types.length; i++) { try { if (MediaRecorder.isTypeSupported(types[i])) return types[i]; } catch (e) {} }
    return "";
  }
  function fmt(ms) { var s = Math.floor(ms / 1000); return Math.floor(s / 60) + ":" + (s % 60 < 10 ? "0" : "") + (s % 60); }
  function label() {
    if (!button) return;
    var on = !!rec;
    button.setAttribute("aria-pressed", on ? "true" : "false");
    button.textContent = on ? T("clips.stop") + " " + fmt(performance.now() - started) : T("clips.record");
  }

  // the game's sound: its output node (platform/Audio.cpp, Module.byopopAudio) into a stream for the clip, beside the speakers
  function audioTrack() {
    try {
      var a = window.Module && Module.byopopAudio, node = a && a.node, ctx = node && node.context;
      if (!ctx || !ctx.createMediaStreamDestination) return null;
      audioDest = ctx.createMediaStreamDestination();
      node.connect(audioDest);
      audioNode = node;
      return audioDest.stream.getAudioTracks()[0] || null;
    } catch (e) { return null; }
  }
  function releaseAudio() {
    try { if (audioNode && audioDest) audioNode.disconnect(audioDest); } catch (e) {}
    audioNode = null;
    audioDest = null;
  }

  function start() {
    if (rec || !supported()) return;
    var canvas = document.getElementById("canvas"), stream = canvas.captureStream(60), track = audioTrack();
    if (track) stream.addTrack(track);
    var type = mimeType();
    try { rec = new MediaRecorder(stream, type ? { mimeType: type } : undefined); } catch (e) { rec = null; releaseAudio(); return; }
    chunks = [];
    rec.ondataavailable = function (ev) { if (ev.data && ev.data.size) chunks.push(ev.data); };
    rec.onstop = function () {
      stream.getTracks().forEach(function (t) { if (t !== track) t.stop(); });
      releaseAudio();
      var blob = new Blob(chunks, { type: (rec && rec.mimeType) || type || "video/webm" });
      chunks = [];
      rec = null;
      clearInterval(tick);
      label();
      save(blob);
    };
    rec.start(1000);
    started = performance.now();
    tick = setInterval(function () {
      label();
      if (performance.now() - started >= MAX_MS) stop();
    }, 500);
    label();
  }
  function stop() { if (rec && rec.state !== "inactive") rec.stop(); }
  function save(blob) {
    if (!blob.size) return;
    var ext = /mp4/.test(blob.type) ? "mp4" : "webm", d = new Date();
    function two(n) { return (n < 10 ? "0" : "") + n; }
    var name = (pageGame || "game") + "-" + d.getFullYear() + two(d.getMonth() + 1) + two(d.getDate()) + "-" + two(d.getHours()) + two(d.getMinutes()) + two(d.getSeconds()) + "." + ext;
    var url = URL.createObjectURL(blob), a = document.createElement("a");
    a.href = url;
    a.download = name;
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(function () { URL.revokeObjectURL(url); }, 60000);
  }

  function sync() {
    var want = setting("clips") === "on" && supported();
    if (!want) {
      stop();
      if (button) { button.remove(); button = null; }
      return;
    }
    if (button) return;
    var files = document.getElementById("btn-files");
    if (!files) return;
    button = document.createElement("button");
    button.type = "button";
    button.id = "btn-record";
    button.title = T("clips.title");
    button.addEventListener("click", function () { if (rec) stop(); else start(); });
    files.parentNode.insertBefore(button, files);
    label();
  }
  function setGame(id) { pageGame = id || null; sync(); }
  window.addEventListener("byopop-setting", function (ev) { if (ev.detail && ev.detail.name === "clips") sync(); });
  window.addEventListener("storage", function (ev) { if (ev.key === "byopop-settings" || ev.key === null) sync(); });
  // the game ended (the page is left, or the game quit): what was recorded is saved
  window.addEventListener("pagehide", stop);
  return { setGame: setGame, start: start, stop: stop, recording: function () { return !!rec; } };
})();
