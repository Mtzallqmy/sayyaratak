#!/bin/bash
# Sayyaratak build script. Needs: Android SDK (platforms;android-34, build-tools;34.0.0), NDK 26.1, kotlinc 2.0.21
set -e
SDK=${ANDROID_HOME:-$HOME/android}
BT=$SDK/build-tools/34.0.0
AJ=$SDK/platforms/android-34/android.jar
NDK=$SDK/ndk/26.1.10909125
CXX=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++
KOTLINC=${KOTLINC:-$HOME/kotlin/kotlinc/bin/kotlinc}
mkdir -p build/lib/arm64-v8a build/classes build/dex build/pkg/lib/arm64-v8a
$CXX -shared -fPIC -O2 -std=c++17 -o build/lib/arm64-v8a/libsayya.so \
    jni/engine.cpp jni/scene.cpp jni/jni.cpp jni/audio.cpp \
    -static-libstdc++ -lGLESv3 -lEGL -laaudio -llog -landroid -lm
$KOTLINC src/com/sayyaratak/*.kt -classpath $AJ -d build/classes -no-reflect
$BT/d8 --lib $AJ --min-api 26 --output build/dex $(find build/classes -name '*.class') \
    $HOME/kotlin/kotlinc/lib/kotlin-stdlib.jar
$BT/aapt2 compile --dir res -o build/res.zip
$BT/aapt2 link -o build/base.apk -I $AJ --manifest AndroidManifest.xml -A assets build/res.zip
cp build/base.apk build/sayyaratak.apk
(cd build/dex && zip -q ../sayyaratak.apk classes.dex)
cp build/lib/arm64-v8a/libsayya.so build/pkg/lib/arm64-v8a/
(cd build/pkg && zip -q ../sayyaratak.apk lib/arm64-v8a/libsayya.so)
$BT/zipalign -f -p 4 build/sayyaratak.apk build/sayyaratak-aligned.apk
$BT/apksigner sign --ks build/testkey.jks --ks-pass pass:sayyaratak123 --key-pass pass:sayyaratak123 \
    --out build/Sayyaratak.apk build/sayyaratak-aligned.apk
echo "Built build/Sayyaratak.apk"
