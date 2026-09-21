# Extreme disaster story - version 4

## Start the story

Run Play.cmd. Walk to the glowing red emergency baton just left of the starting area, southwest of the tea table. It is marked D on the minimap. Approach until the E prompt says Activate disaster sequence, then press E. The baton is a physical control on a pedestal, modeled from cylinders, rings and a glowing tip.

Press **F** during the disaster to cycle **1x -> 4x -> 8x -> 1x**. The current speed appears beside the cinematic title. At 8x, the story takes roughly 20 seconds. Fast-forward advances all animation and effects in small simulation steps, preserving collisions and the final tea performance. Pause still works, and restart returns to 1x.

The character walks to the tea table and drinks before the warning begins. The cinematic takes roughly two and a half minutes, depending on the initial approach. It runs in the real 3D world using the same character, table, house and farm.

During the story, movement, camera orbit, ordinary interactions, seasons and manual weather are temporarily controlled by the sequence. P pauses everything, R restarts the healthy world, Esc exits, Tab releases the mouse, and M toggles camera shake. This is a directed cinematic; it does not implement player death or free-roaming survival. The original restoration campaign remains available before activating the baton and after restarting.

## Story phases

| Phase | Behavior |
|---|---|
| Peaceful cup | Walk, turn, sit, pick up the cup, drink and replace it |
| Warning | Six seconds of darkening clouds, a warning caption and small tremors |
| Earthquake | Twelve seconds; stand, look around, cover the head, run away and look back toward the table |
| Lava | Twelve seconds; fissures widen, uneven lava rises, embers and small stones erupt |
| Meteors | Fourteen seconds; accelerating meteors with fire/smoke trails, explosions, impact lights, dust, debris, camera jolts and craters |
| Extreme storm | Fourteen seconds; heavy rain, faster wind/clouds, lightning, reduced visibility and flying debris |
| Ruin | Eight seconds; trees topple, wildlife leaves, flowers disappear, the river darkens and bridge planks drop |
| Last cup | Return along a collision-free path; sit, slowly drink, survey the damaged world, replace the cup and stand silently |
| Final collapse | Sixteen seconds of maximum combined effects; the character retreats again |
| Final silence | The storm subsides, the camera approaches the abandoned half-finished cup, steam dissipates and the image fades to darkness |

The final cup is a separate performance, not a static pose. Its drinking animation runs at 62% of the ordinary speed. The final collapse waits for the cup to be returned and the character to stand.

## Rendering and simulation

Disaster.h defines the phase machine and fixed pools. Disaster.inl implements the sequence, character direction, camera, effects and story overlays. All geometry uses the existing C++17 / compatibility OpenGL / GLU / FreeGLUT pipeline.

- Ground vertices and separate slabs move vertically; slabs rotate. Jagged dark fissures grow into animated glowing lava channels. Five eruption sites have irregular surfaces and rising jets.
- Meteor travel uses squared normalized time, producing acceleration toward the ground. A first impact is composed near the story camera; later positions are randomized within safe areas. Impact positions avoid the central tea/escape area, home and farm.
- At most 10 meteors, 480 physics particles and 12 crater records are stored. Particle and crater slots are recycled. Sparks and rock chips fall with gravity; dust and smoke drift and fade.
- The quake offsets the rendered camera without accumulating drift. M removes that offset. Ground displacement excludes the central route and home floor. Damage is simulated with transformations and overlays, not deletion of the world.
- Trees sway and progressively rotate down; wildlife moves away; flowers disappear with damage. The distant bridge deck partly collapses. Plaster fractures and fallen trim mark the house. Cows pause and raise their heads during the worst shaking.
- Existing rain, wind, cloud and lightning systems are driven by the story. OpenGL lights 2 and 3 provide lava and temporary impact lighting. Blended effects use depth testing with depth writes disabled.
- The character uses the existing navigation, leg animation and cup-hand attachment. It runs, shields its head, tracks active meteors, looks back at the table, and completes the slow final sip. The camera keeps the table prominent and moves inward for the ending.
- The narrative ends in visual silence. No audio track or synthesized thunder has been added; lightning and impact flashes provide the requested thunder-like visual feedback.

## Validation

The logic test runs the complete story through the actual baton interaction. It checks phase order, collision-free continuous movement, running and protective poses, the slow final sip, cup attachment and replacement, repeated impacts, full damage, fixed memory budgets, pause, reset, shake suppression, safe impact selection and meteor acceleration. It also confirms the original recovery objective counters are not changed by the disaster.

`dist/last_cup.exe --render-check screenshots` includes stages 29-39 for the disaster. These fixtures advance the actual sequence at fixed timesteps with a repeatable random seed; they do not directly set a disaster pose. `dist/last_cup.exe --disaster-benchmark` measures 240 animated frames during the final collapse. Ordinary play uses a fresh meteor seed on activation.
