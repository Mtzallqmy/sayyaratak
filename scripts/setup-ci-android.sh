#!/usr/bin/env bash
set -euo pipefail

sdk="$RUNNER_TEMP/android-sdk"
tools="$sdk/cmdline-tools"
mkdir -p "$tools"
curl --fail --location --retry 3 \
    https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip \
    --output "$RUNNER_TEMP/android-command-line-tools.zip"
unzip -q "$RUNNER_TEMP/android-command-line-tools.zip" -d "$tools"
mv "$tools/cmdline-tools" "$tools/latest"

sdkmanager="$tools/latest/bin/sdkmanager"
export ANDROID_HOME="$sdk" ANDROID_SDK_ROOT="$sdk"
# yes exits on SIGPIPE after sdkmanager finishes reading the license responses.
(set +o pipefail; yes | "$sdkmanager" --licenses >/dev/null)
"$sdkmanager" "platform-tools" "$@"
printf 'ANDROID_HOME=%s\nANDROID_SDK_ROOT=%s\n' "$sdk" "$sdk" >> "$GITHUB_ENV"
printf '%s\n%s\n' "$tools/latest/bin" "$sdk/platform-tools" >> "$GITHUB_PATH"
