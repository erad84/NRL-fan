#!/usr/bin/env python3
import os
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(ROOT, "resources", "images", "_raw")
OUT = os.path.join(ROOT, "resources", "images")

CLUBS = [
    "broncos",
    "bulldogs",
    "cowboys",
    "dolphins",
    "dragons",
    "eels",
    "knights",
    "panthers",
    "rabbitohs",
    "raiders",
    "roosters",
    "sea-eagles",
    "sharks",
    "storm",
    "titans",
    "warriors",
    "wests-tigers",
]

ORIGIN = ["blues", "maroons"]

UA = {"User-Agent": "Mozilla/5.0 (compatible; NRLFan/1.0)"}


def fetch(url, dest):
    req = urllib.request.Request(url, headers=UA)
    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            data = resp.read()
            ctype = resp.headers.get("Content-Type", "")
            print("OK", resp.status, ctype, len(data), url)
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            with open(dest, "wb") as f:
                f.write(data)
            return True
    except Exception as e:
        print("FAIL", url, e)
        return False


def probe_club(key):
    urls = [
        "https://www.nrl.com/.theme/{}/badge.svg".format(key),
        "https://www.nrl.com/.theme/{}/badge.png".format(key),
        "https://www.nrl.com/logos/{}-badge.svg".format(key),
        "https://www.nrl.com/src/resources/nrl/{}/badge.svg".format(key),
    ]
    for url in urls:
        dest = os.path.join(RAW, key + os.path.splitext(url.split("?")[0])[1])
        if fetch(url, dest):
            return dest
    return None


COMP_LOGOS = [
    (
        "https://upload.wikimedia.org/wikipedia/commons/5/50/Telstra_NRL_Women%27s_Premiership.png",
        "nrlw_wiki.png",
    ),
    (
        "https://static.wikia.nocookie.net/logopedia/images/3/36/Women%27s_State_Of_Origin_Logo_2021_(White).png",
        "origin_w.png",
    ),
    (
        "https://static.wikia.nocookie.net/logopedia/images/b/b4/StateOfOrigin_2021.svg",
        "origin_men.svg",
    ),
]


def main():
    os.makedirs(RAW, exist_ok=True)
    fetch("https://www.nrl.com/.theme/nrl/badge.svg", os.path.join(RAW, "nrl.svg"))
    fetch("https://www.nrl.com/.theme/nrl/badge.png", os.path.join(RAW, "nrl.png"))
    for url, name in COMP_LOGOS:
        fetch(url, os.path.join(RAW, name))
    for key in CLUBS + ORIGIN:
        probe_club(key)


if __name__ == "__main__":
    main()
