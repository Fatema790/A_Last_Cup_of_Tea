# A Last Cup of Tea
### A Campaign for Nature

A native, real-time **3D interactive simulation** for a Computer Graphics & Animation Lab. Built in C++17 with compatibility OpenGL, GLU and FreeGLUT. The entire valley is procedural geometry: no downloaded models, textures, shaders, video or web runtime.

> A peaceful moment with nature should never become our last one.

## Run on Windows

Double-click *Play.cmd* in this folder to play. Or download last_cup.exe from the Releases section (right side of this page). It is a 64-bit Windows executable. An OpenGL-capable display driver is required. Click inside the window to capture the mouse. Tab releases it.

Version 4 adds a complete disaster story: activate the glowing red baton with E, then watch the warning, earthquake, lava eruptions, meteor impacts, extreme storm, slow last cup, final collapse and fade. P pauses, R restarts, and M switches off camera shake. See [Disaster story](docs/DISASTER.md).

Version 3 added a larger furnished, walkable house with an automatic front door, first-person view, warm lighting, paths and a fenced cow farm with four animated cows, a barn, hay and water troughs. See [Home and farm](docs/HOMESTEAD.md) for a tour.

Version 2 added a visible, articulated human protagonist, a following camera, an animated tea-drinking sequence, four seasons and six weather states. The original valley, observations, damage event, restoration tasks and final tea objective remain in place.

## Build

### Windows: portable tools included locally

Run `build-windows.ps1` in PowerShell. The script uses the compiler and CMake in `.tools` when present; it also supports CMake and a MinGW compiler on PATH. The local tools directory is excluded from Git. FreeGLUT source is taken from `.tools/freeglut-3.6.0` if available, otherwise CMake downloads the pinned official source release.

```powershell
powershell -ExecutionPolicy Bypass -File .\build-windows.ps1
.\dist\last_cup.exe
```

### Windows: Visual Studio with Desktop development with C++

```powershell
cmake -S . -B build-vs
cmake --build build-vs --config Release
ctest --test-dir build-vs -C Release --output-on-failure
.\build-vs\Release\last_cup.exe
```

CMake fetches and builds FreeGLUT; the first configure needs internet access. Use the **Desktop development with C++** workload, including the Windows SDK. An IDE installation alone does not supply a compiler.

### Linux with a system FreeGLUT installation

On Debian/Ubuntu, install `build-essential cmake freeglut3-dev libglu1-mesa-dev`, then:

```sh
cmake -S . -B build -DTEA_FETCH_FREEGLUT=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/last_cup
```

Single-file compilation is also supported:

```sh
g++ -std=c++17 -O2 main.cpp -o last_cup -lglut -lGLU -lGL
```

Linux build instructions are provided for portability; see `docs/VALIDATION.md` for the platform actually tested.

## Controls

| Input | Action |
|---|---|
| W / A / S / D | Move forward / left / backward / right |
| Mouse | Orbit the following camera after clicking in the window |
| E | Interact; leave the tea table; toggle home lights or put out hay |
| F | Cycle disaster playback speed: 1x, 4x, 8x |
| M | Toggle disaster camera shake |
| V | Toggle first-person and following-camera views |
| Q / C | Raise / lower the camera angle while the character stays on the ground |
| F1 / F2 / F3 / F4 | Spring / summer / autumn / winter |
| 1 / 2 / 3 | Clear / cloudy / rain |
| 4 / 5 / 6 | Storm / snow / windy |
| T | Toggle automatic seasons, one season every 90 seconds |
| P | Pause / resume |
| R | Restart the entire campaign |
| H | Toggle the short help panel |
| Tab | Capture / release the mouse |
| Escape | Exit |

The player is now the human character, and interaction distance is measured from the character rather than the following camera. Movement is normalized so diagonals are not faster. Mouse pitch ranges from -65 to -5 degrees in the following view, and -65 to +65 in first person. The camera boom shortens around buildings and tree canopies. Q/C changes the viewing angle; E is exclusively for interactions.

Selecting a season gently blends the entire landscape and chooses a suitable starting weather: clear in spring/summer, windy in autumn, snow in winter. Number keys then let you choose any weather independently. Weather never awards restoration progress or cancels environmental damage.

Rain and storms prompt the character to walk to the covered bench on the enlarged house porch; a character already indoors stays there. The route uses the bridge and avoids major obstacles. Press WASD to take control at any point; if already seated, the character stands first. Clear weather automatically releases the character from shelter. Snow slows walking slightly and makes breath visible. Lightning occurs infrequently, roughly every 10–17 seconds after a storm settles in.

## Campaign walkthrough

