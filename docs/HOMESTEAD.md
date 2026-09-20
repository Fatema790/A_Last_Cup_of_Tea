# Home and cow farm - version 3

Run Play.cmd (or dist/last_cup.exe). The house is northwest of the starting tea table, marked H on the minimap. Follow the stepping stones to its porch. Approach the central door: it swings inward automatically, remains open while you are near either side, then closes after you leave. If you stand in its swing path, it waits for clearance.

Walk through with WASD. Press V for first-person viewing, click to enable mouse look, and use Tab to release the pointer. V returns to the following camera. The original tea animation keeps its dedicated camera.

## Furnished rooms

- Living room: cushioned sofa, striped rug, coffee table, books, mugs, bookshelf, framed landscape, floor lamp and plant.
- Kitchen and dining area: cupboards, counters, sink and tap, stove, kettle, fridge, dining table, chairs and fruit bowl.
- Bedroom: bed, quilt, pillows, wardrobe, bedside table, lamp and framed art.
- Bathroom: tiled floor, bath, toilet, basin and decorative mirror panel.

Use E near the living-room floor lamp to switch the warm home lighting. The furniture is decorative except for the lighting interaction. Walls, major furniture and doors have collision. Two permanent internal doorways lead from the main room into the bedroom and bathroom. No jump is needed to enter the home.

## Cow farm

The farm is east of the tea table, marked C on the minimap. Its west fence has an open pedestrian gate. Inside are four cows, including a smaller calf, an open-front barn, hay bales, feed and water troughs. Cows wander within bounded grazing areas, move their legs with travel, lower their heads, chew and swish their tails. They stop moving when the character comes close, and their bodies block walking through them.

Press E near the western feed trough to put out fresh hay. Feeding and lighting do not change the six restoration objectives. Seasons and weather still affect the landscape and roofs. Rain and snow are omitted beneath covered spaces.

## Implementation

Homestead.h stores shared dimensions, furniture footprints and four cow states. Homestead.inl builds the house from individual walls and openings. The rendered door and collision segment share a hinge, width and angle. Door motion checks the character before advancing. Camera collision uses walls, door, furniture and ceiling; first person gives a direct indoor view.

Automated checks cover entry and exit, door proximity, delayed closing, all four room routes, furniture and wall collisions, first-person eye placement, indoor rain shelter, lamp and feed interactions, bounded cow movement, pause and reset. Native render fixtures 21-28 show the exterior, doorway, rooms and farm. These are procedural, stylized OpenGL models; the mirror and water are decorative surfaces, not real-time reflections.
