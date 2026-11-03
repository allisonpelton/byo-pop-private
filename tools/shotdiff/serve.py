#!/usr/bin/env python3
"""Serve build/shotreview/ at http://127.0.0.1:18770/ and save the review pages' marks (docs/SHOTDIFF.md).

    python tools/shotdiff/serve.py [--port 18770] [--root build/shotreview]

The root page lists every game's latest run. A page POSTs its marks to marks.json beside itself
(<game>/<run>/marks.json), which this server writes; nothing else can be written. It listens on the loopback address only
and refuses requests whose Host or Origin is not this server's own.
"""
import argparse
import functools
import http.server
import json
import os
import re
import sys
import urllib.parse

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
DEFAULT_ROOT = os.path.join(ROOT, "build", "shotreview")
DEFAULT_PORT = 18770
MARKS_PATH = re.compile(r"^/([A-Za-z0-9._-]+)/([A-Za-z0-9._-]+)/marks\.json$")
MAX_BODY = 1 << 20


class Handler(http.server.SimpleHTTPRequestHandler):
	server_version = "shotreview"

	def end_headers(self):
		self.send_header("Cache-Control", "no-store")
		super().end_headers()

	def own_origin(self, value):
		port = self.server.server_address[1]
		return value in ("http://127.0.0.1:%d" % port, "http://localhost:%d" % port)

	def reply(self, code, obj):
		body = json.dumps(obj).encode("utf-8")
		self.send_response(code)
		self.send_header("Content-Type", "application/json")
		self.send_header("Content-Length", str(len(body)))
		self.end_headers()
		self.wfile.write(body)

	def do_POST(self):
		# The body is read before any reply, refusals included. A server that answers 403/404 and closes while the
		# client's body is still unread makes Windows reset the connection (RST), and the client then loses the
		# reply it was about to read. A body over MAX_BODY (or without a usable Content-Length) is not read: that
		# client is refused with 413/411 and may see the reset, which is no loss.
		try:
			length = int(self.headers.get("Content-Length", ""))
		except ValueError:
			length = None
		body = self.rfile.read(length) if length is not None and 0 <= length <= MAX_BODY else None
		port = self.server.server_address[1]
		host = self.headers.get("Host", "")
		if host not in ("127.0.0.1:%d" % port, "localhost:%d" % port):
			return self.reply(403, {"error": "wrong Host"})
		origin = self.headers.get("Origin")
		if origin is not None and not self.own_origin(origin):
			return self.reply(403, {"error": "wrong Origin"})
		path = urllib.parse.unquote(urllib.parse.urlparse(self.path).path)
		m = MARKS_PATH.match(path)
		if not m:
			return self.reply(404, {"error": "only <game>/<run>/marks.json can be written"})
		root = os.path.realpath(self.directory)
		run_dir = os.path.realpath(os.path.join(root, m.group(1), m.group(2)))
		if os.path.dirname(os.path.dirname(run_dir)) != root or not os.path.isdir(run_dir):
			return self.reply(404, {"error": "no such run"})
		if length is None:
			return self.reply(411, {"error": "Content-Length needed"})
		if body is None:
			return self.reply(413, {"error": "too large"})
		try:
			doc = json.loads(body.decode("utf-8"))
		except (ValueError, UnicodeDecodeError):
			return self.reply(400, {"error": "not JSON"})
		if not isinstance(doc, dict) or not isinstance(doc.get("marks"), dict):
			return self.reply(400, {"error": "expected an object with a \"marks\" object"})
		target = os.path.join(run_dir, "marks.json")
		tmp = target + ".tmp"
		with open(tmp, "w", encoding="utf-8", newline="\n") as f:
			json.dump(doc, f, indent=1)
			f.write("\n")
		os.replace(tmp, target)
		self.reply(200, {"ok": True, "marks": len(doc["marks"])})

	def log_message(self, fmt, *args):
		sys.stderr.write("shotreview: %s\n" % (fmt % args))


def make_server(root=DEFAULT_ROOT, port=DEFAULT_PORT):
	"""A server on the loopback address (port 0 picks a free one); the caller runs serve_forever()."""
	os.makedirs(root, exist_ok=True)
	return http.server.ThreadingHTTPServer(("127.0.0.1", port), functools.partial(Handler, directory=os.path.abspath(root)))


def main(argv=None):
	ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
	ap.add_argument("--port", type=int, default=DEFAULT_PORT)
	ap.add_argument("--root", default=DEFAULT_ROOT)
	a = ap.parse_args(argv)
	try:
		srv = make_server(a.root, a.port)
	except OSError as e:
		sys.exit("shotreview: cannot listen on 127.0.0.1:%d (%s); another server may hold the port" % (a.port, e))
	print("shotreview: serving %s at http://127.0.0.1:%d/" % (os.path.abspath(a.root), srv.server_address[1]), flush=True)
	try:
		srv.serve_forever()
	except KeyboardInterrupt:
		pass
	finally:
		srv.server_close()


if __name__ == "__main__":
	main()
