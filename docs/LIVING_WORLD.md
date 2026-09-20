# Living World — Human, Seasons and Weather

This update extends the original valley. It keeps the tea table, house, bridge, river, trees, factory, observations, destruction sequence and six recovery actions. Version 3 expands the house into a furnished home and relocates the shelter bench to its larger porch. See HOMESTEAD.md.

## Try the additions

Run `Play.cmd`. You start behind the protagonist. Click to enable mouse orbit and use WASD to walk. Walk to the tea table and press E to watch the full performance. Press F3 for autumn, F4 for winter, and 4 for a storm. WASD overrides the character's automatic walk to shelter.

| Keys | Result |
|---|---|
| F1 | Spring: fresh foliage, flowers, butterflies and drifting petals |
| F2 | Summer: warmer sunlight, clear water and active wildlife |
| F3 | Autumn: orange foliage, drier grass, wind and falling leaves |
| F4 | Winter: snow-covered ground and crowns, cold light, snow and breath |
| 1–6 | Clear, cloudy, rain, storm, snow, windy |
| T | Toggle automatic seasons every 90 seconds |
| E | Interact or request to leave tea |
| Q / C | Adjust camera height without lifting the person off the ground |

## Human model

The person is built entirely from OpenGL geometry. Cylinders make the shirt, arms and legs. Scaled spheres form the head, ears, eyes, hair, hands and shoes. Clothing has its own colour and collar geometry. Matrix push/pop calls isolate the body, head and limb transforms.

The character's world position owns movement, collision and interaction distance. The camera follows that position and eases around the scene. It shortens its distance before entering buildings or tree crowns. The player marker on the map follows the person, not the camera.

Walking advances according to distance travelled. A collision stops both movement and the gait. Arms swing in opposite phase to the legs. A two-link leg calculation keeps the stance foot near the ground while the other foot rises and swings forward. On stopping, the gait eases to neutral; breathing and small head motion remain active. Snow reduces walking speed by up to 23%.

## Tea performance

The action state progresses through approach, turn, sit, drink, rest and stand. Approach routing uses the same collision map as manual movement, including the river and bridge. The character can start on either side of the table and walk around it to the chair.

The chair and character move together during sitting. Knees bend, the upper body leans toward the table, and the camera eases to a view of the face and cup. The drinking stage lasts 9.4 seconds: reach, grip, lift, sip, lower and release. The first tea objective is awarded after the cup returns to the table. The restored-world ending also waits for the completed performance.

The cup is rendered once. A shared cup transform determines both its position and its handle position; the right hand reaches that handle through a two-bone arm calculation. The saucer remains on the table. The cup is sized to fit the human model. Steam follows the moving cup continuously. During storms, the left hand supports the cup and the sipping tilt is smaller.

Pressing E during a sip queues a safe exit: return the cup, then stand. Rain selected during tea also waits for this safe return before the character walks to shelter. Reset restores the cup, character and chair immediately as part of restarting the whole simulation.

## Atmosphere

Seasons and weather use smoothly interpolated weights rather than replacing the scene. The weights affect grass, tree foliage, snow caps, flowers, wildlife, sunlight, sky, fog, clouds and river colour. Environmental damage remains a separate factor, so changing to spring does not repair a damaged valley.

Wind sways the upper parts of trees, bends grass, carries petals and leaves, drifts precipitation, moves clouds, pushes smoke and bends steam. Trunks remain rooted. Cloud density, speed and darkness increase in storms. Rain darkens the ground gradually and speeds river waves. Winter clouds and lighting are cooler. Cold breath is emitted near the mouth; steam is denser and slower in cold weather.

The character briefly looks up and raises a hand when rain starts, then walks faster to the porch and sits under its roof. Precipitation below the porch roof is omitted, so it does not visibly pass through the sheltered character. Manual input takes priority over the shelter walk. Clearing the weather lets the character stand again.

## Particle budgets

| System | Maximum | Geometry |
|---|---:|---|
| Rain | 500 | Short line segments |
| Snow | 400 | Rotating camera-facing quads |
| Leaves / petals | 100 | Rotating triangles |
| Factory smoke | 28 | Low-resolution transparent spheres |
| Tea steam | 20 | Soft transparent camera-facing puffs |
| Cold breath | 12 | Soft transparent camera-facing puffs |

Arrays are allocated once and recycled. Particles reset after reaching the ground or the end of their lifetime. No external model files, textures, shader framework or audio dependency is required. Pause freezes simulation time, particle motion, weather transitions and the character animation. Restart resets all of them.

## Source guide

`main.cpp` retains the original environment and campaign. It includes `LivingWorld.h` for new data and function declarations, and `LivingWorld.inl` for implementation. This keeps one compilation unit while separating the new code into readable sections. Keep these files and Homestead.h / Homestead.inl and Disaster.h / Disaster.inl together when copying the source to another computer.

The main functions to explain during a viva are `updateHuman()`, `planWalk()`, `updateFollowCamera()`, `animateSit()`, `animateDrink()`, `drawHuman()`, `drawArm()`, `drawLeg()` and `updateAtmosphere()`.
