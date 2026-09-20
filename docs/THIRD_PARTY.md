# Third-party components

The project uses FreeGLUT 3.6.0 under its permissive license. The complete notice is included as `dist/FreeGLUT-LICENSE.txt`.

The Windows executable is built using GCC / MinGW-w64 from w64devkit. MinGW-w64 runtime notices are included as `dist/COPYING.MinGW-w64-runtime.txt`. GCC runtime libraries use the [GCC Runtime Library Exception](https://www.gnu.org/licenses/gcc-exception-3.1.html).

OpenGL and GLU are supplied by the operating system / graphics driver. CMake and the compiler are build tools; they are not bundled in the project ZIP. The local `.tools` directory is retained only to support rebuilding on this machine.

Sources:

- FreeGLUT: https://github.com/freeglut/freeglut/tree/v3.6.0
- w64devkit: https://github.com/skeeto/w64devkit/releases/tag/v2.10.0
- CMake: https://github.com/Kitware/CMake/releases/tag/v3.31.6
