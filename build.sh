#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

sdk=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-$HOME/android}}
build_tools="$sdk/build-tools/35.0.0"
android_jar="$sdk/platforms/android-34/android.jar"
ndk="$sdk/ndk/28.2.13676358"
kotlinc=${KOTLINC:-$HOME/kotlin/kotlinc/bin/kotlinc}
kotlin_lib=$(dirname "$(realpath "$kotlinc")")/../lib/kotlin-stdlib.jar

abi=${1:-arm64-v8a}
case "$abi" in
    arm64-v8a) target=aarch64 ;;
    x86_64) target=x86_64 ;;
    *) echo "Expected arm64-v8a or x86_64" >&2; exit 2 ;;
esac

: "${APK_KEYSTORE:?Set APK_KEYSTORE to an authorized test or release keystore}"
: "${APK_KEY_ALIAS:?Set APK_KEY_ALIAS}"
: "${APK_STORE_PASSWORD:?Set APK_STORE_PASSWORD}"
: "${APK_KEY_PASSWORD:?Set APK_KEY_PASSWORD}"
export APK_STORE_PASSWORD APK_KEY_PASSWORD

out="build/$abi"
mkdir -p "$out"
stage=$(mktemp -d "$PWD/build/stage.XXXXXX")
trap 'rm -rf "$stage"' EXIT
mkdir -p "$stage/lib/$abi" "$stage/classes" "$stage/dex"

sources=(src/com/sayyaratak/*.kt)
manifest=AndroidManifest.xml
link_flags=()
if [[ ${DRIVE_INSTRUMENTATION:-0} == 1 ]]; then
    sources+=(tests/android/*.kt)
    manifest="$stage/AndroidManifest.xml"
    sed '/<\/manifest>/i\    <instrumentation android:name=".DriveInstrumentation" android:targetPackage="com.sayyaratak"/>' AndroidManifest.xml > "$manifest"
    link_flags+=(--debug-mode)
fi

"$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/${target}-linux-android26-clang++" \
    -shared -fPIC -O2 -std=c++17 -o "$stage/lib/$abi/libsayya.so" \
    jni/engine.cpp jni/scene.cpp jni/jni.cpp jni/audio.cpp \
    -static-libstdc++ -lGLESv3 -lEGL -laaudio -llog -landroid -lm

"$kotlinc" "${sources[@]}" -classpath "$android_jar" -d "$stage/classes" -no-reflect
mapfile -t classes < <(find "$stage/classes" -name '*.class' -type f | sort)
"$build_tools/d8" --lib "$android_jar" --min-api 26 --output "$stage/dex" "${classes[@]}" "$kotlin_lib"
"$build_tools/aapt2" compile --dir res -o "$stage/res.zip"
"$build_tools/aapt2" link "${link_flags[@]}" -o "$stage/base.apk" -I "$android_jar" \
    --manifest "$manifest" -A assets "$stage/res.zip"
(cd "$stage/dex" && zip -q ../base.apk classes*.dex)
(cd "$stage" && zip -q -0 base.apk "lib/$abi/libsayya.so")

"$build_tools/zipalign" -f -P 16 4 "$stage/base.apk" "$stage/aligned.apk"
"$build_tools/apksigner" sign --ks "$APK_KEYSTORE" --ks-key-alias "$APK_KEY_ALIAS" \
    --ks-pass env:APK_STORE_PASSWORD --key-pass env:APK_KEY_PASSWORD \
    --out "$stage/Sayyaratak.apk" "$stage/aligned.apk"
"$build_tools/zipalign" -c -P 16 4 "$stage/Sayyaratak.apk"
"$build_tools/apksigner" verify --print-certs "$stage/Sayyaratak.apk"
mv "$stage/Sayyaratak.apk" "$out/Sayyaratak.apk"
(cd "$out" && sha256sum Sayyaratak.apk > Sayyaratak.apk.sha256)
echo "Built $out/Sayyaratak.apk"
