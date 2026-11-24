#!/usr/bin/env python3
# Press and hold the center of the first UI node whose text contains the given
# substring, via a stationary swipe. Usage: hold_text.py "وقود" 4000
import subprocess, re, sys

needle = sys.argv[1]
ms = sys.argv[2] if len(sys.argv) > 2 else "3000"
subprocess.run(["adb", "shell", "uiautomator", "dump", "/sdcard/ui.xml"], capture_output=True)
data = subprocess.run(["adb", "shell", "cat", "/sdcard/ui.xml"], capture_output=True, text=True).stdout
for m in re.finditer(r'text="([^"]*)"[^>]*bounds="\[(\d+),(\d+)\]\[(\d+),(\d+)\]"', data):
    if needle in m.group(1):
        x = (int(m.group(2)) + int(m.group(4))) // 2
        y = (int(m.group(3)) + int(m.group(5))) // 2
        print("hold", repr(m.group(1))[:40], x, y, ms + "ms")
        subprocess.run(["adb", "shell", "input", "swipe", str(x), str(y), str(x), str(y), ms])
        sys.exit(0)
print("not found:", needle)
sys.exit(1)
