# Null Relay (Native)

**Atmospheric 2D pixel-art survival / psychological horror**  
Written in pure **C + Raylib**. Compiles to a **single standalone Windows .exe** with zero runtime dependencies.

---

## Pre-built Downloads (Recommended)

Go to the **[Releases](https://github.com/siddop422/null-relay-native/releases)** page.

You will find:

| File | Description |
|------|-------------|
| **NullRelay.exe** | Portable, pre-compiled, single-file executable. Just download and double-click. |
| **NullRelay-Portable.zip** | Same .exe inside a zip (for people who prefer zip downloads). |

No installation. No extra DLLs. No Visual C++ redistributable. Fully static.

> The Releases are automatically created by GitHub Actions whenever a version tag is pushed (example: `v1.0.0`).

---

## How to get the pre-built .exe right now

1. Go to: https://github.com/siddop422/null-relay-native/actions
2. Click the latest successful workflow run
3. Download the artifact **NullRelay-Windows** (it contains both the .exe and the zip)

Or wait for a formal Release (I recommend creating a tag):

```bash
git tag v1.0.0
git push origin v1.0.0
```

This will automatically publish the .exe and zip to the Releases page.

---

## Controls

| Key              | Action                  |
|------------------|-------------------------|
| WASD / Arrows    | Move                    |
| Left Shift       | Sprint (uses stamina)   |
| F                | Toggle flashlight       |
| E / Space        | Interact (notes / gens) |
| Tab              | Open / close Journal    |
| Esc              | Pause / Quit on ending  |

---

## Multiple Endings

- **Escape** – Fix both generators and reach the surface lock
- **Truth** – Collect the important logs and go deep into containment
- **Trapped** – Let your sanity reach zero

---

## Build from source (if you want)

### Requirements
- Windows
- MinGW-w64 **or** MSVC
- Raylib (static preferred)

### Quick build with the included script
Edit the paths in `build.bat` then run it.

### Manual (MinGW example)
```bash
gcc -o NullRelay.exe src/main.c ^
    -I"path/to/raylib/include" ^
    -L"path/to/raylib/lib" ^
    -lraylib -lopengl32 -lgdi32 -lwinmm ^
    -O2 -s -mwindows -static
```

---

## Project Structure

```
null-relay-native/
├── src/
│   └── main.c          # Entire game (single file)
├── .github/workflows/
│   └── build.yml       # Automatic Windows .exe build + Release
├── build.bat
└── README.md
```

---

## License
MIT – free to use, modify, and distribute.
