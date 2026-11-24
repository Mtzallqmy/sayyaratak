#!/usr/bin/env bash
set -euo pipefail

apk=${1:?Expected path to the instrumented APK}
mkdir -p shots

capture() {
    if adb shell run-as com.sayyaratak test -s cache/drive.png >/dev/null 2>&1; then
        adb exec-out run-as com.sayyaratak cat cache/drive.png > shots/driving.png || true
    else
        adb exec-out screencap -p > shots/driving.png || true
    fi
    adb logcat -d -v threadtime > shots/logcat.txt || true
}
trap capture EXIT

adb shell getprop > shots/device-properties.txt
api=$(adb shell getprop ro.build.version.sdk | tr -d '\r')
if [[ "$api" -lt 28 ]]; then
    # Older AOSP images require root for svc wifi/data, unlike current shell builds.
    if [[ $(adb shell getprop ro.kernel.qemu | tr -d '\r') != 1 ]]; then
        echo "This script requires a disposable Android emulator" >&2
        exit 1
    fi
    adb root
    adb wait-for-device
fi
adb shell settings put secure immersive_mode_confirmations confirmed
adb shell svc wifi disable
adb shell svc data disable
adb install -r "$apk"
adb shell pm clear com.sayyaratak | tee shots/app-data.txt
grep -Fxq Success shots/app-data.txt
adb logcat -c || echo "Previous device logs could not be cleared"
timeout 180 adb shell am instrument -w com.sayyaratak/.DriveInstrumentation \
    | tr -d '\r' | tee shots/instrumentation.txt
grep -Fxq DRIVE_OK shots/instrumentation.txt
grep -Fxq 'INSTRUMENTATION_CODE: -1' shots/instrumentation.txt
