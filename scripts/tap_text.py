#!/usr/bin/env python3
# Tap the center of the first UI node whose text contains the given substring.
# Usage: tap_text.py "GOT IT"  -> exit 0 if tapped, 1 if not found
import subprocess, re, sys

needle = sys.argv[1]
subprocess.run(["adb", "shell", "uiautomator", "dump", "/sdcard/ui.xml"], capture_output=True)
data = subprocess.run(["adb", "shell", "cat", "/sdcard/ui.xml"], capture_output=True, text=True).stdout
for m in re.finditer(r'text="([^"]*)"[^>]*bounds="\[(\d+),(\d+)\]\[(\d+),(\d+)\]"', data):
    if needle in m.group(1):
        x = (int(m.group(2)) + int(m.group(4))) // 2
        y = (int(m.group(3)) + int(m.group(5))) // 2
        print("tap", repr(m.group(1))[:40], x, y)
        subprocess.run(["adb", "shell", "input", "tap", str(x), str(y)])
        sys.exit(0)
print("not found:", needle)
sys.exit(1)
