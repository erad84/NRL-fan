import json
import sys
import zipfile

path = sys.argv[1] if len(sys.argv) > 1 else "build/NRL Watch.pbw"
with zipfile.ZipFile(path) as z:
    data = json.loads(z.read("appinfo.json"))
print("versionLabel:", data.get("versionLabel"))
print("versionCode:", data.get("versionCode"))
