#!/usr/bin/env python3
"""Serve build/web for the player-only page's tests, with the staged game files and a place for its shots (docs/WEB_PLAYER.md,
"The player-only page").

    python tools/webtest/serve.py [--port 18780] [--root build/web] [--assets assets] [--shots build/webshots]

  /...                          the web build (build/web)
  /__assets/<game>/__list       the staged files of assets/<game>/ (git-ignored), as a JSON list of paths
  /__assets/<game>/<path>       one of them
  /__repo/games/<game>/tools/<path>.js   a session's driver script (?driver=), from the repo
  /__repo/tools/web/<name>.js           a shared driver (tools/web/pagecheck.js)
  /__repo/tools/a11ycheck.js            the accessibility checker, which tools/web/deskcheck.js runs on the live page
  /__repo/docs/game_status.json         each game's steps (the coordinator's), shown on the listing on development hosts only
                                        (web/phase.js); nothing else of the repo
  POST /__shot                  {"run": ..., "name": ..., "png": "data:image/png;base64,..."}: saved as
                                build/webshots/<run>/<name>.png, for tools/shotdiff (--port build/webshots/<run>)

It listens on the loopback address only and refuses requests whose Host or Origin is not this server's own, and writes
nothing but those PNGs.
"""
import argparse
import base64
import functools
import http.server
import json
import os
import re
import sys
import urllib.parse

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
DEFAULT_PORT = 18780
NAME = re.compile(r"^[A-Za-z0-9._-]{1,100}$")
MAX_BODY = 64 << 20
DRIVER = re.compile(r"^(games/[a-z0-9]+/tools/[A-Za-z0-9._/-]+|tools/web/[A-Za-z0-9._-]+|tools/a11ycheck)\.js$")
PNG_PREFIX = "data:image/png;base64,"


