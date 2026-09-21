Dawn Signal tutorial prototype

Uses the existing C++ / OpenGL / GLEW / freeglut solution.
Controls: WASD move, F interact, R restart, Escape exit.
Leave the overgrown lobby, speak to the caretaker, collect both supply bundles,
then return to the caretaker. The two shops can be visited in either order.

Build the solution with the installed v145 C++ toolset and Windows SDK.
The project copies Shaders and the matching runtime DLLs beside the executable.
OpenGL 3.3 is required. No additional asset downloads are needed.

See ../outputs/ for the Korean implementation guide and level design.
This implementation has been statically reviewed, not built or run by Codex.
