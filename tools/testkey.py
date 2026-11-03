"""Makes the site's test key (docs/WEB_PLAYER.md, "The terms"): secrets/web-testkey.txt, git-ignored, holding one random key.
A deployed page opened with ?testkey=<key> skips the terms card for its tab and counts no play; the web build bakes in only the
key's SHA-256 (cmake/ByopopWeb.cmake). Run it once in the main checkout; tools/setup_worktree.sh copies the file into new
worktrees. It never overwrites a key that is there, and it prints the file's path, not the key.

    python tools/testkey.py
"""
import os
import secrets

path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "secrets", "web-testkey.txt")
path = os.path.normpath(path)
if os.path.exists(path):
    print("testkey.py: already there, unchanged: " + path)
else:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="ascii", newline="\n") as f:
        f.write(secrets.token_urlsafe(24) + "\n")
    print("testkey.py: made " + path + " (configure the web build again to use it)")