class Handler(http.server.SimpleHTTPRequestHandler):
	server_version = "webtest"
	extensions_map = dict(http.server.SimpleHTTPRequestHandler.extensions_map, **{".wasm": "application/wasm", ".js": "text/javascript"})

	def end_headers(self):
		self.send_header("Cache-Control", "no-store")
		super().end_headers()

	def own(self):
		port = self.server.server_address[1]
		mine = ("127.0.0.1:%d" % port, "localhost:%d" % port)
		if self.headers.get("Host", "") not in mine:
			return False
		origin = self.headers.get("Origin")
		return origin is None or origin in ("http://" + h for h in mine)

	def reply(self, code, obj):
		body = json.dumps(obj).encode("utf-8")
		self.send_response(code)
		self.send_header("Content-Type", "application/json")
		self.send_header("Content-Length", str(len(body)))
		self.end_headers()
		self.wfile.write(body)

	def assets_path(self, path):
		"""(game folder, rel) for /__assets/<game>/<rel>, or None"""
		m = re.match(r"^/__assets/([A-Za-z0-9._-]+)/(.*)$", path)
		if not m or m.group(1) in (".", ".."):
			return None
		base = os.path.realpath(os.path.join(self.server.assets, m.group(1)))
		if os.path.dirname(base) != os.path.realpath(self.server.assets) or not os.path.isdir(base):
			return None
		return base, m.group(2)

	def do_GET(self):
		if not self.own():
			return self.reply(403, {"error": "wrong Host or Origin"})
		path = urllib.parse.unquote(urllib.parse.urlparse(self.path).path)
		if path.startswith("/__repo/"):
			return self.driver(path[len("/__repo/"):])
		if not path.startswith("/__assets/"):
			return super().do_GET()
		found = self.assets_path(path)
		if not found:
			return self.reply(404, {"error": "no such staged game"})
		base, rel = found
		if rel == "__list":
			files = []
			for d, _, names in os.walk(base):
				for n in names:
					files.append(os.path.relpath(os.path.join(d, n), base).replace(os.sep, "/"))
			files.sort()
			return self.reply(200, files)
		target = os.path.realpath(os.path.join(base, rel))
		if not target.startswith(base + os.sep) or not os.path.isfile(target):
			return self.reply(404, {"error": "no such file"})
		with open(target, "rb") as f:
			data = f.read()
		self.send_response(200)
		self.send_header("Content-Type", "application/octet-stream")
		self.send_header("Content-Length", str(len(data)))
		self.end_headers()
		self.wfile.write(data)

	def driver(self, rel):
		if rel == "docs/game_status.json":
			target = os.path.join(ROOT, "docs", "game_status.json")
			if not os.path.isfile(target):
				return self.reply(404, {"error": "no docs/game_status.json"})
			with open(target, "rb") as f:
				data = f.read()
			self.send_response(200)
			self.send_header("Content-Type", "application/json; charset=utf-8")
			self.send_header("Content-Length", str(len(data)))
			self.end_headers()
			self.wfile.write(data)
			return
		if not DRIVER.match(rel) or ".." in rel.split("/"):
			return self.reply(404, {"error": "only games/<game>/tools/<path>.js and tools/web/<name>.js"})
		target = os.path.realpath(os.path.join(ROOT, rel))
		tools = os.path.realpath(os.path.join(ROOT, *rel.split("/")[:3 if rel.startswith("games/") else 2 if rel.startswith("tools/web/") else 1]))
		if not target.startswith(tools + os.sep) or not os.path.isfile(target):
			return self.reply(404, {"error": "no such driver"})
		with open(target, "rb") as f:
			data = f.read()
		self.send_response(200)
		self.send_header("Content-Type", "text/javascript; charset=utf-8")
		self.send_header("Content-Length", str(len(data)))
		self.end_headers()
		self.wfile.write(data)

	def do_HEAD(self):
		if not self.own():
			return self.reply(403, {"error": "wrong Host or Origin"})
		return super().do_HEAD()

	def do_POST(self):
		# the body is read before any reply, refusals included (tools/shotdiff/serve.py says why)
		try:
			length = int(self.headers.get("Content-Length", ""))
		except ValueError:
			length = None
		body = self.rfile.read(length) if length is not None and 0 <= length <= MAX_BODY else None
		if not self.own():
			return self.reply(403, {"error": "wrong Host or Origin"})
		if urllib.parse.urlparse(self.path).path != "/__shot":
			return self.reply(404, {"error": "only /__shot can be posted"})
		if body is None:
			return self.reply(413 if length is not None else 411, {"error": "no usable body"})
		try:
			doc = json.loads(body.decode("utf-8"))
			run, name, png = doc["run"], doc["name"], doc["png"]
		except (ValueError, UnicodeDecodeError, KeyError, TypeError):
			return self.reply(400, {"error": "expected {run, name, png}"})
		if not (isinstance(run, str) and isinstance(name, str) and NAME.match(run) and NAME.match(name)) or run.startswith(".") or name.startswith("."):
			return self.reply(400, {"error": "run and name: letters, digits, '.', '_' and '-'"})
		if not isinstance(png, str) or not png.startswith(PNG_PREFIX):
			return self.reply(400, {"error": "png: a PNG data URL"})
		try:
			data = base64.b64decode(png[len(PNG_PREFIX):], validate=True)
		except ValueError:
			return self.reply(400, {"error": "png: bad base64"})
		if not name.lower().endswith(".png"):
			name += ".png"
		out = os.path.join(self.server.shots, run)
		os.makedirs(out, exist_ok=True)
		target = os.path.join(out, name)
		with open(target + ".tmp", "wb") as f:
			f.write(data)
		os.replace(target + ".tmp", target)
		self.reply(200, {"ok": True, "path": os.path.relpath(target, ROOT).replace(os.sep, "/"), "bytes": len(data)})

	def log_message(self, fmt, *args):
		if self.server.verbose:
			sys.stderr.write("webtest: %s\n" % (fmt % args))


def make_server(root, assets, shots, port=DEFAULT_PORT, verbose=False):
	"""A server on the loopback address (port 0 picks a free one); the caller runs serve_forever()."""
	srv = http.server.ThreadingHTTPServer(("127.0.0.1", port), functools.partial(Handler, directory=os.path.abspath(root)))
	srv.assets, srv.shots, srv.verbose = os.path.abspath(assets), os.path.abspath(shots), verbose
	return srv


def main(argv=None):
	ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
	ap.add_argument("--port", type=int, default=DEFAULT_PORT)
	ap.add_argument("--root", default=os.path.join(ROOT, "build", "web"))
	ap.add_argument("--assets", default=os.path.join(ROOT, "assets"))
	ap.add_argument("--shots", default=os.path.join(ROOT, "build", "webshots"))
	ap.add_argument("--verbose", action="store_true", help="log every request")
	a = ap.parse_args(argv)
	try:
		srv = make_server(a.root, a.assets, a.shots, a.port, a.verbose)
	except OSError as e:
		sys.exit("webtest: cannot listen on 127.0.0.1:%d (%s); another server may hold the port" % (a.port, e))
	print("webtest: serving %s at http://127.0.0.1:%d/ (staged files from %s)" % (os.path.abspath(a.root), srv.server_address[1], a.assets), flush=True)
	try:
		srv.serve_forever()
	except KeyboardInterrupt:
		pass
	finally:
		srv.server_close()


if __name__ == "__main__":
	main()
