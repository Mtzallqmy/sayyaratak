#!/usr/bin/env bash
set -euo pipefail

apk=${1:?Expected path to the instrumented APK}
mkdir -p shots

capture() {
    adb exec-out screencap -p > shots/driving.png || true
    adb logcat -d -v threadtime > shots/logcat.txt || true
}
trap capture EXIT

adb shell getprop > shots/device-properties.txt
adb shell settings put global immersive_mode_confirmations confirmed
adb shell svc wifi disable
adb shell svc data disable
adb install -r "$apk"
adb shell pm clear com.sayyaratak | tee shots/app-data.txt
grep -Fxq Success shots/app-data.txt
adb logcat -c
timeout 180 adb shell am instrument -w com.sayyaratak/.DriveInstrumentation \
    | tr -d '\r' | tee shots/instrumentation.txt
grep -Fxq DRIVE_OK shots/instrumentation.txt
grep -Fxq 'INSTRUMENTATION_CODE: -1' shots/instrumentation.txt
