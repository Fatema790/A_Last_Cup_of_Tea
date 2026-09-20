# Viva Questions & Answers

1. **What is OpenGL?**  
   OpenGL is a graphics API used to draw objects and images. This project uses it to render a 3D world.

2. **What is GLUT, and why use FreeGLUT?**  
   GLUT is a toolkit for windows and input. FreeGLUT is an open-source implementation that also supports key-release callbacks and leaving the main loop.

3. **Why is this project 3D?**  
   Objects have x, y and z coordinates. The camera can move around them, and depth and perspective affect their appearance.

4. **What is perspective projection?**  
   It makes distant objects appear smaller, similar to how we see the real world.

5. **Why use `gluPerspective()`?**  
   It creates the perspective viewing volume using a field of view, aspect ratio, near plane and far plane.

6. **What does `gluLookAt()` do?**  
   It creates a view from an eye position, a point to look toward and an up direction.

7. **How does the camera work?**  
   The human owns the player position. A camera follows behind it, using yaw and pitch to orbit smoothly around the character.

8. **What is a viewing transformation?**  
   It expresses world objects relative to the camera so the scene is drawn from the player's viewpoint.

9. **What is translation?**  
   Translation moves an object. `glTranslatef()` is used to place trees, furniture and other objects.

10. **What is rotation?**  
    Rotation turns an object around an axis. Falling trees rotate around their bases.

11. **What is scaling?**  
    Scaling changes an object's size. It creates different tree sizes and animates the growing sapling.

12. **What is hierarchical transformation?**  
    Child parts inherit their parent's transformations. A tree's leaves and trunk move together when the whole tree falls.

13. **Why use `glPushMatrix()`?**  
    It saves the current matrix before applying local transformations to an object.

14. **Why use `glPopMatrix()`?**  
    It restores the saved matrix so one object's transformations do not affect the next object.

15. **What is depth testing?**  
    It compares fragment depth with the depth buffer and keeps the visible surface closest to the camera.

16. **Why enable `GL_DEPTH_TEST`?**  
    It prevents distant objects from incorrectly drawing over nearby objects.

17. **What is double buffering?**  
    The program draws into a back buffer and then displays it by swapping buffers. This reduces flicker.

18. **Why use a GLUT timer?**  
    It schedules updates and requests new frames. We measure elapsed time because timer delivery is not perfectly regular.

19. **How does player movement work?**  
    Held keys define forward and sideways movement. Camera yaw rotates them into world coordinates, and collision checks move the human. The camera then follows.

20. **How does mouse look work?**  
    Horizontal mouse movement changes yaw. Vertical movement changes pitch, which is clamped to stop the camera flipping over.

21. **How does collision detection work?**  
    Simple boxes, circles and boundaries reject blocked positions. The x and z movement components are tested separately so the player can slide along obstacles.

22. **How does interaction detection work?**  
    The nearest available object within 3.15 units of the character's interaction point can be used by pressing E.

23. **How are trees modeled?**  
    A cylinder forms the trunk and scaled spheres form foliage. Small rotated cylinders form bare branches.

24. **How is the tea cup modeled?**  
    It has outer and inner cylinder walls, a ring-shaped lip, a torus handle, a saucer and a disk for tea.

25. **How is steam animated?**  
    Twenty transparent particles rise, drift and fade, then recycle. Their positions follow the cup even while the person drinks.

26. **How is smoke animated?**  
    Particles rise, drift, grow and fade with age. They are drawn from far to near for better transparency.

27. **How does tree falling work?**  
    A tree changes from normal to shaking, falling and removed. Its fall angle increases smoothly around its base, and a stump remains.

28. **How does pollution affect the environment?**  
    A damage value gradually changes the sky, fog, sunlight, river, grass, trees and wildlife.

29. **How does recovery work?**  
    Six completed actions set a target percentage. The visible recovery value moves toward that target over time.

30. **How is lighting implemented?**  
    `GL_LIGHTING` and `GL_LIGHT0` are enabled. A directional sun supplies ambient, diffuse and specular light.

31. **What is ambient light?**  
    Ambient light gives surfaces basic illumination even when they do not face the main light.

32. **What is diffuse light?**  
    Diffuse light depends on the surface direction relative to the light and helps show an object's shape.

33. **What is specular light?**  
    It produces a shiny highlight. The cup and water use material settings that can reflect a highlight.

34. **How is transparency implemented?**  
    Alpha blending combines a surface with the colour behind it. Transparent passes keep depth testing but disable depth writes.

35. **How is the HUD created?**  
    Rectangles and bitmap text are drawn in screen coordinates after the 3D world.

36. **Why use orthographic projection for the HUD?**  
    It keeps interface elements the same size and position regardless of camera movement or scene depth.

37. **How does river animation work?**  
    Small sine-based vertex offsets create waves, and moving bright line segments suggest flowing highlights.

38. **How does tree planting work?**  
    An interaction enables a seed and sapling. A growth value increases and scales the sapling toward its full size.

39. **How does the player complete the campaign?**  
    Enjoy the first tea, investigate the warning, complete all restoration actions, wait for full recovery, and return to the tea table.

40. **What is the main message?**  
    A peaceful moment with nature should never become our last one. Our actions help protect future moments.

41. **Why separate updating from rendering?**  
    Updating changes the simulation, while rendering displays it. This makes timing, pause and testing easier to manage.

42. **Why use `GL_NORMALIZE`?**  
    Scaling can change normal lengths. Normalization keeps lighting calculations correct for scaled objects.

43. **What happens when the window is resized?**  
    The viewport and projection aspect ratio are updated so the scene does not stretch.

44. **Are the shadows and reflections physically accurate?**  
    No. Shadows are flattened transparent patches and river highlights are animated geometry. These simple methods suit a basic graphics lab.

45. **How is the project tested?**  
    Deterministic tests check interactions, state changes, pause, reset and collisions. Separate OpenGL captures check rendered states and graphics errors.

46. **How is the human modeled?**  
    Cylinders, spheres and cuboids form the body, face, hair, clothes and limbs. Each part uses local transformations.

47. **Why use a hierarchy for the limbs?**  
    The forearm follows the upper arm and the lower leg follows the thigh. This keeps joints connected as they rotate.

48. **How does walking avoid sliding through the world?**  
    Walking phase advances with actual distance. During the stance phase, the foot moves backward relative to the body as the body moves forward. Collision stops the gait.

49. **What is inverse kinematics in this project?**  
    It calculates two joint angles from a desired hand or foot position. The arm reaches the cup handle and the legs place the feet.

50. **How does the cup stay in the hand?**  
    The cup and the gripping hand use the same handle position throughout the lift and sip.

51. **How does the character find shelter?**  
    A breadth-first search finds a path through free ground cells. The character follows the path using normal collision checks.

52. **How do season changes remain smooth?**  
    Four season weights gradually move toward the selected season. Colours and environmental features blend using those weights.

53. **Are weather and environmental damage the same state?**  
    No. They are independent. A healthy valley can have a storm, and choosing clear weather does not remove pollution damage.

54. **Why recycle particles?**  
    Reusing fixed arrays keeps memory and drawing work bounded. A particle returns to a starting position when its life ends.

55. **How is lightning created?**  
    A short flash temporarily brightens the sky and light. A jagged line shows the bolt, and a cooldown prevents frequent flashes.
