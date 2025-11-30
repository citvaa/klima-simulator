# AC Simulator

A tiny 2D OpenGL demo of an AC remote and unit.

- Toggle the lamp to power the AC on or off.
- Adjust the desired temperature with the arrow panel or keyboard keys.
- Vent animates; status icon reflects temps; water fills over time and Space drains it.
- Cursor is always a built-in procedural design.

Build & Run:
- Install dependencies: OpenGL, GLFW, GLEW, and FreeType (e.g., `vcpkg install freetype` or add a NuGet FreeType package). The code expects FreeType headers/libs to be on the include/lib path.
- By default the text renderer loads `C:\Windows\Fonts\arial.ttf`; change the font path in `Source/TextRenderer.cpp` if desired.
- Open ac-simulator.sln in Visual Studio (x64), build, and run. The window resizes and centers the scene automatically.