1. Walk toward the tea table and press **E**. The character walks around to the chair, turns, sits, slides the chair closer, reaches for the cup, lifts it to the mouth, sips, and puts it back. The camera moves to a side view so the performance is visible. Allow about 12–18 seconds, depending on the approach. Press E to leave; an early request waits for the cup to return safely before standing.
2. Explore the flowers, tree, river and birdwatching perch. The top-right map shows the player direction, tea table (**T**), house (**H**), factory (**F**), warning (**!**) and active restoration goals (**G**).
3. Find the warning sign at **(12, -8)** and investigate. The valley deteriorates over **24 seconds**. Trees shake and fall in staggered groups; the river darkens, grass dries, smoke grows and wildlife fades.
4. Complete six actions, in any order, once the damage event ends:
   - Plant a tree at **(-9, 1)**.
   - Collect three litter piles at **(-5, 8)**, **(6, 8)** and **(5, 19)**.
   - Switch off the factory at **(21, -15)**.
   - Restore the wildlife refuge at **(-12, 23)**.
5. Use the bridge at **x = 0**, spanning **z = 9–18**, to reach the far bank. Coordinates here are `(x, z)` in the horizontal ground plane.
6. Recovery eases toward the fraction of actions completed. Grass returns around 20%, trees around 40%, butterflies around 60%, and birds around 80%. These thresholds use smooth transition bands, rather than instantaneous switches.
7. When recovery reaches 100%, return to the tea table and press E. Stay for the final message: **“Don't let this be the last cup.”**

The warning cannot trigger until the first tea interaction. Repeated task presses do not award additional progress. Restoring nature is an intentionally compressed educational metaphor, not a scientific ecosystem forecast.

## Project structure

- `main.cpp` — original environment, campaign, input, rendering and integrated tests.
- `Disaster.h` / `Disaster.inl` - disaster story, procedural fissures and lava, bounded meteors/particles, character direction and cinematic camera.
- `Homestead.h` / `Homestead.inl` - furnished home, wall and door collisions, automatic door, first-person support and animated cow farm.
- `LivingWorld.h` — character, season, weather and bounded particle data.
- `LivingWorld.inl` — navigation, human animation, articulated model, camera, weather and particle implementation. Included by `main.cpp`, so a single compiler command still works.
- `Play.cmd` — double-click launcher for Windows.
- `CMakeLists.txt` — native build and CTest configuration.
- `build-windows.ps1` — local Windows build, tests and executable packaging.
- `docs/LAB_REPORT.md` — thirteen report sections, with screenshot references.
- `docs/VIVA.md` — 55 short viva questions and answers.
- `docs/LIVING_WORLD.md` — new controls, animation stages and implementation notes.
- `docs/VALIDATION.md` — build and test evidence and known limits.
- `screenshots/` — screenshots generated by the real OpenGL renderer.
- `dist/` — local executable and required third-party notices after building.

## Architecture and lab concepts

`World` holds campaign data, `Human` holds the protagonist's pose and action, and `Atmosphere` holds weather and season weights. `update()` advances the simulation using elapsed seconds; rendering reads this state. `EnvironmentState` controls the healthy → warning → damaged → recovering → restored sequence. Each tree separately follows normal → shake → falling → removed. The keyboard callbacks only record movement state or dispatch one-shot actions. The GLUT timer updates the simulation and requests a repaint.

The world uses x for left/right, y for height and z for depth. `gluPerspective` supplies perspective and `gluLookAt` constructs the camera view. Objects use nested translation, rotation and scaling with matrix push/pop. Cylinder axes are rotated from GLU's z direction into the world's y direction. `GL_NORMALIZE` corrects scaled normals for lighting. OpenGL depth testing determines which surfaces are visible. The HUD saves the perspective/view matrices, switches to orthographic pixel coordinates, then restores both stacks.

Materials use vertex colour for ambient/diffuse response and `glMaterialfv` for specular response. Sunlight and fog interpolate with damage. Blending is enabled for water, steam, smoke and contact shadows; transparent passes disable depth writes and smoke particles are sorted from far to near. Shadows are stylized contact patches, not shadow maps. River streaks suggest reflected light; there is no physically calculated reflection or refraction.

## Verification commands

```powershell
.\dist\last_cup.exe --self-test
.\dist\last_cup.exe --render-check screenshots
.\dist\last_cup.exe --benchmark
```

The first command runs 144 simulation checks without creating a graphics window. The second opens the native renderer, captures 39 campaign, season, weather, character, home, farm and disaster states to portable pixmap (`.ppm`) files and exits. These screenshots are deterministic rendering fixtures; separate logic tests exercise actual interactions and complete animations. The benchmark measures 240 animated storm frames and checks for OpenGL errors. Normal play always starts healthy and requires player actions.

## Dependencies and credits

- [FreeGLUT 3.6.0 source and license](https://github.com/freeglut/freeglut/tree/v3.6.0)
- [OpenGL reference pages](https://registry.khronos.org/OpenGL-Refpages/gl2.1/)
- [w64devkit](https://github.com/skeeto/w64devkit), portable Windows compiler used for local validation
- [CMake](https://cmake.org/), build system

All scene geometry and campaign text are implemented in this project. No audio is included. See the third-party notices packaged with the executable.
