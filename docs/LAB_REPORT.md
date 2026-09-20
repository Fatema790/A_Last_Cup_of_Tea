# A Last Cup of Tea — Lab Report

## 1. Title

**A Last Cup of Tea: A Campaign for Nature**  
Computer Graphics & Animation Lab — Interactive 3D Simulation

## 2. Introduction

This project puts the player inside a small natural valley. A warm cup of tea provides a quiet starting point. After exploring the surroundings, the player sees the effects of pollution and habitat loss, then takes practical actions to restore the area. The project uses interaction to connect graphics concepts with an environmental message.

## 3. Objectives

- Build a navigable 3D world from reusable procedural objects.
- Apply perspective projection, camera transformations, lighting and materials.
- Animate water, wildlife, steam, smoke, falling trees and plant growth.
- Detect collisions and nearby objects using simple mathematics.
- Make environmental damage and recovery visible through gradual changes.
- Complete a campaign through player actions rather than a fixed sequence of slides.

## 4. Tools / Technology

C++17 handles program logic. OpenGL renders the scene. GLU provides perspective, camera and cylinder/disk helpers. FreeGLUT manages the window, input and timer callbacks. CMake builds the project. Windows validation uses the portable MinGW-w64 compiler supplied by w64devkit. There are no external art assets or shaders.

## 5. System Description

The program separates simulation updates from drawing. `World` stores the current environment state, completed tasks, tree states and animation progress. The GLUT timer measures elapsed time and calls `update()`. The display callback renders the current state using a movable camera. Keyboard and mouse callbacks provide continuous movement and individual interactions.

The environment has five states: healthy, warning, damaged, recovering and restored. The warning starts a 24-second damage sequence. Six restoration actions determine the recovery target. A smoothly changing recovery value makes the visual response gradual. The campaign ends only after the player returns to the tea table at full recovery.

## 6. 3D Environment Design

The valley contains low-poly mountains, trees, flowers, grass, rocks, clouds and a sun. A river crosses the playable area, with a wooden bridge providing passage. A small house, table, chair and campfire form the central clearing. The tea cup uses an open cylinder wall, an inner wall, a lip, a torus handle, a saucer and a tea surface.

The ground is a triangle grid with gentle height variation. The x and z axes represent horizontal position, while y represents height. Object placement remains constant across environmental states so the player sees the same place change over time.

## 7. Interaction Design

The program measures the 3D distance between the human's interaction point and each available object. The nearest object within 3.15 units receives a ground highlight and an on-screen prompt. Pressing E observes the object or performs its action. Tasks become unavailable after completion. The camera follows the human and does not control interaction distance.

The first tea interaction is required before investigating the warning. After the damage sequence, the player can plant a tree, collect three litter piles, stop the factory and restore a wildlife refuge. The tasks can be completed in any order. The map shows active goals and the player's heading.

## 8. Animation Techniques

Sine functions create tree sway, wing movement and floating motion. Water vertices and surface streaks change with time. Steam and breath use soft transparent particles. Smoke uses expanding, fading particles. Falling trees shake first and then rotate around their bases before leaving stumps. Tree planting uses scaling to grow a sapling from the planting circle.

The human uses hierarchical limbs, distance-based walking, a breathing idle pose and two-link arm/leg calculations. The tea action routes the person to the chair, turns, sits, reaches for the cup, lifts it to the mouth, replaces it and returns to a seated pose. The cup and hand share one handle position. Smooth season and weather weights change vegetation, snowfall, petals, leaves, rain, wind, clouds and lighting. Rain and storms can send the character to a covered porch.

Environment progress interpolates sky colour, sunlight, fog, water colour and vegetation. Recovery reverses these effects, with staggered wildlife thresholds. Pause stops simulation time, and restart clears all campaign and animation state.

## 9. OpenGL Techniques Used

`gluPerspective()` creates perspective projection. `gluLookAt()` applies the camera view. Translation, rotation and scaling assemble each object. Matrix stacks isolate local transformations. Depth testing hides surfaces behind nearer geometry, while double buffering presents complete frames.

The sun is a directional light. Ambient, diffuse and specular properties provide surface shading. Transparent water, steam and smoke use alpha blending. Flattened translucent contact patches provide simple shadow-like depth cues. The HUD temporarily uses orthographic projection and disables scene lighting and depth testing.

## 10. Keyboard / Mouse Controls

| Control | Function |
|---|---|
| WASD | Move through the valley |
| Mouse after click | Orbit the following camera |
| E | Interact or request to leave the tea table |
| Q / C | Raise / lower the camera viewing angle |
| F1 / F2 / F3 / F4 | Spring / summer / autumn / winter |
| 1 / 2 / 3 / 4 / 5 / 6 | Clear / cloudy / rain / storm / snow / windy |
| T | Toggle automatic seasonal progression |
| P | Pause or resume |
| R | Restart |
| H | Toggle help |
| Tab | Release or capture the mouse |
| Escape | Exit |

## 11. Screenshots to Capture

The project includes twenty rendered reference images in `screenshots/`. These are real OpenGL captures of deterministic world states.

1. **Healthy valley:** bridge, river, mountains, wildlife and green vegetation.
2. **Damaged valley:** dry ground, tree stumps, dirty water, smoke and dark sky.
3. **Recovery:** growing tree, returning vegetation and partially completed task list.
4. **Restored valley:** clean river, green trees and full nature health.
5. **Final tea:** steaming cup and the concluding campaign message.

During a live presentation, also capture a falling tree mid-rotation and the interaction prompt beside the factory shutoff.

The added captures show spring walking, summer, autumn, winter, rain, storm lightning, snow in summer, wind, reaching for tea, lifting, sipping, returning the cup, shelter seating, turning and sitting. These demonstrate that weather and seasons affect the existing 3D world and human, not only the sky colour.

## 12. Expected Output

The executable opens a real-time 3D window. The player walks, looks around and interacts with objects. Damage develops in the existing scene. Completed restoration tasks gradually return the valley to health. The final visit to the tea table displays the campaign's closing message. There is no pre-rendered movie and no automatic task completion in normal play.

## 13. Conclusion

The project demonstrates how basic OpenGL techniques can create an understandable interactive world. The same transformation and animation methods build a peaceful environment, show its decline and make its recovery visible. The final cup of tea connects the player's small actions with the need to protect the natural world.


## Version 3: home and farm

The house now has a furnished living room, kitchen and dining space, bedroom and bathroom. A hinged front door opens on proximity from either side, with collision-aware motion. Individual wall and furniture footprints permit walking indoors; V selects first-person viewing. A fenced farm includes an open barn, four animated cows, hay and water troughs. See HOMESTEAD.md for controls and implementation, and VALIDATION.md for the current checks.


## Version 4: extreme disaster story

A new emergency baton starts a complete cinematic sequence in the existing world. A phase machine orchestrates the first tea, warning, earthquake, lava, meteors, extreme storm, ruined environment, slow final tea, maximum collapse and final fade. Terrain and slabs deform procedurally; particles, meteors and craters use fixed-size pools. The character follows collision-aware routes and existing cup/hand animation, with running, protective poses and meteor tracking. The original restoration campaign stays separate. See DISASTER.md for the stage timing, controls and implementation; VALIDATION.md records the complete automated story run, 39 OpenGL captures and final-collapse benchmark.
