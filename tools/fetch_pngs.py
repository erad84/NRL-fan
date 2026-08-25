#!/usr/bin/env python3
import os
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(ROOT, "resources", "images", "_raw")
KEYS = [
    "broncos", "bulldogs", "cowboys", "dolphins", "dragons", "eels",
    "knights", "panthers", "rabbitohs", "raiders", "roosters", "sea-eagles",
    "sharks", "storm", "titans", "warriors", "wests-tigers",
    "blues", "maroons", "nrlw",
]
UA = {"User-Agent": "Mozilla/5.0 (compatible; NRLFan/1.0)"}


def fetch(url, dest):
    req = urllib.request.Request(url, headers=UA)
    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            data = resp.read()
            if resp.status == 200 and data[:8] == b"\x89PNG\r\n\x1a\n":
                with open(dest, "wb") as f:
                    f.write(data)
                print("PNG", len(data), url)
                return True
            print("NOTPNG", resp.status, url, data[:20])
    except Exception as e:
        print("FAIL", url, e)
    return False


for key in KEYS:
    dest = os.path.join(RAW, key + ".png")
    if fetch("https://www.nrl.com/.theme/{}/badge.png".format(key), dest):
        continue
    fetch("https://www.nrl.com/.theme/{}/badge-basic24.png".format(key), dest)
