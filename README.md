# سيارتك | Sayyaratak

Native Kotlin and C++ driving game for landscape Android 8 and later. The
driving-foundation preview starts from main commit
`33c4d2e4bdd85cf859bce81d562a51c17fa17fb4`.

## Build

Use JDK 17, Kotlin 2.0.21, Android SDK platform 34, build tools 35.0.0, NDK
28.2.13676358 and a Linux x86_64 build host. Set `ANDROID_HOME` and `KOTLINC`
to the SDK directory and Kotlin compiler. Set `APK_KEYSTORE`, `APK_KEY_ALIAS`,
`APK_STORE_PASSWORD` and `APK_KEY_PASSWORD` using your authorized signer.
GitHub Actions generates an independent test key for its artifacts.

```sh
./build.sh arm64-v8a
```

Find the standalone APK and SHA-256 in `build/arm64-v8a/`. The build verifies
its signature and 16 KB APK alignment. Installing over an earlier copy
requires a compatible signing certificate.

## Verification

```sh
DRIVE_INSTRUMENTATION=1 ./build.sh x86_64
bash scripts/android-smoke.sh build/x86_64/Sayyaratak.apk
```

Run the smoke script on a disposable Android emulator. It clears this package's
data and disables emulator Wi-Fi and mobile data. The instrumentation opens
free drive using injected touch events. It measures acceleration,
displacement, two-pointer steering, braking and reverse. It backgrounds the
app, recreates the EGL context, checks the returned scene and drives again.
The GitHub Actions artifacts include logs and screenshots.

The Linux/EGL host harness is available in `host_main.cpp` for renderer and
native-engine regression checks. It uses a host-generated ground texture and
stub audio. Physical ARM64 launch, GPU compatibility, audio and sustained
performance must be measured separately. The current preview uses the
existing generated world and cockpit overlay. The Taiz–Sabir production
route, 3D interiors and expanded content are future milestones.

See [THIRD_PARTY.md](THIRD_PARTY.md) and [asset notices](assets/LICENSES.txt).

برمجة وتطوير معتز العلقمي – تعز – اليمن – 2026م
