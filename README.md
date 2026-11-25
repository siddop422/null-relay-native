# Null Relay (Native)

Atmospheric 2D pixel-art survival / psychological horror game written in **pure C + Raylib**.

No Python. No Pygame. No runtime. Compiles to a **single standalone Windows .exe**.

## Why this version?
- Zero installation for the player
- Zero external DLLs (when statically linked)
- Native performance
- Easy to compile with MinGW or MSVC

## Controls
- **WASD / Arrows** – Move
- **Left Shift** – Sprint
- **F** – Toggle flashlight
- **E / Space** – Interact (notes / generators)
- **Tab** – Journal
- **Esc** – Pause / Quit

## How to build (Windows)

### Option A – Easiest (Raylib pre-built)
1. Download Raylib for Windows: https://www.raylib.com/
2. Or use the provided `build.bat` after installing Raylib via vcpkg / scoop / manual.

### Option B – Recommended single .exe (static)
```bash
# Using MinGW-w64 + static Raylib
gcc -o NullRelay.exe src/main.c -Ipath/to/raylib/include -Lpath/to/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm -static -O2 -s
```

A ready `build.bat` is included.

## Run
Just double-click `NullRelay.exe`. No install. No dependencies.

## Features kept from the original design
- Procedural-feeling pixel art (drawn with Raylib primitives + simple sprites)
- Flashlight + battery
- Stamina + Sanity system with visual tension
- Horror entity with state machine (Patrol → Stalk → Chase)
- Notes / environmental storytelling
- Multiple endings (Escape / Truth / Trapped)
- 320×180 internal resolution, nearest-neighbor scaled

## License
MIT
