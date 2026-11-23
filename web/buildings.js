// Venice's buildings, the Gallery's entry for Venice (web/gallery.js; the user, 2026-11-18: "I'd love to see a combined image
// of all of Venice's buildings as a continuous shot, if something like that is possible, and full images of the individual
// buildings of Venice"): Venice Deluxe's buildings from the player's own files in this browser, each one whole and all of
// them in one row, and the one shown saved as a PNG. Nothing is shipped: the pictures are decoded here from the player's
// files and kept only while the Gallery shows them (panel(game).show to .hide).
//
// Each building is one picture, the one its levels name as their thumbnail (gfx/levelselect/thumbnail1.png to
// thumbnail8.png; the win sequence names the trophy, thumbnail9.png). The level select shows them in a row that slides past
// (games/venice/src/LevelSelect.cpp), and the row is laid out here as LevelSelect::Load lays it: down Journey's list of
// levels (config.xml's level_file_relaxed) in order, a picture for the levels at 1, 10, 19 and so on of those with a
// thumbnail (the second of each building's nine), each level moving the next place on by a ninth of its picture's width
// less four, and the win sequence's picture 90 further on. Each picture is a little lower in depth than the one before, so
// where two overlap the earlier building is drawn over the later.
//
// With Background set to the level select, the row stands in that screen's scene (levels/levelselect.xml), each object
// placed as SceneObject::DrawSelf places it and as it stands at the end of its first stage (the screen faded in, the layers
// slid home). Three things differ from any one screen of the game: both layers sit at their home places, where in play the
// near one slides twice as far as the distant city; the rows of tiles (the quay and the water) are carried on to the row's
// ends where they stop short; and of the screen's own pictures, which stay put while the row slides (the sky, the birds and
// clouds, the scroll and its text), only the sky is kept, repeated along the row with every other copy mirrored so that the
// copies meet.

