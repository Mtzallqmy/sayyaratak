# Third-party open-source credits / المصادر المفتوحة

سيارتك (Sayyaratak) uses algorithms and ideas ported from these permissively
licensed projects. Full license texts are in `third_party/`.

## Code/algorithm ports (MIT)

| Project | Author | What it contributes |
| --- | --- | --- |
| DasEtwas/enginesound | DasEtwas | Engine audio model in `jni/audio.cpp`: 4-stroke crank cycle, per-cylinder piston motion `cos(4*pi*crank)` and sine ignition pulses in the power stroke |
| DAShoe1/Godot-Easy-Vehicle-Physics | DAShoe1 | Steering feel in `jni/engine.cpp`: separate steer-in (4.25/s) and countersteer (11/s) rates, speed-based steering decay |
| Dechode/Godot-Advanced-Vehicle | Dechode | Automatic gearbox logic in `jni/engine.cpp`: shift by comparing wheel torque in adjacent gears, 0.7 s shift cooldown |
| dicnunz/vehicle-physics | dicnunz | Tire slip curve shape reference for the grip/traction model |
| kakoiihito/Shift | kakoiihito | Transmission/shift timing reference |
| TheDuckCow/godot-road-generator | TheDuckCow | Road ribbon construction ideas for the 3D road mesh (`jni/scene.cpp`) |
| TokisanGames/Terrain3D | Tokisan Games | Heightmap terrain + material blending ideas (mountains/ground) |
| webgl-games/opencity | webgl-games | City block / building placement and facade variation ideas |
| stranmous/RallyCraft | stranmous | Surface grip tables (asphalt/sand) reference |
| EthicalSoftwareCommunity/RacingGame | EthicalSoftwareCommunity | Checkpoint/race flow reference for missions |

## Studied only, no code copied

- Rigs of Rods, trussline, VDrift, SuperTuxKart, Speed Dreams, stuntrally3 (GPL): soft-body and handling ideas studied, no code used.
- ShivamKR12/GTA-Mobile (no license): traffic/mission structure concepts only.

## Assets (CC0)

- Sky: Poly Haven "Syferfontein 0d Clear Pure Sky" HDRI (CC0), tone-mapped to `assets/tex/sky.jpg`.
- Car model, textures: CC0 sources (see assets/ notes).
