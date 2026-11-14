#!/usr/bin/env python3
"""Serve the PrismaUI panel (ui/) in a normal browser, backed by PracticeSim.

PracticeSim runs the real sub-skill model (plugin/src/Core.cpp) without the
game. This server:
  * serves ui/ as-is, except that index.html gets the simulator bridge
    (/sim/dev-bridge.js, /sim/dev.css) injected before app.js;
  * serves tools/ui-sim/ under /sim/;
  * forwards POST /sim/action (one JSON action) to PracticeSim and returns its
    snapshot.
Binds to 127.0.0.1 only. Standard library only. See tools/ui-sim/README.md.
"""
import argparse
import http.server
import json
import pathlib
import subprocess
import sys
import threading

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
UI = ROOT / "ui"
CANDIDATES = [
    ROOT / "build/prototype/Release/PracticeSim.exe",
    ROOT / "build/prototype/PracticeSim.exe",
    ROOT / "build/prototype/PracticeSim",
    ROOT / "build/sim/PracticeSim",
    ROOT / "build/sim/Release/PracticeSim.exe",
]
TYPES = {".html": "text/html; charset=utf-8", ".js": "text/javascript; charset=utf-8",
         ".css": "text/css; charset=utf-8", ".json": "application/json"}


class Backend:
    def __init__(self, binary, config, base):
        self.args = [str(binary), str(config), str(base)]
        self.lock = threading.Lock()
        self.proc = None
        self.start()

    def start(self):
        self.proc = subprocess.Popen(self.args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     text=True, encoding="utf-8", bufsize=1)

    def call(self, action):
        line = json.dumps(action, ensure_ascii=False)
        with self.lock:
            if self.proc.poll() is not None:
                print("PracticeSim exited; restarting (state is lost)", file=sys.stderr)
                self.start()
            self.proc.stdin.write(line + "\n")
            self.proc.stdin.flush()
            answer = self.proc.stdout.readline()
        if not answer:
            raise RuntimeError("PracticeSim gave no answer (see its stderr)")
        return answer


def make_handler(backend):
    class Handler(http.server.BaseHTTPRequestHandler):
        def log_message(self, fmt, *args):
            pass  # keep the console for PracticeSim errors

        def send(self, code, body, kind):
            data = body.encode("utf-8") if isinstance(body, str) else body
            self.send_response(code)
            self.send_header("Content-Type", kind)
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(data)

        def static(self, base, name):
            path = (base / name).resolve()
            if base.resolve() not in path.parents or not path.is_file():
                return self.send(404, "not found", "text/plain")
            data = path.read_bytes()
            if base == UI and name == "index.html":
                html = data.decode("utf-8")
                inject = ('<link rel="stylesheet" href="/sim/dev.css">'
                          '<script src="/sim/dev-bridge.js"></script>')
                if '<script src="app.js">' not in html:
                    return self.send(500, "index.html: app.js tag not found", "text/plain")
                data = html.replace('<script src="app.js">', inject + '<script src="app.js">', 1)
            self.send(200, data, TYPES.get(path.suffix, "application/octet-stream"))

        def do_GET(self):
            path = self.path.split("?", 1)[0]
            if path in ("/", "/index.html"):
                return self.static(UI, "index.html")
            if path.startswith("/sim/"):
                return self.static(HERE, path[len("/sim/"):])
            return self.static(UI, path.lstrip("/"))

        def do_POST(self):
            if self.path != "/sim/action":
                return self.send(404, "not found", "text/plain")
            length = int(self.headers.get("Content-Length", 0))
            try:
                action = json.loads(self.rfile.read(length) or b"{}")
                self.send(200, backend.call(action), "application/json")
            except Exception as error:  # report to the page, keep serving
                self.send(500, json.dumps({"error": str(error)}), "application/json")

    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--sim", type=pathlib.Path, help="PracticeSim executable")
    parser.add_argument("--config", type=pathlib.Path, default=ROOT / "config")
    parser.add_argument("--base", type=float, default=15, help="start level of every main skill")
    parser.add_argument("--port", type=int, default=8077)
    args = parser.parse_args()
    binary = args.sim or next((c for c in CANDIDATES if c.is_file()), None)
    if not binary or not binary.is_file():
        sys.exit("PracticeSim not found; build it (tools/ui-sim/README.md) or pass --sim")
    backend = Backend(binary, args.config, args.base)
    server = http.server.ThreadingHTTPServer(("127.0.0.1", args.port), make_handler(backend))
    print(f"UI simulator: http://127.0.0.1:{args.port}/  (backend {binary}, config {args.config})")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        backend.proc.terminate()


if __name__ == "__main__":
    main()