var ByopopBuildings = (function () {
  "use strict";
  function T(k, a) { return typeof ByopopStrings !== "undefined" ? ByopopStrings.t(k, a) : k; }
  var f32 = Math.fround;

  // ---------- the scene files ----------
  // A file's tags in order, as games/venice/src/SceneTag.cpp reads them: <name attr="value" ...>, where only <object> is
  // closed (by </object>); names in lower case
  function readTags(text) {
    var out = [], re = /<(\/?)([A-Za-z_][\w]*)([^>]*)>/g, m;
    while ((m = re.exec(text))) {
      var attrs = {}, ra = /([A-Za-z_][\w]*)\s*=\s*"([^"]*)"/g, a;
      while ((a = ra.exec(m[3]))) attrs[a[1]] = a[2];
      out.push({ close: !!m[1], name: m[2].toLowerCase(), attrs: attrs });
    }
    return out;
  }
  function num(s) { var v = parseFloat(s); return isFinite(v) ? v : 0; }
  function int(s) { var v = parseInt(s, 10); return isFinite(v) ? v : 0; }
  // a picture's name as a key of the game's files: forward slashes, lower case
  function key(name) { return String(name || "").replace(/\\/g, "/").replace(/^\.?\//, "").toLowerCase(); }

  // The scene's objects as a tree: each with its tag's attributes, its stages (each with the end value of each attribute it
  // sets) and its children
  function parseScene(text) {
    var root = { attrs: {}, stages: [], children: [] }, stack = [root];
    readTags(text).forEach(function (t) {
      var top = stack[stack.length - 1];
      if (t.name === "object") {
        if (t.close) { if (stack.length > 1) stack.pop(); return; }
        var o = { attrs: t.attrs, stages: [], children: [] };
        top.children.push(o);
        stack.push(o);
      } else if (t.close) {
        return;
      } else if (t.name === "stage") {
        top.stages.push({ attrs: t.attrs, values: {} });
      } else if (t.name === "attribute" && top.stages.length && t.attrs.type) {
        top.stages[top.stages.length - 1].values[t.attrs.type] = num("end" in t.attrs ? t.attrs.end : t.attrs.start);
      }
    });
    return root.children;
  }

  // config.xml: each picture's frames (<image name frame_width frame_height num_frames>) and the lists of scenes in order
  function parseConfig(text) {
    var images = {}, lists = { level_file: [], level_file_game: [], level_file_relaxed: [], level_file_trick: [] };
    readTags(text).forEach(function (t) {
      if (t.close || !t.attrs.name) return;
      if (t.name === "image" && int(t.attrs.frame_width) > 0)
        images[key(t.attrs.name)] = { w: int(t.attrs.frame_width), h: int(t.attrs.frame_height), count: int(t.attrs.num_frames) };
      else if (lists.hasOwnProperty(t.name)) lists[t.name].push(t.attrs.name);
    });
    return { images: images, lists: lists };
  }
  // a scene file's own tag (its first): its thumbnail, if it names one
  function thumbnailOf(text) {
    var t = readTags(String(text).slice(0, 4096)).filter(function (x) { return x.name === "object" && !x.close; })[0];
    return t && t.attrs.thumbnail ? t.attrs.thumbnail : "";
  }

  // LevelSelect::Load's places: levels is the list's levels that name a thumbnail, in order, each {name, thumbnail, width}
  // (its picture's width); gives each picture made, {path, x, win}, x from the level select object's place
  function strip(levels) {
    var x = f32(100), out = [];
    levels.forEach(function (l, level) {
      var win = String(l.name).toLowerCase().indexOf("win_seq") >= 0;
      if (level % 9 === 1 || win) out.push({ path: key(l.thumbnail), x: win ? f32(x + 90) : x, win: win });
      x = f32(f32(f32(l.width / 9) - 4) + x);
    });
    return out;
  }

  // ---------- the layout ----------
  var SCREEN_H = 600;
  function values(o) { var s = o.stages[0]; return s ? s.values : {}; }
  function zOf(o) { return o.attrs.z_order !== undefined ? num(o.attrs.z_order) : o.z || 0; }
  // children in depth order, as SceneObject::InsertChildByZOrder keeps them: a child after every sibling of its depth or less
  function byDepth(list) {
    return list.map(function (o, i) { return { o: o, i: i }; })
      .sort(function (a, b) { return zOf(a.o) - zOf(b.o) || a.i - b.i; }).map(function (e) { return e.o; });
  }

  // The draw list. scene: parseScene(levelselect.xml); places: strip(...); size(path) gives a picture's {w, h} (null when the
  // files lack it); frames: parseConfig(...).images. Gives {ops, sky, left, right, top, bottom, pictures}: ops in draw order,
  // each {path, sx, sy, sw, sh, dx, dy, dw, dh, alpha, smooth, clip, picture} where picture is the place's index (-1 for the
  // scene's own), tiled rows kept as rows (row: {tl, tr, tu, td, stepX, stepY}) for fill() to carry to the ends.
  function layout(scene, places, size, frames) {
    var ops = [], sky = null;
    var level = scene.filter(function (o) { return o.attrs.type === "level"; })[0] || scene[0];
    if (!level) return null;
    function frameOf(path, pic, n) {
      var f = frames && frames[path];
      if (!f) return { sx: 0, sy: 0, w: pic.w, h: pic.h };
      var across = Math.max(1, Math.floor(pic.w / f.w)), i = Math.max(0, Math.min(n | 0, Math.max(0, f.count - 1)));
      return { sx: (i % across) * f.w, sy: Math.floor(i / across) * f.h, w: f.w, h: f.h };
    }
    // one object's pictures (SceneObject::DrawSelf, the 2D path: DrawImageRecord with its offsets of one pixel)
    function draw(o, world, bilinear, picture) {
      var path = key(o.attrs.image || o.image), pic = path && size(path);
      if (!pic) return null;
      var fr = frameOf(path, pic, world.frame), x = world.x, y = world.y;
      if (!bilinear) { x = Math.trunc(f32(x + 0.5)); y = Math.trunc(f32(y + 0.5)); }
      var clip = null, cl = int(o.attrs.clip_left), ct = int(o.attrs.clip_top), cr = int(o.attrs.clip_right), cb = int(o.attrs.clip_bottom);
      if (cl || ct || cr || cb) clip = [cl, ct, cr, cb];
      var plain = world.xs === 1 && world.ys === 1, dw = fr.w * world.xs, dh = fr.h * world.ys;
      var op = { path: path, sx: fr.sx, sy: fr.sy, sw: fr.w, sh: fr.h, dw: dw, dh: dh, alpha: world.alpha, smooth: !!bilinear && !plain,
        clip: clip, picture: picture, flipX: !!int(o.attrs.flip_x), flipY: !!int(o.attrs.flip_y) };
      if (plain) { op.dx = Math.trunc(x - fr.w / 2); op.dy = Math.trunc(y - fr.h / 2 + 1); }
      else { op.dx = x - dw / 2; op.dy = y + 1 - dh / 2; }
      var tl = int(o.attrs.tiles_left), tr = int(o.attrs.tiles_right), tu = int(o.attrs.tiles_up), td = int(o.attrs.tiles_down);
      if (tl || tr || tu || td) op.row = { tl: tl, tr: tr, tu: tu, td: td, stepX: fr.w * world.xs, stepY: fr.h * world.ys };
      ops.push(op);
      return op;
    }
    function walk(o, parent, inherited) {
      var v = values(o), world = {
        x: "x" in v ? v.x : 0, y: "y" in v ? v.y : 0, xs: "x_scale" in v ? v.x_scale : 1, ys: "y_scale" in v ? v.y_scale : 1,
        alpha: "alpha" in v ? v.alpha : 1, frame: "frame_number" in v ? v.frame_number : 0 };
      // the layers at their home places (LevelSelect::Position sets where both slide from and to)
      if (o.attrs.type === "layer") world.x = 0;
      if (parent) {
        world.x = f32(f32(world.x * parent.xs) + parent.x); world.y = f32(f32(world.y * parent.ys) + parent.y);
        world.xs *= parent.xs; world.ys *= parent.ys; world.alpha *= parent.alpha;
      }
      var bilinear = o.attrs.bilinear !== undefined ? int(o.attrs.bilinear) : inherited;
      if (o.attrs.image || o.image) draw(o, world, bilinear, o.picture === undefined ? -1 : o.picture);
      var kids = o.children.slice();
      // the level select object's children: the row's pictures (bilinear, as a new object is), each a little lower in depth
      if (o.attrs.type === "level_select") {
        var z = f32(2);
        places.forEach(function (p, i) {
          kids.push({ attrs: { bilinear: "1" }, image: p.path, z: z, picture: i, children: [],
            stages: [{ attrs: {}, values: { x: p.x, y: -110 } }] });
          z = f32(z - f32(0.001));
        });
      }
      byDepth(kids).forEach(function (c) { if (c.attrs.type !== "text" && c.attrs.type !== "cursor") walk(c, world, bilinear); });
    }
    // the level itself, at the end of its first stage; its own pictures stay with the screen, so only the lowest, the sky,
    // is kept, and only its layers are walked in full
    var lv = values(level), root = { x: "x" in lv ? lv.x : 400, y: "y" in lv ? lv.y : 300, xs: 1, ys: 1, alpha: "alpha" in lv ? lv.alpha : 1, frame: 0 };
    var rootBilinear = level.attrs.bilinear !== undefined ? int(level.attrs.bilinear) : 1;
    var own = byDepth(level.children);
    var skyObject = own.filter(function (o) { return o.attrs.image && o.attrs.type !== "layer"; })[0];
    if (skyObject) {
      var at = ops.length;
      walk({ attrs: skyObject.attrs, stages: skyObject.stages, children: [] }, root, rootBilinear);
      sky = ops.splice(at, 1)[0] || null;
    }
    own.forEach(function (o) { if (o.attrs.type === "layer") walk(o, root, rootBilinear); });
    // the row's ends: those of the pictures that are not tiled
    var left = Infinity, right = -Infinity, top = Infinity, bottom = -Infinity;
    ops.forEach(function (op) {
      if (op.row) return;
      left = Math.min(left, Math.floor(op.dx)); right = Math.max(right, Math.ceil(op.dx + op.dw));
    });
    var pictures = [];
    ops.forEach(function (op) {
      if (op.picture < 0) return;
      var r = { x: Math.floor(op.dx), y: Math.floor(op.dy), w: Math.ceil(op.dw), h: Math.ceil(op.dh) };
      pictures[op.picture] = { path: op.path, win: places[op.picture].win, rect: r };
      top = Math.min(top, r.y); bottom = Math.max(bottom, r.y + r.h);
    });
    if (!isFinite(left)) return null;
    return { ops: ops, sky: sky, left: left, right: right, top: 0, bottom: SCREEN_H, pictures: pictures,
      bare: pictures.length ? { left: Math.min.apply(null, pictures.map(function (p) { return p.rect.x; })),
        right: Math.max.apply(null, pictures.map(function (p) { return p.rect.x + p.rect.w; })), top: top, bottom: bottom } : null };
  }

  // The ops that draw [left, right): tiled rows as the tiles that fall there, carried past the game's own count where it stops
  // short of an end; the sky repeated along it, every other copy mirrored. which(op) chooses the ops (null: all).
  function fill(lay, left, right, withSky, which) {
    var out = [];
    if (withSky && lay.sky) {
      var s = lay.sky, n0 = Math.floor((left - s.dx) / s.dw), n1 = Math.ceil((right - s.dx) / s.dw);
      for (var n = n0; n <= n1; n++) {
        var c = Object.assign({}, s, { dx: s.dx + n * s.dw, flipX: s.flipX !== (Math.abs(n) % 2 === 1) });
        if (c.dx < right && c.dx + c.dw > left) out.push(c);
      }
    }
    lay.ops.forEach(function (op) {
      if (which && !which(op)) return;
      if (!op.row) { out.push(op); return; }
      var r = op.row, needL = Math.ceil((op.dx - left) / r.stepX), needR = Math.ceil((right - (op.dx + op.dw)) / r.stepX);
      var lo = -Math.max(r.tl, needL, 0), hi = Math.max(r.tr, needR, 0);
      for (var ty = -r.tu; ty <= r.td; ty++) {
        for (var tx = lo; tx <= hi; tx++) {
          var dx = op.dx + tx * r.stepX;
          if (dx >= right || dx + op.dw <= left) continue;
          var t = Object.assign({}, op, { dx: dx, dy: op.dy + ty * r.stepY });
          delete t.row;
          out.push(t);
        }
      }
    });
    return out;
  }

  // What a choice shows: choice is "all" or a place's index; scene is whether the level select's scene is behind. Gives
  // {left, top, width, height, ops}.
  function picture(lay, choice, scene) {
    var one = choice !== "all", p = one ? lay.pictures[+choice] : null;
    if (one && !p) return null;
    var keep = function (op) { return op.picture < 0 ? scene : !one || op.picture === +choice; };
    var left, right, top, bottom;
    if (scene) { left = one ? p.rect.x : lay.left; right = one ? p.rect.x + p.rect.w : lay.right; top = lay.top; bottom = lay.bottom; }
    else if (one) { left = p.rect.x; right = p.rect.x + p.rect.w; top = p.rect.y; bottom = p.rect.y + p.rect.h; }
    else { left = lay.bare.left; right = lay.bare.right; top = lay.bare.top; bottom = lay.bare.bottom; }
    return { left: left, top: top, width: right - left, height: bottom - top, ops: fill(lay, left, right, scene, keep) };
  }

  // ---------- the player's files ----------
  var source = function (game) {
    var F = typeof ByopopMusic !== "undefined" && ByopopMusic.files;
    if (!F) return Promise.resolve(null);
    return F.open(game.persist).then(function (db) {
      if (!db) return null;
      return F.list(db, game.persist).then(function (rels) {
        return { list: rels, read: function (rel) { return F.read(db, game.persist + "/game/" + rel); }, close: function () { db.close(); } };
      }, function (e) { db.close(); throw e; });
    });
  };
  function latin1(bytes) {
    var s = "";
    for (var i = 0; i < bytes.length; i += 8192) s += String.fromCharCode.apply(null, bytes.subarray(i, Math.min(bytes.length, i + 8192)));
    return s;
  }
  // Everything the row needs, read from the game's files: {lay, files, pictures: {path: drawable}}; decode(bytes, path)
  // gives {width, height, image} (null when it cannot)
  function load(game, decode) {
    var files = null, index = {};
    function bytesOf(path) { var e = index[key(path)]; return e ? e.read() : Promise.resolve(null); }
    function textOf(path) { return bytesOf(path).then(function (b) { return b ? latin1(b instanceof Uint8Array ? b : new Uint8Array(b)) : null; }); }
    return source(game).then(function (src) {
      if (!src) return null;
      files = src;
      src.list.forEach(function (rel) { index[key(rel)] = { read: function () { return src.read(rel); } }; });
      return Promise.all([textOf("config.xml"), textOf("levels/levelselect.xml")]);
    }).then(function (texts) {
      if (!texts || !texts[0] || !texts[1]) throw new Error("missing");
      var config = parseConfig(texts[0]), scene = parseScene(texts[1]), names = config.lists.level_file_relaxed;
      return Promise.all(names.map(function (n) { return textOf(n); })).then(function (levels) {
        var withThumbs = [];
        levels.forEach(function (t, i) { var th = t && thumbnailOf(t); if (th) withThumbs.push({ name: names[i], thumbnail: th }); });
        // every picture the scene and the row draw
        var paths = {};
        withThumbs.forEach(function (l) { paths[key(l.thumbnail)] = 1; });
        (function each(list) { list.forEach(function (o) { if (o.attrs.image) paths[key(o.attrs.image)] = 1; each(o.children); }); })(scene);
        var pictures = {};
        return Promise.all(Object.keys(paths).map(function (p) {
          return bytesOf(p).then(function (b) { return b ? decode(b instanceof Uint8Array ? b : new Uint8Array(b), p) : null; })
            .then(function (d) { if (d) pictures[p] = d; }, function () {});
        })).then(function () {
          withThumbs = withThumbs.filter(function (l) { return pictures[key(l.thumbnail)]; });
          withThumbs.forEach(function (l) { l.width = pictures[key(l.thumbnail)].width; });
          var size = function (p) { var d = pictures[p]; return d ? { w: d.width, h: d.height } : null; };
          var lay = layout(scene, strip(withThumbs), size, config.images);
          if (!lay) throw new Error("missing");
          return { lay: lay, files: files, pictures: pictures };
        });
      });
    }).catch(function (e) { if (files) files.close(); throw e; });
  }
  function decodePicture(bytes, path) {
    var type = /\.png$/i.test(path) ? "image/png" : /\.gif$/i.test(path) ? "image/gif" : /\.bmp$/i.test(path) ? "image/bmp" : "image/jpeg";
    return createImageBitmap(new Blob([bytes], { type: type })).then(function (bmp) {
      return { width: bmp.width, height: bmp.height, image: bmp };
    }, function () { return null; });
  }

  // ---------- the window ----------
  function make(tag, cls, text) { var e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; }
  function labelled(text, control) { var l = make("label", "", text); l.appendChild(control); return l; }
  function options(sel, list) {
    sel.textContent = "";
    list.forEach(function (o) { var e = make("option", "", o[1]); e.value = o[0]; sel.appendChild(e); });
  }
  function ratio() { var K = window.ByopopSkin; return K && K.ratio ? K.ratio() : window.devicePixelRatio || 1; }
  function paint(view, pictures) {
    var c = make("canvas", "fv-pic");
    c.width = Math.max(1, Math.round(view.width)); c.height = Math.max(1, Math.round(view.height));
    var g = c.getContext("2d");
    view.ops.forEach(function (op) {
      var d = pictures[op.path];
      if (!d) return;
      g.save();
      if (op.clip) { g.beginPath(); g.rect(op.clip[0] - view.left, op.clip[1] - view.top, op.clip[2] - op.clip[0], op.clip[3] - op.clip[1]); g.clip(); }
      g.globalAlpha = Math.max(0, Math.min(1, op.alpha));
      g.imageSmoothingEnabled = op.smooth;
      var x = op.dx - view.left, y = op.dy - view.top;
      g.translate(x + (op.flipX ? op.dw : 0), y + (op.flipY ? op.dh : 0));
      g.scale(op.flipX ? -1 : 1, op.flipY ? -1 : 1);
      g.drawImage(d.image, op.sx, op.sy, op.sw, op.sh, 0, 0, op.dw, op.dh);
      g.restore();
    });
    return c;
  }

  var GAME = "venice";
  // the entry's panel under the Gallery's Game list: { el, show, hide, redraw, state }
  function build(game) {
    var el = make("div", "vb-panel");
    // the picture and Save As on one row, as the Fonts window has its game, font and Save As, and how it is drawn below
    var bar1 = make("div", "fv-bar"), bar2 = make("div", "fv-bar fv-options");
    var pick = make("select", "vb-pick"), backPick = make("select"), zoomPick = make("select"), save = make("button", "fv-save", T("buildings.save"));
    save.type = "button";
    options(backPick, [["scene", T("buildings.back.scene")], ["none", T("buildings.back.none")]]);
    options(zoomPick, [["0.25", "25%"], ["0.5", "50%"], ["1", "100%"], ["2", "200%"]]);
    zoomPick.value = "0.5";
    bar1.appendChild(labelled(T("buildings.picture"), pick));
    bar1.appendChild(save);
    bar2.appendChild(labelled(T("buildings.back"), backPick));
    bar2.appendChild(labelled(T("buildings.zoom"), zoomPick));
    var well = make("div", "fv-sheet vb-well"), status = make("div", "fv-status");
    el.appendChild(bar1); el.appendChild(bar2); el.appendChild(well); el.appendChild(status);

    var data = null, busy = 0, shown = null;
    function say(s) { status.textContent = s; }
    function enable(on) { [pick, backPick, zoomPick, save].forEach(function (c) { c.disabled = !on; }); }
    function present() {
      try { return ByopopGameFiles.present(ByopopGameFiles.all()[GAME]); } catch (e) { return false; }
    }
    function release() {
      if (data) {
        Object.keys(data.pictures).forEach(function (p) { var im = data.pictures[p].image; if (im && im.close) im.close(); });
        if (data.files) data.files.close();
      }
      data = null; shown = null; well.textContent = "";
    }
    function show() {
      var ticket = ++busy;
      release();
      enable(false);
      options(pick, []);
      if (!present()) { say(T("buildings.missing")); return Promise.resolve(); }
      say(T("buildings.reading"));
      return load(game, decodePicture).then(function (d) {
        if (ticket !== busy) { if (d) { data = d; release(); } return; }
        if (!d) { say(T("buildings.missing")); return; }
        data = d;
        var list = [["all", T("buildings.all")]], n = 0;
        d.lay.pictures.forEach(function (p, i) { list.push([String(i), p.win ? T("buildings.trophy") : T("buildings.building", { n: ++n })]); });
        options(pick, list);
        enable(true);
        draw();
      }, function () { if (ticket === busy) say(T("buildings.missing")); });
    }
    function hide() { busy++; release(); enable(false); }
    function draw() {
      well.textContent = "";
      shown = null;
      if (!data) return;
      var scene = backPick.value === "scene", view = picture(data.lay, pick.value, scene);
      if (!view) return;
      var c = paint(view, data.pictures), zoom = +zoomPick.value, r = ratio();
      // at 100% and above each pixel a square of whole screen pixels; below it the browser's smooth scaling
      var k = zoom >= 1 ? Math.max(1, Math.round(zoom * r)) / r : zoom;
      c.style.width = c.width * k + "px";
      c.style.height = c.height * k + "px";
      if (zoom < 1) c.style.imageRendering = "auto";
      well.classList.toggle("fv-clear", !scene);
      well.appendChild(c);
      shown = { canvas: c, name: pick.value === "all" ? "venice-buildings" :
        data.lay.pictures[+pick.value].win ? "venice-trophy" : "venice-building-" + (+pick.value + 1) };
      say(T("buildings.size", { w: c.width, h: c.height }));
    }
    function saveAs() {
      if (!shown) return;
      var name = shown.name;
      shown.canvas.toBlob(function (blob) {
        if (!blob) return;
        var url = URL.createObjectURL(blob), a = document.createElement("a");
        a.href = url;
        a.download = name + ".png";
        document.body.appendChild(a);
        a.click();
        a.remove();
        setTimeout(function () { URL.revokeObjectURL(url); }, 60000);
      }, "image/png");
    }
    [pick, backPick, zoomPick].forEach(function (s) { s.addEventListener("change", draw); });
    save.addEventListener("click", saveAs);
    enable(false);
    return { el: el, show: show, hide: hide, redraw: draw, state: function () { return { loaded: !!data, choice: pick.value, shown: shown && shown.name }; } };
  }

  return { panel: build, setSource: function (fn) { source = fn; },
    readTags: readTags, parseScene: parseScene, parseConfig: parseConfig, thumbnailOf: thumbnailOf, strip: strip,
    layout: layout, fill: fill, picture: picture, load: load };
})();
if (typeof module !== "undefined" && module.exports) module.exports = ByopopBuildings;
