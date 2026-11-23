# Sayyaratak - remaining work for the complete build

DONE (verified on headless GLES3 harness):
- Black-ground root cause fixed (shared texture store, was per-TU static)
- Cities densified (street walls, outskirts rings, denser Shibam), drooping palms, HUD insets
- CC0 supercar (tevy.dev Mid-Engine Supercar, CC0 1.0): tools/glb2car.py converts
  decompressed GLB -> assets/models/car.bin (+car_lod.bin). gltf-transform CLI used to
  decode EXT_meshopt_compression first. Wheels/steering animated parts, paint recolor works.
- Weight transfer (pitch/roll) on the car body
- CC0 textures in assets/tex (ambientCG: Road012A asphalt, Ground054 sand, Plaster007,
  Rock035 - all CC0) wired into WorldPainter ground + facades (with procedural fallback)
- Git repo + .github/workflows/build-and-test.yml (arm64 build + x86_64 emu APK +
  emulator run with screenshots). CI generates a throwaway key; no secrets in repo.

WAITING ON: user saves fine-grained GitHub PAT (repo sayyaratak only, Contents RW +
Actions RW) via vault link; then: git remote add origin https://github.com/Mtzallqmy/sayyaratak,
push main, workflow runs, pull screenshots via API, iterate.

TODO next (in order):
1. Push + CI screenshots once token arrives (vault write-file to copy token into sandbox)
2. Moving traffic: cars following road lanes (use car_lod.bin parts, per-car paint)
3. Missions + progression: per-city checkpoint races, timed deliveries, drift targets;
   persistent currency/stars/level (SharedPreferences); workshop items unlock with earnings
4. Credits screen + assets/LICENSES.txt (tevy.dev CC0 model; ambientCG CC0 textures;
   Tajawal font OFL; gltf-transform Apache-2.0 build tool)
5. Extend host harness into scripted playtest (drive each city, workshop toggles, mission)
6. Final verify + build v5 (versionCode 5) + honest report

HONEST LIMITS to keep reporting: Kotlin-side texture path not covered by host harness
(harness paints its own ground); verified by compile + CI emulator screenshots.
